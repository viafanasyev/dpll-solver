#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "debug.h"
#include "trivector.h"

TriVector* create_tri_vector(size_t len) {
    TriVector* tv = (TriVector*) calloc(1, sizeof(TriVector));
    if (tv == NULL) {
        fprintf(stderr, "Tri Vector Error: Insufficient memory\n");
        return NULL;
    }

    tv->len = len;
    tv->states = (signed char*) calloc(len, sizeof(signed char));
    if (tv->states == NULL) {
        fprintf(stderr, "Tri Vector Error: Insufficient memory\n");
        free(tv);
        return NULL;
    }
    return tv;
}

TriVector* clone_tri_vector(const TriVector* origin) {
    assert(origin != NULL);

    size_t len = origin->len;
    TriVector* clone = create_tri_vector(len);
    if (clone == NULL) {
        fprintf(stderr, "Tri Vector Error: Insufficient memory\n");
        return NULL;
    }

    clone->len = len;
    memcpy(clone->states, origin->states, len * sizeof(signed char));
    return clone;
}

void set_state(TriVector* tv, size_t index, bool positive) {
    assert(tv != NULL);
    assertf(index < tv->len, "Expected size in [0; %lu), but got %lu", tv->len, index);

    tv->states[index] = positive ? 1 : -1;
}

signed char get_state(const TriVector* tv, size_t index) {
    assert(tv != NULL);
    assertf(index < tv->len, "Expected size in [0; %lu), but got %lu", tv->len, index);

    return tv->states[index];
}

bool is_set_positive(const TriVector* tv, size_t index) {
    assert(tv != NULL);
    assertf(index < tv->len, "Expected size in [0; %lu), but got %lu", tv->len, index);

    return tv->states[index] > 0;
}

bool is_set_negative(const TriVector* tv, size_t index) {
    assert(tv != NULL);
    assertf(index < tv->len, "Expected size in [0; %lu), but got %lu", tv->len, index);

    return tv->states[index] < 0;
}

bool is_not_set(const TriVector* tv, size_t index) {
    assert(tv != NULL);
    assertf(index < tv->len, "Expected size in [0; %lu), but got %lu", tv->len, index);

    return tv->states[index] == 0;
}

size_t get_index_of_non_set(const TriVector* tv) {
    assert(tv != NULL);

    size_t len = tv->len;
    signed char* states = tv->states;
    for (size_t i = 0; i < len; ++i) {
        if (states[i] == 0) {
            return i;
        }
    }
    return len;
}

