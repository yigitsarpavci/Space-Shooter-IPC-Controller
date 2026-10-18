/*
 * CMPE322 Operating Systems Project: Shooter
 * Author: Mehmet Saraçoğlu
 * Designed as course material for use in the CMPE322 Operating Systems lectures.
 */

#ifndef INPUT_STUDENT_H
#define INPUT_STUDENT_H

#include <signal.h>
#include <sys/types.h>
#include "../input.h"

#define SIG_PAUSE SIGUSR1 // Pause signal.
#define SIG_QUIT  SIGTERM // Quit signal.

#define SO_PATH        "implementation/libfifo_management.so" // Fifo library path.
#define SO_SYM_INIT    "fifo_init" // Init symbol.
#define SO_SYM_READ    "fifo_read" // Read symbol.
#define SO_SYM_DESTROY "fifo_destroy" // Destroy symbol.

#define SHIELD_PATH "implementation/shield" // Shield child binary.
#define GATHER_PATH "implementation/gather" // Gather child binary.

#define RING_SIZE 64 // Ring capacity.
#define POLL_STEP_NS (1000000000L / 60) // One frame, in ns.

// Ring buffer of ops.
extern enum input_op ring[RING_SIZE];
extern int ring_head, ring_tail, ring_count;

void ring_init(void); // Empty the ring.
void ring_push(enum input_op op); // Queue op, drop if full.
int ring_pop(enum input_op *op); // Oldest op; 0 if empty.

void signal_handler(int signum); // Push signal's op.
int signal_register(void); // Install signal handlers.
void signal_unregister(void); // Restore default handlers.

int so_load(const char *path); // Load library, check symbols.
void so_unload(void); // Unload library.
int so_call(const char *symbol); // Call symbol, or -1.

pid_t child_create(enum input_op op); // Fork + exec child.

int dispatch(void); // Drain fifo into ring.

#endif
