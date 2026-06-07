#ifndef _MESH_DIALOG_H_
#define _MESH_DIALOG_H_


#include "Part.h"
#include "model/Model.h"

//SAME DIALOG FROM CREATE AND EDIT MATERIAL
// IS BASICALLY THE SAME 
struct MeshDialog{
  
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


  
  const bool & isPartCreated()const{return create_part;}
  bool hasMeshRequest() const { return m_apply_mesh; }
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
