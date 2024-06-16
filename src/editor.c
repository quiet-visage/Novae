#include "editor.h"

#include <assert.h>
#include <raylib.h>

#include "c32_vec.h"
#include "chrono.h"
#include "config.h"

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

Editor editor_create(void) { return (Editor){0}; }

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
        m->cursor += 1;
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
