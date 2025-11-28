#ifndef _CONDITION_H_
#define _CONDITION_H_

#include "Entity.h"

enum class ConditionKind { Boundary, Initial };

enum BCApplyTo {
    ApplyToPart,
    ApplyToNodes
};

enum BCType {
    VelocityBC,
    DisplacementBC,
    SymmetryBC,
    TempBC
};

enum class ICType {
    Temperature,
    Velocity,
    CustomFunction
};


enum class CType {
    BoundaryVelocity,
    BoundaryDisplacement,
    BoundarySymmetry,
    BoundaryTemp,
    InitialVelocity,
    InitialTemperature,
    InitialCustom
};


class Condition {
public:
    ConditionKind kind;


    Condition(ConditionKind k, BCApplyTo a, int id)
        : kind(k), m_applyTo(a), m_targetId(id) {
          
    }

    Condition(){}
    virtual ~Condition() {}

    virtual std::string getName() const {return "";}
    
    virtual double3 getValue() const {return make_double3(0,0,0);}
    //virtual std::array<double,3> 
    //virtual double3 getValue(double x, double y, double z, double t=0.0) const = 0;




    //Condition() : m_type(VelocityBC), m_applyTo(ApplyToPart), m_targetId(-1) {}


    void setType(BCType t) { m_type = t; }
    void setApplyTo(BCApplyTo a) { m_applyTo = a; }
    void setTargetId(int id) { m_targetId = id; }
    void setValue(const double3 &v) { m_velocity = v; }

    BCType getType() const { return m_type; }
    BCApplyTo getApplyTo() const { return m_applyTo; }
    int getTargetId() const { return m_targetId; }
//    double3 getValue() const { return m_velocity; }
    
    void setNormal(const double3 &n) { m_normal = n; }
    double3 getNormal() const { return m_normal; }
    
protected:
    BCType m_type;
    int m_targetId;     // ID de parte o de zona/nodo
    double3 m_velocity;
    double3 m_normal;     // usado en Symmetry
    BCApplyTo m_applyTo;
    
};


#endif
