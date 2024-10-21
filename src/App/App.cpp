#include "App.h"

App * App::_pcSingleton = nullptr;

void App::initApp(){
    App::_pcSingleton = new App();
}


void App::setActiveModel(Model *m){
    _activeModel = m;
}
