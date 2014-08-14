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

#define MAJOR_NUM 	245 //改进使用参数的方式传递
#define GPIO_PORT	14

#define led_ON	1
#define led_OFF	2

#define DEVICE_NAME	"mygpio"
#define label		"mygpio"

static int ledgpio_MAJOR = MAJOR_NUM;

//加入linux/device.h linux/cdev.h等自动生成/dev/*

static  struct cdev cdev_ledgpio;	//定义一个cdev结构体变量
struct class *ledgpio_class;		//定义一个class结构体指针变量

/******ioctl函数*********/
static long ledgpio_ioctl(struct file *filp,unsigned int cmd,unsigned long arg)
{
//	unsigned char k;
	unsigned int err;
	err=(gpio_request(GPIO_PORT,label));
	if(err)
	{
		//错误
		printk("the err num is:%d \n",err);
		return 0;
	}
	switch(cmd)	
	{
		case 0:
			if(gpio_direction_output(GPIO_PORT,1) == -ENOSYS)
			{
				printk("direction output error!\n");
			}
			gpio_set_value(GPIO_PORT,0);
			break;
		case 1:
                        if(gpio_direction_output(GPIO_PORT,1) == -ENOSYS)
                        {
                                printk("direction output error!\n");
                        }

			gpio_set_value(GPIO_PORT,1);
			/*gpio设置一个引脚电平状态函数，原函数形式：
			static inline void gpio_set_value(unsigned gpio,int value)第一个参数gpio指定引脚，第二参数设置引脚的高低电平*/
                        /*if(gpio_direction_output(GPIO_PORT,0) == -ENOSYS)
                        {
                                printk("direction output error!\n");
                        }

			k = gpio_get_value(GPIO_PORT);*/
			/*获得第一个引脚的电平状态函数，原型为:
			static inline int gpio_get_value(unsigned gpio)*/
			//printk("<0>\n LED is ON GPP0 = %d\n",k);//打印函数，<>中为级别 
			break;
		default:
			printk("fail \n");
			printk("<0>\n please enter the cmd value again!\n");
		return -EINVAL;
			
	}
	gpio_free(GPIO_PORT);
	return 0;
}

/*static int ledgpio_open(struct inode *inode,struct file *filp)
{
	
}*/

static struct file_operations ledgpio_fops=
{
	.owner = THIS_MODULE,
	.unlocked_ioctl = ledgpio_ioctl,
};

static int __init ledgpio_init(void)
{
	int result;
//	unsigned char i=0;
	dev_t devno = MKDEV(MAJOR_NUM,0);
	result = alloc_chrdev_region(&devno,0,1,DEVICE_NAME);
	ledgpio_MAJOR = MAJOR(devno);
	
	if(result)
	{
		printk(KERN_NOTICE"Error %d register ledgpio",result);
		return result;
	}
	cdev_init(&cdev_ledgpio,&ledgpio_fops);			//对空间起到一个清零的作用
	result = cdev_add(&cdev_ledgpio,devno,1);
	if(result)
	{
		printk(KERN_NOTICE"Error %d a ding ledgpio",result);
		return result;
	}
	ledgpio_class = class_create(THIS_MODULE,"ledgpio_class");
	/*创建一个设备并且将其在sysfs中注册*/
	device_create(ledgpio_class,NULL,MKDEV(ledgpio_MAJOR,0),"NULL",DEVICE_NAME);
        result=(gpio_request(GPIO_PORT,label));
	if(result)
	{
		printk("err request!\n");
		return 0;
	}
	if(gpio_direction_output(GPIO_PORT,1) == -ENOSYS)	
	{
		printk("direction output error!\n");
	}
	gpio_set_value(GPIO_PORT,1);
	gpio_free(GPIO_PORT);

	printk(KERN_NOTICE"GPIO set state!\n");
	return 0;
}

/*设备驱动模块卸载函数*/
static void __exit ledgpio_exit(void)
{
	cdev_del(&cdev_ledgpio);
	/*当一个字符设备驱动不再需要的时候，比如模块卸载就是可以用cdev_del()函数来释放cdev占用的内存*/
	unregister_chrdev_region(MKDEV(ledgpio_MAJOR,0),1);//注销设备区域
	device_destroy(ledgpio_class,MKDEV(ledgpio_MAJOR,0));
	/*删除处于/dev目录下手驱动节点*/
	class_destroy(ledgpio_class);
	/*删除我们所创建的class*/
}

module_init(ledgpio_init);
module_exit(ledgpio_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("SHIZHAI");
