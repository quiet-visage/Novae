#include "clip.h"

#include <assert.h>
#include <glad.h>
#include <raylib.h>
#include <rlgl.h>
#include <stdio.h>
#include <string.h>

short g_layer = 0;

bool g_layer_was_previously_used[256] = {0};

void clip_init(void) {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
}

void clip_end_frame(void) {
    assert(g_layer == 0 && "unmatched clip_begin and clip_end");
    memset(g_layer_was_previously_used, 0, sizeof(g_layer_was_previously_used));
}

void clip_begin_custom_shape(void) {
    assert(g_layer < 0xff && g_layer >= 0);
    rlDrawRenderBatchActive();
    g_layer += 1;
    if (g_layer == 1) {
        // glClear(GL_STENCIL_BUFFER_BIT);
        glClear(GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glStencilFunc(GL_ALWAYS, g_layer, 0xff);
        glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
    } else {
        glStencilFunc(GL_EQUAL, g_layer - 1, 0xff);
        glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
    }

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glStencilMask(0xff);
}

void clip_end_custom_shape(void) {
    rlDrawRenderBatchActive();

    glStencilMask(0x00);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glStencilFunc(GL_EQUAL, g_layer, 0xFF);
}

void clip_begin(float x, float y, float w, float h) {
    assert(g_layer < 0xff && g_layer >= 0);
    rlDrawRenderBatchActive();
    g_layer += 1;
    if (g_layer == 1) {
        glStencilMask(0xff);
        glClear( GL_DEPTH_BUFFER_BIT);
        glStencilFunc(GL_ALWAYS, g_layer, 0xff);
        glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
    } else {
        if (g_layer_was_previously_used[g_layer]) {
            glStencilMask(1);
            glClear(GL_STENCIL_BUFFER_BIT);

            glStencilFunc(GL_EQUAL, g_layer - 1, 0xff);
            glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
        } else {
            glStencilFunc(GL_EQUAL, g_layer - 1, 0xff);
            glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
        }

        g_layer_was_previously_used[g_layer] = 1;
    }

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glStencilMask(0xff);
    DrawRectangle(x, y, w, h, BLACK);
    rlDrawRenderBatchActive();

    glStencilMask(0x00);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glStencilFunc(GL_EQUAL, g_layer, 0xFF);
}

void clip_begin_rounded(float x, float y, float w, float h, float rad) {
    assert(g_layer < 0xff && g_layer >= 0);
    rlDrawRenderBatchActive();
    g_layer += 1;
    glStencilMask(0xff);
    if (g_layer == 1) {
        glClear(GL_STENCIL_BUFFER_BIT);
        glStencilFunc(GL_ALWAYS, g_layer, 0xff);
        glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
    } else {
        if (g_layer_was_previously_used[g_layer]) {
            glStencilMask(1);
            glClear(GL_STENCIL_BUFFER_BIT);

            glStencilFunc(GL_EQUAL, g_layer - 1, 0xff);
            glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
        } else {
            glStencilFunc(GL_EQUAL, g_layer - 1, 0xff);
            glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
        }

        g_layer_was_previously_used[g_layer] = 1;
    }

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    DrawRectangleRounded((Rectangle){x, y, w, h}, rad, 24, WHITE);
    rlDrawRenderBatchActive();

    glStencilMask(0x00);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glStencilFunc(GL_EQUAL, g_layer, 0xFF);
}

void clip_end(void) {
    assert(g_layer < 0xff && g_layer >= 0);
    rlDrawRenderBatchActive();
    if (!g_layer) return;

    if (g_layer < 256) {
        size_t clear_idx = g_layer + 1;
        bool* clear_ptr = &g_layer_was_previously_used[clear_idx];
        size_t clear_sz = (256 - clear_idx) * sizeof(*g_layer_was_previously_used);
        memset(clear_ptr, 0, clear_sz);
    }

    if (g_layer == 1) {
        glStencilMask(0xff);
        glClear(GL_STENCIL_BUFFER_BIT);
        glStencilFunc(GL_ALWAYS, g_layer, 0xFF);
        glStencilMask(0);
        g_layer -= 1;
        return;
    }
    glStencilFunc(GL_EQUAL, g_layer, 0xFF);
    g_layer -= 1;
}

void clip_print_layer(void) { printf("%d\n", g_layer); }
