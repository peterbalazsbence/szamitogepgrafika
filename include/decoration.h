#ifndef DOOM3D_DECORATION_H
#define DOOM3D_DECORATION_H

#include "common.h"
#include "math3d.h"

/* Decoration variant: barrel emits fire particles, crate is just visual. */
typedef enum { DECO_BARREL, DECO_CRATE } DecoType;

/* A static decoration in the world. */
typedef struct {
    Vec3     pos;
    DecoType type;
    float    rotation;
} Decoration;

extern Decoration decorations[MAX_DECORATIONS];
extern int        deco_count;

/* Adds a new decoration at (x,z). type selects barrel or crate. */
void deco_spawn(float x, float z, DecoType type);

/* Renders all decorations using their respective OBJ models. */
void draw_decorations(void);

/* Removes all decorations from the world. */
void decorations_reset(void);

#endif
