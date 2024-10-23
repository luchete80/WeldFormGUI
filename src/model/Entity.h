#ifndef _ENTITY_H_
#define _ENTITY_H_

class Mesh;

class Entity {
public:


protected:
  unsigned int m_id;
  char *       m_name;
};

class MeshEntity {
public:


protected:
  Mesh *m_mesh;
  unsigned int m_id;
};


#endif
