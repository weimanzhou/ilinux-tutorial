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
ASM_PARA		= -i boot/include/ -f elf
QEMU 			= qemu-system-x86_64
LD 				= ld 
LD_PARA			= -Ttext 0x1000 -m elf_i386
CC				= gcc
CC_PARA			= -I $(sk)/include -I include -fno-builtin -fno-stack-protector -m32 -c 


# ===================================================
# 目标程序以及编译的中间文件
# ---------------------------------------------------
ILINUX_BOOT     = target/boot/boot.bin
ILINUX_LOADER 	= target/boot/loader.bin
ILINUX_KERNEL 	= target/kernel/kernel.bin


KERNEL_OBJS		= target/kernel/kernel.o target/kernel/main.o target/kernel/kernel_i386_lib.o 
OBJS			= $(KERNEL_OBJS) $(LIB_OBJS)

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


target/kernel/kernel.o: kernel/kernel.asm
	$(ASM) $(ASM_PARA) $< -o $@
target/kernel/kernel_i386_lib.o: kernel/kernel_i386_lib.asm
	$(ASM) $(ASM_PARA) $< -o $@
target/kernel/main.o: kernel/main.c
	$(CC) $(CC_PARA) -o $@ $<
