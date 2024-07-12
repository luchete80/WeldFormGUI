#ifndef _PART_H_
#define _PART_H_

class Mesh;

enum Part_Type {Elastic=0, Rigid};

class Part {
  
public:
  int id;
  Part(){}
  Part(Mesh *mesh);
  Mesh* getMesh(){return m_msh;}
  virtual Part_Type getType(){return m_type}
protected:
  Mesh* m_msh;
  Part_Type m_type;
  
};

class RigidPart:
public Part{
  
  
};

#endif