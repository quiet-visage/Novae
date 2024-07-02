#pragma once

#include <sys/types.h>

#include "motion.h"

enum Cursor_Flags {
    CURSOR_FLAG_RECENTLY_MOVED = (1 << 0),
    CURSOR_FLAG_FOCUSED = (1 << 1),
    _CURSOR_FLAG_BLINK_DELAY = (1 << 2),
    _CURSOR_FLAG_BLINK = (1 << 3),
};

typedef struct {
    Motion motion;
    // motion_t smear_motion;
    float blink_duration_ms;
    float blink_delay_ms;
    unsigned char max_alpha;
    unsigned char alpha;
    unsigned char flags;
} Cursor;

void cursor_initialize(void);
void cursor_terminate(void);
void cursor_set_focused(Cursor* m, bool on);
void cursor_recently_moved(Cursor* m);
Cursor cursor_new(void);
void cursor_draw(Cursor* c, float x, float y);
