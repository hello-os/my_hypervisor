#include "dt.h"
#include "mm.h"
#include "main.h" // For uart_puts and print_hex
#include <stddef.h>
// #include <stdio.h> // No longer needed as we're not using standard printf

// Forward declarations for helper functions
void *dt_find_node(void *dtb, const char *name);
void *dt_get_property(void *node, const char *prop_name);

static dt_info_t dt_info;

int dt_init(void *dtb, size_t dtb_size) {
    uint8_t *ptr = (uint8_t *)dtb;
    
    // Validate DTB header
    if (ptr[0] != 0xd0 || ptr[1] != 0x0d || (ptr[2] != 0xfe && ptr[3] != 0xed)) { // Fixed operator precedence
        return -1; // Invalid magic
    }
    
    // Parse memory reservation block
    // uint32_t mem_rsv_size = *offset++; // Removed unused variable
    
    // Parse memory node
    void *mem_node = dt_find_node(dtb, "/memory"); // Removed cast, direct use of void*
    if (!mem_node) {
        return -2; // Memory node not found
    }
    
    dt_info.mem_start = *(uint64_t*)((uint8_t*)mem_node + 2);
    dt_info.mem_size = *(uint64_t*)((uint8_t*)mem_node + 3);
    
    // Parse chosen node
    void *chosen_node = dt_find_node(dtb, "/chosen"); // Removed cast, direct use of void*
    if (chosen_node) {
        dt_info.chosen_addr = *(uint64_t*)dt_get_property(chosen_node, "stdout-path");
    }
    
    return 0;
}

void dt_dump(void) {
    uart_puts("DT Info:\n");
    uart_puts("  Memory: ");
    print_hex(dt_info.mem_start);
    uart_puts(" - ");
    print_hex(dt_info.mem_start + dt_info.mem_size);
    uart_puts("\n");
    uart_puts("  Chosen: ");
    print_hex(dt_info.chosen_addr);
    uart_puts("\n");
}

// Helper functions (simplified for example)
void *dt_find_node(void *dtb, const char *name) {
    // Implementation for finding node in DTB
    return NULL; // Placeholder
}

void *dt_get_property(void *node, const char *prop_name) {
    // Implementation for getting property value
    return NULL; // Placeholder
}
