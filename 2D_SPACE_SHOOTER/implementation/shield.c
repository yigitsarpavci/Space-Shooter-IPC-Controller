/*
 * CMPE322 Operating Systems Project: Shooter
 * Shield Child Process
 */

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "fifo_management.h"
#include "../input.h"

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

int main(void) {
    // Ignore SIGPIPE to avoid abnormal termination if reader closes pipe
    signal(SIGPIPE, SIG_IGN);

    const char *fifo = get_fifo_path();
    int fd = -1;
    while ((fd = open(fifo, O_WRONLY)) < 0 && errno == EINTR);

    if (fd >= 0) {
        char ch = FIFO_OP_CHAR(OP_SHIELD_ACTIVATE);
        ssize_t written = 0;
        while (written < 1) {
            ssize_t n = write(fd, &ch, 1);
            if (n > 0)
                written += n;
            else if (n < 0 && errno != EINTR)
                break;
        }
        close(fd);
    }

    // Shield effect duration is 1 second
    unsigned int remaining = 1;
    while (remaining > 0)
        remaining = sleep(remaining);

    return 0;
}
