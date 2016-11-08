/* dev_info.c
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
/* dev_info.c
 * private function from Longcheer (LCT) software.
 * This is designed to record information of all special devices for phonecells,
 * such as LCM, MCP, Touchpannel, cameras, sensors and so on.
 * By   : shaohui@longcheer.net
 * Date : 2012/11/26
 *
 * */
/*=================================================================*/
/*
*	Date	: 2013.01.31
*	By		: shaohui
*	Discription:
*	change function devinfo_check_add_device(struct devinfo_struct *dev)
*	For if a same device (just diffrent used state) found, replace it!
*/
/*=================================================================*/
/*=================================================================*/
/*
*	Date	: 2015.12.21
*	By		: shaohui
*	Discription:
*	1,create sysfs to access devices info
*	2,Fix some bugs for -Werror
*	3,follow Device trees
*/
/*=================================================================*/
/*=================================================================*/
/*
*	Date	: 2016.11.1
*	By		: shaohui
*	Discription:
*   1, c files move to misc folder 
*   2, h file move to misc folder
*   3, change Kconfig and Makefile
*   4, add log tag
*/
/*=================================================================*/

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/list.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#endif

/*shaohui add 
 * TO define some */
//#include <linux/dev_info.h>
#include "dev_info.h"
#include <linux/seq_file.h>
#include <linux/miscdevice.h>


#include "lct_print.h"
#ifdef LCT_LOG_MOD
#undef LCT_LOG_MOD
#define LCT_LOG_MOD "[DEVINFO]" 
#endif


#ifdef CONFIG_OF
static const struct of_device_id lct_devinfo_dt_match[] = {
	{.compatible = "mediatek,lct_devinfo"},
	{},
};
//MODULE_DEVICE_TABLE(of, lct_devinfo_dt_match);
#endif

static DEFINE_SPINLOCK(dev_lock);
static struct list_head active_devinfo_list;

static unsigned int device_num = 0;
/*********************************************************************************
 * This functions is designed to add a new devinfo struct to list;
 * Input:	devinfo_struct
 * Output:  0
 * Note:
 * *******************************************************************************/
int devinfo_declare_new_device(struct devinfo_struct *dev)
{
    unsigned long irqflags;

	lct_pr_info("add new device:%s,%s,%s,%s,%s,%s\n",
			dev->device_module,dev->device_vendor,dev->device_ic,dev->device_version,dev->device_info,dev->device_used);
    spin_lock_irqsave(&dev_lock, irqflags);
    list_add_tail(&dev->device_link, &active_devinfo_list);
    spin_unlock_irqrestore(&dev_lock, irqflags);

	device_num ++;

	return 0;
}	

/*********************************************************************************
 * This functions is designed to delate a certain devinfo;
 * Input:	
 * Output:  0
 * Note: Not complete!
 * *******************************************************************************/
int devinfo_delete_new_device(struct devinfo_struct *dev)
{

	// it is unnessary to call this function.??
	return 0;
}

/*********************************************************************************
 * This functions is designed to show all list;
 * Input:	NULL
 * Output:  0
 * Note: Do not complete!
 * *******************************************************************************/
/*
static int devinfo_show_device(void)
{
//NULL fuc
	struct devinfo_struct *dev ;
	
    list_for_each_entry(dev, &active_devinfo_list, device_link) {
	}
	return 0;
}
*/
/*********************************************************************************
 * This functions is designed to modify a certain devinfo params;
 * Input:	devinfo_struct
 * Output:  0
 * Note:
 * *******************************************************************************/
int devinfo_mod_para(char* name,char* type,char* new_value)
{
    unsigned long irqflags;

	struct devinfo_struct *dev;
//printk("[DEVINFO]add new device:%s,%s,%s,%s,%s,%s\n",dev->device_module,dev->device_vendor,dev->device_ic,dev->device_version,dev->device_info,dev->device_used);
    spin_lock_irqsave(&dev_lock, irqflags);
    list_for_each_entry(dev, &active_devinfo_list, device_link) {
//	if(strcmp(name,dev->type)==0)
//	{
//		printk("[DEVINFO] find the name\n");
//	}
	
	
	}
    spin_unlock_irqrestore(&dev_lock, irqflags);

	device_num ++;

	return 0;
}	


/*********************************************************************************
 * This functions is designed to check if declared already, and add new device if not yet;
 * Input:	devinfo_struct
 * Output:  1 / 0
 * Note: return 1 for there have a same device registed,0 for new device
 * *******************************************************************************/
int devinfo_check_add_device(struct devinfo_struct *dev)
{
    unsigned long irqflags;
	struct devinfo_struct *dev_all;
	lct_pr_debug("devinfo_check!\n");
	spin_lock_irqsave(&dev_lock, irqflags);
	if(list_empty(&active_devinfo_list) != 1)
    list_for_each_entry(dev_all, &active_devinfo_list, device_link) {
	lct_pr_debug("dev type:%s\n",dev->device_type);
	lct_pr_debug("dev list type:%s\n",dev_all->device_type);
	if((strcmp(dev_all->device_type,dev->device_type)==0) && (strcmp(dev_all->device_module,dev->device_module)==0) &&
		(strcmp(dev_all->device_vendor,dev->device_vendor)==0) && (strcmp(dev_all->device_ic,dev->device_ic)==0) &&
			(strcmp(dev_all->device_version,dev->device_version)==0) &&(strcmp(dev_all->device_info,dev->device_info)==0))// && shaohui mods here
		// It will be replaced if there is a used device found! Do not mention HOT plug device! 2013.01.31 
		//	(strcmp(dev_all->device_used,dev->device_used)==0))
	{
		if(strcmp(dev_all->device_used,dev->device_used)==0)
		{
			lct_pr_warn("find the same device\n");
		}else if(strcmp(dev_all->device_used,DEVINFO_UNUSED)==0)
		{
		//we belive that we find a existed device! del the unexisted one!
			lct_pr_warn("find device,but unused state!\n");
			list_del(&dev_all->device_link);
    		list_add_tail(&dev->device_link, &active_devinfo_list);
		//	list_replace(&dev_all->device_link, &active_devinfo_list);
			spin_unlock_irqrestore(&dev_lock, irqflags);
			return 0;	
		}else{
		//If a for-existed device is found lost,Do nothing~
			lct_pr_warn("find device,but used state!\n");
		}

		spin_unlock_irqrestore(&dev_lock, irqflags);
		return 1;
	}
	
	}
    list_add_tail(&dev->device_link, &active_devinfo_list);
    spin_unlock_irqrestore(&dev_lock, irqflags);
	return 0;
}


#if 0
/* shaohui add*/
/*misc device start*/

static int misc_devinfo_open(struct inode *inode, struct file *file)
{
	return 0;
}

static long misc_devinfo_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{

	return 0;
}

/*----------------------------------------------------------------------------*/
static struct file_operations devinfo_fops = {
	.owner = THIS_MODULE,
	.open = misc_devinfo_open,
//	.release = misc_devinfo_release,
	.unlocked_ioctl = misc_devinfo_unlocked_ioctl,
};
/*----------------------------------------------------------------------------*/
static struct miscdevice devinfo_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "devinfo",
	.fops = &devinfo_fops,
};

/*misc device end*/
#endif



static int devinfo_probe(struct platform_device *pdev)
{
		lct_pr_info("DEVINFO,devinfo probe\n");
		INIT_LIST_HEAD(&active_devinfo_list);
		device_num = 0;
		
		return 0;
}

static struct platform_driver devinfo_driver = {
	.driver = {
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(lct_devinfo_dt_match),
#endif
		.name = "devinfo",
	},
    .probe = devinfo_probe,
    #ifndef CONFIG_HAS_EARLYSUSPEND
    .suspend    = NULL,
    .resume     = NULL,
    #endif
};
#ifndef CONFIG_OF
static struct platform_device devinfo_device = {
    .name = "devinfo",
};
#endif





/*shaohui add for proc interface start*/

static int devinfo_stats_show(struct seq_file *m, void *unused)
{
    unsigned long irqflags;
	struct devinfo_struct *dev ;
    int ret;

    spin_lock_irqsave(&dev_lock, irqflags);
    ret = seq_puts(m, "name\t\tmodule\t\tvender\t\tIC\tVersion\tinfo\t\tused\n");
    list_for_each_entry(dev, &active_devinfo_list, device_link)
    	seq_printf(m, "%s\t\t%s\t\t%s\t\t%s\t%s\t%s\t\t%s\n",
			dev->device_type,dev->device_module,dev->device_vendor,dev->device_ic,dev->device_version,dev->device_info,dev->device_used);
    spin_unlock_irqrestore(&dev_lock, irqflags);
    return 0;
}
static int devinfo_stats_open(struct inode *inode, struct file *file)
{
    return single_open(file, devinfo_stats_show, NULL);
}


static const struct file_operations devinfo_stats_fops = {
    .owner = THIS_MODULE,
    .open = devinfo_stats_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

/*shaohui add for proc interface end*/



/*shaohui add for attr func start*/
#ifdef SYSFS_INFO_SUPPORT
static ssize_t devinfo_show(struct device *sdev,struct device_attribute *attr,char *buf)
{
	unsigned long irqflags;
	struct devinfo_struct *dev ;
    int lenth;
	char temp_buf[1024];
	memset(temp_buf,0,1024);
    spin_lock_irqsave(&dev_lock, irqflags);
    
	list_for_each_entry(dev, &active_devinfo_list, device_link)
	{	
		if (dev->device_used == DEVINFO_USED)
			snprintf(temp_buf, PAGE_SIZE, "%s %s\n",temp_buf,dev->device_comments);
	}
    
    snprintf(buf,PAGE_SIZE,"%s",temp_buf);
	spin_unlock_irqrestore(&dev_lock, irqflags);
    return strlen(buf);//lenth;

}

/*Add attr store API*/
static char rec_buf[128];
static ssize_t devinfo_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	memset(rec_buf,0,128);
	sprintf(rec_buf,"FingerPrint: %s",buf);
	lct_pr_debug("%s buf: %s !\n",__func__,buf);
	lct_pr_debug("%s buf: %s !\n",__func__,rec_buf);
	DEVINFO_CHECK_DECLARE_VERSION(rec_buf);
	return count;
}
static DEVICE_ATTR(devs,0660,devinfo_show,devinfo_store);//modify Dev_attr Name by liuzhen

/*shaohui add for attr func end*/

#endif



static int __init lct_devinfo_init(void)
{
    int ret;
#ifdef SYSFS_INFO_SUPPORT
	struct kobject *kobj;
#endif
	lct_pr_info("DEVINFO,devinfo init!\n");
#ifndef CONFIG_OF
	ret = platform_device_register(&devinfo_device);
    if (ret) {
        lct_pr_err("platform_device_register failed\n");
        goto err_platform_device_register;
    }
#endif
    ret = platform_driver_register(&devinfo_driver);
    if (ret) {
        lct_pr_err("platform_driver_register failed\n");
#ifndef CONFIG_OF
        goto err_platform_driver_register;
#endif
	}
// add for proc 
    proc_create("devicesinfo", S_IRUGO, NULL, &devinfo_stats_fops);

/*shaohui add to create sysfs*/
#ifdef SYSFS_INFO_SUPPORT
	kobj = kobject_create_and_add("main_devices",NULL);
	ret = sysfs_create_file(kobj,&dev_attr_devs.attr);
#endif
	return 0;


#ifndef CONFIG_OF
err_platform_driver_register:
    platform_device_unregister(&devinfo_device);
err_platform_device_register:
#endif
    return ret;
}

static void  __exit lct_devinfo_exit(void)
{
    remove_proc_entry("devicesinfo", NULL);

    platform_driver_unregister(&devinfo_driver);
#ifndef CONFIG_OF
    platform_device_unregister(&devinfo_device);
#endif
}

core_initcall(lct_devinfo_init);
module_exit(lct_devinfo_exit);
MODULE_AUTHOR("shaohui@longcheer.com");
MODULE_LICENSE("GPL");
