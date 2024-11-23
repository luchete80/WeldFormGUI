#include "Entity.h"

int & getMaxId(std::vector<Entity*> ent){
  int max=-1;
  for(int i=0;i<ent.size();i++){
    if (ent[i]->getId()>max)
      max = ent[i]->getId();
  }
  return max;
}