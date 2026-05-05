// Standard Library
#include <iostream>
#include <filesystem>


/////https://github.com/trlsmax/imgui-vtk/tree/master

// OpenGL Loader
// This can be replaced with another loader, e.g. glad, but
// remember to also change the corresponding initialize call!
#include <GL/gl3w.h>            // GL3w, initialized with gl3wInit() below

// Include glfw3.h after our OpenGL definitions
#include <GLFW/glfw3.h>

// ImGui + imgui-vtk
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include "VtkViewer.h"

// VTK
#include <vtkSmartPointer.h>
#include <vtkActor.h>
#include "editor.h"



// File-Specific Includes
#include "imgui_vtk_demo.h" // Actor generator for this demo


#include <vtkActor.h>
//#include <vtkArrowSource.h>
#include <vtkNamedColors.h>
#include <vtkScalarBarActor.h>
#include <vtkCamera.h>
#include <vtkMapper.h>
#include <vtkProperty.h>


#include "graphics/axis.h" //test



#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>

#include <vtkArrowSource.h>

///// FOR GEOMETRIA
#include "vtkOCCTReader.h"
#include <vtkCompositePolyDataMapper.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkRegressionTestImage.h>

#include "geom/vtkOCCTGeom.h"

#include <gmsh.h>

#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifdef BUILD_PYTHON
#include <Python.h>
#endif

#include "App/App.h"
#include "GraphicMesh.h"

#include "results.h"
#include "load_plot_dialog.h"
//using App;
#include "geom/vtkOCCTGeom.h"
#include "geom/ShapeToPolyData.h"

#include <vtkSmartPointer.h>
#include <vtkFileOutputWindow.h>
#include <vtkOutputWindow.h>

//THIS IS TO AVOIDERROR WITH MINMAX
#ifdef _WIN32 
  #undef min
  #undef max
#endif

#include "demo_dialog.h"

// Simple helper function to load an image into a OpenGL texture with common settings
bool LoadTextureFromMemory(const void* data, size_t data_size, GLuint* out_texture, int* out_width, int* out_height)
{
    // Load from file
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load_from_memory((const unsigned char*)data, (int)data_size, &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return false;

    // Create a OpenGL texture identifier
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixels into texture
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    *out_texture = image_texture;
    *out_width = image_width;
    *out_height = image_height;

    return true;
}

namespace {
enum class UIFontChoice {
    ImGuiDefault,
    Ubuntu
};

enum class ModelDisplayMode {
    Surface,
    Wireframe
};

struct ModelViewportOverlayState {
    ModelDisplayMode displayMode = ModelDisplayMode::Surface;
    bool showEdges = false;
    bool axesVisible = false;
    bool orthographic = false;
};

void applyDisplayModeToActor(vtkActor* actor, ModelDisplayMode mode, bool showEdges, bool preserveScalarColors)
{
    if (actor == nullptr || actor->GetProperty() == nullptr) {
        return;
    }

    vtkMapper* mapper = actor->GetMapper();
    if (mapper != nullptr) {
        if (preserveScalarColors) {
            mapper->ScalarVisibilityOn();
        } else {
            mapper->ScalarVisibilityOff();
        }
    }

    const bool hasScalarColors =
        preserveScalarColors && mapper != nullptr && mapper->GetScalarVisibility();
    vtkProperty* property = actor->GetProperty();
    switch (mode) {
    case ModelDisplayMode::Surface:
        property->SetRepresentationToSurface();
        if (showEdges) {
            property->EdgeVisibilityOn();
            property->SetEdgeColor(0.0, 0.0, 0.0);
        } else {
            property->EdgeVisibilityOff();
        }
        if (!hasScalarColors) {
            property->SetColor(0.84, 0.84, 0.84);
        }
        break;
    case ModelDisplayMode::Wireframe:
        property->SetRepresentationToWireframe();
        property->EdgeVisibilityOff();
        property->SetColor(0.0, 0.0, 0.0);
        break;
    }
}

void applyDisplayModeToActiveModel(ModelDisplayMode mode, bool showEdges)
{
    Model& model = getApp().getActiveModel();
    for (int p = 0; p < model.getPartCount(); ++p) {
        Part* part = model.getPart(p);
        if (part == nullptr) {
            continue;
        }

        if (GraphicMesh* graphicMesh = getApp().getGraphicMeshFromPart(part)) {
            applyDisplayModeToActor(graphicMesh->getActor(), mode, showEdges, false);
        }

        if (vtkOCCTGeom* visual = getApp().getVisualForPart(part)) {
            applyDisplayModeToActor(visual->actor, mode, showEdges, false);
        }
    }
}

void applyDisplayModeToResults(ModelDisplayMode mode, bool showEdges, Editor* editor)
{
    if (editor == nullptr || editor->getResults() == nullptr) {
        return;
    }

    for (auto& frame : editor->getResults()->frames) {
        if (!frame || !frame->actor) {
            continue;
        }
        applyDisplayModeToActor(frame->actor, mode, showEdges, true);
    }
}

bool drawToolbarButton(const char* label, bool active = false, const char* tooltip = nullptr)
{
    const ImVec2 buttonSize(0.0f, 24.0f);
    if (active) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.24f, 0.48f, 0.86f, 0.92f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.55f, 0.92f, 0.98f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.18f, 0.40f, 0.78f, 1.0f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.11f, 0.14f, 0.18f, 0.42f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.16f, 0.20f, 0.26f, 0.70f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.20f, 0.25f, 0.32f, 0.82f));
    }

    const bool pressed = ImGui::Button(label, buttonSize);
    if (tooltip != nullptr && ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", tooltip);
    }

    ImGui::PopStyleColor(3);

    return pressed;
}

bool drawAxisButton(const char* label, const ImVec4& baseColor)
{
    const ImVec2 buttonSize(24.0f, 24.0f);
    const ImVec4 hoveredColor(
        (std::min)(baseColor.x + 0.10f, 1.0f),
        (std::min)(baseColor.y + 0.10f, 1.0f),
        (std::min)(baseColor.z + 0.10f, 1.0f),
        1.0f);
    const ImVec4 activeColor(
        (std::max)(baseColor.x - 0.08f, 0.0f),
        (std::max)(baseColor.y - 0.08f, 0.0f),
        (std::max)(baseColor.z - 0.08f, 0.0f),
        1.0f);

    ImGui::PushStyleColor(ImGuiCol_Button, baseColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoveredColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeColor);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.949f, 0.957f, 0.965f, 1.0f));

    const bool pressed = ImGui::Button(label, buttonSize);

    ImGui::PopStyleColor(4);
    return pressed;
}

void drawOverlaySeparator()
{
    ImGui::SameLine(0.0f, 8.0f);
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddLine(ImVec2(pos.x, pos.y + 3.0f), ImVec2(pos.x, pos.y + 21.0f),
                       IM_COL32(255, 255, 255, 28), 1.0f);
    ImGui::Dummy(ImVec2(1.0f, 24.0f));
    ImGui::SameLine(0.0f, 8.0f);
}

bool drawProjectionButton(const char* id, bool orthographic, bool active, const char* tooltip)
{
    const ImVec2 size(24.0f, 24.0f);
    ImGui::PushID(id);
    const bool pressed = ImGui::InvisibleButton("##projection", size);
    const bool hovered = ImGui::IsItemHovered();
    const bool held = ImGui::IsItemActive();

    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    ImU32 bgColor = active ? IM_COL32(61, 122, 219, 235) : IM_COL32(28, 34, 44, 110);
    if (held) {
        bgColor = active ? IM_COL32(53, 106, 191, 255) : IM_COL32(46, 74, 122, 220);
    } else if (hovered) {
        bgColor = active ? IM_COL32(70, 132, 232, 245) : IM_COL32(54, 64, 80, 180);
    }

    const ImU32 strokeColor = IM_COL32(236, 240, 245, 255);
    drawList->AddRectFilled(min, max, bgColor, 6.0f);
    drawList->AddRect(min, max, IM_COL32(255, 255, 255, 18), 6.0f, 0, 1.0f);

    const float left = min.x + 5.0f;
    const float right = max.x - 5.0f;
    const float top = min.y + 6.0f;
    const float bottom = max.y - 5.0f;
    const float thickness = 1.4f;

    if (orthographic) {
        const float x1 = left + 1.0f;
        const float x2 = (left + right) * 0.5f;
        const float x3 = right - 1.0f;
        drawList->AddLine(ImVec2(x1, top), ImVec2(x1, bottom), strokeColor, thickness);
        drawList->AddLine(ImVec2(x2, top), ImVec2(x2, bottom), strokeColor, thickness);
        drawList->AddLine(ImVec2(x3, top), ImVec2(x3, bottom), strokeColor, thickness);
        drawList->AddLine(ImVec2(x1 - 1.0f, top), ImVec2(x3 + 1.0f, top), strokeColor, thickness);
        drawList->AddLine(ImVec2(x1 - 1.0f, bottom), ImVec2(x3 + 1.0f, bottom), strokeColor, thickness);
    } else {
        const float offset = 3.0f;
        drawList->AddLine(ImVec2(left + offset, top), ImVec2(right, top + 1.0f), strokeColor, thickness);
        drawList->AddLine(ImVec2(left, bottom - 1.0f), ImVec2(right - offset, bottom), strokeColor, thickness);
        drawList->AddLine(ImVec2(left + offset, top), ImVec2(left, bottom - 1.0f), strokeColor, thickness);
        drawList->AddLine(ImVec2((left + right) * 0.5f + 1.0f, top + 1.0f),
                          ImVec2((left + right) * 0.5f - 2.0f, bottom), strokeColor, thickness);
        drawList->AddLine(ImVec2(right, top + 1.0f), ImVec2(right - offset, bottom), strokeColor, thickness);
    }

    if (tooltip != nullptr && hovered) {
        ImGui::SetTooltip("%s", tooltip);
    }

    ImGui::PopID();
    return pressed;
}

bool drawFitIconButton()
{
    const ImVec2 size(24.0f, 24.0f);
    ImGui::PushID("fit_icon_button");
    const bool pressed = ImGui::InvisibleButton("##fit", size);
    const bool hovered = ImGui::IsItemHovered();
    const bool held = ImGui::IsItemActive();

    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    ImU32 bgColor = IM_COL32(28, 34, 44, 110);
    if (held) {
        bgColor = IM_COL32(46, 74, 122, 220);
    } else if (hovered) {
        bgColor = IM_COL32(54, 64, 80, 180);
    }

    const ImU32 strokeColor = hovered || held
        ? IM_COL32(255, 255, 255, 255)
        : IM_COL32(220, 224, 230, 255);

    drawList->AddRectFilled(min, max, bgColor, 6.0f);
    drawList->AddRect(min, max, IM_COL32(255, 255, 255, 18), 6.0f, 0, 1.0f);

    const float pad = 5.5f;
    const float arm = 4.0f;
    const float thickness = 1.6f;

    const float left = min.x + pad;
    const float right = max.x - pad;
    const float top = min.y + pad;
    const float bottom = max.y - pad;

    drawList->AddLine(ImVec2(left, top + arm), ImVec2(left, top), strokeColor, thickness);
    drawList->AddLine(ImVec2(left, top), ImVec2(left + arm, top), strokeColor, thickness);

    drawList->AddLine(ImVec2(right - arm, top), ImVec2(right, top), strokeColor, thickness);
    drawList->AddLine(ImVec2(right, top), ImVec2(right, top + arm), strokeColor, thickness);

    drawList->AddLine(ImVec2(left, bottom - arm), ImVec2(left, bottom), strokeColor, thickness);
    drawList->AddLine(ImVec2(left, bottom), ImVec2(left + arm, bottom), strokeColor, thickness);

    drawList->AddLine(ImVec2(right - arm, bottom), ImVec2(right, bottom), strokeColor, thickness);
    drawList->AddLine(ImVec2(right, bottom - arm), ImVec2(right, bottom), strokeColor, thickness);

    if (hovered) {
        ImGui::SetTooltip("Fit");
    }

    ImGui::PopID();
    return pressed;
}

void drawViewportOverlay(VtkViewer& viewer,
                         ModelViewportOverlayState& state,
                         const char* windowId,
                         bool allowAxes)
{
    const ImVec2 viewportMin = viewer.getViewportScreenMin();
    const ImVec2 viewportMax = viewer.getViewportScreenMax();
    if (viewportMax.x <= viewportMin.x || viewportMax.y <= viewportMin.y) {
        return;
    }

    ImGui::SetNextWindowPos(ImVec2(viewportMin.x + 12.0f, viewportMin.y + 12.0f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.18f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 6.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 4.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 7.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.04f, 0.05f, 0.07f, 0.18f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 0.06f));

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoMove;

    if (ImGui::Begin(windowId, nullptr, flags)) {
        if (drawFitIconButton()) {
            viewer.resetCamera();
        }
        ImGui::SameLine();
        if (drawAxisButton("X", ImVec4(0.70f, 0.24f, 0.24f, 1.0f))) {
            viewer.orientCameraToAxis(0);
        }
        ImGui::SameLine();
        if (drawAxisButton("Y", ImVec4(0.22f, 0.58f, 0.28f, 1.0f))) {
            viewer.orientCameraToAxis(1);
        }
        ImGui::SameLine();
        if (drawAxisButton("Z", ImVec4(0.24f, 0.40f, 0.76f, 1.0f))) {
            viewer.orientCameraToAxis(2);
        }

        drawOverlaySeparator();
        if (drawProjectionButton("ortho", true, state.orthographic, "Orthographic")) {
            state.orthographic = true;
            viewer.setProjectionMode(VtkViewer::ProjectionMode::Orthographic);
        }
        ImGui::SameLine();
        if (drawProjectionButton("persp", false, !state.orthographic, "Perspective")) {
            state.orthographic = false;
            viewer.setProjectionMode(VtkViewer::ProjectionMode::Perspective);
        }
        if (allowAxes) {
            ImGui::SameLine();
            if (drawToolbarButton("Ax", state.axesVisible, "Axes")) {
                state.axesVisible = !state.axesVisible;
                viewer.setAxesVisible(state.axesVisible);
            }
        }

        drawOverlaySeparator();
        if (drawToolbarButton("Ed", state.showEdges, "Element edges")) {
            state.showEdges = !state.showEdges;
        }
        ImGui::SameLine();
        if (drawToolbarButton("Wi", state.displayMode == ModelDisplayMode::Wireframe, "Wireframe")) {
            state.displayMode = ModelDisplayMode::Wireframe;
        }
        ImGui::SameLine();
        if (drawToolbarButton("Sf", state.displayMode == ModelDisplayMode::Surface, "Surface")) {
            state.displayMode = ModelDisplayMode::Surface;
        }
    }
    ImGui::End();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(4);

    viewer.setProjectionMode(state.orthographic
        ? VtkViewer::ProjectionMode::Orthographic
        : VtkViewer::ProjectionMode::Perspective);
    if (allowAxes) {
        viewer.setAxesVisible(state.axesVisible);
    }
}

std::filesystem::path ResolveExistingFontPath(const std::filesystem::path& executable_path,
                                              const std::vector<std::filesystem::path>& relative_candidates)
{
    std::vector<std::filesystem::path> search_roots;
    if (!executable_path.empty()) {
        search_roots.push_back(std::filesystem::absolute(executable_path).parent_path());
    }
    search_roots.push_back(std::filesystem::current_path());
    search_roots.push_back(std::filesystem::current_path() / "resources");
    search_roots.push_back(std::filesystem::current_path() / "data");
    search_roots.push_back(std::filesystem::current_path().parent_path() / "WeldFormGUI");
    search_roots.push_back(std::filesystem::current_path().parent_path() / "WeldFormGUI" / "resources");
    search_roots.push_back(std::filesystem::current_path().parent_path() / "WeldFormGUI" / "data");

    for (const auto& root : search_roots) {
        for (const auto& candidate : relative_candidates) {
            const std::filesystem::path full_path = root / candidate;
            if (std::filesystem::exists(full_path)) {
                return full_path;
            }
        }
    }

    return {};
}
}

// Open and read a file, then forward to LoadTextureFromMemory()
bool LoadTextureFromFile(const char* file_name, GLuint* out_texture, int* out_width, int* out_height)
{
    FILE* f = fopen(file_name, "rb");
    if (f == NULL)
        return false;
    fseek(f, 0, SEEK_END);
    size_t file_size = (size_t)ftell(f);
    if (file_size == -1)
        return false;
    fseek(f, 0, SEEK_SET);
    void* file_data = IM_ALLOC(file_size);
    const size_t bytes_read = fread(file_data, 1, file_size, f);
    if (bytes_read != file_size) {
        IM_FREE(file_data);
        fclose(f);
        return false;
    }
    bool ret = LoadTextureFromMemory(file_data, file_size, out_texture, out_width, out_height);
    IM_FREE(file_data);
    fclose(f);
    return ret;
}

static void glfw_error_callback(int error, const char* description)
{
  fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

#ifdef BUILD_PYTHON
namespace {
void addPythonSearchPath(const std::filesystem::path& path)
{
  if (path.empty()) {
    return;
  }

  const std::string normalized = std::filesystem::absolute(path).lexically_normal().string();
  if (normalized.empty()) {
    return;
  }

  PyObject* sysPath = PySys_GetObject("path");
  if (sysPath == nullptr || !PyList_Check(sysPath)) {
    std::cerr << "Failed to access Python sys.path" << std::endl;
    return;
  }

  PyObject* pyPath = PyUnicode_FromString(normalized.c_str());
  if (pyPath == nullptr) {
    PyErr_Clear();
    std::cerr << "Failed to convert Python search path: " << normalized << std::endl;
    return;
  }

  if (PySequence_Contains(sysPath, pyPath) == 0) {
    PyList_Insert(sysPath, 0, pyPath);
  }
  Py_DECREF(pyPath);
}
}
#endif


int main(int argc, char* argv[])
{
  vtkSmartPointer<vtkFileOutputWindow> output =
  vtkSmartPointer<vtkFileOutputWindow>::New();
  output->SetFileName("vtk_warnings.log");
  vtkOutputWindow::SetInstance(output);
    
  // Setup pipeline
  //auto actor = SetupDemoPipeline();

  // Setup window
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit()){
    return 1;
  }

  // Use GL 3.2 (All Platforms)
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

  // Decide GLSL version
#ifdef __APPLE__
  // GLSL 150
  const char* glsl_version = "#version 150";
#else
  // GLSL 130
  const char* glsl_version = "#version 130";
#endif


  const GLFWvidmode* modes = glfwGetVideoMode(glfwGetPrimaryMonitor()/*, &count*/);
  cout << "Monitor width: "<<modes->width<<", height: "<<modes->height<<endl;

  // Create window with graphics context
  GLFWwindow* window = glfwCreateWindow(modes->width, modes->height-80, "WeldForm GUI", NULL, NULL);
  
  //glfwSetWindowAttrib(window, GLFW_MAXIMIZED, GLFW_TRUE);
  glfwSetWindowPos(window, 1, 30);

  if (window == NULL){
    return 1;
  }
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // Enable vsync

  // Initialize OpenGL loader
  if (gl3wInit() != 0){
    fprintf(stderr, "Failed to initialize OpenGL loader!\n");
    return 1;
  }

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImPlot::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  ImFont* font1 = nullptr;
  ImFont* font_ubu = nullptr;
  bool ubuntu_font_available = false;
  float ui_font_size = 16.0f;
  float pending_ui_font_size = ui_font_size;
  bool font_rebuild_requested = false;
  const std::filesystem::path ubuntu_font_path = ResolveExistingFontPath(
      argc > 0 && argv[0] != nullptr ? std::filesystem::path(argv[0]) : std::filesystem::path(),
      {
          "Ubuntu-Regular.ttf",
          "Ubuntu-L.ttf",
          std::filesystem::path("resources") / "Ubuntu-Regular.ttf",
          std::filesystem::path("resources") / "Ubuntu-L.ttf",
          std::filesystem::path("data") / "Ubuntu-Regular.ttf",
          std::filesystem::path("data") / "Ubuntu-L.ttf"
      });
  if (!ubuntu_font_path.empty()) {
    std::fprintf(stderr, "INFO: Ubuntu font candidate found at: %s\n", ubuntu_font_path.string().c_str());
  } else {
    std::fprintf(stderr, "INFO: Ubuntu font candidate not found. cwd=%s\n",
                 std::filesystem::current_path().string().c_str());
    if (argc > 0 && argv[0] != nullptr) {
      std::fprintf(stderr, "INFO: Executable path argument: %s\n", argv[0]);
    }
  }
  UIFontChoice current_font_choice = UIFontChoice::Ubuntu;
  ImFont* current_ui_font = nullptr;

  auto rebuildUiFonts = [&]() {
    io.Fonts->Clear();
    font1 = io.Fonts->AddFontDefault();
    font_ubu = nullptr;
    if (!ubuntu_font_path.empty()) {
      font_ubu = io.Fonts->AddFontFromFileTTF(ubuntu_font_path.string().c_str(), ui_font_size, NULL, io.Fonts->GetGlyphRangesDefault());
    }
    ubuntu_font_available = font_ubu != nullptr;
    if (!ubuntu_font_available) {
      fprintf(stderr,"WARNING. Cannot load Ubuntu font, using Dear ImGui default.\n");
      current_font_choice = UIFontChoice::ImGuiDefault;
    } else {
      std::fprintf(stderr, "INFO: Ubuntu font added successfully at %.1f px.\n", ui_font_size);
    }

    current_ui_font = (current_font_choice == UIFontChoice::Ubuntu && ubuntu_font_available) ? font_ubu : font1;
    io.FontDefault = current_ui_font;
    io.Fonts->Build();
  };

  rebuildUiFonts();
    
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows'

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  ImGui::GetStyle().ScaleAllSizes(1.1f);
  ImGuiStyle& style = ImGui::GetStyle();
  style.Colors[ImGuiCol_Text]               = ImVec4(0.949f, 0.957f, 0.965f, 1.0f); // #f2f4f6
  style.Colors[ImGuiCol_TextDisabled]       = ImVec4(0.620f, 0.671f, 0.733f, 1.0f);
  style.Colors[ImGuiCol_WindowBg]           = ImVec4(0.067f, 0.094f, 0.153f, 0.96f); // #111827
  style.Colors[ImGuiCol_ChildBg]            = ImVec4(0.067f, 0.094f, 0.153f, 0.92f);
  style.Colors[ImGuiCol_PopupBg]            = ImVec4(0.090f, 0.117f, 0.176f, 0.98f);
  style.Colors[ImGuiCol_FrameBg]            = ImVec4(0.118f, 0.145f, 0.204f, 0.88f);
  style.Colors[ImGuiCol_FrameBgHovered]     = ImVec4(0.157f, 0.188f, 0.255f, 0.95f);
  style.Colors[ImGuiCol_FrameBgActive]      = ImVec4(0.184f, 0.220f, 0.294f, 1.0f);
  style.Colors[ImGuiCol_TitleBg]            = ImVec4(0.067f, 0.094f, 0.153f, 1.0f);
  style.Colors[ImGuiCol_TitleBgActive]      = ImVec4(0.090f, 0.117f, 0.176f, 1.0f);
  style.Colors[ImGuiCol_MenuBarBg]          = ImVec4(0.090f, 0.117f, 0.176f, 0.98f);
  style.Colors[ImGuiCol_Header]             = ImVec4(0.149f, 0.184f, 0.255f, 0.85f);
  style.Colors[ImGuiCol_HeaderHovered]      = ImVec4(0.188f, 0.227f, 0.310f, 0.95f);
  style.Colors[ImGuiCol_HeaderActive]       = ImVec4(0.220f, 0.259f, 0.353f, 1.0f);
  style.Colors[ImGuiCol_Button]             = ImVec4(0.149f, 0.184f, 0.255f, 0.80f);
  style.Colors[ImGuiCol_ButtonHovered]      = ImVec4(0.188f, 0.227f, 0.310f, 0.92f);
  style.Colors[ImGuiCol_ButtonActive]       = ImVec4(0.220f, 0.259f, 0.353f, 1.0f);
  style.Colors[ImGuiCol_Tab]                = ImVec4(0.118f, 0.145f, 0.204f, 0.94f);
  style.Colors[ImGuiCol_TabHovered]         = ImVec4(0.157f, 0.188f, 0.255f, 0.98f);
  style.Colors[ImGuiCol_TabActive]          = ImVec4(0.184f, 0.220f, 0.294f, 1.0f);
  style.Colors[ImGuiCol_TabUnfocused]       = ImVec4(0.090f, 0.117f, 0.176f, 0.92f);
  style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.149f, 0.184f, 0.255f, 0.96f);
  style.Colors[ImGuiCol_Separator]          = ImVec4(0.220f, 0.259f, 0.353f, 0.65f);
  style.Colors[ImGuiCol_Border]             = ImVec4(0.220f, 0.259f, 0.353f, 0.40f);
  style.Colors[ImGuiCol_CheckMark]          = ImVec4(0.949f, 0.957f, 0.965f, 1.0f);
  style.Colors[ImGuiCol_SliderGrab]         = ImVec4(0.482f, 0.580f, 0.741f, 0.90f);
  style.Colors[ImGuiCol_SliderGrabActive]   = ImVec4(0.561f, 0.659f, 0.820f, 1.0f);

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);


  // Initialize VtkViewer objects
  //VtkViewer vtkViewer1;
  //vtkViewer1.addActor(actor);
  
  VtkViewer vtkViewer2;
  VtkViewer vtkViewer_res;
  LoadPlotDialog loadPlotDialog;
  bool showLoadPlotDialog = false;

  static DemoDialog demoDialog;
  static bool showDemoDialog = false;
  static bool showDemoLoadedPopup = false;
  demoDialog.SetDemoRoot("examples/demo");

  //vtkViewer2.getRenderer()->SetBackground(0, 0, 0); // Black background

  vtkViewer2.getRenderer()->SetBackground(0.2,0.2,0.4);
  vtkViewer2.getRenderer()->SetBackground2(0.8,0.8,0.8);

  Axis axis;  
  axis.setInteractor(vtkViewer2.getInteractor(), vtkViewer2.getRenderer());  
  Axis axis_res;

  vtkViewer_res.getRenderer()->SetBackground(0.2,0.2,0.4);
  vtkViewer_res.getRenderer()->SetBackground2(0.8,0.8,0.8);
  axis_res.setInteractor(vtkViewer_res.getInteractor(), vtkViewer_res.getRenderer());

  //vtkViewer2.addActor(actor);

  // Our state
  bool show_demo_window = true;
  bool show_another_window = false;
  bool vtk_2_open = true;
  bool vtk_res_open = true;
  ImVec4 clear_color = ImVec4(0.067f, 0.094f, 0.153f, 1.00f);
  

  cout << "Done. "<<endl;
  cout << "Initialize gmsh"<<endl;
  gmsh::initialize(argc, argv);
        
	//cout << "creating app app"<<endl;
	//pApp= new EditorApp();

/*
    vtkNew<vtkNamedColors> colors;
    vtkNew<vtkArrowSource> arrowSource;
    arrowSource->Update();
    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(arrowSource->GetOutputPort());
    vtkNew<vtkActor> arrow_actor;
    vtkViewer2.addActor(arrow_actor);
    
    
    actor->SetMapper(mapper);
*/    


    //vtkViewer2.addActor(axis.actor);


/*    
    vtkOCCTGeom geom;
    geom.TestReader("valoppi_z_3.step", vtkOCCTReader::Format::STEP);
    //widget->SetInteractor(renderWindowInteractor);
    vtkViewer2.addActor(geom.actor);
*/


      GLuint my_image_texture = 0;
      int my_image_width = 0;
      int my_image_height = 0;
      cout << "Opening button images.."<<endl;
      bool ret = LoadTextureFromFile("buttons/xy.png", &my_image_texture, &my_image_width, &my_image_height);
      IM_ASSERT(ret);
      cout << "Done."<<endl;
          

  // Main loop
  #ifdef BUILD_PYTHON
  Py_Initialize();
  addPythonSearchPath(std::filesystem::current_path());
  if (argc > 0 && argv[0] != nullptr) {
    addPythonSearchPath(std::filesystem::absolute(argv[0]).parent_path());
  }
  #endif
  
  App::initApp(); //singleton
  ///AFTER APP INITIALIZATIO
  cout << "Creating Editor"<<endl;
  Editor* editor = new Editor();//THIS RELIES ON THE App Singleton!! ALWAIS GENERATE IT FIRST AND THE CALL EDITOR, otherwise crashes
  editor->addViewer(&vtkViewer2);  
  editor->addResViewer(&vtkViewer_res);  
  cout << "Done "<<endl;
  //getApp().setActiveModel(m_model);
  
          //~ std::string filename = "out_0.000010.vtk";
      //~ ResultFrame *frame = new ResultFrame(filename);
    //~ frame->printAvailableFields();
    
    //~ // Mostrar el campo DISP (magnitud del vector)
    //~ frame->setActiveScalarField("DISP");
          
      //~ vtkViewer2.addActor(frame->actor);


      
  #ifdef BUILD_PYTHON
  //PyRun_SimpleString("from model import *");
  #endif
  while (!glfwWindowShouldClose(window))
  {
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    glfwPollEvents();

    if (font_rebuild_requested) {
      ui_font_size = pending_ui_font_size;
      rebuildUiFonts();
      ImGui_ImplOpenGL3_DestroyFontsTexture();
      ImGui_ImplOpenGL3_CreateFontsTexture();
      font_rebuild_requested = false;
    }

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

    if (ImGui::BeginMainMenuBar()) {
      if (ImGui::BeginMenu("File")) {
        ImGui::MenuItem("Project actions are available in the left panel", nullptr, false, false);
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Edit")) {
        ImGui::MenuItem("Editing tools are available in the left panel", nullptr, false, false);
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("View")) {
        if (ImGui::BeginMenu("Font")) {
          const bool use_ubuntu = current_font_choice == UIFontChoice::Ubuntu;
          if (ImGui::MenuItem("Ubuntu", nullptr, use_ubuntu, ubuntu_font_available)) {
            current_font_choice = UIFontChoice::Ubuntu;
            current_ui_font = font_ubu;
            io.FontDefault = current_ui_font;
          }
          if (ImGui::MenuItem("ImGui Default", nullptr, current_font_choice == UIFontChoice::ImGuiDefault)) {
            current_font_choice = UIFontChoice::ImGuiDefault;
            current_ui_font = font1;
            io.FontDefault = current_ui_font;
          }
          ImGui::Separator();
          float new_font_size = pending_ui_font_size;
          if (ImGui::SliderFloat("Size", &new_font_size, 16.0f, 20.0f, "%.1f px")) {
            pending_ui_font_size = new_font_size;
            font_rebuild_requested = true;
          }
          ImGui::EndMenu();
        }
        ImGui::EndMenu();
      }
      ImGui::EndMainMenuBar();
    }
    
    editor->drawGui();
    
    editor->processInput(window); //KEYBOARD, good for maintain pressed
    
    /*
    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).

    if (show_demo_window){
      ImGui::ShowDemoWindow(&show_demo_window);
    }
    */

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
    {
      static float f = 0.0f;
      static int counter = 0;

      ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.
      ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
      ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
      ImGui::Checkbox("Another Window", &show_another_window);
      ImGui::Checkbox("VTK Model  Viewer", &vtk_2_open);
      ImGui::Checkbox("VTK Result Viewer", &vtk_res_open);

      ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
      ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

      if (ImGui::Button("Button")){                            // Buttons return true when clicked (most widgets return true when edited/activated)
        counter++;
      }
      ImGui::SameLine();
      ImGui::Text("counter = %d", counter);

      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    }
    ImGui::End();

    // 3. Show another simple window.
    if (show_another_window){
      ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
      ImGui::Text("Hello from another window!");
      if (ImGui::Button("Close Me")){
        show_another_window = false;
      }
      ImGui::End();
    }
    
    //vtkArrowSource *arrowSource = vtkNew<vtkArrowSource>;
    
    // 4. Show a simple VtkViewer Instance (Always Open)
    ImGui::SetNextWindowSize(ImVec2(360, 240), ImGuiCond_FirstUseEver);
    
    //ImGui::Begin("Vtk Viewer 1", nullptr, VtkViewer::NoScrollFlags());
    //vtkViewer1.render(); // default render size = ImGui::GetContentRegionAvail()
    //ImGui::End();

    // 5. Show a more complex VtkViewer Instance (Closable, Widgets in Window)
    //ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(400, 200),ImGuiCond_FirstUseEver);
    
    ImGui::Begin("VTK Viewers"); // ventana padre para los tabs

      if (ImGui::BeginTabBar("##ViewersTabBar", ImGuiTabBarFlags_None))
      {
          bool activate_results_tab = editor->consumeResultsViewerActivationRequest();
          if (activate_results_tab) {
              vtk_res_open = true;
          }

          // ================= TAB: Modelo =================
	          if (vtk_2_open && ImGui::BeginTabItem("Model Viewer", &vtk_2_open))
          {
	              ImGui::PushFont(current_ui_font);

	              auto renderer = vtkViewer2.getRenderer();
                static ModelViewportOverlayState modelOverlayState;

	              if (ImGui::Button("Demo")) {
                  showDemoDialog = true;
	                // const std::filesystem::path demo_path("./demo/demo.wfmodel");
	                // if (std::filesystem::exists(demo_path))
	                  // editor->openModelFromPath(demo_path.string());
	                // else
	                  // std::cout << "Demo file not found: " << demo_path.string() << std::endl;
	              }
	              ImGui::SameLine();
	              if (ImGui::Button("Run")) {
	                editor->createJobFromActiveModel(true);
	              }
	              ImGui::SameLine();
	              if (ImGui::Button("Zoom all")) {
	                vtkViewer2.resetCamera();
	              }

	              // Botones de background específicos
	              //~ if (ImGui::Button("Black BG"))        renderer->SetBackground(0,0,0);
	              //~ ImGui::SameLine();
              //~ if (ImGui::Button("Red BG"))          renderer->SetBackground(1,0,0);
              //~ ImGui::SameLine();
              //~ if (ImGui::Button("Green BG"))        renderer->SetBackground(0,1,0);
              //~ ImGui::SameLine();
              //~ if (ImGui::Button("Blue BG"))         { renderer->SetBackground(0.2,0.2,0.4); renderer->SetBackground2(0.8,0.8,0.8); }

              // Slider de alpha del background
              static float vtk2BkgAlpha = 0.2f;
              ImGui::SliderFloat("BG Alpha", &vtk2BkgAlpha, 0.0f, 1.0f);
              renderer->SetBackgroundAlpha(vtk2BkgAlpha);

              applyDisplayModeToActiveModel(modelOverlayState.displayMode, modelOverlayState.showEdges);
              // Render del viewer
              vtkViewer2.render();
              drawViewportOverlay(vtkViewer2, modelOverlayState, "##ModelViewportOverlay", true);
              editor->handleSelectionInteraction();
              editor->drawSelectionOverlay();

              ImGui::PopFont();
              ImGui::EndTabItem();
          }

          // ================= TAB: Resultados =================
          ImGuiTabItemFlags results_tab_flags = activate_results_tab ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
          if (vtk_res_open && ImGui::BeginTabItem("Results Viewer", &vtk_res_open, results_tab_flags))
          {
	              ImGui::PushFont(current_ui_font);

	              auto renderer = vtkViewer_res.getRenderer();
                static ModelViewportOverlayState resultsOverlayState;
	              static int currentFrame = 0;
	              static int lastFrame = -1;
	              static double globalMin = 0.0;
	              static double globalMax = 1.0;
	              static bool manualColorScale = false;
	              static double manualMin = 0.0;
	              static double manualMax = 1.0;
	              
	              static bool isCellField = false;
	                            
	              static std::string activeFieldName = "";
	              static vtkSmartPointer<vtkScalarBarActor> currentScalarBar = nullptr;
	              static int activeFieldComponents = 1;
	              static int selectedFieldComponent = -2;
	              auto getActiveArray = [&](ResultFrame& resultFrame) -> vtkDataArray* {
	                  if (activeFieldName.empty() || !resultFrame.mesh)
	                      return nullptr;
	                  return isCellField
	                      ? resultFrame.mesh->GetCellData()->GetArray(activeFieldName.c_str())
	                      : resultFrame.mesh->GetPointData()->GetArray(activeFieldName.c_str());
	              };
	              auto recomputeActiveFieldScale = [&]() {
	                  if (manualColorScale) {
	                      globalMin = manualMin;
	                      globalMax = manualMax;
	                      return;
	                  }
	                  if (!editor->getResults() || activeFieldName.empty())
	                      return;

	                  globalMin = 1.0e10;
	                  globalMax = -1.0e10;

	                  for (auto& f : editor->getResults()->frames) {
	                      if (!f || !f->mesh)
	                          continue;

	                      vtkDataArray* array = getActiveArray(*f);

	                      if (!array)
	                          continue;

	                      double range[2];
	                      if (selectedFieldComponent >= 0 &&
	                          selectedFieldComponent < array->GetNumberOfComponents())
	                          array->GetRange(range, selectedFieldComponent);
	                      else if (array->GetNumberOfComponents() == 3 && selectedFieldComponent == 3)
	                          array->GetRange(range, -1);
	                      else
	                          array->GetRange(range);

	                      globalMin = (std::min)(globalMin, range[0]);
	                      globalMax = (std::max)(globalMax, range[1]);
	                  }

	                  if (globalMin > globalMax) {
	                      globalMin = 0.0;
	                      globalMax = 1.0;
	                  }
	                  manualMin = globalMin;
	                  manualMax = globalMax;
	              };
	              auto applyActiveFieldSelection = [&](ResultFrame& resultFrame) {
	                  if (activeFieldName.empty())
	                      return;

	                  auto mapper = resultFrame.actor->GetMapper();
	                  vtkDataArray* array = getActiveArray(resultFrame);
	                  if (!mapper || !array)
	                      return;

	                  if (array->GetNumberOfComponents() == 3 && selectedFieldComponent == 3) {
	                      resultFrame.setActiveScalarField(activeFieldName);
	                      resultFrame.updateScalarBar(activeFieldName, globalMin, globalMax);
	                      return;
	                  }

	                  if (isCellField)
	                      mapper->SetScalarModeToUseCellFieldData();
	                  else
	                      mapper->SetScalarModeToUsePointFieldData();

	                  mapper->SelectColorArray(activeFieldName.c_str());
	                  if (selectedFieldComponent >= 0 &&
	                      selectedFieldComponent < array->GetNumberOfComponents())
	                      mapper->SetArrayComponent(selectedFieldComponent);

	                  mapper->SetScalarRange(globalMin, globalMax);
	                  mapper->ScalarVisibilityOn();
	                  mapper->Update();
	                  resultFrame.updateScalarBar(activeFieldName, globalMin, globalMax);
	              };
	              auto applyActiveFieldSelectionToAllFrames = [&]() {
	                  if (!editor->getResults() || activeFieldName.empty())
	                      return;
	                  for (auto& resultFrame : editor->getResults()->frames) {
	                      if (resultFrame)
	                          applyActiveFieldSelection(*resultFrame);
	                  }
	              };

	              // Botones de background específicos
	              //~ if (ImGui::Button("Black BG"))        renderer->SetBackground(0,0,0);
	              //~ ImGui::SameLine();
	              //~ if (ImGui::Button("Red BG"))          renderer->SetBackground(1,0,0);
	              //~ ImGui::SameLine();
	              //~ if (ImGui::Button("Green BG"))        renderer->SetBackground(0,1,0);
	              //~ ImGui::SameLine();
	              //~ if (ImGui::Button("Blue BG"))         { renderer->SetBackground(0.2,0.2,0.4); renderer->SetBackground2(0.8,0.8,0.8); }
	              //~ ImGui::SameLine();
	              if (ImGui::Button("Refresh Results")) {
	                  int previousFrame = currentFrame;
	                  if (editor->refreshOpenResults()) {
	                      if (editor->getResults() && !editor->getResults()->frames.empty()) {
	                          currentFrame = std::min(previousFrame, (int)editor->getResults()->frames.size() - 1);
	                          recomputeActiveFieldScale();
	                          applyActiveFieldSelectionToAllFrames();
	                      } else {
	                          currentFrame = 0;
	                      }
	                      lastFrame = -1;
	                  }
	              }
	              ImGui::SameLine();
	              if (ImGui::Button("load plot")) {
	                  std::filesystem::path csv_path;
	                  if (editor->getResults()) {
	                      if (!editor->getResults()->sourceDirectory.empty()) {
	                          csv_path = editor->getResults()->sourceDirectory / "Contact_Forces.csv";
	                      } else if (!editor->getResults()->frames.empty()) {
	                          csv_path = std::filesystem::path(editor->getResults()->frames.front()->name).parent_path() / "Contact_Forces.csv";
	                      }
	                  }
	                  loadPlotDialog.SetCsvPath(csv_path.string());
	                  showLoadPlotDialog = true;
	              }
		              if (editor->getResults()){
	              if (!editor->getResults()->frames.empty()) {
	                  if (currentFrame >= (int)editor->getResults()->frames.size())
	                      currentFrame = 0;
	                  ImGui::SliderInt("Frame", &currentFrame, 0, (int)editor->getResults()->frames.size() - 1);
	                  auto& frame = *editor->getResults()->frames[currentFrame];  // referencia al frame actual
	                  if (frame.getScalarBarActor() && currentScalarBar != frame.getScalarBarActor()) {
	                      if (currentScalarBar)
	                          renderer->RemoveActor2D(currentScalarBar);
	                      currentScalarBar = frame.getScalarBarActor();
	                      renderer->AddActor2D(currentScalarBar);
	                  }

	                  if (currentFrame != lastFrame) {              // Solo si cambió el frame
	                      vtkCamera* camera = renderer->GetActiveCamera();
	                      bool preserveView = (lastFrame >= 0 && camera != nullptr);
	                      double cameraPosition[3] = {0.0, 0.0, 1.0};
	                      double cameraFocalPoint[3] = {0.0, 0.0, 0.0};
	                      double cameraViewUp[3] = {0.0, 1.0, 0.0};
	                      double cameraClippingRange[2] = {0.1, 1000.0};
	                      double cameraParallelScale = 1.0;
	                      double cameraViewAngle = 30.0;
	                      int cameraParallelProjection = 0;
	                      if (preserveView) {
	                          camera->GetPosition(cameraPosition);
	                          camera->GetFocalPoint(cameraFocalPoint);
	                          camera->GetViewUp(cameraViewUp);
	                          camera->GetClippingRange(cameraClippingRange);
	                          cameraParallelScale = camera->GetParallelScale();
	                          cameraViewAngle = camera->GetViewAngle();
	                          cameraParallelProjection = camera->GetParallelProjection();
	                      }

	                      vtkViewer_res.setActor(editor->getResults()->frames[currentFrame]->actor);
	                      if (preserveView) {
	                          camera->SetPosition(cameraPosition);
	                          camera->SetFocalPoint(cameraFocalPoint);
	                          camera->SetViewUp(cameraViewUp);
	                          camera->SetClippingRange(cameraClippingRange);
	                          camera->SetParallelScale(cameraParallelScale);
	                          camera->SetViewAngle(cameraViewAngle);
	                          camera->SetParallelProjection(cameraParallelProjection);
	                          renderer->ResetCameraClippingRange();
	                      } else {
	                          renderer->ResetCamera();
	                      }
	                      if (!activeFieldName.empty()) {
	                          applyActiveFieldSelection(*editor->getResults()->frames[currentFrame]);
	                      }
                        applyDisplayModeToActor(editor->getResults()->frames[currentFrame]->actor,
                                                resultsOverlayState.displayMode, resultsOverlayState.showEdges, true);
	                      
	                      lastFrame = currentFrame;                // Actualizamos el frame anterior
                      //vtkViewer_res.render();
	                  }

	                  auto fieldNames = frame.getAvailableFieldNames();

                  static int selectedField = 0;
                  //~ if (!fieldNames.empty()) {
                      //~ // Crear array de const char* para ImGui
                      //~ std::vector<const char*> fieldCStrs;
                      //~ for (auto& s : fieldNames) fieldCStrs.push_back(s.c_str());

                      //~ ImGui::Text("Active Field:");
                      //~ if (ImGui::Combo("##FieldSelector", &selectedField, fieldCStrs.data(), fieldCStrs.size())) {
                          //~ // cuando cambia la selección
                          //~ frame.setActiveScalarField(fieldNames[selectedField]);
                          //~ frame.actor->GetMapper()->Update();
                          //~ vtkViewer_res.render();
                      //~ }
                  //~ } else {
                      //~ ImGui::Text("No fields available");
                  //~ }


	                    if (!fieldNames.empty()) {
	                        std::vector<const char*> fieldCStrs;
	                        for (auto& s : fieldNames) fieldCStrs.push_back(s.c_str());

                        ImGui::Text("Active Field:");
                        if (ImGui::Combo("##FieldSelector", &selectedField, fieldCStrs.data(), fieldCStrs.size())) {

                            std::string selected = fieldNames[selectedField];
                            isCellField = (selected.rfind("[C]", 0) == 0);
                            activeFieldName = selected.substr(4); // remueve "[C] " o "[P] "
                            vtkDataArray* selectedArray = getActiveArray(frame);
                            activeFieldComponents = selectedArray ? selectedArray->GetNumberOfComponents() : 1;
	                            selectedFieldComponent = (activeFieldComponents == 3) ? 3 :
	                                                     (activeFieldComponents > 1 ? 0 : -1);

	                            recomputeActiveFieldScale();
                            cout << "Setting range in "<<globalMin <<", "<<globalMax<<endl;
	                            applyActiveFieldSelectionToAllFrames();
                              applyDisplayModeToResults(resultsOverlayState.displayMode, resultsOverlayState.showEdges, editor);

	                            vtkViewer_res.render();
	                        }
	                    } else {
	                        ImGui::Text("No fields available");
	                    }

	                        vtkDataArray* activeArray = getActiveArray(frame);
	                        if (activeArray)
	                            activeFieldComponents = activeArray->GetNumberOfComponents();
	                    if (!activeFieldName.empty() && activeFieldComponents > 1) {
	                        ImGui::Separator();
	                        ImGui::Text("Component");
	                        int buttonCount = activeFieldComponents;
	                        bool vectorMagnitudeButton = (activeFieldComponents == 3);
	                        if (vectorMagnitudeButton)
	                            buttonCount = 4;

	                        for (int comp = 0; comp < buttonCount; ++comp) {
	                            std::string label = std::to_string(comp);
	                            bool isSelected = (selectedFieldComponent == comp);
	                            if (isSelected)
	                                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

	                            if (ImGui::Button(label.c_str())) {
	                                selectedFieldComponent = comp;
	                                recomputeActiveFieldScale();
	                                applyActiveFieldSelectionToAllFrames();
	                                vtkViewer_res.render();
	                            }

	                            if (isSelected)
	                                ImGui::PopStyleColor();

	                            if (comp + 1 < buttonCount)
	                                ImGui::SameLine();
	                        }
	                    }

	                    if (!activeFieldName.empty()) {
	                        ImGui::Separator();
	                        ImGui::Text("Color Bar");
	                        bool autoScale = !manualColorScale;
	                        if (ImGui::RadioButton("Auto", autoScale)) {
	                            manualColorScale = false;
	                            recomputeActiveFieldScale();
	                            applyActiveFieldSelectionToAllFrames();
	                        }
	                        ImGui::SameLine();
	                        if (ImGui::RadioButton("Manual", manualColorScale)) {
	                            manualColorScale = true;
	                            manualMin = globalMin;
	                            manualMax = globalMax;
	                        }
	                        if (manualColorScale) {
	                            bool changed = false;
	                            changed |= ImGui::InputDouble("Min##ManualColorScale", &manualMin, 0.0, 0.0, "%.6g");
	                            changed |= ImGui::InputDouble("Max##ManualColorScale", &manualMax, 0.0, 0.0, "%.6g");
	                            if (manualMax < manualMin)
	                                manualMax = manualMin;
	                            if (changed) {
	                                recomputeActiveFieldScale();
	                                applyActiveFieldSelectionToAllFrames();
	                            }
	                        }
	                        ImGui::Text("Variable: %s", activeFieldName.c_str());
	                        ImGui::Text("Min: %.6g", globalMin);
	                        ImGui::Text("Max: %.6g", globalMax);
	                    }
	                                  
	              }
	            }
              
              // Slider de alpha del background
              static float vtkResBkgAlpha = 0.2f;
              ImGui::SliderFloat("BG Alpha", &vtkResBkgAlpha, 0.0f, 1.0f);
              renderer->SetBackgroundAlpha(vtkResBkgAlpha);

              applyDisplayModeToResults(resultsOverlayState.displayMode, resultsOverlayState.showEdges, editor);
              // Render del viewer
              vtkViewer_res.render();
              drawViewportOverlay(vtkViewer_res, resultsOverlayState, "##ResultsViewportOverlay", false);

              ImGui::PopFont();
              ImGui::EndTabItem();
          }

            ImGui::EndTabBar();
        }

        ImGui::End(); // cierre de la ventana contenedora

    loadPlotDialog.Draw("Load Plot", &showLoadPlotDialog);
    demoDialog.Draw("Available Demos", &showDemoDialog);
    std::string selectedDemoFolder = demoDialog.ConsumeSelectedDemoPath();

    if (!selectedDemoFolder.empty()) {
        std::filesystem::path demoModel =
            std::filesystem::path(selectedDemoFolder) / "demo.wfmodel";

	      std::cout << "Opening model: " << demoModel.string() << std::endl;
        if (std::filesystem::exists(demoModel)) {
            editor->openModelFromPath(demoModel.string());
            showDemoLoadedPopup = true;

            vtkViewer2.getRenderer()->ResetCamera();
            vtkViewer2.getRenderer()->ResetCameraClippingRange();
        } else {
            std::cout << "Demo model not found: "
                      << demoModel.string() << std::endl;
        }
    }

    if (showDemoLoadedPopup) {
        ImGui::OpenPopup("Demo Loaded");
        showDemoLoadedPopup = false;
    }

    ImGui::SetNextWindowSize(ImVec2(420.0f, 0.0f), ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(),
                            ImGuiCond_Appearing,
                            ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Demo Loaded", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize |
                               ImGuiWindowFlags_NoResize |
                               ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::TextWrapped("Demo loaded. Click on \"Run\" and then \"Load Results\".");
        ImGui::Spacing();
        if (ImGui::Button("OK", ImVec2(120.0f, 0.0f))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    const bool blockViewerInteraction =
        editor->hasBlockingDialogOpen() ||
        showLoadPlotDialog ||
        showDemoDialog ||
        ImGui::IsPopupOpen("Demo Loaded");
    vtkViewer2.setInputEnabled(!blockViewerInteraction);
    vtkViewer_res.setInputEnabled(!blockViewerInteraction);
    
    getApp().checkUpdate(); //To new Graphics Meshed and so on
    for (vtkSmartPointer<vtkProp>& actor : getApp().getPendingActorRemovals()) {
      if (actor != nullptr) {
        std::cout << "[main] removing pending actor " << actor.GetPointer()
                  << " class=" << actor->GetClassName() << std::endl;
        vtkViewer2.removeActor(actor);
      } else {
        std::cout << "[main] pending actor is null" << std::endl;
      }
    }
    getApp().clearPendingActorRemovals();

    for (int gm=0;gm<getApp().getGraphicMeshCount();gm++) {
      //cout << "is actor needed for mesh "<<gm<<": "<<getApp().getGraphicMesh(gm)->isActorNeeded()<<endl;
        if (getApp().getGraphicMesh(gm)->isActorNeeded()){
        vtkSmartPointer<vtkActor> act = getApp().getGraphicMesh(gm)->getActor();
        if (act != nullptr)
          std::cout << "Actor class: " << act->GetClassName() << std::endl;
        else
          std::cout << "Null actor pointer!" << std::endl;
          cout << "Adding Actor"<<endl;
          if (getApp().getGraphicMesh(gm)->getActor() != nullptr)
            vtkViewer2.addActor(getApp().getGraphicMesh(gm)->getActor());
          else 
            cout <<"ERROR:Null mesh ptr"<<endl;
          cout << "added "<<endl;
          getApp().getGraphicMesh(gm)->setActorNeeded(false); //CHANGE THIS TO SOMEHOW CONTAIN THE RENDERER
          
        }
      //cout << "graphi mesh count "<<getApp().getGraphicMeshCount()<<endl;
    }

    std::unordered_map<Geom*, vtkOCCTGeom*>& geomMap = getApp().getGeomToVisual();
    for (std::unordered_map<Geom*, vtkOCCTGeom*>::iterator it = geomMap.begin(); it != geomMap.end(); ++it) {
        Geom* geom = it->first;
        vtkOCCTGeom* visual = it->second;

        if (!visual->isRendered && visual->actor) {
            vtkViewer2.addActor(visual->actor);
            visual->isRendered = true;
        }
    }
    ImGui::Render();

    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Update and Render additional Platform Windows
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }

    glfwSwapBuffers(window);
  
  
    
  }//main while

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImPlot::DestroyContext();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
