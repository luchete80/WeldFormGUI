#ifndef _ENTITY_H_
#define _ENTITY_H_

#include <vector>
#include <iostream>
#include "double3.h"
#include <string>

using namespace std;

class Mesh;

class Entity {
public:

  Entity(){}
  Entity(int &id){m_id=id;}
  const int & getId()const{return m_id;} ///REMEMBER TO RETURN AS CONST &, CONSIDERING SWIG WRAPPING
  
  const double3 & getVel()const {return m_v;}
  void setVel(const double3 &v){m_v = v;}
  const char* getName(){return m_name.c_str();}
  const int & getID()const{return m_id;}
  void setName(const char *name){m_name = name;}
  void setId(int &i){
    m_id=i;
    cout <<"Id set to "<<m_id<<endl;}
protected:
  int m_id;
  std::string m_name = "";
  double3       m_v;// Velocity
};

class MeshEntity {
public:


protected:
  Mesh *m_mesh;
  int m_id;
};

/// CREATE ALSO TEMPLATE

int getMaxId(std::vector<Entity*> ent);

// Function to operate on a vector of Base pointers
/*
template <typename T>
void processObjects(const std::vector< std::unique_ptr<T> > & objects) {
    for (const auto& obj : objects) {
        obj->doSomething(); // Polymorphic call
    }
}
*/
/*
template <typename T>
void processObjects2(const std::vector< *T >& objects) {
    for (const auto& obj : objects) {
        obj->doSomething(); // Polymorphic call
    }
}
*/
#endif
