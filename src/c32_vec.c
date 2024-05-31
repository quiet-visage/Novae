#include "c32_vec.h"

#include <assert.h>
#include <string.h>

#define AUTO_EXPAND_DA(DA_PTR, COUNT)                                     \
    do {                                                                  \
        size_t req_cap = (DA_PTR->len + COUNT) * sizeof(DA_PTR->data[0]); \
        while (req_cap > DA_PTR->cap) {                                   \
            req_cap *= 2;                                                 \
            DA_PTR->data = realloc(DA_PTR->data, req_cap);                \
            assert(DA_PTR->data);                                         \
        }                                                                 \
    } while (0)

C32_Vec c32_vec_create(void) {
    C32_Vec ret = {.data = malloc(0x100), .cap = 0x100, .len = 0};
    assert(ret.data);
    return ret;
}

void c32_vec_destroy(C32_Vec* m) { free(m->data); }

void c32_vec_ins_str(C32_Vec* m, size_t pos, C32* str, size_t len) {
    assert(pos <= m->len);
    AUTO_EXPAND_DA(m, len);

    memmove(&m->data[pos + len], &m->data[pos], (m->len - pos) * sizeof(C32));
    memcpy(&m->data[pos], str, len * sizeof(C32));
    m->len += len;
}

void c32_vec_del_str(C32_Vec* m, size_t pos, size_t len) {
    assert((pos + len) <= m->len);
    assert(m->len >= len);
    memmove(&m->data[pos], &m->data[pos + len], (m->len - (pos + len)) * sizeof(C32));
    m->len -= len;
}
