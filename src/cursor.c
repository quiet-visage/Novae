#include "cursor.h"

#include <math.h>
#include <raylib.h>

#include "alpha_inherit.h"
#include "config.h"
#include "motion.h"
#include "raymath.h"

Cursor cursor_new() {
    Cursor result = {0};
    result.motion.f = 4.5f;
    result.motion.z = 1.0f;
    result.motion.r = -1.0f;
    // result.smear_motion = result.motion;
    // result.smear_motion.f += .05f;
    return result;
}

static inline float cursor_compute_phase(void) { return fabs(cosf(GetTime() * 2)) * .7f; }

static void cursor_handle_alplha_change(Cursor* this) {
    if (this->flags & CURSOR_FLAG_FOCUSED) {
        this->max_alpha = 0xff;
        this->alpha = this->max_alpha;
    } else {
        this->max_alpha = 0x30;
        this->alpha = this->max_alpha;
    }

    if (this->flags & CURSOR_FLAG_RECENTLY_MOVED) {
        this->flags &= ~CURSOR_FLAG_RECENTLY_MOVED;
        this->flags |= _CURSOR_FLAG_BLINK_DELAY;
        this->flags &= ~_CURSOR_FLAG_BLINK;
        this->blink_delay_ms = 1e3;
        this->alpha = 0xff;
    }

    if (this->flags & _CURSOR_FLAG_BLINK_DELAY) {
        this->blink_delay_ms -= GetFrameTime() * 1e3;
        if (this->blink_delay_ms <= 0 && (cursor_compute_phase() - .7f) >= -.01f) {
            this->blink_duration_ms = 8 * 1e3;
            this->flags &= ~_CURSOR_FLAG_BLINK_DELAY;
            this->flags |= _CURSOR_FLAG_BLINK;
        }
    }

    if (this->flags ^ _CURSOR_FLAG_BLINK_DELAY && this->flags & _CURSOR_FLAG_BLINK) {
        float phase = cursor_compute_phase();
        this->alpha = phase * this->max_alpha;
        this->blink_duration_ms -= GetFrameTime() * 1e3;
        if (this->blink_duration_ms <= 0 && .7f - phase < .1f) {
            this->flags &= ~_CURSOR_FLAG_BLINK;
            this->alpha = 0xff;
        }
    }
}

void cursor_draw(Cursor* this, float x, float y) {
    cursor_handle_alplha_change(this);

    motion_update(&this->motion, (float[2]){x, y}, GetFrameTime());
    Color color = GET_RCOLOR(COLOR_TEXT);
    color.a = MIN(this->alpha, alpha_inherit_get_alpha());
    DrawRectangleRec(
        (Rectangle){.x = this->motion.position[0], .y = this->motion.position[1], .width = 2.0f, .height = 14.f},
        color);
}

inline void cursor_set_focused(Cursor* m, bool on) {
    if (on)
        m->flags |= CURSOR_FLAG_FOCUSED;
    else
        m->flags &= ~CURSOR_FLAG_FOCUSED;
}

inline void cursor_recently_moved(Cursor* m) { m->flags |= CURSOR_FLAG_RECENTLY_MOVED; }
