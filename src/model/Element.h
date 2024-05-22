#ifndef _ELEMENT_H_
#define _ELEMENT_H_

#include <vector>
class Node;
class Model;
class Element {
  
friend class Model;
friend class Mesh;

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
protected:
  std::vector<Node*> m_node;  //Is like elnod
  int m_nodecount;
};

class Quad:
public Element {
public:
  Quad(){
    m_nodecount = 4;m_node.resize(4);}
  Quad(std::vector<Node*>nv){
    m_nodecount = 4;
    m_node.resize(4);
    for (int n=0;n<4;n++)
      m_node[n]=nv[n];
  }

};

#endif