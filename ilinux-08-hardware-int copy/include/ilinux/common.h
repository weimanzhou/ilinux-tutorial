/*************************************************************************
	> File Name: common.h
	> Author: snowflake
	> Mail: 278121951@qq.com 
	> Created Time: Thu 04 Feb 2021 10:55:32 PM CST
 ************************************************************************/

#ifndef _LINUX_COMMON_H
#define _LINUX_COMMON_H


/* 系统调用例程可以支持的操作 */
#define SEND            0x1    	/* 0001: 发送一条消息 */
#define RECEIVE         0x2    	/* 0010: 接收一条消息 */
#define SEND_RECE       0x3    	/* 0011: 发送一条消息并等待对方响应一条消息 */
#define I_O_BOX			0x4   	/* 0100: 设置固定收发件箱  */
#define ANY             0x3ea   /* 魔数，它是一个不存在的进程逻辑编号，用于表示任何进程
                                 *      receive(ANY, msg_buf) 表示接收任何进程的消息
                                 */


#define CLOCK_TASK		-3 	// 定时任务
#define IDLE_TASK		-2 	// 待机任务
#define HARDWARE		-1	// 代表硬件，用于生成软件生成硬件中断，并不存在的实际任务

#define GET_UPTIME      1   // 获取时钟运行时间(tick)
#define GET_TIME        2   // 获取时钟实时时间(s)
#define SET_TIME        3   // 设置时钟实时时间(s)

// 以下是所有需要的消息字段
/* 时钟任务消息中使用的消息字段 */
#define CLOCK_TIME      m6_l1	/* 时间值 */


#endif
