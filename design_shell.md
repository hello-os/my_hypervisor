# 交互式命令行 Shell 模块设计文档 - my_hypervisor

本项目设计并实现了一个面向 **ARMv8 (AArch64)** 的极简交互式 Shell。该模块参考了 **Xvisor** 的命令提示系统与人机交互机制，直接运行于 Exception Level 2 (EL2)，通过 PL011 物理 UART 接口与用户实现低延迟、高可靠的命令交互。

---

## 1. 为什么创建该模块？（设计目的与需求）

在裸金属或 Hypervisor 系统开发过程中，实时系统状态监控与在线调试手段必不可少：
1. **实时硬件/内存监控**: 通过内存转储命令 `md`，可随时查看物理内存中的硬件寄存器或页表，为调试驱动提供极大便利。
2. **交互式测试与验证**: 提供在线测试命令（如 `heap`），不需要每次修改源码并重新编译，即可在线动态运行单元测试。
3. **系统状态展示**: 类似于 Xvisor 提供的 `show` 系列命令，通过 `version` 快速反馈底层控制状态，辅助确认 hypervisor 环境健康。

---

## 2. 软件模块架构与关键技术

Shell 模块通过三个核心层面实现：底层的物理串口输入输出、中间层的字符串解析及高级的内置命令分发引擎。

```text
+-------------------------------------------------------------+
|                     User Control Console                    |
+-------------------------------------------------------------+
                               |  (Keyboard input / Screen echo)
                               v
+-------------------------------------------------------------+
|                    PL011 UART Controller                    |
| - Registers: UART_DR (Data), UART_FR (Flag)                 |
| - Mechanics: RXFE Polling for Non-blocking character input  |
+-------------------------------------------------------------+
                               |
                               v
+-------------------------------------------------------------+
|                Shell Buffer & Editing Engine                |
| - Buffer size: 64 bytes                                     |
| - Support: Backspace handling (ASCII 127 / '\b')             |
| - Echo back mode for real-time terminal sync                |
+-------------------------------------------------------------+
                               |
                               v
+-------------------------------------------------------------+
|                  Command Dispatch Engine                    |
| - Tokenizer: Extracts Command and 1 Hex/String Argument     |
| - Commands: help, version, heap, md                         |
+-------------------------------------------------------------+
```

### 2.1 底层 PL011 UART 驱动的扩展 (vmm_shell.c)
在 `vmm_shell.c` 中扩展了读取 UART 输入字符的阻塞轮询（Polling）算法：
```c
static char uart_getc(void)
{
    volatile unsigned int *uart = (volatile unsigned int *)PL011_UART_BASE;
    while (uart[UART_FR / 4] & RXFE) {
        /* 当接收 FIFO 为空 (RXFE 为 1) 时自旋，直到有输入数据 */
    }
    return (char)(uart[UART_DR / 4] & 0xFF);
}
```
*   **为什么这样做**：PL011 是 ARM 标准串口控制器。它的 Flags Register (`UART_FR`) 位 4 是 `RXFE`（Receive FIFO Empty）。只有当硬件准备好至少一个字符时，软件方可读取 Data Register (`UART_DR`)，从而避免读入无效噪音。

### 2.2 命令行编辑缓存与回显 (vmm_shell.c)
```c
while (1) {
    char c = uart_getc();
    if (c == '\r' || c == '\n') {
        uart_puts("\n");
        buf[idx] = '\0';
        break;
    } else if (c == 127 || c == '\b') {
        if (idx > 0) {
            idx--;
            uart_puts("\b \b"); /* 回退字符并用空格擦除 */
        }
    } else {
        if (idx < 63) {
            buf[idx++] = c;
            uart_putc(c); /* 实时回显字符 */
        }
    }
}
```
*   **为什么这样做**：在裸金属串口终端下，如果没有回显，用户无法得知自己输入了什么。如果没有对 `Backspace`（ASCII 127 / `\b`）进行物理处理，并在屏幕上通过 `\b \b` 进行覆盖式擦除，退格操作就会变成输入乱码。

### 2.3 命令解析器与分发设计
系统通过分步解析命令行字符，将首个空白字符前的子串作为“命令”，其后的子串作为“参数”。
*   **首次适应字符串比对 (`strcmp`)**：裸金属没有标准 C 库，自行设计了高效紧凑的 `strcmp`。
*   **十六进制参数解析 (`parse_hex`)**：提取特定命令（如 `md`）传入的十六进制物理地址，支持 `0x` 前缀，并将其直接转化为 AArch64 下的 `unsigned long` 无符号长整型，便于直接访问内存。

---

## 3. 命令行交互运行流程大纲

```text
                  +--------------------------------+
                  |    System Boot & MMU Enabled   |
                  +--------------------------------+
                                  |
                                  v
                  +--------------------------------+
                  |         shell_loop()           |
                  +--------------------------------+
                                  |
                                  v
                  +--------------------------------+
                  |    Print Banner & Prompt (#)   |
                  +--------------------------------+
                                  |
                                  +<---------------+
                                  |                |
                                  v                |
                  +--------------------------------+  No (Ctrl+C/Keep Typing)
                  |    Wait & Read UART Input      |--+
                  +--------------------------------+
                                  | Yes (Enter)
                                  v
                  +--------------------------------+
                  |      Tokenize CommandLine      |
                  +--------------------------------+
                                  |
                                  v
                  +--------------------------------+
                  |     Command Route & Match      |
                  +--------------------------------+
                  |  - "help"                      |
                  |  - "version"                   |
                  |  - "heap"                      |
                  |  - "md <addr>"                 |
                  +--------------------------------+
                                  |
                                  v
                  +--------------------------------+
                  |      Execute and Output        |
                  +--------------------------------+
                                  |
                                  +----------------+
```

---

## 4. 模块验证与测试方法

1. **编译**: 运行 `make` 编译整个 hypervisor 项目。
2. **QEMU 启动**: 运行 `./run.sh` 开启带有 `-nographic` 的 QEMU AArch64 模拟器。
3. **输入指令测试**:
   * 输入 `help` 确认帮助菜单正确显示。
   * 输入 `version` 获取程序版本详情。
   * 输入 `heap` 进行堆分配压力测试。
   * 输入 `md 0x40080000` 读取启动加载处的第一个 32 位机器字，验证地址寻址能力。
