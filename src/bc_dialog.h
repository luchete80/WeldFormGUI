#ifndef _BC_DIALOG_H_
#define _BC_DIALOG_H_


#include "BoundaryCondition.h"

//SAME DIALOG FROM CREATE AND EDIT MATERIAL
// IS BASICALLY THE SAME 
struct BCDialog{
  
  //void    AddLog(const char* fmt, ...);
  double m_density_const; //TODO; CHANGE TO VECTOR (TEMP FUNCTION)
  double m_elastic_const;
  double m_poisson_const;
  
  bool cancel_action;
  bool create_bc;
  
  const bool & isBCCreated()const{return create_bc;}
  void   Draw(const char* title, bool* p_open = NULL, Material_* mat = NULL);  
};

//Returns true if NEW material is created or if changes are saved, if no
//if no material is created, pointer is null
Material_ ShowCreateBCDialog(bool* p_open, MaterialDialog *, bool* ret);
bool ShowEditBCDialog(bool* p_open, BCDialog *, BoundaryCondition *);


#endif