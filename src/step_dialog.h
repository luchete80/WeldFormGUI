#ifndef _STEP_DIALOG_H_
#define _STEP_DIALOG_H_

#include "Dialog.h"
#include "model/Step.h"

struct StepDialog {
  bool m_initialized = false;
  bool m_cancelled = false;
  bool m_saved = false;

  int m_step_type = ExplicitStep;
  char m_name[128] = "Step-1";

  int m_nproc = 1;
  double m_cflFactor = 0.3;
  bool m_autoTS[3] = {false, false, false};
  bool m_kernelGradCorr = false;
  double m_simTime = 200.0;
  double m_artifViscAlpha = 1.0;
  double m_artifViscBeta = 0.0;
  double m_outTime = 1.0;
  bool m_fixedTS = false;
  bool m_axiSymmVol = false;
  double m_elemLengthFraction = 0.2;

  bool m_meshingDebug = true;
  double m_maxElemAngle = 150.0;
  double m_minElemAngle = 30.0;

  char m_implicit_type[128] = "Picard";
  double m_velTol = 5e-2;
  double m_pressTol = 10.0;
  double m_forceTol = 10.0;
  double m_divTol = 1.0;
  double m_omegaV = 0.4;
  double m_omegaP = 0.1;
  int m_maxIter = 200;
  double m_timeStepGrowthFactor = 1.2;

  void InitFromStep(Step *step);
  void Draw(const char* title, bool* p_open = nullptr, Step* step = nullptr);
};

bool ShowEditStepDialog(bool* p_open, StepDialog *stepdlg, Step *step);

#endif
