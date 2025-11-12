#include "bc_dialog.h"
#include <iostream>
#include "Model.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

//~ void BCDialog::Draw(const char* title, bool* p_open, Model* model, BoundaryCondition *sel_bc) {
    //~ bool create = false;
    //~ if (sel_bc == nullptr)
      //~ create = true;
    //~ if (!ImGui::Begin(title, p_open)) {
        //~ ImGui::End();
        //~ return;
    //~ }

    //~ ImGui::Text("Boundary Condition Type: Velocity");
    //~ ImGui::Separator();

    //~ ImGui::Text("Apply to:");
    //~ ImGui::RadioButton("Part", &m_applyTo, 0);
    //~ ImGui::SameLine();
    //~ ImGui::RadioButton("Nodes", &m_applyTo, 1);

    //~ ImGui::InputInt("Target ID", &m_targetId);

    //~ float v[3] = { m_vel.x, m_vel.y, m_vel.z };
    //~ ImGui::InputFloat3("Velocity", v, "%.3f");
    //~ m_vel.x = v[0]; m_vel.y = v[1]; m_vel.z = v[2];
    
    //~ std::string butleg = "OK";
    //~ BoundaryCondition *bc;
    //~ if (create){
      //~ butleg = "Create";
    //~ }
    //~ if (ImGui::Button(butleg.c_str())) {
        //~ BCApplyTo target = (m_applyTo == 0) ? ApplyToPart : ApplyToNodes;
        //~ if (create)
          //~ bc = new BoundaryCondition(VelocityBC, target, m_targetId, m_vel);
        //~ else 
          //~ bc = sel_bc;

        //~ model->addBoundaryCondition(bc);

        //~ std::cout << "Added BC for "
                  //~ << ((target == ApplyToPart) ? "Part " : "Nodes ")
                  //~ << m_targetId
                  //~ << " with velocity = (" << m_vel.x << "," << m_vel.y << "," << m_vel.z << ")\n";
        //~ cout << "New BC Count Size: "<<model->getBCCount()<<endl;
        //~ *p_open = false;
    //~ }

    //~ ImGui::SameLine();
    //~ if (ImGui::Button("Cancel")) {
        //~ *p_open = false;
    //~ }

    //~ ImGui::End();
//~ }

#include "bc_dialog.h"
#include <iostream>
#include "Model.h"
#include "Part.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

void BCDialog::Draw(const char* title, bool* p_open, Model* model, BoundaryCondition *sel_bc) {
    
    bool create = (sel_bc == nullptr);  
    static bool initialized = false;
    
    if (!initialized) {
      if (!create && sel_bc) {
          m_applyTo = (sel_bc->getApplyTo() == ApplyToPart) ? 0 : 1;
          m_targetId = sel_bc->getTargetId();
          m_vel = sel_bc->getVelocity();
      }
      initialized = true;
    }

  

    if (!ImGui::Begin(title, p_open)) {
        ImGui::End();
        return;
    }

    ImGui::Text("Boundary Condition Type: Velocity");
    ImGui::Separator();

    // Selección de tipo de aplicación
    ImGui::Text("Apply to:");
    ImGui::RadioButton("Part", &m_applyTo, 0);
    ImGui::SameLine();
    ImGui::RadioButton("Nodes", &m_applyTo, 1);

    if (m_applyTo == 0) {
      // --- mantener estos estáticos ---
      static std::vector<std::string> partNames;
      static int selectedPartIndex = -1;

      // Solo actualizamos si cambió el modelo o la cantidad de partes
      int numParts = model->getPartCount();
      if ((int)partNames.size() != numParts) {
          partNames.clear();
          for (int i = 0; i < numParts; i++) {
              std::string name = model->getPart(i)->getName();
              if (name.empty()) name = "Part " + std::to_string(model->getPart(i)->getID());
              partNames.push_back(name);
          }
      }

      // Sincronizar si estamos editando una BC existente
      if (!create && selectedPartIndex == -1) {
          for (int i = 0; i < numParts; i++) {
              if (model->getPart(i)->getID() == m_targetId) {
                  selectedPartIndex = i;
                  break;
              }
          }
      }
      // Si es nueva y aún no hay selección
      if (create && selectedPartIndex == -1 && numParts > 0) {
          selectedPartIndex = 0;
          m_targetId = model->getPart(0)->getID();
      }

      const char* preview = (selectedPartIndex >= 0 && selectedPartIndex < numParts)
          ? partNames[selectedPartIndex].c_str() : "Select Part";

      // Combo con label único ("##" oculta el texto visible)
      if (ImGui::BeginCombo("Select Part##BCDialogPartCombo", preview)) {
          for (int i = 0; i < numParts; i++) {
              bool is_selected = (selectedPartIndex == i);
              if (ImGui::Selectable(partNames[i].c_str(), is_selected)) {
                  selectedPartIndex = i;
                  m_targetId = model->getPart(i)->getID();
              }
              if (is_selected) ImGui::SetItemDefaultFocus();
          }
          ImGui::EndCombo();
      }

      if (selectedPartIndex >= 0 && selectedPartIndex < numParts)
          ImGui::Text("Selected ID: %d", model->getPart(selectedPartIndex)->getID());
    }


    else {
        // Modo "Nodes" → ID manual
        ImGui::InputInt("Target Node Set ID", &m_targetId);
    }

    // --- Velocidad ---
    float v[3] = { m_vel.x, m_vel.y, m_vel.z };
    ImGui::InputFloat3("Velocity", v, "%.3f");
    m_vel.x = v[0]; m_vel.y = v[1]; m_vel.z = v[2];

    // --- Botones ---
    std::string butleg = create ? "Create" : "OK";
    if (ImGui::Button(butleg.c_str())) {
        BCApplyTo target = (m_applyTo == 0) ? ApplyToPart : ApplyToNodes;

        BoundaryCondition *bc = nullptr;
        if (create)
            bc = new BoundaryCondition(VelocityBC, target, m_targetId, m_vel);
        else {
            bc = sel_bc;
            bc->setApplyTo(target);
            bc->setTargetId(m_targetId);
            bc->setVelocity(m_vel);
        }

        if (create)
            model->addBoundaryCondition(bc);

        std::cout << (create ? "Created" : "Updated") << " BC for "
                  << ((target == ApplyToPart) ? "Part " : "Nodes ")
                  << m_targetId
                  << " with velocity = (" << m_vel.x << "," << m_vel.y << "," << m_vel.z << ")\n";
        std::cout << "Total BCs: " << model->getBCCount() << std::endl;

        *p_open = false;
        sel_bc = nullptr;
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
        *p_open = false;
        sel_bc = nullptr;
    }
    
    if (!(*p_open)) {
      initialized = false;
    }

    ImGui::End();
}

