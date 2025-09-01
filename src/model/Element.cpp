#include "Element.h"
#include "Node.h"

const int & Element::getNodeId(const int &i)const{
  return m_node[i]->getId();
}

Quad::Quad(Node *n0, Node *n1, Node *n2, Node *n3){ //Good fo python binding
  init();
  m_node[0]=n0;  m_node[1]=n1;  m_node[2]=n2;  m_node[3]=n3;
  
}

Line::Line(Node *n0, Node *n1){ //Good fo python binding
  init();
  m_node[0]=n0;  m_node[1]=n1;  
  
}

Tria::Tria(Node *n0, Node *n1, Node *n2){ //Good fo python binding
  init();
  m_node[0]=n0;  m_node[1]=n1;  m_node[2]=n2;  
  
}
