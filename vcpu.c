#include "vcpu.h"
#include "mm.h"
#include "main.h"

static vcpu_context_t vcpu_ctx;

int vcpu_init(void) {
    // Initialize VCPU context with default values
    vcpu_ctx.sp = 0x40080000;  // Initial stack pointer
    vcpu_ctx.pc = (uint64_t)main;  // Entry point after hypervisor init
    vcpu_ctx.pstate = 0x0000000000000005;  // EL1h mode
    
    // Zero out general registers
    for (int i = 0; i < 31; i++) {
        vcpu_ctx.regs[i] = 0;
    }
    
    return 0;
}


