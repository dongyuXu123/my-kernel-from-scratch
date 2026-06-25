# Day 19: exec系统调用

> 对照: reference/linux-7.1/fs/exec.c

## 1. 目标
- **D1**: 多文件C+ASM混合编译, ramfs 16文件数组设计
- **D2**: create(名称)→write(fd,data,len)→read(fd,buf,len)→list()
- **D3**: 解析ELF64 header(e_magic/e_type/e_machine), 复制PT_LOAD段
- **D4**: .do_exec: 读ramfs文件→elf_load_mem→修改iretq帧RIP→新程序运行
- **D5**: 用户态汇编Shell: readline→parse→dispatch(help/echo/ls/cat/exec/insmod)

## 2. 关键实现
-  — file_table[16], ramfs_create/write/read/list, ramfs_lookup/fd_read/fd_write
-  — elf_load_mem(): 验证magic/class/type/machine→遍历phdr→复制PT_LOAD→清BSS
-  — do_sys_exec(): ramfs_read_file→elf_load_mem→返回entry
-  — puts/readline/strcmp辅助函数, 命令分派跳转表

## 3. ELF 段地址限制
- **问题**: 编译在 0x400000 的 ELF 无法执行
- **根因**: 页表只映射 0-4MB, 0x400000 超出 PM4[1]范围
- **修复**: 编译时 , 确保在映射范围内

## 4. 验收
- _TEMPLATE.md
stage-a-boot
stage-b-memory
stage-c-process
stage-d-fs
stage-e-net→列出 hello.txt readme.txt hello_mod.o
- →打印 "Hello from /bin/hello!"
- →打印 "Hello from asm module!"
