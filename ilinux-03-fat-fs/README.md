# 04文件系统

[toc]

在上一个实验中，我们已经写了一个启动扇区，但是只有512字节的空间无法满足我们写一个操作系统的需求，为了这个目标，下面我们来为操作系统添加一个文件系统吧。

这次我们要使用到的是FAT12文件系统，下面我们来使用这个文件系统吧，在具体使用这个文件系统之前，我们先简单的介绍一下这个文件系统。

## FAT12文件系统

### 文件系统简介

FAT12文件系统是指：在磁盘上规定一种特定的存储格式，这种存储格式高效方便，功能强大，因此形成了统一的规定。

### 文件系统基础知识

具体来说FAT12文件系统为1.44M的软盘设计。1.44M的软盘有2880个扇区，一个扇区有512个字节；那么FAT12文件系统的管理的空间大小就是2880 * 512 = 1474560个字节

实际的扇区是在磁盘上，但是我们可以把所有的储存空间看作是一个很大的数组，把扇区当作是连续排列的。扇区当作数组看待时，称为逻辑扇区，从0开始编号。

### FAT12文件系统结构

首先FAT12文件系统将2880个扇区分成5个部分：**MBR引导记录、FAT1表、FAT2表、根目录、数据区**

![fat12](https://gitee.com/wei_manzhou/blog_picture/raw/master/20210413141400.png)

### MBR引导记录

MBR引导记录有512个字节，最后两个字节是0x55和0xAA；MBR记录的所有信息如下

MBR引导记录就是BOIS读入内存的一个扇区，可以看到第一条指令就是汇编语言x86中的跳转指令，这条指令将跳转到后面的汇编代码区域进行执行。由于我们在做模拟FAT12文件系统程序时不需要考虑后面一段代码，因此后面的汇编代码通常为0。

真正要注意的地方就是前面的一些FAT12文件的参数。之后为了方便说明统一用默认值考虑FAT12文件系统，在DOS系统中这些值基本上也是固定的。

### FAT1、FAT2表

这两个表完全相同，FAT2表存在的目的就是为了修复FAT1表。因此在实际操作的过程中可以将FAT1表在关机的时候赋值给FAT2即可。以后再说FAT表时，默认为FAT1表

### 根目录区的每一个条目

根目录区来存储每一个文件的简单信息，类似于目录的功能，后续我们查找文件的时候需要利用到这个地方。

![img](https://img-blog.csdnimg.cn/20190315171036393.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzM5NjU0MTI3,size_16,color_FFFFFF,t_70)

## 代码示例

首先我们新创建一个文件夹用来完成这次的实验

```bash
$ pwd
/root/os/ilinux/
$ mkdir ilinux-03-fat-fs
$ cd ilinux-03-fat-fs
```

为了组织我们的代码，我们建立了如下的目录结构

```bash
$ tree .
.
├── boot					; 用来存储启动相关的文件
│   ├── include				; 用来存储启动相关文件所需要的头文件
│   │   ├── fat12hdr.inc
│   │   └── pm.inc
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

我们可以看到上面多了很多我们不知道的文件，仿佛一些子变的很难，其实是没有的，只是为了后续我们组织成这样，下面我们来一步一步实现上面的结构吧。

第一步，我们先创建一个`boot/include/fat12hdr.inc`文件用来定义文件系统需要的一些常量，内容如下。

```assembly
BS_OEMName      DB '---XX---'   		; OEM string, 必须是 8 个字节
BPB_BytsPerSec  DW 0x0200               ; 每扇区字节数
BPB_SecPerClus  DB 0x01                 ; 每簇多少个扇区
BPB_RsvdSecCnt  DW 0x01                 ; Boot 记录占用多少扇区
BPB_NumFATs     DB 0x02                 ; 共有多少 FAT 表
BPB_RootEntCnt  DW 0xe0                 ; 根目录文件最大数
BPB_TotSec16    DW 0x0b40               ; 逻辑扇区总数
BPB_Media       DB 0xF0                 ; 媒体描述符
BPB_FATSz16     DW 0x09                 ; 每FAT扇区数
BPB_SecPerTrk   DW 0x12                 ; 每磁道扇区数
BPB_NumHeads    DW 0x02                 ; 磁头数（面数）
BPB_HiddSec     DD 0                    ; 隐藏扇区数
BPB_TotSec32    DD 0                    ; 如果 wTotalSectorCount 是 0 由这个值记录扇区数
BS_DrvNum       DB 0                    ; 中断13的驱动器号
BS_Reserved1    DB 0                    ; 未使用
BS_BootSig      DB 0x29                 ; 扩展引导标记（29h）
BS_VolID        DD 0                    ; 卷序列号
BS_VolLab       DB 'snowflake01'		; 卷标，必须11个字节
BS_FileSysType  DB 'FAT12'              ; 文件系统类型，必须8个字节
```

为了开发的更加便利，我们创建一个`boot/include/pm.inc`文件并中其中添加一些常量

```assembly
; 加载的 LOADER.bin 文件的名词
LOADER_FILE_NAME                db "LOADER  BIN", 0   
; LOADER.bin 文件加载到内存的基址
LOADER_FILE_BASE          		equ 0x9000
; LOADER.bin 
LOADER_FILE_OFFSET        		equ 0x0100

; FAT12 根目录占用扇区数目
FAT12_ROOT_DIR_SECTORS        	equ 14
; FAT12 根目录区的起始扇区号
FAT12_SECTOR_NUM_OF_ROOT_DIR    equ 19
W_ROOT_DIR_SIZE_FOR_LOOP        dw FAT12_ROOT_DIR_SECTORS
W_SECTOR_NO                     dw 0
B_ODD                           db 0
```

然后我们在`boot/boot.asm`中编写如下的代码，并引入我们的文件系统（要注意代码中的注释）

```assembly
; define _BOOT_DEBUG_
%ifdef _BOOT_DEBUG_
        org 0x0100				; 调试状态，做成 .com 文件，可调试
%else
        org 0x7c00				; Boot 状态，BIOS将把 bootector
								; 加载到 0:7c00 处并开始执行
%endif

jmp short LABEL_START			; 这里三行的结构一定要是这样
nop								; 根据 fat12 所需要参数的要求，我们首先要有一个跳转指令，然后跟上一个
								; nop 空指令，接着跟上 fat12hdr.inc 中常量，这就表明我们引入了
%include "fat12hdr.inc"			; fat12 文件系统

%include "pm.inc"

LABEL_START:
    mov ax, cs
    mov ds, ax
    mov	es, ax
    mov ss, ax
    mov sp, STACK_BASE

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
	mov dh, 3
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
    ; and dh, 1               	; dh & 1 = 磁头号
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
```

修改后的`boot.asm`文件如上，下面我们进行编译。

为了能够编译文件，我们这里需要创建一个`Makefile`文件来定义编译规则，`Makefile`内容如下

```makefile

```



首先我们创建一个`loader.asm`文件，具有输出字符串的功能，内容如下

```assembly
org	0100h

	jmp START

BASE_OF_STACK		equ 0x100

START:
	mov ax, cs
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov sp, BASE_OF_STACK

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
```

到这一步我们就获取到了我们需要的`loader.asm`文件了。为了能够获取一个镜像文件，我们需要使用如下命令生成一个镜像文件

```bash
$ bximage
========================================================================
                                bximage
                  Disk Image Creation Tool for Bochs
          $Id: bximage.c 11315 2012-08-05 18:13:38Z vruppert $
========================================================================

Do you want to create a floppy disk image or a hard disk image?
Please type hd or fd. [hd] fd <-- 这里选择 fd

Choose the size of floppy disk image to create, in megabytes.
Please type 0.16, 0.18, 0.32, 0.36, 0.72, 1.2, 1.44, 1.68, 1.72, or 2.88.
 [1.44] <-- 直接回车即可
I will create a floppy image with
  cyl=80
  heads=2
  sectors per track=18
  total sectors=2880
  total bytes=1474560

What should I name the image?
[a.img] ilinux.img <-- 输入软盘的文件名

Writing: [] Done.

I wrote 1474560 bytes to boot.img.

The following line should appear in your bochsrc:
  floppya: image="boot.img", status=inserted
```

好，到目前为止我们所需要的代码文件都已经编写成功来，那我们就来编译运行它们吧。

首先我们创建一个`Makefile`文件，其中定义来一些命令，以及中间文件的生成规则。

```makefile
# ===================================================
# ---------------------------------------------------
# 变量
# ---------------------------------------------------
# 编译中间目录

# bochs 配置文件
BOCHS_CONFIG    = bochsrc.bxrc
BOCHS_ARGA		= -f $(BOCHS_CONFIG)

FD 				= ilinux.img
IMG_MOUNT_PATH	= /media/floppy0/

# 所需要的汇编器以及汇编参数
ASM				= nasm
ASM_PARA		= -i boot/include/ -f elf -f bin
QEMU 			= qemu-system-x86_64

# ===================================================
# 目标程序以及编译的中间文件
# ---------------------------------------------------
ILINUX_BOOT     = target/boot/boot.bin
ILINUX_LOADER 	= target/boot/loader.bin

# ===================================================
# 所有的功能
# ---------------------------------------------------
.PHONY: nop all image debug run clean
# 默认选项（输入make但是没有子命令）
nop:
	@echo "init                     创建所有必须的文件夹）"
	@echo "all                      编译所有文件，生成目标文件（二进制文件，boot.bin）"
	@echo "image                    生成镜像文件ilinux.img"
	@echo "debug                    打开bochs运行系统并调试"
	@echo "run                      提示用于如何将系统安装到虚拟机中"
	@echo "clean                    清理文件"

init:
	mkdir target/boot -p

all: $(ILINUX_BOOT)
	@echo "boot.bin 生成成功"

image: $(FD) $(ILINUX_BOOT) $(ILINUX_LOADER)
	dd if=$(ILINUX_BOOT) of=$(FD) bs=512 count=1 conv=notrunc
	sudo mount -o loop $(FD) $(IMG_MOUNT_PATH)
	cp $(ILINUX_LOADER) $(IMG_MOUNT_PATH)
	sudo umount $(IMG_MOUNT_PATH)
	@echo "ilinux.img 生成成功"


# 打开bochs进行调试
debug: $(ILINUX_BOOT) $(BOCHS_CONFIG)
	bochs $(BOCHS_ARGA)

run: $(FD)
	$(QEMU) -drive file=$<,if=floppy

clean:
	rm -r target

# ===================================================
# 目标文件生成规则
# ---------------------------------------------------
#  软件镜像不存在时，将会自动生成
$(FD):
	dd if=/dev/zero of=$(FD) bs=512 count=2880

$(ILINUX_BOOT): boot/boot.asm
	$(ASM) $(ASM_PARA) $< -o $@
$(ILINUX_LOADER): boot/loader.asm
	$(ASM) $(ASM_PARA) $< -o $@
```

接着我们使用`make init`来创建编译后的文件存储所需要的文件夹

```bash
$ make init
```

然后使用`make all`来编译文件

```bash
$ make all
```

接着生成系统镜像

```bash
$ make image
```

此时我们使用`file`命令查看`ilinux.img`，显示信息如下

```bash
$ file ilinux.img
boot.img: DOS/MBR boot sector, code offset 0x4a+2, OEM-ID "---XX---", root entries 224, sectors 2880 (volumes <=32 MB), sectors/FAT 9, sectors/track 18, serial number 0x0, label: "snowflake01", FAT (12 bit)
```

会发现这些信息刚好是我们定义在`fat12hdr.inc`中的信息。现在文件系统所需要的常量全部引入进来了，那我们就利用文件系统来存储文件以及加载文件吧。

然后我们需要将这个文件拷贝到我们之前创建好的`ilinux.img`中，为了将`loader.bin`文件复制到`ilinux.img`中，首先我们将`ilinux.img`挂载到我们系统上

这个时候我们的`loader.bin`文件已经被复制到`ilinux.img`中了，为了验证这一点我们使用`hexdump`命令来查看`ilinux.img`中的内容，由于根目录区从第19扇区开始，每个扇区512字节，所以第一个字节位于偏移19*512=9728=0x2600。

```bash
$ hexdump -C -n 64 -s 0x2600 ilinux.img
00002600  4c 4f 41 44 45 52 20 20  42 49 4e 20 00 00 00 00  |LOADER  BIN ....|
00002610  00 00 00 00 00 00 ef 78  8c 52 03 00 0f 00 00 00  |.......x.R......|
```

我们发现上面有一个`LOADER  BIN`字符串，说明我们的文件被复制到了该`img`中。

现在我们运行代码，看能否成功的加载到`loader.bin`文件。

```bash
$ make run
```

![image-20210413103946116](https://gitee.com/wei_manzhou/blog_picture/raw/master/20210413103947.png)

能看到`founded`，说明我们成功的找到`loader.bin`文件啦。也就说明我们的文件系统成功的应用成功。

既然已经能够找到`loader.bin`文件了，下一步我们想要把该文件加载到内存中并运行它。下面我们修改`boot.asm`文件，修改成如下：

```assembly
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
    mov	es, ax
    mov ss, ax
    mov sp, STACK_BASE

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
```

接着我们使用`make`来编译运行它

```bash
$ # 清楚上一个版本
$ make clean
$ # 创建所需要的文件夹
$ make init
$ # 编译文件
$ make all
$ # 生成镜像文件
$ make image
$ # 运行
$ make run
```

也可以用一条命令替换上面的

```bash
$ make clean init all image run
```

成功运行

![image-20210413135813571](https://gitee.com/wei_manzhou/blog_picture/raw/master/20210413135820.png)

说明我们成功的利用`fat12`文件系统存储文件，以及利用汇编找到该文件，并加载其到内存，然后成功的运行来该文件。

## 参考文档

- [INT 13h / AH = 02h](http://www.ablmcc.edu.hk/~scy/CIT/8086_bios_and_dos_interrupts.htm#int13h_02h)：读取扇区中断
- [INT 13h / AH = 00h](http://www.ablmcc.edu.hk/~scy/CIT/8086_bios_and_dos_interrupts.htm#int13h_00h)：磁盘复位系统中断
- [INT 10h / AH = 06h](http://www.ablmcc.edu.hk/~scy/CIT/8086_bios_and_dos_interrupts.htm#int10h_06h)：滚屏中断，用来清屏

