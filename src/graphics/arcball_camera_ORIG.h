#ifndef _ARCBALL_CAMERA_H_
#define _ARCBALL_CAMERA_H_

////////////////////////////////////////////////////
///////////////////////// https://github.com/oguz81/ArcballCamera/blob/main/main.cpp
/// CAMERA ROTATES ONLY WHEN FLAG IS ACTIVE

class ArcballCamera{
public:
    
    glm::vec3 position;
    glm::vec3 startPos;
    glm::vec3 currentPos;
    glm::vec3 startPosUnitVector;
    glm::vec3 currentPosUnitVector;

    Quaternion currentQuaternion;
    Quaternion lastQuaternion;
    
    bool flag; //Rotation is active
    
    float cosValue, cosValue_2;
    float theta;
    float angle = 180.0f;
    glm::vec3 rotationalAxis = glm::vec3(1.0f, 0.0f, 0.0f);                       
    glm::vec3 rotationalAxis_2;
    ArcballCamera ();
    float z_axis(float,float);
    glm::vec3 getUnitVector(glm::vec3);
    
    void OnMouse(float x, float y);
    void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    void mouse_pos_callback(GLFWwindow* window, int xpos, int ypos);
    void setRotFlag(const bool &f){flag=f;}
    
    float dotProduct();
    void rotation();
    void replace();

    
};


#endif