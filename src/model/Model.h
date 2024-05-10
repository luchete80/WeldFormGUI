#ifndef _MODEL_H_
#define _MODEL_H_

class Element;

class Part;
class Model {
public:
  Model(){}
protected:
  std::vector <Part*> m_part;
  
};

#endif