#pragma once

#include <raylib.h>

void blur_init(void);
void blur_terminate(void);
void blur_begin(void);
void blur_end(float width, float height, unsigned tap);
