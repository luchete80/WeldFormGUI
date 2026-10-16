#pragma once

#include "model/Mesh.h"

#include <nlohmann/json.hpp>

#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using InitialStateJson = nlohmann::json;

inline InitialStateJson SerializeInitialStateMesh(const Mesh& mesh) {
  InitialStateJson state = InitialStateJson::object();
  state["version"] = 1;
  const auto& optionalProvenance = mesh.getInitialStateProvenance();
  InitialStateMeshProvenance provenance = optionalProvenance
      ? *optionalProvenance : InitialStateMeshProvenance{};
  if (provenance.historyValid && !mesh.initialStateTopologyMatches()) {
    provenance.historyValid = false;
    provenance.historyDiagnostic =
        "La conectividad o el conjunto de entidades cambió después de asociar pl_strain; historia invalidada.";
  }
  const bool hasHistory = mesh.hasInitialPlasticStrainHistory();
  if (provenance.historyValid && (!hasHistory || mesh.getElemCount() == 0))
    throw std::runtime_error("Valid pl_strain history cannot be persisted without values on a non-empty mesh.");
  std::string historyDiagnostic = provenance.historyDiagnostic;
  bool omittedInvalidRecord = false;
  state["provenance"] = {
      {"importedFromVtk", provenance.importedFromVtk},
      {"sourcePath", provenance.sourcePath},
      {"hasPlasticStrainHistory", hasHistory},
      {"historyValid", provenance.historyValid},
      {"historyDiagnostic", historyDiagnostic},
      {"historyMaterialDecisionMade", provenance.historyMaterialDecisionMade},
      {"preserveHistoryWithMaterial", provenance.preserveHistoryWithMaterial},
      {"historyEffectiveMaterialIndex", provenance.historyEffectiveMaterialIndex},
      {"historyMaterialFingerprint", provenance.historyMaterialFingerprint},
      {"historyDecisionSectionId", provenance.historyDecisionSectionId},
      {"historyDecisionSectionMaterialIndex", provenance.historyDecisionSectionMaterialIndex},
      {"historyDecisionPartWasElastic", provenance.historyDecisionPartWasElastic}};
  if (provenance.sourcePartId)
    state["provenance"]["sourcePartId"] = *provenance.sourcePartId;

  state["pl_strain"] = InitialStateJson::array();
  if (!hasHistory) return state;

  std::set<int> elementIds;
  std::set<std::int64_t> sourceCellIds;
  for (int i = 0; i < mesh.getElemCount(); ++i) {
    const Element* element = mesh.getElem(i);
    if (element == nullptr)
      throw std::runtime_error("Initial-state mesh contains a null element.");
    const int elementId = element->getId();
    if (elementId <= 0 || !elementIds.insert(elementId).second)
      throw std::runtime_error("Initial-state EIDs must be positive and unique for project persistence.");
    const auto& value = element->getInitialPlasticStrain();
    if (!value) {
      if (provenance.historyValid)
        throw std::runtime_error("Valid pl_strain history must cover every current element EID.");
      omittedInvalidRecord = true;
      continue; // preserve explicitly invalid/incomplete history without inventing zeros
    }
    if (!std::isfinite(*value) || *value < 0.0) {
      if (provenance.historyValid)
        throw std::runtime_error("Cannot persist non-finite or negative pl_strain; no zero fallback is allowed.");
      omittedInvalidRecord = true;
      continue;
    }
    const auto& sourceCellId = element->getSourceVtkCellId();
    if ((!sourceCellId && provenance.importedFromVtk) ||
        (sourceCellId && (*sourceCellId < 0 || !sourceCellIds.insert(*sourceCellId).second))) {
      if (provenance.historyValid)
        throw std::runtime_error("Persisted VTK-imported pl_strain records require unique non-negative source VTK cell IDs.");
      omittedInvalidRecord = true;
      continue;
    }
    InitialStateJson record = {
        {"elementId", elementId},
        {"value", *value}};
    if (sourceCellId) record["sourceCellId"] = *sourceCellId;
    state["pl_strain"].push_back(std::move(record));
  }
  if (provenance.historyValid && elementIds.size() != state["pl_strain"].size())
    throw std::runtime_error("Valid pl_strain history does not have exact current EID coverage.");
  if (omittedInvalidRecord) {
    std::string& diagnostic = state["provenance"]["historyDiagnostic"].get_ref<std::string&>();
    if (!diagnostic.empty()) diagnostic += " ";
    diagnostic += "Invalid or unassociated pl_strain records were omitted from the project snapshot; history remains invalid.";
  }
  return state;
}

inline bool InitialStateJsonInteger(const InitialStateJson& value) {
  return value.is_number_integer() || value.is_number_unsigned();
}

inline std::int64_t InitialStateJsonInt64(const InitialStateJson& value, const char* fieldName) {
  if (!InitialStateJsonInteger(value))
    throw std::runtime_error(std::string("Initial-state integer field is invalid: ") + fieldName);
  if (value.is_number_unsigned()) {
    const std::uint64_t raw = value.get<std::uint64_t>();
    if (raw > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
      throw std::runtime_error(std::string("Initial-state integer field is out of range: ") + fieldName);
    return static_cast<std::int64_t>(raw);
  }
  return value.get<std::int64_t>();
}

inline int InitialStateJsonInt(const InitialStateJson& object, const char* key, int fallback) {
  if (!object.contains(key)) return fallback;
  const InitialStateJson& value = object.at(key);
  const std::int64_t raw = InitialStateJsonInt64(value, key);
  if (raw < std::numeric_limits<int>::min() || raw > std::numeric_limits<int>::max())
    throw std::runtime_error(std::string("Initial-state integer field is invalid: ") + key);
  return static_cast<int>(raw);
}

inline void ApplyInitialStateMeshJson(Mesh& mesh, const InitialStateJson& state) {
  if (!state.is_object() || !state.contains("version") ||
      !InitialStateJsonInteger(state.at("version")) ||
      InitialStateJsonInt64(state.at("version"), "version") != 1)
    throw std::runtime_error("Unsupported or malformed initial-state mesh version.");
  if (!state.contains("provenance") || !state.at("provenance").is_object() ||
      !state.contains("pl_strain") || !state.at("pl_strain").is_array())
    throw std::runtime_error("Initial-state mesh needs provenance and a pl_strain array.");

  const InitialStateJson& p = state.at("provenance");
  InitialStateMeshProvenance provenance;
  auto boolField = [&p](const char* key, bool fallback) {
    if (!p.contains(key)) return fallback;
    if (!p.at(key).is_boolean())
      throw std::runtime_error(std::string("Initial-state boolean field is invalid: ") + key);
    return p.at(key).get<bool>();
  };
  auto stringField = [&p](const char* key, const std::string& fallback) {
    if (!p.contains(key)) return fallback;
    if (!p.at(key).is_string())
      throw std::runtime_error(std::string("Initial-state string field is invalid: ") + key);
    return p.at(key).get<std::string>();
  };
  provenance.importedFromVtk = boolField("importedFromVtk", false);
  provenance.sourcePath = stringField("sourcePath", std::string{});
  provenance.hasPlasticStrainHistory = boolField("hasPlasticStrainHistory", false);
  provenance.historyValid = boolField("historyValid", false);
  provenance.historyDiagnostic = stringField("historyDiagnostic", std::string{});
  provenance.historyMaterialDecisionMade = boolField("historyMaterialDecisionMade", false);
  provenance.preserveHistoryWithMaterial = boolField("preserveHistoryWithMaterial", false);
  provenance.historyEffectiveMaterialIndex = InitialStateJsonInt(p, "historyEffectiveMaterialIndex", -1);
  provenance.historyMaterialFingerprint = stringField("historyMaterialFingerprint", std::string{});
  provenance.historyDecisionSectionId = InitialStateJsonInt(p, "historyDecisionSectionId", -1);
  provenance.historyDecisionSectionMaterialIndex =
      InitialStateJsonInt(p, "historyDecisionSectionMaterialIndex", -1);
  provenance.historyDecisionPartWasElastic = boolField("historyDecisionPartWasElastic", true);
  if (p.contains("sourcePartId"))
    provenance.sourcePartId = InitialStateJsonInt(p, "sourcePartId", -1);
  if (provenance.historyValid && !provenance.hasPlasticStrainHistory)
    throw std::runtime_error("Initial-state metadata cannot mark absent history as valid.");

  std::map<int, Element*> elementsById;
  for (int i = 0; i < mesh.getElemCount(); ++i) {
    Element* element = mesh.getElem(i);
    if (element == nullptr || element->getId() <= 0 ||
        !elementsById.emplace(element->getId(), element).second)
      throw std::runtime_error("Loaded mesh EIDs must be positive, canonical and unique before history attachment.");
  }

  std::set<int> seenElementIds;
  std::set<std::int64_t> seenSourceCellIds;
  struct Record { Element* element; int elementId; std::optional<std::int64_t> sourceCellId; double value; };
  std::vector<Record> records;
  records.reserve(state.at("pl_strain").size());
  for (const InitialStateJson& record : state.at("pl_strain")) {
    if (!record.is_object() || !record.contains("elementId") ||
        !InitialStateJsonInteger(record.at("elementId")) ||
        (record.contains("sourceCellId") && !InitialStateJsonInteger(record.at("sourceCellId"))) ||
        !record.contains("value") ||
        !record.at("value").is_number())
      throw std::runtime_error("Each pl_strain record requires integer elementId, optional integer sourceCellId and numeric value.");
    const std::int64_t rawElementId = InitialStateJsonInt64(record.at("elementId"), "elementId");
    std::optional<std::int64_t> sourceCellId;
    if (record.contains("sourceCellId")) {
      const std::int64_t rawSourceCellId = InitialStateJsonInt64(record.at("sourceCellId"), "sourceCellId");
      if (rawSourceCellId < 0)
        throw std::runtime_error("Initial-state source cell IDs must be non-negative.");
      sourceCellId = rawSourceCellId;
    }
    if (provenance.importedFromVtk && !sourceCellId)
      throw std::runtime_error("VTK-imported initial-state records require their source-cell provenance.");
    if (rawElementId <= 0 || rawElementId > std::numeric_limits<int>::max())
      throw std::runtime_error("Initial-state element IDs must be canonical positive EIDs.");
    const int elementId = static_cast<int>(rawElementId);
    const double value = record.at("value").get<double>();
    if (!std::isfinite(value) || value < 0.0)
      throw std::runtime_error("Initial-state pl_strain must be finite and non-negative.");
    const auto element = elementsById.find(elementId);
    if (element == elementsById.end() || !seenElementIds.insert(elementId).second ||
        (sourceCellId && !seenSourceCellIds.insert(*sourceCellId).second))
      throw std::runtime_error("Initial-state records contain an unknown or duplicated EID/source cell ID.");
    records.push_back({element->second, elementId, sourceCellId, value});
  }
  if (!provenance.hasPlasticStrainHistory && !records.empty())
    throw std::runtime_error("Discarded/no-history metadata cannot contain pl_strain values.");
  if (provenance.historyValid && elementsById.empty())
    throw std::runtime_error("Valid pl_strain history cannot be attached to an empty mesh.");
  if (provenance.historyValid &&
      (records.size() != elementsById.size() || seenElementIds.size() != elementsById.size()))
    throw std::runtime_error("Valid pl_strain history must cover every loaded mesh EID exactly once.");

  // Apply only after the entire structure and exact valid-history coverage pass.
  mesh.clearSourceVtkCellElements();
  for (const auto& element : elementsById) {
    element.second->clearInitialPlasticStrain();
    element.second->clearSourceVtkCellId();
  }
  for (const Record& record : records) {
    record.element->setInitialPlasticStrain(record.value);
    if (record.sourceCellId) {
      record.element->setSourceVtkCellId(*record.sourceCellId);
      mesh.registerSourceVtkCellElement(*record.sourceCellId, record.element);
    }
  }
  mesh.setInitialStateProvenance(provenance);
}
