#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "move_part_dialog.h"

#include <iostream>


using namespace std;

void  MovePartDialog::Draw(Part *model){

  //~ if (!m_initialized) {
      //~ //m_id = part->m_id;  // inicializamos la UI solo la primera vez
      //~ m_initialized = true;
      //~ if (model->getAnalysisType() == Solid3D)
        //~ m_antype = Solid3D;
      //~ else if (model->getAnalysisType() == Axisymmetric2D)
        //~ m_antype = Axisymmetric2D;
        
      //~ cout << "Not initialized "<<endl;
  //~ }
    
  
  if (!ImGui::Begin("Moving Part"))
  {
      ImGui::End();
      return;
  }


  //~ if (ImGui::RadioButton("Solid 3D", m_antype == Solid3D)) {
      //~ m_antype = Solid3D;
  //~ }
  //~ if (ImGui::RadioButton("AxiSymm 2D", m_antype == Axisymmetric2D)) {
      //~ m_antype = Axisymmetric2D;
  //~ }


  
  ImGui::InputDouble("Elastic Mod", &m_elastic_const, 0.0f, 1.0f, "%.2e");  
  ImGui::InputDouble("Poisson Mod", &m_poisson_const, 0.0f, 1.0f, "%.2e");  

  //~ if (ImGui::Button("Ok")) {
    //~ //create_material = true;
    //~ //art->m_id = m_id;
    //~ //cout << "m_id"<<m_id<<endl;
    //~ m_initialized = false;  // reset para la próxima vez
    //~ model->setAnalysisType(m_antype);
    
    //~ //part->setType(part_type);
    //~ *p_open = false;

    //~ }
  //~ ImGui::SameLine();
  if (ImGui::Button("Cancel")) {
    m_initialized = false;  // cancelamos cambios

    //*p_open = false;    //cancel_action = false;
  }
  
  ImGui::End();
  

}

