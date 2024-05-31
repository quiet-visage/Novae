#pragma once
#include <stddef.h>

typedef struct {
    char* data;
    size_t len;
    size_t cap;
} Str;

Str str_create(void);
void str_cat(Str* m, char* str, size_t len);
void str_clear(Str* m);
void str_null_terminate(Str* m);
void str_destroy(Str* m);
