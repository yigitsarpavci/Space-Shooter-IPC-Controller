/*
 * CMPE322 Operating Systems Project: Shooter
 * Author: Mehmet Saraçoğlu
 * Designed as course material for use in the CMPE322 Operating Systems lectures.
 */

#ifndef LEVEL_H
#define LEVEL_H

#include "space.h"

// Level file loader.
struct level {
    unsigned int seed;
    int has_seed;
    struct entity *player;
    int enemy_move_speed; // Enemy ship defaults.
    int enemy_bullet_speed;
    double enemy_fire_rate;
    double spawn_every; // Spawn interval in seconds.
};

// Loads a level file.
int level_load(const char *path, struct level *out, char *error, int error_len);

#endif
