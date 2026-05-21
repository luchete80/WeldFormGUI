#ifndef _CONDITION_H_
#define _CONDITION_H_

#include "Entity.h"
#include <array>
#include <algorithm>
#include <utility>
#include <vector>

enum class ConditionKind { Boundary, Initial };

enum BCApplyTo {
    ApplyToPart,
    ApplyToNodeSet
};

enum ConditionValueType {
    ConstantValue = 0,
    AmplitudeValue = 1
};

//~ enum BCType {
    //~ VelocityBC,
    //~ DisplacementBC,
    //~ SymmetryBC,
    //~ TempBC
//~ };

//~ enum class ICType {
    //~ Temperature,
    //~ Velocity,
    //~ CustomFunction
//~ };


enum BCType {
    VelocityBC,
    DisplacementBC,
    SymmetryBC,
    TempBC,
    VelocityIC,
    DisplacementIC,
    TempIC
    
    // FUNCTIONS AND SOME OTHERS
};


class Condition {
public:
    ConditionKind kind;


    Condition(ConditionKind k, BCApplyTo a, int id)
        : kind(k), m_applyTo(a), m_targetId(id) {
        m_dofMask = {true, true, true};
    }

    Condition(){
        m_dofMask = {true, true, true};
    }
    virtual ~Condition() {}

    virtual std::string getName() const {return "";}
    
    virtual double3 getValue() const {return m_velocity;}
    //virtual std::array<double,3> 
    //virtual double3 getValue(double x, double y, double z, double t=0.0) const = 0;




    //Condition() : m_type(VelocityBC), m_applyTo(ApplyToPart), m_targetId(-1) {}


    void setType(BCType t) { m_type = t; }
    void setApplyTo(BCApplyTo a) { m_applyTo = a; }
    void setTargetId(int id) { m_targetId = id; }
    void setValue(const double3 &v) { m_velocity = v; }
    void setValueType(ConditionValueType valueType) { m_valueType = valueType; }
    void setValueType(int valueType) {
        m_valueType = (valueType == static_cast<int>(AmplitudeValue)) ? AmplitudeValue : ConstantValue;
    }
    double getValueX() const { return m_velocity.x; }
    double getValueY() const { return m_velocity.y; }
    double getValueZ() const { return m_velocity.z; }
    ConditionValueType getValueType() const { return m_valueType; }
    bool usesAmplitude() const { return m_valueType == AmplitudeValue; }
    void setDofMask(bool x, bool y, bool z) { m_dofMask = {x, y, z}; }
    void setDofMaskX(bool active) { m_dofMask[0] = active; }
    void setDofMaskY(bool active) { m_dofMask[1] = active; }
    void setDofMaskZ(bool active) { m_dofMask[2] = active; }
    void setAmplitudeFactor(double factor) { m_amplitudeFactor = factor; }
    double getAmplitudeFactor() const { return m_amplitudeFactor; }
    void setAmplitudeTable(const std::vector<double> &timeValues, const std::vector<double> &amplitudeValues) {
        m_amplitudeTime.clear();
        m_amplitudeValue.clear();

        const std::size_t pointCount = (std::min)(timeValues.size(), amplitudeValues.size());
        if (pointCount == 0)
            return;

        std::vector<std::pair<double, double>> table;
        table.reserve(pointCount);
        for (std::size_t i = 0; i < pointCount; ++i)
            table.push_back({timeValues[i], amplitudeValues[i]});

        std::stable_sort(table.begin(), table.end(),
                         [](const std::pair<double, double> &lhs, const std::pair<double, double> &rhs) {
                             return lhs.first < rhs.first;
                         });

        m_amplitudeTime.reserve(pointCount);
        m_amplitudeValue.reserve(pointCount);
        for (const auto &point : table) {
            m_amplitudeTime.push_back(point.first);
            m_amplitudeValue.push_back(point.second);
        }
    }
    const std::vector<double> &getAmplitudeTime() const { return m_amplitudeTime; }
    const std::vector<double> &getAmplitudeValue() const { return m_amplitudeValue; }
    
    BCType getType() const { return m_type; }
    BCApplyTo getApplyTo() const { return m_applyTo; }
    int getTargetId() const { return m_targetId; }
//    double3 getValue() const { return m_velocity; }
    bool getDofMaskX() const { return m_dofMask[0]; }
    bool getDofMaskY() const { return m_dofMask[1]; }
    bool getDofMaskZ() const { return m_dofMask[2]; }
    bool isDofActive(int index) const {
        return (index >= 0 && index < 3) ? m_dofMask[index] : false;
    }
    
    void setNormal(const double3 &n) { m_normal = n; }
    double3 getNormal() const { return m_normal; }
    double getNormalX() const { return m_normal.x; }
    double getNormalY() const { return m_normal.y; }
    double getNormalZ() const { return m_normal.z; }
    
protected:
    BCType m_type;
    int m_targetId;     // ID de parte o de zona/nodo
    double3 m_velocity;
    double3 m_normal;     // usado en Symmetry
    BCApplyTo m_applyTo;
    std::array<bool, 3> m_dofMask;
    ConditionValueType m_valueType = ConstantValue;
    double m_amplitudeFactor = 1.0;
    std::vector<double> m_amplitudeTime;
    std::vector<double> m_amplitudeValue;
    
};


#endif
