#include "check_btn.h"

#include "draw_opts.h"
#include "motion.h"
#include "raylib.h"

#define RADIUS 16.0f

Check_Btn check_btn_create() {
    Check_Btn ret = {.check_anim = motion_new(), .uncheck_anim = motion_new()};
    ret.uncheck_anim.position[0] = RADIUS;
    ret.uncheck_anim.position[1] = 0xff;
    ret.uncheck_anim.f = 1.8f;
    ret.uncheck_anim.z = 1.0f;
    ret.check_anim.f = 1.8f;
    ret.check_anim.z = 1.0f;
    return ret;
}

bool check_btn_draw(Check_Btn* m, bool* checked, float x, float y, Draw_Opts opts) {
    // return true if this has been clicked
    bool ret = false;

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 point = GetMousePosition();
        Vector2 center = {x, y};
        if (opts & DRAW_ENABLE_MOUSE_INPUT && CheckCollisionPointCircle(point, center, RADIUS)) {
            *checked = !(*checked);
            ret = true;
        }
    }

    float dt = GetFrameTime();
    if (*checked) {
        float check_targ[2] = {RADIUS, 0xff};
        float uncheck_targ[2] = {0, 0};

        motion_update(&m->check_anim, check_targ, dt);
        motion_update(&m->uncheck_anim, uncheck_targ, dt);
    } else {
        float check_targ[2] = {0, 0};
        float uncheck_targ[2] = {RADIUS, 0xff};

        motion_update(&m->check_anim, check_targ, dt);
        motion_update(&m->uncheck_anim, uncheck_targ, dt);
    }

    float check_rad = m->check_anim.position[0];
    float uncheck_rad = m->uncheck_anim.position[0];
    float check_alpha = m->check_anim.position[1];
    float uncheck_alpha = m->uncheck_anim.position[1];
    Color check_color = {0xff, 0xff, 0xff, check_alpha};
    Color uncheck_color = {0xff, 0xff, 0xff, uncheck_alpha};

    DrawCircle(x, y, check_rad, check_color);
    DrawCircleLines(x, y, uncheck_rad, uncheck_color);

    return ret;
}
