/*
 * @Author: error: git config user.name & please set dead value or install git
 * @Date: 2024-08-14 15:33:56
 * @LastEditTime: 2024-08-19 16:23:35
 * @FilePath: /dtsled.c
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

#define DTSLED_CNT 1         // 设备号个数
#define DTSLED_NAME "dtsled" // 设备名称

#define LEDOFF 0 /* 关灯 */
#define LEDON 1  /* 开灯 */

struct dtsled_dev
{
    dev_t devid;            /* 设备号 */
    struct cdev cdev;       /* cdev */
    struct class *class;    /* 类 */
    struct device *device;  /* 设备 */
    int major;              /* 主设备号 */
    int minor;              /* 次设备号 */
    struct device_node *nd; /* 设备节点 */
};
struct dtsled_dev dtsled;

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

/*
 * @description		: 打开设备
 * @param - inode 	: 传递给驱动的inode
 * @param - filp 	: 设备文件，file结构体有个叫做private_data的成员变量
 * 					  一般在open的时候将private_data指向设备结构体。
 * @return 			: 0 成功;其他 失败
 */
static int dtsled_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &dtsled; /* 设置私有数据 */
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
static ssize_t dtsled_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
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
static ssize_t dtsled_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
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

    if (ledstat == LEDON)
    {
        // led_switch(LEDON); /* 打开LED灯 */
        gpio5_registers->DR |= (1 << 4);
    }
    else if (ledstat == LEDOFF)
    {
        // led_switch(LEDOFF); /* 关闭LED灯 */
        gpio5_registers->DR &= ~(1 << 4);
    }
    return 0;
}

/*
 * @description		: 关闭/释放设备
 * @param - filp 	: 要关闭的设备文件(文件描述符)
 * @return 			: 0 成功;其他 失败
 */
static int dtsled_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static struct file_operations dtsled_fops = {
    .owner = THIS_MODULE,
    .open = dtsled_open,
    .read = dtsled_read,
    .write = dtsled_write,
    .release = dtsled_release,
};

/**
 * @brief 入口
 *
 * @return int
 */
static int __init dtsled_init(void)
{
    // u32 val = 0;
    int ret = 0;
    u32 regdata[6];
    const char *str;
    struct property *proper;

    /* 获取设备树中的属性数据 */
    /* 1、获取设备节点：alphaled */
    dtsled.nd = of_find_node_by_path("/alphaled");
    if (dtsled.nd == NULL)
    {
        device_destroy(dtsled.class, dtsled.devid);
        printk("alphaled node nost find!\r\n");
        return -EINVAL;
    }
    else
    {
        printk("alphaled node find!\r\n");
    }

    /* 2、获取compatible属性内容 */
    proper = of_find_property(dtsled.nd, "compatible", NULL);
    if (proper == NULL)
    {
        printk("compatible property find failed\r\n");
    }
    else
    {
        printk("compatible = %s\r\n", (char *)proper->value);
    }

    /* 3、获取status属性内容 */
    ret = of_property_read_string(dtsled.nd, "status", &str);
    if (ret < 0)
    {
        printk("status read failed!\r\n");
    }
    else
    {
        printk("status = %s\r\n", str);
    }

    /* 4、获取reg属性内容 */
    ret = of_property_read_u32_array(dtsled.nd, "reg", regdata, 6);
    if (ret < 0)
    {
        printk("reg property read failed!\r\n");
    }
    else
    {
        u8 i = 0;
        printk("reg data:\r\n");
        for (i = 0; i < 6; i++)
            printk("%#X ", regdata[i]);
        printk("\r\n");
    }

    // 开始写led部分的驱动

    // 映射地址
    CCM_CCGR1 = ioremap(regdata[0], regdata[1]);
    IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER4 = ioremap(regdata[2], regdata[3]);
    gpio5_registers = ioremap(regdata[4], sizeof(struct imx6ull_gpio_registers));

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

    dtsled.major = 0;
    // 注册字符设备
    if (dtsled.major)
    {
        dtsled.devid = MKDEV(dtsled.major, 0);
        ret = register_chrdev_region(dtsled.devid, DTSLED_CNT, DTSLED_NAME);
    }
    else // 没有设备号
    {
        alloc_chrdev_region(&dtsled.devid, 0, DTSLED_CNT, DTSLED_NAME);
        dtsled.major = MAJOR(dtsled.devid);
        dtsled.minor = MINOR(dtsled.devid);
    }
    if (ret < 0)
    {
        return ret;
    }

    // 添加字符设备
    dtsled.cdev.owner = THIS_MODULE;
    cdev_init(&dtsled.cdev, &dtsled_fops);
    ret = cdev_add(&dtsled.cdev, dtsled.devid, DTSLED_CNT);
    if (ret < 0)
    {
        unregister_chrdev_region(dtsled.devid, DTSLED_CNT);
        return ret;
    }

    /* 4、创建类 */
    dtsled.class = class_create(THIS_MODULE, DTSLED_NAME);
    if (IS_ERR(dtsled.class))
    {
        cdev_del(&dtsled.cdev);
        unregister_chrdev_region(dtsled.devid, DTSLED_CNT);
        return PTR_ERR(dtsled.class);
    }

    /* 5、创建设备 */
    dtsled.device = device_create(dtsled.class, NULL, dtsled.devid, NULL, DTSLED_NAME);
    if (IS_ERR(dtsled.device))
    {
        class_destroy(dtsled.class);
        return PTR_ERR(dtsled.device);
    }

    return ret;
}

static void __exit dtsled_exit(void)
{
    iounmap(CCM_CCGR1);
    iounmap(IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER4);
    iounmap(gpio5_registers);

    /* 注销字符设备驱动 */
    cdev_del(&dtsled.cdev);                             /*  删除cdev */
    unregister_chrdev_region(dtsled.devid, DTSLED_CNT); /* 注销设备号 */

    device_destroy(dtsled.class, dtsled.devid);
    class_destroy(dtsled.class);
}

// 注册驱动和卸载驱动
module_init(dtsled_init);
module_exit(dtsled_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("bluebox");

// struct imx6ull_gpio_registers
// {
//     volatile unsigned int DR;       // gpio data register
//     volatile unsigned int GDIR;     // gpio direction register
//     volatile unsigned int PSR;      // gpio pad status register
//     volatile unsigned int ICR1;     // gpio interrupt configuration register1
//     volatile unsigned int ICR2;     // gpio interrupt configuration register2
//     volatile unsigned int IMR;      // gpio interrupt mask register
//     volatile unsigned int ISR;      // gpio interrupt status register
//     volatile unsigned int EDGE_SEL; // gpio edge select register
// };

// static volatile unsigned int *CCM_CCGR1;
// static volatile unsigned int *IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER4;
// static struct imx6ull_gpio_registers *gpio5_registers;

// /* newchrled 设备结构体 */
// struct newchrled_dev
// {
//     dev_t devid;           /* 设备号 */
//     struct cdev cdev;      /* cdev */
//     struct class *class;   /* 类 */
//     struct device *device; /* 设备 */
//     int major;             /* 主设备号 */
//     int minor;             /* 次设备号 */
// };
// struct newchrled_dev newchrled; /* led 设备 */

// static int led_open(struct inode *inode, struct file *filp)
// {
//     return 0;
// }

// static ssize_t led_read(struct file *filp, char __user *buf,
//                         size_t cnt, loff_t *offt)
// {
//     return 0;
// }

// static ssize_t led_write(struct file *filp, const char __user *buf,
//                          size_t cnt, loff_t *offt)
// {
//     int retvalue;
//     unsigned char databuf[1];
//     unsigned char ledstat;

//     retvalue = copy_from_user(databuf, buf, cnt);
//     if (retvalue < 0)
//     {
//         printk("kernel write failed!\r\n");
//         return -EFAULT;
//     }

//     ledstat = databuf[0]; /* 获取状态值 */
//     // unsigned int mask = (1 << 1) | (1 << 4) | (1 << 5) | (1 << 8); // 位掩码，设置 1、4、5 和 8 位

//     // 低电平点亮
//     if (ledstat == LEDON)
//     {
//         //* 打开 LED 灯 */
//         gpio5_registers->DR |= (1 << 4);
//         // gpio5_registers->DR |= mask; // 设置掩码所表示的位，设置为 1
//     }
//     else if (ledstat == LEDOFF)
//     {
//         /* 关闭 LED 灯 */

//         gpio5_registers->DR &= ~(1 << 4);
//         // gpio5_registers->DR &= ~mask; // 清除掩码所表示的位，设置为 0
//     }
//     return 0;
// }

// static int led_release(struct inode *inode, struct file *filp)
// {
//     return 0;
// }

// static struct file_operations newchrled_fops = {
//     .owner = THIS_MODULE,
//     .open = led_open,
//     .read = led_read,
//     .write = led_write,
//     .release = led_release,
// };

// static int __init led_init(void)
// {
//     int retvalue = 0;

//     // 映射地址
//     CCM_CCGR1 = ioremap(0x20C406C, 4);
//     IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER4 = ioremap(0x02290018, 4);

//     gpio5_registers = ioremap(0x20AC000, sizeof(struct imx6ull_gpio_registers));

//     // 开启时钟
//     *CCM_CCGR1 |= 0x11;

//     // 配置io口为GPIO口
//     *IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER4 &= ~(0xf);
//     *IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER4 |= 0x5;

//     // 设置gpio口为输出模式
//     gpio5_registers->GDIR = (1 << 4);
//     // 设置gpio口为高电平（低电平点亮）
//     gpio5_registers->DR |= (1 << 4);
//     gpio5_registers->DR &= ~(1 << 4);

//     /* 注册字符设备驱动 */
//     /* 1、创建设备号 */
//     if (newchrled.major)
//     { /* 定义了设备号 */
//         newchrled.devid = MKDEV(newchrled.major, 0);
//         register_chrdev_region(newchrled.devid, NEWCHRLED_CNT,
//                                NEWCHRLED_NAME);
//     }
//     else
//     { /* 没有定义设备号 */
//         alloc_chrdev_region(&newchrled.devid, 0, NEWCHRLED_CNT,
//                             NEWCHRLED_NAME);      /* 申请设备号 */
//         newchrled.major = MAJOR(newchrled.devid); /* 获取主设备号 */
//         newchrled.minor = MINOR(newchrled.devid); /* 获取次设备号 */
//     }
//     printk("newcheled major=%d,minor=%d\r\n", newchrled.major, newchrled.minor);

//     /* 2、初始化 cdev */
//     newchrled.cdev.owner = THIS_MODULE;
//     cdev_init(&newchrled.cdev, &newchrled_fops);

//     /* 3、添加一个 cdev */
//     cdev_add(&newchrled.cdev, newchrled.devid, NEWCHRLED_CNT);
//     /* 4、创建类 */
//     newchrled.class = class_create(THIS_MODULE, NEWCHRLED_NAME);
//     if (IS_ERR(newchrled.class))
//     {
//         return PTR_ERR(newchrled.class);
//     }

//     /* 5、创建设备 */
//     newchrled.device = device_create(newchrled.class, NULL, newchrled.devid, NULL, NEWCHRLED_NAME);
//     if (IS_ERR(newchrled.device))
//     {
//         return PTR_ERR(newchrled.device);
//     }
//     return 0;
// }

// static void __exit led_exit(void)
// {
//     /* 注销字符设备 */
//     cdev_del(&newchrled.cdev); /* 删除 cdev */
//     unregister_chrdev_region(newchrled.devid, NEWCHRLED_CNT);
//     device_destroy(newchrled.class, newchrled.devid);
//     class_destroy(newchrled.class);
// }

// module_init(led_init);
// module_exit(led_exit);
// MODULE_LICENSE("GPL");
// MODULE_AUTHOR("zuozhongkai");
