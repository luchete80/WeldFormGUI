#ifndef _MODEL_H_
#define _MODEL_H_

#include <vector>
#include <string>
#include <map>

class Element;
class Node;
class Material;
class Part;
class Mesh;
class Particle;

enum model_type {FEM_Model=1, SPH_Model};
//HERE WE COULD SE IF SPH IS IN THE PART INSTANCE 
class Model {
public:
  Model(){}
  Model(std::string );
  Mesh* getPartMesh(const int &i);
  int getPartCount(){return m_part.size();}
  const model_type& getModelType () const {return m_modeltype;}
protected:
  std::vector <Part*>       m_part;
  std::vector <Material*>   m_mat;  
  // TODO: SHOULD ANALYZE IF IT IS NECESARY TO HAVE REPEATED POINTERS FOR MESH AND MODEL 
  // NDOES AND ELEMENTS
  std::vector <Particle* >  m_particle; 
  std::vector <Node* >      m_node; //Mesh part refer to this
  std::vector <Element* >   m_elem; //Mesh part refer to this
  std::map <std::pair<int,int>, int> m_linemap;
  model_type m_modeltype;
};

class FEMModel:
public Model{
public:
  FEMModel(){ m_modeltype = FEM_Model;}
  FEMModel(std::string );
  Mesh* getPartMesh(const int &i);
protected:

};

// DEFINED IN DOMAIN_d
// class SPHModel:
// public Model{
// public:
  // SPHModel(){ m_modeltype = SPH_Model;}
  // SPHModel(std::string );
  // //Mesh* getPartMesh(const int &i);
// protected:

// };

#endif