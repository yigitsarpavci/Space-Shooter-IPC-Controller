/*
 * CMPE322 Operating Systems Project: Shooter
 * Author: Mehmet Saraçoğlu
 * Designed as course material for use in the CMPE322 Operating Systems lectures.
 */

#ifndef PLANET_H
#define PLANET_H

struct planet {
    char name[256];
    int ID;
    int size_x; // Size in cells.
    int size_y;
};

int planet_create(struct planet *p, const char *name, int size_x, int size_y);
void planet_remove(struct planet *p);
int planet_edit(struct planet *p, const char *name, int size_x, int size_y);

#endif
