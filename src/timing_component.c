#include "timing_component.h"

#include <math.h>
#include <rlgl.h>

#include "button.h"
#include "chrono.h"
#include "clip.h"
#include "colors.h"
#include "config.h"
#include "fieldfusion.h"
#include "motion.h"
#include "raylib.h"

#define ALARM_PATH "resources/sound/alarm.mp3"
#define ALARM_VOL 0.05f

static void set_current_time_target_secs(Timing_Component *m) {
    switch (m->pomo) {
        case TC_POMO_STATE_FOCUS:
            m->current_time_target_secs = min_to_sec(g_cfg.clock_focus_mins) + g_cfg.clock_focus_secs;
            break;
        case TC_POMO_STATE_REST:
            m->current_time_target_secs = min_to_sec(g_cfg.clock_rest_mins) + g_cfg.clock_rest_secs;
            break;
    }
    m->chrono.secs_left = m->current_time_target_secs;
}

void timing_component_create(Timing_Component *m) {
    memset(m, 0, sizeof(*m));
    m->alarm = LoadSound(ALARM_PATH);
    m->interrupt = btn_create();
    m->skip = btn_create();
    btn_set_label(&m->interrupt, "start");
    btn_set_label(&m->skip, "skip");
    m->state = TC_STATE_PAUSED;
    m->pomo = TC_POMO_STATE_FOCUS;
    m->mo = motion_new();
    m->mo.f = 0.8f;
    m->mo.z = 1.5f;
    m->previous_chrono_secs_left = 0;
    set_current_time_target_secs(m);
    SetSoundVolume(m->alarm, ALARM_VOL);
}

void timing_component_destroy(Timing_Component *m) {
    ff_glyph_vec_destroy(&m->glyphs);
    UnloadSound(m->alarm);
    btn_destroy(&m->interrupt);
    btn_destroy(&m->skip);
}

static void handle_buttons(Timing_Component *m, float lbtn_x, float lbtn_y, float rbtn_x, float rbtn_y) {
    if (btn_draw(&m->interrupt, lbtn_x, lbtn_y)) {
        switch (m->state) {
            case TC_STATE_RUNNING: {
                btn_set_label(&m->interrupt, "pause");
                m->state = TC_STATE_PAUSED;
            } break;
            case TC_STATE_FINISHED:
            case TC_STATE_PAUSED: {
                btn_set_label(&m->interrupt, "pause");
                m->state = TC_STATE_RUNNING;
            } break;
        }
        StopSound(m->alarm);
    }

    btn_draw(&m->skip, rbtn_x, rbtn_y);
}

float timing_component_glyphs_width(Timing_Component *m) {
    size_t mins = chrono_clock_mins(&m->chrono);
    size_t secs = chrono_clock_secs(&m->chrono);
    char time_numbers[16] = {0};
    snprintf(time_numbers, 15, "%02lu:%02lu", mins, secs);
    size_t time_number_len = strlen(time_numbers);
    return ff_measure_utf8(time_numbers, time_number_len, g_cfg.bstyle).width;
}

void timing_component_draw_glyphs(Timing_Component *m, float x, float y) {
    size_t mins = chrono_clock_mins(&m->chrono);
    size_t secs = chrono_clock_secs(&m->chrono);
    char time_numbers[16] = {0};
    snprintf(time_numbers, 15, "%02lu:%02lu", mins, secs);
    size_t time_number_len = strlen(time_numbers);
    ff_draw_str8(time_numbers, time_number_len, x, y, (float *)g_cfg.global_projection, g_cfg.bstyle);
}

// void timing_component_draw_note_glyphs(Timing_Component *m,
//                                        float x, float y) {
//     struct ff_print print = g_cfg.c_clock_print;
//     print.typography.size = 13;
//     print.typography.color =
//     g_color[g_cfg.theme][color_name_subtext1];

//     char notes[128] = {0};
//     snprintf(notes, 127, "focus counter: %ld", m->focused_count);
//     size_t notes_len = strlen(notes);
//     ff_draw_imediatly_utf8(print, notes, notes_len,
//                            (float *)g_cfg.global_projection, x, y);
// }

void on_rest_finish(Timing_Component *m) {
    m->rested_count += 1;
    m->pomo = TC_POMO_STATE_FOCUS;
    set_current_time_target_secs(m);
}

void on_focus_finish(Timing_Component *m) {
    m->focused_count += 1;
    m->pomo = TC_POMO_STATE_REST;
    set_current_time_target_secs(m);
}

void on_chrono_done(Timing_Component *m, TC_Fin *fin) {
    m->state = TC_STATE_FINISHED;
    btn_set_label(&m->interrupt, "start");
    if (m->pomo == TC_POMO_STATE_REST) {
        on_rest_finish(m);
        *fin = TC_FIN_REST;
    } else {
        on_focus_finish(m);
        *fin = TC_FIN_FOCUS;
    }
    m->perc_done = 1.0f;
    PlaySound(m->alarm);
}

float timing_component_width(Timing_Component *m) {
    float lbtn_width = btn_width(&m->interrupt);
    float rbtn_width = btn_width(&m->skip);
    float chrn_width = timing_component_glyphs_width(m);
    float width = g_cfg.outer_gap * 6 + fmaxf(lbtn_width + rbtn_width + g_cfg.inner_gap, chrn_width);
    return width;
}

float timing_component_height(Timing_Component *m) {
    float lbtn_height = btn_height(&m->interrupt);
    float chrn_height = g_cfg.bstyle.typo.size;
    float height = g_cfg.outer_gap2 + g_cfg.inner_gap2 + lbtn_height + chrn_height;
    return height;
}

TC_Return timing_component_draw(Timing_Component *m, float x, float y, float max_width) {
    TC_Return ret = {0};

    if (m->state == TC_STATE_RUNNING) {
        m->previous_chrono_secs_left = m->chrono.secs_left;
        Chrono_Stat stat = chrono_update(&m->chrono);
        float delta = m->previous_chrono_secs_left - m->chrono.secs_left;
        ret.spent_delta = fabs(delta);
        float elapsed = (float)m->current_time_target_secs - m->chrono.secs_left;
        m->perc_done = (float)elapsed / (float)m->current_time_target_secs;
        if (stat == CHRONO_DONE) on_chrono_done(m, &ret.finished);
    }

    float lbtn_width = btn_width(&m->interrupt);
    float lbtn_height = btn_height(&m->interrupt);
    float rbtn_width = btn_width(&m->skip);
    float chrn_width = timing_component_glyphs_width(m);
    float chrn_height = g_cfg.bstyle.typo.size;

    float width = g_cfg.outer_gap * 6 + fmaxf(lbtn_width + rbtn_width + g_cfg.inner_gap, chrn_width);
    float height = g_cfg.outer_gap2 + g_cfg.inner_gap2 + lbtn_height + chrn_height;
    float rad = RADIUS_TO_ROUNDNESS(g_cfg.bg_radius, height);
    Rectangle bg = {x, y, MAX(width, max_width), height};
    DRAW_BG(bg, g_cfg.bg_radius, COLOR_BASE);

    float lbtn_x = x + width * .5f - lbtn_width * .5f - g_cfg.inner_gap * .5f - rbtn_width * .5f;
    float rbtn_x = x + width * .5f - rbtn_width * .5f + g_cfg.inner_gap * .5f + lbtn_width * .5f;

    float chrn_x = x + width * .5f - chrn_width * .5f;
    float chrn_y = y + g_cfg.outer_gap;

    float lbtn_y = chrn_y + chrn_height + g_cfg.inner_gap2;
    float rbtn_y = lbtn_y;

    timing_component_draw_glyphs(m, chrn_x, chrn_y);
    handle_buttons(m, lbtn_x, lbtn_y, rbtn_x, rbtn_y);
    // timing_component_draw_note_glyphs(m, x + width * .55f,
    //                                   y + height - 20);
    float target[2] = {width * m->perc_done, 0};
    clip_begin_rounded(bg.x, bg.y, bg.width, bg.height, rad);
    motion_update(&m->mo, target, GetFrameTime());
    Color bar_col = GET_RCOLOR(COLOR_SKY);
    DrawRectangle(x, y + height - 10, m->mo.position[0], 10, bar_col);
    clip_end();

    return ret;
}
