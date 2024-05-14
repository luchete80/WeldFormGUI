#ifndef _MESH_H_
#define _MESH_H_

#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include "common/math/math.h"

class Element;
class Node;

class Mesh{
public:
  Mesh();
  void addNode();
  void addBoxLength(Vector3f L, Vector3f V, double r);
protected:
  int m_node_count;
  int m_elem_count;
  std::vector <Node*>    m_node;
  std::vector <Element*> m_elem;

};

#endif