#include "text_view.h"

#include <raylib.h>

#include "config.h"
#include "fieldfusion.h"
#include "motion.h"

void horizontal_scroll(const C32* buf, size_t len, size_t curs, float* target_scroll, FF_Style style, float max_w) {
    if (!len) return;
    {  // right horizontal scroll
        size_t column = MIN(curs + 5, len);
        float w = ff_measure_utf32(buf, column, style).width;
        bool out_of_bounds = w > max_w + *target_scroll;
        if (out_of_bounds) *target_scroll = w - max_w;
    }
    {  // left horizontal scroll
        size_t column = curs <= 5 ? curs : curs - 5;
        float w = ff_measure_utf32(buf, column, style).width;
        bool cursor_is_out_of_view = w < *target_scroll;
        if (cursor_is_out_of_view) *target_scroll = w;
    }
}

void get_text_projection(float out[4][4], Motion* mo) {
    if (mo) {
        float horsz_offset = mo->position[0];
        float left = horsz_offset;
        float right = GetScreenWidth() + horsz_offset;
        float bottom = GetScreenHeight();
        float top = 0;
        ff_get_ortho_projection(left, right, bottom, top, -1.f, 1.f, out);
        return;
    } else
        out = g_cfg.global_projection;
}

inline void update_scroll_animation(Motion* motion, float target_motion_x, float target_motion_y) {
    float target[2] = {target_motion_x, target_motion_y};
    motion_update(motion, target, GetFrameTime());
}

float get_cursor_x(const C32* buf, size_t len, FF_Style style, size_t cursor_pos, float x) {
    if (!cursor_pos) return x;

    const C32* buf_cursor_ptr = &buf[cursor_pos];
    float cursor_offset = ff_measure_utf32(buf_cursor_ptr, cursor_pos, style).width;
    return x + cursor_offset;
}
