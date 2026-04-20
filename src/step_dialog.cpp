#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "step_dialog.h"

#include <cstring>

void StepDialog::InitFromStep(Step *step) {
  if (!step) {
    return;
  }

  std::strncpy(m_name, step->getName(), sizeof(m_name) - 1);
  m_name[sizeof(m_name) - 1] = '\0';
  m_step_type = step->getStepType();

  m_nproc = step->m_nproc;
  m_cflFactor = step->m_cflFactor;
  for (int i = 0; i < 3; ++i)
    m_autoTS[i] = step->m_autoTS[i];
  m_kernelGradCorr = step->m_kernelGradCorr;
  m_simTime = step->m_simTime;
  m_artifViscAlpha = step->m_artifViscAlpha;
  m_artifViscBeta = step->m_artifViscBeta;
  m_outTime = step->m_outTime;
  m_fixedTS = step->m_fixedTS;
  m_axiSymmVol = step->m_axiSymmVol;
  m_elemLengthFraction = step->m_elemLengthFraction;

  m_meshingDebug = step->m_meshingDebug;
  m_maxElemAngle = step->m_maxElemAngle;
  m_minElemAngle = step->m_minElemAngle;

  std::strncpy(m_implicit_type, step->m_implicitType.c_str(), sizeof(m_implicit_type) - 1);
  m_implicit_type[sizeof(m_implicit_type) - 1] = '\0';
  m_velTol = step->m_velTol;
  m_pressTol = step->m_pressTol;
  m_forceTol = step->m_forceTol;
  m_divTol = step->m_divTol;
  m_omegaV = step->m_omegaV;
  m_omegaP = step->m_omegaP;
  m_maxIter = step->m_maxIter;
  m_timeStepGrowthFactor = step->m_timeStepGrowthFactor;
}

void StepDialog::Draw(const char* title, bool* p_open, Step* step) {
  m_saved = false;
  m_cancelled = false;

  if (!m_initialized) {
    InitFromStep(step);
    m_initialized = true;
  }

  if (!ImGui::Begin(title, p_open)) {
    ImGui::End();
    return;
  }

  ImGui::InputText("Name", m_name, IM_ARRAYSIZE(m_name));

  if (ImGui::RadioButton("Explicit", m_step_type == ExplicitStep))
    m_step_type = ExplicitStep;
  ImGui::SameLine();
  if (ImGui::RadioButton("Implicit", m_step_type == ImplicitStep))
    m_step_type = ImplicitStep;

  ImGui::InputInt("Nproc", &m_nproc);
  ImGui::InputDouble("CFL Factor", &m_cflFactor, 0.0, 1.0, "%.3f");
  ImGui::Checkbox("Kernel Grad Corr", &m_kernelGradCorr);
  ImGui::Checkbox("Fixed TS", &m_fixedTS);
  ImGui::Checkbox("AxiSymm Vol", &m_axiSymmVol);
  ImGui::InputDouble("Sim Time", &m_simTime, 0.0, 1.0, "%.4f");
  ImGui::InputDouble("Out Time", &m_outTime, 0.0, 1.0, "%.4f");
  ImGui::InputDouble("Artif Visc Alpha", &m_artifViscAlpha, 0.0, 1.0, "%.3f");
  ImGui::InputDouble("Artif Visc Beta", &m_artifViscBeta, 0.0, 1.0, "%.3f");
  ImGui::InputDouble("Elem Length Fraction", &m_elemLengthFraction, 0.0, 1.0, "%.3f");
  ImGui::Checkbox("Auto TS X", &m_autoTS[0]);
  ImGui::Checkbox("Auto TS Y", &m_autoTS[1]);
  ImGui::Checkbox("Auto TS Z", &m_autoTS[2]);

  if (ImGui::CollapsingHeader("Meshing", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Checkbox("Debug", &m_meshingDebug);
    ImGui::InputDouble("Max Elem Angle", &m_maxElemAngle, 0.0, 1.0, "%.2f");
    ImGui::InputDouble("Min Elem Angle", &m_minElemAngle, 0.0, 1.0, "%.2f");
  }

  if (m_step_type == ImplicitStep && ImGui::CollapsingHeader("Implicit Solver", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::InputText("Type", m_implicit_type, IM_ARRAYSIZE(m_implicit_type));
    ImGui::InputDouble("Vel Tol", &m_velTol, 0.0, 1.0, "%.4g");
    ImGui::InputDouble("Press Tol", &m_pressTol, 0.0, 1.0, "%.4g");
    ImGui::InputDouble("Force Tol", &m_forceTol, 0.0, 1.0, "%.4g");
    ImGui::InputDouble("Div Tol", &m_divTol, 0.0, 1.0, "%.4g");
    ImGui::InputDouble("Omega V", &m_omegaV, 0.0, 1.0, "%.4f");
    ImGui::InputDouble("Omega P", &m_omegaP, 0.0, 1.0, "%.4f");
    ImGui::InputInt("Max Iter", &m_maxIter);
    ImGui::InputDouble("TS Growth Factor", &m_timeStepGrowthFactor, 0.0, 1.0, "%.3f");
  }

  if (ImGui::Button("Ok")) {
    if (step) {
      step->setName(m_name);
      step->setStepType(static_cast<StepType>(m_step_type));
      step->m_nproc = m_nproc;
      step->m_cflFactor = m_cflFactor;
      for (int i = 0; i < 3; ++i)
        step->m_autoTS[i] = m_autoTS[i];
      step->m_kernelGradCorr = m_kernelGradCorr;
      step->m_simTime = m_simTime;
      step->m_artifViscAlpha = m_artifViscAlpha;
      step->m_artifViscBeta = m_artifViscBeta;
      step->m_outTime = m_outTime;
      step->m_fixedTS = m_fixedTS;
      step->m_axiSymmVol = m_axiSymmVol;
      step->m_elemLengthFraction = m_elemLengthFraction;
      step->m_meshingDebug = m_meshingDebug;
      step->m_maxElemAngle = m_maxElemAngle;
      step->m_minElemAngle = m_minElemAngle;
      step->m_implicitType = m_implicit_type;
      step->m_velTol = m_velTol;
      step->m_pressTol = m_pressTol;
      step->m_forceTol = m_forceTol;
      step->m_divTol = m_divTol;
      step->m_omegaV = m_omegaV;
      step->m_omegaP = m_omegaP;
      step->m_maxIter = m_maxIter;
      step->m_timeStepGrowthFactor = m_timeStepGrowthFactor;
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

bool ShowEditStepDialog(bool* p_open, StepDialog *stepdlg, Step *step) {
  ImGui::SetNextWindowSize(ImVec2(500, 500), ImGuiCond_FirstUseEver);
  stepdlg->Draw("Step", p_open, step);
  return stepdlg->m_saved;
}
