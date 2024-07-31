
#include "job_dialog.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"


void  JobDialog::Draw(const char* title, bool* p_open, Job *job){

  //create_job = false; 
  // if (!ImGui::Begin(title, p_open))
  {
      ImGui::End();
      return;
  }
  //Vec3_t size;

  // ImGui::InputDouble("Density ", &m_density_const, 0.00f, 1.0f, "%.4f");  

  // if (ImGui::Button("Create")) {create_material = true;}
  // ImGui::SameLine();
  // if (ImGui::Button("Cancel")) {
    // cancel_action = false;
  // }
  // ImGui::End();
  
  //m_density_const = dens;
}

Job ShowCreateJobDialog(bool* p_open, JobDialog *, bool* ret){
  
}

// //SAME DIALOG FROM CREATE AND EDIT MATERIAL
// // IS BASICALLY THE SAME 
// struct MaterialDialog{
  
  // //void    AddLog(const char* fmt, ...);
  // double m_density_const; //TODO; CHANGE TO VECTOR (TEMP FUNCTION)
  // double m_elastic_const;
  // double m_poisson_const;
  
  // bool cancel_action;
  // bool create_material;
  
  // const bool & isJobCreated()const{return create_material;}
  // void   Draw(const char* title, bool* p_open = NULL, Material_* mat = NULL);  
// };

// //Returns true if NEW material is created or if changes are saved, if no
// //if no material is created, pointer is null
// Material_ ShowCreateMaterialDialog(bool* p_open, MaterialDialog *, bool* ret);
// bool ShowEditMaterialDialog(bool* p_open, MaterialDialog *, Material_ *);


//template  void EntityDialog<Job>:: Draw(const char* title, bool* p_open, Job *mat);


