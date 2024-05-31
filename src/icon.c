#include "icon.h"

#include <assert.h>
#include <stddef.h>

#include "raylib.h"

#define ICON_RELATIVE_FOLDER "resources/icons"
#define ICON_PATH(NAME) (ICON_RELATIVE_FOLDER "/" NAME)

const char* g_icon_paths[] = {[ICON_ADD_CIRCLE] = ICON_PATH("add_circle.svg"),
                              [ICON_INFINITE] = ICON_PATH("infinite.svg"),
                              [ICON_MOVE_UP] = ICON_PATH("move_up.svg")};
Texture g_icons[ICON_COUNT] = {0};
static_assert(sizeof(g_icon_paths) / sizeof(*g_icon_paths) == ICON_COUNT &&
              "Icon paths need to be updated");

void icon_init(void) {
    assert(DirectoryExists(ICON_RELATIVE_FOLDER));
    for (size_t i = 0; i < ICON_COUNT; i += 1) {
        const char* path = g_icon_paths[i];
        assert(FileExists(path));
        assert(IsFileExtension(path, ".svg"));
        Image icon_img = LoadImageSvg(path, ICON_SIZE, ICON_SIZE);
        ImageBlurGaussian(&icon_img, ICON_SIZE / 64);
        g_icons[i] = LoadTextureFromImage(icon_img);
        UnloadImage(icon_img);
    }
}

void icon_terminate(void) {
    for (size_t i = 0; i < ICON_COUNT; i += 1) {
        UnloadTexture(g_icons[i]);
    }
}

Texture icon_get(Icon icon) { return g_icons[icon]; }
