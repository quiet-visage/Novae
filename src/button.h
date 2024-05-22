#pragma once
#include <fieldfusion.h>
#include <raylib.h>

#include "icon.h"
#include "motion.h"

typedef enum {
    BTN_FLAG_DONT_DRAW_BG = (1 << 0),
} Btn_Flags;

typedef struct {
    Motion motion;
    FF_Glyph_Vec glyphs;
    Color color;
    float glyphs_width;
    float glyphs_height;
    int flags;
} Btn;

Btn btn_create(void);
bool btn_draw(Btn* m, float x, float y);
bool btn_draw_with_icon(Btn* m, Icon icon, float x, float y);
void btn_set_label(Btn* m, const char* str);
float btn_width(Btn* m);
float btn_height(Btn* m);
void btn_destroy(Btn* m);
