#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "remesh_dialog.h"

void RemeshDialog::InitFromModel(Model* model)
{
  if (model == nullptr) {
    return;
  }

  const RemeshingSettings& remesh = model->remeshing();
  m_enabled = remesh.enabled;
  m_minStrain = remesh.minStrain;
  m_maxStrain = remesh.maxStrain;
  m_mapVel = remesh.mapVel;
  m_mapAcc = remesh.mapAcc;
  m_maxCount = remesh.maxCount;
  m_dampFactor = remesh.dampFactor;
  m_minFrac = remesh.minFrac;
  m_maxFrac = remesh.maxFrac;
  m_epsRef = remesh.epsRef;
  m_beta = remesh.beta;
  m_type = remesh.type;
  m_debug = remesh.debug;
  m_minElemAngle = remesh.minElemAngle;
  m_maxElemAngle = remesh.maxElemAngle;
  m_transitionAngle = remesh.transitionAngle;
}

void RemeshDialog::Draw(const char* title, bool* p_open, Model* model)
{
  m_saved = false;
  m_cancelled = false;

  if (!m_initialized) {
    InitFromModel(model);
    m_initialized = true;
  }

  if (!ImGui::Begin(title, p_open)) {
    ImGui::End();
    return;
  }

  ImGui::Checkbox("Enabled", &m_enabled);

  if (ImGui::CollapsingHeader("Trigger", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::InputDouble("Min Strain", &m_minStrain, 0.0, 0.0, "%.6g");
    ImGui::InputDouble("Max Strain", &m_maxStrain, 0.0, 0.0, "%.6g");
    ImGui::InputInt("Max Count", &m_maxCount);
    ImGui::InputDouble("Damp Factor", &m_dampFactor, 0.0, 0.0, "%.6g");
    ImGui::InputDouble("Min Frac", &m_minFrac, 0.0, 0.0, "%.6g");
    ImGui::InputDouble("Max Frac", &m_maxFrac, 0.0, 0.0, "%.6g");
    ImGui::InputDouble("Eps Ref", &m_epsRef, 0.0, 0.0, "%.6g");
    ImGui::InputDouble("Beta", &m_beta, 0.0, 0.0, "%.6g");
    ImGui::InputInt("Type", &m_type);
  }

  if (ImGui::CollapsingHeader("Mapping", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Checkbox("Map Vel", &m_mapVel);
    ImGui::Checkbox("Map Acc", &m_mapAcc);
  }

  if (ImGui::CollapsingHeader("Quality", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Checkbox("Debug", &m_debug);
    ImGui::InputDouble("Min Elem Angle", &m_minElemAngle, 0.0, 0.0, "%.6g");
    ImGui::InputDouble("Max Elem Angle", &m_maxElemAngle, 0.0, 0.0, "%.6g");
    ImGui::InputDouble("Transition Angle", &m_transitionAngle, 0.0, 0.0, "%.6g");
  }

  if (ImGui::Button("Ok")) {
    if (model != nullptr) {
      RemeshingSettings& remesh = model->remeshing();
      remesh.enabled = m_enabled;
      remesh.minStrain = m_minStrain;
      remesh.maxStrain = m_maxStrain;
      remesh.mapVel = m_mapVel;
      remesh.mapAcc = m_mapAcc;
      remesh.maxCount = m_maxCount;
      remesh.dampFactor = m_dampFactor;
      remesh.minFrac = m_minFrac;
      remesh.maxFrac = m_maxFrac;
      remesh.epsRef = m_epsRef;
      remesh.beta = m_beta;
      remesh.type = m_type;
      remesh.debug = m_debug;
      remesh.minElemAngle = m_minElemAngle;
      remesh.maxElemAngle = m_maxElemAngle;
      remesh.transitionAngle = m_transitionAngle;
    }
    m_saved = true;
    m_initialized = false;
    *p_open = false;
  }
  ImGui::SameLine();
  if (ImGui::Button("Cancel")) {
    m_cancelled = true;
    m_initialized = false;
    *p_open = false;
  }

  ImGui::End();
}

bool ShowEditRemeshDialog(bool* p_open, RemeshDialog* remeshdlg, Model* model)
{
  ImGui::SetNextWindowSize(ImVec2(520, 460), ImGuiCond_FirstUseEver);
  remeshdlg->Draw("Remeshing", p_open, model);
  return remeshdlg->m_saved;
}
