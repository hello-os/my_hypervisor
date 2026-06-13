/* start.S for my_hypervisor */

.section .text.boot, "ax"
.globl _start

_start:
    /* Disable interrupts (D, A, I, F) */
    msr daifset, #0xf

    /* Determine current Exception Level */
    mrs x4, currentel
    and x4, x4, #0xc
    cmp x4, #0xc    /* EL3 is 0xc (bits [3:2] = 0b11) */
    b.eq el3_entry
    cmp x4, #0x8    /* EL2 is 0x8 (bits [3:2] = 0b10) */
    b.eq el2_entry

hang:
    wfe
    b hang

el3_entry:
    /* Configure SCR_EL3:
     * - NS (bit 0) = 1 (Non-secure state)
     * - HCE (bit 8) = 1 (Hypervisor Call Enabled)
     * - RW (bit 10) = 1 (Lower exception levels are AArch64)
     */
    mov x0, #((1 << 0) | (1 << 8) | (1 << 10))
    msr scr_el3, x0

    /* Set up SPSR_EL3 to drop to EL2h (EL2 with SP_ELx)
     * M[3:0] = 0b1001 (EL2h)
     * F, I, A, D bits set to 1 (Interrupts masked)
     */
    mov x0, #0x3c9
    msr spsr_el3, x0

    /* Set ELR_EL3 to the address of el2_entry */
    adr x0, el2_entry
    msr elr_el3, x0

    /* Execute exception return to drop to EL2 */
    eret

el2_entry:
    /* Configure SCTLR_EL2 to safe default values (disable MMU, caching, etc. initially) */
    mrs x0, sctlr_el2
    bic x0, x0, #(1 << 0)   /* Clear M (MMU disable) */
    bic x0, x0, #(1 << 2)   /* Clear C (Data cache disable) */
    bic x0, x0, #(1 << 12)  /* Clear I (Instruction cache disable) */
    bic x0, x0, #(1 << 1)   /* Clear A (Alignment check disable) */
    msr sctlr_el2, x0
    isb

    /* Configure HCR_EL2: 
     * - VM (bit 0): Enable stage 2 translation
     * - RW (bit 31): EL1 is AArch64
     * - IMO (bit 9): Physical IRQ routed to EL2
     * - FMO (bit 8): Physical FIQ routed to EL2
     * - TGE (bit 27): Trap general exceptions to EL2
     *
     * Note: We should NOT set E2H=1 if we are not intending to use VHE (Virtualization Host Extensions).
     */
    ldr x0, =((1 << 31) | (1 << 27) | (1 << 9) | (1 << 8))
    msr hcr_el2, x0
    isb

    /* Set up early stack pointer for EL2 */
    ldr x0, =__stack_top
    mov sp, x0

    /* Set VBAR_EL2 to point to our exception vector table */
    adr x0, el2_vector_table
    msr vbar_el2, x0
    isb

    /* Initialize BSS section */
    ldr x0, =__bss_start
    ldr x1, =__bss_end
bss_loop:
    cmp x0, x1
    b.ge bss_done
    str xzr, [x0], #8
    b bss_loop

bss_done:
    /* 
     * Jump to C entry point (main)
     * Keep boot arguments in x0-x3 (passed from bootloader/QEMU)
     */
    bl main

    /* If main returns, hang */
    b hang

/* Macros for saving/restoring registers */
.macro save_regs
    sub sp, sp, #544
    
    /* Store x0-x29 (fp) */
    stp x0, x1, [sp, #0]
    stp x2, x3, [sp, #16]
    stp x4, x5, [sp, #32]
    stp x6, x7, [sp, #48]
    stp x8, x9, [sp, #64]
    stp x10, x11, [sp, #80]
    stp x12, x13, [sp, #96]
    stp x14, x15, [sp, #112]
    stp x16, x17, [sp, #128]
    stp x18, x19, [sp, #144]
    stp x20, x21, [sp, #160]
    stp x22, x23, [sp, #176]
    stp x24, x25, [sp, #192]
    stp x26, x27, [sp, #208]
    stp x28, x29, [sp, #224]
    
    /* Store x30 (lr) */
    str x30, [sp, #240]

    /* Save system registers */
    mrs x0, sp_el0
    mrs x1, elr_el2
    mrs x2, spsr_el2
    mrs x3, esr_el2
    mrs x4, far_el2

    str x0, [sp, #248]
    stp x1, x2, [sp, #256]
    stp x3, x4, [sp, #272]
    
    /* Restore clobbered registers x0-x4 */
    ldp x0, x1, [sp, #0]
    ldp x2, x3, [sp, #16]
    ldr x4, [sp, #32]
.endm

.macro restore_regs
    /* Restore system registers */
    ldr x0, [sp, #248]
    ldp x1, x2, [sp, #256]
    
    msr sp_el0, x0
    msr elr_el2, x1
    msr spsr_el2, x2

    /* Restore x0-x30 */
    ldp x0, x1, [sp, #0]
    ldp x2, x3, [sp, #16]
    ldp x4, x5, [sp, #32]
    ldp x6, x7, [sp, #48]
    ldp x8, x9, [sp, #64]
    ldp x10, x11, [sp, #80]
    ldp x12, x13, [sp, #96]
    ldp x14, x15, [sp, #112]
    ldp x16, x17, [sp, #128]
    ldp x18, x19, [sp, #144]
    ldp x20, x21, [sp, #160]
    ldp x22, x23, [sp, #176]
    ldp x24, x25, [sp, #192]
    ldp x26, x27, [sp, #208]
    ldp x28, x29, [sp, #224]
    ldr x30, [sp, #240]

    add sp, sp, #544
.endm

/*
 * EL2 Exception Vector Table
 * Must be 2048-byte aligned (2^11)
 */
.section .vectors, "ax", %progbits
.globl el2_vector_table
.align 11
el2_vector_table:
    /* Current EL with SP0 */
    .align 7
    save_regs
    mov x0, sp
    mov x1, #0
    bl default_exception_handler
    restore_regs
    eret

    .align 7
    save_regs
    mov x0, sp
    mov x1, #1
    bl default_exception_handler
    restore_regs
    eret

    .align 7
    save_regs
    mov x0, sp
    mov x1, #2
    bl default_exception_handler
    restore_regs
    eret

    .align 7
    save_regs
    mov x0, sp
    mov x1, #3
    bl default_exception_handler
    restore_regs
    eret

    /* Current EL with SPx */
    .align 7
    save_regs
    mov x0, sp
    bl handle_exception
    restore_regs
    eret

    .align 7
    save_regs
    mov x0, sp
    bl gic_irq_handler
    restore_regs
    eret

    .align 7
    save_regs
    mov x0, sp
    mov x1, #6
    bl default_exception_handler
    restore_regs
    eret

    .align 7
    save_regs
    mov x0, sp
    mov x1, #7
    bl default_exception_handler
    restore_regs
    eret

    /* Lower EL using AArch64 */
    .align 7
    save_regs
    mov x0, sp
    bl handle_exception
    restore_regs
    eret

    .align 7
    save_regs
    mov x0, sp
    bl gic_irq_handler
    restore_regs
    eret

    .align 7
    save_regs
    mov x0, sp
    mov x1, #10
    bl default_exception_handler
    restore_regs
    eret

    .align 7
    save_regs
    mov x0, sp
    mov x1, #11
    bl default_exception_handler
    restore_regs
    eret

    /* Lower EL using AArch32 */
    .align 7
    save_regs
    mov x0, sp
    mov x1, #12
    bl default_exception_handler
    restore_regs
    eret

    .align 7
    save_regs
    mov x0, sp
    mov x1, #13
    bl default_exception_handler
    restore_regs
    eret

    .align 7
    save_regs
    mov x0, sp
    mov x1, #14
    bl default_exception_handler
    restore_regs
    eret

    .align 7
    save_regs
    mov x0, sp
    mov x1, #15
    bl default_exception_handler
    restore_regs
    eret