/*
 * CMPE322 Operating Systems Project: Shooter
 * Author: Mehmet Saraçoğlu
 * Designed as course material for use in the CMPE322 Operating Systems lectures.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "space.h"
#include "renderer.h"
#include "realtime.h"
#include "window.h"
#include "control.h"
#include "input.h"
#include "hud.h"
#include "level.h"

#define DEFAULT_LEVEL "levels/level01.txt" // Used without a level argument.
#define STUDENT_ID_LEN 10 // Digits in a student ID.
#define FRAMERATE 60 // Frames per second.
#define MAX_OPS 64 // Input ops per poll.

static double now_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + ts.tv_nsec / 1e9;
}

// Fits view to terminal.
static void fit_view(void) {
    int slots_x, slots_y;
    window_get_capacity(&slots_x, &slots_y);
    if (slots_x > RENDER_MAX_VIEW_X)
        slots_x = RENDER_MAX_VIEW_X;
    if (slots_y > RENDER_MAX_VIEW_Y)
        slots_y = RENDER_MAX_VIEW_Y;
    renderer_set_view(slots_x, slots_y);
}

// Refreshes the side panel.
static void show_hud(struct entity *player) {
    static char text[HUD_MAX_LINES][HUD_LINE_LEN];
    const char *lines[HUD_MAX_LINES];
    int count = hud_build(player, text, HUD_MAX_LINES);
    for (int i = 0; i < count; i++)
        lines[i] = text[i];
    window_set_panel(lines, count);
}

// Shows the outcome banner.
static void show_outcome(void) {
    static int shown = 0;
    enum control_outcome o = control_get_outcome();
    if (shown || o == OUTCOME_RUNNING)
        return;

    char text[96];
    double seconds = control_get_outcome_tick() * SPACE_TICK_SECONDS;
    snprintf(text, sizeof(text), o == OUTCOME_WON ? "YOU WIN - reached the exit in %.1f s. Press Q to quit."
                                                  : "YOU LOSE - shot down after %.1f s. Press Q to quit.", seconds);
    window_set_banner(text);
    shown = 1;
}

// Student ID to seed.
static int seed_from_id(const char *id, unsigned int *seed) {
    unsigned long long value = 0;
    for (int i = 0; i < STUDENT_ID_LEN; i++) {
        if (id[i] < '0' || id[i] > '9')
            return -1;
        value = value * 10 + (unsigned long long)(id[i] - '0');
    }
    if (id[STUDENT_ID_LEN] != '\0')
        return -1;
    *seed = (unsigned int)(value ^ (value >> 32));
    return 0;
}

int main(int argc, char **argv) {
    unsigned int seed;
    if (argc < 2 || seed_from_id(argv[1], &seed) != 0) {
        fprintf(stderr, "usage: %s <student_id: %d digits> [level_file]\n", argv[0], STUDENT_ID_LEN);
        return 1;
    }
    const char *student_id = argv[1];
    const char *level_path = argc > 2 ? argv[2] : DEFAULT_LEVEL;
    double rate = argc > 3 ? atof(argv[3]) : REALTIME_DEFAULT_RATE;
    if (rate < 0) {
        fprintf(stderr, "usage: %s <student_id> [level_file] [ticks_per_second >= 0]\n", argv[0]);
        return 1;
    }

    struct level level;
    char error[256];
    if (level_load(level_path, &level, error, sizeof(error)) != 0) {
        fprintf(stderr, "%s\n", error);
        return 1;
    }
    struct entity *player = level.player;

    if (window_init() != 0) {
        fprintf(stderr, "cannot start the terminal screen\n");
        return 1;
    }
    if (input_init() != 0) {
        window_shutdown();
        fprintf(stderr, "cannot open the input source\n");
        return 1;
    }

    control_init(player, seed);
    control_set_spawn(level.spawn_every, level.enemy_move_speed, level.enemy_bullet_speed, level.enemy_fire_rate);
    show_hud(player);
    renderer_set_framerate(FRAMERATE);
    renderer_set_anchor(ANCHOR_CENTER);
    renderer_target_entity(player);
    fit_view();
    realtime_init(rate);

    double period = 1.0 / renderer_get_framerate();
    double next_frame = now_seconds();
    int input_ended = 0;

    while (!control_quit_requested() && !input_ended) {
        int wait_ms = (int)((next_frame - now_seconds()) * 1000);
        enum input_op ops[MAX_OPS];
        int n = input_poll(wait_ms > 0 ? wait_ms : 0, ops, MAX_OPS);
        if (n < 0)
            input_ended = 1;
        for (int i = 0; i < n; i++)
            control_apply(ops[i]);

        realtime_update();
        show_outcome();

        double now = now_seconds();
        if (now >= next_frame) {
            struct time_info t;
            show_hud(player);
            fit_view();
            realtime_read(&t);
            window_draw(renderer_get_frame(), &t);
            next_frame += period;
            if (next_frame < now)
                next_frame = now + period;
        }
    }

    input_shutdown();
    window_shutdown();

    const struct ship *s = entity_get_ship(player);
    const char *result = control_get_outcome() == OUTCOME_WON ? "Won: reached the exit"
                       : control_get_outcome() == OUTCOME_LOST ? "Lost: shot down" : "Quit";
    printf("%s at %.1f s (tick %ld, student %s, seed %u, %.2f ticks/s). Shield %d.\n", result,
           space_get_seconds(), space_get_time(), student_id, seed, rate, s->shield_energy);
    return 0;
}
