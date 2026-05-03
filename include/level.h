#ifndef DOOM3D_LEVEL_H
#define DOOM3D_LEVEL_H

#include "common.h"

#define MAP_ROWS_N  40
#define MAP_COLS    40

/* The 40x40 grid map, loaded from assets/map.txt at startup.
 * Each cell is one character: '#'=wall, '.'=floor, 'E'=enemy,
 * 'P'=player, 'B'=barrel, 'C'=crate, 'W'=glass window. */
extern char MAP_DATA[MAP_ROWS_N][MAP_COLS+1];

/* Loads the map grid from a text file (one row per line).
 * Returns 1 on success, 0 if the file is missing or malformed. */
int load_map_from_file(const char *path);

/* Returns 1 if the cell blocks movement (solid wall or window). */
int map_is_wall(int col, int row);

/* Returns 1 only for fully solid walls ('#'). Windows are NOT solid here. */
int map_is_solid_wall(int col, int row);

/* Returns 1 if the cell is a glass window ('W'). */
int map_is_window(int col, int row);

/* Stepped raycast: returns 1 if no solid wall blocks the line between
 * (x1,z1) and (x2,z2). Windows are transparent for line-of-sight. */
int has_line_of_sight(float x1, float z1, float x2, float z2);

/* AABB-based player/enemy collision against the grid. */
int collides_map(float x, float z);

#endif
