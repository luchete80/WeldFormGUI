#ifndef CAMERA_H
#define	CAMERA_H

#include "ogldev_math_3d.h"
#include "ogldev_keys.h"

class Camera
{
public:

    Camera(int WindowWidth, int WindowHeight);

    Camera(int WindowWidth, int WindowHeight, const Vector3f& Pos, const Vector3f& Target, const Vector3f& Up);

    bool OnKeyboard(int Key);

    void OnMouse(int x, int y);

    void OnRender();
    
    void MoveFwd(double offset); //Luciano

    const Vector3f& GetPos() const
    {
        return m_pos;
    }

    void SetPos(const Vector3f& Pos) //LUCIANO
    {
        m_pos = Pos;
    }
    void SetTarget(const Vector3f& Tgt) //LUCIANO
    {
        m_target = Tgt;
    }
    const Vector3f& GetTarget() const
    {
        return m_target;
    }

    //For editing camera which rotates when a button is pressed
    void SetLastMousePos(const int &x, const int &y) //LUCIANO
    {
        m_mousePos.x = x;         m_mousePos.y = y;
    }
    
    void SetUp(const Vector3f& up)
    {
      m_up = up;
    }

    const Vector3f& GetUp() const
    {
        return m_up;
    }

protected:

    void Init();
    void Update();

    Vector3f m_pos;
    Vector3f m_target;
    Vector3f m_up;

    int m_windowWidth;
    int m_windowHeight;

    float m_AngleH;
    float m_AngleV;

    bool m_OnUpperEdge;
    bool m_OnLowerEdge;
    bool m_OnLeftEdge;
    bool m_OnRightEdge;

    Vector2i m_mousePos;
};

#endif	/* CAMERA_H */

