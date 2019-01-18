#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <chrono>
#include <inttypes.h>

#define M_PI 3.1415926

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

    float mag() { return sqrtf(*this % *this); }
};

struct Ray {
    int hitType;
    Vec origin;
    Vec direction;
    float traveled;
    Vec normal;
};

#define HIT_NONE 0
#define HIT_FLOOR 1
#define HIT_GLASS 1
struct HitInfo {
    int hitType;
    float distance;
};

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
HitInfo Query(Vec position) {
    float distance = 1e9;
    int hitType = HIT_NONE;

    distance = SphereTest(position, Vec(0, 0.3, 0), 0.3);
    hitType = HIT_GLASS;

    float floorDist = BoxTest(position, Vec(-100, -100, -100), Vec(100, 0, 100)); // Floor
    if (floorDist < distance) {
        distance = floorDist;
        hitType = HIT_FLOOR;
    }

    return { hitType, distance };
}

uint64_t totalRays = 0;
// Signed sphere distance ray marching
Ray RayCast(Vec origin, Vec direction) {
    totalRays++;
    float d = 0;
    int noHitCount = 0;
    for (float total_d = 0; total_d < 100; total_d += d) {
        Vec hitPoint = origin + direction * total_d;
        HitInfo info = Query(hitPoint);
        d = info.distance;
        if (info.distance < 0.01) {
            return {
                info.hitType,
                origin,
                direction,
                total_d,
                !Vec(
                    Query(hitPoint + Vec(0.01, 0)).distance - d,
                    Query(hitPoint + Vec(0, 0.01)).distance - d,
                    Query(hitPoint + Vec(0, 0, 0.01)).distance - d
                )
            };
        } else if (++noHitCount > 99) break;
    }
    return { HIT_NONE, origin, direction, 0, Vec(0) };
}

// Rays continue while distance is negative (inside object)
Ray SubsurfaceRayCast(Vec origin, Vec direction) {
    totalRays++;
    float d = 0;
    int noHitCount = 0;
    for (float total_d = 0; total_d < 100; total_d = d) {
        Vec hitPoint = origin + direction * total_d;
        HitInfo info = Query(hitPoint);
        d = -info.distance;
        if (d < 0.01) {
            return {
                info.hitType,
                origin,
                direction,
                total_d,
                !Vec(
                    -Query(hitPoint + Vec(0.01, 0)).distance - d,
                    -Query(hitPoint + Vec(0, 0.01)).distance - d,
                    -Query(hitPoint + Vec(0, 0, 0.01)).distance - d
                )
            };
        } else if (++noHitCount > 99) break;
    }
    return { HIT_NONE, origin, direction, 0, Vec(0) };
}

Vec UniformHemisphereSampler(Vec normal) {
    Vec tangent = Vec(normal.y, -normal.x);
    Vec bitangent = tangent.cross(normal);

    float angle = 6.28318531 * random();
    float height = random();
    // Selects random unit vector in hemisphere of normal vector
    return !(normal * height + tangent * cosf(angle) + bitangent * sinf(angle));
}

Vec SnellsLaw(Vec normal, Vec s1, float n1, float n2) {
    // Snell's law
    // Formula from
    // http://www.starkeffects.com/snells-law-vector.shtml
    float IORf = n1 / n2;
    Vec crossed = normal.cross(s1);
    Vec subdirP1 = normal.cross((normal * -1).cross(s1)) * IORf;
    Vec subdirP2 = normal * sqrtf(1 - crossed % crossed * IORf * IORf);
    return subdirP1 + subdirP2 * -1;
}

#define BOUNCE_COUNT 12
Vec TracePath(Vec origin, Vec direction, int depth=0) {
    Vec skyColor(1, 1, 1);

    if (depth >= BOUNCE_COUNT) return Vec(0);

    Ray hit = RayCast(origin, direction);
    int hitType = hit.hitType;
    Vec samplePosition = hit.origin + hit.direction * hit.traveled;

    if (hitType == HIT_NONE) {
        // Skybox color
        return skyColor;
    }

    Vec normal = hit.normal;
    Vec lightDir = !Vec(-0.2, 0.4, -0.5);

    // Calculate incoming light
    float sunIncidence = normal % lightDir;
    Vec incomingLight(0);
    if (sunIncidence > 0) {
        Ray sunHit = RayCast(samplePosition + hit.normal * 0.02, lightDir);
        if (sunHit.hitType == HIT_NONE) {
            incomingLight = Vec(1, 1, 1) * sunIncidence;
        }
    }

    if (hitType == HIT_FLOOR) {
        // Lambertian diffuse material
        Vec newDirection = UniformHemisphereSampler(normal);

        // Checkerboard calculation
        const float spacing = 1.;
        const float quarterSpacing = spacing / 4.;
        // Some weird math to determine which color the samplePosition should be
        float cy = fmodf(fabsf(samplePosition.z) + quarterSpacing, spacing) / spacing * 2 - 1;
        float cx = fmodf(fabsf(samplePosition.x) + quarterSpacing, spacing) / spacing * 2 - 1;
        // Set color
        Vec reflectance = cy * cx < 0 ? Vec(0.7) : Vec(1);

        float cos_theta = newDirection % normal;
        Vec brdf = reflectance * (1 / 3.1415926);

        Vec newOrigin = samplePosition + normal * 0.1;
        Vec incoming = TracePath(newOrigin, newDirection, depth + 1);
        incoming = incoming + incomingLight;

        return brdf * incoming * (cos_theta * 6.28318531);
    }

    if (hitType == HIT_GLASS) {
        const float IOR = 1.45;

        Vec subDirection = SnellsLaw(normal, direction, 1, IOR);

        Vec subOrigin = samplePosition + normal * -0.01;
        Ray subHit = SubsurfaceRayCast(subOrigin, subDirection);
        Vec subHitPos = subOrigin + subDirection * subHit.traveled;

        Vec newDirection = SnellsLaw(normal, subDirection, IOR, 1);
        Vec newOrigin = subHitPos + subHit.normal * -0.01;

        TracePath(newOrigin, newDirection, depth + 1);
        Vec brdf = Vec(0.8, 0.2, 0.95) * (1 / M_PI);

        return brdf * incomingLight * 2 * M_PI;
    }

    // if (HitReflective(hitType)) {
    //     // Sharp reflective material
    //     Vec newDirection = !(direction + normal * ((normal % direction) * -2));

    //     Vec reflectance = GetReflectance(hitType);

    //     float cos_theta = newDirection % normal;
    //     Vec brdf = reflectance * (1/ 3.1415926);

    //     Vec newOrigin = samplePosition + normal * 0.1;
    //     Vec incoming = TracePath(newOrigin, newDirection, depth + 1);

    //     // Calculate specular highlight of the light
    //     float lightAngle = acos(lightDir % normal);
    //     float lightArgument = lightAngle / 0.01;
    //     float lightStrength = exp(-lightArgument * lightArgument);

    //     incoming = incoming + (incomingLight * lightStrength);

    //     return brdf * incoming * (0.5 * 6.28318531);
    // }

    return Vec(0);
}

uint64_t GetMicros() {
    std::chrono::high_resolution_clock m_clock;
    return std::chrono::duration_cast<std::chrono::microseconds>(m_clock.now().time_since_epoch()).count();
}

Vec GetCameraRay(float x, float y, const float w, const float h, const float fov) {
    const float aspect = w / h;

    const float pixelMultiplier = tan(fov / 2 * M_PI /  180);
    float px = ((x + 0.5) / w * 2 - 1) * pixelMultiplier * aspect;
    float py = ((y + 0.5) / h * 2 - 1) * pixelMultiplier;
    Vec origin(0);
    Vec rayTarget(px, py, -1);
    return !(rayTarget + origin * -1);
}

int main() {
    const int w = 1920 * 0.2;
    const int h = 1080 * 0.2;
    const int samples = 32;

    const float aperture = 0.08;
    const float focal_length = 2.7; // Distance from camera to sphere
    const float fov = 30.;

    Vec position(-0.87731, 0.16249, -2.67723);
    Vec target = !Vec(0.38, 0.15, 1);
    Vec up = Vec(0, 1, 0);
    Vec right = target.cross(up);

    FILE *fp;
    errno_t err = fopen_s(&fp, "tracer2.ppm", "wb");
    if (err != 0) {
        return -1;
    }
    fprintf(fp, "P6 %d %d 255\n", w, h);

    uint64_t start = GetMicros();
    for (int y = h; y--;) {
        for (int x = 0; x < w; x++) {
            Vec color;
            // Camera space ray
            Vec cd = GetCameraRay((float)(w - x), (float)y, (float)w, (float)h, fov);
            // World space ray
            Vec direction = !(right * cd.x + up * cd.y + target * -cd.z);

            // Focal point
            Vec focal_point = position + direction * focal_length;

            for (int p = 0; p < samples; p++) {
                // Randomly offset origin
                Vec origin = position + right * ((random() - 0.5) * aperture) + up * ((random() - 0.5) * aperture);
                Vec dir = !(focal_point + origin * -1);
                color = color + TracePath(origin, dir);
            }
            color = color * (1. / samples) * 255;
            color = color.limit(255);
            fprintf(fp, "%c%c%c",(int)color.x, (int)color.y, (int)color.z);
        }
    }
    uint64_t end = GetMicros();
    fclose(fp);

    float dtime = (float)(end - start) / 1e6;
    printf("Casted %" PRIu64 " rays in %f seconds @ %f rays per second", totalRays, dtime, (float)totalRays / dtime);

    return 0;
}