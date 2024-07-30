#ifndef _ENTITY_DIALOG_H_
#define _ENTITY_DIALOG_H_


//SAME DIALOG FROM CREATE AND EDIT MATERIAL
// IS BASICALLY THE SAME 
template <typename T> 
struct EntityDialog{
  
  //void    AddLog(const char* fmt, ...);
  double m_density_const; //TODO; CHANGE TO VECTOR (TEMP FUNCTION)
  double m_elastic_const;
  double m_poisson_const;
  
  bool cancel_action;
  bool create_entity;
  
  const bool & isEntityCreated()const{return create_entity;}
  virtual void Draw(const char* title, bool* p_open = NULL, T* entity = NULL);  
};

//Returns true if NEW material is created or if changes are saved, if no
//if no material is created, pointer is null
template <typename T>
T ShowCreateEntityDialog(bool* p_open, EntityDialog<T> *, bool* ret){}

template <typename T>
bool ShowEditEntityDialog(bool* p_open, EntityDialog<T> *, T *){}


// Material_ ShowCreateMaterialDialog(bool* p_open, MaterialDialog *matdlg, bool *create){
  
  // Material_ ret;
  // ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
  // // ImGui::Begin("test", p_open);
  // // ImGui::End();
  
  // matdlg->Draw("Material", p_open);
  
  // if (matdlg->isMaterialCreated()){
    // *create =true;
    // ret.setDensityConstant(matdlg->m_density_const);
    
    // if (matdlg->m_elastic_const != 0.0 && matdlg->m_elastic_const != 0.0){
      // Elastic_ el(matdlg->m_elastic_const ,matdlg->m_elastic_const);
      // ret = Material_(el);
    // } else {
      // cout << "Material elastic constants should not be zero;"<<endl;
    // };
  // }
  // //cout << "density "<<    ret.getDensityConstant()<<endl;
  // return ret;
// }

// bool ShowEditMaterialDialog(bool* p_open, MaterialDialog *matdlg, Material_ *mat){
  // ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
  // matdlg->Draw("Material", p_open, mat);

    
  
// }

#endif