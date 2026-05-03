#ifndef DOOM3D_LIGHTING_H
#define DOOM3D_LIGHTING_H

#include "common.h"

/* All lighting and fog parameters bundled into one struct
 * to minimize the number of globals. Adjusted by user input. */
typedef struct {
    /* Brightness multiplier in [LIGHT_MIN, LIGHT_MAX], adjusted with +/-. */
    float brightness;
    /* Color palette: 0=white, 1=red, 2=blue. Cycled with L. */
    int   color_mode;
    /* Player-attached spotlight on/off, toggled with F. */
    int   flashlight_on;
    /* Position of the movable scene light, controlled by arrow keys. */
    float scene_light_pos[4];
    /* Distance fog: G toggles, [ and ] adjust density. */
    int   fog_enabled;
    float fog_density;
} Lighting;

extern Lighting light;

/* Configures GL_LIGHT0 (positional scene light) and GL_LIGHT1 (spotlight). */
void setup_lighting(void);

/* Configures GL_FOG state. Called every frame because the HUD pass
 * disables and re-enables it. */
void setup_fog(void);

/* Renders a small floating diamond at the scene light's position. */
void draw_light_marker(void);

#endif
