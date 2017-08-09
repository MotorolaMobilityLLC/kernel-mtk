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

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <asm/atomic.h>
#include <linux/module.h>

#include "da9214.h"

#define PMIC_DEBUG_PR_DBG
/**********************************************************
  *
  *   [I2C Slave Setting]
  *
  *********************************************************/
#define DA9214_SLAVE_ADDR_WRITE	0xD0
#define DA9214_SLAVE_ADDR_READ	0xD1

#define da9214_BUSNUM			6

static struct i2c_client *new_client;
static const struct i2c_device_id da9214_i2c_id[] = { {"da9214", 0}, {} };

#ifdef CONFIG_OF
static const struct of_device_id da9214_of_ids[] = {
	{.compatible = "mediatek,vproc_buck"},
	{},
};
/*-----PinCtrl START-----*/
static const struct of_device_id da9214_buck_of_ids[] = {
	{.compatible = "mediatek,ext_buck_vproc"},
	{},
};
/*-----PinCtrl END-----*/
#endif

static int da9214_driver_probe(struct i2c_client *client, const struct i2c_device_id *id);

static struct i2c_driver da9214_driver = {
	.driver = {
		   .name = "da9214",
#ifdef CONFIG_OF
		   .of_match_table = da9214_of_ids,
#endif
		   },
	.probe = da9214_driver_probe,
	.id_table = da9214_i2c_id,
};

/*-----PinCtrl START-----*/
#define DA9214_GPIO_MODE_EN_DEFAULT 0

char *da9214_gpio_cfg[] = {  "da9214_gpio_default"};

void switch_da9214_gpio(struct pinctrl *ppinctrl, int mode)
{
	struct pinctrl_state *ppins = NULL;

	/*pr_debug("[DA9214][PinC]%s(%d)+\n", __func__, mode);*/

	if (mode >= (sizeof(da9214_gpio_cfg) / sizeof(da9214_gpio_cfg[0]))) {
		pr_err("[DA9214][PinC]%s(%d) fail!! - parameter error!\n", __func__, mode);
		return;
	}

	if (IS_ERR(ppinctrl)) {
		pr_err("[DA9214][PinC]%s ppinctrl:%p is error! err:%ld\n",
		       __func__, ppinctrl, PTR_ERR(ppinctrl));
		return;
	}

	ppins = pinctrl_lookup_state(ppinctrl, da9214_gpio_cfg[mode]);
	if (IS_ERR(ppins)) {
		pr_err("[DA9214][PinC]%s pinctrl_lockup(%p, %s) fail!! ppinctrl:%p, err:%ld\n",
		       __func__, ppinctrl, da9214_gpio_cfg[mode], ppins, PTR_ERR(ppins));
		return;
	}

	pinctrl_select_state(ppinctrl, ppins);
	/*pr_debug("[DA9214][PinC]%s(%d)-\n", __func__, mode);*/
}
/*-----PinCtrl END-----*/

/**********************************************************
  *
  *   [Global Variable]
  *
  *********************************************************/
static DEFINE_MUTEX(da9214_i2c_access);
static DEFINE_MUTEX(da9214_lock_mutex);

int g_da9214_driver_ready = 0;
int g_da9214_hw_exist = 0;
int g_da9214_buckb_en_debug = 0;
unsigned int g_da9214_cid = 0;

#define PMICTAG                "[DA9214] "
#if defined(PMIC_DEBUG_PR_DBG)
#define PMICLOG1(fmt, arg...)   pr_err(PMICTAG fmt, ##arg)
#else
#define PMICLOG1(fmt, arg...)
#endif

/**********************************************************
  *
  *   [I2C Function For Read/Write da9214]
  *
  *********************************************************/
#ifdef CONFIG_MTK_I2C_EXTENSION
unsigned int da9214_read_byte(unsigned char cmd, unsigned char *returnData)
{
	char cmd_buf[1] = { 0x00 };
	char readData = 0;
	int ret = 0;

	mutex_lock(&da9214_i2c_access);

	/*
	   new_client->ext_flag =
	   ((new_client->ext_flag) & I2C_MASK_FLAG) | I2C_WR_FLAG | I2C_DIRECTION_FLAG;
	 */
	new_client->ext_flag =
	    ((new_client->ext_flag) & I2C_MASK_FLAG) | I2C_WR_FLAG | I2C_PUSHPULL_FLAG |
	    I2C_HS_FLAG;
	new_client->timing = 3400;


	cmd_buf[0] = cmd;
	ret = i2c_master_send(new_client, &cmd_buf[0], (1 << 8 | 1));
	if (ret < 0) {
		PMICLOG1("[da9214_read_byte] ret=%d\n", ret);

		new_client->ext_flag = 0;
		mutex_unlock(&da9214_i2c_access);
		return ret;
	}

	readData = cmd_buf[0];
	*returnData = readData;

	new_client->ext_flag = 0;

	mutex_unlock(&da9214_i2c_access);
	return 1;
}

unsigned int da9214_write_byte(unsigned char cmd, unsigned char writeData)
{
	char write_data[2] = { 0 };
	int ret = 0;

	if (g_da9214_buckb_en_debug) {
		if (cmd == 0x5e) {
			if ((writeData & 0x01) == 0x0)
				BUG_ON(1);
		}
	}
	mutex_lock(&da9214_i2c_access);

	write_data[0] = cmd;
	write_data[1] = writeData;


	/* new_client->ext_flag = ((new_client->ext_flag) & I2C_MASK_FLAG) | I2C_DIRECTION_FLAG; */
	new_client->ext_flag =
	    ((new_client->ext_flag) & I2C_MASK_FLAG) | I2C_DIRECTION_FLAG | I2C_PUSHPULL_FLAG |
	    I2C_HS_FLAG;
	new_client->timing = 3400;



	ret = i2c_master_send(new_client, write_data, 2);
	if (ret < 0) {
		PMICLOG1("[da9214_write_byte] ret=%d\n", ret);

		new_client->ext_flag = 0;
		mutex_unlock(&da9214_i2c_access);
		return ret;
	}

	new_client->ext_flag = 0;
	mutex_unlock(&da9214_i2c_access);
	return 1;
}
#else
unsigned int da9214_read_byte(unsigned char cmd, unsigned char *returnData)
{
	unsigned char xfers = 2;
	int ret, retries = 1;

	mutex_lock(&da9214_i2c_access);
	do {
		struct i2c_msg msgs[2] = {
			{
			 .addr = new_client->addr,
			 .flags = 0,
			 .len = 1,
			 .buf = &cmd,
			 }, {

			     .addr = new_client->addr,
			     .flags = I2C_M_RD,
			     .len = 1,
			     .buf = returnData,
			     }
		};

		/*
		 * Avoid sending the segment addr to not upset non-compliant
		 * DDC monitors.
		 */
		ret = i2c_transfer(new_client->adapter, msgs, xfers);

		if (ret == -ENXIO) {
			PMICLOG1("skipping non-existent adapter %s\n", new_client->adapter->name);
			break;
		}
	} while (ret != xfers && --retries);

	mutex_unlock(&da9214_i2c_access);

	return ret == xfers ? 1 : -1;
}

unsigned int da9214_write_byte(unsigned char cmd, unsigned char writeData)
{
	unsigned char xfers = 1;
	int ret, retries = 1;
	unsigned char buf[8];
	if (g_da9214_buckb_en_debug) {
		if (cmd == 0x5e) {
			if ((writeData & 0x01) == 0x0)
				BUG_ON(1);
		}
	}
	mutex_lock(&da9214_i2c_access);

	buf[0] = cmd;
	memcpy(&buf[1], &writeData, 1);

	do {
		struct i2c_msg msgs[1] = {
			{
			 .addr = new_client->addr,
			 .flags = 0,
			 .len = 1 + 1,
			 .buf = buf,
			 },
		};

		/*
		 * Avoid sending the segment addr to not upset non-compliant
		 * DDC monitors.
		 */
		ret = i2c_transfer(new_client->adapter, msgs, xfers);

		if (ret == -ENXIO) {
			PMICLOG1("skipping non-existent adapter %s\n", new_client->adapter->name);
			break;
		}
	} while (ret != xfers && --retries);

	mutex_unlock(&da9214_i2c_access);

	return ret == xfers ? 1 : -1;
}
#endif
/*
 *   [Read / Write Function]
 */
unsigned int da9214_read_interface(unsigned char RegNum, unsigned char *val, unsigned char MASK,
				   unsigned char SHIFT)
{
	unsigned char da9214_reg = 0;
	unsigned int ret = 0;

	/* PMICLOG1("--------------------------------------------------\n"); */

	ret = da9214_read_byte(RegNum, &da9214_reg);

	/* PMICLOG1("[da9214_read_interface] Reg[%x]=0x%x\n", RegNum, da9214_reg); */

	da9214_reg &= (MASK << SHIFT);
	*val = (da9214_reg >> SHIFT);

	/* PMICLOG1("[da9214_read_interface] val=0x%x\n", *val); */

	return ret;
}

unsigned int da9214_config_interface(unsigned char RegNum, unsigned char val, unsigned char MASK,
				     unsigned char SHIFT)
{
	unsigned char da9214_reg = 0;
	unsigned int ret = 0;

	/*PMICLOG1("--------------------------------------------------\n"); */

	ret = da9214_read_byte(RegNum, &da9214_reg);
	/* PMICLOG1("[da9214_config_interface] Reg[%x]=0x%x\n", RegNum, da9214_reg); */

	da9214_reg &= ~(MASK << SHIFT);
	da9214_reg |= (val << SHIFT);

	ret = da9214_write_byte(RegNum, da9214_reg);
	/*PMICLOG1("[da9214_config_interface] write Reg[%x]=0x%x\n", RegNum, da9214_reg); */

	/* Check */
	/*ret = da9214_read_byte(RegNum, &da9214_reg);
	   PMICLOG1("[da9214_config_interface] Check Reg[%x]=0x%x\n", RegNum, da9214_reg);
	 */

	return ret;
}

void da9214_set_reg_value(unsigned int reg, unsigned int reg_val)
{
	unsigned int ret = 0;

	ret = da9214_config_interface((unsigned char)reg, (unsigned char)reg_val, 0xFF, 0x0);
}

unsigned int da9214_get_reg_value(unsigned int reg)
{
	unsigned int ret = 0;
	unsigned char reg_val = 0;

	ret = da9214_read_interface((unsigned char)reg, &reg_val, 0xFF, 0x0);

	return reg_val;
}

/*
 *   [APIs]
 */
void da9214_lock(void)
{
	mutex_lock(&da9214_lock_mutex);
}

void da9214_unlock(void)
{
	mutex_unlock(&da9214_lock_mutex);
}

void da9214_buckb_lock(unsigned int reg_val)
{
	if (reg_val)
		g_da9214_buckb_en_debug = 1;
	else
		g_da9214_buckb_en_debug = 0;

}


/*
 *   [Internal Function]
 */
void da9214_dump_register(void)
{
	unsigned char i = 0;

	/* within 0x50 to 0x67, 0xD0 to DF, 0x140 to 0x14F and (read only) 0x200 to 0x27F. */
	/*
	   000: Selects Register 0x01 to 0x3F
	   001: Selects Register 0x81 to 0xCF
	   010: Selects Register 0x101 to 0x1CF
	 */
	/* ---------------------------------------------------------------- */
	PMICLOG1("[da9214] page 0,1:\n");
	PMICLOG1("[0x%x]=0x%x ", 0x0, da9214_get_reg_value(0x0));
	for (i = 0x50; i <= 0x5E; i++)
		PMICLOG1("Dump: [0x%x]=0x%x\n", i, da9214_get_reg_value(i));

	for (i = 0xD0; i <= 0xDA; i++) {
		da9214_config_interface(0x0, 0x1, 0xF, 0);	/* select to page 1 */
		PMICLOG1("Dump: [0x%x]=0x%x\n", i, da9214_get_reg_value(i));
	}
	PMICLOG1("\n");
	/* ---------------------------------------------------------------- */
	PMICLOG1("[da9214] page 2,3:\n");
	for (i = 0x01; i <= 0x06; i++) {
		da9214_config_interface(0x0, 0x2, 0xF, 0);	/* select to page 2,3 */
		PMICLOG1("Dump: [0x%x]=0x%x\n", i, da9214_get_reg_value(i));
	}
	for (i = 0x43; i <= 0x48; i++) {
		da9214_config_interface(0x0, 0x2, 0xF, 0);	/* select to page 2,3 */
		PMICLOG1("Dump: [0x%x]=0x%x\n", i, da9214_get_reg_value(i));
	}
	PMICLOG1("\n");
	/* ---------------------------------------------------------------- */
	da9214_config_interface(0x0, 0x1, 0xF, 0);	/* select to page 0,1 */
	/* ---------------------------------------------------------------- */
}

int get_da9214_i2c_ch_num(void)
{
	return da9214_BUSNUM;
}

void da9214_hw_init(void)
{
	PMICLOG1("[da9214_hw_init] 20150902, Anderson Tsai\n");
	/* Enable Watchdog */

	PMICLOG1("[da9214_hw_init] Done\n");
}

void da9214_hw_component_detect(void)
{
	unsigned int ret = 0;
	unsigned char val = 0;

	ret = da9214_config_interface(0x0, 0x1, 0x1, 7);	/* page reverts to 0 after one access */
	ret = da9214_config_interface(0x0, 0x2, 0xF, 0);	/* select to page 2,3 */

	ret = da9214_read_interface(0x5, &val, 0xF, 4);	/* Read IF_BASE_ADDR1 */

	/* check default SPEC. value */
	if (val == 0xD)
		g_da9214_hw_exist = 1;
	else
		g_da9214_hw_exist = 0;


	PMICLOG1("[da9214_hw_component_detect] exist=%d, Reg[0x105][7:4]=0x%x\n",
		 g_da9214_hw_exist, val);
}

int is_da9214_sw_ready(void)
{
	/*PMICLOG1("g_da9214_driver_ready=%d\n", g_da9214_driver_ready); */

	return g_da9214_driver_ready;
}

int is_da9214_exist(void)
{
	/*PMICLOG1("g_da9214_hw_exist=%d\n", g_da9214_hw_exist); */

	return g_da9214_hw_exist;
}

int da9214_vosel_buck_a(unsigned long val)
{
	int ret = 1;
	unsigned long reg_val = 0;

	/* reg_val = ( (val) - 30000 ) / 1000; //300mV~1570mV, step=10mV */
	reg_val = ((((val * 10) - 300000) / 1000) + 9) / 10;

	if (reg_val > 127)
		reg_val = 127;

	ret = da9214_write_byte(0xD7, reg_val);

	return ret;
}

int da9214_vosel_buck_b(unsigned long val)
{
	int ret = 1;
	unsigned long reg_val = 0;

	/* reg_val = ( (val) - 30000 ) / 1000; //300mV~1570mV, step=10mV */
	reg_val = ((((val * 10) - 300000) / 1000) + 9) / 10;

	if (reg_val > 127)
		reg_val = 127;

	ret = da9214_write_byte(0xD9, reg_val);

	return ret;
}

static int da9214_driver_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int err = 0;

	PMICLOG1("[da9214_driver_probe]\n");
	new_client = kmalloc(sizeof(struct i2c_client), GFP_KERNEL);
	if (new_client == NULL) {
		err = -ENOMEM;
		goto exit;
	}
	memset(new_client, 0, sizeof(struct i2c_client));

	new_client = client;

	/* --------------------- */
	da9214_hw_component_detect();
	if (g_da9214_hw_exist == 1) {
		da9214_hw_init();
		da9214_dump_register();
	}
	g_da9214_driver_ready = 1;

	PMICLOG1("[da9214_driver_probe] g_da9214_hw_exist=%d, g_da9214_driver_ready=%d\n",
		 g_da9214_hw_exist, g_da9214_driver_ready);

	if (g_da9214_hw_exist == 0) {
		PMICLOG1("[da9214_driver_probe] return err\n");
		return err;
	}

	da9214_config_interface(0x0, 0x1, 0xF, 0);	/* select to page 0,1 */

	return 0;

exit:
	PMICLOG1("[da9214_driver_probe] exit: return err\n");
	return err;
}

/*
 *   [platform_driver API]
 */
#ifdef DA9214_AUTO_DETECT_DISABLE
    /* TBD */
#else
/*
 * da9214_access
 */
unsigned char g_reg_value_da9214 = 0;
static ssize_t show_da9214_access(struct device *dev, struct device_attribute *attr, char *buf)
{
	PMICLOG1("[show_da9214_access] 0x%x\n", g_reg_value_da9214);
	return sprintf(buf, "%u\n", g_reg_value_da9214);
}

static ssize_t store_da9214_access(struct device *dev, struct device_attribute *attr,
				   const char *buf, size_t size)
{
	int ret;
	char *pvalue = NULL, *addr, *val;
	unsigned int reg_value = 0;
	unsigned int reg_address = 0;

	pr_err("[store_da9214_access]\n");

	if (buf != NULL && size != 0) {
		/* PMICLOG1("[store_da9214_access] buf is %s and size is %d\n",buf,size); */
		/* reg_address = simple_strtoul(buf, &pvalue, 16); */

		pvalue = (char *)buf;
		if (size > 5) {
			addr = strsep(&pvalue, " ");
			ret = kstrtou32(addr, 16, (unsigned int *)&reg_address);
		} else
			ret = kstrtou32(pvalue, 16, (unsigned int *)&reg_address);
		/*ret = kstrtoul(buf, 16, (unsigned long *)&reg_address); */

		if (size > 5) {
			/*reg_value = simple_strtoul((pvalue + 1), NULL, 16); */
			val = strsep(&pvalue, " ");
			ret = kstrtou32(val, 16, (unsigned int *)&reg_value);
			pr_err("[store_da9214_access] write da9214 reg 0x%x with value 0x%x !\n",
			       reg_address, reg_value);

			if (reg_address < 0x100) {
				pr_notice("[store_da9214_access] page 0,1\n");
				da9214_config_interface(0x0, 0x0, 0xF, 0);	/* select to page 0,1 */
			} else {
				pr_notice("[store_da9214_access] page 2,3\n");
				da9214_config_interface(0x0, 0x2, 0xF, 0);	/* select to page 2,3 */
				reg_address = reg_address & 0xFF;
			}
			ret = da9214_config_interface(reg_address, reg_value, 0xFF, 0x0);

			/* restore to page 0,1 */
			da9214_config_interface(0x0, 0x0, 0xF, 0);	/* select to page 0,1 */
		} else {
			if (reg_address < 0x100) {
				pr_notice("[store_da9214_access] page 0,1\n");
				da9214_config_interface(0x0, 0x0, 0xF, 0);	/* select to page 0,1 */
			} else {
				pr_notice("[store_da9214_access] page 2,3\n");
				da9214_config_interface(0x0, 0x2, 0xF, 0);	/* select to page 2,3 */
				reg_address = reg_address & 0xFF;
			}
			ret = da9214_read_interface(reg_address, &g_reg_value_da9214, 0xFF, 0x0);

			/* restore to page 0,1 */
			da9214_config_interface(0x0, 0x0, 0xF, 0);	/* select to page 0,1 */

			pr_err("[store_da9214_access] read da9214 reg 0x%x with value 0x%x !\n",
			       reg_address, g_reg_value_da9214);
			pr_err
			    ("[store_da9214_access] use \"cat da9214_access\" to get value(decimal)\r\n");
		}
	}
	return size;
}

static DEVICE_ATTR(da9214_access, 0664, show_da9214_access, store_da9214_access);	/* 664 */

/*
 * da9214_user_space_probe
 */
static int da9214_user_space_probe(struct platform_device *dev)
{
	int ret_device_file = 0;
	struct pinctrl *ppinctrl_da9214;

	PMICLOG1("******** da9214_user_space_probe!! ********\n");

	/*-----PinCtrl START-----*/
	ppinctrl_da9214 = devm_pinctrl_get(&dev->dev);
	if (IS_ERR(ppinctrl_da9214)) {
		pr_err("[DA9214][PinC]cannot find pinctrl. ptr_err:%ld\n",
		       PTR_ERR(ppinctrl_da9214));
		return PTR_ERR(ppinctrl_da9214);
	}
	pr_err("[DA9214][PinC]devm_pinctrl_get ppinctrl:%p\n", ppinctrl_da9214);

	/* Set GPIO as default */
	switch_da9214_gpio(ppinctrl_da9214, DA9214_GPIO_MODE_EN_DEFAULT);
	/*-----PinCtrl END-----*/
	ret_device_file = device_create_file(&(dev->dev), &dev_attr_da9214_access);

	return 0;
}

struct platform_device da9214_user_space_device = {
	.name = "da9214-user",
	.id = -1,
};

static struct platform_driver da9214_user_space_driver = {
	.probe = da9214_user_space_probe,
	.driver = {
		   .name = "da9214-user",
#ifdef CONFIG_OF
		   .of_match_table = da9214_buck_of_ids,
#endif
		   },
};

/*
static struct i2c_board_info __initdata i2c_da9214 =
{  I2C_BOARD_INFO("XXXXXXXX", (DA9214_SLAVE_ADDR_WRITE >> 1)) };
*/

#endif

static int __init da9214_init(void)
{
#ifdef da9214_AUTO_DETECT_DISABLE

	PMICLOG1("[da9214_init] DA9214_AUTO_DETECT_DISABLE\n");
	g_da9214_hw_exist = 0;
	g_da9214_driver_ready = 1;

#else

	int ret = 0;

	/* if (g_vproc_vsel_gpio_number != 0) { */
	if (1) {
		PMICLOG1("[da9214_init] init start. ch=%d!!\n", da9214_BUSNUM);

		/* i2c_register_board_info(da9214_BUSNUM, &i2c_da9214, 1); */

		if (i2c_add_driver(&da9214_driver) != 0)
			PMICLOG1("[da9214_init] failed to register da9214 i2c driver.\n");
		else
			PMICLOG1("[da9214_init] Success to register da9214 i2c driver.\n");

		/* da9214 user space access interface */
/*		ret = platform_device_register(&da9214_user_space_device);
		if (ret) {
			PMICLOG1("****[da9214_init] Unable to device register(%d)\n", ret);
			return ret;
		}
*/
		ret = platform_driver_register(&da9214_user_space_driver);
		if (ret) {
			PMICLOG1("****[da9214_init] Unable to register driver (%d)\n", ret);
			return ret;
		}
	} else {
		pr_notice("[da9214_init] DCT no define EXT BUCK\n");
		g_da9214_hw_exist = 0;
		g_da9214_driver_ready = 1;
	}

#endif

	return 0;
}

static void __exit da9214_exit(void)
{
	i2c_del_driver(&da9214_driver);
}
module_init(da9214_init);
module_exit(da9214_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("I2C da9214 Driver");
MODULE_AUTHOR("Anderson Tsai");
