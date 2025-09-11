#ifndef _MODEL_DIALOG_H_
#define _MODEL_DIALOG_H_


#include "Model.h"

//SAME DIALOG FROM CREATE AND EDIT MATERIAL
// IS BASICALLY THE SAME 
struct ModelDialog{
  
  //void    AddLog(const char* fmt, ...);
  int m_id; //TODO; CHANGE TO VECTOR (TEMP FUNCTION)
  double m_elastic_const;
  double m_poisson_const;
  AnalysisType m_antype = Solid3D ;
  
  bool m_initialized = false;
  
  bool cancel_action;
  bool create_part;
  
  const bool & isModelCreated()const{return create_part;}
  void   Draw(const char* title, bool* p_open = NULL, Model* prt = NULL);  
};

//Returns true if NEW material is created or if changes are saved, if no
//if no material is created, pointer is null
Model ShowCreateModelDialog(bool* p_open, ModelDialog *, bool* ret);
bool ShowEditModelDialog(bool* p_open, ModelDialog *, Model *);


#endif
