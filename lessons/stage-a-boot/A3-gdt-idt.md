# Day 4: GDT/IDT 初始化

> 对照: reference/linux-7.1/arch/x86/kernel/idt.c, arch/x86/mm/fault.c

## 1. 目标
- **GDT/IDT**: C 代码填充 GDT(6条目)/IDT(256项×16B)，lidr 加载，int3 验证
- **异常**: #PF(打印CR2), #GP(打印'G'), #DF(打印'D')

## 2. 实现
-  — gdt_init(): 填充 TSS 描述符(GDT[5], 16B)
-  — fill_idt_entry(向量,handler,DPL), idt_init(): 注册异常+syscall向量, lidt_wrapper
-  — int3_handler/pf_handler/gp_handler/df_handler

## 3. 关键知识点
- 64位 IDT 门: gate[0-1]=offset_low, gate[2-3]=seg(0x08), gate[4]=IST, gate[5]=P|DPL|Type(0x8E)
- Page Fault: CR2=出错地址, error code(bit0=P,bit1=W/R,bit2=U/S,bit4=指令fetch)
- Double Fault(#DF): 处理异常时再次异常触发

## 4. 验收
- int3 触发→打印'3'→返回继续
- 访问 0xDEAD0000→#PF→打印CR2→返回
