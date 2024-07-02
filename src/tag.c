#include "tag.h"

#include "alpha_inherit.h"
#include "clip.h"
#include "colors.h"
#include "config.h"
#include "fieldfusion.h"
#include "raylib.h"
#include "sdf_draw.h"

static FF_Style* g_style = &g_cfg.sstyle;

inline float tag_core_width(Tag* m) {
    size_t name_len = strlen(m->name);
    return ff_measure_utf8(m->name, name_len, *g_style).width;
}

inline float tag_core_height(void) { return g_style->typo.size; }

float tag_width(Tag* tag) { return tag_core_width(tag) + g_cfg.inner_gap2; }

float tag_height(void) { return tag_core_height() + g_cfg.inner_gap; }

void draw_underglow(Rectangle bg, Color col) {
    col.a = alpha_inherit_get_alpha();
    sdf_draw_rounded_rectangle_glow(bg.x, bg.y, bg.width, bg.height, 1.0, col);
}

Rectangle tag_draw(Tag* m, float x, float y, float max_w) {
    float core_w = tag_core_width(m);
    bool overflow = max_w ? core_w > max_w : 0;
    core_w = max_w ? MIN(core_w, max_w) : core_w;
    float core_h = tag_core_height();
    Rectangle bg = {.x = x, .y = y, .width = core_w + g_cfg.inner_gap2, .height = core_h + g_cfg.inner_gap};

    Color bg_color = GET_RCOLOR(COLOR_SURFACE0);
    bg_color.a = alpha_inherit_get_alpha();

    FF_Style style = *g_style;
    style.typo.color &= ~0xff;
    style.typo.color |= bg_color.a;
    Rectangle outline = bg;
    outline.width += 2.f;
    outline.height += 2.f;
    outline.x -= 1.f;
    outline.y -= 1.f;

    DrawRectangleRounded(bg, 1.0f, g_cfg.rounded_rec_segments, bg_color);
    rlDrawRenderBatchActive();

    clip_begin_rounded(bg.x, bg.y, bg.width, bg.height, 0x400);
    float fg_x = CENTER(bg.x, bg.width, core_w);
    float fg_y = CENTER(bg.y, bg.height, core_h);
    size_t name_len = strlen(m->name);
    ff_draw_str8(m->name, name_len, fg_x, fg_y, (float*)g_cfg.global_projection, style);
    if (overflow) {
        Color col1 = bg_color;
        col1.a = 0;
        DrawRectangleGradientH(bg.x + bg.width - g_cfg.inner_gap2, bg.y, g_cfg.inner_gap2, bg.height, col1, bg_color);
        DrawRectangleGradientH(bg.x + bg.width - g_cfg.inner_gap2, bg.y, g_cfg.inner_gap2, bg.height, col1, bg_color);
    }
    draw_underglow(bg, GetColor(m->color));
    clip_end();
    return bg;
}
