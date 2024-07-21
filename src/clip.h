#pragma once

#include <raylib.h>

void clip_init(void);
void clip_begin_custom_shape(void);  // must be the first layer
void clip_end_custom_shape(void);
void clip_end_frame(void);
void clip_begin(Rectangle bounds);
void clip_begin_rounded(Rectangle bounds, float radius);
void clip_end(void);
void clip_print_layer(void);
