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
BS_FileSysType  DB 'FAT12   '           ; 文件系统类型，必须8个字节

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
; FAT12 文件系统 FAT1 表的起始扇区
FAT12_SECTOR_NO_OF_FAT1         equ 1

FAT12_DELTA_SECTOR_NO           equ 17

W_ROOT_DIR_SIZE_FOR_LOOP        dw FAT12_ROOT_DIR_SECTORS
W_SECTOR_NO                     dw 0
B_ODD                           db 0
