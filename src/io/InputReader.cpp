#include "InputReader.h"

#include "BoundaryCondition.h"
#include "InitialCondition.h"
#include "Material.h"
#include "Mesh.h"
#include "Model.h"
#include "Node.h"
#include "Part.h"
#include "Step.h"
#include "Element.h"
#include "StrictJson.h"
#include "InitialStateSidecar.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace {
namespace fs = std::filesystem;

bool isJsonInt(const json &value)
{
  if (value.is_number_unsigned())
    return value.get<unsigned long long>() <= static_cast<unsigned long long>(std::numeric_limits<int>::max());
  return value.is_number_integer() && value.get<long long>() >= std::numeric_limits<int>::min() &&
         value.get<long long>() <= std::numeric_limits<int>::max();
}

std::string lowerCopy(std::string value)
{
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}

double3 readDouble3(const json& value, const double3& fallback = make_double3(0.0, 0.0, 0.0))
{
  double3 out = fallback;
  if (!value.is_array())
    return out;

  if (value.size() > 0)
    out.x = value[0].get<double>();
  if (value.size() > 1)
    out.y = value[1].get<double>();
  if (value.size() > 2)
    out.z = value[2].get<double>();
  return out;
}

std::vector<double> readDoubleArray(const json& value)
{
  if (!value.is_array())
    return {};
  return value.get<std::vector<double>>();
}

fs::path resolveRelative(const fs::path& base_dir, const std::string& file_name)
{
  fs::path path(file_name);
  if (path.is_relative())
    path = base_dir / path;
  return path;
}

AnalysisType analysisFromDomType(const std::string& dom_type)
{
  const std::string type = lowerCopy(dom_type);
  if (type == "planestress" || type == "plane_stress")
    return PlaneStress2D;
  if (type == "plstrain" || type == "planestrain" || type == "plane_strain")
    return PlaneStrain2D;
  if (type == "axisymm" || type == "axisymmetric" || type == "axisymmetric2d")
    return Axisymmetric2D;
  return Solid3D;
}

bool angularKeysOnlyInBoundaryConditions(const json &root)
{
  static const std::vector<std::string> keys = {"valueAng", "rotationCenter", "angularValueType",
      "angularAmplitudeId", "angularAmplitudeFactor"};
  std::function<bool(const json &, bool)> visit = [&](const json &value, bool isDirectBc) {
    if (value.is_object()) {
      for (auto it = value.begin(); it != value.end(); ++it) {
        const bool keyAllowed = std::find(keys.begin(), keys.end(), it.key()) != keys.end();
        if (keyAllowed && !isDirectBc) return false;
        if (!visit(it.value(), false)) return false;
      }
    } else if (value.is_array()) {
      for (const json &item : value) if (!visit(item, false)) return false;
    }
    return true;
  };
  if (!root.is_object()) return true;
  for (auto it = root.begin(); it != root.end(); ++it) {
    if (std::find(keys.begin(), keys.end(), it.key()) != keys.end()) return false;
    if (it.key() == "BoundaryConditions" && it.value().is_array()) {
      for (const json &bc : it.value()) if (!visit(bc, true)) return false;
    } else if (!visit(it.value(), false)) return false;
  }
  return true;
}

void readStep(const json& root, Model* model)
{
  if (model == nullptr || !root.contains("Configuration") || !root["Configuration"].is_object())
    return;

  const json& conf = root["Configuration"];
  Step* step = new Step();
  if (conf.contains("solver") && conf["solver"].is_object() && conf["solver"].contains("implicit")) {
    step->setStepType(ImplicitStep);
    const json& implicit = conf["solver"]["implicit"];
    step->m_implicitFormulation =
        implicitFormulationFromConfigString(implicit.value("formulation", std::string("rigid_viscoplastic")));
    if (implicit.contains("type")) {
      step->m_implicitType = implicit.value("type", step->m_implicitType);
    } else if (conf["solver"].contains("type")) {
      step->m_implicitType = conf["solver"].value("type", step->m_implicitType);
    }
    step->m_velTol = implicit.value("velTol", step->m_velTol);
    step->m_bubbleVelTol = implicit.value("bubbleVelTol", step->m_velTol);
    step->m_pressTol = implicit.value("pressTol", step->m_pressTol);
    step->m_forceTol = implicit.value("forceTol", step->m_forceTol);
    step->m_divTol = implicit.value("divTol", step->m_divTol);
    step->m_omegaV = implicit.value("omegaV", step->m_omegaV);
    step->m_omegaP = implicit.value("omegaP", step->m_omegaP);
    step->m_maxIter = implicit.value("maxIter", step->m_maxIter);
    step->m_timeStepGrowthFactor = implicit.value("timeStepGrowthFactor", step->m_timeStepGrowthFactor);
    step->m_initialTimeStep = implicit.value("initialTimeStep", step->m_initialTimeStep);
    step->m_maxStepRetries = std::max(0, implicit.value("maxStepRetries", step->m_maxStepRetries));
    step->m_nonConvergenceCutbackFactor = implicit.value("nonConvergenceCutbackFactor", step->m_nonConvergenceCutbackFactor);
    if (!(step->m_initialTimeStep >= 0.0) || !std::isfinite(step->m_initialTimeStep)) step->m_initialTimeStep = 0.0;
    if (!(step->m_nonConvergenceCutbackFactor > 0.0 && step->m_nonConvergenceCutbackFactor < 1.0) || !std::isfinite(step->m_nonConvergenceCutbackFactor)) step->m_nonConvergenceCutbackFactor = 0.5;
    step->m_useWeakSprings = implicit.value("useSprings", step->m_useWeakSprings);
    step->m_springFactor = implicit.value("springFactor", step->m_springFactor);
    step->m_springStiffness = implicit.value("springStiffness", step->m_springStiffness);
    step->m_springMode = implicit.value("springMode", step->m_springMode);
    step->m_rigidModeStabilization = implicit.value("rigidModeStabilization", step->m_rigidModeStabilization);
    step->m_rigidModeStabilizationFactor = implicit.value("rigidModeStabilizationFactor", step->m_rigidModeStabilizationFactor);
    step->m_rigidModeContactFade = implicit.value("rigidModeContactFade", step->m_rigidModeContactFade);
    step->m_rigidModeContactFadeScale = implicit.value("rigidModeContactFadeScale", step->m_rigidModeContactFadeScale);
    step->m_adaptiveDtLimiter = implicit.value("adaptiveDtLimiter", step->m_adaptiveDtLimiter);
    step->m_adaptiveDtMin = implicit.value("adaptiveDtMin", step->m_adaptiveDtMin);
    step->m_maxNodalDisplacementPerStep = implicit.value("maxNodalDisplacementPerStep", step->m_maxNodalDisplacementPerStep);
    step->m_maxEffectiveStrainIncrementPerStep = implicit.value("maxEffectiveStrainIncrementPerStep", step->m_maxEffectiveStrainIncrementPerStep);
    step->m_picardUseFixedPointResidual = implicit.value("picardUseFixedPointResidual", step->m_picardUseFixedPointResidual);
    step->m_picardStagnationEnabled = implicit.value("picardStagnationEnabled", step->m_picardStagnationEnabled);
    step->m_picardStagnationWindow = std::max(2, implicit.value("picardStagnationWindow", step->m_picardStagnationWindow));
    step->m_picardStagnationMinIter = std::max(0, implicit.value("picardStagnationMinIter", step->m_picardStagnationMinIter));
    step->m_picardStagnationMinImprovement = std::clamp(implicit.value("picardStagnationMinImprovement", step->m_picardStagnationMinImprovement), 0.0, 1.0);
    step->m_picardStagnationMaxWindows = std::max(1, implicit.value("picardStagnationMaxWindows", step->m_picardStagnationMaxWindows));
    step->m_picardUseAitken = implicit.value("picardUseAitken", step->m_picardUseAitken);
    step->m_picardAitkenMinOmega = implicit.value("picardAitkenMinOmega", step->m_picardAitkenMinOmega);
    step->m_picardAitkenMaxOmega = std::max(step->m_picardAitkenMinOmega, implicit.value("picardAitkenMaxOmega", step->m_picardAitkenMaxOmega));
    step->m_picardUseAitkenBubble = implicit.value("picardUseAitkenBubble", step->m_picardUseAitkenBubble);
    step->m_omegaBubble = implicit.value("omegaBubble", step->m_omegaBubble);
    step->m_picardAitkenBubbleMinOmega = implicit.value("picardAitkenBubbleMinOmega", step->m_picardAitkenBubbleMinOmega);
    step->m_picardAitkenBubbleMaxOmega = std::max(step->m_picardAitkenBubbleMinOmega, implicit.value("picardAitkenBubbleMaxOmega", step->m_picardAitkenBubbleMaxOmega));
    step->m_picardAcceleration = implicit.value("picardAcceleration", step->m_picardAcceleration);
    step->m_picardAndersonDepth = std::clamp(implicit.value("picardAndersonDepth", step->m_picardAndersonDepth), 1, 5);
    step->m_picardAndersonConditionLimit = implicit.value("picardAndersonConditionLimit", step->m_picardAndersonConditionLimit);
    step->m_picardAndersonCoefficientLimit = implicit.value("picardAndersonCoefficientLimit", step->m_picardAndersonCoefficientLimit);
    step->m_picardAndersonStepFactorLimit = implicit.value("picardAndersonStepFactorLimit", step->m_picardAndersonStepFactorLimit);
    step->m_picardAndersonRejectFactor = implicit.value("picardAndersonRejectFactor", step->m_picardAndersonRejectFactor);
    step->m_picardAndersonMaxRecoveries = std::max(0, implicit.value("picardAndersonMaxRecoveries", step->m_picardAndersonMaxRecoveries));
    step->m_picardAndersonRecoveryIterations = std::max(1, implicit.value("picardAndersonRecoveryIterations", step->m_picardAndersonRecoveryIterations));
    step->m_picardSmoothViscosity = implicit.value("picardSmoothViscosity", step->m_picardSmoothViscosity);
    step->m_picardStrainRateRegularization = implicit.value("picardStrainRateRegularization", step->m_picardStrainRateRegularization);
    if (!(step->m_picardAitkenMinOmega > 0.0) || !std::isfinite(step->m_picardAitkenMinOmega)) step->m_picardAitkenMinOmega = 0.05;
    if (!(step->m_picardAitkenMaxOmega >= step->m_picardAitkenMinOmega) || !std::isfinite(step->m_picardAitkenMaxOmega)) step->m_picardAitkenMaxOmega = 0.8;
    if (!(step->m_omegaBubble > 0.0) || !std::isfinite(step->m_omegaBubble)) step->m_omegaBubble = 0.4;
    if (!(step->m_picardAitkenBubbleMinOmega > 0.0) || !std::isfinite(step->m_picardAitkenBubbleMinOmega)) step->m_picardAitkenBubbleMinOmega = 0.05;
    if (!(step->m_picardAitkenBubbleMaxOmega >= step->m_picardAitkenBubbleMinOmega) || !std::isfinite(step->m_picardAitkenBubbleMaxOmega)) step->m_picardAitkenBubbleMaxOmega = step->m_picardAitkenBubbleMinOmega;
    if (!(step->m_picardStrainRateRegularization > 0.0) || !std::isfinite(step->m_picardStrainRateRegularization)) step->m_picardStrainRateRegularization = 0.001;
  }

  step->m_nproc = conf.value("Nproc", step->m_nproc);
  step->m_cflFactor = conf.value("cflFactor", step->m_cflFactor);
  step->m_simTime = conf.value("simTime", step->m_simTime);
  step->m_outTime = conf.value("outTime", step->m_outTime);
  step->m_fixedTS = conf.value("fixedTS", step->m_fixedTS);
  step->m_kernelGradCorr = conf.value("kernelGradCorr", step->m_kernelGradCorr);
  step->m_axiSymmVol = conf.value("AxiSymmVol", step->m_axiSymmVol);
  step->m_artifViscAlpha = conf.value("artifViscAlpha", step->m_artifViscAlpha);
  step->m_artifViscBeta = conf.value("artifViscBeta", step->m_artifViscBeta);
  if (conf.contains("artifViscCoeffs") && conf["artifViscCoeffs"].is_array()) {
    const json &coeffs = conf["artifViscCoeffs"];
    if (coeffs.size() > 0)
      step->m_artifViscAlpha = coeffs[0].get<double>();
    if (coeffs.size() > 1)
      step->m_artifViscBeta = coeffs[1].get<double>();
  }
  step->m_elemLengthFraction = conf.value("elemLengthFraction",
    conf.value("elemLentghFraction", step->m_elemLengthFraction));

  if (root.contains("Stabilization") && root["Stabilization"].is_object()) {
    const json &stab = root["Stabilization"];
    step->m_stabAlphaFree = stab.value("alpha_free", step->m_stabAlphaFree);
    step->m_stabAlphaContact = stab.value("alpha_contact", step->m_stabAlphaContact);
    step->m_stabHgCoeffFree = stab.value("hg_coeff_free", step->m_stabHgCoeffFree);
    step->m_stabHgCoeffContact = stab.value("hg_coeff_contact", step->m_stabHgCoeffContact);
    step->m_stabAvCoeffDiv = stab.value("av_coeff_div", step->m_stabAvCoeffDiv);
    step->m_stabAvCoeffBulk = stab.value("av_coeff_bulk", step->m_stabAvCoeffBulk);
    step->m_stabLogFactor = stab.value("log_factor", step->m_stabLogFactor);
    step->m_stabPspgScale = stab.value("pspg_scale", step->m_stabPspgScale);
    step->m_stabPspgBulkFactor = stab.value("p_pspg_bulkfac", step->m_stabPspgBulkFactor);
    step->m_stabJMin = stab.value("J_min", step->m_stabJMin);
    step->m_stabHgVisc = stab.value("hg_visc", step->m_stabHgVisc);
    step->m_stabHgStiff = stab.value("hg_stiff", step->m_stabHgStiff);
  }

  if (conf.contains("autoTS") && conf["autoTS"].is_array()) {
    for (int i = 0; i < 3 && i < static_cast<int>(conf["autoTS"].size()); ++i)
      step->m_autoTS[i] = conf["autoTS"][i].get<bool>();
  }

  model->addStep(step);
}

void readSymmetryPlanes(const json& root, Model* model)
{
  if (model == nullptr || !root.contains("Configuration") || !root["Configuration"].is_object())
    return;

  const json& conf = root["Configuration"];
  if (conf.value("xSymm", false)) {
    model->upsertSymmetryPlane({true, 0, conf.value("xSymmPlane", 0.0)});
  }
  if (conf.value("ySymm", false)) {
    model->upsertSymmetryPlane({true, 1, conf.value("ySymmPlane", 0.0)});
  }
  if (conf.value("zSymm", false)) {
    model->upsertSymmetryPlane({true, 2, conf.value("zSymmPlane", 0.0)});
  }
}

void readContact(const json& root, Model* model)
{
  if (model == nullptr)
    return;

  ContactProperties& props = model->contactProps();
  props.contactActivationRamp = false;
  props.contactActivationRampWidth = 0.001;
  if (!root.contains("Contact") || !root["Contact"].is_array() || root["Contact"].empty())
    return;

  const json& contact = root["Contact"][0];
  props.autoPenalty = contact.value("auto", contact.value("autoPenalty", props.autoPenalty));
  props.autoFactor = contact.value("autoFactor", props.autoFactor);
  props.diagnosticLevel = std::max(0, std::min(2,
      contact.value("diagnosticLevel", props.diagnosticLevel)));
  props.fricCoeffStatic = contact.value("fricCoeffStatic", props.fricCoeffStatic);
  props.frictionRegularizationVelocity =
      contact.value("frictionRegularizationVelocity", props.frictionRegularizationVelocity);
  props.gapPenaltyScale = contact.value("gapPenaltyScale", props.gapPenaltyScale);
  props.heatCondCoeff = contact.value("heatCondCoeff", props.heatCondCoeff);
  props.heatConductance = contact.value("heatConductance", props.heatConductance);
  props.maxAccel = contact.value("maxAccel", props.maxAccel);
  props.maxPenetRatio = contact.value("maxPenetRatio", props.maxPenetRatio);
  props.penaltyFactor = contact.value("penaltyFactor", props.penaltyFactor);
  props.useGapPenalty = contact.value("useGapPenalty", props.useGapPenalty);
  props.contactActivationRamp = contact.value("contactActivationRamp", false);
  props.contactActivationRampWidth = contact.value("contactActivationRampWidth", 0.001);
}

void readMeshing(const json& root, Model* model)
{
  if (model == nullptr || !root.contains("Meshing") || !root["Meshing"].is_object())
    return;

  const json& meshing = root["Meshing"];
  RemeshingSettings& settings = model->remeshing();
  settings.enabled = meshing.value("enabled", settings.enabled);
  settings.stepInterval = std::max(0, meshing.value("stepInterval", settings.stepInterval));
  settings.minStrain = meshing.value("minStrain", settings.minStrain);
  settings.maxStrain = meshing.value("maxStrain", settings.maxStrain);
  settings.mapVel = meshing.value("mapVel", settings.mapVel);
  settings.mapAcc = meshing.value("mapAcc", settings.mapAcc);
  settings.maxCount = meshing.value("maxCount", settings.maxCount);
  settings.dampFactor = meshing.value("dampFactor", settings.dampFactor);
  settings.minFrac = meshing.value("minFrac", settings.minFrac);
  settings.maxFrac = meshing.value("maxFrac", settings.maxFrac);
  settings.epsRef = meshing.value("epsRef", settings.epsRef);
  settings.beta = meshing.value("beta", settings.beta);
  settings.hausdorffTolerance = meshing.value("hausdorffTolerance", settings.hausdorffTolerance);
  settings.type = meshing.value("type", settings.type);
  settings.refineOnlyBoundary = meshing.value("refineOnlyBoundary", settings.refineOnlyBoundary);
  settings.boundaryLayers = std::max(0, meshing.value("boundaryLayers", settings.boundaryLayers));
  settings.debug = meshing.value("debug", settings.debug);
  settings.minElemAngle = meshing.value("minElemAngle", settings.minElemAngle);
  settings.maxElemAngle = meshing.value("maxElemAngle", settings.maxElemAngle);
  settings.transitionAngle = meshing.value("transitionAngle", settings.transitionAngle);
}

void readMaterials(const json& root, Model* model)
{
  if (model == nullptr || !root.contains("Materials") || !root["Materials"].is_array())
    return;

  for (const json& mat : root["Materials"]) {
    const double E = mat.value("youngsModulus", 0.0);
    const double nu = mat.value("poissonsRatio", 0.0);
    Elastic_ elastic(E, nu);
    Material_* material = new Material_(elastic);
    material->setDensityConstant(mat.value("density0", 0.0));
    material->yieldStress0 = mat.value("yieldStress0", material->yieldStress0);
    material->k_T = mat.value("thermalCond", material->k_T);
    material->cp_T = mat.value("thermalHeatCap", material->cp_T);
    material->exp_T = mat.value("thermalExp", material->exp_T);

    if (mat.contains("strRange") && mat["strRange"].is_array() && mat["strRange"].size() >= 2) {
      material->strRange = {mat["strRange"][0].get<double>(), mat["strRange"][1].get<double>()};
      material->e_min = material->strRange[0];
      material->e_max = material->strRange[1];
    }
    if (mat.contains("strdotRange") && mat["strdotRange"].is_array() && mat["strdotRange"].size() >= 2) {
      material->er_min = mat["strdotRange"][0].get<double>();
      material->er_max = mat["strdotRange"][1].get<double>();
    }
    if (mat.contains("tempRange") && mat["tempRange"].is_array() && mat["tempRange"].size() >= 2) {
      material->T_min = mat["tempRange"][0].get<double>();
      material->T_max = mat["tempRange"][1].get<double>();
    }

    const std::string type = mat.value("type", std::string("Elastic"));
    if (type == "Hollomon" && mat.contains("const") && mat["const"].is_array() && mat["const"].size() >= 2) {
      const double K = mat["const"][0].get<double>();
      const double n = mat["const"][1].get<double>();
      material->InitHollomon(elastic, material->yieldStress0, K, n);
      material->m_plastic = new Hollomon(K, n);
      material->m_isplastic = true;
    } else if (type == "Bilinear" && mat.contains("const") && mat["const"].is_array() && mat["const"].size() >= 1) {
      material->m_plastic = new Bilinear(material->yieldStress0, mat["const"][0].get<double>());
      material->m_isplastic = true;
    }

    model->addMaterial(material);
  }
}

bool addFilePart(Model* model,
                 const fs::path& base_dir,
                 const json& part_json,
                 bool rigid,
                 int fallback_id)
{
  if (model == nullptr) return false;

  const std::string file_name = part_json.value("fileName", std::string());
  if (file_name.empty())
    return !part_json.contains("initialStateFile");

  fs::path mesh_path = resolveRelative(base_dir, file_name);
  Part* part = new Part();
  int part_id = part_json.value("zoneId", fallback_id);
  part->setId(part_id);
  part->setType(rigid ? 1 : 0);
  part->setName(rigid ? "Rigid File" : "Domain File");

  if (fs::exists(mesh_path)) {
    try {
      part->generateMeshFromNastranFile(mesh_path.string());
    } catch (const std::exception& error) {
      std::cerr << "[InputReader] Invalid Nastran mesh " << mesh_path
                << ": " << error.what() << std::endl;
      delete part;
      return false;
    }
    part->setMeshSourceFile(mesh_path.string());
  } else {
    std::cerr << "[InputReader] Mesh file not found: " << mesh_path << std::endl;
    if (part_json.contains("initialStateFile")) {
      delete part;
      return false;
    }
  }

  if (part_json.contains("initialStateFile")) {
    if (rigid || part->getMesh() == nullptr || !part_json.at("initialStateFile").is_string()) {
      std::cerr << "[InputReader] initialStateFile requires a file-backed deformable part." << std::endl;
      delete part;
      return false;
    }
    const fs::path sidecarPath = resolveRelative(base_dir,
        part_json.at("initialStateFile").get<std::string>());
    try {
      ApplyInitialStateSidecar(*part->getMesh(), sidecarPath, mesh_path, *model, *part);
    } catch (const std::exception& error) {
      std::cerr << "[InputReader] Invalid initial-state sidecar " << sidecarPath
                << ": " << error.what() << std::endl;
      delete part;
      return false;
    }
  }

  model->addPart(part);
  return true;
}

void addPlaneRigidPart(Model* model, const json& rigid_json, int fallback_id)
{
  if (model == nullptr)
    return;

  const double3 start = readDouble3(rigid_json.value("start", json::array()));
  const double3 dim = readDouble3(rigid_json.value("dim", json::array()), make_double3(1.0, 1.0, 0.0));
  const double lx = std::fabs(dim.x) > 0.0 ? std::fabs(dim.x) : 1.0;
  const double ly = std::fabs(dim.y) > 0.0 ? std::fabs(dim.y) : 1.0;
  const int nx = 10;
  const int ny = 10;

  Mesh* mesh = new Mesh();
  std::vector<Node*> nodes;
  std::vector<Element*> elements;
  nodes.reserve((nx + 1) * (ny + 1));
  elements.reserve(nx * ny * 2);

  for (int j = 0; j <= ny; ++j) {
    const double y = start.y + ly * static_cast<double>(j) / static_cast<double>(ny);
    for (int i = 0; i <= nx; ++i) {
      const double x = start.x + lx * static_cast<double>(i) / static_cast<double>(nx);
      const int node_id = static_cast<int>(nodes.size()) + 1;
      nodes.push_back(new Node(x, y, start.z, node_id));
    }
  }

  for (int j = 0; j < ny; ++j) {
    for (int i = 0; i < nx; ++i) {
      const int n0 = j * (nx + 1) + i;
      const int n1 = n0 + 1;
      const int n3 = (j + 1) * (nx + 1) + i;
      const int n2 = n3 + 1;

      Tria* first = new Tria(nodes[n0], nodes[n1], nodes[n2]);
      int first_id = static_cast<int>(elements.size()) + 1;
      first->setId(first_id);
      elements.push_back(first);

      Tria* second = new Tria(nodes[n0], nodes[n2], nodes[n3]);
      int second_id = static_cast<int>(elements.size()) + 1;
      second->setId(second_id);
      elements.push_back(second);
    }
  }
  mesh->assignValues(nodes, elements);
  mesh->setDim(2);

  Part* part = new Part();
  int part_id = rigid_json.value("zoneId", fallback_id);
  part->setId(part_id);
  part->setType(1);
  part->setName("Rigid Plane");
  part->setMesh(mesh);
  model->addPart(part);
}

std::map<int, std::pair<std::vector<double>, std::vector<double>>> readAmplitudes(const json& root,
                                                                                bool strictAngular,
                                                                                bool &validAngularTables)
{
  std::map<int, std::pair<std::vector<double>, std::vector<double>>> amplitudes;
  std::set<int> seenIds;
  if (!root.contains("Amplitudes") || !root["Amplitudes"].is_array())
    return amplitudes;

  for (const json& amp : root["Amplitudes"]) {
    if (!amp.is_object() || !amp.contains("id") || !isJsonInt(amp["id"])) {
      if (strictAngular) {
        std::cerr << "[InputReader] ERROR: amplitude table id must be an integer in range." << std::endl;
        validAngularTables = false;
      }
      continue;
    }
    const int id = amp["id"].get<int>();
    if (id < 0)
      continue;
    if (!seenIds.insert(id).second) {
      if (strictAngular) {
        std::cerr << "[InputReader] ERROR: duplicate amplitude id in angular-motion input: " << id << std::endl;
        validAngularTables = false;
      }
      if (strictAngular) continue;
      amplitudes.erase(id);
    }
    amplitudes[id] = {readDoubleArray(amp.value("time", json::array())),
                      readDoubleArray(amp.value("value", json::array()))};
    const auto &times = amplitudes[id].first;
    const auto &values = amplitudes[id].second;
    if (strictAngular) {
      bool valid = times.size() >= 2 && times.size() == values.size();
      for (std::size_t i = 0; valid && i < times.size(); ++i)
        valid = std::isfinite(times[i]) && times[i] >= 0.0 && std::isfinite(values[i]) &&
                (i == 0 || times[i] > times[i - 1]);
      if (!valid) {
        amplitudes.erase(id);
        for (const json &bc : root.value("BoundaryConditions", json::array()))
          if (bc.is_object() && bc.value("angularAmplitudeId", -1) == id) {
            std::cerr << "[InputReader] ERROR: angular amplitude table must contain finite values and at least two strictly increasing non-negative times." << std::endl;
            validAngularTables = false;
          }
      }
    }
  }

  return amplitudes;
}

bool readBoundaryConditions(const json& root, Model* model)
{
  if (model == nullptr || !root.contains("BoundaryConditions") || !root["BoundaryConditions"].is_array())
    return true;

  const bool containsAngular = std::any_of(root["BoundaryConditions"].begin(), root["BoundaryConditions"].end(),
      [](const json &bc) {
        if (!bc.is_object()) return false;
        for (const char *key : {"valueAng", "rotationCenter", "angularValueType",
                                "angularAmplitudeId", "angularAmplitudeFactor"})
          if (bc.contains(key)) return true;
        return false;
      });
  bool validAngularTables = true;
  const auto amplitudes = readAmplitudes(root, containsAngular, validAngularTables);
  if (!validAngularTables) return false;
  if (containsAngular) {
    std::map<int, int> rigidIds;
    for (int p = 0; p < model->getPartCount(); ++p) {
      Part *part = model->getPart(p);
      if (part != nullptr && part->getType() == Rigid && ++rigidIds[part->getId()] > 1) {
        std::cerr << "[InputReader] ERROR: duplicate rigid tool zoneId in angular-motion input: "
                  << part->getId() << std::endl;
        return false;
      }
    }
    if (root.contains("Sets") && root["Sets"].is_array()) {
      for (const json &set : root["Sets"]) {
        if (!set.is_object() || !set.contains("id") || !isJsonInt(set["id"])) continue;
        const int setId = set["id"].get<int>();
        if (rigidIds.count(setId) != 0) {
          std::cerr << "[InputReader] ERROR: Set ID " << setId
                    << " aliases a rigid tool zone in angular-motion input." << std::endl;
          return false;
        }
      }
    }
    for (const json &entry : root["BoundaryConditions"]) {
      if (!entry.is_object()) continue;
      const bool setTarget = entry.contains("setId");
      const json &rawTarget = setTarget ? entry["setId"] : entry.value("zoneId", json(-1));
      if (!isJsonInt(rawTarget)) {
        std::cerr << "[InputReader] ERROR: boundary-condition target ID must be an integer in range in angular-motion input." << std::endl;
        return false;
      }
      const int zoneId = rawTarget.get<int>();
      bool rigidTarget = false;
      for (int p = 0; p < model->getPartCount(); ++p) {
        Part *part = model->getPart(p);
        rigidTarget = rigidTarget || (part != nullptr && part->getId() == zoneId && part->getType() == Rigid);
      }
      if (!rigidTarget) continue;
      if (setTarget) {
        std::cerr << "[InputReader] ERROR: NodeSet ID aliases a rigid tool zone in angular-motion input." << std::endl;
        return false;
      }
      if (entry.contains("type") && (!isJsonInt(entry["type"]) || entry["type"].get<int>() != 0)) {
        std::cerr << "[InputReader] ERROR: angular-motion tool BCs must be Velocity type 0." << std::endl;
        return false;
      }
      if (!entry.contains("value") || !entry["value"].is_array() || entry["value"].size() != 3) {
        std::cerr << "[InputReader] ERROR: rigid tool velocity value must have exactly three components." << std::endl;
        return false;
      }
      for (const json &component : entry["value"])
        if (!component.is_number() || !std::isfinite(component.get<double>())) {
          std::cerr << "[InputReader] ERROR: rigid tool velocity components must be finite numbers." << std::endl;
          return false;
        }
      if (entry.contains("valueType") && !isJsonInt(entry["valueType"])) {
        std::cerr << "[InputReader] ERROR: rigid tool valueType must be an integer." << std::endl;
        return false;
      }
      const int valueType = entry.contains("valueType") && isJsonInt(entry["valueType"])
          ? entry["valueType"].get<int>() : static_cast<int>(ConstantValue);
      if (valueType != ConstantValue && valueType != AmplitudeValue) {
        std::cerr << "[InputReader] ERROR: rigid tool velocity valueType must be 0 or 1 in angular-motion input." << std::endl;
        return false;
      }
      if (valueType == AmplitudeValue) {
        if (!entry.contains("amplitudeId") || !isJsonInt(entry["amplitudeId"]) ||
            amplitudes.count(entry["amplitudeId"].get<int>()) == 0 ||
            (entry.contains("amplitudeFactor") && (!entry["amplitudeFactor"].is_number() ||
             !std::isfinite(entry["amplitudeFactor"].get<double>())))) {
          std::cerr << "[InputReader] ERROR: rigid tool linear amplitude fields are incomplete or invalid." << std::endl;
          return false;
        }
      }
    }
  }
  for (const json& bc_json : root["BoundaryConditions"]) {
    const char *angularKeys[] = {"valueAng", "rotationCenter", "angularValueType",
                                 "angularAmplitudeId", "angularAmplitudeFactor"};
    bool hasAngular = false;
    for (const char *key : angularKeys) hasAngular = hasAngular || bc_json.contains(key);
    const auto fail = [](const std::string &message) {
      std::cerr << "[InputReader] ERROR: invalid angular boundary condition: " << message << std::endl;
      return false;
    };
    if (hasAngular) {
      if (bc_json.contains("setId")) return fail("angular BC target must be a Part zoneId, not setId");
      if (bc_json.contains("type") && (!isJsonInt(bc_json["type"]) || bc_json["type"].get<int>() != 0))
        return fail("angular motion is valid only for Velocity boundary conditions (type 0)");
      if (!bc_json.contains("valueAng") || !bc_json.contains("rotationCenter") ||
          (bc_json.contains("angularValueType") && !isJsonInt(bc_json["angularValueType"])))
        return fail("angular BC requires valueAng and rotationCenter; angularValueType, when present, must be an integer");
      if (!bc_json["valueAng"].is_array() || bc_json["valueAng"].size() != 3 ||
          !bc_json["rotationCenter"].is_array() || bc_json["rotationCenter"].size() != 3)
        return fail("vectors must have three components");
      for (const json *vector : {&bc_json["valueAng"], &bc_json["rotationCenter"]})
        for (const json &component : *vector)
          if (!component.is_number() || !std::isfinite(component.get<double>()))
            return fail("vector components must be finite numbers");
      const int angularType = bc_json.value("angularValueType", static_cast<int>(ConstantValue));
      if (angularType != ConstantValue && angularType != AmplitudeValue)
        return fail("angularValueType must be 0 or 1");
      const bool hasAngularId = bc_json.contains("angularAmplitudeId");
      const bool hasAngularFactor = bc_json.contains("angularAmplitudeFactor");
      if ((angularType == ConstantValue && hasAngularId) ||
          (angularType == AmplitudeValue && !hasAngularId))
        return fail("angular amplitude fields contradict angularValueType");
      if (angularType == AmplitudeValue && (!isJsonInt(bc_json["angularAmplitudeId"]) ||
          amplitudes.count(bc_json["angularAmplitudeId"].get<int>()) == 0))
        return fail("angularAmplitudeId does not reference an amplitude");
      if (hasAngularFactor && (!bc_json["angularAmplitudeFactor"].is_number() ||
          !std::isfinite(bc_json["angularAmplitudeFactor"].get<double>())))
        return fail("angularAmplitudeFactor must be a finite number");
      if (angularType == ConstantValue && hasAngularFactor &&
          bc_json["angularAmplitudeFactor"].get<double>() != 1.0)
        return fail("constant angularAmplitudeFactor must equal its default value 1");
    }
    const bool target_set = bc_json.contains("setId");
    const json &targetValue = target_set ? bc_json.value("setId", json(-1)) : bc_json.value("zoneId", json(-1));
    if (hasAngular && !isJsonInt(targetValue)) return fail("angular BC zoneId must be an integer in range");
    const int target_id = isJsonInt(targetValue) ? targetValue.get<int>() : -1;
    if (target_id < 0) {
      if (hasAngular) return fail("angular BC requires a valid zoneId target");
      continue;
    }
    if (hasAngular) {
      Part *target = nullptr;
      int targetMatches = 0;
      for (int p = 0; p < model->getPartCount(); ++p) {
        Part *candidate = model->getPart(p);
        if (candidate != nullptr && candidate->getId() == target_id) { target = candidate; ++targetMatches; }
      }
      if (targetMatches != 1 || target == nullptr || target->getType() != Rigid || !target->isMeshed())
        return fail("angular BC zoneId must resolve to a meshed rigid Part");
    }

    const double3 value = readDouble3(bc_json.value("value", json::array()));
    std::unique_ptr<BoundaryCondition> bc(new BoundaryCondition(VelocityBC,
                                                  target_set ? ApplyToNodeSet : ApplyToPart,
                                                  target_id,
                                                  value));
    bc->setValueType(bc_json.value("valueType", 0));
    if (bc_json.contains("direction")) {
      bc->setDofMask(false, false, false);
      if (bc_json["direction"].is_number_integer()) {
        bc->setDofMaskX(bc_json["direction"].get<int>() == 0);
        bc->setDofMaskY(bc_json["direction"].get<int>() == 1);
        bc->setDofMaskZ(bc_json["direction"].get<int>() == 2);
      } else if (bc_json["direction"].is_array()) {
        for (const json& direction : bc_json["direction"]) {
          const int dir = direction.get<int>();
          if (dir == 0) bc->setDofMaskX(true);
          if (dir == 1) bc->setDofMaskY(true);
          if (dir == 2) bc->setDofMaskZ(true);
        }
      }
    }

    const int amplitude_id = bc_json.value("amplitudeId", -1);
    auto amp_it = amplitudes.find(amplitude_id);
    const bool linearAmplitudeRequested = bc_json.value("valueType", static_cast<int>(ConstantValue)) ==
        static_cast<int>(AmplitudeValue);
    if (amp_it != amplitudes.end() && (!containsAngular || linearAmplitudeRequested)) {
      bc->setValueType(AmplitudeValue);
      bc->setAmplitudeFactor(bc_json.value("amplitudeFactor", 1.0));
      bc->setAmplitudeTable(amp_it->second.first, amp_it->second.second);
    }

    if (hasAngular) {
      bc->setAngularMotionEnabled(true);
      bc->setAngularVelocity(readDouble3(bc_json["valueAng"]));
      bc->setRotationCenter(readDouble3(bc_json["rotationCenter"]));
      const int angularType = bc_json.value("angularValueType", static_cast<int>(ConstantValue));
      bc->setAngularValueType(angularType);
      if (angularType == AmplitudeValue) {
        const int angularId = bc_json["angularAmplitudeId"].get<int>();
        bc->setAngularAmplitudeFactor(bc_json.value("angularAmplitudeFactor", 1.0));
        bc->setAngularAmplitudeTable(amplitudes.at(angularId).first, amplitudes.at(angularId).second);
      }
      std::string angularError;
      if (!bc->isAngularMotionValid(&angularError)) {
        std::cerr << "[InputReader] ERROR: invalid angular BC: " << angularError << std::endl;
        return false;
      }
    }

    model->addBoundaryCondition(bc.release());
  }
  bool modelHasAngular = false;
  for (int i = 0; i < model->getBCCount(); ++i)
    modelHasAngular = modelHasAngular || (model->getBC(i) != nullptr && model->getBC(i)->hasAngularMotion());
  if (modelHasAngular) {
    std::map<int, int> motionTargets;
    for (int i = 0; i < model->getBCCount(); ++i) {
      BoundaryCondition *bc = model->getBC(i);
      if (bc == nullptr || bc->getApplyTo() != ApplyToPart || bc->getType() != VelocityBC) continue;
      Part *target = nullptr;
      for (int p = 0; p < model->getPartCount(); ++p) {
        Part *candidate = model->getPart(p);
        if (candidate != nullptr && candidate->getId() == bc->getTargetId()) target = candidate;
      }
      if (target != nullptr && target->getType() == Rigid && ++motionTargets[bc->getTargetId()] > 1) {
        std::cerr << "[InputReader] ERROR: multiple velocity BCs target rigid tool zone "
                  << bc->getTargetId() << "; angular motion is ambiguous." << std::endl;
        return false;
      }
    }
  }
  return true;
}

void readInitialConditions(const json& root, Model* model)
{
  if (model == nullptr || !root.contains("InitialConditions") || !root["InitialConditions"].is_array())
    return;

  for (const json& ic_json : root["InitialConditions"]) {
    if (ic_json.contains("Temp") && ic_json.is_object()) {
      InitialCondition* ic = new InitialCondition(TempIC, ApplyToPart, 0,
                                                  make_double3(ic_json["Temp"].get<double>(), 0.0, 0.0));
      model->addInitialCondition(ic);
      continue;
    }

    const bool target_set = ic_json.contains("setId");
    const int target_id = target_set ? ic_json.value("setId", -1) : ic_json.value("zoneId", -1);
    if (target_id < 0)
      continue;

    const std::string type = ic_json.value("type", std::string("Velocity"));
    const BCType ic_type = (type == "Temperature") ? TempIC : VelocityIC;
    double3 value = make_double3(0.0, 0.0, 0.0);
    if (ic_type == TempIC && ic_json.contains("value")) {
      value.x = ic_json["value"].get<double>();
    } else {
      value = readDouble3(ic_json.value("value", json::array()));
    }

    model->addInitialCondition(new InitialCondition(ic_type,
                                                    target_set ? ApplyToNodeSet : ApplyToPart,
                                                    target_id,
                                                    value));
  }
}
}

InputReader::InputReader(Model* model) : m_model(model) {}

bool InputReader::readFromFile(const std::string& fname)
{
  if (m_model == nullptr) {
    std::cerr << "[InputReader] Error: model pointer is null." << std::endl;
    return false;
  }

  std::ifstream input(fname);
  if (!input.is_open()) {
    std::cerr << "[InputReader] Error: cannot open file: " << fname << std::endl;
    return false;
  }

  json root;
  try {
    root = ParseJsonRejectDuplicateKeys(input);
  } catch (const std::exception& e) {
    std::cerr << "[InputReader] JSON parse error: " << e.what() << std::endl;
    return false;
  }
  if (!angularKeysOnlyInBoundaryConditions(root)) {
    std::cerr << "[InputReader] ERROR: angular motion fields are allowed only on direct BoundaryConditions entries." << std::endl;
    return false;
  }
  bool rootHasAngular = false;
  if (root.contains("BoundaryConditions") && root["BoundaryConditions"].is_array()) {
    for (const json &bc : root["BoundaryConditions"])
      if (bc.is_object() && (bc.contains("valueAng") || bc.contains("rotationCenter") ||
          bc.contains("angularValueType") || bc.contains("angularAmplitudeId") ||
          bc.contains("angularAmplitudeFactor"))) rootHasAngular = true;
  }
  if (rootHasAngular && root.contains("Configuration") && root["Configuration"].is_object()) {
    const json *implicit = nullptr;
    const json &configuration = root["Configuration"];
    if (configuration.contains("solver") && configuration["solver"].is_object() &&
        configuration["solver"].contains("implicit") && configuration["solver"]["implicit"].is_object())
      implicit = &configuration["solver"]["implicit"];
    if (implicit != nullptr) {
      const bool restartRequested = implicit->contains("restartFile") &&
          (!implicit->at("restartFile").is_string() || !implicit->at("restartFile").get<std::string>().empty());
      const bool checkpointEnabled = implicit->value("checkpointEnabled", false);
      const bool checkpointStop = implicit->contains("checkpointStopTime") &&
          (!implicit->at("checkpointStopTime").is_number() || implicit->at("checkpointStopTime").get<double>() >= 0.0);
      const bool checkpointInterval = implicit->contains("checkpointInterval") &&
          (!isJsonInt(implicit->at("checkpointInterval")) || implicit->at("checkpointInterval").get<int>() > 0);
      if (restartRequested || checkpointEnabled || checkpointStop || checkpointInterval) {
        std::cerr << "[InputReader] ERROR: restart and checkpoint settings are unsupported with angular tool motion." << std::endl;
        return false;
      }
    }
  }
  if (rootHasAngular && root.contains("RigidBodies") && root["RigidBodies"].is_array()) {
    int fallbackBase = root.contains("DomainBlocks") && root["DomainBlocks"].is_array()
        ? static_cast<int>(root["DomainBlocks"].size()) : 0;
    std::set<int> rigidZoneIds;
    for (std::size_t i = 0; i < root["RigidBodies"].size(); ++i) {
      const json &body = root["RigidBodies"][i];
      const json idValue = body.is_object() && body.contains("zoneId")
          ? body["zoneId"] : json(fallbackBase + static_cast<int>(i));
      if (!isJsonInt(idValue) || idValue.get<int>() < 0) {
        std::cerr << "[InputReader] ERROR: RigidBodies zoneId must be a non-negative integer in range." << std::endl;
        return false;
      }
      if (!rigidZoneIds.insert(idValue.get<int>()).second) {
        std::cerr << "[InputReader] ERROR: duplicate raw RigidBodies zoneId in angular-motion input: "
                  << idValue.get<int>() << std::endl;
        return false;
      }
    }
  }

  const fs::path input_path(fname);
  const fs::path base_dir = input_path.has_parent_path() ? input_path.parent_path() : fs::path(".");

  int initialStateBlockCount = 0;
  int fileDomainBlockCount = 0;
  if (root.contains("DomainBlocks") && root["DomainBlocks"].is_array()) {
    for (const json& block : root["DomainBlocks"]) {
      if (block.is_object() && block.value("type", std::string()) == "File")
        ++fileDomainBlockCount;
      if (block.is_object() && block.contains("initialStateFile")) {
        ++initialStateBlockCount;
        if (block.value("type", std::string()) != "File" || !block["initialStateFile"].is_string()) {
          std::cerr << "[InputReader] initialStateFile is valid only on a file-backed DomainBlock." << std::endl;
          return false;
        }
      }
    }
  }
  if (initialStateBlockCount > 0) {
    if (initialStateBlockCount != 1 || fileDomainBlockCount != 1) {
      std::cerr << "[InputReader] MINI3D initial-state input supports exactly one file-backed deformable DomainBlock."
                << std::endl;
      return false;
    }
    const json* configuration = root.contains("Configuration") && root["Configuration"].is_object()
        ? &root["Configuration"] : nullptr;
    const json* implicit = configuration != nullptr && configuration->contains("solver") &&
        configuration->at("solver").is_object() && configuration->at("solver").contains("implicit")
        ? &configuration->at("solver").at("implicit") : nullptr;
    const std::string domType = configuration != nullptr &&
        (!configuration->contains("domType") || configuration->at("domType").is_string())
        ? lowerCopy(configuration->value("domType", std::string("3D"))) : std::string{};
    const bool isSolid3d = domType == "3d" || domType == "solid3d";
    const bool isMini3d = implicit != nullptr && implicit->is_object() &&
        implicit->contains("formulation") && implicit->at("formulation").is_string() &&
        implicit->at("formulation").get<std::string>() == "rigid_viscoplastic";
    const bool hasRestart = implicit != nullptr && implicit->is_object() &&
        implicit->contains("restartFile") &&
        (!implicit->at("restartFile").is_string() ||
         !implicit->at("restartFile").get<std::string>().empty());
    if (!isSolid3d || !isMini3d) {
      std::cerr << "[InputReader] Initial-state sidecars require the 3D MINI3D rigid-viscoplastic implicit solver; explicit and J2 inputs are unsupported."
                << std::endl;
      return false;
    }
    if (hasRestart) {
      std::cerr << "[InputReader] An initial-state sidecar cannot be combined with restartFile." << std::endl;
      return false;
    }
  }
  if (root.contains("RigidBodies") && root["RigidBodies"].is_array()) {
    for (const json& rigid : root["RigidBodies"]) {
      if (rigid.is_object() && rigid.contains("initialStateFile")) {
        std::cerr << "[InputReader] initialStateFile is supported only on a deformable DomainBlock." << std::endl;
        return false;
      }
    }
  }

  m_model->setFilePath(fname);
  if (root.contains("Configuration") && root["Configuration"].is_object()) {
    const json& conf = root["Configuration"];
    m_model->setAnalysisType(analysisFromDomType(conf.value("domType", std::string("3D"))));
    m_model->m_thermal_coupling = conf.value("thermal", false);
    if (conf.contains("plHeatFrac") && conf["plHeatFrac"].is_number())
      m_model->setPlasticHeatFraction(conf["plHeatFrac"].get<double>());
    else
      m_model->setPlasticHeatFraction(0.9);
  }

  readStep(root, m_model);
  readSymmetryPlanes(root, m_model);
  readContact(root, m_model);
  readMeshing(root, m_model);
  readMaterials(root, m_model);

  int fallback_id = 0;
  if (root.contains("DomainBlocks") && root["DomainBlocks"].is_array()) {
    for (const json& block : root["DomainBlocks"]) {
      if (block.value("type", std::string()) == "File")
        if (!addFilePart(m_model, base_dir, block, false, fallback_id)) return false;
      ++fallback_id;
    }
  }

  if (root.contains("RigidBodies") && root["RigidBodies"].is_array()) {
    for (const json& rigid : root["RigidBodies"]) {
      const std::string type = rigid.value("type", std::string());
      if (type == "File") {
        if (!addFilePart(m_model, base_dir, rigid, true, fallback_id)) return false;
      } else if (type == "Plane") {
        addPlaneRigidPart(m_model, rigid, fallback_id);
      } else {
        std::cerr << "[InputReader] Unsupported rigid body type: " << type << std::endl;
      }
      ++fallback_id;
    }
  }

  try {
    if (!readBoundaryConditions(root, m_model)) return false;
  } catch (const std::exception& e) {
    std::cerr << "[InputReader] ERROR: malformed boundary condition data: " << e.what() << std::endl;
    return false;
  }
  readInitialConditions(root, m_model);

  std::cout << "[InputReader] Input imported as editable model: " << fname << std::endl;
  return true;
}
