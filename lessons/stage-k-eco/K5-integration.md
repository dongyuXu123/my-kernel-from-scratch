# K5: 最终集成测试 (Day 60)

## 对照源码
- 全部 60 天功能回归

## 学习目标
1. 全功能回归: 引导 → 内存 → 进程 → FS → 网络 → GUI → SMP
2. 性能基准: 上下文切换延迟, 吞吐量测试
3. 系统能力清单: 20 syscalls, 12 drivers, 6 filesystems

## 代码导读
- 全部 mykernel/ 源文件 (60 天累积)
- 最终 mykernel.elf: ~85 个源文件, ~10,000 行代码

## 验证
```bash
cd course/day60 && make
qemu-system-x86_64 -kernel mykernel.elf -smp 4 -display none -serial stdio -no-reboot -no-shutdown
# 预期: 全部测试通过，打印系统能力清单
```
