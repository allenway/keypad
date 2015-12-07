/*
 * *******************************************************************************
 * *   FileName    :    keypad.c
 * *   Description :
 * *   Author      :    2015-11-17 14:24   by   liulu
 * ********************************************************************************
 * */

#include <linux/module.h>     /* 引入与模块相关的宏 */
#include <linux/init.h>        /* 引入module_init() module_exit()函数 */
#include <linux/moduleparam.h> /* 引入module_param() */
#include <linux/workqueue.h>
#include <asm/io.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include "keypad-hw.h"

//#define __DEBUG
static volatile void *keypad_iomux_mux_base;
static volatile void *keypad_iomux_pad_base;
static volatile void *keypad_gpio_base;
static volatile void *keypad_gpio_dr;
static volatile void *keypad_gpio_gdir;
static volatile void *keypad_gpio_psr;
static volatile void *keypad_gpio_icr1;
static volatile void *keypad_gpio_icr2;
static volatile void *keypad_gpio_imr;
static volatile void *keypad_gpio_isr;
static volatile void *keypad_gpio_edge_sel;

//read gpio
static int read_gpio(struct keypad_hw *pin)
{
    return (ioread32(keypad_gpio_dr) & (1 << pin->gpio_nr))?1:0;
}
//write gpio
static void write_gpio(struct keypad_hw *pin,int val)
{
    unsigned int reg;
    reg = ioread32(keypad_gpio_dr);
    reg &= ~(1<<pin->gpio_nr);
    if(val)
        reg |= 1<<pin->gpio_nr;
    iowrite32(reg,keypad_gpio_dr);
}
//上报按键事件
static struct input_dev *input;
static unsigned char keycode;  //按键键值,0表示无按键事件
static int current_row; //当前row，表示发生变化的row
static int current_col; //当前col，表示发生变化的col
static unsigned int rows_sel;
static unsigned int cols_sel;
//工作队列
static struct workqueue_struct *keypad_wq;
static struct delayed_work keypad_work;
static int keypad_open(struct input_dev *dev)
{
    keycode = 0;
    //使能中断
    iowrite32(ioread32(keypad_gpio_imr)|rows_sel,keypad_gpio_imr);
    return 0;
}

static void keypad_close(struct input_dev *dev)
{
    flush_workqueue(keypad_wq);
    cancel_delayed_work(&keypad_work);
    //禁止中断
    iowrite32(ioread32(keypad_gpio_imr)&(~rows_sel),keypad_gpio_imr);
    return;
}

void report_keypad(unsigned char code,int release)
{
    int r,c;
#ifdef __DEBUG
    printk(KERN_INFO"%s entry\n",__func__);
    printk(KERN_INFO"keypad event## row %d col %d code 0x%x %s\n",code>>3,code&0x7,code,release?"release":"press" );
#endif
    r = code >>3 ;
    c = code & 0x7 ;
    if(r<1||r>rows_size||c<1||c>cols_size)
    {
#ifdef __DEBUG
        printk(KERN_INFO"keypad event## outrange row %d col %d code 0x%x %s\n",code>>3,code&0x7,code,release?"release":"press" );
#endif
        return;
    }
    input_report_key(input,key[r-1][c-1],!release);
    input_sync(input);
#ifdef __DEBUG
    printk(KERN_INFO"%s ok\n",__func__);
#endif
}
static void keypad_func(struct work_struct *work)
{
    int j;
    int key_release;
#ifdef __DEBUG
    printk(KERN_INFO"%s entry\n",__func__);
#endif
    if(current_row>=rows_size || current_row <0)
        return;
    //禁止中断
    iowrite32(ioread32(keypad_gpio_imr)&(~rows_sel),keypad_gpio_imr);
    current_col = -1;
    key_release = read_gpio(&rows[current_row]);
    if(!key_release)//press动作
    {
        if(keycode!=0)  //表示当前已经有按键,所以要先release这个按键
        {
            //上报按键release事件
            report_keypad(keycode,1);
        }
        //查找press的col
        for(j=0;j<cols_size;j++)
        {
            write_gpio(&cols[j],1); //拉高col
            //mdelay(10);
            if(read_gpio(&rows[current_row]))
            {
                current_col = j;
                break;
            }
        }
        //计算keycode
        if (current_col<0 || current_col>=cols_size)
            keycode = 0;  //无效键值
        else
            keycode = rows[current_row].code | cols[current_col].code;
    }
    //上报按键事件
    report_keypad(keycode,key_release);
    current_row = -1;
    if(key_release)  //release动作需要重置keycode
        keycode = 0;
    //设置col输出为0
    for(j=0;j<cols_size;j++)
        write_gpio(&cols[j],0);
    //使能中断
    iowrite32(ioread32(keypad_gpio_imr)|rows_sel,keypad_gpio_imr);
#ifdef __DEBUG
    printk(KERN_INFO"%s ok\n",__func__);
#endif
}
static irqreturn_t keypad_handler(int irq,void *dev_id)
{
    int i;
    for(i=0;i<rows_size;i++)
    {
        if(irq==rows[i].irq)
        {
            current_row = i;
            break;
        }
    }
    if(current_row>=0 && current_row<rows_size)
        queue_delayed_work(keypad_wq,&keypad_work,HZ/10);  //delay 100ms
    return IRQ_HANDLED;
}
static int __init keypad_init(void)
{
    int ret,i,j;
    volatile unsigned int val;
    printk(KERN_INFO"%s entery\n",__func__);
    //1.初始化HW
    //ioremap
    keypad_iomux_mux_base = ioremap(KEYPAD_IOMUX_MUX_PHY_ADDR,KEYPAD_IOMUX_MUX_PHY_SIZE);
    if(keypad_iomux_mux_base==NULL)
    {
        printk(KERN_INFO"%s ioremap for keypad_iomux_mux_base failed",__func__);
        goto ERR0;
    }
    keypad_iomux_pad_base = ioremap(KEYPAD_IOMUX_PAD_PHY_ADDR,KEYPAD_IOMUX_PAD_PHY_SIZE);
    if(keypad_iomux_pad_base==NULL)
    {
        printk(KERN_INFO"%s ioremap for keypad_iomux_pad_base failed ",__func__);
        goto ERR1;
    }
    keypad_gpio_base = ioremap(KEYPAD_GPIO_PHY_ADDR,KEYPAD_GPIO_PHY_SIZE);
    if(keypad_gpio_base==NULL)
    {
        printk(KERN_INFO"%s ioremap for keypad_gpio_base failed",__func__);
        goto ERR2;
    }
    keypad_gpio_dr = keypad_gpio_base +  KEYPAD_GPIO_DR_OFFSET;
    keypad_gpio_gdir = keypad_gpio_base +  KEYPAD_GPIO_GDIR_OFFSET;
    keypad_gpio_psr = keypad_gpio_base +  KEYPAD_GPIO_PSR_OFFSET;
    keypad_gpio_icr1 = keypad_gpio_base +  KEYPAD_GPIO_ICR1_OFFSET;
    keypad_gpio_icr2 = keypad_gpio_base +  KEYPAD_GPIO_ICR2_OFFSET;
    keypad_gpio_imr = keypad_gpio_base +  KEYPAD_GPIO_IMR_OFFSET;
    keypad_gpio_isr = keypad_gpio_base +  KEYPAD_GPIO_ISR_OFFSET;
    keypad_gpio_edge_sel = keypad_gpio_base +  KEYPAD_GPIO_EDGE_SEL_OFFSET;
    //初始化rows[]和cols[]数据
    rows_sel = 0;
    cols_sel = 0;
    for(i=0;i<rows_size;i++)
    {
        //初始化iomux
        val = ioread32(keypad_iomux_mux_base+rows[i].iomux_offset);
        val = (val & (~0x17)) | 0x5;  //set gpio mode
        iowrite32(val,keypad_iomux_mux_base+rows[i].iomux_offset);
        val = ioread32(keypad_iomux_pad_base+rows[i].iomux_offset);
        val = (val & (~0x1f8f9)) | 0xb0b0;  //pull up
        iowrite32(val,keypad_iomux_pad_base+rows[i].iomux_offset);
        rows_sel |= 1<<rows[i].gpio_nr;
        rows[i].irq = gpio_to_irq(rows[i].gpio);
    }
    for(i=0;i<cols_size;i++)
    {
        //初始化iomux
        val = ioread32(keypad_iomux_mux_base+cols[i].iomux_offset);
        val = (val & (~0x17)) | 0x5;  //set gpio mode
        iowrite32(val,keypad_iomux_mux_base+cols[i].iomux_offset);
        val = ioread32(keypad_iomux_pad_base+cols[i].iomux_offset);
        val = (val & (~0x1f8f9)) | 0x90b0; //keep
        iowrite32(val,keypad_iomux_pad_base+cols[i].iomux_offset);
        cols_sel |= 1<<cols[i].gpio_nr;
    }
    //初始化gpio
    val = ioread32(keypad_gpio_dr);
    val &= ~(rows_sel|cols_sel); //clear
    iowrite32(val,keypad_gpio_dr);
    
    val = ioread32(keypad_gpio_gdir);
    val &= (~rows_sel); //set input
    val |= cols_sel;    //set output
    iowrite32(val,keypad_gpio_gdir);
    printk(KERN_INFO"%s already init %dR x %dC keypad pins\n",__func__,rows_size,cols_size);
    //2.注册input设备
    input = input_allocate_device();
    if(input==NULL)
    {
        printk(KERN_INFO"%s input_allocate_device failed\n",__func__);
        goto ERR3;
    }
    input->name = "rha-keypad";
    //input->phys = "gpio/rha-keypad";
    //input->dev.parent = NULL;
    input->open = keypad_open;
    input->close = keypad_close;
    //input->id.bustype = BUS_HOST;
    //input->id.vendor = 0xf001;
    //input->id.product = 0xf001;
    //input->id.version = 0xf100;
    for(i=0;i<rows_size;i++)
        for(j=0;j<cols_size;j++)
            input_set_capability(input,EV_KEY,key[i][j]);
    ret = input_register_device(input);
    if(ret)
    {
        printk(KERN_INFO"%s input_register_device failed\n",__func__);
        goto ERR4;
    }
    //3.创建工作队列
    keypad_wq = create_workqueue("keypad workqueue");
    if(keypad_wq==NULL)
    {
        printk(KERN_INFO"%s create_workqueue failed",__func__);
        goto ERR5;
    }
    INIT_DELAYED_WORK(&keypad_work,keypad_func);
    //4.注册中断处理函数
    for(i=0;i<rows_size;i++)
    {
        ret = request_irq(rows[i].irq,
                keypad_handler,
                IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING,
                "rha-keypad",NULL);
        if(ret)
        {
            printk(KERN_INFO"%s requeset_irq failed,ret=%d\n",__func__,ret);
            while(i>0)
            {
                i--;
                free_irq(rows[i].irq,NULL);
            }
            goto ERR6;
        }
        else
        {
            printk(KERN_INFO"%s requeset_irq %d ok\n",__func__,rows[i].irq);
        }
    }
    //禁止中断
    iowrite32(ioread32(keypad_gpio_imr)&(~rows_sel),keypad_gpio_imr);
    printk(KERN_INFO"%s ok\n",__func__);
    return 0;
ERR6:
    flush_workqueue(keypad_wq);
    destroy_workqueue(keypad_wq);
ERR5:
    input_unregister_device(input);
ERR4:
    input_free_device(input);
ERR3:
    iounmap(keypad_gpio_base);
ERR2:
    iounmap(keypad_iomux_pad_base);
ERR1:
    iounmap(keypad_iomux_mux_base);
ERR0:
    printk(KERN_INFO"%s failed\n",__func__);
    return -1;
}

static void __exit keypad_exit(void)
{
    int i;
    printk(KERN_INFO"%s entery\n",__func__);
    //1.释放中断
    for(i=0;i<rows_size;i++)
        free_irq(rows[i].irq,NULL);
    //2注销工作队列
    flush_workqueue(keypad_wq);
    destroy_workqueue(keypad_wq);
    //3.释放input设备
    input_unregister_device(input);
    input_free_device(input);
    //4.反初始化HW
    iounmap(keypad_iomux_mux_base);
    iounmap(keypad_iomux_pad_base);
    iounmap(keypad_gpio_base);
    printk(KERN_INFO"%s ok\n",__func__);
}

module_init(keypad_init);
module_exit(keypad_exit);
MODULE_AUTHOR("RHA,Liu Lu");
MODULE_LICENSE("GPL"); 
MODULE_DESCRIPTION("Keypad driver for RHA PIS");

