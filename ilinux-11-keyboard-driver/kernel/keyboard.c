/*************************************************************************
	> File Name: keyboard.c
	> Author: snowflake
	> Mail: 278121951@qq.com 
	> Created Time: Sat 30 Jan 2021 10:07:03 PM CST
    >
    > PC 和 AT键盘系统任务（驱动程序）
 ************************************************************************/

#include "kernel.h"
#include <ilinux/keymap.h>
#include "keymaps/us-std.h"
#include <string.h>

/* 标准键盘和AT键盘 */
#define KEYBOARD_DATA 0x60 /* 键盘数据的I/O端口，用于和键盘控制器的底层交互。 */

/* AT键盘 */
#define KEYBOARD_COMMAND 0x64 /* AT上的命令I/o端口 */
#define KEYBOARD_STATUS 0x64  /* AT上的状态I/O端口 */
#define KEYBOARD_ACK 0xFA     /* 键盘相应确认 */

#define KEYBOARD_OUT_FULL 0x01           /* 字符按键按下时该状态位被设置 */
#define KEYBOARD_IN_FULL 0x02            /* 未准备接收字符时该状态位被设置 */
#define LED_CODE 0xED                    /* 设置键盘灯的命令 */
#define MAX_KEYBOARD_ACK_RETRIES 0x1000  /* 等待键盘响应的最大等待时间 */
#define MAX_KEYBOARD_BUSY_RETRIES 0x1000 /* 键盘忙时循环的最大时间 */
#define KEY_BIT 0x80                     /* 将字符打包传输到键盘的位 */

/* 它们用于滚屏操作 */
#define SCROLL_UP 0   /* 前滚，用于滚动屏幕 */
#define SCROLL_DOWN 1 /* 后滚 */

/* 锁定键激活位，应该要等于键盘上的LED灯位 */
#define SCROLL_LOCK 0x01  /* 二进制：0001 */
#define NUM_LOCK 0x02     /* 二进制：0010 */
#define CAPS_LOCK 0x04    /* 二进制：0100 */
#define DEFAULT_LOCK 0x02 /* 默认：小键盘也是打开的 */

/* 键盘缓冲区 */
#define KEYBOARD_IN_BYTES 32 /* 键盘输入缓冲区的大小 */

/* 其他用途 */
#define ESC_SCAN 0x01    /* 重启键，当宕机时可用 */
#define SLASH_SCAN 0x35  /* 识别小键盘区的斜杠 */
#define RSHIFT_SCAN 0x36 /* 区分左移和右移 */
#define HOME_SCAN 0x47   /* 数字键盘上的第一个按键 */
#define INS_SCAN 0x52    /* INS键，为了使用 CTRL-ALT-INS 重启快捷键 */
#define DEL_SCAN 0x53    /* DEL键，为了使用 CTRL-ALT-DEL 重启快捷键 */

/* 当前键盘所处的各种状态，解释一个按键需要使用这些状态 */
PRIVATE bool_t esc = FALSE;         /* 是一个转义扫描码？收到一个转义扫描码时，被置位 */
PRIVATE bool_t alt_left = FALSE;    /* 左ALT键状态 */
PRIVATE bool_t alt_right = FALSE;   /* 右ALT键状态 */
PRIVATE bool_t alt = FALSE;         /* ALT键状态，不分左右 */
PRIVATE bool_t ctrl_left = FALSE;   /* 左CTRL键状态 */
PRIVATE bool_t ctrl_right = FALSE;  /* 右CTRL键状态 */
PRIVATE bool_t ctrl = FALSE;        /* CTRL键状态，不分左右 */
PRIVATE bool_t shift_left = FALSE;  /* 左SHIFT键状态 */
PRIVATE bool_t shift_right = FALSE; /* 右SHIFT键状态 */
PRIVATE bool_t shift = FALSE;       /* SHIFT键状态，不分左右 */
PRIVATE bool_t num_down = FALSE;    /* 数字锁定键(数字小键盘锁定键)按下 */
PRIVATE bool_t caps_down = FALSE;   /* 大写锁定键按下 */
PRIVATE bool_t scroll_down = FALSE; /* 滚动锁定键按下 */
PRIVATE u8_t locks[NR_CONSOLES] = { /* 每个控制台的锁定键状态 */
                                   DEFAULT_LOCK, DEFAULT_LOCK, DEFAULT_LOCK};

/* 数字键盘的转义字符映射 */
PRIVATE char numpad_map[] =
    {'H', 'Y', 'A', 'B', 'D', 'C', 'V', 'U', 'G', 'S', 'T', '@'};

/* 缓冲区相关 */
PRIVATE u8_t input_buff[KEYBOARD_IN_BYTES];
PRIVATE int input_count;
PRIVATE u8_t *input_free = input_buff; /* 指向输入缓冲区的下一个空闲位置 */
PRIVATE u8_t *input_todo = input_buff; /* 指向被处理并返回给终端的位置 */

// 定义一个缓冲区，用来接收命令
PRIVATE u8_t input_command_buff[256];
PRIVATE u8_t *input_command_free = input_command_buff; /* 指向输入缓冲区的下一个空闲位置 */
PRIVATE int input_command_count;
PRIVATE u8_t *input_command_todo = input_command_buff;


/*************************************************************************
    > 返回一个扫描码对应的ASCII码，忽略修饰符
    > 
	> @name map_key0    
 ************************************************************************/
#define map_key0(scan_code) ((u16_t)keymap[scan_code * MAP_COLS])
/*************************************************************************
    > 返回一个扫描码对应的ASCII码，转为大写形式
    > 
	> @name map_key0    
 ************************************************************************/
#define map_key1(scan_code) ((u16_t)keymap[scan_code * MAP_COLS + 1])
#define map_key2(scan_code) ((u16_t)keymap[scan_code * MAP_COLS + 2])

FORWARD _PROTOTYPE(void main, (voie));

/*************************************************************************
    > 扫描键盘获得键盘码
    > 
	> @name scan_key    
 ************************************************************************/
PRIVATE u8_t scan_key(void)
{
    // 1. 从键盘控制器端口获取键盘码
    u8_t scan_code = in_byte(KEYBOARD_DATA);

    // 2. 选通键盘
    int val = in_byte(PORT_B);
    out_byte(PORT_B, val | KEY_BIT);
    out_byte(PORT_B, val);

    return scan_code;
}

static int count = 0;

/*************************************************************************
    > 键盘中断处理程序
    > 
	> @name keyboard_handler    
 ************************************************************************/
PRIVATE int keyboard_handler(int irq)
{
    u8_t scan_code = scan_key();

    switch (scan_code)
    {
    // 松开左 shift
    case 157:
        ctrl_left = FALSE;
        break;
    case 170:
        shift_left = FALSE;
        break;
    case 182:
        shift_right = FALSE;
        break;
    default:
        break;
    }

    if (scan_code < 1 || scan_code > 88)
        return ENABLE;

    // 处理一些特殊字符，存储一些状态
    switch (scan_code)
    {
    case 14:
        print_bs();
        return ENABLE;
    // 处理 tab 键，目前无处理
    case 15:
        return ENABLE;
    // 回车键
    case 28:
        printf("\n");
        // 为命令缓冲区尾部添加一个 '\0' 标记命令结束，用于 shell 的字符比对
        *input_command_free++ = '\0';
        // 调用 shell 方法，处理用户输入的命令
        eof();

        input_command_free = input_command_buff;
        input_command_count = 0;
        return ENABLE;
    case 29:
        ctrl_left = TRUE;
        return ENABLE;
    case 42:
        shift_left = TRUE;
        return ENABLE;
    case 54:
        shift_right = TRUE;
        return ENABLE;
    // 切换大小写
    case 58:
        caps_down = caps_down == TRUE ? FALSE : TRUE;
        // printf(caps_down == TRUE ? "ok": "no");
        return ENABLE;
    default:
        break;
    }

    /* Buffer available. */
    if (input_count < KEYBOARD_IN_BYTES)
    {
        *input_free++ = scan_code;
        ++input_count;

        // 定义一个
        u8_t prb[2];

        // 判断当前用户键入的是小写字符还是大写字符
        bool_t caps = (caps_down == TRUE && shift_left != TRUE && shift_right != TRUE) || (!(caps_down == TRUE) && (shift_left == TRUE || shift_right == TRUE));

        // 小写字符
        int key0 = map_key0(input_buff[0]);
        // 大写字符
        int key1 = map_key1(input_buff[0]);

        // 根据大写状态是否触发，来确定输出的是小写字符还是大写字符
        prb[0] = caps == TRUE ? key1 : key0;
        prb[1] = '\0';

        // 将要显示的字符存储到 input_command_free 缓冲区中
        *input_command_free++ = prb[0];
        // 命令字符个数加一
        ++input_command_count;

        // 输出用户键入的字符
        printf("%s", prb);

        // 清空缓冲区
        input_free = input_buff;
        input_count = 0;
    }
    // 清空缓冲区
    else
    {
        input_free = input_buff;
        input_count = 0;
    }

    // 开启中断
    return ENABLE;
}

PUBLIC char *get_str()
{
    // 初始化回退位置
    last_position = display_position;

    // 开启键盘中断
    enable_irq(KEYBOARD_IRQ);
    while (TRUE);
}

PUBLIC void eof()
{

    shell();

    // 关闭键盘中断
    dsable_irq(KEYBOARD_IRQ);
}

/*************************************************************************
    > 处理用户输入的命令
    > 
	> @name shell
 ************************************************************************/
PUBLIC void shell(void)
{
    int rs;
    // int rs = strcmp(input_command_buff, "htop");
    // if (!rs)
    // {
    //     proc_dump();
    //     return;
    // }

    // rs = strcmp(input_command_buff, "map");
    // if (!rs)
    // {
    //     map_dump();
    //     return;
    // }
    rs = strcmp(input_command_buff, "nico");
    if (!rs)
    {
        main();
    }

    rs = strcmp(input_command_buff, "clear");
    if (!rs)
    {
        clear_screen();
    }

    if (rs)
    {
        printf("command not resolve, please re input: %s\n", input_command_buff);
    }
    printf("#root >");
}

/*************************************************************************
    > 初始化键盘驱动
    > 
	> @name init_keyboard
 ************************************************************************/
PUBLIC void init_keyboard(void)
{
    printf("#[KEY_BD] -> init...\n");

    /* 扫描键盘以确保没有残余的键入，清空 */
    (void)scan_key();

    /* 设置键盘中断处理例程 */
    put_irq_handler(KEYBOARD_IRQ, keyboard_handler);
    // enable_irq(KEYBOARD_IRQ);

    printf("#[KEY_BD] -> init END\n");

    while (TRUE)
    {
        get_str();
    }
}

char *niconiconi[15] = {
    "+------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------+",
    "+------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++----------------------------------------------------------------****------***-++----------------------------------------------------------------*****-----***-++----------------------------------------------------------------*****-----***-++----------------------------------------------------------------******----***-++----------------------------------------------------------------***-***---***-++----------------------------------------------------------------***--***--***-++----------------------------------------------------------------***--****-***-++----------------------------------------------------------------***---*******-++----------------------------------------------------------------***----******-++----------------------------------------------------------------***-----*****-++----------------------------------------------------------------***------****-++----------------------------------------------------------------***-------***-++----------------------------------------------------------------***-------***-++----------------------------------------------------------------***-------***-++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------+",
    "+------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++----------------------------------------------------------****------***---***-++----------------------------------------------------------*****-----***---***-++----------------------------------------------------------*****-----***---***-++----------------------------------------------------------******----***---***-++----------------------------------------------------------***-***---***---***-++----------------------------------------------------------***--***--***---***-++----------------------------------------------------------***--****-***---***-++----------------------------------------------------------***---*******---***-++----------------------------------------------------------***----******---***-++----------------------------------------------------------***-----*****---***-++----------------------------------------------------------***------****---***-++----------------------------------------------------------***-------***---***-++----------------------------------------------------------***-------***---***-++----------------------------------------------------------***-------***---***-++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------+",
    "+------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------****------***---***-------*******---++------------------------------------------*****-----***---***-----**********--++------------------------------------------*****-----***---***----****----****-++------------------------------------------******----***---***---****----------++------------------------------------------***-***---***---***---***-----------++------------------------------------------***--***--***---***---**------------++------------------------------------------***--****-***---***---**------------++------------------------------------------***---*******---***---**------------++------------------------------------------***----******---***---**------------++------------------------------------------***-----*****---***---***-----------++------------------------------------------***------****---***---****----------++------------------------------------------***-------***---***----****----****-++------------------------------------------***-------***---***-----**********--++------------------------------------------***-------***---***-------*******---++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------+",
    "+------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++---------------------------****------***---***-------*******-------*******----++---------------------------*****-----***---***-----**********-----*********---++---------------------------*****-----***---***----****----****---***-----***--++---------------------------******----***---***---****-----------***-------***-++---------------------------***-***---***---***---***------------***-------***-++---------------------------***--***--***---***---**-------------**---------**-++---------------------------***--****-***---***---**-------------**---------**-++---------------------------***---*******---***---**-------------**---------**-++---------------------------***----******---***---**-------------**---------**-++---------------------------***-----*****---***---***------------***-------***-++---------------------------***------****---***---****-----------***-------***-++---------------------------***-------***---***----****----****---***-----***--++---------------------------***-------***---***-----**********-----*********---++---------------------------***-------***---***-------*******-------*******----++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------+",
    "+------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++-------------****------***---***-------*******-------*******----****------***-++-------------*****-----***---***-----**********-----*********---*****-----***-++-------------*****-----***---***----****----****---***-----***--*****-----***-++-------------******----***---***---****-----------***-------***-******----***-++-------------***-***---***---***---***------------***-------***-***-***---***-++-------------***--***--***---***---**-------------**---------**-***--***--***-++-------------***--****-***---***---**-------------**---------**-***--****-***-++-------------***---*******---***---**-------------**---------**-***---*******-++-------------***----******---***---**-------------**---------**-***----******-++-------------***-----*****---***---***------------***-------***-***-----*****-++-------------***------****---***---****-----------***-------***-***------****-++-------------***-------***---***----****----****---***-----***--***-------***-++-------------***-------***---***-----**********-----*********---***-------***-++-------------***-------***---***-------*******-------*******----***-------***-++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------+",
    "+------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++--------****------***---***-------*******-------*******----****------***--***-++--------*****-----***---***-----**********-----*********---*****-----***--***-++--------*****-----***---***----****----****---***-----***--*****-----***--***-++--------******----***---***---****-----------***-------***-******----***--***-++--------***-***---***---***---***------------***-------***-***-***---***--***-++--------***--***--***---***---**-------------**---------**-***--***--***--***-++--------***--****-***---***---**-------------**---------**-***--****-***--***-++--------***---*******---***---**-------------**---------**-***---*******--***-++--------***----******---***---**-------------**---------**-***----******--***-++--------***-----*****---***---***------------***-------***-***-----*****--***-++--------***------****---***---****-----------***-------***-***------****--***-++--------***-------***---***----****----****---***-----***--***-------***--***-++--------***-------***---***-----**********-----*********---***-------***--***-++--------***-------***---***-------*******-------*******----***-------***--***-++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------+",
    "+------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++---***--***------*******-------*******-----****------***---***------*******---++---***--***----**********-----*********----*****-----***---***----**********--++---***--***---****----****---***-----***---*****-----***---***---****----****-++---***--***--****-----------***-------***--******----***---***--****----------++---***--***--***------------***-------***--***-***---***---***--***-----------++---***--***--**-------------**---------**--***--***--***---***--**------------++-*-***--***--**-------------**---------**--***--****-***---***--**------------++-*****--***--**-------------**---------**--***---*******---***--**------------++-*****--***--**-------------**---------**--***----******---***--**------------++-*****--***--***------------***-------***--***-----*****---***--***-----------++--****--***--****-----------***-------***--***------****---***--****----------++---***--***---****----****---***-----***---***-------***---***---****----****-++---***--***----**********-----*********----***-------***---***----**********--++---***--***------*******-------*******-----***-------***---***------*******---++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------+",
    "+------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++--*******-------*******-----****------***---***------*******-------*******----++**********-----*********----*****-----***---***----**********-----*********---++***----****---***-----***---*****-----***---***---****----****---***-----***--++**-----------***-------***--******----***---***--****-----------***-------***-++*------------***-------***--***-***---***---***--***------------***-------***-++-------------**---------**--***--***--***---***--**-------------**---------**-++-------------**---------**--***--****-***---***--**-------------**---------**-++-------------**---------**--***---*******---***--**-------------**---------**-++-------------**---------**--***----******---***--**-------------**---------**-++*------------***-------***--***-----*****---***--***------------***-------***-++**-----------***-------***--***------****---***--****-----------***-------***-++***----****---***-----***---***-------***---***---****----****---***-----***--++**********-----*********----***-------***---***----**********-----*********---++--*******-------*******-----***-------***---***------*******-------*******----++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------+",
    "+------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++-*******-----****------***---***------*******-------*******-----****------***-++*********----*****-----***---***----**********-----*********----*****-----***-++**-----***---*****-----***---***---****----****---***-----***---*****-----***-++*-------***--******----***---***--****-----------***-------***--******----***-++*-------***--***-***---***---***--***------------***-------***--***-***---***-++---------**--***--***--***---***--**-------------**---------**--***--***--***-++---------**--***--****-***---***--**-------------**---------**--***--****-***-++---------**--***---*******---***--**-------------**---------**--***---*******-++---------**--***----******---***--**-------------**---------**--***----******-++*-------***--***-----*****---***--***------------***-------***--***-----*****-++*-------***--***------****---***--****-----------***-------***--***------****-++**-----***---***-------***---***---****----****---***-----***---***-------***-++*********----***-------***---***----**********-----*********----***-------***-++-*******-----***-------***---***------*******-------*******-----***-------***-++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------+",
    "+------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++***-----****------***---***------*******-------*******-----****------***--***-++****----*****-----***---***----**********-----*********----*****-----***--***-++--***---*****-----***---***---****----****---***-----***---*****-----***--***-++---***--******----***---***--****-----------***-------***--******----***--***-++---***--***-***---***---***--***------------***-------***--***-***---***--***-++----**--***--***--***---***--**-------------**---------**--***--***--***--***-++----**--***--****-***---***--**-------------**---------**--***--****-***--***-++----**--***---*******---***--**-------------**---------**--***---*******--***-++----**--***----******---***--**-------------**---------**--***----******--***-++---***--***-----*****---***--***------------***-------***--***-----*****--***-++---***--***------****---***--****-----------***-------***--***------****--***-++--***---***-------***---***---****----****---***-----***---***-------***--***-++****----***-------***---***----**********-----*********----***-------***--***-++***-----***-------***---***------*******-------*******-----***-------***--***-++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------+",
    "+------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++-****------***---***------*******-------*******-----****------***--***--------++-*****-----***---***----**********-----*********----*****-----***--***--------++-*****-----***---***---****----****---***-----***---*****-----***--***--------++-******----***---***--****-----------***-------***--******----***--***--------++-***-***---***---***--***------------***-------***--***-***---***--***--------++-***--***--***---***--**-------------**---------**--***--***--***--***--------++-***--****-***---***--**-------------**---------**--***--****-***--***--------++-***---*******---***--**-------------**---------**--***---*******--***--------++-***----******---***--**-------------**---------**--***----******--***--------++-***-----*****---***--***------------***-------***--***-----*****--***--------++-***------****---***--****-----------***-------***--***------****--***--------++-***-------***---***---****----****---***-----***---***-------***--***--------++-***-------***---***----**********-----*********----***-------***--***--------++-***-------***---***------*******-------*******-----***-------***--***--------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------+",
    "+------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++**---***------*******-------*******-----****------***--***--------------------++**---***----**********-----*********----*****-----***--***--------------------++**---***---****----****---***-----***---*****-----***--***--------------------++**---***--****-----------***-------***--******----***--***--------------------++**---***--***------------***-------***--***-***---***--***--------------------++**---***--**-------------**---------**--***--***--***--***--------------------++**---***--**-------------**---------**--***--****-***--***--------------------++**---***--**-------------**---------**--***---*******--***--------------------++**---***--**-------------**---------**--***----******--***--------------------++**---***--***------------***-------***--***-----*****--***--------------------++**---***--****-----------***-------***--***------****--***--------------------++**---***---****----****---***-----***---***-------***--***--------------------++**---***----**********-----*********----***-------***--***--------------------++**---***------*******-------*******-----***-------***--***--------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------+",
    "+------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++**-------*******-----****------***--***---------------------------------------++***-----*********----*****-----***--***---------------------------------------++****---***-----***---*****-----***--***---------------------------------------++------***-------***--******----***--***---------------------------------------++------***-------***--***-***---***--***---------------------------------------++------**---------**--***--***--***--***---------------------------------------++------**---------**--***--****-***--***---------------------------------------++------**---------**--***---*******--***---------------------------------------++------**---------**--***----******--***---------------------------------------++------***-------***--***-----*****--***---------------------------------------++------***-------***--***------****--***---------------------------------------++****---***-----***---***-------***--***---------------------------------------++***-----*********----***-------***--***---------------------------------------++**-------*******-----***-------***--***---------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------+",
    "+------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++-****------***--***-----------------------------------------------------------++-*****-----***--***-----------------------------------------------------------++-*****-----***--***-----------------------------------------------------------++-******----***--***-----------------------------------------------------------++-***-***---***--***-----------------------------------------------------------++-***--***--***--***-----------------------------------------------------------++-***--****-***--***-----------------------------------------------------------++-***---*******--***-----------------------------------------------------------++-***----******--***-----------------------------------------------------------++-***-----*****--***-----------------------------------------------------------++-***------****--***-----------------------------------------------------------++-***-------***--***-----------------------------------------------------------++-***-------***--***-----------------------------------------------------------++-***-------***--***-----------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------++------------------------------------------------------------------------------+",
};

/*************************************************************************
    > 测试输出字符画程序
    > 
	> @name main
 ************************************************************************/
void main()
{
    while (TRUE)
    {
        for (int i = 0; i < 15; i++)
        {
            print_graph(niconiconi[i]);
            for (int j = 0; j < 4000000; j++);
        }
    }
}
