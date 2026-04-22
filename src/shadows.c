#include "shadows.h"
#include "math3d.h"
#include "player.h"
#include "enemy.h"
#include "decoration.h"
#include "pickup.h"
#include "textures.h"
#include "obj_loader.h"
#include "hud.h"

/* Build a 4x4 projection matrix that flattens geometry onto the Y=FLOOR_Y plane
 * from a light at (lx, ly, lz). Classic "shadow projection" trick - the resulting
 * rank-3 matrix collapses the Y component when applied. */
void make_shadow_matrix(float mat[16], float lx, float ly, float lz) {
    float ny = 1.0f;
    float d  = -FLOOR_Y;
    float dot = ny * ly + d;

    mat[ 0] = dot;         mat[ 1] = 0;            mat[ 2] = 0;      mat[ 3] = 0;
    mat[ 4] = -lx * ny;    mat[ 5] = dot - ly*ny;  mat[ 6] = -lz*ny; mat[ 7] = -ny;
    mat[ 8] = 0;           mat[ 9] = 0;            mat[10] = dot;    mat[11] = 0;
    mat[12] = -lx * d;     mat[13] = -ly*d;        mat[14] = -lz*d;  mat[15] = dot;
}

/* Simple soft oval shadow using radial alpha texture.
 * Used for spheres/cylinders where projection produces weird donut shapes. */
void draw_shadow_oval(float x, float z, float rx, float rz) {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_shadow);
    glPushMatrix();
    glTranslatef(x, FLOOR_Y + 0.01f, z); /* a hair above floor */
    glRotatef(-90, 1, 0, 0);             /* lay flat */
    glScalef(rx, rz, 1);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-1, -1, 0);
    glTexCoord2f(1, 0); glVertex3f( 1, -1, 0);
    glTexCoord2f(1, 1); glVertex3f( 1,  1, 0);
    glTexCoord2f(0, 1); glVertex3f(-1,  1, 0);
    glEnd();
    glPopMatrix();
    glDisable(GL_TEXTURE_2D);
}

void draw_shadows(void) {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    /* Stencil prevents over-darkening where shadows overlap */
    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    /* Polygon offset to avoid z-fighting with the floor */
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);
    glDisable(GL_CULL_FACE);

    glColor4f(1, 1, 1, 1);
    float shadow_mat[16];

    /* ─── Enemies: soft oval (sphere projection looks like a donut) ─── */
    for(int i=0;i<enemy_count;i++) {
        if(!enemies[i].alive) continue;
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        draw_shadow_oval(enemies[i].pos.x, enemies[i].pos.z, 0.5f, 0.5f);
    }

    /* ─── Decorations: oval for barrels, projected box for crates ─── */
    for(int i=0;i<deco_count;i++) {
        Vec3 p = decorations[i].pos;
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);

        if(decorations[i].type == DECO_BARREL) {
            draw_shadow_oval(p.x, p.z, 0.4f, 0.4f);
        } else {
            /* Projected cube geometry - proper box-shaped shadow */
            glDisable(GL_TEXTURE_2D);
            glColor4f(0, 0, 0, 0.4f);
            make_shadow_matrix(shadow_mat, p.x, CEIL_Y - 0.1f, p.z);
            glPushMatrix();
            glMultMatrixf(shadow_mat);
            glTranslatef(p.x, p.y, p.z);
            glRotatef(decorations[i].rotation, 0, 1, 0);
            glScalef(0.7f, 0.7f, 0.7f);
            obj_draw(&model_cube);
            glPopMatrix();
            glColor4f(1, 1, 1, 1);
        }
    }

    /* ─── Pickups: tiny projected cube shadows ─── */
    glDisable(GL_TEXTURE_2D);
    glColor4f(0, 0, 0, 0.4f);
    for(int i=0;i<pickup_count;i++) {
        if(!pickups[i].active) continue;
        Vec3 p = pickups[i].pos;
        float bob = sinf(pickups[i].bob_phase)*0.1f;

        make_shadow_matrix(shadow_mat, p.x, CEIL_Y - 0.1f, p.z);
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        glPushMatrix();
        glMultMatrixf(shadow_mat);
        glTranslatef(p.x, p.y+bob, p.z);
        glRotatef(game_time*90, 0, 1, 0);
        glScalef(0.3f, 0.3f, 0.3f);
        obj_draw(&model_cube);
        glPopMatrix();
    }

    glDisable(GL_STENCIL_TEST);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}
