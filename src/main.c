#include <assert.h>
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <stdio.h>

#include "clip.h"
#include "colors.h"
#include "config.h"
#include "db.h"
#include "db_activity_map.h"
#include "draw_opts.h"
#include "fieldfusion.h"
#include "heatmap.h"
#include "icon.h"
#include "pchart.h"
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

extern const unsigned char g_nova_mono_bytes[];
extern const int g_nova_mono_bytes_len;
size_t g_nova_mono_font;
Shader g_bloom;

// TODO: DRAW MOVE UP BUTTON BACKGROUND
// TODO: REMOVE COMPARISONS, tasklist by priority

int cmp_t(const void* a, const void* b) {
    const Task* ap = a;
    const Task* bp = b;
    return (ap->done >= ap->left) - (bp->done >= bp->left);
}
int cmp_t0(const void* a, const void* b) {
    const float* ap = a;
    const float* bp = b;
    return *bp - *ap;
}

void main_init() {
    db_init();
    db_activity_init();
    config_init();
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_ALWAYS_RUN);
    InitWindow(g_cfg.window_width, g_cfg.window_height,
               g_cfg.window_name);
    InitAudioDevice();
    ff_initialize("430");
    SetTargetFPS(60);
    icon_init();
    clip_init();
    g_bloom = LoadShader(0, "shaders/bloom.fs");
    streak_init(g_bloom);

    FF_Font_Config novae_config = {
        .scale = 1.8f,
        .range = 1.6f,
        .texture_width = 1024,
        .texture_padding = 4,
    };

    g_nova_mono_font = ff_new_load_font_from_memory(
        g_nova_mono_bytes, g_nova_mono_bytes_len, novae_config);
    g_cfg.btn_typo.font = g_nova_mono_font;
    g_cfg.c_clock_typo.font = g_nova_mono_font;
};

void main_terminate() {
    db_acitivity_terminate();
    db_terminate();
    ff_terminate();
    icon_terminate();
    streak_terminate();
    CloseWindow();
}

static inline void begin_frame(void) {
    ff_get_ortho_projection(0, GetScreenWidth(), GetScreenHeight(), 0,
                            -1.0f, 1.0f, g_cfg.global_projection);
    ClearBackground(GetColor(g_color[g_cfg.theme][COLOR_CRUST]));
    BeginDrawing();
}

static inline void end_frame(void) {
    clip_end_frame();
    EndDrawing();
}

void main_loop() {
    Timing_Component tc;
    Task_Creator ts_c = task_creator_create();
    Task new_task = task_create();
    size_t tasks_n = 0;
    Task_List tl = task_list_create();

    size_t todays_task_count = db_get_todays_task_count();
    if (todays_task_count) {
        task_list_prealloc(&tl, todays_task_count);
        db_get_todays_task(tl.tasks.data, 0);
        tl.tasks.len += todays_task_count;
        qsort(tl.tasks.data, tl.tasks.len, sizeof(tl.tasks.data[0]),
              cmp_t);
    }

    timing_component_create(&tc);

    // Shader horz_blur = LoadShader(0, "shaders/horz_blur.fs");
    // Shader vert_blur = LoadShader(0, "shaders/vert_blur.fs");
    // Shader mix = LoadShader(0, "shaders/mix.fs");
    // RenderTexture2D t1 = LoadRenderTexture(200, 200);
    // // RenderTexture2D t2 = LoadRenderTexture(200, 200);
    // // RenderTexture2D t3 = LoadRenderTexture(200, 200);
    // Image strk_effect_img = GenImageColor(200, 200, BLANK);
    // Texture strk_tex = LoadTextureFromImage(strk_effect_img);

    // // int tex1loc = GetShaderLocation(mix, "texture1");

    // float deg = 0;

    while (!WindowShouldClose()) {
        // deg = GetTime() * 100;
        // sin(x/360+t*1.5)*360+360
        db_activity_auto_sync();
        begin_frame();

        float tc_width = timing_component_width(&tc);
        float x = GetScreenWidth() * .5f - tc_width * .5f;
        TC_Return tc_ret =
            timing_component_draw(&tc, x, g_cfg.outer_gap);
        float y = g_cfg.outer_gap + timing_component_height(&tc) +
                  g_cfg.inner_gap;

        bool add_task =
            task_creator_draw(&ts_c, &new_task, x, y, tc_width, 1);
        if (add_task && tasks_n < 8) {
            char task_name[new_task.name_len + 1];
            task_name[new_task.name_len] = 0;
            ff_utf32_to_utf8(task_name, new_task.name,
                             new_task.name_len);
            new_task.db_id = db_create_task(task_name, new_task.done,
                                            new_task.left);

            task_list_push(&tl, new_task);
            new_task = task_create();
            qsort(tl.tasks.data, tl.tasks.len,
                  sizeof(tl.tasks.data[0]), cmp_t);
        }

        y += task_creator_height() + g_cfg.inner_gap;
        assert(GetScreenHeight() > y);
        float max_height = 800 - y;
        Changed_Task changed_task = task_list_draw(
            &tl, x, y, tc_width, max_height, DRAW_ENABLE_SCROLL);
        if (changed_task) {
            db_set_completed(changed_task->db_id);
            db_set_done(changed_task->db_id, changed_task->done);
        }
        if (tl.tasks.len) {
            Task* task = &tl.tasks.data[0];
            if (tc_ret.spent_delta) {
                if (tc.pomo == TC_POMO_STATE_FOCUS)
                    db_incr_time(task->db_id, tc_ret.spent_delta,
                                 INCR_TIME_SPENT_FOCUS);
                if (tc.pomo == TC_POMO_STATE_REST)
                    db_incr_time(task->db_id, tc_ret.spent_delta,
                                 INCR_TIME_SPENT_REST);
            }
            if (tc_ret.finished == TC_FIN_FOCUS) {
                db_incr_done(task->db_id);
                task->done += 1;
                if (task->done == task->left) {
                    db_set_completed(task->db_id);
                }
                qsort(tl.tasks.data, tl.tasks.len,
                      sizeof(tl.tasks.data[0]), cmp_t);
            }
        }
        heatmap_draw();
        time_activity_graph_draw(30, 350);
        streak_draw();

        // DrawCircle(100,100,40, WHITE);

        pchart_draw();
        // Vector2 line0_beg = {ring_pos.x + ring_out_rad * .5f,
        //                      ring_pos.y};
        // float orbit_rad = (ring_out_rad + ring_in_rad) * .5f;
        // line0_beg.x =
        //     ring_pos.x + cosf(ring_ang1 * .5f * DEG2RAD) *
        //     orbit_rad;
        // line0_beg.y =
        //     ring_pos.y + sinf(ring_ang1 * .5f * DEG2RAD) *
        //     orbit_rad;

        // Color col = GetColor(g_color[g_cfg.theme][COLOR_TEAL]);
        // if (line0_beg.x > ring_pos.x) {
        //     float line0_w = 20;
        //     Vector2 line0_end = {line0_beg.x + line0_w,
        //                          line0_beg.y - line0_w * .5f};
        //     DrawLineEx(line0_beg, line0_end, 1.0f, col);

        //     Vector2 line1_beg = {line0_end.x, line0_end.y};
        //     float line1_w = 90;
        //     Vector2 line1_end = {line0_end.x + line1_w,
        //     line0_end.y}; DrawLineEx(line1_beg, line1_end, 1.0f,
        //     col);
        // } else {
        //     float line0_w = 20;
        //     Vector2 line0_end = {line0_beg.x - line0_w,
        //                          line0_beg.y - line0_w * .5f};
        //     DrawLineEx(line0_beg, line0_end, 1.0f, col);

        //     Vector2 line1_beg = {line0_end.x, line0_end.y};
        //     float line1_w = 90;
        //     Vector2 line1_end = {line0_end.x - line1_w,
        //     line0_end.y}; DrawLineEx(line1_beg, line1_end, 1.0f,
        //     col);
        // }

        // DrawLine(100, 100, 120, 85, GREEN);
        // DrawLine(120, 85, 180, 85, GREEN);

        end_frame();
    }

    UnloadShader(g_bloom);

    task_list_destroy(&tl);
    task_destroy(&new_task);
    task_creator_destroy(&ts_c);
    timing_component_destroy(&tc);
    CloseAudioDevice();
}

int main(void) {
    // SetTraceLogLevel(LOG_NONE);
    main_init();
    main_loop();
    main_terminate();
    return 0;
}
