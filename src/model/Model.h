#ifndef _MODEL_H_
#define _MODEL_H_

#include <vector>
#include <string>
#include <map>
#include <string>

using namespace std;

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
class GraphicMesh;
class ModelWriter;
class InputWriter;

enum AnalysisType {
    PlaneStress2D,
    PlaneStrain2D,
    Axisymmetric2D,
    Solid3D
};

enum model_type {FEM_Model=1, SPH_Model};
//HERE WE COULD SE IF SPH IS IN THE PART INSTANCE 
class Model {
  friend Editor;
  friend LSDynaWriter;
  friend GraphicMesh;
  friend ModelWriter;
  friend InputWriter;
public:
  Model(){part_count=0;
    m_hasname = false;
    m_name = "";
    m_mat_count=0;   
    m_analysisType =  Solid3D;
  }
  Model(std::string );
  //Mesh* getPartMesh(const int &i);
  void addPart(Part *);
  void addPart(Geom *);
  void addGeom(Geom* );
  Node * getNode(const int &n) {return m_node[n];}
  int getNodeCount() {return m_node.size();}
  int getElemCount() {return m_elem.size();}
  Geom* getLastGeom(){return m_geom[m_geom.size()-1];}
  int getPartCount(){return m_part.size();}
  const model_type& getModelType () const {return m_modeltype;}
  const Material_* getMaterial (const int &m)const{return m_mat[m];}
  void addMaterial(Material_*m){m_mat.push_back(m);} //Existent material
  const int & getMaterialCount()const{return m_mat_count;}
  const bool &isAnyMesh()const {return have_meshes;}
   Part* getPart(const int &i) {return m_part[i];}
  Part & getPartRef(const int &i) {return *m_part[i];}
  virtual void AddBoxLength				(int tag, Vec3_t const &V, double Lx, double Ly, double Lz,double r, double Density,
                                double h,int type, int rotation, bool random, bool Fixed){};									//Add a cube of particles with a defined dimensions
                                
  bool & getHasName(){return m_hasname;}
  virtual ~Model(){}
  void setName (string n){m_name=n;m_hasname=true;}
  string getName(){return m_name;}
  
  int part_count;
  void delPart(const int &p){m_part.erase(m_part.begin()+p);};

    void setAnalysisType(AnalysisType t) { m_analysisType = t; }
    AnalysisType getAnalysisType() const { return m_analysisType; }

    int getDimension() const {
        switch(m_analysisType) {
            case AnalysisType::PlaneStress2D:
            case AnalysisType::PlaneStrain2D:
            case AnalysisType::Axisymmetric2D:
                return 2;
            case AnalysisType::Solid3D:
                return 3;
        }
        return -1; // error
    }

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
  
  //IO AND SETTING THINGS
  bool m_hasname;
  string m_name;
  AnalysisType m_analysisType = Solid3D;


};

class FEMModel:
public Model{
public:
  FEMModel(){ 
    m_modeltype = FEM_Model;
    m_hasname = false;
    m_name = "";
    m_mat_count=0;
  
  }
  FEMModel(std::string ){};
  //Mesh* getPartMesh(const int &i){};
           
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
