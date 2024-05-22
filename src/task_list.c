#include "task_list.h"

#include <assert.h>
#include <raylib.h>
#include <rlgl.h>

#include "clip.h"
#include "colors.h"
#include "config.h"
#include "draw_opts.h"
#include "task.h"

#define TASK_VEC_INIT_CAP 0x100

static Task_Vec task_vec_create(void) {
    Task_Vec ret = {.data = malloc(TASK_VEC_INIT_CAP),
                    .len = 0,
                    .cap = TASK_VEC_INIT_CAP};
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

static void task_vec_push(Task_Vec* m, Task t) {
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
        task_destroy(&m->data[i]);
    }
    free(m->data);
}

Task_List task_list_create(void) {
    Task_List ret = {.tasks = task_vec_create()};
    return ret;
}

inline void task_list_push(Task_List* m, Task t) {
    task_vec_push(&m->tasks, t);
}

void task_list_prealloc(Task_List* m, size_t number_of_tasks) {
    task_vec_prealloc(&m->tasks, number_of_tasks);
}

void task_list_destroy(Task_List* m) { task_vec_destroy(&m->tasks); }

static void handle_scroll(Task_List* m, float y, float max_h) {
    if (!m->tasks.len) return;
    float mouse_y_scroll = GetMouseWheelMove();
    if (!mouse_y_scroll) return;
    float mb =
        (task_height() + g_cfg.inner_gap) * (m->tasks.len + 2) +
        (m->scroll);

    if (mb > max_h && mouse_y_scroll < 0)
        m->scroll += mouse_y_scroll * MOUSE_WHEEL_Y_SCROLL_SCALE;
    if (mouse_y_scroll > 0)
        m->scroll += mouse_y_scroll * MOUSE_WHEEL_Y_SCROLL_SCALE;
    if (m->scroll > 0) m->scroll = 0;
}

static void move_task_up(Task_List* m, size_t idx) {
    assert(idx);
    Task tmp = m->tasks.data[idx - 1];
    size_t copy_size = sizeof(*m->tasks.data);
    memcpy(&m->tasks.data[idx - 1], &m->tasks.data[idx], copy_size);
    memcpy(&m->tasks.data[idx], &tmp, copy_size);
}

Changed_Task task_list_draw(Task_List* m, float x, float y,
                            float max_w, float max_h,
                            Draw_Opts opts) {
    Changed_Task result = 0;
    if (opts & DRAW_ENABLE_SCROLL) handle_scroll(m, y, max_h);

    const float task_h = task_height();
    float task_y = y + m->scroll;

    clip_begin(x, y, max_w, max_h);
    for (size_t i = 0; i < m->tasks.len;
         i += 1, task_y += task_h + g_cfg.inner_gap) {
        if ((task_y + task_h) < y) continue;
        if ((task_y - y) >= max_h) break;
        Task* task = &m->tasks.data[i];
        Task_Return_Flags t_ret = task_draw(task, x, task_y, max_w);
        if (i && t_ret & TASK_MOVE_UP) {
            move_task_up(m, i);
            task = &m->tasks.data[i - 1];
        }
        if (t_ret & TASK_CHANGED) result = task;
    }

    Color grad_col1 = GetColor(g_color[g_cfg.theme][COLOR_CRUST]);
    Color grad_col0 = grad_col1;
    grad_col0.a = 0x00;
    DrawRectangleGradientV(x, y + max_h * 0.75f, max_w, max_h * .25f,
                           grad_col0, grad_col1);
    clip_end();

    return result;
}
