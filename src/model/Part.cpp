#include "Part.h"

#include <iostream>

using namespace std; 

Part::Part(int &id, Mesh *m)
//:Entity(id),
:m_msh(m){
  m_id = id;
  m_geom = nullptr;
  m_ismeshed = (m != nullptr);
  m_isgeom = false;
  m_type = Elastic;
  }

Part::Part(int &id)
//:Entity(id)
{
  }

Part::Part(Mesh *mesh):
m_msh(mesh){
  m_id = -1;
  m_geom = nullptr;
  if (mesh==nullptr){
    cout << "ERROR, NULL mesh pointer"<<endl;
  }
  m_ismeshed = (mesh != nullptr);
  m_isgeom = false;
  m_type = Elastic;
  
}

Part::Part(Geom* geom){
  m_id = -1;
  m_msh = nullptr;
  m_ismeshed = false;
  m_isgeom = (geom != nullptr);
  m_geom = geom;
  m_type = Elastic;
}

void Part::generateMesh(){
  this->m_msh = new Mesh;
  this->m_msh->genFromGmshModel();
  m_mesh_source_file.clear();
  m_ismeshed = true;
}

void Part::generateMeshFromNastranFile(const std::string& filename){
  this->m_msh = new Mesh;
  this->m_msh->genFromNastranFile(filename);
  m_mesh_source_file = filename;
  m_ismeshed = true;
}

void Part::deleteMesh(){
  delete this->m_msh;
  this->m_msh = nullptr;
  m_ismeshed = false;
}


void Part::setMesh(Mesh* m){
  if (m!=nullptr){
    m_msh = (Mesh*) m;
    m_ismeshed = true;
    m_mesh_source_file.clear();
      cout << "Set Mesh "<<m_msh->getNodeCount()<< " nodes "<<endl;
  }
  else {
    m_msh = nullptr;
    m_ismeshed = false;
    cout << "ERROR. null msh address"<<endl;
  }
  }
