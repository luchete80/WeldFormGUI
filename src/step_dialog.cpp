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
  if (implicit_type == "j2" || implicit_type == "j2ep" || implicit_type == "implicit_ep_j2")
    return 4;
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
    case 4:
      return "j2";
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

const char* implicitFormulationLabel(int combo_index) {
  switch (combo_index) {
    case 1:
      return "J2 Elastoplastic";
    case 0:
    default:
      return "Rigid Viscoplastic";
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
  m_stabAlphaFree = step->m_stabAlphaFree;
  m_stabAlphaContact = step->m_stabAlphaContact;
  m_stabHgCoeffFree = step->m_stabHgCoeffFree;
  m_stabHgCoeffContact = step->m_stabHgCoeffContact;
  m_stabAvCoeffDiv = step->m_stabAvCoeffDiv;
  m_stabAvCoeffBulk = step->m_stabAvCoeffBulk;
  m_stabLogFactor = step->m_stabLogFactor;
  m_stabPspgScale = step->m_stabPspgScale;
  m_stabPspgBulkFactor = step->m_stabPspgBulkFactor;
  m_stabJMin = step->m_stabJMin;
  m_stabHgVisc = step->m_stabHgVisc;
  m_stabHgStiff = step->m_stabHgStiff;
  m_outTime = step->m_outTime;
  m_fixedTS = step->m_fixedTS;
  m_axiSymmVol = step->m_axiSymmVol;
  m_elemLengthFraction = step->m_elemLengthFraction;

  m_implicit_formulation = static_cast<int>(step->m_implicitFormulation);
  std::strncpy(m_implicit_type, step->m_implicitType.c_str(), sizeof(m_implicit_type) - 1);
  m_implicit_type[sizeof(m_implicit_type) - 1] = '\0';
  m_velTol = step->m_velTol;
  m_bubbleVelTol = step->m_bubbleVelTol;
  m_pressTol = step->m_pressTol;
  m_forceTol = step->m_forceTol;
  m_divTol = step->m_divTol;
  m_omegaV = step->m_omegaV;
  m_omegaP = step->m_omegaP;
  m_maxIter = step->m_maxIter;
  m_timeStepGrowthFactor = step->m_timeStepGrowthFactor;
  m_initialTimeStep = step->m_initialTimeStep;
  m_maxStepRetries = step->m_maxStepRetries;
  m_nonConvergenceCutbackFactor = step->m_nonConvergenceCutbackFactor;
  m_useWeakSprings = step->m_useWeakSprings;
  m_springFactor = step->m_springFactor;
  m_springStiffness = step->m_springStiffness;
  m_springMode = step->m_springMode;
  m_rigidModeStabilization = step->m_rigidModeStabilization;
  m_rigidModeStabilizationFactor = step->m_rigidModeStabilizationFactor;
  m_rigidModeContactFade = step->m_rigidModeContactFade;
  m_rigidModeContactFadeScale = step->m_rigidModeContactFadeScale;
  m_adaptiveDtLimiter = step->m_adaptiveDtLimiter;
  m_adaptiveDtMin = step->m_adaptiveDtMin;
  m_maxNodalDisplacementPerStep = step->m_maxNodalDisplacementPerStep;
  m_maxEffectiveStrainIncrementPerStep = step->m_maxEffectiveStrainIncrementPerStep;
  m_picardUseFixedPointResidual = step->m_picardUseFixedPointResidual;
  m_picardStagnationEnabled = step->m_picardStagnationEnabled;
  m_picardStagnationWindow = step->m_picardStagnationWindow;
  m_picardStagnationMinIter = step->m_picardStagnationMinIter;
  m_picardStagnationMinImprovement = step->m_picardStagnationMinImprovement;
  m_picardStagnationMaxWindows = step->m_picardStagnationMaxWindows;
  m_picardUseAitken = step->m_picardUseAitken;
  m_picardAitkenMinOmega = step->m_picardAitkenMinOmega;
  m_picardAitkenMaxOmega = step->m_picardAitkenMaxOmega;
  m_picardUseAitkenBubble = step->m_picardUseAitkenBubble;
  m_omegaBubble = step->m_omegaBubble;
  m_picardAitkenBubbleMinOmega = step->m_picardAitkenBubbleMinOmega;
  m_picardAitkenBubbleMaxOmega = step->m_picardAitkenBubbleMaxOmega;
  m_picardSmoothViscosity = step->m_picardSmoothViscosity;
  m_picardStrainRateRegularization = step->m_picardStrainRateRegularization;
}

void StepDialog::Draw(const char* title, bool* p_open, Step* step, bool is3D) {
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

  const bool isImplicit = (m_step_type == ImplicitStep);

  ImGui::InputInt("Nproc", &m_nproc);
  if (!isImplicit) {
    ImGui::InputDouble("CFL Factor", &m_cflFactor, 0.0, 1.0, "%.3f");
  }
  ImGui::Checkbox("Kernel Grad Corr", &m_kernelGradCorr);
  ImGui::Checkbox("Fixed TS", &m_fixedTS);
  ImGui::Checkbox("AxiSymm Vol", &m_axiSymmVol);
  ImGui::InputDouble("Sim Time", &m_simTime, 0.0, 1.0, "%.4f");
  ImGui::InputDouble("Out Time", &m_outTime, 0.0, 1.0, "%.4f");
  if (!isImplicit) {
    ImGui::InputDouble("Artif Visc Alpha", &m_artifViscAlpha, 0.0, 1.0, "%.3f");
    ImGui::InputDouble("Artif Visc Beta", &m_artifViscBeta, 0.0, 1.0, "%.3f");
    if (ImGui::TreeNode("Stabilization")) {
      ImGui::InputDouble("Alpha free", &m_stabAlphaFree, 0.0, 0.0, "%.4g");
      ImGui::InputDouble("Alpha contact", &m_stabAlphaContact, 0.0, 0.0, "%.4g");
      ImGui::InputDouble("HG coeff free", &m_stabHgCoeffFree, 0.0, 0.0, "%.4g");
      ImGui::InputDouble("HG coeff contact", &m_stabHgCoeffContact, 0.0, 0.0, "%.4g");
      ImGui::InputDouble("AV coeff div", &m_stabAvCoeffDiv, 0.0, 0.0, "%.4g");
      ImGui::InputDouble("AV coeff bulk", &m_stabAvCoeffBulk, 0.0, 0.0, "%.4g");
      ImGui::InputDouble("Log factor", &m_stabLogFactor, 0.0, 0.0, "%.4g");
      ImGui::InputDouble("PSPG scale", &m_stabPspgScale, 0.0, 0.0, "%.4g");
      ImGui::InputDouble("PSPG bulk factor", &m_stabPspgBulkFactor, 0.0, 0.0, "%.4g");
      ImGui::InputDouble("J min", &m_stabJMin, 0.0, 0.0, "%.4g");
      ImGui::InputDouble("HG visc", &m_stabHgVisc, 0.0, 0.0, "%.4g");
      ImGui::InputDouble("HG stiff", &m_stabHgStiff, 0.0, 0.0, "%.4g");
      ImGui::TreePop();
    }
  }
  ImGui::InputDouble("Elem Length Fraction", &m_elemLengthFraction, 0.0, 1.0, "%.3f");
  ImGui::Checkbox("Auto TS X", &m_autoTS[0]);
  ImGui::Checkbox("Auto TS Y", &m_autoTS[1]);
  ImGui::Checkbox("Auto TS Z", &m_autoTS[2]);

  if (isImplicit && ImGui::CollapsingHeader("Implicit Solver", ImGuiTreeNodeFlags_DefaultOpen)) {
    static const char* spring_mode_items[] = {"1 node", "3-2-1"};
    static const char* implicit_solver_items[] = {"Picard", "Hybrid", "Newton", "NR"};
    static const char* implicit_formulation_items[] = {"Rigid Viscoplastic", "J2 Elastoplastic"};
    int spring_mode_index = springModeComboIndex(m_springMode);

    ImGui::Combo("Formulation", &m_implicit_formulation, implicit_formulation_items, IM_ARRAYSIZE(implicit_formulation_items));
    const bool isJ2Implicit =
        m_implicit_formulation == static_cast<int>(ImplicitFormulation::J2Elastoplastic);
    if (isJ2Implicit) {
      std::strncpy(m_implicit_type, "j2", IM_ARRAYSIZE(m_implicit_type) - 1);
      m_implicit_type[IM_ARRAYSIZE(m_implicit_type) - 1] = '\0';
      ImGui::TextDisabled("Type: J2");
    } else {
      int implicit_solver_index = implicitSolverTypeComboIndex(m_implicit_type);
      if (ImGui::Combo("Type", &implicit_solver_index, implicit_solver_items, IM_ARRAYSIZE(implicit_solver_items))) {
        std::strncpy(m_implicit_type,
                     implicitSolverTypeFromComboIndex(implicit_solver_index),
                     IM_ARRAYSIZE(m_implicit_type) - 1);
        m_implicit_type[IM_ARRAYSIZE(m_implicit_type) - 1] = '\0';
      }
    }
    ImGui::InputDouble("Vel Tol", &m_velTol, 0.0, 1.0, "%.4g");
    if (is3D)
      ImGui::InputDouble("Bubble Vel Tol", &m_bubbleVelTol, 0.0, 1.0, "%.4g");
    ImGui::InputDouble("Press Tol", &m_pressTol, 0.0, 1.0, "%.4g");
    ImGui::InputDouble("Force Tol", &m_forceTol, 0.0, 1.0, "%.4g");
    ImGui::InputDouble("Div Tol", &m_divTol, 0.0, 1.0, "%.4g");
    ImGui::InputDouble("Omega V", &m_omegaV, 0.0, 1.0, "%.4f");
    ImGui::InputDouble("Omega P", &m_omegaP, 0.0, 1.0, "%.4f");
    ImGui::InputInt("Max Iter", &m_maxIter);
    ImGui::InputDouble("TS Growth Factor", &m_timeStepGrowthFactor, 0.0, 1.0, "%.3f");
    ImGui::InputDouble("Initial Time Step (0 = automatic)", &m_initialTimeStep, 0.0, 0.0, "%.6g");
    if (m_initialTimeStep < 0.0) m_initialTimeStep = 0.0;
    ImGui::InputInt("Max Step Retries", &m_maxStepRetries);
    if (m_maxStepRetries < 0) m_maxStepRetries = 0;
    ImGui::InputDouble("Non-convergence Cutback Factor", &m_nonConvergenceCutbackFactor, 0.0, 0.0, "%.3f");
    if (!(m_nonConvergenceCutbackFactor > 0.0 && m_nonConvergenceCutbackFactor < 1.0))
      m_nonConvergenceCutbackFactor = 0.5;
    ImGui::InputDouble("Adaptive DT min", &m_adaptiveDtMin, 0.0, 0.0, "%.4g");
    ImGui::Checkbox("Adaptive DT limiter", &m_adaptiveDtLimiter);
    ImGui::BeginDisabled(!m_adaptiveDtLimiter);
    ImGui::InputDouble("Max nodal displacement per step", &m_maxNodalDisplacementPerStep, 0.0, 0.0, "%.4g");
    ImGui::InputDouble("Max effective strain increment per step", &m_maxEffectiveStrainIncrementPerStep, 0.0, 0.0, "%.4g");
    ImGui::EndDisabled();
    ImGui::TextDisabled("Formulation: %s", implicitFormulationLabel(m_implicit_formulation));

    if (ImGui::TreeNode("Picard options")) {
      ImGui::Checkbox("Use fixed-point residual", &m_picardUseFixedPointResidual);
      ImGui::Checkbox("Enable stagnation detection", &m_picardStagnationEnabled);
      ImGui::InputInt("Stagnation window", &m_picardStagnationWindow);
      if (m_picardStagnationWindow < 2) m_picardStagnationWindow = 2;
      ImGui::InputInt("Stagnation minimum iteration", &m_picardStagnationMinIter);
      if (m_picardStagnationMinIter < 0) m_picardStagnationMinIter = 0;
      ImGui::InputDouble("Stagnation minimum improvement", &m_picardStagnationMinImprovement, 0.0, 0.0, "%.4g");
      if (m_picardStagnationMinImprovement < 0.0) m_picardStagnationMinImprovement = 0.0;
      if (m_picardStagnationMinImprovement > 1.0) m_picardStagnationMinImprovement = 1.0;
      ImGui::InputInt("Stagnation maximum windows", &m_picardStagnationMaxWindows);
      if (m_picardStagnationMaxWindows < 1) m_picardStagnationMaxWindows = 1;
      ImGui::Checkbox("Use Aitken omega", &m_picardUseAitken);
      ImGui::InputDouble("Aitken minimum omega", &m_picardAitkenMinOmega, 0.0, 0.0, "%.4g");
      if (m_picardAitkenMinOmega <= 0.0) m_picardAitkenMinOmega = 0.05;
      ImGui::InputDouble("Aitken maximum omega", &m_picardAitkenMaxOmega, 0.0, 0.0, "%.4g");
      if (m_picardAitkenMaxOmega < m_picardAitkenMinOmega) m_picardAitkenMaxOmega = m_picardAitkenMinOmega;
      ImGui::Checkbox("Use Aitken for bubble", &m_picardUseAitkenBubble);
      ImGui::BeginDisabled(!m_picardUseAitkenBubble);
      ImGui::InputDouble("Bubble omega", &m_omegaBubble, 0.0, 0.0, "%.4g");
      if (m_omegaBubble <= 0.0) m_omegaBubble = 0.4;
      ImGui::InputDouble("Bubble Aitken min omega", &m_picardAitkenBubbleMinOmega, 0.0, 0.0, "%.4g");
      if (m_picardAitkenBubbleMinOmega <= 0.0) m_picardAitkenBubbleMinOmega = 0.05;
      ImGui::InputDouble("Bubble Aitken max omega", &m_picardAitkenBubbleMaxOmega, 0.0, 0.0, "%.4g");
      if (m_picardAitkenBubbleMaxOmega < m_picardAitkenBubbleMinOmega)
        m_picardAitkenBubbleMaxOmega = m_picardAitkenBubbleMinOmega;
      ImGui::EndDisabled();
      ImGui::Checkbox("Smooth viscosity", &m_picardSmoothViscosity);
      ImGui::InputDouble("Strain-rate regularization", &m_picardStrainRateRegularization, 0.0, 0.0, "%.4g");
      if (m_picardStrainRateRegularization <= 0.0) m_picardStrainRateRegularization = 0.001;
      ImGui::TreePop();
    }

    ImGui::Separator();
    if (ImGui::Checkbox("Use weak springs", &m_useWeakSprings) && m_useWeakSprings)
      m_rigidModeStabilization = false;
    ImGui::SameLine();
    if (ImGui::Checkbox("Constrain rigid modes", &m_rigidModeStabilization) &&
        m_rigidModeStabilization)
      m_useWeakSprings = false;
    ImGui::InputDouble("Spring factor", &m_springFactor, 0.0, 0.0, "%.4g");
    if (ImGui::Combo("Spring mode", &spring_mode_index, spring_mode_items, IM_ARRAYSIZE(spring_mode_items)))
      m_springMode = springModeFromComboIndex(spring_mode_index);

    ImGui::BeginDisabled(!m_rigidModeStabilization);
    ImGui::InputDouble("Rigid mode factor", &m_rigidModeStabilizationFactor, 0.0, 0.0, "%.4g");
    if (m_rigidModeStabilizationFactor <= 0.0)
      m_rigidModeStabilizationFactor = 1.0e-3;
    ImGui::Checkbox("Fade constrained modes on contact", &m_rigidModeContactFade);
    ImGui::BeginDisabled(!m_rigidModeContactFade);
    ImGui::InputDouble("Rigid mode contact fade scale", &m_rigidModeContactFadeScale, 0.0, 0.0, "%.4g");
    if (m_rigidModeContactFadeScale <= 0.0)
      m_rigidModeContactFadeScale = 1.0;
    ImGui::EndDisabled();
    ImGui::EndDisabled();

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
      step->m_stabAlphaFree = m_stabAlphaFree;
      step->m_stabAlphaContact = m_stabAlphaContact;
      step->m_stabHgCoeffFree = m_stabHgCoeffFree;
      step->m_stabHgCoeffContact = m_stabHgCoeffContact;
      step->m_stabAvCoeffDiv = m_stabAvCoeffDiv;
      step->m_stabAvCoeffBulk = m_stabAvCoeffBulk;
      step->m_stabLogFactor = m_stabLogFactor;
      step->m_stabPspgScale = m_stabPspgScale;
      step->m_stabPspgBulkFactor = m_stabPspgBulkFactor;
      step->m_stabJMin = m_stabJMin;
      step->m_stabHgVisc = m_stabHgVisc;
      step->m_stabHgStiff = m_stabHgStiff;
      step->m_outTime = m_outTime;
      step->m_fixedTS = m_fixedTS;
      step->m_axiSymmVol = m_axiSymmVol;
      step->m_elemLengthFraction = m_elemLengthFraction;
      step->m_implicitFormulation = static_cast<ImplicitFormulation>(m_implicit_formulation);
      step->m_implicitType = m_implicit_type;
      step->m_velTol = m_velTol;
      step->m_bubbleVelTol = m_bubbleVelTol;
      step->m_pressTol = m_pressTol;
      step->m_forceTol = m_forceTol;
      step->m_divTol = m_divTol;
      step->m_omegaV = m_omegaV;
      step->m_omegaP = m_omegaP;
      step->m_maxIter = m_maxIter;
      step->m_timeStepGrowthFactor = m_timeStepGrowthFactor;
      step->m_initialTimeStep = m_initialTimeStep;
      step->m_maxStepRetries = m_maxStepRetries;
      step->m_nonConvergenceCutbackFactor = m_nonConvergenceCutbackFactor;
      step->m_useWeakSprings = m_useWeakSprings;
      step->m_springFactor = m_springFactor;
      step->m_springStiffness = m_springStiffness;
      step->m_springMode = m_springMode;
      step->m_rigidModeStabilization = m_rigidModeStabilization;
      step->m_rigidModeStabilizationFactor = m_rigidModeStabilizationFactor;
      step->m_rigidModeContactFade = m_rigidModeContactFade;
      step->m_rigidModeContactFadeScale = m_rigidModeContactFadeScale;
      step->m_adaptiveDtLimiter = m_adaptiveDtLimiter;
      step->m_adaptiveDtMin = m_adaptiveDtMin;
      step->m_maxNodalDisplacementPerStep = m_maxNodalDisplacementPerStep;
      step->m_maxEffectiveStrainIncrementPerStep = m_maxEffectiveStrainIncrementPerStep;
      step->m_picardUseFixedPointResidual = m_picardUseFixedPointResidual;
      step->m_picardStagnationEnabled = m_picardStagnationEnabled;
      step->m_picardStagnationWindow = m_picardStagnationWindow;
      step->m_picardStagnationMinIter = m_picardStagnationMinIter;
      step->m_picardStagnationMinImprovement = m_picardStagnationMinImprovement;
      step->m_picardStagnationMaxWindows = m_picardStagnationMaxWindows;
      step->m_picardUseAitken = m_picardUseAitken;
      step->m_picardAitkenMinOmega = m_picardAitkenMinOmega;
      step->m_picardAitkenMaxOmega = m_picardAitkenMaxOmega;
      step->m_picardUseAitkenBubble = m_picardUseAitkenBubble;
      step->m_omegaBubble = m_omegaBubble;
      step->m_picardAitkenBubbleMinOmega = m_picardAitkenBubbleMinOmega;
      step->m_picardAitkenBubbleMaxOmega = m_picardAitkenBubbleMaxOmega;
      step->m_picardSmoothViscosity = m_picardSmoothViscosity;
      step->m_picardStrainRateRegularization = m_picardStrainRateRegularization;
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

bool ShowEditStepDialog(bool* p_open, StepDialog *stepdlg, Step *step, bool is3D) {
  ImGui::SetNextWindowSize(ImVec2(500, 500), ImGuiCond_FirstUseEver);
  stepdlg->Draw("Step", p_open, step, is3D);
  return stepdlg->m_saved;
}
