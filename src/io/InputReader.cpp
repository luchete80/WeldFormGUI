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

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace {
namespace fs = std::filesystem;

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
    step->m_pressTol = implicit.value("pressTol", step->m_pressTol);
    step->m_forceTol = implicit.value("forceTol", step->m_forceTol);
    step->m_divTol = implicit.value("divTol", step->m_divTol);
    step->m_omegaV = implicit.value("omegaV", step->m_omegaV);
    step->m_omegaP = implicit.value("omegaP", step->m_omegaP);
    step->m_maxIter = implicit.value("maxIter", step->m_maxIter);
    step->m_timeStepGrowthFactor = implicit.value("timeStepGrowthFactor", step->m_timeStepGrowthFactor);
    step->m_useWeakSprings = implicit.value("useSprings", step->m_useWeakSprings);
    step->m_springFactor = implicit.value("springFactor", step->m_springFactor);
    step->m_springStiffness = implicit.value("springStiffness", step->m_springStiffness);
    step->m_springMode = implicit.value("springMode", step->m_springMode);
    step->m_adaptiveDtLimiter = implicit.value("adaptiveDtLimiter", step->m_adaptiveDtLimiter);
    step->m_adaptiveDtMin = implicit.value("adaptiveDtMin", step->m_adaptiveDtMin);
    step->m_maxNodalDisplacementPerStep = implicit.value("maxNodalDisplacementPerStep", step->m_maxNodalDisplacementPerStep);
    step->m_maxEffectiveStrainIncrementPerStep = implicit.value("maxEffectiveStrainIncrementPerStep", step->m_maxEffectiveStrainIncrementPerStep);
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
  if (model == nullptr || !root.contains("Contact") || !root["Contact"].is_array() || root["Contact"].empty())
    return;

  const json& contact = root["Contact"][0];
  ContactProperties& props = model->contactProps();
  props.autoPenalty = contact.value("auto", contact.value("autoPenalty", props.autoPenalty));
  props.autoFactor = contact.value("autoFactor", props.autoFactor);
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
}

void readMeshing(const json& root, Model* model)
{
  if (model == nullptr || !root.contains("Meshing") || !root["Meshing"].is_object())
    return;

  const json& meshing = root["Meshing"];
  RemeshingSettings& settings = model->remeshing();
  settings.enabled = meshing.value("enabled", settings.enabled);
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

void addFilePart(Model* model,
                 const fs::path& base_dir,
                 const json& part_json,
                 bool rigid,
                 int fallback_id)
{
  if (model == nullptr)
    return;

  const std::string file_name = part_json.value("fileName", std::string());
  if (file_name.empty())
    return;

  fs::path mesh_path = resolveRelative(base_dir, file_name);
  Part* part = new Part();
  int part_id = part_json.value("zoneId", fallback_id);
  part->setId(part_id);
  part->setType(rigid ? 1 : 0);
  part->setName(rigid ? "Rigid File" : "Domain File");

  if (fs::exists(mesh_path)) {
    part->generateMeshFromNastranFile(mesh_path.string());
    part->setMeshSourceFile(mesh_path.string());
  } else {
    std::cerr << "[InputReader] Mesh file not found: " << mesh_path << std::endl;
  }

  model->addPart(part);
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

std::map<int, std::pair<std::vector<double>, std::vector<double>>> readAmplitudes(const json& root)
{
  std::map<int, std::pair<std::vector<double>, std::vector<double>>> amplitudes;
  if (!root.contains("Amplitudes") || !root["Amplitudes"].is_array())
    return amplitudes;

  for (const json& amp : root["Amplitudes"]) {
    const int id = amp.value("id", -1);
    if (id < 0)
      continue;
    amplitudes[id] = {readDoubleArray(amp.value("time", json::array())),
                      readDoubleArray(amp.value("value", json::array()))};
  }

  return amplitudes;
}

void readBoundaryConditions(const json& root, Model* model)
{
  if (model == nullptr || !root.contains("BoundaryConditions") || !root["BoundaryConditions"].is_array())
    return;

  const auto amplitudes = readAmplitudes(root);
  for (const json& bc_json : root["BoundaryConditions"]) {
    const bool target_set = bc_json.contains("setId");
    const int target_id = target_set ? bc_json.value("setId", -1) : bc_json.value("zoneId", -1);
    if (target_id < 0)
      continue;

    const double3 value = readDouble3(bc_json.value("value", json::array()));
    BoundaryCondition* bc = new BoundaryCondition(VelocityBC,
                                                  target_set ? ApplyToNodeSet : ApplyToPart,
                                                  target_id,
                                                  value);
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
    if (amp_it != amplitudes.end()) {
      bc->setValueType(AmplitudeValue);
      bc->setAmplitudeFactor(bc_json.value("amplitudeFactor", 1.0));
      bc->setAmplitudeTable(amp_it->second.first, amp_it->second.second);
    }

    model->addBoundaryCondition(bc);
  }
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
    input >> root;
  } catch (const std::exception& e) {
    std::cerr << "[InputReader] JSON parse error: " << e.what() << std::endl;
    return false;
  }

  const fs::path input_path(fname);
  const fs::path base_dir = input_path.has_parent_path() ? input_path.parent_path() : fs::path(".");

  m_model->setFilePath(fname);
  if (root.contains("Configuration") && root["Configuration"].is_object()) {
    const json& conf = root["Configuration"];
    m_model->setAnalysisType(analysisFromDomType(conf.value("domType", std::string("3D"))));
    m_model->m_thermal_coupling = conf.value("thermal", false);
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
        addFilePart(m_model, base_dir, block, false, fallback_id);
      ++fallback_id;
    }
  }

  if (root.contains("RigidBodies") && root["RigidBodies"].is_array()) {
    for (const json& rigid : root["RigidBodies"]) {
      const std::string type = rigid.value("type", std::string());
      if (type == "File")
        addFilePart(m_model, base_dir, rigid, true, fallback_id);
      else if (type == "Plane")
        addPlaneRigidPart(m_model, rigid, fallback_id);
      else
        std::cerr << "[InputReader] Unsupported rigid body type: " << type << std::endl;
      ++fallback_id;
    }
  }

  readBoundaryConditions(root, m_model);
  readInitialConditions(root, m_model);

  std::cout << "[InputReader] Input imported as editable model: " << fname << std::endl;
  return true;
}
