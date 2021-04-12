# 02启动扇区

[toc]

## 首先编写代码

在**01-ilinux-bootsect**文件夹下创建**boot.asm**

```bash
$ mkdir ilinux-02-bootsect
$ cd ilinux-02-bootsect
$ vim boot.asm
```

编写代码如下

```assembly
; int 10h / ah = 0x13 document http://www.ablmcc.edu.hk/~scy/CIT/8086_bios_and_dos_interrupts.htm#int10h_13h
org 0x7c00

jmp START

stack_base equ 0x7c00

START:
    mov ax, cs
    mov ds, ax
    mov ss, ax
    mov sp, stack_base

    ; 打印字符 Booting...
    mov al, 1           ; write mode bit1: [包含属性的字符串[颜色等]]
    mov bh, 0           ; page number
    mov bl, 0x07    	; 黑底白字
    mov cx, boot_message_len - boot_message
    					; 9 个字符
    mov dl, 0x00        ; column[首字符出现的列]
    mov dh, 0x00        ; row[首字符出现的行]
    push cs             ; es:bp points to string to be printed
    pop es
    mov bp, boot_message
    mov ah, 0x13
    int 0x10            ; 调用中断，实现文字显示
    jmp $               ; 跳转到当前地址，实现循环

boot_message:   dd "hello os", 0
						; 定义一个字符串为 dd 类型，并且后要添加一个 0，表示字符串的结束
boot_message_len:       ; boot_message_len - boot_message 刚好为字符串的大小

times 510 - ($ - $$) db 0
						; 填充 0 ，$ 代表当前地址，$$ 代表上一个代码的地址减去结尾地址
dw 0xaa55               ; 标志这个扇区为一个启动扇区
```

在下面会讲解每一段代码的意思。

在完成代码的编写之后，我们需要编译编码，为了后续方便，我们这里直接使用`make`来编写编译规则，我们创建一个`Makefile`文件，内容如下

```makefile
# ===================================================
# ---------------------------------------------------
# 变量
# ---------------------------------------------------
# 编译中间目录

# bochs 配置文件
BOCHS_CONFIG    = bochsrc.bxrc
BOCHS_ARGA		= -f $(BOCHS_CONFIG)

# 所需要的汇编器以及汇编参数
ASM				= nasm
ASM_PARA		= -f bin

# ===================================================
# 目标程序以及编译的中间文件
# ---------------------------------------------------
ILINUX_BOOT     = boot.bin

# ===================================================
# 所有的功能
# ---------------------------------------------------
.PHONY: nop all image debug run clean
# 默认选项（输入make但是没有子命令）
nop:
	@echo "all                      编译所有文件，生成目标文件（二进制文件，boot.bin）"
	@echo "debug                    打开bochs运行系统并调试"
	@echo "run                      提示用于如何将系统安装到虚拟机中"
	@echo "clean                    清理文件"

all: $(ILINUX_BOOT)

# 打开bochs进行调试
debug: $(ILINUX_BOOT) $(BOCHS_CONFIG)
	bochs $(BOCHS_ARGA)

run: $(ILINUX_BOOT)
	qemu-system-x86_64 $<

clean:
	rm -r $(ILINUX_BOOT)

# ===================================================
# 目标文件生成规则
# ---------------------------------------------------
#  软件镜像不存在时，将会自动生成
$(FD):
	dd if=/dev/zero of=$(FD) bs=512 count=2880

$(ILINUX_BOOT): boot.asm
	$(ASM) $(ASM_PARA) $< -o $@
```

我们可以发现，如上有四个命令：`all`，`debug`（后续我们会提到这个），`run`，`clean`命令

我们使用`all`命令生成`boot.bin`文件

```bash
$ make all
boot.bin 生成成功
$ file boot.bin
boot: DOS/MBR boot sector
```

使用`file`命令查看`boot.bin`文件，发现其是一个启动扇区。

下面我们使用`run`命令，来运行这个程序

```bash
$ make run
```

这时，会有一个窗口弹出。屏幕输出**hello os**即表示成功！

![image-20210330163814017](https://gitee.com/wei_manzhou/blog_picture/raw/master/20210330163815.png)

## 代码详解

在代码讲解开始，我们先解释一下计算机从通电到OS启动经历了哪些步骤：

1. X86 PC机开机时，CPU处于实模式，这时候内存的计算方式是 `段基址 << 4 + 段内偏移`
2. CPU的第一条指令是通过`CS：IP`来取得，而此时CS=0xFFFF，IP=0x0000。这是硬件设定好的。
3. 所以最开始执行的指令地址就是0xFFFF0，这个内存地址映射在主板的BIOS ROM（只读存储区）中。
4. ROM中的程序会检测RAM、键盘、显示器、软硬磁盘是否正确工作。同时会从地址0开始设置BIOS的中断向量表。
5. ROM中的程序继续执行，将**启动设备**磁盘0磁道0扇区1，一个512字节的扇区读到内存0x07c00处。并检查这个扇区的最后两个字节是否是`0xaa55`
6. 如果检查正确则设置cs=0x07c0，ip=0x0000，然后运行第一个扇区的代码，否则按照预设的顺序加载其它设备
7. ROM中的程序执行结束，转到0x07c00处开始执行。

最开始我们看到

```assembly
org 0x7c00
```

这段代码是告诉编译器，这段代码是要加载到0x7c00处，并且编译器在编译代码的时候如果涉及到变量寻址操作是会自动加上0x7c00这个基址的。

接下来看到

```assembly
jmp START
```

这个代码是一个跳转指令，也就是跳转到标签为`START`的代码处。

接下来是

```assembly
stack_base equ 0x7c00
```

这里使用到`equ`，这个是nasm汇编提供的一个定义常量的指令，等价的c语言为`int stack_base = 0x7c00`

接着是

```assembly
START:
    mov ax, cs
    mov ds, ax
    mov ss, ax
```

在这里我们看到了`START`标签，前面的`jmp`指令也就是跳转到这里执行，通过ROM的处理此时`cs=0x7c00`，这里我们又接触了`mov`指令以及一堆寄存器，下面以表格的形式列出

| 寄存器 | 功能         | 类别       |
| ------ | ------------ | ---------- |
| cs     | 代码段寄存器 | 段寄存器   |
| ds     | 数据段寄存器 | 段寄存器   |
| ss     | 栈段寄存器   | 段寄存器   |
| sp     | 栈顶指针     | 段寄存器   |
| ax     |              | 通用寄存器 |

实现是将`cs`的值复制到`ax`通用寄存器中，然后依次将`ax`的值赋值给`ds`和`ss`。等价的c代码为

```c
ss = ds = cs
```

接着我们看到

```assembly
    mov sp, stack_base
```

这里我们可以发现使用到了我们之前定义的一个常量，并将它赋值到`sp`寄存器中，等价c代码为`sp = 0x7c00`

接着我们看到了很长的一段汇编代码

```assembly
    ; 打印字符 Booting...
    mov al, 1           ; write mode bit1: [包含属性的字符串[颜色等]]
    mov bh, 0           ; page number
    mov bl, 0x07    	; 黑底白字
    mov cx, boot_message_len - boot_message
    					; boot_message_len - boot_message 个字符
    mov dl, 0x00        ; column[首字符出现的列]
    mov dh, 0x00        ; row[首字符出现的行]
    push cs             ; es:bp points to string to be printed
    pop es
    mov bp, boot_message
    mov ah, 0x13
    int 0x10            ; 调用中断，实现文字显示
```

主要实现了输出字符串的功能。在详解这段代码之前我们先介绍一下`bios`中断，这里我们主要使用到了`10H`号中断`13H`服务，主要的功能是显示字符串。

中断是`bios`提供给我们的一些功能入口点，我们通过对指定的寄存器设置相应的值，然后通过`int`指令调用相应的中断，`bios`会处理这些中断，实现相应的效果。

8086汇编提供的所有的中断可以查看如下网址：[中断列表](http://www.ablmcc.edu.hk/~scy/CIT/8086_bios_and_dos_interrupts.htm#int10h_13h)

要想实现我们显示的功能，我们需要了解下面这些参数：

> **INT 10h** / **AH = 13h** - write string.
>
> input:
> **AL** = write mode:
>   **bit 0**: update cursor after writing; [写入支付川后更新光标]
>   **bit 1**: string contains [attributes](http://www.ablmcc.edu.hk/~scy/CIT/8086_bios_and_dos_interrupts.htm#attrib). [带有属性的字符，比如：颜色，背景色]
> **BH** = page number. [页数：整个显示的区域被分成了好多页]
> **BL** = [attribute](http://www.ablmcc.edu.hk/~scy/CIT/8086_bios_and_dos_interrupts.htm#attrib) if string contains only characters (bit 1 of AL is zero). [设置字体颜色和背景色]
> **CX** = number of characters in string (attributes are not counted). [设置显示的字符个数]
> **DL,DH** = column, row at which to start writing. [设置第一个字符显示的行数和列数]
> **ES:BP** points to string to be printed. [设置要显示的字符串的地址]

接着我们看到了

```assembly
    jmp $               ; 跳转到当前地址，实现循环
```

这里`jmp`指令我们之前已经接触过了，是一个跳转指令，但是这里出现了一个奇怪的符号`$`，这个`$`代表当前指令被汇编后的地址，整个是跳转到该指令处，通过这样的形似，实现了类似c的`while(TRUE)`的功能。实现死循环。可能不是特别好理解，没关系，我们将刚刚生成的二进制文件反汇编看看。

```bash
$ ndisasm boot.bin >> boot_ndisasm.asm
```

反汇编的结果

```bash
00000022  EBFE              jmp short 0x22
```

我们注意到`$`符号被替换为了该行所在的地址，也就是`0x22`。

到这个地方剩下的代码已经不多了，再加油一下吧。

```assembly
boot_message:   db "hello os", 0   
						; 定义一个字符串为 dd 类型，并且后要添加一个 0，表示字符串的结束
boot_message_len:       ; boot_message_len - boot_message 刚好为字符串的大小
```

这里我们定义两个标签，一个是要显示的字符串，这里使用到了`db`伪指令，这是`nasm`汇编器提供给我们，通过`db`伪指令指明每一个字符都是**一个字节**，与此类似的指令还有`dw`，`dd`，分别对应两个字节（十六位），一个双字（32位）。剩下一个标签在这里的作用用来计算字符串的长度，`boot_message_len - boot_message`即为字串的长度。

最终我们来到了代码的末尾

```assembly
times 510 - ($ - $$) db 0
						; 填充 0 ，$ 代表当前地址，$$ 代表上一个代码的地址减去结尾地址
dw 0xaa55               ; 标志这个扇区为一个启动扇区
```

这里我们可以注意到，又有一个`$$`符号，这是什么呢？它表示一个节（section）的开始被汇编后的地址。在这里，我们的程序只有一个节，所以，`$$`实际上表示程序被编译后的开始地址，也就是`0x7c00`。

由于启动扇区需要指定的512字节，并且最后两个字节必须是`0xaa55`，所以前面不足510字节，我们需要填0。`times`是一个重复指令，`$`表示当前指令的地址，`$$`表示当前section的地址，这里也就是`0x7c00`，因此`$ - $$`表示前面代码所占的字节数，因此`510 - ($ - $$)`等于要填充的0的个数。最后使用`dw`伪指令声明一个双字。至此代码已经全部解释完毕。

## 调试代码

经过上面的操作我们可以编写运行代码了，但是我们怎么能够调试我们的内核代码呢？笔者采用的是使用`bochs`来调试，这一步我们之前安装的`bochs`开始发挥作用啦。

首先我们创建一个配置文件，内容如下：

```bash
###############################################################
# bochsrc.bxrc file for ilinux.
###############################################################

# how much memory the emulated machine will have
megs: 32

# filename of ROM images, 这里可能需要根据本机进行修改
romimage: file=$BXSHARE/BIOS-bochs-latest
vgaromimage: file=$BXSHARE/VGABIOS-lgpl-latest

# what disk images will be used
# 这里配置我们使用到的镜像文件
floppya: 1_44=boot.bin, status=inserted

# choose the boot disk.
boot: a

# where do we send log messages?
log: log/bochsout.txt

# disable the mouse, since ilinux is text only
mouse: enabled=0

# enable key mapping, using US layout as default.
# 这里也可能需要根据本机进行相应的修改
keyboard_mapping: enabled=1, map=$BXSHARE/keymaps/x11-pc-us.map

# 断点设置
magic_break: enabled=1

# GUI debug, 开启图形化调试界面
display_library: x, options=gui_debug
```

有了这个文件，我们就可以使用上面提及的`debug`命令啦，那我们立刻使用`debug`命令来调试我们的程序吧。

```bash
$ make debug
```

运行之后会有两个界面窗口，并且控制台也会输出很多信息

系统窗口

![image-20210331110134021](https://gitee.com/wei_manzhou/blog_picture/raw/master/20210331110136.png)

调试软件窗口

![image-20210331110115292](https://gitee.com/wei_manzhou/blog_picture/raw/master/20210331110117.png)

终端控制台输出

![image-20210331110156309](https://gitee.com/wei_manzhou/blog_picture/raw/master/20210331110201.png)

关于`bochs`的所提供的指令可以查看如下的网址

[Using Bochs internal debugger](https://bochs.sourceforge.io/cgi-bin/topper.pl?name=New+Bochs+Documentation&url=https://bochs.sourceforge.io/doc/docbook/user/index.html)

## 总结与回顾

经过上面的过程，我们已经学会了怎么编写一个简单的bootsector，并且编译运行，以及调试这个程序。但是这离我们编写一个操作系统还有些距离。

在下一节中，我们将会介绍保护模式。掌握了保护模式，我们才知道`Intel`的CPU如何运行在32位模式下，从而可以编写一个32位的操作系统。

如果读者已经掌握了保护模式，则可以跳过下一节。