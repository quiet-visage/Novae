#include "button.h"

#include <rlgl.h>

#include "colors.h"
#include "config.h"
#include "fieldfusion.h"
#include "icon.h"
#include "motion.h"
#include "raylib.h"

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

    FF_Dimensions dims =
        ff_print_utf8_vec(&m->glyphs, str, str_len, g_cfg.btn_typo, 0,
                          0, FF_FLAG_DEFAULT, 0);

    m->glyphs_width = dims.width;
    m->glyphs_height = dims.height;
}

float btn_width(Btn *m) {
    return g_cfg.btn_pad_horz * 2 + m->glyphs_width;
}

#define MAX(X, Y) (X > Y ? X : Y)
float btn_height(Btn *m) {
    return g_cfg.btn_pad_vert * 2 +
           MAX(BTN_ICON_SIZE, g_cfg.btn_typo.size);
}

void btn_handle_expansion_anim(Btn *m, Rectangle *bounds) {
    motion_update(&m->motion, &bounds->width, GetFrameTime());
    bounds->width = m->motion.position[0];
    bounds->height = m->motion.position[1];
}

bool btn_draw(Btn *m, float x, float y) {
    FF_Typo fg_typo = g_cfg.btn_typo;
    Rectangle bounds = {
        .x = x,
        .y = y,
        .width = btn_width(m),
        .height = btn_height(m),
    };

    bool hovering =
        CheckCollisionPointRec(GetMousePosition(), bounds);
    bool released = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
    bool result = hovering && released ? true : false;

    bool draw_bg = !(m->flags & BTN_FLAG_DONT_DRAW_BG);
    if (draw_bg) {
        float target[2] = {0, 0};
        if (hovering) {
            target[0] += 6;
            target[1] += 4;
            fg_typo.color = g_color[g_cfg.theme][COLOR_ROSEWATER];
        }
        motion_update(&m->motion, target, GetFrameTime());
        bounds.width += m->motion.position[0];
        bounds.height += m->motion.position[1];
        bounds.x -= m->motion.position[0] * .5f;
        bounds.y -= m->motion.position[1] * .5f;

        float rad = bounds.width > bounds.height
                        ? 2 * 8 / bounds.height
                        : 2 * 8 / bounds.width;
        DrawRectangleRounded(
            bounds, rad, 6,
            GetColor(g_color[g_cfg.theme][COLOR_SURFACE0]));
        rlDrawRenderBatchActive();
    }

    float fg_x =
        bounds.x + bounds.width * .5f - m->glyphs_width * .5f;
    float fg_y =
        bounds.y + bounds.height * .5f - m->glyphs_height * .5f;
    ff_set_glyphs_pos(m->glyphs.data, m->glyphs.len, fg_x, fg_y);

    ff_draw(fg_typo.font, m->glyphs.data, m->glyphs.len,
            (float *)g_cfg.global_projection);

    return result;
}

bool btn_draw_with_icon(Btn *m, Icon icon, float x, float y) {
    FF_Typo fg_typo = g_cfg.btn_typo;
    Rectangle bounds = {
        .x = x,
        .y = y,
        .width = btn_width(m) + BTN_ICON_SIZE,
        .height = btn_height(m),
    };

    bool hovering =
        CheckCollisionPointRec(GetMousePosition(), bounds);
    bool released = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
    bool result = hovering && released ? true : false;

    bool draw_bg = !(m->flags & BTN_FLAG_DONT_DRAW_BG);
    if (draw_bg) {
        float target[2] = {0, 0};
        if (hovering) {
            target[0] += 6;
            target[1] += 4;
            fg_typo.color = g_color[g_cfg.theme][COLOR_ROSEWATER];
        }
        motion_update(&m->motion, target, GetFrameTime());
        bounds.width += m->motion.position[0];
        bounds.height += m->motion.position[1];
        bounds.x -= m->motion.position[0] * .5f;
        bounds.y -= m->motion.position[1] * .5f;

        float rad = bounds.width > bounds.height
                        ? 2 * 8 / bounds.height
                        : 2 * 8 / bounds.width;
        DrawRectangleRounded(
            bounds, rad, 6,
            GetColor(g_color[g_cfg.theme][COLOR_SURFACE0]));
        rlDrawRenderBatchActive();
    }

    Texture icon_tex = icon_get(icon);
    Rectangle icon_src = {0, 0, icon_tex.width, icon_tex.width};
    Rectangle icon_dst = {
        bounds.x + g_cfg.btn_pad_horz + m->motion.position[0] * .5,
        bounds.y + g_cfg.btn_pad_vert + m->motion.position[1] * .5,
        BTN_ICON_SIZE, BTN_ICON_SIZE};
    Vector2 icon_origin = {0};
    Color icon_col = GetColor(g_color[g_cfg.theme][COLOR_SKY]);
    DrawTexturePro(icon_tex, icon_src, icon_dst, icon_origin, 0,
                   icon_col);

    float icon_offset = g_cfg.btn_pad_horz * 2 + BTN_ICON_SIZE;
    bounds.width -= icon_offset;
    bounds.x += icon_offset;
    float fg_x =
        bounds.x + bounds.width * .5f - m->glyphs_width * .5f;
    float fg_y =
        bounds.y + bounds.height * .5f - m->glyphs_height * .5f;
    ff_set_glyphs_pos(m->glyphs.data, m->glyphs.len, fg_x, fg_y);

    ff_draw(fg_typo.font, m->glyphs.data, m->glyphs.len,
            (float *)g_cfg.global_projection);

    return result;
}

void btn_destroy(Btn *m) { ff_glyph_vec_destroy(&m->glyphs); }
