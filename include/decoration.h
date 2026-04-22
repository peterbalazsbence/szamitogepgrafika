#ifndef DOOM3D_DECORATION_H
#define DOOM3D_DECORATION_H

#include "common.h"
#include "math3d.h"

typedef enum { DECO_BARREL, DECO_CRATE } DecoType;

typedef struct {
    Vec3     pos;
    DecoType type;
    float    rotation;
} Decoration;

extern Decoration decorations[MAX_DECORATIONS];
extern int        deco_count;

void deco_spawn(float x, float z, DecoType type);
void draw_decorations(void);
void decorations_reset(void);

#endif
