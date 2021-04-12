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
%include "pm.inc"

LABEL_START:
    mov ax, cs
    mov ds, ax
    mov ss, ax
    mov sp, STACK_BASE

    ; 进行磁盘复位
    xor ah, ah
    xor dl, dl
    int 13h

FUN_LOAD_LOADER_FILE:
    ; 下面在A盘的根目录寻找loader.bin
    mov word [W_SECTOR_NO], FAT12_SECTOR_NUM_OF_ROOT_DIR
LABEL_SEARCH_IN_ROOT_DIR_BEGIN:
    cmp word [W_ROOT_DIR_SIZE_FOR_LOOP], 0  ; 判断根目录区是否已经读完,
    jz LABEL_NO_LOADERBIN                   ; 如果读完表示没有找到loader.bin
    dec word [W_ROOT_DIR_SIZE_FOR_LOOP]
    mov ax, LOADER_FILE_BASE
    mov es, ax
    mov bx, LOADER_FILE_OFFSET
    mov ax, [W_SECTOR_NO]
    mov cl, 1
    call FUN_READ_SECTOR

	mov si, LOADER_FILE_NAME
    mov di, LOADER_FILE_OFFSET

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

    ; 进行清屏操作
    ; mov ax, 0x0600
    ; mov bx, 0x0700
    ; mov cx, 0
    ; mov dx, 0x0184f
    ; int 0x10

    ; mov cx, 0x09
    ; push cs
    ; pop es
    ; mov bp, BOOT_MESSAGE

	mov dh, 1
	call FUN_DISP_STR

    jmp $                       ; 跳转到当前地址，实现循环

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
	; push bp
    ;mov bp, sp
    ;sub esp, 2              	; 劈出2字节的堆栈区域保存要读的扇区数
                            	; byte [bp-2]
    ;mov byte [bp-2], cl
    push cx
    push bx                 	; 保存bx
    mov bl, [BPB_SecPerTrk]		; b1: 除数
    div bl                  	; y在al中,z在ah中
    inc ah                  	; z++
    mov cl, ah              	; cl <- 起始扇区号
    mov dh, al              	; dh <- y
    shr al, 1               	; y >> 1 (其实是 y / BPB_NUM_HEADS),这里
                              	; BPB_NUM_HEADS=2
    mov ch, al              	; ch <- 柱面号
    and dh, 1               	; dh & 1 = 磁头号
    pop bx                  	; 恢复bx
    ; 至此,*柱面号,起始扇区,磁头号*全部得到
    mov dl, [BS_DrvNum]    	; 驱动器号(0表示A盘)

    pop ax
.GO_ON_READING:
    mov ah, 2         			; 读
;    mov al, byte [bp-2]			; 读al个扇区
    int 13h
    jc .GO_ON_READING			; 如果读取错误,CF会被置1,这时不停的读,直到
								; 正确为止
    add esp, 2
    ;pop bp
    ret

FUN_DISP_STR:
    mov ax, MESSAGE_LENGTH
    mul dh
    add ax, BOOT_MESSAGE
    mov bp, ax
    mov ax, ds
    mov cx, MESSAGE_LENGTH
    mov ax, 0x1301
    mov bx, 0x0007
    mov dl, 0
    int 10h
    ret

; 为简化代码,下面每个字符串的长度均为MESSAGE_LENGTH
MESSAGE_LENGTH	         		equ 9
BOOT_MESSAGE            		db "Booting  "
MESSAGE1                        db "Ready.   "
MESSAGE2                        db "NO LOADER"

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