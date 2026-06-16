#ifndef _REMESH_DIALOG_H_
#define _REMESH_DIALOG_H_

#include "model/Model.h"

struct RemeshDialog {
  bool m_initialized = false;
  bool m_saved = false;
  bool m_cancelled = false;

  bool m_enabled = true;
  double m_minStrain = -1.0;
  double m_maxStrain = 1.0e6;
  bool m_mapVel = false;
  bool m_mapAcc = false;
  int m_maxCount = 1000000;
  double m_dampFactor = 0.02;
  double m_minFrac = 2.0;
  double m_maxFrac = 2.0;
  double m_epsRef = 1.0;
  double m_beta = 4.0;
  int m_type = 0;
  bool m_refineOnlyBoundary = false;
  int m_boundaryLayers = 2;
  bool m_debug = false;
  double m_minElemAngle = 15.0;
  double m_maxElemAngle = 165.0;
  double m_transitionAngle = 15.0;

  void InitFromModel(Model* model);
  void Draw(const char* title, bool* p_open = nullptr, Model* model = nullptr);
};

bool ShowEditRemeshDialog(bool* p_open, RemeshDialog* remeshdlg, Model* model);

#endif
