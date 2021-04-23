/*************************************************************************
	> File Name: process.c
	> Author: snowflake
	> Mail: 278121951@qq.com 
	> Created Time: Wed 24 Feb 2021 05:52:48 PM CST


    > 进程调度
 ************************************************************************/

#include "kernel.h"
#include "process.h"
#include "ilinux/common.h"

// 进程正在切换则为 TRUE，否则为 FALSE
// 当 TRUE 时应该禁止 硬件中断的产生，不然会产生一些严重的问题
PRIVATE bool_t switching = FALSE;

FORWARD _PROTOTYPE(void hunter, (void));
FORWARD _PROTOTYPE(void schedule, (void));

/*************************************************************************
    > 选定一个进程执行
    > 
    > ilinux 和 minix 一样，使用三个就绪队列：
    >   1. 系统任务
    >   2. 系统服务
    >   3. 用户进程
    > 
    > 调度算法描述：
    >   1. 找到优先级最高的非空队列，并选择队首进程即可
    >   2. 如果所有队列均为空，则执行 IDLE （待机进程）
    >   
    > 该方法完成设置 curr_proc （系统当前运行的进程）。
    > 任何影响到选择下一个运行进程的对这些队列的改变都要再次调用 hunter 。
    > 无论进程在什么时候阻塞，都调用 curr_proc 来重新调度
    > 
	> @method: hunter
 ************************************************************************/
PRIVATE void hunter(void)
{
    register process_t *prey;
    if ((prey = ready_head[TASK_QUEUE]) != NIL_PROC)
    {
        curr_proc = prey;
        return;
    }

    if ((prey = ready_head[SERVER_QUEUE]) != NIL_PROC)
    {
        curr_proc = prey;
        return;
    }

    if ((prey = ready_head[USER_QUEUE]) != NIL_PROC)
    {
        bill_proc = curr_proc = prey;
        return;
    }

    printf("idle task running\n");
    bill_proc = curr_proc = prey = proc_addr(IDLE_TASK);
}

/*************************************************************************
    > 进程就绪
    > 对于每一种进程，采取不同的处理：
    > 1. task_proc 和 server_proc 的进程直接插入尾部
    > 2. user_proc 插入到头部
    > 
	> @method: ready
 ************************************************************************/
PUBLIC void ready(struct process_s *process)
{
    printf("new ready process name: %d\n", process->logic_nr);
    if (is_task_proc(process))
    {
        if (ready_head[TASK_QUEUE] != NIL_PROC)
        {
            ready_tail[TASK_QUEUE]->next_ready = process;
        }
        else
        {
            curr_proc = ready_head[TASK_QUEUE] = process;
        }
        ready_tail[TASK_QUEUE] = process;
        process->next_ready = NIL_PROC;
        return;
    }

    if (is_serv_proc(process))
    {
        if (ready_head[SERVER_QUEUE] != NIL_PROC)
        {
            ready_tail[SERVER_QUEUE]->next_ready = process;
        }
        else
        {
            curr_proc = ready_head[SERVER_QUEUE] = process;
        }
        ready_tail[SERVER_QUEUE] = process;
        process->next_ready = NIL_PROC;
        return;
    }

    // TODO 这个地方跟视频不一样 16:48
    // 为了让用户进程等待时间没有那么久，每次用户进程转到就绪态时，
    // 总插入到用户队列的头部
    if (is_user_proc(process))
    {
        if (ready_head[USER_QUEUE] != NIL_PROC)
        {
            process->next_ready = ready_head[USER_QUEUE];
            ready_head[USER_QUEUE] = process;
        }
        else
        {
            ready_head[USER_QUEUE] = process;
            curr_proc = ready_tail[USER_QUEUE] = process;
        }
        return;
    }
}

/*************************************************************************
    > 取消进程就绪
    > 
    > 将一个不再就绪的进程从其队列中删除，即堵塞。
    > 通常它是将就绪队列头部的进程去掉，因为一个进程只有处于运行状态才可以阻塞。

    > unready 之前一定要调用 hunter
    > 
	> @method: unready
 ************************************************************************/
PUBLIC void unready(struct process_s *process)
{
    register process_t *xp;

    if (is_task_proc(process))
    {
        if ((xp = ready_head[TASK_QUEUE]) == NIL_PROC)
            return;
        if (xp == process)
        {
            ready_head[TASK_QUEUE] = process->next_ready;
            if (xp == curr_proc)
                hunter();
            return;
        }
        while (xp != NIL_PROC)
        {
            xp = xp->next_ready;
            if (xp == process)
                break;
        }
        if (xp != NIL_PROC && xp->next_ready != NIL_PROC)
            xp->next_ready = xp->next_ready->next_ready;
        else
        {
            ready_head[TASK_QUEUE] = NIL_PROC;
            ready_tail[TASK_QUEUE] = NIL_PROC;
        }
        if (ready_tail[TASK_QUEUE] == process)
            ready_tail[TASK_QUEUE] = xp;
        return;
    }

    if (is_serv_proc(process))
    {
        if ((xp = ready_head[SERVER_QUEUE]) == NIL_PROC)
            return;
        if (xp == process)
        {
            ready_head[SERVER_QUEUE] = process->next_ready;
            if (xp == curr_proc)
                hunter();
            return;
        }
        while (xp != NIL_PROC)
        {
            xp = xp->next_ready;
            if (xp == process)
                break;
        }
        if (xp != NIL_PROC && xp->next_ready != NIL_PROC)
            xp->next_ready = xp->next_ready->next_ready;
        else
        {
            ready_head[SERVER_QUEUE] = NIL_PROC;
            ready_tail[SERVER_QUEUE] = NIL_PROC;
        }
        if (ready_tail[SERVER_QUEUE] == process)
            ready_tail[SERVER_QUEUE] = xp;
        return;
    }

    if (is_user_proc(process))
    {
        if ((xp = ready_head[USER_QUEUE]) == NIL_PROC)
            return;
        if (xp == process)
        {
            ready_head[USER_QUEUE] = process->next_ready;
            if (xp == curr_proc)
                hunter();
            return;
        }
        while (xp != NIL_PROC)
        {
            xp = xp->next_ready;
            if (xp == process)
                break;
        }
        if (xp != NIL_PROC && xp->next_ready != NIL_PROC)
            xp->next_ready = xp->next_ready->next_ready;
        else
        {
            ready_head[USER_QUEUE] = NIL_PROC;
            ready_tail[USER_QUEUE] = NIL_PROC;
        }
        if (ready_tail[USER_QUEUE] == process)
            ready_tail[USER_QUEUE] = xp;
        return;
    }
}

PUBLIC void unready_tmp(
    register process_t *proc /* 堵塞的进程 */
)
{
    /* 将一个不再就绪的进程从其队列中删除，即堵塞。
     * 通常它是将队列头部的进程去掉，因为一个进程只有处于运行状态才可被阻塞。
     * unready 在返回之前要一般要调用 hunter。
     */
    register process_t *xp;
    if (is_task_proc(proc))
    { /* 系统任务？ */
        /* 如果系统任务的堆栈已经不完整，内核出错。 */
        if (*proc->stack_guard_word != SYS_TASK_STACK_GUARD)
        {
            panic("stack over run by task", proc->logic_nr);
        }

        xp = ready_head[TASK_QUEUE]; /* 得到就绪队列头的进程 */
        if (xp == NIL_PROC)
            return; /* 并无就绪的系统任务 */
        if (xp == proc)
        {
            /* 如果就绪队列头的进程就是我们要让之堵塞的进程，那么我们将它移除出就绪队列 */
            ready_head[TASK_QUEUE] = xp->next_ready;
            if (proc == curr_proc)
            {
                /* 如果堵塞的进程就是当前正在运行的进程，那么我们需要重新狩猎以得到一个新的运行进程 */
                hunter();
            }
            return;
        }
        /* 如果这个进程不在就绪队列头，那么搜索整个就绪队列寻找它 */
        while (xp->next_ready != proc)
        {
            xp = xp->next_ready;
            if (xp == NIL_PROC)
                return; /* 到边界了，说明这个进程根本就没在就绪队列内 */
        }
        /* 找到了，一样，从就绪队列中移除它 */
        xp->next_ready = xp->next_ready->next_ready;
        /* 如果之前队尾就是要堵塞的进程，那么现在我们需要重新调整就绪队尾指针（因为它现在指向了一个未就绪的进程） */
        if (ready_tail[TASK_QUEUE] == proc)
            ready_tail[TASK_QUEUE] = xp; /* 现在找到的xp进程是队尾 */
    }
    else if (is_serv_proc(proc))
    { /* 系统服务 */
        /* 所作操作同上的系统任务一样 */
        xp = ready_head[SERVER_QUEUE];
        if (xp == NIL_PROC)
            return;
        if (xp == proc)
        {
            ready_head[SERVER_QUEUE] = xp->next_ready;
            /* 这里注意，因为不是系统任务，我们不作那么严格的判断了 */
            hunter();
            return;
        }
        while (xp->next_ready != proc)
        {
            xp = xp->next_ready;
            if (xp == NIL_PROC)
                return;
        }
        xp->next_ready = xp->next_ready->next_ready;
        if (ready_tail[SERVER_QUEUE] == proc)
            ready_tail[SERVER_QUEUE] = xp;
    }
    else
    { /* 用户进程 */
        xp = ready_head[USER_QUEUE];
        if (xp == NIL_PROC)
            return;
        if (xp == proc)
        {
            ready_head[USER_QUEUE] = xp->next_ready;
            /* 这里注意，因为不是系统任务，我们不作那么严格的判断了 */
            hunter();
            return;
        }
        while (xp->next_ready != proc)
        {
            xp = xp->next_ready;
            if (xp == NIL_PROC)
                return;
        }
        xp->next_ready = xp->next_ready->next_ready;
        if (ready_tail[USER_QUEUE] == proc)
            ready_tail[USER_QUEUE] = xp;
    }
}

/*************************************************************************
    > 进程调度
    > 
    > 当时间片用完时进行调度
    > 该算法的结果是将用户进程按照时间片轮转算法运行。文件系统、内存管理器和I/O任务
    > 绝对不回放在队尾，因为他们肯定不回运行的太久。这些进程可以被认为是非常可靠的，
    > 因为它们是我们编写的，而且在完成要做的工作后将阻塞。
    > 
	> @method: schedule
 ************************************************************************/
PRIVATE void schedule(void)
{
    if (ready_head[USER_QUEUE] == NIL_PROC)
        return;

    // TODO 跟视频不一样
    process_t *tmp;
    tmp = ready_head[USER_QUEUE]->next_ready;
    ready_head[USER_QUEUE]->next_ready = NIL_PROC;
    ready_tail[USER_QUEUE]->next_ready = ready_head[USER_QUEUE];
    ready_head[USER_QUEUE] = tmp;

    hunter();
}

/*************************************************************************
    > 进程调度
    > 
    > 进程调度停止，只针对于用户程序
    > 
	> @method: schedule
 ************************************************************************/
PUBLIC void schedule_stop(void)
{
    ready_head[USER_QUEUE] = NIL_PROC;
}

/*************************************************************************
    > 加锁版本安全的狩猎进程
    > 
	> @method: schedule
 ************************************************************************/
PUBLIC void lock_hunter(void)
{
    switching = TRUE;
    hunter();
    switching = FALSE;
}

/*************************************************************************
    > 加锁版本安全的进程就绪例程
    > 
	> @method: schedule
 ************************************************************************/
PUBLIC void lock_ready(struct process_s *process)
{
    switching = TRUE;
    ready(process);
    switching = FALSE;
}

/*************************************************************************
    > 加锁版本安全的进程取消就绪例程
    > 
	> @method: schedule
 ************************************************************************/
PUBLIC void lock_unready(struct process_s *process)
{
    switching = TRUE;
    unready(process);
    switching = FALSE;
}

/*************************************************************************
    > 加锁版本安全的进程就绪例程（只针对于用户进程）
    > 
	> @method: lock_schedule
 ************************************************************************/
PUBLIC void lock_schedule(void)
{
    switching = TRUE;
    schedule();
    switching = FALSE;
}

/*************************************************************************
    > 加锁版本安全的进程调度完毕例程
    > 
	> @method: lock_schedule_stop
 ************************************************************************/
PUBLIC void lock_schedule_stop(void)
{
    switching = TRUE;
    schedule_stop();
    switching = FALSE;
}
