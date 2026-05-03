#ifndef DOOM3D_PARTICLES_H
#define DOOM3D_PARTICLES_H

#include "common.h"
#include "math3d.h"

/* Particle visual category. Determines integration physics and blend mode. */
typedef enum { PART_FIRE, PART_SMOKE, PART_BLOOD } ParticleType;

/* A single particle in the system. */
typedef struct {
    Vec3  pos;
    Vec3  vel;
    float life, max_life;
    float size;
    float r, g, b, a;
    ParticleType type;
    int   active;
} Particle;

/* Particle pool. Inactive slots are reused on emit. */
extern Particle particles[MAX_PARTICLES];
extern int      particle_count;

/* Spawns one particle. Reuses an inactive slot or overwrites the
 * particle with the least life remaining if the pool is full. */
void particle_emit(Vec3 pos, Vec3 vel, float life, float size,
                   float r, float g, float b, float a, ParticleType type);

/* Advances all particles by dt seconds (physics + life). */
void particles_update(float dt);

/* Renders all active particles as camera-facing billboards. */
void particles_draw(void);

/* Emits fire and smoke particles around a barrel position.
 * Throttles its own emission rate via an internal accumulator. */
void emit_barrel_fire(Vec3 pos, float dt);

/* Emits a burst of red blood particles at the given position. */
void emit_blood(Vec3 pos);

/* Marks all particles inactive (used on level restart). */
void particles_reset(void);

#endif
