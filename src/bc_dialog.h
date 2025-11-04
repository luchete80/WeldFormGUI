#ifndef _BC_DIALOG_H_
#define _BC_DIALOG_H_


#include "BoundaryCondition.h"
#include "Dialog.h"
#include "double3.h"

class Model;

//SAME DIALOG FROM CREATE AND EDIT MATERIAL
// IS BASICALLY THE SAME 
struct BCDialog:
public Dialog
{
  
  //void    AddLog(const char* fmt, ...);
  double m_density_const; //TODO; CHANGE TO VECTOR (TEMP FUNCTION)
  double m_elastic_const;
  double m_poisson_const;
  
  bool cancel_action;
  bool create_bc;
  int m_targetId;
  
  int m_applyTo;
  double3 m_vel;
  
  const bool & isBCCreated()const{return create_bc;}
  void   Draw(const char* title, bool* p_open, Model* model, BoundaryCondition *sel_bc = nullptr);  
};


#endif
