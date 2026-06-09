/* mm.h - Memory Management for my_hypervisor */
#ifndef MM_H
#define MM_H

#define PAGE_SIZE       4096
#define PAGE_SHIFT      12
#define PAGE_MASK       (~(PAGE_SIZE - 1))

/* Memory attribute indices in MAIR_EL2 */
#define ATTR_DEVICE_INDEX    0
#define ATTR_NORMAL_INDEX    1

#define ATTR_DEVICE_VAL      0x00    /* Device-nGnRnE */
#define ATTR_NORMAL_VAL      0xff    /* Normal memory, Outer/Inner Write-Back, Outer/Inner Write-Back */

/* Page Table Entry (PTE) Flags for Stage 1 EL2 */
#define PTE_VALID            (1ULL << 0)
#define PTE_TABLE            (1ULL << 1)
#define PTE_BLOCK            (0ULL << 1)
#define PTE_PAGE             (1ULL << 1)

#define PTE_ATTR_INDEX(idx)  (((unsigned long)(idx) & 7) << 2)
#define PTE_NS               (1ULL << 5)
#define PTE_AP_RW            (1ULL << 6)   /* Read/Write */
#define PTE_AP_RO            (3ULL << 6)   /* Read-only */
#define PTE_SH_INNER         (3ULL << 8)   /* Inner Shareable */
#define PTE_AF               (1ULL << 10)  /* Access Flag */
#define PTE_PXN              (1ULL << 53)  /* Privileged Execute-Never */
#define PTE_UXN              (1ULL << 54)  /* Unprivileged Execute-Never */

/* High-level Memory mapping flags */
#define MAP_NORMAL           (PTE_VALID | PTE_PAGE | PTE_ATTR_INDEX(ATTR_NORMAL_INDEX) | PTE_AF | PTE_SH_INNER)
#define MAP_DEVICE           (PTE_VALID | PTE_PAGE | PTE_ATTR_INDEX(ATTR_DEVICE_INDEX) | PTE_AF)

/* API Declarations */
void mm_init(void);
void *alloc_page(void);
void free_page(void *ptr);
int map_page(unsigned long va, unsigned long pa, unsigned long flags);
void enable_mmu_el2(void);

#endif /* MM_H */
