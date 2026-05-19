#include "../App/App.h"

void syncScriptModelWithEditor(Model* model)
{
    if (model == nullptr) {
        return;
    }
    getApp().setActiveModel(model);
}
