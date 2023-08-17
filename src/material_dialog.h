#ifndef _MATERIAL_DIALOG_H_
#define _MATERIAL_DIALOG_H_

class Editor;

#include "Material.h"

//SAME DIALOG FROM CREATE AND EDIT MATERIAL
// IS BASICALLY THE SAME 
struct MaterialDialog{
  
  //void    AddLog(const char* fmt, ...);
  double m_density_const;
  void    Draw(const char* title, bool* p_open = NULL);
  
  bool cancel_action;
  bool create_material;
  
  const bool & isMaterialCreated()const{return create_material;}
  
};

//Returns true if NEW material is created or if changes are saved, if no
//if no material is created, pointer is null
Material_ ShowCreateMaterialDialog(bool* p_open, MaterialDialog *, bool* ret);
bool ShowEditMaterialDialog(bool* p_open, MaterialDialog *, Material_ *);


#endif