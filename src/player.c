#include "player.h"
#include "level.h"
#include "enemy.h"
#include "pickup.h"
#include "particles.h"
#include "hud.h"
#include "lighting.h"

Player player;

Vec3 player_forward(void) {
    return v3(sinf(player.yaw)*cosf(player.pitch),
              sinf(player.pitch),
              cosf(player.yaw)*cosf(player.pitch));
}

void player_update(float dt) {
    if(ui.game_over || ui.game_win) return;

    const Uint8 *keys = SDL_GetKeyboardState(NULL);

    /* Movement: build XZ forward/right vectors from yaw */
    Vec3 fwd_xz = v3(sinf(player.yaw), 0, cosf(player.yaw));
    Vec3 rgt_xz = v3(cosf(player.yaw), 0, -sinf(player.yaw));
    Vec3 move = v3(0, 0, 0);
    if(keys[SDL_SCANCODE_W]) move = v3add(move, fwd_xz);
    if(keys[SDL_SCANCODE_S]) move = v3sub(move, fwd_xz);
    if(keys[SDL_SCANCODE_A]) move = v3add(move, rgt_xz);
    if(keys[SDL_SCANCODE_D]) move = v3sub(move, rgt_xz);

    float ml = v3len(move);
    if(ml > 0) {
        move = v3scale(move, MOVE_SPEED*dt/ml);
        float nx = player.pos.x + move.x;
        float nz = player.pos.z + move.z;
        /* Axis-separated movement for wall sliding */
        if(!collides_map(nx, player.pos.z)) player.pos.x = nx;
        if(!collides_map(player.pos.x, nz)) player.pos.z = nz;
    }

    /* Gravity & vertical movement */
    if(!player.on_ground) player.vy -= GRAVITY*dt;
    player.pos.y += player.vy*dt;

    /* Floor */
    if(player.pos.y <= PLAYER_HEIGHT) {
        player.pos.y = PLAYER_HEIGHT;
        player.vy = 0;
        player.on_ground = 1;
    }
    /* Ceiling */
    if(player.pos.y > CEIL_Y - 0.1f) {
        player.pos.y = CEIL_Y - 0.1f;
        player.vy = 0;
    }
    /* Jump */
    if(keys[SDL_SCANCODE_SPACE] && player.on_ground) {
        player.vy = JUMP_VEL;
        player.on_ground = 0;
    }

    /* Timers */
    if(player.shoot_cd > 0)     player.shoot_cd     -= dt;
    if(player.muzzle_flash > 0) player.muzzle_flash -= dt;
    if(player.damage_flash > 0) player.damage_flash -= dt;

    /* Arrow keys move the scene light (for lighting effects demo) */
    float lms = 5.0f * dt;
    if(keys[SDL_SCANCODE_UP])    light.scene_light_pos[2] += lms;
    if(keys[SDL_SCANCODE_DOWN])  light.scene_light_pos[2] -= lms;
    if(keys[SDL_SCANCODE_LEFT])  light.scene_light_pos[0] -= lms;
    if(keys[SDL_SCANCODE_RIGHT]) light.scene_light_pos[0] += lms;

    /* Pickup collection */
    for(int i=0;i<pickup_count;i++) {
        if(!pickups[i].active) continue;
        pickups[i].bob_phase += dt * 3.0f;
        float dx = player.pos.x - pickups[i].pos.x;
        float dz = player.pos.z - pickups[i].pos.z;
        if(dx*dx + dz*dz < 1.0f) {
            switch(pickups[i].type) {
                case PICKUP_HEALTH:
                    if(player.health < 100) {
                        player.health += 25;
                        if(player.health > 100) player.health = 100;
                        pickups[i].active = 0;
                    }
                    break;
                case PICKUP_AMMO:
                    player.ammo_pistol  += 15;
                    player.ammo_shotgun += 4;
                    pickups[i].active    = 0;
                    break;
                case PICKUP_ARMOR:
                    if(player.armor < 200) {
                        player.armor += 25;
                        if(player.armor > 200) player.armor = 200;
                        pickups[i].active = 0;
                    }
                    break;
            }
        }
    }
}

void player_shoot(void) {
    float cooldown=0, range=0;
    int damage=0;

    switch(player.weapon) {
        case WPN_FIST:
            cooldown = SHOOT_COOLDOWN_FIST;
            range = FIST_RANGE;
            damage = FIST_DAMAGE;
            break;
        case WPN_PISTOL:
            if(player.ammo_pistol <= 0) return;
            cooldown = SHOOT_COOLDOWN_PISTOL;
            range = PISTOL_RANGE;
            damage = PISTOL_DAMAGE;
            break;
        case WPN_SHOTGUN:
            if(player.ammo_shotgun <= 0) return;
            cooldown = SHOOT_COOLDOWN_SHOTGUN;
            range = SHOTGUN_RANGE;
            damage = SHOTGUN_DAMAGE;
            break;
    }
    if(player.shoot_cd > 0) return;
    if(player.weapon == WPN_PISTOL)  player.ammo_pistol--;
    if(player.weapon == WPN_SHOTGUN) player.ammo_shotgun--;

    player.shoot_cd = cooldown;
    player.muzzle_flash = MUZZLE_TIME;

    /* Hitscan: project each enemy onto the forward ray */
    Vec3 fwd = player_forward();
    for(int i=0;i<enemy_count;i++) {
        if(!enemies[i].alive) continue;
        Vec3 d = v3sub(enemies[i].pos, player.pos);
        float along = v3dot(d, fwd);
        if(along < 0.5f || along > range) continue;
        Vec3 closest = v3sub(d, v3scale(fwd, along));
        float hit_r = (player.weapon == WPN_SHOTGUN) ? SHOTGUN_SPREAD : 0.55f;
        if(v3len(closest) < hit_r) {
            /* Block bullets with walls (includes windows) */
            float ex = enemies[i].pos.x, ez = enemies[i].pos.z;
            float px2 = player.pos.x, pz2 = player.pos.z;
            float ddx = ex - px2, ddz = ez - pz2;
            float dist2 = sqrtf(ddx*ddx + ddz*ddz);
            int blocked = 0;
            if(dist2 > 0.1f) {
                float step = 0.4f;
                int steps = (int)(dist2/step) + 1;
                for(int s=1;s<steps;s++) {
                    float t = (float)s/steps;
                    int cc = (int)floorf(px2 + ddx*t);
                    int rr = (int)floorf(pz2 + ddz*t);
                    if(map_is_wall(cc, rr)) { blocked = 1; break; }
                }
            }
            if(blocked) continue;

            enemies[i].health     -= damage;
            enemies[i].pain_timer  = 0.2f;
            enemies[i].state       = AI_CHASE;
            enemies[i].last_known_player = player.pos;
            emit_blood(enemies[i].pos);

            if(enemies[i].health <= 0) {
                enemies[i].alive = 0;
                /* Check win condition */
                int alive = 0;
                for(int j=0;j<enemy_count;j++) alive += enemies[j].alive;
                if(!alive) ui.game_win = 1;
            }
        }
    }
}

void player_reset(void) {
    /* Full state reset - position will be set by level load */
    player.yaw = 0; player.pitch = 0; player.vy = 0;
    player.on_ground = 1;
    player.health = 100;
    player.armor = 0;
    player.ammo_pistol  = 50;
    player.ammo_shotgun = 10;
    player.weapon = WPN_PISTOL;
    player.shoot_cd = 0;
    player.muzzle_flash = 0;
    player.damage_flash = 0;
}
