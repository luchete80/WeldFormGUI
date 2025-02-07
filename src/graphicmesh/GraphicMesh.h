#ifndef _GRAPHICMESH_H_
#define _GRAPHICMESH_H_

/////////////// FINITE ELEMENT MESH ///////////


#include <vector>
//#include <glm/gtc/matrix_transform.hpp>
#include "../common/math/math_.h"
#include "Element.h"

#include "Entity.h"
#include "Set.h"

#include <vtkSmartPointer.h>
#include <vtkActor.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>


// SPH
//https://examples.vtk.org/site/Cxx/Filtering/Glyph3D/
#include <vtkCubeSource.h>
#include <vtkSphereSource.h>
#include <vtkGlyph3D.h>

//https://examples.vtk.org/site/Cxx/GeometricObjects/Cube/

class Element;
class Node;
class Model;
class Mesh;
class VtkViewer;

//INHERIT SPH & FEM

class GraphicMesh{
  friend class Model;
  friend class VtkViewer;
public:
  GraphicMesh(){  
    m_needs_polydata = true;
    m_needs_actor = true; //needs actor to be shown
    mesh_actor = nullptr;
    mesh_pdata = nullptr;
  } //To create polydata and actor
  GraphicMesh(Mesh *Mesh);
  void initValues(  std::vector <Node*>    m_node, //LOCATED ON MODEL SPACE!!!!
                    std::vector < std::vector <int> >      elnod_h);
  //This function does not create the pointers
  void assignValues( std::vector <Node*>     m_node, //LOCATED ON MODEL SPACE!!!!
                     std::vector <Element* > m_elem);
  void addNode();
  void addBoxLength(Vector3f L, Vector3f V, double r);
  const int & getNodeCount()const {return m_node_count;}
  const int & getElemCount()const {return m_elem_count;}
  Node*     getNode(const int &i){return m_node[i];} 
  Element*  getElem(const int &i){return m_elem[i];} 
  bool & isActorNeeded(){return m_needs_actor;}
  const Vector3f& getNodePos(const int &i)const; //Used by the renderer to get Node positions, this calls to NODE POINTER
  virtual int createVTKPolyData(); //FROM EXTERNAL VALUES
  virtual int createVTKPolyData(Mesh &);
  void setActorNeeded(bool an){m_needs_actor=an;}
  //int createVTKPolyData_Tri(std::vector <std::array<float,3>>, std::vector <std::array<int,3>> elnodes);
  //int createVTKPolyData_Quad(std::vector <std::array<float,3>>, std::vector <std::array<int,4>> elnodes);
  
  vtkSmartPointer<vtkActor> getActor(){return mesh_actor;}
  Mesh* getMesh() {return m_mesh;}
  bool & isPolydataNeeded(){return m_needs_polydata;}
  void setPoints(Mesh &mesh); //NOT VIRTUAL
  void setViewer(VtkViewer* v){m_viewer = v;}
  
  
protected:
  bool                  m_needs_polydata;
  bool                  m_needs_actor;
  Mesh*                 m_mesh;
  bool                  m_needs_viewer;
  int m_node_count;
  int m_elem_count;
  std::vector <Node*>    m_node; //LOCATED ON MODEL SPACE!!!!
  std::vector <Element*> m_elem; //BUT THIS ARE FROM THE PART!!
  std::vector <int>      elnod_h;
  
  std::vector < Set<Element> >  m_set_elem;
  std::vector <Node*>     m_set_node;
  VtkViewer*              m_viewer;
  
  
  //VTK THING, for visualization
   vtkSmartPointer<vtkActor> mesh_actor;


  vtkSmartPointer<vtkPolyData> mesh_pdata;
  vtkSmartPointer<vtkPolyDataMapper> mesh_Mapper;

  
  //GENERAL FROM BASE CLASS
  vtkSmartPointer<vtkPoints> points;
  vtkSmartPointer<vtkCellArray> polys;
  vtkSmartPointer<vtkFloatArray> scalars;  
  


  
};

class GraphicSPHMesh:
public GraphicMesh {
public:
  GraphicSPHMesh();
  //GraphicSPHMesh(const double & radius);
  GraphicSPHMesh(Mesh*);
  virtual int createVTKPolyData(Mesh &mesh);
  
protected:
  vtkSmartPointer<vtkGlyph3D> m_glyph3D;  

  float m_radius;
};

#endif
