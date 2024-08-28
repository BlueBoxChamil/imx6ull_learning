
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
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/irq.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/platform_device.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define LEDDEV_CNT 1             /* 设备号长度 */
#define LEDDEV_NAME "dtsplatled" /* 设备名字 */
#define LEDOFF 0
#define LEDON 1

/* leddev 设备结构体 */
struct leddev_dev
{
    dev_t devid;              /* 设备号 */
    struct cdev cdev;         /* cdev */
    struct class *class;      /* 类 */
    struct device *device;    /* 设备 */
    int major;                /* 主设备号 */
    struct device_node *node; /*led设备节点*/
    int led0;                 /*led灯gpio标号*/
};

struct leddev_dev leddev; /* led 设备 */

/*
 * @description : 打开设备
 * @param – inode : 传递给驱动的 inode
 * @param - filp : 设备文件，file 结构体有个叫做 private_data 的成员变量
 * 一般在 open 的时候将 private_data 指向设备结构体。
 * @return : 0 成功;其他 失败
 */
static int led_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &leddev; /* 设置私有数据 */
    return 0;
}

/*
 * @description : 向设备写数据
 * @param – filp : 设备文件，表示打开的文件描述符
 * @param - buf : 要写给设备写入的数据
 * @param - cnt : 要写入的数据长度
 * @param - offt : 相对于文件首地址的偏移
 * @return : 写入的字节数，如果为负值，表示写入失败
 */
static ssize_t led_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
    int retvalue;
    unsigned char databuf[1];
    unsigned char ledstat;

    retvalue = copy_from_user(databuf, buf, cnt);
    if (retvalue < 0)
    {
        return -EFAULT;
    }

    ledstat = databuf[0]; /* 获取状态值 */
    if (ledstat == LEDON)
    {
        // led0_switch(LEDON); /* 打开 LED 灯 */
        gpio_set_value(leddev.led0, 1);
    }
    else if (ledstat == LEDOFF)
    {
        // led0_switch(LEDOFF); /* 关闭 LED 灯 */
        gpio_set_value(leddev.led0, 0);
    }
    return 0;
}

/* 设备操作函数 */
static struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .write = led_write,
};

/*
 * @description : flatform 驱动的 probe 函数，当驱动与设备
 * 匹配以后此函数就会执行
 * @param - dev : platform 设备
 * @return : 0，成功;其他负值,失败
 */
static int led_probe(struct platform_device *dev)
{
    printk("led driver and device was matched!\r\n");

    // 设置设备号
    if (leddev.major)
    {
        leddev.devid = MKDEV(leddev.major, 0);
        register_chrdev_region(leddev.devid, LEDDEV_CNT, LEDDEV_NAME);
    }
    else
    {
        alloc_chrdev_region(&leddev.devid, 0, LEDDEV_CNT, LEDDEV_NAME);
        leddev.major = MAJOR(leddev.devid);
    }

    // 注册设备
    cdev_init(&leddev.cdev, &led_fops);
    cdev_add(&leddev.cdev, leddev.devid, LEDDEV_CNT);

    // 创建类
    leddev.class = class_create(THIS_MODULE, LEDDEV_NAME);
    if (IS_ERR(leddev.class))
    {
        return PTR_ERR(leddev.class);
    }

    // 创建设备
    leddev.device = device_create(leddev.class, NULL, leddev.devid, NULL, LEDDEV_NAME);
    if (IS_ERR(leddev.device))
    {
        return PTR_ERR(leddev.device);
    }

    // 初始化io
    leddev.node = of_find_node_by_path("/gpioled");

    if (leddev.node == NULL)
    {
        printk("gpioled node nost find!\r\n");
        return -EINVAL;
    }

    leddev.led0 = of_get_named_gpio(leddev.node, "led-gpio", 0);
    if (leddev.led0 < 0)
    {
        printk("can't get led-gpio\r\n");
        return -EINVAL;
    }

    // 打印获取到的 GPIO 引脚编号
    printk("Got GPIO pin number: %d\r\n", leddev.led0);

    gpio_request(leddev.led0, "led0");
    gpio_direction_output(leddev.led0, 1); /*设置为输出，默认低电平 */

    gpio_set_value(leddev.led0, 0);
    return 0;
}

/*
 * @description :移除 platform 驱动的时候此函数会执行
 * @param - dev : platform 设备
 * @return : 0，成功;其他负值,失败
 */
static int led_remove(struct platform_device *dev)
{
    gpio_set_value(leddev.led0, 0); /* 卸载驱动的时候关闭 LED */

    cdev_del(&leddev.cdev); /* 删除 cdev */
    unregister_chrdev_region(leddev.devid, LEDDEV_CNT);
    device_destroy(leddev.class, leddev.devid);
    class_destroy(leddev.class);
    return 0;
}

/* 匹配列表 */
static const struct of_device_id led_of_match[] = {
    {.compatible = "bluebox_gpio_led"},
    {}};

/* platform 驱动结构体 */
static struct platform_driver led_driver = {
    .driver = {
        .name = "imx6ul-led",           /* 驱动名字，用于和设备匹配 */
        .of_match_table = led_of_match, /* 设备树匹配表 */
    },
    .probe = led_probe,
    .remove = led_remove,
};

/*
 * @description : 驱动模块加载函数
 * @param : 无
 * @return : 无
 */
static int __init leddriver_init(void)
{
    return platform_driver_register(&led_driver);
}

/*
 * @description : 驱动模块卸载函数
 * @param : 无
 * @return : 无
 */
static void __exit leddriver_exit(void)
{
    platform_driver_unregister(&led_driver);
}

module_init(leddriver_init);
module_exit(leddriver_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("zuozhongkai");