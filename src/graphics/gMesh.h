#ifndef _QUADGMESH_
#define _QUADGMESH_

#include "myMesh.h"

class Mesh; //FEM MESH
///// GRPHICS QUAD MESH
class gMesh:
public myMesh{

public:
  gMesh(){}
  gMesh(Mesh*);  

protected:
  Mesh *m_msh;  
  
};


#endif