.section .text
.align 2

.globl vcpu_save
vcpu_save:
    stp x0, x1, [x0, #0]
    stp x2, x3, [x0, #16]
    stp x4, x5, [x0, #32]
    stp x6, x7, [x0, #48]
    stp x8, x9, [x0, #64]
    stp x10, x11, [x0, #80]
    stp x12, x13, [x0, #96]
    stp x14, x15, [x0,#112]
    stp x16, x17, [x0,#128]
    stp x18, x19, [x0,#144]
    stp x20, x21, [x0,#160]
    stp x22, x23, [x0,#176]
    stp x24, x25, [x0,#192]
    stp x26, x27, [x0,#208]
    stp x28, x29, [x0,#224]
    str x30, [x0,#240]
    mrs x1, sp_el2
    str x1, [x0,#248]
    mrs x1, elr_el2
    str x1, [x0,#256]
    mrs x1, spsr_el2
    str x1, [x0,#264]
    ret

.globl vcpu_restore
vcpu_restore:
    ldr x30, [x0,#240]
    ldr x2, [x0,#248]
    msr sp_el2, x2
    ldr x2, [x0,#256]
    msr elr_el2, x2
    ldr x2, [x0,#264]
    msr spsr_el2, x2
    ldp x2, x3, [x0, #16]
    ldp x4, x5, [x0, #32]
    ldp x6, x7, [x0, #48]
    ldp x8, x9, [x0, #64]
    ldp x10, x11, [x0, #80]
    ldp x12, x13, [x0, #96]
    ldp x14, x15, [x0,#112]
    ldp x16, x17, [x0,#128]
    ldp x18, x19, [x0,#144]
    ldp x20, x21, [x0,#160]
    ldp x22, x23, [x0,#176]
    ldp x24, x25, [x0,#192]
    ldp x26, x27, [x0,#208]
    ldp x28, x29, [x0,#224]
    ldp x0, x1, [x0, #0]
    ret

.globl context_switch
context_switch:
    /* Save prev context */
    stp x19, x20, [x0, #144]
    stp x21, x22, [x0, #160]
    stp x23, x24, [x0, #176]
    stp x25, x26, [x0, #192]
    stp x27, x28, [x0, #208]
    stp x29, x30, [x0, #224]
    mov x19, sp
    str x19, [x0, #248]
    /* Restore next context */
    mov x0, x1
    b vcpu_restore
