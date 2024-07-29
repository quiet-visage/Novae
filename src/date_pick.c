#include "date_pick.h"

#include <time.h>

#include "c32_vec.h"
#include "colors.h"
#include "config.h"
#include "date.h"
#include "date_edit.h"
#include "editor.h"
#include "fieldfusion.h"
#include "hint.h"
#include "icon.h"
#include "raylib.h"

#define REC_FROM_CIRCLE(x, y, r) \
    (Rectangle) { x - r, y - r, r * 2, r * 2 }

static FF_Style* g_calendar_style = &g_cfg.sstyle;

static bool is_today(Date date) {
    time_t now;
    time(&now);
    struct tm* now_local = localtime(&now);
    return (now_local->tm_mday == date.day) && (now_local->tm_mon == date.month) &&
           (now_local->tm_year == (date.year - 1900));
}

Date_Pick date_pick_create(void) {
    Date_Pick m = {0};
    m.calendar_button = btn_create();
    m.from_buf = c32_vec_create();
    m.from_edit = date_edit_create(&m.from_buf);

    m.to_buf = c32_vec_create();
    m.to_edit = editor_create();
    m.to_edit.flags |= EDITOR_DIGIT_ONLY;
    return m;
}

void date_pick_destroy(Date_Pick* m) {
    btn_destroy(&m->calendar_button);
    c32_vec_destroy(&m->from_buf);
    c32_vec_destroy(&m->to_buf);
    editor_destroy(&m->to_edit);
}

static void compact_view(Date_Pick* m, float cx, float cy, bool enabled) {
    bool no_custom_date_selected = is_today(m->date_range.from) && is_today(m->date_range.to);
    float btn_radius = BTN_ICON_SIZE * 1.75;  // TODO;
    Icon calendar_btn_icon = no_custom_date_selected ? ICON_CALENDAR_EDIT : ICON_CALENDAR_RANGE;
    btn_draw_icon(&m->calendar_button, calendar_btn_icon, cx, cy);

    if (enabled && m->state == DATE_PICK_STATE_COMPACT)
        hint_view("open tag selection menu", REC_FROM_CIRCLE(cx, cy, btn_radius));
    bool clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                   CheckCollisionPointCircle(GetMousePosition(), (Vector2){cx, cy}, btn_radius);  // TODO
    if (!clicked) return;
    m->state = DATE_PICK_STATE_OPEN;
}

static Date_Range* open_view(Date_Pick* m, bool enable_menu_input) {
    if (m->state != DATE_PICK_STATE_OPEN) return 0;
    DrawRectangle(100, 100, 100, 100, BLACK);
    rlDrawRenderBatchActive();

    float cal_num_w = ff_measure_utf32(L"30", 2, *g_calendar_style).width;

    float ix = 100;
    float iy = 100;

    Date now = get_current_date();
    size_t month = 2;
    size_t year = now.year;
    Week_Day first_day = get_first_month_weekday(year, month);
    int day_n = 0;
    float max_days = get_number_of_days(year, month) + 1;

    float x = ix;
    float y = iy;
    for (int i = 0; i < 6; ++i) {
        for (int ii = 0; ii < 7; ++ii, x += cal_num_w + g_cfg.inner_gap) {
            if (!i && ii < first_day) continue;
            ++day_n;
            if (day_n >= max_days) continue;
            char str[3] = {0};
            size_t str_len = snprintf(str, 3, "%d", day_n);
            ff_draw_str8(str, str_len, x, y, (float*)g_cfg.global_projection, *g_calendar_style);
        }
        x = ix;
        y += g_calendar_style->typo.size + g_cfg.inner_gap;
    }

    y += 100;

    date_edit_view(&m->from_edit, &m->from_buf, x, y, 1);
    y += 200;
    // editor_view(&m->to_edit, &m->to_buf, x, y, 1);

    if (IsKeyPressed(KEY_ESCAPE)) {
        m->state = DATE_PICK_STATE_COMPACT;
        return (Date_Range*)-1;
    }
}

Date_Range* date_pick_view(Date_Pick* m, float cx, float cy, bool enable_hint, bool enable_menu_input) {
    compact_view(m, cx, cy, enable_hint);
    return open_view(m, enable_menu_input);
}
