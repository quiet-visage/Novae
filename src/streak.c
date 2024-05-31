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

#include "config.h"
#include "db_cache.h"
#include "fieldfusion.h"
#include "shader.h"

#define BASE_IMG_SIZE 200.f
#define DEG_TIME_STEP_SCALE 100.f
#define PARTICLE_RADIUS 8.f
#define ORBIT_RADIUS 90.f
#define PARTICLE_BLUR_RADIUS 2
#define RADIUS_DECAY .1f
#define ALPHA_DECAY 3
#define DEGREE_DECAY 1.5f

static Image g_img;
static Texture g_texture;
static RenderTexture g_render_texture;
static RenderTexture g_render_texture0;
static RenderTexture g_render_texture1;
static Color g_base_color;
static float g_deg;
static FF_Style* g_style = &g_cfg.sstyle;
float g_streak_count;

void streak_init(void) {
    g_img = GenImageColor(BASE_IMG_SIZE, BASE_IMG_SIZE, BLANK);
    g_texture = LoadTextureFromImage(g_img);
    g_render_texture = LoadRenderTexture(BASE_IMG_SIZE, BASE_IMG_SIZE);
    g_render_texture0 = LoadRenderTexture(BASE_IMG_SIZE, BASE_IMG_SIZE);
    // SetTextureFilter(g_render_texture.texture, TEXTURE_FILTER_BILINEAR);
    g_render_texture1 = LoadRenderTexture(BASE_IMG_SIZE, BASE_IMG_SIZE);
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

static void streak_update_image() {
    BeginTextureMode(g_render_texture);
    ClearBackground(BLANK);
    draw_particle(g_deg, g_base_color);
    Color col = GET_RCOLOR(COLOR_TEAL);
    draw_particle(g_deg + 180.f, col);
    EndTextureMode();

    float tap = 1;
    Shader horz_blur = shader_get(SHADER_HORZ_BLUR);
    Shader vert_blur = shader_get(SHADER_VERT_BLUR);
    Shader mix = shader_get(SHADER_MIX);
    int texture1 = GetShaderLocation(mix, "texture1");
    for (size_t i = 0; i < tap; i += 1) {
        BeginTextureMode(g_render_texture0);
        ClearBackground(BLANK);
        BeginShaderMode(horz_blur);
        DrawTexture(g_render_texture.texture, 0, 0, WHITE);
        EndShaderMode();
        EndTextureMode();

        BeginTextureMode(g_render_texture1);
        ClearBackground(BLANK);
        BeginShaderMode(vert_blur);
        DrawTexture(g_render_texture.texture, 0, 0, WHITE);
        EndShaderMode();
        EndTextureMode();

        BeginTextureMode(g_render_texture);
        BeginShaderMode(mix);
        SetShaderValueTexture(mix, texture1, g_render_texture1.texture);
        DrawTexture(g_render_texture0.texture, 0, 0, WHITE);
        EndShaderMode();
        EndTextureMode();
    }
}

void streak_draw(float x, float y) {
    streak_update_deg();
    streak_update_image();

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

    static const Rectangle src = {0, 0, BASE_IMG_SIZE, BASE_IMG_SIZE};
    static const Vector2 orig = {0};
    Rectangle dst = {ring_particle_x, ring_particle_y, ring_particle_sz, ring_particle_sz};

    BeginShaderMode(shader_get(SHADER_BLOOM));
    DrawTexturePro(g_render_texture.texture, src, dst, orig, 0, WHITE);
    EndShaderMode();
}

void streak_terminate() {
    UnloadImage(g_img);
    UnloadTexture(g_texture);
    UnloadRenderTexture(g_render_texture);
    UnloadRenderTexture(g_render_texture0);
    UnloadRenderTexture(g_render_texture1);
}
