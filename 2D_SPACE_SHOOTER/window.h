/*
 * CMPE322 Operating Systems Project: Shooter
 * Author: Mehmet Saraçoğlu
 * Designed as course material for use in the CMPE322 Operating Systems lectures.
 */

#ifndef WINDOW_H
#define WINDOW_H

#include "renderer.h"
#include "realtime.h"

#define WINDOW_PIXEL_W 2 // Characters per pixel.
#define WINDOW_PANEL_W 34 // Panel width with separator.
#define WINDOW_PANEL_LINES 32

// Terminal setup and restore.
int window_init(void);
void window_shutdown(void);

// Slots fitting the terminal.
void window_get_capacity(int *slots_x, int *slots_y);

// Side panel; '!' marks bold.
void window_set_panel(const char *const *lines, int count);

// Centered banner; NULL clears.
void window_set_banner(const char *text);

// Draws one frame.
void window_draw(const struct frame *f, const struct time_info *t);

#endif
