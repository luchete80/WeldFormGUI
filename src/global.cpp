
#include "global.h"

Model *_currModel = NULL;

void add_currModel(Model &model){
  
  _currModel = new Model(model);

}

