#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "step_dialog.h"

#include <cstring>

namespace {
int springModeComboIndex(int spring_mode) {
  return spring_mode == 321 ? 1 : 0;
}

int springModeFromComboIndex(int combo_index) {
  return combo_index == 1 ? 321 : 1;
}

int implicitSolverTypeComboIndex(const std::string& implicit_type) {
  if (implicit_type == "hybrid")
    return 1;
  if (implicit_type == "newton")
    return 2;
  if (implicit_type == "nr")
    return 3;
  return 0;
}

const char* implicitSolverTypeFromComboIndex(int combo_index) {
  switch (combo_index) {
    case 1:
      return "hybrid";
    case 2:
      return "newton";
    case 3:
      return "nr";
    case 0:
    default:
      return "picard";
  }
}
}

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
  m_useWeakSprings = step->m_useWeakSprings;
  m_springFactor = step->m_springFactor;
  m_springStiffness = step->m_springStiffness;
  m_springMode = step->m_springMode;
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

  if (m_step_type == ImplicitStep && ImGui::CollapsingHeader("Implicit Solver", ImGuiTreeNodeFlags_DefaultOpen)) {
    static const char* spring_mode_items[] = {"1 node", "3-2-1"};
    static const char* implicit_solver_items[] = {"Picard", "Hybrid", "Newton", "NR"};
    int spring_mode_index = springModeComboIndex(m_springMode);
    int implicit_solver_index = implicitSolverTypeComboIndex(m_implicit_type);

    if (ImGui::Combo("Type", &implicit_solver_index, implicit_solver_items, IM_ARRAYSIZE(implicit_solver_items))) {
      std::strncpy(m_implicit_type,
                   implicitSolverTypeFromComboIndex(implicit_solver_index),
                   IM_ARRAYSIZE(m_implicit_type) - 1);
      m_implicit_type[IM_ARRAYSIZE(m_implicit_type) - 1] = '\0';
    }
    ImGui::InputDouble("Vel Tol", &m_velTol, 0.0, 1.0, "%.4g");
    ImGui::InputDouble("Press Tol", &m_pressTol, 0.0, 1.0, "%.4g");
    ImGui::InputDouble("Force Tol", &m_forceTol, 0.0, 1.0, "%.4g");
    ImGui::InputDouble("Div Tol", &m_divTol, 0.0, 1.0, "%.4g");
    ImGui::InputDouble("Omega V", &m_omegaV, 0.0, 1.0, "%.4f");
    ImGui::InputDouble("Omega P", &m_omegaP, 0.0, 1.0, "%.4f");
    ImGui::InputInt("Max Iter", &m_maxIter);
    ImGui::InputDouble("TS Growth Factor", &m_timeStepGrowthFactor, 0.0, 1.0, "%.3f");
    ImGui::TextDisabled("Type is exported as Configuration.solver.type for implicit inputs.");

    ImGui::Separator();
    ImGui::Checkbox("Use weak springs", &m_useWeakSprings);
    ImGui::InputDouble("Spring factor", &m_springFactor, 0.0, 0.0, "%.4g");
    if (ImGui::Combo("Spring mode", &spring_mode_index, spring_mode_items, IM_ARRAYSIZE(spring_mode_items)))
      m_springMode = springModeFromComboIndex(spring_mode_index);

    if (ImGui::TreeNode("Advanced")) {
      ImGui::InputDouble("Spring stiffness override", &m_springStiffness, 0.0, 0.0, "%.4g");
      ImGui::TreePop();
    }
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
      step->m_implicitType = m_implicit_type;
      step->m_velTol = m_velTol;
      step->m_pressTol = m_pressTol;
      step->m_forceTol = m_forceTol;
      step->m_divTol = m_divTol;
      step->m_omegaV = m_omegaV;
      step->m_omegaP = m_omegaP;
      step->m_maxIter = m_maxIter;
      step->m_timeStepGrowthFactor = m_timeStepGrowthFactor;
      step->m_useWeakSprings = m_useWeakSprings;
      step->m_springFactor = m_springFactor;
      step->m_springStiffness = m_springStiffness;
      step->m_springMode = m_springMode;
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
