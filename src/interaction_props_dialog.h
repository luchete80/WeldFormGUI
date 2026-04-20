#ifndef _INTERACTION_PROPS_DIALOG_H_
#define _INTERACTION_PROPS_DIALOG_H_

#include "Dialog.h"
#include "model/Model.h"

struct InteractionPropsDialog : public ObjDialog {
  InteractionPropsDialog() : m_model(nullptr) {}

  void setModel(Model *model) { m_model = model; }
  void Draw();

  Model *m_model;
};

#endif
