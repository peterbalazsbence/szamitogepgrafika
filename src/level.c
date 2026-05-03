#include "level.h"

char MAP_DATA[MAP_ROWS_N][MAP_COLS+1];

int load_map_from_file(const char *path) {
    FILE *f = fopen(path, "r");
    if(!f) {
        fprintf(stderr, "Cannot open map file: %s\n", path);
        return 0;
    }
    char line[256];
    int row = 0;
    while(row < MAP_ROWS_N && fgets(line, sizeof(line), f)) {
        /* Strip trailing newline / carriage return */
        size_t len = strlen(line);
        while(len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
            line[--len] = 0;
        }
        if(len < (size_t)MAP_COLS) {
            fprintf(stderr, "Map row %d too short (%zu < %d): %s\n",
                    row, len, MAP_COLS, line);
            fclose(f);
            return 0;
        }
        memcpy(MAP_DATA[row], line, MAP_COLS);
        MAP_DATA[row][MAP_COLS] = 0;
        row++;
    }
    fclose(f);
    if(row != MAP_ROWS_N) {
        fprintf(stderr, "Map has %d rows, expected %d\n", row, MAP_ROWS_N);
        return 0;
    }
    printf("Loaded map: %s\n", path);
    return 1;
}

int map_is_wall(int col, int row) {
    if(row<0||row>=MAP_ROWS_N||col<0||col>=MAP_COLS) return 1;
    char ch = MAP_DATA[row][col];
    return ch=='#' || ch=='W';
}

int map_is_solid_wall(int col, int row) {
    if(row<0||row>=MAP_ROWS_N||col<0||col>=MAP_COLS) return 1;
    return MAP_DATA[row][col]=='#';
}

int map_is_window(int col, int row) {
    if(row<0||row>=MAP_ROWS_N||col<0||col>=MAP_COLS) return 0;
    return MAP_DATA[row][col]=='W';
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
