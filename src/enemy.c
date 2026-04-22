#include "enemy.h"
#include "level.h"
#include "player.h"
#include "textures.h"
#include "obj_loader.h"
#include "hud.h"

Enemy enemies[MAX_ENEMIES];
int   enemy_count = 0;

void enemy_spawn(float x, float z) {
    if(enemy_count >= MAX_ENEMIES) return;
    Enemy *e = &enemies[enemy_count];
    e->pos       = v3(x, 0.8f, z);
    e->spawn_pos = e->pos;
    e->last_known_player = e->pos;
    e->health    = ENEMY_HEALTH;
    e->alive     = 1;
    e->pain_timer = 0;
    e->bob_phase = randf() * 2*PI;
    e->state     = AI_IDLE;
    e->state_timer = 0;
    e->patrol_angle = randf() * 2*PI;
    e->attack_cooldown = 0;
    enemy_count++;
}

void enemies_update(float dt) {
    for(int i=0;i<enemy_count;i++) {
        Enemy *e = &enemies[i];
        if(!e->alive) continue;
        if(e->pain_timer > 0)     e->pain_timer     -= dt;
        if(e->attack_cooldown > 0) e->attack_cooldown -= dt;
        e->bob_phase    += dt * 3.0f;
        e->state_timer  += dt;

        /* Distance to player + line-of-sight check */
        Vec3 to_player = v3sub(player.pos, e->pos);
        to_player.y = 0;
        float dist = v3len(to_player);
        int can_see = (dist < ENEMY_SIGHT_RANGE) &&
                      has_line_of_sight(e->pos.x, e->pos.z,
                                        player.pos.x, player.pos.z);

        /* ─── State machine ─── */
        switch(e->state) {
            case AI_IDLE:
                if(can_see) {
                    e->state = AI_CHASE;
                    e->last_known_player = player.pos;
                    e->state_timer = 0;
                } else if(e->state_timer > 2.0f + (rand()%30)/10.0f) {
                    e->state = AI_PATROL;
                    e->patrol_angle += (float)(rand()%314 - 157)/100.0f;
                    e->state_timer = 0;
                }
                break;

            case AI_PATROL: {
                if(can_see) {
                    e->state = AI_CHASE;
                    e->last_known_player = player.pos;
                    e->state_timer = 0;
                    break;
                }
                float px2 = e->pos.x + sinf(e->patrol_angle)*ENEMY_SPEED*0.5f*dt;
                float pz2 = e->pos.z + cosf(e->patrol_angle)*ENEMY_SPEED*0.5f*dt;

                /* If we wandered too far from spawn, steer back */
                float ds  = px2 - e->spawn_pos.x;
                float dds = pz2 - e->spawn_pos.z;
                if(ds*ds + dds*dds > ENEMY_PATROL_RANGE*ENEMY_PATROL_RANGE) {
                    e->patrol_angle = atan2f(e->spawn_pos.x - e->pos.x,
                                             e->spawn_pos.z - e->pos.z);
                }
                if(!collides_map(px2, e->pos.z)) e->pos.x = px2;
                else e->patrol_angle += PI*0.5f;
                if(!collides_map(e->pos.x, pz2)) e->pos.z = pz2;
                else e->patrol_angle += PI*0.5f;

                if(e->state_timer > 3.0f + (rand()%20)/10.0f) {
                    e->state = AI_IDLE;
                    e->state_timer = 0;
                }
                break;
            }

            case AI_CHASE:
                if(dist < ENEMY_ATTACK_RANGE) {
                    e->state = AI_ATTACK;
                    e->state_timer = 0;
                } else if(!can_see && e->state_timer > 0.5f) {
                    /* Lost sight - go investigate last known position */
                    e->state = AI_SEARCH;
                    e->state_timer = 0;
                } else {
                    if(can_see) e->last_known_player = player.pos;
                    Vec3 dir = v3norm(to_player);
                    float spd = ENEMY_SPEED * dt;
                    float nx = e->pos.x + dir.x*spd;
                    float nz = e->pos.z + dir.z*spd;

                    /* Wall sliding for smooth movement */
                    if(!collides_map(nx, e->pos.z)) e->pos.x = nx;
                    else {
                        float sx = e->pos.x + dir.x*spd*0.5f;
                        float sz = e->pos.z + dir.z*spd;
                        if(!collides_map(sx, sz)) { e->pos.x=sx; e->pos.z=sz; }
                    }
                    if(!collides_map(e->pos.x, nz)) e->pos.z = nz;
                    else {
                        float sx = e->pos.x + dir.x*spd;
                        float sz = e->pos.z + dir.z*spd*0.5f;
                        if(!collides_map(sx, sz)) { e->pos.x=sx; e->pos.z=sz; }
                    }
                }
                break;

            case AI_SEARCH: {
                if(can_see) {
                    e->state = AI_CHASE;
                    e->last_known_player = player.pos;
                    e->state_timer = 0;
                    break;
                }
                /* Walk toward last-known player position */
                Vec3 tl = v3sub(e->last_known_player, e->pos);
                tl.y = 0;
                float dl = v3len(tl);
                if(dl > 0.5f) {
                    Vec3 dir = v3scale(tl, 1.0f/dl);
                    float spd = ENEMY_SPEED * 0.7f * dt;
                    float nx = e->pos.x + dir.x*spd;
                    float nz = e->pos.z + dir.z*spd;
                    if(!collides_map(nx, e->pos.z)) e->pos.x = nx;
                    if(!collides_map(e->pos.x, nz)) e->pos.z = nz;
                } else {
                    e->state = AI_IDLE;
                    e->state_timer = 0;
                }
                if(e->state_timer > 5.0f) {
                    e->state = AI_IDLE;
                    e->state_timer = 0;
                }
                break;
            }

            case AI_ATTACK:
                if(dist > ENEMY_ATTACK_RANGE*1.5f) {
                    e->state = AI_CHASE;
                    e->state_timer = 0;
                }
                if(e->attack_cooldown <= 0 && dist < ENEMY_ATTACK_RANGE*1.2f) {
                    /* Armor absorbs half of incoming damage */
                    int dmg = 10;
                    if(player.armor > 0) {
                        int absorb = dmg/2;
                        if(absorb > player.armor) absorb = player.armor;
                        player.armor -= absorb;
                        dmg -= absorb;
                    }
                    player.health -= dmg;
                    player.damage_flash = 0.3f;
                    if(player.health <= 0) { player.health = 0; game_over = 1; }
                    e->attack_cooldown = 0.8f;
                }
                break;
        }

        /* Alert nearby enemies within 8 units when this one spots player */
        if(can_see && (e->state == AI_CHASE || e->state == AI_ATTACK)) {
            for(int j=0;j<enemy_count;j++) {
                if(j == i || !enemies[j].alive) continue;
                if(enemies[j].state == AI_IDLE || enemies[j].state == AI_PATROL) {
                    Vec3 dd = v3sub(enemies[j].pos, e->pos);
                    dd.y = 0;
                    if(v3len(dd) < 8.0f) {
                        enemies[j].state = AI_SEARCH;
                        enemies[j].last_known_player = player.pos;
                        enemies[j].state_timer = 0;
                    }
                }
            }
        }
    }
}

void draw_enemies(void) {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_enemy);

    for(int i=0;i<enemy_count;i++) {
        if(!enemies[i].alive) continue;
        Vec3 p = enemies[i].pos;
        float bob = sinf(enemies[i].bob_phase) * 0.1f;

        /* Pain flash */
        if(enemies[i].pain_timer > 0) glColor3f(1, 0.3f, 0.3f);
        else                           glColor3f(1, 1, 1);

        glPushMatrix();
        glTranslatef(p.x, p.y + bob, p.z);
        /* Face the player */
        float dx = player.pos.x - p.x;
        float dz = player.pos.z - p.z;
        glRotatef(atan2f(dx, dz)*180.0f/PI, 0, 1, 0);
        glScalef(0.6f, 0.8f, 0.6f);
        obj_draw(&model_enemy);
        glPopMatrix();
    }
    glColor3f(1, 1, 1);
    glDisable(GL_TEXTURE_2D);
}

void enemies_reset(void) {
    enemy_count = 0;
}
