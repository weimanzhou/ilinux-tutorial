/*************************************************************************
	> File Name: c.c
	> Author: snowflake
	> Mail: 278121951@qq.com 
	> Created Time: Fri 05 Feb 2021 06:12:37 PM CST
	> 本文件其编译生成的目标文件将包含所有的核心数据结构,我们已经看到许多这类数据结构定义在glo.h 和proc.h中。
 	>
 	> 文件开始就定义了宏_TABLE,正好位于 #include语句之前。该定义将导致EXTERN被定义为空串,于是为EXTERN之后
 	> 的所有数据声明分配存储空间。除了global.h 和 process.h中的结构以外,tty.h中定义的由终端任务使用的几个全局变量
 	> 也都在这里分配存储空间。
 	>
 	> 这个方法非常巧妙，使用这个方法，一次头文件extern声明全局变量，在这个文件中导入，又将真正的变量声明出来并
 	> 分配空间。
 ************************************************************************/

#define _TABLE

#include "kernel.h"
#include <stdlib.h>
#include <termios.h>
#include <ilinux/common.h>
#include "process.h"
#include "protect.h"
// #include "assert.h"

// INIT_ASSERT

/* === 堆栈相关 === */
/* 一个 512 字节 的小栈 */
#define SMALL_STACK 		(128 * sizeof(char*))
/* 这是一个普通堆栈大小，1KB */
#define NORMAL_STACK 		(256 * sizeof(char*))


/* 时钟任务栈 */
// #define CLOCK_TASK_STACK    SMALL_STACK
/* 待机任务堆栈 */
// #define IDLE_TASK_STACK     (19 * sizeof(char*))    /* 3 个中断, 3 缓存 , 4 个字节，所以理论上只要 10 个。
//                                                      * 个人喜欢单数所以给多 9 个，没关系的。当然嫌麻烦直接
//                                                      * 设置 SMALL_STACK 也没问题啦~ */

/* 虚拟硬件栈 */
// #define HARDWARE_STACK      0

/* 所有系统进程的栈空间总大小 */
#define TOTAL_SYS_PROC_STACK   	(SMALL_STACK + SMALL_STACK + SMALL_STACK + SMALL_STACK)

/* 所有系统进程堆栈的堆栈空间。 （声明为（char *）使其对齐。） */
PUBLIC char *sys_proc_stack[TOTAL_SYS_PROC_STACK / sizeof(char *)];

FORWARD _PROTOTYPE(void check_process_table, (void));


/* === 系统进程表，包含系统任务以及系统服务 === */
PUBLIC sys_proc_t sys_proc_table[] = {
        /* ************************* 系统任务 ************************* */
        /* 时钟任务 */
        // { clock_task, CLOCK_TASK_STACK, "CLOCK" },
        // /* 待机任务 */
        { idle_task, 	SMALL_STACK, "IDLE" },
        // /* 虚拟硬件任务，只是占个位置 - 用作判断硬件中断 */
        // { 0, HARDWARE_STACK, "HARDWARE" },
        { test_task_a, 	SMALL_STACK, "TEST_A" },
		{ test_task_b, 	SMALL_STACK, "TEST_B" },
		{ test_task_c, 	SMALL_STACK, "TEST_C" },
        /* ************************* 系统服务 ************************* */
};

/* 
 * 虽然已经尽量将所有用户可设置的配置消息单独放在 include/flyanx/config.h 中,
 * 但是在将系统任务表的大小与
 * NR_TASKS 相匹配时仍可能会出现错误。在这里我们使用了一个小技巧对这个错误进行检测。方法是在这里声明一个
 * dummy_task_table，声明的方式是假如发生了前述的错误,则 dummy_task_table 的大小将是非法的,从而导致编译错误。
 * 由于哑数组声明为 extern ,此处并不会为它分配空间(其他地方也不为其分配空间)。因为在代码中任何地方都不会
 * 引用到它,所以编译器和地址空间不会受任何影响。
 *
 * 简单解释：减去的是 ORIGIN，这些都不属于系统进程。
 */
//#define NKT (sizeof(task_table) / sizeof(Task_t) - (ORIGIN_PROC_NR + 1))
// #define NKT ( sizeof(sys_proc_table) / sizeof(SysProc_t) )

// extern int dummy_task_table_check[NR_TASKS + NR_SERVERS == NKT ? 1 : -1];


PUBLIC void init_mul_process() 
{	
	// check_process_table();

    /* 进程表的所有表项都被标志为空闲;
     * 对用于加快进程表访问的 p_proc_addr 数组进行初始化。
     */
    register process_t *proc;
    register int logic_nr;
    for(proc = BEG_PROC_ADDR, logic_nr = -NR_TASKS; proc < END_PROC_ADDR; proc++, logic_nr++) {
        if(logic_nr > 0)    /* 系统服务和用户进程 */
            memcpy(proc->name, "unused", 6);
            // strcpy(proc->name, "unused");
        proc->logic_nr = logic_nr;
        p_proc_addr[logic_nr_2_index(logic_nr)] = proc;
    }

    /* 
	 * 初始化多任务支持
     * 为系统任务和系统服务设置进程表，它们的堆栈被初始化为数据空间中的数组
     */
    sys_proc_t *sys_proc;
    reg_t sys_proc_stack_base = (reg_t) sys_proc_stack;
    u8_t privilege;         /* CPU 权限 */
    u8_t rpl;               /* 段访问权限 */
    for(logic_nr = -NR_TASKS; logic_nr <= LOW_USER; logic_nr++) {   /* 遍历整个系统进程 */

        proc = proc_addr(logic_nr);                                 /* 拿到系统进程对应应该放在的进程指针 */
        sys_proc = &sys_proc_table[logic_nr_2_index(logic_nr)];     /* 系统进程项 */
        // strcpy(proc->name, sys_proc->name);                      /* 拷贝名称 */
        /* 判断是否是系统任务还是系统服务 */
        if(logic_nr < 0) {  /* 系统任务 */
            /* 设置权限 */
            proc->priority = PROC_PRI_TASK;
            rpl = privilege = TASK_PRIVILEGE;
        } else {            /* 系统服务 */
            proc->priority = PROC_PRI_SERVER;
            rpl = privilege = SERVER_PRIVILEGE;
        }
        /* 堆栈基地址 + 分配的栈大小 = 栈顶 */
        sys_proc_stack_base += sys_proc->stack_size;

        /* 初始化系统进程的 LDT */
        proc->ldt[CS_LDT_INDEX] = gdt[TEXT_INDEX];  /* 和内核公用段 */
        proc->ldt[DS_LDT_INDEX] = gdt[DATA_INDEX];
        /* 改变DPL描述符特权级 */
        proc->ldt[CS_LDT_INDEX].access = (DA_CR | (privilege << 5));
        proc->ldt[DS_LDT_INDEX].access = (DA_DRW | (privilege << 5));
        /* 设置任务和服务的内存映射 */
        proc->map.base = KERNEL_TEXT_SEG_BASE;
        proc->map.size = 0;     /* 内核的空间是整个内存，所以设置它没什么意义，为 0 即可 */
        /* 初始化系统进程的栈帧以及上下文环境 */
        proc->regs.cs = ((CS_LDT_INDEX * DESCRIPTOR_SIZE) | SA_TIL | rpl);
        proc->regs.ds = ((DS_LDT_INDEX * DESCRIPTOR_SIZE) | SA_TIL | rpl);
        proc->regs.es = proc->regs.fs = proc->regs.ss = proc->regs.ds;  /* C 语言不加以区分这几个段寄存器 */
        proc->regs.gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK | rpl);       /* gs 指向显存 */
        proc->regs.eip = (reg_t) sys_proc->initial_eip;                 /* eip 指向要执行的代码首地址 */
        proc->regs.esp = sys_proc_stack_base;                           /* 设置栈顶 */
        proc->regs.eflags = is_task_proc(proc) ? INIT_TASK_PSW : INIT_PSW;

        /* 进程刚刚初始化，让它处于可运行状态，所以标志位上没有1 */
        proc->flags = CLEAN_MAP;

         if (!is_idle_hardware(logic_nr))
            ready(proc);
    }

    /* 设置消费进程，它需要一个初值。因为系统闲置刚刚启动，所以此时闲置进程是一个最合适的选择。
     * 随后在调用下一个函数 lock_hunter 进行第一次进程狩猎时可能会选择其他进程。
     */
    // bill_proc = proc_addr(IDLE_TASK);
    // proc_addr(IDLE_TASK)->priority = PROC_PRI_IDLE;
}

/*************************************************************************
	> check_process_table
	> 
    > 检查 include/ilinux/const.h 中的  NR_TASKS + NR_SERVERS 与
    > sizeof(sys_proc_table) / sizeof(sys_proc_t) 是否相等。如果不相等，则报错
 ************************************************************************/
// PRIVATE void check_process_table() {
// 	int expect_process_count = NR_TASKS + NR_SERVERS;
// 	int actual_process_count = sizeof(sys_proc_table) / sizeof(sys_proc_t);
// 	assert(expect_process_count == actual_process_count);
// }


PUBLIC void test_task_a(void) {
	int i, j, k;
	k = 0;
	while (TRUE)
	{
		for (i = 0; i < 100; i++) {
			for (j = 0; j < 1000000; j++) {

			}
		}
		printf("#{A} -> %2d\n", k++);
	}
	
}

PUBLIC void test_task_b(void) {
	int i, j, k;
	k = 0;
	while (TRUE)
	{
		for (i = 0; i < 100; i++) {
			for (j = 0; j < 1000000; j++) {

			}
		}
		printf("#{B} -> %2d\n", k++);
	}
}

PUBLIC void test_task_c(void) {
	int i, j, k;
	k = 0;
	while (TRUE)
	{
		for (i = 0; i < 100; i++) {
			for (j = 0; j < 1000000; j++) {

			}
		}
		printf("#{C} -> %2d\n", k++);
	}
}

PUBLIC void idle_task(void) 
{
    while (1)
    {
        level0(halt);
    }
}
