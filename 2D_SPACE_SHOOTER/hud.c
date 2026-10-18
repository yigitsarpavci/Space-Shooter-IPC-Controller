/*
 * CMPE322 Operating Systems Project: Shooter
 * Author: Mehmet Saraçoğlu
 * Designed as course material for use in the CMPE322 Operating Systems lectures.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "hud.h"
#include "control.h"

struct hud_text {
    char (*lines)[HUD_LINE_LEN];
    int max;
    int count;
};

static void add(struct hud_text *t, const char *format, ...) {
    if (t->count >= t->max)
        return;

    va_list args;
    va_start(args, format);
    vsnprintf(t->lines[t->count++], HUD_LINE_LEN, format, args);
    va_end(args);
}

static const char *orientation_name(enum orientation_values o) {
    switch (o) {
    case NORTH: return "N";
    case EAST:  return "E";
    case SOUTH: return "S";
    case WEST:  return "W";
    }
    return "?";
}

static int distance(const struct space_info *a, const struct space_info *b) {
    return abs(a->pos_x - b->pos_x) + abs(a->pos_y - b->pos_y);
}

static void add_ship(struct hud_text *t, struct entity *player, const struct space_info *me, int placed) {
    const struct ship *s = entity_get_ship(player);
    struct control_player_state st;
    control_get_player_state(&st);

    char bar[SHIP_MAX_SHIELD + 1];
    for (int i = 0; i < SHIP_MAX_SHIELD; i++)
        bar[i] = i < s->shield_energy ? '#' : '-';
    bar[SHIP_MAX_SHIELD] = '\0';

    add(t, "!%s", s->name);
    add(t, "Shield   [%s] %d/%d", bar, s->shield_energy, SHIP_MAX_SHIELD);
    if (st.block_armed)
        add(t, "Block    ARMED");
    else if (st.block_ready_in > 0)
        add(t, "Block    recharging %.1f s", st.block_ready_in);
    else
        add(t, "Block    ready (J)");

    if (!s->firing_ability)
        add(t, "Weapon   none");
    else if (st.fire_ready_in > 0)
        add(t, "Weapon   reloading %.2f s", st.fire_ready_in);
    else
        add(t, "Weapon   ready (L)");
    add(t, "Bullets  %d d/s, %.1f/s", s->bullet_speed, s->fire_rate);

    if (!placed) {
        add(t, "Position destroyed");
        return;
    }
    add(t, "Position %d,%d facing %s", me->pos_x, me->pos_y, orientation_name(me->orientation));
    add(t, "Speed    %d d/s (max %d)", me->speed, s->move_speed);
}

static void add_enemies(struct hud_text *t, const struct space_info *me, int placed) {
    int total = 0, alive = 0, nearest = -1, lined_up = 0;
    for (int i = 0; i < MAX_ENTITIES; i++) {
        struct entity *e = entity_at_index(i);
        struct ship *s = entity_get_ship(e);
        if (s == NULL || s->ship_type != ENEMY)
            continue;

        total++;
        struct space_info in;
        if (!s->is_alive || space_get_info(e, &in) != 0)
            continue;

        alive++;
        if (!placed)
            continue;
        int d = distance(me, &in);
        if (nearest < 0 || d < nearest) {
            nearest = d;
            lined_up = in.pos_x == me->pos_x || in.pos_y == me->pos_y;
        }
    }

    add(t, "!Enemies");
    add(t, "Alive    %d of %d", alive, total);
    if (nearest >= 0)
        add(t, "Nearest  %d slots%s", nearest, lined_up ? ", LINED UP" : "");
}

static void add_stations(struct hud_text *t, const struct space_info *me, int placed) {
    int exit_distance = -1, dock_distance = -1, dock_energy = 0;
    const char *dock_name = NULL;

    for (int i = 0; i < MAX_ENTITIES; i++) {
        struct entity *e = entity_at_index(i);
        struct station *st = entity_get_station(e);
        struct space_info in;
        if (st == NULL || !placed || space_get_info(e, &in) != 0)
            continue;

        int d = distance(me, &in);
        if (st->station_type == END && (exit_distance < 0 || d < exit_distance))
            exit_distance = d;
        if (st->station_type == RECHARGE && st->is_active && st->remaining_energy > 0
            && (dock_distance < 0 || d < dock_distance)) {
            dock_distance = d;
            dock_energy = st->remaining_energy;
            dock_name = st->name;
        }
    }

    add(t, "!Stations");
    if (exit_distance >= 0)
        add(t, "Exit     %d slots away", exit_distance);
    if (dock_name == NULL)
        add(t, "Recharge none left");
    else if (dock_distance == 1)
        add(t, "Recharge %d left: K to gather", dock_energy);
    else
        add(t, "Recharge %s, %d left, %d away", dock_name, dock_energy, dock_distance);
}

int hud_build(struct entity *player, char lines[][HUD_LINE_LEN], int max_lines) {
    struct hud_text t = { lines, max_lines, 0 };
    if (entity_get_ship(player) == NULL)
        return 0;

    struct space_info me;
    int placed = space_get_info(player, &me) == 0;

    add(&t, "!GOAL: reach the exit (E)");
    switch (control_get_outcome()) {
    case OUTCOME_WON:  add(&t, "!>> YOU WIN <<"); break;
    case OUTCOME_LOST: add(&t, "!>> YOU LOSE <<"); break;
    default:           add(&t, "Don't get shot down."); break;
    }
    add(&t, "");
    add_ship(&t, player, &me, placed);
    add(&t, "");
    add_enemies(&t, &me, placed);
    add(&t, "");
    add_stations(&t, &me, placed);
    add(&t, "");
    add(&t, "!Keys");
    add(&t, "WASD move    L shoot");
    add(&t, "J block      K gather");
    add(&t, "P pause      Q quit");
    return t.count;
}
