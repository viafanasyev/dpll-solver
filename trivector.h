#pragma once
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    NOT_SET,
    SET_TRUE,
    SET_FALSE,
} __attribute__((__packed__)) TriVectorState;

typedef struct TriVector {
    size_t len;
    TriVectorState* states;
} TriVector;

TriVector* create_trivector(size_t len);

TriVector* clone_trivector(const TriVector* origin);

void free_trivector(TriVector* tv);

static inline void trivector_set(TriVector* tv, size_t index, bool is_true) {
    assert(tv != NULL);
    assertf(index < tv->len, "Expected size in [0; %lu), but got %lu", tv->len, index);

    tv->states[index] = is_true ? SET_TRUE : SET_FALSE;
}

static inline bool trivector_is_set_true(const TriVector* tv, size_t index) {
    assert(tv != NULL);
    assertf(index < tv->len, "Expected size in [0; %lu), but got %lu", tv->len, index);

    return tv->states[index] == SET_TRUE;
}

static inline bool trivector_is_set_false(const TriVector* tv, size_t index) {
    assert(tv != NULL);
    assertf(index < tv->len, "Expected size in [0; %lu), but got %lu", tv->len, index);

    return tv->states[index] == SET_FALSE;
}

static inline bool trivector_is_not_set(const TriVector* tv, size_t index) {
    assert(tv != NULL);
    assertf(index < tv->len, "Expected size in [0; %lu), but got %lu", tv->len, index);

    return tv->states[index] == NOT_SET;
}

size_t trivector_index_of_not_set(const TriVector* tv);

