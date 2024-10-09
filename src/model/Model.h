#ifndef _MODEL_H_
#define _MODEL_H_

#include <vector>
#include <string>
#include <map>

class Element;
class Node;
class Material_;
class Part;
class Mesh;
class Particle;
class Geom;
class BoundaryCondition;
class Vec3_t;
class Editor;
class LSDynaWriter;

enum model_type {FEM_Model=1, SPH_Model};
//HERE WE COULD SE IF SPH IS IN THE PART INSTANCE 
class Model {
  friend Editor;
  friend LSDynaWriter;
public:
  Model(){part_count=0;}
  Model(std::string );
  Mesh* getPartMesh(const int &i);
  void addPart(Part *);
  void addPart(Geom *);
  void addGeom(Geom* );
  Geom* getLastGeom(){return m_geom[m_geom.size()-1];}
  int getPartCount(){return m_part.size();}
  const model_type& getModelType () const {return m_modeltype;}
  const Material_* getMaterial (const int &m)const{return m_mat[m];}
  const int & getMaterialCount()const{return m_mat_count;}
  const bool &isAnyMesh()const {return have_meshes;}
   Part* getPart(const int &i) {return m_part[i];}
  virtual void AddBoxLength				(int tag, Vec3_t const &V, double Lx, double Ly, double Lz,double r, double Density,
                                double h,int type, int rotation, bool random, bool Fixed){};									//Add a cube of particles with a defined dimensions
                                
  virtual ~Model(){}
  
  int part_count;
protected:
  std::vector <Part*>        m_part;
  std::vector <Material_*>   m_mat;  
  std::vector <Geom*>        m_geom;
  // TODO: SHOULD ANALYZE IF IT IS NECESARY TO HAVE REPEATED POINTERS FOR MESH AND MODEL 
  // NDOES AND ELEMENTS
  std::vector <Particle* >  m_particle; 
  std::vector <Node* >      m_node; //Mesh part refer to this
  std::vector <Element* >   m_elem; //Mesh part refer to this
  std::map <std::pair<int,int>, int> m_linemap;
  std::vector <Particle*>   Particles; //SPH
  
  model_type m_modeltype;
  int m_mat_count;
  
  bool have_meshes;
};

class FEMModel:
public Model{
public:
  FEMModel(){ m_modeltype = FEM_Model;}
  FEMModel(std::string ){};
  Mesh* getPartMesh(const int &i){};
           
  ~FEMModel(){}
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
