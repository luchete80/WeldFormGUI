#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include "action.h"

#include "editor.h"
#include "model/Mesh.h"
#include "model/Set.h"

CreateNodeSetAction::CreateNodeSetAction(Editor* editor, Mesh* mesh, const NodeSet& nodeSet)
  : Action("Create Node Set"),
    m_editor(editor),
    m_mesh(mesh),
    m_node_set(nodeSet)
{
}

bool CreateNodeSetAction::undo()
{
  if (m_editor == nullptr || m_mesh == nullptr) {
    return false;
  }

  if (!m_mesh->removeNodeSetById(m_node_set.getId())) {
    return false;
  }

  m_editor->clearSelectedNodeSet();
  return true;
}

bool CreateNodeSetAction::redo()
{
  if (m_editor == nullptr || m_mesh == nullptr) {
    return false;
  }

  if (m_mesh->findNodeSetById(m_node_set.getId()) == nullptr) {
    m_mesh->addNodeSet(m_node_set);
  }

  m_editor->selectNodeSetById(m_node_set.getId());
  return true;
}

CreateElementSetAction::CreateElementSetAction(Editor* editor, Mesh* mesh, const ElementSet& elementSet)
  : Action("Create Element Set"),
    m_editor(editor),
    m_mesh(mesh),
    m_element_set(elementSet)
{
}

bool CreateElementSetAction::undo()
{
  if (m_editor == nullptr || m_mesh == nullptr) {
    return false;
  }

  if (!m_mesh->removeElementSetById(m_element_set.getId())) {
    return false;
  }

  m_editor->clearSelectedElementSet();
  return true;
}

bool CreateElementSetAction::redo()
{
  if (m_editor == nullptr || m_mesh == nullptr) {
    return false;
  }

  if (m_mesh->findElementSetById(m_element_set.getId()) == nullptr) {
    m_mesh->addElementSet(m_element_set);
  }

  m_editor->selectElementSetById(m_element_set.getId());
  return true;
}

CreateFaceSetAction::CreateFaceSetAction(Editor* editor, Mesh* mesh, const FaceSet& faceSet)
  : Action("Create Face Set"),
    m_editor(editor),
    m_mesh(mesh),
    m_face_set(faceSet)
{
}

bool CreateFaceSetAction::undo()
{
  if (m_editor == nullptr || m_mesh == nullptr) {
    return false;
  }

  if (!m_mesh->removeFaceSetById(m_face_set.getId())) {
    return false;
  }

  m_editor->clearSelectedFaceSet();
  return true;
}

bool CreateFaceSetAction::redo()
{
  if (m_editor == nullptr || m_mesh == nullptr) {
    return false;
  }

  if (m_mesh->findFaceSetById(m_face_set.getId()) == nullptr) {
    m_mesh->addFaceSet(m_face_set);
  }

  m_editor->selectFaceSetById(m_face_set.getId());
  return true;
}
