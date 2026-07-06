#ifndef INCLUDED_ZSP_INDEXED_POOL_H
#define INCLUDED_ZSP_INDEXED_POOL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct zsp_indexed_pool_s {
    uint32_t  size;
    uint32_t *used_mask;  /* one bit per slot */
    uint32_t  seed;       /* for random selection */
} zsp_indexed_pool_t;

void zsp_indexed_pool_init(zsp_indexed_pool_t *p, uint32_t size);
int  zsp_indexed_pool_acquire(zsp_indexed_pool_t *p, int idx);
int  zsp_indexed_pool_acquire_random(zsp_indexed_pool_t *p);
void zsp_indexed_pool_release(zsp_indexed_pool_t *p, int idx);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDED_ZSP_INDEXED_POOL_H */
