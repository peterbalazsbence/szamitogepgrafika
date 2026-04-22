#ifndef DOOM3D_PARTICLES_H
#define DOOM3D_PARTICLES_H

#include "common.h"
#include "math3d.h"

typedef enum { PART_FIRE, PART_SMOKE, PART_BLOOD } ParticleType;

typedef struct {
    Vec3  pos;
    Vec3  vel;
    float life, max_life;
    float size;
    float r, g, b, a;
    ParticleType type;
    int   active;
} Particle;

extern Particle particles[MAX_PARTICLES];
extern int      particle_count;

void particle_emit(Vec3 pos, Vec3 vel, float life, float size,
                   float r, float g, float b, float a, ParticleType type);

void particles_update(float dt);
void particles_draw(void);

/* Effect emitters */
void emit_barrel_fire(Vec3 pos, float dt);
void emit_blood(Vec3 pos);

void particles_reset(void);

#endif
