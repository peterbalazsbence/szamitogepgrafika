#ifndef DOOM3D_MATH3D_H
#define DOOM3D_MATH3D_H

#include "common.h"

typedef struct { float x, y, z; } Vec3;

static inline Vec3 v3(float x, float y, float z) { Vec3 v={x,y,z}; return v; }
static inline Vec3 v3add(Vec3 a, Vec3 b) { return v3(a.x+b.x, a.y+b.y, a.z+b.z); }
static inline Vec3 v3sub(Vec3 a, Vec3 b) { return v3(a.x-b.x, a.y-b.y, a.z-b.z); }
static inline Vec3 v3scale(Vec3 a, float s) { return v3(a.x*s, a.y*s, a.z*s); }
static inline float v3dot(Vec3 a, Vec3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
static inline float v3len(Vec3 a) { return sqrtf(v3dot(a,a)); }
static inline Vec3 v3norm(Vec3 a) {
    float l=v3len(a);
    if(l<1e-6f) return v3(0,0,0);
    return v3scale(a, 1.0f/l);
}

static inline float randf(void) { return (float)rand()/(float)RAND_MAX; }
static inline float randf_range(float lo, float hi) { return lo + randf()*(hi-lo); }

#endif
