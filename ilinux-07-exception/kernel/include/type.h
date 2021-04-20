/*************************************************************************
	> File Name: type.h
	> Author: snowflake
	> Mail: 278121951@qq.com 
	> Created Time: Fri 05 Feb 2021 05:26:51 PM CST
	>
	> 内核所需的类型定义 
 ************************************************************************/
#ifndef ILINUX_TYPE_H
#define ILINUX_TYPE_H

#if (CHIP == INTEL)
/* 端口数据类型，用于访问I/O端口 */
typedef unsigned port_t;

/* 寄存器数据类型，用于访问存储器段和CPU寄存器 */
typedef unsigned reg_t;

/* 
 * 受保护模式的段描述符
 * 段描述符是与Intel处理器结构相关的另一个结构,它是保证进程
 * 不会发生内存访问越限机制的一部分。
 */
typedef struct seg_descriptor_s {
    u16_t limit_low;        /* 段界限低16位 */
    u16_t base_low;         /* 段基址低16位 */
    u8_t base_middle;       /* 段基址中8位 */
    u8_t access;		    /* 访问权限：| P | DL | 1 | X | E | R | A | */
    u8_t granularity;		/* 比较杂，最重要的有段粒度以及段界限的高4位| G | X  | 0 | A | LIMIT HIGHT | */
    u8_t base_high;         /* 段基址高8位 */
} seg_descriptor_t;

/* 硬件（异常）中断处理函数原型 */
typedef _PROTOTYPE( void (*int_handler_t), (void) );

#endif /* (CHIP == INTEL) */

#endif //ILINUX_TYPE_H
