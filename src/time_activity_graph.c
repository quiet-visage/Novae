#include "time_activity_graph.h"

#include <assert.h>
#include <math.h>
#include <raylib.h>
#include <rlgl.h>

#include "clip.h"
#include "colors.h"
#include "config.h"
#include "db.h"
#include "db_cache.h"
#include "fieldfusion.h"
#include "indicator.h"
#include "motion.h"
#include "time.h"

#define GRAPH_WIDTH (400)
#define GRAPH_HEIGHT (300)
#define GRAPH_SAMPLE_SIZE (60)
#define Y_AXIS_SCALE_COUNT 4
#define X_AXIS_SCALE_COUNT 3

static_assert(X_AXIS_SCALE_COUNT > 1);

static FF_Style* g_style = &g_cfg.estyle;
static Motion g_this_day_motion = {0};

void time_activity_graph_init(void) {
    g_this_day_motion = motion_new();
    g_this_day_motion.z = 1.0f;
}

static float get_y_scale(float peak) {
    int digits = (int)log(peak);
    if (digits < 1) digits = 1;
    return ceil(peak / digits) * digits;
}

static size_t get_days_ago(Date date) {
    struct tm tm = {0};
    tm.tm_mon = date.month;
    tm.tm_mday = date.day;
    tm.tm_year = date.year - 1900;
    time_t date_time = mktime(&tm);
    time_t now = time(0);
    struct tm* now_local_tm = localtime(&now);
    time_t now_local = mktime(now_local_tm);
    double secs_diff = difftime(now_local, date_time);
    return secs_diff / 86400;
}

static void draw_y_axis(float* y_axis_text_width, float y_axis_scale_max, float x, float y, float vert_peak,
                        float graph_height) {
    for (size_t i = Y_AXIS_SCALE_COUNT + 1; i-- > 0;) {
        float y_perc = (float)(i) / (float)Y_AXIS_SCALE_COUNT;
        float ty = y + graph_height - graph_height * y_perc;

        float text_indicator = y_axis_scale_max * y_perc;
        char text[32] = {0};
        size_t text_len = snprintf(text, 32, "%.0f", text_indicator);
        float text_w = ff_measure_utf8(text, text_len, *g_style).width;
        *y_axis_text_width = MAX(text_w, *y_axis_text_width);

        ff_draw_str8(text, text_len, x - (text_w - *y_axis_text_width), ty - g_style->typo.size,
                     (float*)g_cfg.global_projection, *g_style);
    }
}

static void draw_x_axis(float x, float y, float graph_width, Time_Activity* activity, size_t activity_count) {
    size_t sample_count = MIN(activity_count, GRAPH_SAMPLE_SIZE);
    if (sample_count < 2) sample_count = 2;
    int scale_count = X_AXIS_SCALE_COUNT;
    if (sample_count <= scale_count) scale_count = sample_count - 1;

    for (size_t i = scale_count + 1; i-- > 0;) {
        float perc = (float)i / scale_count;
        float tx = x + graph_width * perc;

        size_t sample_idx = (size_t)round(((float)activity_count - 1) * perc);
        assert(sample_idx < activity_count);
        Date sample_date = activity[sample_idx].date;
        size_t days_ago = get_days_ago(sample_date);

        char text[32] = {0};
        size_t text_len = snprintf(text, 32, days_ago ? "-%ld" : "%ld", days_ago);
        ff_draw_str8(text, text_len, tx, y + GRAPH_HEIGHT - g_style->typo.size, (float*)g_cfg.global_projection,
                     *g_style);
    }
}

static void draw_graph_gradient(size_t points_count, Vector2* points, float x, float y, float graph_width,
                                float graph_height) {
    clip_begin_custom_shape();
    for (size_t i = points_count; i-- > 0;) {
        if (i) {
            Vector2 pa = points[i - 1];
            Vector2 pb = points[i];
            rlBegin(RL_QUADS);
            rlVertex2f(pa.x, pa.y);
            rlVertex2f(pa.x, y + graph_height);
            rlVertex2f(pb.x, y + graph_height);
            rlVertex2f(pb.x, pb.y);
            rlEnd();
        }
    }
    clip_end_custom_shape();
    DrawRectangleGradientV(x, y, graph_width, graph_height * 0.5f, GET_RCOLOR(COLOR_TEAL), BLANK);
    clip_end();
}

static void get_points(float x, float y, float graph_w, float graph_h, float y_scale_max, size_t idx_offset,
                       Time_Activity* activities, size_t points_count, Vector2* points, bool has_fake_point,
                       bool graph_overflows) {
    for (size_t i = points_count; i-- > 0;) {
        Vector2* point = &points[i];
        bool is_today = i == points_count - 1;
        if (is_today) {
            float target[2] = {0};
            float x_perc = (float)i / (points_count - 1);
            target[0] = x + graph_w * x_perc;

            size_t activity_idx = has_fake_point ? 0 : i;
            if (graph_overflows) activity_idx += idx_offset;
            float y_perc = activities[activity_idx].activity.focus / y_scale_max;
            if (!y_scale_max) y_perc = 0;
            target[1] = y + graph_h - (graph_h * y_perc);
            if (!g_this_day_motion.position[0]) {
                g_this_day_motion.position[0] = target[0];
                g_this_day_motion.position[1] = target[1];
                g_this_day_motion.previous_input[0] = target[0];
                g_this_day_motion.previous_input[1] = target[1];
            }
            motion_update(&g_this_day_motion, target, GetFrameTime());
            point->x = g_this_day_motion.position[0];
            point->y = g_this_day_motion.position[1];
        } else {
            float x_perc = (float)i / (points_count - 1);
            point->x = x + graph_w * x_perc;

            size_t activity_idx = has_fake_point ? 0 : i;
            if (graph_overflows) activity_idx += idx_offset;
            float y_perc = activities[activity_idx].activity.focus / y_scale_max;
            point->y = y + graph_h - (graph_h * y_perc);
        }
    }
}

inline float time_activity_graph_max_width(void) { return GRAPH_WIDTH + g_cfg.outer_gap2; }
inline float time_activity_graph_max_height(void) { return GRAPH_HEIGHT + g_cfg.outer_gap2; }

void time_activity_graph_draw(float x, float y) {
    Rectangle bg = {
        .x = x, .y = y, .width = time_activity_graph_max_width(), .height = time_activity_graph_max_height()};
    // DrawRectangleLinesEx(bg, 1., WHITE);
    // DRAW_BG(bg, g_cfg.bg_radius, COLOR_BASE);
    x += g_cfg.outer_gap;
    y += g_cfg.outer_gap;

    Color outline_color = WHITE;
    outline_color.a = 0x40;

    size_t activity_count = db_cache_get_activity_count();
    Time_Activity* activities = db_cache_get_activity_array();
    float vert_peak = db_cache_get_max_diligence();

    float graph_h = GRAPH_HEIGHT - g_style->typo.size;

    float y_axis_text_w = 0;
    float y_scale_max = get_y_scale(vert_peak);
    draw_y_axis(&y_axis_text_w, y_scale_max, x, y, vert_peak, graph_h);

    float graph_w = GRAPH_WIDTH - y_axis_text_w;
    x += y_axis_text_w;

    draw_x_axis(x, y, graph_w, activities, activity_count);

    DrawLine(x, y, x, y + graph_h, outline_color);
    DrawLine(x, y + graph_h, x + graph_w, y + graph_h, outline_color);

    size_t points_count = activity_count == 1 ? 2 : activity_count;
    bool has_fake_point = points_count > activity_count;
    size_t activity_idx_offset = 0;
    bool graph_overflows = activity_count > GRAPH_SAMPLE_SIZE;
    if (graph_overflows) {
        points_count = GRAPH_SAMPLE_SIZE;
        activity_idx_offset = activity_count - GRAPH_SAMPLE_SIZE;
    }

    Vector2 points[points_count];
    get_points(x, y, graph_w, graph_h, y_scale_max, activity_idx_offset, activities, points_count, points,
               has_fake_point, graph_overflows);

    draw_graph_gradient(points_count, points, x, y, graph_w, graph_h);

    size_t point_being_hovered = -1;
    Vector2 mouse = GetMousePosition();
    for (size_t i = points_count; i-- > 0;) {
        Color col = GET_RCOLOR(COLOR_TEAL);
        if (i) {
            Vector2 pa = points[i - 1];
            Vector2 pb = points[i];
            DrawLineEx(pa, pb, 1.0f, col);
        }
        bool is_fake_point = has_fake_point && !i;
        if (!is_fake_point) DrawCircleV(points[i], 3.f, col);
        bool hover = CheckCollisionPointCircle(mouse, points[i], 8.f);
        if (hover) point_being_hovered = i;
    }
    rlDrawRenderBatchActive();
    bool hovering = point_being_hovered != (size_t)-1;
    bool hovering_over_fake_point = has_fake_point && point_being_hovered == 0;
    if (hovering && !hovering_over_fake_point) {
        if (has_fake_point) point_being_hovered -= 1;
        size_t hover_data_idx = graph_overflows ? point_being_hovered + activity_idx_offset : point_being_hovered;
        Time_Activity* hover_data = &activities[hover_data_idx];

        int focus_mins = hover_data->activity.focus / 60.f;
        int focus_hours = focus_mins / 60;
        focus_mins %= 60;
        float focus_secs = fmodf(hover_data->activity.focus, 60.f);
        char diligence_buf[64] = {0};

        if (focus_hours) {
            snprintf(diligence_buf, 64, "diligence: %dhours %dmins %.0fsecs", focus_hours, focus_mins, focus_secs);
        } else if (focus_mins) {
            snprintf(diligence_buf, 64, "diligence: %dmins %.0fsecs", focus_mins, focus_secs);
        } else {
            snprintf(diligence_buf, 64, "diligence: %.0fsecs", focus_secs);
        }

        char days_ago_buf[64] = {0};
        size_t days_ago = get_days_ago(hover_data->date);
        size_t days_ago_buf_len = !days_ago        ? snprintf(days_ago_buf, 64, "today")
                                  : days_ago == 1 ? snprintf(days_ago_buf, 64, "yesterday")
                                                   : snprintf(days_ago_buf, 64, "%ld days ago", days_ago);

        {
            float fw = ff_measure_utf8(diligence_buf, strlen(diligence_buf), *g_style).width;
            Indicator indicator = indicator_create();
            float line_width = fw * 1.25;
            indicator_calculate(&indicator, points[point_being_hovered].x, points[point_being_hovered].y, line_width);

            Color col_1 = GET_RCOLOR(COLOR_CRUST);
            Color col_0 = col_1;
            col_0.a = 0x0;
            Rectangle rec;
            rec.x = indicator.line_begin.x;
            rec.y = indicator.line_begin.y - 22.f;
            rec.width = line_width;
            rec.height = 22.0f;
            DrawRectangleGradientV(rec.x, rec.y, rec.width, rec.height, col_0, col_1);
            rlDrawRenderBatchActive();
            indicator_draw_faded(&indicator, GET_RCOLOR(COLOR_TEAL));
            ff_draw_str8(diligence_buf, strlen(diligence_buf), indicator.line_begin.x + line_width * .5f - fw * .5f,
                         indicator.line_end.y - g_style->typo.size - 4.0f, (float*)g_cfg.global_projection, *g_style);
            ff_draw_str8(days_ago_buf, days_ago_buf_len, indicator.line_begin.x + line_width * .5f - fw * .5f,
                         indicator.line_end.y - g_style->typo.size - 13.f - 4.0f, (float*)g_cfg.global_projection,
                         *g_style);
        }
    }
}
