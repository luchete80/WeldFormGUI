#ifndef _MATERIAL_DIALOG_H_
#define _MATERIAL_DIALOG_H_


#include "Material.h"
#include "Dialog.h"

//SAME DIALOG FROM CREATE AND EDIT MATERIAL
// IS BASICALLY THE SAME 
struct MaterialDialog:
public Dialog{
  
  //void    AddLog(const char* fmt, ...);
  double m_density_const; //TODO; CHANGE TO VECTOR (TEMP FUNCTION)
  double m_elastic_const;
  double m_poisson_const;
  
  bool cancel_action;
  bool create_material;
  
  const bool & isMaterialCreated()const{return create_material;}
  void   Draw(const char* title, bool* p_open = NULL, Material_* mat = NULL);  
};


//Returns true if NEW material is created or if changes are saved, if no
//if no material is created, pointer is null
Material_ ShowCreateMaterialDialog(bool* p_open, MaterialDialog *, bool* ret);
bool ShowEditMaterialDialog(bool* p_open, MaterialDialog *, Material_ *);


#endif
