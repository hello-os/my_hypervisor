#include "vmm_shell.h"
#include "vmm_heap.h"

#define PL011_UART_BASE 0x09000000
#define UART_FR         0x018
#define UART_DR         0x000

#define RXFE            (1 << 4) /* Receive FIFO empty */

extern void uart_putc(char c);
extern void uart_puts(const char *s);
extern void print_hex(unsigned long val);

static char uart_getc(void)
{
    volatile unsigned int *uart = (volatile unsigned int *)PL011_UART_BASE;
    /* Wait until a character is available */
    while (uart[UART_FR / 4] & RXFE) {
        /* Busy wait */
    }
    return (char)(uart[UART_DR / 4] & 0xFF);
}

static int strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

static unsigned long parse_hex(const char *s)
{
    unsigned long val = 0;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
    }
    while (*s) {
        char c = *s++;
        val <<= 4;
        if (c >= '0' && c <= '9') {
            val += c - '0';
        } else if (c >= 'a' && c <= 'f') {
            val += c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
            val += c - 'A' + 10;
        } else {
            break;
        }
    }
    return val;
}

void shell_init(void)
{
    uart_puts("\n--------------------------------------------\n");
    uart_puts("   Welcome to my_hypervisor Interactive Shell\n");
    uart_puts("   Type 'help' to see available commands.     \n");
    uart_puts("--------------------------------------------\n");
}

static void shell_exec(char *cmdline)
{
    /* Simple tokenization (only support single argument) */
    char cmd[32] = {0};
    char arg[32] = {0};
    int i = 0, j = 0;

    /* Get command */
    while (cmdline[i] && cmdline[i] != ' ' && i < 31) {
        cmd[i] = cmdline[i];
        i++;
    }
    cmd[i] = '\0';

    if (cmdline[i] == ' ') {
        i++;
        /* Get argument */
        while (cmdline[i] && cmdline[i] != ' ' && j < 31) {
            arg[j] = cmdline[i];
            i++;
            j++;
        }
        arg[j] = '\0';
    }

    if (strcmp(cmd, "help") == 0) {
        uart_puts("Available commands:\n");
        uart_puts("  help             - Show this help menu\n");
        uart_puts("  version          - Show hypervisor version\n");
        uart_puts("  heap             - Quick heap allocator verification\n");
        uart_puts("  md <addr>        - Memory Dump (hex dump 32-bit value)\n");
    } else if (strcmp(cmd, "version") == 0) {
        uart_puts("my_hypervisor version 0.1 (aligned with Xvisor 0.3.2 structure)\n");
    } else if (strcmp(cmd, "heap") == 0) {
        uart_puts("Triggering Heap Test:\n");
        void *p = vmm_malloc(128);
        uart_puts("Allocated 128 bytes at: ");
        print_hex((unsigned long)p);
        uart_puts("\nFreeing memory...\n");
        vmm_free(p);
        uart_puts("Heap test complete.\n");
    } else if (strcmp(cmd, "md") == 0) {
        if (arg[0] == '\0') {
            uart_puts("Usage: md <addr>\n");
        } else {
            unsigned long addr = parse_hex(arg);
            uart_puts("Reading from Address ");
            print_hex(addr);
            uart_puts(": ");
            volatile unsigned int *ptr = (volatile unsigned int *)addr;
            print_hex(*ptr);
            uart_puts("\n");
        }
    } else if (cmd[0] != '\0') {
        uart_puts("Unknown command: ");
        uart_puts(cmd);
        uart_puts("\nType 'help' for command list.\n");
    }
}

void shell_loop(void)
{
    char buf[64];
    int idx = 0;

    shell_init();

    while (1) {
        uart_puts("my_hypervisor# ");
        idx = 0;
        while (1) {
            char c = uart_getc();
            if (c == '\r' || c == '\n') {
                uart_puts("\n");
                buf[idx] = '\0';
                break;
            } else if (c == 127 || c == '\b') { /* Backspace */
                if (idx > 0) {
                    idx--;
                    uart_puts("\b \b");
                }
            } else {
                if (idx < 63) {
                    buf[idx++] = c;
                    uart_putc(c); /* Echo back */
                }
            }
        }
        shell_exec(buf);
    }
}
