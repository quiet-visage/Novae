#include "shader.h"

#include <raylib.h>
#include <stddef.h>

#define SHADER_FOLDER "shaders"

const char* g_shader_path[] = {
    [SHADER_BLUR] = SHADER_FOLDER "/blur.fs",
    [SHADER_BLOOM] = SHADER_FOLDER "/bloom.fs",
    [SHADER_MIX] = SHADER_FOLDER "/mix.fs",
    [SHADER_ROUND_REC] = SHADER_FOLDER "/round_rec.fs",
    [SHADER_ROUND_REC_GLOW] = SHADER_FOLDER "/round_rec_glow.fs",
};

Shader g_shaders[SHADER_COUNT] = {0};

static_assert(sizeof(g_shaders) / sizeof(*g_shaders) == SHADER_COUNT);
static_assert(sizeof(g_shader_path) / sizeof(char*) == SHADER_COUNT);

inline Shader shader_get(Shader_Kind kind) {
    if (g_shaders[kind].id) return g_shaders[kind];
    g_shaders[kind] = LoadShader(0, g_shader_path[kind]);
    return g_shaders[kind];
}

void shader_reload(void) {
    for (size_t i = 0; i < SHADER_COUNT; i += 1) {
        if (!g_shaders[i].id) continue;
        UnloadShader(g_shaders[i]);
        g_shaders[i] = LoadShader(0, g_shader_path[i]);
    }
}

void shader_terminate(void) {
    for (size_t i = 0; i < SHADER_COUNT; i += 1) {
        UnloadShader(g_shaders[i]);
    }
}
