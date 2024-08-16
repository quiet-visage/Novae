#pragma once
#include <fieldfusion.h>

#include "date.h"
#include "swipe_btn.h"

typedef unsigned short U8;

typedef struct {
    C32* name;
    size_t name_len;
    U8 left;
    U8 done;
    bool complete;  // NOTE could be a flag, with is_complete and is_hidden info
    int db_id;
    int tag_id;
    Motion check_btn_mo;
    Motion up_btn_mo;
    Motion bar_mo;
    Swipe_Btn swipe;
    Date date_created;
    Date_Range date_range;
    float diligence;
    float idle;
    float rest;
} Task;

typedef enum {
    TASK_NONE = 0,
    TASK_MOVE_UP,
    TASK_MOVE_TOP,
    TASK_DELETE,
    TASK_MARK_DONE,
} Task_Return;

Task task_create(void);
void task_destroy(Task* m);
void task_set_name(Task* m, C32* name, size_t len);
float task_height(void);
Task_Return task_draw(Task* m, float x, float y, float max_width, Rectangle bounds, bool enabled);
