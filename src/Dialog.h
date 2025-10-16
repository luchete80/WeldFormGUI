#ifndef _DIALOG_H_
#define _DIALOG_H_


#include "Material.h"

//SAME DIALOG FROM CREATE AND EDIT MATERIAL
// IS BASICALLY THE SAME 
struct Dialog{

  Dialog (){m_isopen = false;}
  bool isOpen(){return m_isopen;}
  virtual void Draw(){};
  void ShowIfEnabled(){if (m_show) this->Draw();}
  
  void setShow(const bool &state){m_show = state;};
  


  bool m_isopen;
  bool m_show = false; //If true, then draw me
  
  // const bool & isMaterialCreated()const{return create_material;}
  // void   Draw(const char* title, bool* p_open = NULL, Material_* mat = NULL);  
};

//Class Involving entity object
struct ObjDialog:
public Dialog{

  void CheckAndCreateEntity();
  bool cancel_action;
  bool create_entity;
  
  const bool & isEntityCreated()const{return create_entity;}
  
};

// //Returns true if NEW material is created or if changes are saved, if no
// //if no material is created, pointer is null
// Material_ ShowCreateMaterialDialog(bool* p_open, MaterialDialog *, bool* ret);
// bool ShowEditMaterialDialog(bool* p_open, MaterialDialog *, Material_ *);


#endif
