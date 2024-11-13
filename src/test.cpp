#include "App.h"
//#include "Python.h"
#include <iostream>

int main(){
    App::initApp(); //singleton
    bool end= false;
    cout <<"Update App"<<endl;
    getApp().Update();
    getApp().checkUpdate(); 
    
/*
      Py_Initialize();

    PyRun_SimpleString("import sys; sys.path.append('.')");
    PyRun_SimpleString("from model import *");
    PyRun_SimpleString("getApp().Update()");    
    */
    
    //while (!end){
      
      
    //}
}
