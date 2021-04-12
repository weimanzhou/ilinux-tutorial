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

; 栈顶地址
STACK_BASE                      equ 0x7c00