; define _BOOT_DEBUG_
%ifdef _BOOT_DEBUG_
        org 0x0100                                      ; 调试状态，做成 .com 文件，可调试
%else
        org 0x7c00                                      ; Boot 状态，BIOS将把 bootector
                                                        ; 加载到 0:7c00 处并开始执行
%endif

jmp short LABEL_START
nop

%include "fat12hdr.inc"


; 栈顶地址
STACK_TOP                      equ 0x7c00

LABEL_START:
    mov ax, cs
    mov ds, ax
    mov	es, ax
    mov ss, ax
    mov sp, STACK_TOP

    ; 清屏幕操作(scroll up window.)
	mov	ax, 0600h		; AH = 6,  AL = 0h
	mov	bx, 0700h		; 属性被用来在窗口底部写入空白行(BL = 07h)
	mov	cx, 0			; 屏幕左上角: (0, 0)
	mov	dx, 184fh		; 屏幕右下角: (80, 50)
	int	10h				; int 10h

	mov	dh, 0			; "booting.."
	call FUN_DISP_STR	; 调用显示字符串函数

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
    mov ax, LOADER_FILE_BASE				;
    mov es, ax								; 设置缓冲区段地址 
    mov bx, LOADER_FILE_OFFSET				; 设置缓冲区偏移地址
    mov ax, [W_SECTOR_NO]					; 设置从第 ax 个扇区读取
    mov cl, 1								; 读取扇区的个数
    call FUN_READ_SECTOR					; 调用读取扇区

	mov si, LOADER_FILE_NAME                ; ds:si -> "LOADER  BIN"
    mov di, LOADER_FILE_OFFSET              ; es:di -> BaseOfLoader:0100 = BaseOfLoader*10h+100

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
    mov si, LOADER_FILE_NAME
    jmp LABEL_SEARCH_FOR_LOADERBIN

LABEL_GOTO_NEXT_SECTOR_IN_ROOT_DIR:
    add word [W_SECTOR_NO], 1
    jmp LABEL_SEARCH_IN_ROOT_DIR_BEGIN

LABEL_NO_LOADERBIN:
    mov dh, 2
    call FUN_DISP_STR

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
	mov	ax, LOADER_FILE_BASE
	mov	es, ax			                    ; es <- LOADER_FILE_BASE
	mov	bx, LOADER_FILE_OFFSET	            ; bx <- LOADER_FILE_OFFSET
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
	; 文件找到并加载之后，跳转到这个位置运行
	jmp	LOADER_FILE_BASE:LOADER_FILE_OFFSET	; 


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

FUN_DISP_STR:
    mov ax, MESSAGE_LENGTH
    mul dh
    add ax, BOOT_MESSAGE
    mov bp, ax
    mov ax, ds
	mov	es, ax
    mov cx, MESSAGE_LENGTH
    mov ax, 0x1301
    mov bx, 0x0007
    mov dl, 0
    int 10h
    ret

; 为简化代码,下面每个字符串的长度均为MESSAGE_LENGTH
MESSAGE_LENGTH	         		equ 9
BOOT_MESSAGE            		db "booting.."
MESSAGE1                        db "ready...."
MESSAGE2                        db "no loader"
MESSAGE3						db "founded.."
MESSAGE4						db "loaded..."

; ============================================================================
; time n m
;       n: 重复多少次
;       m: 重复的代码
;
; $      : 当前地址
; $$ : 代表上一个代码的地址减去起始地址
; ============================================================================
times 510-($-$$) db 0   ; 填充 0
dw 0xaa55                               ; 可引导扇区标志,必须是 0xaa55,不然bios无法识别