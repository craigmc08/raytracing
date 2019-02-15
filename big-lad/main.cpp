// inspired by that postcard sized raytracer
// multi-threading based on https://bunjevac.net/2019/02/14/parallelizing-raytracer-in-a-weekend.html

#include <stdio.h>
#include <stdlib.h>
#include <random>
#include <mutex>
#include <thread>
#include <chrono>
#include <vector>

#include "Vector.hpp"
#include "Ray.hpp"
#include "Camera.hpp"
#include "util.hpp"
#include "stopwatch.hpp"

#define WIDTH 1920
#define HEIGHT 1080
#define TILE_WIDTH 32
#define TILE_HEIGHT TILE_WIDTH
#define FOV 90
#define BOUNCES 4
#define SAMPLES 64

#define FILENAME "image.ppm"

#define PI 3.141592653
#define TWO_PI 6.283185307
#define ROOT2 1.41421356237

constexpr int TILE_H_COUNT = HEIGHT / TILE_HEIGHT + 1;
constexpr int TILE_W_COUNT = WIDTH / TILE_WIDTH + 1;

Vector cameraPos(-3, 5, 5);
float azimuth = -PI / 4;
float cameraZRot = -PI / 6;

// Global random engine
std::default_random_engine generator;

Vector Trace(Ray ray, int samples, int depth=0);
Vector IncomingLuminance(RayHit surface, int samples, int depth);
Vector IncomingLight(RayHit hit, Vector &lightDir);
float GetDistance(Vector position, int &hitType);

Vector CheckerColor(Vector pos);

Camera camera(WIDTH, HEIGHT, FOV);
int pixels[WIDTH * HEIGHT * 3];
int doneThreads;

// The struct that is in charge of each thread
struct Task {
    Task() : my_id{id++} {}
    Task(int x, int y) : sx{x}, sy{y}, my_id{id++} {}

    bool get_next_task() {
        static bool taken[TILE_H_COUNT][TILE_W_COUNT] = {};
        static std::mutex m;

        std::lock_guard<std::mutex> guard{m};

        bool found = false;
        int x = 0;
        int y = 0;
        while (!found) {
            if (!taken[y][x]) {
                sx = x * TILE_WIDTH;
                sy = y * TILE_HEIGHT;
                taken[y][x] = true;
                found = true;
            } else {
                x++;
                if (x >= TILE_W_COUNT) { x = 0; y++; }
                if (y >= TILE_H_COUNT) break;
            }
        }
        return found;
    }

    void operator()() {
        bool done = false;
        do {
            if (!get_next_task()) {
                done = true;
                continue;
            }

            for (unsigned y = sy; y < sy + TILE_HEIGHT; y++) {
                for (unsigned x = sx; x < sx + TILE_WIDTH; x++) {
                    if (x >= WIDTH || y >= HEIGHT) continue;
                    Vector luminance = Trace(camera.getCameraRay(x, y), SAMPLES);
                    luminance = luminance + (5. / 241.);
                    luminance = luminance / (1. + luminance);
                    Vector color = luminance * 255;
                    int r = color.x > 255 ? 255 : (int)color.x;
                    int g = color.y > 255 ? 255 : (int)color.y;
                    int b = color.z > 255 ? 255 : (int)color.z;
                    int i = (y * WIDTH + x) * 3;
                    pixels[i] = r;
                    pixels[i+1] = g;
                    pixels[i+2] = b;
                }
            }
        } while (!done);

        printf("Thread %d finished.\n", my_id);
        doneThreads++;
    }

    int sx = -1, sy = -1;
    int my_id;
    static int id;
};

int Task::id = 0;

int main() {
    FILE* fp;
    if (fopen_s(&fp, FILENAME, "wb") != 0) {
        fprintf(fp, "Failed to open file.");
        return -1;
    }
    fprintf(fp, "P6 %d %d 255\n", WIDTH, HEIGHT);

    camera.setPosition(cameraPos);
    camera.setZRot(cameraZRot);
    camera.setAzimuth(azimuth);
    camera.cacheLookDir();

    printf("Rendering %d tiles @ %dX%d...\n", TILE_H_COUNT * TILE_W_COUNT, TILE_WIDTH, TILE_HEIGHT);

    // Setup threads
    const unsigned int n_threads = std::thread::hardware_concurrency();
    printf("Detected %d threads.\n", n_threads);
    std::vector<std::thread> threads{n_threads};

    stopwatch runtime;
    
    for (auto &t : threads) t = std::thread{Task{}};

    // Wait for threads to finish (is this good?)
    while (doneThreads != n_threads) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    for (auto &t: threads) t.join();

    long long millis = runtime.elapsed_millis();
    float seconds = (float)millis / 1000.;
    printf("Took %f seconds, avg. of %f tiles per second\n", seconds, TILE_H_COUNT * TILE_W_COUNT / seconds);

    for (int i = 0; i < WIDTH * HEIGHT * 3; i++) {
        fprintf(fp, "%c", pixels[i]);
    }

    fclose(fp);
}

Vector Trace(Ray ray, int samples, int depth) {
    if (depth > BOUNCES) return Vector(0);

    RayHit surface = RayMarch(ray, &GetDistance);

    // Special case for sky
    if (surface.material == 0) return Vector(0); // sky color

    Vector incoming = IncomingLuminance(surface, samples, depth);

    // Could ray march and get material to determine emission
    Vector emission(0);

    return emission + incoming;
}

Vector GetReflectionRay(Vector normal, Vector incoming, float roughness, float *probability) {
    std::normal_distribution<float> distribution(0.0, roughness / ROOT2);

    Vector zAxis = incoming + normal * (normal % incoming * -2);
    Vector xAxis = zAxis.cross(normal);
    Vector yAxis = xAxis.cross(zAxis);
    
    const float pi2 = PI / 2;
    const float pi2Sqr = pi2 * pi2;
    float xzRand = distribution(generator);
    float zyRand = distribution(generator);
    float xzTheta = min(fabsf(xzRand), pi2) * (xzRand < 0 ? -1 : 1);
    float zyTheta = min(fabsf(zyRand), pi2) * (zyRand < 0 ? -1 : 1);

    float sinXZ, cosXZ;
    sincosf(xzTheta, &sinXZ, &cosXZ);
    float sinZY, cosZY;
    sincosf(zyTheta, &sinZY, &cosZY);

    Vector xzRay = zAxis * sinXZ + xAxis * cosXZ;
    Vector reflectRay = xzRay * cosZY + yAxis * sinZY;

    Vector halfRay = !(-incoming + reflectRay);
    float angle = acosf(halfRay % normal);
    *probability = expf(- angle * angle / roughness / roughness);

    return reflectRay;
}

Vector IncomingLight(RayHit hit, Vector &lightDir) {
    int material = hit.material;
    Vector normal = hit.normal;
    Vector hitPos = hit.hitPos;

    if (material == 0) return Vector(0);

    Vector lightPos(0, 5, 0);
    Vector lightColor = Vector(1, 0.95, 0.85) * 2;
    float sqrLightRange = 15 * 15;

    Vector lightDisp = lightPos - hitPos;
    lightDir = !lightDisp;

    float sqrLightDist = lightDisp.sqrMagnitude();
    float lightPower = max(0, (sqrLightRange - sqrLightDist) / sqrLightRange);
    float lightStrength = lightPower;

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
Vector IncomingLuminance(RayHit surface, int samples, int depth) {
    if (depth > BOUNCES) return Vector(0);

    Ray ray = surface.ray;
    int material = surface.material;
    Vector normal = surface.normal;
    Vector hitPos = surface.hitPos;

    Vector lightDir;
    Vector incomingLight = IncomingLight(surface, lightDir);

    Vector sum(0);

    Vector ballColor(1, 0.6, 0.9);

    if (material == 1) {
        // Ball incoming light
        Vector halfLight = !(lightDir + -ray.direction);
        float lightAngle = normal.angleTo(halfLight);
        // Gaussian microfacet brdf
        float lightStrength = exp(-lightAngle * lightAngle / 0.01);
        incomingLight = ballColor * incomingLight * lightStrength / TWO_PI;
    } else if (material == 2) {
        // Floor
        Vector reflectance = CheckerColor(hitPos);
        incomingLight = reflectance * incomingLight * (lightDir % normal) / TWO_PI;
    }
    sum = incomingLight * samples;

    for (int p = samples; p--;) {
        if (material == 1) {
            // Ball
            // Vector newDir = ray.direction + normal * (normal % ray.direction * -2);
            float rayProbability;
            Vector newDir = GetReflectionRay(normal, ray.direction, 0.1, &rayProbability);
            Ray reflectRay = {
                hitPos + normal * 0.05,
                newDir
            };
            Vector halfVector = !(newDir + ray.direction);
            float reflectAngle = acosf(halfVector % normal);
            float reflectStrength = expf(-reflectAngle * reflectAngle / 0.01);
            Vector L_i = Trace(reflectRay, 1, depth + 1);
            Vector reflectance = ballColor * reflectStrength;

            Vector value = reflectance * L_i / 1;
            sum = sum + value;
        } else if (material == 2) {
            // Floor
            Vector reflectance = CheckerColor(hitPos);

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
            Vector L_i = Trace(newRay, 1, depth + 1) * newDir % normal;

            Vector value = reflectance * L_i / TWO_PI;

            sum = sum + value;
        }
    }

    // Final step of monte carlo integration:
    // divide by samples and multiply by set volume
    sum = sum / (samples * 2) * TWO_PI;
    return sum;
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

Vector CheckerColor(Vector pos) {
    const float spacing = 2;
    const float quarterSpacing = spacing / 4;
    float cx = fmodf(fabsf(pos.x) + quarterSpacing, spacing) / spacing * 2 - 1;
    float cy = fmodf(fabsf(pos.z) + quarterSpacing, spacing) / spacing * 2 - 1;
    return cy * cx < 0 ? Vector(0.7) : Vector(1);
}