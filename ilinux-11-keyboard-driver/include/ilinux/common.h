/*************************************************************************
	> File Name: common.h
	> Author: snowflake
	> Mail: 278121951@qq.com 
	> Created Time: Thu 04 Feb 2021 10:55:32 PM CST
 ************************************************************************/

#ifndef _LINUX_COMMON_H
#define _LINUX_COMMON_H

#define ANY             0x3ea   /* 魔数，它是一个不存在的进程逻辑编号，用于表示任何进程
                                 *      receive(ANY, msg_buf) 表示接收任何进程的消息
                                 */


#define CLOCK_TASK		-3 	// 定时任务
#define IDLE_TASK		-2 	// 待机任务
#define HARDWARE		-1	// 代表硬件，用于生成软件生成硬件中断，并不存在的实际任务


#endif
