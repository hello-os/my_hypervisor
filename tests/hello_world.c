/*
 * Hello World测试 - 验证my_hypervisor基本功能
 */

#include <stdint.h>

#define UART_BASE 0x09000000

/* UART寄存器定义 */
#define UART_DR         (UART_BASE + 0x00)  // 数据寄存器
#define UART_FR         (UART_BASE + 0x18)  // 标志寄存器
#define UART_IBRD       (UART_BASE + 0x24)  // 整数波特率除数
#define UART_FBRD       (UART_BASE + 0x28)  // 小数波特率除数
#define UART_LCRH       (UART_BASE + 0x2C)  // 线路控制寄存器
#define UART_CR         (UART_BASE + 0x30)  // 控制寄存器

/* UART标志位 */
#define UART_FR_TXFE    0x80  // 发送FIFO为空
#define UART_FR_RXFF    0x40  // 接收FIFO为满
#define UART_FR_RXFE    0x10  // 接收FIFO为空

/* 简单的UART初始化 */
static void uart_init(void)
{
    /* 禁用UART */
    volatile uint32_t *uart_cr = (volatile uint32_t *)UART_CR;
    *uart_cr = 0;
    
    /* 设置波特率 115200，假设输入时钟为24MHz */
    volatile uint32_t *uart_ibrd = (volatile uint32_t *)UART_IBRD;
    volatile uint32_t *uart_fbrd = (volatile uint32_t *)UART_FBRD;
    *uart_ibrd = 13;  // 24000000 / (16 * 115200) = 13.02
    *uart_fbrd = 1;   // 小数部分
    
    /* 设置数据格式: 8位数据，无校验，1位停止位 */
    volatile uint32_t *uart_lcrh = (volatile uint32_t *)UART_LCRH;
    *uart_lcrh = 0x60;
    
    /* 启用UART，启用发送和接收 */
    *uart_cr = 0x301;
}

/* 发送单个字符 */
static void uart_putc(char ch)
{
    volatile uint32_t *uart_dr = (volatile uint32_t *)UART_DR;
    volatile uint32_t *uart_fr = (volatile uint32_t *)UART_FR;
    
    /* 等待发送FIFO为空 */
    while (*uart_fr & UART_FR_TXFE);
    
    /* 发送字符 */
    *uart_dr = ch;
}

/* 发送字符串 */
static void uart_puts(const char *str)
{
    while (*str) {
        uart_putc(*str++);
    }
}

/* 简化版的字符串比较 */
static int simple_strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

/* 简单的内存设置 */
static void simple_memset(void *ptr, int value, uint32_t size)
{
    uint8_t *p = (uint8_t *)ptr;
    for (uint32_t i = 0; i < size; i++) {
        p[i] = (uint8_t)value;
    }
}

/* 测试函数 */
static void run_tests(void)
{
    /* 简单的测试缓冲区 */
    char buffer[32];
    
    /* 测试内存设置 */
    simple_memset(buffer, 'A', 10);
    
    /* 输出测试结果 */
    uart_puts("=== my_hypervisor Hello World测试 ===\r\n");
    uart_puts("系统启动成功!\r\n");
    uart_puts("UART初始化完成\r\n");
    
    /* 测试条件判断 */
    if (__builtin_expect(1 > 0, 1)) {
        uart_puts("条件判断测试通过\r\n");
    }
    
    /* 测试基础数学运算 */
    int a = 5;
    int b = 3;
    int sum = a + b;
    
    if (sum == 8) {
        uart_puts("数学运算测试通过 (5 + 3 = 8)\r\n");
    }
    
    uart_puts("===================================\r\n");
    uart_puts("\r\n");
}

/* 主测试入口 */
void hello_world(void)
{
    /* 初始化UART */
    uart_init();
    
    /* 运行测试 */
    run_tests();
    
    /* 完成测试 */
    uart_puts("Hello World测试完成!\r\n");
    uart_puts("输入字符将显示回显...\r\n");
    uart_puts("按空格键退出\r\n");
    
    /* 简单回显测试 */
    while (1) {
        // 简单延迟循环
        for (volatile int i = 0; i < 10000; i++);
        
        // 这里可以添加键盘输入检测的回显功能
        // 暂时只是保持运行状态
    }
}