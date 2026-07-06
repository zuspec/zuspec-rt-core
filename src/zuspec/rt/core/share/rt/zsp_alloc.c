#include <stdlib.h>
#include "zsp_alloc.h"

static void *zsp_alloc_malloc_alloc(zsp_alloc_t *alloc, size_t sz) {
    return malloc(sz);
}

static void zsp_alloc_malloc_free(zsp_alloc_t *alloc, void *p) {
    free(p);
}

void zsp_alloc_malloc_init(zsp_alloc_t *alloc) {
    alloc->alloc = &zsp_alloc_malloc_alloc;
    alloc->free = &zsp_alloc_malloc_free; 
}

zsp_alloc_t *zsp_alloc_malloc_create() {
    zsp_alloc_t *alloc = (zsp_alloc_t *)malloc(sizeof(zsp_alloc_t));
    zsp_alloc_malloc_init(alloc);
    return alloc;
}

