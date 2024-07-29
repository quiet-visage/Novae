#include "scr_space.h"

#include "config.h"
#include "raylib.h"

inline float scr_space_x(void) { return MARGIN; }

inline float scr_space_y(void) { return MARGIN; }

inline float scr_space_width(void) { return GetScreenWidth() - MARGIN; }

inline float scr_space_height(void) { return GetScreenHeight() - MARGIN; }

inline float scr_space_half_width(void) { return (scr_space_x() + scr_space_width()) * .5; }

inline float scr_space_half_height(void) { return (scr_space_y() + scr_space_height()) * .5; }
