#pragma once

#include "model/Mesh.h"
#include "model/Node.h"
#include "InitialStateSha256.h"

#include <nlohmann/json.hpp>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <locale>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

inline bool ExportInitialStatePiece(
    Mesh& mesh,
    const std::filesystem::path& bdfPath,
    const std::filesystem::path& jsonPath,
    std::string* diagnostic = nullptr,
    std::map<const Element*, int>* elementIdsWritten = nullptr) {
  if (elementIdsWritten != nullptr) elementIdsWritten->clear();
  if (!mesh.validateInitialStateTopology()) {
    if (diagnostic != nullptr) *diagnostic = "Initial pl_strain history was invalidated by a topology change.";
    return false;
  }
  auto fail = [diagnostic](const std::string& message) {
    if (diagnostic != nullptr) *diagnostic = message;
    return false;
  };
  const auto& provenance = mesh.getInitialStateProvenance();
  if (mesh.getDim() != 3 || mesh.getElemCount() <= 0)
    return fail("Initial-state export requires a non-empty 3D Tet4 mesh.");
  if (!mesh.hasInitialPlasticStrainHistory() || !provenance ||
      !provenance->hasPlasticStrainHistory || !provenance->historyValid)
    return fail("Initial-state export requires validated pl_strain history.");
  if (!provenance->historyMaterialDecisionMade || !provenance->preserveHistoryWithMaterial)
    return fail("Initial-state export requires an explicit decision to preserve history with the current material.");
  if (bdfPath.empty() || jsonPath.empty() || bdfPath.filename().empty())
    return fail("Initial-state BDF and JSON paths must be explicit files.");
  std::error_code pathError;
  const std::filesystem::path absoluteBdf = std::filesystem::absolute(bdfPath, pathError).lexically_normal();
  if (pathError) return fail("Cannot resolve initial-state BDF path: " + bdfPath.string());
  const std::filesystem::path absoluteJson = std::filesystem::absolute(jsonPath, pathError).lexically_normal();
  if (pathError) return fail("Cannot resolve initial-state JSON path: " + jsonPath.string());
  if (absoluteBdf == absoluteJson)
    return fail("Initial-state JSON path must not overwrite the BDF mesh.");
  const std::filesystem::path relativeBdf =
      absoluteBdf.lexically_relative(absoluteJson.parent_path());
  if (relativeBdf.empty() || relativeBdf.is_absolute())
    return fail("BDF path cannot be represented relative to the initial-state JSON directory.");

  std::set<int> nodeIds;
  for (int i = 0; i < mesh.getNodeCount(); ++i) {
    Node* node = mesh.getNode(i);
    if (node == nullptr || node->getId() <= 0 || node->getId() > 99999999 ||
        !std::isfinite(node->getPos(0)) || !std::isfinite(node->getPos(1)) ||
        !std::isfinite(node->getPos(2)) || !nodeIds.insert(node->getId()).second)
      return fail("Initial-state BDF requires finite coordinates and unique positive node IDs that fit an 8-character field.");
  }

  std::map<const Element*, double> strainByElement;
  std::set<int> currentEids;
  for (int i = 0; i < mesh.getElemCount(); ++i) {
    const Element* element = mesh.getElem(i);
    if (element == nullptr || element->getNodeCount() != 4)
      return fail("Initial-state BDF supports Tet4 elements only.");
    const int currentEid = element->getId();
    if (currentEid <= 0 || currentEid > 99999999 || !currentEids.insert(currentEid).second)
      return fail("Initial-state BDF requires unique positive current EIDs that fit an 8-character field.");
    std::set<int> localNodeIds;
    for (int local = 0; local < element->getNodeCount(); ++local) {
      const int nodeId = element->getNodeId(local);
      if (nodeIds.find(nodeId) == nodeIds.end() || !localNodeIds.insert(nodeId).second)
        return fail("Tet4 connectivity contains an absent or repeated node ID.");
    }
    const auto& value = element->getInitialPlasticStrain();
    if (!value || !std::isfinite(*value) || *value < 0.0)
      return fail("Valid pl_strain history must cover every exported Tet4 with a finite non-negative value.");
    if (!strainByElement.emplace(element, *value).second)
      return fail("Mesh contains the same Tet4 element pointer more than once.");
  }

  std::map<const Element*, int> writtenElementIds;
  if (!mesh.exportToNASTRAN(bdfPath.string(), MeshNastranExportPurpose::InitialStatePackage,
                            &writtenElementIds))
    return fail("Failed to export the initial-state BDF.");
  if (writtenElementIds.size() != strainByElement.size())
    return fail("BDF writer did not emit every selected Tet4 element.");
  std::set<int> writtenEids;
  for (const auto& item : strainByElement) {
    const auto written = writtenElementIds.find(item.first);
    if (written == writtenElementIds.end() || written->second <= 0 || written->second > 99999999 ||
        !writtenEids.insert(written->second).second)
      return fail("BDF writer returned missing, duplicate or unsupported element IDs.");
  }

  try {
    const std::string bdfSha256 = InitialStateSha256File(bdfPath.string());
    nlohmann::json state = {
      {"format", "weldform_initial_state"},
      {"version", 1},
      {"mesh", relativeBdf.generic_string()},
      {"mesh_sha256", bdfSha256},
      {"element_fields", {{"pl_strain", nlohmann::json::object()}}}
    };
    for (const auto& item : strainByElement) {
      const int eid = writtenElementIds.at(item.first);
      state["element_fields"]["pl_strain"][std::to_string(eid)] = item.second;
    }
    std::ofstream output(jsonPath);
    if (!output.is_open()) return fail("Cannot open initial-state JSON for writing: " + jsonPath.string());
    output.imbue(std::locale::classic());
    output << state.dump(2) << '\n';
    output.flush();
    if (!output.good()) {
      output.close();
      return fail("Failed while writing initial-state JSON: " + jsonPath.string());
    }
    output.close();
    if (output.fail()) return fail("Failed while closing initial-state JSON: " + jsonPath.string());
  } catch (const std::exception& error) {
    return fail(error.what());
  }

  if (diagnostic != nullptr) diagnostic->clear();
  if (elementIdsWritten != nullptr) *elementIdsWritten = writtenElementIds;
  return true;
}
