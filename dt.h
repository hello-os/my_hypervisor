#ifndef __VMLINUX_DT_H
#define __VMLINUX_DT_H

#include <stdint.h>

typedef __SIZE_TYPE__ size_t;

typedef struct {
    uint64_t mem_start;
    uint64_t mem_size;
    uint64_t chosen_addr;
} dt_info_t;

int dt_init(void *dtb, size_t dtb_size);
void dt_dump(void);

#endif /* __VMLINUX_DT_H */