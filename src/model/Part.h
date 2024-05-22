#ifndef _PART_H_
#define _PART_H_

class Mesh;

class Part {
  
public:
  int id;
  Part(){}
  Part(Mesh *mesh);
  Mesh* getMesh(){return m_msh;}
protected:
  Mesh* m_msh;
  
};

#endif