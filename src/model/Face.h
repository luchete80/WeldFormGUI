#ifndef _FACE_H_
#define _FACE_H_

#include <vector>

#include "Entity.h"

class Face : public Entity {
public:
  using Entity::getId;
  using Entity::getID;

  Face() = default;
  explicit Face(const std::vector<int>& nodeIds) : m_node_ids(nodeIds) {}

  void setEntityId(int id) { m_id = id; }
  void setNodeIds(const std::vector<int>& nodeIds) { m_node_ids = nodeIds; }
  const std::vector<int>& getNodeIds() const { return m_node_ids; }
  int getNodeCount() const { return static_cast<int>(m_node_ids.size()); }
  int getNodeId(int index) const { return m_node_ids[index]; }

  void setOwnerElementId(int ownerElementId) { m_owner_element_id = ownerElementId; }
  int getOwnerElementId() const { return m_owner_element_id; }

  void setLocalFaceIndex(int localFaceIndex) { m_local_face_index = localFaceIndex; }
  int getLocalFaceIndex() const { return m_local_face_index; }

private:
  std::vector<int> m_node_ids;
  int m_owner_element_id = -1;
  int m_local_face_index = -1;
};

#endif
