#pragma once
#include <fieldfusion.h>

#include "button.h"
#include "chrono.h"
#include "motion.h"
#include "raylib.h"

typedef enum {
    PTIMER_STATE_FINISHED,
    PTIMER_STATE_PAUSED,
    PTIMER_STATE_RUNNING,
} PTimer_State;

typedef enum {
    PTIMER_PSTATE_FOCUS,
    PTIMER_PSTATE_REST,
} PTimer_PSTATE;

typedef enum {
    PTIMER_FIN_0,
    PTIMER_FIN_FOCUS,
    PTIMER_FIN_REST,
} PTimer_Fin;

typedef struct {
    double spent_delta;
    PTimer_Fin finished;
} PTimer_Return;

typedef struct {
    Sound alarm;
    PTimer_State state;
    PTimer_PSTATE pomo;
    FF_Glyph_Vec glyphs;
    Chrono chrono;
    Btn interrupt;
    Btn skip;
    size_t focused_count;
    size_t rested_count;
    size_t skipped_count;
    U8 current_time_target_secs;
    float perc_done;
    Motion mo;
    double previous_chrono_secs_left;
} PTimer;

void ptimer_create(PTimer* m);
void ptimer_destroy(PTimer* m);
PTimer_Return ptimer_draw(PTimer* m, float x, float y, float max_width);
float ptimer_radius(PTimer* m);
