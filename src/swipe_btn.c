#include "swipe_btn.h"

#include <assert.h>
#include <raylib.h>

#include "clip.h"
#include "colors.h"
#include "config.h"
#include "fieldfusion.h"
#include "motion.h"

#define BUTTON_RADIUS 10.
#define INITIAL_RADIUS 115.
#define EXPANDED_RADIUS 250.

Swipe_Btn swipe_btn_create(void) {
    Swipe_Btn result = {0};
    result.bg_motion = motion_new();
    result.text_motion = motion_new();
    return result;
}

static inline void draw_half_gradient(float bx, float by, float x, float y, float radius, Color color) {
    Rectangle rec = {bx, by, EXPANDED_RADIUS, EXPANDED_RADIUS * 2};
    clip_begin(rec);
    DrawCircleGradient(x, y, radius, color, BLANK);
    clip_end();
}

static void draw_text(float x, float y, const char* text, int alpha) {
    assert(text);
    size_t len = strlen(text);
    float width = ff_measure_utf8(text, len, g_cfg.estyle).width;
    float new_x = x - width * .5;
    FF_Style style = g_cfg.estyle;
    style.typo.color &= ~0xff;
    style.typo.color |= alpha;
    ff_draw_str8(text, len, new_x, y, (float*)g_cfg.global_projection, style);
}

int swipe_btn_view(Swipe_Btn* m, float x, float y, Icon icon_left, Icon icon_right, const char* text_left,
                   const char* text_right, bool enabled) {
    int result = 0;
    Vector2 mp = GetMousePosition();
    bool clicked =
        enabled && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointCircle(mp, (Vector2){x, y}, BUTTON_RADIUS);
    m->expand = m->expand || clicked;
    float motion_target[] = {INITIAL_RADIUS, INITIAL_RADIUS};
    if (m->expand && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        bool dir_right = mp.x > x + BUTTON_RADIUS;
        bool dir_left = mp.x < x - BUTTON_RADIUS;
        float text_target[2] = {dir_left, dir_right};
        motion_update(&m->text_motion, text_target, GetFrameTime());
        if (dir_left) motion_target[0] = EXPANDED_RADIUS;
        if (dir_right) motion_target[1] = EXPANDED_RADIUS;

        Color grad_color = GET_RCOLOR(COLOR_CRUST);
        float radius_left = m->bg_motion.position[0];
        float left_grad_x = x - EXPANDED_RADIUS;
        float left_grad_y = y - EXPANDED_RADIUS;
        draw_half_gradient(left_grad_x, left_grad_y, x, y, radius_left, grad_color);

        float radius_right = m->bg_motion.position[1];
        float right_grad_y = y - EXPANDED_RADIUS;
        draw_half_gradient(x, right_grad_y, x, y, radius_right, grad_color);

        float left_alpha_perc = radius_left / EXPANDED_RADIUS;
        Color icon_color = GET_RCOLOR(COLOR_RED);
        icon_color.a = 0xff * left_alpha_perc;

        Texture left_tex = icon_get(icon_left);
        Rectangle src = {0, 0, left_tex.width, left_tex.height};
        Rectangle dest = {x - BUTTON_RADIUS * 5 - BTN_ICON_SIZE * .5, y - BTN_ICON_SIZE * .5, BTN_ICON_SIZE,
                          BTN_ICON_SIZE};
        Vector2 orig = {0};
        DrawCircle(dest.x + BTN_ICON_SIZE * .5, dest.y + BTN_ICON_SIZE * .5, BUTTON_RADIUS, GET_RCOLOR(COLOR_SURFACE0));
        DrawCircleLines(dest.x + BTN_ICON_SIZE * .5, dest.y + BTN_ICON_SIZE * .5, BUTTON_RADIUS, icon_color);
        DrawTexturePro(left_tex, src, dest, orig, 0, icon_color);

        if (text_left) {
            float text_x = dest.x;
            float text_y = dest.y - g_cfg.estyle.typo.size - g_cfg.inner_gap;
            draw_text(text_x, text_y, text_left, 0xff * m->text_motion.position[0]);
        }

        float right_alpha_perc = radius_right / EXPANDED_RADIUS;
        icon_color = GET_RCOLOR(COLOR_BLUE);
        icon_color.a = 0xff * right_alpha_perc;

        Texture right_tex = icon_get(icon_right);
        dest.x = x + BUTTON_RADIUS * 5.0 - BTN_ICON_SIZE * .5;

        if (text_right) {
            float text_x = dest.x;
            float text_y = dest.y - g_cfg.estyle.typo.size - g_cfg.inner_gap;
            draw_text(text_x, text_y, text_right, 0xff * m->text_motion.position[1]);
        }

        DrawCircle(dest.x + BTN_ICON_SIZE * .5, dest.y + BTN_ICON_SIZE * .5, BUTTON_RADIUS, GET_RCOLOR(COLOR_SURFACE0));
        DrawCircleLines(dest.x + BTN_ICON_SIZE * .5, dest.y + BTN_ICON_SIZE * .5, BUTTON_RADIUS, icon_color);
        DrawTexturePro(right_tex, src, dest, orig, 0, icon_color);
    } else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && m->expand) {
        bool dir_right = mp.x > x + BUTTON_RADIUS;
        bool dir_left = mp.x < x - BUTTON_RADIUS;
        if (dir_right) result = 1;
        if (dir_left) result = -1;
        m->expand = false;
    } else {
        m->expand = false;
    }

    DrawCircle(x, y, BUTTON_RADIUS * 1.25, GET_RCOLOR(COLOR_SURFACE1));
    Texture cancel_icon = icon_get(ICON_SWIPE);
    Rectangle src = {0, 0, cancel_icon.width, cancel_icon.height};
    Rectangle dest = {x - BTN_ICON_SIZE * .5, y - BTN_ICON_SIZE * .5, BTN_ICON_SIZE, BTN_ICON_SIZE};
    Vector2 orig = {0};
    DrawTexturePro(cancel_icon, src, dest, orig, 0, WHITE);

    motion_update(&m->bg_motion, motion_target, GetFrameTime());
    return result;
}
