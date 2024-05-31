#include "clip.h"

#include <assert.h>
#include <glad.h>
#include <raylib.h>
#include <rlgl.h>

short g_layer = 0;

void clip_init(void) { glEnable(GL_STENCIL_TEST); }

void clip_end_frame(void) { assert(g_layer == 0 && "unmatched clip_begin and clip_end"); }

void clip_begin_custom_shape(void) {
    assert(g_layer < 0xff && g_layer >= 0);
    rlDrawRenderBatchActive();
    g_layer += 1;
    if (g_layer == 1) {
        glClear(GL_STENCIL_BUFFER_BIT);
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
        glClear(GL_STENCIL_BUFFER_BIT);
        glStencilFunc(GL_ALWAYS, g_layer, 0xff);
        glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
    } else {
        glStencilFunc(GL_EQUAL, g_layer - 1, 0xff);
        glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
    }

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glStencilMask(0xff);
    DrawRectangle(x, y, w, h, WHITE);
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
        glStencilFunc(GL_EQUAL, g_layer - 1, 0xff);
        glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
    }

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    DrawRectangleRounded((Rectangle){x, y, w, h}, rad, 32, WHITE);
    rlDrawRenderBatchActive();

    glStencilMask(0x00);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glStencilFunc(GL_EQUAL, g_layer, 0xFF);
}

void clip_end(void) {
    assert(g_layer < 0xff && g_layer >= 0);
    rlDrawRenderBatchActive();
    if (!g_layer) return;
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
