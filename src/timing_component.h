#pragma once
#include <fieldfusion.h>

#include "button.h"
#include "chrono.h"
#include "motion.h"
#include "raylib.h"

typedef enum {
    TC_STATE_FINISHED,
    TC_STATE_PAUSED,
    TC_STATE_RUNNING,
} TC_State;

typedef enum {
    TC_POMO_STATE_FOCUS,
    TC_POMO_STATE_REST,
} TC_Pomo_State;

typedef enum {
    TC_FIN_0,
    TC_FIN_FOCUS,
    TC_FIN_REST,
} TC_Fin;

typedef struct {
    double spent_delta;
    TC_Fin finished;
} TC_Return;

typedef struct {
    Sound alarm;
    TC_State state;
    TC_Pomo_State pomo;
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
} Timing_Component;

void timing_component_create(Timing_Component* m);
void timing_component_destroy(Timing_Component* m);
TC_Return timing_component_draw(Timing_Component* m, float x, float y);
float timing_component_width(Timing_Component* m);
float timing_component_height(Timing_Component* m);
