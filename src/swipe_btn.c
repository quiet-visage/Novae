#include "swipe_btn.h"

#include <assert.h>
#include <raylib.h>

#include "clip.h"
#include "colors.h"
#include "config.h"
#include "fieldfusion.h"
#include "motion.h"
#include "tag.h"

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

static inline void update_anim_targets(bool left_hover, bool right_hover, float text_targets[2],
                                       float motion_targets[2]) {
    text_targets[0] = left_hover;
    text_targets[1] = right_hover;
    if (left_hover) motion_targets[0] = EXPANDED_RADIUS;
    if (right_hover) motion_targets[1] = EXPANDED_RADIUS;
}

int swipe_btn_view(Swipe_Btn* m, float x, float y, Icon icon_left, Icon icon_right, const char* text_left,
                   const char* text_right, bool enabled) {
    int result = 0;
    Vector2 mouse = GetMousePosition();
    float button_radius = tag_height() * .5;
    bool clicked = enabled && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                   CheckCollisionPointCircle(mouse, (Vector2){x, y}, button_radius);
    m->expand = m->expand || clicked;
    float motion_target[] = {INITIAL_RADIUS, INITIAL_RADIUS};

    if (m->expand && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        bool right_hover = mouse.x > x + button_radius;
        bool left_hover = mouse.x < x - button_radius;
        float text_target[2] = {0};
        update_anim_targets(left_hover, right_hover, text_target, motion_target);
        motion_update(&m->text_motion, text_target, GetFrameTime());

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

        const float sides_icon_size = BTN_ICON_SIZE * 2.;
        Texture left_tex = icon_get(icon_left);
        Rectangle src = {0, 0, left_tex.width, left_tex.height};
        float icon_offset = button_radius * 5. + sides_icon_size * .5;
        Rectangle dest = {x - icon_offset - sides_icon_size * .5, y - sides_icon_size * .5, sides_icon_size,
                          sides_icon_size};
        Vector2 orig = {0};
        DrawTexturePro(left_tex, src, dest, orig, 0, icon_color);

        if (text_left) {
            float text_x = x - icon_offset;
            float text_y = dest.y - g_cfg.estyle.typo.size - g_cfg.inner_gap;
            draw_text(text_x, text_y, text_left, 0xff * m->text_motion.position[0]);
        }

        float right_alpha_perc = radius_right / EXPANDED_RADIUS;
        icon_color = GET_RCOLOR(COLOR_BLUE);
        icon_color.a = 0xff * right_alpha_perc;

        Texture right_tex = icon_get(icon_right);
        dest.x = x + icon_offset - sides_icon_size * .5;

        if (text_right) {
            float text_y = dest.y - g_cfg.estyle.typo.size - g_cfg.inner_gap;
            draw_text(x + icon_offset, text_y, text_right, 0xff * m->text_motion.position[1]);
        }

        DrawTexturePro(right_tex, src, dest, orig, 0, icon_color);
    } else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && m->expand) {
        bool rigth_hover = mouse.x > x + button_radius;
        bool left_hover = mouse.x < x - button_radius;
        if (rigth_hover) result = 1;
        if (left_hover) result = -1;
        m->expand = false;
    } else {
        m->expand = false;
    }

    DrawCircle(x, y, button_radius, GET_RCOLOR(COLOR_SURFACE1));
    Texture cancel_icon = icon_get(ICON_SWIPE);
    Rectangle src = {0, 0, cancel_icon.width, cancel_icon.height};
    Rectangle dest = {x - BTN_ICON_SIZE * .5, y - BTN_ICON_SIZE * .5, BTN_ICON_SIZE, BTN_ICON_SIZE};
    Vector2 orig = {0};
    DrawTexturePro(cancel_icon, src, dest, orig, 0, WHITE);

    motion_update(&m->bg_motion, motion_target, GetFrameTime());
    return result;
}
