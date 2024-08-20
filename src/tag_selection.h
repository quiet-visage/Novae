#pragma once

#include <fieldfusion.h>
#include <raylib.h>

#include "button.h"
#include "c32_vec.h"
#include "cursor.h"
#include "editor.h"
#include "motion.h"
#include "tag.h"

typedef enum {
    TAG_SELECTION_STATE_COMPACT,
    TAG_SELECTION_STATE_OPEN,
} Tag_Selection_State;

typedef struct {
    FF_Glyph_Vec glyphs;
    C32_Vec input;
    Editor edit;
    Cursor cursor;
    size_t previous_input_len;
} Search_Input;

Search_Input search_input_create(void);
float search_input_width(Search_Input* m);
void search_input_view(Search_Input* m, float x, float y, bool enabled);
void search_input_destroy(Search_Input* m);

typedef struct {
    float radius;
    int selected;
} Picker_Wheel;

typedef struct {
    Tag* tag;
    Motion mo;
    float target_alpha;
    Tag_Selection_State state;
    Search_Input search;
    Picker_Wheel hue_wheel;
    Picker_Wheel value_wheel;
    Picker_Wheel saturation_wheel;
    Btn add_tag_btn;
    Btn search_btn;
    size_t selected;
    size_t hint_key;
} Tag_Selection;

Tag_Selection tag_selection_create(void);
void tag_selection_destroy(Tag_Selection* m);
Tag* tag_selection_view(Tag_Selection* m, float x, float y, bool enable_hint, bool enable_menu_input);
Tag* tag_selection_get_selected(Tag_Selection* m);
