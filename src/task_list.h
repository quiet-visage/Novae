#pragma once

#include "draw_opts.h"
#include "task.h"

typedef struct {
    Task task;
    size_t display_index;
    Motion mo;
    float target_pos;
} Task_List_Item;

typedef struct {
    Task_List_Item* data;
    size_t len;
    size_t cap;
} Task_Vec;

typedef struct {
    Task_Vec tasks;
    float scroll;
    Motion scroll_mo;
} Task_List;

Task_List task_list_create(void);
Task* task_list_get(Task_List* m, size_t idx);
void task_list_push(Task_List* m, Task t);
Task* task_list_get_prioritized(Task_List* m);
void task_list_prealloc(Task_List* m, size_t number_of_tasks);
void task_list_draw(Task_List* m, float x, float y, float max_w, float max_h, Draw_Opts opts);
void task_list_destroy(Task_List* m);
void task_list_normalize_display_index(Task_List* m);
