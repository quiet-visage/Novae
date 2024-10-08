#include "clip.h"

#include <assert.h>
#include <glad.h>
#include <raylib.h>
#include <rlgl.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

typedef enum { CLIP_SHAPE_RECTANGLE, CLIP_SHAPE_RECTANGLE_ROUNDED } Clip_Shape;

typedef union {
    size_t radius;
} Clip_Parameter;

typedef struct {
    Clip_Shape shape;
    Rectangle bounds;
    Clip_Parameter parameter;
} Clip_Call;

short g_layer = 0;
Clip_Call g_callstack[0xff] = {0};
size_t g_callstack_len = 0;
bool g_layer_was_previously_used[0xff] = {0};

void clip_begin_primitive() {
    assert(g_layer < 0xff && g_layer >= 0);
    g_layer += 1;
    rlDrawRenderBatchActive();
    glStencilMask(0xff);
    if (g_layer == 1) {
        glClear(GL_STENCIL_BUFFER_BIT);
        glStencilFunc(GL_ALWAYS, g_layer, 0xff);
        glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
    } else {
        glStencilFunc(GL_EQUAL, g_layer - 1, 0xff);
        glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
    }
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
}

void clip_end_primitive() {
    rlDrawRenderBatchActive();
    glStencilMask(0x00);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glStencilFunc(GL_EQUAL, g_layer, 0xFF);
}

void clip_begin_call(Clip_Call call) {
    clip_begin_primitive();

    assert(!g_layer_was_previously_used[g_layer]);
    g_layer_was_previously_used[g_layer] = 1;

    switch (call.shape) {
        case CLIP_SHAPE_RECTANGLE: DrawRectangleRec(call.bounds, WHITE); break;
        case CLIP_SHAPE_RECTANGLE_ROUNDED:
            DrawRectangleRounded(call.bounds, RADIUS_TO_ROUNDNESS(call.parameter.radius, call.bounds.height),
                                 g_cfg.rounded_rec_segments, WHITE);
            break;
    }

    clip_end_primitive();
}

void clip_recall() {
    glStencilMask(0xff);
    glClear(GL_STENCIL_BUFFER_BIT);
    g_layer = 0;
    memset(g_layer_was_previously_used, 0, sizeof(g_layer_was_previously_used));

    for (size_t i = 0; i < g_callstack_len; ++i) {
        clip_begin_call(g_callstack[i]);
    }
}

void clip_begin_custom_shape(void) {
    assert(g_layer == 0);

    g_callstack_len++;  // first layer doens't really need to be store, so just skip.
    clip_begin_primitive();
}

void clip_end_custom_shape(void) { clip_end_primitive(); }

void clip_begin(Rectangle bounds) {
    Clip_Call call = {0};
    call.bounds = bounds;
    call.shape = CLIP_SHAPE_RECTANGLE;

    g_callstack[g_callstack_len++] = call;
    if (g_layer_was_previously_used[g_layer + 1]) {
        return clip_recall();
    }

    clip_begin_call(call);
}

void clip_begin_rounded(Rectangle bounds, float radius) {
    Clip_Call call = {0};
    call.bounds = bounds;
    call.shape = CLIP_SHAPE_RECTANGLE_ROUNDED;
    call.parameter.radius = radius;

    g_callstack[g_callstack_len++] = call;
    if (g_layer != 1 && g_layer_was_previously_used[g_layer + 1]) {
        return clip_recall();
    }

    clip_begin_call(call);
}

void clip_end(void) {
    assert(g_layer < 0xff && g_layer >= 0);
    rlDrawRenderBatchActive();
    if (g_callstack_len > 1) {
        g_callstack_len--;
        clip_recall();
    } else {
        g_layer = 0;
        g_callstack_len = 0;
        glStencilMask(0xff);
        glClear(GL_STENCIL_BUFFER_BIT);
        glStencilFunc(GL_ALWAYS, g_layer, 0xFF);
        glStencilMask(0);
    }
}

void clip_init(void) { glEnable(GL_STENCIL_TEST); }

void clip_end_frame(void) {
    assert(g_layer == 0 && "unmatched clip_begin and clip_end");
    memset(g_layer_was_previously_used, 0, sizeof(g_layer_was_previously_used));
}
