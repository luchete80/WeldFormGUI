#include "Part.h"
#include "Mesh.h"

Part::Part(Mesh *mesh):
m_msh(mesh){
  
}

Part::Part(Geom* geom){
  
}

void Part::generateMesh(){
  this->m_msh = new Mesh;

}
