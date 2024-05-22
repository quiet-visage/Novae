#include "streak.h"

#include <math.h>
#include <raylib.h>
#include <raymath.h>

#include "config.h"
#include "fieldfusion.h"

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
static Color g_base_color;
static float g_deg;
static Shader g_bloom;
float g_streak_count;

void streak_init(Shader bloom) {
    g_img = GenImageColor(BASE_IMG_SIZE, BASE_IMG_SIZE, BLANK);
    g_texture = LoadTextureFromImage(g_img);
    g_render_texture =
        LoadRenderTexture(BASE_IMG_SIZE, BASE_IMG_SIZE);
    g_base_color = GetColor(g_color[g_cfg.theme][COLOR_MAUVE]);
    g_bloom = bloom;
}

static void streak_update_deg() {
    g_deg = fmodf((GetTime() * DEG_TIME_STEP_SCALE), 360.f);
}

static void streak_update_image() {
    ImageClearBackground(&g_img, BLANK);
    Color col0 = g_base_color;
    const Vector2 pos = {BASE_IMG_SIZE * .5f, BASE_IMG_SIZE * .5f};
    Vector2 pos0 = {pos.x + cosf(DEG2RAD * g_deg) * ORBIT_RADIUS,
                    pos.y + sinf(DEG2RAD * g_deg) * ORBIT_RADIUS};

    float radius = PARTICLE_RADIUS;
    ImageDrawCircle(&g_img, pos0.x, pos0.y, radius, col0);
    for (size_t i = 0; i < 100; i += 1) {
        float ndeg = g_deg - DEGREE_DECAY * i;
        pos0.x = pos.x + cosf(DEG2RAD * ndeg) * ORBIT_RADIUS;
        pos0.y = pos.y + sinf(DEG2RAD * ndeg) * ORBIT_RADIUS;
        ImageDrawCircle(&g_img, pos0.x, pos0.y, radius, col0);

        radius -= RADIUS_DECAY;
        radius = Clamp(radius, 2, 10);

        col0.a = col0.a < 0 ? 0 : col0.a;
        if (col0.a <= ALPHA_DECAY)
            col0.a = 0;
        else
            col0.a -= ALPHA_DECAY;
    }

    float mdeg = 180.f + g_deg;
    pos0 = (Vector2){pos.x + cosf(DEG2RAD * mdeg) * ORBIT_RADIUS,
                     pos.y + sinf(DEG2RAD * mdeg) * ORBIT_RADIUS};
    radius = PARTICLE_RADIUS;
    col0 = GetColor(g_color[g_cfg.theme][COLOR_TEAL]);
    ImageDrawCircle(&g_img, pos0.x, pos0.y, radius, col0);
    for (size_t i = 0; i < 100; i += 1) {
        float ndeg = (mdeg)-DEGREE_DECAY * i;
        pos0.x = pos.x + cosf(DEG2RAD * ndeg) * ORBIT_RADIUS;
        pos0.y = pos.y + sinf(DEG2RAD * ndeg) * ORBIT_RADIUS;
        ImageDrawCircle(&g_img, pos0.x, pos0.y, radius, col0);

        radius -= RADIUS_DECAY;
        radius = Clamp(radius, 2, 10);

        col0.a = col0.a < 0 ? 0 : col0.a;
        if (col0.a <= ALPHA_DECAY)
            col0.a = 0;
        else
            col0.a -= ALPHA_DECAY;
    }
    ImageBlurGaussian(&g_img, PARTICLE_BLUR_RADIUS);
}

static void streak_update_texture() {
    UpdateTexture(g_texture, g_img.data);
}

static void streak_update_render_texture() {
    BeginTextureMode(g_render_texture);
    ClearBackground(BLANK);
    DrawTexture(g_texture, 0, 0, WHITE);
    EndTextureMode();
}

void streak_draw(void) {
    streak_update_deg();
    streak_update_image();
    streak_update_texture();
    streak_update_render_texture();

    float x = 0;
    float y = 0;
    float t_width = ff_measure_utf32(L"130", 3, g_cfg.btn_typo.font,
                                     g_cfg.btn_typo.size, 0)
                        .width;
    ff_draw_str32(L"130", 3, x, y, (float*)g_cfg.global_projection,
                  g_cfg.btn_typo, 0, 0);
    float ring_particle_sz = g_cfg.btn_typo.size * 3;
    float ring_particle_x =
        x + t_width * .5f - ring_particle_sz * .5f;
    float ring_particle_y =
        y + g_cfg.btn_typo.size * .5f - ring_particle_sz * .5f;

    BeginShaderMode(g_bloom);
    static const Rectangle src = {0, 0, BASE_IMG_SIZE, BASE_IMG_SIZE};
    static const Vector2 orig = {0};
    Rectangle dst = {ring_particle_x, ring_particle_y, ring_particle_sz, ring_particle_sz};
    DrawTexturePro(g_render_texture.texture, src, dst, orig, 0, WHITE);
    EndShaderMode();
}

void streak_terminate() {
    UnloadImage(g_img);
    UnloadTexture(g_texture);
    UnloadRenderTexture(g_render_texture);
}
