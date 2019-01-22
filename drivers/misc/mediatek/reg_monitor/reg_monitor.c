/*
* Copyright (C) 2016 MediaTek Inc.
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

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/input.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
/* #include <mach/hardware.h> */
/* #include <plat/gpio-cfg.h> */
#include "reg_monitor.h"


#define REG_MON_NUM 10

int reg_mon_enable;
int reg_mon_type;
int reg_mon_check_type;

struct REG_MON {
	unsigned int addr;
	unsigned int mask;
	unsigned int val;
} REG_MON_T;

struct REG_MON reg_mon_list[REG_MON_NUM] = { {0, 0, 0}, };

int reg_mon_list_i;
int reg_mon_list_wrapped;

int _reg_mon_init(void)
{
	int i;

	for (i = 0; i < REG_MON_NUM; i++) {
		reg_mon_list[i].addr = -1;
		reg_mon_list[i].mask = -1;
		reg_mon_list[i].val = -1;
	}
	return 0;
}

void _reg_mon_check_list(unsigned int reg, unsigned int val)
{
	int i;
	int shift = 0;
	int check_num;
	int reg_mon_hit = 0;
	unsigned int check_data = 0;

	if (reg_mon_list_wrapped == 1)
		check_num = REG_MON_NUM;
	else
		check_num = reg_mon_list_i;


	for (i = 0; i < check_num; i++) {
		if ((reg_mon_list[i].addr == reg) && (reg_mon_list[i].mask == 0)) {
			pr_debug("reg_mon hit: reg=0x%x val=0x%x\n", reg, val);
			pr_debug("reg_mon hit: hit reg 0x%x\n", reg);
			reg_mon_hit = 1;
		} else if (reg_mon_list[i].addr == reg) {
			check_data = reg_mon_list[i].val ^ ~val;/* ex. 110 ^ ~(010) = 011, if value same = 1 */
			check_data &= reg_mon_list[i].mask;	/* ex. 011 & 101 = 001 */

			switch (reg_mon_check_type) {
			case 1:	/* check AND type */
			{
			if ((check_data ^ reg_mon_list[i].mask) == 0) {
				pr_debug("reg_mon hit: AND hit\n");
				pr_debug("reg_mon hit: reg=0x%x val=0x%x\n", reg, val);
				while (shift < 32) {
					if ((check_data & 0x1) != 0)
						pr_debug("reg_mon hit: hit bit %d\n", shift);

					check_data >>= 1;
					shift++;
				}
				shift = 0;
				reg_mon_hit = 1;
			}
			break;
			}
			case 2:	/* check OR type */
			{
			if (check_data != 0) {
				pr_debug("reg_mon hit: OR hit\n");
				pr_debug("reg_mon hit: reg=0x%x val=0x%x\n", reg, val);
				while (shift < 32) {
					if ((check_data & 0x1) != 0)
						pr_debug("reg_mon hit: hit bit %d\n", shift);

					check_data >>= 1;
					shift++;
				}
				shift = 0;
				reg_mon_hit = 1;
			}
			break;
			}
			default:
			{
				break;
			}
			}
		}

		if (reg_mon_hit == 1) {
			switch (reg_mon_type) {
			case 1:
				{
					dump_stack();
					break;
				}
			case 2:
				{
					WARN_ON(1);
					break;
				}
			default:
				{
					break;
				}
			}
		}
		reg_mon_hit = 0;
	}
}

void _reg_mon_check(unsigned int reg, unsigned int val)
{
	if (reg_mon_enable == 1)
		_reg_mon_check_list(reg, val);
#if 0
	else
		pr_debug("reg monitor mechanism not enable\n");
#endif
}

int _reg_mon_show_list(char *buf, ssize_t bufLen)
{
	int i, len = 0;

	char tmp[] = "[addr] [mask] [val]\n";

	len += snprintf(buf + len, bufLen - len, "%s", tmp);
	for (i = 0; i < REG_MON_NUM; i++) {

		if (reg_mon_list[i].addr == -1)
			break;

		len += snprintf(buf + len, bufLen - len, "0x%x  0x%x  0x%x\n",
				reg_mon_list[i].addr, reg_mon_list[i].mask, reg_mon_list[i].val);
	}

	return len;
}


static ssize_t reg_mon_show(struct device *dev, struct device_attribute *attr, char *buf)
{

	pr_debug("reg_mon_show\n");

	return _reg_mon_show_list(buf, PAGE_SIZE);
	/* return 0; */
}

static ssize_t reg_mon_store(struct device *dev, struct device_attribute *attr,
			     const char *buf, size_t size)
{
	pr_debug("reg_mon_store No Operation\n");

	return -1;
}

static ssize_t reg_mon_add_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	pr_debug("reg_mon_add_show No Operation\n");
	return 0;
}

static ssize_t reg_mon_add_store(struct device *dev, struct device_attribute *attr,
				 const char *buf, size_t size)
{
	int i, add_new = 1;
	unsigned int addr, mask, val;

	pr_debug("reg_mon_add_store\n");

	if (sscanf(buf, "%x %x %x", &addr, &mask, &val) == 3)
		pr_debug("good format: '%s'", buf);
	else
		pr_debug("invalid format: '%s'", buf);


	for (i = 0; i < REG_MON_NUM; i++) {
		if (reg_mon_list[i].addr == addr) {
			reg_mon_list[i].mask = mask;
			reg_mon_list[i].val = val;
			add_new = 0;
			break;
		}
	}
	if (add_new == 1) {
		reg_mon_list[reg_mon_list_i].addr = addr;
		reg_mon_list[reg_mon_list_i].mask = mask;
		reg_mon_list[reg_mon_list_i].val = val;

		reg_mon_list_i++;

		if (reg_mon_list_i == REG_MON_NUM) {
			reg_mon_list_i = 0;
			reg_mon_list_wrapped = 1;	/* list already wrapped */
		}
	}

	return -1;
}

int _reg_mon_ctrl_show(char *buf, ssize_t bufLen)
{
	int len = 0;

	if (reg_mon_enable == 1)
		len +=
		    snprintf(buf + len, bufLen - len, "reg_mon_enable: %d --> has enable\n",
			     reg_mon_enable);
	else
		len +=
		    snprintf(buf + len, bufLen - len, "reg_mon_enable: %d --> not enable\n",
			     reg_mon_enable);

	switch (reg_mon_type) {
	case 1:
		{
			len +=
			    snprintf(buf + len, bufLen - len,
				     "reg_mon_type: %d --> print backtrace\n", reg_mon_type);
			break;
		}
	case 2:
		{
			len +=
			    snprintf(buf + len, bufLen - len, "reg_mon_type: %d --> BUG_ON\n",
				     reg_mon_type);
			break;
		}
	default:
		{
			len +=
			    snprintf(buf + len, bufLen - len, "reg_mon_type: %d --> No Operation\n",
				     reg_mon_type);
			break;
		}
	}

	switch (reg_mon_check_type) {
	case 1:
		{
			len +=
			    snprintf(buf + len, bufLen - len, "reg_mon_check_type: %d --> AND\n",
				     reg_mon_check_type);
			break;
		}
	case 2:
		{
			len +=
			    snprintf(buf + len, bufLen - len, "reg_mon_check_type: %d --> OR\n",
				     reg_mon_check_type);
			break;
		}
	default:
		{
			len +=
			    snprintf(buf + len, bufLen - len,
				     "reg_mon_check_type: %d --> No Operation\n",
				     reg_mon_check_type);
			break;
		}
	}

	return len;
}

static ssize_t reg_mon_ctrl_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	pr_debug("reg_mon_ctrl_show\n");

	return _reg_mon_ctrl_show(buf, PAGE_SIZE);
}

static ssize_t reg_mon_ctrl_store(struct device *dev, struct device_attribute *attr,
				  const char *buf, size_t size)
{
	int en, type, check_type;

	pr_debug("reg_mon_ctrl_store\n");

	if (sscanf(buf, "%d %d %d", &en, &type, &check_type) == 3)
		pr_debug("good format: '%s'", buf);
	else
		pr_debug("invalid format: '%s'", buf);

	reg_mon_enable = en;
	reg_mon_type = type;
	reg_mon_check_type = check_type;


	return -1;
}

static ssize_t reg_mon_reset_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	pr_debug("reg_mon_reset_show No Operation\n");
	return 0;
}

static ssize_t reg_mon_reset_store(struct device *dev, struct device_attribute *attr,
				   const char *buf, size_t size)
{
	int i;

	pr_debug("reg_mon_reset_store\n");


	for (i = 0; i < REG_MON_NUM; i++) {
		reg_mon_list[i].addr = -1;
		reg_mon_list[i].mask = -1;
		reg_mon_list[i].val = -1;
	}
	reg_mon_list_i = 0;
	reg_mon_list_wrapped = 0;	/* wrapped flag reset */


	pr_debug("reg_mon_reset done\n");

	return -1;
}


static struct device_attribute reg_mon_adv_setting = {
	.attr = {
		 .name = "reg_mon_list",
		 .mode = 0664,
		 },
	.show = reg_mon_show,
	.store = reg_mon_store,
};

static DEVICE_ATTR(reg_mon_add, 0664, reg_mon_add_show, reg_mon_add_store);
static DEVICE_ATTR(reg_mon_ctrl, 0664, reg_mon_ctrl_show, reg_mon_ctrl_store);
static DEVICE_ATTR(reg_mon_reset, 0664, reg_mon_reset_show, reg_mon_reset_store);

static struct device_attribute *reg_mon_adv_group[] = {
	&reg_mon_adv_setting,	/* /sys/devices/platform/reg_mon.0/reg_mon_list */
	/* /sys/bus/platform/devices/reg_mon.0/reg_mon_list */

	&dev_attr_reg_mon_add,
	&dev_attr_reg_mon_ctrl,
	&dev_attr_reg_mon_reset,
};

static int reg_mon_probe(struct platform_device *pdev)
{
	int i = 0, ret;

	pr_debug("reg_mon_probe\n");

	/* 1. reg_mon init */
	_reg_mon_init();
#if 0
	reg_mon_list[0].addr = 0;
	reg_mon_list[0].mask = 0;
	reg_mon_list[0].val = 0;
	reg_mon_list[1].addr = 1;
	reg_mon_list[1].mask = 2;
	reg_mon_list[1].val = 3;
#endif

	/* 2. create attribute */
	for (i = 0; i < ARRAY_SIZE(reg_mon_adv_group); i++) {
		ret = device_create_file(&pdev->dev, reg_mon_adv_group[i]);
		if (ret) {
			dev_err(&pdev->dev, "failed: sysfs file %s\n",
				reg_mon_adv_group[i]->attr.name);
		}
	}

	return 0;

}

static int __exit reg_mon_remove(struct platform_device *pdev)
{
	pr_debug("reg_mon_remove\n");
	return 0;
}

static int reg_mon_suspend(struct platform_device *pdev, pm_message_t state)
{
	pr_debug("reg_mon_suspend\n");
	return 0;
}

static int reg_mon_resume(struct platform_device *pdev)
{
	pr_debug("reg_mon_resume\n");
	return 0;
}


static struct platform_driver reg_mon_driver = {
	.probe = reg_mon_probe,
	.remove = /*__devexit_p(*/ reg_mon_remove /*) */,
	.driver = {
		   .name = "reg_mon",
		   .owner = THIS_MODULE,
		   },
	.suspend = reg_mon_suspend,
	.resume = reg_mon_resume,
};

static struct platform_device reg_mon_device = {
	.name = "reg_mon",
	.dev = {
		/* .platform_data = &reg_mon_data, */
		}
};

static int __init reg_mon_init(void)
{
	int ret = 0;


	ret = platform_device_register(&reg_mon_device);
	pr_debug("reg_mon platform_device_register\n");

	if (ret == 0) {
		ret = platform_driver_register(&reg_mon_driver);
		pr_debug("reg_mon platform_driver_register\n");
	}


	return ret;
}
module_init(reg_mon_init);

static void __exit reg_mon_exit(void)
{

	platform_driver_unregister(&reg_mon_driver);
	platform_device_unregister(&reg_mon_device);
}
module_exit(reg_mon_exit);

MODULE_AUTHOR("Lomen Wu");
MODULE_DESCRIPTION("reg_mon driver");
MODULE_LICENSE("GPL");
