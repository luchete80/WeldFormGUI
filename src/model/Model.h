#ifndef _MODEL_H_
#define _MODEL_H_

#include <vector>
#include <string>
#include <map>
#include <string>
#include <filesystem>

#include "BoundaryCondition.h"
#include "InitialCondition.h"

using namespace std;

class Element;
class Node;
class Material_;
class Part;
class Mesh;
class Particle;
class Geom;
class Vec3_t;
class Section;
class Editor;
class LSDynaWriter;
class GraphicMesh;
class ModelWriter;
class InputWriter;
class Step;

enum AnalysisType {
    PlaneStress2D,
    PlaneStrain2D,
    Axisymmetric2D,
    Solid3D
};

enum model_type {FEM_Model=1, SPH_Model};
enum TwoDMeshGenerator {
  MeshGeneratorMeshAdapt = 0,
  MeshGeneratorGmsh = 1
};

struct ContactProperties {
  bool autoPenalty = false;
  double autoFactor = 0.5;
  double fricCoeffStatic = 0.0;
  double frictionRegularizationVelocity = 0.0;
  double gapPenaltyScale = 2.0;
  double heatCondCoeff = 0.5;
  bool heatConductance = false;
  double maxAccel = 100000.0;
  double maxPenetRatio = 0.05;
  double penaltyFactor = 5000.0;
  bool useGapPenalty = true;
};

struct RemeshingSettings {
  bool enabled = false;
  double minStrain = -1.0;
  double maxStrain = 1.0e6;
  bool mapVel = false;
  bool mapAcc = false;
  int maxCount = 1000000;
  double dampFactor = 0.02;
  double minFrac = 2.0;
  double maxFrac = 2.0;
  double epsRef = 1.0;
  double beta = 4.0;
  int type = 0;
  bool refineOnlyBoundary = false;
  int boundaryLayers = 2;
  bool debug = false;
  double minElemAngle = 15.0;
  double maxElemAngle = 165.0;
  double transitionAngle = 15.0;
};

struct SymmetryPlane {
  bool enabled = true;
  int axis = 0;
  double value = 0.0;
};
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
    m_element_size = 1.0;
    m_dirty = false;
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
  Section* getSection(const int &s){return m_section[s];}
  int getSectionCount(){return m_section.size();}
  void addSection(Section* section){m_section.push_back(section);}
  void delSection(const int &s){m_section.erase(m_section.begin() + s);}
  Section* findSectionById(int id);
  const bool &isAnyMesh()const {return have_meshes;}
   Part* getPart(const int &i) {return m_part[i];}
  Part* getLastPart(){return m_part[m_part.size()-1];} 
  Part & getPartRef(const int &i) {return *m_part[i];}
  virtual void AddBoxLength				(int tag, Vec3_t const &V, double Lx, double Ly, double Lz,double r, double Density,
                                double h,int type, int rotation, bool random, bool Fixed){};									//Add a cube of particles with a defined dimensions

  int getBCCount(){return m_bc.size();}
  BoundaryCondition* getBC(const int &i){return m_bc[i];}
  void delBC(const int &i){
    delete m_bc[i];
    m_bc.erase(m_bc.begin() + i);
  }

  int getICCount(){return m_ic.size();}
  InitialCondition* getIC(const int &i){return m_ic[i];}
  void delIC(const int &i){
    delete m_ic[i];
    m_ic.erase(m_ic.begin() + i);
  }
  int getStepCount(){return m_steps.size();}
  Step* getStep(const int &i){return m_steps[i];}
  void addStep(Step *step){m_steps.push_back(step);}
    
  bool & getHasName(){return m_hasname;}
  virtual ~Model(){}
  void setName (string n){m_prev_name=m_name; m_name=n; m_hasname=true;}
  void setNoSaveAs(){m_prev_name = m_name;}
  string getName(){return m_name;}
  string getPrevName(){return m_prev_name;}
  void setFilePath(const std::string &path){m_file_path = path;}
  const std::string & getFilePath() const {return m_file_path;}
  bool isDirty() const { return m_dirty; }
  void markDirty() { m_dirty = true; }
  void markSaved() { m_dirty = false; }
  std::string getBaseDir() const {
    if (m_file_path.empty()) return ".";
    return std::filesystem::path(m_file_path).parent_path().string();
  }
  void setElementSize(const double &element_size){m_element_size = element_size;}
  const double & getElementSize() const {return m_element_size;}
  void setTwoDMeshGenerator(TwoDMeshGenerator generator) { m_2d_mesh_generator = generator; }
  TwoDMeshGenerator getTwoDMeshGenerator() const { return m_2d_mesh_generator; }
  void addBoundaryCondition(BoundaryCondition *make_double2bc);
  void addInitialCondition(InitialCondition *bc){m_ic.push_back(bc);}
  ContactProperties & contactProps(){return m_contact_props;}
  const ContactProperties & contactProps() const {return m_contact_props;}
  RemeshingSettings & remeshing(){return m_remeshing;}
  const RemeshingSettings & remeshing() const {return m_remeshing;}
  std::vector<SymmetryPlane> & symmetryPlanes(){return m_symmetry_planes;}
  const std::vector<SymmetryPlane> & symmetryPlanes() const {return m_symmetry_planes;}
  SymmetryPlane* findSymmetryPlaneByAxis(int axis) {
    for (std::size_t i = 0; i < m_symmetry_planes.size(); ++i) {
      if (m_symmetry_planes[i].axis == axis) return &m_symmetry_planes[i];
    }
    return nullptr;
  }
  const SymmetryPlane* findSymmetryPlaneByAxis(int axis) const {
    for (std::size_t i = 0; i < m_symmetry_planes.size(); ++i) {
      if (m_symmetry_planes[i].axis == axis) return &m_symmetry_planes[i];
    }
    return nullptr;
  }
  void upsertSymmetryPlane(const SymmetryPlane &plane) {
    for (std::size_t i = 0; i < m_symmetry_planes.size(); ++i) {
      if (m_symmetry_planes[i].axis == plane.axis) {
        m_symmetry_planes[i] = plane;
        return;
      }
    }
    m_symmetry_planes.push_back(plane);
  }
  bool removeSymmetryPlaneByAxis(int axis) {
    for (std::vector<SymmetryPlane>::iterator it = m_symmetry_planes.begin();
         it != m_symmetry_planes.end(); ++it) {
      if (it->axis == axis) {
        m_symmetry_planes.erase(it);
        return true;
      }
    }
    return false;
  }


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
  std::vector <Section*>     m_section;
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
  std::vector <Step*>              m_steps;
  
  model_type m_modeltype;
  
  bool have_meshes;
  
  //IO AND SETTING THINGS
  bool m_hasname;
  bool m_dirty;
  string m_name;
  string m_prev_name;
  string m_file_path;
  AnalysisType m_analysisType = Solid3D;
  double m_element_size = 1.0;
  TwoDMeshGenerator m_2d_mesh_generator = MeshGeneratorMeshAdapt;
  ContactProperties m_contact_props;
  RemeshingSettings m_remeshing;
  std::vector<SymmetryPlane> m_symmetry_planes;


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
