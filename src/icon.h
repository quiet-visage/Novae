#pragma once
#include <raylib.h>

#define ICON_SIZE 128
typedef enum {
    ICON_ADD_CIRCLE,
    ICON_INFINITE,
    ICON_MOVE_UP,
    ICON_TARGET,
    ICON_ARROW_UP,
    ICON_TAG_ADD,
    ICON_SEARCH,
    ICON_COUNT
} Icon;

void icon_init(void);
void icon_terminate(void);
Texture icon_get(Icon icon);
