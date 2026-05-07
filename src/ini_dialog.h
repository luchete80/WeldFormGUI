#ifndef _INI_DIALOG_H_
#define _INI_DIALOG_H_

#include "Dialog.h"
#include "double3.h"

class Model;
class InitialCondition;

struct IniDialog:
public Dialog
{
  bool initialized = false;
  int m_applyTo = 0;
  int m_targetId = -1;
  int m_iniType = 0; // 0 = Velocity, 1 = Temperature
  double3 m_velocity = make_double3(0, 0, 0);
  double m_temperature = 20.0;
  bool m_dof_mask[3] = {true, true, true};

  void Draw(const char* title, bool* p_open, Model* model, InitialCondition** sel_ic);
};

#endif
