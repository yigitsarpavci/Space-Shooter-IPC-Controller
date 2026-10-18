/*
 * CMPE322 Operating Systems Project: Shooter
 * Author: Mehmet Saraçoğlu
 * Designed as course material for use in the CMPE322 Operating Systems lectures.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "control.h"

#define NO_MOVE -1

static struct control_time time_latch = { -1, 0, 0 };

static struct entity *player = NULL;
static uint64_t seed = CONTROL_DEFAULT_SEED;
static int quit_requested = 0;
static enum control_outcome outcome = OUTCOME_RUNNING;
static long outcome_tick = -1;

static int move_latch = NO_MOVE;
static int shoot_latch = 0, shield_latch = 0, gather_latch = 0;

static int shield_armed = 0;
static long shield_ready_tick = 0;

static int move_dir = NO_MOVE;
static long move_until = 0;

// Per-ship runtime state.
struct ship_state {
    int ID;
    long next_fire_tick;
    long next_tick;
    long decisions; // Seeds random draws.
    int started;
};

static struct ship_state states[MAX_TRACKED_SHIPS];
static int state_count = 0;

// Spawn station timer.
struct spawner {
    int ID;
    long next_tick;
};

static struct spawner spawners[MAX_SPAWNERS];
static int spawner_count = 0;
static int spawned = 0;
static double spawn_every = 6.0;
static int spawn_move = 3, spawn_bullet = 10;
static double spawn_rate = 1.5;

int control_init(struct entity *p, unsigned int s) {
    if (entity_get_ship(p) == NULL)
        return -1;

    player = p;
    seed = s;
    quit_requested = 0;
    outcome = OUTCOME_RUNNING;
    outcome_tick = -1;
    move_latch = NO_MOVE;
    shoot_latch = shield_latch = gather_latch = 0;
    shield_armed = 0;
    shield_ready_tick = 0;
    move_dir = NO_MOVE;
    move_until = 0;
    state_count = 0;
    spawner_count = 0;
    spawned = 0;
    control_take_time(NULL);
    return 0;
}

void control_apply(enum input_op op) {
    if (outcome != OUTCOME_RUNNING && op != OP_QUIT)
        return;

    switch (op) {
    case OP_UP:              move_latch = NORTH; break;
    case OP_DOWN:            move_latch = SOUTH; break;
    case OP_LEFT:            move_latch = WEST; break;
    case OP_RIGHT:           move_latch = EAST; break;
    case OP_SHIELD_ACTIVATE: shield_latch = 1; break;
    case OP_GATHER:          gather_latch = 1; break;
    case OP_SHOOT:           shoot_latch = 1; break;
    case OP_PAUSE:           time_latch.pause_request = time_latch.pause_request == 2 ? -1 : 2; break;
    case OP_QUIT:            quit_requested = 1; break;
    case OP_COUNT:           break;
    }
}

void control_set_spawn(double every_seconds, int move_speed, int bullet_speed, double fire_rate) {
    if (every_seconds > 0)
        spawn_every = every_seconds;
    spawn_move = move_speed;
    spawn_bullet = bullet_speed;
    spawn_rate = fire_rate;
}

int control_quit_requested(void) {
    return quit_requested;
}

enum control_outcome control_get_outcome(void) {
    return outcome;
}

long control_get_outcome_tick(void) {
    return outcome_tick;
}

static long seconds_to_ticks(double seconds);
static struct ship_state *state_of(struct entity *e);

static double ticks_left(long until) {
    long left = until - space_get_time();
    return left > 0 ? left * SPACE_TICK_SECONDS : 0;
}

void control_get_player_state(struct control_player_state *out) {
    if (out == NULL)
        return;

    struct ship_state *st = player != NULL ? state_of(player) : NULL;
    out->block_armed = shield_armed;
    out->block_ready_in = shield_armed ? 0 : ticks_left(shield_ready_tick);
    out->fire_ready_in = st != NULL ? ticks_left(st->next_fire_tick) : 0;
}

void control_pause(void) {
    time_latch.pause_request = 1;
}

void control_continue(void) {
    if (outcome == OUTCOME_RUNNING)
        time_latch.pause_request = 0;
}

int control_set_time_rate(double ticks_per_second) {
    if (ticks_per_second < 0)
        return -1;

    time_latch.has_rate = 1;
    time_latch.rate = ticks_per_second;
    return 0;
}

int control_take_time(struct control_time *out) {
    int latched = time_latch.pause_request != -1 || time_latch.has_rate;
    if (out != NULL)
        *out = time_latch;

    time_latch.pause_request = -1;
    time_latch.has_rate = 0;
    return latched;
}

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

static long seconds_to_ticks(double seconds) {
    return (long)(seconds / SPACE_TICK_SECONDS + 0.5);
}

static struct ship_state *state_of(struct entity *e) {
    int ID = entity_get_ID(e);
    for (int i = 0; i < state_count; i++)
        if (states[i].ID == ID)
            return &states[i];
    if (state_count == MAX_TRACKED_SHIPS)
        return NULL;

    struct ship_state *st = &states[state_count++];
    st->ID = ID;
    st->next_fire_tick = 0;
    st->next_tick = 0;
    st->decisions = 0;
    st->started = 0;
    return st;
}

static int is_active_ship(struct entity *e) {
    struct ship *s = entity_get_ship(e);
    return s != NULL && s->is_alive && space_is_placed(e);
}

static void remove_entity(struct entity *e) {
    space_unplace(e);
    entity_remove(e);
}

static void apply_hit(struct entity *target) {
    struct ship *s = entity_get_ship(target);
    if (s != NULL) {
        if (target == player && shield_armed) {
            shield_armed = 0;
            shield_ready_tick = space_get_time() + seconds_to_ticks(SHIELD_COOLDOWN_SECONDS);
            return;
        }
        if (s->shield_energy > 0)
            s->shield_energy--;
        if (s->shield_energy == 0) {
            s->is_alive = 0;
            space_unplace(target);
        }
        return;
    }
    if (entity_get_bullet(target) != NULL)
        remove_entity(target);
}

// Fires one bullet ahead.
static void fire(struct entity *shooter) {
    struct ship *s = entity_get_ship(shooter);
    struct ship_state *st = state_of(shooter);
    struct space_info in;
    long now = space_get_time();
    if (s == NULL || st == NULL || !s->firing_ability || now < st->next_fire_tick
        || space_get_info(shooter, &in) != 0)
        return;
    if (s->fire_rate > 0)
        st->next_fire_tick = now + seconds_to_ticks(1.0 / s->fire_rate);

    int dx, dy;
    orientation_delta(in.orientation, &dx, &dy);
    int x = in.pos_x + dx, y = in.pos_y + dy;
    if (!space_in_bounds(x, y))
        return;

    struct entity *occupant = space_get(x, y);
    if (occupant != NULL) {
        apply_hit(occupant);
        return;
    }

    struct entity *b = entity_create_bullet(s->bullet_speed);
    if (b != NULL && space_place(b, x, y, in.orientation) != 0)
        entity_remove(b);
}

// Recharges shield from station.
static void gather(struct entity *e) {
    struct ship *s = entity_get_ship(e);
    struct space_info in;
    if (s == NULL || s->shield_energy >= SHIP_MAX_SHIELD || space_get_info(e, &in) != 0)
        return;

    for (int o = NORTH; o <= WEST; o++) {
        int dx, dy;
        orientation_delta((enum orientation_values)o, &dx, &dy);
        struct entity *n = space_get(in.pos_x + dx, in.pos_y + dy);
        struct station *st = entity_get_station(n);
        if (st == NULL || st->station_type != RECHARGE || !st->is_active || st->remaining_energy <= 0)
            continue;

        int left = st->remaining_energy - 1;
        entity_edit_station(n, NULL, left, left > 0);
        s->shield_energy++;
        return;
    }
}

// Applies latched user movement.
static void commit_move(void) {
    long now = space_get_time();
    if (move_latch != NO_MOVE) {
        if (move_latch != move_dir || now >= move_until)
            space_step(player, (enum orientation_values)move_latch);
        move_dir = move_latch;
        move_until = now + seconds_to_ticks(MOVE_HOLD_SECONDS);
    }
    if (move_dir != NO_MOVE && now >= move_until)
        move_dir = NO_MOVE;
    space_set_speed(player, move_dir == NO_MOVE ? 0 : entity_get_ship(player)->move_speed);
}

static void commit_user(void) {
    if (player == NULL || !is_active_ship(player))
        return;

    commit_move();
    if (shield_latch && !shield_armed && space_get_time() >= shield_ready_tick)
        shield_armed = 1;
    if (gather_latch)
        gather(player);
    if (shoot_latch)
        fire(player);
}

// SplitMix64 hash.
static uint64_t mix(uint64_t z) {
    z += 0x9e3779b97f4a7c15ull;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    return z ^ (z >> 31);
}

// Deterministic value in [0, 1).
static double roll(int ID, long decision, uint64_t salt) {
    uint64_t r = mix(seed ^ mix((uint64_t)ID * 0x632be59bd9b4e019ull)
                          ^ mix((uint64_t)decision * 0x8cb92ba72f3d8dd7ull) ^ mix(salt));
    return (double)(r >> 11) / 9007199254740992.0;
}

static enum orientation_values toward(int d, enum orientation_values negative, enum orientation_values positive) {
    return d < 0 ? negative : positive;
}

// One enemy decision.
static void enemy_decide(struct entity *e, const struct ship_state *m) {
    struct space_info me, target;
    if (player == NULL || !is_active_ship(player) || space_get_info(e, &me) != 0
        || space_get_info(player, &target) != 0)
        return;

    int dx = target.pos_x - me.pos_x, dy = target.pos_y - me.pos_y;
    enum orientation_values along_x = toward(dx, WEST, EAST), along_y = toward(dy, NORTH, SOUTH);

    if (dx == 0 || dy == 0) {
        enum orientation_values facing = dx == 0 ? along_y : along_x;
        if (roll(m->ID, m->decisions, 1) < ENEMY_P_SHOOT_ALIGNED) {
            space_set_orientation(e, facing);
            fire(e);
        } else {
            space_step(e, facing);
        }
        return;
    }

    int x_first = abs(dx) >= abs(dy);
    if (roll(m->ID, m->decisions, 2) < ENEMY_P_OTHER_AXIS)
        x_first = !x_first;
    if (space_step(e, x_first ? along_x : along_y) != 0)
        space_step(e, x_first ? along_y : along_x);
}

// Enemy think time in ticks.
static long think_time(struct entity *e, const struct ship_state *m) {
    double span = ENEMY_THINK_MAX_SECONDS - ENEMY_THINK_MIN_SECONDS;
    long ticks = seconds_to_ticks(ENEMY_THINK_MIN_SECONDS + roll(m->ID, m->decisions, 4) * span);
    int speed = entity_get_ship(e)->move_speed;
    long step = speed > 0 ? seconds_to_ticks(1.0 / speed) : 0;
    return ticks > step ? ticks : step;
}

static void enemy_act(struct entity *e) {
    struct ship_state *m = state_of(e);
    long now = space_get_time();
    if (m == NULL)
        return;
    if (!m->started) {
        m->started = 1;
        m->next_tick = now + seconds_to_ticks(roll(m->ID, -1, 3) * ENEMY_THINK_MAX_SECONDS);
    }
    if (now < m->next_tick)
        return;

    enemy_decide(e, m);
    m->next_tick = now + think_time(e, m);
    m->decisions++;
}

#define STATION_STREAM 1000000 // Separates station random streams.

static struct spawner *spawner_of(struct entity *station) {
    int ID = entity_get_ID(station);
    for (int i = 0; i < spawner_count; i++)
        if (spawners[i].ID == ID)
            return &spawners[i];
    if (spawner_count == MAX_SPAWNERS)
        return NULL;

    struct spawner *sp = &spawners[spawner_count++];
    sp->ID = ID;
    sp->next_tick = space_get_time() + seconds_to_ticks((0.5 + roll(STATION_STREAM + ID, -1, 5)) * spawn_every);
    return sp;
}

// Spawns one adjacent enemy.
static int release_enemy(struct entity *station) {
    struct space_info at, target;
    if (space_get_info(station, &at) != 0)
        return 0;
    int has_target = player != NULL && space_get_info(player, &target) == 0;

    for (int o = NORTH; o <= WEST; o++) {
        int dx, dy;
        orientation_delta((enum orientation_values)o, &dx, &dy);
        int x = at.pos_x + dx, y = at.pos_y + dy;
        if (!space_in_bounds(x, y) || space_get(x, y) != NULL)
            continue;

        char name[32];
        snprintf(name, sizeof(name), "Spawn-%d", ++spawned);
        struct entity *e = entity_create_ship(name, ENEMY, 1, spawn_move, spawn_bullet, spawn_rate);
        enum orientation_values facing = SOUTH;
        if (has_target) {
            int tx = target.pos_x - x, ty = target.pos_y - y;
            facing = abs(tx) > abs(ty) ? toward(tx, WEST, EAST) : toward(ty, NORTH, SOUTH);
        }
        if (e != NULL && space_place(e, x, y, facing) == 0)
            return 1;
        if (e != NULL)
            entity_remove(e);
        return 0;
    }
    return 0;
}

static void run_spawners(void) {
    struct entity *stations[MAX_SPAWNERS];
    int count = 0;
    for (int i = 0; i < MAX_SPACE_OBJECTS && count < MAX_SPAWNERS; i++) {
        struct entity *e = space_object_at_index(i);
        struct station *st = entity_get_station(e);
        if (st != NULL && st->station_type == ENEMY_SPAWN && st->is_active && st->remaining_energy > 0)
            stations[count++] = e;
    }

    long now = space_get_time();
    for (int i = 0; i < count; i++) {
        struct spawner *sp = spawner_of(stations[i]);
        if (sp == NULL || now < sp->next_tick)
            continue;

        if (!release_enemy(stations[i])) {
            sp->next_tick = now + seconds_to_ticks(SPAWN_RETRY_SECONDS);
            continue;
        }
        struct station *st = entity_get_station(stations[i]);
        int left = st->remaining_energy - 1;
        entity_edit_station(stations[i], NULL, left, left > 0);
        sp->next_tick = now + seconds_to_ticks(spawn_every);
    }
}

void control_commit(void) {
    if (outcome != OUTCOME_RUNNING)
        return;

    commit_user();
    move_latch = NO_MOVE;
    shoot_latch = shield_latch = gather_latch = 0;

    struct entity *enemies[MAX_SPACE_OBJECTS];
    int count = 0;
    for (int i = 0; i < MAX_SPACE_OBJECTS; i++) {
        struct entity *e = space_object_at_index(i);
        struct ship *s = entity_get_ship(e);
        if (s != NULL && s->ship_type == ENEMY && s->is_alive)
            enemies[count++] = e;
    }
    for (int i = 0; i < count; i++)
        if (is_active_ship(enemies[i]))
            enemy_act(enemies[i]);
    run_spawners();
}

static int next_to_end_station(struct entity *e) {
    struct space_info in;
    if (space_get_info(e, &in) != 0)
        return 0;

    for (int o = NORTH; o <= WEST; o++) {
        int dx, dy;
        orientation_delta((enum orientation_values)o, &dx, &dy);
        struct station *st = entity_get_station(space_get(in.pos_x + dx, in.pos_y + dy));
        if (st != NULL && st->station_type == END)
            return 1;
    }
    return 0;
}

// Checks for win or loss.
static void update_outcome(void) {
    if (outcome != OUTCOME_RUNNING || player == NULL)
        return;

    if (!entity_get_ship(player)->is_alive)
        outcome = OUTCOME_LOST;
    else if (next_to_end_station(player))
        outcome = OUTCOME_WON;
    else
        return;

    outcome_tick = space_get_time();
    move_dir = NO_MOVE;
    space_set_speed(player, 0);
    time_latch.pause_request = 1;
}

// Resolves blocked bullet hits.
void control_resolve(void) {
    struct entity *bullets[MAX_SPACE_OBJECTS];
    int count = 0;
    for (int i = 0; i < MAX_SPACE_OBJECTS; i++) {
        struct entity *e = space_object_at_index(i);
        struct bullet *b = entity_get_bullet(e);
        if (b != NULL && b->speed > 0)
            bullets[count++] = e;
    }

    for (int i = 0; i < count; i++) {
        struct entity *e = bullets[i];
        struct space_info in;
        if (space_get_info(e, &in) != 0 || !in.blocked)
            continue;

        int dx, dy;
        orientation_delta(in.orientation, &dx, &dy);
        int x = in.pos_x + dx, y = in.pos_y + dy;
        if (!space_in_bounds(x, y)) {
            remove_entity(e);
            continue;
        }

        struct entity *occupant = space_get(x, y);
        if (occupant != NULL) {
            apply_hit(occupant);
            remove_entity(e);
        }
    }
    update_outcome();
}
