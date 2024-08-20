#pragma once

#include <raylib.h>
#include <stdio.h>

void hint_init(void);
size_t hint_generate_instance_key();
void hint_view(size_t key, const char *desc, Rectangle bounds);
void hint_end_frame(void);
