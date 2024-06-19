#pragma once

#include <raylib.h>

typedef enum { SHADER_HORZ_BLUR, SHADER_BLOOM, SHADER_MIX, SHADER_COUNT } Shader_Kind;

Shader shader_get(Shader_Kind kind);
void shader_reload(void);
void shader_terminate(void);
