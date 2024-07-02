#include "alpha_inherit.h"

#include <raymath.h>

static int g_alpha = 0xff;

void alpha_inherit_begin(float alpha_perc) { g_alpha = alpha_perc; }

int alpha_inherit_get_alpha(void) { return g_alpha; }

void alpha_inherit_end(void) { g_alpha = 0xff; }
