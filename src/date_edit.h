#pragma once

#include "c32_vec.h"
#include "cursor.h"
#include "date.h"

typedef enum {
    DATE_EDIT_STATE_NORMAL,
    DATE_EDIT_STATE_SELECTION,
} Date_Edit_State;

typedef enum {
    DATE_EDIT_PLACEMENT_DAY,
    DATE_EDIT_PLACEMENT_MON,
    DATE_EDIT_PLACEMENT_YEAR,
} Date_Edit_Placement;

typedef struct {
    Date_Edit_State state;
    Date_Edit_Placement placement;
    Date input_date;
    Cursor cursor_view;
    size_t cursor;
    size_t sel_from;
    size_t sel_to;
} Date_Edit;

Date_Edit date_edit_create(C32_Vec* buf);
bool date_edit_view(Date_Edit* m, C32_Vec* buf, float x, float y, bool focused);
