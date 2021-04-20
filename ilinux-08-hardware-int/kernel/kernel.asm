; =====================================================================
; 导出的函数或变量
; ---------------------------------------------------------------------
; 导入头文件
; ---------------------------------------------------------------------
%include "asm_const.inc"

; ---------------------------------------------------------------------
; 导出的函数
; ---------------------------------------------------------------------
global _start

global divide_error
global single_step_exception
global nmi
global breakpoint_exception
global overflow
global bounds_check
global inval_opcode
global copr_not_available
global double_fault
global copr_seg_overrun
global inval_tss
global segment_not_present
global stack_exception
global general_protection
global page_fault
global copr_error

; ---------------------------------------------------------------------
; 所有中断处理入口，一共16个(两个8259A)
; ---------------------------------------------------------------------
global	hwint00
global	hwint01
global	hwint02
global	hwint03
global	hwint04
global	hwint05
global	hwint06
global	hwint07
global	hwint08
global	hwint09
global	hwint10
global	hwint11
global	hwint12
global	hwint13
global	hwint14
global	hwint15


; ---------------------------------------------------------------------
; 导出的变量
; ---------------------------------------------------------------------

; =====================================================================
; 导入的函数或变量
; ---------------------------------------------------------------------
; 导入的函数
; ---------------------------------------------------------------------
extern ilinux_init						; 初始化一些事情，主要是改变GDT_PTR，让它指向新的GDT
extern ilinux_main						; 内核主函数

extern exception_handler				; 异常统一处理例程

; ---------------------------------------------------------------------
; 导入的变量
; ---------------------------------------------------------------------
extern gdt_ptr							; GDT指针
extern idt_ptr							; IDT指针
extern tss								; 任务状态段
extern irq_handler_table				; 硬件中断请求表

; =====================================================================
; kernel data segment
; ---------------------------------------------------------------------
[section .data]
bits 32
	nop

; =====================================================================
; kernel stack segment
; ---------------------------------------------------------------------
[section .stack]
STACK_SPACE:		times 4 * 1024 db 0	; 4KB栈空间
STACK_TOP:								; 栈顶

; =====================================================================
; 内核代码段
; ---------------------------------------------------------------------
[section .text]
_start:								; 程序入口
	; reg reset
	; es = fs = ss = es， 在C语言中，它们是等同的
    mov ax, ds
	mov ss, ax
	mov es, ax
	mov fs, ax	
	mov sp, STACK_TOP

    mov edi, (80 * 11 + 0) * 2              ; 屏幕第10行,第0列
	mov ah, 0x0c                            ; 0000: 黑底 1100: 红字
	mov al, 'K'
	mov [gs:edi], ax
	add edi, 2
	mov al, 'N'
	mov [gs:edi], ax

	; 将GDT拷贝到内核中
	sgdt [gdt_ptr]						; 将 LOADER 中的 GDT 指针保存到 gdt_ptr

	call ilinux_init					; 初始化工作

	lgdt [gdt_ptr]						
	lidt [idt_ptr]				

	jmp _init

_init:

	; 加载任务状态段 TSS
	xor eax, eax
	mov ax, SELECTOR_TSS
	ltr ax								; load tss		

	jmp ilinux_main					; 跳入到C语言的主函数

	jmp	$		; Start




;======================================================================
;   硬件中断处理
; ---------------------------------------------------------------------
%macro hwint_master 1
	; 1. 屏蔽当前中断
	in al, INT_M_CTLMASK	; 取出 8259 的屏蔽位图
	or al, (1 << %1)		; 将该中断的屏蔽位置置位，表示屏蔽它
	out INT_M_CTLMASK, al	; 输出新的屏蔽位图，表示屏蔽当前中断

	; 2. 重新启用 8259a 和中断响应
	mov al, EOI
	out INT_M_CTL, al		; EOI位置位，重新启用 8259
	nop 
	sti 					; 打开中断响应

	; 3. 调用中断处理例程表中的例程
	push %1
	call [irq_handler_table + 4 * %1]
	add esp, 4

	; 4. 最后，判断中断处理例程的返回值，如果颂 DISABLE（0），我们就直接结束，如果不为0，那么我们
	; 重新启用当前中断
	cli 					; 先将中断关闭，这一步不能被其它中断影响
	cmp eax, 0
	je .0

	in al, INT_M_CTLMASK	; 取出 8259 屏蔽位图
	and al, ~(1 << %1)		; 将该位置0，启用中断
	out INT_M_CTLMASK, al	; 输出新的屏蔽位图，启用当前中断
	sti
.0:	
	iret
%endmacro

align	16
hwint00:		; Interrupt routine for irq 0 (the clock)，时钟中断
 	hwint_master	0

align	16
hwint01:		; Interrupt routine for irq 1 (keyboard)，键盘中断
 	hwint_master	1

align	16
hwint02:		; Interrupt routine for irq 2 (cascade!)
 	hwint_master	2

align	16
hwint03:		; Interrupt routine for irq 3 (second serial)
 	hwint_master	3

align	16
hwint04:		; Interrupt routine for irq 4 (first serial)
 	hwint_master	4

align	16
hwint05:		; Interrupt routine for irq 5 (XT winchester)
 	hwint_master	5

align	16
hwint06:		; Interrupt routine for irq 6 (floppy)，软盘中断
 	hwint_master	6

align	16
hwint07:		; Interrupt routine for irq 7 (printer)，打印机中断
 	hwint_master	7



%macro hwint_slave 1
	; 1. 屏蔽当前中断
	in al, INT_M_CTLMASK	; 取出次 8259 的屏蔽位图
	or al, (1 << %1 - 8)	; 将该中断的屏蔽位置置位，表示屏蔽它
	out INT_M_CTLMASK, al	; 输出新的屏蔽位图，表示屏蔽当前中断

	; 2. 重新启用 8259a 和中断响应
	mov al, EOI
	out INT_M_CTL, al		; EOI位置位，重新启用主 8259
	nop 
	out INT_S_CTL, al		; EOI位置位，重新启用次 8259
	sti 					; 打开中断响应

	; 3. 调用中断处理例程表中的例程
	push %1
	call [irq_handler_table + (4 * %1)]
	add esp, 4

	; 4. 最后，判断中断处理例程的返回值，如果颂 DISABLE（0），我们就直接结束，如果不为0，那么我们
	; 重新启用当前中断
	cli 					; 先将中断关闭，这一步不能被其它中断影响
	cmp eax, 0
	je .0

	in al, INT_M_CTLMASK	; 取出 8259 屏蔽位图
	and al, ~(1 << (%1 - 8))	; 将该位置0，启用中断
	out INT_M_CTLMASK, al	; 输出新的屏蔽位图，启用当前中断
	sti
.0:	
	iret
%endmacro

align	16
hwint08:		; Interrupt routine for irq 8 (realtime clock).
 	hwint_slave	8

align	16
hwint09:		; Interrupt routine for irq 9 (irq 2 redirected)
 	hwint_slave	9

align	16
hwint10:		; Interrupt routine for irq 10
 	hwint_slave	10

align	16
hwint11:		; Interrupt routine for irq 11
 	hwint_slave	11

align	16
hwint12:		; Interrupt routine for irq 12
 	hwint_slave	12

align	16
hwint13:		; Interrupt routine for irq 13 (FPU exception)
 	hwint_slave	13

align	16
hwint14:		; Interrupt routine for irq 14 (AT winchester)
 	hwint_slave	14

align	16
hwint15:		; Interrupt routine for irq 15
 	hwint_slave	15



;======================================================================
;   异常处理
; ---------------------------------------------------------------------
divide_error:
    
	push	0xffffffff					; 没有错误代码，用0xffffffff表示
	push	0		    				; 中断向量号	= 0
	jmp	exception
single_step_exception:
    
	push	0xffffffff					; 没有错误代码，用0xffffffff表示
	push	1		    				; 中断向量号	= 1
	jmp	exception
nmi:
    
	push	0xffffffff					; 没有错误代码，用0xffffffff表示
	push	2		    				; 中断向量号	= 2
	jmp	exception
breakpoint_exception:
    
	push	0xffffffff					; 没有错误代码，用0xffffffff表示
	push	3		    				; 中断向量号	= 3
	jmp	exception
overflow:
    
	push	0xffffffff					; 没有错误代码，用0xffffffff表示
	push	4		    				; 中断向量号	= 4
	jmp	exception
bounds_check:
    
	push	0xffffffff					; 没有错误代码，用0xffffffff表示
	push	5		    				; 中断向量号	= 5
	jmp	exception
inval_opcode:
    
	push	0xffffffff					; 没有错误代码，用0xffffffff表示
	push	6		    				; 中断向量号	= 6
	jmp	exception
copr_not_available:
    
	push	0xffffffff					; 没有错误代码，用0xffffffff表示
	push	7		    				; 中断向量号	= 7
	jmp	exception				
double_fault:				
    				
	push	8		    				; 中断向量号	= 8
	jmp	exception				
copr_seg_overrun:				
    				
	push	0xffffffff					; 没有错误代码，用0xffffffff表示
	push	9		    				; 中断向量号	= 9
	jmp	exception				
inval_tss:				
    				
	push	10		    				; 中断向量号	= 10
	jmp	exception				
segment_not_present:				
    				
	push	11		    				; 中断向量号	= 11
	jmp	exception				
stack_exception:				
    				
	push	12		    				; 中断向量号	= 12
	jmp	exception				
general_protection:				
    				
	push	13		    				; 中断向量号	= 13
	jmp	exception				
page_fault:				
    				
	push	14		    				; 中断向量号	= 14
	jmp	exception				
copr_error:				
    				
	push	0xffffffff					; 没有错误代码，用0xffffffff表示
	push	16		    				; 中断向量号	= 16
	jmp	exception

exception:
	call	exception_handler
	add	esp, 4 * 2	    				; 让栈顶指向 EIP，堆栈中从顶向下依次是：EIP、CS、EFLAGS
    ret                 				; 系统已将异常解决，继续运行！
