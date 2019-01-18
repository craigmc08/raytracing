#include <stdlib.h>
#include <stdio.h>
#include "Util.hpp"

#define BOUNCE_COUNT 4
#define SAMPLES 1
#define WIDTH 192
#define HEIGHT 108
#define FOV 30
#define FILENAME "minimal.ppm"

struct Camera {
    int w;
    int h;
    float fov;
    Vec pos;
    Vec forward;
    Vec right;
    Vec up;
};
float random() { return (float)rand() / RAND_MAX; }

// Get distance to closest object (negative means inside)
float QueryDistance(Vec p, int &hitType) {
    Vec sphereCenter(0, 1, 0);
    const float sphereRadius = 1;
    const float repeat = 4;
    Vec sphereOffset = Vec(p.x, p.y, p.z) - sphereCenter;

    float distance = sqrtf(sphereOffset%sphereOffset) - sphereRadius;
    hitType = 1;

    float floorDist = p.y; // Below 0 is floor
    if (floorDist < distance) {
        distance = floorDist;
        hitType = 2;
    }

    // printf("%f %d\t", distance, hitType);
    return distance;
}

Vec PixelToRay(int x, int y, Camera *camera);

Vec Trace(Vec origin, Vec direction, int depth=0) {
    Vec skyColor(0, 0, 0);

    if (depth >= BOUNCE_COUNT) return Vec(0);

    Ray ray = { origin, direction };
    RayHit hit = RayMarch(ray, &QueryDistance);

    int hitType = hit.hitType;

    Vec lightDir = !Vec(-0.2, 0.4, 0.5);

    if (hitType == 0) return skyColor;
    if (hitType == 1) {
        return Vec(0, 0, 1);
    }
    if (hitType == 2) {
        return Vec(1, 0, 0);
    }

    return Vec(0);
}
Vec Luminance(int x, int y, Camera *camera, int samples) {
    // Monte Carlo integrator of the rendering equation

    // First get emission of object
    // technically, ray march to determine type of object
    // but for now
    Vec emission(0);

    // Trace for the number of samples
    Vec sum(0);
    Vec pixelRay = PixelToRay(x, y, camera);
    for (int p = samples; p--;) {
        sum = sum + Trace(camera->pos, pixelRay);
    }
    Vec luminance = emission + sum * (6.283185307 / samples);

    return luminance;
}

Vec PixelToRay(int x, int y, Camera *camera) {
    int aspect = camera->w / camera->h;
    float pixelMult = tanf(camera->fov / 2 * 3.141592653 / 180);
    float px = ((x + 0.5) / camera->w * 2 - 1) * pixelMult * aspect;
    float py = ((y + 0.5) / camera->h * 2 - 1) * pixelMult;
    
    Vec target(px, py, -1);
    return !(camera->forward * target.z + camera->right * target.x + camera->up * target.y);
}

int main() {
    Vec position(0, 1, -5);
    Vec forward(0, 0, -1);
    Vec right(1, 0, 0);
    Vec up = forward.cross(right);
    Camera camera = {
        WIDTH, HEIGHT, FOV,
        position, forward, right, up
    };

    FILE *fp;
    if (fopen_s(&fp, FILENAME, "wb") != 0) {
        printf("Failed to open file");
        return -1;
    }
    fprintf(fp, "P6 %d %d 255\n", WIDTH, HEIGHT);

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = WIDTH; x--;) {
            Vec luminance = Luminance(x, y, &camera, SAMPLES);
            Vec color = (luminance * 255).limit(255);
            fprintf(fp, "%c%c%c", (int)color.x, (int)color.y, (int)color.z);
        }
    }
    fclose(fp);

    printf("Output to %s", FILENAME);

    return 0;
}