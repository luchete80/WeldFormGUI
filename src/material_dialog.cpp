#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"

#include "material_dialog.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>


using namespace std;

namespace {

constexpr int kMaterialCurveSamples = 256;

double ClampNonNegative(double value) {
    return value < 0.0 ? 0.0 : value;
}

bool BuildHollomonCurve(const MaterialDialog& dialog,
                        std::vector<double>& strain_values,
                        std::vector<double>& stress_values,
                        double& eps0,
                        double& eps1,
                        double& eps_max,
                        double& sigma_max) {
    strain_values.clear();
    stress_values.clear();

    const double elastic_modulus = dialog.m_elastic_const;
    const double yield_stress0 = dialog.m_yield_stress0;
    const double K = dialog.hollomon_K;
    const double m = dialog.hollomon_n;

    eps0 = 0.0;
    eps1 = 0.0;
    eps_max = std::max(dialog.m_strain_range_min, dialog.m_strain_range_max);
    sigma_max = 0.0;

    if (elastic_modulus <= 0.0 || yield_stress0 <= 0.0 || K <= 0.0 || m <= 0.0) {
        return false;
    }

    Material_ preview_material;
    preview_material.strRange = {
        ClampNonNegative(std::min(dialog.m_strain_range_min, dialog.m_strain_range_max)),
        std::max(dialog.m_strain_range_min, dialog.m_strain_range_max)
    };
    preview_material.InitHollomon(Elastic_(elastic_modulus, dialog.m_poisson_const), yield_stress0, K, m);

    eps0 = preview_material.eps0;
    eps1 = preview_material.eps1;
    eps_max = std::max(preview_material.e_max, eps0);
    eps_max = std::max(eps_max, eps1);

    sigma_max = K * std::pow(eps_max, m);
    if (sigma_max < yield_stress0) {
        sigma_max = yield_stress0;
    }

    strain_values.reserve(kMaterialCurveSamples + 1);
    stress_values.reserve(kMaterialCurveSamples + 1);

    const double strain_start = ClampNonNegative(std::min(dialog.m_strain_range_min, dialog.m_strain_range_max));
    const double strain_end = std::max(eps_max, strain_start + 1.0e-9);
    const double delta = (strain_end - strain_start) / static_cast<double>(kMaterialCurveSamples);

    for (int i = 0; i <= kMaterialCurveSamples; ++i) {
        const double strain = strain_start + delta * static_cast<double>(i);
        double stress = 0.0;

        if (strain <= eps0) {
            stress = elastic_modulus * strain;
        } else if (strain <= eps1) {
            stress = yield_stress0;
        } else if (strain <= eps_max) {
            stress = K * std::pow(strain, m);
        } else {
            stress = sigma_max;
        }

        if (strain > eps_max) {
            stress = sigma_max;
        }

        strain_values.push_back(strain);
        stress_values.push_back(stress);
    }

    if (strain_values.back() < eps_max) {
        strain_values.push_back(eps_max);
        stress_values.push_back(sigma_max);
    }

    if (strain_values.back() <= eps_max) {
        const double plateau_extension = std::max(0.01, 0.05 * std::max(1.0, eps_max));
        strain_values.push_back(eps_max + plateau_extension);
        stress_values.push_back(sigma_max);
    }

    return true;
}

void DrawHollomonPlot(const MaterialDialog& dialog) {
    static bool fit_next = true;

    std::vector<double> strain_values;
    std::vector<double> stress_values;
    double eps0 = 0.0;
    double eps1 = 0.0;
    double eps_max = 0.0;
    double sigma_max = 0.0;

    if (!BuildHollomonCurve(dialog, strain_values, stress_values, eps0, eps1, eps_max, sigma_max)) {
        ImGui::TextDisabled("Defina E, sigma_y0, K y n con valores positivos para graficar.");
        return;
    }

    ImGui::Text("eps0 = %.4e   eps1 = %.4e   eps_max = %.4e", eps0, eps1, eps_max);
    if (eps0 > eps1) {
        ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.45f, 1.0f),
                           "Advertencia: eps0 > eps1. Revise sigma_y0, E, K o n.");
    }

    if (ImGui::Button("Reset Zoom")) {
        fit_next = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Auto Fit")) {
        fit_next = true;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("Rueda: zoom | Arrastrar: pan");

    if (fit_next) {
        ImPlot::SetNextAxesToFit();
    }

    if (ImPlot::BeginPlot("##HollomonCurve", ImVec2(-1.0f, 260.0f))) {
        ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside);
        ImPlot::SetupAxes("Strain", "Stress");
        ImPlot::SetupAxisLimits(ImAxis_X1, strain_values.front(), strain_values.back(), ImGuiCond_Once);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, sigma_max * 1.05, ImGuiCond_Once);
        ImPlot::PlotLine("Hollomon", strain_values.data(), stress_values.data(), static_cast<int>(strain_values.size()));

        const double markers_x[] = {eps0, eps1, eps_max};
        const double markers_y[] = {
            dialog.m_yield_stress0,
            dialog.m_yield_stress0,
            sigma_max
        };
        ImPlot::PlotScatter("Key points", markers_x, markers_y, 3,
                            ImPlotSpec(ImPlotProp_Marker, ImPlotMarker_Circle,
                                       ImPlotProp_MarkerSize, 4.0f));

        if (ImPlot::IsPlotHovered()) {
            const ImPlotPoint mouse_pos = ImPlot::GetPlotMousePos();
            ImGui::BeginTooltip();
            ImGui::Text("Strain: %.6e", mouse_pos.x);
            ImGui::Text("Stress: %.6e", mouse_pos.y);
            ImGui::EndTooltip();
        }

        fit_next = false;
        ImPlot::EndPlot();
    }
}

}  // namespace

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
        m_k_T = 0.0;
        m_cp_T = 0.0;
        m_yield_stress0 = 190.0E6;
        m_strain_range_min = 0.0;
        m_strain_range_max = 0.65;
        
        return;
    }
    
    else {
    m_density_const = mat->getDensityConstant();
    m_elastic_const = mat->Elastic().E();
    m_poisson_const =  mat->Elastic().nu();

    m_cp_T= mat->cp_T;
    m_k_T = mat->k_T;   
    m_yield_stress0 = mat->yieldStress0;
    m_strain_range_min = 0.0;
    m_strain_range_max = 0.65;
    if (mat->strRange.size() >= 2) {
        m_strain_range_min = mat->strRange[0];
        m_strain_range_max = mat->strRange[1];
    }

        
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
                if (m_pl_const.size() >= 2) {
                    hollomon_K = m_pl_const[0];
                    hollomon_n = m_pl_const[1];
                } else {
                    hollomon_K = mat->K;
                    hollomon_n = mat->m;
                }
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
    
  create_material = false; 
  cancel_action = false;
  if (!m_initiated){
    InitFromMaterial(mat);
    item_current = m_selected_model;
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

  // Opcional: mostrar los campos como grises
  //ImGui::BeginDisabled();
  ImGui::InputDouble("Heat Capacity (cp)", &m_cp_T, 0.0, 1.0, "%.2f");
  ImGui::InputDouble("Thermal Conductivity (k)", &m_k_T, 0.0, 1.0, "%.2f");
  ImGui::InputDouble("Initial Yield Stress", &m_yield_stress0, 0.0, 1.0, "%.2e");
  ImGui::InputDouble("Strain Range Min", &m_strain_range_min, 0.0, 1.0, "%.4f");
  ImGui::InputDouble("Strain Range Max", &m_strain_range_max, 0.0, 1.0, "%.4f");
  //ImGui::EndDisabled();



  ImGui::SetNextItemOpen(true, ImGuiCond_Once);
  if (ImGui::CollapsingHeader("Plastic")){
    
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
      ImGui::Spacing();
      DrawHollomonPlot(*this);
            
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

        mat->cp_T = m_cp_T;
        mat->k_T = m_k_T;
        mat->yieldStress0 = m_yield_stress0;
        mat->strRange = {m_strain_range_min, m_strain_range_max};
        mat->e_min = m_strain_range_min;
        mat->e_max = m_strain_range_max;

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
            mat->InitHollomon(mat->Elastic(), m_yield_stress0, hollomon_K, hollomon_n);
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
  ImGui::SetNextWindowSize(ImVec2(760, 720), ImGuiCond_FirstUseEver);
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
  ImGui::SetNextWindowSize(ImVec2(760, 720), ImGuiCond_FirstUseEver);
  matdlg->Draw("MaterialDlg", p_open, mat);

    
  return true;
}
