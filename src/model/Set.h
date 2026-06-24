#ifndef _SET_H_
#define _SET_H_

#include <string>
#include <vector>
#include <algorithm>
#include "Entity.h"
#include "Face.h"

class Mesh;
class Node;
class Element;

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
  void remove(T* item){
    m_item.erase(std::remove(m_item.begin(), m_item.end(), item), m_item.end());
  }
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
  void remove(Node* node){ Set<Node>::remove(node); }
  int getItemCount() const { return Set<Node>::getItemCount(); }
  Node* getItem(const int& i){ return Set<Node>::getItem(i); }
  const Node* getItem(const int& i) const { return Set<Node>::getItem(i); }
  const int& getId() const { return Entity::getId(); }
  const int& getID() const { return Entity::getID(); }
	  void setLabel(const std::string& label){ Set<Node>::setLabel(label); }
	  const std::string& getLabel() const { return Set<Node>::getLabel(); }
	  void setEntityId(int id) { Set<Node>::setEntityId(id); }
	  };

class ElementSet
:public Set<Element>{

public:
  using Entity::getId;
  using Entity::getID;
  ElementSet() = default;
  explicit ElementSet(Mesh* mesh) : Set<Element>(mesh) {}

  void add(Element* element){ Set<Element>::add(element); }
  void clear(){ Set<Element>::clear(); }
  void remove(Element* element){ Set<Element>::remove(element); }
  int getItemCount() const { return Set<Element>::getItemCount(); }
  Element* getItem(const int& i){ return Set<Element>::getItem(i); }
  const Element* getItem(const int& i) const { return Set<Element>::getItem(i); }
  const int& getId() const { return Entity::getId(); }
  const int& getID() const { return Entity::getID(); }
  void setLabel(const std::string& label){ Set<Element>::setLabel(label); }
  const std::string& getLabel() const { return Set<Element>::getLabel(); }
  void setEntityId(int id) { Set<Element>::setEntityId(id); }
};

class FaceSet : public Entity {
public:
  using Entity::getId;
  using Entity::getID;

  FaceSet() = default;
  explicit FaceSet(Mesh* mesh) : m_msh(mesh) {}

  Mesh* getMesh(){ return m_msh; }
  const Mesh* getMesh() const { return m_msh; }
  void setMesh(Mesh* mesh){ m_msh = mesh; }

  void add(const Face& face){ m_faces.push_back(face); }
  void clear(){ m_faces.clear(); }
  int getItemCount() const { return static_cast<int>(m_faces.size()); }
  Face* getItem(const int& i){ return &m_faces[i]; }
  const Face* getItem(const int& i) const { return &m_faces[i]; }
  void setEntityId(int id) { m_id = id; }
  void removeFacesForOwnerElementId(int ownerElementId) {
    m_faces.erase(
      std::remove_if(m_faces.begin(), m_faces.end(),
        [ownerElementId](const Face& face) {
          return face.getOwnerElementId() == ownerElementId;
        }),
      m_faces.end());
  }

  void setLabel(const std::string& label){ m_label = label; }
  const std::string& getLabel() const { return m_label; }

private:
  Mesh* m_msh = nullptr;
  std::vector<Face> m_faces;
  std::string m_label;
};

#endif
