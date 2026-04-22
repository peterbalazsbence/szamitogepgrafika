#ifndef DOOM3D_LEVEL_H
#define DOOM3D_LEVEL_H

#include "common.h"

#define MAP_ROWS_N  40
#define MAP_COLS    40

extern const char *MAP_ROWS[];

int map_is_wall(int col, int row);
int map_is_solid_wall(int col, int row);
int map_is_window(int col, int row);


int has_line_of_sight(float x1, float z1, float x2, float z2);

int collides_map(float x, float z);

#endif
