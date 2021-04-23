# ===================================================
# ---------------------------------------------------
# 变量
# ---------------------------------------------------
# 编译中间目录

# bochs 配置文件
BOCHS_CONFIG    = conf/bochsrc.bxrc
BOCHS_ARGA		= -f $(BOCHS_CONFIG)

FD 				= ilinux.img
IMG_MOUNT_PATH	= /media/floppy0/

# 所需要的汇编器以及汇编参数
ASM				= nasm
ASM_PARA		= -i boot/include/ -i kernel/include/ -f elf
QEMU 			= qemu-system-x86_64
LD 				= ld 
LD_PARA			= -Ttext 0x1000 -m elf_i386
CC				= gcc
CC_PARA			= -I kernel/include -I include -fno-builtin -Wimplicit-function-declaration -fno-stack-protector -Wvarargs -m32 -c 


# ===================================================
# 目标程序以及编译的中间文件
# ---------------------------------------------------
ILINUX_BOOT     = target/boot/boot.bin
ILINUX_LOADER 	= target/boot/loader.bin
ILINUX_KERNEL 	= target/kernel/kernel.bin

KERNEL_A_OBJS 	= kernel/kernel kernel/kernel_i386_lib
KERNEL_C_OBJS	= kernel/main kernel/table kernel/dump kernel/process \
				  kernel/init kernel/protect kernel/exception \
				  kernel/mics kernel/i8259 kernel/clock \
				  kernel/keyboard

KERNEL_A_T_OBJS	= target/kernel/kernel.o target/kernel/kernel_i386_lib.o
KERNEL_C_T_OBJS	= target/kernel/main.o target/kernel/table.o target/kernel/dump.o \
				  target/kernel/init.o target/kernel/protect.o target/kernel/exception.o \
				  target/kernel/mics.o target/kernel/i8259.o target/kernel/clock.o \
				  target/kernel/process.o target/kernel/keyboard.o

KERNEL_OBJS		= $(KERNEL_A_OBJS) $(KERNEL_C_OBJS)
KERNEL_T_OBJS 	= $(KERNEL_A_T_OBJS) $(KERNEL_C_T_OBJS)

LIB_A_OBJS		= lib/ansi/string
LIB_C_OBJS		= lib/stdio/vsprintf lib/ansi/stringc

LIB_A_T_OBJS	= target/lib/ansi/string.o
LIB_C_T_OBJS	= target/lib/stdio/vsprintf.o target/lib/ansi/stringc.o

LIB_OBJS		= $(LIB_A_OBJS) $(LIB_C_OBJS)
LIB_T_OBJS		= $(LIB_A_T_OBJS) $(LIB_C_T_OBJS)

OBJS			= $(KERNEL_T_OBJS) $(LIB_T_OBJS)

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
	mkdir target/kernel -p
	mkdir target/lib/ansi -p 
	mkdir target/lib/stdio -p 

all: $(ILINUX_BOOT)
	@echo "boot.bin 生成成功"

image: $(FD) $(ILINUX_BOOT) $(ILINUX_LOADER) $(ILINUX_KERNEL)
	dd if=$(ILINUX_BOOT) of=$(FD) bs=512 count=1 conv=notrunc
	sudo mount -o loop $(FD) $(IMG_MOUNT_PATH)
	cp $(ILINUX_LOADER) $(IMG_MOUNT_PATH)
	cp $(ILINUX_KERNEL) $(IMG_MOUNT_PATH)
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
	$(ASM) $(ASM_PARA) -f bin $< -o $@
$(ILINUX_LOADER): boot/loader.asm
	$(ASM) $(ASM_PARA) -f bin $< -o $@
$(ILINUX_KERNEL): $(OBJS)
	$(LD) $(LD_PARA) -o $@ $^

$(KERNEL_T_OBJS): 
	for x in $(KERNEL_C_OBJS); do $(CC) $(CC_PARA) $$x.c -o target/$$x.o; done
	for x in $(KERNEL_A_OBJS); do $(ASM) $(ASM_PARA) $$x.asm -o target/$$x.o; done

$(LIB_T_OBJS):
	for x in $(LIB_C_OBJS); do $(CC) $(CC_PARA) $$x.c -o target/$$x.o; done
	for x in $(LIB_A_OBJS); do $(ASM) $(ASM_PARA) $$x.asm -o target/$$x.o; done
