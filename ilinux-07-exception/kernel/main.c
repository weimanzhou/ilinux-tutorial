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

// 从第 10 行第 0 列开始显示
int display_position = (80 * 12 + 0) * 2;
void printf(char* str);

void ilinux_main(void) 
{
	// printf("HELLO C!!!\n");

	printf("HELLO C!!!\n");

	// 测试零除错误
	int a = 0;
	int b = 5 / a;

	for (;;);
}
