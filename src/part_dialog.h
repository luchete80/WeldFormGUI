#ifndef _PART_DIALOG_H_
#define _PART_DIALOG_H_


#include "Part.h"

//SAME DIALOG FROM CREATE AND EDIT MATERIAL
// IS BASICALLY THE SAME 
struct PartDialog{
  
   PartDialog() {
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
  
  const bool & isPartCreated()const{return create_part;}
  void   Draw(const char* title, bool* p_open = NULL, Part* prt = NULL);  
};

//Returns true if NEW material is created or if changes are saved, if no
//if no material is created, pointer is null
Part ShowCreateMaterialDialog(bool* p_open, PartDialog *, bool* ret);
bool ShowEditPartDialog(bool* p_open, PartDialog *, Part *);


#endif
