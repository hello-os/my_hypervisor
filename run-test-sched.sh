#!/bin/bash
# run-test-sched.sh - 调度器功能测试脚本

# 编译代码
make clean && make

# 使用QEMU运行，并利用串口输出验证
# 通过 -nographic 看到输出
qemu-system-aarch64 -M virt -cpu cortex-a53 -m 128 -nographic -kernel my_hypervisor.bin
