#include "App.h"
#include <iostream>
#include "GraphicMesh.h"
#include "Part.h"
#include "Mesh.h"

//#include "VtkViewer.h"

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

void App::checkUpdate(){
  if (_updateNeeded){
      updateMeshes();
  }
}

void App::updateMeshes(){
  cout << "searching on "<<_activeModel->getPartCount()<<" parts"<<endl;
  cout << "mesh address "<<_activeModel->getPart(0)->getMesh()<<endl;
  
  for (int p=0;p<_activeModel->getPartCount();p++){
    bool not_found = true;
      for (int gm=0;gm<m_graphicmeshes.size();gm++){
        if (m_graphicmeshes[gm]->getMesh() == _activeModel->getPart(p)->getMesh()){
            not_found = false;
          
          }
        
        }
      if (not_found){
        cout << "Creating Graphic Mesh for part "<<p<<endl;
        if (_activeModel->getPart(p)->getMesh()!=nullptr)
          m_graphicmeshes.push_back(new GraphicMesh(_activeModel->getPart(p)->getMesh()));
        else 
          cout << "ERROR: Part mesh is null pointer"<<endl;
      }
      } //part
      
  _updateNeeded = false;    
}


