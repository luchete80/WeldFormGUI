#ifndef _DIALOG_H_
#define _DIALOG_H_


#include "Material.h"

//SAME DIALOG FROM CREATE AND EDIT MATERIAL
// IS BASICALLY THE SAME 
class Dialog{
public:
  Dialog (){m_isopen = false;}
  bool isOpen(){return m_isopen;}
  virtual void Draw(){};
  
protected:

  bool m_isopen;
  // const bool & isMaterialCreated()const{return create_material;}
  // void   Draw(const char* title, bool* p_open = NULL, Material_* mat = NULL);  
};

// //Returns true if NEW material is created or if changes are saved, if no
// //if no material is created, pointer is null
// Material_ ShowCreateMaterialDialog(bool* p_open, MaterialDialog *, bool* ret);
// bool ShowEditMaterialDialog(bool* p_open, MaterialDialog *, Material_ *);


#endif