/*
 * CMPE322 Operating Systems Project: Shooter
 * Author: Mehmet Saraçoğlu
 * Designed as course material for use in the CMPE322 Operating Systems lectures.
 */

#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>
#include "window.h"

#define STATUS_ROWS 1
#define MAX_LINE (RENDER_MAX_VIEW_X * RENDER_CELL * WINDOW_PIXEL_W)
#define FPS_WINDOW 0.5 // Averaging period in seconds.
#define BANNER_MAX 128
#define MIN_MAP_COLS 20

static int started = 0;
static chtype style_attr[STYLE_STATION + 1];
static chtype line[MAX_LINE + 1];
static char banner[BANNER_MAX] = "";
static char panel[WINDOW_PANEL_LINES][WINDOW_PANEL_W];
static int panel_count = 0;

// Map width beside panel.
static int map_cols(void) {
    int with_panel = panel_count > 0 && COLS >= WINDOW_PANEL_W + MIN_MAP_COLS;
    return with_panel ? COLS - WINDOW_PANEL_W : COLS;
}

static double fps_since = 0;
static int fps_frames = 0;
static double fps = 0;

static double now_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + ts.tv_nsec / 1e9;
}

// Colors, or fallback attributes.
static void setup_styles(void) {
    style_attr[STYLE_EMPTY] = A_NORMAL;
    style_attr[STYLE_PLAYER] = A_BOLD;
    style_attr[STYLE_ENEMY] = A_REVERSE;
    style_attr[STYLE_BULLET] = A_BOLD;
    style_attr[STYLE_PLANET] = A_DIM;
    style_attr[STYLE_STATION] = A_UNDERLINE;
    if (!has_colors() || start_color() == ERR)
        return;

    use_default_colors();
    init_pair(STYLE_PLAYER, COLOR_GREEN, -1);
    init_pair(STYLE_ENEMY, COLOR_RED, -1);
    init_pair(STYLE_BULLET, COLOR_YELLOW, -1);
    init_pair(STYLE_PLANET, COLOR_BLUE, -1);
    init_pair(STYLE_STATION, COLOR_CYAN, -1);
    style_attr[STYLE_PLAYER] = COLOR_PAIR(STYLE_PLAYER) | A_BOLD;
    style_attr[STYLE_ENEMY] = COLOR_PAIR(STYLE_ENEMY) | A_BOLD;
    style_attr[STYLE_BULLET] = COLOR_PAIR(STYLE_BULLET) | A_BOLD;
    style_attr[STYLE_PLANET] = COLOR_PAIR(STYLE_PLANET);
    style_attr[STYLE_STATION] = COLOR_PAIR(STYLE_STATION);
}

int window_init(void) {
    if (started)
        return 0;
    if (initscr() == NULL)
        return -1;

    curs_set(0);
    leaveok(stdscr, TRUE);
    setup_styles();
    fps_since = now_seconds();
    fps_frames = 0;
    fps = 0;
    started = 1;
    return 0;
}

void window_shutdown(void) {
    if (!started)
        return;
    curs_set(1);
    endwin();
    started = 0;
}

// Polls terminal size changes.
static void follow_resize(void) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0 && ws.ws_col > 0
        && (ws.ws_row != LINES || ws.ws_col != COLS)) {
        resizeterm(ws.ws_row, ws.ws_col);
        clear();
    }
}

void window_get_capacity(int *slots_x, int *slots_y) {
    if (started)
        follow_resize();
    int rows = started ? LINES - STATUS_ROWS : 0, cols = started ? map_cols() : 0;

    if (slots_x != NULL)
        *slots_x = cols / (RENDER_CELL * WINDOW_PIXEL_W);
    if (slots_y != NULL)
        *slots_y = rows / RENDER_CELL;
}

static void measure_fps(void) {
    double now = now_seconds();
    fps_frames++;
    if (now - fps_since >= FPS_WINDOW) {
        fps = fps_frames / (now - fps_since);
        fps_since = now;
        fps_frames = 0;
    }
}

// Crops or centers the frame.
static void draw_map(const struct frame *f) {
    int rows = LINES - STATUS_ROWS, cols = map_cols();
    if (rows <= 0 || cols <= 0)
        return;

    int frame_cols = f->pixel_x * WINDOW_PIXEL_W;
    int show_rows = f->pixel_y < rows ? f->pixel_y : rows;
    int show_cols = frame_cols < cols ? frame_cols : cols;
    if (show_cols > MAX_LINE)
        show_cols = MAX_LINE;

    int src_row = (f->pixel_y - show_rows) / 2, src_col = (frame_cols - show_cols) / 2;
    int dst_row = (rows - show_rows) / 2, dst_col = (cols - show_cols) / 2;

    for (int r = 0; r < show_rows; r++) {
        const struct render_pixel *row = f->pixels[src_row + r];
        for (int c = 0; c < show_cols; c++) {
            const struct render_pixel *p = &row[(src_col + c) / WINDOW_PIXEL_W];
            line[c] = (chtype)(unsigned char)p->glyph | style_attr[p->style <= STYLE_STATION ? p->style : 0];
        }
        mvaddchnstr(dst_row + r, dst_col, line, show_cols);
    }
}

static void draw_status(const struct frame *f, const struct time_info *t) {
    int row = LINES - 1;
    if (row < 0)
        return;

    move(row, 0);
    attron(A_REVERSE);
    if (t != NULL)
        printw(" t=%.2fs (tick %ld)  %.2f t/s  %s", t->space_seconds, t->space_time, t->rate,
               t->is_paused ? "PAUSED" : "running");
    if (t != NULL && t->until_next_tick >= 0)
        printw("  next %.2fs", t->until_next_tick);
    printw("  view %dx%d @(%d,%d)  %.0f fps ", f->view_x, f->view_y, f->origin_x, f->origin_y, fps);
    attroff(A_REVERSE);
    clrtoeol();
}

void window_set_panel(const char *const *lines, int count) {
    if (count > WINDOW_PANEL_LINES)
        count = WINDOW_PANEL_LINES;
    panel_count = lines == NULL || count < 0 ? 0 : count;
    for (int i = 0; i < panel_count; i++) {
        strncpy(panel[i], lines[i] != NULL ? lines[i] : "", WINDOW_PANEL_W - 1);
        panel[i][WINDOW_PANEL_W - 1] = '\0';
    }
}

static void draw_panel(void) {
    int left = map_cols(), rows = LINES - STATUS_ROWS;
    if (left == COLS)
        return;

    mvvline(0, left, ACS_VLINE, rows);
    for (int i = 0; i < panel_count && i < rows; i++) {
        const char *text = panel[i];
        int bold = text[0] == '!';
        if (bold) {
            text++;
            attron(A_BOLD);
        }
        mvprintw(i, left + 2, "%.*s", WINDOW_PANEL_W - 3, text);
        if (bold)
            attroff(A_BOLD);
    }
}

void window_set_banner(const char *text) {
    if (text == NULL) {
        banner[0] = '\0';
        return;
    }
    strncpy(banner, text, BANNER_MAX - 1);
    banner[BANNER_MAX - 1] = '\0';
}

// Three reverse-video rows.
static void draw_banner(void) {
    int len = (int)strlen(banner);
    int rows = LINES - STATUS_ROWS, cols = map_cols();
    if (len == 0 || rows < 3)
        return;

    int width = len + 4 < cols ? len + 4 : cols;
    int top = rows / 2 - 1, left = (cols - width) / 2;
    attron(A_REVERSE | A_BOLD);
    for (int r = 0; r < 3; r++)
        mvprintw(top + r, left, "%*s", width, "");
    mvprintw(top + 1, left + (width - len > 0 ? (width - len) / 2 : 0), "%.*s", width, banner);
    attroff(A_REVERSE | A_BOLD);
}

void window_draw(const struct frame *f, const struct time_info *t) {
    if (!started || f == NULL)
        return;

    follow_resize();
    measure_fps();
    erase();
    draw_map(f);
    draw_panel();
    draw_banner();
    draw_status(f, t);
    refresh();
}
