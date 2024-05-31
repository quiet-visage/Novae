/*
FIX: time activity graph shows too many indicators
on the x axis, it shows 4 even when the graph has only 2
data points
FIX: rename max_width functions to min_width functions
because that's what it should be
*/

#include <assert.h>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <stdio.h>

#include "clip.h"
#include "colors.h"
#include "config.h"
#include "date_time_view.h"
#include "db.h"
#include "db_cache.h"
#include "draw_opts.h"
#include "fieldfusion.h"
#include "heatmap.h"
#include "icon.h"
#include "pchart.h"
#include "shader.h"
#include "streak.h"
#include "task.h"
#include "task_creator.h"
#include "task_list.h"
#include "time_activity_graph.h"
#include "timing_component.h"

// clang-format off
#define EMBED_FILE(var_name, file_name)                              \
    __asm__("" #var_name ": .incbin \"" #file_name "\"");            \
    __asm__("" #var_name "_end: .byte 0");                           \
    __asm__("" #var_name "_len: .int " #var_name "_end - " #var_name \
            "")
EMBED_FILE(g_nova_mono_bytes, resources/fonts/NovaMono-Regular.ttf);
// clang-format on
#define BATCH_FLUSH_DELAY (0.75f)

extern const unsigned char g_nova_mono_bytes[];
extern const int g_nova_mono_bytes_len;
size_t g_nova_mono_font = {0};
Timing_Component g_timing_comp = {0};
Task_Creator g_task_creator = {0};
Task_List g_task_list = {0};
Task g_new_task = {0};
size_t g_tasks_count = {0};
float g_batch_flush_delay = BATCH_FLUSH_DELAY;

int cmp_t(const void *a, const void *b) {
    const Task *ap = a;
    const Task *bp = b;
    return (ap->done >= ap->left) - (bp->done >= bp->left);
}
int cmp_t0(const void *a, const void *b) {
    const float *ap = a;
    const float *bp = b;
    return *bp - *ap;
}

void main_init() {
    color_init();
    db_init();
    db_cache_init();
    config_init();
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_ALWAYS_RUN);
    InitWindow(g_cfg.window_width, g_cfg.window_height, g_cfg.window_name);
    InitAudioDevice();
    ff_initialize("430");
    SetTargetFPS(60);
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

    g_nova_mono_font =
        ff_new_load_font_from_memory(g_nova_mono_bytes, g_nova_mono_bytes_len, novae_config);

    g_cfg.mstyle.typo.font = g_nova_mono_font;
    g_cfg.bstyle.typo.font = g_nova_mono_font;
    g_cfg.estyle.typo.font = g_nova_mono_font;
    g_cfg.sstyle.typo.font = g_nova_mono_font;

    g_task_creator = task_creator_create();
    g_task_list = task_list_create();
    g_new_task = task_create();
    size_t todays_task_count = db_get_todays_task_count();
    timing_component_create(&g_timing_comp);
    if (todays_task_count) {
        task_list_prealloc(&g_task_list, todays_task_count);
        db_get_todays_task(g_task_list.tasks.data, 0);
        g_task_list.tasks.len += todays_task_count;
        qsort(g_task_list.tasks.data, g_task_list.tasks.len, sizeof(g_task_list.tasks.data[0]),
              cmp_t);
    }
};

void main_terminate() {
    db_cache_terminate();
    db_terminate();
    ff_terminate();
    icon_terminate();
    streak_terminate();
    shader_terminate();
    task_list_destroy(&g_task_list);
    task_destroy(&g_new_task);
    task_creator_destroy(&g_task_creator);
    timing_component_destroy(&g_timing_comp);
    CloseAudioDevice();
    CloseWindow();
}

static inline void begin_frame(void) {
    ff_get_ortho_projection(0, GetScreenWidth(), GetScreenHeight(), 0, -1.0f, 1.0f,
                            g_cfg.global_projection);
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
    g_new_task.db_id = db_create_task(task_name, g_new_task.done, g_new_task.left);

    task_list_push(&g_task_list, g_new_task);
    g_new_task = task_create();
    qsort(g_task_list.tasks.data, g_task_list.tasks.len, sizeof(g_task_list.tasks.data[0]), cmp_t);
}

static void synchronize_task_time_spent(Task *priority_task, TC_Return timing_comp_ret) {
    if (!timing_comp_ret.spent_delta)
        db_batch_incr_time(priority_task->db_id, GetFrameTime(), INCR_TIME_SPENT_IDLE);
    if (g_timing_comp.pomo == TC_POMO_STATE_FOCUS)
        db_batch_incr_time(priority_task->db_id, timing_comp_ret.spent_delta,
                           INCR_TIME_SPENT_FOCUS);
    if (g_timing_comp.pomo == TC_POMO_STATE_REST)
        db_batch_incr_time(priority_task->db_id, timing_comp_ret.spent_delta, INCR_TIME_SPENT_REST);
}

void main_loop() {
    while (!WindowShouldClose()) {
        db_cache_auto_sync();
        begin_frame();

        float timing_comp_width = timing_component_width(&g_timing_comp);
        float timing_comp_x = CENTER(0, GetScreenWidth(), timing_comp_width);
        TC_Return timing_comp_ret =
            timing_component_draw(&g_timing_comp, timing_comp_x, g_cfg.inner_gap);

        float offset_y =
            g_cfg.inner_gap + timing_component_height(&g_timing_comp) + g_cfg.inner_gap;
        bool should_add_task = task_creator_draw(&g_task_creator, &g_new_task, timing_comp_x,
                                                 offset_y, timing_comp_width, 1);
        if (should_add_task) add_task();

        offset_y += task_creator_height() + g_cfg.inner_gap;
        assert(GetScreenHeight() > offset_y);

        float task_list_max_height = 800 - offset_y;
        Changed_Task changed_task =
            task_list_draw(&g_task_list, timing_comp_x, offset_y, timing_comp_width,
                           task_list_max_height, DRAW_ENABLE_SCROLL);
        if (changed_task) {
            db_set_completed(changed_task->db_id);
            db_set_done(changed_task->db_id, changed_task->done);
        }
        if (g_task_list.tasks.len) {
            Task *task = &g_task_list.tasks.data[0];
            synchronize_task_time_spent(task, timing_comp_ret);
            if (timing_comp_ret.finished == TC_FIN_FOCUS) {
                db_incr_done(task->db_id);
                task->done += 1;
                if (task->done == task->left) db_set_completed(task->db_id);
                qsort(g_task_list.tasks.data, g_task_list.tasks.len,
                      sizeof(g_task_list.tasks.data[0]), cmp_t);
            }
        }

        float date_time_x = timing_comp_x + timing_comp_width + g_cfg.inner_gap;
        float date_time_y = g_cfg.inner_gap;
        date_time_view(date_time_x, g_cfg.inner_gap, g_timing_comp.chrono.secs_left);

        float pchart_w = pchart_max_width();
        float pchart_x = timing_comp_x - pchart_w - g_cfg.inner_gap;
        float pchart_y = g_cfg.inner_gap;
        pchart_draw(pchart_x, g_cfg.inner_gap);

        float streak_x = pchart_x + pchart_w * .5f;
        float streak_y = g_cfg.inner_gap + pchart_max_height() * .5f;
        streak_draw(streak_x, streak_y);

        float time_activity_x = timing_comp_x - time_activity_graph_max_width() - g_cfg.inner_gap;
        float time_activity_y = pchart_max_height() + pchart_y + g_cfg.inner_gap;
        time_activity_graph_draw(time_activity_x, time_activity_y);

        float heatmap_x = date_time_x;
        heatmap_draw(heatmap_x, date_time_y + date_time_max_height() + g_cfg.inner_gap);

        if (IsKeyPressed(KEY_F5)) {
            shader_reload();
        }
        end_frame();
    }
}

int main(void) {
    config_init();
    color_init();
    db_init();
    int id0 = db_create_tag("work0", g_color[g_cfg.theme][COLOR_TEAL]);
    int id1 = db_create_tag("work1", g_color[g_cfg.theme][COLOR_TEAL]);

    size_t tags_len = db_get_tag_count();
    Tag tags[tags_len];
    db_get_tags(tags);
    for (size_t i = 0; i < tags_len; i += 1) {
        Tag *tag = &tags[i];
        printf("id %d\n", tag->id);
        printf("name %s\n", tag->name);
        printf("color 0x%08x\n\n", tag->color);
        free(tag->name);
    }

    db_delete_tag(id0);
    db_delete_tag(id1);
    db_terminate();
    // SetTraceLogLevel(LOG_NONE);
    // main_init();
    // main_loop();
    // main_terminate();
    return 0;
}
