#ifndef _VECTOR_H_
#define _VECTOR_H_

///// IF PYTHON HBUILD
///// SWIG SEARCHES FOR floor
#ifdef __GNU__
#include <cmath>
#endif
struct Vector2i
{
    int x;
    int y;
};

struct Vector2f
{
    double x;
    double y;

    Vector2f()
    {
    }

    Vector2f(double _x, double _y)
    {
        x = _x;
        y = _y;
    }
};


struct Vector3f
{
    double x;
    double y;
    double z;

    Vector3f() {}

    Vector3f(double _x, double _y, double _z)
    {
        x = _x;
        y = _y;
        z = _z;
    }

    Vector3f(const double* pdouble)
    {
        x = pdouble[0];
        y = pdouble[0];
        z = pdouble[0];
    }

    Vector3f(double f)
    {
        x = y = z = f;
    }

    Vector3f& operator+=(const Vector3f& r)
    {
        x += r.x;
        y += r.y;
        z += r.z;

        return *this;
    }

    Vector3f& operator-=(const Vector3f& r)
    {
        x -= r.x;
        y -= r.y;
        z -= r.z;

        return *this;
    }

    Vector3f& operator*=(double f)
    {
        x *= f;
        y *= f;
        z *= f;

        return *this;
    }

    Vector3f& operator/(double f)
    {
        x /= f;
        y /= f;
        z /= f;

        return *this;
    }

    operator const double*() const
    {
        return &(x);
    }


    Vector3f Cross(const Vector3f& v) const{
      Vector3f ret; // TO CHANGE
      
      return ret;
    }

    Vector3f& Normalize(){
      return *this; //TO CHANGE
    }

    void Rotate(double Angle, const Vector3f& Axis){}

    void Print() const
    {
        printf("(%.02f, %.02f, %.02f)", x, y, z);
    }
};


struct Vector4f
{
    double x;
    double y;
    double z;
    double w;

    Vector4f()
    {
    }

    Vector4f(double _x, double _y, double _z, double _w)
    {
        x = _x;
        y = _y;
        z = _z;
        w = _w;
    }

    void Print(bool endl = true) const
    {
        printf("(%.02f, %.02f, %.02f, %.02f)", x, y, z, w);

        if (endl) {
            printf("\n");
        }
    }

    Vector3f to3f() const
    {
        Vector3f v(x, y, z);
        return v;
    }
};



inline Vector3f operator+(const Vector3f& l, const Vector3f& r)
{
    Vector3f Ret(l.x + r.x,
                 l.y + r.y,
                 l.z + r.z);

    return Ret;
}

inline Vector3f operator-(const Vector3f& l, const Vector3f& r)
{
    Vector3f Ret(l.x - r.x,
                 l.y - r.y,
                 l.z - r.z);

    return Ret;
}

inline Vector3f operator*(const Vector3f& l, double f)
{
    Vector3f Ret(l.x * f,
                 l.y * f,
                 l.z * f);

    return Ret;
}

#endif
