/*
 * @Author: error: git config user.name & please set dead value or install git
 * @Date: 2024-08-14 15:33:56
 * @LastEditTime: 2024-08-14 16:58:53
 * @FilePath: /newchrled.c
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
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

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

static volatile unsigned int *CCM_CCGR1;
static volatile unsigned int *IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER4;
static struct imx6ull_gpio_registers *gpio5_registers;

#define NEWCHRLED_CNT 1            /* 设备号个数 */
#define NEWCHRLED_NAME "newchrled" /* 名字 */
#define LEDOFF 0                   /* 关灯 */
#define LEDON 1                    /* 开灯 */

/* newchrled 设备结构体 */
struct newchrled_dev
{
    dev_t devid;           /* 设备号 */
    struct cdev cdev;      /* cdev */
    struct class *class;   /* 类 */
    struct device *device; /* 设备 */
    int major;             /* 主设备号 */
    int minor;             /* 次设备号 */
};

struct newchrled_dev newchrled; /* led 设备 */

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

    ledstat = databuf[0]; /* 获取状态值 */
    // unsigned int mask = (1 << 1) | (1 << 4) | (1 << 5) | (1 << 8); // 位掩码，设置 1、4、5 和 8 位

    // 低电平点亮
    if (ledstat == LEDON)
    {
        //* 打开 LED 灯 */
        gpio5_registers->DR |= (1 << 4);
        // gpio5_registers->DR |= mask; // 设置掩码所表示的位，设置为 1
    }
    else if (ledstat == LEDOFF)
    {
        /* 关闭 LED 灯 */

        gpio5_registers->DR &= ~(1 << 4);
        // gpio5_registers->DR &= ~mask; // 清除掩码所表示的位，设置为 0
    }
    return 0;
}

static int led_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static struct file_operations newchrled_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .read = led_read,
    .write = led_write,
    .release = led_release,
};

static int __init led_init(void)
{
    int retvalue = 0;

    // 映射地址
    CCM_CCGR1 = ioremap(0x20C406C, 4);
    IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER4 = ioremap(0x02290018, 4);

    gpio5_registers = ioremap(0x20AC000, sizeof(struct imx6ull_gpio_registers));

    // 开启时钟
    *CCM_CCGR1 |= 0x11;

    // 配置io口为GPIO口
    *IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER4 &= ~(0xf);
    *IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER4 |= 0x5;

    // 设置gpio口为输出模式
    gpio5_registers->GDIR = (1 << 4);
    // 设置gpio口为高电平（低电平点亮）
    gpio5_registers->DR |= (1 << 4);
    gpio5_registers->DR &= ~(1 << 4);

    /* 注册字符设备驱动 */
    /* 1、创建设备号 */
    if (newchrled.major)
    { /* 定义了设备号 */
        newchrled.devid = MKDEV(newchrled.major, 0);
        register_chrdev_region(newchrled.devid, NEWCHRLED_CNT,
                               NEWCHRLED_NAME);
    }
    else
    { /* 没有定义设备号 */
        alloc_chrdev_region(&newchrled.devid, 0, NEWCHRLED_CNT,
                            NEWCHRLED_NAME);      /* 申请设备号 */
        newchrled.major = MAJOR(newchrled.devid); /* 获取主设备号 */
        newchrled.minor = MINOR(newchrled.devid); /* 获取次设备号 */
    }
    printk("newcheled major=%d,minor=%d\r\n", newchrled.major, newchrled.minor);

    /* 2、初始化 cdev */
    newchrled.cdev.owner = THIS_MODULE;
    cdev_init(&newchrled.cdev, &newchrled_fops);

    /* 3、添加一个 cdev */
    cdev_add(&newchrled.cdev, newchrled.devid, NEWCHRLED_CNT);
    /* 4、创建类 */
    newchrled.class = class_create(THIS_MODULE, NEWCHRLED_NAME);
    if (IS_ERR(newchrled.class))
    {
        return PTR_ERR(newchrled.class);
    }

    /* 5、创建设备 */
    newchrled.device = device_create(newchrled.class, NULL, newchrled.devid, NULL, NEWCHRLED_NAME);
    if (IS_ERR(newchrled.device))
    {
        return PTR_ERR(newchrled.device);
    }
    return 0;
}

static void __exit led_exit(void)
{
    /* 注销字符设备 */
    cdev_del(&newchrled.cdev); /* 删除 cdev */
    unregister_chrdev_region(newchrled.devid, NEWCHRLED_CNT);
    device_destroy(newchrled.class, newchrled.devid);
    class_destroy(newchrled.class);
}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("zuozhongkai");
