#include "Part.h"

#include <iostream>

using namespace std; 

Part::Part(int &id, Mesh *m):
Entity(id),
m_msh(m){
  
  }

Part::Part(int &id):
Entity(id){
  }

Part::Part(Mesh *mesh):
m_msh(mesh){
  
}

Part::Part(Geom* geom){
  
}void Part::generateMesh(){
  this->m_msh = new Mesh;
  this->m_msh->genFromGmshModel();
}


void Part::setMesh(Mesh* m){
  if (m!=nullptr){
    m_msh = (Mesh*) m;
      cout << "Set Mesh "<<m_msh->getNodeCount()<< " nodes "<<endl;
  }
  else
  cout << "ERROR. null msh address"<<endl;
  }
