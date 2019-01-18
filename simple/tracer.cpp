#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <random>

#define SAMPLES 16
#define BOUNCES 4

#define M_PI 3.14159263

std::default_random_engine generator;
std::uniform_real_distribution<float> distribution(0, 1);

float randomVal() {return distribution(generator);}

struct Vec {
    float x, y, z;
    Vec (float a=0) {x=y=z=a;}
    Vec (float a, float b, float c=0) {x=a;y=b;z=c;}
    Vec operator+(Vec o) {return Vec(x + o.x, y + o.y, z + o.z);}
    Vec operator*(Vec o) {return Vec(x * o.x, y * o.y, z * o.z);}
    float operator%(Vec o) {return x*o.x+ y*o.y + z*o.z;}
    Vec operator!() {return *this * (1 / sqrtf(*this % *this));}
    Vec cross(Vec b) {return Vec(
        y * b.z - z * b.y,
        z * b.x - x * b.z,
        x * b.y - y * b.x
    );}
    Vec limit(float max) {return Vec(
        x > max ? max : x,
        y > max ? max : y,
        z > max ? max : z
    );}

};

float SphereTest(Vec p, Vec c, float r) {
    Vec disp = p + c * -1;
    return sqrtf(disp%disp) - r;
}

#define HIT_NONE 0
#define HIT_BALL 1
float Distance(Vec p, int &hitType) {
    hitType = HIT_BALL;
    // float distance = SphereTest(p, Vec(-0.7, 1, 0), 1) + SphereTest(p, Vec(0.7, 1, 0), 0.8);
    float distance = SphereTest(p, Vec(0, 0, -5), 1);
    return distance;
}

int RayMarch(Vec origin, Vec direction, Vec &hitPos, Vec &hitNorm) {
    float d;
    int noHitCount = 0;
    for (float total_d = 0; total_d < 100; total_d += d) {
        hitPos = origin + direction * total_d;
        int hitType = 0;
        d = Distance(hitPos, hitType);
        if (d < 0.01) {
            hitNorm = Vec(
                Distance(hitPos + Vec(0.01, 0), noHitCount) - d,
                Distance(hitPos + Vec(0, 0.01), noHitCount) - d,
                Distance(hitPos + Vec(0, 0, 0.01), noHitCount) - d
            );
            return hitType;
        } else if (++noHitCount > 99) {
            return HIT_NONE;
        }
    }
    return HIT_NONE;
}

Vec UniformHemisphereSampler(Vec normal) {
    Vec tangent = !Vec(normal.y, -normal.x);
    Vec bitangent = !normal.cross(tangent);

    float angle = 2 * M_PI * randomVal();
    float height = randomVal();
    return !(normal * height + tangent * cosf(angle) + bitangent * sinf(angle));
}

Vec Trace(Vec origin, Vec direction, int depth=0) {
    if (depth > BOUNCES) return Vec(0);

    Vec pos, normal;
    int hitType = RayMarch(origin, direction, pos, normal);

    // Sky color
    if (hitType == HIT_NONE) return Vec(1, 1, 1);

    Vec lightDir = (0.6, 0.6, 0.6);
    if (hitType == HIT_BALL) {
        Vec emmitance(0);

        Vec dir = UniformHemisphereSampler(normal);
        Vec incoming = Trace(pos + normal * 0.01, dir, depth + 1);
        float cos_theta = dir % normal;
        Vec BRDF = Vec(1, 0, 0) * (1 / M_PI);
        return incoming * BRDF * cos_theta;
    }
}

Vec CalcPixel(int x, int y, const int w, const int h, Vec pos, Vec goal, Vec up, Vec right) {
    Vec color;
    for (int p = SAMPLES; p--;) {
        Vec direction = !(goal + right*(x-w/2) + up*(y-h/2));
        color = color + Trace(pos, direction);
    }
    return color * (2 * M_PI / SAMPLES);
}

int main() {
    const int w = 192;
    const int h = 108;

    Vec position(0);
    Vec goal(0, 0, -1);
    Vec up = Vec(0, 1, 0) * (1. / w);
    Vec right = goal.cross(up);

    char pixels[w * h * 3];
    printf("Starting path tracing...\n");
    for (int y = h; y--;) {
        for (int x = w; x--;) {
            Vec direction = !(goal + right * (x - w/2) + up * (y - h/2));
            Vec color = CalcPixel(x,y,w,h,position,goal,up,right).limit(1);
            int i = (y * w + x) * 3;
            pixels[i] = (int)(color.x * 255);
            pixels[i + 1] = (int)(color.y * 255);
            pixels[i + 2] = (int)(color.z * 255);
        }
    }
    printf("Finished path tracing, now outputting to 'simple.ppm'\n");

    FILE *fp;
    if (fopen_s(&fp, "simple.ppm", "wb") != 0) {
        printf("Failed to open output file");
        return -1;
    }
    fprintf(fp, "P6 %d %d 255\n", w, h);
    for (int i = 0; i < w * h * 3; i++) {
        fprintf(fp, "%c", pixels[i]);
    }
    fclose(fp);
    printf("All done!");

    return 0;
}