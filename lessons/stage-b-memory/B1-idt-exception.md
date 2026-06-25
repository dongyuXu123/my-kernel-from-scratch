# B1 GDT/IDT/异常向量

## 1. 目标与验收

- **本节目标**: 设置 IDT 表,注册 `int $3` 中断处理函数,触发中断后 handler 通过串口打印消息并 `iretq` 正确返回
- **验收标准**: QEMU 串口依次输出 `Hello from my kernel` → `INT3 caught!` → `Returned from INT3` 三行

## 2. 真实源码导读

- `reference/linux-7.1/arch/x86/kernel/idt.c` — 真实内核的 IDT 初始化
  - `struct gate_struct`(desc_defs.h): 64 位 IDT 门描述符,每项 16 字节
  - `G()` 宏: 生成 `idt_data` 结构(vector + bits + addr + segment)
  - `INTG()` 宏: 中断门(type=0xE, dpl=0)
  - `idt_setup_early_traps()`: 设置早期陷阱(DB, BP),加载 IDT
  - `load_idt(&idt_descr)`: 执行 `lidt` 指令
- `reference/linux-7.1/arch/x86/include/asm/desc_defs.h` — 门描述符定义
  - `gate_struct`: `{u16 offset_low; u16 segment; idt_bits bits; u16 offset_middle; u32 offset_high; u32 reserved;}`

**关键知识点(64 位 IDT 门格式):**

| 偏移 | 大小 | 字段 | 说明 |
|---|---|---|---|
| 0 | 2 | offset_low | handler 地址的 bit[15:0] |
| 2 | 2 | segment | CS 选择子(内核=0x08) |
| 4 | 2 | bits | p(1) \| dpl(2) \| 0(1) \| type(4) \| 0(5) \| ist(3) |
| 6 | 2 | offset_mid | handler 地址的 bit[31:16] |
| 8 | 4 | offset_high | handler 地址的 bit[63:32] |
| 12 | 4 | reserved | 必须为 0 |

## 3. 我的实现(mykernel)

### 关键代码

- `arch/x86/entry.S`:
  - `setup_idt`: 计算 int3_handler 地址,拆分为 low/mid/high 填入 `idt[3]`
  - `idt`: 256 项 × 16 字节 IDT 表(数据段,初始化全零)
  - `idt_ptr`: 10 字节 `{limit=4095; base=&idt}` 供 `lidt` 指令用
  - `int3_handler`: 压栈保存 %rdi/%rax → `call serial_puts("INT3 caught!")` → 恢复 → `iretq`
  - `start64` 流程: Hello 输出 → `call setup_idt` → `lidt idt_ptr` → `int3` → 返回后打印 resume 消息

### 运行验证 [2026-06-23]

**命令**: `make && qemu-system-x86_64 -kernel mykernel.elf -serial file:log`
**输出**:
```
Hello from my kernel
INT3 caught!
Returned from INT3
```
**验收**: ✅ 通过。三行消息证明 IDT 设置、中断触发、handler 执行、iretq 返回全部正确

### gdb 验证

```
断点 setup_idt: RIP=0x1000d1, 填入 IDT[3].bits=0x8E00
断点 int3_handler: RIP=0x10010e, RSP=0x109fd8
  栈: [RIP]=0x10014c [CS]=0x8 [RFLAGS]=0x2
  指令流: push → lea msg → call serial_puts → pop → iretq
iretq 后: 继续执行 start64, 打印 "Returned from INT3"
```

## 4. 答疑沉淀

(暂无)

## 5. 纠错日志

### [2026-06-23] .quad idt 在 ELF32 中的重定位问题

- **现象**: 链接时 `.quad idt` 可能触发 64 位重定位在 ELF32 中不支持的错误
- **根因**: ELF32 链接器不支持 64 位绝对重定位(R_386_64),`.quad symbol` 需要 8 字节绝对地址
- **对照**: 本内核地址空间全部在低 4GB 内,IDT 基地址同理
- **修复**: 改用 `.long idt; .long 0` 拆分 base[31:0] 和 base[63:32],与 `lidt` 的 10 字节操作数要求一致
- **改进方法**: ELF32 混合 64 位代码时,绝对地址一律拆分为高低 32 位,或使用 RIP-relative 寻址
