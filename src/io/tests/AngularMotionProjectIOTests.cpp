#include "io/ModelReader.h"
#include "io/ModelWriter.h"
#include "io/InputWriter.h"
#include "io/InputReader.h"
#include "model/Model.h"
#include "model/Part.h"
#include "model/Step.h"

#include <nlohmann/json.hpp>

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using json = nlohmann::json;

void require(bool condition, const std::string &message)
{
  if (!condition)
    throw std::runtime_error(message);
}

json readJson(const fs::path &path)
{
  std::ifstream input(path);
  require(input.is_open(), "could not open generated project: " + path.string());
  json result;
  input >> result;
  return result;
}

void writeJson(const fs::path &path, const json &value)
{
  std::ofstream output(path);
  require(output.is_open(), "could not create project fixture: " + path.string());
  output << value.dump(2);
}

void setAngularAmplitude(BoundaryCondition *bc, const double3 &omega, const double3 &center,
                         double factor, const std::vector<double> &times,
                         const std::vector<double> &values)
{
  bc->setAngularMotionEnabled(true);
  bc->setAngularVelocity(omega);
  bc->setRotationCenter(center);
  bc->setAngularValueType(AmplitudeValue);
  bc->setAngularAmplitudeFactor(factor);
  bc->setAngularAmplitudeTable(times, values);
}

void testLegacyAndRoundTrip(const fs::path &directory)
{
  const fs::path legacyPath = directory / "legacy.wfmodel";
  writeJson(legacyPath, {
    {"Configuration", {{"analysisType", "Solid3D"}}},
    {"model", {{"parts", json::array({
      {{"id", 11}, {"name", "Rigid tool A"}, {"isRigid", true}},
      {{"id", 22}, {"name", "Rigid tool B"}, {"isRigid", true}}
    })}}},
    {"BoundaryConditions", json::array({{
      {"type", "VelocityBC"}, {"applyTo", "Part"}, {"targetId", 11},
      {"value", json::array({0.25, -0.5, 1.0})}, {"valueType", 1},
      {"amplitudeFactor", 0.6}, {"amplitudeTime", json::array({0.0, 2.0})},
      {"amplitudeValue", json::array({0.0, 1.0})}
    }})}
  });

  Model legacy;
  ModelReader legacyReader(&legacy);
  require(legacyReader.readFromFile(legacyPath.string()), "legacy project without angular fields should load");
  require(legacy.getBCCount() == 1 && !legacy.getBC(0)->hasAngularMotion(),
          "legacy linearly-only BC should load with angular block absent");
  require(legacy.getBC(0)->getValueX() == 0.25 && legacy.getBC(0)->getAmplitudeTime() ==
              std::vector<double>({0.0, 2.0}), "legacy linear values must remain intact");
  legacy.delBC(0);

  BoundaryCondition *first = new BoundaryCondition(VelocityBC, ApplyToPart, 11,
                                                    make_double3(1.0, 2.0, 3.0));
  first->setValueType(AmplitudeValue);
  first->setAmplitudeFactor(0.75);
  first->setAmplitudeTable({0.0, 1.5, 3.0}, {0.0, 0.4, 1.0});
  setAngularAmplitude(first, make_double3(0.0, 0.0, 2.5), make_double3(4.0, -2.0, 1.0),
                      1.25, {0.0, 0.8, 2.0}, {0.0, 0.6, 1.0});
  legacy.addBoundaryCondition(first);

  BoundaryCondition *second = new BoundaryCondition(VelocityBC, ApplyToPart, 22,
                                                     make_double3(-1.0, 0.5, 0.0));
  second->setValueType(AmplitudeValue);
  second->setAmplitudeFactor(2.0);
  second->setAmplitudeTable({0.0, 0.25, 1.0}, {1.0, 0.5, 0.0});
  setAngularAmplitude(second, make_double3(1.0, 0.0, -0.5), make_double3(-3.0, 5.0, 2.0),
                      0.4, {0.0, 0.5, 1.25}, {1.0, 0.3, -0.2});
  legacy.addBoundaryCondition(second);

  const fs::path generatedPath = directory / "angular_roundtrip.wfmodel";
  ModelWriter writer(legacy);
  require(writer.writeToFile(generatedPath.string()), "angular project should save");
  const json output = readJson(generatedPath);
  const json &writtenBcs = output.at("BoundaryConditions");
  require(writtenBcs.size() == 2, "both rigid-tool BCs should be saved");
  require(writtenBcs[0].at("valueAng") == json::array({0.0, 0.0, 2.5}), "first angular vector must save");
  require(writtenBcs[0].at("rotationCenter") == json::array({4.0, -2.0, 1.0}), "first center must save");
  require(writtenBcs[0].at("amplitudeTime") != writtenBcs[0].at("angularAmplitudeTime"),
          "linear and angular tables must remain independent");
  require(writtenBcs[1].at("amplitudeValue") != writtenBcs[1].at("angularAmplitudeValue"),
          "second linear and angular values must remain independent");

  Model roundTripped;
  ModelReader roundTripReader(&roundTripped);
  require(roundTripReader.readFromFile(generatedPath.string()), "generated angular project should load");
  require(roundTripped.getPartCount() == 2 && roundTripped.getPart(0)->getType() == Rigid &&
              roundTripped.getPart(1)->getType() == Rigid,
          "round-trip targets must remain the two rigid parts");
  require(roundTripped.getBCCount() == 2, "both BCs must round-trip");
  for (int i = 0; i < 2; ++i) {
    const BoundaryCondition *bc = roundTripped.getBC(i);
    require(bc->hasAngularMotion(), "angular block presence must round-trip");
    require(bc->usesAmplitude() && bc->usesAngularAmplitude(), "linear/angular modes must remain active");
    require(bc->getAmplitudeTime() != bc->getAngularAmplitudeTime(), "independent tables must round-trip");
  }
  const BoundaryCondition *roundTripFirst = roundTripped.getBC(0);
  require(roundTripFirst->getAngularAmplitudeFactor() == 1.25, "first angular factor must round-trip");
  require(roundTripFirst->getAngularAmplitudeTime() == std::vector<double>({0.0, 0.8, 2.0}),
          "first angular times must round-trip without normalization");
  require(roundTripFirst->getAngularAmplitudeValue() == std::vector<double>({0.0, 0.6, 1.0}),
          "first angular values must round-trip");
  require(roundTripFirst->getAmplitudeTime() == std::vector<double>({0.0, 1.5, 3.0}) &&
              roundTripFirst->getAmplitudeValue() == std::vector<double>({0.0, 0.4, 1.0}),
          "first linear table must round-trip independently");
  require(roundTripFirst->getRotationCenter().x == 4.0 && roundTripFirst->getRotationCenter().y == -2.0 &&
              roundTripFirst->getRotationCenter().z == 1.0, "nonzero center must round-trip");
  const BoundaryCondition *roundTripSecond = roundTripped.getBC(1);
  require(roundTripSecond->getAngularAmplitudeFactor() == 0.4 &&
              roundTripSecond->getAngularAmplitudeTime() == std::vector<double>({0.0, 0.5, 1.25}) &&
              roundTripSecond->getAngularAmplitudeValue() == std::vector<double>({1.0, 0.3, -0.2}),
          "second angular table and factor must round-trip independently");
  require(roundTripSecond->getAmplitudeTime() == std::vector<double>({0.0, 0.25, 1.0}) &&
              roundTripSecond->getAmplitudeValue() == std::vector<double>({1.0, 0.5, 0.0}),
          "second linear table must round-trip independently");
  require(roundTripSecond->getRotationCenter().x == -3.0 && roundTripSecond->getRotationCenter().y == 5.0 &&
              roundTripSecond->getRotationCenter().z == 2.0, "second nonzero center must round-trip");
}

void testAngularDefaults(const fs::path &directory)
{
  const fs::path path = directory / "angular_defaults.wfmodel";
  writeJson(path, {
    {"Configuration", {{"analysisType", "Solid3D"}}},
    {"BoundaryConditions", json::array({{
      {"type", "VelocityBC"}, {"applyTo", "Part"}, {"targetId", 11},
      {"valueAng", json::array({0.0, 0.0, 0.0})},
      {"rotationCenter", json::array({2.0, 3.0, 4.0})}
    }})}
  });
  Model model;
  ModelReader reader(&model);
  require(reader.readFromFile(path.string()), "constant angular block with omitted defaults should load");
  require(model.getBCCount() == 1 && model.getBC(0)->hasAngularMotion(),
          "constant angular block presence must be retained");
  require(model.getBC(0)->getAngularValueType() == ConstantValue &&
              model.getBC(0)->getAngularAmplitudeFactor() == 1.0,
          "omitted angular mode and factor must default to constant and one");
}

void testMalformedAngularBlocksReject(const fs::path &directory)
{
  const json base = {
    {"Configuration", {{"analysisType", "Solid3D"}}},
    {"BoundaryConditions", json::array({{
      {"type", "VelocityBC"}, {"applyTo", "Part"}, {"targetId", 11},
      {"value", json::array({0.0, 0.0, 0.0})}, {"valueAng", json::array({0.0, 0.0, 1.0})},
      {"rotationCenter", json::array({1.0, 2.0, 3.0})}, {"angularValueType", 1},
      {"angularAmplitudeTime", json::array({0.0, 1.0})},
      {"angularAmplitudeValue", json::array({0.0, 1.0})}
    }})}
  };
  const json invalidCases[] = {
    [] (json value) { value["BoundaryConditions"][0].erase("targetId"); return value; }(base),
    [] (json value) { value["BoundaryConditions"][0]["angularAmplitudeTime"] = {0.0, 0.0}; return value; }(base),
    [] (json value) { value["BoundaryConditions"][0]["angularAmplitudeTime"] = {-0.1, 1.0}; return value; }(base),
    [] (json value) { value["BoundaryConditions"][0]["angularAmplitudeValue"] = {0.0}; return value; }(base),
    [] (json value) { value["BoundaryConditions"][0]["valueAng"][1] = "bad"; return value; }(base),
    [] (json value) { value["BoundaryConditions"][0]["angularAmplitudeFactor"] = nullptr; return value; }(base)
  };
  int index = 0;
  for (const json &fixture : invalidCases) {
    const fs::path path = directory / ("invalid_angular_" + std::to_string(index++) + ".wfmodel");
    writeJson(path, fixture);
    Model model;
    ModelReader reader(&model);
    require(!reader.readFromFile(path.string()), "malformed angular block must reject project import");
  }
}

void testInvalidAngularSaveRejects(const fs::path &directory)
{
  Model model;
  BoundaryCondition *bc = new BoundaryCondition(VelocityBC, ApplyToPart, 11, make_double3(0.0, 0.0, 0.0));
  setAngularAmplitude(bc, make_double3(0.0, 0.0, 1.0), make_double3(0.0, 0.0, 0.0),
                      1.0, {0.0, 1.0}, {0.0, 1.0});
  bc->setAngularAmplitudeTable({0.0, 1.0, 1.0}, {0.0, 0.5, 1.0});
  model.addBoundaryCondition(bc);
  const fs::path output = directory / "invalid_save.wfmodel";
  ModelWriter writer(model);
  require(!writer.writeToFile(output.string()), "invalid angular table must reject project save");
  require(!fs::exists(output), "invalid angular table must not create a project file");
}

void testExportRejectsGlobalSetZoneAlias(const fs::path &directory)
{
  Model model;
  model.setAnalysisType(Solid3D);
  auto *step = new Step();
  step->setStepType(ImplicitStep);
  step->m_implicitFormulation = ImplicitFormulation::RigidViscoplastic;
  model.addStep(step);
  auto *rigidMesh = new Mesh();
  std::vector<Node*> rigidNodes = {new Node(0,0,0,1), new Node(1,0,0,2), new Node(0,1,0,3)};
  rigidMesh->assignValues(rigidNodes, {new Tria(rigidNodes[0], rigidNodes[1], rigidNodes[2])});
  rigidMesh->setDim(3);
  auto *rigid = new Part(rigidMesh);
  rigid->setId(41);
  rigid->setType(Rigid);
  model.addPart(rigid);
  auto *deformableMesh = new Mesh();
  std::vector<Node*> deformableNodes = {new Node(0,0,0,1), new Node(1,0,0,2), new Node(0,1,0,3)};
  deformableMesh->assignValues(deformableNodes, {new Tria(deformableNodes[0], deformableNodes[1], deformableNodes[2])});
  NodeSet alias(deformableMesh);
  alias.setEntityId(41);
  alias.add(deformableNodes[0]);
  deformableMesh->addNodeSet(alias);
  auto *deformable = new Part(deformableMesh);
  deformable->setId(9);
  deformable->setType(Elastic);
  model.addPart(deformable);
  auto *bc = new BoundaryCondition(VelocityBC, ApplyToPart, 41, make_double3(0,0,0));
  bc->setAngularMotionEnabled(true);
  bc->setAngularVelocity(make_double3(0,0,1));
  bc->setRotationCenter(make_double3(0,0,0));
  bc->setAngularValueType(ConstantValue);
  model.addBoundaryCondition(bc);
  InputWriter writer(&model);
  require(!writer.writeToFile((directory / "global_set_zone_alias.wfinput").string()),
          "a NodeSet on another Part must not alias a rigid zone during angular export");
}

void testSolverAngularRoundTrip(const fs::path &directory)
{
  Model model;
  model.setName("angular_solver_case");
  auto *step = new Step();
  step->setStepType(ImplicitStep);
  step->m_implicitFormulation = ImplicitFormulation::RigidViscoplastic;
  model.addStep(step);
  for (int id : {11, 22}) {
    auto *mesh = new Mesh();
    std::vector<Node*> nodes = {new Node(0,0,0,0), new Node(1,0,0,1), new Node(0,1,0,2)};
    mesh->assignValues(nodes, {new Tria(nodes[0], nodes[1], nodes[2])});
    mesh->setDim(3);
    auto *part = new Part(mesh);
    part->setId(id);
    part->setType(1);
    model.addPart(part);
    auto *bc = new BoundaryCondition(VelocityBC, ApplyToPart, id, make_double3(id, 0, 0));
    bc->setValueType(AmplitudeValue);
    bc->setAmplitudeFactor(0.5 + id * 0.01);
    bc->setAmplitudeTable({0.0, 1.0}, {0.0, 1.0});
    if (id == 11) setAngularAmplitude(bc, make_double3(0,0,2), make_double3(3,4,5), 1.25,
                                      {0.0, 0.5, 1.0}, {0.0, 0.4, 1.0});
    model.addBoundaryCondition(bc);
  }
  const fs::path inputPath = directory / "angular_solver.wfinput";
  InputWriter writer(&model);
  require(writer.writeToFile(inputPath.string()), "MINI3D angular input should export");
  const json root = readJson(inputPath);
  require(root.at("Amplitudes").size() == 3, "linear and angular amplitudes must all be exported");
  const auto &first = root.at("BoundaryConditions").at(0);
  require(first.at("amplitudeId") != first.at("angularAmplitudeId"), "linear/angular IDs must not collide");
  require(first.at("rotationCenter") == json::array({3.0, 4.0, 5.0}), "rotation center must export");
  require(!root.at("BoundaryConditions").at(1).contains("valueAng"), "linear-only BC must remain angular-free");
  Model imported;
  InputReader reader(&imported);
  require(reader.readFromFile(inputPath.string()), "MINI3D angular input should import");
  require(imported.getBCCount() == 2 && imported.getBC(0)->hasAngularMotion(), "angular BC should restore");
  require(imported.getBC(0)->getAngularAmplitudeTime() == std::vector<double>({0.0, 0.5, 1.0}) &&
          imported.getBC(0)->getRotationCenter().y == 4.0 && imported.getBC(0)->getAngularAmplitudeFactor() == 1.25,
          "angular table, center, and factor should restore independently");
}

void testSolverAngularDefaultsAndRejects(const fs::path &directory)
{
  Model source;
  source.setAnalysisType(Solid3D);
  auto *step = new Step();
  step->setStepType(ImplicitStep);
  source.addStep(step);
  auto *part = new Part();
  part->setId(41);
  part->setType(Rigid);
  auto *mesh = new Mesh();
  std::vector<Node*> nodes = {new Node(0,0,0,1), new Node(1,0,0,2), new Node(0,1,0,3)};
  mesh->assignValues(nodes, {new Tria(nodes[0], nodes[1], nodes[2])});
  mesh->setDim(3);
  part->setMesh(mesh);
  source.addPart(part);
  auto *bc = new BoundaryCondition(VelocityBC, ApplyToPart, 41, make_double3(0,0,0));
  bc->setAngularMotionEnabled(true);
  bc->setAngularVelocity(make_double3(0,0,2));
  bc->setRotationCenter(make_double3(4,5,6));
  bc->setAngularValueType(ConstantValue);
  source.addBoundaryCondition(bc);
  const fs::path path = directory / "angular_defaults.wfinput";
  InputWriter writer(&source);
  require(writer.writeToFile(path.string()), "constant angular fixture should export");
  json root = readJson(path);
  auto &jbc = root["BoundaryConditions"][0];
  jbc.erase("angularValueType");
  jbc["angularAmplitudeFactor"] = 1.0;
  writeJson(path, root);
  Model imported;
  InputReader reader(&imported);
  require(reader.readFromFile(path.string()), "omitted angular mode and default factor should import");
  require(imported.getBCCount() == 1 && imported.getBC(0)->hasAngularMotion() &&
          imported.getBC(0)->getAngularValueType() == ConstantValue &&
          imported.getBC(0)->getAngularAmplitudeFactor() == 1.0,
          "angular omitted defaults should resolve to constant and factor one");

  json ignoredLinearId = root;
  ignoredLinearId["BoundaryConditions"][0]["amplitudeId"] = 9;
  ignoredLinearId["BoundaryConditions"][0]["amplitudeFactor"] = 2.5;
  ignoredLinearId["Amplitudes"] = json::array({{{"id", 9}, {"time", {0.0, 1.0}}, {"value", {0.0, 1.0}}}});
  writeJson(path, ignoredLinearId);
  Model constantLinear;
  InputReader constantLinearReader(&constantLinear);
  require(constantLinearReader.readFromFile(path.string()) &&
          constantLinear.getBC(0)->getValueType() == ConstantValue,
          "linear amplitudeId is ignored when valueType is constant in the angular solver contract");

  const auto rejectImport = [&](json invalidRoot, const std::string &message) {
    writeJson(path, invalidRoot);
    Model invalidModel;
    InputReader invalidReader(&invalidModel);
    require(!invalidReader.readFromFile(path.string()), message);
  };

  json shortVector = root;
  shortVector["BoundaryConditions"][0]["value"] = {1.0, 2.0};
  rejectImport(shortVector, "short linear tool velocity vectors must reject import");
  json nonFiniteLinear = root;
  nonFiniteLinear["BoundaryConditions"][0]["value"][0] = nullptr;
  rejectImport(nonFiniteLinear, "non-finite linear tool velocity components must reject import");
  json shortAngular = root;
  shortAngular["BoundaryConditions"][0]["valueAng"] = {0.0, 0.0};
  rejectImport(shortAngular, "short angular vectors must reject import");

  json setAlias = root;
  setAlias["Sets"] = json::array({{{"id", 41}, {"nodes", {1, 2, 3}}}});
  rejectImport(setAlias, "Set IDs that alias rigid zones must reject angular import");

  json duplicateSameTool = root;
  json linearCopy = duplicateSameTool["BoundaryConditions"][0];
  for (const char *key : {"valueAng", "rotationCenter", "angularValueType",
                          "angularAmplitudeId", "angularAmplitudeFactor"}) linearCopy.erase(key);
  duplicateSameTool["BoundaryConditions"].push_back(linearCopy);
  rejectImport(duplicateSameTool, "angular plus linear movement on one tool must reject import");

  json duplicateOtherTool = root;
  json otherBody = duplicateOtherTool["RigidBodies"][0];
  otherBody["zoneId"] = 42;
  duplicateOtherTool["RigidBodies"].push_back(otherBody);
  json otherLinear = linearCopy;
  otherLinear["zoneId"] = 42;
  duplicateOtherTool["BoundaryConditions"].push_back(otherLinear);
  duplicateOtherTool["BoundaryConditions"].push_back(otherLinear);
  rejectImport(duplicateOtherTool, "two linear movements on a second tool must reject angular import");

  json duplicateTables = root;
  duplicateTables["BoundaryConditions"][0]["angularValueType"] = 1;
  duplicateTables["BoundaryConditions"][0]["angularAmplitudeId"] = 7;
  duplicateTables["BoundaryConditions"][0]["angularAmplitudeFactor"] = 1.0;
  duplicateTables["Amplitudes"] = json::array({
      {{"id", 7}, {"time", {0.0, 0.0}}, {"value", {0.0, 1.0}}},
      {{"id", 7}, {"time", {0.0, 1.0}}, {"value", {0.0, 1.0}}}});
  rejectImport(duplicateTables, "duplicate amplitude IDs must reject even if the first table is malformed");

  json hugeId = duplicateTables;
  hugeId["BoundaryConditions"][0]["angularAmplitudeId"] = std::uint64_t{4294967301ULL};
  rejectImport(hugeId, "out-of-range angular amplitude IDs must reject import");

  json rootAngularKey = root;
  rootAngularKey["valueAng"] = {1.0, 2.0, 3.0};
  rejectImport(rootAngularKey, "angular keys outside direct BC entries must reject import");

  json duplicateRigid = root;
  duplicateRigid["RigidBodies"].push_back(duplicateRigid["RigidBodies"][0]);
  rejectImport(duplicateRigid, "duplicate rigid tool IDs must reject angular import");

  json hugeToolId = duplicateOtherTool;
  hugeToolId["RigidBodies"][1]["zoneId"] = std::uint64_t{4294967301ULL};
  rejectImport(hugeToolId, "out-of-range other-tool zone IDs must reject angular import");

  for (const std::string &setting : {"restartFile", "checkpointEnabled", "checkpointStopTime", "checkpointInterval"}) {
    json restart = root;
    if (setting == "restartFile") restart["Configuration"]["solver"]["implicit"][setting] = "state.wfrst";
    else if (setting == "checkpointEnabled") restart["Configuration"]["solver"]["implicit"][setting] = true;
    else if (setting == "checkpointStopTime") restart["Configuration"]["solver"]["implicit"][setting] = 0.0;
    else restart["Configuration"]["solver"]["implicit"][setting] = 1;
    rejectImport(restart, "angular input restart/checkpoint settings must reject import");
  }

  json incompatibleSolver = root;
  incompatibleSolver["Configuration"]["solver"]["implicit"]["formulation"] = "j2";
  writeJson(path, incompatibleSolver);
  Model preserved;
  InputReader incompatibleReader(&preserved);
  require(incompatibleReader.readFromFile(path.string()) && preserved.getBC(0)->hasAngularMotion(),
          "incompatible solver selection must preserve imported project motion data");
  const fs::path incompatibleProject = directory / "incompatible_solver_preserved.wfmodel";
  ModelWriter projectWriter(preserved);
  require(projectWriter.writeToFile(incompatibleProject.string()), "angular project should save under alternate solver selection");
  InputWriter incompatibleWriter(&preserved);
  require(!incompatibleWriter.writeToFile((directory / "must_reject_j2.wfinput").string()),
          "angular solver export under J2 must reject");

  json legacyRoot = root;
  for (const char *key : {"valueAng", "rotationCenter", "angularValueType",
                          "angularAmplitudeId", "angularAmplitudeFactor"})
    legacyRoot["BoundaryConditions"][0].erase(key);
  const fs::path legacyInput = directory / "legacy_solver_input.wfinput";
  writeJson(legacyInput, legacyRoot);
  Model legacyModel;
  InputReader legacyReader(&legacyModel);
  require(legacyReader.readFromFile(legacyInput.string()), "legacy solver input without angular fields should load");
  const fs::path legacyReexport = directory / "legacy_solver_reexport.wfinput";
  InputWriter legacyWriter(&legacyModel);
  require(legacyWriter.writeToFile(legacyReexport.string()), "legacy solver input should re-export");
  const json legacyOutput = readJson(legacyReexport);
  require(!legacyOutput["BoundaryConditions"][0].contains("valueAng") &&
          legacyOutput["BoundaryConditions"][0]["zoneId"] == 41 &&
          legacyOutput["BoundaryConditions"][0]["value"] == legacyRoot["BoundaryConditions"][0]["value"],
          "legacy re-export should preserve linear semantics without angular fields");

  root["BoundaryConditions"][0].erase("zoneId");
  rejectImport(root, "angular block without a destination must reject import");
}

int main(int argc, char **argv)
{
  if (argc == 4 && std::string(argv[1]) == "--roundtrip-input") {
    Model imported;
    InputReader inputReader(&imported);
    if (!inputReader.readFromFile(argv[2])) return 10;
    const fs::path projectPath = fs::path(argv[3]).concat(".wfmodel");
    ModelWriter projectWriter(imported);
    if (!projectWriter.writeToFile(projectPath.string())) return 11;
    Model restored;
    ModelReader projectReader(&restored);
    if (!projectReader.readFromFile(projectPath.string())) return 12;
    InputWriter inputWriter(&restored);
    if (!inputWriter.writeToFile(argv[3])) return 13;
    return 0;
  }
  const auto nonce = std::chrono::high_resolution_clock::now().time_since_epoch().count();
  const fs::path directory = fs::temp_directory_path() /
      ("wfg_angular_motion_project_io_" + std::to_string(nonce));
  try {
    fs::create_directories(directory);
    testLegacyAndRoundTrip(directory);
    testAngularDefaults(directory);
    testMalformedAngularBlocksReject(directory);
    testInvalidAngularSaveRejects(directory);
    testExportRejectsGlobalSetZoneAlias(directory);
    testSolverAngularRoundTrip(directory);
    testSolverAngularDefaultsAndRejects(directory);
    fs::remove_all(directory);
    std::cout << "Angular motion project IO tests passed.\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
