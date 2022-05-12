#ifndef _EDITOR_H_
#define _EDITOR_H_


#include "ogldev_pipeline.h"
#include "ogldev_camera.h"
#include "ogldev_camera.h"

#include "text_renderer.h"

#include "myMesh.h"

#include "Renderer.h"
#include "ogldev_basic_lighting.h"

// PHYSCIS
#include "Plane.h"
#include "Structure.h"
#include "Contact.h"

#define COLOR_TEXTURE_UNIT_INDEX        0

#include <iostream>

#include "picking_texture.h"
#include "picking_technique.h"

#include "ogldev_util.h"
#include "ogldev_callbacks.h"
#include "ogldev_app.h"

float vertices[] = {
     0.5f, 1.5f, 0.0f,  // top right
     0.5f, 0.5f, 0.0f,  // bottom right
    -0.5f, 0.5f, 1.0f,  // bottom left
    -0.5f, 1.5f, 1.0f   // top left 
    
    // -10.f, 0.f, -10.f,   // GROUND START! bottom left
     // 10.f, 0.f, -10.f,   // 
     // 10.f, 0.f,  10.f,   // t 
    // -10.f, 0.f,  10.0f,   // 
    
};

  unsigned int indices[] = {  // note that we start from 0!
      0, 1, 
      0, 2,  // first Triangle
      0, 3,
      1, 2,
      2, 3,
      3, 1
      // 4, 5,
      // 5, 6,
      // 6, 7,
      // 7, 4  
      // GROUND
  };
  

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
  void SolveStruct(float &dt);
  void MoveNode();
  
  void processInput(GLFWwindow *window);
  void CursorPos(double x, double y);
  void drawGui();
  virtual void RenderPass(){}; //ADD ANOTHER CALLBACK
  
  void CalcFPS();
  
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
  TextRenderer *m_Text;
  Plane  *m_plane;
  Structure *st;

  Camera *camera;
  PersProjInfo m_persProjInfo;

  GLuint gWVPLocation;

  myMesh ground_mesh;

  long long m_start_time;

  bool rotatecam;
  
  unsigned int VBO, VAO, EBO; //TODO: THIS SHOULD BE MOVED TO 

  
  // PHysics Things
  int   m_sel_node;
  bool  m_is_node_sel;
  
  float last_mouse_x,last_mouse_y;
  long long m_last_mouse_dragtime;
  
  bool m_pause;
  
  float kin_energy;
  float m_impact_force;
  
  //Like in Ogldev App
  long long m_frameTime;
	long long m_startTime;
	int m_frameCount;
  int m_fps;
  
};


#endif