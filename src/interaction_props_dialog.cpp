#include "interaction_props_dialog.h"

#include <algorithm>
#include <iostream>

#include "imgui.h"
#include "model/Step.h"

using namespace std;

namespace {
bool supportsContactActivationRamp(Model* model)
{
  if (model == nullptr || model->getAnalysisType() != Solid3D || model->getStepCount() <= 0)
    return false;
  Step* step = model->getStep(0);
  return step != nullptr && step->isImplicit() &&
      step->m_implicitFormulation == ImplicitFormulation::RigidViscoplastic;
}
}

void InteractionPropsDialog::Draw()
{
  if (!m_show)
    return;

  ImGui::SetNextWindowSize(ImVec2(560, 520), ImGuiCond_FirstUseEver);
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

  ImGui::InputInt("Diagnostic Level (0-2)", &contact.diagnosticLevel);
  contact.diagnosticLevel = std::max(0, std::min(2, contact.diagnosticLevel));
  ImGui::InputDouble("Static Friction", &contact.fricCoeffStatic, 0.01, 0.1, "%.4f");
  ImGui::InputDouble("Friction Reg. Vel. (implicit solver)",
                     &contact.frictionRegularizationVelocity,
                     0.01,
                     0.1,
                     "%.6g");
  ImGui::Checkbox("Auto", &contact.autoPenalty);
  ImGui::BeginDisabled(contact.autoPenalty);
  ImGui::InputDouble("Penalty Factor", &contact.penaltyFactor, 10.0, 100.0, "%.4f");
  ImGui::Checkbox("Use Gap Penalty", &contact.useGapPenalty);
  ImGui::EndDisabled();
  ImGui::InputDouble("Gap Penalty Scale", &contact.gapPenaltyScale, 0.1, 1.0, "%.4f");
  ImGui::BeginDisabled(!contact.autoPenalty);
  ImGui::InputDouble("Auto Factor", &contact.autoFactor, 0.05, 0.1, "%.4f");
  ImGui::EndDisabled();
  ImGui::Checkbox("Heat Conductance", &contact.heatConductance);
  ImGui::InputDouble("Heat Cond Coeff", &contact.heatCondCoeff, 0.1, 1.0, "%.4f");
  ImGui::InputDouble("Max Penetration Ratio", &contact.maxPenetRatio, 0.01, 0.1, "%.4f");
  ImGui::InputDouble("Max Accel", &contact.maxAccel, 1000.0, 10000.0, "%.4f");

  if (supportsContactActivationRamp(m_model)) {
    ImGui::Separator();
    ImGui::Checkbox("Rampa experimental de activación del contacto",
                    &contact.contactActivationRamp);
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Suaviza la activación de la rigidez normal y la fricción en puntos de Gauss con penetración pequeña. Experimental para el solver implícito 3D MINI.");
    }
    ImGui::BeginDisabled(!contact.contactActivationRamp);
    ImGui::InputDouble("Ancho de rampa (fracción de h_eff)",
                       &contact.contactActivationRampWidth,
                       0.0,
                       0.0,
                       "%.6g");
    ImGui::EndDisabled();
  }

  if (!isContactActivationRampConfigurationValid(contact)) {
    ImGui::TextColored(ImVec4(1.0f, 0.25f, 0.25f, 1.0f),
                       "El ancho debe ser un número finito y mayor que cero mientras la rampa está habilitada.");
  }

  if (ImGui::Button("Close")) {
    m_show = false;
  }

  ImGui::End();
}
