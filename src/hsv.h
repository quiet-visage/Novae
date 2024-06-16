#pragma once

#include "raylib.h"
typedef struct {
    float hue;
    float saturation;
    float value;
} HSV;

HSV rgb2hsv(Color col);
Color hsv2rgb(HSV hsv);
int hsv2rgb_i(HSV hsv);
