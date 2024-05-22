#include "pchart.h"

#include <assert.h>
#include <math.h>
#include <raylib.h>
#include <rlgl.h>
#include <stddef.h>

#include "clip.h"
#include "colors.h"
#include "config.h"
#include "fieldfusion.h"

#define PCHART_IN_RAD 30.0f
#define PCHART_OUT_RAD 40.0f
#define PCHART_RAD_STEP 5.0f
#define RING_SEGMENTS 32

static_assert(PCHART_IN_RAD < PCHART_OUT_RAD);

typedef enum {
    SEGMENT_FOCUS,
    SEGMENT_REST,
    SEGMENT_IDLE,
    SEGMENT_COUNT
} Segment_Kind;

static_assert(SEGMENT_COUNT == 3);

typedef struct {
    float segment_perc[SEGMENT_COUNT];
    char segment_weight[SEGMENT_COUNT];
} PChart;

static PChart g_pchart;

void pchart_init() {}

void pchart_terminate() {}

static void update_percs() {
    float a = (sinf(GetTime()) + 1) * .5f;
    float b = (cosf(GetTime()) + 1) * .5f;
    float c = (sinf(cosf(GetTime())) + 1) * .5f;
    float d = a + b + c;
    a /= d;
    b /= d;
    c /= d;
    g_pchart.segment_perc[SEGMENT_FOCUS] = a;
    g_pchart.segment_perc[SEGMENT_REST] = b;
    g_pchart.segment_perc[SEGMENT_IDLE] = c;
}

static void update_weights() {
    size_t segment_wheight_size = sizeof(g_pchart.segment_weight);
    memset(g_pchart.segment_weight, 0, segment_wheight_size);
    size_t big = 0;
    for (size_t i = 1; i < 3; i += 1) {
        if (g_pchart.segment_perc[i] > g_pchart.segment_perc[big])
            big = i;
    }
    size_t small = 0;
    for (size_t i = 1; i < 3; i += 1) {
        if (g_pchart.segment_perc[i] < g_pchart.segment_perc[small])
            small = i;
    }
    g_pchart.segment_weight[big] = 1;
    g_pchart.segment_weight[small] = -1;
}

static void draw_segment_info(Segment_Kind seg, float out_radius,
                              float angle0, float angle1) {
    Vector2 pos = {190, 190};
    Color_Name col_name = seg == SEGMENT_FOCUS  ? COLOR_BLUE
                          : seg == SEGMENT_IDLE ? COLOR_SURFACE1
                                                : COLOR_TEAL;
    // Color_Name col_name = COLOR_MAUVE;
    Color col = GetColor(g_color[g_cfg.theme][col_name]);

    Vector2 line0_beg = {pos.x, pos.y};
    float orbit_rad = (PCHART_IN_RAD + out_radius) * .5f;
    float angle = angle0 + angle1 * .5f;
    line0_beg.x = pos.x + cosf(angle * DEG2RAD) * orbit_rad;
    line0_beg.y = pos.y + sinf(angle * DEG2RAD) * orbit_rad;

    Vector2 line0_end = line0_beg;
    float diag_width = 30.f;
    line0_end.x += line0_beg.x > pos.x ? diag_width : -diag_width;
    line0_end.y += line0_beg.y > pos.y ? 15 : -15;

    Vector2 line1_beg = line0_end;
    Vector2 line1_end = line1_beg;

    char text[64] = {0};
    snprintf(text, 64, "%s: %.2f%%",
             seg == SEGMENT_FOCUS  ? "Focus"
             : seg == SEGMENT_REST ? "Rest"
                                   : "Idle",
             g_pchart.segment_perc[seg] * 100);
    size_t text_len = strlen(text);
    float text_width =
        ff_measure_utf8(text, text_len, g_cfg.btn_typo.font,
                        g_cfg.btn_typo.size, 0)
            .width;
    float line_width = text_width * 1.75f;
    line1_end.x += line0_beg.x > pos.x ? line_width : -line_width;
    float text_x =
        line1_beg.x > line1_end.x
            ? line1_beg.x - line_width * .5f - text_width * .5f
            : line1_beg.x + line_width * .5f - text_width * .5f;
    float text_y = line0_end.y - g_cfg.btn_typo.size - 4;
    ff_draw_str8(text, text_len, text_x, text_y,
                 (float*)g_cfg.global_projection, g_cfg.btn_typo, 0,
                 0);

    clip_begin_custom_shape();
    DrawLineEx(line0_beg, line0_end, 2.0f, WHITE);
    DrawLineEx(line1_beg, line1_end, 2.0f, WHITE);
    clip_end_custom_shape();
    Color col_base = BLANK;
    Color col1 = line0_beg.x > line1_end.x ? col_base : col;
    Color col2 = line0_beg.x > line1_end.x ? col : col_base;
    DrawRectangleGradientH(
        line0_beg.x > line1_end.x ? line1_end.x : line0_beg.x,
        line0_beg.y > line1_end.y ? line1_end.y : line0_beg.y,
        line_width+diag_width, 200, col1, col2);
    clip_end();
}

void pchart_draw() {
    update_percs();
    update_weights();

    Vector2 pos = {190, 190};
    Color color[SEGMENT_COUNT];
    color[SEGMENT_FOCUS] = GetColor(g_color[g_cfg.theme][COLOR_BLUE]);
    color[SEGMENT_IDLE] =
        GetColor(g_color[g_cfg.theme][COLOR_SURFACE1]);
    color[SEGMENT_REST] = GetColor(g_color[g_cfg.theme][COLOR_TEAL]);

    float angle_start = 0;
    for (size_t i = 0; i < SEGMENT_COUNT; i += 1) {
        float segment_angle = g_pchart.segment_perc[i] * 360;
        float angle_end = segment_angle + angle_start;
        float radius_step =
            PCHART_RAD_STEP * g_pchart.segment_weight[i];
        float out_radius = PCHART_OUT_RAD + radius_step;
        DrawRing(pos, PCHART_IN_RAD, out_radius, angle_start,
                 angle_end, RING_SEGMENTS, color[i]);
        draw_segment_info(i, out_radius, angle_start, segment_angle);
        angle_start += segment_angle;
    }
}
