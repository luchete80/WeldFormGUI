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

  ImGui::InputDouble("Density ", &m_density_const, 0.00f, 1.0f, "%.4f");  
  ImGui::InputDouble("Elastic Mod", &m_elastic_const, 0.0f, 1.0f, "%.2e");  
  ImGui::InputDouble("Poisson Mod", &m_poisson_const, 0.0f, 1.0f, "%.2e");  

  if (ImGui::Button("Create")) {create_material = true;}
  ImGui::SameLine();
  if (ImGui::Button("Cancel")) {
    cancel_action = false;
  }
  ImGui::End();
  
  //m_density_const = dens;
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
    
    if (matdlg->m_elastic_const != 0.0 && matdlg->m_elastic_const != 0.0){
      Elastic_ el(matdlg->m_elastic_const ,matdlg->m_elastic_const);
      ret = Material_(el);
    } else {
      cout << "Material elastic constants should not be zero;"<<endl;
    };
  }
  //cout << "density "<<    ret.getDensityConstant()<<endl;
  return ret;
}

bool ShowEditMaterialDialog(bool* p_open, MaterialDialog *matdlg, Material_ *mat){
  
  
}