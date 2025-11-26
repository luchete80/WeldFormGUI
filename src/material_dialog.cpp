#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "material_dialog.h"

#include <iostream>


using namespace std;

void MaterialDialog::InitFromMaterial(Material_* mat) {


    if (!mat) {
        // Valores por defecto para creación
        m_density_const = 1.0;          // elige un valor razonable
        m_elastic_const = 2.1e11;      // ejemplo
        m_poisson_const = 0.3;
        m_pl = nullptr;
        m_selected_model = 0;
        m_pl_const.clear();
        // inicializa variables específicas
        bilinear_sy0 = 0.0;
        bilinear_Et = 0.0;
        hollomon_K = 0.0;
        hollomon_n = 0.0;
        // etc...
        
        return;
    }
    
    else {
    m_density_const = mat->getDensityConstant();
    m_elastic_const = mat->Elastic().E();
    m_poisson_const =  mat->Elastic().nu();

    m_thermal_coupling_flag= mat->thermalCoupling;
    m_cp_T= mat->cp_T;
    m_k_T = mat->k_T;   

        
    if (mat->isPlastic()) {
        m_pl = mat->getPlastic()->clone(); // su propio clon (virtual)
        m_selected_model = m_pl->getType(); // p.ej. GMT, Hollomon, etc.
        m_pl_const = m_pl->getPlasticConstants();
        cout << "Plastic Constants"<<endl;
        for (int i=0;i<m_pl_const.size();i++)
          cout << m_pl_const[i]<<", ";
        cout <<endl;

        // Ahora asigno a las variables específicas según el tipo de material
        switch (m_selected_model) {
            case BILINEAR:
                bilinear_sy0 = m_pl_const[0];
                bilinear_Et  = m_pl_const[1];
                break;

            case HOLLOMON:
                cout << "Material is Hollomon"<<endl;
                hollomon_K = m_pl_const[0];
                hollomon_n = m_pl_const[1];
                break;

            case _GMT_:
                gmt_n1 = m_pl_const[0];
                gmt_n2 = m_pl_const[1];
                gmt_m1 = m_pl_const[2];
                gmt_m2 = m_pl_const[3];
                gmt_I1 = m_pl_const[4];
                gmt_I2 = m_pl_const[5];
                break;

            // otros modelos...
        }

    } else {
        m_pl = nullptr;
        m_selected_model = 0;
    }
    
  }
}

void  MaterialDialog::Draw(const char* title, bool* p_open, Material_ *mat, Material_Db *mat_db){

  static int item_current = 0;
  static bool open_plastic = false;
  
  ImGuiTreeNodeFlags flags = 0;
    
  create_material = false; 
  cancel_action = false;
  if (!m_initiated){
    InitFromMaterial(mat);
    item_current = m_selected_model;
    if (mat != nullptr)
    if (mat->isPlastic()){
      open_plastic = true;
      flags |= ImGuiTreeNodeFlags_DefaultOpen;
      ImGui::SetNextItemOpen(true, ImGuiCond_Always);
      cout << "DetaultOpen"<<endl;
    }
    m_initiated = true;
  }
  if (!ImGui::Begin(title, p_open))
  {
      ImGui::End();
      return;
  }
  //Vec3_t size;

  ImGui::InputDouble("Density ", &m_density_const, 0.00f, 1.0f, "%.4f");  
  ImGui::InputDouble("Elastic Mod", &m_elastic_const, 0.0f, 1.0f, "%.2e");  
  ImGui::InputDouble("Poisson Mod", &m_poisson_const, 0.0f, 1.0f, "%.2e");  

  const char* items[] = { "None", "Bilinear", "Hollomon", "Johnson Cook", "GMT", "Sinh"};

  ImGui::Checkbox("Thermal Coupling", &m_thermal_coupling_flag);

  if (m_thermal_coupling_flag) {
      ImGui::InputDouble("Heat Capacity (cp)", &m_cp_T, 0.0, 1.0, "%.2f");
      ImGui::InputDouble("Thermal Conductivity (k)", &m_k_T, 0.0, 1.0, "%.2f");
  } else {
      // Opcional: mostrar los campos como grises
      ImGui::BeginDisabled();
      ImGui::InputDouble("Heat Capacity (cp)", &m_cp_T, 0.0, 1.0, "%.2f");
      ImGui::InputDouble("Thermal Conductivity (k)", &m_k_T, 0.0, 1.0, "%.2f");
      ImGui::EndDisabled();
  }


  if (ImGui::CollapsingHeader("Plastic"/*, flags*/)){
    
    {
  //MUST BE SAVED CURRENT STATE
  ImGui::Combo("Yield Criteria", &item_current, items, IM_ARRAYSIZE(items) ) ;
  // if (ImGui::Button("Hollomon")){
        // Aquí verificas qué opción está seleccionada
    if (item_current == 0) { 
      //NONE
    }

    if (item_current == 1) { //BILINEAR
    
      
      ImGui::InputDouble("Yield Stress ", &bilinear_sy0, 0.00f, 1.0f, "%.4f");   
      ImGui::InputDouble("Tangent mod ", &bilinear_Et, 0.00f, 1.0f, "%.4f");   
                  
    }
    if (item_current == 2) { // Hollomon
    
      //ImGui::InputDouble("K ", &mat->, 0.00f, 1.0f, "%.4f");  
      ImGui::InputDouble("K ", &hollomon_K, 0.00f, 1.0f, "%.4f");   
      ImGui::InputDouble("n ", &hollomon_n, 0.00f, 1.0f, "%.4f");   
            
    }
    if (item_current == 3) { // Johnson Cook
      //~ ImGui::InputDouble("A ", &m_gmt.n1, 0.00f, 1.0f, "%.4f");  
      //~ ImGui::InputDouble("B ", &m_gmt.n2, 0.00f, 1.0f, "%.4f");
        
      //~ ImGui::InputDouble("n ", &m_gmt.m1, 0.00f, 1.0f, "%.4f");  
      //~ ImGui::InputDouble("C ", &m_gmt.m2, 0.00f, 1.0f, "%.4f");  

      //~ ImGui::InputDouble("I1 ", &m_gmt.m1, 0.00f, 1.0f, "%.4f");  
      //~ ImGui::InputDouble("I2 ", &m_gmt.m2, 0.00f, 1.0f, "%.4f");  
                      
    }
    if (item_current == 4) { // GMT 

      ImGui::InputDouble("n1 ", &m_gmt.n1, 0.00f, 1.0f, "%.4f");  
      ImGui::InputDouble("n2 ", &m_gmt.n2, 0.00f, 1.0f, "%.4f");
        
      ImGui::InputDouble("m1 ", &m_gmt.m1, 0.00f, 1.0f, "%.4f");  
      ImGui::InputDouble("m2 ", &m_gmt.m2, 0.00f, 1.0f, "%.4f");  

      ImGui::InputDouble("I1 ", &m_gmt.m1, 0.00f, 1.0f, "%.4f");  
      ImGui::InputDouble("I2 ", &m_gmt.m2, 0.00f, 1.0f, "%.4f");  
                  
    }
          
    
  // }
  }//Yield Criteria
  }//Plastic


  if (ImGui::Button("Save to Database")) {
      if (mat &&mat_db->isActive()) {
          mat_db->addMaterial(mat);
          mat_db->saveToJson("materials_db.json");
      }
  }
  
  if (ImGui::Button("Ok")) {
    cout << "Created material "<<endl;
    create_material = true;
    *p_open = false;

    m_selected_model = item_current;

    if (!mat) {
      if (!m_temp_mat) {
        m_temp_mat = new Material_();
        mat = m_temp_mat;
        cout << "CREATED MATERIAL"<<endl;
      }
    }
    
    if (mat) {
        mat->setDensityConstant(m_density_const);
        mat->elastic_m = Elastic_(m_elastic_const,m_poisson_const);
        //mat->E = m_elastic_const;
        //mat->nu = m_poisson_const;

        mat->thermalCoupling = m_thermal_coupling_flag;
        mat->cp_T = m_cp_T;
        mat->k_T = m_k_T;

        if (m_selected_model != 0) {
            // Liberar el modelo previo
          
          cout << "New Plastic Rule: ";
          switch (m_selected_model){

          case BILINEAR:
            m_pl = new Bilinear(bilinear_sy0,bilinear_Et);
            cout << "Bilinear"<<endl;
            break;

          case HOLLOMON:
            m_pl = new Hollomon(hollomon_K,hollomon_n);
            cout << "Hollomon"<<endl;
            break;
            
          case JOHNSON_COOK:
            m_pl = new JohnsonCook();
            cout << "Johnson Cook"<<endl;
            break;
            
          case _GMT_:
            cout << "GMT"<<endl;
            break;
            
            
            default:
              break;  
          }
          cout <<endl;
          
            delete mat->m_plastic;
            // Clonar o copiar el modelo temporal
            if (m_pl){
              mat->m_plastic = m_pl->clone();
            } else {
              m_pl = nullptr;
              cout << "ERROR: No plastic"<<endl;
            }

            mat->m_isplastic = true;

        } else {
            cout << "Set material to elastic."<<endl;
            if (mat->m_plastic)
              delete mat->m_plastic;
            mat->m_plastic = nullptr;
            mat->m_isplastic = false;
        }
    }
        
    
  }//OK Button
  ImGui::SameLine();
  if (ImGui::Button("Cancel")) {
    cancel_action = true;
    *p_open = false;
  }
  ImGui::End();
  
  //m_density_const = dens;
}

Material_ ShowCreateMaterialDialog(bool* p_open, MaterialDialog *matdlg, bool *create, Material_Db *mat_db){
  
  Material_ ret;
  ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
  // ImGui::Begin("test", p_open);
  // ImGui::End();
  
  //matdlg->Draw("Material", p_open);
  
  matdlg->Draw("Material", p_open, nullptr, mat_db);
  
  if (matdlg->isMaterialCreated()){
    *create =true;
    ret.setDensityConstant(matdlg->m_density_const);
    
    if (matdlg->m_elastic_const != 0.0 && matdlg->m_elastic_const != 0.0){
      Elastic_ el(matdlg->m_elastic_const ,matdlg->m_elastic_const);
      ret = Material_(el);
      if (matdlg->m_pl != nullptr)
        //ret.m_plastic = new Plastic_(*matdlg->m_pl);
        ret.m_plastic = matdlg->m_pl;
        ret.m_isplastic = true;
    } else {
      cout << "Material elastic constants should not be zero;"<<endl;
    };
  } else if (matdlg->cancel_action){
    
  }
  //cout << "density "<<    ret.getDensityConstant()<<endl;
  return ret;
}

bool ShowEditMaterialDialog(bool* p_open, MaterialDialog *matdlg, Material_ *mat){
  ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
  matdlg->Draw("MaterialDlg", p_open, mat);

    
  return true;
}
