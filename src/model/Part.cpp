#include "Part.h"

#include <iostream>
#include <memory>

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
  m_id = id;
  m_msh = nullptr;
  m_geom = nullptr;
  m_ismeshed = false;
  m_isgeom = false;
  m_type = Elastic;
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

bool Part::generateMesh(){
  if (m_msh != nullptr && m_msh->hasInitialPlasticStrainHistory()) {
    std::cerr << "Cannot replace mesh: explicitly discard its initial pl_strain history first." << std::endl;
    return false;
  }
  std::unique_ptr<Mesh> generated = std::make_unique<Mesh>();
  generated->genFromGmshModel();
  delete m_msh;
  this->m_msh = generated.release();
  m_mesh_source_file.clear();
  m_ismeshed = true;
  return true;
}

bool Part::generateMeshFromNastranFile(const std::string& filename){
  if (m_msh != nullptr && m_msh->hasInitialPlasticStrainHistory()) {
    std::cerr << "Cannot replace mesh: explicitly discard its initial pl_strain history first." << std::endl;
    return false;
  }
  std::unique_ptr<Mesh> loadedMesh = std::make_unique<Mesh>();
  loadedMesh->genFromNastranFile(filename);
  delete m_msh;
  this->m_msh = loadedMesh.release();
  m_mesh_source_file = filename;
  m_ismeshed = true;
  return true;
}

bool Part::deleteMesh(){
  if (m_msh != nullptr && m_msh->hasInitialPlasticStrainHistory()) {
    std::cerr << "Cannot delete mesh: explicitly discard its initial pl_strain history first." << std::endl;
    return false;
  }
  delete this->m_msh;
  this->m_msh = nullptr;
  m_ismeshed = false;
  return true;
}


bool Part::setMesh(Mesh* m){
  if (m_msh != nullptr && m_msh != m && m_msh->hasInitialPlasticStrainHistory()) {
    std::cerr << "Cannot replace mesh: explicitly discard its initial pl_strain history first." << std::endl;
    return false;
  }
  if (m!=nullptr){
    m_msh = (Mesh*) m;
    m_ismeshed = true;
    m_mesh_source_file.clear();
      cout << "Set Mesh "<<m_msh->getNodeCount()<< " nodes "<<endl;
    return true;
  }
  else {
    std::cerr << "Cannot set a null mesh." << std::endl;
    return false;
  }
  }
