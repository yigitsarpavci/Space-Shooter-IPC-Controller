/*
 * CMPE322 Operating Systems Project: Shooter
 * Author: Mehmet Saraçoğlu
 * Designed as course material for use in the CMPE322 Operating Systems lectures.
 */

#ifndef SHIP_H
#define SHIP_H

#define SHIP_DEFAULT_SHIELD 3 // Initial shield energy.
#define SHIP_MAX_SHIELD SHIP_DEFAULT_SHIELD
#define SHIP_MAX_SPEED 120 // Max speed, d/s.
#define SHIP_MAX_FIRE_RATE 60.0 // Shots per second.

enum ship_type_values {
    PLAYER,
    ENEMY
};

struct ship {
    char name[256];
    int ID;
    enum ship_type_values ship_type;
    int shield_energy;
    int firing_ability; // 0: no, 1: yes.
    int move_speed;
    int bullet_speed;
    double fire_rate; // Zero means no limit.
    int is_alive;
};

// -1 if out of range.
int ship_create(struct ship *s, const char *name, enum ship_type_values ship_type, int firing_ability,
                int move_speed, int bullet_speed, double fire_rate);
void ship_remove(struct ship *s);
int ship_edit(struct ship *s, const char *name, int firing_ability, int move_speed, int bullet_speed,
              double fire_rate);

#endif
