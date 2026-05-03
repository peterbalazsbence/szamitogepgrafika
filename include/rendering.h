#ifndef DOOM3D_RENDERING_H
#define DOOM3D_RENDERING_H

#include "common.h"

/* Sets up the perspective projection matrix once at startup. */
void set_projection(void);

/* Builds the view matrix every frame based on player position/orientation. */
void set_camera(void);

/* Draws a textured quad with an explicit normal vector for lighting.
 * Vertices must be given in counter-clockwise order from the visible side.
 * us, vs are the texture-coordinate scale (1,1 = full texture). */
void quad_n(float x0,float y0,float z0, float x1,float y1,float z1,
            float x2,float y2,float z2, float x3,float y3,float z3,
            float nx,float ny,float nz, float us, float vs);

/* Draws all opaque level geometry (walls, floor, ceiling). */
void draw_level(void);

/* Draws all glass windows. Called separately, after opaque geometry,
 * with depth writes disabled - this is the standard transparency pass. */
void draw_windows(void);

#endif
