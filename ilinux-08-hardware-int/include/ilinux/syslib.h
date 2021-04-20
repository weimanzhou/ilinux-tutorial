/*************************************************************************
	> File Name: syslib.h
	> Author: snowflake
	> Mail: 278121951@qq.com 
	> Created Time: Fri 05 Feb 2021 03:17:57 PM CST
 ************************************************************************/

#ifndef _ILINUX_SYSLIB_H
#define _ILINUX_SYSLIB_H

#ifndef ILINUX_TYPES_H
#include <sys/types.h>
#endif

/* ilinux 用户和系统共用库 */
_PROTOTYPE( int send_rece, (int src, message_t* io_msg) );
_PROTOTYPE( int in_outbox, (message_t* in_msg, message_t* out_msg) );


/* ilinux 系统专有库 */
_PROTOTYPE( int send, (int dest, message_t* out_msg) );
_PROTOTYPE( int receive, (int src, message_t* in_msg) );


#endif // _ILINUX_SYSLIB_H

