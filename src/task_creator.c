#include "task_creator.h"

#include <assert.h>
#include <fieldfusion.h>
#include <math.h>
#include <raylib.h>
#include <rlgl.h>

#include "button.h"
#include "c32_vec.h"
#include "clip.h"
#include "colors.h"
#include "config.h"
#include "cursor.h"
#include "editor.h"
#include "icon.h"
#include "motion.h"
#include "text_view.h"

Task_Creator task_creator_create() {
    Task_Creator ret = {.name_buf = c32_vec_create(),
                        .digit_buf = c32_vec_create(),
                        .name_ed = editor_create(),
                        .digit_ed = editor_create(),
                        .name_curs = cursor_new(),
                        .digit_curs = cursor_new(),
                        .name_hor_motion = motion_new(),
                        .target_scroll = 0,
                        .focus = TC_NAME,
                        .add_btn = btn_create()};
    ret.add_btn.flags |= BTN_FLAG_DONT_DRAW_BG;
    ret.digit_ed.flags = EDITOR_DIGIT_ONLY;
    ret.name_ed.limit = 64;
    ret.digit_ed.limit = 3;
    ret.name_hor_motion.f = 3.0f;
    ret.name_hor_motion.z = 1.0f;
    return ret;
}

void task_creator_destroy(Task_Creator* m) {
    c32_vec_destroy(&m->digit_buf);
    c32_vec_destroy(&m->name_buf);
    btn_destroy(&m->add_btn);
}

static float get_name_cursor_x(const C32_Vec* buf, size_t curs,
                               float offset_x, float horz_scroll) {
    assert(curs <= buf->len);
    if (curs == 0) return offset_x;
    float w = ff_measure_utf32(buf->data, curs, g_cfg.btn_typo.font,
                               g_cfg.btn_typo.size, 0)
                  .width;
    return offset_x + w - horz_scroll;
}

static void draw_selection(C32_Vec* buf, Editor* ed, float x,
                           float y) {
    assert(ed->state == EDITOR_SELECTION);
    assert(ed->sel_to >= ed->sel_to);
    size_t sel_len = ed->sel_to - ed->sel_from;
    if (!sel_len) return;

    float sx = x;
    if (ed->sel_from) {
        sx += ff_measure_utf32(buf->data, ed->sel_from,
                               g_cfg.btn_typo.font,
                               g_cfg.btn_typo.size, 0)
                  .width;
    }
    float width =
        ff_measure_utf32(&buf->data[ed->sel_from], sel_len,
                         g_cfg.btn_typo.font, g_cfg.btn_typo.size, 0)
            .width;

    Color col = GetColor(g_color[g_cfg.theme][COLOR_SURFACE2]);
    col.a = 0x9a;
    DrawRectangle(sx, y, width, g_cfg.btn_typo.size, col);
    rlDrawRenderBatchActive();
}

// static void handle_mouse_selection(Task_Creator* m, C32_Vec* buf,
//                                    Rectangle bounds) {
//     if (!buf->len) return;
//     Vector2 mouse = GetMousePosition();
//     if (!CheckCollisionPointRec(mouse, bounds)) return;
// }

static void handle_digit_edit(Task_Creator* m, Rectangle n_task_bg,
                              bool enabled) {
    if (enabled) {
        bool changed =
            editor_handle_input(&m->digit_ed, &m->digit_buf);
        m->digit_curs.flags |= CURSOR_FLAG_FOCUSED;
        if (changed)
            m->digit_curs.flags |= CURSOR_FLAG_RECENTLY_MOVED;
    } else {
        m->digit_curs.flags &= ~CURSOR_FLAG_FOCUSED;
    }

    if (m->name_ed.state == EDITOR_SELECTION && m->digit_buf.len)
        draw_selection(&m->digit_buf, &m->digit_ed, n_task_bg.x,
                       n_task_bg.y);

    float glyph_width =
        ff_measure_utf32(m->digit_buf.data, m->digit_buf.len,
                         g_cfg.btn_typo.font, g_cfg.btn_typo.size, 0)
            .width;
    float x = CENTER(n_task_bg.x, n_task_bg.width, glyph_width);
    ff_draw_str32(m->digit_buf.data, m->digit_buf.len, x,
                  n_task_bg.y + g_cfg.btn_typo.size * .25f,
                  (float*)g_cfg.global_projection, g_cfg.btn_typo,
                  FF_FLAG_DEFAULT, 0);

    float cursor_y =
        CENTER(n_task_bg.y, n_task_bg.height, g_cfg.btn_typo.size);
    cursor_draw(&m->digit_curs,
                get_name_cursor_x(&m->digit_buf, m->digit_ed.cursor,
                                  x, m->name_hor_motion.position[0]),
                cursor_y);

    m->digit_curs.flags &= ~CURSOR_FLAG_FOCUSED;
}

static void handle_name_edit(Task_Creator* m, Rectangle name_bg,
                             bool enabled) {
    if (enabled) {
        bool changed = editor_handle_input(&m->name_ed, &m->name_buf);
        m->name_curs.flags |= CURSOR_FLAG_FOCUSED;
        if (changed) m->name_curs.flags |= CURSOR_FLAG_RECENTLY_MOVED;
    } else {
        m->name_curs.flags &= ~CURSOR_FLAG_FOCUSED;
    }

    horizontal_scroll(m->name_buf.data, m->name_buf.len,
                      m->name_ed.cursor, &m->target_scroll,
                      g_cfg.btn_typo.font, g_cfg.btn_typo.size,
                      name_bg.width);
    update_scroll_animation(&m->name_hor_motion, m->target_scroll, 0);
    float projection[4][4];
    get_text_projection(projection, &m->name_hor_motion);
    if (m->name_buf.len)
        ff_draw_str32(m->name_buf.data, m->name_buf.len, name_bg.x,
                      name_bg.y, (float*)projection, g_cfg.btn_typo,
                      0, 0);

    float cursor_scroll_offset = m->name_hor_motion.position[0];
    float cursor_x = get_cursor_x(m->name_buf.data, m->name_buf.len,
                                  g_cfg.btn_typo, m->name_ed.cursor,
                                  name_bg.x - cursor_scroll_offset);
    cursor_draw(&m->name_curs, cursor_x, name_bg.y);
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

inline float task_creator_height(void) {
    return g_cfg.btn_typo.size + g_cfg.inner_gap * 2;
}

static void clear(Task_Creator* m) {
    m->name_buf.len = 0;
    m->digit_buf.len = 0;
    editor_clear(&m->digit_ed);
    editor_clear(&m->name_ed);
}

bool task_creator_draw(Task_Creator* m, Task* out, float x, float y,
                       float max_width, bool enable_input) {
    Rectangle bg_rec;
    bg_rec.x = x;
    bg_rec.y = y;
    bg_rec.width = max_width;
    bg_rec.height = task_creator_height();
    float bg_rad = RADIUS_TO_ROUNDNESS(8, bg_rec.height);
    Color bg_col = GetColor(g_color[g_cfg.theme][COLOR_BASE]);
    DrawRectangleRounded(bg_rec, bg_rad, 6, bg_col);

    float cx = x + g_cfg.inner_gap;

    float btn_x = cx;
    float btn_sz = btn_height(&m->add_btn);
    float btn_y = CENTER(y, bg_rec.height, btn_sz);
    bool clicked_add = btn_draw_with_icon(
        &m->add_btn, ICON_ADD_CIRCLE, btn_x, btn_y);

    Rectangle n_task_bg;
    n_task_bg.width = 2 * g_cfg.btn_typo.size;

    Rectangle name_bg;
    name_bg.x = btn_x + btn_sz + g_cfg.inner_gap;
    float right_side_width = n_task_bg.width + g_cfg.inner_gap * 2;
    float left_side_width = btn_sz + g_cfg.inner_gap * 2;
    name_bg.width =
        (bg_rec.width - right_side_width - left_side_width);
    name_bg.height = g_cfg.btn_typo.size * 1.5f;
    name_bg.y = CENTER(y, bg_rec.height, name_bg.height);
    Color surface0_col =
        GetColor(g_color[g_cfg.theme][COLOR_SURFACE0]);
    DrawRectangleRounded(name_bg, 1.0f, 6, surface0_col);

    n_task_bg.x =
        bg_rec.x + bg_rec.width - g_cfg.inner_gap - n_task_bg.width;
    n_task_bg.height = name_bg.height;
    n_task_bg.y = name_bg.y;
    DrawRectangleRounded(n_task_bg, 1.0f, 6, surface0_col);
    rlDrawRenderBatchActive();

    clip_begin(bg_rec.x, bg_rec.y, bg_rec.width, bg_rec.height);

    clip_begin_rounded(name_bg.x, name_bg.y, name_bg.width,
                       name_bg.height, 16.f);
    handle_name_edit(m, name_bg, m->focus == TC_NAME);
    clip_end();

    clip_begin_rounded(n_task_bg.x, n_task_bg.y, n_task_bg.width,
                       n_task_bg.height, 16.f);
    handle_digit_edit(m, n_task_bg, m->focus == TC_COUNT);
    clip_end();
    clip_end();

    if (STICKY(KEY_TAB)) {
        m->focus = (m->focus + 1) % 2;
    }

    if ((clicked_add || STICKY(KEY_ENTER)) && m->name_buf.len &&
        m->digit_buf.len) {
        assert(out);
        task_set_name(out, m->name_buf.data, m->name_buf.len);
        out->left = get_left(m);
        out->done = 0;
        clear(m);
        return 1;
    }
    return 0;
}
