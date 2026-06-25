/*
 * fs/elf.c — 最小 ELF 加载器(x86-64, 静态链接)
 *
 * 对照: reference/linux-7.1/fs/binfmt_elf.c (load_elf_binary)
 *
 * 简化: 只支持 ET_EXEC(静态), 对每个 PT_LOAD 直接复制段到目标地址
 */

#include "kernel.h"

/* -------- ELF 类型(uapi/linux/elf.h) -------- */
#define ELFMAG0    0x7F
#define ELFMAG1    'E'
#define ELFMAG2    'L'
#define ELFMAG3    'F'
#define ELFCLASS64 2
#define ET_EXEC    2
#define EM_X86_64  62
#define PT_LOAD    1

typedef unsigned long long Elf64_Addr;
typedef unsigned long long Elf64_Off;
typedef unsigned short      Elf64_Half;
typedef unsigned int        Elf64_Word;

struct elf64_hdr {
    unsigned char e_ident[16];
    Elf64_Half    e_type;
    Elf64_Half    e_machine;
    Elf64_Word    e_version;
    Elf64_Addr    e_entry;
    Elf64_Off     e_phoff;
    Elf64_Off     e_shoff;
    Elf64_Word    e_flags;
    Elf64_Half    e_ehsize;
    Elf64_Half    e_phentsize;
    Elf64_Half    e_phnum;
    Elf64_Half    e_shentsize;
    Elf64_Half    e_shnum;
    Elf64_Half    e_shstrndx;
};

struct elf64_phdr {
    Elf64_Word  p_type;
    Elf64_Word  p_flags;
    Elf64_Off   p_offset;
    Elf64_Addr  p_vaddr;
    Elf64_Addr  p_paddr;
    Elf64_Off   p_filesz;
    Elf64_Off   p_memsz;
    Elf64_Off   p_align;
};

/* ================================================================
 * elf_load_mem — 从内存缓冲区加载 ELF
 * 返回 entry point, 失败返回 0
 * ================================================================ */
unsigned long elf_load_mem(void *data, unsigned int len)
{
    if (len < 64) return 0;

    struct elf64_hdr *ehdr = (struct elf64_hdr *)data;

    if (ehdr->e_ident[0] != ELFMAG0 || ehdr->e_ident[1] != ELFMAG1 ||
        ehdr->e_ident[2] != ELFMAG2 || ehdr->e_ident[3] != ELFMAG3 ||
        ehdr->e_ident[4] != ELFCLASS64 ||
        ehdr->e_type != ET_EXEC || ehdr->e_machine != EM_X86_64) {
        return 0;
    }

    serial_puts("elf: entry=");
    print_hex64(ehdr->e_entry);
    serial_puts("\r\n");

    for (int i = 0; i < ehdr->e_phnum; i++) {
        unsigned int phoff = ehdr->e_phoff + i * sizeof(struct elf64_phdr);
        if (phoff + sizeof(struct elf64_phdr) > len) break;

        struct elf64_phdr *phdr = (struct elf64_phdr *)((char *)data + phoff);
        if (phdr->p_type != PT_LOAD) continue;

        serial_puts("elf: LOAD vaddr=");
        print_hex64(phdr->p_vaddr);
        serial_puts(" size=");
        print_hex64(phdr->p_filesz);
        serial_puts("\r\n");

        if (phdr->p_filesz > 0) {
            if (phdr->p_offset + phdr->p_filesz > len) {
                serial_puts("elf: segment OOB\r\n");
                return 0;
            }
            char *src = (char *)data + phdr->p_offset;
            char *dst = (char *)(unsigned long)phdr->p_vaddr;
            for (unsigned long j = 0; j < phdr->p_filesz; j++)
                dst[j] = src[j];
        }

        if (phdr->p_memsz > phdr->p_filesz) {
            unsigned long bss_start = phdr->p_vaddr + phdr->p_filesz;
            unsigned long bss_size  = phdr->p_memsz - phdr->p_filesz;
            for (unsigned long j = 0; j < bss_size; j++)
                ((char *)(unsigned long)bss_start)[j] = 0;
        }
    }

    return ehdr->e_entry;
}
