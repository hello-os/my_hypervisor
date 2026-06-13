/* irq.c - Interrupt Request (IRQ) handling for my_hypervisor */
#include "irq.h"
#include "mm.h" /* For MMIO mapping */

/* Simple UART print functions (copy from main.c for now) */
#define PL011_UART_BASE 0x09000000
static void irq_uart_putc(char c)
{
    volatile unsigned int *uart = (volatile unsigned int *)PL011_UART_BASE;
    *uart = c;
}

static void irq_uart_puts(const char *s)
{
    while (*s) {
        if (*s == '\n') {
            irq_uart_putc('\r');
        }
        irq_uart_putc(*s++);
    }
}

static void irq_print_hex(unsigned long val)
{
    char hex_chars[] = "0123456789ABCDEF";
    irq_uart_puts("0x");
    for (int i = 60; i >= 0; i -= 4) {
        irq_uart_putc(hex_chars[(val >> i) & 0xf]);
    }
}

/* GIC registers accessors */
#define GIC_WRITE_REG(base, offset, val)    (*(volatile unsigned long *)((base) + (offset)) = (val))
#define GIC_READ_REG(base, offset)          (*(volatile unsigned long *)((base) + (offset)))

/* Global IRQ handler table (for future, not used yet) */
typedef void (*irq_handler_t)(unsigned int irq_num, struct cpu_regs *regs);
static irq_handler_t irq_handlers[1024]; /* Max 1024 interrupts for GICv2 */

/* Forward declarations */
static void gic_dist_init(void);
static void gic_cpu_init(void);

/* IRQ initialization */
void irq_init(void)
{
    irq_uart_puts("[IRQ] Initializing GIC...");
    gic_dist_init();
    gic_cpu_init();
    irq_uart_puts("Done.\n");

    /* Initialize generic timer (EL2 Physical Timer) for 10ms interval */
    timer_init(10000); /* 10ms in microseconds */
    irq_uart_puts("[IRQ] Generic Timer initialized for 10ms interval.\n");
}

/* Enable IRQ interrupts (A bit in PSTATE) */
void irq_enable(void)
{
    __asm__ volatile("msr daifclr, #2"); /* Clear A bit */
}

/* Disable IRQ interrupts (A bit in PSTATE) */
void irq_disable(void)
{
    __asm__ volatile("msr daifset, #2"); /* Set A bit */
}

/* GIC Distributor initialization */
static void gic_dist_init(void)
{
    unsigned int gic_typer;
    unsigned int i;
    unsigned int num_irq_lines;

    /* Disable distributor */
    GIC_WRITE_REG(GICD_BASE, GICD_CTLR, 0);

    /* Get number of IRQ lines */
    gic_typer = GIC_READ_REG(GICD_BASE, GICD_TYPER);
    num_irq_lines = ((gic_typer & 0x1f) + 1) * 32;
    irq_uart_puts("[IRQ] GIC reports ");
    irq_print_hex(num_irq_lines);
    irq_uart_puts(" IRQ lines.\n");

    /* Disable all interrupts */
    for (i = 0; i < num_irq_lines / 32; i++) {
        GIC_WRITE_REG(GICD_BASE, GICD_ICENABLER + i * 4, 0xFFFFFFFF);
        GIC_WRITE_REG(GICD_BASE, GICD_ICPENDR + i * 4, 0xFFFFFFFF);
    }

    /* Set all interrupts to Group 0 (Secure interrupts for EL3, Non-secure for EL2) */
    /* QEMU virt GIC is typically Group 0 for EL2 */
    for (i = 0; i < num_irq_lines / 32; i++) {
        // GIC_WRITE_REG(GICD_BASE, GICD_IGROUPR + i * 4, 0);
    }

    /* Set all interrupts to priority 0xf0 (lowest configurable priority) */
    for (i = 0; i < num_irq_lines / 4; i++) {
        GIC_WRITE_REG(GICD_BASE, GICD_IPRIORITYR + i * 4, 0xF0F0F0F0);
    }

    /* Set all interrupts to target CPU0 (for now) */
    for (i = 0; i < num_irq_lines / 4; i++) {
        GIC_WRITE_REG(GICD_BASE, GICD_ITARGETSR + i * 4, 0x01010101); /* Target CPU0 */
    }
    
    /* Enable distributor */
    GIC_WRITE_REG(GICD_BASE, GICD_CTLR, 1); /* Enable GICD_CTLR.EnableGroup0 and EnableGroup1 */
}

/* GIC CPU Interface initialization */
static void gic_cpu_init(void)
{
    /* Disable CPU interface */
    GIC_WRITE_REG(GICC_BASE, GICC_CTLR, 0);

    /* Set running priority to lowest (allow all priorities to pass) */
    GIC_WRITE_REG(GICC_BASE, GICC_PMR, 0xFF);

    /* Enable CPU interface */
    GIC_WRITE_REG(GICC_BASE, GICC_CTLR, 1); /* EnableGroup0 and EnableGroup1 */
}

/* Initialize ARM Generic Timer */
void timer_init(unsigned int us)
{
    unsigned long cntfrq_el0; /* Counter-timer frequency */
    unsigned long interval_ticks;

    __asm__ volatile("mrs %0, " CNTFRQ_EL0_REG : "=r" (cntfrq_el0));
    if (cntfrq_el0 == 0) {
        irq_uart_puts("[IRQ ERROR] CNTFRQ_EL0 is 0, cannot setup timer!\n");
        return;
    }

    /* Calculate interval in ticks */
    interval_ticks = (cntfrq_el0 * us) / 1000000;

    /* Set timer value and enable timer with interrupt */
    __asm__ volatile("msr " CNTV_TVAL_EL0_REG ", %0" :: "r" (interval_ticks));
    __asm__ volatile("msr " CNTV_CTL_EL0_REG ", %0" :: "r" (1)); /* Enable, IMASK=0, ISTATUS=0 */
}

/* Generic exception handler */
void default_exception_handler(struct cpu_regs *regs, unsigned int exception_type)
{
    irq_uart_puts("\n[EXCEPTION] Unhandled Exception!\n");
    irq_uart_puts("  Exception Type: ");
    irq_print_hex(exception_type);
    irq_uart_puts("\n");

    irq_uart_puts("  ELR_EL2: ");
    irq_print_hex(regs->elr_el2);
    irq_uart_puts("\n");

    irq_uart_puts("  SPSR_EL2: ");
    irq_print_hex(regs->spsr_el2);
    irq_uart_puts("\n");

    irq_uart_puts("  ESR_EL2: ");
    irq_print_hex(regs->esr_el2);
    irq_uart_puts("\n");

    unsigned int ec = (regs->esr_el2 >> ESR_EL2_EC_SHIFT) & ESR_EL2_EC_MASK;
    irq_uart_puts("  ESR_EL2 Exception Class (EC): ");
    irq_print_hex(ec);
    irq_uart_puts("\n");

    if (ec == EC_DATA_ABORT_EL1 || ec == EC_INST_ABORT_EL1) {
        irq_uart_puts("  FAR_EL2: ");
        irq_print_hex(regs->far_el2);
        irq_uart_puts("\n");
    }

    while (1); /* Hang */
}

/* GIC IRQ handler */
void gic_irq_handler(struct cpu_regs *regs)
{
    unsigned int irq_num;

    /* Read Interrupt Acknowledge Register to get interrupt ID */
    irq_num = GIC_READ_REG(GICC_BASE, GICC_IAR);

    /* Special interrupt IDs */
    if (irq_num >= 1020) { /* Spurious interrupt or other special ID */
        /* Write to End Of Interrupt Register only if it's not a spurious interrupt */
        if (irq_num != 1023) {
            GIC_WRITE_REG(GICC_BASE, GICC_EOIR, irq_num);
        }
        return;
    }

    irq_uart_puts("[IRQ] Handling IRQ: ");
    irq_print_hex(irq_num);
    irq_uart_puts("\n");

    /* Handle Generic Timer interrupt (Physical EL2 Timer, IRQ 27) */
    if (irq_num == 27) {
        irq_uart_puts("[IRQ] Timer interrupt!\n");
        /* Re-enable timer with a new value */
        __asm__ volatile("msr " CNTV_TVAL_EL0_REG ", %0" :: "r" (10000 * (GIC_READ_REG(GICD_BASE, GICD_TYPER) & 0x1f) + 1)); // Placeholder for actual frequency
        // In real system, would read CNTFRQ_EL0_REG and set for next 10ms
        __asm__ volatile("msr " CNTV_TVAL_EL0_REG ", %0" :: "r" (10000 * 200000)); // QEMU specific
    }

    /* Call specific handler if registered (not yet implemented) */
    if (irq_handlers[irq_num]) {
        irq_handlers[irq_num](irq_num, regs);
    }

    /* Write to End Of Interrupt Register */
    GIC_WRITE_REG(GICC_BASE, GICC_EOIR, irq_num);
}

void data_abort_handler(struct cpu_regs *regs)
{
    irq_uart_puts("\n[DATA ABORT] Data Abort from EL1!\n");
    irq_uart_puts("  ESR_EL2: ");
    irq_print_hex(regs->esr_el2);
    irq_uart_puts("\n");
    irq_uart_puts("  FAR_EL2: ");
    irq_print_hex(regs->far_el2);
    irq_uart_puts("\n");

    /* Advance ELR_EL2 to skip the faulting instruction */
    regs->elr_el2 += 4;
}

void hvc_handler(struct cpu_regs *regs)
{
    irq_uart_puts("\n[HVC] Hypervisor Call from EL1!\n");
    irq_uart_puts("  HVC Number: ");
    irq_print_hex(regs->x0);
    irq_uart_puts("\n");
    
    /* For now, just return from HVC */
    regs->elr_el2 += 4; /* Advance ELR_EL2 to skip HVC instruction */
}

/* Common Synchronous exception dispatcher */
void handle_exception(struct cpu_regs *regs)
{
    unsigned int ec = (regs->esr_el2 >> ESR_EL2_EC_SHIFT) & ESR_EL2_EC_MASK;

    irq_uart_puts("\n[DEBUG] handle_exception EC: ");
    irq_print_hex(ec);
    irq_uart_puts("\n");
    irq_uart_puts("\n[DEBUG] EC_DATA_ABORT_EL1: ");
    irq_print_hex(EC_DATA_ABORT_EL1);
    irq_uart_puts("\n");

    if (ec == EC_HVC || ec == EC_SVC_AARCH64) {
        hvc_handler(regs);
    } else if (ec == EC_DATA_ABORT_EL1) { /* Correctly handle Data Abort from EL1 */
        data_abort_handler(regs);
    } else {
        default_exception_handler(regs, SYNC_EXCEPTION);
    }
}


