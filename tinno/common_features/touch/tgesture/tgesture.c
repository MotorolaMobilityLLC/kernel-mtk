/* 
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
#include <linux/kthread.h>
	 
//#include <linux/wakelock.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/clk.h>
#include <linux/ioctl.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/atomic.h>
	 
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>	 
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/uaccess.h>
#include <linux/kobject.h>
#include <linux/types.h>
#include <linux/atomic.h>
#include <linux/io.h>
#include <linux/sched.h>

#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/types.h>

#include "tgesture.h"
/******************************************************************************
 * configuration
*******************************************************************************/
/*----------------------------------------------------------------------------*/
#define APS_TAG                  "[TGesture] "
#define APS_FUN(f)               printk(KERN_INFO APS_TAG"%s\n", __FUNCTION__)
#define APS_ERR(fmt, args...)    printk(KERN_ERR  APS_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define APS_LOG(fmt, args...)    printk(KERN_ERR APS_TAG fmt, ##args)
#define APS_DBG(fmt, args...)    printk(KERN_INFO APS_TAG fmt, ##args)         




extern int gBackLightLevel;
u8 gTGesture = 0;

static int enable_key = 1;

static s32 tgesture_state = 1; //open status
int  bEnTGesture = 0;
char Tg_buf[16]={"-1"};
static int TGesture_probe(struct platform_device *pdev);
static int TGesture_remove(struct platform_device *pdev);
static ssize_t TGesture_config_read_proc(struct file *file, char __user *page, size_t size, loff_t *ppos);
static ssize_t TGesture_config_write_proc(struct file *filp, const char __user *buffer, size_t count, loff_t *off);
/******************************************************************************
 * extern functions
*******************************************************************************/

atomic_t tp_write_flag=ATOMIC_INIT(1);
struct platform_device TGesture_sensor = {
	.name	       = "tgeseture",
	.id            = -1,
};
struct of_device_id TGesture_of_match[] = {
	{ .compatible = "mediatek,TGesture", },
	{},
};

static struct platform_driver TGesture_driver = {
	.probe      = TGesture_probe,
	.remove     = TGesture_remove,    
	.driver     = {
		.name  = "tgeseture",
		.of_match_table = TGesture_of_match,	

	}
};
static const struct file_operations config_proc_ops = {
    .owner = THIS_MODULE,
    .read = TGesture_config_read_proc,
    .write = TGesture_config_write_proc,
};

#define TGesture_CONFIG_PROC_FILE     "tgeseture_config"
#define TGesture_CONFIG_MAX_LENGTH       240

static struct proc_dir_entry *tgesture_config_proc = NULL;
static char config[TGesture_CONFIG_MAX_LENGTH] = {0};
/*-------------------------------attribute file for debugging----------------------------------*/

/******************************************************************************
 * Sysfs attributes
*******************************************************************************/
/*----------------------------------------------------------------------------*/
static ssize_t TGesture_show_state(struct device_driver *ddri, char *buf)
{
	APS_LOG("tgesture_state = %d\n", tgesture_state);
	return snprintf(buf, PAGE_SIZE, "%d\n", tgesture_state);
}
/*----------------------------------------------------------------------------*/
static ssize_t TGesture_show_key(struct device_driver *ddri, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", enable_key);
}
/*----------------------------------------------------------------------------*/
static ssize_t TGesture_store_key(struct device_driver *ddri, const char *buf, size_t count)
{
    int enable;
   
	
	if(1 == sscanf(buf, "%d", &enable))
	{
		enable_key = enable;
	}
	else 
	{
		APS_ERR("invalid enable content: '%s', length = %d\n", buf, count);
	}
	return count;    
}
/*---------------------------------------------------------------------------------------*/
static DRIVER_ATTR(tgesture_state,     S_IWUSR | S_IRUGO, TGesture_show_state, NULL);
static DRIVER_ATTR(key_tgesture_enable,    0664, TGesture_show_key, TGesture_store_key);
/*----------------------------------------------------------------------------*/
static struct driver_attribute *TGesture_attr_list[] = {
    &driver_attr_tgesture_state,
    &driver_attr_key_tgesture_enable,
};





static ssize_t TGesture_config_read_proc(struct file *file, char __user *page, size_t size, loff_t *ppos)
{
    char buff[120];
	int desc = 0;
    if (*ppos)  // CMD call again
    {
        return 0;
    }
	printk("TGesture1:%d\n",gTGesture);
    desc +=sprintf(buff + desc, "0x%c,%s",gTGesture,Tg_buf);
    printk("TGesture2:%s\n",buff);
  
    return simple_read_from_buffer(page, size, ppos, buff, desc);
}
static ssize_t TGesture_config_write_proc(struct file *filp, const char __user *buffer, size_t count, loff_t *off)
{
   

    printk("====LGC=========TGesture_config_write_procwrite count %d\n", count);

    if (count > TGesture_CONFIG_MAX_LENGTH  )
    {
        printk("size not match [%d:%d]\n", TGesture_CONFIG_MAX_LENGTH, count);
        return -EFAULT;
    }

    if (copy_from_user(&config, buffer, count))
    {
        printk("copy from user fail\n");
        return -EFAULT;
    }
   //  tp__write_flag;

if(atomic_read(&tp_write_flag))
{
     atomic_set(&tp_write_flag,0);
     bEnTGesture=config[0]-48;
     printk("===== TGesture_config_write_proc%d=====",bEnTGesture);
     atomic_set(&tp_write_flag,1);
}
    // printk("====LGC=TGesture_config_write_procwrite bEnTGesture==%d===\n",bEnTGesture);
 /*   ret = gtp_send_cfg(i2c_client_point);
    abs_x_max = (config[RESOLUTION_LOC + 1] << 8) + config[RESOLUTION_LOC];
    abs_y_max = (config[RESOLUTION_LOC + 3] << 8) + config[RESOLUTION_LOC + 2];
    int_type = (config[TRIGGER_LOC]) & 0x03;

    if (ret < 0)
    {
        printk("send config failed.");
    }
*/
    return count;
}

/*----------------------------------------------------------------------------*/
static int TGesture_create_attr(struct device_driver *driver) 
{
	int idx, err = 0;
	int num = (int)(sizeof(TGesture_attr_list)/sizeof(TGesture_attr_list[0]));
	if (driver == NULL)
	{
		return -EINVAL;
	}

	for(idx = 0; idx < num; idx++)
	{
		if((err = driver_create_file(driver, TGesture_attr_list[idx])))
		{            
			APS_ERR("driver_create_file (%s) = %d\n", TGesture_attr_list[idx]->attr.name, err);
			break;
		}
	}    
	return err;
}
/*----------------------------------------------------------------------------*/
	static int TGesture_delete_attr(struct device_driver *driver)
	{
	int idx ,err = 0;
	int num = (int)(sizeof(TGesture_attr_list)/sizeof(TGesture_attr_list[0]));

	if (!driver)
	return -EINVAL;

	for (idx = 0; idx < num; idx++) 
	{
		driver_remove_file(driver, TGesture_attr_list[idx]);
	}
	
	return err;
}





/*----------------------------------------------------------------------------*/

static int TGesture_probe(struct platform_device *pdev) 
{
        int err;
	APS_FUN();  
	
	printk("==============TGesture==================\n");
	if((err = TGesture_create_attr(&TGesture_driver.driver)))
	{
		printk("create attribute err = %d\n", err);
		return 0;
	}
    // Create proc file system
       tgesture_config_proc = proc_create(TGesture_CONFIG_PROC_FILE, 0666, NULL, &config_proc_ops);
     if (tgesture_config_proc == NULL)
    {
        printk("create_proc_entry %s failed\n", TGesture_CONFIG_PROC_FILE);
     }
    else
    {
        printk("create proc entry %s success", TGesture_CONFIG_PROC_FILE);
    }
	return 0;
}
/*----------------------------------------------------------------------------*/
static int TGesture_remove(struct platform_device *pdev)
{
	
	APS_FUN(); 
	
         printk("==============TGesture_remove==================\n");		
	TGesture_delete_attr(&TGesture_driver.driver);
	
	return 0;
}



/*----------------------------------------------------------------------------*/
static int __init TGesture_init(void)
{
	APS_FUN();
   //Begin<REQ><EGAFM-80><add TP gesture>;xiongdajun
   if( platform_device_register(&TGesture_sensor))
	{
		printk("failed to register driver");
		return 0;
	}
     //END<REQ><EGAFM-80><add TP gesture>;xiongdajun
  if(platform_driver_register(&TGesture_driver))
	{
		APS_ERR("failed to register driver");
		return -ENODEV;
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit TGesture_exit(void)
{
	APS_FUN();
	platform_driver_unregister(&TGesture_driver);
}
/*----------------------------------------------------------------------------*/
module_init(TGesture_init);
module_exit(TGesture_exit);
/*----------------------------------------------------------------------------*/
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("tgesture DRIRVER");
MODULE_AUTHOR("yaohua.li");


