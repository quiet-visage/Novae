#pragma once

#include "button.h"
#include "c32_vec.h"
#include "date_edit.h"
#include "editor.h"

typedef enum {
    DATE_PICK_STATE_COMPACT = 0,
    DATE_PICK_STATE_OPEN,
} Date_Pick_State;

typedef struct {
    Date from;
    Date to;
} Date_Range;

typedef struct {
    Date_Pick_State state;
    Date_Range date_range;
    Btn calendar_button;
    int flags;
    C32_Vec from_buf;
    Date_Edit from_edit;
    C32_Vec to_buf;
    Editor to_edit;
} Date_Pick;

Date_Pick date_pick_create(void);
Date_Range* date_pick_view(Date_Pick* m, float cx, float cy, bool enable_hint, bool enable_input);
void date_pick_destroy(Date_Pick* m);
