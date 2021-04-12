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
ASM_PARA		= -f bin

# ===================================================
# 目标程序以及编译的中间文件
# ---------------------------------------------------
ILINUX_BOOT     = boot.bin
ILINUX_LOADER 	= loader.bin

# ===================================================
# 所有的功能
# ---------------------------------------------------
.PHONY: nop all image debug run clean
# 默认选项（输入make但是没有子命令）
nop:
	@echo "all                      编译所有文件，生成目标文件（二进制文件，boot.bin）"
	@echo "image                    生成镜像文件ilinux.img"
	@echo "debug                    打开bochs运行系统并调试"
	@echo "run                      提示用于如何将系统安装到虚拟机中"
	@echo "clean                    清理文件"

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

run: $(ILINUX_BOOT)
	qemu-system-x86_64 $<

clean:
	rm -r $(ILINUX_BOOT)
	rm -r $(ILINUX_LOADER)

# ===================================================
# 目标文件生成规则
# ---------------------------------------------------
#  软件镜像不存在时，将会自动生成
$(FD):
	dd if=/dev/zero of=$(FD) bs=512 count=2880

$(ILINUX_BOOT): boot.asm
	$(ASM) $(ASM_PARA) $< -o $@
$(ILINUX_LOADER): loader.asm
	$(ASM) $(ASM_PARA) $< -o $@