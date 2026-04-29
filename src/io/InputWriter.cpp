#include "InputWriter.h"
#include "Model.h"
#include "Part.h"
#include "Material.h"
#include "Geom.h"
#include "BoundaryCondition.h"
#include "Step.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>

#include <nlohmann/json.hpp>
#include "json_io.h"

using json = nlohmann::json;

namespace {
namespace fs = std::filesystem;

std::string materialIdFromIndex(int index) {
  if (index == 0)
    return "Solid";
  return "Material_" + std::to_string(index);
}

std::string modelStem(Model *model) {
  if (model == nullptr)
    return "model";

  if (model->getHasName()) {
    fs::path name_path(model->getName());
    std::string stem = name_path.stem().string();
    if (!stem.empty())
      return stem;
    std::string filename = name_path.filename().string();
    if (!filename.empty())
      return filename;
  }

  return "model";
}

std::string domTypeFromAnalysis(Model *model) {
  if (model == nullptr)
    return "3D";

  switch (model->getAnalysisType()) {
    case PlaneStress2D:
      return "PlaneStress";
    case PlaneStrain2D:
      return "plStrain";
    case Axisymmetric2D:
      return "AxiSymm";
    case Solid3D:
    default:
      return "3D";
  }
}

Step *activeStep(Model *model) {
  if (model == nullptr || model->getStepCount() == 0)
    return nullptr;
  return model->getStep(0);
}

json makeImplicitSolverJson(const Step *step) {
  json implicit;
  implicit["type"] = step ? step->m_implicitType : "Picard";
  implicit["velTol"] = step ? step->m_velTol : 5e-2;
  implicit["pressTol"] = step ? step->m_pressTol : 10.0;
  implicit["forceTol"] = step ? step->m_forceTol : 10.0;
  implicit["divTol"] = step ? step->m_divTol : 1.0;
  implicit["omegaV"] = step ? step->m_omegaV : 0.4;
  implicit["omegaP"] = step ? step->m_omegaP : 0.1;
  implicit["maxIter"] = step ? step->m_maxIter : 200;
  implicit["timeStepGrowthFactor"] = step ? step->m_timeStepGrowthFactor : 1.2;
  return implicit;
}

Part *findPartById(Model *model, int part_id) {
  if (model == nullptr)
    return nullptr;

  for (int i = 0; i < model->getPartCount(); ++i) {
    Part *part = model->getPart(i);
    if (part != nullptr && part->getId() == part_id)
      return part;
  }

  return nullptr;
}

bool appendRigidBoundaryCondition(json &bc_array, Model *model, BoundaryCondition *bc) {
  if (bc == nullptr)
    return false;

  if (bc->getApplyTo() != ApplyToPart) {
    std::cerr << "ERROR: BoundaryCondition export only supports ApplyToPart. TargetId="
              << bc->getTargetId() << std::endl;
    return false;
  }

  Part *part = findPartById(model, bc->getTargetId());
  if (part == nullptr) {
    std::cerr << "ERROR: BoundaryCondition target part not found. TargetId="
              << bc->getTargetId() << std::endl;
    return false;
  }

  if (part->getType() != Rigid) {
    std::cerr << "ERROR: BoundaryCondition on deformable part is not allowed in exported input. PartId="
              << part->getId() << std::endl;
    return false;
  }

  json jbc;
  jbc["zoneId"] = part->getId();

  if (bc->getType() == DisplacementBC) {
    jbc["valueType"] = 1;
    jbc["amplitudeId"] = 1;
    jbc["amplitudeFactor"] = 1.0;
  } else {
    jbc["valueType"] = 0;
  }

  double3 v = bc->getValue();
  jbc["value"] = {v.x, v.y, v.z};
  bc_array.push_back(jbc);
  return true;
}
}

InputWriter::InputWriter(Model*m) : m_model(m) {}

void InputWriter::writeToFile(std::string fname) {
  namespace fs = std::filesystem;

  if (m_model == nullptr) {
    std::cout << "ERROR: null model in InputWriter" << std::endl;
    return;
  }

  Step *step = activeStep(m_model);
  if (step != nullptr && step->isImplicit()) {
    writeImplicitToFile(fname);
    return;
  }

  json m_json;
  fs::path json_path(fname);
  fs::path json_dir = json_path.has_parent_path() ? json_path.parent_path() : fs::path(".");
  fs::create_directories(json_dir);
  std::string base_name = modelStem(m_model);

  std::ofstream o(fname);
  if (!o.is_open()) {
    std::cout << "ERROR: could not open output file " << fname << std::endl;
    return;
  }

  m_json["Configuration"]["cflFactor"] = step ? step->m_cflFactor : 0.3;
  m_json["Configuration"]["fixedTS"] = step ? step->m_fixedTS : false;
  m_json["Configuration"]["solver"] = "WeldForm";
  m_json["Configuration"]["simTime"] = step ? step->m_simTime : 1.0;
  m_json["Configuration"]["outTime"] = step ? step->m_outTime : 1.0e-4;
  m_json["Configuration"]["domType"] = domTypeFromAnalysis(m_model);
  if (m_model->m_thermal_coupling)
    m_json["Configuration"]["thermal"] = true;

  const ContactProperties &contact = m_model->contactProps();
  json cont;
  cont["fricCoeffStatic"] = contact.fricCoeffStatic;
  cont["gapPenaltyScale"] = contact.gapPenaltyScale;
  cont["heatCondCoeff"] = contact.heatCondCoeff;
  cont["heatConductance"] = contact.heatConductance;
  cont["maxAccel"] = contact.maxAccel;
  cont["maxPenetRatio"] = contact.maxPenetRatio;
  cont["penaltyFactor"] = contact.penaltyFactor;
  cont["useGapPenalty"] = contact.useGapPenalty;
  m_json["Contact"] = json::array();
  m_json["Contact"].push_back(cont);

  if (m_model->getMaterialCount() > 0) {
    m_json["Materials"] = json::array();

    for (int i = 0; i < m_model->getMaterialCount(); ++i) {
      auto *mat = m_model->getMaterial(i);
      if (mat == nullptr)
        continue;

      json mat_json;
      mat_json["id"] = materialIdFromIndex(i);
      mat_json["density0"] = mat->getDensityConstant();
      mat_json["poissonsRatio"] = mat->Elastic().nu();
      mat_json["youngsModulus"] = mat->Elastic().E();
      mat_json["thermalHeatCap"] = mat->cp_T;
      mat_json["thermalCond"] = mat->k_T;

      if (!mat->isPlastic()) {
        mat_json["type"] = "Elastic";
      } else {
        switch (mat->m_plastic->Material_model) {
          case _GMT_:
            mat_json["type"] = "GMT";
            break;
          case HOLLOMON:
            mat_json["type"] = "Hollomon";
            break;
          default:
            mat_json["type"] = "UnknownPlastic";
            break;
        }

        std::vector<double> plasticConst = mat->m_plastic->getPlasticConstants();
        if (!plasticConst.empty())
          mat_json["const"] = plasticConst;
      }

      m_json["Materials"].push_back(mat_json);
    }
  }

  bool is_elastic = false;
  for (std::vector<Part*>::iterator it = m_model->m_part.begin(); it != m_model->m_part.end(); ++it) {
    Part* part = *it;
    if (part == nullptr || !part->isMeshed())
      continue;

    fs::path mesh_path = json_dir / (base_name + "_part_" + std::to_string(part->getId()) + ".bdf");
    part->getMesh()->exportToNASTRAN(mesh_path.string());

    if (part->getType() == Elastic) {
      if (is_elastic) {
        std::cout << "ERROR: More than one elastic body not supported in current explicit exporter." << std::endl;
        continue;
      }
      json block;
      block["type"] = "File";
      block["fileName"] = fs::relative(mesh_path, json_dir).string();
      block["scale"] = {1, 1, 1};
      m_json["DomainBlocks"].push_back(block);
      is_elastic = true;
    } else {
      json rigidBody;
      rigidBody["type"] = "File";
      rigidBody["zoneId"] = part->getId();
      rigidBody["start"] = {0.0, 0.0, 0.0};
      rigidBody["scale"] = {1, 1, 1};
      rigidBody["orientNormals"] = true;
      rigidBody["fileName"] = fs::relative(mesh_path, json_dir).string();
      m_json["RigidBodies"].push_back(rigidBody);

      json bc;
      bc["zoneId"] = part->getId();
      bc["valueType"] = 0;
      bc["value"] = {part->getVel().x, part->getVel().y, part->getVel().z};
      m_json["BoundaryConditions"].push_back(bc);
    }
  }

  bool xSymm = false;
  bool ySymm = false;
  bool zSymm = false;
  m_json["BoundaryConditions"] = json::array();
  if (m_model->getBCCount() > 0) {
    for (int i = 0; i < m_model->getBCCount(); ++i) {
      BoundaryCondition* bc = m_model->getBC(i);
      if (!bc)
        continue;

      if (bc->getType() == SymmetryBC) {
        double3 n = bc->getNormal();
        if (std::fabs(n.x) > 0.5) xSymm = true;
        if (std::fabs(n.y) > 0.5) ySymm = true;
        if (std::fabs(n.z) > 0.5) zSymm = true;

        m_json["Configuration"]["xSymm"] = xSymm;
        m_json["Configuration"]["ySymm"] = ySymm;
        m_json["Configuration"]["zSymm"] = zSymm;
      } else {
        appendRigidBoundaryCondition(m_json["BoundaryConditions"], m_model, bc);
      }
    }
  }

  o << std::setw(4) << m_json << std::endl;
}

void InputWriter::writeImplicitToFile(std::string fname) {
  namespace fs = std::filesystem;

  if (m_model == nullptr) {
    std::cout << "ERROR: null model in InputWriter" << std::endl;
    return;
  }

  Step *step = activeStep(m_model);

  json m_json;
  fs::path json_path(fname);
  fs::path json_dir = json_path.has_parent_path() ? json_path.parent_path() : fs::path(".");
  fs::create_directories(json_dir);
  std::string base_name = modelStem(m_model);

  std::ofstream o(fname);
  if (!o.is_open()) {
    std::cout << "ERROR: could not open output file " << fname << std::endl;
    return;
  }

  m_json["Configuration"]["Nproc"] = step ? step->m_nproc : 1;
  m_json["Configuration"]["cflFactor"] = step ? step->m_cflFactor : 0.3;
  m_json["Configuration"]["autoTS"] = {
    step ? step->m_autoTS[0] : false,
    step ? step->m_autoTS[1] : false,
    step ? step->m_autoTS[2] : false
  };
  m_json["Configuration"]["kernelGradCorr"] = step ? step->m_kernelGradCorr : false;
  m_json["Configuration"]["simTime"] = step ? step->m_simTime : 200.0;
  m_json["Configuration"]["artifViscAlpha"] = step ? step->m_artifViscAlpha : 1.0;
  m_json["Configuration"]["artifViscBeta"] = step ? step->m_artifViscBeta : 0.0;
  m_json["Configuration"]["outTime"] = step ? step->m_outTime : 1.0;
  m_json["Configuration"]["fixedTS"] = step ? step->m_fixedTS : false;
  m_json["Configuration"]["domType"] = domTypeFromAnalysis(m_model);
  m_json["Configuration"]["AxiSymmVol"] = step ? step->m_axiSymmVol : false;
  m_json["Configuration"]["elemLentghFraction"] = step ? step->m_elemLengthFraction : 0.2;
  m_json["Configuration"]["solver"]["implicit"] = makeImplicitSolverJson(step);

  m_json["Meshing"]["debug"] = step ? step->m_meshingDebug : true;
  m_json["Meshing"]["maxElemAngle"] = step ? step->m_maxElemAngle : 150.0;
  m_json["Meshing"]["minElemAngle"] = step ? step->m_minElemAngle : 30.0;

  const ContactProperties &contact = m_model->contactProps();
  m_json["Contact"] = json::array();
  m_json["Contact"].push_back({
    {"fricCoeffStatic", contact.fricCoeffStatic},
    {"gapPenaltyScale", contact.gapPenaltyScale},
    {"heatCondCoeff", contact.heatCondCoeff},
    {"heatConductance", contact.heatConductance},
    {"maxAccel", contact.maxAccel},
    {"maxPenetRatio", contact.maxPenetRatio},
    {"penaltyFactor", contact.penaltyFactor},
    {"useGapPenalty", contact.useGapPenalty}
  });

  m_json["Amplitudes"] = json::array();
  m_json["BoundaryConditions"] = json::array();
  m_json["Materials"] = json::array();
  m_json["DomainBlocks"] = json::array();
  m_json["RigidBodies"] = json::array();

  if (m_model->getMaterialCount() > 0) {
    for (int i = 0; i < m_model->getMaterialCount(); ++i) {
      auto *mat = m_model->getMaterial(i);
      if (mat == nullptr)
        continue;

      json mat_json;
      mat_json["id"] = materialIdFromIndex(i);
      mat_json["density0"] = mat->getDensityConstant();
      mat_json["thermalHeatCap"] = mat->cp_T;
      mat_json["thermalCond"] = mat->k_T;
      mat_json["youngsModulus"] = mat->Elastic().E();
      mat_json["poissonsRatio"] = mat->Elastic().nu();
      mat_json["yieldStress0"] = mat->yieldStress0;
      if (mat->strRange.size() >= 2)
        mat_json["strRange"] = {mat->strRange[0], mat->strRange[1]};
      else
        mat_json["strRange"] = {0.0, 0.65};

      if (!mat->isPlastic()) {
        mat_json["type"] = "Elastic";
      } else {
        switch (mat->m_plastic->Material_model) {
          case HOLLOMON:
            mat_json["type"] = "Hollomon";
            break;
          case _GMT_:
            mat_json["type"] = "GMT";
            break;
          case BILINEAR:
            mat_json["type"] = "Bilinear";
            break;
          default:
            mat_json["type"] = "UnknownPlastic";
            break;
        }

        std::vector<double> plasticConst = mat->m_plastic->getPlasticConstants();
        if (!plasticConst.empty())
          mat_json["const"] = plasticConst;
      }

      m_json["Materials"].push_back(mat_json);
    }
  }

  int amplitude_id = 1;
  for (std::vector<Part*>::iterator it = m_model->m_part.begin(); it != m_model->m_part.end(); ++it) {
    Part* part = *it;
    if (part == nullptr || !part->isMeshed())
      continue;

    fs::path mesh_path = json_dir / (base_name + "_part_" + std::to_string(part->getId()) + ".bdf");
    part->getMesh()->exportToNASTRAN(mesh_path.string());
    std::string relative_mesh = fs::relative(mesh_path, json_dir).string();

    if (part->getType() == Elastic) {
      m_json["DomainBlocks"].push_back({
        {"type", "File"},
        {"fileName", relative_mesh},
        {"scale", {1, 1, 1}}
      });
    } else {
      m_json["RigidBodies"].push_back({
        {"type", "File"},
        {"fileName", relative_mesh},
        {"zoneId", part->getId()},
        // The exported BDF already contains the current transformed node coordinates.
        // Using the geometry origin here would apply the same translation twice.
        {"start", {0.0, 0.0, 0.0}},
        {"orientNormals", true}
      });
    }
  }

  for (int i = 0; i < m_model->getBCCount(); ++i) {
    BoundaryCondition* bc = m_model->getBC(i);
    if (!bc)
      continue;
    if (bc->getType() == SymmetryBC)
      continue;

    appendRigidBoundaryCondition(m_json["BoundaryConditions"], m_model, bc);
  }

  if (m_json["BoundaryConditions"].empty()) {
    for (std::vector<Part*>::iterator it = m_model->m_part.begin(); it != m_model->m_part.end(); ++it) {
      Part* part = *it;
      if (part == nullptr || part->getType() != Rigid)
        continue;

      m_json["BoundaryConditions"].push_back({
        {"zoneId", part->getId()},
        {"valueType", 0},
        {"value", {part->getVel().x, part->getVel().y, part->getVel().z}}
      });
    }
  }

  o << std::setw(4) << m_json << std::endl;
}
