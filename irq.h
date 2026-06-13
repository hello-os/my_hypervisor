/* irq.h - Interrupt Request (IRQ) handling for my_hypervisor */
#ifndef IRQ_H
#define IRQ_H

#define GICD_BASE       0x08000000ULL /* GIC Distributor base address */
#define GICC_BASE       0x08010000ULL /* GIC CPU Interface base address */

/* GIC Distributor Register Offsets */
#define GICD_CTLR       (0x000) /* Distributor Control Register */
#define GICD_TYPER      (0x004) /* Interrupt Controller Type Register */
#define GICD_ISENABLER  (0x100) /* Interrupt Set-Enable Registers */
#define GICD_ICENABLER  (0x180) /* Interrupt Clear-Enable Registers */
#define GICD_ISPENDR    (0x200) /* Interrupt Set-Pending Registers */
#define GICD_ICPENDR    (0x280) /* Interrupt Clear-Pending Registers */
#define GICD_IPRIORITYR (0x400) /* Interrupt Priority Registers */
#define GICD_ITARGETSR  (0x800) /* Interrupt Processor Targets Registers */
#define GICD_ICFGR      (0xC00) /* Interrupt Configuration Registers */

/* GIC CPU Interface Register Offsets */
#define GICC_CTLR       (0x000) /* CPU Interface Control Register */
#define GICC_PMR        (0x004) /* Priority Mask Register */
#define GICC_IAR        (0x00C) /* Interrupt Acknowledge Register */
#define GICC_EOIR       (0x010) /* End of Interrupt Register */
#define GICC_HPPIR      (0x018) /* Highest Priority Pending Interrupt Register */

/* ARM Generic Timer Registers (CNTV_CTL_EL0 / CNTV_TVAL_EL0) */
#define CNTFRQ_EL0_REG  "cntfrq_el0"
#define CNTV_CTL_EL0_REG "cntv_ctl_el0"
#define CNTV_TVAL_EL0_REG "cntv_tval_el0"
#define CNTV_CVAL_EL0_REG "cntv_cval_el0"

/* Exception Syndrome Register (ESR_EL2) bits */
#define ESR_EL2_EC_SHIFT    26
#define ESR_EL2_EC_MASK     0x3F

/* Exception Classes (EC) for ESR_EL2 */
#define EC_UNKNOWN          0b000000
#define EC_FP_SIMD          0b000111
#define EC_ILL_INSTR        0b001110
#define EC_PC_ALIGN         0b010000
#define EC_SP_ALIGN         0b010001
#define EC_DATA_ABORT_EL0   0b100100
#define EC_DATA_ABORT_EL1   0b100101
#define EC_INST_ABORT_EL0   0b100000
#define EC_INST_ABORT_EL1   0b100001
#define EC_SVC_AARCH64      0b010101 /* HVC/SVC in AArch64 */
#define EC_HVC              0b010001 /* HVC exception from a lower EL */
#define EC_IRQ              0b010101 /* IRQ exception */

#define SPSR_EL2_MODE_EL1H  0b0101
#define SPSR_EL2_MODE_EL1T  0b0100
#define SPSR_EL2_D_BIT      (1 << 9)
#define SPSR_EL2_A_BIT      (1 << 8)
#define SPSR_EL2_I_BIT      (1 << 7)
#define SPSR_EL2_F_BIT      (1 << 6)

/* Exception types for vector table */
#define SYNC_EXCEPTION      0
#define IRQ_EXCEPTION       1
#define FIQ_EXCEPTION       2
#define SERROR_EXCEPTION    3

/* Frame structure for saving registers on exception entry */
struct cpu_regs {
    unsigned long x0;
    unsigned long x1;
    unsigned long x2;
    unsigned long x3;
    unsigned long x4;
    unsigned long x5;
    unsigned long x6;
    unsigned long x7;
    unsigned long x8;
    unsigned long x9;
    unsigned long x10;
    unsigned long x11;
    unsigned long x12;
    unsigned long x13;
    unsigned long x14;
    unsigned long x15;
    unsigned long x16;
    unsigned long x17;
    unsigned long x18;
    unsigned long x19;
    unsigned long x20;
    unsigned long x21;
    unsigned long x22;
    unsigned long x23;
    unsigned long x24;
    unsigned long x25;
    unsigned long x26;
    unsigned long x27;
    unsigned long x28;
    unsigned long fp;           /* x29 */
    unsigned long lr;           /* x30 */
    unsigned long sp_el0;       /* SP_EL0 */
    unsigned long elr_el2;      /* ELR_EL2 */
    unsigned long spsr_el2;     /* SPSR_EL2 */
    unsigned long esr_el2;      /* ESR_EL2 */
    unsigned long far_el2;      /* FAR_EL2 */
    unsigned long q0_q31[32];   /* Placeholder for FP/SIMD registers */
};

/* Public API */
void irq_init(void);
void irq_enable(void);
void irq_disable(void);
void timer_init(unsigned int us);
void handle_exception(struct cpu_regs *regs);
void default_exception_handler(struct cpu_regs *regs, unsigned int exception_type);
void gic_irq_handler(struct cpu_regs *regs);
void hvc_handler(struct cpu_regs *regs);

#endif /* IRQ_H */
