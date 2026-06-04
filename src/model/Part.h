#ifndef _PART_H_
#define _PART_H_

#include "Entity.h"
#include "Mesh.h"
#include <iostream>

using namespace std;

class Geom;
class Section;

enum Part_Type {Elastic=0, Rigid};

struct PartDialog;

class Part :
public Entity{
friend PartDialog;
  
public:
  Part(int &id);
  Part(){
    m_id = -1;
    m_msh = nullptr;
    m_geom = nullptr;
    m_ismeshed = false;
    m_isgeom   = false;
    m_type     = Elastic; //Deformable
  }
  Part(int &id, Mesh *mesh);
  Part(Mesh *mesh);
  Part(Geom*);
  Mesh* getMesh(){
    if (m_msh != nullptr) {
      //cout << "Address "<<m_msh<<endl;
      return m_msh;
    }
    return nullptr;
  }
  void setId(const int &id){m_id = id;}

  Mesh & getRef(){return *m_msh;}
  void setMesh(Mesh* m);
  void generateMesh();
  void generateMeshFromNastranFile(const std::string& filename);
  void deleteMesh();
  void setMeshSourceFile(const std::string& filename){m_mesh_source_file = filename;}
  const std::string& getMeshSourceFile() const {return m_mesh_source_file;}
  virtual Part_Type getType(){return m_type;}
  void setType(const int &t){
    if (t==0) m_type = Elastic;
    else if (t==1)m_type = Rigid;
  }
  int getSectionId() const { return m_section_id; }
  void setSectionId(int section_id) { m_section_id = section_id; }
  
  const bool isMeshed () const{return m_ismeshed;}
  const bool isGeom () const{return m_isgeom;}
  Geom* getGeom(){return m_geom;}
protected:
  Mesh* m_msh;
  Part_Type m_type;
  Geom* m_geom = nullptr;
  std::string m_mesh_source_file;
  int m_section_id = -1;
  bool m_ismeshed;
  bool m_isgeom;
  
};

class RigidPart:
public Part{
  
  
};

#endif
