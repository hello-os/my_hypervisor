# 虚拟机第二阶段内存翻译 (Stage 2 Translation) 设计文档 - my_hypervisor

本项目针对 **ARMv8 (AArch64)** 的特权虚拟化控制环境，设计并实现了虚拟机第二阶段内存翻译 (Stage 2 MMU 映射)。该模块是实现虚拟机内存隔离与独立寻址的关键，对齐 **Xvisor** 的 Stage 2 地址翻译与客户机物理地址空间 (IPA to PA) 转换机制。

---

## 1. 为什么创建该模块？（设计目的与需求）

在单阶段 MMU 下，程序执行将虚拟地址 (VA) 直接翻译为物理地址 (PA)。但在多虚拟机（Multi-VM）场景中：
1. **内存空间冲突**: 每个 Guest OS 都希望自己运行在物理基地址（如 `0x40000000`）处，且都认为自己独占了整个物理内存。如果不进行二次翻译，多个客户机将会发生严重物理地址重合崩溃。
2. **安全隔离屏障**: 客户机代码直接运行在物理硬件上，如果不设防，Guest A 可以任意读写 Guest B 甚至 Hypervisor 本身的内存空间。
3. **中间物理地址 (IPA) 的引入**: 引入第二阶段翻译后，异常级别 EL1/0 的内存访问由二级 MMU 拦截：
   * Stage 1: **虚拟地址 (VA)** -> **中间物理地址 (Intermediate Physical Address, IPA)**
   * Stage 2: **中间物理地址 (IPA)** -> **物理地址 (PA)**

---

## 2. 软件模块架构与控制寄存器设计

Stage 2 页表机制与 Stage 1 类似，同样支持 4 级转换。但其核心配置寄存器与控制流运行在 Exception Level 2 (EL2)：

```text
+-----------------------------------------------------------+
|               Guest OS (VA) [EL1/0 Access]                |
+-----------------------------------------------------------+
                              |
                              | (Stage 1 MMU translation)
                              v
+-----------------------------------------------------------+
|         Intermediate Physical Address (IPA Space)         |
+-----------------------------------------------------------+
                              |
                              | (Stage 2 MMU hardware interception)
                              v
+-----------------------------------------------------------+
|              VTTBR_EL2 (Stage 2 Page Table Root)          |
|              VTCR_EL2 (Stage 2 Translation Control)       |
+-----------------------------------------------------------+
                              |
                              | (Stage 2 translation completed)
                              v
+-----------------------------------------------------------+
|              Actual Physical Address (PA Space)           |
+-----------------------------------------------------------+
```

### 2.1 关键控制寄存器
1. **`VTTBR_EL2` (Virtualization Translation Table Base Register)**:
   - 保存 Stage 2 的根页表物理指针（即二级页表首地址）。
   - 低 16 位可设定 `VMID`（Virtual Machine Identifier），用于缓存 TLB 时标记不同的虚拟机标识，避免频繁刷新 TLB 导致性能下降。
2. **`VTCR_EL2` (Virtualization Translation Control Register)**:
   - `T0SZ`: 设定 IPA 的地址范围大小（如 40 位寻址）。
   - `TG0`: 指定 Stage 2 页表颗粒大小，常配为 4KB。
   - `SH0 / ORGN0 / IRGN0`: 设置内存共享属性与缓存回写机制。
   - `SL0`: 设置起始页表的查找级别（通常设为 1，代表从 L1 页表开始，或 0 代表 L0 开始）。

### 2.2 二级页表描述符属性 (Stage 2 Descriptors)
二级页表与一级页表的描述符格式（PTE）略有不同：
- **`MEMATTR` (Bits[5:2])**: 设定 Guest 内存访问的硬件属性（如将某些设备寄存器空间设置为 Device-nGnRnE，普通 RAM 区设为 Normal Memory Cacheable）。
- **`AP` (Bits[7:6])**: 读写权限控制，可将其配置为只读、只写或不可访问，用于实现写时复制 (COW) 或监控 Guest。

---

## 3. 内存隔离与翻译流程

```text
                  +--------------------------------+
                  |     Guest OS Accesses memory   |
                  +--------------------------------+
                                  |
                                  v
                  +--------------------------------+
                  |  Check TLB (Stage 1 & Stage 2) |
                  +--------------------------------+
                                  |
            +---------------------+---------------------+
            |                                           |
            v (TLB Hit)                                 v (TLB Miss)
  +---------------------------+               +---------------------------+
  |  Direct Physical Access  |               |  Walk Stage 1 Page Table  |
  +---------------------------+               |  (Returns IPA address)    |
                                              +---------------------------+
                                                            |
                                                            v
                                              +---------------------------+
                                              |  Walk Stage 2 Page Table  |
                                              |  - VTTBR_EL2 points to L0 |
                                              |  - Resolve IPA -> PA      |
                                              +---------------------------+
                                                            |
                                                            v
                                              +---------------------------+
                                              |     Fill TLB & Access RAM |
                                              +---------------------------+
```

---

## 4. 验证方案

1. **页表创建验证**: 在 hypervisor 堆中分配单独的 L0 二级页表，为虚拟机分配 `0x40000000` 开始的 16MB 连续物理区间。
2. **属性校验**: 通过读取 `HCR_EL2.VM` 确认二级 MMU 控制开关联动就绪。
