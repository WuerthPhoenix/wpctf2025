//
// Created by pb00170 on 9/16/25.
//

#ifndef WPFTPD_POOL_H
#define WPFTPD_POOL_H

#include "os.h"
#include <limits.h>

#define POOL_SIZE (1024 * 1024)

typedef struct wp_chunk wp_chunk;

struct wp_chunk {
    uint64_t size; // size of usable data
    char *data; // pointer into pool memory
    struct wp_chunk *next; // next chunk in list (allocated order)
    struct wp_chunk *prev; // previous chunk
    uint64_t ref_count; // not used yet, reserved for future
    uint64_t is_free; // simple flag to allow trivial future reuse
};

typedef struct wp_pool {
    struct wp_pool *next; // not used yet (for possible chained pools)
    wp_chunk *first;
    wp_chunk *last;
    uint64_t size; // total size of data region
    uint64_t used; // bytes consumed (simple bump pointer)
    char data[POOL_SIZE];
} wp_pool;

extern struct wp_pool *main_pool; // global per-process pool root

void init_pool();

void destroy_pool();

void *wpmalloc(size_t size); // allocate from pool
void wpfree(void *ptr); // mark chunk as free for reuse (no coalescing)
void wpfreepool();

#endif //WPFTPD_POOL_H
