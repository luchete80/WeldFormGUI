#include "InputWriter.h"
#include "Model.h"
#include "Part.h"
#include "Material.h"
#include "Geom.h"
#include "BoundaryCondition.h"
#include "InitialCondition.h"
#include "Node.h"
#include "Step.h"

#include <cmath>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

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

void appendSymmetryPlanesToConfiguration(json &configuration, Model *model)
{
  if (model == nullptr)
    return;

  bool xSymm = false;
  bool ySymm = false;
  bool zSymm = false;

  for (const SymmetryPlane &plane : model->symmetryPlanes()) {
    if (!plane.enabled)
      continue;

    if (plane.axis == 0) {
      xSymm = true;
      configuration["xSymmPlane"] = plane.value;
    } else if (plane.axis == 1) {
      ySymm = true;
      configuration["ySymmPlane"] = plane.value;
    } else if (plane.axis == 2) {
      zSymm = true;
      configuration["zSymmPlane"] = plane.value;
    }
  }

  if (xSymm) configuration["xSymm"] = true;
  if (ySymm) configuration["ySymm"] = true;
  if (zSymm) configuration["zSymm"] = true;
}

Step *activeStep(Model *model) {
  if (model == nullptr || model->getStepCount() == 0)
    return nullptr;
  return model->getStep(0);
}

std::string normalizeImplicitSolverType(const std::string& type) {
  if (type.empty())
    return "picard";

  std::string normalized = type;
  for (char& c : normalized)
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

  if (normalized == "picard" || normalized == "hybrid" ||
      normalized == "newton" || normalized == "nr") {
    return normalized;
  }

  return "picard";
}

json makeImplicitSolverJson(const Step *step) {
  json implicit;
  implicit["velTol"] = step ? step->m_velTol : 5e-2;
  implicit["pressTol"] = step ? step->m_pressTol : 10.0;
  implicit["forceTol"] = step ? step->m_forceTol : 10.0;
  implicit["divTol"] = step ? step->m_divTol : 1.0;
  implicit["omegaV"] = step ? step->m_omegaV : 0.4;
  implicit["omegaP"] = step ? step->m_omegaP : 0.1;
  implicit["maxIter"] = step ? step->m_maxIter : 200;
  implicit["timeStepGrowthFactor"] = step ? step->m_timeStepGrowthFactor : 1.2;
  implicit["useSprings"] = step ? step->m_useWeakSprings : false;
  implicit["springFactor"] = step ? step->m_springFactor : 1.0e-7;
  implicit["springStiffness"] = step ? step->m_springStiffness : 0.0;
  implicit["springMode"] = step ? step->m_springMode : 1;
  return implicit;
}

int exportedDimensionCount(Model *model) {
  if (model == nullptr)
    return 3;

  switch (model->getAnalysisType()) {
    case PlaneStress2D:
    case PlaneStrain2D:
    case Axisymmetric2D:
      return 2;
    case Solid3D:
    default:
      return 3;
  }
}

void appendNodeSets(json &root, Model *model) {
  if (model == nullptr)
    return;

  root["Sets"] = json::array();

  for (int part_index = 0; part_index < model->getPartCount(); ++part_index) {
    Part *part = model->getPart(part_index);
    if (part == nullptr || part->getMesh() == nullptr)
      continue;

    Mesh *mesh = part->getMesh();
    for (int set_index = 0; set_index < mesh->getNodeSetCount(); ++set_index) {
      const NodeSet &node_set = mesh->getNodeSet(set_index);

      json jset;
      jset["id"] = node_set.getId();
      jset["nodes"] = json::array();

      for (int node_index = 0; node_index < node_set.getItemCount(); ++node_index) {
        const Node *node = node_set.getItem(node_index);
        if (node != nullptr)
          jset["nodes"].push_back(node->getId());
      }

      root["Sets"].push_back(jset);
    }
  }
}

bool appendAmplitudeForCondition(json &amplitude_array, const Condition *condition, int amplitude_id) {
  if (condition == nullptr || !condition->usesAmplitude())
    return false;

  const std::vector<double> &timeValues = condition->getAmplitudeTime();
  const std::vector<double> &amplitudeValues = condition->getAmplitudeValue();
  if (timeValues.size() < 2 || timeValues.size() != amplitudeValues.size())
    return false;

  amplitude_array.push_back({
    {"id", amplitude_id},
    {"valueType", 1},
    {"time", timeValues},
    {"value", amplitudeValues}
  });
  return true;
}

void appendDirectionalBoundaryCondition(json &bc_array,
                                        json &amplitude_array,
                                        Model *model,
                                        BoundaryCondition *bc,
                                        int export_id,
                                        int &next_amplitude_id) {
  if (bc == nullptr || bc->getType() == SymmetryBC)
    return;

  json jbc;
  jbc["id"] = export_id;

  if (bc->getApplyTo() == ApplyToNodeSet) {
    jbc["setId"] = bc->getTargetId();
  } else {
    jbc["zoneId"] = bc->getTargetId();
  }

  jbc["valueType"] = static_cast<int>(bc->getValueType());
  if (bc->usesAmplitude()) {
    if (appendAmplitudeForCondition(amplitude_array, bc, next_amplitude_id)) {
      jbc["amplitudeId"] = next_amplitude_id++;
      jbc["amplitudeFactor"] = bc->getAmplitudeFactor();
    } else {
      jbc["valueType"] = static_cast<int>(ConstantValue);
    }
  }

  const double3 value = bc->getValue();
  jbc["value"] = {value.x, value.y, value.z};

  const int dim_count = exportedDimensionCount(model);
  std::vector<int> directions;
  if (bc->getDofMaskX())
    directions.push_back(0);
  if (bc->getDofMaskY())
    directions.push_back(1);
  if (dim_count == 3 && bc->getDofMaskZ())
    directions.push_back(2);

  const bool all_default_directions =
    (dim_count == 2 && directions.size() == 2) ||
    (dim_count == 3 && directions.size() == 3);

  if (!all_default_directions) {
    if (directions.size() == 1)
      jbc["direction"] = directions.front();
    else
      jbc["direction"] = directions;
  }

  bc_array.push_back(jbc);
}

void appendInitialCondition(json &ic_array, InitialCondition *ic, int export_id) {
  if (ic == nullptr)
    return;

  json jic;
  jic["id"] = export_id;
  if (ic->getApplyTo() == ApplyToNodeSet)
    jic["setId"] = ic->getTargetId();
  else
    jic["zoneId"] = ic->getTargetId();

  const double3 value = ic->getValue();
  if (ic->getType() == TempIC) {
    jic["type"] = "Temperature";
    jic["value"] = value.x;
  } else {
    jic["type"] = "Velocity";
    jic["value"] = {value.x, value.y, value.z};
    jic["dofMask"] = {ic->getDofMaskX(), ic->getDofMaskY(), ic->getDofMaskZ()};
  }

  ic_array.push_back(jic);
}

std::vector<double> defaultStrainRange(const Material_ *mat) {
  if (mat != nullptr && mat->strRange.size() >= 2)
    return {mat->strRange[0], mat->strRange[1]};
  return {0.0, 0.65};
}

std::vector<double> defaultStrainRateRange(const Material_ *mat) {
  if (mat == nullptr)
    return {0.0, 1.0e10};
  return {mat->er_min, mat->er_max > mat->er_min ? mat->er_max : 1.0e10};
}

std::vector<double> defaultTemperatureRange(const Material_ *mat) {
  if (mat == nullptr)
    return {0.0, 1.0e10};
  return {mat->T_min, mat->T_max > mat->T_min ? mat->T_max : 1.0e10};
}

std::vector<double> buildEnginePlasticConstants(const Material_ *mat) {
  if (mat == nullptr || !mat->isPlastic() || mat->m_plastic == nullptr)
    return {};

  switch (mat->m_plastic->Material_model) {
    case BILINEAR:
      return {mat->m_plastic->Et};
    case HOLLOMON:
      return {mat->K, mat->m};
    case JOHNSON_COOK:
      return {mat->B, mat->n, mat->C, mat->eps_0, mat->m, mat->T_m, mat->T_t};
    case _GMT_:
      return {mat->n1, mat->n2, mat->C1, mat->C2, mat->m1, mat->m2, mat->I1, mat->I2};
    case NORTON_HOFF:
      return {mat->K_nh, mat->m_nh};
    default:
      return mat->m_plastic->getPlasticConstants();
  }
}

std::string buildEngineMaterialType(const Material_ *mat) {
  if (mat != nullptr && mat->tabulated_enabled)
    return "Tabulated";

  if (mat == nullptr || !mat->isPlastic() || mat->m_plastic == nullptr)
    return "Elastic";

  switch (mat->m_plastic->Material_model) {
    case BILINEAR:
      return "Bilinear";
    case HOLLOMON:
      return "Hollomon";
    case JOHNSON_COOK:
      return "JohnsonCook";
    case _GMT_:
      return "GMT";
    case NORTON_HOFF:
      return "NortonHoff";
    case TABULATED:
      return "Tabulated";
    default:
      return "UnknownPlastic";
  }
}

json buildEngineMaterialJson(const Material_ *mat, int index) {
  json mat_json;
  mat_json["id"] = materialIdFromIndex(index);
  if (mat == nullptr)
    return mat_json;

  mat_json["density0"] = mat->getDensityConstant();
  mat_json["youngsModulus"] = mat->Elastic().E();
  mat_json["poissonsRatio"] = mat->Elastic().nu();
  mat_json["yieldStress0"] = mat->yieldStress0;
  mat_json["thermalHeatCap"] = mat->cp_T;
  mat_json["thermalCond"] = mat->k_T;
  mat_json["thermalExp"] = mat->exp_T;
  mat_json["strRange"] = defaultStrainRange(mat);
  mat_json["strdotRange"] = defaultStrainRateRange(mat);
  mat_json["tempRange"] = defaultTemperatureRange(mat);
  mat_json["type"] = buildEngineMaterialType(mat);

  if (mat->isPlastic() && mat->m_plastic != nullptr) {
    std::vector<double> plastic_const = buildEnginePlasticConstants(mat);
    if (!plastic_const.empty())
      mat_json["const"] = plastic_const;
  }

  if (mat->tabulated_enabled &&
      !mat->tabulatedStrainGrid.empty() &&
      !mat->tabulatedRateGrid.empty() &&
      !mat->tabulatedTemperatureGrid.empty() &&
      !mat->tabulatedStressValues.empty()) {
    if (mat->tabulated_export_csv_reference && !mat->tableCsvPath.empty()) {
      mat_json["flowStressCsv"] = mat->tableCsvPath;
    } else {
      mat_json["flowStressTable"]["strainGrid"] = mat->tabulatedStrainGrid;
      mat_json["flowStressTable"]["strainRateGrid"] = mat->tabulatedRateGrid;
      mat_json["flowStressTable"]["temperatureGrid"] = mat->tabulatedTemperatureGrid;
      mat_json["flowStressTable"]["stressValues"] = mat->tabulatedStressValues;
    }
  }

  return mat_json;
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
  appendSymmetryPlanesToConfiguration(m_json["Configuration"], m_model);

  const RemeshingSettings &remeshing = m_model->remeshing();
  m_json["Meshing"]["enabled"] = remeshing.enabled;
  m_json["Meshing"]["minStrain"] = remeshing.minStrain;
  m_json["Meshing"]["maxStrain"] = remeshing.maxStrain;
  m_json["Meshing"]["mapVel"] = remeshing.mapVel;
  m_json["Meshing"]["mapAcc"] = remeshing.mapAcc;
  m_json["Meshing"]["maxCount"] = remeshing.maxCount;
  m_json["Meshing"]["dampFactor"] = remeshing.dampFactor;
  m_json["Meshing"]["minFrac"] = remeshing.minFrac;
  m_json["Meshing"]["maxFrac"] = remeshing.maxFrac;
  m_json["Meshing"]["epsRef"] = remeshing.epsRef;
  m_json["Meshing"]["beta"] = remeshing.beta;
  m_json["Meshing"]["type"] = remeshing.type;
  m_json["Meshing"]["debug"] = remeshing.debug;
  m_json["Meshing"]["minElemAngle"] = remeshing.minElemAngle;
  m_json["Meshing"]["maxElemAngle"] = remeshing.maxElemAngle;
  m_json["Meshing"]["transitionAngle"] = remeshing.transitionAngle;

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
  appendNodeSets(m_json, m_model);

  if (m_model->getMaterialCount() > 0) {
    m_json["Materials"] = json::array();

    for (int i = 0; i < m_model->getMaterialCount(); ++i) {
      auto *mat = m_model->getMaterial(i);
      if (mat == nullptr)
        continue;
      m_json["Materials"].push_back(buildEngineMaterialJson(mat, i));
    }
  }

  bool is_elastic = false;
  m_json["BoundaryConditions"] = json::array();
  m_json["InitialConditions"] = json::array();
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
    }
  }

  bool xSymm = false;
  bool ySymm = false;
  bool zSymm = false;
  int amplitude_id = 1;
  int bc_export_id = 1;
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
        appendDirectionalBoundaryCondition(m_json["BoundaryConditions"],
                                           m_json["Amplitudes"],
                                           m_model,
                                           bc,
                                           bc_export_id++,
                                           amplitude_id);
      }
    }
  }

  if (m_json["BoundaryConditions"].empty()) {
    for (std::vector<Part*>::iterator it = m_model->m_part.begin(); it != m_model->m_part.end(); ++it) {
      Part* part = *it;
      if (part == nullptr || part->getType() != Rigid)
        continue;

      m_json["BoundaryConditions"].push_back({
        {"id", bc_export_id++},
        {"zoneId", part->getId()},
        {"valueType", 0},
        {"value", {part->getVel().x, part->getVel().y, part->getVel().z}}
      });
    }
  }

  int ic_export_id = 1;
  for (int i = 0; i < m_model->getICCount(); ++i) {
    appendInitialCondition(m_json["InitialConditions"], m_model->getIC(i), ic_export_id++);
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
  if (m_model->m_thermal_coupling)
    m_json["Configuration"]["thermal"] = true;
  if (step != nullptr && step->isImplicit()) {
    m_json["Configuration"]["solver"]["implicit"] = makeImplicitSolverJson(step);
    if (m_model->getAnalysisType() != Solid3D) {
      m_json["Configuration"]["solver"]["type"] =
          normalizeImplicitSolverType(step->m_implicitType);
    }
  }
  appendSymmetryPlanesToConfiguration(m_json["Configuration"], m_model);

  const RemeshingSettings &remeshing = m_model->remeshing();
  m_json["Meshing"]["enabled"] = remeshing.enabled;
  m_json["Meshing"]["minStrain"] = remeshing.minStrain;
  m_json["Meshing"]["maxStrain"] = remeshing.maxStrain;
  m_json["Meshing"]["mapVel"] = remeshing.mapVel;
  m_json["Meshing"]["mapAcc"] = remeshing.mapAcc;
  m_json["Meshing"]["maxCount"] = remeshing.maxCount;
  m_json["Meshing"]["dampFactor"] = remeshing.dampFactor;
  m_json["Meshing"]["minFrac"] = remeshing.minFrac;
  m_json["Meshing"]["maxFrac"] = remeshing.maxFrac;
  m_json["Meshing"]["epsRef"] = remeshing.epsRef;
  m_json["Meshing"]["beta"] = remeshing.beta;
  m_json["Meshing"]["type"] = remeshing.type;
  m_json["Meshing"]["debug"] = remeshing.debug;
  m_json["Meshing"]["minElemAngle"] = remeshing.minElemAngle;
  m_json["Meshing"]["maxElemAngle"] = remeshing.maxElemAngle;
  m_json["Meshing"]["transitionAngle"] = remeshing.transitionAngle;

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
  appendNodeSets(m_json, m_model);

  m_json["Amplitudes"] = json::array();
  m_json["BoundaryConditions"] = json::array();
  m_json["InitialConditions"] = json::array();
  m_json["Materials"] = json::array();
  m_json["DomainBlocks"] = json::array();
  m_json["RigidBodies"] = json::array();

  if (m_model->getMaterialCount() > 0) {
    for (int i = 0; i < m_model->getMaterialCount(); ++i) {
      auto *mat = m_model->getMaterial(i);
      if (mat == nullptr)
        continue;
      m_json["Materials"].push_back(buildEngineMaterialJson(mat, i));
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

  int bc_export_id = 1;
  for (int i = 0; i < m_model->getBCCount(); ++i) {
    BoundaryCondition* bc = m_model->getBC(i);
    if (!bc)
      continue;
    if (bc->getType() == SymmetryBC)
      continue;

    appendDirectionalBoundaryCondition(m_json["BoundaryConditions"],
                                       m_json["Amplitudes"],
                                       m_model,
                                       bc,
                                       bc_export_id++,
                                       amplitude_id);
  }

  if (m_json["BoundaryConditions"].empty()) {
    for (std::vector<Part*>::iterator it = m_model->m_part.begin(); it != m_model->m_part.end(); ++it) {
      Part* part = *it;
      if (part == nullptr || part->getType() != Rigid)
        continue;

      m_json["BoundaryConditions"].push_back({
        {"id", bc_export_id++},
        {"zoneId", part->getId()},
        {"valueType", 0},
        {"value", {part->getVel().x, part->getVel().y, part->getVel().z}}
      });
    }
  }

  int ic_export_id = 1;
  for (int i = 0; i < m_model->getICCount(); ++i) {
    appendInitialCondition(m_json["InitialConditions"], m_model->getIC(i), ic_export_id++);
  }

  o << std::setw(4) << m_json << std::endl;
}
