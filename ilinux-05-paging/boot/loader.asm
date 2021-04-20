org	0100h
	jmp START

%include "pm.inc"        				; 常量,宏,以及一些说明
%include "loader.inc"					; 地址相关的常量

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

	; 检查并得到内存信息
	mov ebx, 0			; 得到后续的内存信息的值，第一次必须为0
	mov di, _MEM_CHK_BUF; es:di 指向准备写入ADRS的缓冲区地址
LABEL_MEM_CHK_LOOP:
	mov eax, 0xE820		; eax=0x0000E820
	mov ecx, 20			; ecx=ADRS的大小
	mov edx, 0x534D4150 ; 约定签名 "SMAP"
	int 0x15			; 得到ADRS

	jc LABEL_MEM_CHK_ERROR
						; 产生了一个进位标志，CF=1，检查得到ADRS错误
	; CF=0
	add di, 20			; di += 20，es:di指向缓冲区准备放入的下一个ADRS的地址
	inc dword [DD_MCR_COUNT]	
						;ADRS的数量++

	cmp ebx, 0
	jne LABEL_MEM_CHK_LOOP 					; ebx=0 表示拿到最后一个ADRS，完成检查并跳出循环
	; ebx!=0，表示还没有拿到最后一个，继续循环
	jmp LABEL_MEM_CHK_FINISH 

LABEL_MEM_CHK_ERROR:
	mov dword [_DD_MCR_COUNT], 0			; 检查失败，ADRS数量为0

	mov dh, 1
	call DISP_STR

	jmp $

LABEL_MEM_CHK_FINISH:

	; 使用 int 0x88 中断号另外一种方式来获取内存大小
	; mov ah, 0x88			; 设置子功能号
	; int 0x15				; 调用中断
	; and eax, 0xffff			; ?
	; mov cx, 0x400			; cx = 1024
	; mul cx					; dx:ax = ax * cx
	; shl edx, 16				; 将乘积的高位左移16位，移动到edx寄存器的高位
	; or edx, eax				; edx = edx | eax 相当于将乘积的低位移动到 edx 的低位
	; add edx, 0x100000		; edx = edx + 1MB 获取到整个内存的大小

	; mov [DD_MEM_SIZE], edx
	; 目前数据存储在 edx 中


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
	jmp dword SELECTOR_CODE32:LABEL_SEG_CODE32 + LOADER_PHY_ADDR
											; 执行这一句会把SELECT_CODE32
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
	mov gs, ax                              ; 视频段选择子(目的)

	mov edi, (80 * 10 + 0) * 2              ; 屏幕第10行,第0列
	mov ah, 0x0c                            ; 0000: 黑底 1100: 红字
	mov al, 'P'
	mov [gs:edi], ax
	add edi, 2
	mov al, 'M'
	mov [gs:edi], ax

	call FUN_CAL_MM_SIZE					; 计算内存大小
	call FUN_PRINT_MM_SIZE					; 打印内存大小 
	call FUN_SETUP_PAGING					; 开启分页

	; 到此停止
	jmp $



%include "loader_32lib.asm"


[SECTION .data32]
align 32
DATA32:
; =====================================================================
; 实模式下的数据
; ---------------------------------------------------------------------
_DD_MCR_COUNT:			dd 0
_DD_MEM_SIZE:			dd 0
_STR_TEST:				dd "Print ~~~", 10, 0
_STR_KB:				dd "KB", 10, 0
_STR_MEM_INFO:			dd "MEMORY SIZE:", 0
; 存储当前光标所在位置
_DD_DISP_POSITION:		dd (80 * 6 + 0) * 2
; 地址范围描述符结构（Address Range Descriptor Structor）
_ADRS:
	_DD_BASE_ADDR_LOW:	dd 0			; 基地址低32位
	_DD_BASE_ADDR_HIG:	dd 0			; 基地址高32位
	_DD_SIZE_LOW:		dd 0			; 内存大小低32位
	_DD_SIZE_HIG:		dd 0			; 内存大小高32位
	_DD_TYPE:			dd 0			; ADRS类型
; 内存检查结果缓冲区，用于存放没检查的ADRS结构，256字节是为了对齐32位，
; 256/20=12.8，所以这个缓冲区可以存放12个ADRS
_MEM_CHK_BUF:			times 256 db 0


; =====================================================================
; 保护模式下的数据
; ---------------------------------------------------------------------
DD_MCR_COUNT:			equ LOADER_PHY_ADDR + _DD_MCR_COUNT 
DD_MEM_SIZE:			equ LOADER_PHY_ADDR + _DD_MEM_SIZE
STR_TEST:				equ LOADER_PHY_ADDR + _STR_TEST
STR_KB:					equ LOADER_PHY_ADDR + _STR_KB
STR_MEM_INFO:			equ LOADER_PHY_ADDR + _STR_MEM_INFO
DD_DISP_POSITION:		equ LOADER_PHY_ADDR + _DD_DISP_POSITION
; 地址范围描述符结构（Aequress Range Descriptor Structor）
ADRS:					equ LOADER_PHY_ADDR + _ADRS
	DD_BASE_ADDR_LOW:	equ LOADER_PHY_ADDR + _DD_BASE_ADDR_LOW		; 基地址低32位
	DD_BASE_ADDR_HIG:	equ LOADER_PHY_ADDR + _DD_BASE_ADDR_HIG		; 基地址高32位
	DD_SIZE_LOW:		equ LOADER_PHY_ADDR + _DD_SIZE_LOW			; 内存大小低32位
	DD_SIZE_HIG:		equ LOADER_PHY_ADDR + _DD_SIZE_HIG			; 内存大小高32位
	DD_TYPE:			equ LOADER_PHY_ADDR + _DD_TYPE				; ADRS类型
MEM_CHK_BUF:			equ LOADER_PHY_ADDR + _MEM_CHK_BUF			; 


DATA_LEN				equ $ - DATA32


; 堆栈段
[SECTION .gs]
align 32
LABEL_STACK:		times 512 db 0
TOP_OF_STACK		equ $ - LABEL_STACK
