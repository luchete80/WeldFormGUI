#include "bc_dialog.h"
#include <iostream>

void BCDialog::Draw(const char* title, bool* p_open, Model* model) {
    if (!ImGui::Begin(title, p_open)) {
        ImGui::End();
        return;
    }

    ImGui::Text("Boundary Condition Type: Velocity");
    ImGui::Separator();

    ImGui::Text("Apply to:");
    ImGui::RadioButton("Part", &m_applyTo, 0);
    ImGui::SameLine();
    ImGui::RadioButton("Nodes", &m_applyTo, 1);

    ImGui::InputInt("Target ID", &m_targetId);

    float v[3] = { m_vel.x, m_vel.y, m_vel.z };
    ImGui::InputFloat3("Velocity", v, "%.3f");
    m_vel.x = v[0]; m_vel.y = v[1]; m_vel.z = v[2];

    if (ImGui::Button("OK")) {
        BCApplyTo target = (m_applyTo == 0) ? ApplyToPart : ApplyToNodes;
        auto* bc = new BoundaryCondition(VelocityBC, target, m_targetId, m_vel);
        model->addBoundaryCondition(bc);

        std::cout << "Added BC for "
                  << ((target == ApplyToPart) ? "Part " : "Nodes ")
                  << m_targetId
                  << " with velocity = (" << m_vel.x << "," << m_vel.y << "," << m_vel.z << ")\n";

        *p_open = false;
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
        *p_open = false;
    }

    ImGui::End();
}
