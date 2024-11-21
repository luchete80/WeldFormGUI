#ifndef _NODE_H_
#define _NODE_H_

// #include <glm>
#include "../common/math/math.h"
#include "Entity.h"

class Element;

struct Node
:public Entity
{

public:
  Node(){}
  Node(const double &x, const double &y, const double &z = 0, const int &id = 0){
    m_pos = Vector3f(x,y,z);
    m_id = id;
  }  
  const Vector3f & getPos()const {return m_pos;}
  const double & getPos(int i)const {return m_pos[i];}

  const int & getId()const{return m_id;}

protected:
  //double   m_x[3];
  Vector3f m_pos;
  int      m_id;
  Element* m_elem; //Node elements 
};

#endif
