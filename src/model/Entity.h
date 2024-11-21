#ifndef _ENTITY_H_
#define _ENTITY_H_

class Mesh;

class Entity {
public:
  Entity(){}
  Entity(int &id){m_id=id;}
  const int & getId()const{return m_id;} ///REMEMBER TO RETURN AS CONST &, CONSIDERING SWIG WRAPPING

protected:
  int m_id;
  char *       m_name;
};

class MeshEntity {
public:


protected:
  Mesh *m_mesh;
  int m_id;
};


#endif
