#include "mesh_dialog.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"


#include <iostream>


using namespace std;

void MeshDialog::Draw(const char* title, bool* p_open, Part *part){
  
  
  if (!m_initialized) {
      m_id = part->getId();
      m_initialized = true;
      //m_v = part->getVel();
      
      // Inicializar el tamaño de elemento desde la parte
      //m_element_size = part->getElementSize(); // Asume que Part tiene este método
      m_element_size = 0.001;
      
      if (part->getType() == Elastic)
        part_type = 0;
      else 
        part_type = 1;
  }
  
  create_part = false; 
  
  
  
  if (!ImGui::Begin(title, p_open)) {
      ImGui::End();
      return;
  }



  ImGui::InputInt("Id ", &m_id, 1, 10);  

  // Radio buttons para tipo
  if (ImGui::RadioButton("Deformable", part_type == 0)) {
      part_type = 0;
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("Rigid", part_type == 1)) {
      part_type = 1;
  }

  // Velocidad
  float vv[3] = {m_v.x, m_v.y, m_v.z};
  ImGui::InputFloat3("Velocity", vv, "%.4f");
  m_v.x = vv[0];
  m_v.y = vv[1];
  m_v.z = vv[2];
  
  // NUEVO: Deslizador para tamaño de elemento
  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Text("Mesh Settings");
  
  // Define un rango razonable para el tamaño de elemento
  float min_size = 0.01f;
  float max_size = 10.0f;
  
  ImGui::SliderFloat("Element Size", &m_element_size, min_size, max_size, "%.3f", ImGuiSliderFlags_Logarithmic);
  ImGui::SameLine();
  //HelpMarker("Smaller values = finer mesh, larger values = coarser mesh");
  
  // Input numérico para mayor precisión
  ImGui::InputFloat("##ElementSizeInput", &m_element_size, 0.1f, 1.0f, "%.3f");
  ImGui::SameLine();
  if (ImGui::Button("Default##Size")) {
      m_element_size = 1.0f; // Valor por defecto
  }

  // Botones Ok/Cancel
  ImGui::Spacing();
  ImGui::Separator();
  
  if (ImGui::Button("Ok")) {
    part->setId(m_id);
    part->setVel(m_v);
    part->setType(part_type);
    //part->setElementSize(m_element_size); // Guardar el tamaño de elemento
    
    m_initialized = false;
    *p_open = false;
  }
  
  ImGui::SameLine();
  if (ImGui::Button("Cancel")) {
    m_initialized = false;
    *p_open = false;
  }
  
  ImGui::End();
}
