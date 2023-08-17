#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "material_dialog.h"

#include <iostream>


using namespace std;

void  MaterialDialog::Draw(const char* title, bool* p_open){
  
  create_material = false; 
  if (!ImGui::Begin(title, p_open))
  {
      ImGui::End();
      return;
  }
  //Vec3_t size;
  double dens = 7850.0;
  double young = 200.e9;
  ImGui::InputDouble("Density ", &dens, 0.01f, 1.0f, "%.4f");  
  cout << "dens "<< dens<<endl;
  
  ImGui::InputDouble("Elastic Mod", &young, 0.01f, 1.0f, "%.2e");  

  if (ImGui::Button("Create")) {create_material = true;}
  ImGui::SameLine();
  if (ImGui::Button("Cancel")) {
    cancel_action = false;
  }
  ImGui::End();
  
  m_density_const = dens;
}

Material_ ShowCreateMaterialDialog(bool* p_open, MaterialDialog *matdlg, bool *create){
  
  Material_ ret;
  ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
  // ImGui::Begin("test", p_open);
  // ImGui::End();
  
  matdlg->Draw("Material", p_open);
  
  if (matdlg->isMaterialCreated()){
    *create =true;
    ret.setDensityConstant(matdlg->m_density_const);
    }
  //cout << "density "<<    ret.getDensityConstant()<<endl;
  return ret;
}

bool ShowMaterialDialog(bool* p_open, MaterialDialog *matdlg, Material_ *mat);