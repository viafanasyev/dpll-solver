#pragma once
#include <stdbool.h>
#include <stddef.h>

typedef struct TriVector {
    size_t len;
    signed char* states; // TODO: Replace chars with enums of 1-byte size
} TriVector;

TriVector* create_tri_vector(size_t len);

TriVector* clone_tri_vector(const TriVector* origin);

static inline void set_state(TriVector* tv, size_t index, bool positive) {
    assert(tv != NULL);
    assertf(index < tv->len, "Expected size in [0; %lu), but got %lu", tv->len, index);

    tv->states[index] = positive ? 1 : -1;
}

static inline signed char get_state(const TriVector* tv, size_t index) {
    assert(tv != NULL);
    assertf(index < tv->len, "Expected size in [0; %lu), but got %lu", tv->len, index);

    return tv->states[index];
}

static inline bool is_set_positive(const TriVector* tv, size_t index) {
    assert(tv != NULL);
    assertf(index < tv->len, "Expected size in [0; %lu), but got %lu", tv->len, index);

    return tv->states[index] > 0;
}

static inline bool is_set_negative(const TriVector* tv, size_t index) {
    assert(tv != NULL);
    assertf(index < tv->len, "Expected size in [0; %lu), but got %lu", tv->len, index);

    return tv->states[index] < 0;
}

static inline bool is_not_set(const TriVector* tv, size_t index) {
    assert(tv != NULL);
    assertf(index < tv->len, "Expected size in [0; %lu), but got %lu", tv->len, index);

    return tv->states[index] == 0;
}

size_t get_index_of_non_set(const TriVector* tv);

