#ifndef _NODE_H_
#define _NODE_H_

// #include <glm>
#include "common/math/math.h"
class Element;

struct Node:
public Vector3f{

public:
  Node(){}
  Node(const double &x, const double &y, const double &z){
    
  }  
  

protected:
  //double   m_x[3];
  int      m_id;
  Element* m_elem; //Node elements 
};

#endif