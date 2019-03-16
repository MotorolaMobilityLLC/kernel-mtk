/*
 * file abov_sar.c
 * brief abov Driver for two channel SAP using
 *
 * Driver for the ABOV
 * Copyright (c) 2015-2016 ABOV Corp
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#define DRIVER_NAME "abov_sar"

//#define DRIVER_NAME "sar"

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/regulator/consumer.h>
#include <linux/notifier.h>
#include <linux/usb.h>
#include <linux/power_supply.h>

#include "../situation/situation.h"

#if defined(CONFIG_FB)
#include <linux/fb.h>
#endif

#include "abov_sar.h"

#include <asm/segment.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <linux/async.h>
#include <linux/firmware.h>
#define SLEEP(x)	mdelay(x)

#define C_I2C_FIFO_SIZE 8
#define I2C_MASK_FLAG	(0x00ff)

static u8 checksum_h;
static u8 checksum_h_bin;
static u8 checksum_l;
static u8 checksum_l_bin;

#define REST_IDLE 0
#define REST_ACTIVE 1
#define ABOV_DEBUG 1
#define LOG_TAG "ABOV "

#if ABOV_DEBUG
#define LOG_INFO(fmt, args...)  pr_info(LOG_TAG fmt, ##args)
#define LOG_DBG(fmt, args...)	pr_info(LOG_TAG fmt, ##args)
#define LOG_ERR(fmt, args...)   pr_err(LOG_TAG fmt, ##args)
#else
#define LOG_INFO(fmt, args...)
#define LOG_DBG(fmt, args...)
#define LOG_ERR(fmt, args...)
#endif

static int mEnabled;
static int fw_dl_status;
static int programming_done;
static bool user_debug = false;
pabovXX_t abov_sar_ptr;

/**
 * fn static int write_register(pabovXX_t this, u8 address, u8 value)
 * brief Sends a write register to the device
 * param this Pointer to main parent struct
 * param address 8-bit register address
 * param value   8-bit register value to write to address
 * return Value from i2c_master_send
 */
static int write_register(pabovXX_t this, u8 address, u8 value)
{
	struct i2c_client *i2c = 0;
	char buffer[2];
	int returnValue = 0;

	buffer[0] = address;
	buffer[1] = value;
	returnValue = -ENOMEM;
	if (this && this->bus) {
		i2c = this->bus;

		returnValue = i2c_master_send(i2c, buffer, 2);
		LOG_INFO("write_register Addr: 0x%x Val: 0x%x Return: %d\n",
				address, value, returnValue);
	}
	if (returnValue < 0) {
		LOG_DBG("Write_register failed!\n");
	}
	return returnValue;
}

/**
 * fn static int read_register(pabovXX_t this, u8 address, u8 *value)
 * brief Reads a register's value from the device
 * param this Pointer to main parent struct
 * param address 8-Bit address to read from
 * param value Pointer to 8-bit value to save register value to
 * return Value from i2c_smbus_read_byte_data if < 0. else 0
 */
static int read_register(pabovXX_t this, u8 address, u8 *value)
{
	struct i2c_client *i2c = 0;
	s32 returnValue = 0;

	if (this && value && this->bus) {
		i2c = this->bus;
		returnValue = i2c_smbus_read_byte_data(i2c, address);
		LOG_INFO("read_register Addr: 0x%x Return: 0x%x\n",
				address, returnValue);
		if (returnValue >= 0) {
			*value = returnValue;
			return 0;
		} else {
			return returnValue;
		}
	}
	LOG_INFO("read_register failed!\n");
	return -ENOMEM;
}

/**
 * detect if abov exist or not
 * return 1 if chip exist else return 0
 */
static int abov_detect(struct i2c_client *client)
{
	s32 returnValue = 0, i;
	u8 address = ABOV_VENDOR_ID_REG;
	u8 value = 0xAB;

	if (client) {
		for (i = 0; i < 3; i++) {
			returnValue = i2c_smbus_read_byte_data(client, address);
			LOG_INFO("abov read_register for %d time Addr: 0x%02x Return: 0x%02x\n",
					i, address, returnValue);
			if (returnValue >= 0) {
				if (value == returnValue) {
					LOG_INFO("abov detect success!\n");
					return 1;
				}
			}
		}

		for (i = 0; i < 3; i++) {
		     if(abov_tk_fw_mode_enter(client) == 0) {
		         LOG_INFO("abov boot detect success!\n");
		         return 2;
			 }
		}
	}
	LOG_INFO("abov detect failed!!!\n");
	return 0;
}

/**
 * brief Perform a manual offset calibration
 * param this Pointer to main parent struct
 * return Value return value from the write register
 */
static int manual_offset_calibration(pabovXX_t this)
{
	s32 returnValue = 0;

	returnValue = write_register(this, ABOV_RECALI_REG, 0x01);
	return returnValue;
}

/**
 * brief sysfs show function for manual calibration which currently just
 * returns register value.
 */
static ssize_t manual_offset_calibration_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u8 reg_value = 0;
	pabovXX_t this = dev_get_drvdata(dev);

	LOG_INFO("Reading IRQSTAT_REG\n");
	read_register(this, ABOV_IRQSTAT_REG_DETAIL, &reg_value);
	return scnprintf(buf, PAGE_SIZE, "0x%x\n", reg_value);
}

/* brief sysfs store function for manual calibration */
static ssize_t manual_offset_calibration_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	pabovXX_t this = dev_get_drvdata(dev);
	unsigned long val;

	if (kstrtoul(buf, 0, &val))
		return -EINVAL;
	if (val) {
		LOG_INFO("Performing manual_offset_calibration()\n");
		manual_offset_calibration(this);
	}
	return count;
}
static DEVICE_ATTR(calibrate, 0644, manual_offset_calibration_show,
		manual_offset_calibration_store);
static struct attribute *abov_attributes[] = {
	&dev_attr_calibrate.attr,
	NULL,
};
static struct attribute_group abov_attr_group = {
	.attrs = abov_attributes,
};

/**
 * fn static int read_regStat(pabovXX_t this)
 * brief Shortcut to read what caused interrupt.
 * details This is to keep the drivers a unified
 * function that will read whatever register(s)
 * provide information on why the interrupt was caused.
 * param this Pointer to main parent struct
 * return If successful, Value of bit(s) that cause interrupt, else 0
 */
static int read_regStat(pabovXX_t this)
{
	u8 data = 0;

	if (this) {
		if (read_register(this, ABOV_IRQSTAT_REG_DETAIL, &data) == 0)
			return (data & 0x00FF);
	}
	return 0;
}

/**
 * brief  Initialize I2C config from platform data
 * param this Pointer to main parent struct
 *
 */
static void hw_init(pabovXX_t this)
{
	pabov_platform_data_t pdata = 0;
	int i = 0;
	/* configure device */
	LOG_INFO("Going to Setup I2C Registers\n");
	pdata = this->board;
	if (this && this->board && pdata) {
		while (i < pdata->i2c_reg_num) {
			/* Write all registers/values contained in i2c_reg */
			LOG_INFO("Going to Write Reg: 0x%x Value: 0x%x\n",
					pdata->pi2c_reg[i].reg, pdata->pi2c_reg[i].val);
			write_register(this, pdata->pi2c_reg[i].reg,
					pdata->pi2c_reg[i].val);
			i++;
		}
	} else {
		LOG_ERR("ERROR! platform data 0x%p\n", this->board);
	}
}

/**
 * fn static int initialize(pabovXX_t this)
 * brief Performs all initialization needed to configure the device
 * param this Pointer to main parent struct
 * return Last used command's return value (negative if error)
 */
static int initialize(pabovXX_t this)
{
	int ret;

	if (this) {
		/* prepare reset by disabling any irq handling */
		this->irq_disabled = 1;
		disable_irq(this->irq);
		/* perform a reset */
		ret = write_register(this, ABOV_SOFTRESET_REG,
				0x10);
		if (ret < 0)
			goto error_exit;
		/* wait until the reset has finished by monitoring NIRQ */
		LOG_INFO("Software Reset. Waiting device back to continue.\n");
		/* just sleep for awhile instead of using a loop with reading irq status */
		msleep(300);
		/*
		 *    while(this->get_nirq_low && this->get_nirq_low()) { read_regStat(this); }
		 */
		LOG_INFO("Device back from the reset, continuing. NIRQ = %d\n",
				this->get_nirq_low(this->board->irq_gpio));
		hw_init(this);
		msleep(100); /* make sure everything is running */
		ret = manual_offset_calibration(this);
		if (ret < 0)
			goto error_exit;
		/* re-enable interrupt handling */
		enable_irq(this->irq);
		this->irq_disabled = 0;

		/* make sure no interrupts are pending since enabling irq will only
		 * work on next falling edge */
		read_regStat(this);
		LOG_INFO("Exiting initialize(). NIRQ = %d\n",
				this->get_nirq_low(this->board->irq_gpio));
		programming_done = REST_ACTIVE;
		return 0;
	}
	return -ENOMEM;

error_exit:
	programming_done = REST_ACTIVE;
	return ret;
}

/**
 * brief Handle what to do when a touch occurs
 * param this Pointer to main parent struct
 */
static void touchProcess(pabovXX_t this)
{
	u8 i = 0;
	int retry = 0;
	struct abov_platform_data *board;
	struct channel_info *ci;

	board = this->board;
	if (this) {
		LOG_INFO("Inside touchProcess()\n");
		read_register(this, ABOV_IRQSTAT_REG_DETAIL, &i);
		for(retry = 0 ; retry < SAR_CHANNEL_COUNT ; retry++){
			ci = &(this->channelInfor[retry]);
			switch (ci->state) {
				case IDLE:
					if ((i & ci->mask) == ci->mask) {
						ci->state = S_BODY;
						abovXX_sar_data_report(ci->state,ci->channel_id);
						LOG_INFO("CS %d State = BODY.\n", ci->channel_id);
					} else {
						if ((i & ci->mask) == (ci->mask & 0x05)) {
							ci->state = S_PROX;
							abovXX_sar_data_report(ci->state,ci->channel_id);
							LOG_INFO("CS %d State = PROX.\n", ci->channel_id);
						} else {
							LOG_INFO("CS %d still in IDLE State.\n", ci->channel_id);
						}
					}
					break;
				case S_PROX:
					if ((i & ci->mask) == ci->mask) {
						ci->state = S_BODY;
						abovXX_sar_data_report(ci->state,ci->channel_id);
						LOG_INFO("CS %d State = BODY.\n", ci->channel_id);
					} else {
						if ((i & ci->mask) == (ci->mask & 0x05)) {
							LOG_INFO("CS %d still in PROX State.\n", ci->channel_id);
						} else {
							ci->state = IDLE;
							abovXX_sar_data_report(ci->state,ci->channel_id);
							LOG_INFO("CS %d State = IDLE.\n", ci->channel_id);
						}
					}
					break;
				case S_BODY:
					if ((i & ci->mask) == ci->mask) {
						LOG_INFO("CS %d still in BODY State.\n", ci->channel_id);
					} else {
						if ((i & ci->mask) == (ci->mask & 0x05)) {
							ci->state = S_PROX;
							abovXX_sar_data_report(ci->state,ci->channel_id);
							LOG_INFO("CS %d still in PROX State.\n", ci->channel_id);
						} else {
							ci->state = IDLE;
							abovXX_sar_data_report(ci->state,ci->channel_id);
							LOG_INFO("CS %d State = IDLE.\n", ci->channel_id);
						}
					}
					break;
				default:
					LOG_ERR("CS %d State = unknown\n", retry);
					break;
			}
		}
		LOG_INFO("Leaving touchProcess()\n");
	}
}

static int abov_get_nirq_state(unsigned irq_gpio)
{
	if (irq_gpio) {
		return !gpio_get_value(irq_gpio);
	} else {
		LOG_ERR("abov irq_gpio is not set.");
		return -EINVAL;
	}
}

/**
 *fn static void abov_reg_setup_init(struct i2c_client *client)
 *brief read reg val form dts
 *      reg_array_len for regs needed change num
 *      data_array_val's format <reg val ...>
 */
static void abov_reg_setup_init(struct i2c_client *client)
{
	u32 data_array_len = 0;
	u32 *data_array;
	int ret, i, j;
	struct device_node *np = client->dev.of_node;

	ret = of_property_read_u32(np, "reg_array_len", &data_array_len);
	if (ret < 0) {
		LOG_ERR("data_array_len read error");
		return;
	}
	data_array = kmalloc(data_array_len * 2 * sizeof(u32), GFP_KERNEL);
	ret = of_property_read_u32_array(np, "reg_array_val",
			data_array,
			data_array_len*2);
	if (ret < 0) {
		LOG_ERR("data_array_val read error");
		return;
	}
	for (i = 0; i < ARRAY_SIZE(abov_i2c_reg_setup); i++) {
		for (j = 0; j < data_array_len*2; j += 2) {
			if (data_array[j] == abov_i2c_reg_setup[i].reg) {
				abov_i2c_reg_setup[i].val = data_array[j+1];
				LOG_INFO("read dtsi 0x%02x:0x%02x set reg\n",
					data_array[j], data_array[j+1]);
			}
		}
	}
	kfree(data_array);
}

static void abov_platform_data_of_init(struct i2c_client *client,
		pabov_platform_data_t pplatData)
{
	struct device_node *np = client->dev.of_node;
	u32 cap_channel_top, cap_channel_bottom;
	int ret;
	struct device_node *device_node;

	device_node = of_find_compatible_node(NULL, NULL, "mediatek,sar_irq");
	if (device_node) {
		client->irq = irq_of_parse_and_map(device_node, 0);
		LOG_INFO("func = %s, irq = %d\n", __func__, client->irq);
	} else {
		LOG_ERR("func = %s, can't find compatible node.\n", __func__);
	}
	pplatData->irq_gpio = client->irq;
	ret = of_property_read_u32(np, "cap,use_channel_top", &cap_channel_top);

	ret = of_property_read_u32(np, "cap,use_channel_bottom", &cap_channel_bottom);
	pplatData->cap_channel_top = (int)cap_channel_top;
	pplatData->cap_channel_bottom = (int)cap_channel_bottom;

	pplatData->get_is_nirq_low = abov_get_nirq_state;
	pplatData->init_platform_hw = NULL;
	/*  pointer to an exit function. Here in case needed in the future */
	/*
	 *.exit_platform_hw = abov_exit_ts,
	 */
	pplatData->exit_platform_hw = NULL;
	abov_reg_setup_init(client);
	pplatData->pi2c_reg = abov_i2c_reg_setup;
	pplatData->i2c_reg_num = ARRAY_SIZE(abov_i2c_reg_setup);

	ret = of_property_read_string(np, "label", &pplatData->fw_name);
	if (ret < 0) {
		LOG_ERR("firmware name read error!\n");
		return;
	}
	LOG_INFO("fw_name = %s\n", pplatData->fw_name);
}

static ssize_t capsense_reset_show(struct class *class,
		struct class_attribute *attr,
		char *buf)
{
	return snprintf(buf, 8, "%d\n", programming_done);
}

static ssize_t capsense_reset_store(struct class *class,
		struct class_attribute *attr,
		const char *buf, size_t count)
{
	pabovXX_t this = abov_sar_ptr;

	if (!count || (this == NULL))
		return -EINVAL;

	if (!strncmp(buf, "reset", 5) || !strncmp(buf, "1", 1))
		write_register(this, ABOV_SOFTRESET_REG, 0x10);

	this->channelInfor[0].state = IDLE;
	this->channelInfor[1].state = IDLE;
	abovXX_sar_data_report(IDLE,ID_SAR_TOP);
	abovXX_sar_data_report(IDLE,ID_SAR_BOTTOM);

	return count;
}

static CLASS_ATTR(reset, 0660, capsense_reset_show, capsense_reset_store);

static ssize_t capsense_enable_show(struct class *class,
		struct class_attribute *attr,
		char *buf)
{
	return snprintf(buf, 8, "%d\n", mEnabled);
}

static ssize_t capsense_enable_store(struct class *class,
		struct class_attribute *attr,
		const char *buf, size_t count)
{
	pabovXX_t this = abov_sar_ptr;

	if (!count || (this == NULL))
		return -EINVAL;

	if (!strncmp(buf, "1", 1)) {
		LOG_DBG("enable cap sensor\n");
		initialize(this);

		this->channelInfor[0].state = IDLE;
		this->channelInfor[1].state = IDLE;
		abovXX_sar_data_report(IDLE,ID_SAR_TOP);
		abovXX_sar_data_report(IDLE,ID_SAR_BOTTOM);
		if(!mEnabled) mEnabled = 1;
	} else if (!strncmp(buf, "0", 1)) {
		LOG_DBG("disable cap sensor\n");
		write_register(this, ABOV_CTRL_MODE_REG, 0x02);

		this->channelInfor[0].state = DISABLE;
		this->channelInfor[1].state = DISABLE;
		abovXX_sar_data_report(DISABLE,ID_SAR_TOP);
		abovXX_sar_data_report(DISABLE,ID_SAR_BOTTOM);
		mEnabled = 0;
	} else {
		LOG_DBG("unknown enable symbol\n");
	}

	return count;
}

static CLASS_ATTR(enable, 0660, capsense_enable_show, capsense_enable_store);

static ssize_t reg_dump_show(struct class *class,
		struct class_attribute *attr,
		char *buf)
{
	if(!user_debug) {
		u8 reg_value = 0, i;
		pabovXX_t this = abov_sar_ptr;
		if(this->read_flag){
			this->read_flag = 0;
			for(i = 0 ; i < this->read_len ; i++){
				read_register(this,(this->read_reg+i),&reg_value);
				buf[i] = reg_value;
				LOG_INFO("%s : buf[%d] = 0x%x\n",__func__,i,buf[i]);
			}
			return i;
		}
		return -1;
	} else {
		u8 reg_value = 0, i;
		u16 diff_value_ch0 = 0,diff_value_ch1 = 0,cap_value_ch0 = 0,cap_value_ch1 = 0,cap_value_ref = 0;
		pabovXX_t this = abov_sar_ptr;
		char *p = buf;

		if (this->read_flag) {
			this->read_flag = 0;
			read_register(this, this->read_reg, &reg_value);
			p += snprintf(p, PAGE_SIZE, "(0x%02x)=0x%02x\n", this->read_reg,reg_value);
			return (p-buf);
		}

		for (i = 0; i < 0x26; i++) {
			read_register(this, i, &reg_value);
			p += snprintf(p, PAGE_SIZE, "(0x%02x)=0x%02x\n",
				i, reg_value);
		}

		for (i = 0x80; i < 0x8C; i++) {
			read_register(this, i, &reg_value);
			p += snprintf(p, PAGE_SIZE, "(0x%02x)=0x%02x\n",
				i, reg_value);
		}

		/* diff value ch0*/
		read_register(this, 0x1C, &reg_value);
		diff_value_ch0 = reg_value;
		diff_value_ch0 <<= 8;
		read_register(this, 0x1D, &reg_value);
		diff_value_ch0 += reg_value;

		/* diff value ch1 */
		read_register(this, 0x1E, &reg_value);
		diff_value_ch1 = reg_value;
		diff_value_ch1 <<= 8;
		read_register(this, 0x1F, &reg_value);
		diff_value_ch1 += reg_value;

		/* cap value ch0 */
		read_register(this, 0x20, &reg_value);
		cap_value_ch0 = reg_value;
		cap_value_ch0 <<= 8;
		read_register(this, 0x21, &reg_value);
		cap_value_ch0 += reg_value;

		/* cap value ch1 */
		read_register(this, 0x22, &reg_value);
		cap_value_ch1 = reg_value;
		cap_value_ch1 <<= 8;
		read_register(this, 0x23, &reg_value);
		cap_value_ch1 += reg_value;

		/* cap value ch1 */
		read_register(this, 0x24, &reg_value);
		cap_value_ref = reg_value;
		cap_value_ref <<= 8;
		read_register(this, 0x25, &reg_value);
		cap_value_ref += reg_value;

		p += snprintf(p, PAGE_SIZE, "diff_value_ch0=%d\n",
			diff_value_ch0);
		p += snprintf(p, PAGE_SIZE, "diff_value_ch1=%d\n",
			diff_value_ch1);
		p += snprintf(p, PAGE_SIZE, "cap_value_ch0=%d\n",
			cap_value_ch0);
		p += snprintf(p, PAGE_SIZE, "cap_value_ch1=%d\n",
			cap_value_ch1);
		p += snprintf(p, PAGE_SIZE, "cap_value_ref=%d\n",
			cap_value_ref);

		return (p-buf);
	}
}

static ssize_t reg_dump_store(struct class *class,
		struct class_attribute *attr,
		const char *buf, size_t count)
{
	pabovXX_t this = abov_sar_ptr;

	if(!user_debug) {
		u8 regaddr,val;
		int i = 0;

		if( count != 3){
			LOG_ERR("%s :params error[ count == %d !=2]\n",__func__,count);
			return -1;
		}
		for(i = 0 ; i < count ; i++)
			LOG_INFO("%s : buf[%d] = 0x%x\n",__func__,i,buf[i]);
		if(buf[2] == 0){
			regaddr = buf[0];
			val= buf[1];
			write_register(this,regaddr,val);
	} else if(buf[2] == 1) {
			this->read_reg = buf[0];
			this->read_len = buf[1];
			this->read_flag = 1;
		}
	} else {
		unsigned int val, reg, opt;
		if (sscanf(buf, "%x,%x,%x", &reg, &val, &opt) == 3) {
			LOG_INFO("%s, read reg = 0x%02x\n", __func__, *(u8 *)&reg);
			this->read_reg = *((u8 *)&reg);
			this->read_flag = 1;
		} else if (sscanf(buf, "%x,%x", &reg, &val) == 2) {
			LOG_INFO("%s,reg = 0x%02x, val = 0x%02x\n",
				__func__, *(u8 *)&reg, *(u8 *)&val);
			write_register(this, *((u8 *)&reg), *((u8 *)&val));
		}
	}
	return count;
}

static CLASS_ATTR(reg, 0660, reg_dump_show, reg_dump_store);

static struct class capsense_class = {
	.name			= "capsense",
	.owner			= THIS_MODULE,
};

static void ps_notify_callback_work(struct work_struct *work)
{
	pabovXX_t this = container_of(work, abovXX_t, ps_notify_work);
	int ret = 0;

	LOG_INFO("Usb insert,going to force calibrate\n");
	ret = write_register(this, ABOV_RECALI_REG, 0x01);
	if (ret < 0)
		LOG_ERR(" Usb insert,calibrate cap sensor failed\n");

	this->channelInfor[0].state = IDLE;
	this->channelInfor[1].state = IDLE;
	abovXX_sar_data_report(IDLE,ID_SAR_BOTTOM);
	abovXX_sar_data_report(IDLE,ID_SAR_TOP);
}

static int ps_get_state(struct power_supply *psy, bool *present)
{
	union power_supply_propval pval = { 0 };
	int retval;

	retval = power_supply_get_property(psy, POWER_SUPPLY_PROP_ONLINE,/*POWER_SUPPLY_PROP_PRESENT,*/
			&pval);
	if (retval) {
		LOG_ERR("%s psy get property failed\n", psy->desc->name);
		return retval;
	}
	*present = (pval.intval) ? true : false;
	LOG_INFO("%s is %s\n", psy->desc->name,
			(*present) ? "present" : "not present");
	return 0;
}

static int ps_notify_callback(struct notifier_block *self,
		unsigned long event, void *p)
{
	pabovXX_t this = container_of(self, abovXX_t, ps_notif);
	struct power_supply *psy = p;
	bool present;
	int retval;

	if(!psy)
		return -ENODEV;
	if(!psy->desc)
		return -ENODEV;
	if ((event == PSY_EVENT_PROP_CHANGED)
			&& psy && psy->desc->get_property && psy->desc->name &&
			!strncmp(psy->desc->name, "usb", sizeof("usb"))) {
		LOG_INFO("ps notification: event = %lu\n", event);
		retval = ps_get_state(psy, &present);
		if (retval) {
			LOG_ERR("psy get property failed\n");
			return retval;
		}

		if (event == PSY_EVENT_PROP_CHANGED) {
			if (this->ps_is_present == present) {
				LOG_INFO("ps present state not change\n");
				return 0;
			}
		}
		this->ps_is_present = present;
		schedule_work(&this->ps_notify_work);
	}

	return 0;
}

static int _i2c_adapter_block_write(struct i2c_client *client, u8 *data, u8 len)
{
	u8 buffer[C_I2C_FIFO_SIZE];
	u8 left = len;
	u8 offset = 0;
	u8 retry = 0;

	struct i2c_msg msg = {
		.addr = client->addr & I2C_MASK_FLAG,
		.flags = 0,
		.buf = buffer,
	};

	if (data == NULL || len < 1) {
		LOG_ERR("Invalid : data is null or len=%d\n", len);
		return -EINVAL;
	}

	while (left > 0) {
		retry = 0;
		if (left >= C_I2C_FIFO_SIZE) {
			msg.buf = &data[offset];
			msg.len = C_I2C_FIFO_SIZE;
			left -= C_I2C_FIFO_SIZE;
			offset += C_I2C_FIFO_SIZE;
		} else {
			msg.buf = &data[offset];
			msg.len = left;
			left = 0;
		}

		while (i2c_transfer(client->adapter, &msg, 1) != 1) {
			retry++;
			if (retry > 10) {
			    LOG_ERR("OUT : fail - addr:%#x len:%d \n", client->addr, msg.len);
				return -EIO;
			}
		}
	}
	return 0;
}

static int abov_tk_check_busy(struct i2c_client *client)
{
	int ret, count = 0;
	unsigned char val = 0x00;

	do {
		ret = i2c_master_recv(client, &val, sizeof(val));
		if (val & 0x01) {
			count++;
			if (count > 1000) {
				LOG_INFO("%s: val = 0x%x\r\n", __func__, val);
				return ret;
			}
		} else {
			break;
		}
	} while (1);

	return ret;
}

static int abov_tk_fw_write(struct i2c_client *client, unsigned char *addrH,
						unsigned char *addrL, unsigned char *val)
{
	int length = 36, ret = 0;
	unsigned char buf[40] = {0, };

	buf[0] = 0xAC;
	buf[1] = 0x7A;
	memcpy(&buf[2], addrH, 1);
	memcpy(&buf[3], addrL, 1);
	memcpy(&buf[4], val, 32);
	ret = _i2c_adapter_block_write(client, buf, length);
	if (ret < 0) {
		LOG_ERR("Firmware write fail ...\n");
		return ret;
	}

	SLEEP(3);
	abov_tk_check_busy(client);

	return 0;
}

static int abov_tk_reset_for_bootmode(struct i2c_client *client)
{
	int ret, retry_count = 10;
	unsigned char buf[16] = {0, };

retry:
	buf[0] = 0xF0;
	buf[1] = 0xAA;
	ret = i2c_master_send(client, buf, 2);
	if (ret < 0) {
		LOG_ERR("write fail(retry:%d)\n", retry_count);
		if (retry_count-- > 0) {
			goto retry;
		}
		return -EIO;
	} else {
		LOG_INFO("success reset & boot mode\n");
		return 0;
	}
}

static int abov_tk_fw_mode_enter(struct i2c_client *client)
{
	int ret = 0;
	unsigned char buf[40] = {0, };

	buf[0] = 0xAC;
	buf[1] = 0x5B;
	ret = i2c_master_send(client, buf, 2);
	if (ret != 2) {
		LOG_ERR("SEND : fail - addr:%#x data:%#x %#x... ret:%d\n", client->addr, buf[0], buf[1], ret);
		return -EIO;
	}
	LOG_INFO("SEND : succ - addr:%#x data:%#x %#x... ret:%d\n", client->addr, buf[0], buf[1], ret);
	SLEEP(5);

	ret = i2c_master_recv(client, buf, 1);
	if (ret < 0) {
		LOG_ERR("Enter fw downloader mode fail ...\n");
		return -EIO;
	}

	LOG_INFO("succ ... device id=0x%#x\n", buf[0]);

	return 0;
}

static int abov_tk_fw_mode_exit(struct i2c_client *client)
{
	int ret = 0;
	unsigned char buf[40] = {0, };

	buf[0] = 0xAC;
	buf[1] = 0xE1;
	ret = i2c_master_send(client, buf, 2);
	if (ret != 2) {
		LOG_ERR("SEND : fail - addr:%#x data:%#x %#x ... ret:%d\n", client->addr, buf[0], buf[1], ret);
		return -EIO;
	}
	LOG_INFO("SEND : succ - addr:%#x data:%#x %#x ... ret:%d\n", client->addr, buf[0], buf[1], ret);

	return 0;
}

static int abov_tk_flash_erase(struct i2c_client *client)
{
	int ret = 0;
	unsigned char buf[16] = {0, };

	buf[0] = 0xAC;
	buf[1] = 0x2E;


	ret = i2c_master_send(client, buf, 2);
	if (ret != 2) {
		LOG_ERR("SEND : fail - addr:%#x data:%#x %#x ... ret:%d\n", client->addr, buf[0], buf[1], ret);
		return -EIO;
	}

	LOG_INFO("SEND : succ - addr:%#x data:%#x %#x ... ret:%d\n", client->addr, buf[0], buf[1], ret);

	return 0;
}

static int abov_tk_i2c_read_checksum(struct i2c_client *client)
{
	unsigned char checksum[6] = {0, };
	unsigned char buf[16] = {0, };
	int ret;

	checksum_h = 0;
	checksum_l = 0;

	buf[0] = 0xAC;
	buf[1] = 0x9E;
	buf[2] = 0x00;
	buf[3] = 0x00;
	buf[4] = checksum_h_bin;
	buf[5] = checksum_l_bin;
	ret = i2c_master_send(client, buf, 6);

	if (ret != 6) {
		LOG_ERR("SEND : fail - addr:%#x len:%d... ret:%d\n", client->addr, 6, ret);
		return -EIO;
	}
	SLEEP(5);

	buf[0] = 0x00;
	ret = i2c_master_send(client, buf, 1);
	if (ret != 1) {
		LOG_ERR("SEND : fail - addr:%#x data:%#x ... ret:%d\n", client->addr, buf[0], ret);
		return -EIO;
	}
	SLEEP(5);

	ret = i2c_master_recv(client, checksum, 6);
	if (ret < 0) {
		LOG_ERR("Read raw fail ... \n");
		return -EIO;
	}

	checksum_h = checksum[4];
	checksum_l = checksum[5];

	return 0;
}

static int _abov_fw_update(struct i2c_client *client, const u8 *image, u32 size)
{
	int ret, ii = 0;
	int count;
	unsigned short address;
	unsigned char addrH, addrL;
	unsigned char data[32] = {0, };

	LOG_INFO("%s: call in\r\n", __func__);

	if (abov_tk_reset_for_bootmode(client) < 0) {
		LOG_ERR("don't reset(enter boot mode)!");
		return -EIO;
	}

	SLEEP(45);

	for (ii = 0; ii < 10; ii++) {
		if (abov_tk_fw_mode_enter(client) < 0) {
			LOG_ERR("don't enter the download mode! %d", ii);
			SLEEP(40);
			continue;
		}
		break;
	}

	if (10 <= ii) {
		return -EAGAIN;
	}

	if (abov_tk_flash_erase(client) < 0) {
		LOG_ERR("don't erase flash data!");
		return -EIO;
	}

	SLEEP(1400);

	address = 0x800;
	count = size / 32;

	for (ii = 0; ii < count; ii++) {
		/* first 32byte is header */
		addrH = (unsigned char)((address >> 8) & 0xFF);
		addrL = (unsigned char)(address & 0xFF);
		memcpy(data, &image[ii * 32], 32);
		ret = abov_tk_fw_write(client, &addrH, &addrL, data);
		if (ret < 0) {
			LOG_ERR("fw_write.. ii = 0x%x err\r\n", ii);
			return ret;
		}

		address += 0x20;
		memset(data, 0, 32);
	}

	ret = abov_tk_i2c_read_checksum(client);
	ret = abov_tk_fw_mode_exit(client);
	if ((checksum_h == checksum_h_bin) && (checksum_l == checksum_l_bin)) {
		LOG_INFO("checksum successful. checksum_h:0x%02x=0x%02x && checksum_l:0x%02x=0x%02x\n",
			checksum_h, checksum_h_bin, checksum_l, checksum_l_bin);
	} else {
		LOG_INFO("checksum error. checksum_h:0x%02x!=0x%02x && checksum_l:0x%02x!=0x%02x\n",
			checksum_h, checksum_h_bin, checksum_l, checksum_l_bin);
		ret = -1;
	}
	SLEEP(100);

	return ret;
}

static int abov_fw_update(bool force)
{
	int update_loop;
	pabovXX_t this = abov_sar_ptr;
	struct i2c_client *client = this->bus;
	int rc = 0;
	bool fw_upgrade = false;
	u8 fw_version = 0, fw_file_version = 0;
	u8 fw_modelno = 0, fw_file_modeno = 0;
	const struct firmware *fw = NULL;
	char fw_name[32] = {0};

	strlcpy(fw_name, this->board->fw_name, NAME_MAX);
	strlcat(fw_name, ".BIN", NAME_MAX);
	rc = request_firmware(&fw, fw_name, this->pdev);
	if (rc < 0) {
		LOG_ERR("Request firmware failed - %s (%d)\n",
				this->board->fw_name, rc);
		return rc;
	}

	if (force == false) {
		read_register(this, ABOV_VERSION_REG, &fw_version);
		read_register(this, ABOV_MODELNO_REG, &fw_modelno);
	}

	fw_file_modeno = fw->data[1];
	fw_file_version = fw->data[5];
	checksum_h_bin = fw->data[8];
	checksum_l_bin = fw->data[9];

	if ((force) || (fw_version < fw_file_version) || (fw_modelno != fw_file_modeno))
		fw_upgrade = true;
	else {
		LOG_INFO("Exiting fw upgrade...\n");
		fw_upgrade = false;
		fw_dl_status = 0;
		rc = -EIO;
		goto rel_fw;
	}

	if (fw_upgrade) {
		fw_dl_status = 2;
		for (update_loop = 0; update_loop < 10; update_loop++) {
			rc = _abov_fw_update(client, &fw->data[32], fw->size-32);
			if (rc < 0)
				LOG_ERR("retry : %d times!\n", update_loop);
			else {
				initialize(this);
				break;
			}
			SLEEP(400);
		}
		if (update_loop >= 10){
			fw_dl_status = 1;
			rc = -EIO;
		}
		fw_dl_status = 0;
	}

rel_fw:
	release_firmware(fw);
	return rc;
}

static ssize_t capsense_fw_ver_show(struct class *class,
		struct class_attribute *attr,
		char *buf)
{
	u8 fw_version = 0;
	pabovXX_t this = abov_sar_ptr;

	read_register(this, ABOV_VERSION_REG, &fw_version);

	return snprintf(buf, 16, "ABOV:0x%x\n", fw_version);
}

static ssize_t capsense_update_fw_store(struct class *class,
		struct class_attribute *attr,
		const char *buf, size_t count)
{
	pabovXX_t this = abov_sar_ptr;
	unsigned long val;
	int rc;

	if (count > 2)
		return -EINVAL;

	rc = kstrtoul(buf, 10, &val);
	if (rc != 0)
		return rc;

	this->irq_disabled = 1;
	disable_irq(this->irq);
#ifdef USE_THREADED_IRQ
	mutex_lock(&this->mutex);
#else
    spin_lock(&this->lock);
#endif

	if (!this->loading_fw  && val) {
		this->loading_fw = true;
		abov_fw_update(false);
		this->loading_fw = false;
	}
#ifdef USE_THREADED_IRQ
	mutex_unlock(&this->mutex);
#else
	spin_unlock(&this->lock);
#endif

	enable_irq(this->irq);
	this->irq_disabled = 0;

	return count;
}
static CLASS_ATTR(update_fw, 0660, capsense_fw_ver_show, capsense_update_fw_store);

static ssize_t capsense_force_update_fw_store(struct class *class,
		struct class_attribute *attr,
		const char *buf, size_t count)
{
	pabovXX_t this = abov_sar_ptr;
	unsigned long val;
	int rc;

	if (count > 2)
		return -EINVAL;

	rc = kstrtoul(buf, 10, &val);
	if (rc != 0)
		return rc;

	this->irq_disabled = 1;
	disable_irq(this->irq);

#ifdef USE_THREADED_IRQ
	mutex_lock(&this->mutex);
#else
	spin_lock(&this->lock);
#endif

	if (!this->loading_fw  && val) {
		this->loading_fw = true;
		abov_fw_update(true);
		this->loading_fw = false;
	}
#ifdef USE_THREADED_IRQ
	mutex_unlock(&this->mutex);
#else
	spin_unlock(&this->lock);
#endif

	enable_irq(this->irq);
	this->irq_disabled = 0;

	return count;
}
static CLASS_ATTR(force_update_fw, 0660, capsense_fw_ver_show, capsense_force_update_fw_store);

static ssize_t capsense_fw_download_status_show(struct class *class,
		struct class_attribute *attr,
		char *buf)
{
	return snprintf(buf,8,"%d",fw_dl_status);
}
static CLASS_ATTR(fw_download_status, 0660, capsense_fw_download_status_show, NULL);

static ssize_t capsense_user_debug_status_store(struct class *class,
		struct class_attribute *attr,
		const char *buf, size_t count)
{
	if (!count )
		return -EINVAL;

	if (!strncmp(buf, "1", 1)) {
		LOG_DBG("enable cap user debug\n");
		user_debug = true;
	} else if (!strncmp(buf, "0", 1)) {
		LOG_DBG("disable cap user debug\n");
		user_debug = false;
	} else {
		LOG_DBG("unknown enable symbol\n");
	}

	return count;
}

static ssize_t capsense_user_debug_status_show(struct class *class,
		struct class_attribute *attr,
		char *buf)
{
	return snprintf(buf,8,"%d\n",user_debug);
}
static CLASS_ATTR(user_debug_status, 0660, capsense_user_debug_status_show, capsense_user_debug_status_store);

static void capsense_update_work(struct work_struct *work)
{
	pabovXX_t this = container_of(work, abovXX_t, fw_update_work);

	LOG_INFO("%s: start update firmware\n", __func__);
#ifdef USE_THREADED_IRQ
	mutex_lock(&this->mutex);
#else
	spin_lock(&this->lock);
#endif
	this->loading_fw = true;
	abov_fw_update(false);
	this->loading_fw = false;
#ifdef USE_THREADED_IRQ
	mutex_unlock(&this->mutex);
#else
	spin_unlock(&this->lock);
#endif

	LOG_INFO("%s: update firmware end\n", __func__);
}

static void capsense_fore_update_work(struct work_struct *work)
{
	pabovXX_t this = container_of(work, abovXX_t, fw_update_work);

	LOG_INFO("%s: start force update firmware\n", __func__);
#ifdef USE_THREADED_IRQ
	mutex_lock(&this->mutex);
#else
	spin_lock(&this->lock);
#endif
	this->loading_fw = true;
	abov_fw_update(true);
	this->loading_fw = false;
#ifdef USE_THREADED_IRQ
	mutex_unlock(&this->mutex);
#else
	spin_unlock(&this->lock);
#endif

	LOG_INFO("%s: force update firmware end\n", __func__);
}


/**
 * fn static int abov_probe(struct i2c_client *client, const struct i2c_device_id *id)
 * brief Probe function
 * param client pointer to i2c_client
 * param id pointer to i2c_device_id
 * return Whether probe was successful
 */
static int abov_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	pabovXX_t this = 0;
	pabov_platform_data_t pplatData = 0;
	int ret;
	bool isForceUpdate = false;
	struct power_supply *psy = NULL;

	LOG_INFO("abov_probe() start!\n");
	pplatData = kzalloc(sizeof(pplatData), GFP_KERNEL);
	if (!pplatData) {
		LOG_ERR("platform data is required!\n");
		return -EINVAL;
	}
#if 0
	pplatData->cap_vdd = regulator_get(&client->dev, "cap_vdd");
	if (IS_ERR(pplatData->cap_vdd)) {
		if (PTR_ERR(pplatData->cap_vdd) == -EPROBE_DEFER) {
			ret = PTR_ERR(pplatData->cap_vdd);
			goto err_vdd_defer;
		}
		LOG_ERR("%s: Failed to get regulator\n", __func__);
	} else {
		ret = regulator_enable(pplatData->cap_vdd);

		if (ret) {
			regulator_put(pplatData->cap_vdd);
			LOG_ERR("%s: Error %d enable regulator\n",
					__func__, ret);
			goto err_vdd_defer;
		}
		pplatData->cap_vdd_en = true;
		LOG_INFO("cap_vdd regulator is %s\n",
				regulator_is_enabled(pplatData->cap_vdd) ?
				"on" : "off");
	}
//#if 0

	pplatData->cap_svdd = regulator_get(&client->dev, "cap_svdd");
	if (!IS_ERR(pplatData->cap_svdd)) {
		ret = regulator_enable(pplatData->cap_svdd);
		if (ret) {
			regulator_put(pplatData->cap_svdd);
			LOG_INFO("Failed to enable cap_svdd\n");
			goto err_svdd_error;
		}
		pplatData->cap_svdd_en = true;
		LOG_INFO("cap_svdd regulator is %s\n",
				regulator_is_enabled(pplatData->cap_svdd) ?
				"on" : "off");
	} else {
		ret = PTR_ERR(pplatData->cap_vdd);
		if (ret == -EPROBE_DEFER)
			goto err_svdd_error;
	}
#endif
	/* detect if abov exist or not */
	ret = abov_detect(client);
	if (ret == 0) {
		ret = -ENODEV;
		goto err_this_device;
	} else if (ret == 2) {
		LOG_INFO("abov enter boot mode\n");
		isForceUpdate = true;
	}
	abov_platform_data_of_init(client, pplatData);
	client->dev.platform_data = pplatData;

	if (!i2c_check_functionality(client->adapter,
				I2C_FUNC_SMBUS_READ_WORD_DATA)) {
		ret = -EIO;
		goto err_this_device;
	}

	this = kzalloc(sizeof(abovXX_t), GFP_KERNEL); /* create memory for main struct */
	LOG_INFO("\t Initialized Main Memory: 0x%p\n", this);

	if (this) {
		/* In case we need to reinitialize data
		 * (e.q. if suspend reset device) */
		this->init = initialize;
		/* shortcut to read status of interrupt */
		this->refreshStatus = read_regStat;
		/* pointer to function from platform data to get pendown
		 * (1->NIRQ=0, 0->NIRQ=1) */
		this->get_nirq_low = pplatData->get_is_nirq_low;
		/* save irq in case we need to reference it */
		this->irq = client->irq;
		/* do we need to create an irq timer after interrupt ? */
		this->useIrqTimer = 0;
		this->board = pplatData;
		/* Setup function to call on corresponding reg irq source bit */
		if (MAX_NUM_STATUS_BITS >= 2) {
			this->statusFunc[0] = touchProcess; /* RELEASE_STAT */
			this->statusFunc[1] = touchProcess; /* TOUCH_STAT  */
		}

		/* setup i2c communication */
		this->bus = client;
		i2c_set_clientdata(client, this);

		/* record device struct */
		this->pdev = &client->dev;

		abov_sar_ptr = this;

		/* for accessing items in user data (e.g. calibrate) */
		ret = sysfs_create_group(&client->dev.kobj, &abov_attr_group);

		/* Check if we hava a platform initialization function to call*/
		if (pplatData->init_platform_hw)
			pplatData->init_platform_hw();

		ret = class_register(&capsense_class);
		if (ret < 0) {
			LOG_ERR("Create fsys class failed (%d)\n", ret);
			goto err_class_creat;
		}

		ret = class_create_file(&capsense_class, &class_attr_reset);
		if (ret < 0) {
			LOG_ERR("Create reset file failed (%d)\n", ret);
			goto err_class_creat;
		}

		ret = class_create_file(&capsense_class, &class_attr_enable);
		if (ret < 0) {
			LOG_ERR("Create enable file failed (%d)\n", ret);
			goto err_class_creat;
		}

		ret = class_create_file(&capsense_class, &class_attr_reg);
		if (ret < 0) {
			LOG_ERR("Create reg file failed (%d)\n", ret);
			goto err_class_creat;
		}

		ret = class_create_file(&capsense_class, &class_attr_update_fw);
		if (ret < 0) {
			LOG_ERR("Create update_fw file failed (%d)\n", ret);
			goto err_class_creat;
		}

		ret = class_create_file(&capsense_class, &class_attr_force_update_fw);
		if (ret < 0) {
			LOG_ERR("Create update_fw file failed (%d)\n", ret);
			goto err_class_creat;
		}
		ret = class_create_file(&capsense_class, &class_attr_fw_download_status);
		if (ret < 0) {
			LOG_ERR("Create update_fw file failed (%d)\n", ret);
			goto err_class_creat;
		}

		ret = class_create_file(&capsense_class, &class_attr_user_debug_status);
		if (ret < 0) {
			LOG_ERR("Create user_debug_status failed (%d)\n", ret);
			goto err_class_creat;
		}

		abovXX_sar_init(this);
		sar_misc_init(this);
		write_register(this, ABOV_CTRL_MODE_REG, 0x02);
		mEnabled = 0;

		INIT_WORK(&this->ps_notify_work, ps_notify_callback_work);
		this->ps_notif.notifier_call = ps_notify_callback;
		ret = power_supply_reg_notifier(&this->ps_notif);
		if (ret) {
			LOG_ERR(
				"Unable to register ps_notifier: %d\n", ret);
			goto free_ps_notifier;
		}

		psy = power_supply_get_by_name("usb");
		if (psy) {
			ret = ps_get_state(psy, &this->ps_is_present);
			if (ret) {
				LOG_ERR(
					"psy get property failed rc=%d\n",
					ret);
				goto free_ps_notifier;
			}
		}

		this->loading_fw = false;
		fw_dl_status = 1;
		if (isForceUpdate == true) {
		    INIT_WORK(&this->fw_update_work, capsense_fore_update_work);
		} else {
			INIT_WORK(&this->fw_update_work, capsense_update_work);
		}
		schedule_work(&this->fw_update_work);

        LOG_INFO("abov_probe() SUCCESS\n");
		return  0;
	}
	ret =  -ENOMEM;
	goto err_this_device;

free_ps_notifier:
	LOG_ERR("%s free ps notifier:.\n", __func__);
	power_supply_unreg_notifier(&this->ps_notif);

err_class_creat:
	LOG_ERR("%s unregister capsense class.\n", __func__);
	class_unregister(&capsense_class);

err_this_device:
	LOG_ERR("%s device this defer.\n", __func__);
#if 0
	regulator_disable(pplatData->cap_svdd);
	regulator_put(pplatData->cap_svdd);

err_svdd_error:
	LOG_DBG("%s svdd defer.\n", __func__);
	regulator_disable(pplatData->cap_vdd);
	regulator_put(pplatData->cap_vdd);


err_vdd_defer:
	LOG_ERR("%s free pplatData.\n", __func__);
#endif

	kfree(pplatData);
	return ret;
}

/**
 * fn static int abov_remove(struct i2c_client *client)
 * brief Called when device is to be removed
 * param client Pointer to i2c_client struct
 * return Value from abovXX_sar_remove()
 */
static int abov_remove(struct i2c_client *client)
{
	pabov_platform_data_t pplatData = 0;
	pabovXX_t this = i2c_get_clientdata(client);

	if (this) {
		sysfs_remove_group(&client->dev.kobj, &abov_attr_group);
		pplatData = client->dev.platform_data;
		if (pplatData && pplatData->exit_platform_hw)
			pplatData->exit_platform_hw();
	}
	return abovXX_sar_remove(this);
}

#ifdef CONFIG_PM_SLEEP
/*====================================================*/
/***** Kernel Suspend *****/
static int abov_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	pabovXX_t this = i2c_get_clientdata(client);

	abovXX_suspend(this);
	return 0;
}
/***** Kernel Resume *****/
static int abov_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	pabovXX_t this = i2c_get_clientdata(client);

	abovXX_resume(this);
	return 0;
}
/*====================================================*/
#endif

#ifdef CONFIG_OF
static const struct of_device_id abov_match_tbl[] = {
	{ .compatible = "mediatek,sar" },
	{ },
};
MODULE_DEVICE_TABLE(of, abov_match_tbl);
#endif

static struct i2c_device_id abov_idtable[] = {
	{ DRIVER_NAME, 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, abov_idtable);

static SIMPLE_DEV_PM_OPS(abov_pm_ops, abov_suspend, abov_resume);

static struct i2c_driver abov_driver = {
	.driver = {
		.owner  = THIS_MODULE,
		.of_match_table = abov_match_tbl,
		.name   = DRIVER_NAME,
        // .pm = &abov_pm_ops,
	},
	.id_table = abov_idtable,
	.probe	  = abov_probe,
	.remove	  = abov_remove,
};
/*---------------------add to situation----------------*/
static int sar_open_report_data(int open)
{
	pabovXX_t this = abov_sar_ptr;

	if (this == NULL)
		return -EINVAL;

	if (open == 1) {
		if(mEnabled){
			mEnabled++;
			LOG_DBG("Cap sensor is already enabled\n");
			return 0;
		}
		LOG_DBG("enable cap sensor\n");
		initialize(this);

		this->channelInfor[0].state = IDLE;
		this->channelInfor[1].state = IDLE;
		abovXX_sar_data_report(IDLE,ID_SAR_TOP);
		abovXX_sar_data_report(IDLE,ID_SAR_BOTTOM);
		mEnabled++;
	} else if (open == 0) {
		if(!mEnabled){
			LOG_DBG("Cap sensor is already disenabled\n");
			return 0;
		}
		LOG_DBG("disable cap sensor\n");
		write_register(this, ABOV_CTRL_MODE_REG, 0x02);

		this->channelInfor[0].state = DISABLE;
		this->channelInfor[1].state = DISABLE;
		abovXX_sar_data_report(DISABLE,ID_SAR_TOP);
		abovXX_sar_data_report(DISABLE,ID_SAR_BOTTOM);
		mEnabled--;
	} else {
		LOG_DBG("unknown enable symbol\n");
	}

	return 0;
}
static int sar_batch(int flag,
	int64_t samplingPeriodNs, int64_t maxBatchReportLatencyNs)
{
	return 0;
}
static int sar_flush(void)
{
	return 0;
}

static int sar_get_data(int *probability, int *status)
{
	return 0;
}
static bool sar_init_flag = false;
static int abov_sar_top_local_init(void)
{
	struct situation_control_path ctl_sar_top = {0};
	struct situation_data_path data_sar_top = {0};
	int err = 0;

	pr_debug("%s\n", __func__);
	if(!sar_init_flag){
		i2c_add_driver(&abov_driver);
		sar_init_flag = true;
	}
	ctl_sar_top.open_report_data = sar_open_report_data;
	ctl_sar_top.batch = sar_batch;
	ctl_sar_top.flush = sar_flush;
	ctl_sar_top.is_support_wake_lock = true;
	ctl_sar_top.is_support_batch = false;
	err = situation_register_control_path(&ctl_sar_top, ID_SAR_TOP);
	if (err) {
		pr_err("register sar control path err\n");
		goto exit;
	}
	data_sar_top.get_data = sar_get_data;
	err = situation_register_data_path(&data_sar_top, ID_SAR_TOP);
	if (err) {
		pr_err("register sar data path err\n");
		goto exit;
	}

	return 0;
exit:
	return -1;
}
static int abov_sar_top_local_uninit(void)
{
	return 0;
}
static int abov_sar_bottom_local_init(void)
{
	struct situation_control_path ctl_sar_bottom = {0};
	struct situation_data_path data_sar_bottom = {0};
	int err = 0;

	pr_debug("%s\n", __func__);
	if(!sar_init_flag){
		i2c_add_driver(&abov_driver);
		sar_init_flag = true;
	}

	ctl_sar_bottom.open_report_data = sar_open_report_data;
	ctl_sar_bottom.batch = sar_batch;
	ctl_sar_bottom.flush = sar_flush;
	ctl_sar_bottom.is_support_wake_lock = true;
	ctl_sar_bottom.is_support_batch = false;
	err = situation_register_control_path(&ctl_sar_bottom, ID_SAR_BOTTOM);
	if (err) {
		pr_err("register sar control path err\n");
		goto exit;
	}
	data_sar_bottom.get_data = sar_get_data;
	err = situation_register_data_path(&data_sar_bottom, ID_SAR_BOTTOM);
	if (err) {
		pr_err("register sar data path err\n");
		goto exit;
	}

	return 0;
exit:
	return -1;
}
static int abov_sar_bottom_local_uninit(void)
{
	return 0;
}
static struct situation_init_info abov_sar_top_init_info = {
	.name = "abov_sar_top",
	.init = abov_sar_top_local_init,
	.uninit = abov_sar_top_local_uninit,
};
static struct situation_init_info abov_sar_bottom_init_info = {
	.name = "abov_sar_bottom",
	.init = abov_sar_bottom_local_init,
	.uninit = abov_sar_bottom_local_uninit,
};

/*---------------------add to situation----------------*/
static int __init abov_init(void)
{
	printk("func = %s, line = %d\n", __func__, __LINE__);
	situation_driver_add(&abov_sar_top_init_info, ID_SAR_TOP);
	situation_driver_add(&abov_sar_bottom_init_info, ID_SAR_BOTTOM);
	return 0;
}
static void __exit abov_exit(void)
{
	i2c_del_driver(&abov_driver);
}

module_init(abov_init);
module_exit(abov_exit);

MODULE_AUTHOR("ABOV Corp.");
MODULE_DESCRIPTION("ABOV Capacitive Touch Controller Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");

#ifdef USE_THREADED_IRQ
static void abovXX_process_interrupt(pabovXX_t this, u8 nirqlow)
{
	int status = 0;

	if (!this) {
		pr_err("abovXX_worker_func, NULL abovXX_t\n");
		return;
	}
	/* since we are not in an interrupt don't need to disable irq. */
	status = this->refreshStatus(this);
	LOG_INFO("Worker - Refresh Status %d\n", status);
	this->statusFunc[0](this);
	if (unlikely(this->useIrqTimer && nirqlow)) {
		/* In case we need to send a timer for example on a touchscreen
		 * checking penup, perform this here
		 */
		cancel_delayed_work(&this->dworker);
		schedule_delayed_work(&this->dworker, msecs_to_jiffies(this->irqTimeout));
		LOG_INFO("Schedule Irq timer");
	}
}


static void abovXX_worker_func(struct work_struct *work)
{
	pabovXX_t this = 0;

	if (work) {
		this = container_of(work, abovXX_t, dworker.work);
		if (!this) {
			LOG_ERR("abovXX_worker_func, NULL abovXX_t\n");
			return;
		}
		if ((!this->get_nirq_low) || (!this->get_nirq_low(this->board->irq_gpio))) {
			/* only run if nirq is high */
			abovXX_process_interrupt(this, 0);
		}
	} else {
		LOG_ERR("abovXX_worker_func, NULL work_struct\n");
	}
}
static irqreturn_t abovXX_interrupt_thread(int irq, void *data)
{
	pabovXX_t this = 0;
	this = data;

	mutex_lock(&this->mutex);
	LOG_INFO("%s, irq = %d\n", __func__, irq);
	if ((!this->get_nirq_low) || this->get_nirq_low(this->board->irq_gpio))
		abovXX_process_interrupt(this, 1);
	else
		LOG_ERR("%s - nirq read high\n", __func__);
	mutex_unlock(&this->mutex);
	return IRQ_HANDLED;
}
#else
static void abovXX_schedule_work(pabovXX_t this, unsigned long delay)
{
	unsigned long flags;

	if (this) {
		LOG_INFO("abovXX_schedule_work()\n");
		spin_lock_irqsave(&this->lock, flags);
		/* Stop any pending penup queues */
		cancel_delayed_work(&this->dworker);
		/*
		 *after waiting for a delay, this put the job in the kernel-global
		 workqueue. so no need to create new thread in work queue.
		 */
		schedule_delayed_work(&this->dworker, delay);
		spin_unlock_irqrestore(&this->lock, flags);
	} else
		LOG_ERR("abovXX_schedule_work, NULL pabovXX_t\n");
}

static irqreturn_t abovXX_irq(int irq, void *pvoid)
{
	pabovXX_t this = 0;
	if (pvoid) {
		this = (pabovXX_t)pvoid;
		LOG_INFO("abovXX_irq\n");
		if ((!this->get_nirq_low) || this->get_nirq_low(this->board->irq_gpio)) {
			LOG_INFO("abovXX_irq - Schedule Work\n");
			abovXX_schedule_work(this, 0);
		} else
			LOG_INFO("abovXX_irq - nirq read high\n");
	} else
		LOG_INFO("abovXX_irq, NULL pvoid\n");
	return IRQ_HANDLED;
}

static void abovXX_worker_func(struct work_struct *work)
{
	pabovXX_t this = 0;
	int status = 0;
	u8 nirqLow = 0;

	if (work) {
		this = container_of(work, abovXX_t, dworker.work);

		if (!this) {
			LOG_ERR("abovXX_worker_func, NULL abovXX_t\n");
			return;
		}
		if (unlikely(this->useIrqTimer)) {
			if ((!this->get_nirq_low) || this->get_nirq_low(this->board->irq_gpio))
				nirqLow = 1;
		}
		/* since we are not in an interrupt don't need to disable irq. */
		status = this->refreshStatus(this);
		counter = -1;
		LOG_INFO("Worker - Refresh Status %d\n", status);
		this->statusFunc[0](this);

		if (unlikely(this->useIrqTimer && nirqLow)) {
			/* Early models and if RATE=0 for newer models require a penup timer */
			/* Queue up the function again for checking on penup */
			abovXX_schedule_work(this, msecs_to_jiffies(this->irqTimeout));
		}
	} else {
		LOG_ERR("abovXX_worker_func, NULL work_struct\n");
	}
}
#endif

void abovXX_suspend(pabovXX_t this)
{
	if (this) {
		LOG_INFO("ABOV suspend: enter stop mode!\n");
		write_register(this, ABOV_CTRL_MODE_REG, 0x02);
		LOG_INFO("ABOV suspend: disable irq!\n");
		disable_irq(this->irq);
	}
}
void abovXX_resume(pabovXX_t this)
{
	if (this) {
		LOG_INFO("ABOV resume: enter ative mode!\n");
		write_register(this, ABOV_CTRL_MODE_REG, 0x00);
		LOG_INFO("ABOV resume: enable irq!\n");
		enable_irq(this->irq);
	}
}

int abovXX_sar_init(pabovXX_t this)
{
	int err = 0;

	if (this) {
#ifdef USE_THREADED_IRQ

		/* initialize worker function */
		INIT_DELAYED_WORK(&this->dworker, abovXX_worker_func);


		/* initialize mutex */
		mutex_init(&this->mutex);
		/* initailize interrupt reporting */
		this->irq_disabled = 0;
		err = request_threaded_irq(this->irq, NULL, abovXX_interrupt_thread,
				IRQF_TRIGGER_FALLING | IRQF_ONESHOT, this->pdev->driver->name,
				this);
#else
		/* initialize spin lock */
		spin_lock_init(&this->lock);

		/* initialize worker function */
		INIT_DELAYED_WORK(&this->dworker, abovXX_worker_func);

		/* initailize interrupt reporting */
		this->irq_disabled = 0;
		err = request_irq(this->irq, abovXX_irq, IRQF_TRIGGER_FALLING,
				this->pdev->driver->name, this);
#endif
		if (err) {
			LOG_ERR("irq %d busy?\n", this->irq);
			return err;
		}
#ifdef USE_THREADED_IRQ
		LOG_INFO("registered with threaded irq (%d)\n", this->irq);
#else
		LOG_INFO("registered with irq (%d)\n", this->irq);
#endif
		/* call init function pointer (this should initialize all registers */
		if (this->init)
			return this->init(this);
		LOG_DBG("No init function!!!!\n");
	}
	return -ENOMEM;
}

int abovXX_sar_remove(pabovXX_t this)
{
	if (this) {
		cancel_delayed_work_sync(&this->dworker); /* Cancel the Worker Func */
		free_irq(this->irq, this);
		kfree(this);
		return 0;
	}
	return -ENOMEM;
}

int abovXX_sar_data_report(int32_t value,int32_t sar_id)
{
	int err = 0;

	err = moto_sar_data_report(value,sar_id);
	if (err < 0){
		printk("func = %s, fail to report data: %d\n", __func__,value);
	}else{
		printk("func = %s, channel[%d] success to report data: %d\n",__func__, sar_id,value);
	}
	return err;
}

static int sar_open(struct inode *inode, struct file *file)
{
	nonseekable_open(inode, file);
	return 0;
}

static ssize_t sar_read(struct file *file, char __user *buffer,
			  size_t count, loff_t *ppos)
{
	ssize_t read_cnt = 0;

	read_cnt = sensor_event_read(abov_sar_ptr->mdev.minor, file, buffer, count, ppos);

	return read_cnt;
}

static unsigned int sar_poll(struct file *file, poll_table *wait)
{
	return sensor_event_poll(abov_sar_ptr->mdev.minor, file, wait);
}

static const struct file_operations sar_fops = {
	.owner = THIS_MODULE,
	.open = sar_open,
	.read = sar_read,
	.poll = sar_poll,
};

static int sar_misc_init(pabovXX_t this)
{
	int err = 0;
	this->channelInfor[0].channel_id = ID_SAR_BOTTOM;
	this->channelInfor[0].state = IDLE;
	this->channelInfor[0].mask = ABOV_TCHCMPSTAT_TCHSTAT0_FLAG_DETAIL;
	this->channelInfor[1].channel_id = ID_SAR_TOP;
	this->channelInfor[1].state = IDLE;
	this->channelInfor[1].mask = ABOV_TCHCMPSTAT_TCHSTAT1_FLAG_DETAIL;
	this->mdev.minor = ID_SAR;
	this->mdev.name = "sar_misc";
	this->mdev.fops = &sar_fops;
	err = sensor_attr_register(&this->mdev);
	if (err)
		LOG_ERR("unable to register sar misc device!!\n");
	return err;
}
