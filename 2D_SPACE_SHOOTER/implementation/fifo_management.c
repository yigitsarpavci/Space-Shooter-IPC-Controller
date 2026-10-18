/*
 * CMPE322 Operating Systems Project: Shooter
 * FIFO Management Implementation
 */

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "fifo_management.h"
#include "../input.h"

static int fifo_rfd = -1;
static int fifo_wfd = -1;

// Resolves the FIFO path based on working directory.
static const char *get_fifo_path(void) {
    struct stat st;
    if (stat("implementation", &st) == 0 && S_ISDIR(st.st_mode))
        return FIFO_PATH;
    return "game.fifo";
}

// Creates and opens the FIFO in non-blocking mode.
int fifo_init(void) {
    if (fifo_rfd >= 0) {
        close(fifo_rfd);
        fifo_rfd = -1;
    }
    if (fifo_wfd >= 0) {
        close(fifo_wfd);
        fifo_wfd = -1;
    }

    const char *path = get_fifo_path();
    if (mkfifo(path, 0666) != 0 && errno != EEXIST)
        return -1;

    // Open with O_RDWR so the FIFO never signals EOF when writers disconnect
    fifo_rfd = open(path, O_RDWR | O_NONBLOCK);
    if (fifo_rfd < 0) {
        fifo_rfd = open(path, O_RDONLY | O_NONBLOCK);
        if (fifo_rfd < 0)
            return -1;
        fifo_wfd = open(path, O_WRONLY | O_NONBLOCK);
    }

    fcntl(fifo_rfd, F_SETFD, FD_CLOEXEC);
    if (fifo_wfd >= 0)
        fcntl(fifo_wfd, F_SETFD, FD_CLOEXEC);

    return 0;
}

// Reads the next operation from the FIFO without blocking.
int fifo_read(void) {
    if (fifo_rfd < 0)
        return -1;

    while (1) {
        unsigned char ch;
        ssize_t n = read(fifo_rfd, &ch, 1);
        if (n <= 0) {
            if (n < 0 && errno == EINTR)
                continue;
            return -1;
        }

        switch (ch) {
        case CMD_UP:     return OP_UP;
        case CMD_DOWN:   return OP_DOWN;
        case CMD_LEFT:   return OP_LEFT;
        case CMD_RIGHT:  return OP_RIGHT;
        case CMD_SHOOT:  return OP_SHOOT;
        case CMD_SHIELD: return FIFO_SPAWN(OP_SHIELD_ACTIVATE);
        case CMD_GATHER: return FIFO_SPAWN(OP_GATHER);
        default:
            if (ch >= '0' && ch < '0' + OP_COUNT)
                return (int)(ch - '0');
            break;
        }
    }
}

// Closes descriptors and unlinks the FIFO file.
int fifo_destroy(void) {
    if (fifo_rfd >= 0) {
        close(fifo_rfd);
        fifo_rfd = -1;
    }
    if (fifo_wfd >= 0) {
        close(fifo_wfd);
        fifo_wfd = -1;
    }
    unlink(get_fifo_path());
    return 0;
}
