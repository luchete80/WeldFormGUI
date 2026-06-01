#ifndef _STEP_H_
#define _STEP_H_

#include "Entity.h"
#include <string>

enum StepType {
  ExplicitStep = 0,
  ImplicitStep = 1
};

class Step : public Entity {
public:
  Step() {
    m_id = 0;
    m_name = "Step-1";
  }

  void setStepType(StepType type) { m_step_type = type; }
  StepType getStepType() const { return m_step_type; }
  bool isImplicit() const { return m_step_type == ImplicitStep; }

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

  std::string m_implicitType = "Picard";
  double m_velTol = 5e-2;
  double m_pressTol = 10.0;
  double m_forceTol = 10.0;
  double m_divTol = 1.0;
  double m_omegaV = 0.4;
  double m_omegaP = 0.1;
  int m_maxIter = 200;
  double m_timeStepGrowthFactor = 1.2;

protected:
  StepType m_step_type = ExplicitStep;
};

#endif
