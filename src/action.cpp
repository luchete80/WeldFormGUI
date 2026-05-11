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
