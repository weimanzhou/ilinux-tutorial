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

void ilinux_main(void) 
{
	// 1. 清理屏幕
	clear_screen();
	printf("#root >\n");

	// 2. 启动定时任务
	clock_task();

	// 3. 初始化系统进程表
    init_process_table();

	// 4. 设置当前进程
	curr_proc = proc_addr(-3);
	
	// 5. 开始进程
	restart();

	for (;;);
}
