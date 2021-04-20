/*************************************************************************
	> File Name: main.c
	> Author: snowflake
	> Mail: 278121951@qq.com 
	> Created Time: Sat 30 Jan 2021 10:07:03 PM CST
	>
	>
	> 本文件的入口点是：
	>  - ilinux_main           内核跳转到该方法进行C语言模式
 ************************************************************************/
#include "kernel.h"
#include "protect.h" 
#include "assert.h"
#include "process.h"

FORWARD _PROTOTYPE (void init_mul_process_by_me, (void) );

void ilinux_main(void) 
{
	// 1. 清理屏幕
	clear_screen();
	printf("#root >\n");

	// 2. 启动定时任务
	clock_task();

	// 3. 初始化系统进程表
	init_mul_process_by_me();

	// 4. 设置当前进程
	curr_proc = proc_addr(-3);
	
	// 开始进程
	restart();

	for (;;);
}

PRIVATE void init_mul_process_by_me() {
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

        /* ================= 初始化系统进程的 LDT ================= */
        proc->ldt[CS_LDT_INDEX] = gdt[TEXT_INDEX];  /* 和内核公用段 */
        proc->ldt[DS_LDT_INDEX] = gdt[DATA_INDEX];
        /* ================= 改变DPL描述符特权级 ================= */
        proc->ldt[CS_LDT_INDEX].access = (DA_CR | (privilege << 5));
        proc->ldt[DS_LDT_INDEX].access = (DA_DRW | (privilege << 5));
        /* 设置任务和服务的内存映射 */
        proc->map.base = KERNEL_TEXT_SEG_BASE;
        proc->map.size = 0;     /* 内核的空间是整个内存，所以设置它没什么意义，为 0 即可 */
        /* ================= 初始化系统进程的栈帧以及上下文环境 ================= */
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
		printf("#{C} -> %2d", k++);
	}
}