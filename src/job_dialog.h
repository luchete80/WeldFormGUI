#ifndef _JOB_DIALOG_H_
#define _JOB_DIALOG_H_

#include "Job.h"
#include <array>
#include <functional>
#include <string>


//template  EntityDialog<Job>;

//SAME DIALOG FROM CREATE AND EDIT MATERIAL
// IS BASICALLY THE SAME 
// FOR CREATION
struct JobDialog:
public ObjDialog{
  
   bool show_job_files;
   bool m_edit_mode;
   Job *m_job;
   std::function<bool(Job*)> m_open_results;
    
  // //void    AddLog(const char* fmt, ...);
  JobDialog():
  ObjDialog(){
    show_job_files = false;
    create_entity = false;
    m_edit_mode = false;
    m_job = nullptr;
    resetRestartOptions();
  }
  
  std::string m_filename;
  int m_solver_edition = static_cast<int>(Job::SolverEdition::Auto);
  bool m_checkpoint_enabled = false;
  int m_checkpoint_interval = 1;
  std::string m_checkpoint_dir = ".";
  std::string m_checkpoint_prefix = "restart_qt";
  std::string m_restart_file;
  std::string m_result_base_name;
  bool m_show_restart_files = false;
  bool m_show_result_files = false;
  std::array<char, 512> m_checkpoint_dir_buffer{};
  std::array<char, 512> m_checkpoint_prefix_buffer{};
  std::array<char, 1024> m_restart_file_buffer{};
  std::array<char, 512> m_result_base_name_buffer{};

  void resetRestartOptions();
  void loadRestartOptionsFromJob(const Job* job);
  void loadRestartOptionsFromInputFile();
  bool inputSupportsImplicit3DRestart() const;
  
  //void Draw(const char* title, bool* p_open = NULL,  Job* entity = NULL); 
  void Draw();
};

struct JobShowDialog:
public ObjDialog{
  
  // //void    AddLog(const char* fmt, ...);
  JobShowDialog(){
    m_job = nullptr;
    m_last_job = nullptr;
    m_last_refresh_time = -1.0;
    m_max_visible_lines = 100;
  }
  Job *m_job;
  Job *m_last_job;
  std::function<bool(Job*)> m_open_results;
  double m_last_refresh_time;
  int m_max_visible_lines;
  
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
