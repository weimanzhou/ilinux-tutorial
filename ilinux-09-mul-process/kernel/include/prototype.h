/*************************************************************************
	> File Name: prototype.h
	> Author: snowflake
	> Mail: 278121951@qq.com 
	> Created Time: Fri 05 Feb 2021 05:26:51 PM CST
	>
	> 内核所需的所有函数原型
 	> 所有那些必须在其定义所在文件外被感知的函数的原型都放在proto.h中。
 	>
 	> 它使用了_PROTOTYPE技术，这样，ILINUX核心便既可以使用传统的C编译器(由Kernighan和Richie定义)，
 	> 例如Minix初始提供的编译器；又可以使用一个现代的ANSI标准C编译器。
 	>
 	> 这其中的许多函数原型是与系统相关的,包括中断和异常处理例程以及用汇编语言写的一些函数。
 ************************************************************************/
#ifndef ILINUX_PROTOTYPE_H
#define ILINUX_PROTOTYPE_H

#include "kernel.h"

/*************************************************************************
	> kernel.asm
**************************************************************************/
_PROTOTYPE(void p_restart, (void));
_PROTOTYPE(void restart, (void));
_PROTOTYPE(void halt, (void));
_PROTOTYPE(void level0_sys_call, (void));
_PROTOTYPE(void ilinux_386_sys_call, (void));

/*************************************************************************
	> 异常处理入口例程
**************************************************************************/
_PROTOTYPE(void divide_error, (void));
_PROTOTYPE(void single_step_exception, (void));
_PROTOTYPE(void nmi, (void));
_PROTOTYPE(void breakpoint_exception, (void));
_PROTOTYPE(void overflow, (void));
_PROTOTYPE(void bounds_check, (void));
_PROTOTYPE(void inval_opcode, (void));
_PROTOTYPE(void copr_not_available, (void));
_PROTOTYPE(void double_fault, (void));
_PROTOTYPE(void inval_tss, (void));
_PROTOTYPE(void copr_not_available, (void));
_PROTOTYPE(void segment_not_present, (void));
_PROTOTYPE(void stack_exception, (void));
_PROTOTYPE(void general_protection, (void));
_PROTOTYPE(void page_fault, (void));
_PROTOTYPE(void copr_seg_overrun, (void));
_PROTOTYPE(void copr_error, (void));
_PROTOTYPE(void divide_error, (void));

/*************************************************************************
	> 硬件中断处理程序。
**************************************************************************/
_PROTOTYPE(void hwint00, (void));
_PROTOTYPE(void hwint01, (void));
_PROTOTYPE(void hwint02, (void));
_PROTOTYPE(void hwint03, (void));
_PROTOTYPE(void hwint04, (void));
_PROTOTYPE(void hwint05, (void));
_PROTOTYPE(void hwint06, (void));
_PROTOTYPE(void hwint07, (void));
_PROTOTYPE(void hwint08, (void));
_PROTOTYPE(void hwint09, (void));
_PROTOTYPE(void hwint10, (void));
_PROTOTYPE(void hwint11, (void));
_PROTOTYPE(void hwint12, (void));
_PROTOTYPE(void hwint13, (void));
_PROTOTYPE(void hwint14, (void));
_PROTOTYPE(void hwint15, (void));

/*************************************************************************
	> kernel_386lib.asm
**************************************************************************/
_PROTOTYPE(void low_print, (char *_str, u8_t color));
_PROTOTYPE(u8_t in_byte, (port_t port));
_PROTOTYPE(void out_byte, (port_t port, U8_t value));
_PROTOTYPE(void interrupt_lock, (void));
_PROTOTYPE(void interrupt_unlock, (void));
_PROTOTYPE(void dsable_irq, (int int_request));
_PROTOTYPE(void enable_irq, (int int_request));
_PROTOTYPE(void clear_screen, (void));
_PROTOTYPE(void down_run, (void));

/*************************************************************************
	> init.c 
**************************************************************************/
_PROTOTYPE(void ilinux_init, (void));

/*************************************************************************
	> main.c 
**************************************************************************/
_PROTOTYPE(void ilinux_main, (void));
_PROTOTYPE( void test_task_a, (void) );
_PROTOTYPE( void test_task_b, (void) );
_PROTOTYPE( void test_task_c, (void) );

/*************************************************************************
	> protect.c
**************************************************************************/
_PROTOTYPE(void init_segment_desc, (seg_descriptor_t * p_desc, phys_bytes base, phys_bytes limit, u16_t attribute));
_PROTOTYPE(void init_protect, (void));
_PROTOTYPE(phys_bytes seg2phys, (U16_t seg));

/*************************************************************************
	> i8259.c
**************************************************************************/
_PROTOTYPE(void init_interrupt, (void));
_PROTOTYPE(void put_irq_handler, (int irq, irq_handler_t handler));

/*************************************************************************
	> misc.c
**************************************************************************/
_PROTOTYPE(int k_printf, (const char *fmt, ...));
_PROTOTYPE(void panic, (const char *msg, int error_no));

/*************************************************************************
	> misc.c
**************************************************************************/
_PROTOTYPE(void init_process_table, (void));

#endif //ILINUX_PROTOTYPE_H
