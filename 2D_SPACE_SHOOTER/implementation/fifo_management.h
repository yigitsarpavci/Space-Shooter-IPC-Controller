/*
 * CMPE322 Operating Systems Project: Shooter
 * Author: Mehmet Saraçoğlu
 * Designed as course material for use in the CMPE322 Operating Systems lectures.
 */

#ifndef FIFO_MANAGEMENT_H
#define FIFO_MANAGEMENT_H

#include "../input.h"

#define FIFO_PATH "implementation/game.fifo" // Fifo file path.

#define CMD_UP     'u' // Move up.
#define CMD_DOWN   'd' // Move down.
#define CMD_LEFT   'l' // Move left.
#define CMD_RIGHT  'r' // Move right.
#define CMD_SHOOT  'f' // Shoot.
#define CMD_SHIELD 's' // Shield spawn request.
#define CMD_GATHER 'g' // Gather spawn request.

#define FIFO_OP_CHAR(op) ('0' + (op)) // Digit a child writes.
#define FIFO_SPAWN(op)   (OP_COUNT + (op)) // Spawn request code.

int fifo_init(void);    // Create and open fifo.
int fifo_read(void);    // Next op, or -1.
int fifo_destroy(void); // Close and remove fifo.

#endif
