/*
 * CMPE322 Operating Systems Project: Shooter
 * Shield Child Process
 */

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "fifo_management.h"
#include "../input.h"

// Resolves the FIFO path based on working directory.
static const char *get_fifo_path(void) {
    if (access(FIFO_PATH, F_OK) == 0)
        return FIFO_PATH;
    return "game.fifo";
}

int main(void) {
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
