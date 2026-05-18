#ifndef WFGUI_WORKFLOW_API_H
#define WFGUI_WORKFLOW_API_H

#include "../App/App.h"
#include "../geom/Geom.h"
#include "../model/Mesh.h"
#include "../model/Node.h"
#include "../model/Part.h"

#include <set>
#include <string>
#include <vector>

namespace wfgui {
namespace workflow {

inline bool point_in_box(double x,
                         double y,
                         double z,
                         double xmin,
                         double ymin,
                         double zmin,
                         double xmax,
                         double ymax,
                         double zmax)
{
  const double lo_x = (xmin < xmax) ? xmin : xmax;
  const double hi_x = (xmin < xmax) ? xmax : xmin;
  const double lo_y = (ymin < ymax) ? ymin : ymax;
  const double hi_y = (ymin < ymax) ? ymax : ymin;
  const double lo_z = (zmin < zmax) ? zmin : zmax;
  const double hi_z = (zmin < zmax) ? zmax : zmin;

  return x >= lo_x && x <= hi_x &&
         y >= lo_y && y <= hi_y &&
         z >= lo_z && z <= hi_z;
}

inline int next_node_set_id(Mesh* mesh)
{
  if (mesh == nullptr)
    return 0;

  int max_id = -1;
  for (int i = 0; i < mesh->getNodeSetCount(); ++i) {
    if (mesh->getNodeSet(i).getId() > max_id)
      max_id = mesh->getNodeSet(i).getId();
  }
  return max_id + 1;
}

inline int next_element_set_id(Mesh* mesh)
{
  if (mesh == nullptr)
    return 0;

  int max_id = -1;
  for (int i = 0; i < mesh->getElementSetCount(); ++i) {
    if (mesh->getElementSet(i).getId() > max_id)
      max_id = mesh->getElementSet(i).getId();
  }
  return max_id + 1;
}

inline int store_node_set(Mesh* mesh, NodeSet& node_set, int set_id)
{
  node_set.setEntityId(set_id);
  mesh->addNodeSet(node_set);
  return mesh->getNodeSet(mesh->getNodeSetCount() - 1).getId();
}

inline int store_element_set(Mesh* mesh, ElementSet& element_set, int set_id)
{
  element_set.setEntityId(set_id);
  mesh->addElementSet(element_set);
  return mesh->getElementSet(mesh->getElementSetCount() - 1).getId();
}

} // namespace workflow
} // namespace wfgui

inline Model* get_active_model()
{
  return &getApp().getActiveModel();
}

inline void add_part_to_active_model(Part* part)
{
  if (part == nullptr)
    return;

  Model* model = get_active_model();
  if (model != nullptr)
    model->addPart(part);
}

inline void request_view_update()
{
  getApp().Update();
}

inline Part* create_empty_mesh_part()
{
  Mesh* mesh = new Mesh();
  return new Part(mesh);
}

inline Part* create_rectangle_part(double dx,
                                   double dy,
                                   double ox = 0.0,
                                   double oy = 0.0,
                                   double oz = 0.0)
{
  Geom* geom = new Geom();
  geom->LoadRectangle(dx, dy, ox, oy, oz);
  return new Part(geom);
}

inline Part* import_step_part(const std::string& step_path)
{
  Geom* geom = new Geom();
  if (!geom->LoadSTEP(step_path)) {
    delete geom;
    return nullptr;
  }
  return new Part(geom);
}

inline Part* import_step_part_at(const std::string& step_path,
                                 double origin_x,
                                 double origin_y,
                                 double origin_z)
{
  Geom* geom = new Geom();
  if (!geom->LoadSTEP(step_path, origin_x, origin_y, origin_z)) {
    delete geom;
    return nullptr;
  }
  return new Part(geom);
}

inline int add_node_set_from_indices(Mesh* mesh,
                                     const std::string& name,
                                     const std::vector<int>& node_indices,
                                     int set_id = -1)
{
  if (mesh == nullptr)
    return -1;

  NodeSet node_set(mesh);
  node_set.setLabel(name);
  for (int index : node_indices) {
    if (index < 0 || index >= mesh->getNodeCount())
      continue;
    node_set.add(mesh->getNode(index));
  }

  const int resolved_id = (set_id >= 0) ? set_id : wfgui::workflow::next_node_set_id(mesh);
  return wfgui::workflow::store_node_set(mesh, node_set, resolved_id);
}

inline int add_element_set_from_indices(Mesh* mesh,
                                        const std::string& name,
                                        const std::vector<int>& element_indices,
                                        int set_id = -1)
{
  if (mesh == nullptr)
    return -1;

  ElementSet element_set(mesh);
  element_set.setLabel(name);
  for (int index : element_indices) {
    if (index < 0 || index >= mesh->getElemCount())
      continue;
    element_set.add(mesh->getElem(index));
  }

  const int resolved_id = (set_id >= 0) ? set_id : wfgui::workflow::next_element_set_id(mesh);
  return wfgui::workflow::store_element_set(mesh, element_set, resolved_id);
}

inline int add_node_set_from_ids(Mesh* mesh,
                                 const std::string& name,
                                 const std::vector<int>& node_ids,
                                 int set_id = -1)
{
  if (mesh == nullptr)
    return -1;

  const std::set<int> wanted(node_ids.begin(), node_ids.end());
  NodeSet node_set(mesh);
  node_set.setLabel(name);
  for (int i = 0; i < mesh->getNodeCount(); ++i) {
    Node* node = mesh->getNode(i);
    if (node != nullptr && wanted.find(node->getId()) != wanted.end())
      node_set.add(node);
  }

  const int resolved_id = (set_id >= 0) ? set_id : wfgui::workflow::next_node_set_id(mesh);
  return wfgui::workflow::store_node_set(mesh, node_set, resolved_id);
}

inline int add_element_set_from_ids(Mesh* mesh,
                                    const std::string& name,
                                    const std::vector<int>& element_ids,
                                    int set_id = -1)
{
  if (mesh == nullptr)
    return -1;

  const std::set<int> wanted(element_ids.begin(), element_ids.end());
  ElementSet element_set(mesh);
  element_set.setLabel(name);
  for (int i = 0; i < mesh->getElemCount(); ++i) {
    Element* element = mesh->getElem(i);
    if (element != nullptr && wanted.find(element->getId()) != wanted.end())
      element_set.add(element);
  }

  const int resolved_id = (set_id >= 0) ? set_id : wfgui::workflow::next_element_set_id(mesh);
  return wfgui::workflow::store_element_set(mesh, element_set, resolved_id);
}

inline std::vector<int> find_node_indices_in_box(Mesh* mesh,
                                                 double xmin,
                                                 double ymin,
                                                 double zmin,
                                                 double xmax,
                                                 double ymax,
                                                 double zmax)
{
  std::vector<int> indices;
  if (mesh == nullptr)
    return indices;

  for (int i = 0; i < mesh->getNodeCount(); ++i) {
    Node* node = mesh->getNode(i);
    if (node == nullptr)
      continue;

    const Vector3f& pos = node->getPos();
    if (wfgui::workflow::point_in_box(pos.x, pos.y, pos.z,
                                      xmin, ymin, zmin,
                                      xmax, ymax, zmax)) {
      indices.push_back(i);
    }
  }

  return indices;
}

inline std::vector<int> find_node_ids_in_box(Mesh* mesh,
                                             double xmin,
                                             double ymin,
                                             double zmin,
                                             double xmax,
                                             double ymax,
                                             double zmax)
{
  std::vector<int> ids;
  if (mesh == nullptr)
    return ids;

  for (int i = 0; i < mesh->getNodeCount(); ++i) {
    Node* node = mesh->getNode(i);
    if (node == nullptr)
      continue;

    const Vector3f& pos = node->getPos();
    if (wfgui::workflow::point_in_box(pos.x, pos.y, pos.z,
                                      xmin, ymin, zmin,
                                      xmax, ymax, zmax)) {
      ids.push_back(node->getId());
    }
  }

  return ids;
}

inline int add_node_set_from_box(Mesh* mesh,
                                 const std::string& name,
                                 double xmin,
                                 double ymin,
                                 double zmin,
                                 double xmax,
                                 double ymax,
                                 double zmax,
                                 int set_id = -1)
{
  const std::vector<int> node_indices =
    find_node_indices_in_box(mesh, xmin, ymin, zmin, xmax, ymax, zmax);
  return add_node_set_from_indices(mesh, name, node_indices, set_id);
}

#endif
