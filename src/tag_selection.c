// TODO: DRAW SEARCH BAR
// TODO: DRAW TAGS
// TODO: CREATE TAG

#include "tag_selection.h"

#include <assert.h>
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <string.h>

#include "button.h"
#include "c32_vec.h"
#include "colors.h"
#include "config.h"
#include "cursor.h"
#include "db.h"
#include "db_cache.h"
#include "editor.h"
#include "fieldfusion.h"
#include "hsv.h"
#include "icon.h"
#include "motion.h"
#include "tag.h"
#include "text_view.h"

#define OPEN_VIEW_X_RATIO .5f
#define HUE_RAD 64.f
#define HUE_RING_W 12.f
#define SAT_RAD (HUE_RAD - HUE_RING_W - 5.f)

static FF_Style* g_style = &g_cfg.sstyle;
static HSV g_tag_hsv = {285., 1., 1.};

static void draw_hue_wheel(HSV hsv, float cx, float cy, float radius, float ring_width) {
    for (float x = -radius; x < radius; x += 1) {
        for (float y = -radius; y < radius; y += 1) {
            float r = sqrt(x * x + y * y);
            if (r > radius || r < radius - ring_width) continue;
            float phi = atan2(y, x);
            float deg = RAD2DEG * phi + 180;
            HSV hsv = {deg, r / radius, 1.f};
            Color col = hsv2rgb(hsv);
            DrawPixel(x + cx, y + cy, col);
        }
    }
}

static void draw_saturation_wheel(float hue, float value, float cx, float cy, float radius,
                                  float ring_width) {
    for (float x = -radius; x < radius; x += 1) {
        for (float y = -radius; y < radius; y += 1) {
            float r = sqrt(x * x + y * y);
            if (r > radius || r < radius - ring_width) continue;
            float phi = atan2(y, x);
            float deg = hue;
            HSV hsv = {deg, ((RAD2DEG * phi + 180) / 360), value};
            Color col = hsv2rgb(hsv);
            DrawPixel(x + cx, y + cy, col);
        }
    }
}

static void draw_value_wheel(float hue, float sat, float cx, float cy, float radius,
                             float ring_width) {
    for (float x = -radius; x < radius; x += 1) {
        for (float y = -radius; y < radius; y += 1) {
            float r = sqrt(x * x + y * y);
            if (r > radius || r < radius - ring_width) continue;
            float phi = atan2(y, x);
            float deg = hue;
            HSV hsv = {deg, sat, (RAD2DEG * phi + 180) / 360};
            Color col = hsv2rgb(hsv);
            DrawPixel(x + cx, y + cy, col);
        }
    }
}

static void draw_nob(float deg, float cx, float cy, float radius) {
    radius -= HUE_RING_W * .5f;
    Vector2 pos = {radius * cosf((deg - 180) * DEG2RAD) + cx, radius * sinf(-(deg * DEG2RAD)) + cy};
    DrawRing(pos, HUE_RING_W * .25f, HUE_RING_W * .5f, 0, 360, 64, GET_RCOLOR(COLOR_TEAL));
}

static float get_mouse_angle(Vector2 origin, float radius) {
    Vector2 m = GetMousePosition();
    // DrawLine(origin.x, origin.y, origin.x + radius, origin.y, SKYBLUE);
    // DrawLine(origin.x, origin.y, m.x, m.y, SKYBLUE);
    // DrawLineV(origin, m, RED);
    float len = sqrt(powf(origin.x - m.x, 2.f) + powf(origin.y - m.y, 2.f));
    float fx = origin.x + (m.x - origin.x) / len * (radius);
    float fy = origin.y + (m.y - origin.y) / len * (radius);
    // DrawCircle(fx, fy, 6.f, BLUE);
    float f_slope = (fx - origin.x) / (fy - origin.y);
    float angle = RAD2DEG * atan(f_slope);
    float rel_x = origin.x - fx;
    float rel_y = origin.y - fy;
    bool right_from_center = rel_x < 0;
    bool top_from_center = rel_y < 0;
    bool bot_from_center = !top_from_center;
    bool first_quadrant = right_from_center && bot_from_center;
    bool fourth_quadrant = right_from_center && top_from_center;
    bool neg_angle = angle < 0;
    angle -= 360;
    if (neg_angle) {
        if (first_quadrant) {
            angle += 270;
            angle *= -1;
        } else {  // third quadrant
            if (angle == -450)
                angle = 0;
            else {
                angle += 90;
                angle *= -1;
            }
        }
    } else {
        if (fourth_quadrant) {
            angle += 90;
            angle *= -1;
        } else {  // second
            if (angle == -360) {
                angle = m.y > origin.y ? -270 : -90;
            } else if (angle == -270) {
                angle = m.x < origin.x ? 0 : -180;
            } else {
                angle += 270;
            }
            angle *= -1;
        }
    }
    if (angle == 80.f) angle = 180;
    if (angle == 450.f) angle = 0;

    return angle;
}

Search_Input search_input_create(void) {
    Search_Input result = {0};
    result.input = c32_vec_create();
    result.glyphs = ff_glyph_vec_create();
    result.edit = editor_create();
    result.cursor = cursor_new();
    result.edit.limit = MAX_TAG_LEN;
    result.cursor.flags |= CURSOR_FLAG_FOCUSED;
    editor_set_placeholder(&result.edit, "Tag name");
    return result;
}

static void handle_wheel_click(Picker_Wheel* m, float* deg, Vector2 origin) {
    Vector2 mouse = GetMousePosition();
    float m_dist_from_cen = Vector2Distance(mouse, origin);
    float r_dist_from_cen = m->radius;
    float r_in_dist_from_cen = m->radius - HUE_RING_W;
    m->selected =
        m->selected || (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                        m_dist_from_cen < r_dist_from_cen && m_dist_from_cen > r_in_dist_from_cen);
    if (m->selected && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        float angle = get_mouse_angle(origin, m->radius);
        *deg = angle;
    } else {
        m->selected = false;
    }
}
float search_input_width(Search_Input* m) {
    return ff_measure_utf32(m->input.data, m->input.len, *g_style).width;
}

bool search_input_changed(Search_Input* m) {
    bool result = m->input.len != m->previous_input_len;
    m->previous_input_len = m->input.len;
    return result;
}

void search_input_view(Search_Input* m, float x, float y) {
    editor_handle_input(&m->edit, &m->input);

    if (m->glyphs.len != m->input.len) {
        ff_print_utf32_vec(&m->glyphs, m->input.data, m->input.len, x, y, *g_style);
    }
    ff_set_glyphs_pos(m->glyphs.data, m->glyphs.len, x, y);

    float cursor_x = get_cursor_x(m->input.data, m->input.len, *g_style, m->edit.cursor, x);
    float cursor_y = y;
    cursor_draw(&m->cursor, cursor_x, cursor_y);

    if (!m->input.len) {
        assert(m->edit.placeholder);
        size_t len = strlen(m->edit.placeholder);
        FF_Style style = *g_style;
        style.typo.color = GET_COLOR(COLOR_SURFACE1);
        ff_draw_str8(m->edit.placeholder, len, x, y, (float*)g_cfg.global_projection, style);
        return;
    }

    ff_draw_str32(m->input.data, m->input.len, x, y, (float*)g_cfg.global_projection, *g_style);
}

void search_input_destroy(Search_Input* m) {
    ff_glyph_vec_destroy(&m->glyphs);
    c32_vec_destroy(&m->input);
    editor_destroy(&m->edit);
}

void begin_open_view_fade_in(Tag_Selection* m) {
    m->mo.position[0] = 0;
    m->mo.position[1] = 0;
    m->mo.previous_input[0] = 0;
    m->mo.previous_input[1] = 0;
    m->target_alpha = 0xff;
}

Tag_Selection tag_selection_create(void) {
    g_tag_hsv = rgb2hsv(GET_RCOLOR(COLOR_TEAL));
    Tag_Selection result = {0};
    // result.search=TAG_SELECTION_STATE_OPEN;
    result.tag = db_cache_get_default_tag();
    result.mo = motion_new();
    result.target_alpha = 0x0;
    result.search = search_input_create();
    begin_open_view_fade_in(&result);
    result.state = TAG_SELECTION_STATE_OPEN;
    result.hue_wheel.radius = HUE_RAD;
    result.saturation_wheel.radius = HUE_RAD - HUE_RING_W - 5.f;
    result.value_wheel.radius = HUE_RAD - HUE_RING_W * 2 - 5.f * 2;
    result.add_tag_btn = btn_create();
    result.search_btn = btn_create();
    result.selected = (size_t)-1;
    return result;
}

void tag_selection_destroy(Tag_Selection* m) {
    search_input_destroy(&m->search);
    btn_destroy(&m->add_tag_btn);
    btn_destroy(&m->search_btn);
}

static void compact_view(Tag_Selection* m, float x, float y) {
    Rectangle tag_bg = tag_draw(m->tag, x, y);
    bool clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                   CheckCollisionPointRec(GetMousePosition(), tag_bg);
    if (!clicked) return;
    begin_open_view_fade_in(m);
    m->state = TAG_SELECTION_STATE_OPEN;
}

static int get_similiarity(char* str1, size_t str1_len, char* str2, size_t str2_len) {
    int score = 0;
    int prev_match = 0;
    for (size_t i = 0; i < MIN(str1_len, str2_len); i += 1) {
        char c1 = str1[i];
        char c2 = str2[i];
        if (c1 == c2) {
            score += 1 + prev_match;
            prev_match += 1;
        } else {
            prev_match = 0;
        }
    }
    return score;
}

static size_t get_closest_input_match(Tag_Selection* m, Tag* tags, size_t tag_len) {
    size_t result = (size_t)-1;
    size_t result_score = 0;
    char input8[m->search.input.len];
    ff_utf32_to_utf8(input8, m->search.input.data, m->search.input.len);
    for (size_t i = 0; i < tag_len; i += 1) {
        Tag* tag = &tags[i];
        char* name = tag->name;
        size_t name_len = strlen(name);
        int score = get_similiarity(name, name_len, input8, m->search.input.len);
        if (score > result_score) {
            result = i;
            result_score = score;
        }
    }

    return result;
}

static void view_color_picker(Tag_Selection* m, float x, float y) {
    draw_hue_wheel(g_tag_hsv, x, y, HUE_RAD, HUE_RING_W);
    draw_nob(g_tag_hsv.hue, x, y, HUE_RAD);
    Vector2 orig = {x, y};
    handle_wheel_click(&m->hue_wheel, &g_tag_hsv.hue, orig);

    draw_saturation_wheel(g_tag_hsv.hue, g_tag_hsv.value, x, y, SAT_RAD, HUE_RING_W);
    draw_nob(g_tag_hsv.saturation * 360.f, x, y, SAT_RAD);
    float saturation = g_tag_hsv.saturation * 360.f;
    handle_wheel_click(&m->saturation_wheel, &saturation, orig);
    g_tag_hsv.saturation = saturation / 360.f;

    draw_value_wheel(g_tag_hsv.hue, g_tag_hsv.saturation, x, y, m->value_wheel.radius, HUE_RING_W);
    draw_nob(g_tag_hsv.value * 360.f, x, y, m->value_wheel.radius);
    float value = g_tag_hsv.value * 360.f;
    handle_wheel_click(&m->value_wheel, &value, orig);
    g_tag_hsv.value = value / 360.f;
    DrawCircleV(orig, .15 * HUE_RAD, hsv2rgb(g_tag_hsv));
}

static Rectangle get_open_background(Tag_Selection* m) {
    Rectangle result = {0};
    float screen_w = GetScreenWidth();
    float screen_h = GetScreenHeight();
    result.width = screen_w * OPEN_VIEW_X_RATIO;
    result.height = btn_height(&m->search_btn) + HUE_RAD * 2 + g_cfg.outer_gap2 + g_cfg.inner_gap;
    result.x = CENTER(0, screen_w, result.width);
    result.y = CENTER(0, screen_h, result.height);
    return result;
}

static Rectangle get_search_bg(Tag_Selection* m, float bg_y, float create_tag_btn_x,
                               float search_str_w) {
    Rectangle result = {0};
    result.x = create_tag_btn_x + btn_width(&m->add_tag_btn) + g_cfg.inner_gap * .5;
    result.y = bg_y + g_cfg.outer_gap;
    result.width = search_str_w + g_cfg.inner_gap2;
    result.height = g_style->typo.size + g_cfg.inner_gap;
    return result;
}

static float get_search_width(Tag_Selection* m) {
    char max_str[] = "tag name";
    float search_w = search_input_width(&m->search);
    float max_w = MAX(ff_measure_utf8(max_str, strlen(max_str), *g_style).width, search_w);
    return max_w;
}

static void draw_tags(Tag_Selection* m, Tag* tags, size_t tags_len, float x, float y, Rectangle bg,
                      Tag** result) {
    float tags_end = bg.x + bg.width - g_cfg.outer_gap;
    float tag_x = x;
    float tag_y = y;
    Vector2 mouse_pos = GetMousePosition();
    bool is_mouse_pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    for (size_t i = 0; i < tags_len; i += 1) {
        Tag tag = tags[i];
        float tag_w = tag_width(&tag);
        bool overflow = tag_x + tag_w > tags_end;
        if (overflow) {
            tag_x = x;
            tag_y += g_cfg.inner_gap + tag_height();
        }
        Rectangle bounds = tag_draw(&tag, tag_x, tag_y);
        if (m->selected == i) {
            DrawRectangleRoundedLines(bounds, 0x100, g_cfg.rounded_rec_segments, 1.f,
                                      GET_RCOLOR(COLOR_TEAL));
        }

        if (is_mouse_pressed && CheckCollisionPointRec(mouse_pos, bounds)) {
            m->state = TAG_SELECTION_STATE_COMPACT;
            *result = &tags[i];
            m->tag = &tags[i];
        }

        tag_x += g_cfg.inner_gap + tag_w;
    }
}

static Tag* open_view(Tag_Selection* m) {
    motion_update_x(&m->mo, m->target_alpha, GetFrameTime());
    if (!m->mo.position[0]) return 0;
    Tag* result = {0};

    Rectangle bg = get_open_background(m);
    DRAW_BG(bg, g_cfg.bg_radius, COLOR_MANTLE);

    float search_w = get_search_width(m);
    float left_side_w = search_w + btn_width(&m->search_btn) * 2 + g_cfg.inner_gap * 3;

    float create_tag_btn_x = CENTER(bg.x, bg.width * .5, left_side_w);

    float btn_y = g_cfg.outer_gap + bg.y;
    bool add = btn_draw_with_icon(&m->add_tag_btn, ICON_TAG_ADD, create_tag_btn_x, btn_y);

    Rectangle search_bg = get_search_bg(m, bg.y, create_tag_btn_x, search_w);
    DRAW_BG(search_bg, 0x100, COLOR_SURFACE0);
    draw_underglow(search_bg, hsv2rgb(g_tag_hsv), GET_RCOLOR(COLOR_BASE));

    float search_x = search_bg.x + g_cfg.inner_gap;
    float search_y = CENTER(search_bg.y, search_bg.height, g_style->typo.size);

    Tag* tags = db_cache_get_tag_array();
    size_t tags_len = db_cache_get_tag_array_len();
    search_input_view(&m->search, search_x, search_y);
    if (search_input_changed(&m->search)) {
        m->selected = get_closest_input_match(m, tags, tags_len);
    }

    create_tag_btn_x = search_bg.x + search_bg.width + g_cfg.inner_gap * .5;
    btn_draw_with_icon(&m->search_btn, ICON_SEARCH, create_tag_btn_x, btn_y);

    view_color_picker(m, search_bg.x + search_bg.width * .5,
                      bg.y + g_cfg.outer_gap + search_bg.height + g_cfg.inner_gap + HUE_RAD);

    float tags_x = bg.x + bg.width * .5;
    float tags_y = bg.y + g_cfg.outer_gap;
    draw_tags(m, tags, tags_len, tags_x, tags_y, bg, &result);

    if (IsKeyPressed(KEY_ESCAPE)) {
        m->state = TAG_SELECTION_STATE_COMPACT;
        return (Tag*)-1;
    }
    if (IsKeyPressed(KEY_ENTER) && m->selected != (size_t)-1) {
        m->state = TAG_SELECTION_STATE_COMPACT;
        result = &tags[m->selected];
        m->tag = result;
    }

    if (add && m->search.input.len) {
        char name[m->search.input.len + 1];
        ff_utf32_to_utf8(name, m->search.input.data, m->search.input.len);
        name[m->search.input.len] = 0;
        db_create_tag(name, hsv2rgb_i(g_tag_hsv));
        db_cache_sync_tags();
    }

    return result;
}

Tag* tag_selection_view(Tag_Selection* m, float x, float y) {
    switch (m->state) {
        case TAG_SELECTION_STATE_COMPACT: compact_view(m, x, y); break;
        case TAG_SELECTION_STATE_OPEN: return open_view(m);
    }
    return 0;
}

Tag* tag_selection_get_selected(Tag_Selection* m) { return m->tag; }
