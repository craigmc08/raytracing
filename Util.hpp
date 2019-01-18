#ifndef _UTIL_HPP

#include <math.h>

struct Vec {
    float x, y, z;
    Vec(float a=0) { x = y = z = a; }
    Vec(float a, float b, float c=0) { x=a; y=b; z=c; }
    Vec operator+(Vec a) { return Vec(x + a.x, y + a.y, z + a.z); }
    Vec operator-(Vec a) { return Vec(x + a.x, y + a.y, z + a.z); }
    Vec operator*(Vec a) { return Vec(x * a.x, y * a.y, z * a.z); }
    Vec operator/(Vec a) { return Vec(x / a.x, y / a.y, z / a.z); }

    float operator%(Vec a) { return x * a.x + y * a.y + z * a.z; }
    Vec operator!() { return *this * (1 / sqrtf(*this % *this)); }

    Vec cross(Vec b) { return Vec(
        y * b.z - z * b.y,
        z * b.x - x * b.z,
        x * b.y - y * b.x
    ); }

    Vec limit(float max) { return Vec(
        x > max ? max : x,
        y > max ? max : y,
        z > max ? max : z
    ); }

    float angleTo(Vec b) {
        return acosf(*this % b);
    }

    static Vec reflect(Vec normal, Vec incoming) {
        return incoming + normal * (normal % incoming * -2);
    }
    static Vec halfVector(Vec a, Vec b) {
        return (a + b) / 2;
    }

    float mag() { return sqrtf(*this % *this); }
};

float min(float l, float r) { return l < r ? l : r; }

struct Ray {
    Vec origin;
    Vec direction;
};
struct RayHit {
    Ray ray;
    Vec hitPos;
    Vec hitNorm;
    int hitType;
    int steps;
    float distanceTraveled;
    float distance;
    float closestDistance;
};

void GetTangents(Vec normal, Vec &tangent, Vec &bitangent) {
    tangent = Vec(normal.y, -normal.x);
    bitangent = tangent.cross(normal);
}

typedef float (*DistanceEstimator)(Vec, int&);
RayHit RayMarch(Ray ray, DistanceEstimator estimator, float maxDist=100) {
    float d = 0;
    float minDist = 1e9;
    int noHitCount = 0;
    int hitType = 0;
    int steps = 0;
    float totalD;
    for (totalD = 0; totalD < maxDist; totalD += d) {
        steps += 1;
        Vec hitPoint = ray.origin + ray.direction * totalD;
        d = estimator(hitPoint, hitType);
        minDist = min(minDist, d);
        totalD += d;
        if (d < 0.01) {
            Vec hitNorm = !(Vec(
                estimator(hitPoint + Vec(0.01, 0), noHitCount) - d,
                estimator(hitPoint + Vec(0, 0.01), noHitCount) - d,
                estimator(hitPoint + Vec(0, 0, 0.01), noHitCount) - d
            ));
            return {
                ray,
                hitPoint,
                hitNorm,
                hitType,
                steps,
                totalD,
                d,
                minDist
            };
        } else if (++noHitCount > 99) break;
    }
    return { ray, ray.origin, 0, 0, steps, totalD, d, minDist };
}

#endif // _UTIL_HPP