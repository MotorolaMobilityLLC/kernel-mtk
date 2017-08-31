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

#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/cdev.h>
#include <linux/io.h>

#include <generated/autoconf.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>

#include <linux/uaccess.h>

#include <mmprofile_internal.h>
/* #pragma GCC optimize ("O0") */
#define MMP_DEVNAME "mmp"

void mmprofile_start(int start)
{
}

void mmprofile_enable(int enable)
{
}

/* Exposed APIs begin */
mmp_event mmprofile_register_event(mmp_event parent, const char *name)
{
	return 0;
}
EXPORT_SYMBOL(mmprofile_register_event);

mmp_event mmprofile_find_event(mmp_event parent, const char *name)
{
	return 0;
}
EXPORT_SYMBOL(mmprofile_find_event);

void mmprofile_enable_event(mmp_event event, long enable)
{
}
EXPORT_SYMBOL(mmprofile_enable_event);

void mmprofile_enable_event_recursive(mmp_event event, long enable)
{
}
EXPORT_SYMBOL(mmprofile_enable_event_recursive);

long mmprofile_query_enable(mmp_event event)
{
	return 0;
}
EXPORT_SYMBOL(mmprofile_query_enable);

void mmprofile_log_ex(mmp_event event, mmp_log_type type, unsigned long data1, unsigned long data2)
{
}
EXPORT_SYMBOL(mmprofile_log_ex);

void mmprofile_log(mmp_event event, mmp_log_type type)
{
}
EXPORT_SYMBOL(mmprofile_log);

long mmprofile_log_meta(mmp_event event, mmp_log_type type, mmp_metadata_t *pMetaData)
{
	return 0;
}
EXPORT_SYMBOL(mmprofile_log_meta);

long mmprofile_log_meta_structure(mmp_event event, mmp_log_type type,
			       mmp_metadata_structure_t *pMetaData)
{
	return 0;
}
EXPORT_SYMBOL(mmprofile_log_meta_structure);

long mmprofile_log_meta_string_ex(mmp_event event, mmp_log_type type, unsigned long data1,
			      unsigned long data2, const char *str)
{
	return 0;
}
EXPORT_SYMBOL(mmprofile_log_meta_string_ex);

long mmprofile_log_meta_string(mmp_event event, mmp_log_type type, const char *str)
{
	return 0;
}
EXPORT_SYMBOL(mmprofile_log_meta_string);

long mmprofile_log_meta_bitmap(mmp_event event, mmp_log_type type, mmp_metadata_bitmap_t *pMetaData)
{
	return 0;
}
EXPORT_SYMBOL(mmprofile_log_meta_bitmap);

long mmprofile_log_meta_yuv_bitmap(mmp_event event, mmp_log_type type, mmp_metadata_bitmap_t *pMetaData)
{
	return 0;
}
EXPORT_SYMBOL(mmprofile_log_meta_yuv_bitmap);

/* Exposed APIs end */

/* Driver specific begin */
#if 0
static dev_t mmprofile_devno;
static struct cdev *mmprofile_cdev;
static struct class *mmprofile_class;
#endif
static int mmprofile_release(struct inode *inode, struct file *file)
{
	return 0;
}

static int mmprofile_open(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t mmprofile_read(struct file *file, char __user *data, size_t len, loff_t *ppos)
{
	return 0;
}

static ssize_t mmprofile_write(struct file *file, const char __user *data, size_t len,
			       loff_t *ppos)
{
	return 0;
}

static long mmprofile_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return 0;
}

static int mmprofile_mmap(struct file *file, struct vm_area_struct *vma)
{
	return -EINVAL;
}

const struct file_operations mmprofile_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = mmprofile_ioctl,
	.open = mmprofile_open,
	.release = mmprofile_release,
	.read = mmprofile_read,
	.write = mmprofile_write,
	.mmap = mmprofile_mmap,
};

/* fix build warning: unused */
#if 0
static int mmprofile_probe(struct platform_device *pdev)
{
#if 0
	struct class_device *class_dev = 0;
	int ret = alloc_chrdev_region(&mmprofile_devno, 0, 1, MMP_DEVNAME);

	mmprofile_cdev = cdev_alloc();
	mmprofile_cdev->owner = THIS_MODULE;
	mmprofile_cdev->ops = &mmprofile_fops;
	ret = cdev_add(mmprofile_cdev, mmprofile_devno, 1);
	mmprofile_class = class_create(THIS_MODULE, MMP_DEVNAME);
	class_dev =
	    (struct class_device *)device_create(mmprofile_class, NULL, mmprofile_devno, NULL,
						 MMP_DEVNAME);
#endif
	return 0;
}
#endif

/* fix build warning: unused */
#if 0
static int mmprofile_remove(struct platform_device *pdev)
{
	return 0;
}
#endif

#if 0
static struct platform_driver mmprofile_driver = {
	.probe = mmprofile_probe,
	.remove = mmprofile_remove,
	.driver = {.name = MMP_DEVNAME}
};

static struct platform_device mmprofile_device = {
	.name = MMP_DEVNAME,
	.id = 0,
};

#endif

static int __init mmprofile_init(void)
{
#if 0
	if (platform_device_register(&mmprofile_device))
		return -ENODEV;

	if (platform_driver_register(&mmprofile_driver)) {
		platform_device_unregister(&mmprofile_device);
		return -ENODEV;
	}
#endif
	return 0;
}

static void __exit mmprofile_exit(void)
{
#if 0
	device_destroy(mmprofile_class, mmprofile_devno);
	class_destroy(mmprofile_class);
	cdev_del(mmprofile_cdev);
	unregister_chrdev_region(mmprofile_devno, 1);

	platform_driver_unregister(&mmprofile_driver);
	platform_device_unregister(&mmprofile_device);
#endif
}

/* Driver specific end */

module_init(mmprofile_init);
module_exit(mmprofile_exit);
MODULE_AUTHOR("Tianshu Qiu <tianshu.qiu@mediatek.com>");
MODULE_DESCRIPTION("MMProfile Driver");
MODULE_LICENSE("GPL");
