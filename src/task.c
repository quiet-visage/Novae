#include "task.h"

#include <assert.h>
#include <raylib.h>
#include <rlgl.h>

#include "button.h"
#include "check_btn.h"
#include "colors.h"
#include "config.h"
#include "draw_opts.h"
#include "fieldfusion.h"
#include "icon.h"

Task task_create(void) {
    Task result = {.check_btn = check_btn_create(),
                   .move_up_btn = btn_create(),
                   .name = 0,
                   .left = 0,
                   .done = 0,
                   .complete = 0};
    result.move_up_btn.flags |= BTN_FLAG_DONT_DRAW_BG;
    return result;
}

void task_destroy(Task* m) {
    btn_destroy(&m->move_up_btn);
    if (m->name) free(m->name);
}

static size_t get_range_str(Task* m, C32* out) {
    char str[64] = {0};
    snprintf(str, 63, "%d / %d", m->done, m->left);
    size_t ret = strlen(str);
    ff_utf8_to_utf32(out, str, strlen(str));
    return ret;
}

void task_set_name(Task* m, C32* name, size_t len) {
    size_t size = len * sizeof(C32);
    if (m->name) {
        m->name = realloc(m->name, size);
    } else {
        m->name = malloc(size);
    }
    m->name_len = len;
    memcpy(m->name, name, size);
}

inline float task_height(void) {
    return g_cfg.inner_gap * 2 + g_cfg.btn_typo.size;
}

Task_Return_Flags task_draw(Task* m, float x, float y,
                            float max_width) {
#define CHECK_BTN_RAD 16.0f
    Task_Return_Flags result = 0;
    Rectangle bg;
    bg.x =
        x + g_cfg.btn_pad_horz * 2 + BTN_ICON_SIZE + g_cfg.inner_gap;
    bg.y = y;
    bg.height = g_cfg.inner_gap * 2 + g_cfg.btn_typo.size;

    float cx = bg.x + g_cfg.outer_gap;
    float cy = bg.y + g_cfg.inner_gap;

    float check_btn_x = cx + CHECK_BTN_RAD * .5f;
    float check_btn_y = cy + CHECK_BTN_RAD * .5f;

    assert(m->name);
    float name_x = check_btn_x + CHECK_BTN_RAD + g_cfg.inner_gap;
    float name_y = y + bg.height * .5f - g_cfg.btn_typo.size * .5f;

    float move_up_btn_width =
        btn_width(&m->move_up_btn) + BTN_ICON_SIZE;
    bg.width = max_width - g_cfg.inner_gap - move_up_btn_width;

    float rad =
        bg.width > bg.height ? 2 * 8 / bg.height : 2 * 8 / bg.width;
    Color col = GetColor(g_color[g_cfg.theme][COLOR_BASE]);
    DrawRectangleRounded(bg, rad, 6, col);
    rlDrawRenderBatchActive();

    m->complete = m->complete || (m->left && m->done >= m->left);
    bool fpo = m->complete;
    bool clicked_check =
        check_btn_draw(&m->check_btn, &fpo, check_btn_x, check_btn_y,
                       DRAW_ENABLE_MOUSE_INPUT);
    if (clicked_check && !m->complete) {
        m->done = m->left;
        m->complete = 1;
        result |= TASK_CHANGED;
    }

    if (m->left) {
        C32 range_str[64] = {0};
        size_t range_str_len = get_range_str(m, range_str);
        float range_str_w = ff_measure_utf32(range_str, range_str_len,
                                             g_cfg.btn_typo.font,
                                             g_cfg.btn_typo.size, 0)
                                .width;
        float range_str_x = (cx + bg.width) - range_str_w -
                            (g_cfg.outer_gap + g_cfg.inner_gap);
        ff_draw_str32(range_str, range_str_len, range_str_x, name_y,
                      (float*)g_cfg.global_projection, g_cfg.btn_typo,
                      FF_FLAG_DEFAULT, 0);
    } else {
        float icon_x =
            (x + bg.width) - BTN_ICON_SIZE - g_cfg.outer_gap;
        float icon_y = CENTER(bg.y, bg.height, BTN_ICON_SIZE);
        Texture icon = icon_get(ICON_INFINITE);
        Rectangle source = {0, 0, icon.width, icon.height};
        Rectangle dest = {icon_x, icon_y, BTN_ICON_SIZE,
                          BTN_ICON_SIZE};
        Vector2 orig = {0};
        Color col = GetColor(g_color[g_cfg.theme][COLOR_SKY]);
        DrawTexturePro(icon, source, dest, orig, 0, col);
        rlDrawRenderBatchActive();
    }

    ff_draw_str32(m->name, m->name_len, name_x, name_y,
                  (float*)g_cfg.global_projection, g_cfg.btn_typo,
                  FF_FLAG_DEFAULT, 0);

    Rectangle move_up_btn_bg = {
        .x = x,
        .y = y,
        .width = BTN_ICON_SIZE + g_cfg.btn_pad_horz * 2,
        .height = bg.height};
    rad = move_up_btn_bg.width > move_up_btn_bg.height
              ? 2 * 8 / move_up_btn_bg.height
              : 2 * 8 / move_up_btn_bg.width;
    DrawRectangleRounded(move_up_btn_bg, rad, 16, col);
    bool clicked_move_up =btn_draw_with_icon(
        &m->move_up_btn, ICON_MOVE_UP, x ,
        CENTER(bg.y, bg.height, btn_height(&m->move_up_btn)));
    if (clicked_move_up) result |= TASK_MOVE_UP;
    return result;
}
