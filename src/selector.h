#ifndef _SELECTOR_H_
#define _SELECTOR_H_

#include "imgui.h"

#include <algorithm>
#include <vector>

class Node;
class Element;

enum class SelectionMode {
  Pick = 0,
  Box = 1
};

enum class SelectionTarget {
  Node = 0,
  Element = 1,
  Part = 2,
  Geometry = 3
};

class Selector {
public:
  void setMode(SelectionMode mode) { m_mode = mode; }
  SelectionMode getMode() const { return m_mode; }
  bool isBoxMode() const { return m_mode == SelectionMode::Box; }
  bool isPickMode() const { return m_mode == SelectionMode::Pick; }

  void setTarget(SelectionTarget target) { m_target = target; }
  SelectionTarget getTarget() const { return m_target; }
  bool isNodeTarget() const { return m_target == SelectionTarget::Node; }
  bool isElementTarget() const { return m_target == SelectionTarget::Element; }

  void beginBoxSelection(const ImVec2& start) {
    m_boxSelecting = true;
    m_boxStart = start;
    m_boxEnd = start;
  }

  void updateBoxSelection(const ImVec2& current) {
    m_boxEnd = current;
  }

  void finishBoxSelection() {
    m_boxSelecting = false;
  }

  bool isBoxSelecting() const { return m_boxSelecting; }
  const ImVec2& getBoxStart() const { return m_boxStart; }
  const ImVec2& getBoxEnd() const { return m_boxEnd; }

  void clearSelection() {
    m_selectedNodes.clear();
    m_selectedElements.clear();
  }
  void setSingleNode(Node* node) {
    m_selectedNodes.clear();
    if (node != nullptr) {
      m_selectedNodes.push_back(node);
    }
  }
  bool containsNode(Node* node) const {
    return std::find(m_selectedNodes.begin(), m_selectedNodes.end(), node) != m_selectedNodes.end();
  }
  void addNode(Node* node) {
    if (node != nullptr && !containsNode(node)) {
      m_selectedNodes.push_back(node);
    }
  }
  void removeNode(Node* node) {
    m_selectedNodes.erase(std::remove(m_selectedNodes.begin(), m_selectedNodes.end(), node),
                          m_selectedNodes.end());
  }
  void toggleNode(Node* node) {
    if (node == nullptr) {
      return;
    }
    if (containsNode(node)) {
      removeNode(node);
    } else {
      addNode(node);
    }
  }
  void setSelectedNodes(const std::vector<Node*>& nodes) { m_selectedNodes = nodes; }
  const std::vector<Node*>& getSelectedNodes() const { return m_selectedNodes; }
  std::vector<Node*>& getSelectedNodes() { return m_selectedNodes; }
  int getSelectedNodeCount() const { return static_cast<int>(m_selectedNodes.size()); }

  void setSingleElement(Element* element) {
    m_selectedElements.clear();
    if (element != nullptr) {
      m_selectedElements.push_back(element);
    }
  }
  bool containsElement(Element* element) const {
    return std::find(m_selectedElements.begin(), m_selectedElements.end(), element) != m_selectedElements.end();
  }
  void addElement(Element* element) {
    if (element != nullptr && !containsElement(element)) {
      m_selectedElements.push_back(element);
    }
  }
  void removeElement(Element* element) {
    m_selectedElements.erase(std::remove(m_selectedElements.begin(), m_selectedElements.end(), element),
                             m_selectedElements.end());
  }
  void toggleElement(Element* element) {
    if (element == nullptr) {
      return;
    }
    if (containsElement(element)) {
      removeElement(element);
    } else {
      addElement(element);
    }
  }
  void setSelectedElements(const std::vector<Element*>& elements) { m_selectedElements = elements; }
  const std::vector<Element*>& getSelectedElements() const { return m_selectedElements; }
  std::vector<Element*>& getSelectedElements() { return m_selectedElements; }
  int getSelectedElementCount() const { return static_cast<int>(m_selectedElements.size()); }

private:
  SelectionMode m_mode = SelectionMode::Pick;
  SelectionTarget m_target = SelectionTarget::Node;
  bool m_boxSelecting = false;
  ImVec2 m_boxStart = ImVec2(0.0f, 0.0f);
  ImVec2 m_boxEnd = ImVec2(0.0f, 0.0f);
  std::vector<Node*> m_selectedNodes;
  std::vector<Element*> m_selectedElements;
};

#endif
