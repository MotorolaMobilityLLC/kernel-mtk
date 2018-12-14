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


static int charge_speed;
static int target_speed;   //target speed set by app layer.
static int speed_current_map[SPEED_MAX];   //actual current/ma for speed level.
static struct dentry *scc_dentry;
extern void fg_update_routine_wakeup(void);

void scc_init_speed_current_map(const char *node_str){
	struct device_node *node;
	int rc = 0;

	node = of_find_compatible_node(NULL, NULL, node_str);
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
		fg_update_routine_wakeup();
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
	seq_printf(m, "%d\n", scc_get_lcd_brightness());
	return 0;
}
static int scc_lcd_brightness_open(struct inode *inode, struct file *file)
{
	return single_open(file, scc_lcd_brightness_show, NULL);
}

static const struct file_operations scc_lcd_brightness_fops = {
	.owner = THIS_MODULE,
	.open = scc_lcd_brightness_open,
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
}

int scc_get_current(void)
{
	return speed_current_map[target_speed];
}

