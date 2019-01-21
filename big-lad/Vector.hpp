#ifndef _VECTOR_H
#define _VECTOR_H

#include <math.h>

struct Vector {
    float x, y, z;
    Vector(float a=0) { x = y = z = a; }
    Vector(float a, float b, float c) {
        x = a;
        y = b;
        z = c;
    }

    Vector operator+(Vector o) {
        return Vector(x + o.x, y + o.y, z + o.z);
    }
    Vector operator+(float o) {
        return Vector(x + o, y + o, z + o);
    }
    Vector operator-(Vector o) {
        return *this + o * -1;
    }
    Vector operator-(float o) {
        return *this + o * -1;
    }

    Vector operator*(Vector o) {
        return Vector(x * o.x, y * o.y, z * o.z);
    }
    Vector operator*(float o) {
        return Vector(x * o, y * o, z * o);
    }
    Vector operator/(Vector o) {
        return Vector(x / o.x, y / o.y, z / o.z);
    }
    Vector operator/(float o) {
        return *this * (1. / o);
    }

    float operator%(Vector o) {
        return x * o.x + y * o.y + z * o.z;
    }
    float sqrMagnitude() {
        return *this % *this;
    }
    float magnitude() {
        return sqrtf(*this % *this);
    }

    Vector operator-() {
        return Vector(-x, -y, -z);
    }
    Vector operator!() {
        return *this / sqrtf(*this % *this);
    }

    Vector cross(Vector o) {
        return Vector(
            y * o.z - z * o.y,
            z * o.x - x * o.z,
            x * o.y - y * o.x
        );
    }
    
    float angleTo(Vector b) {
        return acosf(*this % b);
    }
};

#endif
