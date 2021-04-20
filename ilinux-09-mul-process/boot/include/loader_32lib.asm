; =====================================================================
; 计算内存大小
; ---------------------------------------------------------------------
FUN_CAL_MM_SIZE:						
	push esi
	push edi
	push ecx
	push edx

	mov esi, MEM_CHK_BUF				; ds:esi 指向缓冲区
	mov ecx, [DD_MCR_COUNT]				; ecx=有多少个ADRS，记为i
.loop:									
	mov edx, 5							; ADRS有5个成员变量，记为j
	mov edi, ADRS						; ds:edi -> 一个ARDS结构
.1:										
	push dword [esi]					; 
	pop eax								; ds:eax -> 缓冲区中的第一个ARDS结构
	stosd								; 将ds:eax中的一个dword内容拷贝到ds:edi，填充ADRS结构，并edit+4 
	add esi, 4							; ds:esi -> 指向ADRS中的下一个变量
	dec edx								; j--
	
	cmp edx, 0							; 									
	jnz .1								; 如果数据没有填充完毕，将继续填充
	
	cmp dword [DD_TYPE], 1				; 
	jne .2								; 

	mov eax, [DD_BASE_ADDR_LOW]			; eax 为基地址低32位
	add eax, [DD_SIZE_LOW]				; eax = 基地址低32位 + 长度低32位 --> 这个ARDS结构指代的内存的大小
										; 为什么不算高32位，因为32位可以表示0~4G大小的内存，而32位CPU也只能到4G
										; 我们编写的是32位操作系统，高32位系统是为64位操作系统准备的，我们不需要

	cmp eax, [DD_MEM_SIZE]
	jb .2

	mov [DD_MEM_SIZE], eax				; 内存大小为 = 最后一个基地址最大的ARDS的基地址32位+长度低32位 
.2:
	loop .loop							; ecx--, jmp .loop  

	pop edx
	pop ecx
	pop edi
	pop esi

	ret
		

; =====================================================================
; 打印内存大小函数
; ---------------------------------------------------------------------
FUN_PRINT_MM_SIZE:
	push ebx
	push ecx

	mov eax, [DD_MEM_SIZE]				; 存放内存大小
	xor edx, edx

	mov ebx, 1024

	div ebx								; eax / 1024 (KB)
										; ax 为商，dx为余数
	push eax
	; 显示一个字符串"MEMORY SIZE"
	push STR_MEM_INFO
	call FUN_PRINT
	add esp, 4
	pop eax								; 重置栈顶指针
	; 将内存大小显示
	push eax							; 将一个数字压入栈
	call FUN_PRINT_INT					; 调用显示数字函数
	add esp, 4							; 重置栈顶指针

	; 显示"KB"
	push STR_KB
	call FUN_PRINT
	add esp, 4							; 重置栈顶指针

	pop ecx
	pop ebx

	ret
	

; =====================================================================
; 保护模式下：打印字符串
; arg: 字符串的地址
; ---------------------------------------------------------------------
FUN_PRINT:
	push esi
	push edi
	push ebx
	push ecx
	push edx

	mov esi, [esp + 4 * 6]				; 得到字符串地址
	mov edi, [DD_DISP_POSITION]			; 得到显示位置
	mov ah, 0x0f

.1:
	lodsb								; ds:esi -> al，esi++
	test al, al

	jz .print_end						; 遇到了0结束打印

	cmp al, 10
	jz .print_nl						; 打印换行符
	; 如果不是换行符，也不是\0，那我们认为是一个可以打印的字符
	mov [gs:edi], ax					; 将字符显示
	add edi, 2							; 调整偏移量
	jmp .1 

.print_nl:
	call FUN_PRINT_NL					; 打印换行符，也就是换行
	jmp .1 

.print_end:
	mov dword [DD_DISP_POSITION], edi	; 打印完毕更新显示

	pop edx
	pop ecx
	pop ebx
	pop edi
	pop esi

	ret


; =====================================================================
; 显示一个整数
; ---------------------------------------------------------------------
FUN_PRINT_INT:
	mov ah, 0x0F						; 设置黑底白字
	mov al, '0'
	push edi
	mov edi, [DD_DISP_POSITION]
	mov [gs:edi], ax
	add edi, 2
	mov al, 'X'
	mov [gs:edi], ax
	add edi, 2
	mov [DD_DISP_POSITION], edi			; 显示完毕后重置光标位置
	pop edi

	mov eax, [esp + 4]
	shr eax, 24
	call FUN_PRINT_AL

	mov eax, [esp + 4]
	shr eax, 16
	call FUN_PRINT_AL

	mov eax, [esp + 4]
	shr eax, 8
	call FUN_PRINT_AL

	mov eax, [esp + 4]
	call FUN_PRINT_AL

	ret



; =====================================================================
; 显示AL中的数字
; ---------------------------------------------------------------------
FUN_PRINT_AL:
	push eax
	push ecx
	push edx
	push edi
    
	mov edi, [DD_DISP_POSITION]			; 取得当前光标所在位置

	mov ah, 0x0F						; 设置黑底白字
	mov dl, al							; dl = al
	shr al, 4							; al 右移 4 位
	mov ecx, 2							; 设置循环次数

.begin:
	and al, 01111b
	cmp al, 9
	ja .1
	add al, '0'
	jmp .2

.1:
	sub al, 10
	add al, 'A'

.2:
	mov [gs:edi], ax
	add edi, 2

	mov al, dl
	loop .begin

	mov [DD_DISP_POSITION], edi			; 显示完毕后更新光标位置

	pop edi
	pop ecx
	pop edx
	pop eax

	ret

; =====================================================================
; 保护模式下：打印换行符
; ---------------------------------------------------------------------
FUN_PRINT_NL:
	push edi
	push ebx 
	push eax


	mov edi, [DD_DISP_POSITION]
	mov eax, edi
	mov bl, 160
	div bl
	
	inc eax
	mov bl, 160
	mul bl
	mov edi, eax

	mov [DD_DISP_POSITION], edi

	pop eax								; 
	pop ebx 
	pop edi 

	ret 


; =====================================================================
; 启动分页机制
; 根据内存大小来计算初始化多少的PDE以及多少的PTE，我们给每页分配4KB大小
; 32操作系统一般是为4K，（Windows）
; 注意：
;	页目录表存放在1  M（0x100000）~1.4M处（0x101000）
;	所有页表存放在1.4M（0x101000）~5.4M处（0x501000）
; ---------------------------------------------------------------------
FUN_SETUP_PAGING:
	xor edx, edx						; edx=0
	mov eax, [DD_MEM_SIZE]				; eax 为内存大小
	mov ebx, 0x400000					; 0x400000 = 4M = 4096 * 1024, 
										; 即一个页表的大小
	div ebx								; 内存大小 / 4M
	mov ecx, eax						; ecx = 页表项的个数，即PDE的个数
	test edx, edx
	jz .no_remainder					; 每有余数
	inc ecx

.no_remainder:
	push ecx							; 保存页表个数
	; ilinux 为了简化处理，所有线性地址对应相应的物理地址，并且暂不考虑内存的空间

	; 首先初始化页目录
	mov ax, SELECTOR_DATA
	mov es, ax
	mov edi, PAGE_DIR_BASE				; edi = 页目录存放的首地址
	xor eax, eax

	; eax = PDE，PG_P（该页存在），PS_US_U（用户级页表），PG_RW_W（可读、写、执行）

	mov eax, PAGE_TABLE_BASE | PG_P | PG_US_U | PG_RW_W
.setup_pde:
	stosd								; 将ds:eax中的一个dword内容拷贝到ds:edi中，填充页目录表结构

	add eax, 4096						
	loop .setup_pde

	; 现在开始初始化所有页表

	pop eax								; 取出页表个数
	mov ebx, 1024						; 每个页表可以存放1024个PTE
	mul ebx								; 页表个数 * 1024，得到需要多少个PTE
	mov ecx, eax 
	mov edi, PAGE_TABLE_BASE			; edi = 页表存放的首地址
	xor eax, eax
	; eax = PTE， 页表从物理地址 0 开始映射，所以 0x0 | 后面的属性，该句可有可无，但是这样看折比较直观
	mov eax, 0x | PG_P | PG_US_U | PG_RW_W

.setup_pte:
	stosd
	add eax, 4096
	loop .setup_pte	

	; 最后设置cr3寄存器和cr0，开启分页机制
	mov eax, PAGE_DIR_BASE
	mov cr3, eax						; 将 PAGE_DIR_BASE 存储到 cr3 中
	mov eax, cr0
	or eax, 0x80000000					; 将 cr0 中的 PG位 置位
	mov cr0, eax

	jmp short .setup_pgok				; 和进入保护模式一样，一个跳转指令使其生效，
										; 表明是一个短跳转，其实不表明也可以

.setup_pgok:
	nop									; 一个小延迟，让CPU反应一下（为什么要反应呢？）
	nop
	ret 



;----------------------------------------------------------------------------
; 方法名: MEM_CPY
; 解  释: 内存地址拷贝，将ds:esi开始的cx大小的内存空间拷贝到es:edi内存空间中 
;----------------------------------------------------------------------------
; 参  数:
;		es:edi:		目内存空间
;		ds:esi:		源内存空间
;		cx:			拷贝字节大小
;----------------------------------------------------------------------------
MEM_CPY: 
	push esi
	push edi
	push ecx

	mov edi, [esp + 4 * 4]
	mov esi, [esp + 4 * 5]
	mov ecx, [esp + 4 * 6]

.loop:
	cmp ecx, 0
	jz .loop_end
	mov al, [ds:esi]
	mov [es:edi], al
	inc edi
	inc esi
	loop .loop					; dec cx; jne .loop 

.loop_end:
	mov eax, [esp + 4 * 4]

	pop ecx
	pop edi
	pop esi

	ret


;----------------------------------------------------------------------------
; 方法名: INIT_KERNEL
; 解  释: 初始化内核
;----------------------------------------------------------------------------
;----------------------------------------------------------------------------
INIT_KERNEL:
	xor esi, esi
	xor ecx, ecx

	mov cx, word [KERNEL_PHY_ADDR + 44]			; cx = e phnum（Program Header数量）
	mov esi, [KERNEL_PHY_ADDR + 28]				; esi = Program header table 在文件中的偏移量
	add esi, KERNEL_PHY_ADDR					; esi = Program header table 在内存中的偏移量
												; ds:esi -> 第一个 Program Header 
.begin:
	mov eax, [esi + 0]							; 段类型

	cmp eax, 0
	jz .no_action								; 不可用的段
	; e_type != 0，说明它是一个可用段

	push dword [esi + 16]						; 压入段大小 size 参数
	mov eax, [esi + 4]							; 段的第一个字节在文件中的偏移
	add eax, KERNEL_PHY_ADDR					; eax -> 段的第一个字节 
	push eax									; 压入源地址 src 参数
	push dword [esi + 8]						; MEM_CPY dest 参数

	call MEM_CPY

	add esp, 4 * 3								; 清理堆栈
	; 继续下一个程序头 
.no_action:
	
	add esi, 32									; 下一个程序头，ds:esi 指向下一个 Program Header
	dec ecx
	cmp ecx, 0
	jne .begin									; 继续下一个程序头

	ret 