// TODO: Make a hash map for the instances, should be more performant
#include "hint.h"

#include <assert.h>
#include <fieldfusion.h>
#include <raylib.h>
#include <raymath.h>

#include "alpha_inherit.h"
#include "colors.h"
#include "config.h"
#include "motion.h"

typedef struct {
    Motion motion;
    Rectangle bg;
    float indicator_m_x;
    float alpha;
    float text_width;
    float time_hovering;
    char desc[1024];
    size_t desc_len;
} Hint_Instance;

#define HINT_INSTANCES_CAP 128

static Hint_Instance g_instances[HINT_INSTANCES_CAP];
static size_t g_instances_len;
static FF_Style* g_style = &g_cfg.sstyle;

void hint_init(void) { memset(g_instances, 0, sizeof(g_instances)); }

void hint_terminate(void) {}

static inline float hint_min_width(void) { return g_cfg.outer_gap2 * 3; }

static Hint_Instance* get_instance(const char* desc) {
    size_t desc_len = strlen(desc);
    for (size_t i = 0; i < HINT_INSTANCES_CAP; ++i) {
        Hint_Instance* instance_ptr = &g_instances[i];
        bool key_matches =
            instance_ptr->desc_len && desc_len == instance_ptr->desc_len && !memcmp(desc, instance_ptr->desc, desc_len);
        if (!key_matches) continue;
        ++g_instances_len;
        return instance_ptr;
    }
    assert(g_instances_len < HINT_INSTANCES_CAP);
    Hint_Instance* instance_ptr = &g_instances[g_instances_len++];
    memcpy(instance_ptr->desc, desc, desc_len);
    instance_ptr->desc_len = desc_len;
    instance_ptr->motion = motion_new();
    return instance_ptr;
}

void hint_view(const char* desc, Rectangle bounds) {
    Hint_Instance* instance = get_instance(desc);
    Vector2 mouse = GetMousePosition();
    bool hovering = CheckCollisionPointRec(mouse, bounds);

    float target_alpha = 0;

    if (hovering)
        instance->time_hovering += GetFrameTime();
    else
        instance->time_hovering = 0.;
    bool active = instance->time_hovering >= 2.;
    if (active) target_alpha = alpha_inherit_get_alpha();

    motion_update_x(&instance->motion, target_alpha, GetFrameTime());
    instance->alpha = instance->motion.position[0];
    if (instance->alpha < .1) return;
    instance->text_width = ff_measure_utf8(desc, strlen(desc), *g_style).width;

    instance->bg.height = g_style->typo.size + g_cfg.inner_gap2;
    instance->bg.y = bounds.y - instance->bg.height - g_cfg.inner_gap;
    instance->bg.width = MAX(instance->text_width + g_cfg.inner_gap2, hint_min_width());
    instance->bg.x = bounds.x + bounds.width * .5 - instance->bg.width * .5;
    Color color = GET_RCOLOR(COLOR_SURFACE1);
    color.a = instance->alpha;

    instance->indicator_m_x = Clamp(mouse.x, bounds.x, bounds.x + bounds.width);
    memset(instance->desc, 0, sizeof(instance->desc));
    memcpy(instance->desc, desc, strlen(desc));
}

void hint_end_frame(void) {
    for (size_t i = 0; i < g_instances_len; ++i) {
        Hint_Instance* instance = &g_instances[i];
        if (instance->alpha < .1) continue;
        Color color = GET_RCOLOR(COLOR_SURFACE1);
        color.a = instance->alpha;

        float indicator_x = Clamp(instance->indicator_m_x, instance->bg.x + g_cfg.inner_gap + g_cfg.bg_radius * .75,
                                  instance->bg.x + instance->bg.width - g_cfg.inner_gap - g_cfg.bg_radius * .75);
        Vector2 v1 = {indicator_x, instance->bg.y + instance->bg.height + g_cfg.inner_gap - 2.};
        Vector2 v2 = {v1.x - g_cfg.inner_gap, instance->bg.y + instance->bg.height};
        Vector2 v3 = {v1.x + g_cfg.inner_gap, instance->bg.y + instance->bg.height};
        DrawTriangle(v2, v1, v3, color);
        DrawRectangleRounded(instance->bg, RADIUS_TO_ROUNDNESS(g_cfg.bg_radius, instance->bg.height),
                             g_cfg.rounded_rec_segments, color);
        rlDrawRenderBatchActive();

        FF_Style style = *g_style;
        style.typo.color &= ~0xff;
        style.typo.color |= (int)(instance->alpha);
        ff_draw_str8(instance->desc, instance->desc_len,
                     instance->bg.x + instance->bg.width * .5 - instance->text_width * .5,
                     instance->bg.y + instance->bg.height * .5 - g_style->typo.size * .5,
                     (float*)g_cfg.global_projection, style);
    }
    g_instances_len = 0;
}
