#ifndef DOOM3D_SHADOWS_H
#define DOOM3D_SHADOWS_H

#include "common.h"

void make_shadow_matrix(float mat[16], float lx, float ly, float lz);
void draw_shadow_oval(float x, float z, float rx, float rz);
void draw_shadows(void);

#endif
