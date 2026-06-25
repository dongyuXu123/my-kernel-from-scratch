# Day 7: Buddy System

> 对照: reference/linux-7.1/mm/page_alloc.c

## 1. 目标与验收
- **B1**: 基于位图的物理页分配器, alloc_page返回物理地址
- **B2**: 11个order(0-10), 分裂/合并算法, alloc_pages(order)
- **B3**: 单页64对象slab, kmalloc/kfree链表管理
- **B4**: PT[512]填充→PM4/PDP U/S→PD[0]切为PT指针→CR3刷新
- **B5**: 动态添加2MB大页到任意PDP索引, 供MMIO映射

## 2. 关键实现
-  — pmm_init标记保留页, alloc_page线性扫描, free_page清bit
-  — buddy_init挂MAX_ORDER块, alloc_pages分裂, free_pages合并
-  — slab_init从buddy取1页, 链64个对象, kmalloc取头, kfree归还
-  — setup_pagetables(): PT[i]=phys|0x07, pd[0]=pt|0x07, write_cr3
-  — map_mmio(): pdp[idx]=pd|0x03, pd[idx]=phys|0x83

## 3. 堆边界修复
**问题**: heap_start 定义在 head.S 的 BSS 末尾，但其他 .c 的 BSS 变量链接在其后
**修复**: linker.ld 添加 ，pmm_init 使用 _kernel_end 作为真正堆起点

## 4. 验收
- alloc_page→0x10D000, buddy alloc→0x10D000, kmalloc→0x10E000/0x10E040
- #PF test→打印 CR2=0xDEAD0000→返回
- map_mmio: phys=FEB80000 pdp=3 pd=1F5
