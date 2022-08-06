//SPDX-FileCopyrightText: 2022 Ryuichi Ueda ryuichiueda@gmail.com
//SPDX-License-Identifier: GPL-2.0

#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/random.h>
//#include <linux/slab.h>

MODULE_AUTHOR("Ryuichi Ueda");
MODULE_DESCRIPTION("Enjoy ddsc");
MODULE_LICENSE("GPL");

static int devmajor = 0;
static int devminor = 0;
static char* msg = "module [ddsc.o]";

static struct cdev cdv;
static int access_num = 0;
static int num_per_line = 1; //the number of ddsc written in each line
static spinlock_t spn_lock;

static struct class *cls = NULL;

static int ddsc_open(struct inode* inode, struct file* filp);
static ssize_t ddsc_read(struct file* filp, char* buf, size_t count, loff_t* pos);
static ssize_t ddsc_write(struct file* filp, const char* buf, size_t count, loff_t* pos);
static int ddsc_release(struct inode* inode, struct file* filp);
//static int get_pseudo_rand(void);

static int copy_to_user_n(char *buf,const char *str,int str_len,int n);

static struct file_operations ddsc_fops = 
{
	owner   : THIS_MODULE,
	read    : ddsc_read,
	write    : ddsc_write,
	open    : ddsc_open,
	release : ddsc_release,
};

/*
static int get_pseudo_rand(void)
{
	struct timespec64 t;
	getnstimeofday(&t);
	return (int)t.tv_nsec;
}*/

static int ddsc_open(struct inode* inode, struct file* filp)
{
	printk(KERN_INFO "%s : open() called\n", msg);

	spin_lock(&spn_lock);

	if(access_num){
		spin_unlock(&spn_lock);
		return -EBUSY;
	}

	access_num++;
	spin_unlock(&spn_lock);

	return 0;
}

static int copy_to_user_n(char *buf,const char *str,int str_len,int n)
{
	int size = 0;
	int i;
	for(i=0;i<n;i++){
		if(copy_to_user(buf+size,str, str_len)){
			printk( KERN_INFO "%s : copy_to_user failed\n", msg );
			return -EFAULT;
		}

		size += str_len;
		if(i>10)
			return size;
	}

	return size;
}

static ssize_t ddsc_read(struct file* filp, char* buf, size_t count, loff_t* pos)
{
	static unsigned int n = 0;
	int ret = 0;
	int size = 0;
	int tmp;
	int i;

	char dd[] = {0xE3,0x83,0x89,0xE3,0x83,0x89,'\0'};
	char sc[] = {0xE3,0x82,0xB9,0xE3,0x82,0xB3,'\0'};
	char nl[] = {'\n','\0'};

	int r;
	get_random_bytes(&r, sizeof(r));

	if(r%2){
		ret = copy_to_user_n(buf+size,(const char *)dd, sizeof(dd),num_per_line);
	}else{
		ret = copy_to_user_n(buf+size,(const char *)sc, sizeof(sc),num_per_line);
	}

		

	if(ret == -EFAULT){
		return -EFAULT;
	}

	size += ret;

	/*
	if(copy_to_user(buf+size,(const char *)nl, sizeof(nl))){
		printk( KERN_INFO "%s : copy_to_user failed\n", msg );
		return -EFAULT;
	}
	size += sizeof(nl);
	*/

	n++;
	return size;
}

static ssize_t ddsc_write(struct file* filp, const char* buf, size_t count, loff_t* pos)
{
	char c;
	if(copy_from_user(&c,buf,sizeof(char))){
		return -EFAULT;
	}
	if(c >= '1' && c <= '9'){
		num_per_line = (int)c - 48;
	}

	return 1;
}

static int ddsc_release(struct inode* inode, struct file* filp)
{
	printk(KERN_INFO "%s : close() called\n", msg);

	spin_lock(&spn_lock);
	access_num--;
	spin_unlock(&spn_lock);

	return 0;
}

static int __init dev_init_module(void)
{
	int retval;
	dev_t dev, devno;
	
	retval =  alloc_chrdev_region(&dev, 0, 1, "ddsc");
	if(retval < 0){
		printk(KERN_ERR "alloc_chrdev_region failed.\n");
		return retval;
	}
	
	cls = class_create(THIS_MODULE,"ddsc");
	if(IS_ERR(cls))
		return PTR_ERR(cls);

	devmajor = MAJOR(dev);
	devno = MKDEV(devmajor, devminor);
	cdev_init(&cdv, &ddsc_fops);
	cdv.owner = THIS_MODULE;
	if(cdev_add(&cdv, devno, 1) < 0)
		printk(KERN_ERR "cdev_add failed minor = %d\n", devminor);
	else
		device_create(cls, NULL, devno, NULL, "ddsc%u",devminor);

	return 0;
}

static void __exit dev_cleanup_module(void)
{
	dev_t devno;

	cdev_del(&cdv);
	devno = MKDEV(devmajor, devminor);
	device_destroy(cls, devno);
	class_destroy(cls);
	unregister_chrdev(devmajor, 1);
	printk(KERN_INFO "%s : removed from kernel\n", msg);
}

module_init(dev_init_module);
module_exit(dev_cleanup_module);
