/*
 * CMPE322 Operating Systems Project: Shooter
 * FIFO Management Implementation
 */

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "fifo_management.h"
#include "../input.h"

// Non-blocking FIFO descriptors: read descriptor and fallback dummy writer.
static int fifo_rfd = -1;
static int fifo_wfd = -1;

/*
 * Resolves the FIFO path based on current working directory.
 * When running from package root, uses FIFO_PATH ("implementation/game.fifo").
 * When running from within implementation/, uses "game.fifo".
 */
static const char *get_fifo_path(void) {
    struct stat st;
    if (stat("implementation", &st) == 0 && S_ISDIR(st.st_mode))
        return FIFO_PATH;
    return "game.fifo";
}

/*
 * Creates and opens the FIFO in non-blocking mode.
 * Opens with O_RDWR (or O_RDONLY with a companion O_WRONLY descriptor) so that
 * the read descriptor never receives an EOF (read() == 0) when external writers
 * disconnect. Sets FD_CLOEXEC to prevent leaking descriptors to children.
 * Returns 0 on success, or -1 on failure.
 */
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

    // If a stale non-FIFO file exists at the path, remove it first
    struct stat st;
    if (stat(path, &st) == 0 && !S_ISFIFO(st.st_mode)) {
        unlink(path);
    }

    if (mkfifo(path, 0666) != 0 && errno != EEXIST)
        return -1;

    // Open O_RDWR so the FIFO never signals EOF when external writers disconnect
    fifo_rfd = open(path, O_RDWR | O_NONBLOCK);
    if (fifo_rfd < 0) {
        // Fallback for systems where O_RDWR on FIFO is unsupported
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

/*
 * Reads the next available operation from the FIFO without blocking.
 * Returns the operation code, FIFO_SPAWN(op) for spawn requests ('s', 'g'),
 * or -1 if no valid operations are waiting. Discards unrecognized characters.
 */
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
            // Child processes write op enum values encoded as digits ('0' + op)
            if (ch >= '0' && ch < '0' + OP_COUNT)
                return (int)(ch - '0');
            break; // Ignore all other characters and read next byte
        }
    }
}

/*
 * Closes the FIFO descriptors and unlinks the FIFO file.
 * Returns 0.
 */
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
