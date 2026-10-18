/*
 * CMPE322 Operating Systems Project: Shooter
 * Author: Mehmet Saraçoğlu
 * Designed as course material for use in the CMPE322 Operating Systems lectures.
 */

#include <stddef.h>
#include <string.h>
#include "space.h"

// Placement record of an entity.
struct space_object {
    int in_use;
    struct entity *e;
    int pos_x;
    int pos_y;
    enum orientation_values orientation;
    int ship_speed; // Ship runtime speed.
    long progress; // Partial step progress.
    int blocked;
    int fp_x;
    int fp_y;
};

static int grid_x = GRID_MAX_X, grid_y = GRID_MAX_Y;
static long space_time = 0;
static int object_count = 0;

static struct entity *grid[GRID_MAX_Y][GRID_MAX_X];
static struct space_object objects[MAX_SPACE_OBJECTS];

static void orientation_delta(enum orientation_values o, int *dx, int *dy) {
    *dx = 0;
    *dy = 0;
    switch (o) {
    case NORTH: *dy = -1; break;
    case EAST:  *dx = 1;  break;
    case SOUTH: *dy = 1;  break;
    case WEST:  *dx = -1; break;
    }
}

// Returns the current speed.
static int object_speed(const struct space_object *o) {
    switch (o->e->entity_type) {
    case SHIP:   return o->ship_speed;
    case BULLET: return entity_get_bullet(o->e)->speed;
    default:     return 0;
    }
}

static struct space_object *object_of(const struct entity *e) {
    if (e == NULL)
        return NULL;
    for (int i = 0; i < MAX_SPACE_OBJECTS; i++)
        if (objects[i].in_use && objects[i].e == e)
            return &objects[i];
    return NULL;
}

// Computes covered cell bounds.
static void footprint_bounds(int pos_x, int pos_y, int fp_x, int fp_y, int *x0, int *y0, int *x1, int *y1) {
    *x0 = pos_x;
    *x1 = pos_x + fp_x - 1;
    *y0 = pos_y - fp_y + 1;
    *y1 = pos_y;
}

// Checks whether a footprint fits.
static int footprint_fits(const struct entity *e, int pos_x, int pos_y, int fp_x, int fp_y) {
    int x0, y0, x1, y1;
    footprint_bounds(pos_x, pos_y, fp_x, fp_y, &x0, &y0, &x1, &y1);

    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            if (!space_in_bounds(x, y))
                return 0;
            if (grid[y][x] != NULL && grid[y][x] != e)
                return 0;
        }
    }
    return 1;
}

static void footprint_write(struct space_object *o, struct entity *value) {
    int x0, y0, x1, y1;
    footprint_bounds(o->pos_x, o->pos_y, o->fp_x, o->fp_y, &x0, &y0, &x1, &y1);

    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++)
            if (value != NULL || grid[y][x] == o->e)
                grid[y][x] = value;
}

// Moves a placed object.
static int object_move(struct space_object *o, int pos_x, int pos_y) {
    if (!footprint_fits(o->e, pos_x, pos_y, o->fp_x, o->fp_y))
        return -1;

    footprint_write(o, NULL);
    o->pos_x = pos_x;
    o->pos_y = pos_y;
    footprint_write(o, o->e);
    return 0;
}

int space_init(int size_x, int size_y) {
    if (size_x <= 0 || size_x > GRID_MAX_X || size_y <= 0 || size_y > GRID_MAX_Y)
        return -1;

    grid_x = size_x;
    grid_y = size_y;
    space_time = 0;
    object_count = 0;
    memset(grid, 0, sizeof(grid));
    memset(objects, 0, sizeof(objects));
    return 0;
}

void space_get_size(int *size_x, int *size_y) {
    if (size_x != NULL)
        *size_x = grid_x;
    if (size_y != NULL)
        *size_y = grid_y;
}

int space_in_bounds(int x, int y) {
    return x >= 0 && x < grid_x && y >= 0 && y < grid_y;
}

long space_get_time(void) {
    return space_time;
}

double space_get_seconds(void) {
    return space_time * SPACE_TICK_SECONDS;
}

// Advances time one tick.
void space_tick(void) {
    space_time++;

    for (int i = 0; i < MAX_SPACE_OBJECTS; i++) {
        struct space_object *o = &objects[i];
        if (!o->in_use)
            continue;

        int dx, dy;
        orientation_delta(o->orientation, &dx, &dy);

        int speed = object_speed(o);
        if (speed <= 0) {
            o->progress = 0;
            continue;
        }

        o->progress += speed;
        while (o->progress >= SPACE_TICKS_PER_SECOND) {
            if (object_move(o, o->pos_x + dx, o->pos_y + dy) != 0) {
                o->progress = 0;
                o->blocked = 1;
                break;
            }
            o->progress -= SPACE_TICKS_PER_SECOND;
            o->blocked = 0;
        }
    }
}

int space_place(struct entity *e, int x, int y, enum orientation_values orientation) {
    if (e == NULL || object_of(e) != NULL)
        return -1;

    int fp_x = 1, fp_y = 1;
    struct planet *p = entity_get_planet(e);
    if (p != NULL) {
        fp_x = p->size_x;
        fp_y = p->size_y;
    }
    if (!footprint_fits(e, x, y, fp_x, fp_y))
        return -1;

    for (int i = 0; i < MAX_SPACE_OBJECTS; i++) {
        if (!objects[i].in_use) {
            struct space_object *o = &objects[i];
            o->in_use = 1;
            o->e = e;
            o->pos_x = x;
            o->pos_y = y;
            o->orientation = orientation;
            o->fp_x = fp_x;
            o->fp_y = fp_y;
            footprint_write(o, e);
            object_count++;
            return 0;
        }
    }
    return -1;
}

int space_unplace(struct entity *e) {
    struct space_object *o = object_of(e);
    if (o == NULL)
        return -1;

    footprint_write(o, NULL);
    memset(o, 0, sizeof(*o));
    object_count--;
    return 0;
}

int space_move_to(struct entity *e, int x, int y) {
    struct space_object *o = object_of(e);
    if (o == NULL)
        return -1;
    return object_move(o, x, y);
}

int space_step(struct entity *e, enum orientation_values direction) {
    struct space_object *o = object_of(e);
    if (o == NULL)
        return -1;

    int dx, dy;
    o->orientation = direction;
    orientation_delta(direction, &dx, &dy);
    return object_move(o, o->pos_x + dx, o->pos_y + dy);
}

int space_set_orientation(struct entity *e, enum orientation_values orientation) {
    struct space_object *o = object_of(e);
    if (o == NULL)
        return -1;

    o->orientation = orientation;
    return 0;
}

int space_get_speed(const struct entity *e) {
    struct space_object *o = object_of(e);
    if (o == NULL)
        return -1;
    return object_speed(o);
}

int space_set_speed(struct entity *e, int speed) {
    struct space_object *o = object_of(e);
    if (o == NULL || e->entity_type != SHIP || speed < 0 || speed > SPACE_MAX_SPEED)
        return -1;

    o->ship_speed = speed;
    return 0;
}

int space_is_placed(const struct entity *e) {
    return object_of(e) != NULL;
}

struct entity *space_get(int x, int y) {
    if (!space_in_bounds(x, y))
        return NULL;
    return grid[y][x];
}

int space_get_info(const struct entity *e, struct space_info *out) {
    struct space_object *o = object_of(e);
    if (o == NULL || out == NULL)
        return -1;

    out->entity = o->e;
    out->entity_type = o->e->entity_type;
    out->ID = entity_get_ID(o->e);
    out->pos_x = o->pos_x;
    out->pos_y = o->pos_y;
    out->orientation = o->orientation;
    out->speed = object_speed(o);
    out->blocked = o->blocked;
    out->size_x = o->fp_x;
    out->size_y = o->fp_y;
    return 0;
}

int space_get_info_at(int x, int y, struct space_info *out) {
    struct entity *e = space_get(x, y);
    if (e == NULL)
        return -1;
    return space_get_info(e, out);
}

int space_object_count(void) {
    return object_count;
}

struct entity *space_object_at_index(int i) {
    if (i < 0 || i >= MAX_SPACE_OBJECTS || !objects[i].in_use)
        return NULL;
    return objects[i].e;
}
