#pragma once

#include "button.h"
#include "c32_vec.h"
#include "date_edit.h"

typedef enum {
    DATE_PICK_STATE_COMPACT = 0,
    DATE_PICK_STATE_OPEN,
} Date_Pick_State;

typedef enum {
    DATE_PICK_FOCUS_EDIT_FROM = 0,
    DATE_PICK_FOCUS_EDIT_TO,
} Date_Pick_Focus;

typedef struct {
    Date_Pick_State state;
    Date_Range date_range;
    Date_Pick_Focus focus;
    Date view_date;
    Btn calendar_btn;
    Btn next_btn;
    Btn prev_btn;
    int flags;
    C32_Vec from_buf;
    Date_Edit from_edit;
    C32_Vec to_buf;
    Date_Edit to_edit;
    Motion mo;
    float target_alpha;
    size_t hint_key;
} Date_Pick;

Date_Pick date_pick_create(void);
Date_Range* date_pick_view(Date_Pick* m, Vector2 cpos, bool enable_hint, bool enable_input);
void date_pick_destroy(Date_Pick* m);
