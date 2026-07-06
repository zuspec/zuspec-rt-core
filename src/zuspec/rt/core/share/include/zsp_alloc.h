#ifndef INCLUDED_ZSP_ALLOC_H
#define INCLUDED_ZSP_ALLOC_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif 

struct zsp_alloc_s;

typedef void *(*zsp_alloc_func)(struct zsp_alloc_s *, size_t);
typedef void (*zsp_free_func)(struct zsp_alloc_s *, void *);

typedef struct zsp_alloc_s {
    zsp_alloc_func  alloc;
    zsp_free_func   free;
} zsp_alloc_t;

void zsp_alloc_malloc_init(zsp_alloc_t *alloc);

zsp_alloc_t *zsp_alloc_malloc_create();

#ifdef __cplusplus
}
#endif 

#endif /* INCLUDED_ZSP_ALLOC_H */
