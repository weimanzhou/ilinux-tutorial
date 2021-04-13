org	0100h
	jmp START

%include "pm.inc"        				; 常量,宏,以及一些说明

[SECTION .gdt]
; GDT BEGIN
LABEL_GDT:			descriptor 0, 		0, 				0
LABEL_DESC_CODE32:	descriptor 0, 		0xFFFFF, 		DA_C + DA_32
LABEL_DESC_VIDEO:	descriptor 0xB8000, 0xFFFFF, 		DA_DRW
LABEL_DESC_DATA:	descriptor 0,		0xFFFFF,		DA_DRW | DA_32 | DA_LIMIT_4K
; GDT END

GDT_LEN         	equ $ - LABEL_GDT	; GDT长度
GDT_PTR         	dw GDT_LEN			; GDT界限
               		dd 0           		; GDT基地址

; GDT选择子
SELECTOR_CODE32     equ LABEL_DESC_CODE32	- LABEL_GDT
SELECTOR_VIDEO      equ LABEL_DESC_VIDEO	- LABEL_GDT
SELECTOR_DATA		equ LABEL_DESC_DATA		- LABEL_GDT
; END OF [SECTION .gdt]

TOP_OF_STACK_S16	equ 0x100

[SECTION .s16]
ALIGN 16
[BITS 16]
START:
	mov ax, cs
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov sp, TOP_OF_STACK_S16

    ; 清屏幕操作(scroll up window.)
	mov	ax, 0600h		; AH = 6,  AL = 0h
	mov	bx, 0700h		; 属性被用来在窗口底部写入空白行(BL = 07h)
	mov	cx, 0			; 屏幕左上角: (0, 0)
	mov	dx, 184fh		; 屏幕右下角: (80, 50)
	int	10h				; int 10h

	mov dh, 0
	call DISP_STR

	; 初始化32位代码段描述符
	xor eax, eax
	mov ax, cs
	shl eax, 4
	; 将[SECTION .32]这个段的物理地址赋给eax
	add eax, LABEL_SEG_CODE32
	; 将 eax 的值分三部分赋值给 DESC_CODE32 中的响应位置.
	; 也就是将基址赋值给 DESC_CODE32
	mov word [LABEL_DESC_CODE32 + 2], ax
	shr eax, 16
	mov byte [LABEL_DESC_CODE32 + 4], al
	mov byte [LABEL_DESC_CODE32 + 7], ah

	; 为加载gdtr做准备
	; 将 eax 寄存器清零
	xor eax, eax
	; 将 ds 寄存器的值移到 ax
	mov ax, ds
	; eax 左移四位
	shl eax, 4
	; 将 GDT 物理地址添加到 eax 寄存器中
	add eax, LABEL_GDT                              ; eax <- gdt 基地址
	; 将 eax 的值插入到 GDT_PTR 的基址属性中

	mov dword [GDT_PTR + 2], eax    ; [GDT_PTR + 2] <- 基地址

	; 加载gdtr
	lgdt [GDT_PTR]

	; 关中断
	cli

	; 打开地址线a20
	in al, 0x92
	; 这里是否是需要更改位 00000001b
	or al, 00000010b
	out 0x92, al

	; 准备切换到保护模式
	mov eax, cr0
	or eax, 1
	mov cr0, eax

	; 真正进入保护模式
	jmp dword SELECTOR_CODE32:0             ; 执行这一句会把SELECT_CODE32
											; 装入CS,并跳转到SELECTOR_CODE32:0

MESSAGE_LENGTH		equ 11
BOOT_MESSAGE:		db "hello world"

DISP_STR:
	mov ax, MESSAGE_LENGTH
	mul dh
	add ax, BOOT_MESSAGE
	mov bp, ax
	mov ax, ds
	mov es, ax
	mov cx, MESSAGE_LENGTH
	mov ax, 0x1301
	mov bx, 0x0007
	mov dl, 0
	int 10h
	ret

[SECTION .s32]  ; 32位代码段,由实模式跳入
ALIGN 32
[BITS 32]
LABEL_SEG_CODE32:
	mov ax, SELECTOR_DATA
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov fs, ax

	mov esp, TOP_OF_STACK

	mov ax, SELECTOR_VIDEO
	mov gs, ax

	; ======================================================================
	; 实现字符输出
	; ======================================================================
	mov ax, SELECTOR_VIDEO
	mov gs, ax                                              ; 视频段选择子(目的)

	mov edi, (80 * 10 + 0) * 2              ; 屏幕第10行,第0列
	mov ah, 0x0c                                    ; 0000: 黑底 1100: 红字
	mov al, 'P'
	mov [gs:edi], ax
	add edi, 2
	mov al, 'M'
	mov [gs:edi], ax

	; 到此停止
	jmp $


; 堆栈段
[SECTION .gs]
align 32
LABEL_STACK:		times 512 db 0
TOP_OF_STACK		equ $ - LABEL_STACK