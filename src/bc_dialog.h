#ifndef _BC_DIALOG_H_
#define _BC_DIALOG_H_


#include "Condition.h"
#include "Dialog.h"
#include "double3.h"

enum class DialogMode {
    Auto,       // usar sel_bc para decidir
    NewBoundary,
    NewInitial
};

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
  bool initialized = false;
  int symPreset = 0;
  
  int m_applyTo;
  double3 m_vel;

  int bcType = 0;      // 0 = Velocity, 2 = Symmetry
  double3 m_normal = make_double3(0,0,1);

  //bool initialized = false;    
      
  const bool & isBCCreated()const{return create_bc;}
  void   Draw(const char* title, bool* p_open, Model* model, Condition **sel_bc, DialogMode mode);  
};


#endif
