#ifndef DOOM3D_PLAYER_H
#define DOOM3D_PLAYER_H

#include "common.h"
#include "math3d.h"

/* Selectable weapons. Slot indices match the ARMS HUD section. */
typedef enum { WPN_FIST=0, WPN_PISTOL=1, WPN_SHOTGUN=2 } WeaponType;

/* Full player state. Modified directly by other modules
 * (e.g. enemies decrement health on hit, pickups restore stats). */
typedef struct {
    Vec3       pos;
    float      yaw, pitch;
    float      vy;            /* vertical velocity (gravity + jumping) */
    int        on_ground;
    int        health;
    int        armor;
    int        ammo_pistol;
    int        ammo_shotgun;
    WeaponType weapon;
    float      shoot_cd;      /* cooldown until next shot allowed */
    float      muzzle_flash;  /* timer for screen flash effect */
    float      damage_flash;  /* timer for red vignette when hit */
} Player;

extern Player player;

/* Returns the unit forward vector built from yaw and pitch. */
Vec3 player_forward(void);

/* Per-frame player update: keyboard input, gravity,
 * collision, pickup collection, timers. */
void player_update(float dt);

/* Fires the currently equipped weapon. Performs a hitscan ray test
 * against all enemies; respects walls and windows as bullet blockers. */
void player_shoot(void);

/* Resets stats to starting values. Position is set by the caller. */
void player_reset(void);

#endif
