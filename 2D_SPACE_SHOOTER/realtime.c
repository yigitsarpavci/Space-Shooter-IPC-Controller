/*
 * CMPE322 Operating Systems Project: Shooter
 * Author: Mehmet Saraçoğlu
 * Designed as course material for use in the CMPE322 Operating Systems lectures.
 */

#include <stddef.h>
#include <time.h>
#include "realtime.h"
#include "control.h"
#include "space.h"
#include "renderer.h"

static double rate = REALTIME_DEFAULT_RATE;
static double start_time = 0;
static double last_time = 0;
static double pending = 0;
static int paused = 0;

static double now_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + ts.tv_nsec / 1e9;
}

// Converts elapsed time to ticks.
static void accumulate(void) {
    double now = now_seconds();
    double gap = now - last_time;
    if (!paused && gap <= REALTIME_MAX_GAP_SECONDS)
        pending += gap * rate;
    last_time = now;
}

static void set_rate(double ticks_per_second) {
    accumulate();
    rate = ticks_per_second;
}

static void set_paused(int value) {
    accumulate();
    paused = value;
}

static void apply_time_requests(void) {
    struct control_time request;
    if (!control_take_time(&request))
        return;

    if (request.pause_request == 2)
        set_paused(!paused);
    else if (request.pause_request != -1)
        set_paused(request.pause_request);
    if (request.has_rate && request.rate >= 0)
        set_rate(request.rate);
}

int realtime_init(double ticks_per_second) {
    if (ticks_per_second < 0)
        return -1;

    control_take_time(NULL);
    rate = ticks_per_second;
    start_time = last_time = now_seconds();
    pending = 0;
    paused = 0;
    renderer_build();
    return 0;
}

long realtime_update(void) {
    apply_time_requests();
    accumulate();

    long due = (long)pending;
    pending -= (double)due;

    for (long i = 0; i < due; i++) {
        control_commit();
        space_tick();
        control_resolve();
    }
    if (due > 0)
        renderer_build();
    return due;
}

double realtime_get_rate(void) {
    return rate;
}

int realtime_is_paused(void) {
    return paused;
}

void realtime_read(struct time_info *out) {
    if (out == NULL)
        return;

    accumulate();
    double fraction = pending - (double)(long)pending;

    out->space_time = space_get_time();
    out->space_seconds = space_get_seconds();
    out->rate = rate;
    out->real_elapsed = last_time - start_time;
    out->tick_progress = fraction;
    out->until_next_tick = !paused && rate > 0 ? (1.0 - fraction) / rate : -1;
    out->is_paused = paused;
}
