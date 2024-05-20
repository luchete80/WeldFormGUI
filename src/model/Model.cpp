#include "Model.h"
#include "libs/LSDynaReader/src/lsdynaReader.h"

using namespace std;
using namespace LS_Dyna;

Model::Model(string name){
  
  lsdynaReader reader(name.c_str());
}