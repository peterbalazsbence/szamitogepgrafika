#ifndef DOOM3D_COMMON_H
#define DOOM3D_COMMON_H

#ifdef _WIN32
  #include <windows.h>
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <GL/gl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* ─────────── Window & rendering ─────────── */
#define WINDOW_W      1280
#define WINDOW_H      720
#define WINDOW_TITLE  "DOOM3D"
#define FOV           70.0f
#define NEAR_CLIP     0.1f
#define FAR_CLIP      100.0f

/* ─────────── Player movement ─────────── */
#define MOVE_SPEED    5.0f
#define TURN_SPEED    0.002f
#define GRAVITY       18.0f
#define JUMP_VEL      7.0f
#define PLAYER_HEIGHT 1.7f
#define PLAYER_RADIUS 0.3f
#define FLOOR_Y       0.0f
#define CEIL_Y        3.0f

/* ─────────── Enemies ─────────── */
#define MAX_ENEMIES        32
#define ENEMY_SPEED        2.5f
#define ENEMY_HEALTH       3
#define ENEMY_SIGHT_RANGE  18.0f
#define ENEMY_ATTACK_RANGE 1.0f
#define ENEMY_PATROL_RANGE 8.0f

/* ─────────── Decorations & pickups ─────────── */
#define MAX_DECORATIONS    64
#define MAX_PICKUPS        64

/* Pickup spawn counts (adjust these to control item density) */
#define PICKUP_COUNT_HEALTH  12
#define PICKUP_COUNT_AMMO    15
#define PICKUP_COUNT_ARMOR   8

/* ─────────── Weapons ─────────── */
#define SHOOT_COOLDOWN_FIST    0.4f
#define SHOOT_COOLDOWN_PISTOL  0.3f
#define SHOOT_COOLDOWN_SHOTGUN 0.7f
#define MUZZLE_TIME            0.08f

#define FIST_DAMAGE     2
#define PISTOL_DAMAGE   1
#define SHOTGUN_DAMAGE  3
#define FIST_RANGE      2.0f
#define PISTOL_RANGE    50.0f
#define SHOTGUN_RANGE   15.0f
#define SHOTGUN_SPREAD  0.8f

/* ─────────── Particles ─────────── */
#define MAX_PARTICLES    512

/* ─────────── Fog ─────────── */
#define FOG_DENSITY_MIN  0.0f
#define FOG_DENSITY_MAX  0.30f
#define FOG_DENSITY_STEP 0.01f

/* ─────────── Lighting ─────────── */
#define LIGHT_MIN   0.1f
#define LIGHT_MAX   2.0f
#define LIGHT_STEP  0.1f

/* ─────────── Math ─────────── */
#define PI  3.14159265358979f
#define DEG2RAD(x) ((x)*PI/180.0f)

#endif
