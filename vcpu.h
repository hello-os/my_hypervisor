#ifndef __VMLINUX_VCPU_H
#define __VMLINUX_VCPU_H

#include <stdint.h>

typedef struct {
    uint64_t regs[31];
    uint64_t sp;
    uint64_t pc;
    uint64_t pstate;
} vcpu_context_t;

void vcpu_save(vcpu_context_t *ctx);
void vcpu_restore(vcpu_context_t *ctx);
int vcpu_init(void);

#endif /* __VMLINUX_VCPU_H */