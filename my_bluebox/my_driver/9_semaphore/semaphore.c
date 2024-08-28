/*
 * @Author: error: git config user.name & please set dead value or install git
 * @Date: 2024-08-14 15:33:56
 * @LastEditTime: 2024-08-20 16:20:53
 * @FilePath: /semaphore.c
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
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define GPIOLED_CNT 1          // 设备号个数
#define GPIOLED_NAME "gpioled" // 设备名称

#define LEDOFF 0 /* 关灯 */
#define LEDON 1  /* 开灯 */

struct gpioled_dev
{
    dev_t devid;            /* 设备号 */
    struct cdev cdev;       /* cdev */
    struct class *class;    /* 类 */
    struct device *device;  /* 设备 */
    int major;              /* 主设备号 */
    int minor;              /* 次设备号 */
    struct device_node *nd; /* 设备节点 */
    int led_gpio;           /*led所使用的gpio编号 */
    struct semaphore sem;   /*信号量*/
};
struct gpioled_dev gpioled;

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

/*
 * @description		: 打开设备
 * @param - inode 	: 传递给驱动的inode
 * @param - filp 	: 设备文件，file结构体有个叫做private_data的成员变量
 * 					  一般在open的时候将private_data指向设备结构体。
 * @return 			: 0 成功;其他 失败
 */
static int gpioled_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &gpioled; /* 设置私有数据 */

    /* 获取信号量,进入休眠状态的进程可以被信号打断 */
    if (down_interruptible(&gpioled.sem))
    {
        return -ERESTARTSYS;
    }

    return 0;
}

/*
 * @description		: 从设备读取数据
 * @param - filp 	: 要打开的设备文件(文件描述符)
 * @param - buf 	: 返回给用户空间的数据缓冲区
 * @param - cnt 	: 要读取的数据长度
 * @param - offt 	: 相对于文件首地址的偏移
 * @return 			: 读取的字节数，如果为负值，表示读取失败
 */
static ssize_t gpioled_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
    return 0;
}

/*
 * @description		: 向设备写数据
 * @param - filp 	: 设备文件，表示打开的文件描述符
 * @param - buf 	: 要写给设备写入的数据
 * @param - cnt 	: 要写入的数据长度
 * @param - offt 	: 相对于文件首地址的偏移
 * @return 			: 写入的字节数，如果为负值，表示写入失败
 */
static ssize_t gpioled_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
    int retvalue;
    unsigned char databuf[1];
    unsigned char ledstat;
    struct gpioled_dev *dev = filp->private_data;

    retvalue = copy_from_user(databuf, buf, cnt);
    if (retvalue < 0)
    {
        printk("kernel write failed!\r\n");
        return -EFAULT;
    }

    ledstat = databuf[0]; /* 获取状态值 */

    if (ledstat == LEDON)
    {
        /* 打开LED灯 */
        gpio_set_value(dev->led_gpio, 1);
    }
    else if (ledstat == LEDOFF)
    {
        /* 关闭LED灯 */
        gpio_set_value(dev->led_gpio, 0);
    }
    return 0;
}

/*
 * @description		: 关闭/释放设备
 * @param - filp 	: 要关闭的设备文件(文件描述符)
 * @return 			: 0 成功;其他 失败
 */
static int gpioled_release(struct inode *inode, struct file *filp)
{
    struct gpioled_dev *dev = filp->private_data;

    up(&dev->sem); /*释放信号量，信号量值加1*/

    return 0;
}

static struct file_operations gpioled_fops = {
    .owner = THIS_MODULE,
    .open = gpioled_open,
    .read = gpioled_read,
    .write = gpioled_write,
    .release = gpioled_release,
};

/**
 * @brief 入口
 *
 * @return int
 */
static int __init gpioled_init(void)
{
    int ret = 0;

    // 初始化信号量
    sema_init(&gpioled.sem, 1);

    /* 获取设备树中的属性数据 */
    /* 1、获取设备节点：alphaled */
    gpioled.nd = of_find_node_by_path("/gpioled");
    if (gpioled.nd == NULL)
    {
        device_destroy(gpioled.class, gpioled.devid);
        printk("gpioled node nost find!\r\n");
        return -EINVAL;
    }
    else
    {
        printk("gpioled node find!\r\n");
    }

    gpioled.led_gpio = of_get_named_gpio(gpioled.nd, "led-gpio", 0);
    if (gpioled.led_gpio < 0)
    {
        printk("can`t get led-gpio\r\n");
        return -EINVAL;
    }
    printk("led-gpio num = %d\r\n", gpioled.led_gpio);

    ret = gpio_direction_output(gpioled.led_gpio, 1);
    if (ret < 0)
    {
        printk("can`t set led-gpio\r\n");
        return -EINVAL;
    }
    gpio_set_value(gpioled.led_gpio, 0);

    /*注册字符设备驱动*/
    // 创建设备号
    if (gpioled.major)
    {
        gpioled.devid = MKDEV(gpioled.major, 0);
        ret = register_chrdev_region(gpioled.devid, GPIOLED_CNT, GPIOLED_NAME);
    }
    else // 没有设备号
    {
        alloc_chrdev_region(&gpioled.devid, 0, GPIOLED_CNT, GPIOLED_NAME);
        gpioled.major = MAJOR(gpioled.devid);
        gpioled.minor = MINOR(gpioled.devid);
    }
    printk("gpio major = %d, minor = %d\r\n", gpioled.major, gpioled.minor);
    if (ret < 0)
    {
        return ret;
    }

    // 初始化cdev
    gpioled.cdev.owner = THIS_MODULE;
    cdev_init(&gpioled.cdev, &gpioled_fops);
    // 添加一个cdev
    ret = cdev_add(&gpioled.cdev, gpioled.devid, GPIOLED_CNT);
    if (ret < 0)
    {
        unregister_chrdev_region(gpioled.devid, GPIOLED_CNT);
        return ret;
    }

    // 创建类
    gpioled.class = class_create(THIS_MODULE, GPIOLED_NAME);
    if (IS_ERR(gpioled.class))
    {
        cdev_del(&gpioled.cdev);
        unregister_chrdev_region(gpioled.devid, GPIOLED_CNT);
        return PTR_ERR(gpioled.class);
    }

    // 创建设备
    gpioled.device = device_create(gpioled.class, NULL, gpioled.devid, NULL, GPIOLED_NAME);
    if (IS_ERR(gpioled.device))
    {
        class_destroy(gpioled.class);
        return PTR_ERR(gpioled.device);
    }

    return ret;
}

static void __exit gpioled_exit(void)
{

    /* 注销字符设备驱动 */
    cdev_del(&gpioled.cdev);                              /*  删除cdev */
    unregister_chrdev_region(gpioled.devid, GPIOLED_CNT); /* 注销设备号 */

    device_destroy(gpioled.class, gpioled.devid);
    class_destroy(gpioled.class);
}

// 注册驱动和卸载驱动
module_init(gpioled_init);
module_exit(gpioled_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("bluebox");
