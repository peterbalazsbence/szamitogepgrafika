#ifndef DOOM3D_ENEMY_H
#define DOOM3D_ENEMY_H

#include "common.h"
#include "math3d.h"

/* AI states for the enemy
 *   IDLE   - standing still, hasn't seen player
 *   PATROL - wandering randomly near spawn point
 *   CHASE  - pursuing player with active line-of-sight
 *   SEARCH - lost LOS, walking toward last known position
 *   ATTACK - close enough to deal melee damage
 */
typedef enum { AI_IDLE, AI_PATROL, AI_CHASE, AI_SEARCH, AI_ATTACK } AIState;

/* Single enemy state. spawn_pos is used to bound patrol wandering;
 * last_known_player is updated whenever the enemy can see the player. */
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

/* Adds a new enemy at (x,z) with default stats. */
void enemy_spawn(float x, float z);

/* Runs AI state machine for every alive enemy and applies movement.
 * Damages the player when an attacking enemy lands a hit. */
void enemies_update(float dt);

/* Renders all alive enemies (billboarded toward the player). */
void draw_enemies(void);

/* Removes all enemies from the world. */
void enemies_reset(void);

#endif
