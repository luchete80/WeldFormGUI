#ifndef _SET_H_
#define _SET_H_

#include <vector>
#include "Entity.h"

class Mesh;

template <typename T>
class Set:
public Entity{
  
public:
  int id;
  Set(){}
  Set(Mesh *mesh);
  Mesh* getMesh(){return m_msh;}
  
protected:
  Mesh* m_msh;
  std::vector <T*> m_item;
};


class NodeSet
:public Set<Node>{

public:
  
  
  
  };

#endif
