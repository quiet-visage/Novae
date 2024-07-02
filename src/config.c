#include "config.h"

#include <fieldfusion.h>
#include <raylib.h>

#include "colors.h"

Config g_cfg = {0};

void config_init(void) {
    g_cfg.clock_focus_mins = 00;
    g_cfg.clock_focus_secs = 1;
    g_cfg.clock_rest_mins = 0;
    g_cfg.clock_rest_secs = 2;

    g_cfg.window_width = 1280;
    g_cfg.window_height = 720;
    g_cfg.window_name = "novae";
    g_cfg.theme = THEME_CATPUCCIN_MOCHA;

    g_cfg.inner_gap = 12.0f;
    g_cfg.inner_gap2 = 2 * g_cfg.inner_gap;
    g_cfg.outer_gap = 16.0f;
    g_cfg.outer_gap2 = 2 * g_cfg.outer_gap;

    ff_get_ortho_projection(0, g_cfg.window_width, g_cfg.window_height, 0, -1.0f, 1.0f, g_cfg.global_projection);

    g_cfg.bstyle = ff_style_create();
    g_cfg.mstyle = ff_style_create();
    g_cfg.sstyle = ff_style_create();
    g_cfg.estyle = ff_style_create();
    g_cfg.bstyle.typo.size = 64;
    g_cfg.mstyle.typo.size = g_cfg.bstyle.typo.size * .35f;
    g_cfg.sstyle.typo.size = 14.f;
    g_cfg.estyle.typo.size = 11.f;
    g_cfg.bstyle.typo.color = GET_COLOR(COLOR_TEXT);
    g_cfg.mstyle.typo.color = GET_COLOR(COLOR_TEXT);
    g_cfg.sstyle.typo.color = GET_COLOR(COLOR_TEXT);
    g_cfg.estyle.typo.color = GET_COLOR(COLOR_TEXT);

    g_cfg.rounded_rec_segments = 24;

    g_cfg.bg_radius = 10.f;
    g_cfg.btn_pad_horz = 8.0f;
    g_cfg.btn_pad_vert = 6.0f;
    g_cfg.btn_expansion_width = 20;
    g_cfg.btn_expansion_height = 20;
    g_cfg.btn_roundness = 20;
}
