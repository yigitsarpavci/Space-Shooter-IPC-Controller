/*
 * CMPE322 Operating Systems Project: Shooter
 * Author: Mehmet Saraçoğlu
 * Designed as course material for use in the CMPE322 Operating Systems lectures.
 */

#include <errno.h>
#include <poll.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include "input.h"

#define ESC 0x1b

static struct termios saved;
static int is_raw = 0;
static int started = 0;

static void restore_terminal(void) {
    if (is_raw) {
        tcsetattr(STDIN_FILENO, TCSANOW, &saved);
        is_raw = 0;
    }
}

// Key to op, or -1.
static int key_op(unsigned char key) {
    switch (key) {
    case 'w': case 'W': return OP_UP;
    case 'a': case 'A': return OP_LEFT;
    case 's': case 'S': return OP_DOWN;
    case 'd': case 'D': return OP_RIGHT;
    case 'j': case 'J': return OP_SHIELD_ACTIVATE;
    case 'k': case 'K': return OP_GATHER;
    case 'l': case 'L': return OP_SHOOT;
    case 'p': case 'P': return OP_PAUSE;
    case 'q': case 'Q': return OP_QUIT;
    default:            return -1;
    }
}

// Raw keys without echo.
int input_init(void) {
    if (started)
        return 0;

    if (isatty(STDIN_FILENO)) {
        if (tcgetattr(STDIN_FILENO, &saved) != 0)
            return -1;

        struct termios raw = saved;
        raw.c_lflag &= ~(tcflag_t)(ICANON | ECHO);
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;
        if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) != 0)
            return -1;

        is_raw = 1;
        static int registered = 0;
        if (!registered && atexit(restore_terminal) == 0)
            registered = 1;
    }
    started = 1;
    return 0;
}

void input_shutdown(void) {
    restore_terminal();
    started = 0;
}

// Escape sequence length.
static int escape_length(const unsigned char *buf, int n) {
    if (n < 2)
        return 1;
    if (buf[1] == 'O')
        return n < 3 ? n : 3;
    if (buf[1] != '[')
        return 1;

    for (int i = 2; i < n; i++)
        if (buf[i] >= 0x40 && buf[i] <= 0x7e)
            return i + 1;
    return n;
}

int input_poll(int timeout_ms, enum input_op *ops, int max_ops) {
    if (!started || ops == NULL || max_ops <= 0)
        return 0;

    struct pollfd pfd = { STDIN_FILENO, POLLIN, 0 };
    int ready = poll(&pfd, 1, timeout_ms > 0 ? timeout_ms : 0);
    if (ready <= 0)
        return 0;
    if (!(pfd.revents & POLLIN))
        return pfd.revents & (POLLHUP | POLLERR) ? -1 : 0;

    unsigned char buf[256];
    ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
    if (n == 0)
        return -1;
    if (n < 0)
        return errno == EINTR || errno == EAGAIN ? 0 : -1;

    int count = 0;
    for (int i = 0; i < n && count < max_ops;) {
        if (buf[i] == ESC) {
            i += escape_length(buf + i, (int)n - i);
            continue;
        }
        int op = key_op(buf[i++]);
        if (op >= 0)
            ops[count++] = (enum input_op)op;
    }
    return count;
}
