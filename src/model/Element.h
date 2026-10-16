#ifndef _ELEMENT_H_
#define _ELEMENT_H_

#include <vector>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <cmath>
#include "Entity.h"

class Node;
class Model;
class Element:
public Entity{ 
  
friend class Model;
friend class Mesh;
//enum elem_type ={2D_Plane_Stress, 2D_Plane_Strain, 3D_Solid};

public:
  using Entity::getId; //SWIG
  Element(){}
  Element(std::vector<Node*>nv){
    m_nodecount = nv.size();
    m_node.resize(nv.size());
    for (int n=0;n<nv.size();n++)
      m_node[n]=nv[n];
  }
  void initValues(std::vector<Node*>nv){ //TO INHERIT
    m_nodecount = nv.size();
    m_node.resize(m_nodecount);
    for (int n=0;n<m_nodecount;n++)
      m_node[n]=nv[n];
  }    
  
  const int & getNodeId(const int &i)const;
  const int & getNodeCount()const{return m_nodecount;}
  void setInitialPlasticStrain(double value) {
    if (!std::isfinite(value) || value < 0.0)
      throw std::invalid_argument("Initial plastic strain must be finite and non-negative.");
    m_initial_plastic_strain = value;
  }
  const std::optional<double>& getInitialPlasticStrain() const { return m_initial_plastic_strain; }
  void clearInitialPlasticStrain() { m_initial_plastic_strain.reset(); }
  void setSourceVtkCellId(std::int64_t cellId) { m_source_vtk_cell_id = cellId; }
  void clearSourceVtkCellId() { m_source_vtk_cell_id.reset(); }
  const std::optional<std::int64_t>& getSourceVtkCellId() const { return m_source_vtk_cell_id; }
protected:
  std::vector<Node*> m_node;  //Is like elnod
  int m_nodecount;
  // Stored on the element so selection, deletion and renumbering cannot shift
  // state onto a different cell.
  std::optional<double> m_initial_plastic_strain;
  std::optional<std::int64_t> m_source_vtk_cell_id;
  //elem_type m_type;
  
};

class Tria:
public Element {
public:
  void init(){
        m_nodecount = 3;m_node.resize(3);
  }
  Tria(){
    init();}
  Tria(std::vector<Node*>nv){
    initValues(nv);   
  }
  //Asumme existence of ech node
  Tria(Node *n0, Node *n1, Node *n2); //Good fo python binding 

};

class Quad:
public Element {
public:
  void init(){
        m_nodecount = 4;m_node.resize(4);
  }
  Quad(){
    init();}
  Quad(std::vector<Node*>nv){
    initValues(nv);   
  }
  //Asumme existence of ech node
  Quad(Node *n0, Node *n1, Node *n2, Node *n3); //Good fo python binding 

};

class Line:
public Element {
public:
  void init(){
        m_nodecount = 2;m_node.resize(2);
  }
  Line(){
    init();}
  Line(std::vector<Node*>nv){
    initValues(nv);   
  }
  //Asumme existence of ech node
  Line(Node *n0, Node *n1); //Good fo python binding 

};

#endif
