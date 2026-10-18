/*
 * CMPE322 Operating Systems Project: Shooter
 * Author: Mehmet Saraçoğlu
 * Designed as course material for use in the CMPE322 Operating Systems lectures.
 */

#include <stddef.h>
#include <string.h>
#include "bullet.h"

static int bullet_ID_values = 0;

int bullet_create(struct bullet *b, int speed) {
    if (b == NULL || speed < 0 || speed > BULLET_MAX_SPEED)
        return -1;

    memset(b, 0, sizeof(*b));
    b->ID = bullet_ID_values++;
    b->speed = speed;
    return 0;
}

void bullet_remove(struct bullet *b) {
    if (b != NULL)
        memset(b, 0, sizeof(*b));
}

int bullet_edit(struct bullet *b, int speed) {
    if (b == NULL || speed < 0 || speed > BULLET_MAX_SPEED)
        return -1;

    b->speed = speed;
    return 0;
}
