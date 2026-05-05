#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "ImGuiFileDialog.h"


#ifndef WIN32
//#include "freetypeGL.h"
#endif

#include "editor.h"
#include <sstream>

//#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>

#include "io/ModelWriter.h"
#include "io/ModelReader.h"
#include "io/InputWriter.h"
#include "LSDynaWriter.h"

//#include "SceneView.h"

//#include "ViewportWindow.h"
#include "Job.h"

#include<iostream>
#include <thread>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <system_error>
#include <chrono>
#include <cmath>
#include <limits>
#include <unordered_set>

#include <gmsh.h>

#include "geom/vtkOCCTGeom.h"
#include "VtkViewer.h"

#include "SPHModel.h"


#include "Part.h"
#include "Node.h"
#include "GraphicMesh.h"

#include "console.h"

#include "App.h"

#include "Condition.h"
#include "BoundaryCondition.h"

#include "graphics/TransformGizmo.h"

#include "Material_Db.h"

//#include "graphics/ModelGizmo.h"

#include <vtkArrowSource.h>
#include <vtkBillboardTextActor3D.h>
#include <vtkCellArray.h>
#include <vtkCenterOfMass.h>
#include <vtkMapper.h>
#include <vtkMath.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkTextProperty.h>


//~ #include <GModel.h>
//~ #include <GModelIO_OCC.h>
//~ #include <TopoDS_Shape.hxx>

using namespace std;
//glm::mat4 trans_mat[1000]; //test
namespace fs = std::filesystem;

namespace {
int curveNodeCountFromElementSize(int curveTag, double elementSize) {
  if (elementSize <= 0.0) return 2;

  double xmin = 0.0, ymin = 0.0, zmin = 0.0;
  double xmax = 0.0, ymax = 0.0, zmax = 0.0;
  gmsh::model::getBoundingBox(1, curveTag, xmin, ymin, zmin, xmax, ymax, zmax);

  const double dx = xmax - xmin;
  const double dy = ymax - ymin;
  const double dz = zmax - zmin;
  const double approxLength = std::sqrt(dx * dx + dy * dy + dz * dz);

  return std::max(2, static_cast<int>(std::ceil(approxLength / elementSize)) + 1);
}

void applyMeshSizeToCurrentGmshModel(double elementSize) {
  if (elementSize <= 0.0) return;

  std::vector<std::pair<int, int>> pointEntities;
  gmsh::model::getEntities(pointEntities, 0);
  for (const auto& entity : pointEntities) {
    gmsh::model::mesh::setSize({entity}, elementSize);
  }

  std::vector<std::pair<int, int>> curveEntities;
  gmsh::model::getEntities(curveEntities, 1);
  for (const auto& entity : curveEntities) {
    gmsh::model::mesh::setTransfiniteCurve(
        entity.second, curveNodeCountFromElementSize(entity.second, elementSize));
  }
}
}


Editor *editor; //TODO: IMPLEMENT CALLBACK CLASS IN EDITOR

float zcam;

static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
static void window_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

ImVec2 vpos, vmin, vmax;

namespace {
ExampleAppConsole* g_app_console = nullptr;
std::string g_pending_console_output;

void flushPendingConsoleOutput()
{
    if (g_app_console == nullptr || g_pending_console_output.empty()) {
        return;
    }
    g_app_console->AddLogString(g_pending_console_output);
    g_pending_console_output.clear();
}

void appendStreamTextToConsole(const char* text, std::size_t size)
{
    if (text == nullptr || size == 0) {
        return;
    }

    if (g_app_console != nullptr) {
        g_app_console->AddLogString(std::string(text, size));
        return;
    }

    g_pending_console_output.append(text, size);
}

class ConsoleTeeBuffer : public std::streambuf {
public:
    explicit ConsoleTeeBuffer(std::streambuf* target)
        : m_target(target) {}

protected:
    int overflow(int ch) override
    {
        if (ch == traits_type::eof()) {
            return sync() == 0 ? traits_type::not_eof(ch) : traits_type::eof();
        }

        const char c = traits_type::to_char_type(ch);
        if (m_target != nullptr && traits_type::eq_int_type(m_target->sputc(c), traits_type::eof())) {
            return traits_type::eof();
        }

        m_buffer.push_back(c);
        flushCompletedLines();
        return ch;
    }

    std::streamsize xsputn(const char* s, std::streamsize count) override
    {
        const std::streamsize written = m_target != nullptr ? m_target->sputn(s, count) : count;
        if (written > 0) {
            m_buffer.append(s, static_cast<std::size_t>(written));
            flushCompletedLines();
        }
        return written;
    }

    int sync() override
    {
        if (m_target != nullptr) {
            m_target->pubsync();
        }
        flushAll();
        return 0;
    }

private:
    void flushCompletedLines()
    {
        std::size_t pos = 0;
        while ((pos = m_buffer.find('\n')) != std::string::npos) {
            appendStreamTextToConsole(m_buffer.data(), pos + 1);
            m_buffer.erase(0, pos + 1);
        }
    }

    void flushAll()
    {
        if (m_buffer.empty()) {
            return;
        }
        appendStreamTextToConsole(m_buffer.data(), m_buffer.size());
        m_buffer.clear();
    }

    std::streambuf* m_target;
    std::string m_buffer;
};

void initializeConsoleStreamMirroring()
{
    static ConsoleTeeBuffer coutBuffer(std::cout.rdbuf());
    static ConsoleTeeBuffer cerrBuffer(std::cerr.rdbuf());
    static bool initialized = false;

    if (initialized) {
        return;
    }

    std::cout.rdbuf(&coutBuffer);
    std::cerr.rdbuf(&cerrBuffer);
    initialized = true;
}

std::string activeModelStem(Model &model)
{
    fs::path model_name(model.getName());
    std::string stem = model_name.stem().string();
    if (stem.empty())
        stem = model_name.filename().string();
    if (stem.empty())
        stem = "model";
    return stem;
}

fs::path activeModelOutputPath(Model &model, const std::string &filename)
{
    return fs::path(model.getBaseDir()) / filename;
}

std::string ensureWfmodelPath(const std::string& filePathName)
{
    fs::path savePath(filePathName);
    if (savePath.extension() != ".wfmodel")
        savePath += ".wfmodel";
    return savePath.string();
}

fs::path preferredScriptsRoot()
{
    const fs::path currentScriptsRoot = (fs::current_path() / "scripts").lexically_normal();
    if (fs::exists(currentScriptsRoot) && fs::is_directory(currentScriptsRoot)) {
        return currentScriptsRoot;
    }

    const fs::path sourceScriptsRoot =
        fs::path(__FILE__).parent_path().parent_path() / "scripts";
    return sourceScriptsRoot.lexically_normal();
}

void appendToAppConsole(const std::string& text)
{
    appendStreamTextToConsole(text.data(), text.size());
}

bool appendFileToAppConsole(const std::string& path)
{
    if (g_app_console == nullptr)
        return false;
    return g_app_console->AddLogFile(path);
}

#ifdef BUILD_PYTHON
bool prependPythonPath(const std::filesystem::path& path)
{
    if (path.empty()) {
        return true;
    }

    const std::string normalized = std::filesystem::absolute(path).lexically_normal().string();
    PyObject* sysPath = PySys_GetObject("path");
    if (sysPath == nullptr || !PyList_Check(sysPath)) {
        std::cerr << "Failed to access Python sys.path" << std::endl;
        return false;
    }

    PyObject* pyPath = PyUnicode_FromString(normalized.c_str());
    if (pyPath == nullptr) {
        PyErr_Clear();
        std::cerr << "Failed to convert Python path " << normalized << std::endl;
        return false;
    }

    if (PySequence_Contains(sysPath, pyPath) == 0) {
        PyList_Insert(sysPath, 0, pyPath);
    }
    Py_DECREF(pyPath);
    return true;
}

bool executePythonFileWithCapturedOutput(const std::string& filePathName, std::string& capturedOutput)
{
    capturedOutput.clear();

    FILE* scriptFile = std::fopen(filePathName.c_str(), "r");
    if (scriptFile == nullptr) {
        std::cerr << "Failed to open Python script: " << filePathName << std::endl;
        return false;
    }

    PyObject* ioModule = PyImport_ImportModule("io");
    if (!ioModule) {
        PyErr_Print();
        std::fclose(scriptFile);
        return false;
    }

    PyObject* stringIOClass = PyObject_GetAttrString(ioModule, "StringIO");
    Py_DECREF(ioModule);
    if (!stringIOClass) {
        PyErr_Print();
        std::fclose(scriptFile);
        return false;
    }

    PyObject* stringIO = PyObject_CallObject(stringIOClass, nullptr);
    Py_DECREF(stringIOClass);
    if (!stringIO) {
        PyErr_Print();
        std::fclose(scriptFile);
        return false;
    }

    PyObject* sysModule = PyImport_ImportModule("sys");
    if (!sysModule) {
        PyErr_Print();
        Py_DECREF(stringIO);
        std::fclose(scriptFile);
        return false;
    }

    PyObject* originalStdout = PyObject_GetAttrString(sysModule, "stdout");
    PyObject* originalStderr = PyObject_GetAttrString(sysModule, "stderr");
    if (!originalStdout || !originalStderr) {
        PyErr_Print();
        Py_XDECREF(originalStdout);
        Py_XDECREF(originalStderr);
        Py_DECREF(sysModule);
        Py_DECREF(stringIO);
        std::fclose(scriptFile);
        return false;
    }

    PyObject_SetAttrString(sysModule, "stdout", stringIO);
    PyObject_SetAttrString(sysModule, "stderr", stringIO);

    prependPythonPath(std::filesystem::path(filePathName).parent_path());

    PyObject* mainModule = PyImport_AddModule("__main__");
    PyObject* mainDict = mainModule ? PyModule_GetDict(mainModule) : nullptr;
    if (mainDict != nullptr) {
        PyObject* pyFilePath = PyUnicode_FromString(filePathName.c_str());
        if (pyFilePath != nullptr) {
            PyDict_SetItemString(mainDict, "__file__", pyFilePath);
            Py_DECREF(pyFilePath);
        } else {
            PyErr_Clear();
        }
    }

    int runStatus = PyRun_SimpleFileEx(scriptFile, filePathName.c_str(), 1);
    scriptFile = nullptr;
    getApp().Update();

    PyObject* getValueMethod = PyObject_GetAttrString(stringIO, "getvalue");
    PyObject* output = getValueMethod ? PyObject_CallObject(getValueMethod, nullptr) : nullptr;
    Py_XDECREF(getValueMethod);

    PyObject_SetAttrString(sysModule, "stdout", originalStdout);
    PyObject_SetAttrString(sysModule, "stderr", originalStderr);

    if (output != nullptr) {
        const char* outputCStr = PyUnicode_AsUTF8(output);
        if (outputCStr != nullptr) {
            capturedOutput = outputCStr;
        } else {
            PyErr_Clear();
        }
        Py_DECREF(output);
    }

    Py_DECREF(originalStdout);
    Py_DECREF(originalStderr);
    Py_DECREF(sysModule);
    Py_DECREF(stringIO);

    if (runStatus != 0) {
        PyErr_Print();
        return false;
    }

    return true;
}
#endif

bool saveModelToPath(Model& model, const std::string& filePathName)
{
    const std::string normalizedPath = ensureWfmodelPath(filePathName);
    fs::path savePath(normalizedPath);
    std::string fileName = savePath.filename().string();
    const std::string suffix = ".wfmodel";
    if (fileName.size() >= suffix.size() &&
        fileName.compare(fileName.size() - suffix.size(), suffix.size(), suffix) == 0) {
        fileName.resize(fileName.size() - suffix.size());
    }

    model.setName(fileName);
    model.setFilePath(normalizedPath);

    ModelWriter writer(model);
    writer.writeToFile(normalizedPath);
    getApp().addRecentFile(normalizedPath);

    appendToAppConsole("Saved Model: " + normalizedPath + "\n");
    cout << "Saved Model: " << normalizedPath << endl;
    return true;
}

Mesh* findOwningMeshForNode(Model* model, Node* targetNode)
{
    if (model == nullptr || targetNode == nullptr) {
        return nullptr;
    }

    for (int p = 0; p < model->getPartCount(); ++p) {
        Part* part = model->getPart(p);
        if (part == nullptr || part->getMesh() == nullptr) {
            continue;
        }

        Mesh* mesh = part->getMesh();
        for (int n = 0; n < mesh->getNodeCount(); ++n) {
            if (mesh->getNode(n) == targetNode) {
                return mesh;
            }
        }
    }

    return nullptr;
}

Mesh* findCommonMeshForNodes(Model* model, const std::vector<Node*>& nodes)
{
    if (model == nullptr || nodes.empty()) {
        return nullptr;
    }

    Mesh* commonMesh = findOwningMeshForNode(model, nodes.front());
    if (commonMesh == nullptr) {
        return nullptr;
    }

    for (std::size_t i = 1; i < nodes.size(); ++i) {
        if (findOwningMeshForNode(model, nodes[i]) != commonMesh) {
            return nullptr;
        }
    }

    return commonMesh;
}

int findNextNodeSetId(Model* model)
{
    if (model == nullptr) {
        return 0;
    }

    int maxId = -1;
    for (int p = 0; p < model->getPartCount(); ++p) {
        Part* part = model->getPart(p);
        if (part == nullptr || part->getMesh() == nullptr) {
            continue;
        }

        Mesh* mesh = part->getMesh();
        for (int s = 0; s < mesh->getNodeSetCount(); ++s) {
            maxId = std::max(maxId, mesh->getNodeSet(s).getId());
        }
    }
    return maxId + 1;
}
}

template <typename T>
std::string to_string_with_precision(const T a_value, const int n = 6)
{
    std::ostringstream out;
    out.precision(n);
    out << std::fixed << a_value;
    return out.str();
}

template <typename T>
std::string to_string_scientific(const T a_value, const int n = 3)
{
    std::ostringstream out;
    out.precision(n);
    out << std::scientific << a_value;
    return out.str();
}

void Editor::drawSelectionControls()
{
  if (!ImGui::CollapsingHeader("Selection", ImGuiTreeNodeFlags_DefaultOpen)) {
    return;
  }

  int target = static_cast<int>(m_selector.getTarget());
  if (ImGui::RadioButton("Nodes", &target, static_cast<int>(SelectionTarget::Node))) {
    m_selector.setTarget(SelectionTarget::Node);
  }
  ImGui::SameLine();
  ImGui::BeginDisabled();
  ImGui::RadioButton("Parts", &target, static_cast<int>(SelectionTarget::Part));
  ImGui::SameLine();
  ImGui::RadioButton("Geometry", &target, static_cast<int>(SelectionTarget::Geometry));
  ImGui::EndDisabled();

  int mode = static_cast<int>(m_selector.getMode());
  if (ImGui::RadioButton("Pick", &mode, static_cast<int>(SelectionMode::Pick))) {
    m_selector.setMode(SelectionMode::Pick);
    box_select_mode = false;
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("Box", &mode, static_cast<int>(SelectionMode::Box))) {
    m_selector.setMode(SelectionMode::Box);
    box_select_mode = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("Clear")) {
    m_selector.clearSelection();
    m_sel_node = -1;
    m_is_node_sel = false;
  }

  if (m_selector.isPickMode()) {
    ImGui::TextDisabled("Pick: Click = single select");
    ImGui::TextDisabled("Pick: Ctrl+Click = add/remove node");
  } else {
    ImGui::TextDisabled("Box: drag on viewport to select nodes");
  }

  ImGui::Text("Selected nodes: %d", m_selector.getSelectedNodeCount());
  const std::vector<Node*>& selectedNodes = m_selector.getSelectedNodes();
  const int previewCount = std::min<int>(selectedNodes.size(), 8);
  for (int i = 0; i < previewCount; ++i) {
    if (selectedNodes[i] == nullptr) {
      continue;
    }
    ImGui::Text("Node %d", selectedNodes[i]->getId());
  }
  if (selectedNodes.size() > static_cast<std::size_t>(previewCount)) {
    ImGui::Text("...");
  }
}

bool Editor::isSelectorInteractionEnabled() const
{
  return m_show_set_dlg;
}

bool Editor::shouldDrawSelectionOverlay() const
{
  return m_show_set_dlg || getSelectedNodeSet() != nullptr;
}

void Editor::selectNodeSet(Mesh* mesh, int setIndex)
{
  if (mesh == nullptr || setIndex < 0 || setIndex >= mesh->getNodeSetCount()) {
    m_selected_node_set_mesh = nullptr;
    m_selected_node_set_index = -1;
    m_selector.clearSelection();
    m_sel_node = -1;
    m_is_node_sel = false;
    return;
  }

  m_selected_node_set_mesh = mesh;
  m_selected_node_set_index = setIndex;

  NodeSet& nodeSet = mesh->getNodeSet(setIndex);
  std::vector<Node*> nodes;
  for (int i = 0; i < nodeSet.getItemCount(); ++i) {
    Node* node = nodeSet.getItem(i);
    if (node != nullptr) {
      nodes.push_back(node);
    }
  }

  m_selector.setSelectedNodes(nodes);
  m_is_node_sel = !nodes.empty();
  m_sel_node = nodes.empty() ? -1 : nodes.front()->getId();
}

NodeSet* Editor::getSelectedNodeSet()
{
  if (m_selected_node_set_mesh == nullptr ||
      m_selected_node_set_index < 0 ||
      m_selected_node_set_index >= m_selected_node_set_mesh->getNodeSetCount()) {
    return nullptr;
  }
  return &m_selected_node_set_mesh->getNodeSet(m_selected_node_set_index);
}

const NodeSet* Editor::getSelectedNodeSet() const
{
  if (m_selected_node_set_mesh == nullptr ||
      m_selected_node_set_index < 0 ||
      m_selected_node_set_index >= m_selected_node_set_mesh->getNodeSetCount()) {
    return nullptr;
  }
  return &m_selected_node_set_mesh->getNodeSet(m_selected_node_set_index);
}

bool Editor::projectNodeToViewport(Node* node, double& x, double& y) const
{
  if (node == nullptr || viewer == nullptr || viewer->getRenderer() == nullptr) {
    return false;
  }

  auto renderer = viewer->getRenderer();
  const Vector3f& pos = node->getPos();
  renderer->SetWorldPoint(pos.x, pos.y, pos.z, 1.0);
  renderer->WorldToDisplay();

  double display[3] = {0.0, 0.0, 0.0};
  renderer->GetDisplayPoint(display);
  if (display[2] < 0.0 || display[2] > 1.0) {
    return false;
  }

  x = display[0];
  y = static_cast<double>(viewer->getViewportHeight()) - display[1];
  return true;
}

Node* Editor::pickClosestNodeAt(double x, double y, double maxDistancePixels) const
{
  if (m_model == nullptr) {
    return nullptr;
  }

  Node* closest = nullptr;
  double bestDist2 = maxDistancePixels * maxDistancePixels;

  for (int p = 0; p < m_model->getPartCount(); ++p) {
    Part* part = m_model->getPart(p);
    if (part == nullptr || !part->isMeshed() || part->getMesh() == nullptr) {
      continue;
    }

    Mesh* mesh = part->getMesh();
    for (int n = 0; n < mesh->getNodeCount(); ++n) {
      Node* node = mesh->getNode(n);
      double sx = 0.0;
      double sy = 0.0;
      if (!projectNodeToViewport(node, sx, sy)) {
        continue;
      }

      const double dx = sx - x;
      const double dy = sy - y;
      const double dist2 = dx * dx + dy * dy;
      if (dist2 <= bestDist2) {
        bestDist2 = dist2;
        closest = node;
      }
    }
  }

  return closest;
}

void Editor::selectNodesInBox(double x0, double y0, double x1, double y1)
{
  if (m_model == nullptr) {
    m_selector.clearSelection();
    return;
  }

  const double minX = std::min(x0, x1);
  const double maxX = std::max(x0, x1);
  const double minY = std::min(y0, y1);
  const double maxY = std::max(y0, y1);

  std::vector<Node*> selectedNodes;
  std::unordered_set<Node*> seen;

  for (int p = 0; p < m_model->getPartCount(); ++p) {
    Part* part = m_model->getPart(p);
    if (part == nullptr || !part->isMeshed() || part->getMesh() == nullptr) {
      continue;
    }

    Mesh* mesh = part->getMesh();
    for (int n = 0; n < mesh->getNodeCount(); ++n) {
      Node* node = mesh->getNode(n);
      double sx = 0.0;
      double sy = 0.0;
      if (!projectNodeToViewport(node, sx, sy)) {
        continue;
      }

      if (sx >= minX && sx <= maxX && sy >= minY && sy <= maxY && seen.insert(node).second) {
        selectedNodes.push_back(node);
      }
    }
  }

  m_selector.setSelectedNodes(selectedNodes);
  m_is_node_sel = !selectedNodes.empty();
  m_sel_node = selectedNodes.empty() ? -1 : selectedNodes.front()->getId();
  cout << "Selected " << selectedNodes.size() << " nodes" << endl;
}

void Editor::handleSelectionInteraction()
{
  if (!isSelectorInteractionEnabled() || m_moving_mode) {
    if (m_selector.isBoxSelecting()) {
      m_selector.finishBoxSelection();
    }
    return;
  }

  if (viewer == nullptr || !m_selector.isNodeTarget()) {
    return;
  }

  const ImVec2 viewportMin = viewer->getViewportScreenMin();
  const ImVec2 viewportMax = viewer->getViewportScreenMax();
  if (viewportMax.x <= viewportMin.x || viewportMax.y <= viewportMin.y) {
    return;
  }

  ImGuiIO& io = ImGui::GetIO();
  const bool hovered = ImGui::IsMouseHoveringRect(viewportMin, viewportMax, false);
  const ImVec2 localMouse(io.MousePos.x - viewportMin.x, io.MousePos.y - viewportMin.y);

  if (m_selector.isPickMode()) {
    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
      Node* node = pickClosestNodeAt(localMouse.x, localMouse.y);
      if (io.KeyCtrl) {
        m_selector.toggleNode(node);
      } else {
        m_selector.setSingleNode(node);
      }
      m_is_node_sel = (m_selector.getSelectedNodeCount() > 0);
      m_sel_node = m_is_node_sel ? m_selector.getSelectedNodes().front()->getId() : -1;
      if (node != nullptr) {
        if (io.KeyCtrl) {
          cout << "Toggled node " << node->getId()
               << ", selected count=" << m_selector.getSelectedNodeCount() << endl;
        } else {
          cout << "Picked node " << node->getId() << endl;
        }
      }
    }
    return;
  }

  if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
    m_selector.beginBoxSelection(localMouse);
  }

  if (m_selector.isBoxSelecting() && io.MouseDown[ImGuiMouseButton_Left]) {
    m_selector.updateBoxSelection(localMouse);
  }

  if (m_selector.isBoxSelecting() && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
    m_selector.updateBoxSelection(localMouse);
    selectNodesInBox(
      m_selector.getBoxStart().x,
      m_selector.getBoxStart().y,
      m_selector.getBoxEnd().x,
      m_selector.getBoxEnd().y
    );
    m_selector.finishBoxSelection();
  }
}

void Editor::drawSelectionOverlay() const
{
  if (m_moving_mode || !shouldDrawSelectionOverlay() || viewer == nullptr) {
    return;
  }

  const ImVec2 viewportMin = viewer->getViewportScreenMin();
  const ImVec2 viewportMax = viewer->getViewportScreenMax();
  if (viewportMax.x <= viewportMin.x || viewportMax.y <= viewportMin.y) {
    return;
  }

  ImDrawList* drawList = ImGui::GetForegroundDrawList();
  if (m_selector.isBoxSelecting()) {
    const ImVec2 start(viewportMin.x + m_selector.getBoxStart().x, viewportMin.y + m_selector.getBoxStart().y);
    const ImVec2 end(viewportMin.x + m_selector.getBoxEnd().x, viewportMin.y + m_selector.getBoxEnd().y);
    drawList->AddRectFilled(start, end, IM_COL32(70, 160, 255, 35));
    drawList->AddRect(start, end, IM_COL32(70, 160, 255, 255), 0.0f, 0, 2.0f);
  }

  for (Node* node : m_selector.getSelectedNodes()) {
    double x = 0.0;
    double y = 0.0;
    if (!projectNodeToViewport(node, x, y)) {
      continue;
    }
    drawList->AddCircleFilled(
      ImVec2(viewportMin.x + static_cast<float>(x), viewportMin.y + static_cast<float>(y)),
      4.0f,
      IM_COL32(255, 210, 0, 255)
    );
  }
}

bool Editor::openResultsFromPath(const std::string& filePathName)
{
  cout << "file path name " << filePathName << endl;

  std::string ext = fs::path(filePathName).extension().string();

  if (ext == ".json" || ext == ".wfresult") {
      const bool opened = beginResultsLoadFromJson(filePathName);
      if (opened) {
        getApp().addRecentFile(filePathName);
      }
      return opened;
  } else if (ext == ".vtk") {
    ResultFrame *frame = new ResultFrame(filePathName);
    frame->ensureRenderingResources();
    frame->printAvailableFields();

    std::string fieldName;
    fieldName = "pl_strain";
    //fieldName = "DISP";

    //IF SCALAR
    frame->setActiveScalarField(fieldName);   // Cambia "TEMP" por el nombre de tu campo escalar
    //if cell data
    //~ frame->actor->GetMapper()->SetScalarModeToUseCellFieldData();
    //~ frame->actor->GetMapper()->SelectColorArray(fieldName.c_str());
    //~ frame->actor->GetMapper()->SelectColorArray(fieldName.c_str());

    //IF VECTOR
    frame->setActiveScalarField("DISP");
    frame->setVectorComponent("DISP", 0); // 0=X, 1=Y, 2=Z

    frame->actor->GetMapper()->ScalarVisibilityOn();
    frame->actor->GetMapper()->Update();

    res_viewer->addActor(frame->actor);
    getApp().addRecentFile(filePathName);
  } else {
    return false;
  }

  m_activate_results_viewer = true;

  getApp().setActiveModel(m_model);

  #ifdef BUILD_PYTHON
  PyRun_SimpleString("GetApplication().getActiveModel()");
  #else
    getApp().getActiveModel();
  #endif

  getApp().Update(); //To create graphic GEOMETRY (ADD vtkOCCTGeom TR)
  return true;
}

bool Editor::beginResultsLoadFromJson(const std::string& jsonFile,
                                      bool replaceExistingResults,
                                      int preferredFrameIndex)
{
  PendingResultsLoad pending;
  pending.entries = CollectResultFrameEntriesFromJson(jsonFile, &pending.sourceDirectory, &pending.sourceJsonFile);
  pending.results.sourceDirectory = pending.sourceDirectory;
  pending.results.sourceJsonFile = pending.sourceJsonFile;
  pending.replaceExistingResults = replaceExistingResults;
  pending.preferredFrameIndex = preferredFrameIndex;
  pending.reloadStartIndex = 0;
  pending.keepPrefixFrameCount = 0;

  if (pending.sourceJsonFile.empty() || !fs::exists(pending.sourceJsonFile)) {
    cout << "Could not prepare results load for JSON: " << jsonFile << endl;
    return false;
  }

  if (replaceExistingResults && m_results != nullptr && preferredFrameIndex > 0) {
    const std::size_t maxKeep = std::min(
      pending.entries.size(),
      m_results->frames.size());
    pending.keepPrefixFrameCount = std::min(
      static_cast<std::size_t>(preferredFrameIndex),
      maxKeep);
    pending.reloadStartIndex = pending.keepPrefixFrameCount;
    pending.nextIndex = pending.reloadStartIndex;
  }

  m_pending_results_load = std::move(pending);
  m_pending_results_load.active = true;
  m_pending_results_load.justStarted = true;
  m_pending_results_load.currentFile.clear();
  m_pending_results_load.errorMessage.clear();
  // cout << "[results-progress] begin load from " << m_pending_results_load.sourceJsonFile.string()
  //      << " with " << m_pending_results_load.entries.size() << " frame entries" << endl;
  return true;
}

bool Editor::openModelFromPath(const std::string& filePathName)
{
  if (filePathName.empty())
    return false;

  ModelReader mr(m_model);
  mr.readFromFile(filePathName);

  int pc = m_model->getPartCount();
  cout << "Model part count: " << pc << endl;

  cout << "Adding vtkgeo meshes" << endl;
  for (int p = 0; p < pc; p++) {
    cout << "part " << p << endl;
    vtkOCCTGeom *geom = new vtkOCCTGeom;

    Part *part = m_model->getPart(p);
    Geom *geo = part ? part->getGeom() : nullptr;
    if (geo != nullptr) {
      geom->LoadFromShape(geo->getShape(), 0.01);
      cout << "Done." << endl;

      viewer->addActor(geom->actor);
      getApp().registerGeometry(geo, geom);
      if (part != nullptr) {
        getApp().registerPartVisual(part, geom);
      }

      cout << "Generating mesh from gmsh" << endl;

      // Mesh visualization is synchronized through App::updateMeshes().
      // Adding it directly here creates duplicate actors that App does not own.
    }
  }

  pc = m_model->getPartCount();
  cout << "Model part count: " << pc << endl;
  m_model = mr.getModel();

  cout << "Model Material Count: " << m_model->getMaterialCount() << endl;

  std::string model_name = fs::path(filePathName).stem().string();
  if (model_name.empty())
    model_name = fs::path(filePathName).filename().string();

  cout << "Setting model name " << model_name << endl;
  m_model->setName(model_name);
  m_model->setFilePath(filePathName);
  m_model->setNoSaveAs();
  is_model = true;
  m_creating_model = false;
  getApp().addRecentFile(filePathName);

  getApp().setActiveModel(m_model);
#ifdef BUILD_PYTHON
  PyRun_SimpleString("GetApplication().getActiveModel()");
#else
  getApp().getActiveModel();
#endif
  getApp().Update();

  return true;
}

bool Editor::openScriptFromPath(const std::string& filePathName)
{
  if (filePathName.empty())
    return false;

#ifdef BUILD_PYTHON
  std::cout << "Running Python script: " << filePathName << std::endl;

  std::string capturedOutput;
  const bool ok = executePythonFileWithCapturedOutput(filePathName, capturedOutput);

  if (!capturedOutput.empty()) {
    std::cout << capturedOutput;
    std::cout.flush();
  }

  if (!ok) {
    std::cerr << "Python script failed: " << filePathName << std::endl;
    return false;
  }

  std::cout << "Finished Python script: " << filePathName << std::endl;
  return true;
#else
  std::cerr << "Python support is not enabled in this build." << std::endl;
  (void)filePathName;
  return false;
#endif
}

bool Editor::importMeshPartFromPath(const std::string& filePathName)
{
  if (filePathName.empty())
    return false;

  fs::path mesh_path(filePathName);
  std::string ext = mesh_path.extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });

  if (ext != ".bdf") {
    cout << "Unsupported orphan mesh format: " << filePathName << endl;
    return false;
  }

  Part* part = new Part();
  part->generateMeshFromNastranFile(filePathName);
  part->setName(mesh_path.stem().string().c_str());
  part->setId(m_model->getPartCount());
  m_model->addPart(part);

  getApp().setActiveModel(m_model);
  getApp().Update();
  return true;
}

void Editor::closeCurrentModel()
{
  clearBoundaryConditionOverlay();
  clearPartOverlay();

  if (m_moving_mode) {
    finishMoveMode(true);
  }

  Model* oldModel = m_model;
  if (oldModel != nullptr) {
    getApp().clearVisualsForModel(oldModel);
  }

  selected_prt = nullptr;
  highlighted_prt = nullptr;
  selected_mod = nullptr;
  selected_step = nullptr;
  selected_bc = nullptr;
  m_show_mod_dlg_edit = false;
  m_creating_model = false;
  m_show_prt_dlg_edit = false;
  m_show_step_dlg_edit = false;
  m_show_bc_dlg_edit = false;
  m_show_interaction_props_dlg = false;
  m_showNewDomain = false;

  m_model = new Model();
  is_model = false;
  getApp().setActiveModel(m_model);
  getApp().Update();

  if (oldModel != nullptr)
    delete oldModel;
}

void Editor::closeCurrentResults()
{
  if (res_viewer != nullptr) {
    res_viewer->setActor(nullptr);
  }

  if (m_results != nullptr) {
    delete m_results;
    m_results = nullptr;
  }

  if (m_pending_results_load.active) {
    m_pending_results_load = PendingResultsLoad{};
  }

  m_pending_results_frame_index = -1;
}

bool Editor::createJobFromActiveModel(bool runJob)
{
  Model &model = getApp().getActiveModel();
  if (!model.getHasName()) {
    cout << "File has not name." << endl;
    return false;
  }

  fs::path input_path = activeModelOutputPath(model, activeModelStem(model) + ".wfinput");
  InputWriter writer(&model);
  writer.writeToFile(input_path.string());

  Job *job = new Job(input_path.string());
  m_jobs.push_back(job);

  if (runJob) {
    job->Run();
    job->UpdateOutput();
    m_jobshowdlg.m_job = job;
    m_jobshowdlg.m_show = true;
  }

  return true;
}

bool Editor::scalePartGeometry(Part* part, double factor)
{
  if (part == nullptr || !part->isGeom() || part->getGeom() == nullptr) {
    cout << "Scale is only available for geometry parts." << endl;
    return false;
  }

  if (factor <= 0.0) {
    cout << "Scale factor must be greater than zero." << endl;
    return false;
  }

  Geom* geom = part->getGeom();
  if (!geom->Scale(factor)) {
    return false;
  }

  vtkOCCTGeom* visual = getApp().getVisualForPart(part);
  if (visual != nullptr) {
    visual->SetGeometry(geom);
    visual->ReloadFromGeometry();
  } else {
    vtkOCCTGeom* newVisual = new vtkOCCTGeom();
    newVisual->SetGeometry(geom);
    newVisual->BuildVTKData();
    getApp().registerGeometry(geom, newVisual);
    getApp().registerPartVisual(part, newVisual);
    if (viewer != nullptr && newVisual->actor != nullptr) {
      viewer->addActor(newVisual->actor);
    }
  }

  if (part->isMeshed()) {
    getApp().removeGraphicMeshForPart(part);
    part->deleteMesh();
    cout << "Deleted mesh after scaling geometry because it became outdated." << endl;
  }

  getApp().Update();
  return true;
}

bool Editor::openResultsForModel()
{
  const std::string& modelFilePath = m_model->getFilePath();
  if (modelFilePath.empty()) {
    cout << "Model has no file path; cannot locate modelo.wfresult" << endl;
    return false;
  }

  fs::path resultsPath = fs::path(modelFilePath).parent_path() / "modelo.wfresult";
  if (!fs::exists(resultsPath)) {
    cout << "Results file not found: " << resultsPath.string() << endl;
    return false;
  }

  return openResultsFromPath(resultsPath.string());
}

bool Editor::openResultsForJob(Job* job)
{
  if (job == nullptr) {
    cout << "Job is null; cannot locate results" << endl;
    return false;
  }

  const std::string& jobPath = job->getPathFile();
  if (jobPath.empty()) {
    cout << "Job has no run file path; cannot locate results" << endl;
    return false;
  }

  fs::path runPath(jobPath);
  fs::path runDir = runPath.parent_path();
  std::string stem = runPath.stem().string();

  std::vector<fs::path> candidates;
  if (runPath.extension() == ".wfinput") {
    candidates.push_back(runDir / (runPath.stem().string() + ".wfresult"));
  }
  candidates.push_back(runDir / (stem + ".wfresult"));
  candidates.push_back(runDir / "modelo.wfresult");

  for (const auto& candidate : candidates) {
    if (fs::exists(candidate)) {
      cout << "[openResultsForJob] opening " << candidate.string()
           << " for job " << jobPath << endl;
      return openResultsFromPath(candidate.string());
    }
  }

  cout << "Could not find results for job: " << jobPath << endl;
  for (const auto& candidate : candidates) {
    cout << "  tried: " << candidate.string() << endl;
  }
  return false;
}

bool Editor::refreshOpenResults(int preferredFrameIndex)
{
  if (m_results == nullptr) {
    cout << "No results loaded; refresh skipped" << endl;
    return false;
  }

  if (m_results->sourceJsonFile.empty()) {
    cout << "Current results were not loaded from a JSON file; refresh skipped" << endl;
    return false;
  }

  return beginResultsLoadFromJson(m_results->sourceJsonFile.string(), true, preferredFrameIndex);
}

bool Editor::hasBlockingDialogOpen() const
{
  return m_show_mat_dlg ||
         m_show_set_dlg ||
         m_show_mat_dlg_edit ||
         m_show_mod_dlg_edit ||
         m_show_prt_dlg_edit ||
         m_show_bc_dlg_edit ||
         m_show_step_dlg_edit ||
         m_show_interaction_props_dlg ||
         m_show_msh_dlg ||
         m_jobdlg.m_show ||
         m_jobshowdlg.m_show;
}

void Editor::showScriptBrowser()
{
  refreshScriptBrowser();
  m_show_script_browser = true;
}

void Editor::refreshScriptBrowser()
{
  m_script_browser_entries.clear();

  const fs::path scriptsRoot = preferredScriptsRoot();
  if (!fs::exists(scriptsRoot) || !fs::is_directory(scriptsRoot)) {
    m_selected_script_browser_index = -1;
    return;
  }

  for (const auto& entry : fs::recursive_directory_iterator(scriptsRoot)) {
    if (!entry.is_regular_file()) {
      continue;
    }

    std::string ext = entry.path().extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
      return static_cast<char>(std::tolower(c));
    });
    if (ext == ".py") {
      m_script_browser_entries.push_back(entry.path());
    }
  }

  std::sort(m_script_browser_entries.begin(), m_script_browser_entries.end(),
            [](const fs::path& a, const fs::path& b) {
              return a.lexically_normal().string() < b.lexically_normal().string();
            });

  if (m_script_browser_entries.empty()) {
    m_selected_script_browser_index = -1;
  } else if (m_selected_script_browser_index < 0 ||
             m_selected_script_browser_index >= static_cast<int>(m_script_browser_entries.size())) {
    m_selected_script_browser_index = 0;
  }
}

void Editor::drawScriptBrowserWindow()
{
  if (!m_show_script_browser) {
    return;
  }

  ImGui::SetNextWindowSize(ImVec2(620.0f, 360.0f), ImGuiCond_FirstUseEver);
  if (!ImGui::Begin("Script Browser", &m_show_script_browser)) {
    ImGui::End();
    return;
  }

  const fs::path scriptsRoot = preferredScriptsRoot();
  ImGui::Text("Folder: %s", scriptsRoot.string().c_str());
  if (ImGui::Button("Refresh")) {
    refreshScriptBrowser();
  }
  ImGui::SameLine();

  const bool hasSelection =
      m_selected_script_browser_index >= 0 &&
      m_selected_script_browser_index < static_cast<int>(m_script_browser_entries.size());
  if (ImGui::Button("Run Selected") && hasSelection) {
    openScriptFromPath(m_script_browser_entries[m_selected_script_browser_index].string());
  }

  ImGui::Separator();
  ImGui::BeginChild("ScriptBrowserList", ImVec2(0, 0), true);

  if (m_script_browser_entries.empty()) {
    ImGui::TextDisabled("No Python scripts found in ./scripts");
  } else {
    for (int i = 0; i < static_cast<int>(m_script_browser_entries.size()); ++i) {
      const bool isSelected = (m_selected_script_browser_index == i);
      fs::path displayPath = m_script_browser_entries[i];
      std::error_code ec;
      if (displayPath.is_absolute()) {
        displayPath = fs::relative(displayPath, scriptsRoot, ec);
        if (ec) {
          displayPath = m_script_browser_entries[i];
        }
      }

      const std::string label = displayPath.string();
      if (ImGui::Selectable(label.c_str(), isSelected)) {
        m_selected_script_browser_index = i;
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", m_script_browser_entries[i].string().c_str());
      }
      if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
        m_selected_script_browser_index = i;
        openScriptFromPath(m_script_browser_entries[i].string());
      }
    }
  }

  ImGui::EndChild();
  ImGui::End();
}

void Editor::applyPartTranslation(Part* part, double dx, double dy, double dz)
{
  if (part == nullptr) {
    return;
  }

  if (std::abs(dx) <= 1.0e-12 && std::abs(dy) <= 1.0e-12 && std::abs(dz) <= 1.0e-12) {
    return;
  }

  vtkOCCTGeom* visual = getApp().getVisualForPart(part);
  if (visual != nullptr && visual->getPolydata() != nullptr) {
    vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
    transform->Translate(dx, dy, dz);

    vtkNew<vtkTransformFilter> tf;
    tf->SetInputData(visual->getPolydata());
    tf->SetTransform(transform);
    tf->Update();
    visual->getPolydata()->ShallowCopy(tf->GetOutput());
  }

  if (part->getGeom() != nullptr) {
    part->getGeom()->Move(dx, dy, dz);
  }

  GraphicMesh* graphicMesh = getApp().getGraphicMeshFromPart(part);
  if (graphicMesh != nullptr) {
    graphicMesh->Translate(dx, dy, dz);
  }

  if (gizmo != nullptr && selected_prt == part) {
    vtkSmartPointer<vtkActor> targetActor = nullptr;
    if (visual != nullptr && visual->actor != nullptr) {
      targetActor = visual->actor;
    } else if (graphicMesh != nullptr) {
      targetActor = graphicMesh->getActor();
    }
    if (targetActor != nullptr) {
      gizmo->SetTargetActor(targetActor);
      gizmo->SetOriginPosition(0.0, 0.0, 0.0);
    }
  }
}

bool Editor::getPartVisualCenter(Part* part, double center[3]) const
{
  if (part == nullptr || center == nullptr) {
    return false;
  }

  vtkOCCTGeom* visual = getApp().getVisualForPart(part);
  if (visual != nullptr && visual->actor != nullptr) {
    double bounds[6];
    visual->actor->GetBounds(bounds);
    center[0] = 0.5 * (bounds[0] + bounds[1]);
    center[1] = 0.5 * (bounds[2] + bounds[3]);
    center[2] = 0.5 * (bounds[4] + bounds[5]);
    return true;
  }

  GraphicMesh* graphicMesh = getApp().getGraphicMeshFromPart(part);
  if (graphicMesh != nullptr && graphicMesh->getActor() != nullptr) {
    double bounds[6];
    graphicMesh->getActor()->GetBounds(bounds);
    center[0] = 0.5 * (bounds[0] + bounds[1]);
    center[1] = 0.5 * (bounds[2] + bounds[3]);
    center[2] = 0.5 * (bounds[4] + bounds[5]);
    return true;
  }

  return false;
}

void Editor::updateMovePartOffsetFromCurrentState()
{
  if (!m_moving_mode || selected_prt == nullptr) {
    return;
  }

  double currentCenter[3];
  if (!getPartVisualCenter(selected_prt, currentCenter)) {
    return;
  }

  m_move_part_offset[0] = currentCenter[0] - m_move_part_initial_center[0];
  m_move_part_offset[1] = currentCenter[1] - m_move_part_initial_center[1];
  m_move_part_offset[2] = currentCenter[2] - m_move_part_initial_center[2];
}

void Editor::resetCurrentPartTransform()
{
  if (selected_prt == nullptr) {
    return;
  }

  updateMovePartOffsetFromCurrentState();
  applyPartTranslation(selected_prt, -m_move_part_offset[0], -m_move_part_offset[1], -m_move_part_offset[2]);
  m_move_part_offset[0] = 0.0;
  m_move_part_offset[1] = 0.0;
  m_move_part_offset[2] = 0.0;
}

void Editor::finishMoveMode(bool acceptTransform)
{
  if (m_moving_mode && !acceptTransform) {
    resetCurrentPartTransform();
  }

  m_moving_mode = false;
  m_show_mov_part = false;

  if (viewer != nullptr) {
    viewer->restoreDefaultInteractorStyle();
    if (gizmo) {
      gizmo->Hide();
      gizmo->RemoveFromRenderer(viewer->getRenderer());
    }
  }
}

void Editor::clearBoundaryConditionOverlay()
{
  if (viewer == nullptr || viewer->getRenderer() == nullptr) {
    m_bc_overlay_actors.clear();
    return;
  }

  vtkSmartPointer<vtkRenderer> renderer = viewer->getRenderer();
  for (std::size_t i = 0; i < m_bc_overlay_actors.size(); ++i) {
    vtkSmartPointer<vtkProp> actor = m_bc_overlay_actors[i];
    if (actor != nullptr && renderer->HasViewProp(actor)) {
      renderer->RemoveViewProp(actor);
    }
  }
  m_bc_overlay_actors.clear();
}

void Editor::clearPartOverlay()
{
  if (viewer == nullptr || viewer->getRenderer() == nullptr) {
    m_part_overlay_actors.clear();
    return;
  }

  vtkSmartPointer<vtkRenderer> renderer = viewer->getRenderer();
  for (std::size_t i = 0; i < m_part_overlay_actors.size(); ++i) {
    vtkSmartPointer<vtkProp> actor = m_part_overlay_actors[i];
    if (actor != nullptr && renderer->HasViewProp(actor)) {
      renderer->RemoveViewProp(actor);
    }
  }
  m_part_overlay_actors.clear();
}

Part* Editor::findBoundaryConditionTargetPart(const Condition* condition) const
{
  if (condition == nullptr || m_model == nullptr || condition->getApplyTo() != ApplyToPart) {
    return nullptr;
  }

  const int targetId = condition->getTargetId();
  for (int i = 0; i < m_model->getPartCount(); ++i) {
    Part* part = m_model->getPart(i);
    if (part != nullptr && part->getId() == targetId) {
      return part;
    }
  }

  if (targetId >= 0 && targetId < m_model->getPartCount()) {
    return m_model->getPart(targetId);
  }

  return nullptr;
}

NodeSet* Editor::findNodeSetById(int setId) const
{
  if (m_model == nullptr) {
    return nullptr;
  }

  for (int i = 0; i < m_model->getPartCount(); ++i) {
    Part* part = m_model->getPart(i);
    if (part == nullptr || part->getMesh() == nullptr) {
      continue;
    }

    if (NodeSet* set = part->getMesh()->findNodeSetById(setId)) {
      return set;
    }
  }

  return nullptr;
}

vtkSmartPointer<vtkPolyData> Editor::getBoundaryConditionTargetPolyData(Part* part) const
{
  if (part == nullptr) {
    return nullptr;
  }

  vtkOCCTGeom* visual = getApp().getVisualForPart(part);
  if (visual != nullptr && visual->getPolydata() != nullptr) {
    return visual->getPolydata();
  }

  GraphicMesh* graphicMesh = getApp().getGraphicMeshFromPart(part);
  if (graphicMesh != nullptr) {
    vtkSmartPointer<vtkActor> actor = graphicMesh->getActor();
    if (actor != nullptr && actor->GetMapper() != nullptr) {
      return vtkPolyData::SafeDownCast(actor->GetMapper()->GetInput());
    }
  }

  return nullptr;
}

vtkSmartPointer<vtkPolyData> Editor::getBoundaryConditionTargetPolyData(const NodeSet* nodeSet) const
{
  if (nodeSet == nullptr || nodeSet->getItemCount() == 0) {
    return nullptr;
  }

  vtkNew<vtkPoints> points;
  vtkNew<vtkCellArray> verts;

  for (int i = 0; i < nodeSet->getItemCount(); ++i) {
    const Node* node = nodeSet->getItem(i);
    if (node == nullptr) {
      continue;
    }
    const Vector3f& pos = node->getPos();
    vtkIdType pointId = points->InsertNextPoint(pos.x, pos.y, pos.z);
    verts->InsertNextCell(1);
    verts->InsertCellPoint(pointId);
  }

  if (points->GetNumberOfPoints() == 0) {
    return nullptr;
  }

  vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
  polyData->SetPoints(points);
  polyData->SetVerts(verts);
  return polyData;
}

vtkSmartPointer<vtkActor> Editor::getPartVisualActor(Part* part) const
{
  if (part == nullptr) {
    return nullptr;
  }

  vtkOCCTGeom* visual = getApp().getVisualForPart(part);
  if (visual != nullptr && visual->actor != nullptr) {
    return visual->actor;
  }

  GraphicMesh* graphicMesh = getApp().getGraphicMeshFromPart(part);
  if (graphicMesh != nullptr) {
    return graphicMesh->getActor();
  }

  return nullptr;
}

void Editor::updateBoundaryConditionOverlay()
{
  clearBoundaryConditionOverlay();

  if (viewer == nullptr || viewer->getRenderer() == nullptr || m_model == nullptr) {
    return;
  }

  vtkSmartPointer<vtkRenderer> renderer = viewer->getRenderer();

  for (int i = 0; i < m_model->getBCCount(); ++i) {
    Condition* condition = m_model->getBC(i);
    if (condition == nullptr) {
      continue;
    }

    Part* targetPart = nullptr;
    const NodeSet* targetNodeSet = nullptr;
    vtkSmartPointer<vtkPolyData> targetPolyData = nullptr;

    if (condition->getApplyTo() == ApplyToPart) {
      targetPart = findBoundaryConditionTargetPart(condition);
      targetPolyData = getBoundaryConditionTargetPolyData(targetPart);
      if (targetPart == nullptr || targetPolyData == nullptr || targetPolyData->GetNumberOfPoints() == 0) {
        continue;
      }
    } else if (condition->getApplyTo() == ApplyToNodeSet) {
      targetNodeSet = findNodeSetById(condition->getTargetId());
      targetPolyData = getBoundaryConditionTargetPolyData(targetNodeSet);
      if (targetNodeSet == nullptr || targetPolyData == nullptr || targetPolyData->GetNumberOfPoints() == 0) {
        continue;
      }
    } else {
      continue;
    }

    double3 velocity = condition->getValue();
    const double velocityMagnitude = length(velocity);
    double bounds[6];
    targetPolyData->GetBounds(bounds);

    const double dx = bounds[1] - bounds[0];
    const double dy = bounds[3] - bounds[2];
    const double dz = bounds[5] - bounds[4];
    double arrowLength = std::max(dx, std::max(dy, dz)) * 0.35;
    if (arrowLength <= 1.0e-12) {
      arrowLength = std::sqrt(dx * dx + dy * dy + dz * dz) * 0.35;
    }
    if (arrowLength <= 1.0e-12) {
      arrowLength = 1.0;
    }

    vtkNew<vtkCenterOfMass> centerOfMass;
    centerOfMass->SetInputData(targetPolyData);
    centerOfMass->SetUseScalarsAsWeights(false);
    centerOfMass->Update();
    double center[3];
    centerOfMass->GetCenter(center);

    if (velocityMagnitude > 1.0e-12) {

      double direction[3] = {velocity.x, velocity.y, velocity.z};
      vtkMath::Normalize(direction);

      vtkNew<vtkArrowSource> arrowSource;
      arrowSource->SetTipLength(0.33);
      arrowSource->SetTipRadius(0.20);
      arrowSource->SetShaftRadius(0.09);
      arrowSource->SetTipResolution(24);
      arrowSource->SetShaftResolution(24);

      vtkNew<vtkPolyDataMapper> arrowMapper;
      arrowMapper->SetInputConnection(arrowSource->GetOutputPort());

      vtkSmartPointer<vtkActor> arrowActor = vtkSmartPointer<vtkActor>::New();
      arrowActor->SetMapper(arrowMapper);

      const double thickness = std::max(arrowLength * 0.18, 1.0e-3);
      arrowActor->SetScale(arrowLength, thickness, thickness);
      arrowActor->SetPosition(center[0] - 0.5 * arrowLength * direction[0],
                              center[1] - 0.5 * arrowLength * direction[1],
                              center[2] - 0.5 * arrowLength * direction[2]);

      const double xAxis[3] = {1.0, 0.0, 0.0};
      double rotationAxis[3];
      vtkMath::Cross(xAxis, direction, rotationAxis);
      const double dotValue = std::max(-1.0, std::min(1.0, vtkMath::Dot(xAxis, direction)));
      const double axisNorm = vtkMath::Norm(rotationAxis);
      if (axisNorm > 1.0e-12) {
        vtkMath::Normalize(rotationAxis);
        arrowActor->RotateWXYZ(vtkMath::DegreesFromRadians(std::acos(dotValue)),
                               rotationAxis[0], rotationAxis[1], rotationAxis[2]);
      } else if (dotValue < 0.0) {
        arrowActor->RotateWXYZ(180.0, 0.0, 1.0, 0.0);
      }

      arrowActor->PickableOff();
      arrowActor->GetProperty()->SetColor(0.95, 0.35, 0.1);
      arrowActor->GetProperty()->SetAmbient(1.0);
      arrowActor->GetProperty()->SetDiffuse(0.0);
      renderer->AddActor(arrowActor);
      m_bc_overlay_actors.push_back(arrowActor);
    }

    if (selected_bc == condition || hovered_bc == condition) {
      vtkNew<vtkPolyDataMapper> surfaceMapper;
      surfaceMapper->SetInputData(targetPolyData);

      vtkSmartPointer<vtkActor> surfaceActor = vtkSmartPointer<vtkActor>::New();
      surfaceActor->SetMapper(surfaceMapper);
      surfaceActor->PickableOff();
      surfaceActor->GetProperty()->SetColor(1.0, 0.95, 0.1);
      surfaceActor->GetProperty()->SetOpacity(condition->getApplyTo() == ApplyToPart ? 0.22 : 1.0);
      surfaceActor->GetProperty()->LightingOff();
      if (condition->getApplyTo() == ApplyToNodeSet) {
        surfaceActor->GetProperty()->SetRepresentationToPoints();
        surfaceActor->GetProperty()->SetPointSize(12.0);
        surfaceActor->GetProperty()->RenderPointsAsSpheresOn();
      }
      renderer->AddActor(surfaceActor);
      m_bc_overlay_actors.push_back(surfaceActor);

      if (condition->getApplyTo() == ApplyToPart) {
        vtkNew<vtkPolyDataMapper> edgeMapper;
        edgeMapper->SetInputData(targetPolyData);

        vtkSmartPointer<vtkActor> edgeActor = vtkSmartPointer<vtkActor>::New();
        edgeActor->SetMapper(edgeMapper);
        edgeActor->PickableOff();
        edgeActor->GetProperty()->SetColor(1.0, 0.95, 0.1);
        edgeActor->GetProperty()->SetOpacity(1.0);
        edgeActor->GetProperty()->SetLineWidth(5.0);
        edgeActor->GetProperty()->SetRepresentationToWireframe();
        edgeActor->GetProperty()->LightingOff();
        edgeActor->GetProperty()->SetRenderLinesAsTubes(true);
        edgeActor->GetProperty()->RenderPointsAsSpheresOn();
        edgeActor->GetProperty()->SetPointSize(9.0);
        renderer->AddActor(edgeActor);
        m_bc_overlay_actors.push_back(edgeActor);
      }

      if (selected_bc == condition) {
        std::ostringstream label;
        if (condition->getApplyTo() == ApplyToNodeSet && targetNodeSet != nullptr) {
          label << targetNodeSet->getLabel() << " ";
        }
        label << "v = ("
              << to_string_scientific(velocity.x, 3) << ", "
              << to_string_scientific(velocity.y, 3) << ", "
              << to_string_scientific(velocity.z, 3) << ")";

        vtkSmartPointer<vtkBillboardTextActor3D> textActor =
            vtkSmartPointer<vtkBillboardTextActor3D>::New();
        textActor->SetInput(label.str().c_str());
        textActor->SetPosition(center[0], center[1], center[2] + arrowLength * 0.20);
        textActor->PickableOff();
        textActor->GetTextProperty()->SetFontSize(18);
        textActor->GetTextProperty()->SetColor(1.0, 0.95, 0.1);
        textActor->GetTextProperty()->SetBold(true);
        textActor->GetTextProperty()->SetBackgroundColor(0.0, 0.0, 0.0);
        textActor->GetTextProperty()->SetBackgroundOpacity(0.45);
        renderer->AddActor(textActor);
        m_bc_overlay_actors.push_back(textActor);
      }
    }
  }
}

void Editor::updatePartOverlay()
{
  clearPartOverlay();

  if (viewer == nullptr || viewer->getRenderer() == nullptr || highlighted_prt == nullptr) {
    return;
  }

  vtkSmartPointer<vtkActor> sourceActor = getPartVisualActor(highlighted_prt);
  vtkSmartPointer<vtkPolyData> targetPolyData = getBoundaryConditionTargetPolyData(highlighted_prt);
  if (sourceActor == nullptr || targetPolyData == nullptr || targetPolyData->GetNumberOfPoints() == 0) {
    return;
  }

  vtkSmartPointer<vtkRenderer> renderer = viewer->getRenderer();
  const bool hasFaces = targetPolyData->GetNumberOfPolys() > 0;
  const bool hasLines = targetPolyData->GetNumberOfLines() > 0;
  const bool hasVerts = targetPolyData->GetNumberOfVerts() > 0;

  if (hasFaces) {
    vtkNew<vtkPolyDataMapper> surfaceMapper;
    surfaceMapper->SetInputData(targetPolyData);

    vtkSmartPointer<vtkActor> surfaceActor = vtkSmartPointer<vtkActor>::New();
    surfaceActor->SetMapper(surfaceMapper);
    surfaceActor->PickableOff();
    surfaceActor->GetProperty()->SetColor(0.25, 0.85, 1.0);
    surfaceActor->GetProperty()->SetOpacity(0.18);
    surfaceActor->GetProperty()->LightingOff();
    renderer->AddActor(surfaceActor);
    m_part_overlay_actors.push_back(surfaceActor);
  }

  vtkSmartPointer<vtkActor> edgeActor = vtkSmartPointer<vtkActor>::New();
  edgeActor->SetMapper(sourceActor->GetMapper());
  edgeActor->SetUserTransform(sourceActor->GetUserTransform());
  if (sourceActor->GetTexture() != nullptr) {
    edgeActor->SetTexture(sourceActor->GetTexture());
  }
  vtkSmartPointer<vtkProperty> overlayProperty = vtkSmartPointer<vtkProperty>::New();
  overlayProperty->DeepCopy(sourceActor->GetProperty());
  edgeActor->SetProperty(overlayProperty);
  edgeActor->PickableOff();
  edgeActor->GetProperty()->SetColor(0.25, 0.85, 1.0);
  edgeActor->GetProperty()->SetOpacity(1.0);
  edgeActor->GetProperty()->LightingOff();
  edgeActor->GetProperty()->SetRenderLinesAsTubes(true);
  edgeActor->GetProperty()->RenderPointsAsSpheresOn();
  edgeActor->GetProperty()->SetLineWidth((hasLines || hasVerts) ? 8.0 : 4.0);
  edgeActor->GetProperty()->SetPointSize((hasLines || hasVerts) ? 11.0 : 8.0);
  if (hasFaces) {
    edgeActor->GetProperty()->SetRepresentationToWireframe();
  } else {
    edgeActor->GetProperty()->SetRepresentation(sourceActor->GetProperty()->GetRepresentation());
  }
  renderer->AddActor(edgeActor);
  m_part_overlay_actors.push_back(edgeActor);
}

bool Editor::consumeResultsViewerActivationRequest()
{
  bool activate = m_activate_results_viewer;
  m_activate_results_viewer = false;
  return activate;
}

bool Editor::isLoadingResults() const
{
  return m_pending_results_load.active;
}

void Editor::advanceResultsLoad()
{
  if (!m_pending_results_load.active)
    return;

  if (m_pending_results_load.justStarted) {
    // cout << "[results-progress] first UI frame reserved for progress window" << endl;
    m_pending_results_load.justStarted = false;
    return;
  }

  const auto startTime = std::chrono::steady_clock::now();
  const auto budget = std::chrono::milliseconds(12);
  const std::size_t maxFramesPerTick = 1;
  std::size_t processedThisTick = 0;

  while (m_pending_results_load.nextIndex < m_pending_results_load.entries.size()) {
    const ResultFrameEntry& entry = m_pending_results_load.entries[m_pending_results_load.nextIndex];
    m_pending_results_load.currentFile = entry.vtkPath.filename().string();

    if (!fs::exists(entry.vtkPath)) {
      ++m_pending_results_load.skippedFrames;
      cout << "Warning: VTK file not found: " << entry.vtkPath.string() << endl;
    } else {
      try {
        auto frame = std::make_unique<ResultFrame>(entry.vtkPath.string());
        m_pending_results_load.results.frames.push_back(std::move(frame));
        ++m_pending_results_load.loadedFrames;
      } catch (const std::exception& e) {
        ++m_pending_results_load.skippedFrames;
        m_pending_results_load.errorMessage = e.what();
        cout << "Error loading " << entry.vtkPath.string() << ": " << e.what() << endl;
      }
    }

    ++m_pending_results_load.nextIndex;
    // cout << "[results-progress] processed " << m_pending_results_load.nextIndex
    //      << "/" << m_pending_results_load.entries.size()
    //      << " current=" << m_pending_results_load.currentFile
    //      << " loaded=" << m_pending_results_load.loadedFrames
    //      << " skipped=" << m_pending_results_load.skippedFrames << endl;
    ++processedThisTick;

    if (processedThisTick >= maxFramesPerTick ||
        (std::chrono::steady_clock::now() - startTime) >= budget)
      break;
  }

  if (m_pending_results_load.nextIndex >= m_pending_results_load.entries.size())
    finishResultsLoad();
}

void Editor::finishResultsLoad()
{
  if (!m_pending_results_load.active)
    return;

  MultiResult mergedResults;
  mergedResults.sourceDirectory = m_pending_results_load.results.sourceDirectory;
  mergedResults.sourceJsonFile = m_pending_results_load.results.sourceJsonFile;

  if (m_pending_results_load.replaceExistingResults && m_results != nullptr) {
    const std::size_t keepCount = std::min(
      m_pending_results_load.keepPrefixFrameCount,
      m_results->frames.size());
    for (std::size_t i = 0; i < keepCount; ++i) {
      mergedResults.frames.push_back(std::move(m_results->frames[i]));
    }
    for (auto& frame : m_pending_results_load.results.frames) {
      mergedResults.frames.push_back(std::move(frame));
    }
    if (res_viewer != nullptr) {
      res_viewer->setActor(nullptr);
    }
    delete m_results;
    m_results = nullptr;
    m_results = new MultiResult(std::move(mergedResults));
  } else {
    m_results = new MultiResult(std::move(m_pending_results_load.results));
  }
  if (m_results != nullptr && !m_results->frames.empty()) {
    m_pending_results_frame_index = std::max(
      0,
      std::min(m_pending_results_load.preferredFrameIndex,
               static_cast<int>(m_results->frames.size()) - 1));
  } else {
    m_pending_results_frame_index = 0;
  }
  m_activate_results_viewer = true;
  getApp().Update();

  cout << "Loaded " << m_pending_results_load.loadedFrames
       << " result frames";
  if (m_pending_results_load.skippedFrames > 0)
    cout << " (" << m_pending_results_load.skippedFrames << " skipped)";
  cout << endl;

  m_pending_results_load = PendingResultsLoad{};
}

int Editor::consumePendingResultsFrameIndex()
{
  const int value = m_pending_results_frame_index;
  m_pending_results_frame_index = -1;
  return value;
}

void Editor::drawResultsLoadProgress()
{
  if (!m_pending_results_load.active)
    return;

  ImGui::OpenPopup("Loading Results");

  const std::size_t total = m_pending_results_load.entries.size();
  const std::size_t start = m_pending_results_load.reloadStartIndex;
  const std::size_t processed = m_pending_results_load.nextIndex;
  const std::size_t displayTotal = total > start ? total - start : 0;
  const std::size_t displayProcessed = processed > start ? processed - start : 0;
  const float progress = displayTotal > 0
    ? static_cast<float>(displayProcessed) / static_cast<float>(displayTotal)
    : 1.0f;

  static std::size_t lastLoggedProcessed = static_cast<std::size_t>(-1);
  if (processed != lastLoggedProcessed) {
    // cout << "[results-progress] draw window processed=" << processed
    //      << "/" << total
    //      << " progress=" << progress;
    // if (!m_pending_results_load.currentFile.empty())
    //   cout << " current=" << m_pending_results_load.currentFile;
    // cout << endl;
    lastLoggedProcessed = processed;
  }

  ImGui::SetNextWindowSize(ImVec2(520.0f, 0.0f), ImGuiCond_Appearing);
  ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(),
                          ImGuiCond_Appearing,
                          ImVec2(0.5f, 0.5f));

  ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize |
                           ImGuiWindowFlags_NoCollapse |
                           ImGuiWindowFlags_NoResize |
                           ImGuiWindowFlags_NoSavedSettings;
  if (ImGui::BeginPopupModal("Loading Results", nullptr, flags)) {
    ImGui::Text("Opening %zu / %zu frames", displayProcessed, displayTotal);
    ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f));

    if (!m_pending_results_load.currentFile.empty())
      ImGui::Text("Current: %s", m_pending_results_load.currentFile.c_str());
    else
      ImGui::TextUnformatted("Current: preparing frame list...");

    ImGui::Text("Loaded: %zu", m_pending_results_load.loadedFrames);
    if (m_pending_results_load.skippedFrames > 0)
      ImGui::Text("Skipped: %zu", m_pending_results_load.skippedFrames);

    if (!m_pending_results_load.errorMessage.empty())
      ImGui::TextWrapped("Last error: %s", m_pending_results_load.errorMessage.c_str());

    // cout << "[results-progress] modal visible processed=" << processed
    //      << "/" << total << endl;
  }
  ImGui::EndPopup();
}

void Editor::meshPart(Part* part){
  if (part == nullptr || part->getGeom() == nullptr){
    return;
  }

  Geom *geo = part->getGeom();
  int part_index = -1;
  for (int i = 0; i < m_model->getPartCount(); ++i){
    if (m_model->getPart(i) == part){
      part_index = i;
      break;
    }
  }

  if (part_index < 0){
    cerr << "Could not find selected part in active model" << endl;
    return;
  }

  fs::path step_path = activeModelOutputPath(*m_model,
                                             activeModelStem(*m_model) + "_part_" + std::to_string(part_index) + ".step");
  geo->setFileName(step_path.string());
  geo->ExportSTEP();

  gmsh::clear();
  gmsh::model::add("t20");
  std::vector<std::pair<int, int> > v;
  gmsh::model::occ::importShapes(step_path.string(), v);
  gmsh::model::occ::synchronize();
  int gmsh_dim = gmsh::model::getDimension();
  AnalysisType analysis_type = m_model->getAnalysisType();
  bool is_2d_analysis = (analysis_type == PlaneStress2D ||
                         analysis_type == PlaneStrain2D ||
                         analysis_type == Axisymmetric2D);
  bool is_rigid_part = (part->getType() == Rigid);
  double element_size = m_model->getElementSize();
  cout << "Geometry dimension: "<<gmsh_dim<<endl;
  cout << "Analysis type: "<<analysis_type<<endl;

  bool mesh_ready = false;
  if (is_2d_analysis && !is_rigid_part){
    fs::path bdf_name = "output_smoothed.bdf";
    fs::path bdf_export_path = activeModelOutputPath(*m_model,
                                                     activeModelStem(*m_model) + "_part_" + std::to_string(part_index) + ".bdf");
    fs::path remesh_log_path = activeModelOutputPath(*m_model,
                                                     activeModelStem(*m_model) + "_part_" + std::to_string(part_index) + "_mesh_adapt.tmp");
    std::string remesh_cmd = "mesh-adapt \"" + step_path.string() + "\" " +
                             std::to_string(element_size) + " > \"" +
                             remesh_log_path.string() + "\" 2>&1";
    cout << "Running 2D remesher: " << remesh_cmd << endl;
    appendToAppConsole("Running 2D remesher: " + remesh_cmd + "\n");

    int remesh_status = std::system(remesh_cmd.c_str());
    appendFileToAppConsole(remesh_log_path.string());
    std::error_code remesh_log_ec;
    std::filesystem::remove(remesh_log_path, remesh_log_ec);
    if (remesh_status != 0){
      cerr << "Error running mesh-adapt. Exit code: " << remesh_status << endl;
      appendToAppConsole("Error running mesh-adapt_std. Exit code: " + std::to_string(remesh_status) + "\n");
    } else if (!std::filesystem::exists(bdf_name)){
      cerr << "mesh-adapt finished but BDF file was not found: " << bdf_name.string() << endl;
      appendToAppConsole("mesh-adapt_std finished but BDF file was not found: " + bdf_name.string() + "\n");
    } else {
      std::error_code ec;
      std::filesystem::copy_file(bdf_name, bdf_export_path,
                                 std::filesystem::copy_options::overwrite_existing, ec);
      if (ec) {
        cerr << "Error copying remeshed BDF to part file: " << ec.message() << endl;
        appendToAppConsole("Error copying remeshed BDF to part file: " + ec.message() + "\n");
        return;
      }
      part->generateMeshFromNastranFile(bdf_export_path.string());
      mesh_ready = true;
    }
  } else if (is_2d_analysis && is_rigid_part) {
    applyMeshSizeToCurrentGmshModel(element_size);

    gmsh::model::mesh::generate(1);

    fs::path mesh_path = activeModelOutputPath(*m_model,
                                               activeModelStem(*m_model) + "_part_" + std::to_string(part_index) + ".msh");
    gmsh::write(mesh_path.string().c_str());

    part->generateMesh();
    mesh_ready = true;
  } else {
    std::vector<std::pair<int, int>> entities;
    gmsh::model::getEntities(entities);
    applyMeshSizeToCurrentGmshModel(element_size);
    
    for(auto &e : entities) {
        if(e.first == 2) {
            gmsh::model::mesh::setTransfiniteSurface(e.second);
            gmsh::model::mesh::setRecombine(2, e.second);
            cout << "Recombine in 2 dim"<<endl;
        }
    }
    
    if (gmsh_dim > -1) gmsh::model::mesh::generate(gmsh_dim);
    
    fs::path mesh_path = activeModelOutputPath(*m_model,
                                               activeModelStem(*m_model) + "_part_" + std::to_string(part_index) + ".msh");
    gmsh::write(mesh_path.string().c_str());
    
    part->generateMesh();
    mesh_ready = true;
  }

  if (!mesh_ready){
    return;
  }

  getApp().setActiveModel(m_model);
  getApp().Update();
  getApp().checkUpdate();
  
  fs::path kpath = activeModelOutputPath(*m_model,
                                         activeModelStem(*m_model) + "_part_" + std::to_string(part_index) + ".k");
  cout << "Exporting to LSDyna..."<<endl;
  part->getMesh()->exportToLSDYNA(kpath.string());
  cout << "Done"<<endl;
}

#define PITCH_WIDTH 20.
#define PITCH_LENGTH 20.

// Helper to wire demo markers located in code to a interactive browser
/*
typedef void (*ImGuiDemoMarkerCallback)(const char* file, int line, const char* section, void* user_data);
extern ImGuiDemoMarkerCallback  GImGuiDemoMarkerCallback;
extern void*                    GImGuiDemoMarkerCallbackUserData;
ImGuiDemoMarkerCallback         GImGuiDemoMarkerCallback = NULL;
void*                           GImGuiDemoMarkerCallbackUserData = NULL;
#define //IMGUI_DEMO_MARKER(section)  do { if (GImGuiDemoMarkerCallback != NULL) GImGuiDemoMarkerCallback(__FILE__, __LINE__, section, GImGuiDemoMarkerCallbackUserData); } while (0)
*/

static void ShowExampleAppLog(bool* p_open);

static void HelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}
// Demo helper function to select among default colors. See ShowStyleEditor() for more advanced options.
// Here we use the simplified Combo() api that packs items into a single literal string.
// Useful for quick combo boxes where the choices are known locally.
bool ImGui::ShowStyleSelector(const char* label)
{
    static int style_idx = -1;
    if (ImGui::Combo(label, &style_idx, "Dark\0Light\0Classic\0"))
    {
        switch (style_idx)
        {
        case 0: ImGui::StyleColorsDark(); break;
        case 1: ImGui::StyleColorsLight(); break;
        case 2: ImGui::StyleColorsClassic(); break;
        }
        return true;
    }
    return false;
}

//// GET MMESH INFO
void getCurrentModelInfo(){
  
  // Print the model name and dimension:
  std::string name;
  gmsh::model::getCurrent(name);
  std::cout << "Model " << name << " (" << gmsh::model::getDimension()
            << "D)\n";

  // Geometrical data is made of elementary model `entities', called `points'
  // (entities of dimension 0), `curves' (entities of dimension 1), `surfaces'
  // (entities of dimension 2) and `volumes' (entities of dimension 3). As we
  // have seen in the other C++ tutorials, elementary model entities are
  // identified by their dimension and by a `tag': a strictly positive
  // identification number. Model entities can be either CAD entities (from the
  // built-in `geo' kernel or from the OpenCASCADE `occ' kernel) or `discrete'
  // entities (defined by a mesh). `Physical groups' are collections of model
  // entities and are also identified by their dimension and by a tag.

  // Get all the elementary entities in the model, as a vector of (dimension,
  // tag) pairs:
  std::vector<std::pair<int, int> > entities;
  gmsh::model::getEntities(entities);

  for(auto e : entities) {
    // Dimension and tag of the entity:
    int dim = e.first, tag = e.second;

    // Mesh data is made of `elements' (points, lines, triangles, ...), defined
    // by an ordered list of their `nodes'. Elements and nodes are identified by
    // `tags' as well (strictly positive identification numbers), and are stored
    // ("classified") in the model entity they discretize. Tags for elements and
    // nodes are globally unique (and not only per dimension, like entities).

    // A model entity of dimension 0 (a geometrical point) will contain a mesh
    // element of type point, as well as a mesh node. A model curve will contain
    // line elements as well as its interior nodes, while its boundary nodes
    // will be stored in the bounding model points. A model surface will contain
    // triangular and/or quadrangular elements and all the nodes not classified
    // on its boundary or on its embedded entities. A model volume will contain
    // tetrahedra, hexahedra, etc. and all the nodes not classified on its
    // boundary or on its embedded entities.

    // Get the mesh nodes for the entity (dim, tag):
    std::vector<std::size_t> nodeTags;
    std::vector<double> nodeCoords, nodeParams;
    gmsh::model::mesh::getNodes(nodeTags, nodeCoords, nodeParams, dim, tag);

    // Get the mesh elements for the entity (dim, tag):
    std::vector<int> elemTypes;
    std::vector<std::vector<std::size_t> > elemTags, elemNodeTags;
    gmsh::model::mesh::getElements(elemTypes, elemTags, elemNodeTags, dim, tag);

    // Elements can also be obtained by type, by using `getElementTypes()'
    // followed by `getElementsByType()'.

    // Let's print a summary of the information available on the entity and its
    // mesh.

    // * Type of the entity:
    std::string type;
    gmsh::model::getType(dim, tag, type);
    std::string name;
    gmsh::model::getEntityName(dim, tag, name);
    if(name.size()) name += " ";
    std::cout << "Entity " << name << "(" << dim << "," << tag << ") of type "
              << type << "\n";

    // * Number of mesh nodes and elements:
    int numElem = 0;
    for(auto &tags : elemTags) numElem += tags.size();
    std::cout << " - Mesh has " << nodeTags.size() << " nodes and " << numElem
              << " elements\n";

    // * Upward and downward adjacencies:
    std::vector<int> up, down;
    gmsh::model::getAdjacencies(dim, tag, up, down);
    if(up.size()) {
      std::cout << " - Upward adjacencies: ";
      for(auto e : up) std::cout << e << " ";
      std::cout << "\n";
    }
    if(down.size()) {
      std::cout << " - Downward adjacencies: ";
      for(auto e : down) std::cout << e << " ";
      std::cout << "\n";
    }

    // * Does the entity belong to physical groups?
    std::vector<int> physicalTags;
    gmsh::model::getPhysicalGroupsForEntity(dim, tag, physicalTags);
    if(physicalTags.size()) {
      std::cout << " - Physical group: ";
      for(auto physTag : physicalTags) {
        std::string n;
        gmsh::model::getPhysicalName(dim, physTag, n);
        if(n.size()) n += " ";
        std::cout << n << "(" << dim << ", " << physTag << ") ";
      }
      std::cout << "\n";
    }

    // * Is the entity a partition entity? If so, what is its parent entity?
    std::vector<int> partitions;
    gmsh::model::getPartitions(dim, tag, partitions);
    if(partitions.size()) {
      std::cout << " - Partition tags:";
      for(auto part : partitions) std::cout << " " << part;
      int parentDim, parentTag;
      gmsh::model::getParent(dim, tag, parentDim, parentTag);
      std::cout << " - parent entity (" << parentDim << "," << parentTag
                << ")\n";
    }

    // * List all types of elements making up the mesh of the entity:
    for(auto elemType : elemTypes) {
      std::string name;
      int d, order, numv, numpv;
      std::vector<double> param;
      gmsh::model::mesh::getElementProperties(elemType, name, d, order, numv,
                                              param, numpv);
      std::cout << " - Element type: " << name << ", order " << order << "\n";
      std::cout << "   with " << numv << " nodes in param coord: (";
      for(auto p : param) std::cout << p << " ";
      std::cout << ")\n";
    }
  }
  
  
}

// Note that shortcuts are currently provided for display only
// (future version will add explicit flags to BeginMenu() to request processing shortcuts)
void ShowExampleMenuFile(Editor *editor)
{
    //IMGUI_DEMO_MARKER("Examples/Menu");
    ImGui::MenuItem("(demo menu)", NULL, false, false);
    if (ImGui::MenuItem("New")) {}
    if (ImGui::MenuItem("Open", "Ctrl+O")) {
        // ////// open Dialog Simple
  // if (ImGui::Button("Open File Dialog"))
      ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".wfmodel", ".");
    }
    if (ImGui::MenuItem("Open Script")) {
      ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgOpenScript", "Choose Python Script", ".py", ".");
    }
    if (ImGui::MenuItem("Script Browser")) {
      if (editor != nullptr) {
        editor->showScriptBrowser();
      }
    }
    if (ImGui::MenuItem("Import Geometry", "Ctrl+I")){
      ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgImport", "Choose File", ".step,.iges",".");
    }
    if (ImGui::MenuItem("Import Mesh", "Ctrl+M")){
      ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgImportMesh", "Choose File", ".bdf,.BDF", ".");
    }
    if (ImGui::MenuItem("Open Result", "Ctrl+O")){
      ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgOpenRes", "Choose File", ".wfresult,.vtk", ".");
    }
    if (ImGui::MenuItem("Export LS-Dyna", "Ctrl+S")){
      ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgExport", "Choose File", ".k", ".");
    }
    if (ImGui::BeginMenu("Open Recent"))
    {
        const std::vector<std::string>& recentFiles = getApp().getRecentFiles();
        if (recentFiles.empty()) {
            ImGui::MenuItem("No recent files", nullptr, false, false);
        } else {
            for (const std::string& recentPath : recentFiles) {
                const std::string label = fs::path(recentPath).filename().string().empty()
                    ? recentPath
                    : fs::path(recentPath).filename().string();
                if (ImGui::MenuItem(label.c_str())) {
                    if (editor != nullptr) {
                        const std::string ext = fs::path(recentPath).extension().string();
                        if (ext == ".wfmodel") {
                            editor->openModelFromPath(recentPath);
                        } else if (ext == ".wfresult" || ext == ".vtk" || ext == ".json") {
                            editor->openResultsFromPath(recentPath);
                        }
                    }
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", recentPath.c_str());
                }
            }
        }
        ImGui::EndMenu();
    }
    //If open
    if (ImGui::MenuItem("Write JSON Input", "Ctrl+J")) {
      InputWriter writer( &getApp().getActiveModel() );
      if ( (getApp().getActiveModel().getHasName()) ) {
        fs::path input_path = activeModelOutputPath(getApp().getActiveModel(),
                                                    activeModelStem(getApp().getActiveModel()) + ".wfinput");
        //writer.writeToFile("Input.json");
        writer.writeToFile(input_path.string());
      }
      else 
        cout << "File has not name."<<endl;
      }
    if (ImGui::MenuItem("Save", "Ctrl+S")) {



	      if (!(getApp().getActiveModel().getHasName()))
	        ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgSave", "Choose File", ".wfmodel", ".");      
	      else {
	        cout << "Model has a name! "<<getApp().getActiveModel().getName()<<endl;
	        std::string save_path = getApp().getActiveModel().getFilePath();
	        if (save_path.empty())
	          save_path = getApp().getActiveModel().getName()+".wfmodel";
	        saveModelToPath(getApp().getActiveModel(), save_path);
	      
	        //ImGui::OpenPopup("Overwrite?");
        //~ if (fileExists(fullName)) {
            //~ ImGui::OpenPopup("Overwrite?");
        //~ }

        //~ if (ImGui::BeginPopupModal("Overwrite?", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            //~ ImGui::Text("El archivo '%s' ya existe.\n¿Desea sobreescribirlo?", fullName.c_str());
            //~ ImGui::Separator();

            //~ static bool confirmed = false;
            //~ if (ImGui::Button("Sí, sobreescribir", ImVec2(150, 0))) {
                //~ confirmed = true;
                //~ ImGui::CloseCurrentPopup();
            //~ }
            //~ ImGui::SameLine();
            //~ if (ImGui::Button("Cancelar", ImVec2(120, 0))) {
                //~ confirmed = false;
                //~ ImGui::CloseCurrentPopup();
            //~ }

            //~ ImGui::EndPopup();

            //~ if (confirmed) {
                //~ // Guardar el archivo realmente aquí
                //~ std::cout << "Sobrescribiendo: " << fullName << std::endl;
                //~ // tu código de guardado...
                //~ confirmed = false; // reset
            //~ }
        //~ }
      }
      cout << "Adress"<<&getApp().getActiveModel()<<endl;
      //ModelWriter(getApp().getActiveModel()); //Once it has name
    }
    
    if (ImGui::MenuItem("Save As..")) {}

    ImGui::Separator();
    //IMGUI_DEMO_MARKER("Examples/Menu/Options");
    if (ImGui::BeginMenu("Options"))
    {
        static bool enabled = true;
        ImGui::MenuItem("Enabled", "", &enabled);
        ImGui::BeginChild("child", ImVec2(0, 60), true);
        for (int i = 0; i < 10; i++)
            ImGui::Text("Scrolling Text %d", i);
        ImGui::EndChild();
        static float f = 0.5f;
        static int n = 0;
        ImGui::SliderFloat("Value", &f, 0.0f, 1.0f);
        ImGui::InputFloat("Input", &f, 0.1f);
        ImGui::Combo("Combo", &n, "Yes\0No\0Maybe\0\0");
        ImGui::EndMenu();
    }

    //IMGUI_DEMO_MARKER("Examples/Menu/Colors");
    if (ImGui::BeginMenu("Colors"))
    {
        float sz = ImGui::GetTextLineHeight();
        for (int i = 0; i < ImGuiCol_COUNT; i++)
        {
            const char* name = ImGui::GetStyleColorName((ImGuiCol)i);
            ImVec2 p = ImGui::GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + sz, p.y + sz), ImGui::GetColorU32((ImGuiCol)i));
            ImGui::Dummy(ImVec2(sz, sz));
            ImGui::SameLine();
            ImGui::MenuItem(name);
        }
        ImGui::EndMenu();
    }

    // Here we demonstrate appending again to the "Options" menu (which we already created above)
    // Of course in this demo it is a little bit silly that this function calls BeginMenu("Options") twice.
    // In a real code-base using it would make senses to use this feature from very different code locations.
    if (ImGui::BeginMenu("Options")) // <-- Append!
    {
        //IMGUI_DEMO_MARKER("Examples/Menu/Append to an existing menu");
        static bool b = true;
        ImGui::Checkbox("SomeOption", &b);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Disabled", false)) // Disabled
    {
        IM_ASSERT(0);
    }
    if (ImGui::MenuItem("Checked", NULL, true)) {}
    if (ImGui::MenuItem("Quit", "Alt+F4")) {}
}

// Demonstrate creating a "main" fullscreen menu bar and populating it.
// Note the difference between BeginMainMenuBar() and BeginMenuBar():
// - BeginMenuBar() = menu-bar inside current window (which needs the ImGuiWindowFlags_MenuBar flag!)
// - BeginMainMenuBar() = helper to create menu-bar-sized window at the top of the main viewport + call BeginMenuBar() into it.
/*static */void ShowExampleAppMainMenuBar(Editor *editor)
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            ShowExampleMenuFile(editor);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
            if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "CTRL+X")) {}
            if (ImGui::MenuItem("Copy", "CTRL+C")) {}
            if (ImGui::MenuItem("Paste", "CTRL+V")) {}
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
        {
            if (ImGui::MenuItem("Console", "CTRL+Z")) {editor->changeShowConsole();}
            ImGui::Separator();

            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();

    }
}

void Editor::drawGui() { 
  hovered_bc = nullptr;
  hovered_prt = nullptr;
  bool model_tree_item_clicked = false;
  bool model_tree_part_clicked = false;

  ImGui::Begin("Hello, world!"); 
  // Menu Bar
  if (ImGui::BeginMenuBar())
  {
      if (ImGui::BeginMenu("Menu"))
      {
          //IMGUI_DEMO_MARKER("Menu/File");
          ShowExampleMenuFile(this);
          ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Examples"))
      {
          //IMGUI_DEMO_MARKER("Menu/Examples");
          ImGui::MenuItem("Main menu bar", NULL, &m_show_app_main_menu_bar);
          ImGui::MenuItem("Console", NULL, &m_show_app_console);
          // ImGui::MenuItem("Log", NULL, &show_app_log);
          // ImGui::MenuItem("Simple layout", NULL, &show_app_layout);
          // ImGui::MenuItem("Property editor", NULL, &show_app_property_editor);
          // ImGui::MenuItem("Long text display", NULL, &show_app_long_text);
          // ImGui::MenuItem("Auto-resizing window", NULL, &show_app_auto_resize);
          // ImGui::MenuItem("Constrained-resizing window", NULL, &show_app_constrained_resize);
          // ImGui::MenuItem("Simple overlay", NULL, &show_app_simple_overlay);
          // ImGui::MenuItem("Fullscreen window", NULL, &show_app_fullscreen);
          // ImGui::MenuItem("Manipulating window titles", NULL, &show_app_window_titles);
          // ImGui::MenuItem("Custom rendering", NULL, &show_app_custom_rendering);
          // ImGui::MenuItem("Documents", NULL, &show_app_documents);
          ImGui::EndMenu();
      }
  
      // ////if (ImGui::MenuItem("MenuItem")) {} // You can also use MenuItem() inside a menu bar!
      // if (ImGui::BeginMenu("Tools"))
      // {
          // //IMGUI_DEMO_MARKER("Menu/Tools");
  // #ifndef IMGUI_DISABLE_METRICS_WINDOW
          // ImGui::MenuItem("Metrics/Debugger", NULL, &show_app_metrics);
          // ImGui::MenuItem("Stack Tool", NULL, &show_app_stack_tool);
  // #endif
          // ImGui::MenuItem("Style Editor", NULL, &show_app_style_editor);
          // ImGui::MenuItem("About Dear ImGui", NULL, &show_app_about);
          // ImGui::EndMenu();
      // }
      ImGui::EndMenuBar();
  }

  /////////////////////////// TREEEEEE
    // //IMGUI_DEMO_MARKER("Widgets/Trees");
    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    if (ImGui::BeginTabBar("TABS", tab_bar_flags)){
    const ImGuiTabItemFlags results_tab_flags = m_activate_results_viewer ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
    if (ImGui::BeginTabItem("Model")) { 
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);

    std::string des = "3D";
    if (m_model->getAnalysisType() == PlaneStress2D) des = "2D PS";
    else if (m_model->getAnalysisType() == PlaneStrain2D) des = "2D PE";
    else if (m_model->getAnalysisType() == Axisymmetric2D) des = "2D AX";
    if (m_model->m_thermal_coupling) des += "-Thermal";
   
 
    bool close_model_requested = false;
    const bool has_loaded_model =
      m_model != nullptr &&
      (is_model ||
       m_model->getHasName() ||
       !m_model->getFilePath().empty() ||
       m_model->getPartCount() > 0 ||
       m_model->getMaterialCount() > 0 ||
       m_model->getStepCount() > 0 ||
       m_model->getBCCount() > 0 ||
       m_model->getICCount() > 0);

	    if (ImGui::TreeNode("Models"))
	    {
	      if (ImGui::IsItemClicked()) {
	        model_tree_item_clicked = true;
	      }
	      if (ImGui::BeginPopupContextItem())
      {
        if (!has_loaded_model && ImGui::MenuItem("New Model")) {
          selected_mod = m_model;
          m_creating_model = true;
          m_show_mod_dlg_edit = true;
        }
        ImGui::EndPopup();
      }

      if (has_loaded_model) {
	        std::string modelTreeLabel = "Model (" + des + ")";
	        bool model_tree_open = ImGui::TreeNode(modelTreeLabel.c_str());
	        if (ImGui::IsItemClicked()) {
	          model_tree_item_clicked = true;
	        }
	        if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered()){                
          m_show_mod_dlg_edit = true;
          selected_mod = m_model;
        }
        if (ImGui::BeginPopupContextItem())
        {
          if (ImGui::MenuItem("Edit"/*, "CTRL+Z"*/)) { 
            m_show_mod_dlg_edit = true;
            selected_mod = m_model;
          }
          if (ImGui::MenuItem("Load Results")) {
            openResultsForModel();
          }
          if (ImGui::MenuItem("Close Model")) {
            close_model_requested = true;
          }
          ImGui::EndPopup();          
        }   
        if (model_tree_open)
        {
        
	        bool open_ = ImGui::TreeNode("Parts");
	        if (ImGui::IsItemClicked()) {
	          model_tree_item_clicked = true;
	        }
        if (ImGui::BeginPopupContextItem())
        {
          if (ImGui::MenuItem("New Geometry from file", "CTRL+Z")) {
            ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgImport", "Choose File", ".step,.STEP,.stp,.STP,.geo", ".");
            
            string test;
            cout << "Model part count "<<m_model->getPartCount()<<endl;
          }
          if (ImGui::MenuItem("New Geometry...", "CTRL+Z")) {
            m_showNewDomain = true;
          }
          if (ImGui::MenuItem("New Mesh from file", "CTRL+Z")) {
            ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgImportMesh", "Choose File", ".bdf,.BDF", ".");
          }
          ImGui::EndPopup();          
        }
        

        static bool show_rename_popup = false;
        static char new_name[128] = "";
        static int rename_part_index = -1;
        static bool show_scale_popup = false;
        static double scale_factor = 1.0;
        static Part* scale_part = nullptr;
                
        /////////////////////// PART TREE
        if (open_){ 
	        for (int i = 0; i < m_model->getPartCount(); i++)
	        {
	          Part* treePart = m_model->getPart(i);
            bool part_branch_hovered = false;
	          // Use SetNextItemOpen() so set the default state of a node to be open. We could
	          // also use TreeNodeEx() with the ImGuiTreeNodeFlags_DefaultOpen flag to achieve the same thing!
	          if (i == 0)
	            ImGui::SetNextItemOpen(true, ImGuiCond_Once);

	          std::string des;
	          if (treePart->getType() == Elastic) des = "Deformable";
	          else                                des = "Rigid";
	          if (ImGui::TreeNode((void*)(intptr_t)i, "Part %d, %s, %s", treePart->getId(), treePart->getName(), des.c_str()))
	          {
	            if (ImGui::IsItemHovered()) {
                part_branch_hovered = true;
	              hovered_prt = treePart;
	            }
	            if (ImGui::IsItemClicked()) {
	              model_tree_item_clicked = true;
	              model_tree_part_clicked = true;
	              highlighted_prt = treePart;
	            }
	            //cout << "Model part count "<<m_model->getPartCount()<<endl;
	            if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered()){                
              m_show_mat_dlg_edit = true;
              //selected_mat = m_mats[i];
              selected_mat = m_model->getMaterial(i);
              m_matdlg.InitFromMaterial(selected_mat); //In order to create a temp to plastic
            }


            
            if (ImGui::BeginPopupContextItem()) {


              if (ImGui::MenuItem("Rename")) {
                  show_rename_popup = true;
                  rename_part_index = i;  // recordamos qué parte queremos renombrar
	                  strcpy(new_name, treePart->getName());
              }
              

          
              if (ImGui::MenuItem("Edit", "CTRL+Z")) {
                m_show_prt_dlg_edit = true;
	                selected_prt = treePart;
              }
              else if (ImGui::MenuItem("Delete", "CTRL+Z")) {
                m_model->delPart(i);
                getApp().Update(); //CRASHES
              } else if (ImGui::MenuItem("Mesh", "CTRL+Z")){
	                selected_prt = treePart;
                m_show_msh_dlg = true;
              }/// "MESH" part
              
              else if (ImGui::MenuItem("Move"/*, "CTRL+Z"*/)){
                
                              
                vtkSmartPointer<vtkActor> move_actor;
                vtkSmartPointer<vtkPolyData> move_polydata;
                GraphicMesh* graphicMesh = nullptr;
                

                //~ ///// IF MOVING BY MESH. THIS WORKS OK
                //~ cout << "------- ASSIGNING GRAPHIC MESH in "<<getApp().getGraphicMeshCount()<<endl;
                //~ int gmid = -1; //graphic mesh id
                //~ cout << "Checking app new parts "<<endl;
                //~ //cout << "Graphics meshes size "<<m_graphicmeshes.size()<<endl;
                //~ bool not_found = true;
                //~ for (int gm=0;gm<getApp().getGraphicMeshCount();gm++){
                  //~ if (getApp().getGraphicMesh(gm)->getMesh() == m_model->getPart(i)->getMesh()){//is related to the part mesh
                    //~ mesh_actor = getApp().getGraphicMesh(gm)->getActor();
                    //~ gmid = gm;
                    //~ cout << "gmid = "<<gmid<<endl;
                    //~ cout << "MESH ACTOR FOUND: "<< mesh_actor << endl;
                    //~ }
                //~ }//gm
                //~ ////////////////////////////
                
                
                ////// IF MOVING BY GEOM, DOES NOT WORK
	                Part* currentPart = treePart;
                cout << "Looking for visual for part: " << currentPart << endl;
                vtkOCCTGeom* visual = getApp().getVisualForPart(currentPart);
                if (visual && visual->actor) {
                    move_actor = visual->actor;
                    move_polydata = visual->getPolydata();
                }

                graphicMesh = getApp().getGraphicMeshFromPart(currentPart);
                if (!move_actor && graphicMesh) {
                    move_actor = graphicMesh->getActor();
                }

                if (move_actor != nullptr){
                gizmo->SetDimension(m_model->getDimension());
                gizmo->Show();
                gizmo->SetTargetActor(move_actor);
                gizmo->SetOriginPosition(0.0, 0.0, 0.0);

                gizmo->AddToRenderer(viewer->getRenderer());
                
                // The rest of your setup code remains the same
                vtkSmartPointer<GizmoInteractorStyle> style = vtkSmartPointer<GizmoInteractorStyle>::New();
                style->SetDefaultRenderer(viewer->getRenderer());
                style->SetCurrentRenderer(viewer->getRenderer()); // Método adicional importante

                style->SetTargetActor(move_actor);
	                style->SetPart(treePart);
                style->SetPolyData(move_polydata);
                style->SetGraphicMesh(graphicMesh);
                style->SetGizmoAxes(gizmo->GetDragAxes());
                style->SetPickAxes(gizmo->GetPickAxes());
                style->SetTransformGizmo(gizmo);

                //interactor->SetInteractorStyle(style);
                viewer->getInteractor()->SetInteractorStyle(style);
                
	                m_moving_mode = true;
	                m_show_mov_part = true;
	                m_move_part_offset[0] = 0.0;
	                m_move_part_offset[1] = 0.0;
	                m_move_part_offset[2] = 0.0;
	                if (m_selector.isBoxSelecting()) {
	                  m_selector.finishBoxSelection();
	                }

	                GLFWcursor* handCursor = glfwCreateStandardCursor(GLFW_HAND_CURSOR);

                // Aplicar cursor a la ventana
                ImGuiIO& io = ImGui::GetIO();
                io.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange; // permite que ImGui cambie el cursor
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                //ImGui_ImplGlfw_UpdateMouseCursor();
                
                selected_prt = currentPart;
                getPartVisualCenter(selected_prt, m_move_part_initial_center);
              
              }else{
                
                cout << "ERROR. No actor found for part move"<<endl;
              }
  
            }///// MESH MOVE
	              else if (treePart->isGeom() && ImGui::MenuItem("Scale")) {
	                show_scale_popup = true;
	                scale_factor = 1.0;
	                scale_part = treePart;
              }
               
              ImGui::EndPopup();
            }//PART CONTEXT MENU 
                   
                               
	              ImGui::SameLine();
	              if (ImGui::SmallButton("edit")) {
	                m_show_prt_dlg_edit = true;
	                selected_prt = treePart;                  
	              }
                if (ImGui::IsItemHovered()) {
                  part_branch_hovered = true;
                  hovered_prt = treePart;
                }


	            if (treePart->isMeshed()){
	              if (ImGui::TreeNode("Mesh"))
	              {
                    if (ImGui::IsItemHovered()) {
                      part_branch_hovered = true;
                      hovered_prt = treePart;
                    }
	                  if (ImGui::IsItemClicked()) {
	                    model_tree_item_clicked = true;
	                  }
	                  bool meshDeleted = false;
                  if (ImGui::BeginPopupContextItem())
                  {
	                      if (ImGui::MenuItem("Delete")) {
	                        getApp().removeGraphicMeshForPart(treePart);
	                        treePart->deleteMesh();
	                        meshDeleted = true;
                        
                      }
                      ImGui::EndPopup();
                  }

	                  if (meshDeleted || !treePart->isMeshed() || treePart->getMesh() == nullptr) {
	                      ImGui::TreePop();
	                      continue;
                  }

                  // Subramas internas
	                  if (ImGui::TreeNode("Nodes"))
	                  {
                        if (ImGui::IsItemHovered()) {
                          part_branch_hovered = true;
                          hovered_prt = treePart;
                        }
	                      if (ImGui::IsItemClicked()) {
	                        model_tree_item_clicked = true;
	                      }
	                      ImGui::Text("Count: %d", treePart->getMesh()->getNodeCount());
	                      ImGui::TreePop();
	                  }

                  //~ if (ImGui::TreeNode("Elements"))
                  //~ {
                      //~ ImGui::Text("Count: %d", m_model->getPart(i)->getMesh()->getElementCount());
                      //~ ImGui::TreePop();
                  //~ }

                  ImGui::TreePop(); // Cierra "Mesh"
              }//If MESH            

              
            }

              if (part_branch_hovered) {
                hovered_prt = treePart;
              }

              ImGui::TreePop();
	          } else if (ImGui::IsItemHovered()) {
	            hovered_prt = treePart;
	          } else if (ImGui::IsItemClicked()) {
	            model_tree_item_clicked = true;
	            model_tree_part_clicked = true;
	            highlighted_prt = treePart;
	          }
	        }//FOR PART
	        
	        // --- Popup modal centrado (se dibuja después del loop, no dentro) ---
        if (show_rename_popup) {
            ImGui::OpenPopup("Rename Part");
            show_rename_popup = false;
        }

        // Centramos el popup en la pantalla actual
        if (ImGui::BeginPopupModal("Rename Part", NULL,
                                   ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::SetNextWindowPos(
                ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f,
                       ImGui::GetIO().DisplaySize.y * 0.5f),
                ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

            ImGui::InputText("New name", new_name, IM_ARRAYSIZE(new_name));
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                if (rename_part_index >= 0 &&
                    rename_part_index < m_model->getPartCount()) {
                    m_model->getPart(rename_part_index)->setName(new_name);
                }
                ImGui::CloseCurrentPopup();
                rename_part_index = -1;
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
                rename_part_index = -1;
            }
            ImGui::EndPopup();
        }

        if (show_scale_popup) {
            ImGui::OpenPopup("Scale Geometry");
            show_scale_popup = false;
        }

        if (ImGui::BeginPopupModal("Scale Geometry", NULL,
                                   ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::InputDouble("Factor", &scale_factor, 0.0, 0.0, "%.6f");

            if (ImGui::Button("OK", ImVec2(120, 0))) {
                bool close_popup = true;
                if (scale_part != nullptr) {
                    close_popup = scalePartGeometry(scale_part, scale_factor);
                }
                if (close_popup) {
                    scale_part = nullptr;
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                scale_part = nullptr;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
      
           ImGui::TreePop();
        }//If model tree open
      } else {
        ImGui::TextUnformatted("No models loaded.");
      }
    }
        if (has_loaded_model) {
          bool open_ = false;
          //-----------------------
          open_ = ImGui::TreeNode("Sets");
          if (ImGui::BeginPopupContextItem())
          {
            if (ImGui::MenuItem("New", "CTRL+Z")) {
              m_setdlg.reset();
              m_setdlg.set_type = NODE_SET;
              m_show_set_dlg = true;
            }
            ImGui::EndPopup();
          }
          if (open_) //Expand
          {
             bool anySetShown = false;
             for (int i = 0; i < m_model->getPartCount(); ++i) {
               Part* part = m_model->getPart(i);
               if (part == nullptr || part->getMesh() == nullptr) {
                 continue;
               }

               Mesh* mesh = part->getMesh();
               for (int s = 0; s < mesh->getNodeSetCount(); ++s) {
                 const NodeSet& nodeSet = mesh->getNodeSet(s);
                 const std::string label = nodeSet.getLabel().empty()
                   ? "Node Set"
                   : nodeSet.getLabel();
                 const bool isSelected =
                   (m_selected_node_set_mesh == mesh && m_selected_node_set_index == s);
                 const std::string displayLabel = label + " [id " + std::to_string(nodeSet.getId()) + "]";
                 if (ImGui::Selectable((displayLabel + "##nodeset_" + std::to_string(i) + "_" + std::to_string(s)).c_str(),
                                       isSelected)) {
                   selectNodeSet(mesh, s);
                 }
                 if (ImGui::IsItemHovered()) {
                   ImGui::SetTooltip("%d nodes", nodeSet.getItemCount());
                 }
                 if (ImGui::BeginPopupContextItem()) {
                   if (ImGui::MenuItem("Rename")) {
                     m_selected_node_set_mesh = mesh;
                     m_selected_node_set_index = s;
                     std::snprintf(m_rename_set_name, sizeof(m_rename_set_name), "%s", label.c_str());
                     m_show_rename_set_dlg = true;
                   }
                   ImGui::EndPopup();
                 }
                 ImGui::SameLine();
                 ImGui::TextDisabled("(%d nodes)", nodeSet.getItemCount());
                 anySetShown = true;
               }
             }
             if (!anySetShown) {
               ImGui::TextDisabled("No sets created.");
             }
             ImGui::TreePop();
          }

          open_ = ImGui::TreeNode("Materials");
          if (ImGui::BeginPopupContextItem())
          {
            if (ImGui::MenuItem("New", "CTRL+Z")) {
              m_show_mat_dlg = true;
              selected_mat = nullptr;
            }
            ImGui::EndPopup();
          }
          if (open_){
          for (int i = 0; i < m_model->getMaterialCount(); i++)
          {
            if (i == 0)
              ImGui::SetNextItemOpen(true, ImGuiCond_Once);

            if (ImGui::TreeNode((void*)(intptr_t)i, "Material %d", i))
            {
              if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered()){                
                m_show_mat_dlg_edit = true;
                selected_mat = m_model->getMaterial(i);
              }
              if (ImGui::BeginPopupContextItem())
              {
                if (ImGui::MenuItem("Edit", "CTRL+Z")) {
                  m_show_mat_dlg_edit = true;
                  selected_mat = m_model->getMaterial(i);
                }
                ImGui::EndPopup();
              }                    
                ImGui::SameLine();
                if (ImGui::SmallButton("button")) {}
                ImGui::TreePop();
            }
          }
             ImGui::TreePop();
          }
          //-----------------------------------------------------
          open_ = ImGui::TreeNode("InteractionProps");
          if (ImGui::BeginPopupContextItem())
          {
            if (ImGui::MenuItem("Edit", "CTRL+Z")) {
              m_interactionpropsdlg.setModel(m_model);
              m_show_interaction_props_dlg = true;
            }
              ImGui::EndPopup();
            
          }
          if (open_)
          {
             ContactProperties &contact = m_model->contactProps();
             ImGui::Text("Static friction: %.4f", contact.fricCoeffStatic);
             ImGui::Text("Penalty factor: %.4f", contact.penaltyFactor);
             if (ImGui::Button("Edit Interaction Props")) {
               m_interactionpropsdlg.setModel(m_model);
               m_show_interaction_props_dlg = true;
             }
             ImGui::TreePop();
          }

          //-----------------------------------------------------
	          open_ = ImGui::TreeNode("Initial Conditions");
	          if (ImGui::IsItemClicked()) {
	            model_tree_item_clicked = true;
	          }
          if (ImGui::BeginPopupContextItem())
          {
            if (ImGui::MenuItem("New", "CTRL+Z")) {}
              ImGui::EndPopup();
              m_create_bc = 2;
              m_show_bc_dlg_edit = true;
          }
          if (open_)
          {
            for (int i = 0; i < m_model->getICCount(); i++)
            {
              if (i == 0)
                ImGui::SetNextItemOpen(true, ImGuiCond_Once);

	              if (ImGui::TreeNode((void*)(intptr_t)i, "Initial Condition %d", i))
	              {
	                if (ImGui::IsItemClicked()) {
	                  model_tree_item_clicked = true;
	                }
                if (ImGui::BeginPopupContextItem())
                {
                  if (ImGui::MenuItem("Edit", "CTRL+Z")) {
                  }
                  ImGui::EndPopup();
                }                    
                  ImGui::SameLine();
                  if (ImGui::SmallButton("button")) {}
                  ImGui::TreePop();
              }
            }
             ImGui::TreePop();
          }

          //-----------------------------------------------------
	          open_ = ImGui::TreeNode("Boundary Conditions");
	          if (ImGui::IsItemClicked()) {
	            model_tree_item_clicked = true;
	          }
          if (ImGui::BeginPopupContextItem())
          {
            if (ImGui::MenuItem("New", "CTRL+Z")) {}
              ImGui::EndPopup();
              m_show_bc_dlg_edit = true;
              m_create_bc = 1;
          }
          if (open_)
          {
            for (int i = 0; i < m_model->getBCCount(); i++)
            {
              if (i == 0)
                ImGui::SetNextItemOpen(true, ImGuiCond_Once);

              Condition* bc = m_model->getBC(i);
              bool bc_branch_hovered = false;
              ImGuiTreeNodeFlags bc_flags = ImGuiTreeNodeFlags_OpenOnArrow |
                                            ImGuiTreeNodeFlags_OpenOnDoubleClick;
              if (selected_bc == bc) {
                bc_flags |= ImGuiTreeNodeFlags_Selected;
              }

	              if (ImGui::TreeNodeEx((void*)(intptr_t)i, bc_flags, "Boundary Condition %d", i))
	              {
	                model_tree_item_clicked = model_tree_item_clicked || ImGui::IsItemClicked();
	                if (ImGui::IsItemClicked()) {
	                  selected_bc = bc;
	                }
                if (ImGui::IsItemHovered()) {
                  bc_branch_hovered = true;
                  hovered_bc = bc;
                }
                if (bc != nullptr) {
                  if (bc->getType() == SymmetryBC) {
                    double3 normal = bc->getNormal();
                    ImGui::Text("Normal: (%s, %s, %s)",
                                to_string_scientific(normal.x, 3).c_str(),
                                to_string_scientific(normal.y, 3).c_str(),
                                to_string_scientific(normal.z, 3).c_str());
                    if (ImGui::IsItemHovered()) {
                      bc_branch_hovered = true;
                      hovered_bc = bc;
                    }
                  } else {
                    double3 velocity = bc->getValue();
                    ImGui::Text("Velocity: (%s, %s, %s)",
                                to_string_scientific(velocity.x, 3).c_str(),
                                to_string_scientific(velocity.y, 3).c_str(),
                                to_string_scientific(velocity.z, 3).c_str());
                    if (ImGui::IsItemHovered()) {
                      bc_branch_hovered = true;
                      hovered_bc = bc;
                    }
                  }
                }
                if (ImGui::IsItemHovered()) {
                  bc_branch_hovered = true;
                  hovered_bc = bc;
                }
                if (ImGui::BeginPopupContextItem())
                {
                  if (ImGui::MenuItem("Edit", "CTRL+Z")) {
                    m_show_bc_dlg_edit = true;
                    selected_bc = bc;
                    m_create_bc = 0;
                  }
                  ImGui::EndPopup();
                }                    
                  ImGui::SameLine();
                  if (ImGui::SmallButton("button")) {}
                  if (ImGui::IsItemHovered()) {
                    bc_branch_hovered = true;
                    hovered_bc = bc;
                  }
                  if (bc_branch_hovered) {
                    hovered_bc = bc;
                  }
                  ImGui::TreePop();
	              } else if (ImGui::IsItemClicked()) {
	                model_tree_item_clicked = true;
	                selected_bc = bc;
	              } else if (ImGui::IsItemHovered()) {
	                hovered_bc = bc;
	              }
            }
             ImGui::TreePop();
          }

          //-----------------------------------------------------
	          open_ = ImGui::TreeNode("Steps");
	          if (ImGui::IsItemClicked()) {
	            model_tree_item_clicked = true;
	          }
          if (ImGui::BeginPopupContextItem())
          {
            if (ImGui::MenuItem("New", "CTRL+Z")) {
              selected_step = new Step();
              int step_id = m_model->getStepCount();
              selected_step->setId(step_id);
              m_creating_step = true;
              m_show_step_dlg_edit = true;
            }
            ImGui::EndPopup();
          }
          if (open_)
          {
            for (int i = 0; i < m_model->getStepCount(); i++)
            {
              Step *step = m_model->getStep(i);
              if (step == nullptr)
                continue;

              const char *step_type = step->isImplicit() ? "Implicit" : "Explicit";
              if (ImGui::TreeNode((void*)(intptr_t)i, "Step %d, %s, %s", step->getId(), step->getName(), step_type))
              {
                if (ImGui::BeginPopupContextItem())
                {
                  if (ImGui::MenuItem("Edit", "CTRL+Z")) {
                    selected_step = step;
                    m_creating_step = false;
                    m_show_step_dlg_edit = true;
                  }
                  ImGui::EndPopup();
                }
                ImGui::TreePop();
              }
            }
            ImGui::TreePop();
          }
        }
      ImGui::TreePop();

    } //MODEL TREE
    


    bool open_ = ImGui::TreeNode("Jobs");
    if (ImGui::BeginPopupContextItem())
    {
      if (ImGui::MenuItem("New", "CTRL+Z")) {
        m_jobdlg.m_edit_mode = false;
        m_jobdlg.m_job = nullptr;
        m_jobdlg.m_filename.clear();
        m_jobdlg.m_show=true;
      }
                       
      ImGui::EndPopup();
    }

    for (int i = 0; i < m_jobs.size(); i++)
    {
      
      if (i == 0)
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);

      if (ImGui::TreeNode((void*)(intptr_t)i, "Job %d", i))
      {
        if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered()){                
          //m_show_job_dlg_edit = true;
        }
          //selected_mat = m_mats[i];}
        if (ImGui::BeginPopupContextItem())
        {
          if (ImGui::MenuItem("Edit", "CTRL+Z")) {
            m_jobdlg.m_show = true;
            m_jobdlg.m_edit_mode = true;
            m_jobdlg.m_job = m_jobs[i];
            m_jobdlg.m_filename = m_jobs[i]->getPathFile();
          }
          if (ImGui::MenuItem("Show Progress", "")) {
            m_jobshowdlg.m_job = m_jobs[i];
            m_jobs[i]->UpdateOutput();

            m_jobshowdlg.m_show = true;
            
            //selected_mat = m_mats[i];
          }
          if (ImGui::MenuItem("Run", "")) {
            m_jobs[i]->Run();
            m_jobshowdlg.m_job = m_jobs[i];
            m_jobs[i]->UpdateOutput();
            m_jobshowdlg.m_show = true;
          }
          if (ImGui::MenuItem("Load Results", "")) {
            openResultsForJob(m_jobs[i]);
          }
          ImGui::EndPopup();
        }                    
        ImGui::SameLine();
        if (ImGui::SmallButton("button")) {}
        ImGui::TreePop();
      }//Is hovered
      
      
    }//Jobs

    m_jobdlg.ShowIfEnabled();

    m_jobshowdlg.ShowIfEnabled();

    if (open_)
    {
       // your tree code stuff
       ImGui::TreePop();
    }
        
    if (close_model_requested) {
      closeCurrentModel();
    }

    ImGui::EndTabItem(); 
    
  } //END MODEL TAB
    
    
    if (ImGui::BeginTabItem("Results", nullptr, results_tab_flags)) { 
      m_activate_results_viewer = false;
      bool close_results_requested = false;
      bool open_ = ImGui::TreeNode("History");      
      if (ImGui::BeginPopupContextItem())
      {
        // if (ImGui::MenuItem("New Geometry", "CTRL+Z")) {}
        // if (ImGui::MenuItem("New Mesh", "CTRL+Z")) {}
          ImGui::EndPopup();          
      }     
        if (open_)
        {
           // your tree code stuff
           ImGui::TreePop();
        }      
      open_ = ImGui::TreeNode("Loaded Results");
      if (ImGui::BeginPopupContextItem())
      {
          if ((m_results != nullptr || m_pending_results_load.active) &&
              ImGui::MenuItem("Close Results")) {
            close_results_requested = true;
          }
          ImGui::EndPopup();
      }
      if (open_)
      {
         if (m_pending_results_load.active) {
           const fs::path pendingPath = m_pending_results_load.sourceJsonFile;
           ImGui::Text("Loading: %s",
                       pendingPath.empty() ? "(unknown)" : pendingPath.string().c_str());
           ImGui::Text("Progress: %zu / %zu",
                       m_pending_results_load.nextIndex,
                       m_pending_results_load.entries.size());
         } else if (m_results != nullptr) {
           const fs::path resultsPath = m_results->sourceJsonFile;
           if (!resultsPath.empty())
             ImGui::TextWrapped("Path: %s", resultsPath.string().c_str());
           else
             ImGui::TextUnformatted("Path: (not available)");

           if (!m_results->sourceDirectory.empty())
             ImGui::TextWrapped("Directory: %s", m_results->sourceDirectory.string().c_str());

           ImGui::Text("Frames: %zu", m_results->frames.size());
         } else {
           ImGui::TextUnformatted("No results loaded.");
         }
         ImGui::TreePop();
      }

      if (close_results_requested) {
        closeCurrentResults();
      }

      open_ = ImGui::TreeNode("Element Sets");      
      if (ImGui::BeginPopupContextItem())
      {
        // if (ImGui::MenuItem("New Geometry", "CTRL+Z")) {}
        // if (ImGui::MenuItem("New Mesh", "CTRL+Z")) {}
          ImGui::EndPopup();          
      }  
      if (open_)
      {
         // your tree code stuff
         ImGui::TreePop();
      }      
      ImGui::EndTabItem(); 
    }
    
      ImGui::EndTabBar();
    }
    ////////////////////// END TAB BAR ///////////////////////////////////

    ImGui::Separator();
    drawSelectionControls();

    // En tu función de renderizado
    bool shouldShow = m_showNewDomain;
    ImGuiTreeNodeFlags flags = shouldShow ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_None;    
    
    
    if (ImGui::CollapsingHeader("New Domain", flags)){
      static int item_current = 0;
      {
          // Using the _simplified_ one-liner Combo() api here
          // See "Combo" section for examples of how to use the more flexible BeginCombo()/EndCombo() api.
          //IMGUI_DEMO_MARKER("Widgets/Basic/Combo");
          //const char* items[] = { "Box", "Cylinder", "Plane"};
          //ImGui::Combo("combo", &item_current, items, IM_ARRAYSIZE(items));
          std::vector<const char*> items;

          switch (m_model->getAnalysisType()) {
              case Solid3D:
                  items = { "Box", "Cylinder", "Plane" };
                  break;
              case Axisymmetric2D:
                  items = {"Profile"};
                  break;
              case PlaneStress2D:
                  break;
              case PlaneStrain2D:
                  items = { "Rectangle", "Circle" };
                  break;
              default:
                  items = { "Unknown" };
                  break;
          }


          ImGui::Combo("Geometry", &item_current, 
             items.data(), static_cast<int>(items.size()));
          ImGui::SameLine(); HelpMarker(
              "Using the simplified one-liner Combo API here.\nRefer to the \"Combo\" section below for an explanation of how to use the more flexible and general BeginCombo/EndCombo API.");
      }
        
      if (ImGui::Button("Box")){
      }
      
      
        ImGuiIO& io = ImGui::GetIO();
        ImGui::Text("WantTextInput: %d", io.WantTextInput);
        // static char buf[32] = "0.";
        // ImGui::InputText("1", buf, IM_ARRAYSIZE(buf));
        ImGui::Text("Origin");
        static double origin[] = {0.0,0.0,0.0};
        ImGui::InputDouble("ox ", &origin[0], 0.01f, 1.0f, "%.4f");
        static double d1 = 0.0;
        ImGui::InputDouble("oy ", &origin[1], 0.01f, 1.0f, "%.4f");
        static double d2   = 0.0;
        ImGui::InputDouble("oz ", &origin[2], 0.01f, 1.0f, "%.4f");
        ImGui::Text("Size");
        //Vec3_t size;
        static double size[] = {0.1,0.1,0.1};
        static std::string label[] = {"x ", "y ", "z "};
        bool show_size[] = {true,true,true};
        
        switch (m_model->getAnalysisType()) {
            case Solid3D:
                if (item_current == 0){ //RECTANGLE
                    
                }else if (item_current == 1){ //CYLINDER
                  label[0] = "radius ";
                  show_size[1] = false;
                  label[2] = "height ";
                }//item cylinder
                //else if (item_current == 2)
                  //geom->LoadCylinder(0.1,0.1); //BOX, CYlinder, Plane
                break;
            case Axisymmetric2D:
                label[0] = "radial ";
                label[1] = "axial ";
                label[2] = "out-of-plane ";
                break;
            case PlaneStress2D:
                break;
            case PlaneStrain2D:

                break;
            default:

                break;
        }
              
        ImGui::InputDouble(label[0].c_str(), &size[0], 0.01f, 1.0f, "%.4f");
        if (show_size[1])
        ImGui::InputDouble(label[1].c_str(), &size[1], 0.01f, 1.0f, "%.4f");
        
        
        ImGui::InputDouble(label[2].c_str(), &size[2], 0.01f, 1.0f, "%.4f");

        static float vec4a[4] = { 0.10f, 0.20f, 0.30f, 0.44f };
        //ImGui::InputFloat3("input float3", vec4a);
        
        static double radius = 0.01;
        //ImGui::InputDouble("Particle Radius",&radius); 
        if (ImGui::Button("Create SPH")){
           cout << "radius "<<radius<<endl;
           cout << "size 0"<<size[0]<<endl;
          if (item_current == 2)//Plane
              cout << "PLANE!"<<endl;
            double L = 0.5;
            double H = 0.5;
            m_dx = 0.05;
            double rho = 1.;
            double h = 1.2*radius;
            cout << "Created Box Length with XYZ Length: "<<size[0]<< ", "<<size[1]<< ", "<<size[2]<< endl;
            if (item_current == 2)//Plane
              size[2] = 0.0;
            //m_model->AddBoxLength(0 ,Vec3_t ( d0 , d1,d2 ), size[0] , size[1],  size[2], radius ,rho, h, 1 , 0 , false, false );     
            double m_radius = 1.0;
            Mesh *m_sph_msh = new SPHMesh();
            m_sph_msh->addBoxLength(Vector3f(0,0,0),Vector3f(size[0],size[1],size[2]),radius);
            cout << "Box created, adding part to model "<<endl;
            m_model->addPart(new Part(m_sph_msh));
            cout << "Part Added "<<endl;
            getApp().setActiveModel(m_model);
            getApp().Update(); //CRASHES
            
            //graphic_mesh = new GraphicSPHMesh(); ///THIS READS FROM GLOBAL GMSH MODEL
            //graphic_mesh->createVTKPolyData(*m_sph_msh);

            //viewer->addActor(graphic_mesh->getActor());
            
            //calcDomainCenter();
            //cout << "Domain Center: "<<m_domain_center.x<<", "<<m_domain_center.y<<", "<<m_domain_center.z<<endl;
            //is_sph_mesh = true;
        } //CREATE SPH
            
            if (ImGui::Button("Create FEM")){
              
              
 
              Mesh *m_fem_msh = new Mesh();
              m_fem_msh->addBoxLength(Vector3f(0,0,0),Vector3f(size[0],size[1],size[2]),radius);
              cout << size[2]<<endl;
              cout << "Adding part" <<endl;
              Part *p = new Part(m_fem_msh);
              m_model->addPart(p);
              cout << "set upate"<<endl;
              getApp().setActiveModel(m_model);
              getApp().Update(); //Create Graphic Mesh
              
              fs::path kpath = activeModelOutputPath(*m_model,
                                                     activeModelStem(*m_model) + "_part_" + std::to_string(m_model->getPartCount() - 1) + ".k");
              p->getMesh()->exportToLSDYNA(kpath.string());
                                
              
              cout << "Done"<<endl;
              //CHANGE TO MESH CONSTRUCTOR
              //graphic_mesh = new GraphicMesh(); ///THIS READS FROM GLOBAL GMSH MODEL
              //graphic_mesh->createVTKPolyData(*m_fem_msh);

              //viewer->addActor(graphic_mesh->getActor());
                            
              //is_fem_mesh = true;
              
            }
            else if (ImGui::Button("Create GEO")){
              cout << "Creatng geom"<<endl;
              vtkOCCTGeom *geom = new vtkOCCTGeom;
              cout <<"Created OCCvtK: "<<geom<<endl;
              int pc = m_model->getPartCount();
              
              fs::path step_path = activeModelOutputPath(*m_model,
                                                         activeModelStem(*m_model) + "_part_" + std::to_string(pc) + ".step");
              //Geom *geo = new Geom(name);
              Geom *geo = new Geom;
              geo->setFileName(step_path.string());
              //cout << "Creating rectangle"<<endl;
              bool created = false;
              cout << "Size: "<<size[0]<<","<<size[1]<<"," << size[2]<<endl; 
              // if (m_model->getAnalysisType() == Axisymmetric2D){
                  // geo->LoadRectangle(size[0],size[1],origin[0],origin[1]);                
              // }
              // if (m_model->getAnalysisType() == Solid3D){
                  // if (item_current == 0)
                    // geo->LoadRectangle(size[0],size[1],origin[0],origin[1]);                
              // }
              switch (m_model->getAnalysisType()) {
                  case Solid3D:
                      if (item_current == 0){ //RECTANGLE
                          
                      }else if (item_current == 1){ //CYLINDER
                        if (size[2] > 0.0){
                          cout << "Creating Cylinder "<<endl;
                          geo->LoadCylinder(size[0],size[2]); //BOX, CYlinder, Plane
                          created = true;
                        } else {
                          cout << "NULL Z DIMENSION VALUE"<<endl;
                          }
                      }//item cylinder
                      //else if (item_current == 2)
                        //geom->LoadCylinder(0.1,0.1); //BOX, CYlinder, Plane
                      break;
                  case Axisymmetric2D:

                      break;
                  case PlaneStress2D:
                      break;
                  case PlaneStrain2D:

                      break;
                  default:

                      break;
              }
                  
              if (size[2] == 0.0){ 
                cout << "Dimension is 2 "<<endl;
                const bool has_x = std::abs(size[0]) > 1.0e-12;
                const bool has_y = std::abs(size[1]) > 1.0e-12;

                if (has_x && has_y) {
                  geo->LoadRectangle(size[0],size[1],origin[0],origin[1],origin[2]);
                  cout << "Loading rectangle "<<endl;
                  created = true;
                } else if (has_x || has_y) {
                  geo->LoadLine(size[0],size[1],origin[0],origin[1]);
                  cout << "Loading line "<<endl;
                  created = true;
                } else {
                  cout << "Not geometry created: zero-size 2D entity" << endl;
                }
              }
              
              if (created){
              cout << "Done. Creating vtkmesh"<<endl;
              geom->LoadFromShape(geo->getShape(), 0.01);
              
              cout << "Done."<<endl;
              //geom->LoadCylinder(0.1,0.1);
              
              //widget->SetInteractor(rendersWindowInteractor);
              viewer->addActor(geom->actor);
              //geom->actor->GetProperty()->SetLineWidth(3.0);
              //geom->actor->GetProperty()->SetColor(1.0, 0.0, 0.0); // rojo para que resalte
                                          
                    
              
              geo->ExportSTEP();
              
              m_model->addPart(geo);

              //m_model->getPart(m_model->getPartCount())->setId(max_pid);
              create_new_part = true;
              
              Part* newPart = m_model->getPart(m_model->getPartCount() - 1);
        
        
        
              getApp().setActiveModel(m_model);              


              ///APPP
              getApp().registerGeometry(geo, geom);
              //getApp().registerPartVisualGeometry(pc,geom);
                      // REGISTRAR usando el puntero a la parte
              getApp().registerPartVisual(newPart, geom);


              #ifdef BUILD_PYTHON
              PyRun_SimpleString("GetApplication().getActiveModel()");
              #else
                getApp().getActiveModel();
              #endif

              getApp().Update(); //To create graphic GEOMETRY (ADD vtkOCCTGeom TR)

              switch (m_model->getAnalysisType()) {
                  case Solid3D:
                    cout<< "SOLID MODEL"<<endl;
                      break;
                  case Axisymmetric2D:

                      break;
                  case PlaneStress2D:
                      break;
                  case PlaneStrain2D:

                      break;
                  default:

                      break;
              }
                  

          }//Created = true
          else {
            cout <<"Not geometry created"<<endl;
            }
            }////CREATE GEO

    }


    
    if (ImGui::CollapsingHeader("BCs")){
      
      if (ImGui::Button("Create")){
        
      }
      
    }
    
    
  ////// OPEN MODEL 
  // ////// open Dialog Simple
  // if (ImGui::Button("Open File Dialog"))
    // ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".dae,.obj,.str", ".");
  if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey")){
    cout << "Open"<<endl;

      std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();

    if (ImGuiFileDialog::Instance()->IsOk())
    {
      openModelFromPath(filePathName);
    }
    ImGuiFileDialog::Instance()->Close();
  }// Open File

  ////////////////////////////////
  // IMPORT STEP GEOMETRY
  /////////////////////////////////
  if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgImport")) 
  {
    // action if OK
    if (ImGuiFileDialog::Instance()->IsOk())
    {
      std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
      std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
      
      cout << "file path name "<<filePathName<<endl;
      //m_model = new Model(filePathName);
      cout << "Adding part "<<endl;
      Geom *geo = new Geom(filePathName);
      m_model->addGeom(geo);
      m_model->addPart(m_model->getLastGeom());
      if (m_model->isAnyMesh()){
      //m_renderer.addMesh(m_model->getPartMesh(0));
      //is_fem_mesh = true;
      }
      // action
      // action
      create_new_part = true;
      
      //test 
      bool errorIfMissing;
      gmsh::model::add("t20");


      vtkOCCTGeom *geom = new vtkOCCTGeom;

      vtkOCCTReader::Format fmt;

      std::string ext = filePathName.substr(filePathName.find_last_of('.') + 1);
      for(auto & c : ext) c = toupper(c);

      if(ext == "STEP" || ext == "STP")
          fmt = vtkOCCTReader::Format::STEP;
      else if(ext == "IGES" || ext == "IGS")
          fmt = vtkOCCTReader::Format::IGES;
      else {
          std::cerr << "Unsupported geometry format: " << ext << std::endl;
          fmt = vtkOCCTReader::Format::STEP; // fallback
      }

      geom->TestReader(filePathName, fmt);
      //geom->TestReader(filePathName, vtkOCCTReader::Format::STEP);
      
      //m_model->addPart(geo);
      
      //widget->SetInteractor(renderWindowInteractor);
      viewer->addActor(geom->actor);
      //RELATE TO THE PART VIA APP!
      

        // Load a STEP file (using `importShapes' instead of `merge' allows to
      // directly retrieve the tags of the highest dimensional imported entities):
      std::vector<std::pair<int, int> > v;
     //try {
        cout << "Loading file "<<filePathName<<endl;
        gmsh::model::occ::importShapes(filePathName, v);
        gmsh::model::occ::synchronize();  // Critical for dimension detection
        int model_dim = gmsh::model::getDimension();
        
        cout << "Dimension: "<<model_dim<<endl;
      //} catch(...) {
      //  gmsh::logger::write("Could not load STEP file: bye!");
      //  gmsh::finalize();
        //return 0;
      //}
      if (model_dim > -1) gmsh::model::mesh::generate(model_dim);       

      gmsh::merge(filePathName);

      
      getApp().setActiveModel(m_model);
      getApp().registerGeometry(geo, geom);
      getApp().registerPartVisual(m_model->getLastPart(), geom);
      
      #ifdef BUILD_PYTHON
      PyRun_SimpleString("GetApplication().getActiveModel()");
      #else
        getApp().getActiveModel();
      #endif

      getApp().Update(); //To create graphic GEOMETRY (ADD vtkOCCTGeom TR)
    
    }
    
    // close
    ImGuiFileDialog::Instance()->Close();
  }  

  if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgImportMesh"))
  {
    if (ImGuiFileDialog::Instance()->IsOk())
    {
      std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
      importMeshPartFromPath(filePathName);
    }
    ImGuiFileDialog::Instance()->Close();
  }

  if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgOpenScript"))
  {
    if (ImGuiFileDialog::Instance()->IsOk())
    {
      std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
      openScriptFromPath(filePathName);
    }
    ImGuiFileDialog::Instance()->Close();
  }
  
  // display
  if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgOpenRes")) 
  {
    // action if OK
    if (ImGuiFileDialog::Instance()->IsOk())
    {
      std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
      openResultsFromPath(filePathName);
    }
    
    // close
    ImGuiFileDialog::Instance()->Close();
  }  

  if (selected_bc != nullptr &&
      hovered_bc == nullptr &&
      ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup) &&
      ImGui::IsAnyItemHovered()) {
    selected_bc = nullptr;
  }

  if (highlighted_prt != nullptr &&
      ImGui::IsMouseClicked(0) &&
      ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup) &&
      ImGui::IsAnyItemHovered() &&
      !model_tree_part_clicked &&
      hovered_prt == nullptr) {
    highlighted_prt = nullptr;
  }
  
  ////// FILE SAVE
  if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgSave")) 
  {
    // action if OK
    if (ImGuiFileDialog::Instance()->IsOk())
    {
      std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
      saveModelToPath(getApp().getActiveModel(), filePathName);
    }
    // close
    ImGuiFileDialog::Instance()->Close();
  }
  
  
  ///// EXPORT
  if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgExport")) 
  {

    // action if OK
    if (ImGuiFileDialog::Instance()->IsOk())
    {
      std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
      std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
      
      cout << "Exporting file "<<filePathName<<endl;
      if (is_sph_mesh){    
 
        //LSDynaWriter writer(m_model, filePathName);
      }
      //m_model = new Model(filePathName);
      //m_renderer.addMesh(m_model->getPartMesh(0));
      //is_fem_mesh = true;
      // action
    }
    
    // close
    ImGuiFileDialog::Instance()->Close();
  }
  
  ShowExampleAppMainMenuBar(this);

  ImGuiIO& saveShortcutIo = ImGui::GetIO();
  const bool saveShortcutPressed =
      saveShortcutIo.KeyCtrl &&
      !saveShortcutIo.KeyAlt &&
      !saveShortcutIo.KeySuper &&
      !saveShortcutIo.WantTextInput &&
      ImGui::IsKeyPressed(ImGuiKey_S, false);

  if (saveShortcutPressed) {
    Model& activeModel = getApp().getActiveModel();
    if (!activeModel.getHasName()) {
      ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgSave", "Choose File", ".wfmodel", ".");
    } else {
      std::string save_path = activeModel.getFilePath();
      if (save_path.empty())
        save_path = activeModel.getName() + ".wfmodel";
      saveModelToPath(activeModel, save_path);
    }
  }
  
  bool show_app_log = true;
  //ShowExampleAppLog(&show_app_log);
  
  //ExampleAppLog logtest;
  //ShowExampleAppLog(&show_app_log, &logtest);
  //cout << "log tst "<<logtest.test<<endl;
  create_new_mat  = false;
  create_new_set  = false;
  create_new_part = false;

  if (m_moving_mode) {
    ImGuiIO& moveIo = ImGui::GetIO();
    if (!moveIo.WantTextInput) {
      if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
        finishMoveMode(false);
      } else if (ImGui::IsKeyPressed(ImGuiKey_Enter, false) ||
                 ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false)) {
        finishMoveMode(true);
      }
    }

    if (!m_moving_mode) {
      return;
    }

    updateMovePartOffsetFromCurrentState();
    MoveCommand move = m_movprtdlg.Draw(m_move_part_step, m_move_part_offset, &m_show_mov_part);
    
    bool auto_;
    double axis[3];
    
    if (move.active) {
      cout << "Position "<<m_move_part_offset[0]<<", "<<m_move_part_offset[1]
           <<", "<<m_move_part_offset[2]<<", "<<endl;

      if (move.type == MoveCommandType::Reset) {
        resetCurrentPartTransform();
      } else if (move.type == MoveCommandType::Translate && selected_prt != nullptr) {
        double move_vec[3] = {0.0, 0.0, 0.0};
        move_vec[move.axis] = move.delta;
        applyPartTranslation(selected_prt, move_vec[0], move_vec[1], move_vec[2]);
        updateMovePartOffsetFromCurrentState();
      }
    }

    if (!m_show_mov_part) {
      finishMoveMode(true);
    }
  }// if move active

  Material_ mat;
  Job job;
  if (m_show_mat_dlg) {mat = ShowCreateMaterialDialog(&m_show_mat_dlg, &m_matdlg, &create_new_mat,&m_mat_db);}
  
  if (m_show_bc_dlg_edit){
    //std::cout << "Opening BC dialog. selected_bc=" << selected_bc << std::endl;
    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    if (m_create_bc == 0)
      m_bcdlg.Draw("Edit Condition",&m_show_bc_dlg_edit,m_model, &selected_bc, DialogMode::Auto);//Select from existing
    else if (m_create_bc == 1)
      m_bcdlg.Draw("New Boundary Conditions",&m_show_bc_dlg_edit,m_model, &selected_bc, DialogMode::NewBoundary);
    if (m_create_bc == 2) 
      m_bcdlg.Draw("New Initial Conditions",&m_show_bc_dlg_edit,m_model, &selected_bc, DialogMode::NewInitial);    
  }
  
  if (m_show_msh_dlg) {  
    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    m_mshdlg.Draw("Part", &m_show_msh_dlg, m_model, selected_prt);
  }
  if (m_show_interaction_props_dlg) {
    m_interactionpropsdlg.setModel(m_model);
    m_interactionpropsdlg.m_show = true;
    m_interactionpropsdlg.Draw();
    m_show_interaction_props_dlg = m_interactionpropsdlg.m_show;
  }
  if (m_mshdlg.hasMeshRequest()) {
    Part* part_to_mesh = m_mshdlg.consumeMeshRequest();
    meshPart(part_to_mesh);
  }

  //if (m_show_job_dlg) {job = ShowCreateJobDialog(&m_show_job_dlg, &m_jobdlg, &create_new_job);}
  if (m_show_mat_dlg_edit) {ShowEditMaterialDialog(&m_show_mat_dlg_edit, &m_matdlg, selected_mat);}
  if (m_show_prt_dlg_edit) {ShowEditPartDialog(&m_show_prt_dlg_edit, &m_prtdlg, selected_prt);}
  if (m_show_mod_dlg_edit) {
    ShowEditModelDialog(&m_show_mod_dlg_edit, &m_moddlg, selected_mod);
    if (!m_show_mod_dlg_edit) {
      if (m_creating_model && m_moddlg.m_saved && selected_mod == m_model) {
        is_model = true;
      }

      if (m_creating_model) {
        m_creating_model = false;
      }

      selected_mod = nullptr;
    }
  }
  if (m_show_step_dlg_edit) {
    ShowEditStepDialog(&m_show_step_dlg_edit, &m_stepdlg, selected_step);
    if (!m_show_step_dlg_edit) {
      if (m_stepdlg.m_saved && m_creating_step && selected_step != nullptr) {
        m_model->addStep(selected_step);
      } else if (m_stepdlg.m_cancelled && m_creating_step && selected_step != nullptr) {
        delete selected_step;
      }

      if (m_creating_step)
        selected_step = nullptr;
      m_creating_step = false;
    }
  }
  else if (m_show_set_dlg) {
    m_setdlg.Draw("Create Set", &m_show_set_dlg, m_selector.getSelectedNodeCount());
    if (!m_show_set_dlg) {
      if (m_setdlg.m_saved) {
        if (m_setdlg.set_type == NODE_SET) {
          Mesh* targetMesh = findCommonMeshForNodes(m_model, m_selector.getSelectedNodes());
          if (targetMesh == nullptr) {
            cout << "Cannot create node set: selected nodes must belong to the same mesh" << endl;
          } else {
            NodeSet nodeSet(targetMesh);
            nodeSet.setEntityId(findNextNodeSetId(m_model));
            nodeSet.setLabel(m_setdlg.m_name);
            for (Node* node : m_selector.getSelectedNodes()) {
              nodeSet.add(node);
            }
            targetMesh->addNodeSet(nodeSet);
            selectNodeSet(targetMesh, targetMesh->getNodeSetCount() - 1);
            cout << "Created node set: name=" << m_setdlg.m_name
                 << ", id=" << nodeSet.getId()
                 << ", nodes=" << nodeSet.getItemCount() << endl;
          }
        } else {
          cout << "Create set requested: name=" << m_setdlg.m_name
               << ", type=" << m_setdlg.set_type << endl;
        }
      } else if (m_setdlg.m_cancelled) {
        cout << "Create set cancelled" << endl;
      }
      m_setdlg.reset();
    }
  }//show_set_mat
  else if (m_show_rename_set_dlg) {
    if (ImGui::Begin("Rename Set", &m_show_rename_set_dlg)) {
      NodeSet* selectedSet = getSelectedNodeSet();
      if (selectedSet == nullptr) {
        ImGui::TextDisabled("No set selected.");
      } else {
        ImGui::InputText("Name", m_rename_set_name, IM_ARRAYSIZE(m_rename_set_name));
        if (ImGui::Button("Save")) {
          selectedSet->setLabel(m_rename_set_name);
          cout << "Renamed set to " << m_rename_set_name << endl;
          m_show_rename_set_dlg = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
          m_show_rename_set_dlg = false;
        }
      }
    }
    ImGui::End();
  }
  
  if (create_new_mat) {
    m_show_mat_dlg=false;
    //SHOULD NOT BE CALLED AGAIN!!!!!!
    //cout << "temp dens" <<m_matdlg.m_density_const<<endl;
    //m_mats.push_back(new Material_(mat));
    //Material_* newMat = new Material_(m_matdlg.m_temp_mat); // clon/copia
    m_mats.push_back(m_matdlg.m_temp_mat);
    getApp().getActiveModel().addMaterial(m_matdlg.m_temp_mat);
    m_matdlg.m_temp_mat = nullptr; // opcional, para que el diálogo se pueda reiniciar
    
    //getApp().getActiveModel().addMaterial(new Material_(mat));
    cout << "Material size is "<< m_mats.size()<<endl;
    cout<<"Material Created"<<endl; 
    //cout << "Density:" <<m_mats[0]->getDensityConstant()<<endl;
    
    

  } else if (m_matdlg.cancel_action) {    
    m_show_mat_dlg=false;
  }
  
  if (m_jobdlg.create_entity){
    cout << "Creating Job "<<m_jobdlg.m_filename<<endl;
    m_jobs.push_back(new Job(m_jobdlg.m_filename)); 
    m_jobdlg.create_entity = false;
    m_jobdlg.m_show=false;
  }
    
  if (m_show_app_console) {
    static ExampleAppConsole console;
    g_app_console = &console;
    flushPendingConsoleOutput();
    console.Draw("Example: Console", &m_show_app_console);    
  } else {
    g_app_console = nullptr;
  }            //ShowExampleAppConsole(&m_show_app_console);

  drawScriptBrowserWindow();
  drawResultsLoadProgress();
  advanceResultsLoad();
  updatePartOverlay();
  updateBoundaryConditionOverlay();
  
  ImGui::End();

} //GUI


static void KeyCallback(GLFWwindow* pWindow, int key, int scancode, int action, int mods) {   
    ImGui_ImplGlfw_KeyCallback(pWindow, key, scancode, action, mods);
    editor->Key(key, scancode, action, mods);

    if (action == GLFW_PRESS) {
      //TODO: CHECK IF NO ACTIVE CRITIC DIALOG IS OPEN
        //~ switch(key) {
            //~ case GLFW_KEY_A:
                //~ std::cout << "Pressed A VIA KEY CALLBACK" << std::endl;
                //~ break;
            //~ case GLFW_KEY_O:
                //~ std::cout << "Pressed O, opening file" << std::endl;
                //~ break;
        //~ }
    }
    
}

void Editor::Key(int key, int scancode, int action, int mods) {   
    (void)scancode;
    (void)mods;
    if (action != GLFW_PRESS) {
      return;
    }

    if (!m_moving_mode) {
      return;
    }

    if (key == GLFW_KEY_ESCAPE) {
      finishMoveMode(false);
    } else if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER) {
      finishMoveMode(true);
    }
}

bool mouse_pressed;

//TODO: WHY ALWAYS IS STATIC?
static void CursorPosCallback(GLFWwindow* pWindow, double x, double y) {
    ImGui_ImplGlfw_CursorPosCallback(pWindow, x, y);
/*    
    // // //If callbacks are set AFTER, THEN THIS NEEDS TO BE CALLED
    // // //ImGui_ImplGlfw_CursorPosCallback(pWindow,x,y);//THIS IS IN ORDER 
    editor->CursorPos(x,  y);
    //cout << "cursor pos "<<x<<", "<<y<<endl;
    
    cout << "x bef"<<x<<endl;
    x -= (vmin.x + vpos.x); 
    y -= (vmin.y + vpos.y);
    float xx = ((x - (scr_width/2) ) / (scr_width/2));
    float yy  = (((scr_height/2) - y) / (scr_height/2));
    cout << "x cent "<<xx<<endl;
    //cout << 
    editor->ArcCamera()->mouse_pos_callback(pWindow, xx,yy);
    */
}


///
void Editor:: CursorPos(double x, double y) {
     if (mouse_pressed){
      //camera->SetLastMousePos(x,y);
      mouse_pressed = false;
    }
    if (rotatecam){
      //camera->OnMouse((int)x, (int)y);
      //cout << "ROT; x, y "<<x <<", "<< y<< endl;
      // cout << "ArcBall Camera pos: "<< arcCamera->currentPos[0] <<", " 
                                  // << arcCamera->currentPos.y <<", "  
                                  // << arcCamera->currentPos.z <<", "<<endl;
      // cout << "ArcBall Camera rot: "<< arcCamera->rotationalAxis.x <<", " 
                                  // << arcCamera->rotationalAxis.y <<", "  
                                  // << arcCamera->rotationalAxis.z <<", "<<endl;
// and gluLookAt is equivalent to
// glMultMatrixf(M);
// glTranslated(-eyex, -eyey, -eyez);
// gluLookAt(GLdouble eyex, GLdouble eyey, GLdouble eyez, GLdouble centerx,
      // GLdouble centery, GLdouble centerz, GLdouble upx, GLdouble upy,
      // GLdouble upz)


    double x,y;
    glfwGetCursorPos(window, &x, &y);
    m_rotation = x-last_mouse_x;
    //cout << "rotation " <<m_rotation<<endl;
      
        
    }
    // if (m_left_button_pressed){
      // PickingTexture::PixelInfo Pixel = m_pickingTexture.ReadPixel(x, SCR_HEIGHT - y - 1);
      // cout << "x,y: "<<x<<" , "<<y<<endl;
      // cout << "Obj ID "<<int(Pixel.ObjectID)<<", DrawID" << int(Pixel.DrawID)<<", PrimID" << int(Pixel.PrimID)<<endl;
    // }
}

static void MouseCallback(GLFWwindow* pWindow, int Button, int Action, int Mode){
  ImGui_ImplGlfw_MouseButtonCallback(pWindow, Button, Action, Mode);
/*  
  editor->Mouse(Button, Action, Mode);
  double x, y;
  glfwGetCursorPos(pWindow, &x, &y); 
    x -= (vmin.x + vpos.x); 
    y -= (vmin.y + vpos.y);
    float xx = ((x - (scr_width/2) ) / (scr_width/2));
    float yy  = (((scr_height/2) - y) / (scr_height/2));
*/
  //editor->ArcCamera()->mouse_button_callback(pWindow, xx, yy, Button, Action, Mode);
}

void Editor::Mouse(int Button, int Action, int Mode) {
  /*
    // // OGLDEV_MOUSE OgldevMouse = GLFWMouseToOGLDEVMouse(Button);

    // OGLDEV_KEY_STATE State = (Action == GLFW_PRESS) ? OGLDEV_KEY_STATE_PRESS : OGLDEV_KEY_STATE_RELEASE;
    //if (Button == GLFW_MOUSE_BUTTON_MIDDLE)
      if(Button == GLFW_MOUSE_BUTTON_MIDDLE && Action == GLFW_PRESS) {
        rotatecam = true;
        if (!mouse_pressed)
          mouse_pressed = true; // To take action in camera target 
      }
      if(Button == GLFW_MOUSE_BUTTON_MIDDLE && Action == GLFW_RELEASE) {
        rotatecam = false;
      }
      if(Button == GLFW_MOUSE_BUTTON_LEFT && Action == GLFW_PRESS) {
        double x,y;
          
        glfwGetCursorPos(window, &x, &y);
        m_left_button_pressed = true;
        cout << "mouse x,y "<<x<<", "<<y<<endl;
        //ImVec2 absmin = vmin + vpos;
        //ImVec2 absmax = vmax + vpos;
        cout << "abs mouse pos x,y "<<vpos.x + vmin.x <<", "<< vpos.y + vmin.x<<endl;
        cout << "vmin x,y "<<vmin.x<<", " << vmin.y<<endl;
        cout << "vmax x,y "<<vmax.x<<", " << vmax.y<<endl;
        // INSIDE MODEL RENDER WINDOW
        if (x >vmin.x + vpos.x && x < vmax.x + vpos.x && 
            y >vmin.y + vpos.y && y < vmax.y + vpos.y) {
          //GET RELATIVE COORD:
          x -= (vmin.x + vpos.x); 
          y -= (vmin.y + vpos.y);
          cout << "rel x, y "<<x << ", "<<y<<endl;
          cout <<"INSIDE"<<endl;
          cout << "scr_width "<< scr_width <<", from vmin and vmax "<< vmax.x-vmin.x<<endl;
        //cout << "screen x y" <<x << ", " <<y<<endl; 
          float xx = ((x - (scr_width/2) ) / (scr_width/2));
          float yy  = (((scr_height/2) - y) / (scr_height/2));
          cout << "x,y: "<<xx<<" , "<<yy<<endl;        
        PickingTexture::PixelInfo Pixel = m_pickingTexture.ReadPixel(x, scr_height - y - 1);
        
        if (!box_select_mode)  { //SINGLE SELECT
        cout << "Pixel.ObjectID " << Pixel.ObjectID<<endl;
              
          cout << "x,y: "<<x<<" , "<<y<<endl;
          int test = m_pickingTexture.ReadPixelToInt(x, scr_height - y - 1)-1;
          if (test >=0 ) {
            //cout<<"SELECTED!"<<endl;
          //CHECK
            Vec3_t v = m_domain.Particles[test]->x;        
            glm::vec4 pos(v(0),v(1),v(2),1.0);
            glm::vec4 res = trans_mat[test] * pos;
            cout << "particle center "<<res.x<<", "<<res.y<<endl;

             m_sel_count = 1;
             //cout << " m_sel_count = 1; "<<endl;
            m_sel_particles.clear();
            m_sel_particles.push_back(test);
          //}//If is sel
          cout << "Pressed"<<endl;
          
            float xx = ((x - (scr_width/2) ) / (scr_width/2));
            float yy  = (((scr_height/2) - y) / (scr_height/2));
            cout << "x,y: "<<xx<<" , "<<yy<<endl;
            }
          else          {cout<<"NOT SELECTED"<<endl;}
          cout << "Obj ID "<<test<<", DrawID" << int(Pixel.DrawID)<<", PrimID" << int(Pixel.PrimID)<<endl;
           logtest.AddLog("Selected particle %d  \n",test);           
        //if (m_is_node_sel){

        } else {
          m_selector.setStartPoint(x,y);
          cout <<"OUTSIDE"<<endl;
        }
        
        }// IF INSIDE SELECTION
        
        last_mouse_x = x;
        last_mouse_y = y;  

      }// if press
      if(Button == GLFW_MOUSE_BUTTON_LEFT && Action == GLFW_RELEASE){
        m_left_button_pressed = false;
        cout <<"released "<<"box_select_mode "<<box_select_mode<<endl;
        if (box_select_mode) {
          m_sel_particles.clear();
          m_sel_count = 0;
          double x,y;
          glfwGetCursorPos(window, &x, &y);
          cout << "xy current "<<x<< "; "<<y<<endl;
          cout << "xy initial "<<last_mouse_x<<", "<<last_mouse_y<<endl;
          
          // float xd_curr,yd_curr;
          // xd_curr = ((x - (scr_width/2) ) / (scr_width/2));
          // yd_curr = (((scr_height/2) - y) / (scr_height/2));
          // float xd_last,yd_last;
          // xd_last = ((last_mouse_x - (scr_width/2) ) / (scr_width/2));
          // yd_last = (((scr_height/2) - last_mouse_y) / (scr_height/2));
          

          // cout << "xy current int "<<xd_curr<< "; "<<yd_curr<<endl;
          // cout << "xy initial int "<<xd_last<<", "<<yd_last<<endl;
          
          //Loop through texture
          for (int p=0;p<m_domain.Particles.size();p++){    
            Vec3_t v = m_domain.Particles[p]->x;
            //Vector3f pos(v(0),v(1),v(2));
            
            glm::vec4 pos(v(0),v(1),v(2),1.0);

        
            glm::vec4 res = trans_mat[p] * pos;
            int resint_x = res.x * (scr_width/2) + (scr_width/2); 
            int resint_y = (scr_height/2) - res.y * (scr_height/2);
            
            //TODO: CHANGE TO AABBBOX
            int xbox[2],ybox[2];
            xbox[0] = std::min(x,last_mouse_x); xbox[1] = std::max(x,last_mouse_x);
            ybox[0] = std::min(y,last_mouse_y); ybox[1] = std::max(y,last_mouse_y);
            
            //cout << "particle " << p<< "pos on screen "<< res.x << "; "<<res.y<<"; "<<res.z<<endl;
            //cout << "particle " << p<< "pos on screen "<< resint_x << "; "<<resint_y<<endl;
            //if (res.x > xd_last && res.x < xd_curr && res.y > yd_last && res.y < yd_curr){
            if (resint_x > xbox[0] && resint_x < xbox[1] && resint_y > ybox[0] && resint_y < ybox[1]){
              //cout << "SELECTED"<<endl;
                
              m_sel_count++;
              m_sel_particles.push_back(p);
            }
          }// For particles

          cout << "Selected %d particles "<<m_sel_count<<endl;
          logtest.AddLog("Selected %d particles \n",m_sel_count);
            
          // for (int i=(int)last_mouse_x;i<=(int)x;i++){
            // for (int j=(int)last_mouse_y;j<=(int)y;j++) {
            // PickingTexture::PixelInfo Pixel = m_pickingTexture.ReadPixel(i, scr_height - j - 1);
            // /// THIS SHOULD BE DONE ONLY WITH THE CENTERS 
            // cout << "obj id " << Pixel.ObjectID<<endl;
              // if (Pixel.ObjectID != 0){
                // Vector3f vel(0.,0.,0.);
                // m_sel_node = Pixel.ObjectID-1;
                // int test = m_pickingTexture.ReadPixelToInt(x, scr_height - y - 1);
                // cout << "Obj ID "<<test<<", DrawID" << int(Pixel.DrawID)<<", PrimID" << int(Pixel.PrimID)<<endl;          
              // }
            // }
          // }

        }//Box select
      }//release button
      
*/
}

void Editor::MoveNode(){
  /*
    if (m_is_node_sel){
      double x,y;
      glfwGetCursorPos(window, &x, &y);
      Vector3f vel(0.,0.,0.);
      float dt = (float) (GetCurrentTimeMillis() - m_last_mouse_dragtime)*1000.;
      //We have to transform coordinates!
      //if (dt > 10){
        vel = Vector3f( (x-last_mouse_x)/dt,
                        (y-last_mouse_y)/dt,
                        0.);


        cout << "Node Vel "<<vel.x<< ", "<< vel.y<< ", "<<vel.z<<endl;
        
        last_mouse_x = x;
        last_mouse_y = y;
        m_last_mouse_dragtime = GetCurrentTimeMillis();
      //}
    } //Is node sel
    */
}

// FROM https://stackoverflow.com/questions/44753169/opengl-glfw-scroll-wheel-not-responding
// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
  ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
  editor->scroll(xoffset, yoffset);
}

void Editor::scroll(double xoffset, double yoffset)
{
  zcam +=0.01*yoffset;
  //camera->MoveFwd(yoffset*0.1);
}


Editor::Editor(){
  initializeConsoleStreamMirroring();
  m_jobdlg.m_open_results = [this](Job* job) { return openResultsForJob(job); };
  m_jobshowdlg.m_open_results = [this](Job* job) { return openResultsForJob(job); };
  /*
  //window = NULL;
  SCR_WIDTH = 800;
  SCR_HEIGHT = 600;
  is_struct = false;
  
  arcCamera = new ArcballCamera;
  
  box_select_mode = false;
  
  m_currentaction = NULL;
  m_show_mat_dlg = false;
  m_show_set_dlg = false;
  m_show_job_dlg = false;
  
  m_sel_particles.resize(1);
  m_sel_count = 0;
  
  is_fem_mesh = false;
  is_sph_mesh = false;
  */
  
  m_show_msh_dlg = false;
  m_show_set_dlg = false;
  
  m_show_app_console = true;
  m_model = new Model();
  getApp().setActiveModel(m_model);
  Init();
}

int Editor::Init(){
  /*
  m_pause = true;
  mouse_pressed = false;
  rotatecam = false;

  // // glfw: initialize and configure
  // // ------------------------------
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // glfw window creation
  // --------------------
  bool isFullScreen = false;
   GLFWmonitor* pMonitor = isFullScreen ? glfwGetPrimaryMonitor() : NULL;
  const GLFWvidmode * mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

  SCR_WIDTH = mode->width;
  SCR_HEIGHT = mode->height;
  cout << "Working area: "<<SCR_WIDTH<< ", "<<SCR_HEIGHT<<endl;
    
  window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "WeldFormGUI", NULL, NULL);
  if (window == NULL)
  {
      std::cout << "Failed to create GLFW window" << std::endl;
      glfwTerminate();
      return -1;
  }
  glfwMakeContextCurrent(window);
  

  // glad: load all OpenGL function pointers
  // ---------------------------------------
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
  {
      std::cout << "Failed to initialize GLAD" << std::endl;
      return -1;
  }


  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSetWindowSizeCallback(window, window_size_callback);


  glfwSetKeyCallback(window, KeyCallback);
  
  glfwSetCursorPosCallback(window, CursorPosCallback);
  glfwSetMouseButtonCallback(window, MouseCallback);
  glfwSetScrollCallback(window, scroll_callback);
  
  // build and compile our shader program
  // ------------------------------------
  // vertex shader
  unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
  glCompileShader(vertexShader);
  // check for shader compile errors
  int success;
  char infoLog[512];
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
  if (!success)
  {
      glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
      std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
  }
  // fragment shader
  unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
  glCompileShader(fragmentShader);
  // check for shader compile errors
  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
  if (!success)
  {
      glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
      std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
  }
  // link shaders
  shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);
  // check for linking errors
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success) {
      glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
      std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
  }
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);
  
  printf("OpenGL %s, GLSL %s\n", glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));


  Vector3f Pos(0.5f, 0.5f, -0.5f);
  Vector3f Target(-0.5f, -0.5f, 1.0f);
  Vector3f Up(0.0, 1.0f, 0.0f);

  //camera = new Camera(SCR_WIDTH, SCR_HEIGHT, Pos, Target, Up);

  Pipeline p;
  m_persProjInfo.FOV    = 60.0f;
  m_persProjInfo.Height = SCR_HEIGHT;
  m_persProjInfo.Width  = SCR_WIDTH;
  m_persProjInfo.zNear  = 1.0e-4f;
  m_persProjInfo.zFar   = 100.0f;  
  
  
  cout << "Loading ground"<<endl;
  LoadSphere();

  /// NOW LIGHT TECHNIQUE
  m_plightEffect = new BasicLightingTechnique();

  if (!m_plightEffect->Init()) {
  printf("Error initializing the lighting technique\n");
  return false;
  }

  m_plightEffect->Enable();

  
  m_directionalLight.Color = Vector3f(0.5f, 1.f, 1.0f);
  m_directionalLight.AmbientIntensity = 0.15f;
  m_directionalLight.DiffuseIntensity = 0.5f;
  m_directionalLight.Direction = Vector3f(1.0f, 1.0, 0.0);


  m_plightEffect->SetColorTextureUnit(COLOR_TEXTURE_UNIT_INDEX);
  m_plightEffect->SetDirectionalLight(m_directionalLight);
  m_plightEffect->SetMatSpecularIntensity(0.0f);
  m_plightEffect->SetMatSpecularPower(0);

  cout << "Creating plane"<<endl;
  //AT LAST, PHYSICS
  //Point and normal
  Vector3f point(1.,0.,1.);
  Vector3f normal(0.,1.,0.);




  ////// Setup Dear ImGui context

  glfwSwapInterval(1); // Enable vsync
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  ////io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
  ////io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  //ImGui::StyleColorsClassic();

  const char* glsl_version = "#version 130";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  //// Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);
  
  cout << "GUI done"<<endl;


  m_Text = new TextRenderer(SCR_WIDTH,SCR_HEIGHT); //CRASH

  // uncomment this call to draw in wireframe polygons.
  //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

  // render loop
  // -----------

  float c_dot;
  Vector3f vold;

  float runtime;
  int framecount=0;
  long long frame_time;

  float vnew [4];
  bool impact = false;

  m_start_time = GetCurrentTimeMillis();
  long long frame_count = 0;

  //For text 
  // glEnable(GL_BLEND);
  // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


  double L = 0.5;
  double H = 0.5;
  m_dx = 0.05;
  double rho = 1.;
  double h = 1.2*m_dx;
  cout << "Generating domain "<<endl;
  m_domain.Dimension = 3;

  cout << "Done. "<<endl;
  

  float kin_energy;

  
  mesh_loaded = false;

  
  

  // is_struct = true;
  // cout << "Nodes "<<st->GetNodeCount()<<endl;
  // for (int i=0;i<st->GetNodeCount();i++) {
    // myMesh *mesh = new myMesh();
      // if (!mesh->LoadMesh(
        // "Sphere.dae"
        // )) {
        // std::cout<<"Mesh load failed"<<endl;
        // printf("Mesh load failed\n");

      // }
      // else {
        // m_nodemesh.push_back(mesh);
        // mesh_loaded = true;
      // }
  // }

   if (!m_pickingTexture.Init(SCR_WIDTH, SCR_HEIGHT)) {
      cout << "Error generating picking texture"<<endl;
      //return false;
  }

  if (!m_pickingEffect.Init()) {
    cout << "Error initializing picking shader"<<endl;
      return false;
  } 
  

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0);   

    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST); //DO NOT FORGET!
    
    
  ///////////////// TEXT ////
  m_Text = new TextRenderer(SCR_WIDTH,SCR_HEIGHT); //CRASH
  
  //Like OgldevApp
  m_frameCount = 0;
  m_frameTime = 0;
  m_fps = 0;
  
  m_is_node_sel = false;

  m_frameTime = m_startTime = GetCurrentTimeMillis();
  
  m_impact_force = 0.;

  m_sel_particles[0] = -1;
  
  m_model = new Model;

  m_add_part = false;
    m_sceneview = new SceneView(100,100);  
  
  m_viewport_win = new ViewportWindow(this);
  
  //ONLY AT THE BEGINING
  scr_width = SCR_WIDTH;
  scr_height = SCR_HEIGHT;
  
  */

  m_show_mat_dlg = false;
  m_show_mat_dlg_edit = false;
  create_new_mat = false;
  
  gizmo = vtkSmartPointer<TransformGizmo>::New();
 
  return 1;
}//Editor::Init()


int Editor::Terminate(){

    // glDeleteProgram(shaderProgram);

    // // glfw: terminate, clearing all previously allocated GLFW resources.
    // // ------------------------------------------------------------------
    // glfwTerminate();
     return 0;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void Editor::processInput(GLFWwindow *window)
{
    // if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        // glfwSetWindowShouldClose(window, true);
      
    //~ if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
      //~ cout << "Pressed A"<<endl;
    
    //~ if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS){
      
      //~ cout << "Opening file "<<endl;
      
      //~ std::string filename = "out_0.000010.vtk";
      //~ ResultFrame *frame = new ResultFrame(filename);
      //~ frame->setActiveScalarField("DISP");      
      //~ viewer->addActor(frame->actor);
    //~ }

    //~ if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
      //~ box_select_mode = !box_select_mode;
      //~ cout << "Pressed S"<<endl;
      //~ cout << "Box Select Mode ";
      //~ if (box_select_mode ) cout << "ON "<<endl;
      //~ else                  cout << "OFF "<<endl;

      
    //~ }

    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
      rotatecam = true;
      //editor->ArcCamera()->setRotFlag(true);
    else
      //(if !MIDDLE BUTTON)
      rotatecam = false;
      //editor->ArcCamera()->setRotFlag(false);
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
      //cout << "pause: "<<endl;
      m_pause = !m_pause;
    }
    
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

void window_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	//editor->getSceneView()->getFrameBuffer()->RescaleFrameBuffer(width, height);
}




void Editor::CalcFPS()
{
    m_frameCount++;
/*
    //long long time = GetCurrentTimeMillis();
    
    if (time - m_frameTime >= 1000) {
        m_frameTime = time;
        m_fps = m_frameCount;
        m_frameCount = 0;
    }
*/
//IMGUI_DEMO_MARKER
}

///// TODO: PASS TO RENDERER
/////////////////////////////
void Editor::calcDomainCenter(){
  
  m_domain_center = 0.0;
  //Converting from Vec3_t to Vector3f
  //SELECT IF DOMAIN IS SPH
  //for (int p=0;p<m_domain->Particles.size();p++)    {
    //m_domain_center.x += m_domain->Particles[p]->x(0);
    //m_domain_center.y += m_domain->Particles[p]->x(1);
    //m_domain_center.z += m_domain->Particles[p]->x(2);
  //}
  //m_domain_center = m_domain_center/m_domain->Particles.size();
  
}

void Editor::calcMeshCenter(){
  /*
  m_femsh_center = 0.0;
  //Converting from Vec3_t to Vector3f
  //SELECT IF DOMAIN IS SPH
  for (int p=0;p<m_domain->Particles.size();p++)    {
    m_femsh_center.x += m_domain->Particles[p]->x(0);
    m_femsh_center.y += m_domain->Particles[p]->x(1);
    m_femsh_center.z += m_domain->Particles[p]->x(2);
  }
  m_femsh_center = m_femsh_center/m_domain->Particles.size();
  */
}

void Editor::addViewer(VtkViewer *v){
  viewer=v;
  
  }
