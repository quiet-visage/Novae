#pragma once

#include <fieldfusion.h>
#include <raylib.h>
#include <rlgl.h>
#include <time.h>

#include "config.h"

static FF_Style *g_time_style = &g_cfg.bstyle;
static FF_Style *g_side_style = &g_cfg.mstyle;
static FF_Style *g_timer_finish_style = &g_cfg.sstyle;

float time_str_width(struct tm *now_local) {
    int hour = now_local->tm_hour;
    int min = now_local->tm_min;
    char time_str[8] = {0};
    size_t time_str_len = snprintf(time_str, 8, "%02d:%02d", hour, min);
    float timer_str_w = ff_measure_utf8(time_str, time_str_len, *g_time_style).width;
    return timer_str_w;
}

void time_view(struct tm *now_local, float x, float y) {
    int hour = now_local->tm_hour;
    int min = now_local->tm_min;

    char time_str[8] = {0};
    size_t time_str_len = snprintf(time_str, 8, "%02d:%02d", hour, min);
    ff_draw_str8(time_str, time_str_len, x, y, (float *)g_cfg.global_projection, *g_time_style);
}

float week_day_str_width(struct tm *now_local) {
    char week_day_str[32] = {0};
    size_t week_day_str_len = strftime(week_day_str, 32, "%A", now_local);
    float week_day_str_w = ff_measure_utf8(week_day_str, week_day_str_len, *g_side_style).width;
    return week_day_str_w;
}

void week_day_str_view(struct tm *now_local, float x, float y) {
    char week_day_str[32] = {0};
    size_t week_day_str_len = strftime(week_day_str, 32, "%A", now_local);
    ff_draw_str8(week_day_str, week_day_str_len, x, y, (float *)g_cfg.global_projection, *g_side_style);
}

float month_str_width(struct tm *now_local) {
    char month_str[32] = {0};
    size_t month_str_len = strftime(month_str, 32, "%B %d", now_local);
    float month_str_w = ff_measure_utf8(month_str, month_str_len, *g_side_style).width;
    return month_str_w;
}

void month_str_view(struct tm *now_local, float x, float y) {
    char month_str[32] = {0};
    size_t month_str_len = strftime(month_str, 32, "%B %d", now_local);
    ff_draw_str8(month_str, month_str_len, x, y, (float *)g_cfg.global_projection, *g_side_style);
}

float timer_finish_str_width(time_t now, struct tm *now_local, float timer_secs) {
    time_t future_time = now + (int)timer_secs;
    struct tm *timer_finish_tm = localtime(&future_time);

    char timer_finish_str[64] = {0};
    size_t timer_finish_str_len =
        strftime(timer_finish_str, 64, "Current timer will finish at: %H:%M", timer_finish_tm);
    float timer_finish_str_w = ff_measure_utf8(timer_finish_str, timer_finish_str_len, *g_timer_finish_style).width;
    return timer_finish_str_w;
}

void timer_finish_view(time_t now, struct tm *now_local, float timer_secs, float x, float y) {
    time_t future_time = now + (int)timer_secs;
    struct tm *timer_finish_tm = localtime(&future_time);

    char timer_finish_str[64] = {0};
    size_t timer_finish_str_len =
        strftime(timer_finish_str, 64, "Current timer will finish in: %H:%M", timer_finish_tm);
    ff_draw_str8(timer_finish_str, timer_finish_str_len, x, y, (float *)g_cfg.global_projection, g_cfg.sstyle);
}

float date_time_width(void) {
    time_t now = time(0);
    struct tm *now_local = localtime(&now);
    float max_side_items = MAX(month_str_width(now_local), week_day_str_width(now_local));
    return max_side_items + g_cfg.inner_gap + time_str_width(now_local);
}

void date_time_view_all(float x, float y, float timer_secs) {
    time_t now = time(0);
    struct tm *now_local = localtime(&now);

    float month_str_w = month_str_width(now_local);
    float week_day_str_w = week_day_str_width(now_local);
    // float timer_finish_w = timer_finish_str_width(now, now_local, timer_secs);
    float time_w = time_str_width(now_local);

    float total_w = month_str_w + week_day_str_w + time_w + g_cfg.inner_gap;

    float mon_x = x;
    float mon_y = y;
    month_str_view(now_local, mon_x, mon_y);

    float week_day_x = x;
    float week_day_y = mon_y + g_cfg.inner_gap + g_side_style->typo.size;
    week_day_str_view(now_local, week_day_x, week_day_y);

    float side_max_w = MAX(month_str_w, week_day_str_w);
    // DrawRectangleLines(x, y, side_max_w, 100, WHITE);
    float time_x = x + side_max_w + g_cfg.inner_gap;
    float time_y = y;
    time_view(now_local, time_x, time_y);

    float timer_finish_y = y + g_time_style->typo.size + g_cfg.inner_gap;
    timer_finish_view(now, now_local, timer_secs, x, timer_finish_y);
}
