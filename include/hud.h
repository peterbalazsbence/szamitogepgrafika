#ifndef DOOM3D_HUD_H
#define DOOM3D_HUD_H

#include "common.h"

extern int show_help;
extern int game_over;
extern int game_win;
extern float game_time;

void ortho_begin(void);
void ortho_end(void);

void fill_rect(float x, float y, float w, float h,
               float r, float g, float b, float a);


void draw_doom_hud(void);

/* F1 help overlay */
void draw_help_screen(void);

#endif
