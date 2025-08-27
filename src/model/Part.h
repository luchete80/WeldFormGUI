#ifndef _PART_H_
#define _PART_H_

#include "Entity.h"
#include "Mesh.h"
#include <iostream>

using namespace std;

class Geom;

enum Part_Type {Elastic=0, Rigid};

class Part :
public Entity{
  
public:
  Part(int &id);
  Part(){
    m_id = -1;
    m_msh = nullptr;
    m_geom = nullptr;
    m_ismeshed = false;
    m_isgeom   = false;
  }
  Part(int &id, Mesh *mesh);
  Part(Mesh *mesh);
  Part(Geom*);
  Mesh* getMesh(){
    if (m_msh != nullptr) {
      //cout << "Address "<<m_msh<<endl;
    return m_msh;
    }else cout << "MESH POINTER "<<endl;
  }
  Mesh & getRef(){return *m_msh;}
  void setMesh(Mesh* m);
  void generateMesh();
  virtual Part_Type getType(){return m_type;}
  const bool isMeshed () const{return m_ismeshed;}
  const bool isGeom () const{return m_isgeom;}
  Geom* getGeom(){return m_geom;}
protected:
  Mesh* m_msh;
  Part_Type m_type;
  Geom* m_geom;
  bool m_ismeshed;
  bool m_isgeom;
  
};

class RigidPart:
public Part{
  
  
};

#endif
