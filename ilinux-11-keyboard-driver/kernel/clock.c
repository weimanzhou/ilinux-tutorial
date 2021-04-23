/*************************************************************************
	> File Name: clock.c
	> Author: snowflake
	> Mail: 278121951@qq.com 
	> Created Time: Fri 26 Feb 2021 15:55:48 PM CST
 ************************************************************************/


#include "kernel.h"
#include "process.h"

#define TIMER0          0x40    // 定时器通道0的IO端口
#define TIMER1          0x41    // 定时器通道1的IO端口
#define TIMER2          0x42    // 定时器通道2的IO端口
#define TIMER_MODE      0x43    // 用于定时器模式控制的IO端口
#define RATE_GENERATOR  0x34    /*
                                 * 00-11-010-0
                                 * counter0 - LSB the MSB - rate generator - binary
                                 */
#define TIMER_FREQ      1193182L// clock frequency for timer in PC and AT
#define TIMER_COUNT     (TIMER_FREQ / HZ)
                                // initial value for counter
#define CLOCK_ACK_BIT   0x80    // PS/2 clock interrupt acknowledge bit

int count = 0;

FORWARD _PROTOTYPE( void init_clock, (void) );
FORWARD _PROTOTYPE( int clock_handler, (int irq) );

PUBLIC void clock_task(void) 
{
    // 初始化 8253
    init_clock();

    // 打开中断
    interrupt_unlock();
}

/*************************************************************************
    > clock_handler
    > 
    > @description  这里运行一段时间后，就会抛出异常，这是因为每次发生中断的时候，
    >               会压入五个参数，而使用 ret 返回的时候只会 esp + 4 ，所以运
    >               行一段时间过后系统就会发生错误，如果想解决这个问题，
    >               只需要使用 iretd 即可，这是一个中断 返回，能够将五个参数全部抛出
	> @name clock_handler
    > @param irq    硬件中断号
 ************************************************************************/
PRIVATE int clock_handler(int irq) 
{
    count++;
    if (count % 100 == 0) {
        // k_printf(">");
        // curr_proc++;
        // if(curr_proc > proc_addr(LOW_USER)) {
        //     curr_proc = proc_addr(-NR_TASKS);
        // }
        lock_schedule();
    }
    return ENABLE;
}

/*************************************************************************
    > @description  初始化时钟任务
	> @name init_clock
 ************************************************************************/
PRIVATE void init_clock(void) 
{
    // 写入模式
    out_byte(TIMER_MODE, RATE_GENERATOR);

    // 写入 counter0 的值
    out_byte(TIMER0, (u8_t) TIMER_COUNT);
    out_byte(TIMER0, (u8_t) (TIMER_COUNT >> 8));

    // 注册时钟中断，并打开中断
    put_irq_handler(CLOCK_IRQ, clock_handler);
    enable_irq(CLOCK_IRQ);
}
