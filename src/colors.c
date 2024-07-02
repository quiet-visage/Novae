#include "colors.h"

#include <stddef.h>
#define RCOLOR(HEX) ((Color){((HEX) >> 24) & 0xff, ((HEX) >> 16) & 0xff, ((HEX) >> 8) & 0xff, (HEX) & 0xff})

#define X(THEME)

Color g_rcolor[THEME_COUNT][COLOR_COUNT] = {0};
int g_color[THEME_COUNT][COLOR_COUNT] = {
    [THEME_CATPUCCIN_LATTE] =
        {
            [COLOR_ROSEWATER] = 0x00000000, [COLOR_FLAMINGO] = 0x00000000, [COLOR_PINK] = 0x00000000,
            [COLOR_MAUVE] = 0x00000000,     [COLOR_RED] = 0x00000000,      [COLOR_MAROON] = 0x00000000,
            [COLOR_PEACH] = 0x00000000,     [COLOR_YELLOW] = 0x00000000,   [COLOR_GREEN] = 0x00000000,
            [COLOR_TEAL] = 0x00000000,      [COLOR_SKY] = 0x00000000,      [COLOR_SAPPHIRE] = 0x00000000,
            [COLOR_BLUE] = 0x00000000,      [COLOR_LAVENDER] = 0x00000000, [COLOR_TEXT] = 0x00000000,
            [COLOR_SUBTEXT1] = 0x00000000,  [COLOR_SUBTEXT0] = 0x00000000, [COLOR_OVERLAY2] = 0x00000000,
            [COLOR_OVERLAY1] = 0x00000000,  [COLOR_OVERLAY0] = 0x00000000, [COLOR_SURFACE2] = 0x00000000,
            [COLOR_SURFACE1] = 0x00000000,  [COLOR_SURFACE0] = 0x00000000, [COLOR_BASE] = 0x00000000,
            [COLOR_MANTLE] = 0x00000000,    [COLOR_CRUST] = 0x00000000,
        },
    [THEME_CATPUCCIN_FRAPPE] =
        {
            [COLOR_ROSEWATER] = 0x00000000, [COLOR_FLAMINGO] = 0x00000000, [COLOR_PINK] = 0x00000000,
            [COLOR_MAUVE] = 0x00000000,     [COLOR_RED] = 0x00000000,      [COLOR_MAROON] = 0x00000000,
            [COLOR_PEACH] = 0x00000000,     [COLOR_YELLOW] = 0x00000000,   [COLOR_GREEN] = 0x00000000,
            [COLOR_TEAL] = 0x00000000,      [COLOR_SKY] = 0x00000000,      [COLOR_SAPPHIRE] = 0x00000000,
            [COLOR_BLUE] = 0x00000000,      [COLOR_LAVENDER] = 0x00000000, [COLOR_TEXT] = 0x00000000,
            [COLOR_SUBTEXT1] = 0x00000000,  [COLOR_SUBTEXT0] = 0x00000000, [COLOR_OVERLAY2] = 0x00000000,
            [COLOR_OVERLAY1] = 0x00000000,  [COLOR_OVERLAY0] = 0x00000000, [COLOR_SURFACE2] = 0x00000000,
            [COLOR_SURFACE1] = 0x00000000,  [COLOR_SURFACE0] = 0x00000000, [COLOR_BASE] = 0x00000000,
            [COLOR_MANTLE] = 0x00000000,    [COLOR_CRUST] = 0x00000000,
        },
    [THEME_CATPUCCIN_MACCHIATO] =
        {
            [COLOR_ROSEWATER] = 0x00000000, [COLOR_FLAMINGO] = 0x00000000, [COLOR_PINK] = 0x00000000,
            [COLOR_MAUVE] = 0x00000000,     [COLOR_RED] = 0x00000000,      [COLOR_MAROON] = 0x00000000,
            [COLOR_PEACH] = 0x00000000,     [COLOR_YELLOW] = 0x00000000,   [COLOR_GREEN] = 0x00000000,
            [COLOR_TEAL] = 0x00000000,      [COLOR_SKY] = 0x00000000,      [COLOR_SAPPHIRE] = 0x00000000,
            [COLOR_BLUE] = 0x00000000,      [COLOR_LAVENDER] = 0x00000000, [COLOR_TEXT] = 0x00000000,
            [COLOR_SUBTEXT1] = 0x00000000,  [COLOR_SUBTEXT0] = 0x00000000, [COLOR_OVERLAY2] = 0x00000000,
            [COLOR_OVERLAY1] = 0x00000000,  [COLOR_OVERLAY0] = 0x00000000, [COLOR_SURFACE2] = 0x00000000,
            [COLOR_SURFACE1] = 0x00000000,  [COLOR_SURFACE0] = 0x00000000, [COLOR_BASE] = 0x00000000,
            [COLOR_MANTLE] = 0x00000000,    [COLOR_CRUST] = 0x00000000,
        },
    [THEME_CATPUCCIN_MOCHA] =
        {
            [COLOR_ROSEWATER] = 0xf5e0dcff, [COLOR_FLAMINGO] = 0xf2cdcdff, [COLOR_PINK] = 0xf5c2e7ff,
            [COLOR_MAUVE] = 0xcba6f7ff,     [COLOR_RED] = 0xf38ba8ff,      [COLOR_MAROON] = 0xeba0acff,
            [COLOR_PEACH] = 0xfab387ff,     [COLOR_YELLOW] = 0xf9e2afff,   [COLOR_GREEN] = 0xa6e3a1ff,
            [COLOR_TEAL] = 0x94e2d5ff,      [COLOR_SKY] = 0x89dcebff,      [COLOR_SAPPHIRE] = 0x74c7ecff,
            [COLOR_BLUE] = 0x89b4faff,      [COLOR_LAVENDER] = 0xb4befeff, [COLOR_TEXT] = 0xcdd6f4ff,
            [COLOR_SUBTEXT1] = 0xbac2deff,  [COLOR_SUBTEXT0] = 0xa6adc8ff, [COLOR_OVERLAY2] = 0x9399b2ff,
            [COLOR_OVERLAY1] = 0x7f849cff,  [COLOR_OVERLAY0] = 0x6c7086ff, [COLOR_SURFACE2] = 0x585b70ff,
            [COLOR_SURFACE1] = 0x45475aff,  [COLOR_SURFACE0] = 0x313244ff, [COLOR_BASE] = 0x1e1e2eff,
            [COLOR_MANTLE] = 0x181825ff,    [COLOR_CRUST] = 0x11111bff,
        },
};

void color_init(void) {
    for (size_t i = 0; i < THEME_COUNT; i += 1) {
        for (size_t ii = 0; ii < COLOR_COUNT; ii += 1) {
            g_rcolor[i][ii] = RCOLOR(g_color[i][ii]);
        }
    }
}
