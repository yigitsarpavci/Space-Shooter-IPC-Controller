/*
 * CMPE322 Operating Systems Project: Shooter
 * Author: Mehmet Saraçoğlu
 * Designed as course material for use in the CMPE322 Operating Systems lectures.
 */

#ifndef INPUT_H
#define INPUT_H

enum input_op {
    OP_UP,
    OP_DOWN,
    OP_LEFT,
    OP_RIGHT,
    OP_SHIELD_ACTIVATE,
    OP_GATHER,
    OP_SHOOT,
    OP_PAUSE, // Toggles pause.
    OP_QUIT,
    OP_COUNT // Count, not an op.
};

// Initialize.
int input_init(void);

// Clean up.
void input_shutdown(void);

// Hand out ops.
int input_poll(int timeout_ms, enum input_op *ops, int max_ops);

#endif
