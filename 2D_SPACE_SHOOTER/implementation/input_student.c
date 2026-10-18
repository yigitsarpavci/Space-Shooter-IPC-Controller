/*
 * CMPE322 Operating Systems Project: Shooter
 * Student Input Mechanism Implementation
 */

#include <dlfcn.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "input_student.h"
#include "../input.h"

// Ring buffer of ops.
enum input_op ring[RING_SIZE];
int ring_head = 0;
int ring_tail = 0;
int ring_count = 0;

// Initializes the circular buffer to empty.
void ring_init(void) {
    ring_head = 0;
    ring_tail = 0;
    ring_count = 0;
}

// Appends an operation to the ring buffer, dropping it if full.
void ring_push(enum input_op op) {
    if (ring_count >= RING_SIZE)
        return;
    ring[ring_tail] = op;
    ring_tail = (ring_tail + 1) % RING_SIZE;
    ring_count++;
}

// Extracts the oldest operation from the ring buffer.
int ring_pop(enum input_op *op) {
    if (ring_count <= 0)
        return 0;
    if (op != NULL)
        *op = ring[ring_head];
    ring_head = (ring_head + 1) % RING_SIZE;
    ring_count--;
    return 1;
}

// Signal handler converting pause and quit signals into operations.
void signal_handler(int signum) {
    if (signum == SIG_PAUSE)
        ring_push(OP_PAUSE);
    else if (signum == SIG_QUIT)
        ring_push(OP_QUIT);
}

// Registers handlers for SIG_PAUSE (SIGUSR1) and SIG_QUIT (SIGTERM).
int signal_register(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIG_PAUSE);
    sigaddset(&sa.sa_mask, SIG_QUIT);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIG_PAUSE, &sa, NULL) != 0)
        return -1;
    if (sigaction(SIG_QUIT, &sa, NULL) != 0) {
        signal_unregister();
        return -1;
    }
    return 0;
}

// Restores default signal dispositions.
void signal_unregister(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sigaction(SIG_PAUSE, &sa, NULL);
    sigaction(SIG_QUIT, &sa, NULL);
}

static void *so_handle = NULL;
typedef int (*fifo_func_t)(void);
static fifo_func_t fn_init = NULL;
static fifo_func_t fn_read = NULL;
static fifo_func_t fn_destroy = NULL;

// Dynamically loads the shared library and resolves required symbols.
int so_load(const char *path) {
    if (path == NULL)
        return -1;
    if (so_handle != NULL)
        so_unload();

    const char *actual_path = path;
    char alt_path[256];

    if (access(path, F_OK) != 0) {
        const char *base = strrchr(path, '/');
        const char *fname = base ? base + 1 : path;
        snprintf(alt_path, sizeof(alt_path), "./%s", fname);
        if (access(alt_path, F_OK) == 0)
            actual_path = alt_path;
    }

    so_handle = dlopen(actual_path, RTLD_NOW);
    if (so_handle == NULL)
        return -1;

    fn_init = (fifo_func_t)dlsym(so_handle, SO_SYM_INIT);
    fn_read = (fifo_func_t)dlsym(so_handle, SO_SYM_READ);
    fn_destroy = (fifo_func_t)dlsym(so_handle, SO_SYM_DESTROY);

    if (fn_init == NULL || fn_read == NULL || fn_destroy == NULL) {
        so_unload();
        return -1;
    }
    return 0;
}

// Unloads the shared library.
void so_unload(void) {
    if (so_handle != NULL) {
        dlclose(so_handle);
        so_handle = NULL;
    }
    fn_init = NULL;
    fn_read = NULL;
    fn_destroy = NULL;
}

// Invokes an exported symbol by name.
int so_call(const char *symbol) {
    if (so_handle == NULL || symbol == NULL)
        return -1;

    fifo_func_t fn = NULL;
    if (strcmp(symbol, SO_SYM_INIT) == 0)
        fn = fn_init;
    else if (strcmp(symbol, SO_SYM_READ) == 0)
        fn = fn_read;
    else if (strcmp(symbol, SO_SYM_DESTROY) == 0)
        fn = fn_destroy;
    else
        fn = (fifo_func_t)dlsym(so_handle, symbol);

    if (fn == NULL)
        return -1;
    return fn();
}

// Spawns a parallel child process for shield or gather requests.
pid_t child_create(enum input_op op) {
    const char *path = NULL;
    if (op == OP_SHIELD_ACTIVATE) {
        if (access(SHIELD_PATH, X_OK) == 0)
            path = SHIELD_PATH;
        else if (access("./shield", X_OK) == 0)
            path = "./shield";
        else
            return -1;
    } else if (op == OP_GATHER) {
        if (access(GATHER_PATH, X_OK) == 0)
            path = GATHER_PATH;
        else if (access("./gather", X_OK) == 0)
            path = "./gather";
        else
            return -1;
    } else {
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0)
        return -1;
    if (pid == 0) {
        execl(path, path, (char *)NULL);
        _exit(127);
    }
    return pid;
}

// Drains pending operations from the FIFO into the ring buffer.
int dispatch(void) {
    int count = 0;
    while (1) {
        int ret = so_call(SO_SYM_READ);
        if (ret < 0)
            break;
        if (ret >= OP_COUNT) {
            enum input_op child_op = (enum input_op)(ret - OP_COUNT);
            child_create(child_op);
        } else {
            ring_push((enum input_op)ret);
        }
        count++;
    }
    while (waitpid(-1, NULL, WNOHANG) > 0);
    return count;
}

static int started = 0;

// Initializes the input subsystem.
int input_init(void) {
    if (started)
        return 0;

    ring_init();
    if (signal_register() != 0)
        return -1;
    if (so_load(SO_PATH) != 0) {
        signal_unregister();
        return -1;
    }
    if (so_call(SO_SYM_INIT) != 0) {
        so_unload();
        signal_unregister();
        return -1;
    }

    started = 1;
    return 0;
}

// Cleans up all resources.
void input_shutdown(void) {
    if (!started)
        return;

    so_call(SO_SYM_DESTROY);
    so_unload();
    signal_unregister();
    while (waitpid(-1, NULL, WNOHANG) > 0);

    started = 0;
}

// Polls for available input operations within the specified timeout.
int input_poll(int timeout_ms, enum input_op *ops, int max_ops) {
    if (!started || ops == NULL || max_ops <= 0)
        return 0;

    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    long timeout_ns = (long)timeout_ms * 1000000L;
    if (timeout_ns < 0)
        timeout_ns = 0;

    while (1) {
        dispatch();

        if (ring_count > 0) {
            int count = 0;
            while (count < max_ops && ring_pop(&ops[count])) {
                count++;
            }
            return count;
        }

        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        long elapsed_ns = (now.tv_sec - start.tv_sec) * 1000000000L + (now.tv_nsec - start.tv_nsec);
        if (elapsed_ns >= timeout_ns)
            break;

        long remaining_ns = timeout_ns - elapsed_ns;
        long step_ns = POLL_STEP_NS / 16;
        if (step_ns > remaining_ns)
            step_ns = remaining_ns;

        struct timespec req;
        req.tv_sec = step_ns / 1000000000L;
        req.tv_nsec = step_ns % 1000000000L;
        nanosleep(&req, NULL);
    }

    return 0;
}
