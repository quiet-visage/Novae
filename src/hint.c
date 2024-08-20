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

#define HASH_TABLE_SIZE 0x200

typedef struct {
    Motion motion;
    Rectangle bg;
    float indicator_m_x;
    float alpha;
    FF_Dimensions text_size;
    float time_hovering;
    char desc[2048];
    size_t desc_len;
    size_t instance_id;
} Hint_Instance;

#define HINT_INSTANCES_CAP 128

static Hint_Instance g_instances[HINT_INSTANCES_CAP];
static size_t g_instances_len;
static FF_Style* g_style = &g_cfg.sstyle;

void hint_init(void) { memset(g_instances, 0, sizeof(g_instances)); }

void hint_terminate(void) {}

static inline float hint_min_width(void) { return g_cfg.outer_gap2 * 3; }

size_t hint_generate_instance_key() {
    assert(g_instances_len < HINT_INSTANCES_CAP);
    size_t key = g_instances_len++;
    memset(&g_instances[key], 0, sizeof(*g_instances));
    g_instances[key].motion = motion_new();
    return key;
}

void hint_update_instance_desc(size_t key, const char* desc) {
    Hint_Instance* instance = &g_instances[key];
    memset(&instance->desc, 0, sizeof(instance->desc));
    memcpy(&instance->desc, desc, strlen(desc));
}

static Hint_Instance* get_instance(size_t key) {
    assert(key < HINT_INSTANCES_CAP);
    return &g_instances[key];
}

void hint_view(size_t key, const char* desc, Rectangle bounds) {
    Hint_Instance* instance = get_instance(key);

    Vector2 mouse = GetMousePosition();
    bool hovering = CheckCollisionPointRec(mouse, bounds);
    if (hovering)
        instance->time_hovering += GetFrameTime();
    else {
        instance->time_hovering = 0.;
    }

    bool active = instance->time_hovering >= 2.;
    float target_alpha = 0;
    if (active) target_alpha = alpha_inherit_get_alpha();

    float target[2] = {target_alpha, 0};
    motion_update(&instance->motion, target, GetFrameTime());
    instance->alpha = instance->motion.position[0];

    // printf("[%zx]: %f on:%d ta:%f mp:%f \n", instance_id, instance->time_hovering, active, target_alpha,
    //        instance->motion.position[0]);
    if (instance->alpha < .1) return;
    instance->desc_len = strlen(desc);
    memcpy(instance->desc, desc, instance->desc_len);
    instance->text_size = ff_measure_utf8(desc, strlen(desc), *g_style);

    instance->bg.height = instance->text_size.height + g_cfg.inner_gap2;
    instance->bg.y = bounds.y - instance->bg.height - g_cfg.inner_gap;
    instance->bg.width = MAX(instance->text_size.width + g_cfg.inner_gap2, hint_min_width());
    instance->bg.x = bounds.x + bounds.width * .5 - instance->bg.width * .5;
    Color color = GET_RCOLOR(COLOR_SURFACE1);
    color.a = instance->alpha;

    instance->indicator_m_x = Clamp(mouse.x, bounds.x, bounds.x + bounds.width);
    memset(instance->desc, 0, sizeof(instance->desc));
    memcpy(instance->desc, desc, strlen(desc));
}

void hint_end_frame(void) {
    printf("%ld\n", g_instances_len);
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
        style.typo.color |= (int)instance->alpha;
        ff_draw_str8(instance->desc, instance->desc_len,
                     instance->bg.x + instance->bg.width * .5 - instance->text_size.width * .5,
                     instance->bg.y + instance->bg.height * .5 - instance->text_size.height * .5,
                     (float*)g_cfg.global_projection, style);
    }
}
