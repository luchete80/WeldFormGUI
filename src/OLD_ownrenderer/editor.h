#ifndef _EDITOR_H_
#define _EDITOR_H_


#include "ogldev_pipeline.h"
#include "ogldev_camera.h"
#include "ogldev_camera.h"

#include "text_renderer.h"

#include "renderer.h"

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
#include "entity_dialog.h"
#include "material_dialog.h"
#include "set_dialog.h"

#include "Material.h"

#include "job_dialog.h"


//#include "gMesh.h"
#include "model/Model.h"
#include "model/Mesh.h"

class SceneView;
class ViewportWindow;

  
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
class Job;


class Editor 
//: public ICallbacks, public OgldevApp
{

public:  
  friend ViewportWindow;
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
  bool LoadGround(Renderer *m_fieldmesh);
  
  bool LoadSphere();
  
  void MoveNode();
  
  void processInput(GLFWwindow *window);
  void CursorPos(double x, double y);
  void drawGui();
  virtual void RenderPass(){}; //ADD ANOTHER CALLBACK
  
  void CalcFPS();
  
  ArcballCamera * ArcCamera(){return arcCamera;}
  SceneView* getSceneView(){return m_sceneview;}
  
  const SPHModel & getDomain() const {return m_domain;}
  
  void calcDomainCenter();
  void calcMeshCenter();
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

  bool mesh_loaded;
  bool box_select_mode;
  TextRenderer *m_Text;

  

  //Camera *camera;
  PersProjInfo m_persProjInfo;

  GLuint gWVPLocation;

  //Renderer ground_mesh;
  Renderer m_renderer;
  
  Mesh   *m_fem_msh;
  bool    is_fem_mesh;
  bool    is_sph_mesh;
  
  DirectionalLight 					m_directionalLight;

  long long m_start_time;
  SPHModel m_domain; /////TODO: MODIFY, CONVERT TO POINTER TO BASE CLASS
  Vector3f m_domain_center;
  Vector3f m_femsh_center;
  double m_dx;

  bool rotatecam;
  
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
  
  MaterialDialog  m_matdlg;
  //JobDialog<Job> m_jobdlg;
  JobDialog       m_jobdlg; //creation
  JobShowDialog   m_jobshowdlg;
  CreateSetDialog m_setdlg;
  
  bool m_show_mat_dlg; //create
  bool m_show_job_dlg; //create
  bool m_show_set_dlg;
  bool m_show_mat_dlg_edit;
  bool create_new_mat;
  bool create_new_job;
  bool create_new_set;

  SceneView *m_sceneview;
  
  float m_rotation;
  
  std::vector <Material_*> m_mats;
  std::vector <Job *> m_jobs;
  
  Model *m_model;
  bool is_model_open;
  Material_ *selected_mat;
  
  Action* m_currentaction;
  bool    is_action_active; //SHOULD BE THE SAME OF if (m_currentaction!=NULL)
  
  bool m_show_app_main_menu_bar;
  bool m_show_app_console;
  
  bool m_add_part;
  
  ViewportWindow *m_viewport_win;
  
};



#endif