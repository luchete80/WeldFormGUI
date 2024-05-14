#ifndef _MODEL_H_
#define _MODEL_H_

#include <vector>

class Element;
class Material;
class Part;


class Model {
public:
  Model(){}
protected:
  std::vector <Part*>       m_part;
  std::vector <Material*>   m_mat;  
};

#endif