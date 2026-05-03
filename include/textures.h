#ifndef DOOM3D_TEXTURES_H
#define DOOM3D_TEXTURES_H

#include "common.h"

/* Procedurally generated textures, created at startup. */
extern GLuint tex_wall;
extern GLuint tex_floor;
extern GLuint tex_ceil;
extern GLuint tex_enemy;
extern GLuint tex_crate;
extern GLuint tex_barrel;
extern GLuint tex_pickup_health;
extern GLuint tex_pickup_ammo;
extern GLuint tex_pickup_armor;

/* RGBA textures used for transparency / blending. */
extern GLuint tex_window;
extern GLuint tex_shadow;

/* Generates all textures procedurally and uploads them to the GPU. */
void init_textures(void);

/* Releases all GL texture objects. */
void free_textures(void);

#endif
