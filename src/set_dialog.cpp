#include "set_dialog.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

template <typename T>
void CreateSetDialog::Draw(const char* title, bool* p_open, Set<T> *mat){

  create_set = false; 
  if (!ImGui::Begin(title, p_open))
  {
      ImGui::End();
      return;
  }
  //Vec3_t size;

  //ImGui::InputDouble("Density ", &m_density_const, 0.00f, 1.0f, "%.4f");  

  if (ImGui::Button("Create")) {create_set = true;}
  ImGui::SameLine();
  if (ImGui::Button("Cancel")) {
    cancel_action = false;
  }
  ImGui::End();
  
  //m_density_const = dens;
}