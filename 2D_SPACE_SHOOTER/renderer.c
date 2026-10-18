/*
 * CMPE322 Operating Systems Project: Shooter
 * Author: Mehmet Saraçoğlu
 * Designed as course material for use in the CMPE322 Operating Systems lectures.
 */

#include <stddef.h>
#include <string.h>
#include "renderer.h"
#include "space.h"

#define TRANSPARENT '-' // Unpainted art pixel.
#define STATION_MARK '@' // Replaced by station letter.

static const char *ship_art[RENDER_CELL] = {
    "-(-)-",
    "-0o0-",
    "08880",
    "-|||-",
    "-UWU-"
};

static const char *bullet_art[RENDER_CELL] = {
    "-----",
    "-----",
    "--.--",
    "-----",
    "-----"
};

static const char *station_art[RENDER_CELL] = {
    "--|--",
    "-[=]-",
    "|=@=|",
    "-[=]-",
    "--|--"
};

#define PLANET_GLYPH '0'

static struct frame frame;
static int view_x = 16, view_y = 8;
static enum render_anchor anchor = ANCHOR_CENTER;
static int target_x = 0, target_y = 0;
static const struct entity *target_entity = NULL;
static int framerate = RENDER_DEFAULT_FRAMERATE;

int renderer_set_view(int slots_x, int slots_y) {
    if (slots_x < 1 || slots_x > RENDER_MAX_VIEW_X || slots_y < 1 || slots_y > RENDER_MAX_VIEW_Y)
        return -1;
    view_x = slots_x;
    view_y = slots_y;
    return 0;
}

void renderer_set_anchor(enum render_anchor a) {
    anchor = a;
}

void renderer_target_slot(int x, int y) {
    target_entity = NULL;
    target_x = x;
    target_y = y;
}

void renderer_target_entity(const struct entity *e) {
    target_entity = e;
}

int renderer_set_framerate(int fps) {
    if (fps < 1 || fps > RENDER_MAX_FRAMERATE)
        return -1;
    framerate = fps;
    return 0;
}

int renderer_get_framerate(void) {
    return framerate;
}

int renderer_get_frame_period_ms(void) {
    return 1000 / framerate;
}

static char station_letter(enum station_type_values type) {
    switch (type) {
    case START:       return 'S';
    case END:         return 'E';
    case RECHARGE:    return 'R';
    case ENEMY_SPAWN: return 'X';
    }
    return '?';
}

// Source pixel after rotation.
static void rotate_source(int r, int c, int turns, int *src_r, int *src_c) {
    int u = 2 * c - (RENDER_CELL - 1);
    int v = 2 * r - (RENDER_CELL - 1);
    for (int i = 0; i < turns; i++) {
        int t = u;
        u = v;
        v = -t;
    }
    *src_c = (u + RENDER_CELL - 1) / 2;
    *src_r = (v + RENDER_CELL - 1) / 2;
}

static void put_pixel(int px, int py, char glyph, enum render_style style) {
    if (glyph == TRANSPARENT)
        return;
    frame.pixels[py][px].glyph = glyph;
    frame.pixels[py][px].style = (unsigned char)style;
}

// Draws a one-slot shape.
static void draw_art(const char **art, int px0, int py0, enum orientation_values o, enum render_style style,
                     char mark) {
    for (int r = 0; r < RENDER_CELL; r++) {
        for (int c = 0; c < RENDER_CELL; c++) {
            int sr, sc;
            rotate_source(r, c, (int)o, &sr, &sc);
            char g = art[sr][sc];
            if (g == STATION_MARK)
                g = mark;
            put_pixel(px0 + c, py0 + r, g, style);
        }
    }
}

// Planet circle part in slot.
static void draw_planet_part(const struct space_info *in, int x, int y, int px0, int py0) {
    long w = (long)in->size_x * RENDER_CELL, h = (long)in->size_y * RENDER_CELL;
    long d = w < h ? w : h;
    int left = in->pos_x, top = in->pos_y - in->size_y + 1;

    for (int r = 0; r < RENDER_CELL; r++) {
        for (int c = 0; c < RENDER_CELL; c++) {
            long p = (long)(x - left) * RENDER_CELL + c;
            long q = (long)(y - top) * RENDER_CELL + r;
            long dx = 2 * p + 1 - w, dy = 2 * q + 1 - h;
            if (dx * dx + dy * dy <= d * d)
                put_pixel(px0 + c, py0 + r, PLANET_GLYPH, STYLE_PLANET);
        }
    }
}

static void draw_slot(int x, int y, int px0, int py0) {
    struct space_info in;
    if (space_get_info_at(x, y, &in) != 0)
        return;

    switch (in.entity_type) {
    case SHIP: {
        struct ship *s = entity_get_ship(in.entity);
        draw_art(ship_art, px0, py0, in.orientation, s->ship_type == PLAYER ? STYLE_PLAYER : STYLE_ENEMY, 0);
        break;
    }
    case BULLET:
        draw_art(bullet_art, px0, py0, in.orientation, STYLE_BULLET, 0);
        break;
    case STATION:
        draw_art(station_art, px0, py0, in.orientation, STYLE_STATION,
                 station_letter(entity_get_station(in.entity)->station_type));
        break;
    case PLANET:
        draw_planet_part(&in, x, y, px0, py0);
        break;
    }
}

static int clamp(int value, int low, int high) {
    if (value > high)
        value = high;
    if (value < low)
        value = low;
    return value;
}

// Places view at anchor.
static void update_origin(int space_x, int space_y) {
    struct space_info in;
    if (target_entity != NULL) {
        if (space_get_info(target_entity, &in) != 0)
            return;
        target_x = in.pos_x;
        target_y = in.pos_y;
    }

    int col = anchor % 3, row = anchor / 3;
    int ax = col == 0 ? 0 : col == 1 ? frame.view_x / 2 : frame.view_x - 1;
    int ay = row == 0 ? 0 : row == 1 ? frame.view_y / 2 : frame.view_y - 1;

    frame.origin_x = clamp(target_x - ax, 0, space_x - frame.view_x);
    frame.origin_y = clamp(target_y - ay, 0, space_y - frame.view_y);
}

const struct frame *renderer_build(void) {
    int space_x, space_y;
    space_get_size(&space_x, &space_y);

    frame.view_x = view_x < space_x ? view_x : space_x;
    frame.view_y = view_y < space_y ? view_y : space_y;
    frame.pixel_x = frame.view_x * RENDER_CELL;
    frame.pixel_y = frame.view_y * RENDER_CELL;
    update_origin(space_x, space_y);

    for (int py = 0; py < frame.pixel_y; py++) {
        for (int px = 0; px < frame.pixel_x; px++) {
            frame.pixels[py][px].glyph = ' ';
            frame.pixels[py][px].style = STYLE_EMPTY;
        }
    }

    for (int y = 0; y < frame.view_y; y++)
        for (int x = 0; x < frame.view_x; x++)
            draw_slot(frame.origin_x + x, frame.origin_y + y, x * RENDER_CELL, y * RENDER_CELL);

    return &frame;
}

const struct frame *renderer_get_frame(void) {
    return &frame;
}
