/* start.S for my_hypervisor */

.section .text.boot, "ax"
.globl _start

_start:
    /* Disable interrupts */
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

    /* Set up early stack pointer for EL2 */
    ldr x0, =__stack_top
    mov sp, x0

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
