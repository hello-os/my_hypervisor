#include "stage2_mmu.h"
#include "mm.h" /* For alloc_page, PAGE_SIZE, PAGE_MASK */
#include "vmm_heap.h" /* For vmm_malloc (if needed for larger tables) */

/* Root Stage 2 Page Table (Level 0) */
static unsigned long *s2_l0_table = 0;

extern void uart_puts(const char *s);
extern void print_hex(unsigned long val);

/* Helper to map a page in Stage 2 tables */
static int s2_map_page(unsigned long *table, unsigned long va, unsigned long pa, unsigned long flags, int level)
{
    unsigned long idx;
    if (level == 0) idx = (va >> 39) & 0x1ff;
    else if (level == 1) idx = (va >> 30) & 0x1ff;
    else if (level == 2) idx = (va >> 21) & 0x1ff;
    else idx = (va >> 12) & 0x1ff;

    unsigned long *next_table;

    if (level < 3) {
        /* Not the final level, need to point to another table or a block */
        if (!(table[idx] & S2_PTE_VALID) || !(table[idx] & S2_PTE_TABLE)) {
            next_table = (unsigned long *)alloc_page();
            if (!next_table) return -1; /* Out of memory */
            table[idx] = (unsigned long)next_table | S2_PTE_TABLE | S2_PTE_VALID;
        } else {
            next_table = (unsigned long *)(table[idx] & PAGE_MASK);
        }
        return s2_map_page(next_table, va, pa, flags, level + 1);
    } else {
        /* Final level (L3), map the page */
        table[idx] = (pa & PAGE_MASK) | flags | S2_PTE_AF | S2_PTE_VALID;
    }
    return 0;
}

void stage2_mmu_init(void)
{
    s2_l0_table = (unsigned long *)alloc_page();
    if (!s2_l0_table) {
        uart_puts("[S2MMU ERROR] Failed to allocate Stage 2 L0 root table!\n");
        return;
    }
    uart_puts("[S2MMU] Root L0 Stage 2 Page Table allocated at: ");
    print_hex((unsigned long)s2_l0_table);
    uart_puts("\n");

    /* Configure HCR_EL2 for Stage 2 MMU */
    unsigned long hcr_el2_val;
    __asm__ volatile("mrs %0, hcr_el2" : "=r" (hcr_el2_val));
    hcr_el2_val |= (1ULL << 0); /* HCR_EL2.VM = 1 (Enable Stage 2 MMU) */
    __asm__ volatile("msr hcr_el2, %0" :: "r" (hcr_el2_val));

    /* Configure VTCR_EL2 */
    unsigned long vtcr_el2_val = 0;
    vtcr_el2_val |= (24ULL << 0);   /* T0SZ = 24 (40-bit IPA) */
    vtcr_el2_val |= (0b00ULL << 14); /* TG0 = 4KB granule */
    vtcr_el2_val |= (0b11ULL << 12); /* SH0 = Inner Shareable */
    vtcr_el2_val |= (0b01ULL << 10); /* ORGN0 = WB RA WA Cacheable */
    vtcr_el2_val |= (0b01ULL << 8);  /* IRGN0 = WB RA WA Cacheable */
    vtcr_el2_val |= (0b010ULL << 16); /* PS = 40-bit PA range */
    vtcr_el2_val |= (1ULL << 6); /* SL0 = 1 (start lookup at L1) */
    __asm__ volatile("msr vtcr_el2, %0" :: "r" (vtcr_el2_val));

    /* Set VTTBR_EL2 to L0 root table and VMID (e.g., VMID 1) */
    __asm__ volatile("msr vttbr_el2, %0" :: "r" ((unsigned long)s2_l0_table | (1ULL << 48))); /* VMID = 1 */

    uart_puts("[S2MMU] HCR_EL2, VTCR_EL2, VTTBR_EL2 configured.\n");
}

int stage2_mmu_map(unsigned long ipa, unsigned long pa, unsigned long flags)
{
    return s2_map_page(s2_l0_table, ipa, pa, flags, 0);
}
