#include "icon.h"

#include <assert.h>
#include <stddef.h>

#include "raylib.h"

#define ICON_RELATIVE_FOLDER "resources/icons"
#define ICON_PATH(NAME) (ICON_RELATIVE_FOLDER "/" NAME)

const char* g_icon_paths[] = {[ICON_ADD_CIRCLE] = ICON_PATH("add_circle.svg"),
                              [ICON_INFINITE] = ICON_PATH("infinite.svg"),
                              [ICON_MOVE_UP] = ICON_PATH("move_up.svg"),
                              [ICON_TARGET] = ICON_PATH("target.svg"),
                              [ICON_ARROW_UP] = ICON_PATH("arrow_up.svg"),
                              [ICON_TAG_ADD] = ICON_PATH("tag_add.svg"),
                              [ICON_SEARCH] = ICON_PATH("search.svg"),
                              [ICON_DELETE_FOREVER] = ICON_PATH("delete_forever.svg"),
                              [ICON_CHECK] = ICON_PATH("check_circle.svg"),
                              [ICON_CANCEL] = ICON_PATH("cancel.svg"),
                              [ICON_SWIPE] = ICON_PATH("swipe.svg"),
                              [ICON_VISIBILY_ON] = ICON_PATH("visibility_on.svg"),
                              [ICON_VISIBILY_OFF] = ICON_PATH("visibility_off.svg"),
                              [ICON_PLAY] = ICON_PATH("play.svg"),
                              [ICON_PAUSE] = ICON_PATH("pause.svg"),
                              [ICON_SKIP] = ICON_PATH("skip.svg")};
Texture g_icons[ICON_COUNT] = {0};
static_assert(sizeof(g_icon_paths) / sizeof(*g_icon_paths) == ICON_COUNT && "Icon paths need to be updated");

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
