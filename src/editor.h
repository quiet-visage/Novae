#pragma once

#include <raylib.h>

#include "c32_vec.h"
#include "cursor.h"
#include "motion.h"

typedef enum {
    EDITOR_NORMAL,
    EDITOR_SELECTION,
} Editor_State;

typedef enum {
    EDITOR_DIGIT_ONLY = (1 << 0),
    EDITOR_DRAW_BG = (1 << 1),
    EDITOR_HANDLE_HORIZONTAL_SCROLLING = (1 << 2),
    EDITOR_CENTER_INPUT = (1 << 3),
} Editor_Flags;

typedef struct {
    Editor_State state;
    char* placeholder;
    int flags;
    size_t sel_from;
    size_t sel_to;
    size_t cursor;
    size_t limit;
    float width;      // 0 when disabled
    float min_width;  // 0 when disabled
    float max_width;  // 0 when disabled
    float horz_scroll;
    Motion horz_scroll_mo;
    Cursor cursor_view;
} Editor;

Editor editor_create(void);
void editor_destroy(Editor* m);
bool editor_handle_input(Editor* m, C32_Vec* buf);
void editor_clear(Editor* m);
void editor_set_placeholder(Editor* m, char* str);
void editor_select(Editor* m, size_t from, size_t to);
Vector2 editor_view_get_dimensions(Editor* m, C32_Vec* buf);
void editor_view(Editor* m, C32_Vec* buf, float x, float y, bool focused);
void editor_set_flag(Editor* m, int flag);
void editor_unset_flag(Editor* m, int flag);
float editor_get_text_width(Editor* m, C32_Vec* buf);
