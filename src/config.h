#pragma once
#include <fieldfusion.h>

#include "chrono.h"
#include "colors.h"

#define STICKY(key) (IsKeyPressed(key) || IsKeyPressedRepeat(key))
#define CENTER(AXIS_POS, OUT_DIM, IN_DIM) \
    (AXIS_POS + OUT_DIM * .5f - IN_DIM * .5f)
#define RADIUS_TO_ROUNDNESS(RAD, BIG_AXIS) (2 * RAD / BIG_AXIS)
#define BTN_ICON_SIZE 28.0f
#define MOUSE_WHEEL_Y_SCROLL_SCALE 16

typedef struct {
    unsigned window_width;
    unsigned window_height;
    char *window_name;
    Theme theme;
    float global_projection[4][4];

    float inner_gap;
    float outer_gap;

    FF_Typo btn_typo;
    FF_Typo c_clock_typo;

    U8 clock_focus_mins;
    U8 clock_focus_secs;
    U8 clock_rest_mins;
    U8 clock_rest_secs;

    float btn_pad_horz;
    float btn_pad_vert;
    float btn_expansion_width;
    float btn_expansion_height;
    float btn_roundness;
} Config;

extern Config g_cfg;

void config_init(void);
