#ifndef _MESH_DIALOG_H_
#define _MESH_DIALOG_H_


#include "Part.h"

//SAME DIALOG FROM CREATE AND EDIT MATERIAL
// IS BASICALLY THE SAME 
struct MeshDialog{
  
   MeshDialog() {
     m_v = make_double3(0,0,0);}
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
  
  const bool & isPartCreated()const{return create_part;}
  void   Draw(const char* title, bool* p_open = NULL, Part* prt = NULL);  
};


bool ShowEditPartDialog(bool* p_open, PartDialog *, Part *);


#endif
