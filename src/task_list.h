#pragma once

#include "draw_opts.h"
#include "task.h"

typedef struct {
    Task* data;
    size_t len;
    size_t cap;
} Task_Vec;

typedef struct {
    Task_Vec tasks;
    float scroll;
} Task_List;

typedef Task* Changed_Task;

Task_List task_list_create(void);
void task_list_push(Task_List* m, Task t);
void task_list_prealloc(Task_List* m, size_t number_of_tasks);
Changed_Task task_list_draw(Task_List* m, float x, float y, float max_w, float max_h,
                            Draw_Opts opts);
void task_list_destroy(Task_List* m);
