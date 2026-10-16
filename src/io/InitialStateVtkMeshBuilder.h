#pragma once

#include "io/InitialStateVtkReader.h"
#include "model/Element.h"
#include "model/Mesh.h"
#include "model/Node.h"

#include <limits>
#include <cmath>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <vector>

// Detached conversion result. Mesh stores raw pointers, so the adjacent owner
// vectors keep nodes and elements alive until a later model/part commit adopts
// this whole result. Declaration order ensures Mesh is destroyed before its
// referenced nodes and elements.
struct InitialStateVtkModelMesh {
  std::vector<std::unique_ptr<Node>> ownedNodes;
  std::vector<std::unique_ptr<Element>> ownedElements;
  std::unique_ptr<Mesh> mesh;
  std::map<vtkIdType, Element*> sourceCellIdToElement;
};

inline InitialStateVtkModelMesh ConvertInitialStateVtkTet4CellsToMesh(
    const InitialStateVtkMesh& source,
    const std::vector<vtkIdType>& selectedSourceCellIds) {
  if (!source.dataset) {
    throw std::runtime_error("No hay un dataset VTK preparado para convertir.");
  }
  if (selectedSourceCellIds.empty()) {
    throw std::runtime_error("Debe seleccionarse al menos un Tet4 para convertir.");
  }
  if (selectedSourceCellIds.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
    throw std::runtime_error("La selección excede el rango de IDs de elemento del modelo.");
  }

  std::set<vtkIdType> allowedCells(source.tetCellIds.begin(), source.tetCellIds.end());
  std::set<vtkIdType> seenCells;
  std::unordered_map<vtkIdType, Node*> sourcePointIdToNode;
  std::vector<Node*> meshNodes;
  std::vector<Element*> meshElements;

  InitialStateVtkModelMesh result;
  result.mesh = std::make_unique<Mesh>();
  result.mesh->setDim(3);
  result.ownedElements.reserve(selectedSourceCellIds.size());
  meshElements.reserve(selectedSourceCellIds.size());

  for (vtkIdType sourceCellId : selectedSourceCellIds) {
    if (sourceCellId < 0 || sourceCellId >= source.dataset->GetNumberOfCells() ||
        allowedCells.find(sourceCellId) == allowedCells.end() ||
        source.dataset->GetCellType(sourceCellId) != VTK_TETRA) {
      throw std::runtime_error("La selección contiene una celda que no es Tet4 del dataset origen.");
    }
    if (!seenCells.insert(sourceCellId).second) {
      throw std::runtime_error("La selección contiene índices de celda repetidos.");
    }

    vtkNew<vtkIdList> sourcePointIds;
    source.dataset->GetCellPoints(sourceCellId, sourcePointIds);
    if (sourcePointIds->GetNumberOfIds() != 4) {
      throw std::runtime_error("Conectividad inválida: cada Tet4 debe tener cuatro nodos.");
    }

    std::vector<Node*> elementNodes;
    elementNodes.reserve(4);
    std::set<vtkIdType> uniquePointIds;
    for (vtkIdType localPoint = 0; localPoint < 4; ++localPoint) {
      const vtkIdType sourcePointId = sourcePointIds->GetId(localPoint);
      if (sourcePointId < 0 || sourcePointId >= source.dataset->GetNumberOfPoints()) {
        throw std::runtime_error("Conectividad contiene un ID de nodo fuera de rango.");
      }
      if (!uniquePointIds.insert(sourcePointId).second) {
        throw std::runtime_error("Conectividad contiene un nodo repetido en un Tet4.");
      }

      Node* node = nullptr;
      const auto foundNode = sourcePointIdToNode.find(sourcePointId);
      if (foundNode != sourcePointIdToNode.end()) {
        node = foundNode->second;
      } else {
        if (result.ownedNodes.size() >= static_cast<std::size_t>(std::numeric_limits<int>::max())) {
          throw std::runtime_error("La selección excede el rango de IDs de nodo del modelo.");
        }
        double point[3];
        source.dataset->GetPoint(sourcePointId, point);
        if (!std::isfinite(point[0]) || !std::isfinite(point[1]) || !std::isfinite(point[2])) {
          throw std::runtime_error("La malla contiene coordenadas no finitas.");
        }
        const int nodeId = static_cast<int>(result.ownedNodes.size()) + 1;
        result.ownedNodes.emplace_back(std::make_unique<Node>(point[0], point[1], point[2], nodeId));
        node = result.ownedNodes.back().get();
        sourcePointIdToNode.emplace(sourcePointId, node);
        meshNodes.push_back(node);
      }
      elementNodes.push_back(node);
    }

    int elementId = static_cast<int>(result.ownedElements.size()) + 1;
    result.ownedElements.emplace_back(std::make_unique<Element>(elementNodes));
    Element* element = result.ownedElements.back().get();
    element->setId(elementId);
    element->setSourceVtkCellId(static_cast<std::int64_t>(sourceCellId));
    meshElements.push_back(element);
    result.sourceCellIdToElement.emplace(sourceCellId, element);
    result.mesh->registerSourceVtkCellElement(static_cast<std::int64_t>(sourceCellId), element);
  }

  result.mesh->assignValues(meshNodes, meshElements);
  result.mesh->setDim(3);
  return result;
}
