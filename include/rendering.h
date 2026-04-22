#ifndef DOOM3D_RENDERING_H
#define DOOM3D_RENDERING_H

#include "common.h"

/* Camera & projection setup */
void set_projection(void);
void set_camera(void);

/* Draw a textured quad with a specified normal (for lighting) */
void quad_n(float x0,float y0,float z0, float x1,float y1,float z1,
            float x2,float y2,float z2, float x3,float y3,float z3,
            float nx,float ny,float nz, float us, float vs);

/* Level / world geometry */
void draw_level(void);
void draw_windows(void); /* transparency pass, after opaque */

#endif
