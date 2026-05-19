#ifndef WFGUI_SET_WORKFLOW_H
#define WFGUI_SET_WORKFLOW_H

#include "Mesh.h"
#include "Model.h"
#include "Set.h"

#include <algorithm>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace wfgui {
namespace setworkflow {

inline std::vector<std::vector<int>> get_element_boundary_sides(Mesh* mesh, const Element* element)
{
  std::vector<std::vector<int>> sides;
  if (mesh == nullptr || element == nullptr)
    return sides;

  const int nodeCount = element->getNodeCount();
  const int meshDim = mesh->getDim();

  if (meshDim == 2) {
    if (nodeCount == 3) {
      sides.push_back({element->getNodeId(0), element->getNodeId(1)});
      sides.push_back({element->getNodeId(1), element->getNodeId(2)});
      sides.push_back({element->getNodeId(2), element->getNodeId(0)});
    } else if (nodeCount == 4) {
      sides.push_back({element->getNodeId(0), element->getNodeId(1)});
      sides.push_back({element->getNodeId(1), element->getNodeId(2)});
      sides.push_back({element->getNodeId(2), element->getNodeId(3)});
      sides.push_back({element->getNodeId(3), element->getNodeId(0)});
    }
  } else if (meshDim == 3) {
    if (nodeCount == 4) {
      sides.push_back({element->getNodeId(0), element->getNodeId(1), element->getNodeId(2)});
      sides.push_back({element->getNodeId(0), element->getNodeId(1), element->getNodeId(3)});
      sides.push_back({element->getNodeId(1), element->getNodeId(2), element->getNodeId(3)});
      sides.push_back({element->getNodeId(2), element->getNodeId(0), element->getNodeId(3)});
    } else if (nodeCount == 8) {
      sides.push_back({element->getNodeId(0), element->getNodeId(1), element->getNodeId(2), element->getNodeId(3)});
      sides.push_back({element->getNodeId(4), element->getNodeId(5), element->getNodeId(6), element->getNodeId(7)});
      sides.push_back({element->getNodeId(0), element->getNodeId(1), element->getNodeId(5), element->getNodeId(4)});
      sides.push_back({element->getNodeId(1), element->getNodeId(2), element->getNodeId(6), element->getNodeId(5)});
      sides.push_back({element->getNodeId(2), element->getNodeId(3), element->getNodeId(7), element->getNodeId(6)});
      sides.push_back({element->getNodeId(3), element->getNodeId(0), element->getNodeId(4), element->getNodeId(7)});
    }
  }

  return sides;
}

inline std::string face_key_from_node_ids(const std::vector<int>& nodeIds)
{
  std::vector<int> sortedIds = nodeIds;
  std::sort(sortedIds.begin(), sortedIds.end());

  std::ostringstream key;
  for (std::size_t i = 0; i < sortedIds.size(); ++i) {
    if (i > 0)
      key << "-";
    key << sortedIds[i];
  }
  return key.str();
}

inline FaceSet build_boundary_face_set_from_element_set(Mesh* mesh,
                                                        const ElementSet& elementSet,
                                                        int faceSetId,
                                                        const std::string& label)
{
  struct FaceCandidate {
    std::vector<int> nodeIds;
    int ownerElementId = -1;
    int localFaceIndex = -1;
    int count = 0;
  };

  std::map<std::string, FaceCandidate> faceMap;
  for (int i = 0; i < elementSet.getItemCount(); ++i) {
    const Element* element = elementSet.getItem(i);
    if (element == nullptr)
      continue;

    const std::vector<std::vector<int>> sides = get_element_boundary_sides(mesh, element);
    for (std::size_t sideIndex = 0; sideIndex < sides.size(); ++sideIndex) {
      const std::vector<int>& sideNodeIds = sides[sideIndex];
      const std::string key = face_key_from_node_ids(sideNodeIds);
      FaceCandidate& candidate = faceMap[key];
      if (candidate.count == 0) {
        candidate.nodeIds = sideNodeIds;
        candidate.ownerElementId = element->getId();
        candidate.localFaceIndex = static_cast<int>(sideIndex);
      }
      candidate.count++;
    }
  }

  FaceSet faceSet(mesh);
  faceSet.setEntityId(faceSetId);
  faceSet.setLabel(label);

  int nextFaceId = 0;
  for (std::map<std::string, FaceCandidate>::const_iterator it = faceMap.begin(); it != faceMap.end(); ++it) {
    const FaceCandidate& candidate = it->second;
    if (candidate.count != 1)
      continue;

    Face face(candidate.nodeIds);
    face.setEntityId(nextFaceId++);
    face.setOwnerElementId(candidate.ownerElementId);
    face.setLocalFaceIndex(candidate.localFaceIndex);
    faceSet.add(face);
  }

  return faceSet;
}

inline int find_next_face_set_id(Model* model)
{
  if (model == nullptr)
    return 0;

  int maxId = -1;
  for (int p = 0; p < model->getPartCount(); ++p) {
    Part* part = model->getPart(p);
    if (part == nullptr || part->getMesh() == nullptr)
      continue;

    Mesh* mesh = part->getMesh();
    for (int s = 0; s < mesh->getFaceSetCount(); ++s) {
      maxId = std::max(maxId, mesh->getFaceSet(s).getId());
    }
  }
  return maxId + 1;
}

inline int add_boundary_face_set_from_element_set(Mesh* mesh,
                                                  const ElementSet& elementSet,
                                                  Model* model,
                                                  const std::string& label)
{
  if (mesh == nullptr || model == nullptr)
    return -1;

  const int faceSetId = find_next_face_set_id(model);
  const FaceSet faceSet = build_boundary_face_set_from_element_set(mesh, elementSet, faceSetId, label);
  mesh->addFaceSet(faceSet);
  return faceSetId;
}

} // namespace setworkflow
} // namespace wfgui

#endif
