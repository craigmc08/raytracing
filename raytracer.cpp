#include <stdlib.h>
#include <stdio.h>
#include <math.h>

struct Vec {
    float x, y, z;
    Vec(float a=0) { x = y = z = a; }
    Vec(float a, float b, float c=0) { x=a; y=b; z=c; }
    Vec operator+(Vec a) { return Vec(x + a.x, y + a.y, z + a.z); }
    Vec operator*(Vec a) { return Vec(x * a.x, y * a.y, z * a.z); }
    Vec operator*(float a) { return Vec(x * a, y * a, z * a); }
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
};

#define HIT_NONE 0
#define HIT_FLOOR 1
#define HIT_RED 2
#define HIT_BLUE 3
#define HIT_WALL 4
#define HIT_MIRROR 5
#define HIT_SUN 6

float min(float l, float r) { return l < r ? l : r; }
float random() { return (float)rand() / RAND_MAX; }

float BoxTest(Vec p, Vec c1, Vec c2) {
    c1 = p + c1 * -1;
    c2 = c2 + p * - 1;
    return -min(
        min(
            min(c1.x, c2.x),
            min(c1.y, c2.y)
        ),
        min(c1.z, c2.z)
    );
}
float SphereTest(Vec p, Vec c, float r) {
    Vec d = c + p * -1;
    return sqrtf(d%d) - r;
}

// CSG function to find distance and type of objects
float Query(Vec p, int &hitType) {
    float distance = 1e9;
    hitType = HIT_NONE;

    distance = SphereTest(p, Vec(-2, 1, 0), 1);
    hitType = HIT_RED;

    float blueSphereDist = SphereTest(p, Vec(2, 1, 0), 1);
    if (blueSphereDist < distance) {
        distance = blueSphereDist;
        hitType = HIT_BLUE;
    }

    float wallDist = -BoxTest(p, Vec(-20, 0, -20), Vec(20, 25, 20));
    if (wallDist < distance) {
        distance = wallDist;
        hitType = HIT_WALL;
    }

    float mirrorDist = BoxTest(p, Vec(-1.5, 0, 3), Vec(1.5, 2, 3.5));
    if (mirrorDist < distance) {
        distance = mirrorDist;
        hitType = HIT_MIRROR;
    }

    float sunDist = 7. - p.y;
    if (sunDist < distance) {
        distance = sunDist;
        hitType = HIT_SUN;
    }

    return distance;
}

int RayMarch(Vec origin, Vec direction, Vec &hitNorm, Vec &hitPos) {
    float d;
    int noHitCount;
    int hitType;
    
    for (float total_d = 0; total_d < 100; total_d += d) {
        hitPos = origin + direction * total_d;
        d = Query(hitPos, hitType);
        if (d < 0.01 || ++noHitCount > 99) {
            // Approximate the normal of the surface
            // by finding distances of slightly nudged points
            // noHitCount is used here just to fill that argument, doesn't do anything
            hitNorm = !Vec(
                Query(hitPos + Vec(0.001, 0), noHitCount) - d,
                Query(hitPos + Vec(0, 0.001), noHitCount) - d,
                Query(hitPos + Vec(0, 0, 0.001), noHitCount) - d
            );
            return hitType;
        }
    }

    return HIT_NONE;
}

// FOR LATER (when implementing Lambertian diffuse surfaces)
/*
to get random vector in hemisphere:
Vec B = Vec(normal.y, -normal.x, 0);
Vec C = B.cross(normal);

float angle = 6.28318531 * random();
float height = random();

Vec final = normal * height + B * sinf(angle) + C * cosf(angle);
// that should work
*/
Vec Trace(Vec origin, Vec direction) {
    Vec normal, samplePosition, color;
    Vec ambientColor(0.05, 0.1, 0.12);

    int hitType = RayMarch(origin, direction, normal, samplePosition);

    if (hitType == HIT_RED) {
        color = Vec(255, 0, 0);
    } else if (hitType == HIT_BLUE) {
        color = Vec(0, 0, 255);
    } else if (hitType == HIT_WALL) {
        color = Vec(200, 190, 160);
    } else if (hitType == HIT_FLOOR) {
        color = Vec(70, 70, 70);
    } else if (hitType == HIT_SUN) {
        color = Vec(90, 130, 240);
        // No lighting calcs for sun
        return color;
    } else if (hitType == HIT_MIRROR) {
        Vec newDir = direction + normal * (normal % direction * -2);
        color = Vec(100, 255, 200);
        // Somehow the next trace call causes
        // half the output screen to be black?????
        //
        // Vec reflectColor = Trace(samplePosition, newDir);
        // color = reflectColor * 0.2;
        // return color;
    }

    // Calculate simple lighting
    Vec lightDirection = !Vec(0.6, 1, -0.3);
    float incidence = normal % lightDirection;

    if (incidence < 0) return ambientColor * color;

    // Ray march to check for sun, otherwise this is shadowed
    Vec sunNorm, sunSample;
    hitType = RayMarch(samplePosition + normal * 0.1, lightDirection, sunNorm, sunSample);
    if (hitType == HIT_SUN) {
        // return Vec(255, 255, incidence * 255);
        return color * (ambientColor + incidence);
    }
    return ambientColor * color;
}

int main() {
    int w = 256*1;
    int h = 192*1;
    int samples = 4;

    Vec position(0, 1.3, -9);
    Vec target = !Vec(0, 0, 1);
    Vec up = Vec(0, 1, 0) * (1. / w);
    Vec left = target.cross(up) * -1;

    FILE *fp = fopen("out.ppm", "wb");
    fprintf(fp, "P6 %d %d 255\n", w, h);
    for (int y = h; y--;) {
        for (int x = 0; x < w; x++) {
            Vec color;
            for (int p = 0; p < samples; p++) {
                Vec direction = !(target + left * (x - w / 2 + random()) + up * (y - h / 2 + random()));
                color = color + Trace(position, direction);
            }
            color = color * (1. / samples);
            color = color.limit(255);
            fprintf(fp, "%c%c%c",(int)color.x, (int)color.y, (int)color.z);
        }
        if (fmodf(y, 1000) == 1001) {
            printf("%c", up);
            printf("%c", left);
        }
    }
    fclose(fp);

    return 0;
}