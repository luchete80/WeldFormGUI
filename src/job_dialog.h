#ifndef _JOB_DIALOG_H_
#define _JOB_DIALOG_H_

#include "Job.h"


//template  EntityDialog<Job>;

//SAME DIALOG FROM CREATE AND EDIT MATERIAL
// IS BASICALLY THE SAME 
// FOR CREATION
struct JobDialog:
public ObjDialog{
  
   bool show_job_files;
    
  // //void    AddLog(const char* fmt, ...);
  JobDialog():
  ObjDialog(){
    show_job_files = false;
    create_entity = false;
  }
  
  std::string m_filename;
  
  //void Draw(const char* title, bool* p_open = NULL,  Job* entity = NULL); 
  void Draw();
};

struct JobShowDialog:
public ObjDialog{
  
  // //void    AddLog(const char* fmt, ...);
  
  Job *m_job;
  
  const bool & isEntityCreated()const{return create_entity;}  
  void Draw(const char* title, bool* p_open = NULL,  Job* entity = NULL); 
  void Draw();
};

Job ShowCreateJobDialog(bool* p_open, JobDialog *, bool* ret);
bool ShowEditJobDialog(bool* p_open, JobDialog *, Job *);

// template  
// class TestDialog:
// public EntityDialog<Job> {
  
  
// };

//void JobDialog<Job>::Draw(const char* title, bool* p_open = NULL, Job* entity = NULL){}

// //Returns true if NEW material is created or if changes are saved, if no
// //if no material is created, pointer is null
// template <typename T>
// T ShowCreateEntityDialog(bool* p_open, EntityDialog<T> *, bool* ret){}

// template <typename T>
// bool ShowEditEntityDialog(bool* p_open, EntityDialog<T> *, T *);


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