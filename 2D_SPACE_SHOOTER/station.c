/*
 * CMPE322 Operating Systems Project: Shooter
 * Author: Mehmet Saraçoğlu
 * Designed as course material for use in the CMPE322 Operating Systems lectures.
 */

#include <stddef.h>
#include <string.h>
#include "station.h"

static int station_ID_values = 0;

int station_create(struct station *s, const char *name, enum station_type_values station_type, int energy) {
    if (s == NULL || name == NULL)
        return -1;

    memset(s, 0, sizeof(*s));
    strncpy(s->name, name, sizeof(s->name) - 1);
    s->ID = station_ID_values++;
    s->station_type = station_type;
    s->remaining_energy = energy;
    s->is_active = 1;
    return 0;
}

void station_remove(struct station *s) {
    if (s != NULL)
        memset(s, 0, sizeof(*s));
}

// NULL name keeps current.
int station_edit(struct station *s, const char *name, int remaining_energy, int is_active) {
    if (s == NULL)
        return -1;

    if (name != NULL) {
        strncpy(s->name, name, sizeof(s->name) - 1);
        s->name[sizeof(s->name) - 1] = '\0';
    }
    s->remaining_energy = remaining_energy;
    s->is_active = is_active;
    return 0;
}
