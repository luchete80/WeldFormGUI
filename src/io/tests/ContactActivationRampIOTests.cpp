#include "io/InputReader.h"
#include "io/InputWriter.h"
#include "io/ModelReader.h"
#include "io/ModelWriter.h"
#include "model/Model.h"
#include "model/Material.h"
#include "model/Step.h"

#include <nlohmann/json.hpp>

#include <chrono>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;
using json = nlohmann::json;

void require(bool condition, const std::string& message)
{
  if (!condition)
    throw std::runtime_error(message);
}

json readJson(const fs::path& path)
{
  std::ifstream input(path);
  require(input.is_open(), "could not open generated JSON: " + path.string());
  json result;
  input >> result;
  return result;
}

void testDefaultsAndLegacyInput(const fs::path& directory)
{
  Model fresh;
  require(!fresh.contactProps().contactActivationRamp, "new cases must default the ramp to disabled");
  require(fresh.contactProps().contactActivationRampWidth == 0.001,
          "new cases must default ramp width to 0.001");

  const fs::path legacyPath = directory / "legacy.wfinput";
  {
    std::ofstream legacy(legacyPath);
    legacy << R"({"Configuration":{"domType":"3D"},"Contact":[{"fricCoeffStatic":0.2}]})";
  }
  Model imported;
  InputReader reader(&imported);
  require(reader.readFromFile(legacyPath.string()), "legacy input should load");
  require(!imported.contactProps().contactActivationRamp,
          "legacy input without the ramp flag must default it to false");
  require(imported.contactProps().contactActivationRampWidth == 0.001,
          "legacy input without the width must default it to 0.001");
}

void testEnabledRoundTripAndPreservation(const fs::path& directory)
{
  const fs::path legacyPath = directory / "legacy_for_roundtrip.wfinput";
  {
    std::ofstream legacy(legacyPath);
    legacy << R"({"Configuration":{"domType":"3D"},"Contact":[{}]})";
  }
  Model model;
  InputReader reader(&model);
  require(reader.readFromFile(legacyPath.string()), "input should load before round-trip");

  ContactProperties& contact = model.contactProps();
  contact.contactActivationRamp = true;
  contact.contactActivationRampWidth = 0.0025;
  InputWriter writer(&model);
  const fs::path enabledPath = directory / "enabled.wfinput";
  require(writer.writeToFile(enabledPath.string()), "enabled input should write");

  const json outputJson = readJson(enabledPath);
  const json& written = outputJson["Contact"][0];
  require(written.contains("contactActivationRamp"), "flag key must keep its exact name");
  require(written.contains("contactActivationRampWidth"), "width key must keep its exact name");
  require(written["contactActivationRamp"].is_boolean() && written["contactActivationRamp"].get<bool>(),
          "flag must serialize as JSON boolean true");
  require(written["contactActivationRampWidth"].is_number() &&
              std::abs(written["contactActivationRampWidth"].get<double>() - 0.0025) < 1e-15,
          "width must serialize as the exact JSON number");

  Model roundTripped;
  InputReader roundTripReader(&roundTripped);
  require(roundTripReader.readFromFile(enabledPath.string()), "generated input should load again");
  require(roundTripped.contactProps().contactActivationRamp, "enabled flag must round-trip");
  require(std::abs(roundTripped.contactProps().contactActivationRampWidth - 0.0025) < 1e-15,
          "width must round-trip");

  require(roundTripped.getStepCount() > 0, "loaded input should contain the active step");
  Step* implicitStep = roundTripped.getStep(0);
  implicitStep->setStepType(ImplicitStep);
  implicitStep->m_rigidModeStabilization = true;
  implicitStep->m_rigidModeStabilizationFactor = 0.002;
  implicitStep->m_rigidModeContactFade = false;
  implicitStep->m_rigidModeContactFadeScale = 1.5;
  InputWriter implicitWriter(&roundTripped);
  const fs::path implicitPath = directory / "enabled_implicit.wfinput";
  require(implicitWriter.writeToFile(implicitPath.string()), "implicit input should write");
  const json implicitJson = readJson(implicitPath);
  const json& implicitSolver = implicitJson["Configuration"]["solver"]["implicit"];
  require(implicitSolver["rigidModeStabilization"].get<bool>(),
          "implicit writer must export rigid-mode stabilization");
  require(std::abs(implicitSolver["rigidModeStabilizationFactor"].get<double>() - 0.002) < 1e-15,
          "implicit writer must export the rigid-mode factor");
  require(!implicitSolver["rigidModeContactFade"].get<bool>(),
          "implicit writer must export the contact-fade flag");
  require(std::abs(implicitSolver["rigidModeContactFadeScale"].get<double>() - 1.5) < 1e-15,
          "implicit writer must export the contact-fade scale");
  const json& implicitContact = implicitJson["Contact"][0];
  require(implicitContact["contactActivationRamp"].is_boolean() &&
              implicitContact["contactActivationRamp"].get<bool>(),
          "implicit writer must preserve the exact boolean field");
  require(implicitContact["contactActivationRampWidth"].is_number() &&
              std::abs(implicitContact["contactActivationRampWidth"].get<double>() - 0.0025) < 1e-15,
          "implicit writer must preserve the exact numeric field");
  Model implicitRoundTripped;
  InputReader implicitReader(&implicitRoundTripped);
  require(implicitReader.readFromFile(implicitPath.string()), "implicit input should load again");
  require(implicitRoundTripped.contactProps().contactActivationRamp &&
              std::abs(implicitRoundTripped.contactProps().contactActivationRampWidth - 0.0025) < 1e-15,
          "implicit input ramp settings must round-trip");
  require(implicitRoundTripped.getStepCount() > 0, "implicit round-trip must contain a step");
  const Step* importedStep = implicitRoundTripped.getStep(0);
  require(importedStep->m_rigidModeStabilization,
          "rigid-mode stabilization must round-trip");
  require(std::abs(importedStep->m_rigidModeStabilizationFactor - 0.002) < 1e-15,
          "rigid-mode factor must round-trip");
  require(!importedStep->m_rigidModeContactFade,
          "contact-fade flag must round-trip");
  require(std::abs(importedStep->m_rigidModeContactFadeScale - 1.5) < 1e-15,
          "contact-fade scale must round-trip");

  ContactProperties& roundTripContact = roundTripped.contactProps();
  roundTripContact.contactActivationRamp = false;
  require(roundTripContact.contactActivationRampWidth == 0.0025,
          "disabling the ramp must preserve its width");
  roundTripContact.contactActivationRamp = true;
  require(roundTripContact.contactActivationRampWidth == 0.0025,
          "re-enabling the ramp must preserve its width");
}

void testInvalidWidthsBlockWrites(const fs::path& directory)
{
  Model model;
  ContactProperties& contact = model.contactProps();
  contact.contactActivationRamp = true;
  InputWriter writer(&model);
  const double invalidWidths[] = {
    0.0,
    -0.001,
    std::numeric_limits<double>::infinity(),
    std::numeric_limits<double>::quiet_NaN()
  };
  int index = 0;
  for (double width : invalidWidths) {
    contact.contactActivationRampWidth = width;
    const fs::path output = directory / ("invalid_" + std::to_string(index++) + ".wfinput");
    require(!writer.writeToFile(output.string()), "invalid enabled width must reject input writing");
    require(!fs::exists(output), "invalid enabled width must not produce an input file");
  }
  contact.contactActivationRamp = false;
  contact.contactActivationRampWidth = 0.0;
  const fs::path disabledPath = directory / "disabled_invalid_width.wfinput";
  require(writer.writeToFile(disabledPath.string()), "invalid width is allowed while the ramp is disabled");
}

void testThermalCouplingPersistence(const fs::path& directory)
{
  const fs::path legacyInput = directory / "legacy_thermal.wfinput";
  {
    std::ofstream legacy(legacyInput);
    legacy << R"({"Configuration":{"domType":"3D","thermal":true},"Contact":[{}]})";
  }
  Model imported;
  InputReader inputReader(&imported);
  require(inputReader.readFromFile(legacyInput.string()), "legacy thermal input should load");
  require(std::abs(imported.getPlasticHeatFraction() - 0.9) < 1e-15,
          "legacy input must default plHeatFrac to 0.9");

  imported.setPlasticHeatFraction(0.73);
  Material_* thermalMaterial = new Material_(Elastic_(1000.0, 0.3));
  thermalMaterial->setDensityConstant(1.0);
  thermalMaterial->cp_T = 2.0;
  thermalMaterial->k_T = 0.5;
  imported.addMaterial(thermalMaterial);
  require(imported.getStepCount() > 0, "legacy input should create a step");
  imported.getStep(0)->setStepType(ImplicitStep);
  InputWriter inputWriter(&imported);
  const fs::path generatedInput = directory / "thermal.wfinput";
  require(inputWriter.writeToFile(generatedInput.string()), "thermal input should write");
  const json inputJson = readJson(generatedInput);
  require(std::abs(inputJson["Configuration"]["plHeatFrac"].get<double>() - 0.73) < 1e-15,
          "thermal input must serialize plHeatFrac");
  require(inputJson["Configuration"]["thermal"].get<bool>(),
          "thermal input must preserve the thermal flag");
  require(std::abs(inputJson["Materials"][0]["thermalHeatCap"].get<double>() - 2.0) < 1e-15,
          "thermal input must serialize heat capacity");
  require(std::abs(inputJson["Materials"][0]["thermalCond"].get<double>() - 0.5) < 1e-15,
          "thermal input must serialize conductivity");

  Model inputRoundTrip;
  InputReader inputRoundTripReader(&inputRoundTrip);
  require(inputRoundTripReader.readFromFile(generatedInput.string()), "thermal input should round-trip");
  require(std::abs(inputRoundTrip.getPlasticHeatFraction() - 0.73) < 1e-15,
          "thermal input plHeatFrac must round-trip");

  const fs::path legacyModel = directory / "legacy.wfmodel";
  {
    std::ofstream legacy(legacyModel);
    legacy << R"({"Configuration":{"analysisType":"Solid3D"}})";
  }
  Model modelImported;
  ModelReader modelReader(&modelImported);
  require(modelReader.readFromFile(legacyModel.string()), "legacy model should load");
  require(std::abs(modelImported.getPlasticHeatFraction() - 0.9) < 1e-15,
          "legacy model must default plHeatFrac to 0.9");

  modelImported.setPlasticHeatFraction(0.73);
  ModelWriter modelWriter(modelImported);
  const fs::path generatedModel = directory / "thermal.wfmodel";
  require(modelWriter.writeToFile(generatedModel.string()), "thermal model should write");
  Model modelRoundTrip;
  ModelReader modelRoundTripReader(&modelRoundTrip);
  require(modelRoundTripReader.readFromFile(generatedModel.string()), "thermal model should round-trip");
  require(std::abs(modelRoundTrip.getPlasticHeatFraction() - 0.73) < 1e-15,
          "thermal model plHeatFrac must round-trip");

  const double invalidValues[] = {-0.1, 1.1, std::numeric_limits<double>::quiet_NaN(),
                                  std::numeric_limits<double>::infinity()};
  for (double value : invalidValues) {
    modelImported.setPlasticHeatFraction(value);
    require(std::abs(modelImported.getPlasticHeatFraction() -
                     (std::isfinite(value) ? std::max(0.0, std::min(1.0, value)) : 0.9)) < 1e-15,
            "model setter must sanitize invalid plHeatFrac values");
  }
  modelImported.m_plastic_heat_fraction = -0.1;
  const fs::path invalidOutput = directory / "invalid_thermal.wfinput";
  require(!inputWriter.writeToFile(invalidOutput.string()),
          "invalid direct plHeatFrac state must block input writing");
}

int main()
{
  const auto nonce = std::chrono::high_resolution_clock::now().time_since_epoch().count();
  const fs::path directory = fs::temp_directory_path() /
      ("wfg_contact_activation_ramp_" + std::to_string(nonce));
  try {
    fs::create_directories(directory);
    testDefaultsAndLegacyInput(directory);
    testEnabledRoundTripAndPreservation(directory);
    testInvalidWidthsBlockWrites(directory);
    testThermalCouplingPersistence(directory);
    fs::remove_all(directory);
    std::cout << "Contact activation ramp IO tests passed.\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
