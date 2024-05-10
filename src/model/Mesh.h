#ifndef _MESH_H_
#define _MESH_H_

class Element;

class Mesh{
public:
  Mesh();
  void addNode();
  void addBoxLength();
protected:
  int m_node_count;
  int m_elem_count;
  std::vector <Node*>    m_node;
  std::vector <Element*> m_elem;

};

#endif