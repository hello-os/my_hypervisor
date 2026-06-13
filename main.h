#ifndef __VMLINUX_MAIN_H
#define __VMLINUX_MAIN_H

void main(unsigned long r0, unsigned long r1, unsigned long r2, unsigned long r3);
void uart_putc(char c);
void uart_puts(const char *s);
void print_hex(unsigned long val);

#endif /* __VMLINUX_MAIN_H */