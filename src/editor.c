#include "editor.h"

#include <assert.h>
#include <raylib.h>

#include "alpha_inherit.h"
#include "c32_vec.h"
#include "chrono.h"
#include "colors.h"
#include "config.h"
#include "cursor.h"
#include "fieldfusion.h"
#include "motion.h"
#include "text_view.h"

#define IS_DIGIT(C) (C >= '0' && C <= '9')

#define SWAP(X, Y) \
    do {           \
        X = Y ^ X; \
        Y = X ^ Y; \
        X = Y ^ X; \
    } while (0)
#define NORMALIZE_SELECTION(SEL_FROM, SEL_TO)          \
    do {                                               \
        if (SEL_FROM > SEL_TO) SWAP(SEL_FROM, SEL_TO); \
    } while (0)

static FF_Style* g_style = &g_cfg.sstyle;

Editor editor_create(void) {
    Editor result = {0};
    result.horz_scroll_mo = motion_new();
    result.flags = EDITOR_DRAW_BG | EDITOR_HANDLE_HORIZONTAL_SCROLLING;
    result.cursor_view = cursor_new();
    return result;
}

static U8 handle_char_input(Editor* m, C32_Vec* buf) {
    if (buf->len >= m->limit) return 0;
    C32 in_c = 0;
    char ret = 0;

    while ((in_c = GetCharPressed())) {
        if (m->flags & EDITOR_DIGIT_ONLY && !IS_DIGIT(in_c)) continue;

        c32_vec_ins_str(buf, m->cursor, &in_c, 1);
        m->cursor += 1;
        assert(m->cursor <= buf->len);
        ret += 1;
    }

    return ret;
}

inline static void move_cursor_left(Editor* m) {
    if (m->cursor) m->cursor -= 1;
}
inline static void move_cursor_right(Editor* m, size_t buf_len) {
    if (m->cursor < buf_len) m->cursor += 1;
}

static void normal_keybinds(Editor* m, C32_Vec* buf) {
    if (STICKY(KEY_BACKSPACE) && buf->len && m->cursor) {
        assert(m->cursor <= buf->len);
        c32_vec_del_str(buf, m->cursor - 1, 1);
        m->cursor -= 1;
    }

    if (STICKY(KEY_DELETE) && buf->len && m->cursor < buf->len) {
        assert(m->cursor < buf->len);
        c32_vec_del_str(buf, m->cursor, 1);
    }

    if (STICKY(KEY_LEFT)) move_cursor_left(m);
    if (STICKY(KEY_RIGHT)) move_cursor_right(m, buf->len);

    if (STICKY(KEY_UP)) {
        m->cursor = buf->len;
    }

    if (STICKY(KEY_DOWN)) {
        m->cursor = 0;
    }

    if (IsKeyDown(KEY_LEFT_CONTROL) && STICKY(KEY_A)) {
        m->state = EDITOR_SELECTION;
        m->cursor = m->sel_to;
        m->sel_from = 0;
        m->sel_to = buf->len;
        m->cursor = 0;
    }
}

static void selection_keybinds(Editor* m, C32_Vec* buf) {
    if (STICKY(KEY_DELETE) || STICKY(KEY_BACKSPACE)) {
        c32_vec_del_str(buf, m->sel_from, m->sel_to);
        m->cursor = m->sel_from;
        m->state = EDITOR_NORMAL;
        return;
    }

    int cpressed = GetCharPressed();
    if (cpressed) {
        c32_vec_del_str(buf, m->sel_from, m->sel_to);
        m->cursor = m->sel_from;
        m->state = EDITOR_NORMAL;
        c32_vec_ins_str(buf, m->cursor, &cpressed, 1);
        handle_char_input(m, buf);
        bool inserted = m->cursor += 1;
        if (inserted) m->cursor_view.flags |= CURSOR_FLAG_RECENTLY_MOVED;
        return;
    }

    if (STICKY(KEY_LEFT) && m->cursor) {
        m->cursor = m->sel_from;
        m->sel_from = 0;
        m->sel_to = 0;
        m->state = EDITOR_NORMAL;
    }
    if (STICKY(KEY_RIGHT) && m->cursor < buf->len) {
        m->cursor = m->sel_to;
        m->sel_from = 0;
        m->sel_to = 0;
        m->state = EDITOR_NORMAL;
    }
    if (STICKY(KEY_UP)) {
        m->cursor = buf->len;
        m->sel_from = 0;
        m->sel_to = 0;
        m->state = EDITOR_NORMAL;
    }
    if (STICKY(KEY_DOWN)) {
        m->cursor = 0;
        m->sel_from = 0;
        m->sel_to = 0;
        m->state = EDITOR_NORMAL;
    }
}

void editor_clear(Editor* m) {
    m->cursor = 0;
    m->sel_from = 0;
    m->sel_to = 0;
    m->state = EDITOR_NORMAL;
}

void editor_select(Editor* m, size_t from, size_t to) {
    m->sel_from = from;
    m->sel_to = to;
    NORMALIZE_SELECTION(m->sel_from, m->sel_to);
    size_t sel_len = m->sel_to - m->sel_from;
    if (!sel_len) return;
    m->state = EDITOR_SELECTION;
    m->cursor = m->sel_to;
}

bool editor_handle_input(Editor* m, C32_Vec* buf) {
    bool ret = 0;
    switch (m->state) {
        case EDITOR_NORMAL: {
            U8 inserted = handle_char_input(m, buf);
            if (inserted) ret = 1;
            if (inserted) m->cursor_view.flags |= CURSOR_FLAG_RECENTLY_MOVED;
            normal_keybinds(m, buf);
        } break;
        case EDITOR_SELECTION: {
            selection_keybinds(m, buf);
        } break;
    }
    return ret;
}

void editor_set_placeholder(Editor* m, char* str) {
    size_t len = strlen(str);
    if (m->placeholder) free(m->placeholder);
    if (!len) {
        m->placeholder = 0;
        return;
    }
    m->placeholder = calloc(1, len + 1);
    memcpy(m->placeholder, str, len);
}

void editor_destroy(Editor* m) {
    if (m->placeholder) {
        free(m->placeholder);
    }
}

float editor_get_text_width(Editor* m, C32_Vec* buf) {
    float result = 0;
    if (buf->len)
        result = ff_measure_utf32(buf->data, buf->len, *g_style).width;
    else if (m->placeholder)
        result = ff_measure_utf8(m->placeholder, strlen(m->placeholder), *g_style).width;
    return m->width ? MIN(result, m->width) : result;
}

Vector2 editor_view_get_dimensions(Editor* m, C32_Vec* buf) {
    Vector2 result = {0};

    float horizontal_pad = g_cfg.inner_gap2 * (m->flags & EDITOR_DRAW_BG);
    float vertical_pad = g_cfg.inner_gap * (m->flags & EDITOR_DRAW_BG);
    float text_width = editor_get_text_width(m, buf);
    result.x += horizontal_pad;
    if (m->width)
        result.x = m->width;
    else {
        if (m->min_width) result.x = MAX(text_width, m->min_width);
        if (m->max_width) result.x = MIN(result.x, m->max_width);
    }
    result.y = g_style->typo.size + vertical_pad;

    return result;
}

size_t get_selection_len(Editor* m, C32_Vec* buf) {
    assert(m->sel_from < buf->len);
    assert(m->sel_from <= m->sel_to);
    return m->sel_to - m->sel_from;
}

float get_selection_x_offset(Editor* m, C32_Vec* buf) {
    assert(m->state == EDITOR_SELECTION);
    assert(buf->len);
    size_t selection_len = get_selection_len(m, buf);
    if (!selection_len || !m->cursor) return 0;
    assert(m->sel_to < buf->len);
    return ff_measure_utf32(buf->data, buf->len, *g_style).width;
}

float get_selection_w(Editor* m, C32_Vec* buf) {
    if (!buf->len) return 0.f;
    assert(m->state == EDITOR_SELECTION);
    assert(m->sel_from < buf->len);
    assert(m->sel_from <= m->sel_to);
    C32* from_ptr = &buf->data[m->sel_from];
    size_t selection_len = m->sel_to - m->sel_from;
    if (!selection_len) return 0;
    float selection_w = ff_measure_utf32(from_ptr, selection_len, *g_style).width;
    return selection_w;
}

void editor_view_selection(Editor* m, C32_Vec* buf, float px, float py) {
    if (!buf->len) return;
    if (m->state != EDITOR_SELECTION) return;
    Rectangle rec = {0};
    rec.x = get_selection_x_offset(m, buf) + px - m->horz_scroll_mo.position[0];
    rec.width = get_selection_w(m, buf);
    if (!rec.width) return;
    rec.y = py;
    rec.height = g_style->typo.size;
    Color color = GET_RCOLOR(COLOR_SURFACE1);
    color.a = 0x90;
    DrawRectangleRec(rec, color);
}

void editor_view(Editor* m, C32_Vec* buf, float px, float py, bool focused) {
    if (focused) {
        cursor_set_focused(&m->cursor_view, 1);
        bool inserted = editor_handle_input(m, buf);
        if (inserted) cursor_recently_moved(&m->cursor_view);
    } else
        cursor_set_focused(&m->cursor_view, 0);
    assert(buf);
    editor_view_selection(m, buf, px + g_cfg.inner_gap, py);
    Vector2 dimensions = editor_view_get_dimensions(m, buf);

    if (m->flags & EDITOR_DRAW_BG) {
        Rectangle rec = {px, py - g_cfg.inner_gap * .25, m->width, dimensions.y};
        DRAW_BG(rec, 0x400, COLOR_SURFACE0);
    }

    float projection[4][4];
    if (m->flags & EDITOR_HANDLE_HORIZONTAL_SCROLLING) {
        horizontal_scroll(buf->data, buf->len, m->cursor, &m->horz_scroll, *g_style, dimensions.x - g_cfg.inner_gap2);
        update_scroll_animation(&m->horz_scroll_mo, m->horz_scroll, 0);
        get_text_projection(projection, &m->horz_scroll_mo);
    } else {
        memcpy(projection, g_cfg.global_projection, sizeof(g_cfg.global_projection));
    }

    float cursor_offset = m->horz_scroll_mo.position[0];
    float x = px + g_cfg.inner_gap;

    FF_Style style = *g_style;
    style.typo.color &= ~0xff;
    style.typo.color |= alpha_inherit_get_alpha();
    ff_draw_str32(buf->data, buf->len, x, py, (float*)projection, style);

    float cursor_x = get_cursor_x(buf->data, buf->len, *g_style, m->cursor, px) - cursor_offset + g_cfg.inner_gap;
    float cursor_y = py + g_style->typo.size * .1;
    cursor_draw(&m->cursor_view, cursor_x, cursor_y);

    if (!buf->len && m->placeholder) {
        size_t placeholder_len = strlen(m->placeholder);
        FF_Style style = *g_style;
        style.typo.color = GET_COLOR(COLOR_SURFACE1);
        style.typo.color &= ~0xff;
        style.typo.color |= alpha_inherit_get_alpha();
        ff_draw_str8(m->placeholder, placeholder_len, x, py, (float*)projection, style);
    }
}

void editor_set_flag(Editor* m, int flag) { m->flags |= flag; }

void editor_unset_flag(Editor* m, int flag) { m->flags &= ~flag; }
