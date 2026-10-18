/*
 * CMPE322 Operating Systems Project: Shooter
 * Author: Mehmet Saraçoğlu
 * Designed as course material for use in the CMPE322 Operating Systems lectures.
 */

#include <stddef.h>
#include <string.h>
#include "entity.h"

int entity_count = 0;

static struct entity entities[MAX_ENTITIES];
static int entity_in_use[MAX_ENTITIES];

// Reserves a free slot.
static struct entity *entity_alloc(enum entity_type_values type) {
    for (int i = 0; i < MAX_ENTITIES; i++) {
        if (!entity_in_use[i]) {
            entity_in_use[i] = 1;
            memset(&entities[i], 0, sizeof(entities[i]));
            entities[i].entity_type = type;
            entity_count++;
            return &entities[i];
        }
    }
    return NULL;
}

static void entity_release(struct entity *e) {
    entity_in_use[e - entities] = 0;
    entity_count--;
}

struct entity *entity_create_ship(const char *name, enum ship_type_values ship_type, int firing_ability,
                                  int move_speed, int bullet_speed, double fire_rate) {
    struct entity *e = entity_alloc(SHIP);
    if (e == NULL)
        return NULL;

    if (ship_create(&e->entity.ship_entity, name, ship_type, firing_ability, move_speed, bullet_speed,
                    fire_rate) != 0) {
        entity_release(e);
        return NULL;
    }
    return e;
}

struct entity *entity_create_planet(const char *name, int size_x, int size_y) {
    struct entity *e = entity_alloc(PLANET);
    if (e == NULL)
        return NULL;

    if (planet_create(&e->entity.planet_entity, name, size_x, size_y) != 0) {
        entity_release(e);
        return NULL;
    }
    return e;
}

struct entity *entity_create_bullet(int speed) {
    struct entity *e = entity_alloc(BULLET);
    if (e == NULL)
        return NULL;

    if (bullet_create(&e->entity.bullet_entity, speed) != 0) {
        entity_release(e);
        return NULL;
    }
    return e;
}

struct entity *entity_create_station(const char *name, enum station_type_values station_type, int energy) {
    struct entity *e = entity_alloc(STATION);
    if (e == NULL)
        return NULL;

    if (station_create(&e->entity.station_entity, name, station_type, energy) != 0) {
        entity_release(e);
        return NULL;
    }
    return e;
}

void entity_remove(struct entity *e) {
    if (e == NULL)
        return;

    switch (e->entity_type) {
    case SHIP:    ship_remove(&e->entity.ship_entity); break;
    case PLANET:  planet_remove(&e->entity.planet_entity); break;
    case BULLET:  bullet_remove(&e->entity.bullet_entity); break;
    case STATION: station_remove(&e->entity.station_entity); break;
    }
    entity_release(e);
}

int entity_edit_ship(struct entity *e, const char *name, int firing_ability, int move_speed, int bullet_speed,
                     double fire_rate) {
    struct ship *s = entity_get_ship(e);
    if (s == NULL)
        return -1;
    return ship_edit(s, name, firing_ability, move_speed, bullet_speed, fire_rate);
}

int entity_edit_planet(struct entity *e, const char *name, int size_x, int size_y) {
    struct planet *p = entity_get_planet(e);
    if (p == NULL)
        return -1;
    return planet_edit(p, name, size_x, size_y);
}

int entity_edit_bullet(struct entity *e, int speed) {
    struct bullet *b = entity_get_bullet(e);
    if (b == NULL)
        return -1;
    return bullet_edit(b, speed);
}

int entity_edit_station(struct entity *e, const char *name, int remaining_energy, int is_active) {
    struct station *s = entity_get_station(e);
    if (s == NULL)
        return -1;
    return station_edit(s, name, remaining_energy, is_active);
}

struct ship *entity_get_ship(struct entity *e) {
    return (e != NULL && e->entity_type == SHIP) ? &e->entity.ship_entity : NULL;
}

struct planet *entity_get_planet(struct entity *e) {
    return (e != NULL && e->entity_type == PLANET) ? &e->entity.planet_entity : NULL;
}

struct bullet *entity_get_bullet(struct entity *e) {
    return (e != NULL && e->entity_type == BULLET) ? &e->entity.bullet_entity : NULL;
}

struct station *entity_get_station(struct entity *e) {
    return (e != NULL && e->entity_type == STATION) ? &e->entity.station_entity : NULL;
}

int entity_get_ID(const struct entity *e) {
    if (e == NULL)
        return -1;

    switch (e->entity_type) {
    case SHIP:    return e->entity.ship_entity.ID;
    case PLANET:  return e->entity.planet_entity.ID;
    case BULLET:  return e->entity.bullet_entity.ID;
    case STATION: return e->entity.station_entity.ID;
    }
    return -1;
}

struct entity *entity_find(enum entity_type_values type, int ID) {
    for (int i = 0; i < MAX_ENTITIES; i++) {
        if (entity_in_use[i] && entities[i].entity_type == type && entity_get_ID(&entities[i]) == ID)
            return &entities[i];
    }
    return NULL;
}

struct entity *entity_at_index(int i) {
    if (i < 0 || i >= MAX_ENTITIES || !entity_in_use[i])
        return NULL;
    return &entities[i];
}
