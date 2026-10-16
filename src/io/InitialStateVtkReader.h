#pragma once

#include <vtkCell.h>
#include <vtkCellData.h>
#include <vtkAbstractArray.h>
#include <vtkDataArray.h>
#include <vtkPointData.h>
#include <vtkDataSetReader.h>
#include <vtkErrorCode.h>
#include <vtkIdList.h>
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLUnstructuredGridReader.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

// Owns a detached copy of imported geometry/results. tetCellIds and
// surfaceCellIds store original VTK cell indices for later selection and field
// transfer, before model entities/EIDs exist.
struct InitialStateVtkMesh {
  vtkSmartPointer<vtkUnstructuredGrid> dataset;
  std::vector<vtkIdType> tetCellIds;
  std::vector<vtkIdType> surfaceCellIds;
  std::filesystem::path sourcePath;
};

enum class InitialStateVtkHistoryStatus {
  Available,
  Missing,
  PointDataOnly,
  Invalid
};

struct InitialStateVtkPlStrain {
  InitialStateVtkHistoryStatus status = InitialStateVtkHistoryStatus::Missing;
  std::string diagnostic;
  std::size_t selectedTetCount = 0;
  bool hasRange = false;
  double minimum = 0.0;
  double maximum = 0.0;
  std::map<vtkIdType, double> valuesBySourceCellId;
};

inline InitialStateVtkMesh ReadInitialStateVtkMesh(const std::filesystem::path& path) {
  auto extension = path.extension().string();
  std::transform(extension.begin(), extension.end(), extension.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

  vtkSmartPointer<vtkUnstructuredGrid> loaded;
  if (extension == ".vtk") {
    auto reader = vtkSmartPointer<vtkDataSetReader>::New();
    reader->SetFileName(path.string().c_str());
    reader->ReadAllScalarsOn();
    reader->ReadAllVectorsOn();
    reader->ReadAllTensorsOn();
    reader->ReadAllNormalsOn();
    reader->ReadAllTCoordsOn();
    reader->ReadAllFieldsOn();
    reader->Update();
    if (reader->GetErrorCode() != vtkErrorCode::NoError)
      throw std::runtime_error("Error al leer el archivo VTK legacy (código " +
                               std::to_string(reader->GetErrorCode()) + ").");
    loaded = vtkUnstructuredGrid::SafeDownCast(reader->GetOutput());
  } else if (extension == ".vtu") {
    auto reader = vtkSmartPointer<vtkXMLUnstructuredGridReader>::New();
    reader->SetFileName(path.string().c_str());
    reader->Update();
    if (reader->GetErrorCode() != vtkErrorCode::NoError)
      throw std::runtime_error("Error al leer el archivo VTU (código " +
                               std::to_string(reader->GetErrorCode()) + ").");
    loaded = reader->GetOutput();
  } else {
    throw std::runtime_error("Formato no admitido: se requiere .vtk legacy o .vtu.");
  }
  if (!loaded || loaded->GetNumberOfPoints() == 0 || loaded->GetNumberOfCells() == 0)
    throw std::runtime_error("El archivo no contiene una malla volumétrica VTK válida.");

  InitialStateVtkMesh result;
  result.dataset = vtkSmartPointer<vtkUnstructuredGrid>::New();
  result.dataset->DeepCopy(loaded);
  result.sourcePath = path;

  for (vtkIdType cellId = 0; cellId < result.dataset->GetNumberOfCells(); ++cellId) {
    const int type = result.dataset->GetCellType(cellId);
    vtkNew<vtkIdList> pointIds;
    result.dataset->GetCellPoints(cellId, pointIds);
    std::vector<vtkIdType> seenPointIds;
    seenPointIds.reserve(static_cast<std::size_t>(pointIds->GetNumberOfIds()));
    for (vtkIdType localId = 0; localId < pointIds->GetNumberOfIds(); ++localId) {
      const vtkIdType pointId = pointIds->GetId(localId);
      if (pointId < 0 || pointId >= result.dataset->GetNumberOfPoints())
        throw std::runtime_error("Conectividad VTK contiene un identificador de nodo fuera de rango.");
      if (std::find(seenPointIds.begin(), seenPointIds.end(), pointId) != seenPointIds.end())
        throw std::runtime_error("Conectividad VTK contiene nodos repetidos dentro de una celda.");
      seenPointIds.push_back(pointId);
    }
    if (type == VTK_TETRA) {
      if (pointIds->GetNumberOfIds() != 4)
        throw std::runtime_error("Celda tetraédrica inválida: Tet4 requiere cuatro nodos.");
      result.tetCellIds.push_back(cellId);
    } else if (type == VTK_TRIANGLE || type == VTK_QUAD || type == VTK_POLYGON) {
      result.surfaceCellIds.push_back(cellId); // e.g. contact/tool skin; not imported as FE elements
    } else {
      throw std::runtime_error("Tipo de celda VTK no admitido; solo Tet4 volumétricos y superficies acompañantes.");
    }
  }
  if (result.tetCellIds.empty())
    throw std::runtime_error("La entrada es exclusivamente superficial: no contiene elementos Tet4.");

  for (vtkIdType pointId = 0; pointId < result.dataset->GetNumberOfPoints(); ++pointId) {
    double point[3];
    result.dataset->GetPoint(pointId, point);
    if (!std::isfinite(point[0]) || !std::isfinite(point[1]) || !std::isfinite(point[2]))
      throw std::runtime_error("La malla contiene coordenadas no finitas.");
  }

  // Reject malformed cell-data arrays now, while retaining the detached arrays
  // for later field-specific validation (for example CELL_DATA pl_strain).
  auto* cellData = result.dataset->GetCellData();
  for (int i = 0; i < cellData->GetNumberOfArrays(); ++i) {
    auto* array = cellData->GetAbstractArray(i);
    if (!array || array->GetNumberOfTuples() != result.dataset->GetNumberOfCells())
      throw std::runtime_error("Array CELL_DATA incompatible: cada array debe tener una tupla por celda.");
  }
  auto* pointData = result.dataset->GetPointData();
  for (int i = 0; i < pointData->GetNumberOfArrays(); ++i) {
    auto* array = pointData->GetAbstractArray(i);
    if (!array || array->GetNumberOfTuples() != result.dataset->GetNumberOfPoints())
      throw std::runtime_error("Array POINT_DATA incompatible: cada array debe tener una tupla por nodo.");
  }
  return result;
}

// Group Tet4 source cells by CELL_DATA Part_ID. A missing array is reported
// explicitly so callers can require a user's one-piece confirmation.
inline std::map<int, std::vector<vtkIdType>> GroupInitialStateVtkTet4ByPartId(
    const InitialStateVtkMesh& mesh, bool& hasPartId) {
  std::map<int, std::vector<vtkIdType>> groups;
  hasPartId = false;
  if (!mesh.dataset) {
    throw std::runtime_error("No hay un dataset VTK preparado para agrupar.");
  }

  vtkAbstractArray* abstractPartIds = mesh.dataset->GetCellData()->GetAbstractArray("Part_ID");
  if (abstractPartIds == nullptr) {
    return groups;
  }
  vtkDataArray* partIds = vtkDataArray::SafeDownCast(abstractPartIds);
  if (partIds == nullptr || partIds->GetNumberOfComponents() != 1 ||
      partIds->GetNumberOfTuples() != mesh.dataset->GetNumberOfCells()) {
    throw std::runtime_error("CELL_DATA Part_ID debe ser un array numérico escalar con una tupla por celda.");
  }

  hasPartId = true;
  for (vtkIdType cellId : mesh.tetCellIds) {
    const double value = partIds->GetComponent(cellId, 0);
    if (!std::isfinite(value) || std::trunc(value) != value ||
        value < static_cast<double>(std::numeric_limits<int>::lowest()) ||
        value > static_cast<double>(std::numeric_limits<int>::max())) {
      throw std::runtime_error("CELL_DATA Part_ID debe contener enteros finitos dentro del rango admitido.");
    }
    groups[static_cast<int>(value)].push_back(cellId);
  }
  if (groups.empty()) {
    throw std::runtime_error("CELL_DATA Part_ID no identifica ningún grupo Tet4.");
  }
  return groups;
}

// Read only scalar CELL_DATA pl_strain values for Tet4 cells. Point data is
// detected for a useful diagnostic, but is never interpolated or consumed.
inline InitialStateVtkPlStrain InspectInitialStateVtkPlStrain(
    const InitialStateVtkMesh& mesh,
    const std::vector<vtkIdType>& selectedSourceCellIds) {
  InitialStateVtkPlStrain result;
  if (!mesh.dataset) {
    result.status = InitialStateVtkHistoryStatus::Invalid;
    result.diagnostic = "No hay un dataset VTK preparado para leer pl_strain.";
    return result;
  }

  vtkAbstractArray* abstractArray = mesh.dataset->GetCellData()->GetAbstractArray("pl_strain");
  if (abstractArray == nullptr) {
    vtkAbstractArray* pointArray = mesh.dataset->GetPointData()->GetAbstractArray("pl_strain");
    result.status = pointArray == nullptr ? InitialStateVtkHistoryStatus::Missing
                                          : InitialStateVtkHistoryStatus::PointDataOnly;
    result.diagnostic = pointArray == nullptr
        ? "Falta CELL_DATA pl_strain. La importación con historia está bloqueada."
        : "pl_strain existe solo en POINT_DATA; no se interpola ni se usa como historia elemental.";
    return result;
  }

  vtkDataArray* array = vtkDataArray::SafeDownCast(abstractArray);
  if (array == nullptr || array->GetNumberOfComponents() != 1 ||
      array->GetNumberOfTuples() != mesh.dataset->GetNumberOfCells()) {
    result.status = InitialStateVtkHistoryStatus::Invalid;
    result.diagnostic = "CELL_DATA pl_strain debe ser un array numérico escalar con una tupla por celda.";
    return result;
  }

  std::set<vtkIdType> selectedCells;
  for (vtkIdType cellId : selectedSourceCellIds) {
    if (cellId < 0 || cellId >= mesh.dataset->GetNumberOfCells() ||
        mesh.dataset->GetCellType(cellId) != VTK_TETRA || !selectedCells.insert(cellId).second) {
      result.status = InitialStateVtkHistoryStatus::Invalid;
      result.diagnostic = "La selección de historia contiene una celda inválida o repetida; se aceptan solo Tet4 seleccionados.";
      return result;
    }
  }
  result.selectedTetCount = selectedCells.size();

  for (vtkIdType cellId : selectedCells) {
    if (cellId < 0 || cellId >= array->GetNumberOfTuples()) {
      result.status = InitialStateVtkHistoryStatus::Invalid;
      result.diagnostic = "CELL_DATA pl_strain no cubre todos los Tet4 seleccionados.";
      result.valuesBySourceCellId.clear();
      return result;
    }
    const double value = array->GetComponent(cellId, 0);
    if (!std::isfinite(value) || value < 0.0) {
      result.status = InitialStateVtkHistoryStatus::Invalid;
      result.diagnostic = "CELL_DATA pl_strain inválido para el Tet4 seleccionado en celda VTK " +
                          std::to_string(cellId) + ": se requiere un valor finito y no negativo.";
      result.valuesBySourceCellId.clear();
      return result;
    }
    result.valuesBySourceCellId.emplace(cellId, value);
    if (!result.hasRange) {
      result.minimum = result.maximum = value;
      result.hasRange = true;
    } else {
      result.minimum = std::min(result.minimum, value);
      result.maximum = std::max(result.maximum, value);
    }
  }
  result.status = InitialStateVtkHistoryStatus::Available;
  result.diagnostic = selectedCells.empty()
      ? "CELL_DATA pl_strain escalar válido; selecciona una pieza deformable para validar sus Tet4."
      : "CELL_DATA pl_strain escalar validado para los Tet4 seleccionados.";
  return result;
}

inline InitialStateVtkPlStrain InspectInitialStateVtkPlStrain(
    const InitialStateVtkMesh& mesh) {
  return InspectInitialStateVtkPlStrain(mesh, mesh.tetCellIds);
}
