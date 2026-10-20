#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "part_dialog.h"

#include <iostream>


using namespace std;

void  PartDialog::Draw(const char* title, bool* p_open, Part *part){

  if (!m_initialized) {
      m_id = part->m_id;  // inicializamos la UI solo la primera vez
      m_initialized = true;
      m_v = part->getVel();
      m_flip_normals = part->getFlipNormals();
      const std::string policy = part->getNormalOrientation();
      const char* policies[] = {"Automatic (toward deformable centroid)", "topology", "outward", "towardPoint", "awayFromPoint", "preserve"};
      m_normal_orientation = 0;
      for (int i = 1; i < 6; ++i)
        if (policy == policies[i]) m_normal_orientation = i;
      const double3 point = part->getOrientPoint();
      m_orient_point[0] = point.x; m_orient_point[1] = point.y; m_orient_point[2] = point.z;
      if (part->getType() == Elastic)
        part_type = 0;
      else 
        part_type = 1;
        
      cout << "Not initialized "<<endl;
  }
    
  
  create_part = false; 
  if (!ImGui::Begin(title, p_open))
  {
      ImGui::End();
      return;
  }
  //Vec3_t size;

  ImGui::InputInt("Id ", &m_id, 1,10);  

  if (ImGui::RadioButton("Deformable", part_type == 0)) {
      part_type = 0;
  }
  if (ImGui::RadioButton("Rigid", part_type == 1)) {
      part_type = 1;
  }
  
  
  if (part_type == 1) {
    ImGui::Separator();
    ImGui::Checkbox("Flip normals", &m_flip_normals);
    const char* policies[] = {"Automatic (toward deformable centroid)", "topology", "outward", "towardPoint", "awayFromPoint", "preserve"};
    ImGui::Combo("Normal orientation", &m_normal_orientation, policies, 6);
    if (m_normal_orientation == 3 || m_normal_orientation == 4) {
      float point[3] = {static_cast<float>(m_orient_point[0]), static_cast<float>(m_orient_point[1]), static_cast<float>(m_orient_point[2])};
      if (ImGui::InputFloat3("Orientation point", point, "%.6g"))
        for (int i = 0; i < 3; ++i) m_orient_point[i] = point[i];
    }
  }
  float vv[3] = {m_v.x,m_v.y,m_v.z};

  ImGui::InputFloat3("Velocity", vv, "%.4f");
  //copying back;
    m_v.x = vv[0];
    m_v.y = vv[1];
    m_v.z = vv[2];
  
  //~ ImGui::InputDouble("Elastic Mod", &m_elastic_const, 0.0f, 1.0f, "%.2e");  
  //~ ImGui::InputDouble("Poisson Mod", &m_poisson_const, 0.0f, 1.0f, "%.2e");  

  if (ImGui::Button("Ok")) {
    //create_material = true;
    part->m_id = m_id;
    cout << "m_id"<<m_id<<endl;
    m_initialized = false;  // reset para la próxima vez
    part->setVel(m_v);
    part->setType(part_type);
    part->setFlipNormals(part_type == 1 && m_flip_normals);
    const char* policies[] = {"", "topology", "outward", "towardPoint", "awayFromPoint", "preserve"};
    part->setNormalOrientation(part_type == 1 ? policies[m_normal_orientation] : "");
    part->setOrientPoint(make_double3(m_orient_point[0], m_orient_point[1], m_orient_point[2]));
    *p_open = false;

    }
  ImGui::SameLine();
  if (ImGui::Button("Cancel")) {
    m_initialized = false;  // cancelamos cambios

    *p_open = false;    //cancel_action = false;
  }
  ImGui::End();
  

}

Part ShowCreatePartDialog(bool* p_open, PartDialog *matdlg, bool *create){
  
  Part ret;
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

bool ShowEditPartDialog(bool* p_open, PartDialog *prtdlg, Part *part){
  bool ret = true;
  ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
  prtdlg->Draw("Part", p_open, part);

  return ret;
  
}
