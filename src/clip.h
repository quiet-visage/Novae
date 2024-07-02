#pragma once

void clip_init(void);
void clip_end_frame(void);
void clip_begin(float x, float y, float w, float h);
void clip_begin_rounded(float x, float y, float w, float h, float rad);
void clip_end(void);
void clip_print_layer(void);
void clip_begin_custom_shape(void);  // Will enable drawing on stencil buffer, must call
                                     // clip_end_custom_shape and clip_end
void clip_end_custom_shape(void);
