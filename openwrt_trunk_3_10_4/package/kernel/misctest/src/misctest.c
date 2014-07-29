/************************************************
**Filename:misctest.c
**Author:shizhai
**Time:13-12-23
*************************************************/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kmod.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <asm/uacces.h>

#define DEVICE_NAME	"misctest"
#define	GLOBALMEM_SIZE	0x1000		//全局内存大小
#define MEM_CLEAR	0X1		//清空全局内存

#define GLOBAL_BUG

struct globalmem_dev
{
	struct cdev cdev;
	unsigned char mem[GLOBALMEM_SIZE];
};

struct globalmem_dev *global_dev;

//打开文件函数
static int globalmem_open(struct inode *inode,struct file *filp)
{
	//将设备结构体指针同仁给私有数据指针
	filp->private_data = global_dev;
	return 0;
}

//文件函数
static ssize_t


