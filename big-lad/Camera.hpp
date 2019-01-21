#ifndef _CAMERA_H
#define _CAMERA_H

#include <math.h>
#include "Vector.hpp"
#include "Ray.hpp"
#include "util.hpp"

class Camera {
public:
    int width;
    int height;
    float fov;

    Vector position;
    float zRot;
    float azimuth;

    Vector forward;
    Vector right;
    Vector up;

    Camera(int width, int height, float fov) {
        this->width = width;
        this->height = height;
        this->fov = fov;
    }

    void setPosition(Vector position) {
        this->position = position;
    }

    void setZRot(float rot) {
        zRot = rot;
    }
    void setAzimuth(float rot) {
        azimuth = rot;
    }

    void cacheLookDir() {
        Vector axisForward = -Vector(sinf(zRot), 0, cosf(zRot));

        forward = axisForward * cosf(azimuth) + Vector(0, 1, 0) * sinf(azimuth);
        up = axisForward * -sinf(azimuth) + Vector(0, 1, 0) * cosf(azimuth);
        right = forward.cross(up);
    }

    // Calculate ray through image plane and convert to world coordinates
    Ray getCameraRay(int x, int y) {
        float aspect = (float)width / (float)height;
        // Magic number is in fact pi / 2 / 180
        float pixelMult = tanf(fov * 0.00872664625);
        float px = ((x + 0.5 + random()*1) / (float)width * 2 - 1) * pixelMult * aspect;
        float py = ((height - y + 0.5 + random()*1) / (float)height * 2 - 1) * pixelMult;

        Vector target = !Vector(px, py, 1);

        return {
            position,
            forward * target.z + right * target.x + up * target.y
        };
    }
};

#endif
