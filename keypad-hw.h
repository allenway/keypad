/*
 * *******************************************************************************
 * *   FileName    :    keypad-hw.h
 * *   Description :    按键寄存器定义头文件，使用EIM_D16,EIM_D17,EIM_D18,EIM_D19,EIM_D20,EIM_D21,
 **                     EIM_D22,EIM_D23,EIM_D30复用为GPIO功能，实现4x4矩阵键盘
                        对应关系：
                        EIM_D16 EIM_D17 EIM_D18 EIM_D19 EIM_D20 EIM_D21 EIM_D22 EIM_D23
                        GP3_16  GP3_17  GP3_18  GP3_19  GP3_20  GP3_21  GP3_22  GP3_23

 * *   Author      :    2015-11-17 15:34   by   liulu
 * ********************************************************************************
 * */

#define KEYPAD_IOMUX_MUX_PHY_ADDR   0x20E0090
#define KEYPAD_IOMUX_MUX_PHY_SIZE   0x40
#define KEYPAD_IOMUX_PAD_PHY_ADDR   0X20E03A4
#define KEYPAD_IOMUX_PAD_PHY_SIZE   0x40

#define KEYPAD_GPIO_PHY_ADDR        0x20A4000
#define KEYPAD_GPIO_PHY_SIZE        0x20

#define KEYPAD_GPIO_DR_OFFSET       0x0
#define KEYPAD_GPIO_GDIR_OFFSET     0x4
#define KEYPAD_GPIO_PSR_OFFSET      0x8
#define KEYPAD_GPIO_ICR1_OFFSET     0xc
#define KEYPAD_GPIO_ICR2_OFFSET     0x10
#define KEYPAD_GPIO_IMR_OFFSET      0x14
#define KEYPAD_GPIO_ISR_OFFSET      0x18
#define KEYPAD_GPIO_EDGE_SEL_OFFSET 0x1c

struct keypad_hw{
    unsigned char code; //键值编码
    int iomux_offset;
    int gpio_nr;
    int gpio;
    int irq;
};
//键值由row和col的组成，row bit[5:3] col bit[2:0]
#define rows_size  4
#define cols_size  4
static struct keypad_hw rows[rows_size] = {
    {0x1<<3,0x0,16,IMX_GPIO_NR(3,16)},    //GPIO3_16
    {0x2<<3,0x4,17,IMX_GPIO_NR(3,17)},    //GPIO3_17
    {0x3<<3,0x8,18,IMX_GPIO_NR(3,18)},    //GPIO3_18
    {0x4<<3,0xc,19,IMX_GPIO_NR(3,19)}     //GPIO3_19
};
static struct keypad_hw cols[cols_size] = {
    {0x1,0x10,20},    //GPIO3_20
    {0x2,0x14,21},    //GPIO3_21
    {0x3,0x18,22},    //GPIO3_22
    {0x4,0x1c,23},    //GPIO3_23
//    {0x5,0x3c,30}     //GPIO3_30
};
static unsigned int key[rows_size][cols_size] = {
    {KEY_1,KEY_2,KEY_3,KEY_4},
    {KEY_Q,KEY_W,KEY_E,KEY_R},
    {KEY_A,KEY_S,KEY_D,KEY_F},
    {KEY_Z,KEY_X,KEY_C,KEY_V},
};

