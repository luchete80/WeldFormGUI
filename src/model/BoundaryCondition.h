
#ifndef _BOUNDARY_CONDITION_H_
#define _BOUNDARY_CONDITION_H_



#include "Condition.h"
// class BoundaryCondition:
// public Entity{
  
  
// };


#include <string>
//#include "double3.h"



class BoundaryCondition:public Condition {
public:
    //BoundaryCondition() : m_type(VelocityBC), m_applyTo(ApplyToPart), m_targetId(-1) {}
    
    BoundaryCondition(BCType t, BCApplyTo a, int id, double3 vel)
        : Condition(ConditionKind::Boundary, a, id)
    {
        m_type = t;
        m_velocity = vel;
        m_normal = vel;
    }

    std::string getName() const override {
        return "BoundaryCondition";
    }
    
    virtual ~BoundaryCondition(){}

private:

};

#endif
