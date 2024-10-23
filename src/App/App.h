#ifndef _APP_H_
#define _APP_H_

#include "Model.h"
#include <iostream>

using namespace std;

class App{

public:  
  friend App &getApp();
  Model &getActiveModel();

  void setActiveModel(Model *);
  bool isUpdateNeeded();
  void setUpdateNeeded(){_updateNeeded=true;}
  static void initApp();
//protected:
  explicit App(){
    _activeModel  = nullptr;
    _updateNeeded = false;}
  ~App(){}
private:
  static App *_pcSingleton;  
  Model *_activeModel;
  bool _updateNeeded;

}; 



inline App &getApp(){
    cout << "app "<<App::_pcSingleton<<endl;
    return *App::_pcSingleton;
}


#endif
