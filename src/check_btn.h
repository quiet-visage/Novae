#pragma once
#include "draw_opts.h"
#include "motion.h"

typedef struct {
    Motion check_anim;
    Motion uncheck_anim;
} Check_Btn;

Check_Btn check_btn_create();
bool check_btn_draw(Check_Btn* m, bool* checked, float x, float y, Draw_Opts opts);
