#ifndef _MESH_H_
#define _MESH_H_

/////////////// FINITE ELEMENT MESH ///////////

#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include "common/math/math.h"
#include "Element.h"

class Element;
class Node;

class Mesh{
public:
  Mesh();
  void addNode();
  void addBoxLength(Vector3f L, Vector3f V, double r);
  const int & getNodeCount()const {return m_node_count;}
  const int & getElemCount()const {return m_elem_count;}
  Node*     getNode(const int &i){return m_node[i];} 
  Element*  getElem(const int &i){return m_elem[i];} 
  
  const Vector3f& getNodePos(const int &i)const;
protected:
  int m_node_count;
  int m_elem_count;
  std::vector <Node*>    m_node;
  std::vector <Element*> m_elem;
  std::vector <int>      elnod_h;

};

#endif