#ifndef _MOVE_PART_DIALOG_H_
#define _MOVE_PART_DIALOG_H_


#include "Part.h"

//SAME DIALOG FROM CREATE AND EDIT MATERIAL
// IS BASICALLY THE SAME 
struct MovePartDialog{
  
  //void    AddLog(const char* fmt, ...);
  int m_id; //TODO; CHANGE TO VECTOR (TEMP FUNCTION)
  double m_elastic_const;
  double m_poisson_const;

  
  bool m_initialized = false;
  
  bool cancel_action;
  bool create_part;
  
  //const bool & isModelCreated()const{return create_part;}
  void   Draw(Part* prt = NULL);  
};


#endif
