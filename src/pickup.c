#include "pickup.h"
#include "level.h"
#include "hud.h"
#include "textures.h"
#include "obj_loader.h"

Pickup pickups[MAX_PICKUPS];
int pickup_count = 0;

void pickup_spawn(float x, float z, PickupType type) {
    if(pickup_count >= MAX_PICKUPS) return;
    pickups[pickup_count].pos   = v3(x, FLOOR_Y + 0.3f, z);
    pickups[pickup_count].type  = type;
    pickups[pickup_count].active = 1;
    pickups[pickup_count].bob_phase = randf() * 2*PI;
    pickup_count++;
}

void scatter_pickups(int n_health, int n_ammo, int n_armor) {
    int open_tiles[MAP_ROWS_N * MAP_COLS][2];
    int oc = 0;
    for(int r=1;r<MAP_ROWS_N-1;r++)
        for(int c=1;c<MAP_COLS-1;c++)
            if(MAP_ROWS[r][c] == '.') {
                open_tiles[oc][0] = c;
                open_tiles[oc][1] = r;
                oc++;
            }

    /* Fisher-Yates shuffle */
    for(int i=oc-1;i>0;i--) {
        int j = rand() % (i+1);
        int tc = open_tiles[i][0], tr = open_tiles[i][1];
        open_tiles[i][0] = open_tiles[j][0];
        open_tiles[i][1] = open_tiles[j][1];
        open_tiles[j][0] = tc;
        open_tiles[j][1] = tr;
    }

    int idx = 0;
    for(int i=0; i<n_health && idx<oc; i++, idx++)
        pickup_spawn(open_tiles[idx][0]+0.5f, open_tiles[idx][1]+0.5f, PICKUP_HEALTH);
    for(int i=0; i<n_ammo && idx<oc; i++, idx++)
        pickup_spawn(open_tiles[idx][0]+0.5f, open_tiles[idx][1]+0.5f, PICKUP_AMMO);
    for(int i=0; i<n_armor && idx<oc; i++, idx++)
        pickup_spawn(open_tiles[idx][0]+0.5f, open_tiles[idx][1]+0.5f, PICKUP_ARMOR);
}

void draw_pickups(void) {
    glEnable(GL_TEXTURE_2D);
    for(int i=0;i<pickup_count;i++) {
        if(!pickups[i].active) continue;
        Vec3 p = pickups[i].pos;
        float bob = sinf(pickups[i].bob_phase) * 0.1f;

        glPushMatrix();
        glTranslatef(p.x, p.y + bob, p.z);
        glRotatef(game_time * 90, 0, 1, 0); /* spin on Y axis */
        glScalef(0.3f, 0.3f, 0.3f);

        switch(pickups[i].type) {
            case PICKUP_HEALTH:
                glBindTexture(GL_TEXTURE_2D, tex_pickup_health);
                glColor3f(0.3f, 1, 0.3f);
                break;
            case PICKUP_AMMO:
                glBindTexture(GL_TEXTURE_2D, tex_pickup_ammo);
                glColor3f(1, 0.9f, 0.3f);
                break;
            case PICKUP_ARMOR:
                glBindTexture(GL_TEXTURE_2D, tex_pickup_armor);
                glColor3f(0.3f, 0.5f, 1);
                break;
        }
        obj_draw(&model_cube);
        glPopMatrix();
    }
    glColor3f(1, 1, 1);
    glDisable(GL_TEXTURE_2D);
}

void pickups_reset(void) {
    pickup_count = 0;
}
