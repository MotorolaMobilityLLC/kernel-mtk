/*add this file for device info --sunsiyuan@wind-mobi.com add 20161129 begin*/
	
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/semaphore.h>
#include <linux/device.h>
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <linux/slab.h>   //tuwenzan@wind-mobi.com add at 20161230

struct device_info_dev{
	struct cdev dev;
	struct semaphore sem;
};

#define DEVICE_INFO_TAG                  "[DEVICE/INFO] "
#define DEVICE_INFO_FUN(f)               printk(KERN_INFO DEVICE_INFO_TAG"%s\n", __FUNCTION__)
#define DEVICE_INFO_ERR(fmt, args...)    printk(KERN_ERR  DEVICE_INFO_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define DEVICE_INFO_LOG(fmt, args...)    printk(KERN_INFO DEVICE_INFO_TAG fmt, ##args)
#define DEVICE_INFO_DBG(fmt, args...)    printk(KERN_INFO DEVICE_INFO_TAG fmt, ##args) 

#define DEV_NAME "device_info"
#define CLASS_NAME "wind_device"
static dev_t device_info_devno = 0;
struct device_info_dev g_device_info_dev;
static struct class *g_device_info_classp =NULL; 

static int test_val = 0;

char *g_lcm_name ="";
char *g_gsensor_name ="";

char *g_fingerprint_name ="";
//liujinzhou@wind-mobi.com add at 20161221 begin
char *g_psensor_name ="";
//liujinzhou@wind-mobi.com add at 20161221 end
char *g_flash_id_str = "";	//tuwenzan@wind-mobi.com add at 20161230
//char *g_msensor_name = "";

unsigned int g_ctp_fwvr; 
unsigned int g_ctp_vendor;	//sunsiyuan@wind-mobi.com add at 20161130
char ctp_vendor[5];	//tuwenzan@wind-mobi.com add at 20170116
char g_ctp_id_str[21];

/*
static ssize_t show_msensor_info(struct device *dev,struct device_attribute *attr, char *buf)
{
	size_t size = 0;
	DEVICE_INFO_FUN();
	if(NULL != g_msensor_name)
		size = sprintf(buf, "%s\n", g_msensor_name);
    return size;
}
*/
//liujinzhou@wind-mobi.com add at 20161221 begin
static ssize_t show_psensor_info(struct device *dev,struct device_attribute *attr, char *buf)
{
	size_t size = 0;
	DEVICE_INFO_FUN();
	if(NULL != g_psensor_name)
		size = sprintf(buf, "%s\n", g_psensor_name);
    return size;
}
static ssize_t store_psensor_info(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
	DEVICE_INFO_FUN();
   	return size;
}
//liujinzhou@wind-mobi.com add at 20161221 end
static ssize_t show_lcm_info(struct device *dev,struct device_attribute *attr, char *buf)
{
	size_t size = 0;
	DEVICE_INFO_FUN();
	if(NULL != g_lcm_name)
		size = sprintf(buf, "%s\n", g_lcm_name);
    return size;
}

static ssize_t store_lcm_info(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
	DEVICE_INFO_FUN();
   	return size;
}

static ssize_t show_gsensor_info(struct device *dev,struct device_attribute *attr, char *buf)
{
	size_t size = 0;
	DEVICE_INFO_FUN();
	if(NULL != g_gsensor_name)
		size = sprintf(buf, "%s\n", g_gsensor_name);
    return size;
}

static ssize_t store_gsensor_info(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
	DEVICE_INFO_FUN();
   	return size;
}
//tuwenzan@wind-mobi.com modify at 20170116 begin
static ssize_t show_ctp_info(struct device *dev,struct device_attribute *attr, char *buf)
{
	char *buf_temp = buf;
	DEVICE_INFO_FUN();
	buf_temp += sprintf(buf_temp, "IC:%s-", g_ctp_id_str);
	buf_temp += sprintf(buf_temp, "vendor:%s-",ctp_vendor);
	buf_temp += sprintf(buf_temp, "config_id:0x%x-",g_ctp_vendor);
	buf_temp += sprintf(buf_temp, "fwvr:0x%3x(%u)\n", g_ctp_fwvr,g_ctp_fwvr);
    return (buf_temp - buf);
}
//tuwenzan@wind-mobi.com modify at 20170116 end
static ssize_t store_ctp_info(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
	DEVICE_INFO_FUN();
   	return size;
}

static ssize_t show_fp_info(struct device *dev,struct device_attribute *attr, char *buf)
{
	size_t size = 0;
	DEVICE_INFO_FUN();
	if(NULL != g_fingerprint_name)
		size = sprintf(buf, "%s\n", g_fingerprint_name);
    return size;
}

static ssize_t store_fp_info(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
	DEVICE_INFO_FUN();
   	return size;
}

extern char g_invokeSensorNameStr[2][32];
static ssize_t show_camera_info(struct device *dev,struct device_attribute *attr, char *buf)
{
	char *buf_temp = buf;
	DEVICE_INFO_FUN();
	buf_temp += sprintf(buf_temp, "(%s)", g_invokeSensorNameStr[0]);
	buf_temp += sprintf(buf_temp, "(%s)\n",g_invokeSensorNameStr[1]);
    return (buf_temp - buf);
}

static ssize_t store_camera_info(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
	DEVICE_INFO_FUN();
   	return size;
}

int battery_boot_data[10] = {
	0, /*get_hw_ocv*/
	0, /*HW_SOC*/
	0, /*SW_SOC*/
	0, /*rtc_fg_soc*/
	0, /*gFG_capacity_by_c*/
	0, /*gFG_DOD0*/
	0, /*gFG_15_vlot*/
	0,
	0,
	0,
};

static ssize_t show_battery_boot_info(struct device *dev,struct device_attribute *attr, char *buf)
{
	char *buf_temp = buf;
	DEVICE_INFO_FUN();
	
	buf_temp += sprintf(buf_temp, "get_hw_ocv = %d\n", battery_boot_data[0]);
	buf_temp += sprintf(buf_temp, "HW_SOC = %d\n", battery_boot_data[1]);
	buf_temp += sprintf(buf_temp, "SW_SOC = %d\n", battery_boot_data[2]);
	buf_temp += sprintf(buf_temp, "rtc_fg_soc = %d\n", battery_boot_data[3]);
	buf_temp += sprintf(buf_temp, "gFG_capacity_by_c = %d\n", battery_boot_data[4]);
	buf_temp += sprintf(buf_temp, "gFG_DOD0 = %d\n", battery_boot_data[5]);
	buf_temp += sprintf(buf_temp, "gFG_15_vlot = %d\n", battery_boot_data[6]);
	
     return (buf_temp - buf);
}

static ssize_t store_battery_boot_info(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
	DEVICE_INFO_FUN();
   	return size;
}

static DEVICE_ATTR(lcm_info, 0664, show_lcm_info, store_lcm_info);
static DEVICE_ATTR(gsensor_info, 0664, show_gsensor_info, store_gsensor_info);
//liujinzhou@wind-mobi.com add at 20161221 begin
static DEVICE_ATTR(psensor_info, 0664, show_psensor_info, store_psensor_info);
//liujinzhou@wind-mobi.com add at 20161221 end
//static DEVICE_ATTR(msensor_info, 0664, show_msensor_info, NULL);
static DEVICE_ATTR(ctp_info, 0664, show_ctp_info, store_ctp_info);
static DEVICE_ATTR(fp_info, 0664, show_fp_info, store_fp_info);
static DEVICE_ATTR(camera_info, 0664, show_camera_info, store_camera_info);
static DEVICE_ATTR(battery_boot_info, 0664, show_battery_boot_info, store_battery_boot_info);

static void attr_files_create(struct device *device)
{	
	device_create_file(device, &dev_attr_lcm_info);	
	device_create_file(device, &dev_attr_gsensor_info);	
	//liujinzhou@wind-mobi.com add at 20161221 begin
	device_create_file(device, &dev_attr_psensor_info);	
	//liujinzhou@wind-mobi.com add at 20161221 end
//	device_create_file(device, &dev_attr_msensor_info);	
	device_create_file(device, &dev_attr_ctp_info);	
	device_create_file(device, &dev_attr_fp_info);	
	device_create_file(device, &dev_attr_camera_info);	
	device_create_file(device, &dev_attr_battery_boot_info);	
}

//tuwenzan@wind-mobi.com add devicesinfo proc interface at 20161229 begin
extern int emmc_id_get(void);
static int devicesinfo_stats_show(struct seq_file *m, void *unused)
{
	DEVICE_INFO_FUN();
	if(0 != emmc_id_get()){
		printk("%s emmc_id_get error\n",__func__);
	}
	if(NULL != g_flash_id_str){
    	seq_printf(m, "Type: MCP(eMMC+LPDDR3)\n%s\n",g_flash_id_str);
		kfree(g_flash_id_str);
		return 0;
	}
	kfree(g_flash_id_str);
    return -1;
}
static int devicesinfo_stats_open(struct inode *inode, struct file *file)
{
    return single_open(file, devicesinfo_stats_show, NULL);
}


static const struct file_operations devicesinfo_stats_fops = {
    .owner = THIS_MODULE,
    .open = devicesinfo_stats_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};
//tuwenzan@wind-mobi.com add devicesinfo proc interface at 20161229 end
static int device_info_open(struct inode *inode, struct file *filp)
{
	struct device_info_dev *device_info_dev = NULL;
	DEVICE_INFO_FUN();
	device_info_dev = container_of(inode->i_cdev, struct device_info_dev, dev);
	filp->private_data = device_info_dev;
	return 0;

}

static int device_info_release(struct inode *inode, struct file *filp)
{
	DEVICE_INFO_FUN();
	return 0;
}

static ssize_t device_info_read(struct file *filp, char __user * buf, size_t count, loff_t *offp)
{
	//struct device_info_dev *device_info_dev = filp ->private_data;
	DEVICE_INFO_FUN();
	if(count > sizeof(int))
		return 0;
	if(copy_to_user(buf, &test_val, sizeof(int)))
	{
		return -EFAULT;
	}
	return sizeof(int);
}

static ssize_t device_info_write(struct file *filp, const char __user *buf, size_t count, loff_t *offp)
{
	//struct device_info_dev *device_info_dev = filp ->private_data;
	DEVICE_INFO_FUN();
	if(count > sizeof(int))
		return 0;
	if(copy_from_user(&test_val, buf, sizeof(int)))
	{
		return -EFAULT;
	}
	return sizeof(int);	
}

static long  device_info_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{    
	int err = 0;
//    void __user *data = (void __user*) arg;
	DEVICE_INFO_FUN();
	//For future use
	switch(cmd)
	{
	}
	return err;
}

static struct file_operations device_info_fops =
{
    .owner = THIS_MODULE,
    .open = &device_info_open,
    .release = &device_info_release,
    .write = &device_info_write,
    .read = &device_info_read,
    .unlocked_ioctl = &device_info_unlocked_ioctl,
};

static int  device_info_init(void)
{
	int err;
    struct device *class_dev = NULL;
	struct device_info_dev *device_info_devp;
	DEVICE_INFO_FUN();
	device_info_devp = &g_device_info_dev;
//1. alloc dev num
	err = alloc_chrdev_region(&device_info_devno, 0, 1, DEV_NAME);
	if(err){
		DEVICE_INFO_ERR("register device number error!!!! \n");
		goto fail;
	}

//2. connect fops with cdev
	cdev_init(&device_info_devp->dev, &device_info_fops);
	device_info_devp->dev.owner = THIS_MODULE;

//3. add cdev to list
   err = cdev_add(&device_info_devp->dev, device_info_devno, 1);
	if(err){
		DEVICE_INFO_ERR("cdev_add error!!!!! \n");
		goto err0;
	}
//4. device create	
    g_device_info_classp = class_create(THIS_MODULE, CLASS_NAME);
    class_dev = (struct device *)device_create(g_device_info_classp, 
                                                   NULL, 
                                                   device_info_devno, 
                                                   NULL, 
                                                   DEV_NAME);
//5. Create attr files
	attr_files_create(class_dev);

	proc_create("devicesinfo", S_IRUGO, NULL, &devicesinfo_stats_fops); //tuwenzan@wind-mobi.com add devicesinfo proc interface at 20161229 
	
	return 0;
	
err0:
	unregister_chrdev_region(device_info_devno, 1);
fail:	
	return err;
}

static void  device_info_exit(void)
{
	// Never get there
	DEVICE_INFO_FUN();
}

module_init(device_info_init);
module_exit(device_info_exit);
MODULE_DESCRIPTION("Wind Device Info");
MODULE_AUTHOR("liqiang<liqiang@wind-mobi.com>");
MODULE_LICENSE("GPL");
/*add this file for device info --sunsiyuan@wind-mobi.com add 20161129 end*/