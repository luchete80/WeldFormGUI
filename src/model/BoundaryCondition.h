
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
    }

    void setType(BCType t) { m_type = t; }
    void setApplyTo(BCApplyTo a) { m_applyTo = a; }
    void setTargetId(int id) { m_targetId = id; }
    void setValue(const double3 &v) { m_velocity = v; }

    BCType getType() const { return m_type; }
    BCApplyTo getApplyTo() const { return m_applyTo; }
    int getTargetId() const { return m_targetId; }
    double3 getValue() const { return m_velocity; }
    
    void setNormal(const double3 &n) { m_normal = n; }
    double3 getNormal() const { return m_normal; }
    

    std::string getName() const override {
        return "BoundaryCondition";
    }
    
    virtual ~BoundaryCondition(){}

private:
    // BCType m_type;

    // int m_targetId;     // ID de parte o de zona/nodo
    // double3 m_velocity;
    // double3 m_normal;     // usado en Symmetry
};

#endif
