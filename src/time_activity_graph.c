#include "time_activity_graph.h"

#include <math.h>
#include <raylib.h>
#include <rlgl.h>

#include "clip.h"
#include "config.h"
#include "db.h"
#include "db_activity_map.h"
#include "fieldfusion.h"

#define GRAPH_WIDTH (420)
#define GRAPH_HEIGHT (300)

void time_activity_graph_draw(float x, float y) {
    size_t activity_count = db_activity_count();
    Time_Activity* activities = db_activity_get_array();
    float vert_peak = db_activity_get_max_focus_spent();

    size_t points_count = activity_count == 1 ? 2 : activity_count;
    bool has_fake_point = points_count > activity_count;
    Vector2 points[points_count];
    for (size_t i = 0; i < points_count; i += 1) {
        Vector2* point = &points[i];
        float x_perc = (float)i / (points_count - 1);
        point->x = x + GRAPH_WIDTH * x_perc;

        size_t activity_idx = has_fake_point ? 0 : i;
        float y_perc =
            activities[activity_idx].activity.focus / vert_peak;
        point->y = y + GRAPH_HEIGHT - (GRAPH_HEIGHT * y_perc);
    }

    clip_begin_custom_shape();
    for (size_t i = points_count; i-- > 0;) {
        if (i) {
            Vector2 pa = points[i - 1];
            Vector2 pb = points[i];
            rlBegin(RL_QUADS);
            rlVertex2f(pa.x, pa.y);
            rlVertex2f(pa.x, y + GRAPH_HEIGHT);
            rlVertex2f(pb.x, y + GRAPH_HEIGHT);
            rlVertex2f(pb.x, pb.y);
            rlEnd();
        }
    }
    clip_end_custom_shape();
    // DrawRectangle(x,y, GRAPH_WIDTH, GRAPH_HEIGHT*0.5f, PINK);
    DrawRectangleGradientV(x, y, GRAPH_WIDTH, GRAPH_HEIGHT * 0.5f,
                           GetColor(g_color[g_cfg.theme][COLOR_TEAL]),
                           BLANK);
    clip_end();

    size_t point_being_hovered = -1;

    Vector2 mouse = GetMousePosition();
    for (size_t i = points_count; i-- > 0;) {
        Color col = GetColor(g_color[g_cfg.theme][COLOR_TEAL]);
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
    bool hovering_over_fake_point =
        has_fake_point && point_being_hovered == 0;
    if (hovering && !hovering_over_fake_point) {
        if (has_fake_point) point_being_hovered -= 1;
        Time_Activity* hover_data = &activities[point_being_hovered];

        int focus_mins = hover_data->activity.focus / 60.f;
        float focus_secs = fmodf(hover_data->activity.focus, 60.f);
        char buf[64] = {0};
        snprintf(buf, 64, "focus: %dmins %.1fsecs", focus_mins,
                 focus_secs);
        float fw =
            ff_measure_utf8(buf, strlen(buf), g_cfg.btn_typo.font,
                            g_cfg.btn_typo.size, 0)
                .width;
        float fh = g_cfg.btn_typo.size;
        DrawRectangle(mouse.x, mouse.y, fw + g_cfg.inner_gap * 2,
                      fh + g_cfg.inner_gap * 2,
                      GetColor(g_color[g_cfg.theme][COLOR_SURFACE0]));
        rlDrawRenderBatchActive();
        ff_draw_str8(buf, strlen(buf), mouse.x + g_cfg.inner_gap,
                     mouse.y + g_cfg.btn_typo.size,
                     (float*)g_cfg.global_projection, g_cfg.btn_typo,
                     FF_FLAG_DEFAULT, 0);
    }

    DrawLine(x, y, x, y + GRAPH_HEIGHT, WHITE);
    DrawLine(x, y + GRAPH_HEIGHT, x + GRAPH_WIDTH, y + GRAPH_HEIGHT,
             WHITE);
}
