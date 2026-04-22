#ifndef DOOM3D_PLAYER_H
#define DOOM3D_PLAYER_H

#include "common.h"
#include "math3d.h"

typedef enum { WPN_FIST=0, WPN_PISTOL=1, WPN_SHOTGUN=2 } WeaponType;

typedef struct {
    Vec3       pos;
    float      yaw, pitch;
    float      vy;
    int        on_ground;
    int        health;
    int        armor;
    int        ammo_pistol;
    int        ammo_shotgun;
    WeaponType weapon;
    float      shoot_cd;
    float      muzzle_flash;
    float      damage_flash; /* red vignette timer when hit */
} Player;

extern Player player;

Vec3 player_forward(void);
void player_update(float dt);
void player_shoot(void);
void player_reset(void);

#endif
