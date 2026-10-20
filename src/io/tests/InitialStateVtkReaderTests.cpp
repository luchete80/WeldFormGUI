#include "io/InitialStateVtkReader.h"
#include "io/InitialStateVtkMeshBuilder.h"
#include "model/InitialStateMaterialPolicy.h"
#include "model/Step.h"
#include "io/InitialStateHistoryJson.h"
#include "io/InitialStatePieceExporter.h"
#include "io/InitialStateSidecar.h"
#include "io/InputReader.h"
#include "io/InputWriter.h"
#include "io/ModelReader.h"
#include "io/ModelWriter.h"
#include "io/ExportGeneration.h"
#include "io/InitialStateSha256.h"
#include "io/StrictJson.h"
#include "model/NastranReader.h"
#include "model/Part.h"

#include <vtkDataArray.h>
#include <vtkDoubleArray.h>
#include <vtkStringArray.h>

#include <cmath>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
std::filesystem::path fixture(const char* name) {
  return std::filesystem::path(INITIAL_STATE_VTK_FIXTURE_DIR) / name;
}

void check(bool condition, const std::string& message) {
  if (!condition) throw std::runtime_error(message);
}

void testLegacyLoadsAllCellScalars() {
  auto mesh = ReadInitialStateVtkMesh(fixture("tetra_legacy.vtk"));
  check(mesh.tetCellIds.size() == 1, "legacy Tet4 count");
  check(mesh.surfaceCellIds.empty(), "legacy surface count");
  auto* first = mesh.dataset->GetCellData()->GetArray("pl_strain");
  auto* second = mesh.dataset->GetCellData()->GetArray("Part_ID");
  check(first && second, "legacy reader must retain every named CELL_DATA scalar");
  check(first->GetNumberOfTuples() == 1 && std::abs(first->GetComponent(0, 0) - 0.375) < 1e-12,
        "legacy pl_strain value");
  check(second->GetNumberOfTuples() == 1 && std::abs(second->GetComponent(0, 0) - 12.0) < 1e-12,
        "legacy Part_ID value");
}

void testXmlMixedTetAndSurface() {
  auto mesh = ReadInitialStateVtkMesh(fixture("mixed_tet_surface.vtu"));
  check(mesh.tetCellIds.size() == 1 && mesh.tetCellIds.front() == 0, "VTU Tet4 source index");
  check(mesh.surfaceCellIds.size() == 1 && mesh.surfaceCellIds.front() == 1, "VTU surface source index");
  auto* history = mesh.dataset->GetCellData()->GetArray("pl_strain");
  check(history && history->GetNumberOfTuples() == 2, "VTU cell field retained");
  check(std::abs(history->GetComponent(0, 0) - 0.25) < 1e-12, "VTU cell field value");
  const auto inspected = InspectInitialStateVtkPlStrain(mesh);
  check(inspected.status == InitialStateVtkHistoryStatus::Available,
        "scalar CELL_DATA pl_strain is accepted");
  check(inspected.valuesBySourceCellId.size() == 1 &&
        std::abs(inspected.valuesBySourceCellId.at(0) - 0.25) < 1e-12,
        "history values map to Tet4 source cell IDs only");
}

void testMissingAndPointDataHistoryAreExplicit() {
  auto missing = ReadInitialStateVtkMesh(fixture("mixed_tet_surface.vtu"));
  missing.dataset->GetCellData()->RemoveArray("pl_strain");
  auto missingHistory = InspectInitialStateVtkPlStrain(missing);
  check(missingHistory.status == InitialStateVtkHistoryStatus::Missing,
        "missing CELL_DATA pl_strain is not replaced with zero");

  auto nodal = ReadInitialStateVtkMesh(fixture("mixed_tet_surface.vtu"));
  nodal.dataset->GetCellData()->RemoveArray("pl_strain");
  auto nodalValues = vtkSmartPointer<vtkDoubleArray>::New();
  nodalValues->SetName("pl_strain");
  nodalValues->SetNumberOfComponents(1);
  nodalValues->SetNumberOfTuples(nodal.dataset->GetNumberOfPoints());
  nodal.dataset->GetPointData()->AddArray(nodalValues);
  auto nodalHistory = InspectInitialStateVtkPlStrain(nodal);
  check(nodalHistory.status == InitialStateVtkHistoryStatus::PointDataOnly,
        "POINT_DATA pl_strain is diagnosed and never used as elemental history");
}

void testVectorAndStringHistoryArraysAreRejected() {
  auto vectorMesh = ReadInitialStateVtkMesh(fixture("mixed_tet_surface.vtu"));
  vectorMesh.dataset->GetCellData()->RemoveArray("pl_strain");
  auto vectorField = vtkSmartPointer<vtkDoubleArray>::New();
  vectorField->SetName("pl_strain");
  vectorField->SetNumberOfComponents(3);
  vectorField->SetNumberOfTuples(vectorMesh.dataset->GetNumberOfCells());
  vectorMesh.dataset->GetCellData()->AddArray(vectorField);
  check(InspectInitialStateVtkPlStrain(vectorMesh).status == InitialStateVtkHistoryStatus::Invalid,
        "vector CELL_DATA pl_strain is rejected");

  auto stringMesh = ReadInitialStateVtkMesh(fixture("mixed_tet_surface.vtu"));
  stringMesh.dataset->GetCellData()->RemoveArray("pl_strain");
  auto stringField = vtkSmartPointer<vtkStringArray>::New();
  stringField->SetName("pl_strain");
  stringField->SetNumberOfComponents(1);
  stringField->SetNumberOfTuples(stringMesh.dataset->GetNumberOfCells());
  stringMesh.dataset->GetCellData()->AddArray(stringField);
  check(InspectInitialStateVtkPlStrain(stringMesh).status == InitialStateVtkHistoryStatus::Invalid,
        "string CELL_DATA pl_strain is rejected");
}

void testNegativeOrNonFiniteHistoryValuesAreRejected() {
  auto negativeMesh = ReadInitialStateVtkMesh(fixture("mixed_tet_surface.vtu"));
  negativeMesh.dataset->GetCellData()->GetArray("pl_strain")->SetComponent(0, 0, -0.1);
  check(InspectInitialStateVtkPlStrain(negativeMesh).status == InitialStateVtkHistoryStatus::Invalid,
        "negative Tet4 pl_strain is rejected");

  auto nonFiniteMesh = ReadInitialStateVtkMesh(fixture("mixed_tet_surface.vtu"));
  nonFiniteMesh.dataset->GetCellData()->GetArray("pl_strain")->SetComponent(
      0, 0, std::numeric_limits<double>::quiet_NaN());
  check(InspectInitialStateVtkPlStrain(nonFiniteMesh).status == InitialStateVtkHistoryStatus::Invalid,
        "nonfinite Tet4 pl_strain is rejected");
}

void testHistoryValidationIsScopedToSelectedCellsAndIgnoresSurfaceFiller() {
  auto mesh = ReadInitialStateVtkMesh(fixture("multipiece_history.vtu"));
  bool hasPartId = false;
  const auto groups = GroupInitialStateVtkTet4ByPartId(mesh, hasPartId);
  check(hasPartId && groups.size() == 2, "multipiece fixture has two Tet4 Part_ID groups");
  check(groups.at(10).size() == 1 && groups.at(10).front() == 0, "first deformable source group");
  check(groups.at(20).size() == 1 && groups.at(20).front() == 1, "second deformable source group");

  const auto validPiece = InspectInitialStateVtkPlStrain(mesh, groups.at(10));
  check(validPiece.status == InitialStateVtkHistoryStatus::Available &&
        validPiece.selectedTetCount == 1 && validPiece.hasRange &&
        std::abs(validPiece.minimum - 0.5) < 1e-12 &&
        std::abs(validPiece.maximum - 0.5) < 1e-12,
        "valid selected piece is accepted despite invalid unselected Tet4 and surface filler");

  const auto invalidPiece = InspectInitialStateVtkPlStrain(mesh, groups.at(20));
  check(invalidPiece.status == InitialStateVtkHistoryStatus::Invalid &&
        invalidPiece.diagnostic.find("celda VTK 1") != std::string::npos,
        "selecting the Tet4 with invalid history rejects history mode with source cell diagnostic");
}

void testSurfaceOnlyRejected() {
  bool rejected = false;
  try {
    (void)ReadInitialStateVtkMesh(fixture("surface_only.vtu"));
  } catch (const std::runtime_error& error) {
    rejected = std::string(error.what()).find("exclusivamente superficial") != std::string::npos;
  }
  check(rejected, "surface-only input must have a clear diagnostic");
}

void testModelMeshUsesFinalPointsWithoutAddingDisplacement() {
  auto vtkMesh = ReadInitialStateVtkMesh(fixture("mixed_tet_surface.vtu"));
  auto converted = ConvertInitialStateVtkTet4CellsToMesh(vtkMesh, {0});
  check(converted.mesh->getDim() == 3, "converted mesh dimension");
  check(converted.mesh->getNodeCount() == 4 && converted.mesh->getElemCount() == 1,
        "selected Tet4 creates a compact four-node mesh");
  check(converted.sourceCellIdToElement.size() == 1 &&
        converted.sourceCellIdToElement.at(0) == converted.mesh->getElem(0),
        "source cell maps directly to created model element");
  const double expected[4][3] = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
  for (int nodeIndex = 0; nodeIndex < 4; ++nodeIndex) {
    const Vector3f& position = converted.mesh->getNode(nodeIndex)->getPos();
    check(std::abs(position.x - expected[nodeIndex][0]) < 1e-12 &&
          std::abs(position.y - expected[nodeIndex][1]) < 1e-12 &&
          std::abs(position.z - expected[nodeIndex][2]) < 1e-12,
          "converted node must use final VTK Points without adding DISP");
    check(converted.mesh->getNode(nodeIndex)->getId() == nodeIndex + 1,
          "converted node IDs are positive and compact");
  }
  Element* element = converted.sourceCellIdToElement.at(0);
  check(element->getId() == 1 && element->getNodeCount() == 4,
        "converted Tet4 element has positive ID and four nodes");
}

void testElementStateAndSourceMappingSurviveCopyAndRemoval() {
  auto vtkMesh = ReadInitialStateVtkMesh(fixture("mixed_tet_surface.vtu"));
  auto converted = ConvertInitialStateVtkTet4CellsToMesh(vtkMesh, {0});
  Element* element = converted.mesh->findElementBySourceVtkCellId(0);
  check(element != nullptr, "mesh retains source-cell-to-element mapping");
  element->setInitialPlasticStrain(0.625);
  Element copied(*element);
  check(copied.getInitialPlasticStrain() &&
        std::abs(*copied.getInitialPlasticStrain() - 0.625) < 1e-12 &&
        copied.getSourceVtkCellId() && *copied.getSourceVtkCellId() == 0,
        "Element copy retains its own history value and source cell provenance");

  InitialStateMeshProvenance provenance;
  provenance.importedFromVtk = true;
  provenance.sourcePath = vtkMesh.sourcePath.string();
  provenance.sourcePartId = 12;
  provenance.hasPlasticStrainHistory = true;
  provenance.historyValid = true;
  provenance.historyDiagnostic = "validated test history";
  converted.mesh->setInitialStateProvenance(provenance);
  converted.mesh->adoptInitialStateStorage(
      std::move(converted.ownedNodes), std::move(converted.ownedElements));
  check(converted.mesh->hasInitialPlasticStrainHistory() &&
        converted.mesh->getInitialStateProvenance()->sourcePartId == 12,
        "mesh owns provenance and state independently from the VTK dataset");
  check(converted.mesh->removeElement(element), "imported element can be removed safely");
  check(converted.mesh->findElementBySourceVtkCellId(0) == nullptr,
        "removing an element removes its stale source-cell mapping");
}

void testInlineHistoryRoundTripUsesEIDAndPreservesCurrentGeometry() {
  auto source = ConvertInitialStateVtkTet4CellsToMesh(
      ReadInitialStateVtkMesh(fixture("mixed_tet_surface.vtu")), {0});
  Element* original = source.mesh->getElem(0);
  int sourceEid = 41;
  original->setId(sourceEid);
  original->setInitialPlasticStrain(0.75);
  original->setSourceVtkCellId(17);
  InitialStateMeshProvenance provenance;
  provenance.importedFromVtk = true;
  provenance.sourcePath = "missing/original/source.vtu";
  provenance.hasPlasticStrainHistory = true;
  provenance.historyValid = true;
  provenance.historyDiagnostic = "validated";
  provenance.historyMaterialDecisionMade = true;
  provenance.preserveHistoryWithMaterial = true;
  provenance.historyEffectiveMaterialIndex = 0;
  provenance.historyMaterialFingerprint = "snapshot";
  source.mesh->setInitialStateProvenance(provenance);

  const InitialStateJson serialized = SerializeInitialStateMesh(*source.mesh);
  auto reopened = ConvertInitialStateVtkTet4CellsToMesh(
      ReadInitialStateVtkMesh(fixture("mixed_tet_surface.vtu")), {0});
  int reopenedEid = 41;
  reopened.mesh->getElem(0)->setId(reopenedEid);
  reopened.mesh->getNode(0)->setPos(3.0, 4.0, 5.0);
  ApplyInitialStateMeshJson(*reopened.mesh, serialized);
  check(reopened.mesh->getElem(0)->getInitialPlasticStrain() &&
        std::abs(*reopened.mesh->getElem(0)->getInitialPlasticStrain() - 0.75) < 1e-12 &&
        reopened.mesh->findElementBySourceVtkCellId(17) == reopened.mesh->getElem(0),
        "inline state reattaches by sparse EID and restores source-cell provenance");
  check(reopened.mesh->getInitialStateProvenance()->sourcePath == "missing/original/source.vtu" &&
        reopened.mesh->getInitialStateProvenance()->historyMaterialDecisionMade,
        "provenance and material decision persist without opening the original VTK");
  const Vector3f& moved = reopened.mesh->getNode(0)->getPos();
  check(std::abs(moved.x - 3.0) < 1e-12 && std::abs(moved.y - 4.0) < 1e-12 &&
        std::abs(moved.z - 5.0) < 1e-12,
        "reopening state does not replace current BDF geometry with original VTK coordinates");

  const auto packageRoot = std::filesystem::temp_directory_path() /
      ("wf_initial_state_move_" + std::to_string(
          std::chrono::steady_clock::now().time_since_epoch().count()));
  const auto originalPackage = packageRoot / "original";
  const auto movedPackage = packageRoot / "moved";
  std::filesystem::create_directories(originalPackage);
  std::filesystem::copy_file(fixture("tetra_eid_41.bdf"), originalPackage / "part.bdf");
  {
    std::ofstream stateFile(originalPackage / "state.json");
    stateFile << serialized.dump(2);
  }
  std::filesystem::rename(originalPackage, movedPackage);
  InitialStateJson movedState;
  {
    std::ifstream stateFile(movedPackage / "state.json");
    movedState = ParseJsonRejectDuplicateKeys(stateFile);
  }
  Mesh movedMesh;
  movedMesh.genFromNastranFile((movedPackage / "part.bdf").string());
  ApplyInitialStateMeshJson(movedMesh, movedState);
  check(movedMesh.getElemCount() == 1 && movedMesh.getElem(0)->getId() == 41 &&
        movedMesh.getElem(0)->getInitialPlasticStrain() &&
        std::abs(*movedMesh.getElem(0)->getInitialPlasticStrain() - 0.75) < 1e-12,
        "moved project package reopens inline history from its BDF without the source VTK");
  std::error_code cleanupError;
  std::filesystem::remove_all(packageRoot, cleanupError);
}

void testInlineHistoryRejectsMalformedCoverageAndIntegerOverflow() {
  auto mesh = ConvertInitialStateVtkTet4CellsToMesh(
      ReadInitialStateVtkMesh(fixture("mixed_tet_surface.vtu")), {0});
  mesh.mesh->getElem(0)->setInitialPlasticStrain(0.5);
  mesh.mesh->getElem(0)->setSourceVtkCellId(0);
  InitialStateMeshProvenance provenance;
  provenance.hasPlasticStrainHistory = true;
  provenance.historyValid = true;
  mesh.mesh->setInitialStateProvenance(provenance);
  InitialStateJson state = SerializeInitialStateMesh(*mesh.mesh);
  state["version"] = std::uint64_t{4294967297ULL};
  bool rejectedVersion = false;
  try { ApplyInitialStateMeshJson(*mesh.mesh, state); }
  catch (const std::runtime_error&) { rejectedVersion = true; }
  check(rejectedVersion, "large unsigned version cannot wrap to supported version 1");

  state = SerializeInitialStateMesh(*mesh.mesh);
  state["pl_strain"].push_back(state["pl_strain"].front());
  bool rejectedDuplicate = false;
  try { ApplyInitialStateMeshJson(*mesh.mesh, state); }
  catch (const std::runtime_error&) { rejectedDuplicate = true; }
  check(rejectedDuplicate, "duplicate EID/source-cell record is rejected");
}

void testProjectJsonParserRejectsDuplicateKeys() {
  std::istringstream duplicate("{\"model\":{\"parts\":[],\"parts\":[]}}");
  bool rejected = false;
  try { (void)ParseJsonRejectDuplicateKeys(duplicate); }
  catch (const std::exception&) { rejected = true; }
  check(rejected, "project JSON duplicate keys are rejected before DOM overwrite");
}

void testNastranReaderAndMeshPreserveSparseReorderedEids() {
  const auto path = fixture("tetra_sparse_eids.bdf");
  NastranReader reader(path.string().c_str());
  check(reader.isReadValid() && reader.getElemCount() == 2 &&
        reader.getElementIds()[0] == 27 && reader.getElementIds()[1] == 5,
        "Nastran reader preserves sparse, reordered EIDs");
  Mesh mesh;
  mesh.genFromNastranFile(path.string());
  check(mesh.getElemCount() == 2 && mesh.getElem(0)->getId() == 27 &&
        mesh.getElem(1)->getId() == 5,
        "project BDF reader retains EIDs required by inline history lookup");
}

void testNastranReaderReportsOpenFailure() {
  const auto path = std::filesystem::temp_directory_path() /
      ("wfg_missing_nastran_" + std::to_string(
          std::chrono::steady_clock::now().time_since_epoch().count()) + ".bdf");
  NastranReader reader(path.string().c_str());
  check(!reader.isReadValid(), "Nastran reader reports an input file that cannot be opened");
}

void testNastranReaderOpensReadOnlyBdf() {
  const auto source = fixture("tetra_sparse_eids.bdf");
  const auto path = std::filesystem::temp_directory_path() /
      ("wfg_readonly_nastran_" + std::to_string(
          std::chrono::steady_clock::now().time_since_epoch().count()) + ".bdf");
  std::filesystem::copy_file(source, path);
  std::filesystem::permissions(path,
      std::filesystem::perms::owner_read | std::filesystem::perms::group_read |
      std::filesystem::perms::others_read, std::filesystem::perm_options::replace);
  NastranReader reader(path.string().c_str());
  check(reader.isReadValid() && reader.getNodeCount() == 4 && reader.getElemCount() == 2,
        "Nastran reader loads BDF files with read-only permissions");
  std::filesystem::permissions(path, std::filesystem::perms::owner_all,
                               std::filesystem::perm_options::replace);
  std::filesystem::remove(path);
}

void testInitialStateSha256KnownVectors() {
  check(InitialStateSha256Hex("") ==
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
        "SHA-256 empty-string vector");
  check(InitialStateSha256Hex("abc") ==
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
        "SHA-256 abc vector");
}

void testInitialStatePackageUsesWrittenSparseEidsAndBdfHash() {
  const auto root = std::filesystem::temp_directory_path() /
      ("wf_initial_state_export_" + std::to_string(
          std::chrono::steady_clock::now().time_since_epoch().count()));
  std::filesystem::create_directories(root);
  Mesh first;
  first.genFromNastranFile(fixture("tetra_sparse_eids.bdf").string());
  check(first.getElemCount() == 2, "sparse-ID export fixture loads");
  first.getElem(0)->setInitialPlasticStrain(0.25);
  first.getElem(0)->setSourceVtkCellId(90);
  first.getElem(1)->setInitialPlasticStrain(0.75);
  first.getElem(1)->setSourceVtkCellId(13);
  InitialStateMeshProvenance provenance;
  provenance.importedFromVtk = true;
  provenance.hasPlasticStrainHistory = true;
  provenance.historyValid = true;
  provenance.historyMaterialDecisionMade = true;
  provenance.preserveHistoryWithMaterial = true;
  provenance.historyEffectiveMaterialIndex = 0;
  provenance.historyMaterialFingerprint = "material-test";
  first.setInitialStateProvenance(provenance);

  std::map<const Element*, int> writtenIds;
  std::string diagnostic;
  const auto bdf1 = root / "piece_a.bdf";
  const auto json1 = root / "piece_a.initial_state.json";
  check(ExportInitialStatePiece(first, bdf1, json1, &diagnostic, &writtenIds),
        "initial-state piece export succeeds: " + diagnostic);
  check(writtenIds.size() == 2 && writtenIds.at(first.getElem(0)) == 27 &&
        writtenIds.at(first.getElem(1)) == 5,
        "export helper returns its explicit Element-to-written-EID map");
  nlohmann::json sidecar;
  {
    std::ifstream input(json1);
    input >> sidecar;
  }
  check(sidecar["format"] == "weldform_initial_state" && sidecar["version"] == 1 &&
        sidecar["mesh"] == "piece_a.bdf" &&
        sidecar["mesh_sha256"] == InitialStateSha256File(bdf1.string()),
        "sidecar records the BDF basename and hash of closed bytes");
  check(sidecar["element_fields"]["pl_strain"]["27"] == 0.25 &&
        sidecar["element_fields"]["pl_strain"]["5"] == 0.75,
        "sidecar maps heterogeneous state to sparse EIDs actually written");

  Mesh second;
  second.genFromNastranFile(fixture("tetra_sparse_eids.bdf").string());
  second.getElem(0)->setInitialPlasticStrain(0.9);
  second.getElem(0)->setSourceVtkCellId(4);
  second.getElem(1)->setInitialPlasticStrain(0.1);
  second.getElem(1)->setSourceVtkCellId(8);
  second.setInitialStateProvenance(provenance);
  const auto bdf2 = root / "piece_b.bdf";
  const auto json2 = root / "piece_b.initial_state.json";
  check(ExportInitialStatePiece(second, bdf2, json2, &diagnostic),
        "second piece export accepts repeated local EIDs");
  nlohmann::json secondSidecar;
  {
    std::ifstream input(json2);
    input >> secondSidecar;
  }
  check(secondSidecar["element_fields"]["pl_strain"]["27"] == 0.9 &&
        secondSidecar["element_fields"]["pl_strain"]["5"] == 0.1,
        "separate piece sidecars keep repeated local EIDs isolated");

  int oversizedEid = 100000000;
  first.getElem(0)->setId(oversizedEid);
  const auto invalidJson = root / "invalid.initial_state.json";
  check(!ExportInitialStatePiece(first, root / "invalid.bdf", invalidJson, &diagnostic) &&
        !std::filesystem::exists(invalidJson),
        "EID wider than the BDF eight-column field blocks sidecar publication");
  std::error_code cleanupError;
  std::filesystem::remove_all(root, cleanupError);
}

void testMaterialDecisionFingerprintAndDiscardPolicy() {
  Material_ material(Elastic_(210000.0, 0.3));
  material.setDensityConstant(7.8);
  const std::string originalFingerprint = InitialStateMaterialFingerprint(&material);
  check(!originalFingerprint.empty(), "material fingerprint is available");
  material.setDensityConstant(7.9);
  check(InitialStateMaterialFingerprint(&material) != originalFingerprint,
        "editing the same material invalidates its snapshot fingerprint");

  Bilinear bilinearA(250.0, 15.0);
  Bilinear bilinearB(250.0, 25.0);
  Material_ bilinearMaterialA(Elastic_(210000.0, 0.3));
  Material_ bilinearMaterialB(Elastic_(210000.0, 0.3));
  bilinearMaterialA.m_plastic = &bilinearA;
  bilinearMaterialB.m_plastic = &bilinearB;
  bilinearMaterialA.m_isplastic = bilinearMaterialB.m_isplastic = true;
  bilinearMaterialA.Material_model = bilinearMaterialB.Material_model = BILINEAR;
  check(InitialStateMaterialFingerprint(&bilinearMaterialA) !=
        InitialStateMaterialFingerprint(&bilinearMaterialB),
        "Bilinear plastic tangent Et used by material serializers is fingerprinted");
  Bilinear bilinearBaseEtA(250.0, 3.0);
  bilinearMaterialA.m_plastic = &bilinearBaseEtA;
  const std::string baseEtFingerprint = InitialStateMaterialFingerprint(&bilinearMaterialA);
  Bilinear bilinearBaseEtB(250.0, 4.0);
  bilinearMaterialA.m_plastic = &bilinearBaseEtB;
  check(InitialStateMaterialFingerprint(&bilinearMaterialA) != baseEtFingerprint,
        "Bilinear base Et used by the Engine input serializer is fingerprinted");

  InitialStateMeshProvenance provenance;
  provenance.historyValid = true;
  provenance.historyMaterialDecisionMade = true;
  provenance.preserveHistoryWithMaterial = true;
  provenance.historyEffectiveMaterialIndex = 0;
  provenance.historyMaterialFingerprint = "material-v1";
  provenance.historyDecisionSectionId = 4;
  provenance.historyDecisionSectionMaterialIndex = 0;
  provenance.historyDecisionPartWasElastic = true;
  InitialStateMaterialResolution current;
  current.effectiveMaterialIndex = 0;
  current.sectionId = 4;
  current.sectionMaterialIndex = 0;
  current.partIsElastic = true;
  current.effectiveMaterial = &material;
  current.fingerprint = "material-v1";
  check(IsInitialStateMaterialDecisionCurrent(provenance, current),
        "matching material, section and part type retain an explicit decision");
  current.fingerprint = "material-v2";
  check(!IsInitialStateMaterialDecisionCurrent(provenance, current),
        "editing the material invalidates the decision");
  current.fingerprint = "material-v1";
  current.sectionMaterialIndex = 1;
  check(!IsInitialStateMaterialDecisionCurrent(provenance, current),
        "changing Section to Material assignment invalidates the decision");
  current.sectionMaterialIndex = 0;
  current.partIsElastic = false;
  check(!IsInitialStateMaterialDecisionCurrent(provenance, current),
        "a rigid part cannot consume the preserved plastic history");

  auto vtkMesh = ReadInitialStateVtkMesh(fixture("mixed_tet_surface.vtu"));
  auto converted = ConvertInitialStateVtkTet4CellsToMesh(vtkMesh, {0});
  Element* element = converted.mesh->getElem(0);
  element->setInitialPlasticStrain(0.4);
  InitialStateMeshProvenance history;
  history.importedFromVtk = true;
  history.hasPlasticStrainHistory = true;
  history.historyValid = true;
  converted.mesh->setInitialStateProvenance(history);
  converted.mesh->discardInitialPlasticStrainHistory();
  check(!converted.mesh->hasInitialPlasticStrainHistory() &&
        !element->getInitialPlasticStrain().has_value() &&
        converted.mesh->getInitialStateProvenance()->historyMaterialDecisionMade &&
        !converted.mesh->getInitialStateProvenance()->preserveHistoryWithMaterial,
        "explicit discard clears element values and records the discard decision");
}

void testExportGenerationReusesDirectoryAndCleansNewDirectoryOnFailure() {
  namespace fs = std::filesystem;
  const auto root = fs::temp_directory_path() /
      ("wf_export_generation_" + std::to_string(
          std::chrono::steady_clock::now().time_since_epoch().count()));
  fs::create_directories(root);
  fs::path owned;
  {
    ExportGenerationDirectory generation;
    std::string diagnostic;
    check(generation.create(root, "case", &diagnostic),
          "generation allocation succeeds: " + diagnostic);
    owned = generation.path();
    check(owned.filename().string().rfind(".wf_export_", 0) == 0 && fs::is_directory(owned),
          "first export uses the stable generation directory");
    std::ofstream staged(owned / "partial.txt");
    staged << "discard on failure";
  }
  check(!fs::exists(owned), "failed first export removes only the directory it created");
  fs::create_directories(owned);
  {
    std::ofstream previous(owned / "old_mesh.bdf");
    previous << "old export";
  }
  {
    ExportGenerationDirectory generation;
    std::string diagnostic;
    check(generation.create(root, "case", &diagnostic),
          "subsequent export reuses generation directory: " + diagnostic);
    check(generation.path() == owned && fs::is_empty(owned),
          "subsequent export clears stale resources in the stable directory");
    std::ofstream current(generation.path() / "case_part_1.bdf");
    current << "current export";
    generation.retain();
  }
  check(fs::exists(owned / "case_part_1.bdf"),
        "successful export retains the resource at the stable filename");

  const auto published = root / "input.json";
  {
    std::ofstream old(published);
    old << "previous generation";
  }
  nlohmann::json value = {{"generation", 2}};
  std::string diagnostic;
  check(!WriteJsonAndPublish(value, root / "missing" / "temporary.json", published,
                             &diagnostic),
        "publication fails if its temporary file cannot be opened");
  std::ifstream old(published);
  std::string contents;
  std::getline(old, contents);
  check(contents == "previous generation", "failed publication preserves the old reference file");
  std::error_code cleanupError;
  fs::remove_all(root, cleanupError);
}

void testSecondPieceFailurePreservesPublishedReferenceAndCleansGeneration() {
  namespace fs = std::filesystem;
  const auto root = fs::temp_directory_path() /
      ("wf_export_second_piece_" + std::to_string(
          std::chrono::steady_clock::now().time_since_epoch().count()));
  fs::create_directories(root);
  const auto published = root / "input.json";
  {
    std::ofstream old(published);
    old << "previous input";
  }
  fs::path generationPath;
  {
    ExportGenerationDirectory generation;
    std::string diagnostic;
    check(generation.create(root, "input", &diagnostic),
          "staged multi-piece generation allocates: " + diagnostic);
    generationPath = generation.path();

    Mesh valid;
    valid.genFromNastranFile(fixture("tetra_sparse_eids.bdf").string());
    InitialStateMeshProvenance provenance;
    provenance.importedFromVtk = true;
    provenance.hasPlasticStrainHistory = true;
    provenance.historyValid = true;
    provenance.historyMaterialDecisionMade = true;
    provenance.preserveHistoryWithMaterial = true;
    provenance.historyEffectiveMaterialIndex = 0;
    provenance.historyMaterialFingerprint = "test";
    valid.setInitialStateProvenance(provenance);
    for (int i = 0; i < valid.getElemCount(); ++i) {
      valid.getElem(i)->setInitialPlasticStrain(0.1 * (i + 1));
      valid.getElem(i)->setSourceVtkCellId(i);
    }
    check(ExportInitialStatePiece(valid, generation.path() / "part_1.bdf",
                                   generation.path() / "part_1.initial_state.json",
                                   &diagnostic),
          "first piece is fully staged before later-piece failure: " + diagnostic);

    Mesh invalid;
    invalid.genFromNastranFile(fixture("tetra_sparse_eids.bdf").string());
    invalid.setInitialStateProvenance(provenance);
    int oversizedId = 100000000;
    invalid.getElem(0)->setId(oversizedId);
    for (int i = 0; i < invalid.getElemCount(); ++i) {
      invalid.getElem(i)->setInitialPlasticStrain(0.2);
      invalid.getElem(i)->setSourceVtkCellId(10 + i);
    }
    check(!ExportInitialStatePiece(invalid, generation.path() / "part_2.bdf",
                                   generation.path() / "part_2.initial_state.json",
                                   &diagnostic),
          "invalid second piece aborts the generation");
  }
  std::ifstream old(published);
  std::string contents;
  std::getline(old, contents);
  check(contents == "previous input" && !fs::exists(generationPath),
        "second-piece failure leaves old input intact and removes only new staged resources");
  std::error_code cleanupError;
  fs::remove_all(root, cleanupError);
}

void testInputSidecarRoundTripRelativeHashAndInlinePersistence() {
  namespace fs = std::filesystem;
  const auto root = fs::temp_directory_path() /
      ("wf_input_sidecar_" + std::to_string(
          std::chrono::steady_clock::now().time_since_epoch().count()));
  const auto package = root / "package";
  fs::create_directories(package);

  Mesh source;
  source.genFromNastranFile(fixture("tetra_sparse_eids.bdf").string());
  InitialStateMeshProvenance sourceProvenance;
  sourceProvenance.importedFromVtk = true;
  sourceProvenance.hasPlasticStrainHistory = true;
  sourceProvenance.historyValid = true;
  sourceProvenance.historyMaterialDecisionMade = true;
  sourceProvenance.preserveHistoryWithMaterial = true;
  sourceProvenance.historyEffectiveMaterialIndex = 0;
  sourceProvenance.historyMaterialFingerprint = "source-material";
  source.setInitialStateProvenance(sourceProvenance);
  source.getElem(0)->setInitialPlasticStrain(0.125);
  source.getElem(0)->setSourceVtkCellId(88);
  source.getElem(1)->setInitialPlasticStrain(0.875);
  source.getElem(1)->setSourceVtkCellId(12);

  std::string diagnostic;
  const auto bdf = package / "piece.bdf";
  const auto sidecar = package / "piece.initial_state.json";
  check(ExportInitialStatePiece(source, bdf, sidecar, &diagnostic),
        "input sidecar fixture export: " + diagnostic);
  const auto movedPackage = root / "moved";
  fs::rename(package, movedPackage);

  Model model;
  Material_ material(Elastic_(210000.0, 0.3));
  material.setDensityConstant(7.8);
  model.addMaterial(&material);
  Part part;
  part.setType(Elastic);
  Mesh reopened;
  reopened.genFromNastranFile((movedPackage / "piece.bdf").string());
  ApplyInitialStateSidecar(reopened, movedPackage / "piece.initial_state.json",
                           movedPackage / "piece.bdf", model, part);
  check(reopened.getElem(0)->getInitialPlasticStrain() &&
        std::abs(*reopened.getElem(0)->getInitialPlasticStrain() - 0.125) < 1e-12 &&
        reopened.getElem(1)->getInitialPlasticStrain() &&
        std::abs(*reopened.getElem(1)->getInitialPlasticStrain() - 0.875) < 1e-12,
        "moved relative BDF/sidecar package reloads heterogeneous values by EID");
  check(!reopened.getElem(0)->getSourceVtkCellId() &&
        reopened.getInitialStateProvenance() &&
        !reopened.getInitialStateProvenance()->importedFromVtk,
        "sidecar import does not invent original VTK cell provenance");

  const InitialStateJson inlineState = SerializeInitialStateMesh(reopened);
  check(!inlineState["pl_strain"][0].contains("sourceCellId"),
        "project inline persistence allows unavailable VTK cell provenance");
  Mesh projectReopened;
  projectReopened.genFromNastranFile((movedPackage / "piece.bdf").string());
  ApplyInitialStateMeshJson(projectReopened, inlineState);
  check(projectReopened.getElemCount() == 2 &&
        std::abs(*projectReopened.getElem(1)->getInitialPlasticStrain() - 0.875) < 1e-12 &&
        !projectReopened.getElem(1)->getSourceVtkCellId(),
        "inline project roundtrip preserves sidecar history without fabricating source-cell IDs");

  InitialStateSidecarJson malformed;
  {
    std::ifstream input(movedPackage / "piece.initial_state.json");
    malformed = ParseJsonRejectDuplicateKeys(input);
  }
  malformed["element_fields"]["pl_strain"].erase("5");
  const auto incomplete = movedPackage / "incomplete.json";
  {
    std::ofstream output(incomplete);
    output << malformed.dump(2);
  }
  Mesh untouched;
  untouched.genFromNastranFile((movedPackage / "piece.bdf").string());
  bool rejectedCoverage = false;
  try {
    ApplyInitialStateSidecar(untouched, incomplete, movedPackage / "piece.bdf", model, part);
  } catch (const std::runtime_error&) { rejectedCoverage = true; }
  check(rejectedCoverage && !untouched.hasInitialPlasticStrainHistory() &&
        !untouched.getElem(0)->getInitialPlasticStrain(),
        "incomplete sidecar coverage is rejected before modifying the mesh");

  {
    std::ofstream duplicate(movedPackage / "duplicate.json");
    duplicate << "{\"format\":\"weldform_initial_state\",\"format\":\"weldform_initial_state\"}";
  }
  Mesh duplicateMesh;
  duplicateMesh.genFromNastranFile((movedPackage / "piece.bdf").string());
  bool rejectedDuplicate = false;
  try {
    ApplyInitialStateSidecar(duplicateMesh, movedPackage / "duplicate.json",
                             movedPackage / "piece.bdf", model, part);
  } catch (const std::runtime_error&) { rejectedDuplicate = true; }
  check(rejectedDuplicate && !duplicateMesh.hasInitialPlasticStrainHistory(),
        "duplicate sidecar keys are rejected before mesh mutation");

  {
    std::ofstream duplicateEid(movedPackage / "duplicate_eid.json");
    duplicateEid << "{\"format\":\"weldform_initial_state\",\"version\":1,"
                    "\"mesh\":\"piece.bdf\",\"mesh_sha256\":\""
                 << InitialStateSha256File((movedPackage / "piece.bdf").string())
                 << "\",\"element_fields\":{\"pl_strain\":{\"27\":0.1,\"27\":0.2}}}";
  }
  Mesh duplicateEidMesh;
  duplicateEidMesh.genFromNastranFile((movedPackage / "piece.bdf").string());
  bool rejectedDuplicateEid = false;
  try {
    ApplyInitialStateSidecar(duplicateEidMesh, movedPackage / "duplicate_eid.json",
                             movedPackage / "piece.bdf", model, part);
  } catch (const std::runtime_error&) { rejectedDuplicateEid = true; }
  check(rejectedDuplicateEid && !duplicateEidMesh.hasInitialPlasticStrainHistory(),
        "duplicate EID keys are rejected by strict sidecar parsing");

  {
    std::ifstream validInput(movedPackage / "piece.initial_state.json");
    malformed = ParseJsonRejectDuplicateKeys(validInput);
    malformed["element_fields"]["pl_strain"]["05"] = 0.5;
    std::ofstream output(movedPackage / "noncanonical_eid.json");
    output << malformed.dump(2);
  }
  Mesh noncanonicalMesh;
  noncanonicalMesh.genFromNastranFile((movedPackage / "piece.bdf").string());
  bool rejectedNoncanonical = false;
  try {
    ApplyInitialStateSidecar(noncanonicalMesh, movedPackage / "noncanonical_eid.json",
                             movedPackage / "piece.bdf", model, part);
  } catch (const std::runtime_error&) { rejectedNoncanonical = true; }
  check(rejectedNoncanonical && !noncanonicalMesh.hasInitialPlasticStrainHistory(),
        "leading-zero EID keys are rejected as non-canonical");

  {
    std::ofstream input(movedPackage / "piece.initial_state.json");
    input << "{\"format\":\"weldform_initial_state\",\"version\":1,\"mesh\":\"piece.bdf\","
             "\"mesh_sha256\":\"" << std::string(64, '0') << "\",\"element_fields\":{"
             "\"pl_strain\":{\"27\":0.125,\"5\":0.875}}}";
  }
  Mesh hashMismatch;
  hashMismatch.genFromNastranFile((movedPackage / "piece.bdf").string());
  bool rejectedHash = false;
  try {
    ApplyInitialStateSidecar(hashMismatch, movedPackage / "piece.initial_state.json",
                             movedPackage / "piece.bdf", model, part);
  } catch (const std::runtime_error&) { rejectedHash = true; }
  check(rejectedHash && !hashMismatch.hasInitialPlasticStrainHistory(),
        "BDF hash mismatch rejects state before mesh mutation");

  std::ifstream originalBdf(movedPackage / "piece.bdf");
  std::ostringstream originalBytes;
  originalBytes << originalBdf.rdbuf();
  const std::vector<std::pair<std::string, bool>> invalidBdfs = {
      {"CQUAD4  100     1       1       2       3       4       \n", false},
      {std::string(8, ' ') + "1", true}};
  for (const auto& invalidCase : invalidBdfs) {
    std::string bytes = originalBytes.str();
    if (invalidCase.second) {
      const std::size_t position = bytes.find("CTETRA  ");
      check(position != std::string::npos, "fixture contains CTETRA card");
      const std::size_t lineEnd = bytes.find('\n', position);
      bytes.insert(lineEnd, invalidCase.first);
    } else {
      bytes += invalidCase.first;
    }
    const std::string stem = invalidCase.second ? "quadratic" : "mixed";
    const auto invalidBdf = movedPackage / (stem + ".bdf");
    const auto invalidSidecar = movedPackage / (stem + ".initial_state.json");
    {
      std::ofstream output(invalidBdf);
      output << bytes;
    }
    InitialStateSidecarJson invalidState = {
      {"format", "weldform_initial_state"}, {"version", 1},
      {"mesh", invalidBdf.filename().string()},
      {"mesh_sha256", InitialStateSha256File(invalidBdf.string())},
      {"element_fields", {{"pl_strain", {{"27", 0.125}, {"5", 0.875}}}}}
    };
    {
      std::ofstream output(invalidSidecar);
      output << invalidState.dump(2);
    }
    Mesh invalidMesh;
    invalidMesh.genFromNastranFile(invalidBdf.string());
    bool rejectedCard = false;
    try {
      ApplyInitialStateSidecar(invalidMesh, invalidSidecar, invalidBdf, model, part);
    } catch (const std::runtime_error&) { rejectedCard = true; }
    check(rejectedCard && !invalidMesh.hasInitialPlasticStrainHistory(),
          invalidCase.second ? "CTETRA10 midside nodes are rejected before state attachment"
                             : "mixed CQUAD4 is rejected before state attachment");
  }
  std::error_code cleanupError;
  fs::remove_all(root, cleanupError);
}

void testTopologySnapshotPreservesTransformsAndBlocksEdits() {
  namespace fs = std::filesystem;
  const auto root = fs::temp_directory_path() /
      ("wf_initial_state_topology_" + std::to_string(
          std::chrono::steady_clock::now().time_since_epoch().count()));
  fs::create_directories(root);
  Mesh mesh;
  mesh.genFromNastranFile(fixture("tetra_sparse_eids.bdf").string());
  for (int i = 0; i < mesh.getElemCount(); ++i)
    mesh.getElem(i)->setInitialPlasticStrain(0.2 + 0.1 * i);
  InitialStateMeshProvenance provenance;
  provenance.hasPlasticStrainHistory = true;
  provenance.historyValid = true;
  provenance.historyMaterialDecisionMade = true;
  provenance.preserveHistoryWithMaterial = true;
  mesh.setInitialStateProvenance(provenance);

  const double originalValue = *mesh.getElem(0)->getInitialPlasticStrain();
  const auto firstBdf = root / "first.bdf";
  const auto firstJson = root / "first.initial_state.json";
  std::string diagnostic;
  check(ExportInitialStatePiece(mesh, firstBdf, firstJson, &diagnostic),
        "topology snapshot initial export: " + diagnostic);
  const std::string firstHash = InitialStateSha256File(firstBdf.string());

  for (int i = 0; i < mesh.getNodeCount(); ++i)
    mesh.getNode(i)->translate(3.0, -2.0, 1.0);
  check(mesh.rotateNodesAroundAxis(90.0, Vector3f(0.0, 0.0, 1.0),
                                   Vector3f(0.0, 0.0, 0.0)),
        "rigid rotation helper accepts a finite nonzero global axis");
  int renumberedNodeId = mesh.getNode(0)->getId() + 1000;
  int renumberedElementId = mesh.getElem(0)->getId() + 1000;
  mesh.getNode(0)->setId(renumberedNodeId);
  mesh.getElem(0)->setId(renumberedElementId);
  check(mesh.validateInitialStateTopology(),
        "coordinate transforms and ID renumbering preserve pointer topology");
  const auto movedBdf = root / "moved.bdf";
  const auto movedJson = root / "moved.initial_state.json";
  check(ExportInitialStatePiece(mesh, movedBdf, movedJson, &diagnostic),
        "transformed Tet4 still exports history: " + diagnostic);
  check(InitialStateSha256File(movedBdf.string()) != firstHash &&
        std::abs(*mesh.getElem(0)->getInitialPlasticStrain() - originalValue) < 1e-12,
        "translation/rotation change BDF hash while preserving the element scalar");

  std::vector<Node*> changedConnectivity = {
      mesh.getNode(0), mesh.getNode(0), mesh.getNode(2), mesh.getNode(3)};
  mesh.getElem(0)->initValues(changedConnectivity);
  check(!mesh.validateInitialStateTopology() && mesh.getInitialStateProvenance() &&
        !mesh.getInitialStateProvenance()->historyValid,
        "direct connectivity edits invalidate the history snapshot");
  check(!ExportInitialStatePiece(mesh, root / "bad.bdf", root / "bad.json", &diagnostic),
        "invalidated topology cannot publish an input sidecar");
  const InitialStateJson persistedInvalid = SerializeInitialStateMesh(mesh);
  check(!persistedInvalid["provenance"]["historyValid"].get<bool>() &&
        !persistedInvalid["provenance"]["historyDiagnostic"].get<std::string>().empty(),
        "invalid topology and its diagnostic persist to project JSON");

  Mesh replacement;
  Part part(&mesh);
  check(!part.setMesh(&replacement) && part.getMesh() == &mesh,
        "Part refuses to replace a mesh that still carries history");
  check(!part.deleteMesh() && part.getMesh() == &mesh,
        "Part refuses to delete history even after topology invalidation");

  Mesh removed;
  removed.genFromNastranFile(fixture("tetra_sparse_eids.bdf").string());
  for (int i = 0; i < removed.getElemCount(); ++i)
    removed.getElem(i)->setInitialPlasticStrain(0.3);
  removed.setInitialStateProvenance(provenance);
  check(removed.removeElement(removed.getElem(0)) && !removed.validateInitialStateTopology(),
        "element removal invalidates history rather than reassigning values");
  std::error_code cleanupError;
  fs::remove_all(root, cleanupError);
}

void testMini3dInputWriterReaderAndProjectRoundTrip() {
  namespace fs = std::filesystem;
  const auto root = fs::temp_directory_path() /
      ("wf_mini3d_initial_state_io_" + std::to_string(
          std::chrono::steady_clock::now().time_since_epoch().count()));
  fs::create_directories(root);

  Model source;
  source.setName("initial_state_case");
  auto* material = new Material_(Elastic_(210000.0, 0.3));
  material->setDensityConstant(7.8);
  source.addMaterial(material);
  auto* step = new Step();
  step->setStepType(ImplicitStep);
  step->m_implicitFormulation = ImplicitFormulation::RigidViscoplastic;
  source.addStep(step);
  auto* mesh = new Mesh();
  mesh->genFromNastranFile(fixture("tetra_sparse_eids.bdf").string());
  InitialStateMeshProvenance provenance;
  provenance.importedFromVtk = true;
  provenance.hasPlasticStrainHistory = true;
  provenance.historyValid = true;
  provenance.historyMaterialDecisionMade = true;
  provenance.preserveHistoryWithMaterial = true;
  provenance.historyEffectiveMaterialIndex = 0;
  provenance.historyMaterialFingerprint = InitialStateMaterialFingerprint(material);
  provenance.historyDecisionPartWasElastic = true;
  mesh->setInitialStateProvenance(provenance);
  mesh->getElem(0)->setInitialPlasticStrain(0.2);
  mesh->getElem(0)->setSourceVtkCellId(100);
  mesh->getElem(1)->setInitialPlasticStrain(0.8);
  mesh->getElem(1)->setSourceVtkCellId(101);
  auto* part = new Part(mesh);
  part->setId(1);
  source.addPart(part);
  const auto currentMaterial = ResolveInitialStateMaterial(source, *part);
  provenance.historyMaterialFingerprint = currentMaterial.fingerprint;
  provenance.historyEffectiveMaterialIndex = currentMaterial.effectiveMaterialIndex;
  provenance.historyDecisionSectionId = currentMaterial.sectionId;
  provenance.historyDecisionSectionMaterialIndex = currentMaterial.sectionMaterialIndex;
  mesh->setInitialStateProvenance(provenance);

  const auto inputPath = root / "mini3d.wfinput";
  InputWriter inputWriter(&source);
  check(inputWriter.writeToFile(inputPath.string()),
        "MINI3D input writer exports initial-state sidecar");
  const auto inputJson = [&]() {
    std::ifstream input(inputPath);
    return ParseJsonRejectDuplicateKeys(input);
  }();
  const auto& block = inputJson.at("DomainBlocks").at(0);
  check(block.contains("initialStateFile") && block.at("initialStateFile").is_string() &&
        fs::path(block.at("initialStateFile").get<std::string>()).is_relative(),
        "MINI3D DomainBlock references a relative initialStateFile");
  const fs::path sidecarPath = inputPath.parent_path() /
      block.at("initialStateFile").get<std::string>();
  check(fs::is_regular_file(sidecarPath), "published input sidecar reference resolves");

  Model imported;
  InputReader inputReader(&imported);
  check(inputReader.readFromFile(inputPath.string()), "MINI3D input reader applies sidecar history");
  check(imported.getPartCount() == 1 && imported.getPart(0)->getMesh()->hasInitialPlasticStrainHistory() &&
        std::abs(*imported.getPart(0)->getMesh()->getElem(1)->getInitialPlasticStrain() - 0.8) < 1e-12,
        "InputReader restores per-EID history from the referenced package");

  imported.setName("mini3d_project_roundtrip");
  const auto projectPath = root / "mini3d.wfmodel";
  ModelWriter modelWriter(imported);
  check(modelWriter.writeToFile(projectPath.string()), "project writer persists imported sidecar history inline");
  Model project;
  ModelReader modelReader(&project);
  check(modelReader.readFromFile(projectPath.string()), "project reader restores inline initial state");
  check(project.getPartCount() == 1 && project.getPart(0)->getMesh()->hasInitialPlasticStrainHistory() &&
        std::abs(*project.getPart(0)->getMesh()->getElem(0)->getInitialPlasticStrain() - 0.2) < 1e-12 &&
        !project.getPart(0)->getMesh()->getElem(0)->getSourceVtkCellId(),
        "InputWriter/Reader and ModelWriter/Reader preserve values without inventing VTK provenance");

  std::error_code cleanupError;
  fs::remove_all(root, cleanupError);
}
} // namespace

int main() {
  try {
    testLegacyLoadsAllCellScalars();
    testXmlMixedTetAndSurface();
    testMissingAndPointDataHistoryAreExplicit();
    testVectorAndStringHistoryArraysAreRejected();
    testNegativeOrNonFiniteHistoryValuesAreRejected();
    testHistoryValidationIsScopedToSelectedCellsAndIgnoresSurfaceFiller();
    testSurfaceOnlyRejected();
    testModelMeshUsesFinalPointsWithoutAddingDisplacement();
    testElementStateAndSourceMappingSurviveCopyAndRemoval();
    testInlineHistoryRoundTripUsesEIDAndPreservesCurrentGeometry();
    testInlineHistoryRejectsMalformedCoverageAndIntegerOverflow();
    testProjectJsonParserRejectsDuplicateKeys();
    testNastranReaderAndMeshPreserveSparseReorderedEids();
    testNastranReaderReportsOpenFailure();
    testNastranReaderOpensReadOnlyBdf();
    testInitialStateSha256KnownVectors();
    testInitialStatePackageUsesWrittenSparseEidsAndBdfHash();
    testMaterialDecisionFingerprintAndDiscardPolicy();
    testExportGenerationReusesDirectoryAndCleansNewDirectoryOnFailure();
    testSecondPieceFailurePreservesPublishedReferenceAndCleansGeneration();
    testInputSidecarRoundTripRelativeHashAndInlinePersistence();
    testTopologySnapshotPreservesTransformsAndBlocksEdits();
    testMini3dInputWriterReaderAndProjectRoundTrip();
  } catch (const std::exception& error) {
    std::cerr << "InitialStateVtkReader test failed: " << error.what() << '\n';
    return 1;
  }
  return 0;
}
