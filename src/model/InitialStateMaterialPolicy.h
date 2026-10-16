#pragma once

#include "Material.h"
#include "Model.h"
#include "Part.h"
#include "Section.h"

#include <iomanip>
#include <fstream>
#include <cstdint>
#include <locale>
#include <sstream>
#include <string>
#include <vector>

struct InitialStateMaterialResolution {
  int effectiveMaterialIndex = -1;
  int sectionId = -1;
  int sectionMaterialIndex = -1;
  bool partIsElastic = false;
  const Material_* effectiveMaterial = nullptr;
  std::string fingerprint;
};

inline void AppendInitialStateFingerprintValues(std::ostringstream& out,
                                               const std::vector<double>& values) {
  out << values.size() << ':';
  for (double value : values) out << value << ',';
  out << ';';
}

inline std::string InitialStateMaterialFingerprint(const Material_* material) {
  if (material == nullptr) return {};
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out << std::setprecision(17) << std::boolalpha;
  out << "density=" << material->getDensityConstant()
      << ";model=" << material->Material_model
      << ";elastic=" << material->Elastic().E() << ',' << material->Elastic().nu()
      << ";plastic=" << material->isPlastic();

  const double scalarValues[] = {
      material->cs0, material->Ep, material->K, material->m, material->eps0,
      material->eps1, material->sy0, material->visc_relax_time, material->perzyna_m,
      material->T_t, material->T_m, material->T_min, material->T_max,
      material->e_min, material->e_max, material->er_min, material->er_max,
      material->A, material->B, material->C, material->n, material->eps_0,
      material->K_visco, material->n_visco, material->C1, material->C2,
      material->m1, material->m2, material->n1, material->n2, material->I1,
      material->I2, material->k_T, material->cp_T, material->exp_T,
      material->K_nh, material->m_nh, material->epsdot0, material->yieldStress0};
  AppendInitialStateFingerprintValues(out, std::vector<double>(scalarValues,
      scalarValues + sizeof(scalarValues) / sizeof(scalarValues[0])));
  AppendInitialStateFingerprintValues(out, material->strRange);
  out << "tabulated=" << material->tabulated_enabled << ','
      << material->tabulated_export_csv_reference << ',' << material->tableCsvPath << ';';
  AppendInitialStateFingerprintValues(out, material->tabulatedStrainGrid);
  AppendInitialStateFingerprintValues(out, material->tabulatedRateGrid);
  AppendInitialStateFingerprintValues(out, material->tabulatedTemperatureGrid);
  AppendInitialStateFingerprintValues(out, material->tabulatedStressValues);

  const Plastic_* plastic = material->getPlastic();
  if (plastic == nullptr) {
    out << "plasticParameters=none";
  } else {
    out << "plasticType=" << plastic->getType() << ";baseEt=" << plastic->Et << ';';
    switch (plastic->getType()) {
      case NONE:
      case BILINEAR:
      case HOLLOMON:
      case JOHNSON_COOK:
      case _GMT_:
      case NORTON_HOFF:
      case TABULATED:
        break;
      default:
        return {}; // Unknown serializer parameters cannot be fingerprinted safely.
    }
    AppendInitialStateFingerprintValues(out, plastic->getPlasticConstants());
  }
  if (material->tabulated_export_csv_reference && !material->tableCsvPath.empty()) {
    std::ifstream csv(material->tableCsvPath, std::ios::binary);
    if (!csv.is_open()) return {};
    std::uint64_t hash = UINT64_C(14695981039346656037);
    char byte = 0;
    while (csv.get(byte)) {
      hash ^= static_cast<unsigned char>(byte);
      hash *= UINT64_C(1099511628211);
    }
    if (csv.bad()) return {};
    out << "csvContentFnv64=" << hash << ';';
  }
  return out.str();
}

inline InitialStateMaterialResolution ResolveInitialStateMaterial(
    Model& model, int sectionId, Part_Type partType) {
  InitialStateMaterialResolution result;
  result.sectionId = sectionId;
  result.partIsElastic = partType == Elastic;
  if (sectionId >= 0) {
    for (int i = 0; i < model.getSectionCount(); ++i) {
      Section* section = model.getSection(i);
      if (section != nullptr && section->getId() == sectionId) {
        result.sectionMaterialIndex = section->getMaterialIndex();
        break;
      }
    }
  }

  // Current BDF/Engine writers attach MAT1 ID 1 to each exported mesh and
  // expose Material 0 as the effective constitutive law. Section material
  // links are retained in model metadata but are not wired into those writers.
  result.effectiveMaterialIndex = model.getMaterialCount() > 0 ? 0 : -1;
  if (result.effectiveMaterialIndex >= 0)
    result.effectiveMaterial = model.getMaterial(result.effectiveMaterialIndex);
  result.fingerprint = InitialStateMaterialFingerprint(result.effectiveMaterial);
  return result;
}

inline InitialStateMaterialResolution ResolveInitialStateMaterial(Model& model,
                                                                  Part& part) {
  return ResolveInitialStateMaterial(model, part.getSectionId(), part.getType());
}

inline bool IsInitialStateMaterialDecisionCurrent(
    const InitialStateMeshProvenance& provenance,
    const InitialStateMaterialResolution& current) {
  return provenance.historyValid && provenance.historyMaterialDecisionMade &&
         provenance.preserveHistoryWithMaterial && current.partIsElastic &&
         current.effectiveMaterial != nullptr && !current.fingerprint.empty() &&
         provenance.historyEffectiveMaterialIndex == current.effectiveMaterialIndex &&
         provenance.historyMaterialFingerprint == current.fingerprint &&
         provenance.historyDecisionSectionId == current.sectionId &&
         provenance.historyDecisionSectionMaterialIndex == current.sectionMaterialIndex &&
         provenance.historyDecisionPartWasElastic == current.partIsElastic;
}

inline std::string InitialStateHistoryExportIssue(Model& model, Part& part) {
  Mesh* mesh = part.getMesh();
  if (mesh == nullptr || !mesh->hasInitialPlasticStrainHistory()) return {};
  const auto& provenance = mesh->getInitialStateProvenance();
  if (!provenance || !IsInitialStateMaterialDecisionCurrent(
          *provenance, ResolveInitialStateMaterial(model, part))) {
    return "pl_strain history decision is missing or stale after material/section/part-type changes; reconfirm or discard it";
  }
  return {};
}
