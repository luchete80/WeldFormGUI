#ifndef _EDITOR_H_
#define _EDITOR_H_

/*
#include "ogldev_pipeline.h"
#include "ogldev_camera.h"
#include "ogldev_camera.h"

#include "text_renderer.h"

#include "renderer.h"

#include "ogldev_basic_lighting.h"


#include "picking_texture.h"
#include "picking_technique.h"

#include "ogldev_util.h"
#include "ogldev_callbacks.h"
#include "ogldev_app.h"
#include "arcball_camera.h"

*/

#include "global.h"


#define COLOR_TEXTURE_UNIT_INDEX        0

#include <iostream>

#include "selector.h"
#include "SPHModel.h"
#include "log.h"
#include "entity_dialog.h"
#include "material_dialog.h"
#include "part_dialog.h"
#include "set_dialog.h"
#include "model_dialog.h"
#include "move_part_dialog.h"

#include "Material.h"
#include "Section.h"

#include "job_dialog.h"
#include "interaction_props_dialog.h"
#include "step_dialog.h"
#include "remesh_dialog.h"
#include "bc_dialog.h"
#include "ini_dialog.h"
#include "mesh_dialog.h"
#include "modelcheck/ModelCheck.h"


//#include "gMesh.h"
#include "model/Model.h"
#include "model/Mesh.h"
#include "geom/Geom.h"

#include <vtkSmartPointer.h>
#include <vtkActor.h>


#include "results.h"
#include <memory>

class TransformGizmo;
class MeasurementTool;
class Action;


class SceneView;
class ViewportWindow;
class VtkViewer;
class GraphicMesh;
class Material_Db;
class vtkPolyData;
class InitialCondition;

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
  
class Job;


class Editor 
//: public ICallbacks, public OgldevApp
{

public:  
  friend ViewportWindow;
  Editor();
  virtual ~Editor();
  int Init();
  void Run();
  int Terminate();
  void Update();
  void showScriptBrowser();
  

  
  virtual void scroll(double xoffset, double yoffset);
  virtual void Mouse(int Button, int Action, int Mode);
  virtual void Key(int key, int scancode, int action, int mods);
  //bool LoadGround(Renderer *m_fieldmesh);
  
  bool LoadSphere();
  
  void MoveNode();
  void meshPart(Part* part);
  
  void processInput(GLFWwindow *window);
  void CursorPos(double x, double y);
  void drawGui();
  void handleSelectionInteraction();
  void drawSelectionOverlay() const;
  void handleMeasurementInteraction();
  void drawMeasurementOverlay() const;
  virtual void RenderPass(){}; //ADD ANOTHER CALLBACK
  bool openModelFromPath(const std::string& filePathName);
  bool openInputFromPath(const std::string& filePathName);
  bool openScriptFromPath(const std::string& filePathName);
  bool importMeshPartFromPath(const std::string& filePathName);
  bool createJobFromActiveModel(bool runJob = false);
  bool openResultsFromPath(const std::string& filePathName);
  bool openResultsForModel();
  bool openResultsForJob(Job* job);
  void requestJobRun(Job* job);
  bool refreshOpenResults(int preferredFrameIndex = -1);
  int consumePendingResultsFrameIndex();
  bool scalePartGeometry(Part* part, double factor);
  void adoptModelFromScript(Model* model);
  bool consumeResultsViewerActivationRequest();
  bool isLoadingResults() const;
  bool hasBlockingDialogOpen() const;
  bool isSetDialogOpen() const { return m_show_set_dlg; }
  bool isSetSelectionActive() const { return m_selection_enabled; }
  int getSelectedNodeCount() const { return m_selector.getSelectedNodeCount(); }
  int getSelectedElementCount() const { return m_selector.getSelectedElementCount(); }
  bool getShowSelectedNodeLabels() const { return m_show_selected_node_labels; }
  void setShowSelectedNodeLabels(bool value) { m_show_selected_node_labels = value; }
  bool getShowAllNodeLabels() const { return m_show_all_node_labels; }
  void setShowAllNodeLabels(bool value) { m_show_all_node_labels = value; }
  bool getShowAllElementLabels() const { return m_show_all_element_labels; }
  void setShowAllElementLabels(bool value) { m_show_all_element_labels = value; }
  void setActiveViewer(VtkViewer* activeViewer) { viewer = activeViewer; }
  void clearBoundaryConditionOverlay();
  void clearPartSelectionState();
  bool canUndo() const;
  bool canRedo() const;
  bool undoLastAction();
  bool redoLastAction();
  void pushAction(std::unique_ptr<Action> action);
  bool selectNodeSetById(int setId);
  bool selectElementSetById(int setId);
  bool selectFaceSetById(int setId);
  void clearSelectedNodeSet();
  void clearSelectedElementSet();
  void clearSelectedFaceSet();
  void closeCurrentModel();
  void closeCurrentResults();
  
  void CalcFPS();
  void addViewer(VtkViewer *);
  void addResViewer(VtkViewer *v){res_viewer=v;}
    
  //ArcballCamera * ArcCamera(){return arcCamera;}
  SceneView* getSceneView(){return m_sceneview;}
  
  



  //Model * getDomain() {return m_domain;}
  
  Model & getModel(){return *m_model;}
  
  void calcDomainCenter();
  void calcMeshCenter();
  void setShowConsole(bool &b){m_show_app_console=b;}
  void changeShowConsole(){m_show_app_console=!m_show_app_console;}
  
  vtkSmartPointer<vtkActor> getCurrResActor(){return m_curr_res_actor;}
  
  MultiResult* getResults(){return m_results;}
  Mesh* getResultViewerTargetMesh() const { return findResultViewerTargetMesh(); }

    // TODO TOMODIFY
    //~ std::vector<Results*> allResults; // todos los resultados cargados
    //~ Results* currentResults = nullptr; // resultado activo
    //~ int currentFrame = 0;              // frame activo del resultado actual

  
  
  
protected:
  void drawSelectionControls();
  bool isSelectorInteractionEnabled() const;
  bool shouldDrawSelectionOverlay() const;
  bool projectNodeToViewport(Node* node, double& x, double& y) const;
  bool projectElementCentroidToViewport(Element* element, double& x, double& y) const;
  Mesh* findResultViewerTargetMesh() const;
  Node* pickClosestNodeAt(double x, double y, double maxDistancePixels = 12.0) const;
  Element* pickClosestElementAt(double x, double y, double maxDistancePixels = 16.0) const;
  void selectNodesInBox(double x0, double y0, double x1, double y1);
  void selectElementsInBox(double x0, double y0, double x1, double y1);
  void selectNodeSet(Mesh* mesh, int setIndex);
  void selectElementSet(Mesh* mesh, int setIndex);
  void selectFaceSet(Mesh* mesh, int setIndex);
  NodeSet* getSelectedNodeSet();
  const NodeSet* getSelectedNodeSet() const;
  ElementSet* getSelectedElementSet();
  const ElementSet* getSelectedElementSet() const;
  FaceSet* getSelectedFaceSet();
  const FaceSet* getSelectedFaceSet() const;
  void clearStateForDeletedMesh(Mesh* mesh);
  void clearStateForDeletedPart(Part* part);
  void clearStateForDeletedCondition(Condition* condition);
  void clearSelectionForHiddenPart(Part* part);

  GLFWwindow* window;
  unsigned int shaderProgram;

  unsigned int SCR_WIDTH;
  unsigned int SCR_HEIGHT;
 
  //MOUSE
  bool is_struct;
  bool m_left_button_pressed;

  bool mesh_loaded;
  bool box_select_mode;

  

  //Camera *camera;
  PersProjInfo m_persProjInfo;

  GLuint gWVPLocation;

  //Renderer ground_mesh;
  
  Mesh   *m_fem_msh;
  //bool    is_fem_mesh;
  bool    is_sph_mesh;
  

  long long m_start_time;
  //Model *m_domain; /////TODO: MODIFY, CONVERT TO POINTER TO BASE CLASS
  Vector3f m_domain_center;
  Vector3f m_femsh_center;
  double m_dx;

  bool rotatecam;
  
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
  PartDialog      m_prtdlg;
  ModelDialog     m_moddlg;
  
  MeshDialog      m_mshdlg;
  
  MovePartDialog     m_movprtdlg;
    
  //JobDialog<Job> m_jobdlg;
  JobDialog       m_jobdlg; //creation
  JobShowDialog   m_jobshowdlg;
  InteractionPropsDialog m_interactionpropsdlg;
  StepDialog      m_stepdlg;
  RemeshDialog    m_remeshdlg;
  CreateSetDialog m_setdlg;
  
  BCDialog        m_bcdlg;
  IniDialog       m_inidlg;
  
  bool m_show_mat_dlg; //REMOVE, CGHANGE TO 

  bool m_show_set_dlg;
  bool m_show_mat_dlg_edit = false;
  bool m_show_mod_dlg_edit = false;  
  bool m_creating_model = false;
  bool m_show_prt_dlg_edit = false;
  bool m_show_bc_dlg_edit = false;
  bool m_show_step_dlg_edit = false;
  bool m_show_remesh_dlg_edit = false;
  bool m_show_interaction_props_dlg = false;
  bool create_new_mat;
  bool create_new_part;
  bool create_new_job;
  bool create_new_set;
  
  bool m_show_msh_dlg;

  bool m_showNewDomain = false; // o true si quieres que inicie abierto

  SceneView *m_sceneview;
  
  float m_rotation;
  
  std::vector <Material_*> m_mats;
  std::vector <Job *> m_jobs;
  
  Model *m_model;
  bool is_model = false;
  Material_ *selected_mat = nullptr;
  Section *selected_section = nullptr;
  Part      *selected_prt = nullptr;
  Part      *highlighted_prt = nullptr;
  Part      *hovered_prt = nullptr;
  Model     *selected_mod = nullptr;
  Step      *selected_step = nullptr;
  Condition *selected_bc = nullptr;
  Condition *hovered_bc = nullptr;
  InitialCondition *selected_ic = nullptr;
  int       m_create_bc;  //boundary or initial condition
  bool      m_creating_step = false;
  
  bool m_show_mov_part = false;
  double m_move_part_offset[3] = {0.0, 0.0, 0.0};
  double m_move_part_initial_center[3] = {0.0, 0.0, 0.0};
  double m_move_part_step = 0.1;
  Mesh* m_selected_node_set_mesh = nullptr;
  int m_selected_node_set_index = -1;
  Mesh* m_selected_element_set_mesh = nullptr;
  int m_selected_element_set_index = -1;
  Mesh* m_selected_face_set_mesh = nullptr;
  int m_selected_face_set_index = -1;
  bool m_show_rename_set_dlg = false;
  char m_rename_set_name[128] = {0};
    
  std::vector<std::unique_ptr<Action>> m_undo_stack;
  std::vector<std::unique_ptr<Action>> m_redo_stack;
  
  bool m_show_app_main_menu_bar;
  bool m_show_app_console;
  bool m_show_script_browser = false;
  std::vector<fs::path> m_script_browser_entries;
  int m_selected_script_browser_index = -1;
  
  bool m_moving_mode = false;
  int  m_mov_part = -1;
   
  bool m_add_part;
  
  ViewportWindow *m_viewport_win;
  
  VtkViewer *viewer = nullptr;
  VtkViewer *model_viewer = nullptr;
  VtkViewer *res_viewer = nullptr;  
  
  //Visual meshes
  //std::vector<GraphicMesh*> graphic_mesh;
  GraphicMesh* graphic_mesh;
  
  vtkSmartPointer<TransformGizmo> gizmo;

  vtkSmartPointer<vtkActor> m_curr_res_actor = nullptr;
  
  MultiResult *m_results = nullptr;
  std::unique_ptr<MeasurementTool> m_measurement_tool;
  bool m_activate_results_viewer = false;
  bool m_selection_enabled = false;
  bool m_show_selected_node_labels = false;
  bool m_show_all_node_labels = false;
  bool m_show_all_element_labels = false;
  enum class SidebarTab {
    Model,
    Results
  };
  SidebarTab m_sidebar_tab = SidebarTab::Model;
  std::vector<vtkSmartPointer<vtkProp>> m_model_bc_overlay_actors;
  std::vector<vtkSmartPointer<vtkProp>> m_results_bc_overlay_actors;
  std::vector<vtkSmartPointer<vtkProp>> m_part_overlay_actors;

  struct PendingResultsLoad {
    bool active = false;
    bool justStarted = false;
    bool replaceExistingResults = false;
    fs::path sourceDirectory;
    fs::path sourceJsonFile;
    MultiResult results;
    std::vector<ResultFrameEntry> entries;
    std::size_t nextIndex = 0;
    std::size_t loadedFrames = 0;
    std::size_t skippedFrames = 0;
    std::size_t reloadStartIndex = 0;
    std::size_t keepPrefixFrameCount = 0;
    int preferredFrameIndex = -1;
    std::string currentFile;
    std::string errorMessage;
  };

  PendingResultsLoad m_pending_results_load;
  int m_pending_results_frame_index = -1;
  bool m_close_results_load_popup = false;
  bool m_expand_model_tree_once = false;
  struct PendingJobRunConfirmation {
    bool open = false;
    Job* job = nullptr;
    std::vector<fs::path> artifacts;
  };
  PendingJobRunConfirmation m_pending_job_run_confirmation;
  struct ModelCheckPopupState {
    bool open = false;
    wfgui::modelcheck::CheckReport report;
  };
  ModelCheckPopupState m_model_check_popup;

  bool beginResultsLoadFromJson(const std::string& jsonFile,
                                bool replaceExistingResults = false,
                                int preferredFrameIndex = -1);
  void advanceResultsLoad();
  void finishResultsLoad();
  void drawResultsLoadProgress();
  void drawPendingJobRunConfirmation();
  void executePendingJobRun(bool deleteExistingArtifacts);
  bool runModelCheckBeforeJobRun(Model& model);
  void drawModelCheckPopup();
  void clearBoundaryConditionOverlayForViewer(VtkViewer* targetViewer,
                                              std::vector<vtkSmartPointer<vtkProp>>& overlayActors);
  void updateBoundaryConditionOverlayForViewer(VtkViewer* targetViewer,
                                               std::vector<vtkSmartPointer<vtkProp>>& overlayActors);
  void updateBoundaryConditionOverlay();
  void clearPartOverlay();
  void updatePartOverlay();
  vtkSmartPointer<vtkActor> getPartVisualActor(Part* part) const;
  void applyPartTranslation(Part* part, double dx, double dy, double dz);
  bool getPartVisualCenter(Part* part, double center[3]) const;
  void updateMovePartOffsetFromCurrentState();
  void resetCurrentPartTransform();
  void finishMoveMode(bool acceptTransform);
  void refreshScriptBrowser();
  void drawScriptBrowserWindow();
  Part* findBoundaryConditionTargetPart(const Condition* condition) const;
  NodeSet* findNodeSetById(int setId) const;
  ElementSet* findElementSetById(int setId) const;
  FaceSet* findFaceSetById(int setId) const;
  vtkSmartPointer<vtkPolyData> getBoundaryConditionTargetPolyData(Part* part) const;
  vtkSmartPointer<vtkPolyData> getBoundaryConditionTargetPolyData(const NodeSet* nodeSet) const;
  
  Material_Db m_mat_db;
    
};



#endif
