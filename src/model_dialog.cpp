#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "model_dialog.h"

#include <iostream>


using namespace std;

void  ModelDialog::Draw(const char* title, bool* p_open, Model *model){

  if (!m_initialized) {
      //m_id = part->m_id;  // inicializamos la UI solo la primera vez
      m_initialized = true;
      if (model->getAnalysisType() == Solid3D)
        m_antype = Solid3D;
      else if (model->getAnalysisType() == Axisymmetric2D)
        m_antype = Axisymmetric2D;
        
      m_thermal_coupling_flag = model->m_thermal_coupling;
        
      cout << "Not initialized "<<endl;
  }
    
  
  create_part = false; 
  if (!ImGui::Begin(title, p_open))
  {
      ImGui::End();
      return;
  }
  //~ //Vec3_t size;

  //~ ImGui::InputInt("Id ", &m_id, 1,10);  

  ImGui::Checkbox("Thermal Coupling", &m_thermal_coupling_flag);
  
  if (ImGui::RadioButton("Solid 3D", m_antype == Solid3D)) {
      m_antype = Solid3D;
  }
  if (ImGui::RadioButton("AxiSymm 2D", m_antype == Axisymmetric2D)) {
      m_antype = Axisymmetric2D;
  }




  
  //~ ImGui::InputDouble("Elastic Mod", &m_elastic_const, 0.0f, 1.0f, "%.2e");  
  //~ ImGui::InputDouble("Poisson Mod", &m_poisson_const, 0.0f, 1.0f, "%.2e");  

  if (ImGui::Button("Ok")) {
    //create_material = true;
    //art->m_id = m_id;
    //cout << "m_id"<<m_id<<endl;
    m_initialized = false;  // reset para la próxima vez
    model->setAnalysisType(m_antype);
    if (m_thermal_coupling_flag)
      model->m_thermal_coupling = true;
    else
      model->m_thermal_coupling = false;
    
    //part->setType(part_type);
    *p_open = false;

    }
  ImGui::SameLine();
  if (ImGui::Button("Cancel")) {
    m_initialized = false;  // cancelamos cambios

    *p_open = false;    //cancel_action = false;
  }
  
  ImGui::End();
  

}

Model ShowCreateModelDialog(bool* p_open, ModelDialog *matdlg, bool *create){
  
  Model ret;
  //~ ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
  //~ // ImGui::Begin("test", p_open);
  //~ // ImGui::End();
  
  //~ matdlg->Draw("Material", p_open);
  
  //~ if (matdlg->isMaterialCreated()){
    //~ *create =true;
    //~ ret.setDensityConstant(matdlg->m_density_const);
    
    //~ if (matdlg->m_elastic_const != 0.0 && matdlg->m_elastic_const != 0.0){
      //~ Elastic_ el(matdlg->m_elastic_const ,matdlg->m_elastic_const);
      //~ ret = Material_(el);
    //~ } else {
      //~ cout << "Material elastic constants should not be zero;"<<endl;
    //~ };
  //~ }
  //cout << "density "<<    ret.getDensityConstant()<<endl;
  return ret;
}

bool ShowEditModelDialog(bool* p_open, ModelDialog *moddlg, Model *model){
  bool ret = true;
  ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
  moddlg->Draw("Part", p_open, model);

  return ret;
  
}
