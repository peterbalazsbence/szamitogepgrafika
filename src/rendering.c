#include "rendering.h"
#include "math3d.h"
#include "player.h"
#include "level.h"
#include "textures.h"

void set_projection(void) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(FOV, (float)WINDOW_W/WINDOW_H, NEAR_CLIP, FAR_CLIP);
    glMatrixMode(GL_MODELVIEW);
}

void set_camera(void) {
    glLoadIdentity();
    Vec3 e = player.pos;
    Vec3 f = player_forward();
    Vec3 c = v3add(e, f);
    gluLookAt(e.x, e.y, e.z,  c.x, c.y, c.z,  0, 1, 0);
}

void quad_n(float x0,float y0,float z0, float x1,float y1,float z1,
            float x2,float y2,float z2, float x3,float y3,float z3,
            float nx,float ny,float nz, float us, float vs)
{
    glNormal3f(nx, ny, nz);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);   glVertex3f(x0, y0, z0);
    glTexCoord2f(us, 0);  glVertex3f(x1, y1, z1);
    glTexCoord2f(us, vs); glVertex3f(x2, y2, z2);
    glTexCoord2f(0, vs);  glVertex3f(x3, y3, z3);
    glEnd();
}

void draw_level(void) {
    glEnable(GL_TEXTURE_2D);
    for(int r=0;r<MAP_ROWS_N;r++)
    for(int c=0;c<MAP_COLS;c++) {
        float x0=(float)c, z0=(float)r, x1=x0+1, z1=z0+1;
        char ch = MAP_ROWS[r][c];

        /* Floor & ceiling under open tiles (not walls or windows) */
        if(ch != '#' && ch != 'W') {
            glBindTexture(GL_TEXTURE_2D, tex_floor);
            glColor3f(1.0f, 1.0f, 1.0f);
            /* CCW winding viewed from above so back-face culling keeps it */
            quad_n(x0,FLOOR_Y,z1, x1,FLOOR_Y,z1, x1,FLOOR_Y,z0, x0,FLOOR_Y,z0,
                   0, 1, 0, 1, 1);

            glBindTexture(GL_TEXTURE_2D, tex_ceil);
            glColor3f(0.6f, 0.6f, 0.7f);
            quad_n(x0,CEIL_Y,z0, x1,CEIL_Y,z0, x1,CEIL_Y,z1, x0,CEIL_Y,z1,
                   0, -1, 0, 1, 1);
        }

        /* Solid wall faces - only draw faces bordering open space */
        if(ch == '#') {
            glBindTexture(GL_TEXTURE_2D, tex_wall);
            glColor3f(1, 1, 1);

            if(!map_is_solid_wall(c,r-1) && !map_is_window(c,r-1))
                quad_n(x0,CEIL_Y,z0, x1,CEIL_Y,z0, x1,FLOOR_Y,z0, x0,FLOOR_Y,z0,
                       0, 0, -1, 1, 1);
            if(!map_is_solid_wall(c,r+1) && !map_is_window(c,r+1))
                quad_n(x1,CEIL_Y,z1, x0,CEIL_Y,z1, x0,FLOOR_Y,z1, x1,FLOOR_Y,z1,
                       0, 0, 1, 1, 1);
            if(!map_is_solid_wall(c-1,r) && !map_is_window(c-1,r))
                quad_n(x0,CEIL_Y,z1, x0,CEIL_Y,z0, x0,FLOOR_Y,z0, x0,FLOOR_Y,z1,
                       -1, 0, 0, 1, 1);
            if(!map_is_solid_wall(c+1,r) && !map_is_window(c+1,r))
                quad_n(x1,CEIL_Y,z0, x1,CEIL_Y,z1, x1,FLOOR_Y,z1, x1,FLOOR_Y,z0,
                       1, 0, 0, 1, 1);

            /* Also draw the solid wall side facing a window (so looking through
             * the window you see the back of the adjacent solid wall, not void) */
            if(map_is_window(c,r-1))
                quad_n(x0,CEIL_Y,z0, x1,CEIL_Y,z0, x1,FLOOR_Y,z0, x0,FLOOR_Y,z0,
                       0, 0, -1, 1, 1);
            if(map_is_window(c,r+1))
                quad_n(x1,CEIL_Y,z1, x0,CEIL_Y,z1, x0,FLOOR_Y,z1, x1,FLOOR_Y,z1,
                       0, 0, 1, 1, 1);
            if(map_is_window(c-1,r))
                quad_n(x0,CEIL_Y,z1, x0,CEIL_Y,z0, x0,FLOOR_Y,z0, x0,FLOOR_Y,z1,
                       -1, 0, 0, 1, 1);
            if(map_is_window(c+1,r))
                quad_n(x1,CEIL_Y,z0, x1,CEIL_Y,z1, x1,FLOOR_Y,z1, x1,FLOOR_Y,z0,
                       1, 0, 0, 1, 1);
        }
    }
    glDisable(GL_TEXTURE_2D);
}

void draw_windows(void) {
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE); /* don't write depth for transparency */
    glBindTexture(GL_TEXTURE_2D, tex_window);
    glColor4f(1, 1, 1, 1); /* RGBA texture drives alpha */

    for(int r=0;r<MAP_ROWS_N;r++)
    for(int c=0;c<MAP_COLS;c++) {
        if(!map_is_window(c, r)) continue;
        float x0=(float)c, z0=(float)r, x1=x0+1, z1=z0+1;

        if(!map_is_solid_wall(c,r-1) && !map_is_window(c,r-1))
            quad_n(x0,CEIL_Y,z0, x1,CEIL_Y,z0, x1,FLOOR_Y,z0, x0,FLOOR_Y,z0,
                   0, 0, -1, 1, 1);
        if(!map_is_solid_wall(c,r+1) && !map_is_window(c,r+1))
            quad_n(x1,CEIL_Y,z1, x0,CEIL_Y,z1, x0,FLOOR_Y,z1, x1,FLOOR_Y,z1,
                   0, 0, 1, 1, 1);
        if(!map_is_solid_wall(c-1,r) && !map_is_window(c-1,r))
            quad_n(x0,CEIL_Y,z1, x0,CEIL_Y,z0, x0,FLOOR_Y,z0, x0,FLOOR_Y,z1,
                   -1, 0, 0, 1, 1);
        if(!map_is_solid_wall(c+1,r) && !map_is_window(c+1,r))
            quad_n(x1,CEIL_Y,z0, x1,CEIL_Y,z1, x1,FLOOR_Y,z1, x1,FLOOR_Y,z0,
                   1, 0, 0, 1, 1);
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}
