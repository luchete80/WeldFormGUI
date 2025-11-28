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
class InitialCondition;
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
  Material_* getMaterial (const int &m){return m_mat[m];}
  void addMaterial(Material_*m){m_mat.push_back(m);} //Existent material
  int getMaterialCount(){return m_mat.size();}
  const bool &isAnyMesh()const {return have_meshes;}
   Part* getPart(const int &i) {return m_part[i];}
  Part* getLastPart(){return m_part[m_part.size()-1];} 
  Part & getPartRef(const int &i) {return *m_part[i];}
  virtual void AddBoxLength				(int tag, Vec3_t const &V, double Lx, double Ly, double Lz,double r, double Density,
                                double h,int type, int rotation, bool random, bool Fixed){};									//Add a cube of particles with a defined dimensions

  int getBCCount(){return m_bc.size();}
  BoundaryCondition* getBC(const int &i){return m_bc[i];}

  int getICCount(){return m_ic.size();}
  InitialCondition* getIC(const int &i){return m_ic[i];}
    
  bool & getHasName(){return m_hasname;}
  virtual ~Model(){}
  void setName (string n){m_prev_name=m_name; m_name=n; m_hasname=true;}
  void setNoSaveAs(){m_prev_name = m_name;}
  string getName(){return m_name;}
  string getPrevName(){return m_prev_name;}
  void addBoundaryCondition(BoundaryCondition *make_double2bc);
  void addInitialCondition(InitialCondition *bc){m_ic.push_back(bc);}


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
  bool m_thermal_coupling = false;

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
  std::vector <BoundaryCondition*> m_bc;
  std::vector <InitialCondition*>  m_ic;
  
  model_type m_modeltype;
  
  bool have_meshes;
  
  //IO AND SETTING THINGS
  bool m_hasname;
  string m_name;
  string m_prev_name;
  AnalysisType m_analysisType = Solid3D;


};

class FEMModel:
public Model{
public:
  FEMModel(){ 
    m_modeltype = FEM_Model;
    m_hasname = false;
    m_name = "";
  
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
