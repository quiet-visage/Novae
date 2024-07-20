#include "clip.h"

#include <assert.h>
#include <glad.h>
#include <raylib.h>
#include <rlgl.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

typedef enum { CLIP_SHAPE_RECTANGLE, CLIP_SHAPE_RECTANGLE_ROUNDED } Clip_Shape;

typedef struct {
    Clip_Shape shape;
    Rectangle bounds;
    size_t radius;  // should be union if more paremeters are needed
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
            DrawRectangleRounded(call.bounds, RADIUS_TO_ROUNDNESS(call.radius, call.bounds.height),
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
    call.radius = radius;

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

// void clip_begin_custom_shape(void) {
//     assert(g_layer < 0xff && g_layer >= 0);
//     rlDrawRenderBatchActive();
//     g_layer += 1;
//     if (g_layer == 1) {
//         // glClear(GL_STENCIL_BUFFER_BIT);
//         glClear(GL_STENCIL_BUFFER_BIT);
//         glStencilFunc(GL_ALWAYS, g_layer, 0xff);
//         glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
//     } else {
//         glStencilFunc(GL_EQUAL, g_layer - 1, 0xff);
//         glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
//     }

//     glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
//     glStencilMask(0xff);
// }

// void clip_end_custom_shape(void) {
//     rlDrawRenderBatchActive();

//     glStencilMask(0x00);
//     glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
//     glStencilFunc(GL_EQUAL, g_layer, 0xFF);
// }

// void clip_begin(float x, float y, float w, float h) {
//     assert(g_layer < 0xff && g_layer >= 0);
//     // Color col[5] = {WHITE, RED, VIOLET, BLUE, DARKBLUE};
//     // DrawRectangleLinesEx((Rectangle){x, y, w, h}, 2., col[g_layer]);
//     rlDrawRenderBatchActive();
//     g_layer += 1;
//     glStencilMask(0xff);
//     if (g_layer == 1) {
//         glClear(GL_DEPTH_BUFFER_BIT);
//         glStencilFunc(GL_ALWAYS, g_layer, 0xff);
//         glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
//     } else {
//         glStencilFunc(GL_EQUAL, g_layer - 1, 0xff);
//         glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);

//         g_layer_was_previously_used[g_layer] = 1;
//         glStencilMask(0xff);
//     }

//     glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
//     DrawRectangle(x, y, w, h, BLACK);
//     rlDrawRenderBatchActive();

//     glStencilMask(0x00);
//     glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
//     glStencilFunc(GL_EQUAL, g_layer, 0xFF);
// }

// void clip_begin_rounded(float x, float y, float w, float h, float rad) {
//     assert(g_layer < 0xff && g_layer >= 0);
//     g_layer += 1;
//     // Color col[5] = {WHITE, RED, VIOLET, BLUE, DARKBLUE};
//     // DrawRectangleRoundedLines((Rectangle){x, y, w, h}, rad, 24, 2., col[g_layer - 1]);
//     rlDrawRenderBatchActive();
//     // glStencilMask(g_layer);
//     glStencilMask(0xff);
//     if (g_layer == 1) {
//         glClear(GL_STENCIL_BUFFER_BIT);
//         glStencilFunc(GL_ALWAYS, g_layer, 0xff);
//         glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
//     } else {
//         glStencilFunc(GL_EQUAL, g_layer - 1, 0xff);
//         glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);

//         g_layer_was_previously_used[g_layer] = 1;
//         glStencilMask(0xff);
//     }

//     glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
//     DrawRectangleRounded((Rectangle){x, y, w, h}, rad, 24, WHITE);
//     rlDrawRenderBatchActive();

//     glStencilMask(0x00);
//     glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
//     glStencilFunc(GL_EQUAL, g_layer, 0xFF);
// }

// void clip_end(void) {
//     assert(g_layer < 0xff && g_layer >= 0);
//     rlDrawRenderBatchActive();
//     if (!g_layer) return;

//     if (g_layer < 256) {
//         size_t clear_idx = g_layer + 1;
//         bool* clear_ptr = &g_layer_was_previously_used[clear_idx];
//         size_t clear_sz = (256 - clear_idx) * sizeof(*g_layer_was_previously_used);
//         memset(clear_ptr, 0, clear_sz);
//     }

//     if (g_layer == 1) {
//         glStencilMask(0xff);
//         glClear(GL_STENCIL_BUFFER_BIT);
//         glStencilFunc(GL_ALWAYS, g_layer, 0xFF);
//         glStencilMask(0);
//         g_layer -= 1;
//         return;
//     } else {
//         glStencilMask(0b000001);
//         glClear(GL_STENCIL_BUFFER_BIT);
//     }
//     glStencilFunc(GL_EQUAL, g_layer, 0xFF);
//     g_layer -= 1;
// }

void clip_print_layer(void) { printf("%d\n", g_layer); }
