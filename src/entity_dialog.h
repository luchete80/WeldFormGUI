#ifndef _ENTITY_DIALOG_H_
#define _ENTITY_DIALOG_H_


//SAME DIALOG FROM CREATE AND EDIT MATERIAL
// IS BASICALLY THE SAME 
// template <typename T> 
// class EntityDialog{
  
  // // //void    AddLog(const char* fmt, ...);

  
  // bool cancel_action;
  // bool create_entity;
  
  // const bool & isEntityCreated()const{return create_entity;}
  // virtual void Draw(const char* title, bool* p_open = NULL, T* entity = NULL){entity->Draw();} 
// };

//Returns true if NEW material is created or if changes are saved, if no
//if no material is created, pointer is null
// template <typename T>
// T ShowCreateEntityDialog(bool* p_open, EntityDialog<T> *, bool* ret){}

// template <typename T>
// bool ShowEditEntityDialog(bool* p_open, EntityDialog<T> *, T *);


class EntityDialog{
  
  // //void    AddLog(const char* fmt, ...);

  
  bool cancel_action;
  bool create_entity;
  
  void CheckAndCreateEntity();
  const bool & isEntityCreated()const{return create_entity;}
  //virtual void Draw(const char* title, bool* p_open = NULL, T* entity = NULL){entity->Draw();} 
};



#endif