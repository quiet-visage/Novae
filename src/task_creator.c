#include "task_creator.h"

#include <assert.h>
#include <fieldfusion.h>
#include <linux/limits.h>
#include <math.h>
#include <raylib.h>
#include <rlgl.h>

#include "c32_vec.h"
#include "clip.h"
#include "colors.h"
#include "config.h"
#include "cursor.h"
#include "editor.h"
#include "icon.h"
#include "motion.h"
#include "task.h"
#include "text_view.h"

#define SIDE_BTN_HOVER_WIDTH_RATIO 0.14f
#define SIDE_BTN_WIDTH_RATIO 0.08f
#define SIDE_BTN_MIN_W 32.f
#define SIDE_BTN_ICON_SZ 14.f
#define SIDE_BTN_ICON_OFFSET 2.f

static FF_Style* g_style = &g_cfg.sstyle;

Task_Creator task_creator_create() {
    Task_Creator ret = {
        .name_buf = c32_vec_create(),
        .digit_buf = c32_vec_create(),
        .name_ed = editor_create(),
        .digit_ed = editor_create(),
        .name_curs = cursor_new(),
        .digit_curs = cursor_new(),
        .name_hor_motion = motion_new(),
        .add_btn_mo = motion_new(),
        .target_scroll = 0,
        .focus = TC_NAME,
    };
    ret.name_ed.limit = 64;
    ret.digit_ed.limit = 3;
    ret.name_hor_motion.f = 3.0f;
    ret.name_hor_motion.z = 1.0f;
    editor_unset_flag(&ret.name_ed, EDITOR_DRAW_BG);
    editor_set_flag(&ret.digit_ed, EDITOR_DIGIT_ONLY);
    editor_unset_flag(&ret.digit_ed, EDITOR_DRAW_BG);
    editor_unset_flag(&ret.digit_ed, EDITOR_HANDLE_HORIZONTAL_SCROLLING);
    editor_set_placeholder(&ret.digit_ed, "0");
    return ret;
}

void task_creator_destroy(Task_Creator* m) {
    c32_vec_destroy(&m->digit_buf);
    c32_vec_destroy(&m->name_buf);
    editor_destroy(&m->digit_ed);
    editor_destroy(&m->name_ed);
}

static void handle_digit_edit(Task_Creator* m, Rectangle left_ed_bg, bool enabled) {
    m->digit_ed.width = left_ed_bg.width;
    left_ed_bg.width = MAX(left_ed_bg.width, g_style->typo.size);
    float t_width = MAX(left_ed_bg.width, editor_get_text_width(&m->digit_ed, &m->digit_buf));
    clip_begin(left_ed_bg.x, left_ed_bg.y, left_ed_bg.width, left_ed_bg.height);
    left_ed_bg.x = CENTER(left_ed_bg.x, left_ed_bg.width, t_width);
    left_ed_bg.x -= g_cfg.inner_gap;
    editor_view(&m->digit_ed, &m->digit_buf, left_ed_bg.x, left_ed_bg.y, enabled);
    clip_end();
}

static void handle_name_edit(Task_Creator* m, Rectangle name_bg, bool enabled) {
    m->name_ed.width = name_bg.width;
    editor_set_flag(&m->name_ed, EDITOR_CENTER_INPUT);
    clip_begin(name_bg.x, name_bg.y, name_bg.width, name_bg.height);
    name_bg.x = CENTER(name_bg.x, name_bg.width, editor_get_text_width(&m->name_ed, &m->name_buf));
    editor_view(&m->name_ed, &m->name_buf, name_bg.x, name_bg.y, enabled);
    clip_end();
}

static U8 get_left(Task_Creator* m) {
    U8 ret = 0;
    U8 dec = m->digit_buf.len - 1;
    for (size_t i = 0; i < m->digit_buf.len; i += 1) {
        U8 digit = m->digit_buf.data[i] - '0';
        ret += digit * pow(10, dec);
        dec -= 1;
    }
    return ret;
}

inline float task_creator_height(void) { return task_height(); }

static void clear(Task_Creator* m) {
    m->name_buf.len = 0;
    m->digit_buf.len = 0;
    editor_clear(&m->digit_ed);
    editor_clear(&m->name_ed);
}

static bool draw_add_btn(Motion* motion, Rectangle bg, Rectangle bounds) {
    bool result = 0;
    float target_w = motion->position[0];
    float btn_h = bg.height;
    float icon_target_alpha = 0;

    Vector2 mouse = GetMousePosition();
    Rectangle btn_bounds = {.x = bg.x, .y = bg.y, .width = target_w, .height = btn_h};
    bool hover = CheckCollisionPointRec(mouse, btn_bounds);
    if (hover) {
        target_w = MAX(bg.width * SIDE_BTN_HOVER_WIDTH_RATIO, SIDE_BTN_MIN_W);
        icon_target_alpha = 0xff;
        result = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    } else {
        target_w = MAX(bg.width * SIDE_BTN_WIDTH_RATIO, SIDE_BTN_MIN_W);
        icon_target_alpha = 0xb1;
    }

    float motion_target[2] = {target_w, icon_target_alpha};
    motion_update(motion, motion_target, GetFrameTime());

    float btn_w = motion->position[0];
    Color col0 = GET_RCOLOR(COLOR_BLUE);
    col0.a = 0x3a;
    Color col1 = col0;
    col1.a = 0;
    DrawRectangleGradientH(bg.x, bg.y, btn_w, btn_h, col0, col1);

    Texture icon = icon_get(ICON_ADD_CIRCLE);
    float icon_w = SIDE_BTN_ICON_SZ;
    float icon_h = SIDE_BTN_ICON_SZ;
    Rectangle icon_src = {0, 0, icon.width, icon.height};
    Rectangle icon_dst = {bg.x + SIDE_BTN_ICON_OFFSET + btn_w * .15f, CENTER(bg.y, bg.height, icon_h), icon_w, icon_h};
    Color icon_col = GET_RCOLOR(COLOR_TEXT);
    icon_col.a = motion->position[1];
    DrawTexturePro(icon, icon_src, icon_dst, (Vector2){0}, 0, icon_col);

    return result;
}

Task_Creator_Ret task_creator_draw(Task_Creator* m, Task* out, float x, float y, float max_width, bool enable_input) {
    Rectangle bg;
    Task_Creator_Ret result = {0};
    bg.x = x;
    bg.y = y;
    bg.width = max_width;
    bg.height = task_creator_height();
    clip_begin_rounded(bg.x, bg.y, bg.width, bg.height, RADIUS_TO_ROUNDNESS(g_cfg.bg_radius, bg.height));
    DrawRectangleRec(bg, GET_RCOLOR(COLOR_BASE));
    rlDrawRenderBatchActive();
    bool clicked_add = draw_add_btn(&m->add_btn_mo, bg, bg);
    clip_end();

    Rectangle name_rec;
    name_rec.width = bg.width * .55f;
    name_rec.x = CENTER(bg.x, bg.width, name_rec.width);
    name_rec.height = g_style->typo.size * 1.5f;
    name_rec.y = bg.y + g_cfg.outer_gap;
    // DRAW_BG(name_rec, 100.f, COLOR_SURFACE0);

    Rectangle bar_rec = {0};
    bar_rec.width = bg.width * .6f;
    bar_rec.height = 5.f;
    bar_rec.x = CENTER(bg.x, bg.width, bar_rec.width);
    bar_rec.y = name_rec.y + g_cfg.inner_gap2;
    DrawRectangleRounded(bar_rec, 1.f, 6, GET_RCOLOR(COLOR_SURFACE0));

    result.tag_sel_x = bar_rec.x;
    result.tag_sel_y = bar_rec.y + bar_rec.height + g_cfg.inner_gap;

    // float tag_x = bar_rec.x;
    float tag_y = bar_rec.y + bar_rec.height + g_cfg.inner_gap;

    Rectangle left_ed_bg;
    left_ed_bg.width = ff_measure_utf32(m->digit_buf.data, m->digit_buf.len, *g_style).width;
    left_ed_bg.x = bar_rec.x + bar_rec.width - left_ed_bg.width;
    left_ed_bg.height = g_style->typo.size;
    left_ed_bg.y = tag_y;

    float zero_of_str_w = ff_measure_utf8("0 of ", 5, *g_style).width;
    float zero_of_str_x = left_ed_bg.x - zero_of_str_w;
    float zero_of_str_y = left_ed_bg.y;
    ff_draw_str8("0 of ", 5, zero_of_str_x, zero_of_str_y, (float*)g_cfg.global_projection, *g_style);
    handle_name_edit(m, name_rec, enable_input && m->focus == TC_NAME);
    handle_digit_edit(m, left_ed_bg, enable_input && m->focus == TC_COUNT);

    if (STICKY(KEY_TAB)) {
        m->focus = (m->focus + 1) % 2;
    }

    if ((clicked_add || STICKY(KEY_ENTER)) && m->name_buf.len && m->digit_buf.len) {
        assert(out);
        task_set_name(out, m->name_buf.data, m->name_buf.len);
        out->left = get_left(m);
        out->done = 0;
        clear(m);
        result.create = 1;
    }

    return result;
}
