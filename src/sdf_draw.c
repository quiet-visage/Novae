#include "sdf_draw.h"

#include <raylib.h>

#include "shader.h"

Texture g_texture = {0};

void sdf_draw_init(void) {
    float size = 4000;
    Image img = GenImageColor(size, size, BLANK);
    g_texture = LoadTextureFromImage(img);
    UnloadImage(img);
}

void sdf_draw_terminate(void) { UnloadTexture(g_texture); }

void sdf_draw_rounded_rectangle(float x, float y, float w, float h, float roundness, Color color) {
    Shader shader = shader_get(SHADER_ROUND_REC);
    BeginShaderMode(shader);

    int locr = GetShaderLocation(shader, "iResolution");
    Vector2 dim = {w, h};
    dim.x = w / g_texture.width;
    dim.y = h / g_texture.height;
    SetShaderValue(shader, locr, &dim, SHADER_UNIFORM_VEC2);
    int locrr = GetShaderLocation(shader, "uroundness");
    float rnd = 0.6;
    SetShaderValue(shader, locrr, &rnd, SHADER_UNIFORM_FLOAT);
    DrawTextureEx(g_texture, (Vector2){x, y}, 0., 1., color);
    EndShaderMode();
}

void sdf_draw_rounded_rectangle_glow(float x, float y, float w, float h, float roundness, Color color) {
    Shader shader = shader_get(SHADER_ROUND_REC_GLOW);
    BeginShaderMode(shader);

    int locr = GetShaderLocation(shader, "iResolution");
    Vector2 dim = {w, h};
    dim.x = w / g_texture.width;
    dim.y = h / g_texture.height;
    SetShaderValue(shader, locr, &dim, SHADER_UNIFORM_VEC2);
    int locrr = GetShaderLocation(shader, "uroundness");
    float rnd = 0.6;
    SetShaderValue(shader, locrr, &rnd, SHADER_UNIFORM_FLOAT);
    DrawTextureEx(g_texture, (Vector2){x, y}, 0., 1., color);
    EndShaderMode();
}
