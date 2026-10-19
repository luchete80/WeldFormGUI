#include "io/ModelReader.h"
#include "io/ModelWriter.h"
#include "model/Model.h"
#include "model/Part.h"

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

int main()
{
  const auto nonce = std::chrono::high_resolution_clock::now().time_since_epoch().count();
  const fs::path directory = fs::temp_directory_path() /
      ("wfg_angular_motion_project_io_" + std::to_string(nonce));
  try {
    fs::create_directories(directory);
    testLegacyAndRoundTrip(directory);
    testAngularDefaults(directory);
    testMalformedAngularBlocksReject(directory);
    testInvalidAngularSaveRejects(directory);
    fs::remove_all(directory);
    std::cout << "Angular motion project IO tests passed.\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
