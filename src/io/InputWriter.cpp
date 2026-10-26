#include "InputWriter.h"
#include "Model.h"
#include "Part.h"
#include "Material.h"
#include "Geom.h"
#include "BoundaryCondition.h"
#include "InitialCondition.h"
#include "Node.h"
#include "Step.h"
#include "InitialStateMaterialPolicy.h"
#include "ExportGeneration.h"
#include "InitialStatePieceExporter.h"

#include <cmath>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <vector>

#include <nlohmann/json.hpp>
#include "json_io.h"

using json = nlohmann::json;

namespace {
namespace fs = std::filesystem;

Step* activeStep(Model* model);

bool validateInitialStateHistoryForInput(Model* model, bool implicitWriter)
{
  if (model == nullptr) return false;
  int historyPartCount = 0;
  int deformablePartCount = 0;
  for (int i = 0; i < model->getPartCount(); ++i) {
    Part* part = model->getPart(i);
    if (part == nullptr || !part->isMeshed() || part->getMesh() == nullptr) continue;
    if (part->getType() == Elastic) ++deformablePartCount;
    part->getMesh()->validateInitialStateTopology();
    if (!part->getMesh()->hasInitialPlasticStrainHistory()) continue;
    ++historyPartCount;
    if (implicitWriter) {
      Step* step = activeStep(model);
      if (model->getAnalysisType() != Solid3D || step == nullptr || !step->isImplicit() ||
          !step->usesRigidViscoplasticImplicitSolver()) {
        std::cerr << "ERROR: initial pl_strain sidecars require the 3D MINI3D rigid-viscoplastic implicit solver; explicit and J2 workflows are unsupported."
                  << std::endl;
        return false;
      }
    } else {
      std::cerr << "ERROR: initial pl_strain sidecars are not supported by the explicit input writer; use 3D MINI3D implicit."
                << std::endl;
      return false;
    }
    if (part->getType() != Elastic) {
      std::cerr << "ERROR: initial pl_strain history requires a deformable part." << std::endl;
      return false;
    }
    const std::string issue = InitialStateHistoryExportIssue(*model, *part);
    if (!issue.empty()) {
      std::cerr << "ERROR: " << issue << " (part " << part->getId() << ")." << std::endl;
      return false;
    }
  }
  if (historyPartCount == 0) return true;
  if (historyPartCount != 1 || deformablePartCount != 1) {
    std::cerr << "ERROR: MINI3D initial pl_strain input currently supports exactly one meshed deformable part."
              << std::endl;
    return false;
  }
  return true;
}

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

bool validateThermalConfiguration(Model *model, const Step *step) {
  if (model == nullptr || !model->m_thermal_coupling)
    return true;

  const double fraction = model->getPlasticHeatFraction();
  if (!std::isfinite(fraction) || fraction < 0.0 || fraction > 1.0) {
    std::cerr << "ERROR: plHeatFrac must be finite and within [0,1]." << std::endl;
    return false;
  }

  for (int i = 0; i < model->getMaterialCount(); ++i) {
    const Material_* material = model->getMaterial(i);
    if (material == nullptr)
      continue;
    const double density = material->getDensityConstant();
    if (!std::isfinite(density) || density <= 0.0) {
      std::cerr << "ERROR: thermal material " << i << " requires density0 > 0." << std::endl;
      return false;
    }
    if (!std::isfinite(material->cp_T) || material->cp_T <= 0.0) {
      std::cerr << "ERROR: thermal material " << i << " requires thermalHeatCap > 0." << std::endl;
      return false;
    }
    if (!std::isfinite(material->k_T) || material->k_T < 0.0) {
      std::cerr << "ERROR: thermal material " << i << " requires thermalCond >= 0." << std::endl;
      return false;
    }
  }

  bool has_temperature = false;
  for (int i = 0; i < model->getICCount(); ++i) {
    if (model->getIC(i) != nullptr && model->getIC(i)->getType() == TempIC) {
      has_temperature = true;
      break;
    }
  }
  if (!has_temperature)
    std::cerr << "WARNING: thermal coupling has no initial temperature; the engine will use 20 C." << std::endl;

  if (step != nullptr && step->isImplicit() && model->getAnalysisType() == Solid3D)
    std::cerr << "WARNING: implicit 3D thermal export assumes deformable elements are TET4; topology validation is unavailable in the GUI." << std::endl;

  return true;
}

std::string normalizeImplicitSolverType(const std::string& type, ImplicitFormulation formulation) {
  if (formulation == ImplicitFormulation::J2Elastoplastic)
    return "j2";

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

json makeImplicitSolverJson(const Step *step, bool is3D) {
  json implicit;
  const ImplicitFormulation formulation =
      step ? step->m_implicitFormulation : ImplicitFormulation::RigidViscoplastic;
  implicit["formulation"] = implicitFormulationToConfigString(formulation);
  implicit["type"] = normalizeImplicitSolverType(step ? step->m_implicitType : "", formulation);
  implicit["velTol"] = step ? step->m_velTol : 5e-2;
  if (is3D)
    implicit["bubbleVelTol"] = step ? step->m_bubbleVelTol : 5e-2;
  implicit["pressTol"] = step ? step->m_pressTol : 10.0;
  implicit["forceTol"] = step ? step->m_forceTol : 10.0;
  implicit["divTol"] = step ? step->m_divTol : 1.0;
  implicit["omegaV"] = step ? step->m_omegaV : 0.4;
  implicit["omegaP"] = step ? step->m_omegaP : 0.1;
  implicit["maxIter"] = step ? step->m_maxIter : 200;
  implicit["timeStepGrowthFactor"] = step ? step->m_timeStepGrowthFactor : 1.2;
  implicit["initialTimeStep"] = step ? step->m_initialTimeStep : 0.0;
  implicit["maxStepRetries"] = step ? step->m_maxStepRetries : 8;
  implicit["nonConvergenceCutbackFactor"] = step ? step->m_nonConvergenceCutbackFactor : 0.5;
  implicit["useSprings"] = step ? step->m_useWeakSprings : false;
  implicit["springFactor"] = step ? step->m_springFactor : 1.0e-7;
  implicit["springStiffness"] = step ? step->m_springStiffness : 0.0;
  implicit["springMode"] = step ? step->m_springMode : 1;
  implicit["rigidModeStabilization"] = step ? step->m_rigidModeStabilization : false;
  implicit["rigidModeStabilizationFactor"] = step ? step->m_rigidModeStabilizationFactor : 1.0e-3;
  implicit["rigidModeContactFade"] = step ? step->m_rigidModeContactFade : true;
  implicit["rigidModeContactFadeScale"] = step ? step->m_rigidModeContactFadeScale : 1.0;
  implicit["adaptiveDtLimiter"] = step ? step->m_adaptiveDtLimiter : false;
  implicit["adaptiveDtMin"] = step ? step->m_adaptiveDtMin : 1.0e-7;
  implicit["maxNodalDisplacementPerStep"] = step ? step->m_maxNodalDisplacementPerStep : 0.0005;
  implicit["maxEffectiveStrainIncrementPerStep"] = step ? step->m_maxEffectiveStrainIncrementPerStep : 0.02;
  implicit["picardUseFixedPointResidual"] = step ? step->m_picardUseFixedPointResidual : false;
  implicit["picardStagnationEnabled"] = step ? step->m_picardStagnationEnabled : false;
  implicit["picardStagnationWindow"] = step ? step->m_picardStagnationWindow : 10;
  implicit["picardStagnationMinIter"] = step ? step->m_picardStagnationMinIter : 30;
  implicit["picardStagnationMinImprovement"] = step ? step->m_picardStagnationMinImprovement : 0.05;
  implicit["picardStagnationMaxWindows"] = step ? step->m_picardStagnationMaxWindows : 2;
  implicit["picardUseAitken"] = step ? step->m_picardUseAitken : false;
  implicit["picardAitkenMinOmega"] = step ? step->m_picardAitkenMinOmega : 0.05;
  implicit["picardAitkenMaxOmega"] = step ? step->m_picardAitkenMaxOmega : 0.8;
  implicit["picardUseAitkenBubble"] = step ? step->m_picardUseAitkenBubble : false;
  implicit["omegaBubble"] = step ? step->m_omegaBubble : 0.4;
  implicit["picardAitkenBubbleMinOmega"] = step ? step->m_picardAitkenBubbleMinOmega : 0.05;
  implicit["picardAitkenBubbleMaxOmega"] = step ? step->m_picardAitkenBubbleMaxOmega : 0.6;
  if (step && !step->m_picardAcceleration.empty())
    implicit["picardAcceleration"] = step->m_picardAcceleration;
  implicit["picardAndersonDepth"] = step ? step->m_picardAndersonDepth : 3;
  implicit["picardAndersonConditionLimit"] = step ? step->m_picardAndersonConditionLimit : 1.0e8;
  implicit["picardAndersonCoefficientLimit"] = step ? step->m_picardAndersonCoefficientLimit : 10.0;
  implicit["picardAndersonStepFactorLimit"] = step ? step->m_picardAndersonStepFactorLimit : 2.0;
  implicit["picardAndersonRejectFactor"] = step ? step->m_picardAndersonRejectFactor : 1.25;
  implicit["picardAndersonMaxRecoveries"] = step ? step->m_picardAndersonMaxRecoveries : 1;
  implicit["picardAndersonRecoveryIterations"] = step ? step->m_picardAndersonRecoveryIterations : 10;
  implicit["picardSmoothViscosity"] = step ? step->m_picardSmoothViscosity : false;
  implicit["picardStrainRateRegularization"] = step ? step->m_picardStrainRateRegularization : 0.001;
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

bool appendDirectionalBoundaryCondition(json &bc_array,
                                        json &amplitude_array,
                                        Model *model,
                                        BoundaryCondition *bc,
                                        int export_id,
                                        int &next_amplitude_id) {
  if (bc == nullptr || bc->getType() == SymmetryBC)
    return true;

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

  if (bc->hasAngularMotion()) {
    std::string angularError;
    Part *target = nullptr;
    if (bc->getApplyTo() == ApplyToPart) {
      for (int i = 0; i < model->getPartCount(); ++i) {
        Part *candidate = model->getPart(i);
        if (candidate != nullptr && candidate->getId() == bc->getTargetId()) {
          target = candidate;
          break;
        }
      }
    }
    Step *step = activeStep(model);
    if (model == nullptr || model->getAnalysisType() != Solid3D || target == nullptr ||
        target->getType() != Rigid || !target->isMeshed() || step == nullptr || !step->isImplicit() ||
        !step->usesRigidViscoplasticImplicitSolver()) {
      return false;
    }
    if (!bc->isAngularMotionValid(&angularError)) {
      std::cerr << "ERROR: invalid angular BC: " << angularError << std::endl;
      return false;
    }

    const double3 angular = bc->getAngularVelocity();
    const double3 center = bc->getRotationCenter();
    jbc["valueAng"] = {angular.x, angular.y, angular.z};
    jbc["rotationCenter"] = {center.x, center.y, center.z};
    jbc["angularValueType"] = static_cast<int>(bc->getAngularValueType());
    if (bc->usesAngularAmplitude()) {
      const int angularId = next_amplitude_id++;
      amplitude_array.push_back({{"id", angularId}, {"valueType", 1},
                                 {"time", bc->getAngularAmplitudeTime()},
                                 {"value", bc->getAngularAmplitudeValue()}});
      jbc["angularAmplitudeId"] = angularId;
      jbc["angularAmplitudeFactor"] = bc->getAngularAmplitudeFactor();
    }
  }

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
  return true;
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

bool InputWriter::writeToFile(std::string fname) {
  if (m_model != nullptr) {
    bool hasAngularMotion = false;
    for (int i = 0; i < m_model->getBCCount(); ++i) {
      BoundaryCondition *bc = m_model->getBC(i);
      hasAngularMotion = hasAngularMotion || (bc != nullptr && bc->hasAngularMotion());
    }
    if (hasAngularMotion) {
      Step *step = activeStep(m_model);
      if (m_model->getAnalysisType() != Solid3D || step == nullptr || !step->isImplicit() ||
          !step->usesRigidViscoplasticImplicitSolver()) {
        std::cerr << "ERROR: angular BC export requires 3D MINI3D rigid-viscoplastic implicit mode." << std::endl;
        return false;
      }
    }
  }
  namespace fs = std::filesystem;

  if (m_model == nullptr) {
    std::cerr << "ERROR: null model in InputWriter" << std::endl;
    return false;
  }
  Step *step = activeStep(m_model);
  if (step != nullptr && step->isImplicit())
    return writeImplicitToFile(fname);
  if (!validateInitialStateHistoryForInput(m_model, false)) return false;
  if (!isContactActivationRampConfigurationValid(m_model->contactProps())) {
    std::cerr << "ERROR: contactActivationRampWidth must be finite and greater than zero when contactActivationRamp is enabled." << std::endl;
    return false;
  }
  if ((step == nullptr || !step->isImplicit()) && !validateThermalConfiguration(m_model, step))
    return false;

  json m_json;
  fs::path json_path(fname);
  fs::path json_dir = json_path.has_parent_path() ? json_path.parent_path() : fs::path(".");
  std::string base_name = modelStem(m_model);
  ExportGenerationDirectory generation;
  std::string generationError;
  if (!generation.create(json_dir, base_name, &generationError)) {
    std::cerr << "ERROR: " << generationError << std::endl;
    return false;
  }

  m_json["Configuration"]["cflFactor"] = step ? step->m_cflFactor : 0.3;
  m_json["Configuration"]["fixedTS"] = step ? step->m_fixedTS : false;
  m_json["Configuration"]["solver"] = "WeldForm";
  m_json["Configuration"]["simTime"] = step ? step->m_simTime : 1.0;
  m_json["Configuration"]["outTime"] = step ? step->m_outTime : 1.0e-4;
  m_json["Configuration"]["artifViscCoeffs"] = {
    step ? step->m_artifViscAlpha : 1.0,
    step ? step->m_artifViscBeta : 0.0,
    0.0
  };
  m_json["Configuration"]["domType"] = domTypeFromAnalysis(m_model);
  m_json["Configuration"]["plHeatFrac"] = m_model->getPlasticHeatFraction();
  if (m_model->m_thermal_coupling)
    m_json["Configuration"]["thermal"] = true;
  appendSymmetryPlanesToConfiguration(m_json["Configuration"], m_model);

  m_json["Stabilization"] = {
    {"alpha_free", step ? step->m_stabAlphaFree : 0.0},
    {"alpha_contact", step ? step->m_stabAlphaContact : 0.0},
    {"hg_coeff_free", step ? step->m_stabHgCoeffFree : 0.0},
    {"hg_coeff_contact", step ? step->m_stabHgCoeffContact : 0.0},
    {"av_coeff_div", step ? step->m_stabAvCoeffDiv : 0.0},
    {"av_coeff_bulk", step ? step->m_stabAvCoeffBulk : 0.0},
    {"log_factor", step ? step->m_stabLogFactor : 0.0},
    {"pspg_scale", step ? step->m_stabPspgScale : 0.0},
    {"p_pspg_bulkfac", step ? step->m_stabPspgBulkFactor : 0.0},
    {"J_min", step ? step->m_stabJMin : 0.0},
    {"hg_visc", step ? step->m_stabHgVisc : 0.1},
    {"hg_stiff", step ? step->m_stabHgStiff : 0.1}
  };

  const RemeshingSettings &remeshing = m_model->remeshing();
  m_json["Meshing"]["enabled"] = remeshing.enabled;
  m_json["Meshing"]["stepInterval"] = remeshing.stepInterval;
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
  m_json["Meshing"]["hausdorffTolerance"] = remeshing.hausdorffTolerance;
  m_json["Meshing"]["type"] = remeshing.type;
  m_json["Meshing"]["refineOnlyBoundary"] = remeshing.refineOnlyBoundary;
  m_json["Meshing"]["boundaryLayers"] = remeshing.boundaryLayers;
  m_json["Meshing"]["debug"] = remeshing.debug;
  m_json["Meshing"]["minElemAngle"] = remeshing.minElemAngle;
  m_json["Meshing"]["maxElemAngle"] = remeshing.maxElemAngle;
  m_json["Meshing"]["transitionAngle"] = remeshing.transitionAngle;

  const ContactProperties &contact = m_model->contactProps();
  json cont;
  cont["auto"] = contact.autoPenalty;
  cont["contactNormalMode"] = contact.unilateralContact ? "unilateral" : "legacy";
  cont["autoFactor"] = contact.autoFactor;
  cont["contactActivationRamp"] = contact.contactActivationRamp;
  cont["contactActivationRampWidth"] = contact.contactActivationRampWidth;
  cont["diagnosticLevel"] = contact.diagnosticLevel;
  cont["unilateralDiagnosticLevel"] = contact.unilateralDiagnosticLevel;
  cont["unilateralDiagnosticTrackedPoints"] = contact.unilateralDiagnosticTrackedPoints;
  cont["frictionModel"] = contact.frictionModel;
  cont["fricCoeffStatic"] = contact.fricCoeffStatic;
  if (contact.frictionModel == "coulomb_tresca")
    cont["coulombCoefficient"] = contact.coulombCoefficient;
  cont["frictionRegularizationVelocity"] = contact.frictionRegularizationVelocity;
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

  for (std::size_t materialIndex = 0; materialIndex < m_json["Materials"].size(); ++materialIndex) {
    json& materialJson = m_json["Materials"][materialIndex];
    if (!materialJson.contains("flowStressCsv")) continue;
    std::string relativeCsv;
    if (!PackageExternalCsv(materialJson["flowStressCsv"].get<std::string>(),
                            static_cast<int>(materialIndex), generation.path(), json_path,
                            relativeCsv, &generationError)) {
      std::cerr << "ERROR: " << generationError << std::endl;
      return false;
    }
    materialJson["flowStressCsv"] = relativeCsv;
  }

  bool is_elastic = false;
  m_json["BoundaryConditions"] = json::array();
  m_json["InitialConditions"] = json::array();
  for (std::vector<Part*>::iterator it = m_model->m_part.begin(); it != m_model->m_part.end(); ++it) {
    Part* part = *it;
    if (part == nullptr || !part->isMeshed())
      continue;

    fs::path mesh_path = generation.path() /
        (base_name + "_part_" + std::to_string(part->getId()) + ".bdf");
    if (!part->getMesh()->exportToNASTRAN(mesh_path.string())) {
      std::cerr << "ERROR: failed to export BDF generation resource: " << mesh_path << std::endl;
      return false;
    }
    std::string relativeMesh;
    if (!ExportGenerationRelativePath(mesh_path, json_path, relativeMesh, &generationError)) {
      std::cerr << "ERROR: " << generationError << std::endl;
      return false;
    }

    if (part->getType() == Elastic) {
      if (is_elastic) {
        std::cout << "ERROR: More than one elastic body not supported in current explicit exporter." << std::endl;
        continue;
      }
      json block;
      block["type"] = "File";
      block["fileName"] = relativeMesh;
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
      rigidBody["flipNormals"] = part->getFlipNormals();
      if (!part->getNormalOrientation().empty())
        rigidBody["normalOrientation"] = part->getNormalOrientation();
      if (part->getNormalOrientation() == "towardPoint" ||
          part->getNormalOrientation() == "awayFromPoint") {
        const double3 orientPoint = part->getOrientPoint();
        rigidBody["orientPoint"] = {orientPoint.x, orientPoint.y, orientPoint.z};
      }
      rigidBody["fileName"] = relativeMesh;
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
        if (!appendDirectionalBoundaryCondition(m_json["BoundaryConditions"],
                                           m_json["Amplitudes"],
                                           m_model,
                                           bc,
                                           bc_export_id++,
                                           amplitude_id)) return false;
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

  const fs::path temporaryInputPath = json_dir /
      (json_path.filename().string() + ".tmp." + generation.name());
  if (!WriteJsonAndPublish(m_json, temporaryInputPath, json_path, &generationError)) {
    std::cerr << "ERROR: " << generationError << std::endl;
    return false;
  }
  generation.retain();
  return true;
}

bool InputWriter::writeImplicitToFile(std::string fname) {
  namespace fs = std::filesystem;

  if (m_model == nullptr) {
    std::cerr << "ERROR: null model in InputWriter" << std::endl;
    return false;
  }
  if (!validateInitialStateHistoryForInput(m_model, true)) return false;
  if (!isContactActivationRampConfigurationValid(m_model->contactProps())) {
    std::cerr << "ERROR: contactActivationRampWidth must be finite and greater than zero when contactActivationRamp is enabled." << std::endl;
    return false;
  }
  Step *step = activeStep(m_model);
  if (!validateThermalConfiguration(m_model, step))
    return false;

  std::map<int, int> angularTargets;
  bool hasAngular = false;
  for (int i = 0; i < m_model->getBCCount(); ++i) {
    BoundaryCondition *bc = m_model->getBC(i);
    hasAngular = hasAngular || (bc != nullptr && bc->hasAngularMotion());
  }
  if (hasAngular) {
    std::set<int> rigidZoneIds;
    for (int p = 0; p < m_model->getPartCount(); ++p) {
      Part *part = m_model->getPart(p);
      if (part != nullptr && part->getType() == Rigid) rigidZoneIds.insert(part->getId());
    }
    for (int p = 0; p < m_model->getPartCount(); ++p) {
      Part *part = m_model->getPart(p);
      if (part == nullptr || part->getMesh() == nullptr) continue;
      Mesh *mesh = part->getMesh();
      for (int s = 0; s < mesh->getNodeSetCount(); ++s) {
        const int setId = mesh->getNodeSet(s).getId();
        if (rigidZoneIds.count(setId) != 0) {
          std::cerr << "ERROR: NodeSet ID " << setId
                    << " aliases a rigid tool zone in angular-motion input." << std::endl;
          return false;
        }
      }
    }
  }
  for (int i = 0; i < m_model->getBCCount(); ++i) {
    BoundaryCondition *bc = m_model->getBC(i);
    if (bc == nullptr) continue;
    if (bc->hasAngularMotion() && bc->getType() != VelocityBC) {
      std::cerr << "ERROR: angular motion is supported only on Velocity boundary conditions." << std::endl;
      return false;
    }
    if (bc->getType() != SymmetryBC && bc->getApplyTo() == ApplyToPart) {
      Part *candidateTarget = nullptr;
      for (int p = 0; p < m_model->getPartCount(); ++p) {
        Part *candidate = m_model->getPart(p);
        if (candidate != nullptr && candidate->getId() == bc->getTargetId()) candidateTarget = candidate;
      }
      if (candidateTarget != nullptr && candidateTarget->getType() == Rigid && hasAngular &&
          bc->getType() != VelocityBC) {
        std::cerr << "ERROR: angular tool motion cannot be combined with non-Velocity BCs on zone "
                  << bc->getTargetId() << "; solver input export cannot preserve that BC type." << std::endl;
        return false;
      }
      if (candidateTarget != nullptr && candidateTarget->getType() == Rigid && hasAngular &&
          bc->getType() == VelocityBC) {
        const double3 value = bc->getValue();
        if (!std::isfinite(value.x) || !std::isfinite(value.y) || !std::isfinite(value.z)) {
          std::cerr << "ERROR: rigid tool velocity components must be finite in angular-motion input." << std::endl;
          return false;
        }
      }
      if (candidateTarget != nullptr && candidateTarget->getType() == Rigid &&
          ++angularTargets[bc->getTargetId()] > 1 && hasAngular) {
        std::cerr << "ERROR: multiple velocity BCs target rigid tool zone " << bc->getTargetId()
                  << "; angular motion export is ambiguous." << std::endl;
        return false;
      }
    }
    if (hasAngular && bc->getApplyTo() == ApplyToNodeSet) {
      for (int p = 0; p < m_model->getPartCount(); ++p) {
        Part *candidate = m_model->getPart(p);
        if (candidate != nullptr && candidate->getType() == Rigid &&
            candidate->getId() == bc->getTargetId()) {
          std::cerr << "ERROR: NodeSet ID " << bc->getTargetId()
                    << " aliases a rigid tool zone in angular-motion input." << std::endl;
          return false;
        }
      }
    }
    if (hasAngular && bc->getType() == VelocityBC && bc->getApplyTo() == ApplyToPart && bc->usesAmplitude()) {
      Part *linearTarget = nullptr;
      for (int p = 0; p < m_model->getPartCount(); ++p) {
        Part *candidate = m_model->getPart(p);
        if (candidate != nullptr && candidate->getId() == bc->getTargetId()) linearTarget = candidate;
      }
      if (linearTarget != nullptr && linearTarget->getType() == Rigid) {
      const auto &times = bc->getAmplitudeTime();
      const auto &values = bc->getAmplitudeValue();
      bool valid = times.size() >= 2 && times.size() == values.size() &&
                   std::isfinite(bc->getAmplitudeFactor());
      for (std::size_t k = 0; valid && k < times.size(); ++k)
        valid = std::isfinite(times[k]) && times[k] >= 0.0 && std::isfinite(values[k]) &&
                (k == 0 || times[k] > times[k - 1]);
      if (!valid) {
        std::cerr << "ERROR: linear amplitude on an angular-motion input must have finite values and at least two strictly increasing non-negative times." << std::endl;
        return false;
      }
      }
    }
    if (!bc->hasAngularMotion()) continue;
    if (m_model->getAnalysisType() != Solid3D || step == nullptr || !step->isImplicit() ||
        !step->usesRigidViscoplasticImplicitSolver() || bc->getApplyTo() != ApplyToPart) {
      std::cerr << "ERROR: angular BC export requires 3D MINI3D rigid-viscoplastic implicit mode and a Part target." << std::endl;
      return false;
    }
    Part *target = nullptr;
    for (int p = 0; p < m_model->getPartCount(); ++p) {
      Part *candidate = m_model->getPart(p);
      if (candidate != nullptr && candidate->getId() == bc->getTargetId()) target = candidate;
    }
    if (target == nullptr || target->getType() != Rigid || !target->isMeshed()) {
      std::cerr << "ERROR: angular BC target must be a meshed rigid Part (zoneId " << bc->getTargetId() << ")." << std::endl;
      return false;
    }
    std::string angularError;
    if (!bc->isAngularMotionValid(&angularError)) {
      std::cerr << "ERROR: invalid angular BC: " << angularError << std::endl;
      return false;
    }
  }


  json m_json;
  fs::path json_path(fname);
  fs::path json_dir = json_path.has_parent_path() ? json_path.parent_path() : fs::path(".");
  std::string base_name = modelStem(m_model);
  ExportGenerationDirectory generation;
  std::string generationError;
  if (!generation.create(json_dir, base_name, &generationError)) {
    std::cerr << "ERROR: " << generationError << std::endl;
    return false;
  }

  m_json["Configuration"]["Nproc"] = step ? step->m_nproc : 1;
  m_json["Configuration"]["autoTS"] = {
    step ? step->m_autoTS[0] : false,
    step ? step->m_autoTS[1] : false,
    step ? step->m_autoTS[2] : false
  };
  m_json["Configuration"]["kernelGradCorr"] = step ? step->m_kernelGradCorr : false;
  m_json["Configuration"]["simTime"] = step ? step->m_simTime : 200.0;
  m_json["Configuration"]["outTime"] = step ? step->m_outTime : 1.0;
  m_json["Configuration"]["fixedTS"] = step ? step->m_fixedTS : false;
  m_json["Configuration"]["domType"] = domTypeFromAnalysis(m_model);
  m_json["Configuration"]["plHeatFrac"] = m_model->getPlasticHeatFraction();
  m_json["Configuration"]["AxiSymmVol"] = step ? step->m_axiSymmVol : false;
  m_json["Configuration"]["elemLentghFraction"] = step ? step->m_elemLengthFraction : 0.2;
  if (m_model->m_thermal_coupling)
    m_json["Configuration"]["thermal"] = true;
  if (step != nullptr && step->isImplicit()) {
    m_json["Configuration"]["solver"]["implicit"] =
        makeImplicitSolverJson(step, m_model->getAnalysisType() == Solid3D);
    m_json["Configuration"]["solver"]["implicit"]["profilingEnabled"] =
        step->m_implicitProfilingEnabled;
  }
  appendSymmetryPlanesToConfiguration(m_json["Configuration"], m_model);

  const RemeshingSettings &remeshing = m_model->remeshing();
  m_json["Meshing"]["enabled"] = remeshing.enabled;
  m_json["Meshing"]["stepInterval"] = remeshing.stepInterval;
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
  m_json["Meshing"]["hausdorffTolerance"] = remeshing.hausdorffTolerance;
  m_json["Meshing"]["type"] = remeshing.type;
  m_json["Meshing"]["refineOnlyBoundary"] = remeshing.refineOnlyBoundary;
  m_json["Meshing"]["boundaryLayers"] = remeshing.boundaryLayers;
  m_json["Meshing"]["debug"] = remeshing.debug;
  m_json["Meshing"]["minElemAngle"] = remeshing.minElemAngle;
  m_json["Meshing"]["maxElemAngle"] = remeshing.maxElemAngle;
  m_json["Meshing"]["transitionAngle"] = remeshing.transitionAngle;

  const ContactProperties &contact = m_model->contactProps();
  m_json["Contact"] = json::array();
  m_json["Contact"].push_back({
    {"contactNormalMode", contact.unilateralContact ? "unilateral" : "legacy"},
    {"auto", contact.autoPenalty},
    {"autoFactor", contact.autoFactor},
    {"contactActivationRamp", contact.contactActivationRamp},
    {"contactActivationRampWidth", contact.contactActivationRampWidth},
    {"diagnosticLevel", contact.diagnosticLevel},
    {"unilateralDiagnosticLevel", contact.unilateralDiagnosticLevel},
    {"unilateralDiagnosticTrackedPoints", contact.unilateralDiagnosticTrackedPoints},
    {"frictionModel", contact.frictionModel},
    {"fricCoeffStatic", contact.fricCoeffStatic},
    {"frictionRegularizationVelocity", contact.frictionRegularizationVelocity},
    {"gapPenaltyScale", contact.gapPenaltyScale},
    {"heatCondCoeff", contact.heatCondCoeff},
    {"heatConductance", contact.heatConductance},
    {"maxAccel", contact.maxAccel},
    {"maxPenetRatio", contact.maxPenetRatio},
    {"penaltyFactor", contact.penaltyFactor},
    {"useGapPenalty", contact.useGapPenalty}
  });
  if (contact.frictionModel == "coulomb_tresca")
    m_json["Contact"][0]["coulombCoefficient"] = contact.coulombCoefficient;
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

  for (std::size_t materialIndex = 0; materialIndex < m_json["Materials"].size(); ++materialIndex) {
    json& materialJson = m_json["Materials"][materialIndex];
    if (!materialJson.contains("flowStressCsv")) continue;
    std::string relativeCsv;
    if (!PackageExternalCsv(materialJson["flowStressCsv"].get<std::string>(),
                            static_cast<int>(materialIndex), generation.path(), json_path,
                            relativeCsv, &generationError)) {
      std::cerr << "ERROR: " << generationError << std::endl;
      return false;
    }
    materialJson["flowStressCsv"] = relativeCsv;
  }

  int amplitude_id = 1;
  for (std::vector<Part*>::iterator it = m_model->m_part.begin(); it != m_model->m_part.end(); ++it) {
    Part* part = *it;
    if (part == nullptr || !part->isMeshed())
      continue;

    fs::path mesh_path = generation.path() /
        (base_name + "_part_" + std::to_string(part->getId()) + ".bdf");
    const bool hasHistory = part->getMesh()->hasInitialPlasticStrainHistory();
    fs::path sidecar_path;
    if (hasHistory) {
      sidecar_path = generation.path() /
          (base_name + "_part_" + std::to_string(part->getId()) + ".initial_state.json");
      std::string stateError;
      if (!ExportInitialStatePiece(*part->getMesh(), mesh_path, sidecar_path, &stateError)) {
        std::cerr << "ERROR: failed to export MINI3D initial-state BDF/sidecar: " << stateError << std::endl;
        return false;
      }
    } else if (!part->getMesh()->exportToNASTRAN(mesh_path.string())) {
      std::cerr << "ERROR: failed to export BDF generation resource: " << mesh_path << std::endl;
      return false;
    }
    std::string relative_mesh;
    if (!ExportGenerationRelativePath(mesh_path, json_path, relative_mesh, &generationError)) {
      std::cerr << "ERROR: " << generationError << std::endl;
      return false;
    }

    if (part->getType() == Elastic) {
      json block = {
        {"type", "File"},
        {"fileName", relative_mesh},
        {"scale", {1, 1, 1}}
      };
      if (hasHistory) {
        std::string relativeSidecar;
        if (!ExportGenerationRelativePath(sidecar_path, json_path, relativeSidecar, &generationError)) {
          std::cerr << "ERROR: " << generationError << std::endl;
          return false;
        }
        block["initialStateFile"] = relativeSidecar;
      }
      m_json["DomainBlocks"].push_back(block);
    } else {
      m_json["RigidBodies"].push_back({
        {"type", "File"},
        {"fileName", relative_mesh},
        {"zoneId", part->getId()},
        // The exported BDF already contains the current transformed node coordinates.
        // Using the geometry origin here would apply the same translation twice.
        {"start", {0.0, 0.0, 0.0}},
        {"orientNormals", true},
        {"flipNormals", part->getFlipNormals()}
      });
      if (!part->getNormalOrientation().empty())
        m_json["RigidBodies"].back()["normalOrientation"] = part->getNormalOrientation();
      if (part->getNormalOrientation() == "towardPoint" ||
          part->getNormalOrientation() == "awayFromPoint") {
        const double3 orientPoint = part->getOrientPoint();
        m_json["RigidBodies"].back()["orientPoint"] =
            {orientPoint.x, orientPoint.y, orientPoint.z};
      }
    }
  }

  int bc_export_id = 1;
  for (int i = 0; i < m_model->getBCCount(); ++i) {
    BoundaryCondition* bc = m_model->getBC(i);
    if (!bc)
      continue;
    if (bc->getType() == SymmetryBC)
      continue;

    if (!appendDirectionalBoundaryCondition(m_json["BoundaryConditions"],
                                       m_json["Amplitudes"],
                                       m_model,
                                       bc,
                                       bc_export_id++,
                                       amplitude_id)) return false;
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

  const fs::path temporaryInputPath = json_dir /
      (json_path.filename().string() + ".tmp." + generation.name());
  if (!WriteJsonAndPublish(m_json, temporaryInputPath, json_path, &generationError)) {
    std::cerr << "ERROR: " << generationError << std::endl;
    return false;
  }
  generation.retain();
  return true;
}
