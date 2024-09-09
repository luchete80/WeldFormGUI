#ifndef _PART_H_
#define _PART_H_

class Mesh;
class Geom;

enum Part_Type {Elastic=0, Rigid};

class Part {
  
public:
  int id;
  Part(){}
  Part(Mesh *mesh);
  Part(Geom*);
  Mesh* getMesh(){return m_msh;}
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
