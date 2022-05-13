#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "ImGuiFileDialog.h"
#include "picking_texture.h"
#include "picking_technique.h"

#ifndef WIN32
#include "freetypeGL.h"
#endif

#include "Editor.h"
#include <sstream>

#include "graphics/sphere_low.h"

using namespace SPH;

Editor *editor; //TODO: IMPLEMENT CALLBACK CLASS IN EDITOR

static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

template <typename T>
std::string to_string_with_precision(const T a_value, const int n = 6)
{
    std::ostringstream out;
    out.precision(n);
    out << std::fixed << a_value;
    return out.str();
}


// settings

#define PITCH_WIDTH 20.
#define PITCH_LENGTH 20.

// Helper to wire demo markers located in code to a interactive browser
typedef void (*ImGuiDemoMarkerCallback)(const char* file, int line, const char* section, void* user_data);
extern ImGuiDemoMarkerCallback  GImGuiDemoMarkerCallback;
extern void*                    GImGuiDemoMarkerCallbackUserData;
ImGuiDemoMarkerCallback         GImGuiDemoMarkerCallback = NULL;
void*                           GImGuiDemoMarkerCallbackUserData = NULL;
#define IMGUI_DEMO_MARKER(section)  do { if (GImGuiDemoMarkerCallback != NULL) GImGuiDemoMarkerCallback(__FILE__, __LINE__, section, GImGuiDemoMarkerCallbackUserData); } while (0)

// Note that shortcuts are currently provided for display only
// (future version will add explicit flags to BeginMenu() to request processing shortcuts)
static void ShowExampleMenuFile()
{
    IMGUI_DEMO_MARKER("Examples/Menu");
    ImGui::MenuItem("(demo menu)", NULL, false, false);
    if (ImGui::MenuItem("New")) {}
    if (ImGui::MenuItem("Open", "Ctrl+O")) {}
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
                ShowExampleMenuFile();
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenu();
    }
    if (ImGui::MenuItem("Save", "Ctrl+S")) {}
    if (ImGui::MenuItem("Save As..")) {}

    ImGui::Separator();
    IMGUI_DEMO_MARKER("Examples/Menu/Options");
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

    IMGUI_DEMO_MARKER("Examples/Menu/Colors");
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
        IMGUI_DEMO_MARKER("Examples/Menu/Append to an existing menu");
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

void Editor::drawGui() { 


  // Menu Bar
  if (ImGui::BeginMenuBar())
  {
      if (ImGui::BeginMenu("Menu"))
      {
          IMGUI_DEMO_MARKER("Menu/File");
          ShowExampleMenuFile();
          ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Examples"))
      {
          IMGUI_DEMO_MARKER("Menu/Examples");
          //ImGui::MenuItem("Main menu bar", NULL, &show_app_main_menu_bar);
          // ImGui::MenuItem("Console", NULL, &show_app_console);
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
          // IMGUI_DEMO_MARKER("Menu/Tools");
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



  ////// open Dialog Simple
  if (ImGui::Button("Open File Dialog"))
    ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".dae,.obj,.str", ".");

  // display
  if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey")) 
  {
    // action if OK
    if (ImGuiFileDialog::Instance()->IsOk())
    {
      std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
      std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
      
      cout << "file path name "<<filePathName<<endl;

    //NODE MESHES: TEMPORARY THIS WAS OF RENDER
    for (int i=0;i<1;i++) {
      if (filePathName.find(".dae")!=string::npos){ //THIS NEEDS TO BE IMPROVED
        myMesh *mesh = new myMesh();
          if (!mesh->LoadMesh(
            filePathName.c_str()
            )) {
            std::cout<<"Mesh load failed"<<endl;
            printf("Mesh load failed\n");

          }
          else {
            m_nodemesh.push_back(mesh);
            mesh_loaded = true;
          }
      } else if (filePathName.find(".str") !=string::npos) {
        cout << "Opening structure..."<<endl;

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
        // }//For nodes
      
      }
    }
      // action
    }
    
    // close
    ImGuiFileDialog::Instance()->Close();
  }
}

const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "uniform mat4 gWVP;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = gWVP * vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    "   //gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    "}\0";
const char *fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
    "}\n\0";
    
//3D THINGS

static void KeyCallback(GLFWwindow* pWindow, int key, int scancode, int action, int mods) {   
    editor->Key(key, scancode, action, mods);
}

void Editor::Key(int key, int scancode, int action, int mods) {   
    camera->OnKeyboard(key);
}

bool mouse_pressed;

//TODO: WHY ALWAYS IS STATIC?
static void CursorPosCallback(GLFWwindow* pWindow, double x, double y) {
    
    // // //If callbacks are set AFTER, THEN THIS NEEDS TO BE CALLED
    // // //ImGui_ImplGlfw_CursorPosCallback(pWindow,x,y);//THIS IS IN ORDER 
    editor->CursorPos(x,  y);
}

void Editor:: CursorPos(double x, double y) {
     if (mouse_pressed){
      camera->SetLastMousePos(x,y);
      mouse_pressed = false;
    }
    if (rotatecam)
      camera->OnMouse((int)x, (int)y);


    
    // if (m_left_button_pressed){
      // PickingTexture::PixelInfo Pixel = m_pickingTexture.ReadPixel(x, SCR_HEIGHT - y - 1);
      // cout << "x,y: "<<x<<" , "<<y<<endl;
      // cout << "Obj ID "<<int(Pixel.ObjectID)<<", DrawID" << int(Pixel.DrawID)<<", PrimID" << int(Pixel.PrimID)<<endl;
    // }
}

static void MouseCallback(GLFWwindow* pWindow, int Button, int Action, int Mode){
  
  editor->Mouse(Button, Action, Mode);
}

void Editor::Mouse(int Button, int Action, int Mode) {
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
        PickingTexture::PixelInfo Pixel = m_pickingTexture.ReadPixel(x, SCR_HEIGHT - y - 1);
        
        if (Pixel.ObjectID != 0){
          Vector3f vel(0.,0.,0.);
          m_sel_node = Pixel.ObjectID-1;
          float dt = (float) (GetCurrentTimeMillis() - m_last_mouse_dragtime)*1000.;
          //We have to transform coordinates!
          vel = Vector3f( (x-last_mouse_x)/dt,
                          (y-last_mouse_y)/dt,
                          0.);
          //if (!m_is_node_sel) { //recently pressed, reset node velocity
            m_last_mouse_dragtime = GetCurrentTimeMillis();
            last_mouse_x = x; last_mouse_y = y;
            vel = Vector3f(0.,20.,0.);
            Vector3f f(0.,100.,0.);
            Vector3f force(0.,10,0.);

            //st->GetNode(m_sel_node)->SetVel(vel);
            //st->GetNode(m_sel_node)->SetAsConstrained();
          //}
          m_is_node_sel = true;



          cout << "Node Vel "<<vel.x<< ", "<< vel.y<< ", "<<vel.z<<endl;
          
          last_mouse_x = x;
          last_mouse_y = y;
          m_last_mouse_dragtime = GetCurrentTimeMillis();
        } else {m_is_node_sel = false;}
        cout << "x,y: "<<x<<" , "<<y<<endl;
        cout << "Obj ID "<<int(Pixel.ObjectID)<<", DrawID" << int(Pixel.DrawID)<<", PrimID" << int(Pixel.PrimID)<<endl;
        cout << "Pressed"<<endl;
      }
      if(Button == GLFW_MOUSE_BUTTON_LEFT && Action == GLFW_RELEASE){
        m_left_button_pressed = false;
      }

}

void Editor::MoveNode(){
  
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

  camera->MoveFwd(yoffset*0.1);
}


Editor::Editor(){
  //window = NULL;
  SCR_WIDTH = 800;
  SCR_HEIGHT = 600;
  is_struct = false;
}

int Editor::Init(){
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
  window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
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


  Vector3f Pos(1.0f, 3.0f, -5.0f);
  Vector3f Target(-0.5f, -0.5f, 1.0f);
  Vector3f Up(0.0, 1.0f, 0.0f);

  camera = new Camera(SCR_WIDTH, SCR_HEIGHT, Pos, Target, Up);

  Pipeline p;
  m_persProjInfo.FOV    = 60.0f;
  m_persProjInfo.Height = SCR_HEIGHT;
  m_persProjInfo.Width  = SCR_WIDTH;
  m_persProjInfo.zNear  = 1.0e-4f;
  m_persProjInfo.zFar   = 10.0f;  
  
  
  cout << "Loading ground"<<endl;
  LoadGround(&ground_mesh);
  LoadSphere();

  /// NOW LIGHT TECHNIQUE
  m_plightEffect = new BasicLightingTechnique();

  if (!m_plightEffect->Init()) {
  printf("Error initializing the lighting technique\n");
  return false;
  }

  m_plightEffect->Enable();

  
  m_directionalLight.Color = Vector3f(0.5f, 1.f, 0.0f);
  m_directionalLight.AmbientIntensity = 0.55f;
  m_directionalLight.DiffuseIntensity = 0.9f;
  m_directionalLight.Direction = Vector3f(1.0f, 0.0, 0.0);


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


  double L = 0.05;
  double H = 0.005;
  m_dx = 0.00085;
  double rho = 1.;
  double h = 1.2*m_dx;
  cout << "Generating domain "<<endl;
  m_domain.Dimension = 3;
  m_domain.AddBoxLength(0 ,Vec3_t ( -L/2.0-L/20.0 , -H, -L/2.0-L/20.0 ), L + L/10.0 + m_dx/10.0 , H ,  L + L/10. , m_dx/2.0 ,rho, h, 1 , 0 , false, false );
  cout << "Done. "<<endl;
  

  float kin_energy;

  
  mesh_loaded = false;

  if (!m_pickingTexture.Init(SCR_WIDTH, SCR_HEIGHT)) {
      return false;
  }

  if (!m_pickingEffect.Init()) {
      return false;
  }
  
  

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

  
  
  //FOR RENDER LINES
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0); 

    // remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0);   

    glFrontFace(GL_CW);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    
    
  ///////////////// TEXT ////
  m_Text = new TextRenderer(SCR_WIDTH,SCR_HEIGHT); //CRASH
  
  //Like OgldevApp
  m_frameCount = 0;
  m_frameTime = 0;
  m_fps = 0;
  
  m_is_node_sel = false;

  m_frameTime = m_startTime = GetCurrentTimeMillis();
  
  m_impact_force = 0.;
  
  return 1; // IF THIS IS NOT HERE CRASHES!!!!
}


void Editor::PickingPhase() {
  Pipeline p;

  p.SetCamera(camera->GetPos(), camera->GetTarget(), camera->GetUp());
  p.SetPerspectiveProj(m_persProjInfo);

 
 
  // render
  // ------
	glClearColor(0.0f, 0.0f, 0.f, 1.0f);  //DO NOT CHANGE THIS!!! BECAUSE OBJECT ID !=0 MEANS SOMETHING SELECTED
  glClear(GL_COLOR_BUFFER_BIT);

  // draw our first triangle
  
  //gWVPLocation = glGetUniformLocation(shaderProgram, "gWVP");
  
  m_pickingTexture.EnableWriting();
  
  
  //glUseProgram(shaderProgram);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  
  //cout << "size"<< m_nodemesh.size()<<endl;
  for (uint i = 0 ; i < m_nodemesh.size() ; i++) {
      m_pickingEffect.Enable();
      Vector3f pos(0.,0.,0.);//= st->GetNode(i)->GetPos();
      p.WorldPos(pos);
      m_pickingEffect.SetObjectIndex((i+1));
      m_pickingEffect.SetWVP(p.GetWVPTrans());    
      //Matrix4f m = p.GetWVPTrans();
      //glUniformMatrix4fv(gWVPLocation, 1, GL_TRUE, &m[0][0]);     
      m_nodemesh[i]->Render(/*&m_pickingEffect*/); //TODO: Is necessary to add this??  
  }
  m_pickingTexture.DisableWriting();

}

void Editor::RenderBeams(){
  
  // for (int n=0;n <st->GetNodeCount();n++){
    // Vector3f pos = st->GetNode(n)->GetPos();
    // vertices[3*n  ] = pos.x;
    // vertices[3*n+1] = pos.y;
    // vertices[3*n+2] = pos.z;
    
  // }
      
  Pipeline p;
  p.SetCamera(camera->GetPos(), camera->GetTarget(), camera->GetUp());
  p.SetPerspectiveProj(m_persProjInfo);
  Matrix4f m = p.GetWVPTrans();
        
  glUseProgram(shaderProgram);
  gWVPLocation = glGetUniformLocation(shaderProgram, "gWVP");
  glUniformMatrix4fv(gWVPLocation, 1, GL_TRUE, &m[0][0]);
        
  glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized

  //LUCIANO: ESTO ES NUEVO
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_DYNAMIC_DRAW);
  glUseProgram(shaderProgram);
 
  //glDrawElements(GL_LINES, 2 * st->GetTrussCount(), GL_UNSIGNED_INT, 0);
  //The first argument specifies the mode we want to draw in, similar to glDrawArrays. 
  //The second argument is the count or number of elements we'd like to draw. 
  //The third argument is the type of the indices which is of type GL_UNSIGNED_INT. 
  //The last argument allows us to specify an offset in the EBO (or pass in an index array, but that is when you're not using element buffer objects), but we're just going to leave this at 0.
  //glDrawElements(GL_TRIANGLES, 20, GL_UNSIGNED_INT, 0);
  glDrawElements(GL_TRIANGLES, 2, GL_UNSIGNED_INT, 0);
  //glDrawArrays(GL_TRIANGLES, 0, 10);

  //glEnable(GL_DEPTH_TEST);

  glBindVertexArray(0); // no need to unbind it every time 
  glUseProgram(0);


  // LUCIANO: ESTO ES NUEVO
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Editor::RenderPhase(){
    // Start the Dear ImGui frame
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  

  Pipeline p;
  /// SHAPE 
  //p.WorldPos(Pos);        
  //p.Rotate(0.,0.,0.);  
  p.SetCamera(camera->GetPos(), camera->GetTarget(), camera->GetUp());
  p.SetPerspectiveProj(m_persProjInfo);
  Matrix4f m = p.GetWVPTrans();

  
  // render
  // ------
	glClearColor(0.25f, 0.25f, 0.25f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // draw our first triangle
  glUseProgram(shaderProgram);
  gWVPLocation = glGetUniformLocation(shaderProgram, "gWVP");
  glUniformMatrix4fv(gWVPLocation, 1, GL_TRUE, &m[0][0]);
  
  
  //RENDERING FIRST IT WORKS; AFTER DOESn'T

    m_plightEffect->Enable();
    m_plightEffect->SetEyeWorldPos(camera->GetPos());
    m_plightEffect->SetWVP(p.GetWVPTrans());
    //ground_mesh.Render();
    

    //for (int p=0;p<m_domain.Particles.size();p++){
      m_plightEffect->Enable();
      Pipeline pn;
      //Vec3_t v = m_domain.Particles[p]->x;
      //Vector3f pos(v(0),v(1),v(2));
      //cout << "vert " <<v(0)<<", "<<endl;
      Vector3f pos(0.,0.,0.);
      
      pn.SetCamera(camera->GetPos(), camera->GetTarget(), camera->GetUp());
      pn.SetPerspectiveProj(m_persProjInfo);
      pn.WorldPos(pos);   
      m_plightEffect->SetEyeWorldPos(camera->GetPos());
      //pn.Scale(m_dx*0.5, m_dx*0.5, m_dx*0.5);
      pn.Scale(0.1, 0.1,0.1);
      m_plightEffect->SetWVP(pn.GetWVPTrans());   
      m_sphere_mesh.Render();
    //}

  // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
  // {
      // static float f = 0.0f;
      // static int counter = 0;

      // ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

      // ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
      // // ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
      // // ImGui::Checkbox("Another Window", &show_another_window);

      // ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
      // //ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

      // if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
          // counter++;
      // ImGui::SameLine();
      // ImGui::Text("counter = %d", counter);

      // ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      // ImGui::End();
  // }
  
  drawGui();


  //// IMGUI Rendering
  ImGui::Render();
  int display_w, display_h;
  glfwGetFramebufferSize(window, &display_w, &display_h);
  glViewport(0, 0, display_w, display_h);
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
  // // // glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
  // // // glClear(GL_COLOR_BUFFER_BIT);
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Editor::Run(){
  cout << "Run "<<endl;
  float dt = 1./60.;
  while (!glfwWindowShouldClose(window)) {
      CalcFPS();

      

      
      camera->OnRender();
      
      processInput(window);
      
      //PickingPhase();
      RenderPhase();
      RenderBeams();

      // glEnable(GL_BLEND);
      // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      // // ///
  
  
      // //"FPS: " + to_string(m_fps)
      // m_Text->RenderText("Max Kin Energy: " + to_string_with_precision(kin_energy,1) + " J" , 
                    // 20.f, 400.0f, 0.5f, glm::vec3(0.3, 0.7f, 0.9f));    
                    
      // m_Text->RenderText("Impact Force: " + to_string_with_precision(m_impact_force,1) + " N", 
                    // 20.f, 350.0f, 0.5f, glm::vec3(0.3, 0.7f, 0.9f));    
      // // m_Text->RenderText("Plastic Energy: " + to_string_with_precision(st->GetPlasticEnergy(),1) + " N", 
                    // // 20.f, 300.0f, 0.5f, glm::vec3(0.3, 0.7f, 0.9f)); 
      // glDisable(GL_BLEND);
        
      // Poll and handle events (inputs, window resize, etc.)
      // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
      // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
      // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
      // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
      glfwPollEvents();



/////////////////////////////////////////////////

      // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
      // -------------------------------------------------------------------------------
      glfwSwapBuffers(window);
      //glfwPollEvents();
    }
}


int Editor::Terminate(){

    // glDeleteProgram(shaderProgram);

    // // glfw: terminate, clearing all previously allocated GLFW resources.
    // // ------------------------------------------------------------------
    // glfwTerminate();
    // return 0;
}
int main()
{
    cout << "Creating editor"<<endl;  
    editor = new Editor();
    cout << "done."<<endl;
    
    cout << "Initializing "<<endl;
    if (!editor->Init())
      cout << "failed!"<<endl;
    
    cout << "Done. "<<endl;
    editor->Run();

}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void Editor::processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
      
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
      cout << "Pressed A"<<endl;

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


bool Editor::LoadGround(myMesh *m_fieldmesh){
	vector<Vector3f> vpos(4), vnorm(4);
	vector<Vector2f> vtex(4);
	vector <unsigned int > vind(6); //2 triangles
  
  	//// VERTICES 
	//// 2 3
	//// 0 1
	/////////
	Vector3f apos[4] ={	
						Vector3f(-PITCH_LENGTH/2., 0.0f,-PITCH_WIDTH/2.),
						Vector3f(-PITCH_LENGTH/2., 0.0f, PITCH_WIDTH/2.),
						Vector3f( PITCH_LENGTH/2., 0.0f,-PITCH_WIDTH/2.),
						Vector3f( PITCH_LENGTH/2., 0.0f, PITCH_WIDTH/2.)};
						
	Vector3f norm=Vector3f(0.,1.,0.);	

	Vector2f atex[4] ={	Vector2f(0.0f, 0.0f),
                      Vector2f(1.0f, 0.0f),
                      Vector2f(0.0f, 1.0f),
						Vector2f(1.0f, 1.0f)};

	unsigned int aind[] = { 0, 1, 2,
							1, 3, 2};
							   
	
	for (int i=0;i<4;i++){
		vpos[i]	=apos[i];
		vnorm[i]=norm;
		vtex[i]	=atex[i];
	}
	
 
	for (int i=0;i<6;i++) vind[i] = aind[i];
	string file = "checker_blue.png";
	
	if (!m_fieldmesh->LoadMesh(vpos, vnorm, vtex,vind,file)){
		std::cout<<"Mesh load failed"<<endl;
		printf("Mesh load failed\n");
		return false;        			
		
	}
  return true;
}


bool Editor::LoadSphere(){

  
  	//// VERTICES 
	//// 2 3
	//// 0 1
	/////////
	Vector3f apos[4] ={	
						Vector3f(-PITCH_LENGTH/2., 0.0f,-PITCH_WIDTH/2.),
						Vector3f(-PITCH_LENGTH/2., 0.0f, PITCH_WIDTH/2.),
						Vector3f( PITCH_LENGTH/2., 0.0f,-PITCH_WIDTH/2.),
						Vector3f( PITCH_LENGTH/2., 0.0f, PITCH_WIDTH/2.)};
						
	Vector3f norm=Vector3f(0.,1.,0.);	

	Vector2f atex[4] ={	Vector2f(0.0f, 0.0f),
                      Vector2f(1.0f, 0.0f),
                      Vector2f(0.0f, 1.0f),
						Vector2f(1.0f, 1.0f)};

	unsigned int aind[] = { 0, 1, 2,
							1, 3, 2};
							   
	int vcount    = sizeof(sphere_low_pos)/(3*sizeof(float));
  int indcount  = sizeof(sphere_low_ind)/sizeof(unsigned int);
	// int vcount    = sizeof(vertices)/(3*sizeof(float));
  // int indcount  = sizeof(indices)/sizeof(unsigned int);
  cout << "Vertex count " << vcount << endl;
  cout << "Index  count " << indcount << endl;
  
	vector <Vector3f> vpos(vcount), vnorm(vcount);
	vector <Vector2f> vtex(vcount);
	vector <unsigned int > vind(indcount); //2 triangles
  
	for (int i=0;i<vcount;i++){
    Vector3f vert(sphere_low_pos[3*i],sphere_low_pos[3*i+1],sphere_low_pos[3*i+2]);
		vpos[i]	= vert;
    Vector3f vn(sphere_low_norm[3*i],sphere_low_norm[3*i+1],sphere_low_norm[3*i+2]);
		vnorm[i]=vn;
		//vtex[i]	=atex[i];
	}
  for (int i=0;i<indcount;i++){
    vind[i] = sphere_low_ind[i]-1;  //FROM OBJ FILE FORMAT, WHICH BEGINS AT ONE
  }

	// for (int i=0;i<vcount;i++){
    // Vector3f vert(vertices[3*i],vertices[3*i+1],vertices[3*i+2]);
		// vpos[i]	= vert;
    // Vector3f vn(sphere_low_norm[3*i],sphere_low_norm[3*i+1],sphere_low_norm[3*i+2]);
		// vnorm[i]=vn;
		// //vtex[i]	=atex[i];
	// }
  // for (int i=0;i<indcount;i++){
    // vind[i] = indices[i];  //FROM OBJ FILE FORMAT, WHICH BEGINS AT ONE
  // }
  
	string file = "checker_blue.png";
	
	if (!m_sphere_mesh.LoadMesh(vpos, vnorm, vtex,vind,file)){
		std::cout<<"Mesh load failed"<<endl;
		printf("Mesh load failed\n");
		return false;        			
		
	}
  return true;
}


void Editor::CalcFPS()
{
    m_frameCount++;

    long long time = GetCurrentTimeMillis();
    
    if (time - m_frameTime >= 1000) {
        m_frameTime = time;
        m_fps = m_frameCount;
        m_frameCount = 0;
    }
}