/*
 * include/module.h — 内核模块加载器 API
 *
 * 对照: reference/linux-7.1/include/linux/module.h
 */

#ifndef _MODULE_H
#define _MODULE_H

struct module {
    void *init;                    /* init_module 函数指针 */
    void *exit;                    /* cleanup_module 函数指针 */
    char name[56];                 /* 模块名 */
};

/* 加载模块(从 ramfs), 返回 0=成功 */
extern int sys_insmod(const char *name);

/* 内核符号表(供模块链接) */
extern void register_kernel_symbol(const char *name, void *addr);
extern void *lookup_symbol(const char *name);

#endif
