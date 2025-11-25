//
// Created by pb00170 on 9/16/25.
//

#include "include/conf.h"

struct wp_pool *main_pool;

void init_pool() {
    if (main_pool) return; // already initialized
    main_pool = (struct wp_pool *) calloc(1, sizeof(struct wp_pool));
    if (!main_pool) {
        fprintf(stderr, "Failed to allocate main pool\n");
        exit(1);
    }
    main_pool->size = POOL_SIZE;
    main_pool->used = 0; // bytes consumed inside data[] (headers + payloads)
    main_pool->first = NULL;
    main_pool->last = NULL;
}

static void destroy_pool_iter(wp_pool *pool) {
    if (!pool) return;
    if (pool->next) destroy_pool_iter(pool->next);
    free(pool);
}

void destroy_pool() {
    destroy_pool_iter(main_pool);
    main_pool = NULL;
}

static size_t align8(size_t v) { return (v + 7u) & ~((size_t) 7u); }

void *wpmalloc(size_t size) {
    if (!main_pool) init_pool();
    if (size == 0) size = 1;
    size = align8(size);

    // First-fit search for a free chunk large enough
    for (wp_chunk *c = main_pool->first; c; c = c->next) {
        if (c->is_free && (size_t) c->size >= size) {
            c->is_free = 0; // reuse
            // split chunk if much larger than requested

            return c->data; // payload pointer
        }
    }

    // Need to create a new chunk inside pool->data[]
    size_t header_size = align8(sizeof(wp_chunk));
    if ((size_t) main_pool->used + header_size + size > (size_t) main_pool->size) {
        // out of memory (no secondary pools implemented yet)
        return NULL;
    }

    char *base = main_pool->data + main_pool->used; // start of new header
    wp_chunk *chunk = (wp_chunk *) base;
    char *payload = base + header_size;

    chunk->size = (int) size;
    chunk->data = payload;
    chunk->next = NULL;
    chunk->prev = main_pool->last;
    chunk->ref_count = 1;
    chunk->is_free = 0;

    if (main_pool->last) main_pool->last->next = chunk;
    else main_pool->first = chunk;
    main_pool->last = chunk;

    main_pool->used += (int) (header_size + size);
    printf("allocated chunk %p\n", payload);
    return payload;
}

void wpfreepool() {
    if (!main_pool) return;
    if (!main_pool->first) return; //skip session

    for (wp_chunk *c = main_pool->first->next; c; c = c->next) {
        if (!c->is_free) {
            printf("freeing chunk %p\n", c);
            c->is_free = 1;
        }
    }
}

void wpfree(void *ptr) {
    if (!ptr || !main_pool) return;
    for (wp_chunk *c = main_pool->first; c; c = c->next) {
        if (c->data == (char *) ptr) {
            printf("freeing chunk %p\n", c);
            if (!c->is_free) c->is_free = 1; // mark free
            // //if there is a free chunk after, consolidate it
            // if (c->next && c->next->is_free) {
            //     c->size += c->next->size;
            //     c->next = c->next->next;
            // }
            // //if there is a free chunk before, consolidate it
            // if (c->prev && c->prev->is_free) {
            //     c->prev->size += c->size;
            //     c->prev->next = c->next;
            //     c = c->prev;
            // }
            return;
        }
    }
}
