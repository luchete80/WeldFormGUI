#ifndef _MESH_H_
#define _MESH_H_

class Element;

class Mesh{
public:
  Mesh();
  void addNode();
  void addBoxLength();
protected:
  std::vector <Node*>    m_node;
  std::vector <Element*> m_elem;

};

#endif