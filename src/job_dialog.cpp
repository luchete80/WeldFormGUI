
#include "job_dialog.h"
#include <iostream>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "ImGuiFileDialog.h"

using namespace std;

void  JobDialog::Draw(){

  ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
  //create_job = false; 
  if (!ImGui::Begin("Create Job", nullptr))
  {
      cout << "error drawing" <<endl;
      ImGui::End();
      return;
  }
  //cout << "drawing "<<endl;
  //Vec3_t size;
  double test;
  if (ImGui::Button("Choose File")){
    ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgImportJob", "Choose File", ".json,.k", ".");
    show_job_files = true;
      }
      
  if (show_job_files) {
    //if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgImport")) 
    
    {
      
    }

  if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgImportJob")) 
  {
    // action if OK
    if (ImGuiFileDialog::Instance()->IsOk())
    {
      m_filename = ImGuiFileDialog::Instance()->GetFilePathName();
      std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
      create_entity = true;
      cout << "file path name "<<m_filename<<endl;
      // m_model = new Model(filePathName);
      // m_renderer.addMesh(m_model->getPartMesh(0));
      // is_fem_mesh = true;
      // action
    }
    
    // close
    ImGuiFileDialog::Instance()->Close();
  }

  }
//  ImGui::InputDouble("Density ", &test, 0.00f, 1.0f, "%.4f");  

  if (ImGui::Button("Create")) {create_entity = true;}
  ImGui::SameLine();
  if (ImGui::Button("Cancel")) {
    cancel_action = false;
  }
  
    if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgImportJob")) 
  {
    // action if OK
    if (ImGuiFileDialog::Instance()->IsOk())
    {
      std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
      std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
      
      // cout << "file path name "<<filePathName<<endl;
      // m_model = new Model(filePathName);
      // m_renderer.addMesh(m_model->getPartMesh(0));
      // is_fem_mesh = true;
      // action
    }
    
    // close
    ImGuiFileDialog::Instance()->Close();
  }
  
  
  ImGui::End();
  
  //m_density_const = dens;
}

void JobShowDialog::Draw(){
  ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
  //create_job = false; 
  if (!ImGui::Begin("Job Progress", nullptr)){
    ImGui::End();
    
  
  }
  
  //cout << "viewing"<<endl;
  string str = m_job->getPathFile();
  ImGui::Text("Path: %s", str.c_str()  );
  

  str = m_job->getLog();

  ImGui::Text("State %s", str.c_str()  );  
  //ImGui::TextUnformatted();

  
  if (ImGui::Button("Close")){m_show=false;}

  ImGui::End();
}
/*
void JobShowDialog::Draw(const char* title, bool* p_open, Job *job){


}
*/

// Job ShowCreateJobDialog(bool* p_open, JobDialog *jobdlg, bool* ret){
  // Job job;

  // jobdlg->Draw("Job", p_open);
  
  // return job;
// }

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


