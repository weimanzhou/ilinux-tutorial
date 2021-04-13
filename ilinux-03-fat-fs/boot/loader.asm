org	0100h

	jmp START

TOP_OF_STACK		equ 0x100

START:
	mov ax, cs
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov sp, TOP_OF_STACK

    ; 清屏幕操作(scroll up window.)
	mov	ax, 0600h		; AH = 6,  AL = 0h
	mov	bx, 0700h		; 属性被用来在窗口底部写入空白行(BL = 07h)
	mov	cx, 0			; 屏幕左上角: (0, 0)
	mov	dx, 184fh		; 屏幕右下角: (80, 50)
	int	10h				; int 10h

	mov dh, 0
	call DISP_STR

	jmp	$		        ; 无限循环


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
