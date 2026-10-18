/*
 * CMPE322 Operating Systems Project: Shooter
 * Author: Mehmet Saraçoğlu
 * Designed as course material for use in the CMPE322 Operating Systems lectures.
 */

#ifndef CONTROL_H
#define CONTROL_H

#include "space.h"
#include "input.h"

#define CONTROL_DEFAULT_SEED 20260925u // Fallback random seed.
#define SHIELD_COOLDOWN_SECONDS 1.0 // Block rearm delay.
#define MOVE_HOLD_SECONDS 0.1 // Movement hold time.
#define ENEMY_THINK_MIN_SECONDS 0.15 // Shortest enemy think time.
#define ENEMY_THINK_MAX_SECONDS 0.35 // Longest enemy think time.
#define ENEMY_P_SHOOT_ALIGNED 0.9 // Aligned firing chance.
#define ENEMY_P_OTHER_AXIS 0.25 // Shorter axis chance.
#define MAX_TRACKED_SHIPS 512
#define MAX_SPAWNERS 64
#define SPAWN_RETRY_SECONDS 1.0 // Blocked spawn retry delay.

// Control layer setup.
int control_init(struct entity *player, unsigned int seed);

// Enemy spawn settings.
void control_set_spawn(double every_seconds, int move_speed, int bullet_speed, double fire_rate);

// Latches one input op.
void control_apply(enum input_op op);
int control_quit_requested(void);

enum control_outcome {
    OUTCOME_RUNNING,
    OUTCOME_WON,
    OUTCOME_LOST
};

enum control_outcome control_get_outcome(void);

// Player state for display.
struct control_player_state {
    int block_armed;
    double block_ready_in;
    double fire_ready_in;
};

void control_get_player_state(struct control_player_state *out);
long control_get_outcome_tick(void);

// Time controls.
void control_pause(void);
void control_continue(void);
int control_set_time_rate(double ticks_per_second);

// Latched time requests.
struct control_time {
    int pause_request; // Pending pause code.
    int has_rate;
    double rate;
};

// Takes latched time requests.
int control_take_time(struct control_time *out);

// Per-tick actions and hit resolution.
void control_commit(void);
void control_resolve(void);

#endif
