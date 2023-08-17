#ifndef _MATERIAL_DIALOG_H_
#define _MATERIAL_DIALOG_H_

class Editor;

#include "Material.h"

struct MaterialDialog{
  
  //void    AddLog(const char* fmt, ...);
  
  void    Draw(const char* title, bool* p_open = NULL);
  
};

Material_ ShowMaterialDialog(bool* p_open, MaterialDialog *);


#endif