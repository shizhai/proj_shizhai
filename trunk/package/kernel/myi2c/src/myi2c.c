#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/i2c.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/delay.h>

#define MYI2C_MAJOR	250

static int myi2c_major = MYI2C_MAJOR;

static ssize_t myi2c_read(struct file *file,char __user *buf,size_t count,loff_t *ppos);
static ssize_t myi2c_write(struct file *file,const char __user *buf,size_t count,loff_t *ppos);
static int myi2c_open(struct inode *inode,struct file *file);
static int myi2c_ioctl(struct file *file,unsigned cmd,unsigned long arg);
static int myi2c_release(struct inode *inodep,struct file *file);

struct myi2c_dev{
	struct i2c_client *client;
	char name[30];
	unsigned short current_pointer;
	struct cdev cdev;
};

struct myi2c_dev *myi2c_devp;

static int myi2c_open(struct inode *inode,struct file *file)
{
	file->private_data=myi2c_devp;
	return 0;
}

static ssize_t myi2c_read(struct file *file,char __user *buf,size_t count,loff_t *ppos)
{
	int i = 0;
	int transferred = 0;
	int ret,my_buf[512];
	struct myi2c_dev *dev=(struct myi2c_dev *)file->private_data;
	dev->current_pointer=*ppos;
	if(i2c_check_functionality(dev->client->adapter,I2C_FUNC_SMBUS_READ_BYTE_DATA))
	{
		while(transferred < count)
		{
			ret = i2c_smbus_read_byte_data(dev->client,dev->current_pointer + i);
			my_buf[i++] = (unsigned short)ret;
			transferred += 1;
		}
		copy_to_user(buf,(void*)my_buf,transferred);
		dev->current_pointer += transferred;
	}
	return transferred;
}

static ssize_t myi2c_write(struct file *file,const char __user *buf,size_t count,loff_t *ppos) 
{
	int i = 0;
	int transferred = 0;
	int ret,my_buf[512];
	
	struct myi2c_dev *dev = (struct myi2c_dev *)file->private_data;
	dev->current_pointer = *ppos;
	if(i2c_check_functionality (dev->client->adapter,I2C_FUNC_SMBUS_BYTE_DATA))
	{
		copy_from_user(my_buf,buf,count);	
		while(transferred < count)
		{
			ret=i2c_smbus_write_byte_data(dev->client,dev->current_pointer+i,my_buf[i]);
			i++;
			transferred++;
		}
		dev->current_pointer += transferred;
	}
	return transferred;
}

static int myi2c_ioctl(struct file *file,unsigned int cmd,unsigned long arg)
{
	return 0;
}

static int myi2c_release(struct inode *inodep,struct file *file)
{
	file->private_data=NULL;
	return 0;
}

static const struct file_operations myi2c_fops={
	.owner = THIS_MODULE,
	.open = myi2c_open,
	.read = myi2c_read,
	.write = myi2c_write,
	.unlocked_ioctl = myi2c_ioctl,
	.release = myi2c_release,
};

static void myi2c_setup_cdev(struct myi2c_dev *dev,int index)
{
	int err;
	dev_t devnum;
	devnum=MKDEV(myi2c_major,index);
	cdev_init(&dev->cdev,&myi2c_fops);
	dev->cdev.owner=THIS_MODULE;
	err=cdev_add(&dev->cdev,devnum,1);
	if(err)
		printk(KERN_NOTICE"Error %d adding myi2c %d \n",err,index);
}

static int myi2c_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
	int ret;
	printk(KERN_NOTICE"myi2c probe is start \n");
	dev_t devnum;
	devnum=MKDEV(myi2c_major,0);
	if(myi2c_major)ret=register_chrdev_region(devnum,1,"myi2c");
	else 
	{
		ret=alloc_chrdev_region(&devnum,0,1,"myi2c");
		myi2c_major=MAJOR(devnum);
	}
	if(ret<0)
	{
		myi2c_devp=kmalloc(sizeof(struct myi2c_dev),GFP_KERNEL);
		if(!myi2c_devp)
		{
			ret = -ENOMEM;
			goto fail_malloc;
		}
		memset(myi2c_devp,0,sizeof(struct myi2c_dev));
		myi2c_devp->client=client;
		myi2c_setup_cdev(myi2c_devp,0);
		return 0;
fail_malloc:
		unregister_chrdev_region(devnum,1);
		return ret;
	}
}

static int myi2c_remove(struct i2c_client *client) 
{
	cdev_del(&myi2c_devp->cdev);
	kfree(myi2c_devp);
	unregister_chrdev_region(MKDEV(myi2c_major,0),1);
	return 0;
}
static const struct i2c_device_id myi2c_id[]={
	{"myi2c",0},
};

MODULE_DEVICE_TABLE(i2c,myi2c_id);
static struct i2c_driver myi2c_driver={
	.driver={
		.name="myi2c",
		.owner=THIS_MODULE,
	},
	.probe=myi2c_probe,
	.remove=myi2c_remove,
	.id_table=myi2c_id,
};

static int __init myi2c_init(void)
{
	printk(KERN_NOTICE"myi2c is insmod \n");
	return i2c_add_driver(&myi2c_driver);
}
void __exit myi2c_exit(void)
{
	printk(KERN_NOTICE"myi2c is rmmod\n");
	i2c_del_driver(&myi2c_driver);
}
MODULE_DESCRIPTION("myi2c eeprom driver\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("YSPROGRAM");
MODULE_VERSION("V1.0");
module_param(myi2c_major,int,S_IRUGO);
module_init(myi2c_init);
module_exit(myi2c_exit);
