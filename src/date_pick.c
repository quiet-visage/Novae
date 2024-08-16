#include "date_pick.h"

#include <time.h>

#include "alpha_inherit.h"
#include "c32_vec.h"
#include "colors.h"
#include "config.h"
#include "date.h"
#include "date_edit.h"
#include "fieldfusion.h"
#include "hint.h"
#include "icon.h"
#include "motion.h"
#include "raylib.h"
#include "scr_space.h"
#include "tag.h"

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
    m.calendar_btn = btn_create();
    m.next_btn = btn_create();
    m.prev_btn = btn_create();
    m.from_buf = c32_vec_create();
    m.from_edit = date_edit_create(&m.from_buf);
    m.to_buf = c32_vec_create();
    m.to_edit = date_edit_create(&m.to_buf);
    m.view_date = get_current_date();
    m.mo = motion_new();
    return m;
}

void date_pick_destroy(Date_Pick* m) {
    btn_destroy(&m->calendar_btn);
    btn_destroy(&m->next_btn);
    btn_destroy(&m->prev_btn);
    c32_vec_destroy(&m->from_buf);
    c32_vec_destroy(&m->to_buf);
}

static void compact_view(Date_Pick* m, Vector2 cpos, bool enabled) {
    float btn_radius = tag_height() * .5;  // TODO;
    long days_diff = day_diffenrence(m->to_edit.input_date, m->from_edit.input_date);
    Icon calendar_btn_icon = days_diff > 0 ? ICON_CALENDAR_RANGE : ICON_CALENDAR_EDIT;
    cpos.y += btn_radius;
    btn_draw_icon_only(&m->calendar_btn, calendar_btn_icon, cpos, btn_radius);

    if (enabled && m->state == DATE_PICK_STATE_COMPACT)
        hint_view("open tag selection menu", REC_FROM_CIRCLE(cpos.x, cpos.y, btn_radius));

    bool clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                   CheckCollisionPointCircle(GetMousePosition(), (Vector2){cpos.x, cpos.y}, btn_radius);  // TODO

    if (!clicked) return;

    m->state = DATE_PICK_STATE_OPEN;
    m->target_alpha = 0xff;
}

static void draw_begin_and_end_rings(Date_Pick* m, Vector2 pos, Date now, float cal_num_w, int day_n) {
    Vector2 c = {0};
    c.x = pos.x + cal_num_w * .5;
    c.y = pos.y + g_calendar_style->typo.size * .5;
    Date loop_date = now;
    loop_date.day = day_n;
    int loop_from_cmp = date_cmp(loop_date, m->from_edit.input_date);
    int loop_to_cmp = date_cmp(loop_date, m->to_edit.input_date);
    bool is_range_begin = !loop_from_cmp;

    Color col0 = GET_RCOLOR(COLOR_GREEN);
    Color col1 = GET_RCOLOR(COLOR_BLUE);
    col0.a = alpha_inherit_get_alpha();
    col1.a = alpha_inherit_get_alpha();

    if (is_range_begin) {
        DrawRing(c, g_calendar_style->typo.size, g_calendar_style->typo.size * 1.25, 0., 360., 32., col0);
    }
    // TODO maybe underline from beginning to end with gradient

    bool is_range_end = !loop_to_cmp;
    if (is_range_end) {
        DrawRing(c, g_calendar_style->typo.size, g_calendar_style->typo.size * 1.25, 0., 360., 32., col1);
    }
}

static inline float calendar_numbers_width(float cal_num_w) { return cal_num_w * 7 + g_cfg.inner_gap * 6; }

static inline float calendar_numbers_height(void) { return g_calendar_style->typo.size * 6 + g_cfg.inner_gap * 5; }

static inline float open_view_width(Date_Pick* m, float cal_num_w) {
    return calendar_numbers_width(cal_num_w) + g_cfg.outer_gap2;
}

static inline float open_view_height() {
    return g_calendar_style->typo.size * 2 + g_cfg.inner_gap2 * 3 + calendar_numbers_height() + date_edit_height() * 2 +
           g_cfg.outer_gap2;
}

static float month_title_view(Date_Pick* m, Vector2 pos, float calendar_nums_w) {
    const char* month_name = get_month_name_full(m->view_date.month);
    char title[32] = {0};
    size_t title_len = snprintf(title, 32, "%s %d", month_name, m->view_date.year);
    float title_w = ff_measure_utf8(title, title_len, *g_calendar_style).width;
    Vector2 title_pos = {.x = pos.x + calendar_nums_w * .5 - title_w * .5, .y = pos.y};
    FF_Style style = *g_calendar_style;
    style.typo.color &= ~0xff;
    style.typo.color |= alpha_inherit_get_alpha();
    ff_draw_str8(title, title_len, title_pos.x, title_pos.y, (float*)g_cfg.global_projection, style);
    return g_calendar_style->typo.size + g_cfg.inner_gap2;
}

static float week_days_view(Vector2 pos, float cal_num_w) {
    Vector2 pos_guide = {pos.x, pos.y};
    FF_Style style = *g_calendar_style;
    style.typo.color &= ~0xff;
    style.typo.color |= alpha_inherit_get_alpha();
    for (size_t i = 0; i < 7; ++i) {
        const char* wday_str = get_week_day_name_short(i);
        size_t wday_str_len = strlen(wday_str);
        ff_draw_str8(wday_str, wday_str_len, pos_guide.x, pos_guide.y, (float*)g_cfg.global_projection, style);
        pos_guide.x += cal_num_w + g_cfg.inner_gap;
    }
    return g_calendar_style->typo.size + g_cfg.inner_gap;
}

static float month_days_view(Date_Pick* m, Vector2 pos, float cal_num_w) {
    Week_Day first_day = get_first_month_weekday(m->view_date.year, m->view_date.month);
    Vector2 pos_guide = pos;
    int day_guide = 0;
    float max_days = get_number_of_days(m->view_date.year, m->view_date.month) + 1;
    FF_Style style = *g_calendar_style;
    style.typo.color &= ~0xff;
    style.typo.color |= alpha_inherit_get_alpha();

    for (int i = 0; i < 6; ++i) {
        for (int ii = 0; ii < 7; ++ii, pos_guide.x += cal_num_w + g_cfg.inner_gap) {
            if (!i && ii < first_day) continue;
            ++day_guide;
            if (day_guide >= max_days) continue;
            draw_begin_and_end_rings(m, pos_guide, m->view_date, cal_num_w, day_guide);

            char str[3] = {0};
            size_t str_len = snprintf(str, 3, "%2d", day_guide);
            ff_draw_str8(str, str_len, pos_guide.x, pos_guide.y, (float*)g_cfg.global_projection, style);
        }
        pos_guide.x = pos.x;
        pos_guide.y += g_calendar_style->typo.size + g_cfg.inner_gap;
    }

    return calendar_numbers_height() + g_cfg.inner_gap;
}

static float view_date_editors(Date_Pick* m, Vector2 pos, float bg_width) {
    Vector2 pos_guide = pos;

    const char* label0 = "from: ";
    const char* label1 = "to: ";
    size_t label0_len = strlen(label0);
    size_t label1_len = strlen(label1);

    float label_width = ff_measure_utf8(label0, label0_len, *g_calendar_style).width;
    float total_width = MAX(date_edit_width(&m->from_buf), date_edit_width(&m->to_buf)) + label_width;

    FF_Style style = *g_calendar_style;
    style.typo.color &= ~0xff;
    style.typo.color |= alpha_inherit_get_alpha();

    pos_guide.x = pos.x - g_cfg.outer_gap + bg_width * .5 - total_width * .5;
    ff_draw_str8(label0, label0_len, pos_guide.x, pos_guide.y, (float*)g_cfg.global_projection, style);
    pos_guide.x += label_width;
    date_edit_view(&m->from_edit, &m->from_buf, pos_guide,
                   m->state == DATE_PICK_STATE_OPEN && m->focus == DATE_PICK_FOCUS_EDIT_FROM);
    pos_guide.y += g_calendar_style->typo.size + g_cfg.inner_gap;
    date_edit_view(&m->to_edit, &m->to_buf, pos_guide,
                   m->state == DATE_PICK_STATE_OPEN && m->focus == DATE_PICK_FOCUS_EDIT_TO);
    pos_guide.x -= label_width;
    ff_draw_str8(label1, label1_len, pos_guide.x, pos_guide.y, (float*)g_cfg.global_projection, style);

    return pos_guide.y - pos.y;
}

static Date_Range* open_view(Date_Pick* m, bool enable_menu_input) {
    motion_update_x(&m->mo, m->target_alpha, GetFrameTime());
    if (m->mo.position[0] < 0.1) return 0;
    rlDrawRenderBatchActive();
    alpha_inherit_begin(m->mo.position[0]);

    float cal_num_w = ff_measure_utf32(L"30", 2, *g_calendar_style).width;
    Rectangle bg = {0};
    bg.width = open_view_width(m, cal_num_w);
    bg.x = scr_space_half_width() - bg.width * .5;
    bg.height = open_view_height();
    bg.y = scr_space_half_height() - bg.height * .5;
    Color bg_color = GET_RCOLOR(COLOR_BASE);
    bg_color.a = alpha_inherit_get_alpha();
    DRAW_BGR(bg, g_cfg.bg_radius, bg_color);

    {
        float radius = g_calendar_style->typo.size;
        Vector2 prev_pos = {bg.x + g_cfg.outer_gap + radius, bg.y + g_cfg.outer_gap + radius * .5};
        Vector2 next_pos = {bg.x + bg.width - radius * 2 - g_cfg.outer_gap + radius,
                            bg.y + g_cfg.outer_gap + radius * .5};
        bool next = btn_draw_icon_only(&m->next_btn, ICON_NEXT, next_pos, radius);
        bool prev = btn_draw_icon_only(&m->prev_btn, ICON_PREV, prev_pos, radius);
        if (next) date_incr_month(&m->view_date);
        if (prev) date_decr_month(&m->view_date);
    }

    Vector2 pos_guide = {bg.x + g_cfg.outer_gap, bg.y + g_cfg.outer_gap};
    float cal_nums_w = calendar_numbers_width(cal_num_w);

    pos_guide.y += month_title_view(m, pos_guide, cal_nums_w);
    pos_guide.y += week_days_view(pos_guide, cal_num_w);
    pos_guide.y += month_days_view(m, pos_guide, cal_num_w);
    view_date_editors(m, pos_guide, bg.width);

    if (IsKeyPressed(KEY_ESCAPE)) {
        m->state = DATE_PICK_STATE_COMPACT;
        m->target_alpha = 0;
        return (Date_Range*)-1;
    }

    if (IsKeyPressed(KEY_ENTER)) {
        if (m->focus == DATE_PICK_FOCUS_EDIT_FROM)
            m->focus = DATE_PICK_FOCUS_EDIT_TO;
        else {
            m->state = DATE_PICK_STATE_COMPACT;
            m->target_alpha = 0;
            m->date_range.from = m->from_edit.input_date;
            m->date_range.to = m->to_edit.input_date;
            return &m->date_range;
        }
    }

    alpha_inherit_end();
    return 0;
}

Date_Range* date_pick_view(Date_Pick* m, Vector2 cpos, bool enable_hint, bool enable_input) {
    compact_view(m, cpos, enable_hint);
    return open_view(m, enable_input);
}
