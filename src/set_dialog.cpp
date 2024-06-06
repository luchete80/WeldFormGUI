#include "set_dialog.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <iostream>

using namespace std;



CreateSetTypeDialog::  CreateSetTypeDialog(const char* title, bool* p_open, int *set_type){ 
  create_set = false; 
  if (!ImGui::Begin(title, p_open))
  { cout << "test"<<endl;
      ImGui::End();
      return;
  }
  //Vec3_t size;
  //cout << "test"<<endl;
  //ImGui::InputDouble("Density ", &m_density_const, 0.00f, 1.0f, "%.4f");  

  if (ImGui::Button("Create")) {
    create_set = true;
    //*set_type=1;
    }
  ImGui::SameLine();
  static int e;
  ImGui::RadioButton("Geometry", set_type, 1); ImGui::SameLine();
  ImGui::RadioButton("Node", set_type, 2); ImGui::SameLine();
  if (ImGui::Button("Cancel")) {
    cancel_action = false;
  }
  ImGui::End();
}
  
  
  
  
template <typename T>
void CreateSetDialog::Draw(const char* title, bool* p_open, Set<T> *set){

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