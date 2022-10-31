#pragma once
#include <stdbool.h>
#include <stddef.h>

typedef struct BitVector {
    size_t len;
    //size_t bytes_num; // == len / 8 + (len % 8 == 0 ? 0 : 1)
    char* bytes;
} BitVector;

BitVector* create_bit_vector(size_t len_in_bits);

BitVector* clone_bit_vector(const BitVector* origin);

void invert_bit(BitVector* bv, size_t bit_index);

bool get_bit_state(const BitVector* bv, size_t bit_index);

bool have_intersection(const BitVector* bv1, const BitVector* bv2);

size_t get_index_both_zero(const BitVector* bv1, const BitVector* bv2);

