#ifndef _BC_H_
#define _BC_H_

#include "Entity.h"


// class BoundaryCondition:
// public Entity{
  
  
// };

#ifndef _BOUNDARY_CONDITION_H_
#define _BOUNDARY_CONDITION_H_

#include <string>
#include "vec3_t.h"

enum BCType {
    VelocityBC,
    DisplacementBC
};

enum BCApplyTo {
    ApplyToPart,
    ApplyToNodes
};

class BoundaryCondition {
public:
    BoundaryCondition() : m_type(VelocityBC), m_applyTo(ApplyToPart), m_targetId(-1) {}
    
    BoundaryCondition(BCType type, BCApplyTo applyTo, int targetId, const Vec3_t &vel)
    : m_type(type), m_applyTo(applyTo), m_targetId(targetId), m_velocity(vel) {}

    void setType(BCType t) { m_type = t; }
    void setApplyTo(BCApplyTo a) { m_applyTo = a; }
    void setTargetId(int id) { m_targetId = id; }
    void setVelocity(const Vec3_t &v) { m_velocity = v; }

    BCType getType() const { return m_type; }
    BCApplyTo getApplyTo() const { return m_applyTo; }
    int getTargetId() const { return m_targetId; }
    Vec3_t getVelocity() const { return m_velocity; }

private:
    BCType m_type;
    BCApplyTo m_applyTo;
    int m_targetId;     // ID de parte o de zona/nodo
    Vec3_t m_velocity;
};

#endif
