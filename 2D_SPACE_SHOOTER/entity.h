/*
 * CMPE322 Operating Systems Project: Shooter
 * Author: Mehmet Saraçoğlu
 * Designed as course material for use in the CMPE322 Operating Systems lectures.
 */

#ifndef ENTITY_H
#define ENTITY_H

#include "ship.h"
#include "bullet.h"
#include "station.h"
#include "planet.h"

#define MAX_ENTITIES 4096

extern int entity_count;

enum entity_type_values {
    SHIP,
    PLANET,
    BULLET,
    STATION
};

union entity_type {
    struct ship ship_entity;
    struct planet planet_entity;
    struct bullet bullet_entity;
    struct station station_entity;
};

struct entity {
    union entity_type entity;
    enum entity_type_values entity_type;
};

// Creates an entity.
struct entity *entity_create_ship(const char *name, enum ship_type_values ship_type, int firing_ability,
                                  int move_speed, int bullet_speed, double fire_rate);
struct entity *entity_create_planet(const char *name, int size_x, int size_y);
struct entity *entity_create_bullet(int speed);
struct entity *entity_create_station(const char *name, enum station_type_values station_type, int energy);

// Removes an entity.
void entity_remove(struct entity *e);

// Edits an entity.
int entity_edit_ship(struct entity *e, const char *name, int firing_ability, int move_speed, int bullet_speed,
                     double fire_rate);
int entity_edit_planet(struct entity *e, const char *name, int size_x, int size_y);
int entity_edit_bullet(struct entity *e, int speed);
int entity_edit_station(struct entity *e, const char *name, int remaining_energy, int is_active);

// Returns typed data, or NULL.
struct ship *entity_get_ship(struct entity *e);
struct planet *entity_get_planet(struct entity *e);
struct bullet *entity_get_bullet(struct entity *e);
struct station *entity_get_station(struct entity *e);

int entity_get_ID(const struct entity *e);
struct entity *entity_find(enum entity_type_values type, int ID);
struct entity *entity_at_index(int i);

#endif
