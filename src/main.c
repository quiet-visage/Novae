/*
TODO: center items on the left to the center of the screen vertically
TODO: if clicked on a hidden task, unhide it
NOTE: may be a great idea to implement a complete calendar o the right side
      with a scroll bar
    : statistics tab would be great on the left side
    : window titles?
    : minimal mode when in focus
TODO: on slide button choide fade in the text
    : hover tooltip
    : use gpu to draw the hue stuff
*/

#include <assert.h>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <stdio.h>

#include "blur.h"
#include "clip.h"
#include "colors.h"
#include "config.h"
#include "date_time_view.h"
#include "db.h"
#include "db_cache.h"
#include "fieldfusion.h"
#include "heatmap.h"
#include "hint.h"
#include "hsv.h"
#include "icon.h"
#include "pchart.h"
#include "ptimer.h"
#include "sdf_draw.h"
#include "shader.h"
#include "streak.h"
#include "tag_selection.h"
#include "task.h"
#include "task_creator.h"
#include "task_list.h"
#include "time_activity_graph.h"

// clang-format off
#define EMBED_FILE(var_name, file_name)                              \
    __asm__("" #var_name ": .incbin \"" #file_name "\"");            \
    __asm__("" #var_name "_end: .byte 0");                           \
    __asm__("" #var_name "_len: .int " #var_name "_end - " #var_name \
            "")
EMBED_FILE(g_nova_mono_bytes, resources/fonts/NovaMono-Regular.ttf);
// clang-format on
#define BATCH_FLUSH_DELAY (0.75f)

typedef enum {
    FOCUS_FLAG_TASK_CREATOR = (1 << 0),
    FOCUS_FLAG_TAG_SELECTION = (1 << 1),
} Focus_Flags;

extern const unsigned char g_nova_mono_bytes[];
extern const int g_nova_mono_bytes_len;
size_t g_nova_mono_font = {0};
PTimer g_timing_comp = {0};
Task_Creator g_task_creator = {0};
Task_List g_task_list = {0};
Task g_new_task = {0};
Task g_default_task = {0};
size_t g_tasks_count = {0};
float g_batch_flush_delay = BATCH_FLUSH_DELAY;
Tag_Selection g_tag_selection = {0};
Focus_Flags g_focus = FOCUS_FLAG_TASK_CREATOR;

void main_init() {
    config_init();
    color_init();

    db_init();
    db_cache_init();
    db_cache_sync_tags();

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_ALWAYS_RUN);
    InitWindow(g_cfg.window_width, g_cfg.window_height, g_cfg.window_name);
    InitAudioDevice();
    SetTargetFPS(60);
    SetExitKey(0);
    sdf_draw_init();
    hint_init();

    ff_initialize("430");
    icon_init();
    clip_init();
    streak_init();
    pchart_init();
    time_activity_graph_init();

    FF_Font_Config novae_config = {
        .scale = 1.8f,
        .range = 1.6f,
        .texture_width = 1024,
        .texture_padding = 4,
    };

    g_nova_mono_font = ff_new_load_font_from_memory(g_nova_mono_bytes, g_nova_mono_bytes_len, novae_config);

    g_cfg.mstyle.typo.font = g_nova_mono_font;
    g_cfg.bstyle.typo.font = g_nova_mono_font;
    g_cfg.estyle.typo.font = g_nova_mono_font;
    g_cfg.sstyle.typo.font = g_nova_mono_font;

    g_task_creator = task_creator_create();
    g_task_list = task_list_create();
    g_new_task = task_create();

    size_t todays_task_count = db_get_todays_task_count();
    ptimer_create(&g_timing_comp);
    if (todays_task_count) {
        Task *tasks = calloc(todays_task_count, sizeof(Task));
        db_get_todays_task(tasks);
        for (size_t i = 0; i < todays_task_count; i += 1) {
            if (db_is_default_task(tasks[i].db_id)) {
                g_default_task = tasks[i];
            } else {
                task_list_push(&g_task_list, tasks[i]);
            }
        }
        free(tasks);
    }

    g_tag_selection = tag_selection_create();
    blur_init();
};

void main_terminate() {
    db_cache_terminate();
    db_terminate();
    ff_terminate();
    tag_selection_destroy(&g_tag_selection);
    icon_terminate();
    streak_terminate();
    sdf_draw_terminate();
    shader_terminate();
    task_list_destroy(&g_task_list);
    task_destroy(&g_new_task);
    task_destroy(&g_default_task);
    task_creator_destroy(&g_task_creator);
    ptimer_destroy(&g_timing_comp);
    blur_terminate();
    CloseAudioDevice();
    CloseWindow();
}

static inline void begin_frame(void) {
    ff_get_ortho_projection(0, GetScreenWidth(), GetScreenHeight(), 0, -1.0f, 1.0f, g_cfg.global_projection);
    ClearBackground(GET_RCOLOR(COLOR_CRUST));
    BeginDrawing();
}

static inline void end_frame(void) {
    g_batch_flush_delay -= GetFrameTime();
    if (g_batch_flush_delay <= 0) {
        db_batch_flush();
        g_batch_flush_delay = BATCH_FLUSH_DELAY;
    }
    clip_end_frame();
    EndDrawing();
}

static void add_task() {
    char task_name[g_new_task.name_len + 1];
    task_name[g_new_task.name_len] = 0;
    ff_utf32_to_utf8(task_name, g_new_task.name, g_new_task.name_len);
    g_new_task.db_id =
        db_create_task(task_name, g_new_task.done, g_new_task.left, tag_selection_get_selected(&g_tag_selection)->id);
    g_new_task.tag_id = tag_selection_get_selected(&g_tag_selection)->id;
    task_list_push(&g_task_list, g_new_task);
    g_new_task = task_create();
}

static void synchronize_task_time_spent(Task *priority_task, PTimer_Return timing_comp_ret) {
    if (!timing_comp_ret.spent_delta) db_batch_incr_time(priority_task->db_id, GetFrameTime(), INCR_TIME_SPENT_IDLE);
    if (g_timing_comp.pomo == PTIMER_PSTATE_FOCUS)
        db_batch_incr_time(priority_task->db_id, timing_comp_ret.spent_delta, INCR_TIME_SPENT_FOCUS);
    if (g_timing_comp.pomo == PTIMER_PSTATE_REST)
        db_batch_incr_time(priority_task->db_id, timing_comp_ret.spent_delta, INCR_TIME_SPENT_REST);
}

static void handle_tag_selection(float x, float y) {
    Tag *tag = tag_selection_view(&g_tag_selection, x, y, 1, g_focus & FOCUS_FLAG_TAG_SELECTION);
    if (g_tag_selection.state == TAG_SELECTION_STATE_OPEN) g_focus = FOCUS_FLAG_TAG_SELECTION;

    if (tag) {
        g_focus = FOCUS_FLAG_TASK_CREATOR;
        if (tag != (Tag *)-1) g_new_task.tag_id = tag->id;
    }
}

void main_loop() {
    while (!WindowShouldClose()) {
        db_cache_auto_sync();
        begin_frame();

        float timing_comp_width = .25f * GetScreenWidth();
        float timing_comp_x = GetScreenWidth() * .5;
        float tcy = ptimer_radius(&g_timing_comp) + g_cfg.inner_gap + 10.;
        PTimer_Return timing_comp_ret = ptimer_draw(&g_timing_comp, timing_comp_x, tcy, timing_comp_width);

        float offset_y = g_cfg.outer_gap + tcy * 2;
        float task_creator_x = CENTER(0, GetScreenWidth(), timing_comp_width);
        Task_Creator_Ret task_creator_ret = task_creator_draw(&g_task_creator, &g_new_task, task_creator_x, offset_y,
                                                              timing_comp_width, g_focus & FOCUS_FLAG_TASK_CREATOR);
        if (task_creator_ret.create) add_task();

        offset_y += task_creator_height() + g_cfg.inner_gap;
        assert(GetScreenHeight() > offset_y);

        float task_list_max_height = g_cfg.window_height - offset_y;
        Task_List_Return task_list_return =
            task_list_view(&g_task_list, task_creator_x, offset_y, timing_comp_width, task_list_max_height,
                           (g_focus & FOCUS_FLAG_TASK_CREATOR ? 0b111 : 0));
        if (task_list_return.related_task_event) {
            assert(task_list_return.related_task);
            if (task_list_return.related_task_event == TASK_EVENT_MARK_DONE) {
                task_list_return.related_task->done = task_list_return.related_task->left;
                db_set_done(task_list_return.related_task->db_id, task_list_return.related_task->done);
                db_set_completed(task_list_return.related_task->db_id);
            } else if (task_list_return.related_task_event == TASK_EVENT_DELETE) {
                // TODO: hide instead of delete
            }
        }

        Task *task = 0;
        if (g_task_list.tasks.len)
            task = task_list_get_prioritized(&g_task_list);
        else
            task = &g_default_task;
        synchronize_task_time_spent(task, timing_comp_ret);
        if (timing_comp_ret.finished == PTIMER_FIN_FOCUS) {
            db_incr_done(task->db_id);
            task->done += 1;
            if (task->done == task->left) db_set_completed(task->db_id);
        }

        float date_time_w = date_time_width();
        float date_time_x = GetScreenWidth()-g_cfg.inner_gap-date_time_w;
        float date_time_y = g_cfg.inner_gap;
        // DrawRectangleLines(date_time_x,date_time_y,date_time_w,100,WHITE);
        date_time_view_all(date_time_x, date_time_y, g_timing_comp.chrono.secs_left);

        {
            float w = pchart_max_width();
            float x = g_cfg.inner_gap;
            float y = g_cfg.inner_gap;
            pchart_draw(x, y);

            float streak_x = x + w * .5f;
            float streak_y = g_cfg.inner_gap + pchart_max_height() * .5f;
            streak_draw(streak_x, streak_y);

            float time_activity_x = g_cfg.inner_gap;
            float time_activity_y = pchart_max_height() + y + g_cfg.inner_gap;
            time_activity_graph_draw(time_activity_x, time_activity_y);
        }
        // float heatmap_x = date_time_x;
        // heatmap_draw(heatmap_x, date_time_y + date_time_max_height() + g_cfg.inner_gap);

        // if (IsKeyPressed(KEY_F5)) {
        //     shader_reload();
        // }
        // // handle_tag_selection(task_creator_ret.tag_sel_x, task_creator_ret.tag_sel_y);

        /*
                Rectangle r1 = {100, 100, 400, 400};
                Rectangle r2 = {90, 95, 400, 400};
                Rectangle rr0 = {100, 100, 200, 200};
                Rectangle rr1 = {190, 90, 200, 100};
                Rectangle rr2 = {340, 90, 200, 100};
                DrawRectangleLines(r1.x, r1.y, r1.width, r1.height, RED);
                DrawRectangleLines(r2.x, r2.y, r2.width, r2.height, BLUE);
                DrawRectangleLines(rr0.x, rr0.y, rr0.width, rr0.height, GREEN);
                DrawRectangleLines(rr1.x, rr1.y, rr1.width, rr1.height, YELLOW);
                DrawRectangleLines(rr2.x, rr2.y, rr2.width, rr2.height, MAGENTA);

                clip_begin(r1);
                clip_begin(r2);

                clip_begin(rr0);
                DrawCircle(rr0.x, rr0.y, 128, GREEN);
                clip_end();

                clip_begin(rr1);
                Color c = YELLOW;
                c.a = 0xab;
                DrawCircle(rr1.x, rr1.y, 150, c);
                clip_end();

                clip_begin(rr2);
                c = MAGENTA;
                c.a = 0xab;
                DrawCircle(rr2.x, rr2.y, 150, c);
                clip_end();

                clip_end();
                clip_end();
        */
        hint_end_frame();
        end_frame();
    }
}

int main(void) {
    main_init();
    main_loop();
    main_terminate();
    return 0;
}
