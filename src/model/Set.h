#ifndef _SET_H_
#define _SET_H_

#include <string>
#include <vector>
#include "Entity.h"

class Mesh;
class Node;

template <typename T>
class Set:
public Entity{
  
public:
  Set() = default;
  explicit Set(Mesh *mesh) : m_msh(mesh) {}

  Mesh* getMesh(){return m_msh;}
  const Mesh* getMesh() const {return m_msh;}
  void setMesh(Mesh* mesh){m_msh = mesh;}

  void add(T* item){ if (item != nullptr) m_item.push_back(item); }
  void clear(){ m_item.clear(); }
  int getItemCount() const { return static_cast<int>(m_item.size()); }
  T* getItem(const int& i){ return m_item[i]; }
  const T* getItem(const int& i) const { return m_item[i]; }
  void setEntityId(int id) { m_id = id; }
  
  void setLabel(const std::string& label){ m_label = label; }
  const std::string& getLabel() const { return m_label; }
  
protected:
  Mesh* m_msh = nullptr;
  std::vector <T*> m_item;
  std::string m_label;
};


class NodeSet
:public Set<Node>{

public:
  using Entity::getId;
  using Entity::getID;
  NodeSet() = default;
  explicit NodeSet(Mesh* mesh) : Set<Node>(mesh) {}

  void add(Node* node){ Set<Node>::add(node); }
  void clear(){ Set<Node>::clear(); }
  int getItemCount() const { return Set<Node>::getItemCount(); }
  Node* getItem(const int& i){ return Set<Node>::getItem(i); }
  const Node* getItem(const int& i) const { return Set<Node>::getItem(i); }
  const int& getId() const { return Entity::getId(); }
  const int& getID() const { return Entity::getID(); }
  void setLabel(const std::string& label){ Set<Node>::setLabel(label); }
  const std::string& getLabel() const { return Set<Node>::getLabel(); }
  void setEntityId(int id) { Set<Node>::setEntityId(id); }
  };

#endif
