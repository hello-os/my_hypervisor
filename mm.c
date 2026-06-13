/* mm.c - Memory Management implementation for my_hypervisor */
#include "mm.h"

extern char _end[];

/* QEMU virt RAM configuration (128MB) */
#define RAM_START       0x40000000
#define RAM_END         0x48000000

/* Linker symbols */
static unsigned long free_mem_start;
static unsigned long free_mem_end = RAM_END;

/* Simple physical page allocator free-list node */
struct free_page_node {
    struct free_page_node *next;
};

static struct free_page_node *free_page_head = 0;
static unsigned long total_free_pages = 0;

/* Root Page Table (Level 0) */
static unsigned long *l0_table = 0;

/* Basic memset utility */
static void *memset(void *s, int c, unsigned long n)
{
    unsigned char *p = s;
    while (n--) {
        *p++ = (unsigned char)c;
    }
    return s;
}

/* Print function from main.c if needed, but we can write simple UART print */
#define PL011_UART_BASE 0x09000000
static void mm_uart_puts(const char *s)
{
    volatile unsigned int *uart = (volatile unsigned int *)PL011_UART_BASE;
    while (*s) {
        if (*s == '\n') {
            *uart = '\r';
        }
        *uart = *s++;
    }
}

static void mm_print_hex(unsigned long val)
{
    char hex_chars[] = "0123456789ABCDEF";
    mm_uart_puts("0x");
    for (int i = 60; i >= 0; i -= 4) {
        char c = hex_chars[(val >> i) & 0xf];
        volatile unsigned int *uart = (volatile unsigned int *)PL011_UART_BASE;
        *uart = c;
    }
}

#include "irq.h"

/* Page Allocator implementation */
void *alloc_page(void)
{
    if (!free_page_head) {
        mm_uart_puts("[MM ERROR] Out of memory (No free pages left)!\n");
        return 0;
    }
    struct free_page_node *node = free_page_head;
    free_page_head = node->next;
    total_free_pages--;

    /* Zero the page */
    memset(node, 0, PAGE_SIZE);
    return (void *)node;
}

void free_page(void *ptr)
{
    if (!ptr) return;
    
    /* Ensure address is page-aligned */
    unsigned long addr = (unsigned long)ptr;
    if (addr & (PAGE_SIZE - 1)) {
        mm_uart_puts("[MM ERROR] Attempted to free unaligned page address: ");
        mm_print_hex(addr);
        mm_uart_puts("\n");
        return;
    }

    struct free_page_node *node = (struct free_page_node *)ptr;
    node->next = free_page_head;
    free_page_head = node;
    total_free_pages++;
}

/* Map a virtual page to physical page (4KB Granularity, 4-level translation) */
int map_page(unsigned long va, unsigned long pa, unsigned long flags)
{
    unsigned long l0_idx = (va >> 39) & 0x1ff;
    unsigned long l1_idx = (va >> 30) & 0x1ff;
    unsigned long l2_idx = (va >> 21) & 0x1ff;
    unsigned long l3_idx = (va >> 12) & 0x1ff;

    unsigned long *l1_table;
    unsigned long *l2_table;
    unsigned long *l3_table;

    /* 1. Resolve Level 0 Entry */
    if (!(l0_table[l0_idx] & PTE_VALID)) {
        l1_table = (unsigned long *)alloc_page();
        if (!l1_table) return -1;
        l0_table[l0_idx] = (unsigned long)l1_table | PTE_TABLE | PTE_VALID;
    } else {
        l1_table = (unsigned long *)(l0_table[l0_idx] & PAGE_MASK);
    }

    /* 2. Resolve Level 1 Entry */
    if (!(l1_table[l1_idx] & PTE_VALID)) {
        l2_table = (unsigned long *)alloc_page();
        if (!l2_table) return -1;
        l1_table[l1_idx] = (unsigned long)l2_table | PTE_TABLE | PTE_VALID;
    } else {
        l2_table = (unsigned long *)(l1_table[l1_idx] & PAGE_MASK);
    }

    /* 3. Resolve Level 2 Entry */
    if (!(l2_table[l2_idx] & PTE_VALID)) {
        l3_table = (unsigned long *)alloc_page();
        if (!l3_table) return -1;
        l2_table[l2_idx] = (unsigned long)l3_table | PTE_TABLE | PTE_VALID;
    } else {
        l3_table = (unsigned long *)(l2_table[l2_idx] & PAGE_MASK);
    }

    /* 4. Map the Page at Level 3 */
    l3_table[l3_idx] = (pa & PAGE_MASK) | flags | PTE_VALID;

    return 0;
}

/* Initialize Physical Allocator and build Identity Mapped Page Tables */
void mm_init(void)
{
    /* Calculate start of free heap memory */
    free_mem_start = ((unsigned long)_end + PAGE_SIZE - 1) & PAGE_MASK;

    mm_uart_puts("[MM] Free memory heap start: ");
    mm_print_hex(free_mem_start);
    mm_uart_puts("\n");

    /* Populate physical page allocator list */
    unsigned long current_addr = free_mem_start;
    while (current_addr + PAGE_SIZE <= free_mem_end) {
        struct free_page_node *node = (struct free_page_node *)current_addr;
        node->next = free_page_head;
        free_page_head = node;
        total_free_pages++;
        current_addr += PAGE_SIZE;
    }

    mm_uart_puts("[MM] Total free pages registered: ");
    mm_print_hex(total_free_pages);
    mm_uart_puts("\n");

    /* Allocate L0 table (the root table) */
    l0_table = (unsigned long *)alloc_page();
    if (!l0_table) {
        mm_uart_puts("[MM ERROR] Failed to allocate L0 root table!\n");
        return;
    }
    mm_uart_puts("[MM] Root L0 Page Table allocated at: ");
    mm_print_hex((unsigned long)l0_table);
    mm_uart_puts("\n");

    /* 
     * Perform Identity Mapping (VA = PA)
     * 1. Map UART registers (Device memory) - 0x09000000
     */
    map_page(PL011_UART_BASE, PL011_UART_BASE, MAP_DEVICE);

    /* 2. Map GIC registers (Device memory) - 0x08000000 to 0x08020000 (roughly 128KB) */
    map_page(GICD_BASE, GICD_BASE, MAP_DEVICE);
    map_page(GICC_BASE, GICC_BASE, MAP_DEVICE);

    /*
     * 3. Map entire RAM range (Normal Memory) - 0x40000000 to 0x48000000
     */
    unsigned long addr = RAM_START;
    while (addr < RAM_END) {
        map_page(addr, addr, MAP_NORMAL);
        addr += PAGE_SIZE;
    }

    mm_uart_puts("[MM] Identity mapping created for UART, GIC, and RAM.\n");
}

/* Configure TCR, MAIR, TTBR, and Enable MMU at EL2 */
void enable_mmu_el2(void)
{
    if (!l0_table) {
        mm_uart_puts("[MM ERROR] MMU enable requested but page tables not initialized!\n");
        return;
    }

    /* 
     * 1. Set MAIR_EL2 (Memory Attribute Indirection Register)
     * Index 0 = Device-nGnRnE (0x00)
     * Index 1 = Normal Memory (0xFF)
     */
    unsigned long mair = (ATTR_DEVICE_VAL << (ATTR_DEVICE_INDEX * 8)) |
                         (ATTR_NORMAL_VAL << (ATTR_NORMAL_INDEX * 8));
    __asm__ volatile("msr mair_el2, %0" :: "r" (mair));

    /*
     * 2. Set TCR_EL2 (Translation Control Register)
     * - T0SZ (bit [5:0]) = 24 (40-bit VA space: 2^(64-24) = 1TB)
     * - TG0 (bit [15:14]) = 0b00 (4KB Page size)
     * - SH0 (bit [13:12]) = 0b11 (Inner Shareable)
     * - ORGN0 (bit [11:10]) = 0b01 (Outer Write-Back Read-Allocate Write-Allocate Cacheable)
     * - IRGN0 (bit [9:8]) = 0b01 (Inner Write-Back Read-Allocate Write-Allocate Cacheable)
     * - PS (bit [18:16]) = 0b010 (40-bit Physical Address size)
     */
    unsigned long tcr = 24 |                /* T0SZ */
                        (0b00ULL << 14) |   /* TG0 */
                        (0b11ULL << 12) |   /* SH0 */
                        (0b01ULL << 10) |   /* ORGN0 */
                        (0b01ULL << 8) |    /* IRGN0 */
                        (0b010ULL << 16);   /* PS */
    __asm__ volatile("msr tcr_el2, %0" :: "r" (tcr));

    /*
     * 3. Set TTBR0_EL2 (Translation Table Base Register) to L0 root table pointer
     */
    __asm__ volatile("msr ttbr0_el2, %0" :: "r" ((unsigned long)l0_table));

    /* Ensure memory writes completed */
    __asm__ volatile("dsb sy; isb");

    /*
     * 4. Enable MMU in SCTLR_EL2 (System Control Register EL2)
     * - M (bit 0) = 1 (Enable MMU)
     * - C (bit 2) = 1 (Enable Data Cache)
     * - I (bit 12) = 1 (Enable Instruction Cache)
     */
    unsigned long sctlr;
    __asm__ volatile("mrs %0, sctlr_el2" : "=r" (sctlr));
    sctlr |= (1ULL << 0) |    /* M */
             (1ULL << 2) |    /* C */
             (1ULL << 12);    /* I */
    __asm__ volatile("msr sctlr_el2, %0" :: "r" (sctlr));

    /* Instruction Barrier to synchronize pipeline with new translation tables */
    __asm__ volatile("isb");

    mm_uart_puts("[MM] MMU and Caches enabled successfully at EL2!\n");
}
