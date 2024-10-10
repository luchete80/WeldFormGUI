#ifndef _GRAPHICMESH_H_
#define _GRAPHICMESH_H_

/////////////// FINITE ELEMENT MESH ///////////


#include <vector>
//#include <glm/gtc/matrix_transform.hpp>
#include "../common/math/math.h"
#include "Element.h"

#include "Entity.h"
#include "Set.h"

#include <vtkSmartPointer.h>
#include <vtkActor.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>

//https://examples.vtk.org/site/Cxx/GeometricObjects/Cube/

class Element;
class Node;
class Model;

class GraphicMesh{
  friend class Model;
public:
  GraphicMesh(){}
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
  
  const Vector3f& getNodePos(const int &i)const; //Used by the renderer to get Node positions, this calls to NODE POINTER
  int createVTKPolyData(); //FROM EXTERNAL VALUES
  int createVTKPolyData(Mesh *);
  //int createVTKPolyData_Tri(std::vector <std::array<float,3>>, std::vector <std::array<int,3>> elnodes);
  //int createVTKPolyData_Quad(std::vector <std::array<float,3>>, std::vector <std::array<int,4>> elnodes);
  
  vtkSmartPointer<vtkActor> getActor(){return mesh_actor;}
protected:
  int m_node_count;
  int m_elem_count;
  std::vector <Node*>    m_node; //LOCATED ON MODEL SPACE!!!!
  std::vector <Element*> m_elem; //BUT THIS ARE FROM THE PART!!
  std::vector <int>      elnod_h;
  
  std::vector < Set<Element> >  m_set_elem;
  std::vector <Node*>     m_set_node;
  
  
  //VTK THING, for visualization
   vtkSmartPointer<vtkActor> mesh_actor;


  vtkSmartPointer<vtkPolyData> mesh_pdata;
  vtkSmartPointer<vtkPolyDataMapper> mesh_Mapper;


  vtkSmartPointer<vtkPoints> points;
  vtkSmartPointer<vtkCellArray> polys;
  vtkSmartPointer<vtkFloatArray> scalars;  

  
};

#endif
