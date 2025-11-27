
#ifndef _INTITAL_CONDITION_H_
#define _INTITAL_CONDITION_H_

#include "Entity.h"


// class BoundaryCondition:
// public Entity{
  
  
// };


#include <string>
//#include "double3.h"

enum class InitialConditionType {
    Temperature,
    Velocity,
    CustomFunction
};

class InitialCondition {
public:
    InitialConditionType type;

    InitialCondition(InitialConditionType t) : type(t) {}
    virtual ~InitialCondition() {}

    // Devuelve un valor genérico: vector de 3 componentes
    // Para temperatura se usa solo val[0]
    virtual std::array<double,3> getValue(double x, double y, double z, double t=0.0) const = 0;
};

#endif
