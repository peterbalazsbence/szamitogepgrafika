#ifndef DOOM3D_PICKUP_H
#define DOOM3D_PICKUP_H

#include "common.h"
#include "math3d.h"

typedef enum { PICKUP_HEALTH, PICKUP_AMMO, PICKUP_ARMOR } PickupType;

typedef struct {
    Vec3       pos;
    PickupType type;
    int        active;
    float      bob_phase;
} Pickup;

extern Pickup pickups[MAX_PICKUPS];
extern int    pickup_count;

void pickup_spawn(float x, float z, PickupType type);
void scatter_pickups(int n_health, int n_ammo, int n_armor);
void draw_pickups(void);
void pickups_reset(void);

#endif
