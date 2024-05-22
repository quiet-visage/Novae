#pragma once

#include "c32_vec.h"

typedef enum {
    EDITOR_NORMAL,
    EDITOR_SELECTION,
} Editor_State;

typedef enum {
    EDITOR_DIGIT_ONLY = 0x1,
} Editor_Flags;

typedef struct {
    Editor_State state;
    int flags;
    size_t sel_from;
    size_t sel_to;
    size_t cursor;
    size_t limit;
} Editor;

Editor editor_create(void);
bool editor_handle_input(Editor* m, C32_Vec* buf);
void editor_clear(Editor* m);
void editor_select(Editor* m, size_t from, size_t to);
