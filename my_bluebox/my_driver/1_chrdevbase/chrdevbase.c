/*
 * @Author: error: git config user.name & please set dead value or install git
 * @Date: 2024-08-13 15:30:07
 * @LastEditTime: 2024-08-13 16:16:52
 * @FilePath: /chrdevbase.c
 * @Description:
 * Copyright (c) 2024 by error: git config user.name & please set dead value or install git, All Rights Reserved.
 */
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>

#define CHRDEVBASE_MAJOR 200
#define CHRDEVBASE_NAME "chrdevbase"

static char readbuf[100];
static char writebuf[100];
static char kerneldata[] = {"kernel data!"};

static int chrdevbase_open(struct inode *inode, struct file *filp)
{
    /* data */
    printk("chrdevbase open!\r\n");
    return 0;
};

static ssize_t chrdevbase_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
    int retvalue = 0;
    memcpy(readbuf, kerneldata, sizeof(kerneldata));
    retvalue = copy_to_user(buf, readbuf, cnt);

    if (retvalue == 0)
    {
        printk("kernel senddata ok \r\n");
    }
    else
    {
        printk("kernel senddata failed \r\n");
    }

    return 0;
}

static ssize_t chrdevbase_write(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
    int retvalue = 0;
    retvalue = copy_from_user(writebuf, buf, cnt);
    if (retvalue == 0)
    {
        printk("kernel recevdata:%s\r\n", writebuf);
    }
    else
    {
        printk("kernel recevdata failed!\r\n");
    }
    return 0;
}

static int chrdevbase_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static struct file_operations chrdevbase_fops = {
    .owner = THIS_MODULE,
    .open = chrdevbase_open,
    .read = chrdevbase_read,
    .write = chrdevbase_write,
    .release = chrdevbase_release,
};

static int __init chrdevbase_init(void)
{
    int retvalue = 0;
    retvalue = register_chrdev(CHRDEVBASE_MAJOR, CHRDEVBASE_NAME, &chrdevbase_fops);
    if (retvalue < 0)
    {
        printk("chrdevbase driver register failed\r\n");
    }
    printk("chrdevbase_init()\r\n");
    return 0;
}

static void __exit chrdevbase_exit(void)
{
    unregister_chrdev(CHRDEVBASE_MAJOR, CHRDEVBASE_NAME);
    printk("chrdevbase_exit()\r\n");
}

module_init(chrdevbase_init);
module_exit(chrdevbase_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("zuozhongkai");