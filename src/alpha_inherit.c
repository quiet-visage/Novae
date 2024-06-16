#include "alpha_inherit.h"

#include <raymath.h>

static float g_alpha = 0.f;

void alpha_inherit_begin(float alpha_perc) { g_alpha += alpha_perc; }

float alpha_inherit_get_alpha(void) { return g_alpha; }

void alpha_inherit_end(void) { g_alpha = 0.f; }
