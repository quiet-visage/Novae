#include "date_edit.h"

#include <assert.h>
#include <inttypes.h>
#include <math.h>

#include "alpha_inherit.h"
#include "c32_vec.h"
#include "config.h"
#include "cursor.h"
#include "date.h"
#include "fieldfusion.h"
#include "raylib.h"

#define IS_NUM(ch) (ch >= '0' && ch <= '9')

typedef struct {
    size_t start;
    size_t end;
} Idx_Range;

FF_Style* g_style = &g_cfg.sstyle;

void date_edit_sync_date_and_buf(Date_Edit* m, C32_Vec* buf) {
    buf->len = 0;
    char str[16] = {0};
    size_t str_len = snprintf(str, 16, "%d/%d/%d", m->input_date.day, m->input_date.month + 1,
                              m->input_date.year);  // month is 0 indexed
    c32_vec_ins_str8(buf, 0, str, str_len);
}

static Idx_Range get_placement_idx_range(C32_Vec* buf, Date_Edit_Placement place) {
    Idx_Range result = {0};
    switch (place) {
        case DATE_EDIT_PLACEMENT_DAY: {
            for (size_t i = 0; i < buf->len; ++i) {
                if (buf->data[i] != L'/') continue;
                result.end = i;
                break;
            }
        } break;
        case DATE_EDIT_PLACEMENT_MON: {
            for (size_t i = 0; i < buf->len; i++) {
                if (buf->data[i] != L'/') continue;
                result.start = i + 1;
                break;
            }
            for (size_t i = buf->len; i-- > 0;) {
                if (buf->data[i] != L'/') continue;
                result.end = i;
                break;
            }
        } break;
        case DATE_EDIT_PLACEMENT_YEAR: {
            for (size_t i = buf->len; i-- > 0;) {
                if (buf->data[i] != L'/') continue;
                result.start = i + 1;
                break;
            }
            result.end = buf->len;
        } break;
    }
    return result;
}

static Date get_date_from_buffer(Date_Edit* m, C32_Vec* buf) {
    char str8buf[buf->len + 1];
    memset(str8buf, 0, buf->len + 1);
    ff_utf32_to_utf8(str8buf, buf->data, buf->len);

    int day = strtoimax(str8buf, 0, 10);
    size_t mon_idx = get_placement_idx_range(buf, DATE_EDIT_PLACEMENT_MON).start;
    int mon = strtoimax(&str8buf[mon_idx], 0, 10) - 1;
    size_t year_idx = get_placement_idx_range(buf, DATE_EDIT_PLACEMENT_YEAR).start;
    int year = strtoimax(&str8buf[year_idx], 0, 10);
    return (Date){year, mon, day};
}

static void auto_correct_buffer_date(Date_Edit* m, C32_Vec* buf) {
    Date now_date = get_current_date();
    Date buf_date = get_date_from_buffer(m, buf);
    buf_date.day = buf_date.day > 31 ? 31 : !buf_date.day ? 1 : buf_date.day;
    buf_date.month = buf_date.month > 12 ? 11 : buf_date.month == -1 ? 0 : buf_date.month;

    buf_date.year = !buf_date.year ? now_date.year : buf_date.year;
    int year_digits = log10(buf_date.year) + 1;
    int factor = pow(10, year_digits);
    int rest = (int)(now_date.year / factor) * factor;
    buf_date.year = year_digits < 4 ? rest + buf_date.year : buf_date.year;

    m->input_date = buf_date;

    // if (m->min_date.year) {
    //     size_t con_min = m->min_date.day * m->min_date.month * m->min_date.year;
    //     size_t con = m->input_date.day * m->input_date.month * m->input_date.year;
    //     bool lesseq = con_min > con;
    //     if (lesseq) {
    //         m->input_date = m->min_date;
    //     }
    // }

    date_edit_sync_date_and_buf(m, buf);
}

static void select_placement(Date_Edit* m, C32_Vec* buf) {
    auto_correct_buffer_date(m, buf);
    Idx_Range range = get_placement_idx_range(buf, m->placement);
    assert(range.end <= buf->len);
    assert(range.start <= range.end);
    if (range.start == range.end) return;
    m->state = DATE_EDIT_STATE_SELECTION;
    m->sel_from = range.start;
    m->sel_to = range.end;
    m->cursor = m->sel_from;
}

static void select_prev_placement(Date_Edit* m, C32_Vec* buf) {
    if (!m->placement)
        m->placement = 2;
    else
        --m->placement;
    select_placement(m, buf);
}

static void select_next_placement(Date_Edit* m, C32_Vec* buf) {
    m->placement = (m->placement + 1) % 3;
    select_placement(m, buf);
}

static void move_cursor_right(Date_Edit* m, C32_Vec* buf) {
    if (m->cursor > buf->len) return;
    if (buf->data[m->cursor] == L'/')
        select_next_placement(m, buf);
    else
        ++m->cursor;
}

static void move_cursor_left(Date_Edit* m, C32_Vec* buf) {
    if (!m->cursor) return;
    --m->cursor;
    if (buf->data[m->cursor] == L'/') select_prev_placement(m, buf);
}

static int get_placement_len(Date_Edit* m, C32_Vec* buf) {
    Idx_Range range = get_placement_idx_range(buf, m->placement);
    return range.end - range.start;
}

static bool placement_overflows(Date_Edit* m, C32_Vec* buf, int len) {
    if (m->placement != DATE_EDIT_PLACEMENT_YEAR) {
        Idx_Range idx_range = get_placement_idx_range(buf, m->placement);
        int idx_len = idx_range.end - idx_range.start;
        return idx_len >= 2;
    } else if (len > 4)
        return 1;
    return 0;
}

static void backspace(Date_Edit* m, C32_Vec* buf) {
    if (!m->cursor) return;
    if (buf->data[m->cursor - 1] == L'/') return;
    c32_vec_del_str(buf, m->cursor - 1, 1);
    --m->cursor;
}

static void delete_c(Date_Edit* m, C32_Vec* buf) {
    if (m->cursor == buf->len) return;
    if (buf->data[m->cursor] == L'/') return;
    c32_vec_del_str(buf, m->cursor, 1);
}

static void input_insert_chars(Date_Edit* m, C32_Vec* buf, char init) {
    char char_inp = init;
    while (char_inp && IS_NUM(char_inp)) {
        if (placement_overflows(m, buf, get_placement_len(m, buf) + 1)) {
            if (m->cursor == buf->len || buf->data[m->cursor] == L'/') {
                size_t pos = !m->cursor ? m->cursor : m->cursor - 1;
                c32_vec_del_str(buf, pos, 1);
                c32_vec_ins_str8(buf, pos, &char_inp, 1);
            } else {
                delete_c(m, buf);
                c32_vec_ins_str8(buf, m->cursor, &char_inp, 1);
                if (m->cursor <= buf->len && buf->data[m->cursor] != L'/') ++m->cursor;
            }
        } else {
            c32_vec_ins_str8(buf, m->cursor, &char_inp, 1);
            if (m->cursor <= buf->len && buf->data[m->cursor] != L'/') ++m->cursor;
        }
        char_inp = GetCharPressed();
    }
}

static void handle_input(Date_Edit* m, C32_Vec* buf) {
    char char_inp = GetCharPressed();
    if (m->state == DATE_EDIT_STATE_SELECTION) {
        if (m->sel_from == m->sel_to)
            m->state = DATE_EDIT_STATE_NORMAL;
        else if (char_inp && IS_NUM(char_inp)) {
            assert(m->sel_from < m->sel_to);
            assert(m->sel_to <= buf->len);
            c32_vec_del_str(buf, m->sel_from, m->sel_to - m->sel_from);
            m->sel_from = 0;
            m->sel_to = 0;
            m->state = DATE_EDIT_STATE_NORMAL;
            input_insert_chars(m, buf, char_inp);
        } else if (STICKY(KEY_BACKSPACE) || STICKY(KEY_DELETE)) {
            c32_vec_del_str(buf, m->sel_from, m->sel_to - m->sel_from);
            m->sel_from = 0;
            m->sel_to = 0;
            m->state = DATE_EDIT_STATE_NORMAL;
        } else if (STICKY(KEY_RIGHT)) {
            select_next_placement(m, buf);
        } else if (STICKY(KEY_LEFT)) {
            select_prev_placement(m, buf);
        }
    } else {
        input_insert_chars(m, buf, char_inp);
        if (STICKY(KEY_LEFT)) {
            move_cursor_left(m, buf);
        } else if (STICKY(KEY_RIGHT)) {
            move_cursor_right(m, buf);
        } else if (STICKY(KEY_BACKSPACE)) {
            backspace(m, buf);
        } else if (STICKY(KEY_DELETE)) {
            delete_c(m, buf);
        }
    }
}

Date_Edit date_edit_create(C32_Vec* buf) {
    Date_Edit m = {0};
    m.cursor_view = cursor_new();

    m.input_date = get_current_date();
    date_edit_sync_date_and_buf(&m, buf);
    m.state = DATE_EDIT_STATE_SELECTION;

    return m;
}

static size_t get_selection_len(Date_Edit* m, C32_Vec* buf) {
    assert(m->sel_from < buf->len);
    assert(m->sel_from <= m->sel_to);
    return m->sel_to - m->sel_from;
}

static float get_selection_x_offset(Date_Edit* m, C32_Vec* buf) {
    assert(m->state == DATE_EDIT_STATE_SELECTION);
    assert(buf->len);
    size_t selection_len = get_selection_len(m, buf);
    if (!selection_len || !m->sel_from) return 0;
    assert(m->sel_to <= buf->len);
    return ff_measure_utf32(buf->data, m->sel_from, *g_style).width;
}

static float get_selection_w(Date_Edit* m, C32_Vec* buf) {
    if (!buf->len) return 0.f;
    assert(m->state == DATE_EDIT_STATE_SELECTION);
    assert(m->sel_from < buf->len);
    assert(m->sel_from <= m->sel_to);
    C32* from_ptr = &buf->data[m->sel_from];
    size_t selection_len = m->sel_to - m->sel_from;
    if (!selection_len) return 0;
    float selection_w = ff_measure_utf32(from_ptr, selection_len, *g_style).width;
    return selection_w;
}

static void view_selection(Date_Edit* m, C32_Vec* buf, float px, float py) {
    if (!buf->len) return;
    if (m->state != DATE_EDIT_STATE_SELECTION) return;
    Rectangle rec = {0};
    rec.x = get_selection_x_offset(m, buf) + px;
    rec.width = get_selection_w(m, buf);
    if (!rec.width) return;
    rec.y = py;
    rec.height = g_style->typo.size;
    Color color = GET_RCOLOR(COLOR_SURFACE1);
    color.a = 0x90;
    DrawRectangleRec(rec, color);
}

static float get_cursor_x(const C32* buf, size_t len, size_t cursor_pos, float x) {
    if (!cursor_pos) return x;

    float cursor_offset = ff_measure_utf32(buf, cursor_pos, *g_style).width;
    return x + cursor_offset;
}

float date_edit_width(C32_Vec* buf) { return ff_measure_utf32(buf->data, buf->len, *g_style).width; }
inline float date_edit_height(void) {return g_style->typo.size;}

bool date_edit_view(Date_Edit* m, C32_Vec* buf, Vector2 pos, bool focused) {
    if (focused) {
        cursor_set_focused(&m->cursor_view, 1);
        handle_input(m, buf);
        if (IsKeyPressed(KEY_TAB)) {
            select_next_placement(m, buf);
        }
    } else {
        cursor_set_focused(&m->cursor_view, 0);
    }

    view_selection(m, buf, pos.x, pos.y);

    FF_Style style = *g_style;
    style.typo.color &= ~0xff;
    style.typo.color |= alpha_inherit_get_alpha();
    ff_draw_str32(buf->data, buf->len, pos.x, pos.y, (float*)(g_cfg.global_projection), style);

    float cursor_x = get_cursor_x(buf->data, buf->len, m->cursor, pos.x);
    float cursor_y = pos.y + g_style->typo.size * .1;
    cursor_draw(&m->cursor_view, cursor_x, cursor_y);

    if (focused && IsKeyPressed(KEY_ENTER)) {
        auto_correct_buffer_date(m, buf);
        m->placement = 0;
        select_placement(m, buf);
        return 1;
    }
    return 0;
}
