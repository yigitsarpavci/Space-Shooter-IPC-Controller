/*
 * CMPE322 Operating Systems Project: Shooter
 * Author: Mehmet Saraçoğlu
 * Designed as course material for use in the CMPE322 Operating Systems lectures.
 */

#include <stddef.h>
#include <string.h>
#include "planet.h"

static int planet_ID_values = 0;

// Sizes must be positive.
int planet_create(struct planet *p, const char *name, int size_x, int size_y) {
    if (p == NULL || name == NULL || size_x <= 0 || size_y <= 0)
        return -1;

    memset(p, 0, sizeof(*p));
    strncpy(p->name, name, sizeof(p->name) - 1);
    p->ID = planet_ID_values++;
    p->size_x = size_x;
    p->size_y = size_y;
    return 0;
}

void planet_remove(struct planet *p) {
    if (p != NULL)
        memset(p, 0, sizeof(*p));
}

// NULL name keeps current.
int planet_edit(struct planet *p, const char *name, int size_x, int size_y) {
    if (p == NULL || size_x <= 0 || size_y <= 0)
        return -1;

    if (name != NULL) {
        strncpy(p->name, name, sizeof(p->name) - 1);
        p->name[sizeof(p->name) - 1] = '\0';
    }
    p->size_x = size_x;
    p->size_y = size_y;
    return 0;
}
