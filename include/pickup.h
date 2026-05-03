#ifndef DOOM3D_PICKUP_H
#define DOOM3D_PICKUP_H

#include "common.h"
#include "math3d.h"

/* Pickup category: green=health, yellow=ammo, blue=armor. */
typedef enum { PICKUP_HEALTH, PICKUP_AMMO, PICKUP_ARMOR } PickupType;

/* A pickup item that the player can collect by walking over it. */
typedef struct {
    Vec3       pos;
    PickupType type;
    int        active;     /* 0 = already collected */
    float      bob_phase;  /* offset for bobbing animation */
} Pickup;

extern Pickup pickups[MAX_PICKUPS];
extern int    pickup_count;

/* Adds one pickup at (x,z). */
void pickup_spawn(float x, float z, PickupType type);

/* Spawns the requested counts of pickups at random open floor tiles
 * using a Fisher-Yates shuffle so they don't overlap. */
void scatter_pickups(int n_health, int n_ammo, int n_armor);

/* Renders all active pickups as spinning, bobbing cubes. */
void draw_pickups(void);

/* Removes all pickups from the world. */
void pickups_reset(void);

#endif
