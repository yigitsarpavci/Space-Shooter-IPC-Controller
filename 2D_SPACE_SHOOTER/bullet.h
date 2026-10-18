/*
 * CMPE322 Operating Systems Project: Shooter
 * Author: Mehmet Saraçoğlu
 * Designed as course material for use in the CMPE322 Operating Systems lectures.
 */

#ifndef BULLET_H
#define BULLET_H

#define BULLET_MAX_SPEED 120 // Max speed, d/s.

struct bullet {
    int ID;
    int speed;
};

// -1 if out of range.
int bullet_create(struct bullet *b, int speed);
void bullet_remove(struct bullet *b);
int bullet_edit(struct bullet *b, int speed);

#endif
