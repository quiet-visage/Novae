#include "config.h"

#include <fieldfusion.h>
#include <raylib.h>

#include "colors.h"

Config g_cfg = {0};

void config_init(void) {
    g_cfg.clock_focus_mins = 20;
    g_cfg.clock_focus_secs = 0;
    g_cfg.clock_rest_mins = 4;
    g_cfg.clock_rest_secs = 0;

    g_cfg.window_width = 1280;
    g_cfg.window_height = 720;
    g_cfg.window_name = "novae";
    g_cfg.theme = THEME_CATPUCCIN_MOCHA;

    g_cfg.inner_gap = 16.0f;
    g_cfg.outer_gap = 32.0f;

    ff_get_ortho_projection(0, g_cfg.window_width,
                            g_cfg.window_height, 0, -1.0f, 1.0f,
                            g_cfg.global_projection);

    g_cfg.btn_typo.color = g_color[g_cfg.theme][COLOR_TEXT];
    g_cfg.btn_typo.font = 0;
    g_cfg.btn_typo.size = 14;

    g_cfg.c_clock_typo.color = g_color[g_cfg.theme][COLOR_TEXT];
    g_cfg.c_clock_typo.font = 0;
    g_cfg.c_clock_typo.size = 84;

    g_cfg.btn_pad_horz = 8.0f;
    g_cfg.btn_pad_vert = 6.0f;
    g_cfg.btn_expansion_width = 20;
    g_cfg.btn_expansion_height = 20;
    g_cfg.btn_roundness = 20;
}
