// Standard Library
#include <iostream>

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
    fread(file_data, 1, file_size, f);
    bool ret = LoadTextureFromMemory(file_data, file_size, out_texture, out_width, out_height);
    IM_FREE(file_data);
    return ret;
}

//#include "graphics/axis.h"
/*
// int argc, char* argv are required for vtkRegressionTestImage
static int TestReader(const std::string& path, unsigned int format)
{
  vtkNew<vtkOCCTReader> reader;
  reader->RelativeDeflectionOn();
  reader->SetLinearDeflection(0.1);
  reader->SetAngularDeflection(0.5);
  reader->ReadWireOn();
  reader->SetFileName(path.c_str());
  reader->SetFileFormat(format);
  reader->Update();

  vtkNew<vtkCompositePolyDataMapper> mapper;
  mapper->SetInputDataObject(reader->GetOutput());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->RotateY(90);

  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renderWindow;
  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindow->AddRenderer(renderer);
  renderer->AddActor(actor);
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderWindow->SetSize(400, 400);
  renderer->ResetCamera();
  renderWindow->Render();

  int retVal;
  //int retVal = vtkRegressionTestImage(renderWindow);
//  if (retVal == vtkRegressionTester::DO_INTERACTOR)
//    renderWindowInteractor->Start();
  

  return retVal;
}
*/

/*
int TestOCCTReader(int argc, char* argv[])
{
  if (argc < 3)
  {
    return EXIT_FAILURE;
  }

  if (!TestReader(
        argc, argv, std::string{ argv[2] } += "/Data/wall.stp", vtkOCCTReader::Format::STEP))
  {
    return EXIT_FAILURE;
  }

  if (!TestReader(
        argc, argv, std::string{ argv[2] } += "/Data/wall.iges", vtkOCCTReader::Format::IGES))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
*/
static void glfw_error_callback(int error, const char* description)
{
  fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

Axis axis;

int main(int argc, char* argv[])
{
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


  //const GLFWvidmode* modes = glfwGetVideoMode(glfwGetPrimaryMonitor()/*, &count*/);
  //cout << "Monitor width: "<<modes->width<<", height: "<<modes->height<<endl;

  // Create window with graphics context
  //GLFWwindow* window = glfwCreateWindow(modes->width, modes->height, "WeldForm GUI", NULL, NULL);
  GLFWwindow* window = glfwCreateWindow(800, 600, "WeldForm GUI", NULL, NULL);
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
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  ImFont* font1 = io.Fonts->AddFontDefault();


  ImFont* font_ubu = io.Fonts->AddFontFromFileTTF("Ubuntu-L.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesDefault()); 
  //ImFont* font_satoshi = io.Fonts->AddFontFromFileTTF("Satoshi.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesDefault());
  //IM_ASSERT(font_satoshi != NULL);
  if (!font_ubu->IsLoaded()){
    fprintf(stderr,"ERRROR. Cannot load font.\n");
    }
  
  //ImGui::PushFont(font1);
    
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows'
  
  
  // Setup Dear ImGui style
  ImGui::StyleColorsDark();

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);


  //ImGui::PushFont(font1);


  // Initialize VtkViewer objects
  //VtkViewer vtkViewer1;
  //vtkViewer1.addActor(actor);

  VtkViewer vtkViewer2;
  vtkViewer2.getRenderer()->SetBackground(0, 0, 0); // Black background
  //vtkViewer2.addActor(actor);

  // Our state
  bool show_demo_window = true;
  bool show_another_window = false;
  bool vtk_2_open = true;
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
  
  cout << "Creating Editor"<<endl;
  Editor* editor = new Editor();
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
    //Axis axis;  
    axis.setInteractor(vtkViewer2.getInteractor());  
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
      bool ret = LoadTextureFromFile("buttons/extents.png", &my_image_texture, &my_image_width, &my_image_height);
      IM_ASSERT(ret);
          
    editor->addViewer(&vtkViewer2);
  // Main loop
  while (!glfwWindowShouldClose(window))
  {
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    glfwPollEvents();

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    
    editor->drawGui();
    
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
      ImGui::Checkbox("VTK Viewer #2", &vtk_2_open);

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
    ImGui::Begin("Vtk Viewer 1", nullptr, VtkViewer::NoScrollFlags());
    //vtkViewer1.render(); // default render size = ImGui::GetContentRegionAvail()
    ImGui::End();

    // 5. Show a more complex VtkViewer Instance (Closable, Widgets in Window)
    ImGui::SetNextWindowSize(ImVec2(720, 480), ImGuiCond_FirstUseEver);
    if (vtk_2_open){

      ImGui::Begin("Model Viewer", &vtk_2_open, VtkViewer::NoScrollFlags());
      ImGui::PushFont(font_ubu); //AFTER NEW FRAME
      // Other widgets can be placed in the same window as the VTKViewer
      // However, since the VTKViewer is rendered to size ImGui::GetContentRegionAvail(),
      // it is best to put all widgets first (i.e., render the VTKViewer last).
      // If you want the VTKViewer to be at the top of a window, you can manually calculate
      // and define its size, accounting for the space taken up by other widgets

      auto renderer = vtkViewer2.getRenderer();
      


      ImTextureID my_tex_id = io.Fonts->TexID;
      //You can return anything but you should cast it as void
      
      ImGui::ImageButton("blah", (void *)(intptr_t)my_image_texture, ImVec2(30, 30));
      ImGui::SameLine();
      if (ImGui::ImageButton("blah", (void *)my_tex_id, ImVec2(30, 30)))
        cout << "clicked"<<endl;
        
      if (ImGui::Button("VTK Background: Black")){
        renderer->SetBackground(0, 0, 0);
      }
      ImGui::SameLine();
      if (ImGui::Button("VTK Background: Red")){
        renderer->SetBackground(1, 0, 0);
      }
      ImGui::SameLine();
      if (ImGui::Button("VTK Background: Green")){
        renderer->SetBackground(0, 1, 0);
      }
      ImGui::SameLine();
      if (ImGui::Button("VTK Background: Blue")){
        //renderer->SetBackground(0.6, 0.6, 0.8);
        renderer->SetBackground(0.2,0.2,0.4);
        renderer->SetBackground2(0.8,0.8,0.8);
      }
      static float vtk2BkgAlpha = 0.2f;
      ImGui::SliderFloat("Background Alpha", &vtk2BkgAlpha, 0.0f, 1.0f);
      renderer->SetBackgroundAlpha(vtk2BkgAlpha);

      vtkViewer2.render();
      
      ImGui::PopFont();
      ImGui::End();
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
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
