#ifndef _STEP_H_
#define _STEP_H_

#include "Entity.h"
#include <string>

enum StepType {
  ExplicitStep = 0,
  ImplicitStep = 1
};

enum class ImplicitFormulation {
  RigidViscoplastic = 0,
  J2Elastoplastic = 1
};

inline const char* implicitFormulationToConfigString(ImplicitFormulation formulation)
{
  switch (formulation) {
    case ImplicitFormulation::J2Elastoplastic:
      return "j2_elastoplastic";
    case ImplicitFormulation::RigidViscoplastic:
    default:
      return "rigid_viscoplastic";
  }
}

inline ImplicitFormulation implicitFormulationFromConfigString(const std::string& value)
{
  if (value == "j2_elastoplastic" || value == "j2" || value == "elastoplastic_j2")
    return ImplicitFormulation::J2Elastoplastic;
  return ImplicitFormulation::RigidViscoplastic;
}

class Step : public Entity {
public:
  Step() {
    m_id = 0;
    m_name = "Step-1";
  }

  void setStepType(StepType type) { m_step_type = type; }
  StepType getStepType() const { return m_step_type; }
  bool isImplicit() const { return m_step_type == ImplicitStep; }
  bool usesRigidViscoplasticImplicitSolver() const { return m_implicitFormulation == ImplicitFormulation::RigidViscoplastic; }
  bool usesJ2ImplicitSolver() const { return m_implicitFormulation == ImplicitFormulation::J2Elastoplastic; }

  int m_nproc = 1;
  double m_cflFactor = 0.3;
  bool m_autoTS[3] = {false, false, false};
  bool m_kernelGradCorr = false;
  double m_simTime = 200.0;
  double m_artifViscAlpha = 1.0;
  double m_artifViscBeta = 0.0;
  // Explicit solver stabilization. Defaults match weldform-engine/Domain_d.h.
  double m_stabAlphaFree = 0.0;
  double m_stabAlphaContact = 0.0;
  double m_stabHgCoeffFree = 0.0;
  double m_stabHgCoeffContact = 0.0;
  double m_stabAvCoeffDiv = 0.0;
  double m_stabAvCoeffBulk = 0.0;
  double m_stabLogFactor = 0.0;
  double m_stabPspgScale = 0.0;
  double m_stabPspgBulkFactor = 0.0;
  double m_stabJMin = 0.0;
  double m_stabHgVisc = 0.1;
  double m_stabHgStiff = 0.1;
  double m_outTime = 1.0;
  bool m_fixedTS = false;
  bool m_axiSymmVol = false;
  double m_elemLengthFraction = 0.2;

  ImplicitFormulation m_implicitFormulation = ImplicitFormulation::RigidViscoplastic;
  std::string m_implicitType = "Picard";
  double m_velTol = 5e-2;
  double m_pressTol = 10.0;
  double m_forceTol = 10.0;
  double m_divTol = 1.0;
  double m_omegaV = 0.4;
  double m_omegaP = 0.1;
  int m_maxIter = 200;
  double m_timeStepGrowthFactor = 1.2;
  bool m_useWeakSprings = false;
  double m_springFactor = 1.0e-7;
  double m_springStiffness = 0.0;
  int m_springMode = 1;
  bool m_adaptiveDtLimiter = false;
  double m_adaptiveDtMin = 1.0e-7;
  double m_maxNodalDisplacementPerStep = 0.0005;
  double m_maxEffectiveStrainIncrementPerStep = 0.02;

protected:
  StepType m_step_type = ExplicitStep;
};

#endif
