#include "indicator.h"

#include "clip.h"
#include "raylib.h"

inline Indicator indicator_create(void) { return (Indicator){0}; }

void indicator_calculate(Indicator* m, float x, float y, float line_width) {
    m->diag_begin.x = x;
    m->diag_begin.y = y;
    m->diag_end.x = x + INDICATOR_DIAG_WIDTH;
    m->diag_end.y = y - INDICATOR_DIAG_HEIGHT;
    m->line_begin = m->diag_end;
    m->line_end = m->line_begin;
    m->line_end.x += line_width;
    m->pivot = PIVOT_RIGHT;
}

void indicator_calculate_pivoted(Indicator* m, float x, float y, float pivot_x, float pivot_y,
                                 float line_width) {
    m->pivot = 0;
    m->diag_begin.x = x;
    m->diag_begin.y = y;
    if (x > pivot_x) {
        m->diag_end.x = x + INDICATOR_DIAG_WIDTH;
        m->line_end.x = m->diag_end.x + line_width;
        m->pivot |= PIVOT_RIGHT;
    } else {
        m->diag_end.x = x - INDICATOR_DIAG_WIDTH;
        m->line_end.x = m->diag_end.x - line_width;
        m->pivot |= PIVOT_LEFT;
    }

    m->line_begin.x = m->diag_end.x;

    if (y > pivot_y) {
        m->diag_end.y = y + INDICATOR_DIAG_HEIGHT;
        m->pivot |= PIVOT_BOTTOM;
    } else {
        m->diag_end.y = y - INDICATOR_DIAG_HEIGHT;
        m->pivot |= PIVOT_TOP;
    }

    m->line_begin.y = m->diag_end.y;
    m->line_end.y = m->diag_end.y;
}

#define MIN(x, y) (x < y ? x : y)
#define MAX(x, y) (x > y ? x : y)
void indicator_draw_faded(Indicator* m, Color base) {
    clip_begin_custom_shape();
    DrawLineEx(m->diag_begin, m->diag_end, 1.0f, WHITE);
    DrawLineEx(m->line_begin, m->line_end, 1.0f, WHITE);
    clip_end_custom_shape();
    float grad_y = MIN(m->diag_begin.y, m->diag_end.y);
    float grad_x = MIN(m->diag_begin.x, m->line_end.x);
    float rec_width = MAX(m->diag_begin.x, m->line_end.x) - MIN(m->diag_begin.x, m->line_end.x);
    Color base_fade = base;
    base_fade.a = 0x10;
    Color col1 = m->pivot & PIVOT_LEFT ? base_fade : base;
    Color col2 = m->pivot & PIVOT_LEFT ? base : base_fade;
    DrawRectangleGradientH(grad_x, grad_y, rec_width, INDICATOR_DIAG_HEIGHT * 2, col1, col2);
    clip_end();
}
