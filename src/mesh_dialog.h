#ifndef _MESH_DIALOG_H_
#define _MESH_DIALOG_H_


#include "Part.h"
#include "model/Model.h"
#include <string>
#include <vector>

//SAME DIALOG FROM CREATE AND EDIT MATERIAL
// IS BASICALLY THE SAME 
struct MeshDialog{
  struct CurveDivisionPreview {
    int tag = -1;
    double approx_length = 0.0;
    int node_count = 0;
    int segment_count = 0;
    bool use_custom_segments = false;
    int custom_segment_count = 0;
    double center_x = 0.0;
    double center_y = 0.0;
    double center_z = 0.0;
  };
  struct SurfacePreview {
    int tag = -1;
    double center_x = 0.0;
    double center_y = 0.0;
    double center_z = 0.0;
  };
  
   MeshDialog() {
     m_v = make_double3(0,0,0);
     m_apply_mesh = false;
     m_mesh_part = nullptr;
   }
  //void    AddLog(const char* fmt, ...);
  int m_id; //TODO; CHANGE TO VECTOR (TEMP FUNCTION)
  double m_elastic_const;
  double m_poisson_const;
  int part_type = 0 ;
  double3 m_v ;
  
  bool m_initialized = false;
  
  bool cancel_action;
  bool create_part;
  float m_element_size;
  int m_2d_mesh_generator = 0;
  bool m_apply_mesh;
  Part* m_mesh_part;
  bool m_curve_preview_visible = false;
  bool m_curve_preview_dirty = false;
  bool m_seed_pick_mode = false;
  int m_selected_curve_tag = -1;
  bool m_apply_transfinite_surfaces = true;
  std::string m_curve_preview_status;
  std::vector<CurveDivisionPreview> m_curve_preview;
  std::vector<SurfacePreview> m_surface_preview;


  
  const bool & isPartCreated()const{return create_part;}
  bool hasMeshRequest() const { return m_apply_mesh; }
  bool isCurvePreviewVisible() const { return m_curve_preview_visible; }
  bool isSeedPickModeActive() const { return m_seed_pick_mode; }
  int getSelectedCurveTag() const { return m_selected_curve_tag; }
  bool shouldApplyTransfiniteSurfaces() const { return m_apply_transfinite_surfaces; }
  const std::vector<CurveDivisionPreview>& getCurvePreview() const { return m_curve_preview; }
  const std::vector<SurfacePreview>& getSurfacePreview() const { return m_surface_preview; }
  const CurveDivisionPreview* getSelectedCurvePreview() const;
  bool selectCurveByTag(int tag);
  bool setSelectedCurveSegments(int segmentCount);
  void resetSelectedCurveSegments();
  void resetAllCurveSegments();
  int getCurveNodeCountOverride(int tag) const;
  Part* consumeMeshRequest() {
    Part* part = m_mesh_part;
    m_apply_mesh = false;
    m_mesh_part = nullptr;
    return part;
  }
  void   Draw(const char* title, bool* p_open = NULL, Model* model = NULL, Part* prt = NULL);  
};


bool ShowEditPartDialog(bool* p_open, PartDialog *, Part *);


#endif
