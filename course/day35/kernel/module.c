/*
 * kernel/module.c — 内核模块加载器
 *
 * 对照: reference/linux-7.1/kernel/module/main.c (load_module)
 *
 * 简化: 单缓冲区加载, R_X86_64_64 + R_X86_64_PC32
 */

#include "kernel.h"
#include "module.h"

/* ELF 类型 */
typedef unsigned long long u64;
typedef unsigned int   u32;
typedef unsigned short u16;

struct elf64_hdr {
    unsigned char e_ident[16]; u16 e_type, e_machine;
    u32 e_version; u64 e_entry, e_phoff, e_shoff;
    u32 e_flags; u16 e_ehsize, e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx;
};

struct elf64_shdr {
    u32 sh_name; u32 sh_type; u64 sh_flags;
    u64 sh_addr, sh_offset, sh_size;
    u32 sh_link, sh_info;
    u64 sh_addralign, sh_entsize;
};

struct elf64_sym {
    u32 st_name; unsigned char st_info, st_other; u16 st_shndx;
    u64 st_value, st_size;
};

struct elf64_rela {
    u64 r_offset, r_info, r_addend;
};

#define ELFMAG     "\177ELF"
#define ET_REL     1
#define EM_X86_64  62
#define SHT_SYMTAB 2
#define SHT_STRTAB 3
#define SHT_RELA   4
#define SHT_NOBITS 8
#define SHF_ALLOC  2
#define SHF_WRITE  1
#define SHF_EXECINSTR 4
#define SHN_UNDEF  0
#define SHN_ABS    0xFFF1

#define R_X86_64_64   1
#define R_X86_64_PC32 2
#define R_X86_64_PLT32 4
#define R_X86_64_32S  11
#define ELF64_R_SYM(i) ((i) >> 32)
#define ELF64_R_TYPE(i) ((i) & 0xFFFFFFFF)

/* ================================================================
 * 内核符号表
 * ================================================================ */
#define MAX_KSYMS 32
static struct { const char *name; void *addr; } ksymtab[MAX_KSYMS];
static int nr_ksyms;

void register_kernel_symbol(const char *name, void *addr)
{
    if (nr_ksyms < MAX_KSYMS) {
        ksymtab[nr_ksyms].name = name;
        ksymtab[nr_ksyms].addr = addr;
        nr_ksyms++;
    }
}

/* 简单 strcmp */
static int strcmp2(const char *a, const char *b)
{
    while (*a && *a == *b) { a++; b++; }
    return *a - *b;
}

void *lookup_symbol(const char *name)
{
    for (int i = 0; i < nr_ksyms; i++)
        if (strcmp2(ksymtab[i].name, name) == 0)
            return ksymtab[i].addr;
    return 0;
}

/* ================================================================
 * sys_insmod — 加载模块(从 ramfs 读取 .o 文件)
 * ================================================================ */
int sys_insmod(const char *name)
{
    extern int ramfs_read_file(const char *, char *, int);

    /* 读模块文件 */
    static char mod_buf[65536];
    int len = ramfs_read_file(name, mod_buf, sizeof(mod_buf));
    if (len <= 0) { serial_puts("insmod: file not found\r\n"); return -1; }

    struct elf64_hdr *ehdr = (struct elf64_hdr *)mod_buf;
    if (ehdr->e_ident[0] != 0x7F || ehdr->e_ident[1] != 'E' ||
        ehdr->e_type != ET_REL || ehdr->e_machine != EM_X86_64) {
        serial_puts("insmod: bad ELF\r\n"); return -1;
    }

    serial_puts("insmod: loading "); serial_puts(name);
    serial_puts(" ("); print_hex64(len);
    serial_puts(" bytes)\r\n");

    /* 解析 section headers */
    u16 shnum = ehdr->e_shnum;
    struct elf64_shdr *shdrs = (struct elf64_shdr *)(mod_buf + ehdr->e_shoff);
    char *shstr = (char *)(mod_buf + shdrs[ehdr->e_shstrndx].sh_offset);

    /* 找 .symtab, .strtab */
    struct elf64_shdr *symtab = 0, *strtab = 0;
    for (int i = 0; i < shnum; i++) {
        if (shdrs[i].sh_type == SHT_SYMTAB) symtab = &shdrs[i];
        if (shdrs[i].sh_type == SHT_STRTAB && i != ehdr->e_shstrndx) strtab = &shdrs[i];
    }
    if (!symtab || !strtab) { serial_puts("insmod: no symtab\r\n"); return -1; }

    /* 为 SHF_ALLOC 段分配内存 */
    for (int i = 0; i < shnum; i++) {
        if (!(shdrs[i].sh_flags & SHF_ALLOC)) continue;
        if (shdrs[i].sh_addr) continue;  /* 已分配 */

        /* 分配内存(用 kmalloc 不够大, 用 alloc_pages) */
        u64 size = shdrs[i].sh_size;
        if (size == 0) continue;
        extern void *alloc_pages(int order);
        int order = 0;
        u64 req = size;
        while ((1UL << (order + 12)) < req) order++;
        void *mem = alloc_pages(order);
        if (!mem) { serial_puts("insmod: OOM\r\n"); return -1; }
        shdrs[i].sh_addr = (u64)mem;
    }

    /* 复制段数据 / 清零 BSS */
    for (int i = 0; i < shnum; i++) {
        if (!(shdrs[i].sh_flags & SHF_ALLOC)) continue;
        if (shdrs[i].sh_type == SHT_NOBITS) {
            /* BSS — 清零 */
            for (u64 j = 0; j < shdrs[i].sh_size; j++)
                ((char *)(u64)shdrs[i].sh_addr)[j] = 0;
        } else if (shdrs[i].sh_type != 0 && shdrs[i].sh_offset) {
            /* 复制数据 */
            char *src = mod_buf + shdrs[i].sh_offset;
            char *dst = (char *)(u64)shdrs[i].sh_addr;
            for (u64 j = 0; j < shdrs[i].sh_size; j++)
                dst[j] = src[j];
        }
    }

    /* 符号解析 */
    struct elf64_sym *syms = (struct elf64_sym *)(mod_buf + symtab->sh_offset);
    int nsyms = symtab->sh_size / sizeof(struct elf64_sym);
    char *symstr = (char *)(mod_buf + strtab->sh_offset);

    for (int i = 0; i < nsyms; i++) {
        const char *sname = symstr + syms[i].st_name;
        if (sname[0] == '\0') continue;  /* 跳过无名符号 */
        if (syms[i].st_shndx == SHN_UNDEF) {
            void *addr = lookup_symbol(sname);
            if (!addr) {
                serial_puts("insmod: undefined: ");
                serial_puts(sname); serial_puts("\r\n");
                return -1;
            }
            syms[i].st_value = (u64)addr;
        } else if (syms[i].st_shndx < shnum && syms[i].st_shndx > 0) {
            /* 本地符号, 加上段基址 */
            syms[i].st_value += shdrs[syms[i].st_shndx].sh_addr;
        }
    }

    /* 重定位 */
    for (int i = 0; i < shnum; i++) {
        if (shdrs[i].sh_type != SHT_RELA) continue;
        int target = shdrs[i].sh_info;  /* 被重定位的段 */
        u64 sec_base = shdrs[target].sh_addr;

        struct elf64_rela *relas = (struct elf64_rela *)(mod_buf + shdrs[i].sh_offset);
        int nrela = shdrs[i].sh_size / sizeof(struct elf64_rela);

        for (int j = 0; j < nrela; j++) {
            u64 offset = relas[j].r_offset;
            int symidx = (int)ELF64_R_SYM(relas[j].r_info);
            int type   = (int)ELF64_R_TYPE(relas[j].r_info);
            u64 addend = relas[j].r_addend;
            u64 S = syms[symidx].st_value;
            u64 P = sec_base + offset;
            u64 val = S + addend;

            switch (type) {
            case R_X86_64_64:
                *(u64 *)(u64)(sec_base + offset) = val;
                break;
            case R_X86_64_PC32:
            case R_X86_64_PLT32:
                val -= P;
                *(u32 *)(u64)(sec_base + offset) = (u32)val;
                break;
            case R_X86_64_32S:
                *(u32 *)(u64)(sec_base + offset) = (u32)val;
                break;
            default:
                serial_puts("insmod: unknown reloc type ");
                print_hex64(type); serial_puts("\r\n");
                break;
            }
        }
    }

    /* 调用 init_module */
    void *init_addr = lookup_symbol("init_module");
    if (!init_addr) {
        /* 查找名为 init_module 的符号 */
        for (int i = 0; i < nsyms; i++) {
            const char *sname = symstr + syms[i].st_name;
            if (strcmp2(sname, "init_module") == 0) {
                init_addr = (void *)(u64)syms[i].st_value;
                break;
            }
        }
    }
    if (!init_addr) { serial_puts("insmod: no init_module\r\n"); return -1; }

    serial_puts("insmod: calling init_module...\r\n");
    int (*init)(void) = (int (*)(void))init_addr;
    int ret = init();
    serial_puts("insmod: init returned "); print_hex64(ret); serial_puts("\r\n");
    return ret;
}
