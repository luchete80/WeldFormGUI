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
  NodeSet() = default;
  explicit NodeSet(Mesh* mesh) : Set<Node>(mesh) {}
  };

#endif
