#include "heatmap.h"

#include <assert.h>
#include <fieldfusion.h>
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <string.h>
#include <time.h>

#include "colors.h"
#include "config.h"
#include "db.h"
#include "db_cache.h"

#define IS_LEAP_YEAR(YEAR) (!(YEAR % 4))
#define ACTIVITY_MAP_CAP 1024
#define UPDATE_EVERY_SEC 1
#define MONTH_ROWS 6
#define MONTH_COLUMNS 7
#define HEATMAP_COLUMNS 3
#define HEATMAP_MONTH_GAP (g_cfg.inner_gap * .5f)

typedef enum {
    MONTH_JAN,
    MONTH_FEB,
    MONTH_MAR,
    MONTH_APR,
    MONTH_MAY,
    MONTH_JUN,
    MONTH_JUL,
    MONTH_AUG,
    MONTH_SEP,
    MONTH_OCT,
    MONTH_NOV,
    MONTH_DEC,
} Month;

typedef enum {
    DAY_MON,
    DAY_TUE,
    DAY_WED,
    DAY_THU,
    DAY_FRI,
    DAY_SAT,
    DAY_SUN,
} Week_Day;

typedef struct {
    int rec_idx;
    Activity* act;
} Hover_Data;

float g_auto_update_chronometer = 1.0f;
static FF_Style* g_style = &g_cfg.sstyle;

static const char* get_month_name(Month month) {
    switch (month) {
        case MONTH_JAN: return "Jan";
        case MONTH_FEB: return "Feb";
        case MONTH_MAR: return "Mar";
        case MONTH_APR: return "Apr";
        case MONTH_MAY: return "May";
        case MONTH_JUN: return "Jun";
        case MONTH_JUL: return "Jul";
        case MONTH_AUG: return "Aug";
        case MONTH_SEP: return "Sep";
        case MONTH_OCT: return "Oct";
        case MONTH_NOV: return "Nov";
        case MONTH_DEC: return "Dec";
    }
    assert(0);
}

static inline Date get_current_date(void) {
    time_t now = time(0);
    struct tm* local_tm = localtime(&now);
    Date date = {.day = local_tm->tm_mday, .month = local_tm->tm_mon, .year = local_tm->tm_year + 1900};
    return date;
}

static int get_number_of_days(int year, Month month) {
    switch (month) {
        case MONTH_JAN: return 31;
        case MONTH_FEB: return IS_LEAP_YEAR(year) ? 29 : 28;
        case MONTH_MAR: return 31;
        case MONTH_APR: return 30;
        case MONTH_MAY: return 31;
        case MONTH_JUN: return 30;
        case MONTH_JUL: return 31;
        case MONTH_AUG: return 31;
        case MONTH_SEP: return 30;
        case MONTH_OCT: return 31;
        case MONTH_NOV: return 30;
        case MONTH_DEC: return 31;
    }
    assert(0);
}

static Week_Day get_week_day_from_time_str(char* str) {
    assert(strlen(str) >= 3);
    if (str[0] == 'M') return DAY_MON;
    if (str[0] == 'T' && str[1] == 'u') return DAY_TUE;
    if (str[0] == 'W') return DAY_WED;
    if (str[0] == 'T') return DAY_THU;
    if (str[0] == 'F') return DAY_FRI;
    if (str[0] == 'S' && str[1] == 'a') return DAY_SAT;
    if (str[0] == 'S') return DAY_SUN;
    assert(0);
}

static Week_Day get_first_month_weekday(int year, Month month) {
    struct tm tm = {0};
    tm.tm_year = year;
    tm.tm_mon = month;
    tm.tm_mday = 0;
    time_t time = mktime(&tm);
    char* time_str = ctime(&time);
    return get_week_day_from_time_str(time_str);
}

static bool is_today(int day, int month, int year) {
    time_t now;
    time(&now);
    struct tm* now_local = localtime(&now);
    return (now_local->tm_mday == day) && (now_local->tm_mon == month) && (now_local->tm_year == (year - 1900));
}

#define PAD (g_cfg.inner_gap * .25f)
#define SIZE (6.0f)

#define COLOR_LERP(A, B, P) ((Color){Lerp(A.r, B.r, P), Lerp(A.g, B.g, P), Lerp(A.b, B.b, P), Lerp(A.a, B.a, P)})
static void draw_month(float ix, float iy, int month, int year) {
    float x = ix;
    float y = iy;

    Week_Day first_day = get_first_month_weekday(year, month);
    int day_n = 0;
    float max_days = get_number_of_days(year, month) + 1;
    size_t recs_count = max_days;
    Rectangle recs[recs_count];

    for (int i = 0; i < 6; i += 1) {
        for (int ii = 0; ii < 7; ii += 1, y += SIZE + PAD) {
            if (!i && ii < first_day) continue;
            day_n += 1;
            if (day_n >= max_days) break;

            recs[day_n].x = x;
            recs[day_n].y = y;
            recs[day_n].width = SIZE;
            recs[day_n].height = SIZE;
        }
        x += SIZE + PAD;
        y = iy;
    }

    Hover_Data hover_data[recs_count];
    size_t hover_data_len = 0;

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
        // DrawCircle(recs[i].x, recs[i].y, 4.0f, color);
        DrawRectangleRec(recs[i], color);
    }

    for (size_t i = 0; i < hover_data_len; i += 1) {
        Hover_Data* data = &hover_data[i];
        Vector2 mouse = GetMousePosition();
        Rectangle rec = recs[data->rec_idx];
        bool hovering = CheckCollisionPointRec(mouse, rec);
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
    const char* name = get_month_name(month);
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
    Rectangle bg = {.x = x, .y = y, .width = max_w + g_cfg.outer_gap2, .height = month_h + g_cfg.outer_gap2};
    DRAW_BG(bg, g_cfg.bg_radius, COLOR_BASE);

    x = x + g_cfg.outer_gap;
    y = y + g_cfg.outer_gap;

    Date display_date = get_current_date();
    draw_month_row(x, y, 3, display_date.year, display_date.month);
}
