#include "task_list.h"

#include <assert.h>
#include <raylib.h>
#include <rlgl.h>

#include "button.h"
#include "clip.h"
#include "colors.h"
#include "config.h"
#include "draw_opts.h"
#include "fieldfusion.h"
#include "hint.h"
#include "icon.h"
#include "motion.h"
#include "raymath.h"
#include "task.h"

#define TASK_VEC_INIT_CAP 0x100
#define SCROLL_BAR_ALPHA_HOVER_ON 0x90
#define SCROLL_BAR_ALPHA_HOVER_OFF 0x30

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
    Task_List ret = {.tasks = task_vec_create(),
                     .scroll = 0,
                     .scroll_mo = motion_new(),
                     .scroll_al = motion_new(),
                     .show_hidden_btn = btn_create()};
    ret.show_hidden_btn.flags |= BTN_FLAG_DONT_DRAW_BG;
    ret.hint_key = hint_generate_instance_key();
    return ret;
}

inline void task_list_push(Task_List* m, Task t) {
    Task_List_Item item = {0};
    item.task = t;
    item.mo = motion_new();

    for (size_t i = 0; i < m->tasks.len; ++i) {
        if (m->tasks.data[i].display_index >= 0) m->tasks.data[i].display_index += 1;
    }

    task_vec_push(&m->tasks, item);
}

void task_list_prealloc(Task_List* m, size_t number_of_tasks) { task_vec_prealloc(&m->tasks, number_of_tasks); }

void task_list_destroy(Task_List* m) {
    task_vec_destroy(&m->tasks);
    btn_destroy(&m->show_hidden_btn);
}

static size_t get_non_hidden_tasks_count(Task_List* m) {
    if (m->flags & TASK_LIST_FLAG_SHOW_HIDDEN) return m->tasks.len;

    size_t result = 0;
    for (size_t i = 0; i < m->tasks.len; i += 1)
        if (!m->tasks.data[i].hidden) result += 1;
    return result;
}

static size_t get_hidden_tasks_count(Task_List* m) { return m->tasks.len - get_non_hidden_tasks_count(m); }

static float get_task_list_unclipped_height(Task_List* m) {
    return (task_height() + g_cfg.inner_gap) * (get_non_hidden_tasks_count(m));
}

static float get_max_scroll(Task_List* m, float unclipped_height, float max_h) { return (unclipped_height - max_h); }

static float get_mouse_scroll(void) { return GetMouseWheelMove() * MOUSE_WHEEL_Y_SCROLL_SCALE; }

static void handle_scroll(Task_List* m, float y, float max_h, float scroll_delta) {
    if (!m->tasks.len) return;
    float th = get_task_list_unclipped_height(m);
    float max_scroll = get_max_scroll(m, th, max_h) * -1;
    m->scroll = Clamp(m->scroll, max_scroll, 0);
    if (!scroll_delta) return;

    if (scroll_delta > 0) m->scroll += scroll_delta * MOUSE_WHEEL_Y_SCROLL_SCALE;
    if (scroll_delta < 0) m->scroll += scroll_delta * MOUSE_WHEEL_Y_SCROLL_SCALE;
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

static void scroll_bar_view(Task_List* m, float x, float max_w, float y, float max_h) {
    float th = get_task_list_unclipped_height(m);
    float scroll_perc = (max_h / th);
    scroll_perc = Clamp(scroll_perc, 0., 1.);
    if (scroll_perc == 1.) return;
    float bar_height = scroll_perc * max_h;
    float bar_y = y - m->scroll_mo.position[0] * scroll_perc;
    Rectangle rec = {x + max_w + g_cfg.inner_gap * .25, bar_y, 6., bar_height};
    Color color = GET_RCOLOR(COLOR_SUBTEXT1);
    color.a = SCROLL_BAR_ALPHA_HOVER_OFF;
    static float scrolling = 0;

    if (scrolling) color.a = SCROLL_BAR_ALPHA_HOVER_ON;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), rec)) scrolling = 1;

    motion_update(&m->scroll_al, (float[2]){color.a, 0}, GetFrameTime());
    color.a = m->scroll_al.position[0];

    if (scrolling && IsMouseButtonDown(MOUSE_LEFT_BUTTON))
        handle_scroll(m, y, max_h, GetMouseDelta().y * -1);
    else
        scrolling = 0;

    DrawRectangleRounded(rec, 1.0, 12., color);
}

Task_List_Return task_list_view(Task_List* m, float x, float y, float max_w, float max_h, Draw_Opts opts) {
    Task_List_Return result = {0};
    float frame_delta = GetFrameTime();
    if (opts & DRAW_ENABLE_SCROLL) {
        handle_scroll(m, y, max_h, get_mouse_scroll());
        motion_update_x(&m->scroll_mo, m->scroll, frame_delta);
    }
    scroll_bar_view(m, x, max_w, y, max_h);

    const float task_h = task_height();
    Rectangle bounds = {x, y, max_w, max_h};

    {
#define hidden_task_str_cap 64
        char ht_str[hidden_task_str_cap] = {0};
        size_t ht_str_len = snprintf(ht_str, hidden_task_str_cap, "%ld task(s) hidden", get_hidden_tasks_count(m));
        ff_draw_str8(ht_str, ht_str_len, x, y, (float*)g_cfg.global_projection, g_cfg.estyle);

        bool showing_hidden = m->flags & TASK_LIST_FLAG_SHOW_HIDDEN;
        int icon = showing_hidden ? ICON_VISIBILY_ON : ICON_VISIBILY_OFF;
        Rectangle bounds = {x + max_w - BTN_ICON_SIZE * 2, y - BTN_ICON_SIZE * .5, BTN_ICON_SIZE*2, BTN_ICON_SIZE*2};
        hint_view(m->hint_key, showing_hidden ? "toggle show hidden off" : "toggle show hidden on", bounds);
        bool clicked =
            btn_draw_with_icon(&m->show_hidden_btn, icon, x + max_w - BTN_ICON_SIZE * 2, y - BTN_ICON_SIZE * .5);
        if (clicked) m->flags ^= TASK_LIST_FLAG_SHOW_HIDDEN;

        y += g_cfg.inner_gap + g_cfg.estyle.typo.size;
    }

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

    clip_begin((Rectangle){x, y, max_w, max_h});
    for (size_t i = 0; i < m->tasks.len; i += 1) {
        float task_y = m->tasks.data[i].mo.position[0] + m->scroll_mo.position[0];
        if (((task_y + task_h) < y) || ((task_y - y) >= max_h)) continue;
        Task* task = &m->tasks.data[i].task;

        if (m->tasks.data[i].hidden && !(m->flags & TASK_LIST_FLAG_SHOW_HIDDEN)) continue;
        Task_Return ret = task_draw(task, x, task_y, max_w, bounds, !m->tasks.data[i].hidden && (opts & DRAW_ENABLE_MOUSE_INPUT));
        if (m->tasks.data[i].hidden) {
            float icon_x = x + max_w * .5;
            float icon_y = task_y + task_height() * .5;
            Texture tex = icon_get(ICON_VISIBILY_OFF);
            Rectangle src = {0, 0, tex.width, tex.height};
            float icon_size = BTN_ICON_SIZE * 2.;
            Rectangle dst = {icon_x - icon_size * .5, icon_y - icon_size * .5, icon_size, icon_size};
            DrawTexturePro(tex, src, dst, (Vector2){0}, 0, WHITE);
        }

        switch (ret) {
            case TASK_NONE: break;
            case TASK_MOVE_UP: move_task_up(m, i); break;
            case TASK_MOVE_TOP: move_task_top(m, i); break;
            case TASK_DELETE: {
                size_t delete_idx = m->tasks.data[i].display_index;
                for (size_t ii = 0; ii < m->tasks.len; ii += 1) {
                    if (m->tasks.data[ii].display_index > delete_idx) m->tasks.data[ii].display_index -= 1;
                }
                m->tasks.data[i].display_index = m->tasks.len - 1;
                m->tasks.data[i].hidden = 1;

                result.related_task = task;
                result.related_task_event = TASK_EVENT_DELETE;
            } break;
            case TASK_MARK_DONE: {
                if (task->done < task->left) {
                    result.related_task = task;
                    result.related_task_event = TASK_EVENT_MARK_DONE;
                }
            } break;
        }
    }
    clip_end();

    Color grad_col1 = GET_RCOLOR(COLOR_CRUST);
    Color grad_col0 = grad_col1;
    grad_col0.a = 0x00;
    DrawRectangleGradientV(x, y + max_h * 0.75f, max_w, max_h * .25f, grad_col0, grad_col1);
    return result;
}

Task* task_list_get(Task_List* m, size_t idx) {
    assert(idx < m->tasks.len);
    return &m->tasks.data[idx].task;
}
