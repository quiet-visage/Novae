#pragma once
#include <fieldfusion.h>

#include "button.h"
#include "check_btn.h"

typedef unsigned short U8;

typedef struct {
    Check_Btn check_btn;
    Btn move_up_btn;
    C32* name;
    size_t name_len;
    U8 left;
    U8 done;
    bool complete;
    int db_id;
    int tag_id;
    Motion check_btn_mo;
    Motion up_btn_mo;
    Motion bar_mo;
} Task;

typedef enum {
    TASK_CHANGED = (1 << 0),
    TASK_MOVE_UP = (1 << 1),
    TASK_MOVE_TOP = (1 << 2),
} Task_Ret_Flags;

typedef int Task_Return_Flags;

Task task_create(void);
void task_destroy(Task* m);
void task_set_name(Task* m, C32* name, size_t len);
float task_height(void);
Task_Return_Flags task_draw(Task* m, float x, float y, float max_width, Rectangle bounds);
