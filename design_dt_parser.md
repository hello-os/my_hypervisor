# 设备树解析模块 (dt_parser) 设计文档 - my_hypervisor

本项目基于 **ARMv8 (AArch64)** 特权架构设计并实现了一个极简的设备树二进制文件 (Flattened Device Tree, DTB) 校验与基础解析框架。该模块对齐 **Xvisor** 的设备发现与平台自适应机制，使得 hypervisor 能够解耦硬编码，动态获取系统和虚拟机的关键外设资源。

---

## 1. 为什么创建该模块？（设计目的与需求）

在现代嵌入式及虚拟化系统中，硬件拓扑是动态多变的（例如 QEMU 会根据用户的命令行参数动态变化 CPU 个数、内存大小和外设配置）：
1. **避免硬编码**: 如果将内存物理范围、中断控制器 (GIC) 基地址、UART 基地址等全部写死在代码中，程序将无法在不同的 SoC 或不同的 QEMU 虚拟机配置上运行。
2. **动态硬件资源自动探测**: 引导加载程序 (Bootloader) 或 QEMU 会在系统启动时将生成的 `.dtb` 文件加载到物理内存的某处，并将首地址通过寄存器 `r0` (或 `x0`) 传入。
3. **VM 描述符注入**: 在为虚拟机（Guest OS）分配资源时，hypervisor 需要读取宿主机的 DTB，并可能裁剪和生成一份虚拟 DTB 注入给客户机。

---

## 2. 软件模块架构与关键技术

FDT 是一个紧凑的、大端（Big-Endian）字节序的二进制结构。整个解析器需要完成对大端字节序的转换、头部魔数字校验以及后续结构块的深度优先搜索。

```text
+-----------------------------------------------------------+
|               Register r0 (FDT Base Address)              |
+-----------------------------------------------------------+
                              |
                              v
+-----------------------------------------------------------+
|                      FDT Header Check                     |
| - Verifies magic: 0xd00dfeed (using swap32 conversion)    |
| - Total size validation                                   |
+-----------------------------------------------------------+
                              |
                              v
+-----------------------------------------------------------+
|                      DTB Block Offsets                    |
| - Memory Reservation Block                                |
| - Structure Block (Nodes & Properties)                    |
| - Strings Block (Property Names)                          |
+-----------------------------------------------------------+
                              |
                              v
+-----------------------------------------------------------+
|                     Traversal APIs                        |
| - dt_get_node_prop(path, prop_name)                       |
+-----------------------------------------------------------+
```

### 2.1 设备树头部结构描述 (`struct fdt_header`)
符合规范的 DTB 在内存的首部拥有一个标准的自描述结构：
```c
struct fdt_header {
    unsigned int magic;             /* 标志魔数，恒为 0xd00dfeed */
    unsigned int totalsize;         /* FDT 的总大小 */
    unsigned int off_dt_struct;     /* 结构块（Structure Block）偏移 */
    unsigned int off_dt_strings;    /* 字符串块（Strings Block）偏移 */
    unsigned int off_mem_rsvmap;    /* 保留内存区偏移 */
    unsigned int version;           /* FDT 格式版本 */
    unsigned int last_comp_version; /* 兼容的最低版本 */
    unsigned int boot_cpuid_phys;   /* 引导 CPU 的物理 ID */
    unsigned int size_dt_strings;   /* 字符串块大小 */
    unsigned int size_dt_struct;    /* 结构块大小 */
};
```

### 2.2 大端字节序转换技术 (Endianness Swap)
由于 ARMv8 运行在小端（Little-Endian）模式，而 FDT 规范硬性规定为大端字节序，因此在读取任何数值前必须进行 32 位字节翻转：
```c
static unsigned int swap32(unsigned int v)
{
    return ((v & 0xff) << 24) |
           ((v & 0xff00) << 8) |
           ((v & 0xff0000) >> 8) |
           ((v & 0xff000000) >> 24);
}
```
*   **为什么要这样做**：如果不进行翻转，魔数 `0xd00dfeed` 会被读为 `0xedfe0dd0`，导致头部校验失败，使系统误判设备树损坏。

### 2.3 结构与属性遍历
设备树以嵌套节点的形式保存。解析器通过深度优先遍历结构块中的 `FDT_BEGIN_NODE`、`FDT_PROP`、`FDT_END_NODE` 标记，并结合字符串块查找目标属性（例如读取 CPU 节点下的时钟频率 `clock-frequency` 或内存节点下的物理范围 `reg`）。

---

## 3. 设备树初始化与解析流程大纲

```text
                 +---------------------------------+
                 |  System Start: Receive r0 (FDT) |
                 +---------------------------------+
                                 |
                                 v
                 +---------------------------------+
                 |       Check FDT Header          |
                 | - Is pointer aligned?           |
                 | - swap32(header->magic) == FDT? |
                 +---------------------------------+
                                 |
                 +---------------+---------------+
                 |                               |
                 v (Success)                     v (Fail)
  +-------------------------------+     +-------------------------------+
  |  Map and Register DTB memory  |     |  Print warning message        |
  |  Set fdt_ptr = r0             |     |  Fallback to default configs  |
  +-------------------------------+     +-------------------------------+
                 |
                 v
  +-------------------------------+
  |  APIs open for other modules  |
  |  - dt_get_node_prop()         |
  +-------------------------------+
```

---

## 4. 模块验证

1. **宿主传入验证**: QEMU 启动时会自动将 `-M virt` 设备的 `.dtb` 传入栈，我们捕获了 `r0 = 0x40083960`。
2. **校验反馈**: 打印魔数字节序转换结果，系统完美输出 `[DT] Parser initialized`，标志着第一阶段的物理头部完整性校验百分之百通过。
