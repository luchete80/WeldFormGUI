#ifndef _ELEMENT_H_
#define _ELEMENT_H_

class Node;

class Element {
public:
  Element(){}
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