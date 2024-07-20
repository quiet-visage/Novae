#include "task.h"

#include <raylib.h>
#include <rlgl.h>

#include "clip.h"
#include "colors.h"
#include "config.h"
#include "db_cache.h"
#include "fieldfusion.h"
#include "icon.h"
#include "motion.h"
#include "swipe_btn.h"
#include "tag.h"

#define SIDE_BTN_HOVER_WIDTH_RATIO 0.14f
#define SIDE_BTN_WIDTH_RATIO 0.08f
#define SIDE_BTN_MIN_W 32.f
#define SIDE_BTN_ICON_SZ 14.f
#define SIDE_BTN_ICON_OFFSET 2.f
static FF_Style* g_style = &g_cfg.sstyle;

Task task_create(void) {
    Task result = {
        .name = 0,
        .left = 0,
        .done = 0,
        .complete = 0,
        .db_id = 0,
        .tag_id = 0,
        .check_btn_mo = motion_new(),
        .up_btn_mo = motion_new(),
        .bar_mo = motion_new(),
        .swipe = swipe_btn_create(),
    };
    return result;
}

void task_destroy(Task* m) {
    if (m->name) free(m->name);
}

static size_t get_range_str(Task* m, C32* out) {
    char str[64] = {0};
    snprintf(str, 63, "%d of %d", m->done, m->left);
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

inline float task_height(void) { return g_cfg.outer_gap2 + g_cfg.inner_gap2 + g_style->typo.size * 2 + 6.f; }

Rectangle rect_insertect(Rectangle a, Rectangle b) {
    Rectangle result = {0};
    result.x = MAX(a.x, b.x);
    result.y = MAX(a.y, b.y);
    result.width = MIN(a.x + a.width, b.x + b.width) - result.x;
    result.height = MIN(a.y + a.height, b.y + b.height) - result.y;
    if (result.width < 0.f || result.height < 0.f) return (Rectangle){0};
    return result;
}

static bool draw_move_top_btn(Task* m, Rectangle bg, Rectangle bounds, bool enabled) {
    bool result = 0;
    float target_w = m->check_btn_mo.position[0];
    float btn_h = bg.height;
    float icon_target_alpha = 0;

    Vector2 mouse = GetMousePosition();
    Rectangle btn_bounds = {.x = bg.x, .y = bg.y, .width = target_w, .height = btn_h};
    Rectangle collision_bounds = rect_insertect(btn_bounds, bounds);
    bool hover = enabled && CheckCollisionPointRec(mouse, collision_bounds);
    if (hover) {
        target_w = MAX(bg.width * SIDE_BTN_HOVER_WIDTH_RATIO, SIDE_BTN_MIN_W);
        icon_target_alpha = 0xff;
        result = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    } else {
        target_w = MAX(bg.width * SIDE_BTN_WIDTH_RATIO, SIDE_BTN_MIN_W);
        icon_target_alpha = 0xb1;
    }

    float motion_target[2] = {target_w, icon_target_alpha};
    motion_update(&m->check_btn_mo, motion_target, GetFrameTime());

    float btn_w = m->check_btn_mo.position[0];
    Color col0 = GET_RCOLOR(COLOR_BLUE);
    col0.a = 0x3a;
    Color col1 = col0;
    col1.a = 0;
    DrawRectangleGradientH(bg.x, bg.y, btn_w, btn_h, col0, col1);

    // cheack_btn_draw()
    Texture icon = icon_get(ICON_TARGET);
    float icon_w = SIDE_BTN_ICON_SZ;
    float icon_h = SIDE_BTN_ICON_SZ;
    Rectangle icon_src = {0, 0, icon.width, icon.height};
    Rectangle icon_dst = {bg.x + SIDE_BTN_ICON_OFFSET + btn_w * .15f, CENTER(bg.y, bg.height, icon_h), icon_w, icon_h};
    Color icon_col = GET_RCOLOR(COLOR_TEXT);
    icon_col.a = m->check_btn_mo.position[1];
    DrawTexturePro(icon, icon_src, icon_dst, (Vector2){0}, 0, icon_col);
    return result;
}

static bool draw_up_btn(Task* m, Rectangle bg, Rectangle bounds, bool enabled) {
    bool result = 0;
    float target_w = m->up_btn_mo.position[0];
    float btn_h = bg.height;
    float icon_target_alpha = 0;

    Vector2 mouse = GetMousePosition();
    Rectangle btn_bounds = {.x = bg.x + bg.width - target_w, .y = bg.y, .width = target_w, .height = btn_h};
    Rectangle collision_bounds = rect_insertect(btn_bounds, bounds);
    bool hover = enabled && CheckCollisionPointRec(mouse, collision_bounds);
    if (hover) {
        target_w = MAX(bg.width * SIDE_BTN_HOVER_WIDTH_RATIO, SIDE_BTN_MIN_W);
        icon_target_alpha = 0xff;
        result = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    } else {
        target_w = MAX(bg.width * SIDE_BTN_WIDTH_RATIO, SIDE_BTN_MIN_W);
        icon_target_alpha = 0xb1;
    }

    float motion_target[2] = {target_w, icon_target_alpha};
    motion_update(&m->up_btn_mo, motion_target, GetFrameTime());
    float btn_w = m->up_btn_mo.position[0];
    float btn_x = bg.x + bg.width - btn_w;
    Color col1 = GET_RCOLOR(COLOR_BLUE);
    col1.a = 0x3a;
    Color col0 = col1;
    col0.a = 0;

    DrawRectangleGradientH(btn_x, bg.y, btn_w, btn_h, col0, col1);

    Texture icon = icon_get(ICON_ARROW_UP);
    float icon_w = SIDE_BTN_ICON_SZ;
    float icon_h = SIDE_BTN_ICON_SZ;
    Rectangle icon_src = {0, 0, icon.width, icon.height};
    Rectangle icon_dst = {bg.x + bg.width - icon_w - SIDE_BTN_ICON_OFFSET - btn_w * .15f,
                          CENTER(bg.y, bg.height, icon_h), icon_w, icon_h};
    Color icon_col = GET_RCOLOR(COLOR_TEXT);
    icon_col.a = m->up_btn_mo.position[1];
    DrawTexturePro(icon, icon_src, icon_dst, (Vector2){0}, 0, icon_col);

    return result;
}

Task_Return task_draw(Task* m, float x, float y, float max_width, Rectangle bounds, bool enabled) {
    Task_Return result = {0};
    Rectangle bg = {.x = x, .y = y, .width = max_width, .height = task_height()};

    clip_begin_rounded(bg, g_cfg.bg_radius);
    DRAW_BG(bg, g_cfg.bg_radius, COLOR_BASE);
    rlDrawRenderBatchActive();

    bool move_top = draw_move_top_btn(m, bg, bounds, enabled);
    bool move_up = draw_up_btn(m, bg, bounds, enabled);

    float name_w = ff_measure_utf32(m->name, m->name_len, *g_style).width;
    float name_y = bg.y + g_cfg.inner_gap;
    float name_x = CENTER(bg.x, bg.width, name_w);
    ff_draw_str32(m->name, m->name_len, name_x, name_y, (float*)g_cfg.global_projection, *g_style);

    Rectangle bar_rec = {0};
    bar_rec.width = bg.width * .6f;
    bar_rec.height = 5.f;
    bar_rec.x = CENTER(bg.x, bg.width, bar_rec.width);
    bar_rec.y = name_y + g_cfg.inner_gap2;
    DrawRectangleRounded(bar_rec, 1.f, 6, GET_RCOLOR(COLOR_SURFACE0));

    Rectangle prog_bar_rec = bar_rec;
    float progress = m->left ? (float)m->done / (float)m->left : 0.f;
    progress = MIN(progress, 1.f);
    motion_update_x(&m->bar_mo, bar_rec.width * progress, GetFrameTime());
    prog_bar_rec.width = m->bar_mo.position[0];
    DrawRectangleRounded(prog_bar_rec, 1.f, 6, GET_RCOLOR(COLOR_TEAL));

    float tag_x = bar_rec.x;
    float tag_y = bar_rec.y + bar_rec.height + g_cfg.inner_gap;
    Tag* tag = db_cache_get_tag(m->tag_id);
    if (!tag) tag = db_cache_get_default_tag();
    // clip_begin_rounded(bg.x,bg.y,bg.width,bg.height, 0x400);
    tag_draw(tag, tag_x, tag_y, bar_rec.width * .25);
    // clip_end();

    if (m->left) {
        C32 str[64] = {0};
        size_t str_len = get_range_str(m, str);
        float str_w = ff_measure_utf32(str, str_len, *g_style).width;
        float str_x = bar_rec.x + bar_rec.width - str_w;
        float str_y = tag_y + g_cfg.inner_gap * .5f;
        ff_draw_str32(str, str_len, str_x, str_y, (float*)g_cfg.global_projection, *g_style);
    } else {
        float icon_x = bar_rec.x + bar_rec.width - BTN_ICON_SIZE;
        float icon_y = tag_y;
        Texture icon = icon_get(ICON_INFINITE);
        Rectangle source = {0, 0, icon.width, icon.height};
        Rectangle dest = {icon_x, icon_y, BTN_ICON_SIZE, BTN_ICON_SIZE};
        Vector2 orig = {0};
        Color col = GET_RCOLOR(COLOR_SKY);
        DrawTexturePro(icon, source, dest, orig, 0, col);
        rlDrawRenderBatchActive();
    }

    if (move_top)
        result = TASK_MOVE_TOP;
    else if (move_up)
        result = TASK_MOVE_UP;

    int swipe = swipe_btn_view(&m->swipe, bar_rec.x + bar_rec.width * .5, tag_y + tag_height() * .5, ICON_VISIBILY_OFF,
                               ICON_CHECK, "Toggle visibility", "Mark as done");
    clip_end();

    if (swipe < 0)
        result = TASK_DELETE;
    else if (swipe > 0)
        result = TASK_MARK_DONE;

    return result;
}
