/*
 * boot.S - mykernel 32 位入口 + 长模式切换
 *
 * 对照: reference/linux-7.1/arch/x86/boot/compressed/head_64.S (startup_32)
 *
 * 职责(汇编必须):
 *   1. multiboot1 头 + PVH ELF note (QEMU -kernel 需要)
 *   2. 建临时 2MB 大页身份映射(前 4MB)
 *   3. 开长模式(EFER.LME, CR4.PAE, CR0.PG)
 *   4. ljmp 到 64 位代码(start64)
 */

/* ============================
 * 1. multiboot1 头(32 位入口)
 * ============================ */
.section .multiboot
.align 4
#define MB_MAGIC    0x1BADB002
#define MB_FLAGS    0x00000003   /* bit0=页对齐, bit1=内存信息 */
#define MB_CHECKSUM -(MB_MAGIC + MB_FLAGS)

multiboot_header:
    .long MB_MAGIC
    .long MB_FLAGS
    .long MB_CHECKSUM
    .long multiboot_header          /* header_addr */
    .long multiboot_header          /* load_addr */
    .long 0                         /* load_end_addr = 整段 */
    .long 0                         /* bss_end_addr = 不清 bss */
    .long _start                    /* entry_addr */

/* ============================
 * 1b. PVH ELF note(QEMU 10.2 需要)
 * ============================ */
.section .note.Xen, "a", @note
.align 4
    .long 2f - 1f          /* namesz = sizeof("Xen") = 4 */
    .long 3f - 2f          /* descsz = 4 (32-bit entry addr) */
    .long 18               /* type = XEN_ELFNOTE_PHYS32_ENTRY */
1:  .asciz "Xen"           /* name */
2:  .align 4
3:  .long _start            /* desc: 32-bit entry 地址 */

/* ============================
 * 2. 32 位入口:建页表 + 开长模式
 * ============================ */
.section .text
.code32
.global _start
.type _start, @function
_start:
    /*
     * 来到此处时:
     *   - CPU 在 32 位保护模式(CR0.PE=1, CR0.PG=0)
     *   - 身份映射(物理地址 = 线性地址)
     *   - multiboot magic 在 eax, info 指针在 ebx
     */

    /* ---- 2.1 建最小页表(身份映射前 4MB, 2MB 大页) ---- */
    /*
     *   PML4 ──→ PDP ──→ PD (2MB huge pages)
     *   只建一个 PML4 entry、一个 PDP entry,
     *   两个 PD entry(各自映射 2MB,共前 4MB 身份映射)。
     */

    /* PML4[0] = PDP 的物理地址 | 0x03 (present + writable) */
    movl    $pdp, %eax
    orl     $0x03, %eax
    movl    %eax, pml4

    /* PDP[0] = PD 的物理地址 | 0x03 */
    movl    $pd, %eax
    orl     $0x03, %eax
    movl    %eax, pdp

    /* PD[0] = 0x00000000 | 0x83 (2MB huge page, present + writable) */
    movl    $0x00000083, pd
    /* PD[1] = 0x00200000 | 0x83 (映射 2MB~4MB) */
    movl    $0x00200083, pd + 8

    /* ---- 2.2 加载 CR3 ---- */
    movl    $pml4, %eax
    movl    %eax, %cr3

    /* ---- 2.3 EFER.LME = 1 + SCE = 1(开长模式 + syscall使能) ---- */
    movl    $0xC0000080, %ecx           /* EFER MSR */
    rdmsr
    orl     $((1 << 8) | 1), %eax       /* EFER.LME=1 | EFER.SCE=1 */
    wrmsr

    /* ---- 2.4 CR4.PAE = 1(物理地址扩展, 长模式必需) ---- */
    movl    %cr4, %eax
    orl     $(1 << 5), %eax             /* CR4.PAE = 1 */
    movl    %eax, %cr4

    /* ---- 2.5 加载 GDT ---- */
    lgdt    gdt_ptr

    /* ---- 2.6 CR0.PG = 1(开分页, 同时也是开长模式) ---- */
    movl    %cr0, %eax
    orl     $(1 << 31), %eax            /* CR0.PG = 1 */
    movl    %eax, %cr0

    /*
     * 此时 CPU 进入 32 位兼容模式(长模式的过渡态):
     * EFER.LMA=1, CS.L=0(CS 仍是旧的 32 位段)。
     * 下面用 ljmp 加载 64 位 CS 即可进入完全 64bit 模式。
     */

    /* ---- 2.7 长跳转进入 64 位模式 ---- */
    ljmp    $0x08, $start64             /* 0x08 = 64-bit CS selector */
