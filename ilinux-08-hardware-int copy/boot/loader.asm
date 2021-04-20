org	0100h
	jmp START

%include "pm.inc"        				; 常量,宏,以及一些说明
%include "loader.inc"					; 地址相关的常量
%include "fat12hdr.inc"

[SECTION .gdt]
; GDT BEGIN
LABEL_GDT:			descriptor 0, 		0, 				0
LABEL_DESC_CODE32:	descriptor 0, 		0xFFFFF, 		DA_CR | DA_32 | DA_LIMIT_4K
LABEL_DESC_DATA:	descriptor 0,		0xFFFFF,		DA_DRW | DA_32 | DA_LIMIT_4K
LABEL_DESC_VIDEO:	descriptor 0xB8000, 0xFFFFF, 		DA_DRW | DA_DPL3
; GDT END

GDT_LEN         	equ $ - LABEL_GDT	; GDT长度
GDT_PTR         	dw GDT_LEN			; GDT界限
               		dd LOADER_PHY_ADDR + LABEL_GDT           		; GDT基地址

; GDT选择子
SELECTOR_CODE32     equ LABEL_DESC_CODE32	- LABEL_GDT
SELECTOR_DATA		equ LABEL_DESC_DATA		- LABEL_GDT
SELECTOR_VIDEO      equ LABEL_DESC_VIDEO	- LABEL_GDT  | SA_RPL3
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

	; 进行磁盘复位
    xor ah, ah
    int 13h

FUN_LOAD_LOADER_FILE:
    ; 下面在A盘的根目录寻找loader.bin
    mov word [W_SECTOR_NO], FAT12_SECTOR_NUM_OF_ROOT_DIR
LABEL_SEARCH_IN_ROOT_DIR_BEGIN:
    cmp word [W_ROOT_DIR_SIZE_FOR_LOOP], 0  ; 判断根目录区是否已经读完,
    jz LABEL_NO_LOADERBIN                   ; 如果读完表示没有找到loader.bin
    dec word [W_ROOT_DIR_SIZE_FOR_LOOP]


	; 从第 ax 个sector开始，将 cl 个sector读入es:bx中
    mov ax, KERNEL_SEG				;
    mov es, ax								; 设置缓冲区段地址 
    mov bx, KERNEL_OFFSET				; 设置缓冲区偏移地址
    mov ax, [W_SECTOR_NO]					; 设置从第 ax 个扇区读取
    mov cl, 1								; 读取扇区的个数
    call FUN_READ_SECTOR					; 调用读取扇区

	mov si, KERNEL_NAME                ; ds:si -> "LOADER  BIN"
    mov di, KERNEL_OFFSET              ; es:di -> BaseOfLoader:0100 = BaseOfLoader*10h+100

	cld
   	mov dx, 10h

LABEL_SEARCH_FOR_LOADERBIN:
    cmp dx, 0

    jz LABEL_GOTO_NEXT_SECTOR_IN_ROOT_DIR
    dec dx
    mov cx, 11
LABEL_CMP_FILENAME:
    cmp cx, 0
    jz LABEL_FILENAME_FOUND
    dec cx
    lodsb
    cmp al, byte [es:di]
    jz LABEL_GO_ON
    jmp LABEL_DIFFERENT

LABEL_GO_ON:
    inc di
    jmp LABEL_CMP_FILENAME

LABEL_DIFFERENT:
    and di, 0xFFE0
    add di, 0x20
    mov si, KERNEL_NAME
    jmp LABEL_SEARCH_FOR_LOADERBIN

LABEL_GOTO_NEXT_SECTOR_IN_ROOT_DIR:
    add word [W_SECTOR_NO], 1
    jmp LABEL_SEARCH_IN_ROOT_DIR_BEGIN

LABEL_NO_LOADERBIN:
    mov dh, 2
    call DISP_STR

%ifdef _BOOT_DEBUG_
    mov ax, 0x4c00
    int 21h
%else
    jmp $
%endif

LABEL_FILENAME_FOUND:
    mov	ax, FAT12_ROOT_DIR_SECTORS
	and	di, 0FFE0h		                    ; di -> 当前条目的开始
	add	di, 01Ah		                    ; di -> 首 SECTOR
	mov	cx, word [es:di]
	push cx			                        ; 保存此 SECTOR 在 FAT 中的序号
	add	cx, ax
	add	cx, FAT12_DELTA_SECTOR_NO	        ; cl 为 LOADER.bin 的起始扇区号
	mov	ax, KERNEL_SEG
	mov	es, ax			                    ; es <- LOADER_FILE_BASE
	mov	bx, KERNEL_OFFSET	            ; bx <- LOADER_FILE_OFFSET
                                            ; es:bx = LOADER_FILE_BASE:LOADER_FILE_OFFSET
	mov	ax, cx			                    ; ax <- SECTOR 号

LABEL_GOON_LOADING_FILE:
	push ax			                        ; 
	push bx			                        ; 
	mov	ah, 0Eh			                    ; 
	mov	al, '.'			                    ; 
	mov	bl, 0Fh			                    ; 
	int	10h			                        ; 
	pop	bx			                        ; 
	pop	ax			                        ; 每读取一个扇区就在 booting 后面输出一个 . 

	mov	cl, 1
	call FUN_READ_SECTOR
	pop	ax			                        ; 取出此 SECTOR 在 FAT 中的序号
	call FUN_GET_FAT_ENTRY
	cmp	ax, 0FFFh
	jz	LABEL_FILE_LOADED
	push ax			                        ; 保存此 SECTOR 在 FAT 中的序号
	mov	dx, FAT12_ROOT_DIR_SECTORS
	add	ax, dx
	add	ax, FAT12_DELTA_SECTOR_NO
	add	bx, [BPB_BytsPerSec]
	jmp	LABEL_GOON_LOADING_FILE
LABEL_FILE_LOADED:


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


; ============================================================================
; 
; description:	根据 fat 将内容加载到内存中
; ============================================================================
FUN_GET_FAT_ENTRY:
	push es
	push bx
	push ax
	mov	ax, LOADER_FILE_BASE	            ; --
	sub	ax, 0100h		                    ; 在 LOADER_FILE_BASE 后面留出 4KB 空间
	mov	es, ax			                    ; 用于存放 FAT
	pop	ax
	mov	byte [B_ODD], 0
	mov	bx, 3
	mul	bx			                        ; dx:ax = ax * 3
	mov	bx, 2
	div	bx			                        ; dx:ax / 2  ==>  ax <- 商, dx <- 余数
	cmp	dx, 0
	jz LABEL_EVEN
	mov	byte [B_ODD], 1
LABEL_EVEN:                                 ; 偶数
	xor	dx, dx			                    ; 现在 ax 是 FAT ENTRY 在 FAT 中的偏移量
	mov	bx, [BPB_BytsPerSec]
	div	bx			                        ; dx:ax / BPB_BytsPerSec  ==>	ax <- 商
	push dx
	mov	bx, 0			                    ; bx <- 0 所以 es:bx = (LOADER_FILE_BASE - 100):00
	add	ax, FAT12_SECTOR_NO_OF_FAT1	        ; 此句执行之后的 ax 就是 FAT ENTRY 所在的扇区号
	mov	cl, 2
	call FUN_READ_SECTOR		            ; 读取 FAT ENTRY 所在的扇区，一次读两个
	pop	dx
	add	bx, dx
	mov	ax, [es:bx]
	cmp	byte [B_ODD], 1
	jnz	LABEL_EVEN_2
	shr	ax, 4
LABEL_EVEN_2:
	and	ax, 0FFFh

LABEL_GET_FAT_ENRY_OK:

	pop	bx
	pop	es
	ret

; ============================================================================
; 
; description:	从第 ax 个sector开始，将 el 个sector读入es:bx中
; 
; params:		al	读取的扇区数目
;				ch	柱面号
;				cl	扇区号
;				dh	磁头号
;				dl	驱动号(0表示A盘)
;				es:bx	数据缓冲区
;
; return:		cf	错误会被置位，成功会被清零
;				ah	成功会被置0
;				al 传输的扇区数目
; notes:		每一个扇区有 512 字节
;
; 2 * 80 * 18 * 512 = 1.44MB
;
; BPB_SecPerTrk
;
; 我们能够取得要读取的数据的扇区号：ax，
; cl = ax % BPB_SecPerTrk
; ch = (ax / BPB_SecPerTrk) >> 1
; dh = (ax / BPB_SecPerTrk) & 1
; ============================================================================
FUN_READ_SECTOR:          		
    push cx						; 保存要读取的扇区数目
    push bx                 	; 保存bx
    mov bl, [BPB_SecPerTrk]		; b1: 除数
    div bl                  	; al = ax / BPB_SecPerTrk; ah = ax % BPB_SecPerTrk(这里我们取到余数，也就是扇区号)
    inc ah                  	; 由于扇区是从1开始计数的，所以要给ah加1
    mov cl, ah              	; cl <- 起始扇区号
    mov dh, al              	; dh <- al
	and dh, 1					; dh & 1 = 磁头号
    shr al, 1               	; al >> 1 (其实是 y / BPB_NUM_HEADS),这里
                              	; BPB_NUM_HEADS=2
    mov ch, al              	; ch <- 柱面号
    pop bx                  	; 恢复bx
    ; 至此,*柱面号,起始扇区,磁头号*全部得到
    mov dl, [BS_DrvNum]    	; 驱动器号(0表示A盘)

    pop ax						; 取出要读取的扇区数目，我们只需要al即可，不过下一步的mov指令能够设置ah
								; 读 al 个扇区
.GO_ON_READING:
    mov ah, 2         			; 读
    int 13h
    jc .GO_ON_READING			; 如果读取错误,CF会被置1,这时不停的读,直到
								; 正确为止
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

	call INIT_KERNEL						; 移动内核代码，并且调整相应寄存器的值
											; 例如设置 ip 寄存器值为内核代码的入口地址

	; 到此停止
	; jmp $

	jmp SELECTOR_CODE32:KERNEL_ENTRY_POINT_PHY_ADDR




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
