#pragma once

typedef struct {
    int id;
    char* name;
    unsigned color;
} Tag;

float tag_min_width();
float tag_min_height();
void tag_draw(float x, float y);
