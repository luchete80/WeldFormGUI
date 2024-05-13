#ifndef _MESH_H_
#define _MESH_H_

#include <vector>
#include <glm/gtc/matrix_transform.hpp>

class Element;
class Node;

class Mesh{
public:
  Mesh();
  void addNode();
  void addBoxLength(Vector L[3], const double &V[3], double r);
protected:
  int m_node_count;
  int m_elem_count;
  std::vector <Node*>    m_node;
  std::vector <Element*> m_elem;

};

#endif