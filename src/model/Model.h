#ifndef _MODEL_H_
#define _MODEL_H_

#include <vector>
#include <string>

class Element;
class Node;
class Material;
class Part;


class Model {
public:
  Model(){}
  Model(std::string );
protected:
  std::vector <Part*>       m_part;
  std::vector <Material*>   m_mat;  
  std::vector <Node* >      m_node; //Mesh part refer to this
};

#endif