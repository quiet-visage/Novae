#include "str.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

Str str_create(void) {
    Str ret;
    ret.cap = 0x100;
    ret.data = malloc(ret.cap);
    assert(ret.data);
    ret.len = 0;
    return ret;
}

void str_cat(Str* m, char* str, size_t len) {
    size_t req_size = (m->len + len) * sizeof(*m->data);
    while (req_size > m->cap) {
        m->cap *= 2;
        m->data = realloc(m->data, m->cap);
        assert(m->data);
    }
    memcpy(&m->data[m->len], str, sizeof(*str) * len);
    m->len += len;
}

inline void str_clear(Str* m) { m->len = 0; }

void str_null_terminate(Str* m) {
    size_t req_size = (m->len + 1) * sizeof(*m->data);
    while (req_size > m->cap) {
        m->cap *= 2;
        m->data = realloc(m->data, m->cap);
        assert(m->data);
    }
    m->data[m->len] = 0;
}

void str_destroy(Str* m) { free(m->data); }
