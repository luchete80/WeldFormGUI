#ifndef _ENTITY_H_
#define _ENTITY_H_

class Mesh;

class Entity {
public:


protected:
  Mesh *m_mesh;
  unsigned int m_id;
};

#endif