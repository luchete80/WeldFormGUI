#ifndef _MESH_H_
#define _MESH_H_

/////////////// FINITE ELEMENT MESH ///////////


#include <vector>
//#include <glm/gtc/matrix_transform.hpp>
#include "../common/math/math.h"
#include "Element.h"

#include "Entity.h"
#include "Set.h"

//ALSO COULD BE DISTINGHUISED BY ELEMENT NODES COUNT
enum Mesh_Type {FEM=0,SPH};

class Element;
class Node;
class Model;
class MeshViewer;
class Part; //If selected to identify

class Mesh{
  friend class Model;
  friend class GraphicMesh;
  
public:
  Mesh(){
    m_type = FEM;
    m_node_count = m_elem_count = 0;
  }
  void genFromGmshModel();
  //void initValues(  std::vector <Node*>    m_node, //LOCATED ON MODEL SPACE!!!!
  //                  std::vector < std::vector <int> >      elnod_h);
  //This function does not create the pointers
  void assignValues( std::vector <Node*>     m_node, //LOCATED ON MODEL SPACE!!!!
                     std::vector <Element* > m_elem);
  //void addNode();
  void addBoxLength(Vector3f L, Vector3f V, double r);
  void addPlane(double x0, double y0, double lx, double ly, double d);
  
  void addNode(double x, double y, double z = 0, int id = -1);
  void addNode(Node *node);
  void addElement(Element *, bool alloc = true);
  void addQuad(int v0, int v1, int v2, int v3, id = -1);
  
  const int & getNodeCount()const {return m_node_count;}
  const int & getElemCount()const {return m_elem_count;}
  Node*     getNode(const int &i){return m_node[i];} 
  Element*  getElem(const int &i){return m_elem[i];}
  Part& getPart(){return *m_part;}
  
  const Vector3f& getNodePos(const int &i)const; //Used by the renderer to get Node positions, this calls to NODE POINTER
  Mesh_Type& getType(){return m_type;}
    
  //vtkSmartPointer<vtkActor> getActor(){return mesh_actor;}
protected:
  int m_node_count;
  int m_elem_count;
  std::vector <Node*>    m_node; //LOCATED ON MODEL SPACE!!!!
  std::vector <Element*> m_elem; //BUT THIS ARE FROM THE PART!!
  std::vector <int>      elnod_h;
  
  std::vector < Set<Element> >  m_set_elem;
  std::vector <Node*>     m_set_node;
  int                     m_gmsh_id; //GMSH entity ID 
  Part*                   m_part;
  Mesh_Type               m_type;
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
    
};

#endif
