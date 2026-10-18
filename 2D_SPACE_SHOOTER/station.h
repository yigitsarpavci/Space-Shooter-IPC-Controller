/*
 * CMPE322 Operating Systems Project: Shooter
 * Author: Mehmet Saraçoğlu
 * Designed as course material for use in the CMPE322 Operating Systems lectures.
 */

#ifndef STATION_H
#define STATION_H

enum station_type_values {
    START,
    END,
    RECHARGE,
    ENEMY_SPAWN
};

struct station {
    char name[256];
    int ID;
    enum station_type_values station_type;
    int remaining_energy;
    int is_active;
};

int station_create(struct station *s, const char *name, enum station_type_values station_type, int energy);
void station_remove(struct station *s);
int station_edit(struct station *s, const char *name, int remaining_energy, int is_active);

#endif
