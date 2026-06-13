#ifndef VMM_HEAP_H
#define VMM_HEAP_H

#include <stddef.h>

void vmm_heap_init(void *start, unsigned long size);
void *vmm_malloc(unsigned long size);
void vmm_free(void *ptr);

#endif
