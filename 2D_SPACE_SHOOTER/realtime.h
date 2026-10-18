/*
 * CMPE322 Operating Systems Project: Shooter
 * Author: Mehmet Saraçoğlu
 * Designed as course material for use in the CMPE322 Operating Systems lectures.
 */

#ifndef REALTIME_H
#define REALTIME_H

#define REALTIME_DEFAULT_RATE 60.0 // Real-time tick rate.
#define REALTIME_MAX_GAP_SECONDS 0.25 // Longer gaps are skipped.

// Time status snapshot.
struct time_info {
    long space_time;
    double space_seconds;
    double rate;
    double real_elapsed;
    double tick_progress; // Fraction toward next tick.
    double until_next_tick; // -1 when stopped.
    int is_paused;
};

// Starts the real clock.
int realtime_init(double ticks_per_second);

// Runs all owed ticks.
long realtime_update(void);

double realtime_get_rate(void);
int realtime_is_paused(void);
void realtime_read(struct time_info *out);

#endif
