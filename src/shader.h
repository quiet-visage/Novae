#pragma once

#include <raylib.h>

typedef enum {
    SHADER_BLUR,
    SHADER_BLOOM,
    SHADER_MIX,
    SHADER_ROUND_REC,
    SHADER_ROUND_REC_GLOW,
    SHADER_COUNT,
} Shader_Kind;

Shader shader_get(Shader_Kind kind);
void shader_reload(void);
void shader_terminate(void);
