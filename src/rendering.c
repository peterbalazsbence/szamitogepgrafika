#include "rendering.h"
#include "math3d.h"
#include "player.h"
#include "level.h"
#include "textures.h"

/* builds a perspective projection matrix:*/
static void perspective_matrix(float fov_deg, float aspect, float znear, float zfar) {
    float f = 1.0f / tanf(fov_deg * 0.5f * PI / 180.0f);
    float m[16] = {0};
    m[0]  = f / aspect;
    m[5]  = f;
    m[10] = (zfar + znear) / (znear - zfar);
    m[11] = -1.0f;
    m[14] = (2.0f * zfar * znear) / (znear - zfar);
    glLoadMatrixf(m);
}

/* Builds a view matrix that puts the camera at `eye` looking toward `center`,
 * with `up` as the world up vector.*/
static void look_at(Vec3 eye, Vec3 center, Vec3 up) {
    Vec3 f = v3norm(v3sub(center, eye));     /* forward */
    Vec3 upn = v3norm(up);
    /* side = forward × up */
    Vec3 s = v3norm(v3(f.y*upn.z - f.z*upn.y,
                       f.z*upn.x - f.x*upn.z,
                       f.x*upn.y - f.y*upn.x));
    /* recomputed up = side × forward */
    Vec3 u = v3(s.y*f.z - s.z*f.y,
                s.z*f.x - s.x*f.z,
                s.x*f.y - s.y*f.x);
    float m[16] = {
         s.x,   u.x,  -f.x, 0,
         s.y,   u.y,  -f.y, 0,
         s.z,   u.z,  -f.z, 0,
         0,     0,     0,   1
    };
    glMultMatrixf(m);
    glTranslatef(-eye.x, -eye.y, -eye.z);
}

void set_projection(void) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    perspective_matrix(FOV, (float)WINDOW_W/WINDOW_H, NEAR_CLIP, FAR_CLIP);
    glMatrixMode(GL_MODELVIEW);
}

void set_camera(void) {
    glLoadIdentity();
    Vec3 e = player.pos;
    Vec3 f = player_forward();
    Vec3 c = v3add(e, f);
    look_at(e, c, v3(0, 1, 0));
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
        char ch = MAP_DATA[r][c];

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
