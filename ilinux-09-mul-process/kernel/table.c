/*************************************************************************
	> File Name: table.c
	> Author: snowflake
	> Mail: 278121951@qq.com 
	> Created Time: Fri 05 Feb 2021 06:12:37 PM CST
 ************************************************************************/

#define _TABLE

#include "kernel.h"
#include <stdlib.h>
#include <termios.h>
#include <ilinux/common.h>
#include "process.h"
#include "assert.h"
#include "protect.h"

INIT_ASSERT

/* === 堆栈相关 === */
/* 一个 512 字节 的小栈 */
#define SMALL_STACK 		(128 * sizeof(char*))
/* 这是一个普通堆栈大小，1KB */
#define NORMAL_STACK 		(256 * sizeof(char*))

/* 所有系统进程的栈空间总大小 */
#define TOTAL_SYS_PROC_STACK   	(SMALL_STACK + SMALL_STACK + SMALL_STACK)

/* 所有系统进程堆栈的堆栈空间。 （声明为（char *）使其对齐。） */
PUBLIC char *sys_proc_stack[TOTAL_SYS_PROC_STACK / sizeof(char *)];

/*************************************************************************
	> 系统进程表，包含系统任务以及系统服务
 ************************************************************************/
PUBLIC sys_proc_t sys_proc_table[] = {
        /* ************************* 系统任务 ************************* */
        { test_task_a, SMALL_STACK, "TEST_A" },
		{ test_task_b, SMALL_STACK, "TEST_B" },
		{ test_task_c, SMALL_STACK, "TEST_C" },
        /* ************************* 系统服务 ************************* */
};

FORWARD _PROTOTYPE(void check_process_table, (void));


/*************************************************************************
	> init_process_table
	> 初始化系统进程表
 ************************************************************************/
PUBLIC void init_process_table() {
	
    // 检查进程表的状态
    check_process_table();

    // 为每个进程分配一个唯一的 LTD
    process_t* process = BEG_PROC_ADDR;
    int ldt_index = LDT_FIRST_INDEX;
    for (; process < END_PROC_ADDR; process++, ldt_index++) {
        memset(process, 0, sizeof(process_t));
        init_segment_desc(
            &gdt[ldt_index],
            vir2phys(process->ldt),
            sizeof(process->ldt) - 1,
            DA_LDT
        );
        process->ldt_sel = ldt_index * DESCRIPTOR_SIZE;
    }

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
    }

}


/*************************************************************************
	> check_process_table
	> 
    > 检查 include/ilinux/const.h 中的  NR_TASKS + NR_SERVERS 与
    > sizeof(sys_proc_table) / sizeof(sys_proc_t) 是否相等。如果不相等，则报错
 ************************************************************************/
PRIVATE void check_process_table() {
	int expect_process_count = NR_TASKS + NR_SERVERS;
	int actual_process_count = sizeof(sys_proc_table) / sizeof(sys_proc_t);
	assert(expect_process_count == actual_process_count);
}


/*************************************************************************
	> test_task_a
	> 
    > 测试任务 a
 ************************************************************************/
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

/*************************************************************************
	> test_task_b
	> 
    > 测试任务 b
 ************************************************************************/
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

/*************************************************************************
	> test_task_c
	> 
    > 测试任务 c
 ************************************************************************/
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
