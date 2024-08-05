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
#include "colors.h"
#include "config.h"
#include "db_cache.h"
#include "fieldfusion.h"
#include "shader.h"

#define BASE_IMG_SIZE 400.f
#define DEG_TIME_STEP_SCALE 110.f
#define PARTICLE_RADIUS 30.f
#define ORBIT_RADIUS 150.f
#define RADIUS_DECAY 0.4f
#define ALPHA_DECAY 1
#define DEGREE_DECAY 1.5f

static Image g_img;
static float g_deg;
static FF_Style* g_style = &g_cfg.sstyle;
float g_streak_count;

void streak_init(void) { g_img = GenImageColor(BASE_IMG_SIZE, BASE_IMG_SIZE, BLANK); }

static void streak_update_deg(void) { g_deg = fmodf((GetTime() * DEG_TIME_STEP_SCALE), 360.f); }

static void draw_particle(float deg, Color base_color) {
    Color col0 = base_color;
    const Vector2 pos = {BASE_IMG_SIZE * .5f, BASE_IMG_SIZE * .5f};
    Vector2 pos0 = {pos.x + cosf(DEG2RAD * deg) * ORBIT_RADIUS, pos.y + sinf(DEG2RAD * deg) * ORBIT_RADIUS};

    float radius = PARTICLE_RADIUS;
    DrawCircle(pos0.x, pos0.y, radius, col0);
    for (size_t i = 0; i < 70; i += 1) {
        if (col0.a == 0) continue;
        float ndeg = deg - DEGREE_DECAY * i;
        pos0.x = pos.x + cosf(DEG2RAD * ndeg) * ORBIT_RADIUS;
        pos0.y = pos.y + sinf(DEG2RAD * ndeg) * ORBIT_RADIUS;
        DrawCircle(pos0.x, pos0.y, radius, col0);

        radius -= RADIUS_DECAY;
        radius = Clamp(radius, 1, PARTICLE_RADIUS);

        int alpha = col0.a;
        alpha -= ALPHA_DECAY;
        alpha = MAX(alpha, 0.0);
        col0.a = alpha;
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

    static bool r1 = 0;
    static Post_Proc_Draw_Info draw_info = {0};
    if (!r1) {
        blur_begin();
        draw_particle(g_deg, GET_RCOLOR(COLOR_SKY));
        Color col = GET_RCOLOR(COLOR_TEAL);
        draw_particle(g_deg + 180.f, col);
        draw_info = blur_end_return(BASE_IMG_SIZE, BASE_IMG_SIZE, 4);
        r1 = 1;
    }
    BeginShaderMode(shader_get(SHADER_BLOOM));
    draw_info.dest.x = ring_particle_x + ring_particle_sz * .5;
    draw_info.dest.y = ring_particle_y + ring_particle_sz * .5;
    draw_info.dest.width = ring_particle_sz;
    draw_info.dest.height = ring_particle_sz;
    Vector2 origin = {draw_info.dest.width * .5, draw_info.dest.height * .5};
    float rotation = GetTime() * 1e2;
    DrawTexturePro(draw_info.texture, draw_info.source, draw_info.dest, origin, rotation, WHITE);
    EndShaderMode();
}

void streak_terminate() { UnloadImage(g_img); }
