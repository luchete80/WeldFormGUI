
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
#include "LSDynaWriter.h"

//#include "SceneView.h"

//#include "ViewportWindow.h"
#include "Job.h"

#include<iostream>
#include <thread>

#include <gmsh.h>

#include "geom/vtkOCCTGeom.h"
#include "VtkViewer.h"

#include "SPHModel.h"


#include "Part.h"
#include "GraphicMesh.h"

#include "console.h"

#include "App.h"

using namespace std;
//glm::mat4 trans_mat[1000]; //test


Editor *editor; //TODO: IMPLEMENT CALLBACK CLASS IN EDITOR

float zcam;

static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
static void window_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

ImVec2 vpos, vmin, vmax;

template <typename T>
std::string to_string_with_precision(const T a_value, const int n = 6)
{
    std::ostringstream out;
    out.precision(n);
    out << std::fixed << a_value;
    return out.str();
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
void ShowExampleMenuFile(const Editor &editor)
{
    //IMGUI_DEMO_MARKER("Examples/Menu");
    ImGui::MenuItem("(demo menu)", NULL, false, false);
    if (ImGui::MenuItem("New")) {}
    if (ImGui::MenuItem("Open", "Ctrl+O")) {
        // ////// open Dialog Simple
  // if (ImGui::Button("Open File Dialog"))
      ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".dae,.obj,.str", ".");
    }
    if (ImGui::MenuItem("Import", "Ctrl+O")){
      ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgImport", "Choose File", ".step|.STEP|.stp|.STP|.json|.k", ".");
    }
    if (ImGui::MenuItem("Export LS-Dyna", "Ctrl+S")){
      ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgExport", "Choose File", ".k", ".");
    }
    if (ImGui::BeginMenu("Open Recent"))
    {
        ImGui::MenuItem("fish_hat.c");
        ImGui::MenuItem("fish_hat.inl");
        ImGui::MenuItem("fish_hat.h");
        if (ImGui::BeginMenu("More.."))
        {
            ImGui::MenuItem("Hello");
            ImGui::MenuItem("Sailor");
            if (ImGui::BeginMenu("Recurse.."))
            {
                ShowExampleMenuFile(editor);
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenu();
    }
    //If open
    if (ImGui::MenuItem("Write JSON Input", "Ctrl+J")) {
      //InputWriter writer("Input.json",editor.getDomain());
      }
    if (ImGui::MenuItem("Save", "Ctrl+S")) {}
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
            ShowExampleMenuFile(*editor);
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

  ImGui::Begin("Hello, world!"); 
  // Menu Bar
  if (ImGui::BeginMenuBar())
  {
      if (ImGui::BeginMenu("Menu"))
      {
          //IMGUI_DEMO_MARKER("Menu/File");
          ShowExampleMenuFile(*this);
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

// //IMGUI_DEMO_MARKER("Configuration");
    if (ImGui::CollapsingHeader("Configuration"))
    {
        ImGuiIO& io = ImGui::GetIO();

        if (ImGui::TreeNode("Configuration##2"))
        {
            ImGui::CheckboxFlags("io.ConfigFlags: NavEnableKeyboard",    &io.ConfigFlags, ImGuiConfigFlags_NavEnableKeyboard);
            ImGui::SameLine(); HelpMarker("Enable keyboard controls.");
            ImGui::CheckboxFlags("io.ConfigFlags: NavEnableGamepad",     &io.ConfigFlags, ImGuiConfigFlags_NavEnableGamepad);
            ImGui::SameLine(); HelpMarker("Enable gamepad controls. Require backend to set io.BackendFlags |= ImGuiBackendFlags_HasGamepad.\n\nRead instructions in imgui.cpp for details.");
            ImGui::CheckboxFlags("io.ConfigFlags: NavEnableSetMousePos", &io.ConfigFlags, ImGuiConfigFlags_NavEnableSetMousePos);
            ImGui::SameLine(); HelpMarker("Instruct navigation to move the mouse cursor. See comment for ImGuiConfigFlags_NavEnableSetMousePos.");
            ImGui::CheckboxFlags("io.ConfigFlags: NoMouse",              &io.ConfigFlags, ImGuiConfigFlags_NoMouse);
            if (io.ConfigFlags & ImGuiConfigFlags_NoMouse)
            {
                // The "NoMouse" option can get us stuck with a disabled mouse! Let's provide an alternative way to fix it:
                if (fmodf((float)ImGui::GetTime(), 0.40f) < 0.20f)
                {
                    ImGui::SameLine();
                    ImGui::Text("<<PRESS SPACE TO DISABLE>>");
                }
                if (ImGui::IsKeyPressed(ImGuiKey_Space))
                    io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
            }
            ImGui::CheckboxFlags("io.ConfigFlags: NoMouseCursorChange", &io.ConfigFlags, ImGuiConfigFlags_NoMouseCursorChange);
            ImGui::SameLine(); HelpMarker("Instruct backend to not alter mouse cursor shape and visibility.");
            ImGui::Checkbox("io.ConfigInputTrickleEventQueue", &io.ConfigInputTrickleEventQueue);
            ImGui::SameLine(); HelpMarker("Enable input queue trickling: some types of events submitted during the same frame (e.g. button down + up) will be spread over multiple frames, improving interactions with low framerates.");
            ImGui::Checkbox("io.ConfigInputTextCursorBlink", &io.ConfigInputTextCursorBlink);
            ImGui::SameLine(); HelpMarker("Enable blinking cursor (optional as some users consider it to be distracting).");
            ImGui::Checkbox("io.ConfigDragClickToInputText", &io.ConfigDragClickToInputText);
            ImGui::SameLine(); HelpMarker("Enable turning DragXXX widgets into text input with a simple mouse click-release (without moving).");
            ImGui::Checkbox("io.ConfigWindowsResizeFromEdges", &io.ConfigWindowsResizeFromEdges);
            ImGui::SameLine(); HelpMarker("Enable resizing of windows from their edges and from the lower-left corner.\nThis requires (io.BackendFlags & ImGuiBackendFlags_HasMouseCursors) because it needs mouse cursor feedback.");
            ImGui::Checkbox("io.ConfigWindowsMoveFromTitleBarOnly", &io.ConfigWindowsMoveFromTitleBarOnly);
            ImGui::Checkbox("io.MouseDrawCursor", &io.MouseDrawCursor);
            ImGui::SameLine(); HelpMarker("Instruct Dear ImGui to render a mouse cursor itself. Note that a mouse cursor rendered via your application GPU rendering path will feel more laggy than hardware cursor, but will be more in sync with your other visuals.\n\nSome desktop applications may use both kinds of cursors (e.g. enable software cursor only when resizing/dragging something).");
            ImGui::Text("Also see Style->Rendering for rendering options.");
            ImGui::TreePop();
            ImGui::Separator();
        }

        //IMGUI_DEMO_MARKER("Configuration/Backend Flags");
        if (ImGui::TreeNode("Backend Flags"))
        {
            HelpMarker(
                "Those flags are set by the backends (imgui_impl_xxx files) to specify their capabilities.\n"
                "Here we expose them as read-only fields to avoid breaking interactions with your backend.");

            // Make a local copy to avoid modifying actual backend flags.
            // FIXME: We don't use BeginDisabled() to keep label bright, maybe we need a BeginReadonly() equivalent..
            ImGuiBackendFlags backend_flags = io.BackendFlags;
            ImGui::CheckboxFlags("io.BackendFlags: HasGamepad",           &backend_flags, ImGuiBackendFlags_HasGamepad);
            ImGui::CheckboxFlags("io.BackendFlags: HasMouseCursors",      &backend_flags, ImGuiBackendFlags_HasMouseCursors);
            ImGui::CheckboxFlags("io.BackendFlags: HasSetMousePos",       &backend_flags, ImGuiBackendFlags_HasSetMousePos);
            ImGui::CheckboxFlags("io.BackendFlags: RendererHasVtxOffset", &backend_flags, ImGuiBackendFlags_RendererHasVtxOffset);
            ImGui::TreePop();
            ImGui::Separator();
        }

        //IMGUI_DEMO_MARKER("Configuration/Style");
        if (ImGui::TreeNode("Style"))
        {
            HelpMarker("The same contents can be accessed in 'Tools->Style Editor' or by calling the ShowStyleEditor() function.");
            //ImGui::ShowStyleEditor();
            ImGui::TreePop();
            ImGui::Separator();
        }

        //IMGUI_DEMO_MARKER("Configuration/Capture, Logging");
        if (ImGui::TreeNode("Capture/Logging"))
        {
            HelpMarker(
                "The logging API redirects all text output so you can easily capture the content of "
                "a window or a block. Tree nodes can be automatically expanded.\n"
                "Try opening any of the contents below in this window and then click one of the \"Log To\" button.");
            ImGui::LogButtons();

            HelpMarker("You can also call ImGui::LogText() to output directly to the log without a visual output.");
            //~ if (ImGui::Button("Copy \"Hello, world!\" to clipboard"))
            //~ {
                //~ ImGui::LogToClipboard();
                //~ ImGui::LogText("Hello, world!");
                //~ ImGui::LogFinish();
            //~ }
            ImGui::TreePop();
        }
    }

    //if (ImGui::CollapsingHeader("New Domain")){
    // //IMGUI_DEMO_MARKER("Widgets/Trees");
    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    if (ImGui::BeginTabBar("TABS", tab_bar_flags)){
    if (ImGui::BeginTabItem("Model")) { 
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode("Model"))
    {

        bool open_ = ImGui::TreeNode("Parts");
        if (ImGui::BeginPopupContextItem())
        {
          if (ImGui::MenuItem("New Geometry from file", "CTRL+Z")) {
            ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgImport", "Choose File", ".step,.STEP,.stp,.STP,.geo", ".");
            
            string test;
            //cout << "Model part count "<<m_model->getPartCount()<<endl;
          }
          if (ImGui::MenuItem("New Geometry: 2D Box", "CTRL+Z")) {
          }
          if (ImGui::MenuItem("New Mesh", "CTRL+Z")) {}
            ImGui::EndPopup();          
        }

        //cout << "Model part count "<<m_model->getPartCount()<<endl;      
        for (int i = 0; i < m_model->getPartCount(); i++)
        {
          // Use SetNextItemOpen() so set the default state of a node to be open. We could
          // also use TreeNodeEx() with the ImGuiTreeNodeFlags_DefaultOpen flag to achieve the same thing!
          if (i == 0)
            ImGui::SetNextItemOpen(true, ImGuiCond_Once);

          if (ImGui::TreeNode((void*)(intptr_t)i, "Part %d", i))
          {
            //cout << "Model part count "<<m_model->getPartCount()<<endl;
            if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered()){                
              m_show_mat_dlg_edit = true;
              selected_mat = m_mats[i];}
            if (ImGui::BeginPopupContextItem())
            {
              if (ImGui::MenuItem("Edit", "CTRL+Z")) {
                m_show_mat_dlg_edit = true;
                selected_mat = m_mats[i];
              }
              ImGui::EndPopup();
            }                    
              ImGui::SameLine();
              if (ImGui::SmallButton("button")) {}
              ImGui::TreePop();
          }
        }
      
        if (open_)
        {
           // your tree code stuff
           ImGui::TreePop();
        }
        //-----------------------
        open_ = ImGui::TreeNode("Sets");
        if (ImGui::BeginPopupContextItem())
        {
          if (ImGui::MenuItem("New", "CTRL+Z")) {
            
            m_show_set_dlg = true;
          }
          ImGui::EndPopup();
        }
        if (open_) //Expand
        {
           // your tree code stuff
           ImGui::TreePop();
        }

        open_ = ImGui::TreeNode("Materials");
        if (ImGui::BeginPopupContextItem())
        {
          if (ImGui::MenuItem("New", "CTRL+Z")) {
            m_show_mat_dlg = true;
            
          }
          ImGui::EndPopup();
        }

        for (int i = 0; i < m_mats.size(); i++)
        {
          // Use SetNextItemOpen() so set the default state of a node to be open. We could
          // also use TreeNodeEx() with the ImGuiTreeNodeFlags_DefaultOpen flag to achieve the same thing!
          if (i == 0)
            ImGui::SetNextItemOpen(true, ImGuiCond_Once);

          if (ImGui::TreeNode((void*)(intptr_t)i, "Material %d", i))
          {
            if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered()){                
              m_show_mat_dlg_edit = true;
              selected_mat = m_mats[i];}
            if (ImGui::BeginPopupContextItem())
            {
              if (ImGui::MenuItem("Edit", "CTRL+Z")) {
                m_show_mat_dlg_edit = true;
                selected_mat = m_mats[i];
              }
              ImGui::EndPopup();
            }                    
              ImGui::SameLine();
              if (ImGui::SmallButton("button")) {}
              ImGui::TreePop();
          }
        }
            
        if (open_)
        {
           // your tree code stuff
           ImGui::TreePop();
        }
        //-----------------------------------------------------
        open_ = ImGui::TreeNode("InteractionProps");
        if (ImGui::BeginPopupContextItem())
        {
          if (ImGui::MenuItem("New", "CTRL+Z")) {}
            ImGui::EndPopup();
          
        }
        if (open_)
        {
           // your tree code stuff
           ImGui::TreePop();
        }
        //-----------------------------------------------------
        open_ = ImGui::TreeNode("Boundary Conditions");
        if (ImGui::BeginPopupContextItem())
        {
          if (ImGui::MenuItem("New", "CTRL+Z")) {}
            ImGui::EndPopup();
          
        }
        if (open_)
        {
           // your tree code stuff
           ImGui::TreePop();
        }

        
   
        ImGui::TreePop();
        
    } //MODEL TREE

        bool open_ = ImGui::TreeNode("Jobs");
        if (ImGui::BeginPopupContextItem())
        {
          if (ImGui::MenuItem("New", "CTRL+Z")) 
            m_jobdlg.m_show=true;
                           
          ImGui::EndPopup();
        }
        m_jobdlg.ShowIfEnabled();
        m_jobshowdlg.ShowIfEnabled();
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
                m_jobshowdlg.m_show = true;
                //selected_mat = m_mats[i];
              }
              if (ImGui::MenuItem("Show Progress", "")) {
                m_jobshowdlg.m_job = m_jobs[i];
                m_jobs[i]->UpdateOutput();

                m_jobshowdlg.m_show = true;
                
                //selected_mat = m_mats[i];
              }
              if (ImGui::MenuItem("Run", "")) {
                m_jobs[i]->Run();
              }
              ImGui::EndPopup();
            }                    
            ImGui::SameLine();
            if (ImGui::SmallButton("button")) {}
            ImGui::TreePop();
          }//Is hovered
        }
        if (open_)
        {
           // your tree code stuff
           ImGui::TreePop();
        }
        
    ImGui::EndTabItem(); }
    if (ImGui::BeginTabItem("Results")) { 
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
    
    
    if (ImGui::CollapsingHeader("New Domain")){
      static int item_current = 0;
      {
          // Using the _simplified_ one-liner Combo() api here
          // See "Combo" section for examples of how to use the more flexible BeginCombo()/EndCombo() api.
          //IMGUI_DEMO_MARKER("Widgets/Basic/Combo");
          const char* items[] = { "Box", "Cylinder", "Plane"};
          
          ImGui::Combo("combo", &item_current, items, IM_ARRAYSIZE(items));
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
            static double d0 = 0.0;
            ImGui::InputDouble("ox ", &d0, 0.01f, 1.0f, "%.4f");
            static double d1 = 0.0;
            ImGui::InputDouble("oy ", &d1, 0.01f, 1.0f, "%.4f");
            static double d2   = 0.0;
            ImGui::InputDouble("oz ", &d2, 0.01f, 1.0f, "%.4f");
            ImGui::Text("Size");
            //Vec3_t size;
            static double size[] = {0.1,0.1,0.1};
            ImGui::InputDouble("x ", &size[0], 0.01f, 1.0f, "%.4f");
            ImGui::InputDouble("y ", &size[1], 0.01f, 1.0f, "%.4f");
            ImGui::InputDouble("z ", &size[2], 0.01f, 1.0f, "%.4f");

            static float vec4a[4] = { 0.10f, 0.20f, 0.30f, 0.44f };
            ImGui::InputFloat3("input float3", vec4a);
            
            static double radius = 0.01;
            ImGui::InputDouble("Particle Radius",&radius); 
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
              m_domain->AddBoxLength(0 ,Vec3_t ( d0 , d1,d2 ), size[0] , size[1],  size[2], radius ,rho, h, 1 , 0 , false, false );     
              calcDomainCenter();
              cout << "Domain Center: "<<m_domain_center.x<<", "<<m_domain_center.y<<", "<<m_domain_center.z<<endl;
              is_sph_mesh = true;
            }
            
            if (ImGui::Button("Create FEM")){
              m_fem_msh = new Mesh();
              m_fem_msh->addBoxLength(Vector3f(0,0,0),Vector3f(size[0],size[1],size[2]),radius);
              
              //m_renderer.addMesh(m_fem_msh);
              
              is_fem_mesh = true;
            }
    }


    
    if (ImGui::CollapsingHeader("BCs")){
      
      if (ImGui::Button("Create")){
        
      }
      
    }

  // ////// open Dialog Simple
  // if (ImGui::Button("Open File Dialog"))
    // ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".dae,.obj,.str", ".");

  // display
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
      string test;
      m_model->addGeom(new Geom(test));
      m_model->addPart(m_model->getLastGeom());
      if (m_model->isAnyMesh()){
      //m_renderer.addMesh(m_model->getPartMesh(0));
      is_fem_mesh = true;
      }
      // action
      create_new_part = true;
      
      //test 
      bool errorIfMissing;
      gmsh::model::add("t20");


      vtkOCCTGeom geom;
      geom.TestReader(filePathName, vtkOCCTReader::Format::STEP);
      //widget->SetInteractor(renderWindowInteractor);
      viewer->addActor(geom.actor);
      

        // Load a STEP file (using `importShapes' instead of `merge' allows to
      // directly retrieve the tags of the highest dimensional imported entities):
      std::vector<std::pair<int, int> > v;
     //try {
        cout << "Loading file "<<filePathName<<endl;
        gmsh::model::occ::importShapes(filePathName, v);
        cout << "Dimension: "<<gmsh::model::getDimension()<<endl;
      //} catch(...) {
      //  gmsh::logger::write("Could not load STEP file: bye!");
      //  gmsh::finalize();
        //return 0;
      //}
      
      //MergeFile(filePathName, errorIfMissing);
      gmsh::merge(filePathName);

      gmsh::model::mesh::generate(2);
      gmsh::write("test.msh");
      m_model->getPart(0)->generateMesh();
      
      //m_model->getPart(0)->genFromGmshModel()
      getApp().setActiveModel(m_model);
      PyRun_SimpleString("GetApplication().getActiveModel()");
        
      graphic_mesh = new GraphicMesh(); ///THIS READS FROM GLOBAL GMSH MODEL
      graphic_mesh->createVTKPolyData();

      viewer->addActor(graphic_mesh->getActor());


    
    }
    
    // close
    ImGuiFileDialog::Instance()->Close();
  }


  if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgExport")) 
  {

    // action if OK
    if (ImGuiFileDialog::Instance()->IsOk())
    {
      std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
      std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
      
      cout << "Exporting file "<<filePathName<<endl;
      if (is_sph_mesh){    
        // for (int p=0;p<m_domain.Particles.size();p++){    
          // float h = m_domain.Particles[0]->h/2.;
          // pn.Scale(h, h,h);  
          // Vec3_t v = m_domain.Particles[p]->x ;
        LSDynaWriter writer(m_domain, filePathName);
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
  
  bool show_app_log = true;
  //ShowExampleAppLog(&show_app_log);
  
  //ExampleAppLog logtest;
  //ShowExampleAppLog(&show_app_log, &logtest);
  //cout << "log tst "<<logtest.test<<endl;
  create_new_mat  = false;
  create_new_set  = false;
  create_new_part = false;

  Material_ mat;
  Job job;
  if (m_show_mat_dlg) {mat = ShowCreateMaterialDialog(&m_show_mat_dlg, &m_matdlg, &create_new_mat);}
  //if (m_show_job_dlg) {job = ShowCreateJobDialog(&m_show_job_dlg, &m_jobdlg, &create_new_job);}
  else if (m_show_mat_dlg_edit) {ShowEditMaterialDialog(&m_show_mat_dlg, &m_matdlg, selected_mat);}
  else if (m_show_set_dlg) {
    
    if(is_fem_mesh)
      CreateSetTypeDialog create("test", &create_new_set, &m_setdlg.set_type, m_model);
    else
      CreateSetTypeDialog create("test", &create_new_set, &m_setdlg.set_type, m_domain);
    if(is_fem_mesh){

      //mat = ShowCreateMaterialDialog(&m_show_mat_dlg, &m_setdlg, &create_new_set);
    } else if (is_sph_mesh){
      
    } //IF IS GEOMETRY??

    //cout << "SET TYPE"<<m_setdlg.set_type<<endl;
  }//show_set_mat
  if (create_new_mat) {
    m_show_mat_dlg=false;
    //SHOULD NOT BE CALLED AGAIN!!!!!!
    //cout << "temp dens" <<m_matdlg.m_density_const<<endl;
    m_mats.push_back(new Material_(mat));
    
    cout << "Material size is "<< m_mats.size()<<endl;
    cout<<"Material Created"<<endl; 
    cout << "Density:" <<m_mats[0]->getDensityConstant()<<endl;

  } else if (m_matdlg.cancel_action)     m_show_mat_dlg=false;
  
  if (m_jobdlg.create_entity){
    cout << "Creating Job "<<m_jobdlg.m_filename<<endl;
    m_jobs.push_back(new Job(m_jobdlg.m_filename)); 
    m_jobdlg.create_entity = false;
    m_jobdlg.m_show=false;
  }
    
  if (m_show_app_console) {
    static ExampleAppConsole console;
    console.Draw("Example: Console", &m_show_app_console);    
    }            //ShowExampleAppConsole(&m_show_app_console);
  
  ImGui::End();

} //GUI


static void KeyCallback(GLFWwindow* pWindow, int key, int scancode, int action, int mods) {   
    editor->Key(key, scancode, action, mods);
}

void Editor::Key(int key, int scancode, int action, int mods) {   
    //camera->OnKeyboard(key);
}

bool mouse_pressed;

//TODO: WHY ALWAYS IS STATIC?
static void CursorPosCallback(GLFWwindow* pWindow, double x, double y) {
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
  editor->scroll(xoffset, yoffset);
}

void Editor::scroll(double xoffset, double yoffset)
{
  zcam +=0.01*yoffset;
  //camera->MoveFwd(yoffset*0.1);
}


Editor::Editor(){
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
  m_show_app_console = true;
  m_model = new Model();
  
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
  
  return 1; // IF THIS IS NOT HERE CRASHES!!!!
  */
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
      
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
      cout << "Pressed A"<<endl;

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
      box_select_mode = !box_select_mode;
      cout << "Pressed S"<<endl;
      cout << "Box Select Mode ";
      if (box_select_mode ) cout << "ON "<<endl;
      else                  cout << "OFF "<<endl;

      
    }

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
  for (int p=0;p<m_domain->Particles.size();p++)    {
    m_domain_center.x += m_domain->Particles[p]->x(0);
    m_domain_center.y += m_domain->Particles[p]->x(1);
    m_domain_center.z += m_domain->Particles[p]->x(2);
  }
  m_domain_center = m_domain_center/m_domain->Particles.size();
  
}

void Editor::calcMeshCenter(){
  
  m_femsh_center = 0.0;
  //Converting from Vec3_t to Vector3f
  //SELECT IF DOMAIN IS SPH
  for (int p=0;p<m_domain->Particles.size();p++)    {
    m_femsh_center.x += m_domain->Particles[p]->x(0);
    m_femsh_center.y += m_domain->Particles[p]->x(1);
    m_femsh_center.z += m_domain->Particles[p]->x(2);
  }
  m_femsh_center = m_femsh_center/m_domain->Particles.size();
}

void Editor::addViewer(VtkViewer *v){
  viewer=v;
  
  }
