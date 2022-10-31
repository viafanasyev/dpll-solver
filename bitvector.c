#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "bitvector.h"
#include "debug.h"

static inline size_t len_to_bytes_num(size_t len) {
    return len / 8 + (len % 8 == 0 ? 0 : 1);
}

BitVector* create_bit_vector(size_t len_in_bits) {
    assert(len_in_bits > 0);

    BitVector* bv = (BitVector*) calloc(1, sizeof(BitVector));
    if (bv == NULL) {
        fprintf(stderr, "Bit Vector Error: Insufficient memory\n");
        return NULL;
    }

    bv->len = len_in_bits;
    size_t bytes_num = len_to_bytes_num(len_in_bits);
    bv->bytes = (char*) calloc(bytes_num, sizeof(char));
    if (bv->bytes == NULL) {
        fprintf(stderr, "Bit Vector Error: Insufficient memory\n");
        free(bv);
        return NULL;
    }
    return bv;
}

BitVector* clone_bit_vector(const BitVector* origin) {
    assert(origin != NULL);

    size_t len_in_bits = origin->len;
    BitVector* clone = create_bit_vector(len_in_bits);
    if (clone == NULL) {
        fprintf(stderr, "Bit Vector Error: Insufficient memory\n");
        return NULL;
    }

    clone->len = len_in_bits;
    size_t bytes_num = len_to_bytes_num(len_in_bits);
    memcpy(clone->bytes, origin->bytes, bytes_num * sizeof(char));
    return clone;
}

void invert_bit(BitVector* bv, size_t bit_index) {
    assert(bv != NULL);
    assertf(bit_index < bv->len, "Expected size in [0; %lu), but got %lu", bv->len, bit_index);

    size_t block_index = bit_index / 8;
    size_t in_block_index = bit_index % 8;
    bv->bytes[block_index] ^= 1 << in_block_index;
}

bool get_bit_state(const BitVector* bv, size_t bit_index) {
    assert(bv != NULL);
    assertf(bit_index < bv->len, "Expected size in [0; %lu), but got %lu", bv->len, bit_index);

    size_t block_index = bit_index / 8;
    size_t in_block_index = bit_index % 8;
    return (bv->bytes[block_index] & (1 << in_block_index)) != 0;
}

bool have_intersection(const BitVector* bv1, const BitVector* bv2) {
    assert(bv1 != NULL);
    assert(bv2 != NULL);
    assert(bv1 != bv2);
    assert(bv1->len == bv2->len);

    size_t len = bv1->len;
    size_t bytes_num = len_to_bytes_num(len);
    char* bv1_bytes = bv1->bytes;
    char* bv2_bytes = bv2->bytes;
    for (size_t i = 0; i < bytes_num; ++i) {
        if ((bv1_bytes[i] & bv2_bytes[i]) != 0) {
            return true;
        }
    }
    return false;
}

static inline size_t get_index_of_one(char byte) {
    for (size_t pos = 0; pos < 8; ++pos) {
        if ((byte & 1) != 0) {
            return pos;
        }
        byte >>= 1;
    }
    return 8;
}

size_t get_index_both_zero(const BitVector* bv1, const BitVector* bv2) {
    assert(bv1 != NULL);
    assert(bv2 != NULL);
    assert(bv1 != bv2);
    assert(bv1->len == bv2->len);

    size_t len = bv1->len;
    size_t bytes_len = len_to_bytes_num(len);
    char* bv1_bytes = bv1->bytes;
    char* bv2_bytes = bv2->bytes;
    for (size_t i = 0; i < bytes_len; ++i) {
        char both_zero = (~bv1_bytes[i]) & (~bv2_bytes[i]);
        if (both_zero != 0) {
            return i * 8 + get_index_of_one(both_zero);
        }
    }
    return len;
}

