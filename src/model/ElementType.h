#ifndef WFGUI_MODEL_ELEMENTTYPE_H
#define WFGUI_MODEL_ELEMENTTYPE_H

#include "Mesh.h"

enum class ElementType {
  Unknown = 0,
  Line2,
  Tria3,
  Quad4,
  Tetra4,
  Hexa8
};

enum class ElementUsage {
  Unknown = 0,
  Boundary,
  Bulk
};

inline ElementType inferElementType(const Mesh* mesh, const Element* element)
{
  if (element == nullptr) {
    return ElementType::Unknown;
  }

  const int nodeCount = element->getNodeCount();
  const int meshDim = (mesh != nullptr) ? mesh->getDim() : -1;

  if (meshDim == 2) {
    if (nodeCount == 2) return ElementType::Line2;
    if (nodeCount == 3) return ElementType::Tria3;
    if (nodeCount == 4) return ElementType::Quad4;
    return ElementType::Unknown;
  }

  if (meshDim == 3) {
    if (nodeCount == 3) return ElementType::Tria3;
    if (nodeCount == 4) return ElementType::Tetra4;
    if (nodeCount == 8) return ElementType::Hexa8;
    return ElementType::Unknown;
  }

  if (nodeCount == 2) return ElementType::Line2;
  if (nodeCount == 3) return ElementType::Tria3;
  if (nodeCount == 4) return ElementType::Unknown;
  if (nodeCount == 8) return ElementType::Hexa8;
  return ElementType::Unknown;
}

inline ElementUsage inferElementUsage(const Mesh* mesh, const Element* element)
{
  const ElementType type = inferElementType(mesh, element);
  const int meshDim = (mesh != nullptr) ? mesh->getDim() : -1;

  switch (type) {
    case ElementType::Line2:
      return ElementUsage::Boundary;
    case ElementType::Tria3:
      return (meshDim == 2) ? ElementUsage::Bulk : ElementUsage::Boundary;
    case ElementType::Quad4:
      return (meshDim == 2) ? ElementUsage::Bulk : ElementUsage::Unknown;
    case ElementType::Tetra4:
    case ElementType::Hexa8:
      return ElementUsage::Bulk;
    case ElementType::Unknown:
    default:
      return ElementUsage::Unknown;
  }
}

inline bool isBulkElement(const Mesh* mesh, const Element* element)
{
  return inferElementUsage(mesh, element) == ElementUsage::Bulk;
}

inline bool isBoundaryElement(const Mesh* mesh, const Element* element)
{
  return inferElementUsage(mesh, element) == ElementUsage::Boundary;
}

inline const char* toString(ElementType type)
{
  switch (type) {
    case ElementType::Line2:
      return "Line2";
    case ElementType::Tria3:
      return "Tria3";
    case ElementType::Quad4:
      return "Quad4";
    case ElementType::Tetra4:
      return "Tetra4";
    case ElementType::Hexa8:
      return "Hexa8";
    case ElementType::Unknown:
    default:
      return "Unknown";
  }
}

inline const char* toString(ElementUsage usage)
{
  switch (usage) {
    case ElementUsage::Boundary:
      return "Boundary";
    case ElementUsage::Bulk:
      return "Bulk";
    case ElementUsage::Unknown:
    default:
      return "Unknown";
  }
}

#endif
