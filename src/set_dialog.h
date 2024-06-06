#ifndef _SET_DIALOG_H_
#define _SET_DIALOG_H_


#include "Material.h"
#include "Dialog.h"
#include "Set.h"

//SAME DIALOG FROM CREATE AND EDIT MATERIAL
// IS BASICALLY THE SAME 

class CreateSetDialog:
public Dialog { 
public:
  CreateSetDialog(){ }
template <typename T>
void  Draw(const char* title, bool* p_open, Set<T> *mat);
void Draw(){}

protected:  
  bool create_set;
  bool cancel_action;
  
  // const bool & isMaterialCreated()const{return create_material;}
  // void   Draw(const char* title, bool* p_open = NULL, Material_* mat = NULL);  
};

// //Returns true if NEW material is created or if changes are saved, if no
// //if no material is created, pointer is null
// Material_ ShowCreateMaterialDialog(bool* p_open, MaterialDialog *, bool* ret);
// bool ShowEditMaterialDialog(bool* p_open, MaterialDialog *, Material_ *);


#endif