#pragma once
#include "fieldfusion.h"
#include "motion.h"

void horizontal_scroll(const C32* buf, size_t len, size_t curs,
                       float* target_scroll, size_t font, size_t size,
                       float max_w);
void get_text_projection(float out[4][4], Motion* mo);
float get_cursor_x(const C32* buf, size_t len, FF_Typo typo,
                   size_t cursor_pos, float x);
void update_scroll_animation(Motion* motion, float target_motion_x,
                             float target_motion_y);
