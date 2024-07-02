#pragma once

#include "button.h"
#include "draw_opts.h"
#include "task.h"

typedef struct {
    Task task;
    size_t display_index;
    Motion mo;
    float target_pos;
    bool hidden;
} Task_List_Item;

typedef struct {
    Task_List_Item* data;
    size_t len;
    size_t cap;
} Task_Vec;

typedef enum { TASK_LIST_FLAG_SHOW_HIDDEN = (1 << 0) } Task_List_Flags;

typedef struct {
    Task_Vec tasks;
    float scroll;
    Motion scroll_mo;
    Motion scroll_al;
    Btn show_hidden_btn;
    int flags;
} Task_List;

typedef enum {
    TASK_EVENT_NONE,
    TASK_EVENT_MARK_DONE,
    TASK_EVENT_DELETE,
} Task_Event;

typedef struct {
    Task* related_task;
    Task_Event related_task_event;
} Task_List_Return;

Task_List task_list_create(void);
Task* task_list_get(Task_List* m, size_t idx);
void task_list_push(Task_List* m, Task t);
Task* task_list_get_prioritized(Task_List* m);
void task_list_prealloc(Task_List* m, size_t number_of_tasks);
Task_List_Return task_list_view(Task_List* m, float x, float y, float max_w, float max_h, Draw_Opts opts);
void task_list_destroy(Task_List* m);
void task_list_normalize_display_index(Task_List* m);
