#include "tag.h"

#include "alpha_inherit.h"
#include "clip.h"
#include "colors.h"
#include "config.h"
#include "fieldfusion.h"
#include "raylib.h"

static FF_Style* g_style = &g_cfg.sstyle;

inline float tag_core_width(Tag* m) {
    size_t name_len = strlen(m->name);
    return ff_measure_utf8(m->name, name_len, *g_style).width;
}

inline float tag_core_height(void) { return g_style->typo.size; }

float tag_width(Tag* tag) { return tag_core_width(tag) + g_cfg.inner_gap2; }

float tag_height(void) { return tag_core_height() + g_cfg.inner_gap; }

Rectangle tag_draw(Tag* m, float x, float y) {
    float core_w = tag_core_width(m);
    float core_h = tag_core_height();
    Rectangle bg = {
        .x = x, .y = y, .width = core_w + g_cfg.inner_gap2, .height = core_h + g_cfg.inner_gap};
    Color bg_color = GET_RCOLOR(COLOR_SURFACE0);
    Rectangle outline = bg;
    outline.width += 2.f;
    outline.height += 2.f;
    outline.x -= 1.f;
    outline.y -= 1.f;

    DrawRectangleRounded(bg, 1.0f, g_cfg.rounded_rec_segments, bg_color);
    clip_begin_custom_shape();
    DrawRectangleRounded(bg, 1.0f, g_cfg.rounded_rec_segments, WHITE);
    clip_end_custom_shape();
    Color col0 = GetColor(m->color);
    Color col1 = bg_color;
    col0.a = 0xff;
    col1.a = 0;
    DrawRectangleGradientV(bg.x, bg.y + bg.height * .75f, bg.width, bg.height * .25f, col1, col0);
    clip_end();

    rlDrawRenderBatchActive();

    float fg_x = CENTER(bg.x, bg.width, core_w);
    float fg_y = CENTER(bg.y, bg.height, core_h);
    size_t name_len = strlen(m->name);
    ff_draw_str8(m->name, name_len, fg_x, fg_y, (float*)g_cfg.global_projection, *g_style);
    return bg;
}
