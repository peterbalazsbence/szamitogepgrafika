#ifndef DOOM3D_TEXTURES_H
#define DOOM3D_TEXTURES_H

#include "common.h"

extern GLuint tex_wall, tex_floor, tex_ceil, tex_enemy, tex_crate, tex_barrel;
extern GLuint tex_pickup_health, tex_pickup_ammo, tex_pickup_armor;
extern GLuint tex_window; /* semi-transparent glass */
extern GLuint tex_shadow; /* soft shadow blob */

void init_textures(void);
void free_textures(void);

#endif
