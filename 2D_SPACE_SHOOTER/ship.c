/*
 * CMPE322 Operating Systems Project: Shooter
 * Author: Mehmet Saraçoğlu
 * Designed as course material for use in the CMPE322 Operating Systems lectures.
 */

#include <stddef.h>
#include <string.h>
#include "ship.h"

static int ship_ID_values = 0;

static int pace_in_range(int move_speed, int bullet_speed, double fire_rate) {
    return move_speed >= 0 && move_speed <= SHIP_MAX_SPEED && bullet_speed >= 0 && bullet_speed <= SHIP_MAX_SPEED
        && fire_rate >= 0 && fire_rate <= SHIP_MAX_FIRE_RATE;
}

int ship_create(struct ship *s, const char *name, enum ship_type_values ship_type, int firing_ability,
                int move_speed, int bullet_speed, double fire_rate) {
    if (s == NULL || name == NULL || !pace_in_range(move_speed, bullet_speed, fire_rate))
        return -1;

    memset(s, 0, sizeof(*s));
    strncpy(s->name, name, sizeof(s->name) - 1);
    s->ID = ship_ID_values++;
    s->ship_type = ship_type;
    s->shield_energy = SHIP_DEFAULT_SHIELD;
    s->firing_ability = firing_ability;
    s->move_speed = move_speed;
    s->bullet_speed = bullet_speed;
    s->fire_rate = fire_rate;
    s->is_alive = 1;
    return 0;
}

void ship_remove(struct ship *s) {
    if (s != NULL)
        memset(s, 0, sizeof(*s));
}

// NULL name keeps current.
int ship_edit(struct ship *s, const char *name, int firing_ability, int move_speed, int bullet_speed,
              double fire_rate) {
    if (s == NULL || !pace_in_range(move_speed, bullet_speed, fire_rate))
        return -1;

    if (name != NULL) {
        strncpy(s->name, name, sizeof(s->name) - 1);
        s->name[sizeof(s->name) - 1] = '\0';
    }
    s->firing_ability = firing_ability;
    s->move_speed = move_speed;
    s->bullet_speed = bullet_speed;
    s->fire_rate = fire_rate;
    return 0;
}
