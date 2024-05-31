#pragma once

#define DA_CLEAR(DA) \
    do (DA->len = 0;) while (0)
#define DA_PUSH(DA, ELEM)                                    \
    do {                                                     \
        size_t req_size = (DA->len + 1) * sizeof(*DA->data); \
        while (req_size > DA->cap) {                         \
            DA->cap *= 2;                                    \
            DA->data = realloc(DA->data, DA->cap);           \
            assert(DA->data);                                \
        }                                                    \
        DA->data[DA->len++] = ELEM;                          \
    } while (0)
