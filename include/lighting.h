#ifndef DOOM3D_LIGHTING_H
#define DOOM3D_LIGHTING_H

#include "common.h"

extern float light_brightness;       /* 0.1 to 2.0, controlled by +/- */
extern int   light_color_mode;       /* 0=white, 1=red, 2=blue, L cycles */
extern int   flashlight_on;          /* F toggles */
extern float ambient_light_pos[4];   /* moved by arrow keys */

/* Fog  */
extern int   fog_enabled;
extern float fog_density;

void setup_lighting(void);
void setup_fog(void);

void draw_light_marker(void);

#endif
