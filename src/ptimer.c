#include "ptimer.h"

#include <math.h>
#include <rlgl.h>

#include "button.h"
#include "chrono.h"
#include "clip.h"
#include "colors.h"
#include "config.h"
#include "fieldfusion.h"
#include "icon.h"
#include "motion.h"
#include "raylib.h"

#define ALARM_PATH "resources/sound/alarm.mp3"
#define ALARM_VOL 0.05f

static void set_current_time_target_secs(PTimer *m) {
    switch (m->pomo) {
        case PTIMER_PSTATE_FOCUS:
            m->current_time_target_secs = min_to_sec(g_cfg.clock_focus_mins) + g_cfg.clock_focus_secs;
            break;
        case PTIMER_PSTATE_REST:
            m->current_time_target_secs = min_to_sec(g_cfg.clock_rest_mins) + g_cfg.clock_rest_secs;
            break;
    }
    m->chrono.secs_left = m->current_time_target_secs;
}

static void draw_timer_progress(float cx, float cy, float radius, float ring_width, float perc) {
    const float nob_radius = ring_width;
    float ig = -202.5;
    float eg = ig + perc * 225.;
    DrawRing((Vector2){cx, cy}, radius, radius + ring_width, -202.5, 22.5, 64, GET_RCOLOR(COLOR_BASE));

    clip_begin_custom_shape();
    DrawRing((Vector2){cx, cy}, radius, radius + ring_width, ig, eg, 64, SKYBLUE);
    float d = eg - 180.;
    float r = radius + ring_width * .5;
    DrawCircle(r * cosf((d - 180) * DEG2RAD) + cx, r * sinf(-(d * DEG2RAD)) + cy, nob_radius, GET_RCOLOR(COLOR_SKY));
    clip_end_custom_shape();

    float tradius = radius + ring_width;
    float x = cx - tradius - nob_radius * .5;
    float y = cy - tradius - nob_radius * .5;
    float size = tradius * 2 + nob_radius;
    DrawRectangleGradientH(x, y, size, size, GET_RCOLOR(COLOR_BLUE), GET_RCOLOR(COLOR_GREEN));
    clip_end();
}

void ptimer_create(PTimer *m) {
    memset(m, 0, sizeof(*m));
    m->alarm = LoadSound(ALARM_PATH);
    m->interrupt = btn_create();
    m->skip = btn_create();
    m->state = PTIMER_STATE_PAUSED;
    m->pomo = PTIMER_PSTATE_FOCUS;
    m->mo = motion_new();
    m->mo.f = 0.8f;
    m->mo.z = 1.5f;
    m->previous_chrono_secs_left = 0;
    set_current_time_target_secs(m);
    SetSoundVolume(m->alarm, ALARM_VOL);
}

void ptimer_destroy(PTimer *m) {
    ff_glyph_vec_destroy(&m->glyphs);
    UnloadSound(m->alarm);
    btn_destroy(&m->interrupt);
    btn_destroy(&m->skip);
}

static void handle_buttons(PTimer *m, float lbtn_x, float lbtn_y, float rbtn_x, float rbtn_y) {
    if (btn_draw_with_icon(&m->interrupt, ICON_PLAY, lbtn_x, lbtn_y)) {
        switch (m->state) {
            case PTIMER_STATE_RUNNING: {
                // btn_set_label(&m->interrupt, "pause");
                m->state = PTIMER_STATE_PAUSED;
            } break;
            case PTIMER_STATE_FINISHED:
            case PTIMER_STATE_PAUSED: {
                // btn_set_label(&m->interrupt, "pause");
                m->state = PTIMER_STATE_RUNNING;
            } break;
        }
        StopSound(m->alarm);
    }

    btn_draw_with_icon(&m->skip, ICON_SKIP, rbtn_x, rbtn_y);
}

static float ptimer_text_width(PTimer *m) {
    size_t mins = chrono_clock_mins(&m->chrono);
    size_t secs = chrono_clock_secs(&m->chrono);
    char time_numbers[16] = {0};
    snprintf(time_numbers, 15, "%02lu:%02lu", mins, secs);
    size_t time_number_len = strlen(time_numbers);
    return ff_measure_utf8(time_numbers, time_number_len, g_cfg.bstyle).width;
}

float ptimer_radius(PTimer *m) { return ptimer_text_width(m) * .6 + g_cfg.outer_gap; }

float ptimer_view_text(PTimer *m, float x, float y) {
    size_t mins = chrono_clock_mins(&m->chrono);
    size_t secs = chrono_clock_secs(&m->chrono);
    char time_numbers[16] = {0};
    snprintf(time_numbers, 15, "%02lu:%02lu", mins, secs);
    size_t time_number_len = strlen(time_numbers);
    ff_draw_str8(time_numbers, time_number_len, x, y, (float *)g_cfg.global_projection, g_cfg.bstyle);
    float width = ff_measure_utf8(time_numbers, time_number_len, g_cfg.bstyle).width;
    return width;
}

void on_rest_finish(PTimer *m) {
    m->rested_count += 1;
    m->pomo = PTIMER_PSTATE_FOCUS;
    set_current_time_target_secs(m);
}

void on_focus_finish(PTimer *m) {
    m->focused_count += 1;
    m->pomo = PTIMER_PSTATE_REST;
    set_current_time_target_secs(m);
}

void on_chrono_done(PTimer *m, PTimer_Fin *fin) {
    m->state = PTIMER_STATE_FINISHED;
    // btn_set_label(&m->interrupt, "start");
    if (m->pomo == PTIMER_PSTATE_REST) {
        on_rest_finish(m);
        *fin = PTIMER_FIN_REST;
    } else {
        on_focus_finish(m);
        *fin = PTIMER_FIN_FOCUS;
    }
    m->perc_done = 1.0f;
    PlaySound(m->alarm);
}

float ptimer_width(PTimer *m) { return ptimer_text_width(m); }

float timing_component_height(PTimer *m) {
    float lbtn_height = btn_height(&m->interrupt);
    float chrn_height = g_cfg.bstyle.typo.size;
    float height = g_cfg.outer_gap2 + g_cfg.inner_gap2 + lbtn_height + chrn_height;
    return height;
}

PTimer_Return ptimer_draw(PTimer *m, float cx, float cy, float max_width) {
    PTimer_Return ret = {0};

    if (m->state == PTIMER_STATE_RUNNING) {
        m->previous_chrono_secs_left = m->chrono.secs_left;
        Chrono_Stat stat = chrono_update(&m->chrono);
        float delta = m->previous_chrono_secs_left - m->chrono.secs_left;
        ret.spent_delta = fabs(delta);
        float elapsed = (float)m->current_time_target_secs - m->chrono.secs_left;
        m->perc_done = (float)elapsed / (float)m->current_time_target_secs;
        if (stat == CHRONO_DONE) on_chrono_done(m, &ret.finished);
    }

    float text_width = ptimer_text_width(m);

    float target[2] = {m->perc_done, 0};
    motion_update(&m->mo, target, GetFrameTime());
    float prog_x = cx;
    float prog_y = cy;
    float prog_r = ptimer_radius(m);
    draw_timer_progress(prog_x, prog_y, prog_r, 8, m->mo.position[0]);

    float text_x = cx - text_width * .5;
    float text_y = cy - g_cfg.bstyle.typo.size * .5 - g_cfg.inner_gap - BTN_ICON_RADIUS * .5;
    ptimer_view_text(m, text_x, text_y);

    float by = cy + g_cfg.bstyle.typo.size * .5 + g_cfg.inner_gap;
    float lbtnx = cx - BTN_ICON_RADIUS - g_cfg.inner_gap * .5;
    Icon toggle_play_icon =
        m->state == PTIMER_STATE_PAUSED || m->state == PTIMER_STATE_FINISHED ? ICON_PLAY : ICON_PAUSE;
    bool toggle_play = btn_draw_icon(&m->interrupt, toggle_play_icon, lbtnx, by);
    float rbtnx = cx + BTN_ICON_RADIUS + g_cfg.inner_gap * .5;
    bool skip = btn_draw_icon(&m->skip, ICON_SKIP, rbtnx, by);

    if (toggle_play) {
        switch (m->state) {
            case PTIMER_STATE_RUNNING: {
                m->state = PTIMER_STATE_PAUSED;
            } break;
            case PTIMER_STATE_FINISHED:
            case PTIMER_STATE_PAUSED: {
                m->state = PTIMER_STATE_RUNNING;
            } break;
        }
        StopSound(m->alarm);
    }

    return ret;
}
