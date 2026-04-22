#ifndef DOOM3D_ENEMY_H
#define DOOM3D_ENEMY_H

#include "common.h"
#include "math3d.h"

/* 5-state finite state machine for enemy AI:
 *   IDLE    - standing still, hasn't seen player
 *   PATROL  - wandering near spawn point
 *   CHASE   - pursuing player with line-of-sight
 *   SEARCH  - lost LOS, moving to last known position
 *   ATTACK  - close enough to melee
 */
typedef enum { AI_IDLE, AI_PATROL, AI_CHASE, AI_SEARCH, AI_ATTACK } AIState;

typedef struct {
    Vec3    pos;
    Vec3    spawn_pos;
    Vec3    last_known_player;
    int     health;
    float   pain_timer;
    int     alive;
    float   bob_phase;
    AIState state;
    float   state_timer;
    float   patrol_angle;
    float   attack_cooldown;
} Enemy;

extern Enemy enemies[MAX_ENEMIES];
extern int   enemy_count;

void enemy_spawn(float x, float z);
void enemies_update(float dt); /* runs AI on all enemies */
void draw_enemies(void);
void enemies_reset(void);

#endif
