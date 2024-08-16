#include "heatmap.h"

#include <assert.h>
#include <fieldfusion.h>
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <string.h>
#include <time.h>

#include "clip.h"
#include "colors.h"
#include "config.h"
#include "db.h"
#include "db_cache.h"

#define ACTIVITY_MAP_CAP 1024
#define UPDATE_EVERY_SEC 1
#define MONTH_ROWS 6
#define MONTH_COLUMNS 7
#define HEATMAP_COLUMNS 3
#define SIZE (6.0f)
#define PAD (g_cfg.inner_gap * .25f + SIZE)
#define HEATMAP_MONTH_GAP (g_cfg.inner_gap)
#define COLOR_INDICATOR_HEIGHT 20.
#include "date.h"

typedef struct {
    int rec_idx;
    Activity* act;
} Hover_Data;

float g_auto_update_chronometer = 1.0f;
static FF_Style* g_style = &g_cfg.sstyle;

static bool is_today(int day, int month, int year) {
    time_t now;
    time(&now);
    struct tm* now_local = localtime(&now);
    return (now_local->tm_mday == day) && (now_local->tm_mon == month) && (now_local->tm_year == (year - 1900));
}

#define COLOR_LERP(A, B, P) ((Color){Lerp(A.r, B.r, P), Lerp(A.g, B.g, P), Lerp(A.b, B.b, P), Lerp(A.a, B.a, P)})
static void draw_month(float ix, float iy, int month, int year) {
    float x = ix;
    float y = iy;

    Week_Day first_day = get_first_month_weekday(year, month);
    int day_n = 0;
    float max_days = get_number_of_days(year, month) + 1;
    size_t recs_count = max_days;
    Vector2 points[recs_count];

    for (int i = 0; i < 6; i += 1) {
        for (int ii = 0; ii < 7; ii += 1, y += SIZE + PAD) {
            if (!i && ii < first_day) continue;
            day_n += 1;
            if (day_n >= max_days) break;

            points[day_n].x = x;
            points[day_n].y = y;
        }
        x += SIZE + PAD;
        y = iy;
    }

    Hover_Data hover_data[recs_count];
    size_t hover_data_len = 0;

    Task* future_tasks = db_cache_get_future_tasks_array();
    size_t future_tasks_len = db_cache_get_future_tasks_len();
    size_t future_tasks_idx = 0;

    for (size_t i = 0; i < recs_count; i += 1) {
        Activity* date_activity = db_cache_get_activity((Date){year, month, i});
        Color color = GET_RCOLOR(COLOR_SURFACE0);
        if (date_activity && date_activity->focus) {
            Color cold = GET_RCOLOR(COLOR_SKY);
            cold.a = 0x66;
            Color hot = GET_RCOLOR(COLOR_SKY);
            float lerp_value = Clamp(date_activity->focus / db_cache_get_max_diligence(), 0.0f, 1.0f);
            color = COLOR_LERP(cold, hot, lerp_value);
            hover_data[hover_data_len].act = date_activity;
            hover_data[hover_data_len].rec_idx = i;
            hover_data_len += 1;
        }

        Vector2 point = points[i];

        Date iteration_date = {year, month, i};

        long iteration_date_diff = day_diffenrence(get_current_date(), iteration_date);
        bool is_after_today = iteration_date_diff < 0;

        for (; is_after_today && future_tasks_idx < future_tasks_len;) {
            Date future_task_to = future_tasks[future_tasks_idx].date_range.to;
            Date future_task_from = future_tasks[future_tasks_idx].date_range.from;

            long to_diff = day_diffenrence(future_task_to, iteration_date);
            bool is_after_future_task_range = to_diff < 0;
            long from_diff = day_diffenrence(future_task_from, iteration_date);
            bool is_before_future_task_range = from_diff > 0;
            bool is_within_future_task_range = !is_after_future_task_range && !is_before_future_task_range;

            if (is_within_future_task_range) {
                color = GET_RCOLOR(COLOR_RED);
                break;
            } else if (to_diff < 0) {
                future_tasks_idx += 1;
            } else {
                break;
            }
        }

        DrawPoly(point, 8, 5., 0., color);
        if (is_today(i, month, year)) DrawCircleLinesV(point, 8., GET_RCOLOR(COLOR_FLAMINGO));
        color = GET_RCOLOR(COLOR_TEAL);
    }

    for (size_t i = 0; i < hover_data_len; i += 1) {
        Hover_Data* data = &hover_data[i];
        Vector2 mouse = GetMousePosition();
        Vector2 point = points[data->rec_idx];
        bool hovering = CheckCollisionPointCircle(mouse, point, SIZE);
        if (!hovering) continue;

        char thing[32] = {0};
        snprintf(thing, 32, "focus: %d mins, %.1f secs", (int)(data->act->focus / 60.f), fmodf(data->act->focus, 60.f));
        float fw = ff_measure_utf8(thing, strlen(thing), *g_style).width;
        float fh = g_style->typo.size;

        DrawRectangle(mouse.x, mouse.y, fw + g_cfg.inner_gap2, fh + g_cfg.inner_gap2, GET_RCOLOR(COLOR_SURFACE0));
        rlDrawRenderBatchActive();

        ff_draw_str8(thing, strlen(thing), mouse.x + g_cfg.inner_gap, mouse.y + g_style->typo.size,
                     (float*)g_cfg.global_projection, *g_style);
    }
}

static void draw_month_name(Month month, float x, float month_w, float y) {
    const char* name = get_month_name_short(month);
    size_t name_len = strlen(name);
    float nx = CENTER(x, month_w, ff_measure_utf8(name, name_len, *g_style).width);
    ff_draw_str8(name, strlen(name), nx, y, (float*)g_cfg.global_projection, *g_style);
}

inline static float month_max_width(void) { return (SIZE + PAD) * MONTH_ROWS; }
inline static float month_name_height(void) { return (g_style->typo.size + HEATMAP_MONTH_GAP); }
inline static float month_max_height(void) { return (SIZE + PAD) * MONTH_COLUMNS + month_name_height(); }
inline float heatmap_max_width(void) {
    return month_max_width() * HEATMAP_COLUMNS + HEATMAP_MONTH_GAP * (HEATMAP_COLUMNS - 1);
}

static void draw_month_row(float x, float y, size_t count, int end_year, int end_month) {
    float month_w = month_max_width();
    x += heatmap_max_width() - month_w;
    float name_height = month_name_height();
    for (size_t i = 0; i < count; i += 1) {
        if (end_month < 0) {
            end_year -= i;
            end_month = MONTH_DEC;
        }

        draw_month_name(end_month, x, month_w, y);
        draw_month(x, y + name_height, end_month, end_year);

        x -= month_w + HEATMAP_MONTH_GAP;
        end_month -= 1;
    }
}

inline float heatmap_max_height(void) { return month_max_height() + g_cfg.outer_gap2; }

void heatmap_draw(float x, float y) {
    float month_h = month_max_height();

    float max_w = heatmap_max_width();
    Rectangle bg = {.x = x,
                    .y = y,
                    .width = max_w + g_cfg.outer_gap2,
                    .height = month_h + COLOR_INDICATOR_HEIGHT + g_cfg.outer_gap2};
    // DrawRectangleLinesEx(bg, 1., WHITE);

    x = x + g_cfg.outer_gap;
    y = y + g_cfg.outer_gap;

    Date display_date = get_current_date();
    display_date.month -= 3;
    for (size_t i = 0; i < 3; i++) {
        draw_month_row(x, y, 3, display_date.year, display_date.month + 1);
        display_date.month += 3;
        y += month_h;
    }

    clip_begin_custom_shape();
    Rectangle lohi_rec = {0};
    lohi_rec.width = bg.width * .4;
    lohi_rec.height = COLOR_INDICATOR_HEIGHT;
    lohi_rec.x = bg.x + bg.width * .5 - lohi_rec.width * .5;
    lohi_rec.y = y + month_h;
    DrawRectangleRounded(lohi_rec, 1., 32, WHITE);
    clip_end_custom_shape();

    DrawRectangleGradientH(lohi_rec.x, lohi_rec.y, lohi_rec.width, lohi_rec.height, GET_RCOLOR(COLOR_BASE),
                           GET_RCOLOR(COLOR_TEAL));

    clip_end();

    float ltext_w = ff_measure_utf32(L"lo", 2, g_cfg.sstyle).width;
    float ltext_x = lohi_rec.x - g_cfg.inner_gap - ltext_w;
    float htext_x = lohi_rec.x + lohi_rec.width + g_cfg.inner_gap;
    float text_y = lohi_rec.y + lohi_rec.height * .5 - g_cfg.sstyle.typo.size * .5;
    ff_draw_str32(L"lo", 2, ltext_x, text_y, (float*)g_cfg.global_projection, g_cfg.sstyle);
    ff_draw_str32(L"hi", 2, htext_x, text_y, (float*)g_cfg.global_projection, g_cfg.sstyle);
}
