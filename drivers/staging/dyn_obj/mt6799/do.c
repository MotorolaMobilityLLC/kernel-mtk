/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/sysfs.h>
#include <linux/mutex.h>

#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_reserved_mem.h>
#include <mt-plat/mtk_memcfg.h>
#include <emi_mpu.h>

#include <linux/printk.h>
#include <linux/string.h>

#include <linux/kdev_t.h>

#include "do_ipi.h"
#include "do_impl.h"

#include <linux/notifier.h>

#define DO_MPU_REG_NUM 21
#define MAX_RETRY_COUNT 10

/*************** Function Prototypes ************/
static int __init do_service_init(void);
static void __exit do_service_exit(void);

static int create_files(void);

static int do_open(struct inode *inode, struct file *filp);
static int do_release(struct inode *inode, struct file *filp);
static ssize_t do_read(struct file *filp, char __user *buf, size_t count, loff_t *pos);
static ssize_t do_write(struct file *filp, const char __user *buf, size_t count, loff_t *pos);

static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = do_open,
	.release = do_release,
	.write = do_write,
	.read = do_read
};

static struct miscdevice do_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "dynamic_obj",
	.fops = &fops
};

/************ scp ready callbacks **************/
static int do_scp_event_handler(struct notifier_block *this, unsigned long event, void *ptr)
{
	switch (event) {
	case SCP_EVENT_READY:
		pr_debug("do_scp_handler: scp READY\n");
		if (!do_send_dram_start_addr(1)) { /* send in isr */
			pr_err("DO DRAM addr is not sent to scp successfully.\n");
		}
		break;
	case SCP_EVENT_STOP:
		pr_debug("do_scp_handler: scp STOP\n");
		reset_do_api();
		break;
	default:
		/* do nothing */
		break;
	}
	return NOTIFY_DONE;
}

static struct notifier_block scp_notifier = {
	.notifier_call = do_scp_event_handler
};

/************* init DRAM memory in device tree **********/
#define SCP_GENERAL_DT "mediatek,scp"

phys_addr_t phys_base_a;
phys_addr_t phys_base_b;

static int do_mem_init(void)
{
	/* find the device node */
	struct device_node *node;
	u32 reg[2];

	node = of_find_compatible_node(NULL, NULL, SCP_GENERAL_DT);
	if (!node) {
		pr_err("[DO]: reserved mem not found\n");
		return 0;
	}

	if (of_property_read_u32_array(node, "do_addr_A", reg, 2)) {
		pr_err("[DO]: attribute do_addr_A not found in dt node\n");
		return 0;
	}
	phys_base_a = reg[1];

	if (of_property_read_u32_array(node, "do_addr_B", reg, 2)) {
		pr_err("[DO]: attribute do_addr_B not found in dt node\n");
		return 0;
	}
	phys_base_b = reg[1];

	pr_debug("base phy addr A = 0x%llx, addr B:0x%llx\n", phys_base_a, phys_base_b);

	return 1;
}

int do_send_dram_start_addr(int in_isr)
{
	int ret = 1;

	if (phys_base_a)
		ret &= do_ipi_send_dram_addr((uint32_t)phys_base_a, SCP_A_ID, in_isr);
	if (phys_base_b)
		ret &= do_ipi_send_dram_addr((uint32_t)phys_base_b, SCP_B_ID, in_isr);
	return ret;
}

/************* sysfs operations ****************/
static ssize_t show_do_info(struct device *dev, struct device_attribute *attr, char *buf)
{
	int len = 0;
	struct do_item *node = get_detail_do_infos();

	while (node) {
		len += scnprintf(buf + len, PAGE_SIZE - len, "%s 0x%x %d\n",
				 node->header->id, node->header->dram_addr, node->header->size);
		node = node->next;
	}
	return len;
}

DEVICE_ATTR(do_info, 0444, show_do_info, NULL);

static ssize_t show_current_do(struct device *dev, struct device_attribute *attr, char *buf)
{
	int len = 0;
	struct do_list_node *current_do;

	current_do = mt_do_get_loaded_do(SCP_A);
	if (current_do)
		len += scnprintf(buf, PAGE_SIZE, "SCP_A: %s\n", current_do->name);
	current_do = mt_do_get_loaded_do(SCP_B);
	if (current_do)
		len += scnprintf(buf, PAGE_SIZE, "SCP_B: %s\n", current_do->name);

	return len;
}

#ifdef CONFIG_MTK_ENG_BUILD
static ssize_t set_load_do(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
	char tmp[50];

	/* remove '\n' */
	strcpy(tmp, buf);
	tmp[n - 1] = '\0';
	if (mt_do_load_do(tmp))
		pr_debug("set_load_do: set do success\n");
	else
		pr_debug("set_load_do: set failed\n");

	return n;
}
DEVICE_ATTR(load_do, S_IRUGO | S_IWUSR, show_current_do, set_load_do);

static ssize_t set_unload_do(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
	char tmp[50];

	/* remove '\n' */
	strcpy(tmp, buf);
	tmp[n - 1] = '\0';
	if (mt_do_unload_do(tmp))
		pr_debug("set_unload_do: set do success\n");
	else
		pr_debug("set_unload_do: set failed\n");

	return n;
}
DEVICE_ATTR(unload_do, S_IRUGO | S_IWUSR, show_current_do, set_unload_do);
#else
DEVICE_ATTR(load_do, 0444, show_current_do, NULL);
DEVICE_ATTR(unload_do, 0444, show_current_do, NULL);
#endif

static int create_files(void)
{
	int res;

	res = misc_register(&do_device);

	if (res != 0) {
		pr_err("ERR: misc register failed.");
		return res;
	}

	res = device_create_file(do_device.this_device, &dev_attr_do_info);
	if (res != 0) {
		pr_err("ERR: create sysfs do_info failed.");
		return res;
	}
	res = device_create_file(do_device.this_device, &dev_attr_load_do);
	if (res != 0) {
		pr_err("ERR: create sysfs load_do failed.");
		return res;
	}
	res = device_create_file(do_device.this_device, &dev_attr_unload_do);
	if (res != 0) {
		pr_err("ERR: create sysfs unload_do failed.");
		return res;
	}
	return 0;
}

/************* kernel module file ops (dummy) ****************/

static int do_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int do_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t do_read(struct file *filp, char __user *buf, size_t count, loff_t *pos)
{
	*pos = 0;
	return 0;
}

static ssize_t do_write(struct file *filp, const char __user *buf, size_t count, loff_t *pos)
{
	return count;
}

/************ kernel module init entry ***************/
static int __init do_service_init(void)
{
	int res;
	ipi_status status;

	res = create_files();
	if (res) {
		pr_err("create sysfs failed: %d\n", res);
		return -1;
	}

	status = scp_ipi_registration(IPI_DO_SCP_MSG, do_ipi_handler, "DO");
	if (status) {
		pr_err("scp_ipi_registration failed: %d\n", status);
		return -1;
	}

	res = do_mem_init();
	if (res) {
		set_do_api_enable();
		/* SCP A currently disabled */
		/* scp_A_register_notify(&scp_notifier); */
		scp_B_register_notify(&scp_notifier);
	} else {
		pr_err("find device tree failed: %d\n", res);
		return -1;
	}
	return 0;
}

/************ kernel module exit entry ***************/
static void __exit do_service_exit(void)
{
	pr_debug("do_exit\n");
	misc_deregister(&do_device);
	deinit_do_infos();
}

module_init(do_service_init);
module_exit(do_service_exit);
