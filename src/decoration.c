#include "decoration.h"
#include "textures.h"
#include "obj_loader.h"

Decoration decorations[MAX_DECORATIONS];
int deco_count = 0;

void deco_spawn(float x, float z, DecoType type) {
    if(deco_count >= MAX_DECORATIONS) return;
    decorations[deco_count].pos      = v3(x, FLOOR_Y, z);
    decorations[deco_count].type     = type;
    decorations[deco_count].rotation = (float)(rand() % 360);
    deco_count++;
}

void draw_decorations(void) {
    glEnable(GL_TEXTURE_2D);
    for(int i=0;i<deco_count;i++) {
        Vec3 p = decorations[i].pos;
        glPushMatrix();
        glTranslatef(p.x, p.y, p.z);
        glRotatef(decorations[i].rotation, 0, 1, 0);

        if(decorations[i].type == DECO_BARREL) {
            glBindTexture(GL_TEXTURE_2D, tex_barrel);
            glColor3f(0.8f, 0.9f, 0.8f);
            glScalef(0.5f, 0.8f, 0.5f);
            obj_draw(&model_barrel);
        } else {
            glBindTexture(GL_TEXTURE_2D, tex_crate);
            glColor3f(0.9f, 0.8f, 0.6f);
            glScalef(0.7f, 0.7f, 0.7f);
            obj_draw(&model_cube);
        }
        glPopMatrix();
    }
    glColor3f(1, 1, 1);
    glDisable(GL_TEXTURE_2D);
}

void decorations_reset(void) {
    deco_count = 0;
}
