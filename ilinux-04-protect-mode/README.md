# 04保护模式

[toc]

## 预准备

在上一节中，我们成功在我们的操作系统中添加了`FAT12`文件系统，在这一节中，我们将我们的操作从**实模式**切换到**保护模式**。

由于这一次的实验是在上一次的实验的基础上添加的，所以我们直接复制上次的文件，然后在此基础上进行添加休息。

首先复制文件

```bash
$ pwd
/root/os/ilinux/
$ cp ilinux-03-fat-fs ilinux-04-protect-mode -r
```

复制后的文件具有如下的项目结构

```bash
$ cd ilinux-04-protect-mode
$ tree .
.
├── boot					; 用来存储启动相关的文件
│   ├── include				; 用来存储启动相关文件所需要的头文件
│   │   └── fat12hdr.inc
│   ├── boot.asm			; 启动扇区代码
│   └── loader.asm			; 用来测试的代码文件（后续用来加载内核代码）
├── conf					; 存储一些配置文件
│   └── bochsrc.bxrc		; bochs 所需要的配置文件
├── ilinux.img				; 系统镜像
├── Makefile				; 工程配置文件，由 make 来使用
├── README.md				; 实验指导书
└── target					; 编译后的文件
    └── boot
        ├── boot.bin		; 编译后的启动扇区
        └── loader.bin		; 编译后的 loader 文件
```

接着我们创建`boot/include/pm.inc`文件，保护模式所需要常量

```assembly
; 描述符
; usage: desciptor base, limit, attr
;                base:  dd
;                limit: dd(low 20 bits available)
;                attr:  dw(lower 4 bits of higher byte are always 0)
%macro descriptor 3
        dw %2 & 0xFFFF          ; 段界限 1                      (2 字节)
        dw %1 & 0xFFFF          ; 段基址 1                      (2 字节)
        db (%1 >> 16) & 0xFF    ; 段基址 2                      (1 字节)
        dw ((%2 >> 8) & 0x0F00) | (%3 & 0xF0FF)
                                ; 属性1 + 段界限2 + 属性2        (2 字节)
        db (%1 >> 24) & 0xFF    ; 段基址 3                      (1 字节)
%endmacro ; 共8字节

; 描述符类型
DA_32       EQU 4000h   ; 32 位段
DA_LIMIT_4K EQU 8000h   ; 粒度4K

DA_DPL0     EQU   00h   ; DPL = 0
DA_DPL1     EQU   20h   ; DPL = 1
DA_DPL2     EQU   40h   ; DPL = 2
DA_DPL3     EQU   60h   ; DPL = 3

; 存储段描述符类型
DA_DR       EQU   90h   ; 存在的只读数据段类型值
DA_DRW      EQU   92h   ; 存在的可读写数据段属性值
DA_DRWA     EQU   93h   ; 存在的已访问可读写数据段类型值
DA_C        EQU   98h   ; 存在的只执行代码段属性值
DA_CR       EQU   9Ah   ; 存在的可执行可读代码段属性值
DA_CCO      EQU   9Ch   ; 存在的只执行一致代码段属性值
DA_CCOR     EQU   9Eh   ; 存在的可执行可读一致代码段属性值

; 系统段描述符类型
DA_LDT      EQU   82h   ; 局部描述符表段类型值
DA_TaskGate EQU   85h   ; 任务门类型值
DA_386TSS   EQU   89h   ; 可用 386 任务状态段类型值
DA_386CGate EQU   8Ch   ; 386 调用门类型值
DA_386IGate EQU   8Eh   ; 386 中断门类型值
DA_386TGate EQU   8Fh   ; 386 陷阱门类型值

; 选择子类型
SA_RPL0     EQU 0       ; ┓
SA_RPL1     EQU 1       ; ┣ RPL
SA_RPL2     EQU 2       ; ┃
SA_RPL3     EQU 3       ; ┛

SA_TIG      EQU 0       ; ┓TI
SA_TIL      EQU 4       ; ┛
;----------------------------------------------------------------------------
```

## 保护模式介绍

在详细解释保护模式之前，我们先看一段代码吧（这段代码为`loader.asm`中的代码），然后顺着这段代码，我们来解释每一个新知识。

```assembly
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
; END OF [SECTION .s16]

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
	mov al, 'P'
	mov [gs:edi], ax

	; 到此停止
	jmp $

; END OF [SECTION .s32]

; 堆栈段
[SECTION .gs]
align 32
LABEL_STACK:		times 512 db 0
TOP_OF_STACK		equ $ - LABEL_STACK
; END OF [SECTION .gs]
```

下面我们来运行这个代码

```bash
$ make clean init all image run
```

可以发现屏幕成功的打印来一个红色的`P`字符，这表示我们成功的进入到来保护模式

![image-20210413150004538](https://gitee.com/wei_manzhou/blog_picture/raw/master/20210413150006.png)

可以看到，整个代码主要是四部分，也就是四个`section`，分别为：`.gdt`，`.s16`，`s32`，`.gs`。

首先我们看一下`.gdt section`。

```assembly
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
```

最开头我们开到定义了一个标签

```assembly
LABEL_GDT:				descriptor 0, 0, 0              ; 空描述符
```

不过后面这个`descriptor`是什么呢？这其实是一个宏，定义在`pm.inc`文件中。

```assembly
; 描述符
; usage: desciptor base, limit, attr
; 	base:  dd
;   limit: dd(low 20 bits available)
;   attr:  dw(lower 4 bits of higher byte are always 0)
%macro descriptor 3
        dw %2 & 0xFFFF          ; 段界限 1 (2 字节)
        dw %1 & 0xFFFF          ; 段基址 1 (2 字节)
        db (%1 >> 16) & 0xFF	; 段基址 2 (1 字节)
        dw ((%2 >> 8) & 0x0F00) | (%3 & 0xF0FF)
                                ; 属性1 + 段界限2 + 属性2	(2 字节)
        db (%1 >> 24) & 0xFF	; 段基址 3 (1 字节)
%endmacro ; 共8字节
```

看到这里又多出来了奇奇怪怪的东西，我们先忽略宏的内容，可以看到宏的整个结构如下

```assembly
; macro BEGIN
%macro <macro_name> <param_count>
        %1
        %2
        %3
        ...<内容>
%endmacro
; macro END
```

首先是一个`%macro`标志，然后跟上宏的名词接着为宏的参数个数，最后以`%endmacro`结束宏的定义。那么我们怎么能够在宏内容里面取到参数呢，可以通过`%n`的格式取到第`n`个参数。

在简要介绍完宏的结构后，我们再来深入一下`GDT`，也就是如上宏的内部结构。

`GDT`，也就是`Global Descriptor Table`，全局描述符表。在实模式下，16位的寄存器需要使用`段:偏移`的形式来达到，1MB的寻址空间，在高的内存就无法寻址到了。而保护模式下，我们借助`GDT`实现了，一个寄存器就可以达到4GB的寻址，我们通过维护一个结构，也就是`GDT`（实际上还有`LDT`），然后让`寄存器`指向这个结构，这个结构定义了段的：

- 起始地址
- 界限
- 属性（访问权限等）

每一个`GDT`的表项也有一个专门的名词，叫做描述符（descriptor）。

接下来我们解释描述符。首先我们看一下描述符的结构。

![GDT](https://gitee.com/wei_manzhou/blog_picture/raw/master/20210331160255.png)

看着结构很复杂，不过我们可以注意到`段基址`和`段界限`出现了好几次。寻址模式仍然是`段基址:段界限`的方式。由于历史的原因，他们被拆开来放。剩下的都是一些属性值，我们用到的时候再解释他们。

这里我们再看`descriptor`这个宏，这个宏使用参数传入的方法，将`基址`、`界限`、`属性`这个三个参数赋值到相应的位置处。

好，现在我们回到`[SECTION .gdt]`这里，可以看到一共定义了三个描述符，为方便起见，我们分别称他们为DESC_CODE32，DESC_VIDEO，DESC_GDT段。现在，我们来看看DESC_VIDEO，它的基址是0xB8000。顾名思义，这个描述符指向的正是显存。

现在我们已经知道了，GDT中的每一个描述符定义一个段，那么，CS，DS等寄存器怎么跟如何和对应的段对应起来的呢？你可能已经注意到了，在段[SECTION .32]中有两句代码是这样的：

```assembly
        mov ax, SELECTOR_VIDEO
        mov gs, ax                                              ; 视频段选择子(目的)
```

看上去，段寄存GS的值变成了SELECTOR_VIDEO，我们可以在上文中可以看到，SELECTOR_VIDEO是这样定义的：

```assembly
SELECTOR_VIDEO      equ LABEL_DESC_VIDEO	- LABEL_GDT
```

直观的看，它是LABEL_DESC_VIDEO相对于LABEL_GDT的偏移。实际上，它有一个专门的名称，叫做选择字（SELECTOR），他不是一个偏移，而是稍微复杂一点，它的结构如下图所示。

![工作簿1](https://gitee.com/wei_manzhou/blog_picture/raw/master/20210331151343.png)

不难理解，当TI和RPL都为零时，选择子就变成了对应描述符相对于基址的偏移，就好像我们程序中那样。

在程序快要结束的位置，我们可以看到这样一行。

```assembly
        mov [gs:edi], ax
```

这里，我们就明白了，GS的值就是SELECTOR_VIDEO，它指示对应显存的描述符DESC_VIDEO，这条指令将把AX的值写入显存中偏移位EDI的位置。

可能你注意到了，上图中“段:偏移”形式的逻辑地址（Logical Addess）经过段机制转化成“线性地址”（Linear Address），而不是“物理地址”（Physical Addres）， 其中的原因我们以后会提到。在上面的程序中，线性地址就是物理地址。另外，包含描述符的，不仅可以是GDT，也可以是LDT。

接着我们来讲解`[SECTION .s16]`，在这段代码中，我们将实现从实模式到保护模式的转换，我们首先看一下

```assembly
        mov ax, cs
        mov ds, ax
        mov es, ax
        mov ss, ax
        mov sp, 0x0100
```

这里主要是初始化一些寄存器，等价的C代码如下

```c
ss = es = ds = cs;
sp = 0x0100
```

设置`ss`、`es`、`ds`的值为`cs`寄存器，栈顶为0x0100.

接着我们看到初始化32位代码段描述符这一段

```assembly
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
```

这里有些人可能会疑惑，为什么我们之前使用宏来初始化DESC_CODE32，这里为什么还要初始化呢，这是因为采用宏的方式它的基地址没有设置正确，从代码来看，基地址不是0，但是宏是0，所以这里我们要来修改基地址。首先，我们使用`xor`异或操作操作清空`eax`寄存器（32位的ax寄存器），然后将代码段寄存器`cs`的值复制到`ax`寄存器中，这个时候为了计算出线性地址，使用`shl`左移操作，将`eax`的值左移4位，然后加上`[SECTION .32]`的偏移值

```assembly
        ; 初始化32位代码段描述符
        xor eax, eax
        mov ax, cs
        shl eax, 4
        ; 将[SECTION .32]这个段的物理地址赋给eax
        add eax, LABEL_SEG_CODE32
```

在这一步完成后，`eax`的值为`[SECTION .32]`代码的线性地址，根据`descriptor`的结构，我们首先将`eax`的低16位，赋值到`[LABEL_DESC_CODE32 + 2]`，也就是

![image-20210331155028387](https://gitee.com/wei_manzhou/blog_picture/raw/master/20210331155032.png)

接着，使用`shr`右移操作符将`eax`寄存器右移16位，然后将低八位赋值给`[LABEL_DESC_CODE32 + 4]`，也就是

![image-20210331155251063](https://gitee.com/wei_manzhou/blog_picture/raw/master/20210331155252.png)

接下来就剩下最后一段代码了

```assembly
        mov byte [LABEL_DESC_CODE32 + 7], ah
```

将`ah`，也就是`ax`的高八位赋值给`[LABEL_DESC_CODE32 + 7]`处，也就是

![image-20210331155357742](https://gitee.com/wei_manzhou/blog_picture/raw/master/20210331155359.png)

在完成32位代码段的初始化后，我们开始为加载`gdtr`做准备

```assembly
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
```

首先仍然是通过`xor`异或操作将`eax`寄存器清0，然后将`ds`寄存器的值复制到`ax`寄存器中，然后通过`shl`左移操作将`eax`寄存器左移4位，接着使用`add`指令加上将`LABEL_GDT`偏移值，到这一步，完成了`GDT`全局描述符表寄存器的地址初始化，然后将`eax`寄存器的值复制到`GDT_PTR`中，也就是如下图所示。

![GDT2_32](https://gitee.com/wei_manzhou/blog_picture/raw/master/20210331162115.png)

值准备好`GDT`和`GDTR`后，下一步我们开始准备跳转到32位代码处了。

```assembly
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
                                                ; 装入CS,并跳转到
```

整个过程为如下：

1. 使用`lgdt`指令将`[GDT_PTR]`的内容赋值给`gdt`寄存器
2. 使用`cli`关闭中断
3. 将`a20`地址线的低2位置1
4. 将`cr0`寄存器的最低位置1
5. 使用`jmp`指令跳转到32位代码处

到这部为止，我们已经完成了从实模式到保护模式的切换。

接着我们来解释最后一段的代码，也就是`[SECTION .s32]`

```assembly
[SECTION .s32]  ; 32位代码段,由实模式跳入
[BITS 32]
LABEL_SEG_CODE32:
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
        mov al, 'P'
        mov [gs:edi], ax


        ; 到此停止
        jmp $
```

整个功能是向显存中写入黑底红字`P`字符，下面我们来解释每一句代码。

```assembly
        mov ax, SELECTOR_VIDEO
        mov gs, ax                                              ; 视频段选择子(目的)
```

该操作在上面已经讲过略过去。

在解释下面这段之前，我们先来讲解一下对显存的操作。

```assembly
        mov edi, (80 * 10 + 0) * 2              ; 屏幕第10行,第0列
        mov ah, 0x0c                            ; 0000: 黑底 1100: 红字
        mov al, 'P'
        mov [gs:edi], ax
```

我们已经完成来对新添加的代码的解释。

下面我们总结一些这一个实验，在这个实验中我们成功的实现来从实模式到保护模式的切换，并成功的操作显存向屏幕输出了字符。