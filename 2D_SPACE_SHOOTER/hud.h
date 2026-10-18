/*
 * CMPE322 Operating Systems Project: Shooter
 * Author: Mehmet Saraçoğlu
 * Designed as course material for use in the CMPE322 Operating Systems lectures.
 */

#ifndef HUD_H
#define HUD_H

struct entity;

#define HUD_MAX_LINES 32
#define HUD_LINE_LEN 32

// Fills panel lines; returns count.
int hud_build(struct entity *player, char lines[][HUD_LINE_LEN], int max_lines);

#endif
