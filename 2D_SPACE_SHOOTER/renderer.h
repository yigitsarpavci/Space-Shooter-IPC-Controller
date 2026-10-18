/*
 * CMPE322 Operating Systems Project: Shooter
 * Author: Mehmet Saraçoğlu
 * Designed as course material for use in the CMPE322 Operating Systems lectures.
 */

#ifndef RENDERER_H
#define RENDERER_H

struct entity;

#define RENDER_CELL 5 // Pixels per slot side.
#define RENDER_MAX_VIEW_X 64 // Visible slots.
#define RENDER_MAX_VIEW_Y 64
#define RENDER_DEFAULT_FRAMERATE 30 // Frames per second.
#define RENDER_MAX_FRAMERATE 240

// View point on target.
enum render_anchor {
    ANCHOR_TOP_LEFT,
    ANCHOR_TOP_MID,
    ANCHOR_TOP_RIGHT,
    ANCHOR_MID_LEFT,
    ANCHOR_CENTER,
    ANCHOR_MID_RIGHT,
    ANCHOR_BOTTOM_LEFT,
    ANCHOR_BOTTOM_MID,
    ANCHOR_BOTTOM_RIGHT
};

enum render_style {
    STYLE_EMPTY,
    STYLE_PLAYER,
    STYLE_ENEMY,
    STYLE_BULLET,
    STYLE_PLANET,
    STYLE_STATION
};

// Space with STYLE_EMPTY: background.
struct render_pixel {
    char glyph;
    unsigned char style;
};

struct frame {
    int origin_x; // Top-left visible slot.
    int origin_y;
    int view_x; // Clamped to space size.
    int view_y;
    int pixel_x;
    int pixel_y;
    struct render_pixel pixels[RENDER_MAX_VIEW_Y * RENDER_CELL][RENDER_MAX_VIEW_X * RENDER_CELL];
};

// Visible slots; -1 if invalid.
int renderer_set_view(int slots_x, int slots_y);
void renderer_set_anchor(enum render_anchor anchor);

// View follows slot or entity.
void renderer_target_slot(int x, int y);
void renderer_target_entity(const struct entity *e);

// Target frame rate (fps).
int renderer_set_framerate(int fps);
int renderer_get_framerate(void);
int renderer_get_frame_period_ms(void);

// Renders and returns the frame.
const struct frame *renderer_build(void);

// Last frame, no rebuild.
const struct frame *renderer_get_frame(void);

#endif
