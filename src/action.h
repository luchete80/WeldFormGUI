#ifndef _ACTION_H_
#define _ACTION_H_

#include <string>
#include <utility>

#include "model/Set.h"

class Editor;
class Mesh;

class Action {
public:
  explicit Action(std::string description) : m_description(std::move(description)) {}
  virtual ~Action() = default;

  const std::string& getDescription() const { return m_description; }
  virtual bool undo() = 0;
  virtual bool redo() = 0;

protected:
  std::string m_description;
};

class CreateNodeSetAction : public Action {
public:
  CreateNodeSetAction(Editor* editor, Mesh* mesh, const NodeSet& nodeSet);

  bool undo() override;
  bool redo() override;

private:
  Editor* m_editor = nullptr;
  Mesh* m_mesh = nullptr;
  NodeSet m_node_set;
};

#endif
