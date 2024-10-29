#ifndef _APP_H_
#define _APP_H_

#include "Model.h"
#include <iostream>
#include <vector>

using namespace std;

class GraphicMesh;
//class VtkViewer;

class App{

public:  
  friend App &getApp();
  Model &getActiveModel();

  void setActiveModel(Model *);
  bool& isUpdateNeeded();
  void Update(){_updateNeeded=true;}
  void checkUpdate();
  static void initApp();
  void updateMeshes(); 
//protected:
  explicit App(){
    _activeModel  = nullptr;
    _updateNeeded = false;}
  ~App(){}
  int  getGraphicMeshCount(){return m_graphicmeshes.size();}
  GraphicMesh * getGraphicMesh(int &i){return m_graphicmeshes[i];}
private:
  static App *_pcSingleton;  
  Model *_activeModel;
  bool _updateNeeded;
  std::vector <GraphicMesh *> m_graphicmeshes;
}; 



inline App &getApp(){
    //cout << "app "<<App::_pcSingleton<<endl;
    return *App::_pcSingleton;
}


#endif
