#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "debug.h"
#include "trivector.h"

TriVector* create_trivector(size_t len) {
    TriVector* tv = (TriVector*) calloc(1, sizeof(TriVector));
    if (tv == NULL) {
        fprintf(stderr, "Tri Vector Error: Insufficient memory\n");
        return NULL;
    }

    tv->len = len;
    tv->states = (TriVectorState*) calloc(len, sizeof(TriVectorState));
    if (tv->states == NULL) {
        fprintf(stderr, "Tri Vector Error: Insufficient memory\n");
        free(tv);
        return NULL;
    }
    return tv;
}

TriVector* clone_trivector(const TriVector* origin) {
    assert(origin != NULL);

    size_t len = origin->len;
    TriVector* clone = create_trivector(len);
    if (clone == NULL) {
        fprintf(stderr, "Tri Vector Error: Insufficient memory\n");
        return NULL;
    }

    clone->len = len;
    memcpy(clone->states, origin->states, len * sizeof(TriVectorState));
    return clone;
}

void free_trivector(TriVector* tv) {
    if (tv != NULL) {
        free(tv->states);
        free(tv);
    }
}

size_t trivector_index_of_not_set(const TriVector* tv) {
    assert(tv != NULL);

    size_t len = tv->len;
    TriVectorState* states = tv->states;
    for (size_t i = 0; i < len; ++i) {
        if (states[i] == NOT_SET) {
            return i;
        }
    }
    return len;
}

