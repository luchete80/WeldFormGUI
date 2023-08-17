#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "material_dialog.h"

#include <iostream>


using namespace std;

void  MaterialDialog::Draw(const char* title, bool* p_open){

  if (!ImGui::Begin(title, p_open))
  {
      ImGui::End();
      return;
  }
  //Vec3_t size;
  static double size[] = {0.1,0.1,0.1};
  static double young = 200.e9;
  ImGui::InputDouble("Density ", &size[0], 0.01f, 1.0f, "%.4f");  
  ImGui::InputDouble("Elastic Mod", &young, 0.01f, 1.0f, "%.2e");  

  if (ImGui::Button("Create")) {}
  ImGui::SameLine();
  if (ImGui::Button("Cancel")) {}
  ImGui::End();
}

Material_ ShowMaterialDialog(bool* p_open, MaterialDialog *matdlg){
  
  Material_ mat;
  
  ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
  // ImGui::Begin("test", p_open);
  // ImGui::End();
  matdlg->Draw("Material", p_open);
  return mat;
}