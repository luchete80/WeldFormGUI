#ifndef _ELEMENT_H_
#define _ELEMENT_H_

#include <vector>
class Node;
class Model;
class Element {
  
friend class Model;
friend class Mesh;
//enum elem_type ={2D_Plane_Stress, 2D_Plane_Strain, 3D_Solid};

public:
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
protected:
  std::vector<Node*> m_node;  //Is like elnod
  int m_nodecount;
  //elem_type m_type;
  
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

#endif
