#pragma once

#include <fieldfusion.h>
#include <raylib.h>
#include <rlgl.h>
#include <time.h>

#include "colors.h"
#include "config.h"

static FF_Style *g_time_style = &g_cfg.bstyle;
static FF_Style *g_side_style = &g_cfg.mstyle;

float date_time_max_height(void) {
    return g_cfg.inner_gap + g_time_style->typo.size + g_cfg.outer_gap2;
}

void date_time_view(float x, float y, float timer_secs) {
    time_t now = time(0);
    struct tm *now_local = localtime(&now);
    int hour = now_local->tm_hour;
    int min = now_local->tm_min;
    // time_t future_time = now + (int)timer_secs;
    // struct tm* timer_finish_tm = localtime(&future_time);

    float cx = x + g_cfg.outer_gap;
    float cy = y + g_cfg.outer_gap;

    char time_str[8] = {0};
    size_t time_str_len = snprintf(time_str, 8, "%02d:%02d", hour, min);
    char month_str[32] = {0};
    size_t month_str_len = strftime(month_str, 32, "%B %d", now_local);
    char week_day_str[32] = {0};
    size_t week_day_str_len = strftime(week_day_str, 32, "%A", now_local);

    // char timer_finish_str[64] = {0};
    // size_t timer_finish_str_len = strftime(
    // timer_finish_str, 64, "Current timer will finish in: %H:%M",
    // timer_finish_tm);

    float timer_str_w = ff_measure_utf8(time_str, time_str_len, *g_time_style).width;
    float week_day_str_w = ff_measure_utf8(week_day_str, week_day_str_len, *g_side_style).width;
    float month_str_w = ff_measure_utf8(month_str, month_str_len, *g_side_style).width;
    // float timer_finish_str_w =
    //     ff_measure_utf8(timer_finish_str, timer_finish_str_len,
    //                     mtypo.font, stypo.size, 0)
    //         .width;

    Rectangle bg = {
        .x = x,
        .y = y,
        .width = timer_str_w + g_cfg.inner_gap + g_cfg.outer_gap2,
        .height = g_cfg.inner_gap + g_time_style->typo.size + g_cfg.outer_gap2,
    };

    DRAW_BG(bg, g_cfg.bg_radius, COLOR_BASE);

    Rectangle bgr1 = {
        .x = bg.x + bg.width + g_cfg.inner_gap,
        .y = y,
        .width = MAX(week_day_str_w, month_str_w) + g_cfg.outer_gap2,
        .height = bg.height,
    };
    DRAW_BG(bgr1, g_cfg.bg_radius, COLOR_BASE);

    ff_draw_str8(time_str, time_str_len, cx, cy, (float *)g_cfg.global_projection, *g_time_style);
    float rcx = bg.x + bg.width + g_cfg.inner_gap + g_cfg.outer_gap;
    float rcy = CENTER(bg.y, bg.height, g_side_style->typo.size * 2);
    ff_draw_str8(week_day_str, week_day_str_len, rcx, rcy, (float *)g_cfg.global_projection,
                 *g_side_style);
    ff_draw_str8(month_str, month_str_len, rcx, rcy + g_side_style->typo.size,
                 (float *)g_cfg.global_projection, *g_side_style);
    // float timer_finish_str_y = cy + btypo.size + g_cfg.inner_gap;
    // float total_width = timer_str_w + month_str_w + week_day_str_w;
    // float timer_finish_str_x =
    //     CENTER(x, bg.width, timer_finish_str_w);
    // ff_draw_str8(timer_finish_str, timer_finish_str_len,
    //              timer_finish_str_x, timer_finish_str_y,
    //              (float*)g_cfg.global_projection, stypo, 0, 0);
}
