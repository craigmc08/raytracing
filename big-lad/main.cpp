#include <stdio.h>
#include <stdlib.h>

#include "Vector.hpp"
#include "Ray.hpp"
#include "Camera.hpp"
#include "util.hpp"

#define WIDTH 384
#define HEIGHT 216 
#define FOV 90
#define BOUNCES 4
#define SAMPLES 32

#define FILENAME "image.ppm"

#define PI 3.141592653
#define TWO_PI 6.283185307

Vector cameraPos(-3, 5, 5);
float azimuth = -PI / 4;
float cameraZRot = -PI / 6;
// Vector cameraPos(0, 5, 0);
// float azimuth = -PI / 2;
// float cameraZRot = 0;

Vector Luminance(int x, int y, Camera camera);
Vector Trace(Ray ray, int depth=0);
Vector TraceLight(RayHit hit, Vector &lightDir);
float GetDistance(Vector position, int &hitType);

int main() {
    FILE* fp;
    if (fopen_s(&fp, FILENAME, "wb") != 0) {
        fprintf(fp, "Failed to open file.");
        return -1;
    }
    fprintf(fp, "P6 %d %d 255\n", WIDTH, HEIGHT);

    Camera camera(WIDTH, HEIGHT, FOV);
    camera.setPosition(cameraPos);
    camera.setZRot(cameraZRot);
    camera.setAzimuth(azimuth);
    camera.cacheLookDir();

    int pixels[WIDTH * HEIGHT * 3];
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = WIDTH; x--;) {
            // Ray ray = camera.getCameraRay(x, y);
            // printf("\n[%f, %f, %f] ", ray.direction.x, ray.direction.y, ray.direction.z);
            Vector luminance = Luminance(x, y, camera);
            Vector color = luminance * 255;
            // int i = (y * WIDTH + x) * 3;
            int r = color.x > 255 ? 255 : (int)color.x;
            int g = color.y > 255 ? 255 : (int)color.y;
            int b = color.z > 255 ? 255 : (int)color.z;
            fprintf(fp, "%c%c%c", r, g, b);
        }
    }

    fclose(fp);
}

Vector Luminance(int x, int y, Camera camera) {
    Vector sum(0);
    for (int p = SAMPLES; p--;) {
        Ray ray = camera.getCameraRay(x, y);
        sum = sum + Trace(ray);
    }
    // Divide by samples and multiply by set volume (surface area of unit hemisphere)
    Vector incoming = sum / SAMPLES * TWO_PI;

    // Could ray march and get material to determine emission
    Vector emission(0);

    return emission + incoming;
}

Vector TraceLight(RayHit hit, Vector &lightDir) {
    int material = hit.material;
    Vector normal = hit.normal;
    Vector hitPos = hit.hitPos;

    if (material == 0) return Vector(0);

    Vector lightPos(0, 5, 0);
    Vector lightColor(1, 0.95, 0.85);
    float sqrLightRange = 10 * 10;

    Vector lightDisp = lightPos - hitPos;
    lightDir = !lightDisp;

    float sqrLightDist = lightDisp.sqrMagnitude();
    float lightPower = max(0, (sqrLightRange - sqrLightDist) / sqrLightRange);
    float lightStrength = 1;

    if (lightStrength > 0) {
        Ray lightRay = {
            hitPos + normal * 0.05,
            lightDir
        };
        RayHit lightCast = RayMarch(lightRay, &GetDistance, sqrtf(sqrLightDist));
        if (lightCast.material != 0) {
            lightStrength = 0;
        }
    }

    return lightColor * lightStrength;
}
Vector Trace(Ray ray, int depth) {
    if (depth > BOUNCES) return Vector(0);

    RayHit hit = RayMarch(ray, &GetDistance);
    int material = hit.material;
    Vector normal = hit.normal;
    Vector hitPos = hit.hitPos;

    Vector lightDir;
    Vector incomingLight = TraceLight(hit, lightDir);

    if (material == 0) return Vector(0);

    if (material == 1) {
        // Ball
        Vector newDir = ray.direction + normal * (normal % ray.direction * -2);
        Ray reflectRay = {
            hitPos + normal * 0.05,
            newDir
        };
        Vector L_i = Trace(reflectRay, depth + 1);

        // Specular highlight of light
        Vector halfVector = !(lightDir + ray.direction);
        float lightAngle = normal.angleTo(lightDir);
        float highlight = exp(-lightAngle * lightAngle / 0.0004);
        incomingLight = incomingLight * highlight;

        return Vector(1, 0.6, 0.9) * (L_i + incomingLight) / TWO_PI;
    }
    if (material == 2) {
        // Floor
        // Calculate checker floor color
        const float spacing = 2;
        const float quarterSpacing = spacing / 4;
        float cx = fmodf(fabsf(hitPos.x) + quarterSpacing, spacing) / spacing * 2 - 1;
        float cy = fmodf(fabsf(hitPos.z) + quarterSpacing, spacing) / spacing * 2 - 1;
        Vector reflectance = cy * cx < 0 ? Vector(0.7) : Vector(1);

        // Incoming light
        Vector tangent = normal.cross(ray.direction);
        Vector bitangent = normal.cross(tangent);
        float theta = random() * TWO_PI;
        float phi = random() * PI / 2;
        Vector newDir = (tangent * cosf(theta) + bitangent * sinf(theta)) * cosf(phi) + normal * sinf(phi);
        Ray newRay = {
            hitPos + normal * 0.05,
            newDir
        };
        Vector L_i = Trace(newRay, depth + 1) * newDir % normal;
        incomingLight = incomingLight * (lightDir % normal);

        return reflectance * (L_i + incomingLight) / TWO_PI;
    }

    return Vector(0);
}

float GetDistance(Vector p, int &hitType) {
    float distance = 1e9;

    float x = fmodf(fabsf(p.x), 4);
    float z = fmodf(fabsf(p.z), 4);
    Vector modP = Vector(x, p.y, z);
    Vector displacement = Vector(2, 1, 2) - modP;

    distance = displacement.magnitude() - 1;
    hitType = 1;

    float floorDist = p.y;
    if (floorDist < distance) {
        distance = floorDist;
        hitType = 2;
    }

    return distance;
}