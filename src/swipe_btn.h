#pragma once
#include "icon.h"
#include "motion.h"

typedef struct {
    Motion bg_motion;
    Motion text_motion;
    int expand;
} Swipe_Btn;

Swipe_Btn swipe_btn_create(void);
int swipe_btn_view(Swipe_Btn* m, float x, float y, Icon icon_left, Icon icon_right, const char* text_left,
                   const char* text_right);
// void swipe_btn_destroy(void);
