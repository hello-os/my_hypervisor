/* main.c for my_hypervisor */

/* QEMU virt machine PL011 UART Base Address */
#define PL011_UART_BASE 0x09000000

static void uart_putc(char c)
{
    volatile unsigned int *uart = (volatile unsigned int *)PL011_UART_BASE;
    /* Wait if transmit FIFO is full (simplified for emulation, QEMU is usually ready) */
    *uart = c;
}

static void uart_puts(const char *s)
{
    while (*s) {
        if (*s == '\n') {
            uart_putc('\r');
        }
        uart_putc(*s++);
    }
}

static void print_hex(unsigned long val)
{
    char hex_chars[] = "0123456789ABCDEF";
    uart_puts("0x");
    for (int i = 60; i >= 0; i -= 4) {
        uart_putc(hex_chars[(val >> i) & 0xf]);
    }
}

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

    uart_puts("\nSystem initialized. Entering hypervisor main loop...\n");

    while (1) {
        /* Hypervisor loop */
    }

    return 0;
}
