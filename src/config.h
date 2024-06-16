#pragma once
#include <fieldfusion.h>

#include "chrono.h"
#include "colors.h"
#define TAG_NAME_LIMIT 256

#define STICKY(key) (IsKeyPressed(key) || IsKeyPressedRepeat(key))
#define CENTER(AXIS_POS, OUT_DIM, IN_DIM) ((AXIS_POS) + (OUT_DIM) * .5f - (IN_DIM) * .5f)
#define RADIUS_TO_ROUNDNESS(RAD, BIG_AXIS) (2 * RAD / BIG_AXIS)
#define GET_COLOR(COL_NAME) (g_color[g_cfg.theme][COL_NAME])
#define GET_RCOLOR(COL_NAME) (g_rcolor[g_cfg.theme][COL_NAME])
#define MAX(X, Y) ((X) > (Y) ? (X) : (Y))
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#define DRAW_BG(REC, RAD, COL)                                             \
    do {                                                                   \
        DrawRectangleRounded(REC, RADIUS_TO_ROUNDNESS(RAD, REC.height),    \
                             g_cfg.rounded_rec_segments, GET_RCOLOR(COL)); \
        rlDrawRenderBatchActive();                                         \
    } while (0)
#define BTN_ICON_SIZE (g_cfg.sstyle.typo.size)
#define MOUSE_WHEEL_Y_SCROLL_SCALE 16

typedef struct {
    unsigned window_width;
    unsigned window_height;
    char *window_name;
    Theme theme;
    float global_projection[4][4];

    float inner_gap;
    float inner_gap2;
    float outer_gap;
    float outer_gap2;

    FF_Style bstyle;
    FF_Style mstyle;
    FF_Style sstyle;
    FF_Style estyle;

    U8 clock_focus_mins;
    U8 clock_focus_secs;
    U8 clock_rest_mins;
    U8 clock_rest_secs;

    int rounded_rec_segments;

    float bg_radius;
    float btn_pad_horz;
    float btn_pad_vert;
    float btn_expansion_width;
    float btn_expansion_height;
    float btn_roundness;
} Config;

extern Config g_cfg;

void config_init(void);
