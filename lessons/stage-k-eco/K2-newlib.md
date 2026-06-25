# K2: 用户态 C 程序 (Day 57)

## 对照源码
- newlib (嵌入式 C 库), Linux syscall ABI

## 学习目标
1. C 运行时: crt0.S (_start → call main → exit)
2. syscall 包装: write/read/open/close/fork/exit/getpid
3. 交叉编译: x86_64-elf-gcc -static → 自研内核加载

## 代码导读
- `initfs/crt0.S`: _start 入口 + main 调用 + exit
- `initfs/syscalls.c`: write, read, open, close, fork, _exit, getpid

## 验证
```bash
cd course/day57 && make
# 交叉编译测试: x86_64-elf-gcc -static hello.c -o hello
```
