#ifndef _NODE_H_
#define _NODE_H_

// #include <glm>
#include "../common/math/math.h"
class Element;

struct Node
//:public Vector3f
{

public:
  Node(){}
  Node(const double &x, const double &y, const double &z, const int &id = 0){
    m_pos = Vector3f(x,y,z);
    m_id = id;
  }  
  const Vector3f & getPos()const {return m_pos;}
  const int & getId()const{return m_id;}

protected:
  //double   m_x[3];
  Vector3f m_pos;
  int      m_id;
  Element* m_elem; //Node elements 
};

#endif