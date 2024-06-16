#pragma once

#include <raylib.h>

#define MAX_TAG_LEN 24

typedef struct {
    int id;
    char* name;
    unsigned color;
} Tag;

float tag_core_width(Tag* tag);
float tag_core_height(void);
float tag_width(Tag* tag);
float tag_height(void);
Rectangle tag_draw(Tag* tag, float x, float y);
