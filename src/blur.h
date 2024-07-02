#pragma once

#include <raylib.h>

typedef struct {
    Texture texture;  // no need to unload
    Rectangle source;
    Rectangle dest;
} Post_Proc_Draw_Info;

void blur_init(void);
void blur_terminate(void);
void blur_begin(void);
void blur_end(float width, float height, unsigned tap);
Post_Proc_Draw_Info blur_end_return(float width, float height, unsigned tap);
