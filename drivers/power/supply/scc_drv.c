/*
* Copyright (C) 2018 TINNO Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See http://www.gnu.org/licenses/gpl-2.0.html for more details.
*/

/*****************************************************************************
 *
 * Filename:
 * ---------
 *    scc_drv.c
 *
 * Project:
 * --------
 *   Android_Software
 *
 * Description:
 * ------------
 * This Module defines functions of the Anroid Battery service for
 * updating the battery status
 *
 * Author:
 * -------
 * Jake.Liang
 *
 ****************************************************************************/
#include <linux/init.h>		/* For init/exit macros */
#include <linux/module.h>	/* For MODULE_ marcros  */
#include <linux/wait.h>		/* For wait queue*/
#include <linux/sched.h>	/* For wait queue*/
#include <linux/kthread.h>	/* For Kthread_run */
#include <linux/platform_device.h>	/* platform device */
#include <linux/time.h>

#include <linux/netlink.h>	/* netlink */
#include <linux/kernel.h>
#include <linux/socket.h>	/* netlink */
#include <linux/skbuff.h>	/* netlink */
#include <net/sock.h>		/* netlink */
#include <linux/cdev.h>		/* cdev */

#include <linux/err.h>	/* IS_ERR, PTR_ERR */
#include <linux/reboot.h>	/*kernel_power_off*/
#include <linux/proc_fs.h>
#include <linux/of_fdt.h>	/*of_dt API*/
#include <linux/vmalloc.h>

//#include <linux/math64.h> /*div_s64*/

#include <linux/debugfs.h>
#include <linux/scc_drv.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/power_supply.h>

static int charge_speed;
static int target_speed;   //target speed set by app layer.
static int speed_current_map[SPEED_MAX];   //actual current/ma for speed level.
#if 0
static struct dentry *scc_dentry;
#endif
//extern void fg_update_routine_wakeup(void);

static int scc_get_usb_online()
{
	int ret;
	struct power_supply *charger_psy;
	union power_supply_propval psy_prop;

	charger_psy = power_supply_get_by_name("mtk_charger_type");
	if (charger_psy) {
		ret = power_supply_get_property(charger_psy, POWER_SUPPLY_PROP_USB_TYPE, &psy_prop);
		if (ret)
			return 0;
		else {
			if (psy_prop.intval == POWER_SUPPLY_USB_TYPE_SDP ||
			    psy_prop.intval == POWER_SUPPLY_USB_TYPE_CDP)
				return 1;
			else
				return 0;
		}
	} else
		return 0;
}

static int scc_get_ac_online()
{
	int ret;
	struct power_supply *charger_psy;
	union power_supply_propval psy_prop;

	charger_psy = power_supply_get_by_name("mtk_charger_type");
	if (charger_psy) {
		ret = power_supply_get_property(charger_psy, POWER_SUPPLY_PROP_USB_TYPE, &psy_prop);
		if (ret)
			return 0;
		else {
			if (psy_prop.intval == POWER_SUPPLY_USB_TYPE_DCP)
				return 1;
			else
				return 0;
		}
	} else
		return 0;
}

void scc_init_speed_current_map(const char *node_str){
	struct device_node *node;
	int rc = 0;

	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6357-gauge");
	if (node) {
		rc = of_property_read_u32_array(node, "speed-current", speed_current_map, SPEED_MAX);
		if (rc) {
			memset(speed_current_map,0,sizeof(speed_current_map));
			printk("geroge Couldn't read speed-current rc = %d\n", rc);
		}
	} else {
		printk("geroge Couldn't read speed-current rc = %d\n", rc);
		memset(speed_current_map,0,sizeof(speed_current_map));
	}
	target_speed = 0;
	charge_speed = 0;
	printk("[SCC]""speed_current_map[] = %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n", 
		speed_current_map[0], speed_current_map[1], speed_current_map[2], speed_current_map[3],
		speed_current_map[4], speed_current_map[5], speed_current_map[6], speed_current_map[7],
		speed_current_map[8], speed_current_map[9]);
}
#if 0
/* "/sys/kernel/debug/scc/speed */
static int scc_show(struct seq_file *m, void *unused)
{
	seq_printf(m, "%d\n",charge_speed);

	return 0;
}

static int scc_open(struct inode *inode, struct file *file)
{
	return single_open(file, scc_show, NULL);
}

static ssize_t scc_write(struct file *file,
				    const char __user *ubuf, size_t count, loff_t *ppos)
{
	char buf[18];
    int speed = 0;

	if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
		return -EFAULT;

	speed = (int)(buf[0] - '0');

	if(speed < 0 || speed >= SPEED_MAX)
		speed = 0;

	printk("[SCC]""Change speed from %d to %d\n", charge_speed, speed);

	charge_speed = speed;
	if(speed != target_speed) {
		target_speed = speed;
		//fg_update_routine_wakeup();
	}

	return count;
}

static const struct file_operations scc_fops = {
	.owner = THIS_MODULE,
	.open = scc_open,
	.write = scc_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static ssize_t scc_dummy_write(struct file *file,
				    const char __user *ubuf, size_t count, loff_t *ppos)
{
	return count;
}

//"/sys/class/leds/lcd-backlight/brightness" -> "/sys/kernel/debug/scc/lcd_brightness" 
static int scc_lcd_brightness_show(struct seq_file *m, void *unused)
{
	//seq_printf(m, "%d\n", scc_get_lcd_brightness());
	return 0;
}
static int scc_lcd_brightness_open(struct inode *inode, struct file *file)
{
	return single_open(file, scc_lcd_brightness_show, NULL);
}

static int scc_temp_show(struct seq_file *m, void *unused)
{
	seq_printf(m, "%d\n", scc_get_temp());
	return 0;
}
static int scc_temp_open(struct inode *inode, struct file *file)
{
	return single_open(file, scc_temp_show, NULL);
}

static int scc_capacity_show(struct seq_file *m, void *unused)
{
	seq_printf(m, "%d\n", scc_get_capacity());
	return 0;
}
static int scc_capacity_open(struct inode *inode, struct file *file)
{
	return single_open(file, scc_capacity_show, NULL);
}

static int scc_usb_online_show(struct seq_file *m, void *unused)
{
	seq_printf(m, "%d\n", scc_get_usb_online());
	return 0;
}
static int scc_usb_online_open(struct inode *inode, struct file *file)
{
	return single_open(file, scc_usb_online_show, NULL);
}

static int scc_ac_online_show(struct seq_file *m, void *unused)
{
	seq_printf(m, "%d\n", scc_get_ac_online());
	return 0;
}
static int scc_ac_online_open(struct inode *inode, struct file *file)
{
	return single_open(file, scc_ac_online_show, NULL);
}

static const struct file_operations scc_lcd_brightness_fops = {
	.owner = THIS_MODULE,
	.open = scc_lcd_brightness_open,
	.write = scc_dummy_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations scc_temp_fops = {
	.owner = THIS_MODULE,
	.open = scc_temp_open,
	.write = scc_dummy_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations scc_capacity_fops = {
	.owner = THIS_MODULE,
	.open = scc_capacity_open,
	.write = scc_dummy_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations scc_usb_online_fops = {
	.owner = THIS_MODULE,
	.open = scc_usb_online_open,
	.write = scc_dummy_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations scc_ac_online_fops = {
	.owner = THIS_MODULE,
	.open = scc_ac_online_open,
	.write = scc_dummy_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

void scc_create_file(void)
{
	scc_dentry = debugfs_create_dir("scc", NULL);
	debugfs_create_file("lcd_brightness", S_IRUGO, scc_dentry, NULL, &scc_lcd_brightness_fops);
	debugfs_create_file("speed", S_IRUGO, scc_dentry, NULL, &scc_fops);
	debugfs_create_file("temp", S_IRUGO, scc_dentry, NULL, &scc_temp_fops);
	debugfs_create_file("capacity", S_IRUGO, scc_dentry, NULL, &scc_capacity_fops);
	debugfs_create_file("usb_online", S_IRUGO, scc_dentry, NULL, &scc_usb_online_fops);
	debugfs_create_file("ac_online", S_IRUGO, scc_dentry, NULL, &scc_ac_online_fops);
}
#endif

static ssize_t speed_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", charge_speed);
}

static ssize_t speed_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t size)
{
	signed int temp;
	int speed = 0;

	if (kstrtoint(buf, 10, &temp) == 0) {
		if (temp < 0)
			speed = 0;
		else
			speed = temp;
	} else {
		pr_err("%s: format error!\n", __func__);
	}

	if(speed < 0 || speed >= SPEED_MAX)
		speed = 0;

	printk("[SCC]""Change speed from %d to %d\n", charge_speed, speed);

	charge_speed = speed;
	if(speed != target_speed) {
		target_speed = speed;
	}
	
	return size;
}

static DEVICE_ATTR_RW(speed);

static ssize_t temp_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", scc_get_temp());
}

static DEVICE_ATTR_RO(temp);

static ssize_t capacity_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", scc_get_capacity());
}

static DEVICE_ATTR_RO(capacity);

static ssize_t usb_online_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", scc_get_usb_online());
}

static DEVICE_ATTR_RO(usb_online);

static ssize_t ac_online_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", scc_get_ac_online());
}

static DEVICE_ATTR_RO(ac_online);

static ssize_t lcd_brightness_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", scc_get_lcd_brightness());
}

static DEVICE_ATTR_RO(lcd_brightness);

int scc_create_file(struct device *dev)
{
	int ret = 0;
	
	pr_err("zhangchao test\n");
	printk("zhangchao test\n");
	ret = device_create_file(dev, &dev_attr_speed);
    if (ret) {
        pr_err("device_create_file speed failed\n");
        goto _out;
    }
	ret = device_create_file(dev, &dev_attr_temp);
    if (ret) {
        pr_err("device_create_file temp failed\n");
        goto _out;
    }
	ret = device_create_file(dev, &dev_attr_capacity);
    if (ret) {
        pr_err("device_create_file capacity failed\n");
        goto _out;
    }
	ret = device_create_file(dev, &dev_attr_usb_online);
    if (ret) {
        pr_err("device_create_file usb_online failed\n");
        goto _out;
    }
	ret = device_create_file(dev, &dev_attr_ac_online);
    if (ret) {
        pr_err("device_create_file ac_online failed\n");
        goto _out;
    }
	ret = device_create_file(dev, &dev_attr_lcd_brightness);
    if (ret) {
        pr_err("device_create_file lcd_brightness failed\n");
        goto _out;
    }
	
	return 0;
_out:
	return ret;
}

int scc_get_current(void)
{
printk("[SCC]""speed_current_map[target_speed] = %d   target_speed = %d\n", speed_current_map[target_speed], target_speed);
	return speed_current_map[target_speed];
}

