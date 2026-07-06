#include "zsp_indexed_pool.h"
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

static uint32_t _mask_words(uint32_t size) {
    return (size + 31u) / 32u;
}

void zsp_indexed_pool_init(zsp_indexed_pool_t *p, uint32_t size) {
    p->size      = size;
    p->seed      = 1u;
    uint32_t words = _mask_words(size);
    p->used_mask = (uint32_t *)calloc(words, sizeof(uint32_t));
}

static bool _is_used(zsp_indexed_pool_t *p, int idx) {
    return (p->used_mask[idx / 32] >> (idx % 32)) & 1u;
}

static void _set_used(zsp_indexed_pool_t *p, int idx) {
    p->used_mask[idx / 32] |= (1u << (idx % 32));
}

static void _clear_used(zsp_indexed_pool_t *p, int idx) {
    p->used_mask[idx / 32] &= ~(1u << (idx % 32));
}

int zsp_indexed_pool_acquire(zsp_indexed_pool_t *p, int idx) {
    assert(idx >= 0 && (uint32_t)idx < p->size && "zsp_indexed_pool_acquire: idx out of range");
    assert(!_is_used(p, idx) && "zsp_indexed_pool_acquire: slot already taken");
    _set_used(p, idx);
    return idx;
}

int zsp_indexed_pool_acquire_random(zsp_indexed_pool_t *p) {
    /* Count free slots */
    uint32_t free_count = 0;
    for (uint32_t i = 0; i < p->size; i++) {
        if (!_is_used(p, i)) free_count++;
    }
    assert(free_count > 0 && "zsp_indexed_pool_acquire_random: pool exhausted");

    /* xorshift32 for random pick */
    p->seed ^= p->seed << 13;
    p->seed ^= p->seed >> 17;
    p->seed ^= p->seed << 5;
    uint32_t pick = p->seed % free_count;

    uint32_t n = 0;
    for (uint32_t i = 0; i < p->size; i++) {
        if (!_is_used(p, i)) {
            if (n == pick) {
                _set_used(p, i);
                return (int)i;
            }
            n++;
        }
    }
    assert(0 && "zsp_indexed_pool_acquire_random: unreachable");
    return -1;
}

void zsp_indexed_pool_release(zsp_indexed_pool_t *p, int idx) {
    assert(idx >= 0 && (uint32_t)idx < p->size && "zsp_indexed_pool_release: idx out of range");
    assert(_is_used(p, idx) && "zsp_indexed_pool_release: slot not acquired");
    _clear_used(p, idx);
}
