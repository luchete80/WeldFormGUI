#pragma once

#include "InitialStateSha256.h"
#include "InitialStateHistoryJson.h"
#include "model/InitialStateMaterialPolicy.h"
#include "model/Model.h"
#include "model/Node.h"
#include "model/Part.h"
#include "StrictJson.h"
#include "model/Mesh.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <charconv>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

using InitialStateSidecarJson = nlohmann::json;

inline void ValidateInitialStateTet4BdfCards(const std::filesystem::path& bdfPath,
                                             std::size_t expectedElementCount) {
  std::ifstream bdf(bdfPath);
  if (!bdf.is_open()) throw std::runtime_error("Cannot open sidecar BDF for Tet4 card validation.");
  std::string line;
  std::size_t tetraCount = 0;
  std::size_t lineNumber = 0;
  auto trimField = [](const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return std::string{};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
  };
  while (std::getline(bdf, line)) {
    ++lineNumber;
    const std::string trimmedLine = trimField(line);
    if (trimmedLine == "BEGIN BULK" || trimmedLine == "ENDDATA") continue;
    const std::string first = trimField(line.substr(0, std::min<std::size_t>(8, line.size())));
    if (first.empty() || first.front() == '$') continue;
    if (first == "+" || first == "*" || first.front() == '+' || first.front() == '*')
      throw std::runtime_error("Initial-state BDF contains an unsupported continuation/large-field card at line " +
                               std::to_string(lineNumber) + ".");
    if (first == "CTETRA") {
      if (line.size() > 56 && !trimField(line.substr(56)).empty())
        throw std::runtime_error("Initial-state BDF contains non-Tet4 CTETRA midside nodes at line " +
                                 std::to_string(lineNumber) + ".");
      ++tetraCount;
      continue;
    }
    if (first == "GRID" || first == "MAT1" || first == "PSOLID")
      continue;
    throw std::runtime_error("Initial-state BDF contains unsupported non-Tet4 card '" + first +
                             "' at line " + std::to_string(lineNumber) + ".");
  }
  if (bdf.bad()) throw std::runtime_error("Failed while reading sidecar BDF cards.");
  if (tetraCount != expectedElementCount)
    throw std::runtime_error("Initial-state BDF CTETRA card count does not match loaded mesh elements.");
}

inline void ApplyInitialStateSidecar(Mesh& mesh,
                                     const std::filesystem::path& sidecarPath,
                                     const std::filesystem::path& bdfPath,
                                     Model& model,
                                     Part& part) {
  namespace fs = std::filesystem;
  auto fail = [](const std::string& message) -> void { throw std::runtime_error(message); };
  std::ifstream input(sidecarPath);
  if (!input.is_open()) fail("Cannot open initial-state sidecar: " + sidecarPath.string());
  InitialStateSidecarJson state;
  try {
    state = ParseJsonRejectDuplicateKeys(input);
  } catch (const std::exception& error) {
    fail(std::string("Invalid initial-state sidecar JSON: ") + error.what());
  }

  const std::set<std::string> expectedTopKeys = {
      "format", "version", "mesh", "mesh_sha256", "element_fields"};
  if (!state.is_object() || state.size() != expectedTopKeys.size())
    fail("Initial-state sidecar must contain exactly the version 1 schema fields.");
  for (const auto& item : state.items())
    if (expectedTopKeys.find(item.key()) == expectedTopKeys.end())
      fail("Unsupported initial-state sidecar field: " + item.key());
  if (!state.at("format").is_string() || state.at("format").get<std::string>() != "weldform_initial_state" ||
      !InitialStateJsonInteger(state.at("version")) ||
      InitialStateJsonInt64(state.at("version"), "version") != 1)
    fail("Unsupported or malformed initial-state sidecar format/version.");
  if (!state.at("mesh").is_string() || state.at("mesh").get<std::string>().empty() ||
      !state.at("mesh_sha256").is_string())
    fail("Initial-state sidecar mesh path or SHA-256 field is malformed.");
  const std::string expectedHash = state.at("mesh_sha256").get<std::string>();
  if (expectedHash.size() != 64 || expectedHash.find_first_not_of("0123456789abcdef") != std::string::npos)
    fail("Initial-state sidecar SHA-256 must be 64 lowercase hexadecimal characters.");

  std::error_code pathError;
  const fs::path relativeMeshPath(state.at("mesh").get<std::string>());
  if (relativeMeshPath.is_absolute())
    fail("Initial-state sidecar mesh reference must be relative to the sidecar file.");
  const fs::path declaredMesh = fs::weakly_canonical(
      sidecarPath.parent_path() / relativeMeshPath, pathError);
  if (pathError) fail("Cannot resolve BDF path named by initial-state sidecar: " + pathError.message());
  const fs::path actualMesh = fs::weakly_canonical(bdfPath, pathError);
  if (pathError || declaredMesh != actualMesh)
    fail("Initial-state sidecar references a different BDF mesh than its DomainBlock.");
  std::string actualHash;
  try {
    actualHash = InitialStateSha256File(actualMesh.string());
  } catch (const std::exception& error) {
    fail(error.what());
  }
  if (actualHash != expectedHash)
    fail("Initial-state sidecar BDF SHA-256 does not match the referenced mesh.");

  const auto& fields = state.at("element_fields");
  if (!fields.is_object() || fields.size() != 1 || !fields.contains("pl_strain") ||
      !fields.at("pl_strain").is_object())
    fail("Initial-state sidecar v1 supports only the pl_strain element field.");
  if (mesh.getDim() != 3 || mesh.getElemCount() <= 0)
    fail("Initial-state sidecar requires a non-empty 3D Tet4 BDF mesh.");
  try {
    ValidateInitialStateTet4BdfCards(actualMesh, static_cast<std::size_t>(mesh.getElemCount()));
  } catch (const std::exception& error) {
    fail(error.what());
  }

  std::map<int, Element*> elementsById;
  std::set<int> nodeIds;
  for (int i = 0; i < mesh.getNodeCount(); ++i) {
    Node* node = mesh.getNode(i);
    if (node == nullptr || node->getId() <= 0 || node->getId() > 99999999 ||
        !std::isfinite(node->getPos(0)) || !std::isfinite(node->getPos(1)) ||
        !std::isfinite(node->getPos(2)) || !nodeIds.insert(node->getId()).second)
      fail("Sidecar BDF requires unique positive 8-column node IDs and finite coordinates.");
  }
  for (int i = 0; i < mesh.getElemCount(); ++i) {
    Element* element = mesh.getElem(i);
    if (element == nullptr || element->getNodeCount() != 4 || element->getId() <= 0 ||
        element->getId() > 99999999 || !elementsById.emplace(element->getId(), element).second)
      fail("Sidecar BDF requires unique positive 8-column EIDs and Tet4 elements only.");
    std::set<int> localNodeIds;
    for (int local = 0; local < element->getNodeCount(); ++local) {
      const int nodeId = element->getNodeId(local);
      if (nodeIds.find(nodeId) == nodeIds.end() || !localNodeIds.insert(nodeId).second)
        fail("Sidecar BDF Tet4 connectivity contains an absent or repeated node ID.");
    }
  }

  std::set<int> seenIds;
  std::vector<std::pair<Element*, double>> bufferedValues;
  bufferedValues.reserve(fields.at("pl_strain").size());
  for (const auto& item : fields.at("pl_strain").items()) {
    const std::string& key = item.key();
    int eid = 0;
    const auto parsed = std::from_chars(key.data(), key.data() + key.size(), eid);
    if (key.empty() || key.front() == '0' || parsed.ec != std::errc{} ||
        parsed.ptr != key.data() + key.size() || eid <= 0 || eid > 99999999 ||
        std::to_string(eid) != key || !seenIds.insert(eid).second)
      fail("Initial-state sidecar contains a non-canonical, duplicate or unsupported EID: " + key);
    if (!item.value().is_number())
      fail("Initial-state pl_strain values must be JSON numbers.");
    const double value = item.value().get<double>();
    if (!std::isfinite(value) || value < 0.0)
      fail("Initial-state pl_strain values must be finite and non-negative.");
    const auto element = elementsById.find(eid);
    if (element == elementsById.end())
      fail("Initial-state sidecar references an EID absent from its BDF mesh.");
    bufferedValues.emplace_back(element->second, value);
  }
  if (seenIds.size() != elementsById.size())
    fail("Initial-state sidecar must cover every BDF element EID exactly once.");

  const InitialStateMaterialResolution material =
      ResolveInitialStateMaterial(model, part.getSectionId(), part.getType());
  if (!material.partIsElastic || material.effectiveMaterial == nullptr ||
      material.fingerprint.empty())
    fail("Initial-state sidecar requires an elastic part and a resolvable current material.");

  // The input file explicitly opts into this state. Bind the state to the
  // material resolved for this new operation and keep source VTK provenance
  // unavailable, since sidecar v1 carries EIDs but no source-cell indices.
  mesh.clearSourceVtkCellElements();
  InitialStateMeshProvenance provenance;
  provenance.importedFromVtk = false;
  provenance.sourcePath = sidecarPath.string();
  provenance.hasPlasticStrainHistory = true;
  provenance.historyValid = true;
  provenance.historyDiagnostic = "Loaded from initial-state sidecar v1; original VTK source-cell IDs are not included.";
  provenance.historyMaterialDecisionMade = true;
  provenance.preserveHistoryWithMaterial = true;
  provenance.historyEffectiveMaterialIndex = material.effectiveMaterialIndex;
  provenance.historyMaterialFingerprint = material.fingerprint;
  provenance.historyDecisionSectionId = material.sectionId;
  provenance.historyDecisionSectionMaterialIndex = material.sectionMaterialIndex;
  provenance.historyDecisionPartWasElastic = material.partIsElastic;
  for (const auto& item : bufferedValues) {
    item.first->setInitialPlasticStrain(item.second);
    item.first->clearSourceVtkCellId();
  }
  mesh.setInitialStateProvenance(provenance);
}
