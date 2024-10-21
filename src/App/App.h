#ifndef _APP_H_
#define _APP_H_

#include "Model.h"



class App{

public:  
  friend App &GetApplication();
  Model *_activeModel;
  void setActiveModel(Model *);
  static void initApp();
protected:
  explicit App(){}
  ~App(){}
private:
  static App *_pcSingleton;  

}; 



inline App &GetApplication(){
    return *App::_pcSingleton;
}


#endif
