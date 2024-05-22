#ifndef _MODEL_H_
#define _MODEL_H_

#include <vector>
#include <string>

class Element;
class Node;
class Material;
class Part;
class Mesh;

class Model {
public:
  Model(){}
  Model(std::string );
  Mesh* getPartMesh(const int &i);
protected:
  std::vector <Part*>       m_part;
  std::vector <Material*>   m_mat;  
  // TODO: SHOULD ANALYZE IF IT IS NECESARY TO HAVE REPEATED POINTERS FOR MESH AND MODEL 
  // NDOES AND ELEMENTS
  std::vector <Node* >      m_node; //Mesh part refer to this
  std::vector <Element* >   m_elem; //Mesh part refer to this
};

#endif