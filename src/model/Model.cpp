#include "Model.h"
#include "../../libs/LSDynaReader/src/lsdynaReader.h"
#include <iostream>

using namespace std;
using namespace LS_Dyna;

Model::Model(string name){
  cout << "Reading "<<name<<endl;
  lsdynaReader reader(name.c_str());
  
}