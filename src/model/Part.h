#ifndef _PART_H_
#define _PART_H_

#include "Entity.h"
#include "Mesh.h"

class Mesh;
class Geom;

enum Part_Type {Elastic=0, Rigid};

class Part :
public Entity{
  
public:
  Part(int &id);
  Part(){}
  Part(int &id, Mesh *mesh);
  Part(Mesh *mesh);
  Part(Geom*);
  Mesh* getMesh(){return m_msh;}
  
  void generateMesh();
  virtual Part_Type getType(){return m_type;}
protected:
  Mesh* m_msh;
  Part_Type m_type;
  Geom* m_geom;
  
};

class RigidPart:
public Part{
  
  
};

#endif
