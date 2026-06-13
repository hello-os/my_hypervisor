#include "vmm_heap.h"
#include <stddef.h>

struct mem_block {
    unsigned long size;
    int free;
    struct mem_block *next;
};

static struct mem_block *heap_start = 0;

void vmm_heap_init(void *start, unsigned long size)
{
    heap_start = (struct mem_block *)start;
    heap_start->size = size;
    heap_start->free = 1;
    heap_start->next = 0;
}

void *vmm_malloc(unsigned long size)
{
    /* Align size to 8 bytes */
    size = (size + 7) & ~7;
    unsigned long total_size = size + sizeof(struct mem_block);

    struct mem_block *curr = heap_start;
    while (curr) {
        if (curr->free && curr->size >= total_size) {
            /* Split block if enough space */
            if (curr->size > total_size + sizeof(struct mem_block) + 8) {
                struct mem_block *next_block = (struct mem_block *)((char *)curr + total_size);
                next_block->size = curr->size - total_size;
                next_block->free = 1;
                next_block->next = curr->next;
                
                curr->size = total_size;
                curr->next = next_block;
            }
            curr->free = 0;
            return (void *)((char *)curr + sizeof(struct mem_block));
        }
        curr = curr->next;
    }
    return 0;
}

void vmm_free(void *ptr)
{
    if (!ptr) return;
    struct mem_block *block = (struct mem_block *)((char *)ptr - sizeof(struct mem_block));
    block->free = 1;

    /* Coalesce */
    struct mem_block *curr = heap_start;
    while (curr && curr->next) {
        if (curr->free && curr->next->free) {
            curr->size += curr->next->size;
            curr->next = curr->next->next;
        } else {
            curr = curr->next;
        }
    }
}
