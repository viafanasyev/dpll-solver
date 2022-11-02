#pragma once
#include <stdbool.h>
#include <stddef.h>

typedef struct TriVector {
    size_t len;
    signed char* states; // TODO: Replace chars with enums of 1-byte size
} TriVector;

TriVector* create_tri_vector(size_t len);

TriVector* clone_tri_vector(const TriVector* origin);

void set_state(TriVector* tv, size_t index, bool positive);

signed char get_state(const TriVector* tv, size_t index);

bool is_set_positive(const TriVector* tv, size_t index);

bool is_set_negative(const TriVector* tv, size_t index);

bool is_not_set(const TriVector* tv, size_t index);

size_t get_index_of_non_set(const TriVector* tv);

