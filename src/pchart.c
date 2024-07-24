#include "pchart.h"

#include <math.h>
#include <raylib.h>
#include <rlgl.h>
#include <stddef.h>

#include "colors.h"
#include "config.h"
#include "db.h"
#include "db_cache.h"
#include "fieldfusion.h"
#include "indicator.h"
#include "motion.h"

#define PCHART_IN_RAD 30.0f
#define PCHART_OUT_RAD 40.0f
#define PCHART_RAD_STEP 5.0f
#define LINE_WIDTH_SCALE 1.25f
#define RING_SEGMENTS 32
#define TOTAL_RADIUS (PCHART_OUT_RAD + PCHART_IN_RAD + PCHART_RAD_STEP)

static_assert(PCHART_IN_RAD < PCHART_OUT_RAD);

typedef enum { SEGMENT_IDLE, SEGMENT_REST, SEGMENT_FOCUS, SEGMENT_COUNT } Segment_Kind;
typedef enum { DAYS, WEEKS, MONTHS, YEARS } SCALE_TYPE;

static_assert(SEGMENT_COUNT == 3);

typedef struct {
    float segment_perc[SEGMENT_COUNT];
    Motion motion_perc[SEGMENT_COUNT];
    char segment_weight[SEGMENT_COUNT];
} PChart;

static PChart g_pchart;
static FF_Style* g_style = &g_cfg.sstyle;

void pchart_init() {
    for (size_t i = 0; i < SEGMENT_COUNT; i += 1) {
        Motion* motion = &g_pchart.motion_perc[i];
        *motion = motion_new();
        motion->z = 1.0f;
    }
}

void pchart_terminate() {}

static float get_segment_perc(Segment_Kind seg) { return g_pchart.motion_perc[seg].position[0]; }

static void update_percs() {
    Activity* today = db_cache_get_todays_activity();
    float max = today->focus + today->rest + today->idle;
    if (!max) {
        g_pchart.segment_perc[SEGMENT_FOCUS] = .0f;
        g_pchart.segment_perc[SEGMENT_REST] = .0f;
        g_pchart.segment_perc[SEGMENT_IDLE] = 1.f;
    } else {
        g_pchart.segment_perc[SEGMENT_FOCUS] = today->focus / max;
        g_pchart.segment_perc[SEGMENT_REST] = today->rest / max;
        g_pchart.segment_perc[SEGMENT_IDLE] = today->idle / max;
    }
    float delta = GetFrameTime();
    for (size_t i = 0; i < SEGMENT_COUNT; i += 1) {
        float target[2] = {g_pchart.segment_perc[i], 0};
        motion_update(&g_pchart.motion_perc[i], target, delta);
    }
    // float a = (sinf(GetTime()) + 1) * .5f;
    // float b = (cosf(GetTime()) + 1) * .5f;
    // float c = (sinf(cosf(GetTime())) + 1) * .5f;
    // float d = a + b + c;
    // a /= d;
    // b /= d;
    // c /= d;
}

static void update_weights() {
    size_t segment_wheight_size = sizeof(g_pchart.segment_weight);
    memset(g_pchart.segment_weight, 0, segment_wheight_size);
    size_t big = 0;
    for (size_t i = 1; i < 3; i += 1) {
        if (g_pchart.segment_perc[i] > g_pchart.segment_perc[big]) big = i;
    }
    size_t small = 0;
    for (size_t i = 1; i < 3; i += 1) {
        if (g_pchart.segment_perc[i] < g_pchart.segment_perc[small]) small = i;
    }
    g_pchart.segment_weight[big] = 1;
    g_pchart.segment_weight[small] = -1;
}

static void draw_segment_info(float x, float y, Segment_Kind seg, float out_radius, float angle0, float angle1) {
    Color_Name col_name = seg == SEGMENT_FOCUS ? COLOR_BLUE : seg == SEGMENT_IDLE ? COLOR_SURFACE1 : COLOR_TEAL;
    Color col = GET_RCOLOR(col_name);

    Indicator indicator = indicator_create();
    Vector2 indicator_pos = {x, y};
    float orbit_rad = (PCHART_IN_RAD + out_radius) * .5f;
    float angle = angle0 + angle1 * .5f;
    indicator_pos.x = x + cosf(angle * DEG2RAD) * orbit_rad;
    indicator_pos.y = y + sinf(angle * DEG2RAD) * orbit_rad;

    char text[64] = {0};
    snprintf(text, 64, "%s: %.1f%%",
             seg == SEGMENT_FOCUS  ? "Diligence"
             : seg == SEGMENT_REST ? "Rest"
                                   : "Idle",
             g_pchart.segment_perc[seg] * 100);
    size_t text_len = strlen(text);
    float text_width = ff_measure_utf8(text, text_len, *g_style).width;
    float line_width = text_width * LINE_WIDTH_SCALE;
    indicator_calculate_pivoted(&indicator, indicator_pos.x, indicator_pos.y, x, y, line_width);

    float text_x = indicator.pivot & PIVOT_LEFT ? indicator.line_begin.x - line_width * .5f - text_width * .5f
                                                : indicator.line_begin.x + line_width * .5f - text_width * .5f;
    float text_y = indicator.line_end.y - g_style->typo.size - 4;
    ff_draw_str8(text, text_len, text_x, text_y, (float*)g_cfg.global_projection, *g_style);

    indicator_draw_faded(&indicator, col);
}

float pchart_max_width(void) {
    char max_str[] = "Diligence";
    float max_str_w = ff_measure_utf8(max_str, sizeof(max_str), *g_style).width;
    float max_line_w = max_str_w * LINE_WIDTH_SCALE;
    return (PCHART_IN_RAD + PCHART_OUT_RAD + INDICATOR_DIAG_WIDTH + max_line_w) * 2 + g_cfg.outer_gap2;
}

float pchart_max_height(void) { return TOTAL_RADIUS * 2 + g_cfg.outer_gap2; }

void pchart_draw(float x, float y) {
    update_percs();
    update_weights();

    float max_w = pchart_max_width();
    Rectangle bg = {.x = x, .y = y, .width = max_w, .height = pchart_max_height()};
    // DrawRectangleLinesEx(bg, 1., WHITE);
    // DRAW_BG(bg, g_cfg.bg_radius, COLOR_BASE);

    Color color[SEGMENT_COUNT];
    color[SEGMENT_FOCUS] = GET_RCOLOR(COLOR_BLUE);
    color[SEGMENT_IDLE] = GET_RCOLOR(COLOR_SURFACE1);
    color[SEGMENT_REST] = GET_RCOLOR(COLOR_TEAL);

    float angle_start = 0;
    Vector2 pos = {bg.x + bg.width * .5f, bg.y + bg.height * .5f};
    for (size_t i = 0; i < SEGMENT_COUNT; i += 1) {
        float segment_angle = get_segment_perc(i) * 360;
        float angle_end = segment_angle + angle_start;
        float radius_step = PCHART_RAD_STEP * g_pchart.segment_weight[i];
        float out_radius = PCHART_OUT_RAD + radius_step;
        DrawRing(pos, PCHART_IN_RAD, out_radius, angle_start, angle_end, RING_SEGMENTS, color[i]);
        if (g_pchart.segment_perc[i]) draw_segment_info(pos.x, pos.y, i, out_radius, angle_start, segment_angle);
        angle_start += segment_angle;
    }
}
