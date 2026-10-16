#ifndef _MESH_H_
#define _MESH_H_

/////////////// FINITE ELEMENT MESH ///////////


#include <vector>
#include <memory>
#include <map>
#include <optional>
#include <string>
#include <set>
//#include <glm/gtc/matrix_transform.hpp>
#include "../common/math/math_.h" //// DO NOT CALL IT MATH BECAUSE OF SWIG PROBLEMS
#include "Element.h"
#include "Node.h"
#include <cstdint>

#include "Entity.h"
#include "Set.h"

//ALSO COULD BE DISTINGHUISED BY ELEMENT NODES COUNT
enum Mesh_Type {FEM=0,SPH};
enum class MeshNastranExportPurpose { Runtime, ProjectSnapshot, InitialStatePackage };

class Element;
class Node;
class Model;
class MeshViewer;
class Part; //If selected to identify

struct InitialStateMeshStorage {
  std::vector<std::unique_ptr<Node>> nodes;
  std::vector<std::unique_ptr<Element>> elements;
};

struct InitialStateMeshProvenance {
  bool importedFromVtk = false;
  std::string sourcePath;
  std::optional<int> sourcePartId;
  bool hasPlasticStrainHistory = false;
  bool historyValid = false;
  std::string historyDiagnostic;
  bool historyMaterialDecisionMade = false;
  bool preserveHistoryWithMaterial = false;
  int historyEffectiveMaterialIndex = -1;
  std::string historyMaterialFingerprint;
  int historyDecisionSectionId = -1;
  int historyDecisionSectionMaterialIndex = -1;
  bool historyDecisionPartWasElastic = true;
};

class Mesh{
  friend class Model;
  friend class GraphicMesh;
  
public:
  Mesh(){
    m_type = FEM;
    m_node_count = m_elem_count = 0;
  }
  void genFromGmshModel();
  void genFromNastranFile(const std::string& filename);
  //void initValues(  std::vector <Node*>    m_node, //LOCATED ON MODEL SPACE!!!!
  //                  std::vector < std::vector <int> >      elnod_h);
  //This function does not create the pointers
  void assignValues( std::vector <Node*>     m_node, //LOCATED ON MODEL SPACE!!!!
                     std::vector <Element* > m_elem);
  void adoptInitialStateStorage(std::vector<std::unique_ptr<Node>>&& nodes,
                                std::vector<std::unique_ptr<Element>>&& elements);
  std::shared_ptr<InitialStateMeshStorage> getInitialStateStorageOwner() const {
    return m_initial_state_storage;
  }
  void setInitialStateProvenance(const InitialStateMeshProvenance& provenance);
  void setInitialStateMaterialDecision(bool preserve,
                                       int effectiveMaterialIndex,
                                       const std::string& materialFingerprint,
                                       int sectionId,
                                       int sectionMaterialIndex,
                                       bool partWasElastic) {
    if (!m_initial_state_provenance) m_initial_state_provenance = InitialStateMeshProvenance{};
    m_initial_state_provenance->historyMaterialDecisionMade = true;
    m_initial_state_provenance->preserveHistoryWithMaterial = preserve;
    m_initial_state_provenance->historyEffectiveMaterialIndex = preserve ? effectiveMaterialIndex : -1;
    m_initial_state_provenance->historyMaterialFingerprint = preserve ? materialFingerprint : std::string{};
    m_initial_state_provenance->historyDecisionSectionId = sectionId;
    m_initial_state_provenance->historyDecisionSectionMaterialIndex = sectionMaterialIndex;
    m_initial_state_provenance->historyDecisionPartWasElastic = partWasElastic;
  }
  void discardInitialPlasticStrainHistory() {
    for (Element* element : m_elem) {
      if (element == nullptr) continue;
      element->clearInitialPlasticStrain();
    }
    if (!m_initial_state_provenance) m_initial_state_provenance = InitialStateMeshProvenance{};
    m_initial_state_provenance->hasPlasticStrainHistory = false;
    m_initial_state_provenance->historyValid = false;
    m_initial_state_provenance->historyDiagnostic = "El usuario descartó explícitamente la historia pl_strain.";
    setInitialStateMaterialDecision(false, -1, std::string{}, -1, -1, true);
  }
  const std::optional<InitialStateMeshProvenance>& getInitialStateProvenance() const {
    return m_initial_state_provenance;
  }
  bool hasInitialPlasticStrainHistory() const {
    if (m_initial_state_provenance && m_initial_state_provenance->hasPlasticStrainHistory)
      return true;
    for (const Element* element : m_elem) {
      if (element != nullptr && element->getInitialPlasticStrain().has_value()) return true;
    }
    return false;
  }
  // Tracks topology by object identity/connectivity, so coordinate transforms and
  // entity ID renumbering preserve history while mesh edits do not.
  bool initialStateTopologyMatches() const;
  bool validateInitialStateTopology();
  bool rotateNodesAroundAxis(double angleDeg, const Vector3f& axis,
                             const Vector3f& pivot);
  bool canRotateNodesAroundAxis(double angleDeg, const Vector3f& axis,
                                const Vector3f& pivot) const;
  void registerSourceVtkCellElement(std::int64_t sourceCellId, Element* element) {
    if (element != nullptr) m_source_vtk_cell_to_element[sourceCellId] = element;
  }
  void clearSourceVtkCellElements() { m_source_vtk_cell_to_element.clear(); }
  Element* findElementBySourceVtkCellId(std::int64_t sourceCellId) const {
    const auto found = m_source_vtk_cell_to_element.find(sourceCellId);
    return found == m_source_vtk_cell_to_element.end() ? nullptr : found->second;
  }
  //void addNode();
  virtual void addBoxLength(Vector3f L, Vector3f V, double r);
  void addPlane(double x0, double y0, double lx, double ly, double d);
  
  void addNode(double x, double y, double z = 0, int id = -1);
  void addNode(Node *node);
  void addElement(Element *, bool alloc = true);
  bool removeElement(Element* element);
  int removeElements(const std::vector<Element*>& elements);
  void addQuad(int v0, int v1, int v2, int v3, int id = -1);
  
  const int & getNodeCount()const {return m_node_count;}
  const int & getElemCount()const {return m_elem_count;}
  Node*     getNode(const int &i){return m_node[i];} 
  Element*  getElem(const int &i){return m_elem[i];}
  const Element* getElem(const int &i) const {return m_elem[i];}
  Part& getPart(){return *m_part;}
  int getNodeSetCount() const { return static_cast<int>(m_node_sets.size()); }
  NodeSet& getNodeSet(const int& i){ return m_node_sets[i]; }
  const NodeSet& getNodeSet(const int& i) const { return m_node_sets[i]; }
  void addNodeSet(const NodeSet& set){ m_node_sets.push_back(set); }
  bool removeNodeSetById(int id) {
    for (std::vector<NodeSet>::iterator it = m_node_sets.begin(); it != m_node_sets.end(); ++it) {
      if (it->getId() == id) {
        m_node_sets.erase(it);
        return true;
      }
    }
    return false;
  }
  NodeSet* findNodeSetById(int id) {
    for (std::size_t i = 0; i < m_node_sets.size(); ++i) {
      if (m_node_sets[i].getId() == id) return &m_node_sets[i];
    }
    return nullptr;
  }
  const NodeSet* findNodeSetById(int id) const {
    for (std::size_t i = 0; i < m_node_sets.size(); ++i) {
      if (m_node_sets[i].getId() == id) return &m_node_sets[i];
    }
    return nullptr;
  }
  int getElementSetCount() const { return static_cast<int>(m_element_sets.size()); }
  ElementSet& getElementSet(const int& i){ return m_element_sets[i]; }
  const ElementSet& getElementSet(const int& i) const { return m_element_sets[i]; }
  void addElementSet(const ElementSet& set){ m_element_sets.push_back(set); }
  bool removeElementSetById(int id) {
    for (std::vector<ElementSet>::iterator it = m_element_sets.begin(); it != m_element_sets.end(); ++it) {
      if (it->getId() == id) {
        m_element_sets.erase(it);
        return true;
      }
    }
    return false;
  }
  ElementSet* findElementSetById(int id) {
    for (std::size_t i = 0; i < m_element_sets.size(); ++i) {
      if (m_element_sets[i].getId() == id) return &m_element_sets[i];
    }
    return nullptr;
  }
  const ElementSet* findElementSetById(int id) const {
    for (std::size_t i = 0; i < m_element_sets.size(); ++i) {
      if (m_element_sets[i].getId() == id) return &m_element_sets[i];
    }
    return nullptr;
  }
  int getFaceSetCount() const { return static_cast<int>(m_face_sets.size()); }
  FaceSet& getFaceSet(const int& i){ return m_face_sets[i]; }
  const FaceSet& getFaceSet(const int& i) const { return m_face_sets[i]; }
  void addFaceSet(const FaceSet& set){ m_face_sets.push_back(set); }
  bool removeFaceSetById(int id) {
    for (std::vector<FaceSet>::iterator it = m_face_sets.begin(); it != m_face_sets.end(); ++it) {
      if (it->getId() == id) {
        m_face_sets.erase(it);
        return true;
      }
    }
    return false;
  }
  FaceSet* findFaceSetById(int id) {
    for (std::size_t i = 0; i < m_face_sets.size(); ++i) {
      if (m_face_sets[i].getId() == id) return &m_face_sets[i];
    }
    return nullptr;
  }
  const FaceSet* findFaceSetById(int id) const {
    for (std::size_t i = 0; i < m_face_sets.size(); ++i) {
      if (m_face_sets[i].getId() == id) return &m_face_sets[i];
    }
    return nullptr;
  }
  
  const int & getDim() const {return m_dim;}
  void setDim(const int &dim){m_dim = dim;}


  bool exportToNASTRAN(const std::string& filename,
                       MeshNastranExportPurpose purpose = MeshNastranExportPurpose::Runtime,
                       std::map<const Element*, int>* writtenElementIds = nullptr);
  
  const Vector3f& getNodePos(const int &i)const; //Used by the renderer to get Node positions, this calls to NODE POINTER
  Mesh_Type& getType(){return m_type;}
  
  
  bool exportToLSDYNA(const std::string & filename);
  void setFlipNormals(const bool & fn){m_flip_normals = fn;}
  const bool & getFlipNormals()const{return m_flip_normals;}
  //vtkSmartPointer<vtkActor> getActor(){return mesh_actor;}
protected:
  int m_node_count;
  int m_elem_count;
  
  int m_dim = 3;
  
  std::vector <Node*>    m_node; //LOCATED ON MODEL SPACE!!!!
  std::vector <Element*> m_elem; //BUT THIS ARE FROM THE PART!!
  std::shared_ptr<InitialStateMeshStorage> m_initial_state_storage;
  std::optional<InitialStateMeshProvenance> m_initial_state_provenance;
  std::map<std::int64_t, Element*> m_source_vtk_cell_to_element;
  std::map<const Element*, std::vector<const Node*>> m_initial_state_element_nodes;
  std::set<const Node*> m_initial_state_nodes;
  std::vector <int>      elnod_h;
  
  std::vector <ElementSet>  m_element_sets;
  std::vector <FaceSet>     m_face_sets;
  std::vector <NodeSet>   m_node_sets;
  std::vector <Node*>     m_set_node;
  int                     m_gmsh_id; //GMSH entity ID 
  Part*                   m_part;
  Mesh_Type               m_type;
  
  bool                    m_flip_normals = false;
  /*
  //VTK THING, for visualization
   vtkSmartPointer<vtkActor> mesh_actor;


  vtkSmartPointer<vtkPolyData> mesh_pdata;
  vtkSmartPointer<vtkPolyDataMapper> mesh_Mapper;


  vtkSmartPointer<vtkPoints> points;
  vtkSmartPointer<vtkCellArray> polys;
  vtkSmartPointer<vtkFloatArray> scalars;  

  */
};

class FEMMesh:
  public Mesh{
    
};


class SPHMesh:
  public Mesh{

public:
SPHMesh(){m_type=SPH;}
  virtual void addBoxLength(Vector3f L, Vector3f V, double r);
};

#endif
