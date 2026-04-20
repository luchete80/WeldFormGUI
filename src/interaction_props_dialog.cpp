#include "interaction_props_dialog.h"

#include <iostream>

#include "imgui.h"

using namespace std;

void InteractionPropsDialog::Draw()
{
  if (!m_show)
    return;

  ImGui::SetNextWindowSize(ImVec2(420, 260), ImGuiCond_FirstUseEver);
  if (!ImGui::Begin("Interaction Properties", &m_show)) {
    ImGui::End();
    return;
  }

  if (m_model == nullptr) {
    ImGui::TextUnformatted("No active model.");
    ImGui::End();
    return;
  }

  ContactProperties &contact = m_model->contactProps();

  ImGui::InputDouble("Static Friction", &contact.fricCoeffStatic, 0.01, 0.1, "%.4f");
  ImGui::InputDouble("Penalty Factor", &contact.penaltyFactor, 10.0, 100.0, "%.4f");
  ImGui::Checkbox("Use Gap Penalty", &contact.useGapPenalty);
  ImGui::InputDouble("Gap Penalty Scale", &contact.gapPenaltyScale, 0.1, 1.0, "%.4f");
  ImGui::Checkbox("Heat Conductance", &contact.heatConductance);
  ImGui::InputDouble("Heat Cond Coeff", &contact.heatCondCoeff, 0.1, 1.0, "%.4f");
  ImGui::InputDouble("Max Penetration Ratio", &contact.maxPenetRatio, 0.01, 0.1, "%.4f");
  ImGui::InputDouble("Max Accel", &contact.maxAccel, 1000.0, 10000.0, "%.4f");

  if (ImGui::Button("Close")) {
    m_show = false;
  }

  ImGui::End();
}
