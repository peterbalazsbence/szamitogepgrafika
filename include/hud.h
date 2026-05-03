#ifndef DOOM3D_HUD_H
#define DOOM3D_HUD_H

#include "common.h"

/* Game UI / flow state grouped into one struct to minimize globals. */
typedef struct {
    int   show_help;   /* F1 toggles */
    int   game_over;   /* set by enemies, reset on R */
    int   game_win;    /* set when last enemy dies */
    float game_time;   /* seconds since level start, used for animations */
} GameUI;

extern GameUI ui;

/* Switches GL state to 2D orthographic rendering for HUD overlays. */
void ortho_begin(void);

/* Restores GL state for 3D rendering after HUD drawing. */
void ortho_end(void);

/* Draws a filled rectangle with alpha blending. The basic HUD primitive. */
void fill_rect(float x, float y, float w, float h,
               float r, float g, float b, float a);

/* Draws the full Doom-style status bar, weapon icon, crosshair,
 * status indicators and any active game-over or win overlay. */
void draw_doom_hud(void);

/* Draws the F1 help screen (full-screen overlay with controls list). */
void draw_help_screen(void);

#endif
