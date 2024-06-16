#include "task_list.h"

#include <assert.h>
#include <raylib.h>
#include <rlgl.h>

#include "clip.h"
#include "colors.h"
#include "config.h"
#include "db.h"
#include "draw_opts.h"
#include "motion.h"
#include "task.h"

#define TASK_VEC_INIT_CAP 0x100

static Task_Vec task_vec_create(void) {
    Task_Vec ret = {.data = malloc(TASK_VEC_INIT_CAP), .len = 0, .cap = TASK_VEC_INIT_CAP};
    assert(ret.data);
    return ret;
}

static void task_vec_prealloc(Task_Vec* m, size_t number_of_tasks) {
    size_t req_size = (m->len + number_of_tasks) * sizeof(m->data[0]);
    while (req_size > m->cap) {
        m->cap *= 2;
        m->data = realloc(m->data, m->cap);
        assert(m->data);
    }
}

static void task_vec_push(Task_Vec* m, Task_List_Item t) {
    size_t req_size = (m->len + 1) * sizeof(m->data[0]);
    while (req_size > m->cap) {
        m->cap *= 2;
        m->data = realloc(m->data, m->cap);
        assert(m->data);
    }

    m->data[m->len++] = t;
}

static void task_vec_destroy(Task_Vec* m) {
    assert(m->data);
    for (size_t i = 0; i < m->len; i += 1) {
        task_destroy(&m->data[i].task);
    }
    free(m->data);
}

Task_List task_list_create(void) {
    Task_List ret = {.tasks = task_vec_create(), .scroll = 0, .scroll_mo = motion_new()};
    return ret;
}

inline void task_list_push(Task_List* m, Task t) {
    Task_List_Item item = {0};
    item.task = t;
    item.mo = motion_new();
    item.display_index = m->tasks.len;
    task_vec_push(&m->tasks, item);
}

// static int sort_based_on_completion_comparison(const void* ap, const void* bp) {
//     const Task_List_Item* a = ap;
//     const Task_List_Item* b = bp;
//     return (a->task.left - a->task.done) - (b->task.left - b->task.done);
// }

// static void sort_based_on_completion(Task_List* m) {
//     qsort(&m->tasks.data, m->tasks.len, sizeof(*m->tasks.data),
//           sort_based_on_completion_comparison);
// }

void task_list_prealloc(Task_List* m, size_t number_of_tasks) {
    task_vec_prealloc(&m->tasks, number_of_tasks);
}

void task_list_destroy(Task_List* m) { task_vec_destroy(&m->tasks); }

static void handle_scroll(Task_List* m, float y, float max_h) {
    if (!m->tasks.len) return;
    float mouse_y_scroll = GetMouseWheelMove();
    if (!mouse_y_scroll) return;
    float mb = (task_height() + g_cfg.inner_gap) * (m->tasks.len + 2) + (m->scroll);

    if (mb > max_h && mouse_y_scroll < 0) m->scroll += mouse_y_scroll * MOUSE_WHEEL_Y_SCROLL_SCALE;
    if (mouse_y_scroll > 0) m->scroll += mouse_y_scroll * MOUSE_WHEEL_Y_SCROLL_SCALE;
    if (m->scroll > 0) m->scroll = 0;
}

static Task_List_Item* find_display_idx(Task_List* m, size_t display_idx) {
    for (size_t i = 0; i < m->tasks.len; i += 1) {
        if (m->tasks.data[i].display_index == display_idx) return &m->tasks.data[i];
    }
    return 0;
}

static void move_task_up(Task_List* m, size_t idx) {
    if (m->tasks.len < 2) return;
    size_t display_idx = m->tasks.data[idx].display_index;
    if (!display_idx) return;
    size_t prev_display_idx = display_idx - 1;
    Task_List_Item* with_prev_dislpay_idx = find_display_idx(m, prev_display_idx);
    with_prev_dislpay_idx->display_index = display_idx;
    m->tasks.data[idx].display_index = prev_display_idx;
}

static void move_task_top(Task_List* m, size_t idx) {
    if (m->tasks.len < 2) return;
    size_t display_idx = m->tasks.data[idx].display_index;
    if (!display_idx) return;
    size_t prev_display_idx = 0;
    Task_List_Item* with_prev_display_idx = find_display_idx(m, prev_display_idx);
    with_prev_display_idx->display_index = display_idx;
    m->tasks.data[idx].display_index = prev_display_idx;
}

Task* task_list_get_prioritized(Task_List* m) {
    if (!m->tasks.len) return 0;
    Task_List_Item* result = find_display_idx(m, 0);
    assert(result);
    return &result->task;
}

void task_list_draw(Task_List* m, float x, float y, float max_w, float max_h, Draw_Opts opts) {
    float frame_delta = GetFrameTime();
    if (opts & DRAW_ENABLE_SCROLL) {
        handle_scroll(m, y, max_h);
        motion_update_x(&m->scroll_mo, m->scroll, frame_delta);
    }

    const float task_h = task_height();
    Rectangle bounds = {x, y, max_w, max_h};

    for (size_t i = 0; i < m->tasks.len; i += 1) {
        Task_List_Item* item = &m->tasks.data[i];
        item->target_pos = y + (task_h + g_cfg.inner_gap) * item->display_index;
        if (!item->mo.position[0]) {
            item->mo.position[0] = item->target_pos;
            item->mo.previous_input[0] = item->target_pos;
        } else {
            motion_update_x(&item->mo, item->target_pos, frame_delta);
        }
    }

    clip_begin(x, y, max_w, max_h);
    for (size_t i = 0; i < m->tasks.len; i += 1) {
        float task_y = m->tasks.data[i].mo.position[0] + m->scroll_mo.position[0];
        if (((task_y + task_h) < y) || ((task_y - y) >= max_h)) continue;
        Task* task = &m->tasks.data[i].task;

        Task_Return_Flags ret = task_draw(task, x, task_y, max_w, bounds);
        if (ret & TASK_MOVE_UP) move_task_up(m, i);
        if (ret & TASK_MOVE_TOP) move_task_top(m, i);
    }

    Color grad_col1 = GET_RCOLOR(COLOR_CRUST);
    Color grad_col0 = grad_col1;
    grad_col0.a = 0x00;
    DrawRectangleGradientV(x, y + max_h * 0.75f, max_w, max_h * .25f, grad_col0, grad_col1);
    clip_end();
}

Task* task_list_get(Task_List* m, size_t idx) {
    assert(idx < m->tasks.len);
    return &m->tasks.data[idx].task;
}
