#pragma once
#include "button.h"
#include "c32_vec.h"
#include "cursor.h"
#include "editor.h"
#include "task.h"
#include "text_view.h"

typedef enum { TC_NAME, TC_COUNT } TC_Focus;

// 360
typedef struct {
    C32_Vec name_buf;
    Editor name_ed;
    Cursor name_curs;
    Motion name_hor_motion;
    C32_Vec digit_buf;
    Editor digit_ed;
    Cursor digit_curs;
    float target_scroll;
    TC_Focus focus;
    Btn add_btn;
} Task_Creator;

Task_Creator task_creator_create();
float task_creator_height(void);
void task_creator_destroy(Task_Creator* m);
bool task_creator_draw(Task_Creator* m, Task* out, float x, float y,
                       float max_width, bool enable_input);
