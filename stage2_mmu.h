#ifndef STAGE2_MMU_H
#define STAGE2_MMU_H

#include <stddef.h>

/* Stage 2 MMU Page Table Entry (PTE) Flags */
#define S2_PTE_VALID        (1ULL << 0)
#define S2_PTE_TABLE        (1ULL << 1) /* Block or Table */
#define S2_PTE_AF           (1ULL << 10) /* Access Flag */
#define S2_PTE_SH_IS        (0b11ULL << 8) /* Shareability: Inner Shareable */
#define S2_PTE_MEMATTR_NORMAL (0b0000ULL << 2) /* Normal Memory, Inner/Outer Write-back Read/Write Allocate */
#define S2_PTE_MEMATTR_DEVICE (0b0100ULL << 2) /* Device-nGnRnE */

/* Stage 2 MMU API */
void stage2_mmu_init(void);
int stage2_mmu_map(unsigned long ipa, unsigned long pa, unsigned long flags);

#endif /* STAGE2_MMU_H */
