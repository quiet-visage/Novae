#pragma once
#include <raylib.h>

#define INDICATOR_DIAG_WIDTH (30.f)
#define INDICATOR_DIAG_HEIGHT (15.f)

typedef enum {
    PIVOT_LEFT = (1 << 0),
    PIVOT_RIGHT = (1 << 1),
    PIVOT_TOP = (1 << 2),
    PIVOT_BOTTOM = (1 << 3),
} Pivot_Mask;

typedef struct {
    Vector2 diag_begin;
    Vector2 diag_end;
    Vector2 line_begin;
    Vector2 line_end;
    Pivot_Mask pivot;
} Indicator;

Indicator indicator_create(void);
void indicator_calculate(Indicator* m, float x, float y, float line_width);
void indicator_calculate_pivoted(Indicator* m, float x, float y, float pivot_x, float pivot_y, float line_width);
void indicator_draw_faded(Indicator* m, Color base);
