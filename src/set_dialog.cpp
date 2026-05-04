#include "set_dialog.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <iostream>

using namespace std;

CreateSetDialog::CreateSetDialog()
{
  set_type = NODE_SET;
  create_set = false;
  cancel_action = false;
  m_saved = false;
  m_cancelled = false;
  m_name[0] = '\0';
}

void CreateSetDialog::reset()
{
  create_set = false;
  cancel_action = false;
  m_saved = false;
  m_cancelled = false;
  m_name[0] = '\0';
  set_type = NODE_SET;
}


CreateSetTypeDialog::  CreateSetTypeDialog(const char* title, bool* p_open, int *set_type, Model *model){ 
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
  if (model->getModelType() == SPH_Model){
    ImGui::RadioButton("Particle", set_type, 3); ImGui::SameLine();

  } else {
   ImGui::BeginDisabled();
  ImGui::RadioButton("Particle", set_type, 3); ImGui::SameLine();
    ImGui::EndDisabled();    
  }
  if (ImGui::Button("Cancel")) {
    cancel_action = false;
  }
  ImGui::End();
}
  
  
  
void CreateSetDialog::Draw(const char* title, bool* p_open, int selected_count){
  if (!ImGui::Begin(title, p_open))
  {
      ImGui::End();
      return;
  }

  const char* setTypeLabel = "Unknown";
  if (set_type == NODE_SET) setTypeLabel = "Node Set";
  else if (set_type == ELEM_SET) setTypeLabel = "Element Set";
  else if (set_type == PARTICLE_SET) setTypeLabel = "Particle Set";

  ImGui::InputText("Name", m_name, IM_ARRAYSIZE(m_name));
  ImGui::Text("Type: %s", setTypeLabel);
  ImGui::Text("Selected items: %d", selected_count);

  const bool canCreate = selected_count > 0 && m_name[0] != '\0';
  if (!canCreate)
    ImGui::BeginDisabled();
  if (ImGui::Button("Create")) {
    create_set = true;
    m_saved = true;
    m_cancelled = false;
    if (p_open != nullptr)
      *p_open = false;
  }
  if (!canCreate)
    ImGui::EndDisabled();

  ImGui::SameLine();
  if (ImGui::Button("Cancel")) {
    cancel_action = true;
    m_cancelled = true;
    m_saved = false;
    if (p_open != nullptr)
      *p_open = false;
  }
  ImGui::End();
}
