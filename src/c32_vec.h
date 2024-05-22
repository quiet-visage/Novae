#pragma once

#include <fieldfusion.h>

typedef struct {
    C32* data;
    size_t len;
    size_t cap;
} C32_Vec;

C32_Vec c32_vec_create(void);
void c32_vec_destroy(C32_Vec* m);
void c32_vec_ins_str(C32_Vec* m, size_t pos, C32* str, size_t len);
void c32_vec_del_str(C32_Vec* m, size_t pos, size_t len);
