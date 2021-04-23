/*************************************************************************
	> File Name: init.c
	> Author: snowflake
	> Mail: 278121951@qq.com 
	> Created Time: Wed 24 Feb 2021 05:52:48 PM CST
 ************************************************************************/

#include "kernel.h"
#include "protect.h" 


/*************************************************************************
	> 进入内核主函数前做一些初始化工作
    > 1. 保存内核数据段基地址
 *************************************************************************/
PUBLIC void ilinux_init(void) 
{

    // 1. 设置显示位置
    display_position = (80 * 7 + 2 * 0) * 2;

    // 2. 保存内核数据段基地址
    // int data_base = seg2phys(SELECTOR_KERNEL_DS);

    // 3. 初始化保护模式
    init_protect();

    // 4. 初始化硬件中断
    init_interrupt();

    // 5. 内核初始化完毕
    printf("------ KERNEL INIT END ------\n");
}
