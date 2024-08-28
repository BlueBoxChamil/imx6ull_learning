/*
 * @Author: error: git config user.name & please set dead value or install git
 * @Date: 2024-08-13 16:41:35
 * @LastEditTime: 2024-08-14 15:10:10
 * @FilePath: /led.c
 * @Description:
 * Copyright (c) 2024 by error: git config user.name & please set dead value or install git, All Rights Reserved.
 */
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define LED_MAJOR 200  /* 主设备号 */
#define LED_NAME "led" /* 设备名字 */
#define LEDOFF 0       /* 关灯 */
#define LEDON 1        /* 开灯 */
/* 寄存器物理地址 */
#define CCM_CCGR1_BASE (0X020C406C)
#define SW_MUX_GPIO1_IO03_BASE (0x02290018) //(0X020E0068)
#define SW_PAD_GPIO1_IO03_BASE (0x0229005C) //(0X020E02F4)
#define GPIO1_DR_BASE (0x020AC000)          //(0X0209C000)
#define GPIO1_GDIR_BASE (0x020AC005)        ///(0X0209C004)
/* 映射后的寄存器虚拟地址指针 */

static volatile unsigned int *CCM_CCGR1;

static volatile unsigned int *IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER4;
static volatile unsigned int *IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER1;
static volatile unsigned int *IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER8;
static volatile unsigned int *IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER5;
struct imx6ull_gpio_registers
{
    volatile unsigned int DR;       // gpio data register
    volatile unsigned int GDIR;     // gpio direction register
    volatile unsigned int PSR;      // gpio pad status register
    volatile unsigned int ICR1;     // gpio interrupt configuration register1
    volatile unsigned int ICR2;     // gpio interrupt configuration register2
    volatile unsigned int IMR;      // gpio interrupt mask register
    volatile unsigned int ISR;      // gpio interrupt status register
    volatile unsigned int EDGE_SEL; // gpio edge select register
};

static struct imx6ull_gpio_registers *gpio5_registers;

static int led_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static ssize_t led_read(struct file *filp, char __user *buf,
                        size_t cnt, loff_t *offt)
{
    return 0;
}

static ssize_t led_write(struct file *filp, const char __user *buf,
                         size_t cnt, loff_t *offt)
{
    int retvalue;
    unsigned char databuf[1];
    unsigned char ledstat;

    retvalue = copy_from_user(databuf, buf, cnt);
    if (retvalue < 0)
    {
        printk("kernel write failed!\r\n");
        return -EFAULT;
    }

    ledstat = databuf[0];                                          /* 获取状态值 */
    unsigned int mask = (1 << 1) | (1 << 4) | (1 << 5) | (1 << 8); // 位掩码，设置 1、4、5 和 8 位

    if (ledstat == LEDON)
    {
        //* 打开 LED 灯 */
        // gpio5_registers->DR &= ~(1 << 4);
        gpio5_registers->DR &= ~mask; // 清除掩码所表示的位，设置为 0
    }
    else if (ledstat == LEDOFF)
    {
        /* 关闭 LED 灯 */
        // gpio5_registers->DR |= (1 << 4);
        gpio5_registers->DR |= mask; // 设置掩码所表示的位，设置为 1
    }
    return 0;
}

static int led_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .read = led_read,
    .write = led_write,
    .release = led_release,
};

static int __init led_init(void)
{
    int retvalue = 0;
    u32 val = 0;

    // 映射地址
    CCM_CCGR1 = ioremap(0x20C406C, 4);
    IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER4 = ioremap(0x02290018, 4);

    IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER1 = ioremap(0x0229000C, 4);
    IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER5 = ioremap(0x0229001C, 4);
    IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER8 = ioremap(0x02290028, 4);

    gpio5_registers = ioremap(0x20AC000, sizeof(struct imx6ull_gpio_registers));

    // 开启时钟
    *CCM_CCGR1 |= 0x11;

    // 配置io口为GPIO口
    *IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER4 &= ~(0xf);
    *IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER4 |= 0x5;
    *IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER1 &= ~(0xf);
    *IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER1 |= 0x5;
    *IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER5 &= ~(0xf);
    *IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER5 |= 0x5;
    *IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER8 &= ~(0xf);
    *IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER8 |= 0x5;

    // 设置gpio口为输出模式
    gpio5_registers->GDIR = (1 << 4) | (1 << 1) | (1 << 5) | (1 << 8);
    // 设置gpio口为高电平（低电平点亮）
    gpio5_registers->DR |= (1 << 4) | (1 << 1) | (1 << 5) | (1 << 8);

    /* 6、注册字符设备驱动 */
    retvalue = register_chrdev(LED_MAJOR, LED_NAME, &led_fops);
    if (retvalue < 0)
    {
        printk("register chrdev failed!\r\n");
        return -EIO;
    }
    return 0;
}

static void __exit led_exit(void)
{
    /* 取消映射 */
    // iounmap(IMX6U_CCM_CCGR1);
    // iounmap(SW_MUX_GPIO1_IO03);
    // iounmap(SW_PAD_GPIO1_IO03);
    // iounmap(GPIO1_DR);
    // iounmap(GPIO1_GDIR);
    /* 注销字符设备驱动 */
    unregister_chrdev(LED_MAJOR, LED_NAME);
}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("zuozhongkai");