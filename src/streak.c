#include "streak.h"

/*
TODO : FIX the blur by having 3 textures
do a 9 tap or 33 tap gaussian blur
fix the gaussian blur alpha
maybe this:
https://stackoverflow.com/questions/35476142/gaussian-blur-handle-with-alpha-transparency
*/

#include <math.h>
#include <raylib.h>
#include <raymath.h>

#include "blur.h"
#include "config.h"
#include "db_cache.h"
#include "fieldfusion.h"

#define BASE_IMG_SIZE 200.f
#define DEG_TIME_STEP_SCALE 100.f
#define PARTICLE_RADIUS 8.f
#define ORBIT_RADIUS 90.f
#define PARTICLE_BLUR_RADIUS 2
#define RADIUS_DECAY .1f
#define ALPHA_DECAY 0
#define DEGREE_DECAY 1.5f

static Image g_img;
static RenderTexture g_render_texture;
static Color g_base_color;
static float g_deg;
static FF_Style* g_style = &g_cfg.sstyle;
float g_streak_count;

void streak_init(void) {
    g_img = GenImageColor(BASE_IMG_SIZE, BASE_IMG_SIZE, BLANK);
    g_base_color = GET_RCOLOR(COLOR_MAUVE);
}

static void streak_update_deg(void) { g_deg = fmodf((GetTime() * DEG_TIME_STEP_SCALE), 360.f); }

static void draw_particle(float deg, Color base_color) {
    Color col0 = base_color;
    const Vector2 pos = {BASE_IMG_SIZE * .5f, BASE_IMG_SIZE * .5f};
    Vector2 pos0 = {pos.x + cosf(DEG2RAD * deg) * ORBIT_RADIUS,
                    pos.y + sinf(DEG2RAD * deg) * ORBIT_RADIUS};

    float radius = PARTICLE_RADIUS;
    DrawCircle(pos0.x, pos0.y, radius, col0);
    for (size_t i = 0; i < 100; i += 1) {
        float ndeg = deg - DEGREE_DECAY * i;
        pos0.x = pos.x + cosf(DEG2RAD * ndeg) * ORBIT_RADIUS;
        pos0.y = pos.y + sinf(DEG2RAD * ndeg) * ORBIT_RADIUS;
        DrawCircle(pos0.x, pos0.y, radius, col0);

        radius -= RADIUS_DECAY;
        radius = Clamp(radius, 2, 10);

        col0.a = col0.a < 0 ? 0 : col0.a;
        if (col0.a <= ALPHA_DECAY)
            col0.a = 0;
        else
            col0.a -= ALPHA_DECAY;
    }
}

void streak_draw(float x, float y) {
    streak_update_deg();

    char streak_str[64] = {0};
    size_t streak_str_len = snprintf(streak_str, 64, "%ld", db_cache_get_streak());

    float t_width = ff_measure_utf8(streak_str, streak_str_len, *g_style).width;

    // center streak on coordinates;
    x = x - t_width * .5f;
    y = y - g_style->typo.size * .5f;

    ff_draw_str8(streak_str, streak_str_len, x, y, (float*)g_cfg.global_projection, *g_style);
    float ring_particle_sz = g_style->typo.size * 3;
    float ring_particle_x = x + t_width * .5f - ring_particle_sz * .5f;
    float ring_particle_y = y + g_style->typo.size * .5f - ring_particle_sz * .5f;

    // BeginShaderMode(shader_get(SHADER_BLOOM));
    blur_begin();
    draw_particle(g_deg, g_base_color);
    Color col = GET_RCOLOR(COLOR_TEAL);
    draw_particle(g_deg + 180.f, col);
    blur_end(g_render_texture.texture.width, g_render_texture.texture.height, 1);
    // EndShaderMode();
}

void streak_terminate() {
    UnloadImage(g_img);
    UnloadRenderTexture(g_render_texture);
}
