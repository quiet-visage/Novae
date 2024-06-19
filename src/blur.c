#include "blur.h"

#include <assert.h>
#include <math.h>
#include <raymath.h>
#include <stdio.h>

#include "raylib.h"
#include "shader.h"

RenderTexture g_render_tex0;
RenderTexture g_render_tex1;
RenderTexture g_render_tex2;
int g_uni_resolution;
int g_uni_radius;
int g_uni_dir;
int g_uni_texture1;
Shader g_blur_shader;
Shader g_mix_shader;

void blur_init(void) {
    float w = 3840;
    float h = 2160;
    g_render_tex0 = LoadRenderTexture(w, h);
    g_render_tex1 = LoadRenderTexture(w, h);
    g_render_tex2 = LoadRenderTexture(w, h);
    g_blur_shader = shader_get(SHADER_HORZ_BLUR);
    g_mix_shader = shader_get(SHADER_MIX);
    g_uni_resolution = GetShaderLocation(g_blur_shader, "resolution");
    g_uni_radius = GetShaderLocation(g_blur_shader, "radius");
    g_uni_dir = GetShaderLocation(g_blur_shader, "dir");
    g_uni_texture1 = GetShaderLocation(g_mix_shader, "texture1");
}

void blur_terminate(void) {
    UnloadRenderTexture(g_render_tex0);
    UnloadRenderTexture(g_render_tex1);
    UnloadRenderTexture(g_render_tex2);
}

void blur_begin(void) {
    BeginTextureMode(g_render_tex0);
    ClearBackground(BLANK);
}

void blur_end(float width, float height, unsigned tap) {
    assert(tap <= 33);
    EndTextureMode();

    float radius = 1.;
    float hresolution = 1920;
    Vector2 hdir = {1., 0.};
    float vresolution = 1080;
    Vector2 vdir = {0., 1.};

    for (size_t i = 0; i < tap; i += 1) {
        BeginTextureMode(g_render_tex1);
        BeginShaderMode(g_blur_shader);
        SetShaderValue(g_blur_shader, g_uni_radius, &radius, SHADER_UNIFORM_FLOAT);
        SetShaderValue(g_blur_shader, g_uni_resolution, &hresolution, SHADER_UNIFORM_FLOAT);
        SetShaderValue(g_blur_shader, g_uni_dir, &hdir, SHADER_UNIFORM_VEC2);
        DrawTexture(g_render_tex0.texture, 0, 0, WHITE);
        EndShaderMode();
        EndTextureMode();

        BeginTextureMode(g_render_tex2);
        BeginShaderMode(g_blur_shader);
        SetShaderValue(g_blur_shader, g_uni_radius, &radius, SHADER_UNIFORM_FLOAT);
        SetShaderValue(g_blur_shader, g_uni_resolution, &vresolution, SHADER_UNIFORM_FLOAT);
        SetShaderValue(g_blur_shader, g_uni_dir, &vdir, SHADER_UNIFORM_VEC2);
        DrawTexture(g_render_tex0.texture, 0, 0, WHITE);
        EndShaderMode();
        EndTextureMode();

        BeginTextureMode(g_render_tex0);
        ClearBackground(BLANK);
        BeginShaderMode(g_mix_shader);
        SetShaderValueTexture(g_mix_shader, g_uni_texture1, g_render_tex1.texture);
        DrawTexture(g_render_tex2.texture, 0, 0, WHITE);
        EndShaderMode();
        EndTextureMode();
    }

    RenderTexture rnd = g_render_tex0;
    Rectangle source = {0, rnd.texture.height - height, width, -height};
    Rectangle dest = {0, 0, width, height};
    Vector2 orig = {0, 0};
    DrawTexturePro(rnd.texture, source, dest, orig, 0., WHITE);
}
