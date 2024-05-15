#include "Element.h"
#include "Node.h"

const int & Element::getNodeId(const int &i)const{
  return m_node[i]->getId();
}