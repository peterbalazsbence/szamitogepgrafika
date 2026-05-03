#ifndef DOOM3D_SHADOWS_H
#define DOOM3D_SHADOWS_H

#include "common.h"

/* Builds a 4x4 shadow projection matrix that flattens any 3D geometry
 * onto the floor plane (Y = FLOOR_Y) from a light at (lx,ly,lz).
 * Output is column-major, suitable for glMultMatrixf. */
void make_shadow_matrix(float mat[16], float lx, float ly, float lz);

/* Draws a soft oval shadow texture at (x,z) with given radii.
 * Used for spheres/cylinders where projection produces hollow shapes. */
void draw_shadow_oval(float x, float z, float rx, float rz);

/* Renders shadows for every shadow-casting object in the scene.
 * Uses the stencil buffer to avoid double-blending where shadows overlap. */
void draw_shadows(void);

#endif
