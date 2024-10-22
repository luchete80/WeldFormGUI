#include "App.h"
#include <iostream>

using namespace std; 

App * App::_pcSingleton = nullptr;



void App::initApp(){
  cout << "Initializing App"<<endl;
    if (App::_pcSingleton == nullptr){
      App::_pcSingleton = new App();
      cout << "App address "<<App::_pcSingleton<<endl;
  }
}

bool App::isUpdateNeeded(){
  }


void App::setActiveModel(Model *m){
    _activeModel = m;
}

Model & App::getActiveModel(){
  if (_activeModel==nullptr)
    cout << "ERROR: No Active Model"<<endl;
  else 
    cout << "Model address"<<_activeModel<<endl;
  return *_activeModel;

}
