
#ifndef _BOUNDARY_CONDITION_H_
#define _BOUNDARY_CONDITION_H_



#include "Condition.h"
// class BoundaryCondition:
// public Entity{
  
  
// };


#include <string>
#include <vector>
#include <cmath>
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
        m_angularVelocity = make_double3(0.0, 0.0, 0.0);
        m_rotationCenter = make_double3(0.0, 0.0, 0.0);
    }

    bool hasAngularMotion() const { return m_hasAngularMotion; }
    void setAngularMotionEnabled(bool enabled) { m_hasAngularMotion = enabled; }
    void setAngularVelocity(const double3 &value) { m_angularVelocity = value; }
    double3 getAngularVelocity() const { return m_angularVelocity; }
    void setRotationCenter(const double3 &center) { m_rotationCenter = center; }
    double3 getRotationCenter() const { return m_rotationCenter; }
    void setAngularValueType(ConditionValueType valueType) { m_angularValueType = valueType; }
    void setAngularValueType(int valueType) { m_angularValueType = static_cast<ConditionValueType>(valueType); }
    ConditionValueType getAngularValueType() const { return m_angularValueType; }
    bool usesAngularAmplitude() const { return m_angularValueType == AmplitudeValue; }
    void setAngularAmplitudeFactor(double factor) { m_angularAmplitudeFactor = factor; }
    double getAngularAmplitudeFactor() const { return m_angularAmplitudeFactor; }
    void setAngularAmplitudeTable(const std::vector<double> &times, const std::vector<double> &values) {
        m_angularAmplitudeTime = times;
        m_angularAmplitudeValue = values;
    }
    const std::vector<double> &getAngularAmplitudeTime() const { return m_angularAmplitudeTime; }
    const std::vector<double> &getAngularAmplitudeValue() const { return m_angularAmplitudeValue; }

    bool isAngularMotionValid(std::string *error = nullptr) const {
        const auto fail = [error](const char *message) {
            if (error != nullptr)
                *error = message;
            return false;
        };
        const auto finite3 = [](const double3 &value) {
            return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
        };
        if (!finite3(m_angularVelocity))
            return fail("angular velocity components must be finite");
        if (!finite3(m_rotationCenter))
            return fail("rotation center components must be finite");
        if (m_angularValueType != ConstantValue && m_angularValueType != AmplitudeValue)
            return fail("angular value type must be Constant (0) or Function (1)");
        if (!std::isfinite(m_angularAmplitudeFactor))
            return fail("angular amplitude factor must be finite");
        if (m_angularValueType == ConstantValue) {
            if (!m_angularAmplitudeTime.empty() || !m_angularAmplitudeValue.empty())
                return fail("constant angular motion cannot contain an amplitude table");
            return true;
        }
        if (m_angularAmplitudeTime.size() != m_angularAmplitudeValue.size() ||
            m_angularAmplitudeTime.size() < 2)
            return fail("angular amplitude table must have matching time/value arrays with at least two points");
        for (std::size_t i = 0; i < m_angularAmplitudeTime.size(); ++i) {
            if (!std::isfinite(m_angularAmplitudeTime[i]) || m_angularAmplitudeTime[i] < 0.0)
                return fail("angular amplitude times must be finite and non-negative");
            if (!std::isfinite(m_angularAmplitudeValue[i]))
                return fail("angular amplitude values must be finite");
            if (i > 0 && !(m_angularAmplitudeTime[i] > m_angularAmplitudeTime[i - 1]))
                return fail("angular amplitude times must be strictly increasing");
        }
        return true;
    }

    std::string getName() const override {
        return "BoundaryCondition";
    }
    
    virtual ~BoundaryCondition(){}

private:
    bool m_hasAngularMotion = false;
    double3 m_angularVelocity{};
    double3 m_rotationCenter{};
    ConditionValueType m_angularValueType = ConstantValue;
    double m_angularAmplitudeFactor = 1.0;
    std::vector<double> m_angularAmplitudeTime;
    std::vector<double> m_angularAmplitudeValue;
};

#endif
