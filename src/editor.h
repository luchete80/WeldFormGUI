#ifndef _EDITOR_H_
#define _EDITOR_H_


#include "ogldev_pipeline.h"
#include "ogldev_camera.h"
#include "ogldev_camera.h"

#include "text_renderer.h"

#include "myMesh.h"

#include "ogldev_basic_lighting.h"



#define COLOR_TEXTURE_UNIT_INDEX        0

#include <iostream>

#include "picking_texture.h"
#include "picking_technique.h"

#include "ogldev_util.h"
#include "ogldev_callbacks.h"
#include "ogldev_app.h"

#include "Domain.h"

#include "arcball_camera.h"

#include "selector.h"

#include "action.h"

#include "log.h"

float vertices[] = {
    // first triangle
     0.5f,  0.5f, 0.0f,  // top right
     0.5f, -0.5f, 0.0f,  // bottom right
    -0.5f,  0.5f, 0.0f,  // top left 
    // second triangle
     0.5f, -0.5f, 0.0f,  // bottom right
    -0.5f, -0.5f, 0.0f,  // bottom left
    -0.5f,  0.5f, 0.0f   // top left
}; 
  // unsigned int indices[] = {  // note that we start from 0!
      // 0, 1, 
      // 0, 2,  // first Triangle
      // 0, 3,
      // 1, 2,
      // 2, 3,
      // 3, 1
      // // 4, 5,
      // // 5, 6,
      // // 6, 7,
      // // 7, 4  
      // // GROUND
  // };
  
  unsigned int indices[] = {  // note that we start from 0!
    0, 1, 3,   // first triangle
    1, 2, 3    // second triangle
  };  
  
    // unsigned int indices[] = {  // note that we start from 0!
      // 0, 1, 
      // 0, 2,  // first Triangle
      // 0, 3,
      // 1, 2,
      // 2, 3,
      // 3, 1
      // // 4, 5,
      // // 5, 6,
      // // 6, 7,
      // // 7, 4  
      // // GROUND
  // };
  
class ExampleAppLog;

class Editor 
//: public ICallbacks, public OgldevApp
{
public:

public:  
  Editor();
  ~Editor(){}
  int Init();
  void Run();
  int Terminate();
  
  virtual void PickingPhase();
  virtual void RenderPhase();
  
  void RenderBeams();
  
  virtual void scroll(double xoffset, double yoffset);
  virtual void Mouse(int Button, int Action, int Mode);
  virtual void Key(int key, int scancode, int action, int mods);
  bool LoadGround(myMesh *m_fieldmesh);
  
  bool LoadSphere();
  
  void MoveNode();
  
  void processInput(GLFWwindow *window);
  void CursorPos(double x, double y);
  void drawGui();
  virtual void RenderPass(){}; //ADD ANOTHER CALLBACK
  
  void CalcFPS();
  
  ArcballCamera * ArcCamera(){return arcCamera;}
  
  const SPH::Domain & getDomain() const {return m_domain;}
  
protected:
  PickingTexture m_pickingTexture;
  PickingTechnique m_pickingEffect;

  BasicLightingTechnique* m_plightEffect;

  GLFWwindow* window;
  unsigned int shaderProgram;

  unsigned int SCR_WIDTH;
  unsigned int SCR_HEIGHT;
 
  //MOUSE
  bool is_struct;
  bool m_left_button_pressed;
  std::vector <myMesh *> m_nodemesh;
  bool mesh_loaded;
  bool box_select_mode;
  TextRenderer *m_Text;


  Camera *camera;
  PersProjInfo m_persProjInfo;

  GLuint gWVPLocation;

  myMesh ground_mesh;
  myMesh m_sphere_mesh;

  DirectionalLight 					m_directionalLight;

  long long m_start_time;
  SPH::Domain m_domain;
  double m_dx;

  bool rotatecam;
  
  unsigned int VBO, VAO, EBO; //TODO: THIS SHOULD BE MOVED TO 
  
  ArcballCamera *arcCamera;
  Selector m_selector;
  
  std::vector < int > m_sel_particles; //TODO: MOVE TO SELECTOR
  int m_sel_count;
  
  // PHysics Things
  int   m_sel_node;
  bool  m_is_node_sel;
  
  double last_mouse_x,last_mouse_y;
  long long m_last_mouse_dragtime;
  
  bool m_pause;
  
  float kin_energy;
  float m_impact_force;
  
  //Like in Ogldev App
  long long m_frameTime;
	long long m_startTime;
	int m_frameCount;
  int m_fps;
  
  ExampleAppLog logtest;
  
  float m_rotation;
  
  Action* m_currentaction;
  bool    is_action_active; //SHOULD BE THE SAME OF if (m_currentaction!=NULL)
  
  bool m_show_app_main_menu_bar;
  bool m_show_app_console;
  
};


#endif