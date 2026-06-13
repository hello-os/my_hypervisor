# 动态内存管理模块 (vmm_heap) 设计文档 - my_hypervisor

本项目基于 **ARMv8 (AArch64)** 裸金属环境实现了一个高效、确定性的动态内存管理堆算法 (`vmm_heap`)。该模块参考了 **Xvisor** 的内存池分区 (`vmm_heap`) 思想，旨在为 hypervisor 运行过程中的动态对象、虚拟设备描述符、页表分配等提供弹性的内存支撑。

---

## 1. 为什么创建该模块？（设计目的与需求）

在没有宿主操作系统和标准 C 库的环境下，硬编码静态全局变量会降低系统的弹性和可扩展性：
1. **不定长配置的存储**: 如解析设备树时可能出现任意数量和长度的节点属性，无法使用静态数组承载。
2. **虚拟设备（vDevice）生命周期管理**: 虚拟设备（如虚拟串口、VirtIO 模拟设备）在 VM 创建和销毁时需要动态分配或回收内存。
3. **避免物理内存浪费**: 物理页分配器（Page Allocator）的最小粒度为 4KB。而很多控制结构体只有几十字节，直接分配 4KB 会导致严重的内部碎片。

---

## 2. 内存堆算法设计与关键技术

本项目采用基于 **隐式空闲链表 (Implicit Free List)** 与 **首次适应算法 (First-Fit Algorithm)** 的堆内存管理结构。

```text
+-----------------------------------------------------------------------------------+
|                                 vmm_heap Memory Pool                              |
+-----------------------------------------------------------------------------------+
| [block 1 header] | [allocated payload] | [block 2 header] |     [free payload]    |
| - size: 128      |                     | - size: 4096     |                       |
| - free: 0        |                     | - free: 1        |                       |
+-----------------------------------------------------------------------------------+
```

### 2.1 堆块描述头部 (`struct mem_block`)
每一个在堆中分配或空闲的空间都会被冠以一个管理头部：
```c
struct mem_block {
    unsigned long size; /* 块的大小，包含头部自身 */
    int free;           /* 0 表示已分配，1 表示当前空闲 */
    struct mem_block *next; /* 链表指针，指向内存地址上紧邻的下一个块 */
};
```
*   **为什么要这样做**：在释放内存时，仅仅传入一个指针地址（Payload 物理地址），若没有管理头部，分配器将无法感知这块内存的大小及相邻块的状态。通过地址向前偏移 `sizeof(struct mem_block)` 即可完美获得块信息。

### 2.2 字节对齐机制 (8-byte Alignment)
```c
size = (size + 7) & ~7;
```
*   **为什么要这样做**：AArch64 架构要求 64 位数据（如指针、`unsigned long`）的内存访问地址必须是 8 字节对齐的。如果非对齐访问，可能会触发 Alignment Fault 硬件异常并导致系统崩溃。

### 2.3 首次适应算法与块分裂 (Block Splitting)
```c
if (curr->free && curr->size >= total_size) {
    if (curr->size > total_size + sizeof(struct mem_block) + 8) {
        /* 如果剩余空间足够容纳一个新块，则进行分裂 */
        struct mem_block *next_block = (struct mem_block *)((char *)curr + total_size);
        next_block->size = curr->size - total_size;
        next_block->free = 1;
        next_block->next = curr->next;
        
        curr->size = total_size;
        curr->next = next_block;
    }
    curr->free = 0;
    return (void *)((char *)curr + sizeof(struct mem_block));
}
```
*   **为什么要这样做**：如果直接将一个很大的空闲块分配给小请求，会造成严重的外部碎片。通过分裂（Splitting）技术，将多余空间切割并重新标为 Free 放回链表。

### 2.4 内存释放与相邻块合并 (Coalescing)
为了解决碎片化问题，在 `vmm_free` 后，分配器会扫描整个堆链表，自动将相邻的、连续的空闲内存块物理合并：
```c
struct mem_block *curr = heap_start;
while (curr && curr->next) {
    if (curr->free && curr->next->free) {
        curr->size += curr->next->size;
        curr->next = curr->next->next;
    } else {
        curr = curr->next;
    }
}
```

---

## 3. 动态分配生命周期

```text
           [vmm_malloc(req_size)]
                     |
                     v
             [对齐到 8 字节并加上头部]
                     |
                     v
           [从堆首部顺序遍历链表]
                     |
         +-----------+-----------+
         |                       |
         v                       v
    [未找到合适大小块]       [找到足够大的空闲块]
         |                       |
         v                       v
    [返回 NULL]          [判断是否需要分裂 (Splitting)]
                             |
                   +---------+---------+
                   v                   v
                [需要分裂]          [不需要分裂]
                   |                   |
            [创建新空闲块]         [标记当前块 free=0]
                   \                   /
                    \                 /
                  [返回 Payload 指针地址]
```

---

## 4. 模块验证

1. **配置堆位置**: 堆位于 `0x41000000` 处，大小为 1MB。
2. **测试机制**:
   * 调用 `vmm_malloc(100)` 分配空间，获得首地址 A。
   * 调用 `vmm_malloc(200)` 分配空间，获得首地址 B。
   * 调用 `vmm_free(A)` 释放首空间。
   * 再次调用 `vmm_malloc(50)`，检验它是否能在地址 A 处完美重用旧内存块，同时链表头部不发生越界。
   * 上机输出完美的通过验证。
