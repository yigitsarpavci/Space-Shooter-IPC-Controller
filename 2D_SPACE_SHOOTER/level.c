/*
 * CMPE322 Operating Systems Project: Shooter
 * Author: Mehmet Saraçoğlu
 * Designed as course material for use in the CMPE322 Operating Systems lectures.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "level.h"

#define MAX_WORDS 16
#define LINE_MAX_LEN 512

struct ship_pace {
    int move;
    int bullet;
    double rate;
};

struct reader {
    const char *path;
    int line;
    char *error;
    int error_len;
};

// Writes an error message.
static int fail(struct reader *r, const char *format, ...) {
    int n = r->line > 0 ? snprintf(r->error, (size_t)r->error_len, "%s:%d: ", r->path, r->line)
                        : snprintf(r->error, (size_t)r->error_len, "%s: ", r->path);
    if (n < 0 || n >= r->error_len)
        return -1;

    va_list args;
    va_start(args, format);
    vsnprintf(r->error + n, (size_t)(r->error_len - n), format, args);
    va_end(args);
    return -1;
}

static int to_int(const char *text, int *out) {
    char *end;
    long v = strtol(text, &end, 10);
    if (*text == '\0' || *end != '\0')
        return -1;
    *out = (int)v;
    return 0;
}

static int to_double(const char *text, double *out) {
    char *end;
    *out = strtod(text, &end);
    return *text == '\0' || *end != '\0' ? -1 : 0;
}

static int to_direction(const char *text, enum orientation_values *out) {
    if (strcmp(text, "N") == 0)      *out = NORTH;
    else if (strcmp(text, "E") == 0) *out = EAST;
    else if (strcmp(text, "S") == 0) *out = SOUTH;
    else if (strcmp(text, "W") == 0) *out = WEST;
    else return -1;
    return 0;
}

static int to_station_type(const char *text, enum station_type_values *out) {
    if (strcmp(text, "START") == 0)            *out = START;
    else if (strcmp(text, "END") == 0)         *out = END;
    else if (strcmp(text, "RECHARGE") == 0)    *out = RECHARGE;
    else if (strcmp(text, "ENEMY_SPAWN") == 0) *out = ENEMY_SPAWN;
    else return -1;
    return 0;
}

// Parses ship pace pairs.
static int read_pace(struct reader *r, char **words, int count, int from, struct ship_pace *pace) {
    if ((count - from) % 2 != 0)
        return fail(r, "expected key-value pairs: move <d/s> bullet <d/s> rate <shots/s>");

    for (int i = from; i < count; i += 2) {
        int ok;
        if (strcmp(words[i], "move") == 0)
            ok = to_int(words[i + 1], &pace->move) == 0;
        else if (strcmp(words[i], "bullet") == 0)
            ok = to_int(words[i + 1], &pace->bullet) == 0;
        else if (strcmp(words[i], "rate") == 0)
            ok = to_double(words[i + 1], &pace->rate) == 0;
        else
            return fail(r, "unknown key '%s' (expected move, bullet or rate)", words[i]);
        if (!ok)
            return fail(r, "bad value '%s' for %s", words[i + 1], words[i]);
    }
    return 0;
}

static int place_or_fail(struct reader *r, struct entity *e, int x, int y, enum orientation_values o,
                         const char *what, const char *name) {
    if (e == NULL)
        return fail(r, "%s %s has values out of range, or there are too many entities", what, name);
    if (space_place(e, x, y, o) != 0) {
        entity_remove(e);
        return fail(r, "%s %s doesn't fit at %d,%d (outside the space or overlapping)", what, name, x, y);
    }
    return 0;
}

// Checks slot passability.
static int passable(int x, int y) {
    struct entity *e = space_get(x, y);
    return space_in_bounds(x, y) && (e == NULL || entity_get_ship(e) != NULL || entity_get_bullet(e) != NULL);
}

static int next_to_end(int x, int y) {
    const int dx[4] = { 0, 1, 0, -1 }, dy[4] = { -1, 0, 1, 0 };
    for (int i = 0; i < 4; i++) {
        struct station *st = entity_get_station(space_get(x + dx[i], y + dy[i]));
        if (st != NULL && st->station_type == END)
            return 1;
    }
    return 0;
}

// Checks exit reachability.
static int exit_reachable(struct entity *player) {
    static unsigned char seen[GRID_MAX_Y][GRID_MAX_X];
    static int queue[GRID_MAX_X * GRID_MAX_Y];
    const int dx[4] = { 0, 1, 0, -1 }, dy[4] = { -1, 0, 1, 0 };
    struct space_info in;
    if (space_get_info(player, &in) != 0)
        return 0;

    memset(seen, 0, sizeof(seen));
    int head = 0, tail = 0;
    queue[tail++] = in.pos_y * GRID_MAX_X + in.pos_x;
    seen[in.pos_y][in.pos_x] = 1;

    while (head < tail) {
        int x = queue[head] % GRID_MAX_X, y = queue[head] / GRID_MAX_X;
        head++;
        if (next_to_end(x, y))
            return 1;
        for (int i = 0; i < 4; i++) {
            int nx = x + dx[i], ny = y + dy[i];
            if (passable(nx, ny) && !seen[ny][nx]) {
                seen[ny][nx] = 1;
                queue[tail++] = ny * GRID_MAX_X + nx;
            }
        }
    }
    return 0;
}

// Parses one level line.
static int read_line(struct reader *r, char **w, int n, struct level *lv, struct ship_pace *player_pace,
                     struct ship_pace *enemy_pace, int *has_space, int *has_end) {
    const char *kind = w[0];

    if (strcmp(kind, "seed") == 0) {
        int v;
        if (n != 2 || to_int(w[1], &v) != 0 || v < 0)
            return fail(r, "expected: seed <number>");
        lv->seed = (unsigned int)v;
        lv->has_seed = 1;
        return 0;
    }
    if (strcmp(kind, "space") == 0) {
        int sx, sy;
        if (*has_space)
            return fail(r, "space given twice");
        if (n != 3 || to_int(w[1], &sx) != 0 || to_int(w[2], &sy) != 0 || space_init(sx, sy) != 0)
            return fail(r, "expected: space <size_x> <size_y>, each 1..%d", GRID_MAX_X);
        *has_space = 1;
        return 0;
    }
    if (strcmp(kind, "player_ship") == 0)
        return read_pace(r, w, n, 1, player_pace);
    if (strcmp(kind, "enemy_ship") == 0)
        return read_pace(r, w, n, 1, enemy_pace);
    if (strcmp(kind, "spawn") == 0) {
        if (n != 3 || strcmp(w[1], "every") != 0 || to_double(w[2], &lv->spawn_every) != 0 || lv->spawn_every <= 0)
            return fail(r, "expected: spawn every <seconds>");
        return 0;
    }

    if (!*has_space)
        return fail(r, "'space' must come before any %s", kind);

    if (strcmp(kind, "player") == 0 || strcmp(kind, "enemy") == 0) {
        int is_player = kind[0] == 'p', x, y;
        enum orientation_values o;
        struct ship_pace pace = is_player ? *player_pace : *enemy_pace;
        if (n < 5 || to_int(w[2], &x) != 0 || to_int(w[3], &y) != 0 || to_direction(w[4], &o) != 0)
            return fail(r, "expected: %s <name> <x> <y> <N|E|S|W> [move ..] [bullet ..] [rate ..]", kind);
        if (is_player && lv->player != NULL)
            return fail(r, "only one player is allowed");
        if (read_pace(r, w, n, 5, &pace) != 0)
            return -1;

        struct entity *e = entity_create_ship(w[1], is_player ? PLAYER : ENEMY, 1, pace.move, pace.bullet, pace.rate);
        if (place_or_fail(r, e, x, y, o, kind, w[1]) != 0)
            return -1;
        if (is_player)
            lv->player = e;
        return 0;
    }
    if (strcmp(kind, "planet") == 0) {
        int size, x, y;
        if (n != 5 || to_int(w[2], &size) != 0 || to_int(w[3], &x) != 0 || to_int(w[4], &y) != 0)
            return fail(r, "expected: planet <name> <size> <x> <y>");
        return place_or_fail(r, entity_create_planet(w[1], size, size), x, y, NORTH, "planet", w[1]);
    }
    if (strcmp(kind, "station") == 0) {
        int x, y, energy = 0;
        enum station_type_values type;
        if ((n != 5 && n != 7) || to_station_type(w[2], &type) != 0 || to_int(w[3], &x) != 0
            || to_int(w[4], &y) != 0 || (n == 7 && (strcmp(w[5], "energy") != 0 || to_int(w[6], &energy) != 0)))
            return fail(r, "expected: station <name> <START|END|RECHARGE|ENEMY_SPAWN> <x> <y> [energy <n>]");
        if (type == END)
            *has_end = 1;
        return place_or_fail(r, entity_create_station(w[1], type, energy), x, y, NORTH, "station", w[1]);
    }
    return fail(r, "unknown entry '%s'", kind);
}

int level_load(const char *path, struct level *out, char *error, int error_len) {
    struct reader r = { path, 0, error, error_len };
    struct ship_pace player_pace = { 8, 20, 4.0 }, enemy_pace = { 3, 10, 1.5 };
    int has_space = 0, has_end = 0;

    memset(out, 0, sizeof(*out));
    out->spawn_every = 6.0;

    FILE *f = fopen(path, "r");
    if (f == NULL) {
        snprintf(error, (size_t)error_len, "%s: cannot open", path);
        return -1;
    }

    char buffer[LINE_MAX_LEN];
    int status = 0;
    while (status == 0 && fgets(buffer, sizeof(buffer), f) != NULL) {
        r.line++;
        char *comment = strchr(buffer, '#');
        if (comment != NULL)
            *comment = '\0';

        char *words[MAX_WORDS];
        int n = 0;
        for (char *t = strtok(buffer, " \t\r\n"); t != NULL; t = strtok(NULL, " \t\r\n")) {
            if (n == MAX_WORDS) {
                status = fail(&r, "too many words on one line");
                break;
            }
            words[n++] = t;
        }
        if (status == 0 && n > 0)
            status = read_line(&r, words, n, out, &player_pace, &enemy_pace, &has_space, &has_end);
    }
    fclose(f);
    if (status != 0)
        return -1;

    r.line = 0;
    if (!has_space)
        return fail(&r, "the level needs a 'space' line");
    if (out->player == NULL)
        return fail(&r, "the level needs a 'player' line");
    if (!has_end)
        return fail(&r, "the level needs at least one END station");
    if (!exit_reachable(out->player))
        return fail(&r, "planets and stations block every path from the player to the exit");

    out->enemy_move_speed = enemy_pace.move;
    out->enemy_bullet_speed = enemy_pace.bullet;
    out->enemy_fire_rate = enemy_pace.rate;
    return 0;
}
