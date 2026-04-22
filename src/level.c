#include "level.h"

/* 40x40 grid map.
 *  '#' = wall               '.' = floor
 *  'E' = enemy spawn        'P' = player spawn
 *  'B' = barrel             'C' = crate
 *  'W' = glass window (transparent wall, blocks movement & bullets)
 */
const char *MAP_ROWS[] = {
    "########################################",
    "#P.........#.........#.................#",
    "#..........#.........#.................#",
    "#..........#....E....W......####.......#",
    "#...B......#.........W......#..#.......#",
    "#..........###..######......#..#.......#",
    "#..........#.........#......####.......#",
    "#..........W.........#.........E.......#",
    "#..E.......W.........#.................#",
    "####.###...#....B....#..........####...#",
    "#..........#.........#..........#..#...#",
    "#.....................B.........#..#...#",
    "#..........#.........#..........####...#",
    "#..B.......#..E......#.................#",
    "#..........###.#######.................#",
    "#..........W.................####......#",
    "####..######.................#..#......#",
    "#..........#.........E.......#..#......#",
    "#..........#.................####......#",
    "#..E.......#...........................#",
    "#..........#####W####..................#",
    "#..........#.........#.....E...........#",
    "#..........#....B....#.................#",
    "#..........W.........#.........####....#",
    "####.###...#.........W.........#..#....#",
    "#..........#.........W.........#..#....#",
    "#..........###..######.........####....#",
    "#.....C....#.........#.................#",
    "#..........#.........#.................#",
    "#..E.......#....E....W...E.............#",
    "#..........#.........#.................#",
    "####.###...#.........####.######.......#",
    "#..........#.........................B.#",
    "#..........#..C......#.................#",
    "#..B.......#.........W.......E.........#",
    "#..........###########.................#",
    "#..............C.......................#",
    "#...E..................................#",
    "#......................................#",
    "########################################",
};

int map_is_wall(int col, int row) {
    if(row<0||row>=MAP_ROWS_N||col<0||col>=MAP_COLS) return 1;
    char ch = MAP_ROWS[row][col];
    return ch=='#' || ch=='W'; /* windows block movement too */
}

int map_is_solid_wall(int col, int row) {
    if(row<0||row>=MAP_ROWS_N||col<0||col>=MAP_COLS) return 1;
    return MAP_ROWS[row][col]=='#';
}

int map_is_window(int col, int row) {
    if(row<0||row>=MAP_ROWS_N||col<0||col>=MAP_COLS) return 0;
    return MAP_ROWS[row][col]=='W';
}

int has_line_of_sight(float x1, float z1, float x2, float z2) {
    float dx=x2-x1, dz=z2-z1;
    float dist=sqrtf(dx*dx+dz*dz);
    if(dist<0.1f) return 1;
    float step=0.5f;
    int steps=(int)(dist/step)+1;
    for(int i=1;i<steps;i++) {
        float t=(float)i/steps;
        float cx=x1+dx*t, cz=z1+dz*t;
        int col=(int)floorf(cx), row=(int)floorf(cz);
        if(map_is_solid_wall(col,row)) return 0;
        /* Windows are transparent for LOS - enemies can see through */
    }
    return 1;
}

int collides_map(float x, float z) {
    int col=(int)floorf(x), row=(int)floorf(z);
    for(int dr=-1;dr<=1;dr++) for(int dc=-1;dc<=1;dc++) {
        if(map_is_wall(col+dc, row+dr)) {
            float wx=(float)(col+dc), wz=(float)(row+dr);
            if(x > wx-PLAYER_RADIUS && x < wx+1+PLAYER_RADIUS &&
               z > wz-PLAYER_RADIUS && z < wz+1+PLAYER_RADIUS)
                return 1;
        }
    }
    return 0;
}
