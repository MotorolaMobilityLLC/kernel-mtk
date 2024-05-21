/* Copyright (c) 2015, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/export.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <linux/mm.h>
#include <linux/io.h>
#include <linux/kobject.h>
#include <linux/of.h>

#define CDBG(fmt, args...) pr_err(fmt, ##args)
static struct kobject *hqdbg_cam_kobj;

typedef unsigned char kal_uint8;
kal_uint8 otp_status_main = 0xFF;
kal_uint8 otp_status_front = 0xFF;
kal_uint8 otp_status_sub = 0xFF;
kal_uint8 otp_status_macro = 0xFF;

#define MODULE_INFO_SIZE  32
char module_info_main[MODULE_INFO_SIZE];
char module_info_front[MODULE_INFO_SIZE];
char module_info_wide[MODULE_INFO_SIZE];

//sys/camera_dbg/otp_status
static ssize_t otp_status_show(struct device *dev,
						struct device_attribute *attr, char *buf){

	int cont = 0;
	cont += snprintf(buf, PAGE_SIZE, "MainCamera:%d,FrontCamera:%d,SubCamera:%d,MacroCamera:%d\n",
			otp_status_main, otp_status_front, otp_status_sub, otp_status_macro);
	return cont;
}
DEVICE_ATTR_RO(otp_status);

//sys/camera_dbg/module_info
static ssize_t module_info_main_show(struct device *dev,
						struct device_attribute *attr, char *buf){

	int cont = 0;
	cont += snprintf(buf, PAGE_SIZE, "%s", module_info_main);
	return cont;
}
DEVICE_ATTR_RO(module_info_main);
static ssize_t module_info_front_show(struct device *dev,
						struct device_attribute *attr, char *buf){

	int cont = 0;
	cont += snprintf(buf, PAGE_SIZE, "%s", module_info_front);
	return cont;
}
DEVICE_ATTR_RO(module_info_front);
static ssize_t module_info_wide_show(struct device *dev,
						struct device_attribute *attr, char *buf){

	int cont = 0;
	cont += snprintf(buf, PAGE_SIZE, "%s", module_info_wide);
	return cont;
}
DEVICE_ATTR_RO(module_info_wide);

static int __init mtk_cam_otp_init_module(void)
{
	//sys/camera_dbg
	int ret = 0;
	hqdbg_cam_kobj = kobject_create_and_add("camera_dbg", NULL);
	pr_err("init module::enter",__func__);
	if (IS_ERR(hqdbg_cam_kobj)){
		pr_err("%s: create sysfs cam fail\n",__func__);
		return -ENOMEM;
	}
	ret = sysfs_create_file(hqdbg_cam_kobj, &dev_attr_otp_status.attr);
	if (ret < 0){
		pr_err("create camera_dbg attribute file fail\n");
	}
	ret = sysfs_create_file(hqdbg_cam_kobj, &dev_attr_module_info_main.attr);
	if (ret < 0){
		pr_err("create dev_attr_module_info_main attribute file fail\n");
	}
	ret = sysfs_create_file(hqdbg_cam_kobj, &dev_attr_module_info_front.attr);
	if (ret < 0){
		pr_err("create dev_attr_module_info_front attribute file fail\n");
	}
	ret = sysfs_create_file(hqdbg_cam_kobj, &dev_attr_module_info_wide.attr);
	if (ret < 0){
		pr_err("create dev_attr_module_info_wide attribute file fail\n");
	}
	return ret;
}

static void __exit mtk_cam_otp_exit_module(void)
{
	if (hqdbg_cam_kobj) {
		sysfs_remove_file(hqdbg_cam_kobj, &dev_attr_otp_status.attr);
		kobject_put(hqdbg_cam_kobj);
	}
}

module_init(mtk_cam_otp_init_module);
module_exit(mtk_cam_otp_exit_module);
MODULE_DESCRIPTION("mtk_otp_dump");
MODULE_LICENSE("GPL v2");
