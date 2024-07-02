#pragma once

#include <raylib.h>

void sdf_draw_init(void);
void sdf_draw_terminate(void);
void sdf_draw_rounded_rectangle(float x, float y, float w, float h, float roundness, Color color);
void sdf_draw_rounded_rectangle_glow(float x, float y, float w, float h, float roundness, Color color);
