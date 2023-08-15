#ifndef _QUATERNION_H_
#define _QUATERNION_H_

#include <glm/glm.hpp>

struct Quaternion{
    float cosine; //cosine of half the rotation angle
    glm::vec3 axis; //unit vector scaled by sine of half the angle

};


#endif