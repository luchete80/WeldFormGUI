
#ifndef _INTITAL_CONDITION_H_
#define _INTITAL_CONDITION_H_

#include "Entity.h"


// class BoundaryCondition:
// public Entity{
  
  
// };




#include <string>
//#include "double3.h"

class InitialCondition:public Condition {
public:

    InitialCondition(BCType t, BCApplyTo a, int id, double3 vel)
        : Condition(ConditionKind::Initial, a, id)
    {
        m_type = t;
        m_velocity = vel;
        m_normal = vel;
    }


    virtual ~InitialCondition() {}

    std::string getName() const override {
        return "InitialCondition";
    }

};

#endif
