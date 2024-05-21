#ifndef _PART_H_
#define _PART_H_

class Mesh;

class Part {
  int id;
  Part(){}
  Part(Mesh *mesh);
  Mesh* m_msh;
  
};

#endif