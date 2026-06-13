/* main.c for my_hypervisor */

/* QEMU virt machine PL011 UART Base Address */
#include "mm.h"
#include "irq.h"
#include "vmm_heap.h"
#include "dt_parser.h"

#define PL011_UART_BASE 0x09000000

void uart_putc(char c)
{
    volatile unsigned int *uart = (volatile unsigned int *)PL011_UART_BASE;
    /* Wait if transmit FIFO is full (simplified for emulation, QEMU is usually ready) */
    *uart = c;
}

void uart_puts(const char *s)
{
    while (*s) {
        if (*s == '\n') {
            uart_putc('\r');
        }
        uart_putc(*s++);
    }
}

void print_hex(unsigned long val)
{
    char hex_chars[] = "0123456789ABCDEF";
    uart_puts("0x");
    for (int i = 60; i >= 0; i -= 4) {
        uart_putc(hex_chars[(val >> i) & 0xf]);
    }
}

unsigned long __hcr_el2_val = (1 << 31) | (1 << 27); /* RW, HCR_EL2.VM */

int main(unsigned long r0, unsigned long r1, unsigned long r2, unsigned long r3)
{
    unsigned long current_el;

    /* Welcome message */
    uart_puts("\n============================================\n");
    uart_puts("       Welcome to my_hypervisor!            \n");
    uart_puts("============================================\n");

    /* Read Current EL */
    __asm__ volatile("mrs %0, CurrentEL" : "=r" (current_el));
    
    /* CurrentEL register format: bits [3:2] contain the Exception Level */
    unsigned long el = (current_el >> 2) & 0x3;

    uart_puts("Hypervisor running at Exception Level: EL");
    uart_putc('0' + el);
    uart_puts("\n");

    uart_puts("Boot Arguments:\n");
    uart_puts("  r0 (FDT address if booted by QEMU): ");
    print_hex(r0);
    uart_puts("\n");
    uart_puts("  r1: ");
    print_hex(r1);
    uart_puts("\n");
    uart_puts("  r2: ");
    print_hex(r2);
    uart_puts("\n");
    uart_puts("  r3: ");
    print_hex(r3);
    uart_puts("\n");

    mm_init(); /* Initialize memory manager and build identity page tables */
    enable_mmu_el2(); /* Enable MMU and caches */

    irq_init(); /* Initialize GIC and timer */
    irq_enable(); /* Enable global IRQ interrupts */

    uart_puts("\nSystem initialized. MMU enabled. Initializing Heap...\n");
    
    /* 1MB heap at 0x41000000 */
    vmm_heap_init((void *)0x41000000, 1024 * 1024);
    uart_puts("[Heap] Initialized at 0x41000000 (1MB)\n");

    /* Test malloc & free */
    void *ptr1 = vmm_malloc(100);
    uart_puts("[Heap] vmm_malloc(100) returned: ");
    print_hex((unsigned long)ptr1);
    uart_puts("\n");

    void *ptr2 = vmm_malloc(200);
    uart_puts("[Heap] vmm_malloc(200) returned: ");
    print_hex((unsigned long)ptr2);
    uart_puts("\n");

    vmm_free(ptr1);
    uart_puts("[Heap] vmm_free(ptr1) done.\n");

    void *ptr3 = vmm_malloc(50);
    uart_puts("[Heap] vmm_malloc(50) returned: ");
    print_hex((unsigned long)ptr3);
    uart_puts("\n");

    vmm_free(ptr2);
    vmm_free(ptr3);

    /* Initialize DT parser */
    uart_puts("\n[DT] Initializing DTB Parser...\n");
    dt_init((void *)r0);
    uart_puts("[DT] Parser initialized.\n");

    /* Start interactive shell */
    #include "vmm_shell.h"
    shell_loop();

    uart_puts("\nEntering hypervisor main loop...\n");

    while (1) {
        /* Hypervisor loop */
    }

    return 0;
}
