#include "App.h"
#include "Python.h"
#include <iostream>

int main(){
    App::initApp(); //singleton
    bool end= false;
    cout <<"Update App"<<endl;
    getApp().Update();
    getApp().checkUpdate(); 
    
    //setenv("PYTHONPATH", "/home/weldform-pc/Numerico/WeldFormGUI_bin", 1);
      Py_Initialize();
    //PyRun_SimpleString("from numpy import *");    

    //PyRun_SimpleString("import sys; sys.path.append('/path/to/your/module/directory')");
    //PyRun_SimpleString("import your_module");
      
    //setenv("PYTHONPATH", "/path/to/your/module/directory", 1);
    //Py_Initialize();
    PyRun_SimpleString("import sys; sys.path.append('.')");
    PyRun_SimpleString("from model import *");
    PyRun_SimpleString("getApp().Update()");    
    //while (!end){
      
      
    //}
}
