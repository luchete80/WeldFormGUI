#ifndef _NODE_H_
#define _NODE_H_

// #include <glm>
#include "../common/math/math_.h"
#include "Entity.h"

class Element;

struct Node
:public Entity
{

public:
  using Entity::getId; //SWIG
  Node(){}
  Node(const double &x, const double &y, const double &z = 0, const int &id = 0){
    m_pos = Vector3f(x,y,z);
    m_id = id;
  }  
  const Vector3f & getPos()const {return m_pos;}
  const double & getPos(int i)const {return m_pos[i];}
  void setPos(const Vector3f& pos){m_pos = pos;}
  void setPos(const double &x, const double &y, const double &z = 0){m_pos = Vector3f(x,y,z);}
  void translate(const double &dx, const double &dy, const double &dz){m_pos += Vector3f(dx,dy,dz);}

  //const int & getId()const{return m_id;}

protected:
  //double   m_x[3];
  Vector3f m_pos;
  //int      m_id;
  Element* m_elem; //Node elements 
};

#endif
