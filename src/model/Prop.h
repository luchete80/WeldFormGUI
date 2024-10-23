#ifndef _PROP_H_
#define _PROP_H_

#include "Entity.h"

class Prop:
public Entity{

public: 
  virtual Prop_Type getType();

protected:
  int m_mat_id;  
  
};


#endif
