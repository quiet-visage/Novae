#include "button.h"

#include <rlgl.h>

#include "colors.h"
#include "config.h"
#include "fieldfusion.h"
#include "icon.h"
#include "motion.h"
#include "raylib.h"

static FF_Style *g_style = &g_cfg.sstyle;

Btn btn_create(void) {
    Btn ret = {.motion = motion_new(),
               .glyphs = ff_glyph_vec_create(),
               .color = {0},
               .glyphs_width = 0,
               .glyphs_height = 0,
               .flags = 0};
    ret.motion.f = 1.8;
    ret.motion.z = 0.9;
    ret.motion.r = 0.9;
    return ret;
}

void btn_set_label(Btn *m, const char *str) {
    ff_glyph_vec_clear(&m->glyphs);

    size_t str_len = strlen(str);

    FF_Dimensions dims = ff_print_utf8_vec(&m->glyphs, str, str_len, 0, 0, *g_style);

    m->glyphs_width = dims.width;
    m->glyphs_height = dims.height;
}

float btn_width(Btn *m) { return g_cfg.outer_gap2 + m->glyphs_width; }

float btn_height(Btn *m) { return g_cfg.inner_gap + g_style->typo.size; }

void btn_handle_expansion_anim(Btn *m, Rectangle *bounds) {
    motion_update(&m->motion, &bounds->width, GetFrameTime());
    bounds->width = m->motion.position[0];
    bounds->height = m->motion.position[1];
}

bool btn_draw(Btn *m, float x, float y) {
    Rectangle bounds = {
        .x = x,
        .y = y,
        .width = btn_width(m),
        .height = btn_height(m),
    };

    bool hovering = CheckCollisionPointRec(GetMousePosition(), bounds);
    bool act = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    bool result = hovering && act ? true : false;

    bool draw_bg = !(m->flags & BTN_FLAG_DONT_DRAW_BG);
    if (draw_bg) {
        float target[2] = {0, 0};
        if (hovering) {
            target[0] += 6;
            target[1] += 4;
            // g_style->typo.color = g_color[g_cfg.theme][COLOR_ROSEWATER];
        }
        motion_update(&m->motion, target, GetFrameTime());
        bounds.width += m->motion.position[0];
        bounds.height += m->motion.position[1];
        bounds.x -= m->motion.position[0] * .5f;
        bounds.y -= m->motion.position[1] * .5f;

        DRAW_BG(bounds, 8.f, COLOR_SURFACE0);
    }

    float fg_x = bounds.x + bounds.width * .5f - m->glyphs_width * .5f;
    float fg_y = bounds.y + bounds.height * .5f - m->glyphs_height * .5f;
    ff_set_glyphs_pos(m->glyphs.data, m->glyphs.len, fg_x, fg_y);

    ff_draw(m->glyphs.data, m->glyphs.len, (float *)g_cfg.global_projection, *g_style);

    return result;
}

bool btn_draw_with_icon(Btn *m, Icon icon, float x, float y) {
    Rectangle bg = {
        .x = x,
        .y = y,
        .width = btn_width(m),
        .height = g_style->typo.size+g_cfg.inner_gap,
    };

    bool hovering = CheckCollisionPointRec(GetMousePosition(), bg);
    bool act = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    bool result = hovering && act ? true : false;

    bool draw_bg = !(m->flags & BTN_FLAG_DONT_DRAW_BG);
    if (draw_bg) {
        float target[2] = {0, 0};
        if (hovering) {
            target[0] += 6;
            target[1] += 4;
            // fg_typo.color = g_color[g_cfg.theme][COLOR_ROSEWATER];
        }
        motion_update(&m->motion, target, GetFrameTime());
        bg.width += m->motion.position[0];
        bg.height += m->motion.position[1];
        bg.x -= m->motion.position[0] * .5f;
        bg.y -= m->motion.position[1] * .5f;

        DRAW_BG(bg, 0x100, COLOR_SURFACE0);
    }

    Texture icon_tex = icon_get(icon);
    Rectangle icon_src = {0, 0, icon_tex.width, icon_tex.width};
    Rectangle icon_dst = {bg.x + g_cfg.btn_pad_horz + m->motion.position[0] * .5,
                          bg.y + g_cfg.btn_pad_vert + m->motion.position[1] * .5,
                          BTN_ICON_SIZE + 4.f, BTN_ICON_SIZE + 4.f};
    Vector2 icon_origin = {0};
    Color icon_col = GET_RCOLOR(COLOR_SKY);
    DrawTexturePro(icon_tex, icon_src, icon_dst, icon_origin, 0, icon_col);

    float fg_x = CENTER(bg.x + BTN_ICON_SIZE, bg.width - BTN_ICON_SIZE, m->glyphs_width);
    float fg_y = CENTER(bg.y, bg.height, g_style->typo.size);
    ff_set_glyphs_pos(m->glyphs.data, m->glyphs.len, fg_x, fg_y);

    ff_draw(m->glyphs.data, m->glyphs.len, (float *)g_cfg.global_projection, *g_style);

    return result;
}

void btn_destroy(Btn *m) { ff_glyph_vec_destroy(&m->glyphs); }
