/*
 * CMPE322 Operating Systems Project: Shooter
 * Author: Mehmet Saraçoğlu
 * Designed as course material for use in the CMPE322 Operating Systems lectures.
 */

#ifndef SPACE_H
#define SPACE_H

#include "entity.h"

#define GRID_MAX_X 256 // Grid width limit.
#define GRID_MAX_Y 256 // Grid height limit.
#define MAX_SPACE_OBJECTS MAX_ENTITIES

#define SPACE_TICKS_PER_SECOND 60
#define SPACE_TICK_SECONDS (1.0 / SPACE_TICKS_PER_SECOND)
#define SPACE_MAX_SPEED 120 // Speed limit in d/s.

// NORTH decreases y.
enum orientation_values {
    NORTH,
    EAST,
    SOUTH,
    WEST
};

// Snapshot of a placed entity.
struct space_info {
    struct entity *entity;
    enum entity_type_values entity_type;
    int ID;
    int pos_x; // Anchor slot, bottom-left.
    int pos_y;
    enum orientation_values orientation;
    int speed; // Speed in d/s.
    int blocked; // Last move was blocked.
    int size_x; // Footprint width.
    int size_y;
};

// Resets the grid and time.
int space_init(int size_x, int size_y);
void space_get_size(int *size_x, int *size_y);
int space_in_bounds(int x, int y);
long space_get_time(void);
double space_get_seconds(void);

// Advances time one tick.
void space_tick(void);

// Places and moves entities.
int space_place(struct entity *e, int x, int y, enum orientation_values orientation);
int space_unplace(struct entity *e);
int space_move_to(struct entity *e, int x, int y);
int space_step(struct entity *e, enum orientation_values direction);
int space_set_orientation(struct entity *e, enum orientation_values orientation);

// Speed access in d/s.
int space_get_speed(const struct entity *e);
int space_set_speed(struct entity *e, int speed);

// Queries placed entities.
int space_is_placed(const struct entity *e);
struct entity *space_get(int x, int y);
int space_get_info(const struct entity *e, struct space_info *out);
int space_get_info_at(int x, int y, struct space_info *out);
int space_object_count(void);
struct entity *space_object_at_index(int i);

#endif
