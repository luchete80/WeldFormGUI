#ifndef _APP_H_
#define _APP_H_

#include "Model.h"
#include <iostream>
#include <vector>
#include "geom/vtkOCCTGeom.h"


//#define MY_DLL_API __declspec(dllexport)



using namespace std;

class GraphicMesh;
//class VtkViewer;
//SHOULD ALSO BEE OF INTEREST TO HAVE A REFERENCE TO THE RENDERER
///SINGLETON

class /*__declspec(dllexport)*/  App{

public:  
  friend App &getApp();
  Model &getActiveModel();

  void setActiveModel(Model *);
  bool& isUpdateNeeded();
  void Update(){_updateNeeded=true;}
  void checkUpdate();
  static void initApp();
  void updateMeshes(); 
  void updateGeoms(); 
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
  std::vector <vtkOCCTGeom*> m_geoms;
}; 



/*static */ 
App &getApp();


#endif
