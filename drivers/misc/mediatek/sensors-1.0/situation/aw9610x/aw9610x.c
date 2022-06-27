/*
* aw9610x.c
*
* Copyright (c) 2022 AWINIC Technology CO., LTD
*
* Author: Bob <renxinghu@awinic.com>
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation; either version 2 of the License, or (at your
* option) any later version.
*/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/firmware.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>
#include <linux/string.h>
#include <linux/jiffies.h>
#include <linux/notifier.h>
#include <linux/usb.h>
#include <linux/power_supply.h>
#include <linux/regulator/consumer.h>
#include "aw9610x_bin_parse.h"
#include "aw9610x.h"
#include "aw9610x_reg.h"
#ifdef AW9610X_SUPPORT_MTK
#include "../situation.h"
#include "sar_factory.h"
#endif
#define AW9610X_I2C_NAME "aw9610x_sar"
#ifdef AW9610X_SUPPORT_MTK
#define AW9610X_DRIVER_VERSION "v1.1.0.1"
#else
#define AW9610X_DRIVER_VERSION "v1.1.0"
#endif

#define CONFIG_AW9610X_MTK_CHARGER

#define AW_READ_CHIPID_RETRIES		(3)
#define AW_I2C_RETRIES			(5)
#define AW9610X_SCAN_DEFAULT_TIME	(10000)
#define CALI_FILE_MAX_SIZE		(128)

#ifdef AW_USB_PLUG_CALI
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,9,0)
//#define USB_POWER_SUPPLY_NAME   "charger"
#define USB_POWER_SUPPLY_NAME   "mtk_charger_type"
#else
#define USB_POWER_SUPPLY_NAME   "usb"
#endif
#endif

#ifdef AW9610X_SUPPORT_MTK
static struct aw9610x *g_aw9610x;
static int32_t sarValue[3] = {0};
#define AW9610X_MEAS_CHX	(0)
#define AW9610X_TAG					"[AW9610X]"
#define AW9610X_FUN(f)				pr_err(AW9610X_TAG"[%s]\n", __FUNCTION__)
#define AW9610X_ERR(fmt, args...)	pr_err(AW9610X_TAG"[ERROR][%s][%d]: "fmt, __FUNCTION__, __LINE__, ##args)
#define AW9610X_LOG(fmt, args...)	pr_err(AW9610X_TAG"[INFO][%s][%d]:  "fmt, __FUNCTION__, __LINE__, ##args)
#define AW9610X_DBG(fmt, args...)	pr_err(AW9610X_TAG"[DEBUG][%s][%d]:  "fmt, __FUNCTION__, __LINE__, ##args)
#endif

static uint32_t attr_buf[] = {
	8, 10,
	9, 100,
	10, 1000,
};

#ifdef AW_HEADSET_PLUG_CALI
#define PLUG_HEADSET   1
static struct aw9610x *h_aw9610x;
#endif

/******************************************************
*
* aw9610x i2c write/read
*
******************************************************/
static int32_t
i2c_write(struct aw9610x *aw9610x, uint16_t reg_addr16, uint32_t reg_data32)
{
	int32_t ret =  -ENOMEM;
	struct i2c_client *i2c = aw9610x->i2c;
	struct i2c_msg msg;
	uint8_t w_buf[6];

	/*reg_addr*/
	w_buf[0] = (u8)(reg_addr16 >> 8);
	w_buf[1] = (u8)(reg_addr16);
	/*data*/
	w_buf[2] = (u8)(reg_data32 >> 24);
	w_buf[3] = (u8)(reg_data32 >> 16);
	w_buf[4] = (u8)(reg_data32 >> 8);
	w_buf[5] = (u8)(reg_data32);

	msg.addr = i2c->addr;
	msg.flags = AW9610X_I2C_WR;
	msg.len = 6;
	/*2 bytes regaddr + 4 bytes data*/
	msg.buf = (unsigned char *)w_buf;

	ret = i2c_transfer(i2c->adapter, &msg, 1);
	if (ret < 0)
		AWLOGE(aw9610x->dev,
			"Write reg is 0x%x,error value = %d", reg_addr16, ret);

	return ret;
}

static int32_t
i2c_read(struct aw9610x *aw9610x, uint16_t reg_addr16, uint32_t *reg_data32)
{
	int32_t ret =  -ENOMEM;
	struct i2c_client *i2c = aw9610x->i2c;
	struct i2c_msg msg[2];
	uint8_t w_buf[2];
	uint8_t buf[4];

	w_buf[0] = (unsigned char)(reg_addr16 >> 8);
	w_buf[1] = (unsigned char)(reg_addr16);
	msg[0].addr = i2c->addr;
	msg[0].flags = AW9610X_I2C_WR;
	msg[0].len = 2;
	msg[0].buf = (unsigned char *)w_buf;

	msg[1].addr = i2c->addr;
	msg[1].flags = AW9610X_I2C_RD;
	msg[1].len = 4;
	msg[1].buf = (unsigned char *)buf;

	ret = i2c_transfer(i2c->adapter, msg, 2);
	if (ret < 0)
		AWLOGE(aw9610x->dev,
			"Read reg is 0x%x,error value = %d", reg_addr16, ret);

	reg_data32[0] = ((u32)buf[3]) | ((u32)buf[2]<<8) |
			((u32)buf[1]<<16) | ((u32)buf[0]<<24);

	return ret;
}

static int32_t aw9610x_i2c_write(struct aw9610x *aw9610x,
				uint16_t reg_addr16, uint32_t reg_data32)
{
	int32_t ret = -1;
	uint8_t cnt = 0;

	while (cnt < AW_I2C_RETRIES) {
		ret = i2c_write(aw9610x, reg_addr16, reg_data32);
		if (ret < 0)
			AWLOGE(aw9610x->dev,
					"write cnt = %d,error = %d", cnt, ret);
		else
			break;

		cnt++;
	}

	return ret;
}

static int32_t aw9610x_i2c_read(struct aw9610x *aw9610x,
				uint16_t reg_addr16, uint32_t *reg_data32)
{
	int32_t ret = -1;
	uint8_t cnt = 0;

	while (cnt < AW_I2C_RETRIES) {
		ret = i2c_read(aw9610x, reg_addr16, reg_data32);
		if (ret < 0)
			AWLOGE(aw9610x->dev,
					"i2c_read cnt=%d error=%d", cnt, ret);
		else
			break;
		cnt++;
	}

	return ret;
}

static int32_t
aw9610x_i2c_write_bits(struct aw9610x *aw9610x, uint16_t reg_addr16,
				uint32_t mask, uint32_t reg_data32)
{
	uint32_t reg_val;

	aw9610x_i2c_read(aw9610x, reg_addr16, &reg_val);
	reg_val &= mask;
	reg_val |= (reg_data32 & (~mask));
	aw9610x_i2c_write(aw9610x, reg_addr16, reg_val);

	return 0;
}

/******************************************************************************
*
* aw9610x i2c sequential write/read --- one first addr with multiple data.
*
******************************************************************************/
static int32_t i2c_write_seq(struct aw9610x *aw9610x)
{
	int32_t ret =  -ENOMEM;
	struct i2c_client *i2c = aw9610x->i2c;
	struct i2c_msg msg;
	uint8_t w_buf[228];
	uint8_t addr_bytes = aw9610x->aw_i2c_package.addr_bytes;
	uint8_t msg_cnt = 0;
	uint8_t data_bytes = aw9610x->aw_i2c_package.data_bytes;
	uint8_t reg_num = aw9610x->aw_i2c_package.reg_num;
	uint8_t *p_reg_data = aw9610x->aw_i2c_package.p_reg_data;
	uint8_t msg_idx = 0;

	for (msg_idx = 0; msg_idx < addr_bytes; msg_idx++) {
		w_buf[msg_idx] = aw9610x->aw_i2c_package.init_addr[msg_idx];
		AWLOGI(aw9610x->dev, "w_buf_addr[%d] = 0x%02x",
						msg_idx, w_buf[msg_idx]);
	}
	msg_cnt = addr_bytes;
	for (msg_idx = 0; msg_idx < data_bytes * reg_num; msg_idx++) {
		w_buf[msg_cnt] = *p_reg_data++;
		msg_cnt++;
	}
	AWLOGD(aw9610x->dev, "%d reg_num = %d", msg_cnt, reg_num);
	p_reg_data = aw9610x->aw_i2c_package.p_reg_data;
	msg.addr = i2c->addr;
	msg.flags = AW9610X_I2C_WR;
	msg.len = msg_cnt;
	msg.buf = (uint8_t *)w_buf;
	ret = i2c_transfer(i2c->adapter, &msg, 1);
	if (ret < 0)
		AWLOGE(aw9610x->dev, "i2c write seq error %d", ret);

	return ret;
}

static int32_t i2c_read_seq(struct aw9610x *aw9610x, uint8_t *reg_data)
{
	int32_t ret =  -ENOMEM;
	struct i2c_client *i2c = aw9610x->i2c;
	struct i2c_msg msg[2];
	uint8_t w_buf[4];
	uint8_t buf[228];
	uint8_t data_bytes = aw9610x->aw_i2c_package.data_bytes;
	uint8_t reg_num = aw9610x->aw_i2c_package.reg_num;
	uint8_t addr_bytes = aw9610x->aw_i2c_package.addr_bytes;
	uint8_t msg_idx = 0;
	uint8_t msg_cnt = 0;

	/*
	* step 1 : according to addr_bytes assemble first_addr.
	* step 2 : initialize msg[0] including first_addr transfer to client.
	* step 3 : wait for client return reg_data.
	*/
	for (msg_idx = 0; msg_idx < addr_bytes; msg_idx++) {
		w_buf[msg_idx] = aw9610x->aw_i2c_package.init_addr[msg_idx];
		AWLOGD(aw9610x->dev, "w_buf_addr[%d] = 0x%02x",
					msg_idx, w_buf[msg_idx]);
	}
	msg[0].addr = i2c->addr;
	msg[0].flags = AW9610X_I2C_WR;
	msg[0].len = msg_idx;
	msg[0].buf = (uint8_t *)w_buf;

	/*
	* recieve client to msg[1].buf.
	*/
	msg_cnt = data_bytes * reg_num;
	msg[1].addr = i2c->addr;
	msg[1].flags = AW9610X_I2C_RD;
	msg[1].len = msg_cnt;
	msg[1].buf = (uint8_t *)buf;

	ret = i2c_transfer(i2c->adapter, msg, 2);
	if (ret < 0) {
		AWLOGE(aw9610x->dev, "i2c write error %d", ret);
		return ret;
	}

	for (msg_idx = 0; msg_idx < msg_cnt; msg_idx++) {
		reg_data[msg_idx] = buf[msg_idx];
		AWLOGD(aw9610x->dev, "buf = 0x%02x", buf[msg_idx]);
	}

	return ret;
}

static void
aw9610x_addrblock_load(struct device *dev, const char *buf)
{
	uint32_t addrbuf[4] = { 0 };
	uint8_t temp_buf[2] = { 0 };
	uint32_t i = 0;
	struct aw9610x *aw9610x = dev_get_drvdata(dev);
	uint8_t addr_bytes = aw9610x->aw_i2c_package.addr_bytes;
	uint8_t reg_num = aw9610x->aw_i2c_package.reg_num;

	for (i = 0; i < addr_bytes; i++) {
		if (reg_num < attr_buf[1]) {
			temp_buf[0] = buf[attr_buf[0] + i * 5];
			temp_buf[1] = buf[attr_buf[0] + i * 5 + 1];
		} else if (reg_num >= attr_buf[1] && reg_num < attr_buf[3]) {
			temp_buf[0] = buf[attr_buf[2] + i * 5];
			temp_buf[1] = buf[attr_buf[2] + i * 5 + 1];
		} else if (reg_num >= attr_buf[3] && reg_num < attr_buf[5]) {
			temp_buf[0] = buf[attr_buf[4] + i * 5];
			temp_buf[1] = buf[attr_buf[4] + i * 5 + 1];
		}
		if (sscanf(temp_buf, "%02x", &addrbuf[i]) == 1)
			aw9610x->aw_i2c_package.init_addr[i] =
							(uint8_t)addrbuf[i];
	}
}

/******************************************************
 *
 *the document of storage_spedata
 *
 ******************************************************/
static int32_t aw9610x_filedata_deal(struct aw9610x *aw9610x)
{
	struct file *fp = NULL;
	mm_segment_t fs;
	int8_t *buf;
	int8_t temp_buf[8] = { 0 };
	uint8_t i = 0;
	uint8_t j = 0;
	int32_t ret;
	uint32_t nv_flag = 0;
	uint8_t cali_file_name[20] = { 0 };

	snprintf(cali_file_name, sizeof(cali_file_name), "aw_cali_%d.bin", aw9610x->sar_num);
	AWLOGI(aw9610x->dev, "cali_file_name : %s", cali_file_name);

	fp = filp_open(cali_file_name, O_RDWR | O_CREAT, 0644);
	if (IS_ERR(fp)) {
		AWLOGE(aw9610x->dev, "open failed!");
		return -EINVAL;
	}

	fs = get_fs();
	set_fs(KERNEL_DS);
	buf = (char *)kzalloc(CALI_FILE_MAX_SIZE, GFP_KERNEL);
	if (!buf) {
		AWLOGE(aw9610x->dev, "malloc failed!");
		filp_close(fp, NULL);
		set_fs(fs);
		return -EINVAL;
	}

	ret = vfs_read(fp, buf, CALI_FILE_MAX_SIZE, &(fp->f_pos));
	if (ret < 0) {
		AWLOGE(aw9610x->dev, "read failed");
		set_fs(fs);
		aw9610x->cali_flag = AW_CALI;
		return ret;
	} else if (ret == 0) {
		AWLOGE(aw9610x->dev, "read len = 0");
		set_fs(fs);
		aw9610x->cali_flag = AW_CALI;
		return ret;
	} else {
		for (i = 0; i < AW_SPE_REG_NUM; i++) {
			for (j = 0; j < AW_SPE_REG_DWORD; j++)
				temp_buf[j] = buf[AW_SPE_REG_DWORD * i + j];

			if (sscanf(temp_buf, "%08x",
					&aw9610x->nvspe_data[i]) == 1)
				AWLOGD(aw9610x->dev,
						"nv_spe_data[%d] = 0x%08x",
						i, aw9610x->nvspe_data[i]);
			}
	}

	set_fs(fs);
	filp_close(fp, NULL);
	kfree(buf);

	/* nvspe_datas come from nv*/
	for (i = 0; i < AW_SPE_REG_NUM; i++) {
		nv_flag |= aw9610x->nvspe_data[i];
		if (nv_flag != 0)
			break;
	}

	if (nv_flag == 0) {
		aw9610x->cali_flag = AW_CALI;
		AWLOGI(aw9610x->dev,
			"the chip need to cali! nv_flag = 0x%08x", nv_flag);
	} else {
		aw9610x->cali_flag = AW_NO_CALI;
		AWLOGI(aw9610x->dev,
			"chip not need to cali! nv_flag = 0x%08x", nv_flag);
	}

	return 0;
}

static int32_t
aw9610x_store_spedata_to_file(struct aw9610x *aw9610x, char *buf)
{
	struct file *fp = NULL;
	loff_t pos = 0;
	mm_segment_t fs;
	uint8_t cali_file_name[20] = { 0 };

	AWLOGD(aw9610x->dev, "buf = %s", buf);

	snprintf(cali_file_name, 20, "aw_cali_%d.bin", aw9610x->sar_num);
	AWLOGI(aw9610x->dev, "cali_file_name : %s", cali_file_name);

	fp = filp_open(cali_file_name, O_RDWR | O_CREAT, 0644);
	if (IS_ERR(fp)) {
		AWLOGE(aw9610x->dev, "open failed!");
		return -EINVAL;
	}

	fs = get_fs();
	set_fs(KERNEL_DS);

	vfs_write(fp, buf, strlen(buf), &pos);

	set_fs(fs);

	AWLOGI(aw9610x->dev, "write successfully!");

	filp_close(fp, NULL);
	return 0;
}

/******************************************************
 *
 *configuration of special reg
 *
 ******************************************************/
static void aw9610x_get_calidata(struct aw9610x *aw9610x)
{
	uint8_t i = 0;
	uint32_t buf_size = 0;
	int32_t ret;
	uint32_t reg_val = 0;
	uint8_t temp_buf[9] = { 0 };
	uint8_t buf[CALI_FILE_MAX_SIZE] = { 0 };

	AWLOGD(aw9610x->dev, "enter");

	/*class 1 special reg*/
	for (i = 0; i < AW_CLA1_SPE_REG_NUM; i++)
		aw9610x_i2c_read(aw9610x,
		REG_AFECFG1_CH0 + i * AW_CL1SPE_CALI_OS, &aw9610x->spedata[i]);

	/*class 2 special reg*/
	for (; i < AW_SPE_REG_NUM; i++)
		aw9610x_i2c_read(aw9610x,
			REG_ATCCR0 + (i - AW_CHANNEL_MAX) *
				AW_CL2SPE_CALI_OS, &aw9610x->spedata[i]);

	for (i = AW_CLA1_SPE_REG_NUM; i < AW_SPE_REG_NUM; i++) {
		ret = aw9610x->spedata[i] & 0x07;
		switch (ret) {
		case AW_CHANNEL0:
			aw9610x_i2c_read(aw9610x, REG_COMP_CH0,
							&reg_val);
			break;
		case AW_CHANNEL1:
			aw9610x_i2c_read(aw9610x, REG_COMP_CH1,
							&reg_val);
			break;
		case AW_CHANNEL2:
			aw9610x_i2c_read(aw9610x, REG_COMP_CH2,
							&reg_val);
			break;
		case AW_CHANNEL3:
			aw9610x_i2c_read(aw9610x, REG_COMP_CH3,
							&reg_val);
			break;
		case AW_CHANNEL4:
			aw9610x_i2c_read(aw9610x, REG_COMP_CH4,
							&reg_val);
			break;
		case AW_CHANNEL5:
			aw9610x_i2c_read(aw9610x, REG_COMP_CH5,
							&reg_val);
			break;
		default:
			return;
		}
		aw9610x->spedata[i] = ((reg_val >> 6) & 0x03fffff0) |
					(aw9610x->spedata[i] & 0xfc00000f);
	}
	/* spedatas come from register*/

	/* write spedatas to nv */
	for (i = 0; i < AW_SPE_REG_NUM; i++) {
		snprintf(temp_buf, sizeof(temp_buf), "%08x",
							aw9610x->spedata[i]);
		memcpy(buf + buf_size, temp_buf, strlen(temp_buf));
		buf_size = strlen(buf);
	}
	ret = aw9610x_store_spedata_to_file(aw9610x, buf);
	if (ret < 0) {
		AWLOGE(aw9610x->dev, "store spedata failed");
		return;
	}

	AWLOGD(aw9610x->dev, "successfully write_spereg_to_file");
}

static void aw9610x_class1_reg(struct aw9610x *aw9610x)
{
	int32_t i = 0;
	uint32_t reg_val;

	AWLOGD(aw9610x->dev, "enter");

	for (i = 0; i < AW_CLA1_SPE_REG_NUM; i++) {
		reg_val = (aw9610x->nvspe_data[i] >> 16) & 0x0000ffff;
		aw9610x_i2c_write_bits(aw9610x, REG_INITPROX0_CH0 +
				i * AW_CL1SPE_DEAL_OS, ~(0xffff), reg_val);
	}
}

static void aw9610x_class2_reg(struct aw9610x *aw9610x)
{
	int32_t i = 0;

	AWLOGD(aw9610x->dev, "enter");

	for (i = AW_CLA1_SPE_REG_NUM; i < AW_SPE_REG_NUM; i++) {
		aw9610x_i2c_write(aw9610x,
			REG_ATCCR0 + (i - AW_CLA1_SPE_REG_NUM) * AW_CL2SPE_DEAL_OS,
			aw9610x->nvspe_data[i]);
	}
}

static void aw9610x_spereg_deal(struct aw9610x *aw9610x)
{
	AWLOGD(aw9610x->dev, "enter!");

	aw9610x_class1_reg(aw9610x);
	aw9610x_class2_reg(aw9610x);
}

static void aw9610x_datablock_load(struct device *dev, const char *buf)
{
	uint32_t i = 0;
	uint8_t reg_data[220] = { 0 };
	uint32_t databuf[220] = { 0 };
	uint8_t temp_buf[2] = { 0 };
	struct aw9610x *aw9610x = dev_get_drvdata(dev);
	uint8_t addr_bytes = aw9610x->aw_i2c_package.addr_bytes;
	uint8_t data_bytes = aw9610x->aw_i2c_package.data_bytes;
	uint8_t reg_num = aw9610x->aw_i2c_package.reg_num;

	for (i = 0; i < data_bytes * reg_num; i++) {
		if (reg_num < attr_buf[1]) {
			temp_buf[0] = buf[attr_buf[0] + (addr_bytes + i) * 5];
			temp_buf[1] =
				buf[attr_buf[0] + (addr_bytes + i) * 5 + 1];
		} else if (reg_num >= attr_buf[1] && reg_num < attr_buf[3]) {
			temp_buf[0] = buf[attr_buf[2] + (addr_bytes + i) * 5];
			temp_buf[1] =
				buf[attr_buf[2] + (addr_bytes + i) * 5 + 1];
		} else if (reg_num >= attr_buf[3] && reg_num < attr_buf[5]) {
			temp_buf[0] = buf[attr_buf[4] + (addr_bytes + i) * 5];
			temp_buf[1] =
				buf[attr_buf[4] + (addr_bytes + i) * 5 + 1];
		}
		sscanf(temp_buf, "%02x", &databuf[i]);
		reg_data[i] = (uint8_t)databuf[i];
	}
	aw9610x->aw_i2c_package.p_reg_data = reg_data;
	i2c_write_seq(aw9610x);
}

static void aw9610x_power_on_prox_detection(struct aw9610x *aw9610x)
{
	int32_t ret = 0;
	uint32_t reg_data = 0;
	uint32_t temp_time = AW9610X_SCAN_DEFAULT_TIME;

	AWLOGD(aw9610x->dev, "enten");

	ret = aw9610x_filedata_deal(aw9610x);
	if ((aw9610x->cali_flag == AW_NO_CALI) && (ret >= 0))
		aw9610x_spereg_deal(aw9610x);

	aw9610x_i2c_write(aw9610x, REG_IRQEN, 0);
	aw9610x_i2c_write(aw9610x, REG_CMD, 0x0001);
	while ((temp_time)--) {
		aw9610x_i2c_read(aw9610x, REG_IRQSRC, &reg_data);
		reg_data = (reg_data >> 4) & 0x01;
		if (reg_data == 1) {
			AWLOGI(aw9610x->dev,
				"time = %d", temp_time);
			if ((aw9610x->cali_flag == AW_CALI) && ret >= 0)
				aw9610x_get_calidata(aw9610x);
			break;
		}
		msleep(1);
	}
	aw9610x_i2c_read(aw9610x, REG_STAT2, &reg_data);
	if (reg_data & 0x10000)
		aw9610x->power_prox = 1;
}

static int32_t aw9610x_input_init(struct aw9610x *aw9610x, uint8_t *err_num)
{
	uint32_t i = 0;
	int32_t ret = 0;
	uint32_t j = 0;

	aw9610x->channels_arr = devm_kzalloc(aw9610x->dev,
				sizeof(struct aw_channels_info) * AW_CHANNEL_MAX,
				GFP_KERNEL);
	if (aw9610x->channels_arr == NULL) {
		AWLOGE(aw9610x->dev, "devm_kzalloc err");
		return -AW_ERR;
	}

	for (i = 0; i < AW_CHANNEL_MAX; i++) {
		snprintf(aw9610x->channels_arr[i].name,
				sizeof(aw9610x->channels_arr->name),
				"aw_sar%d_ch%d",
				aw9610x->sar_num, i);

		aw9610x->channels_arr[i].last_channel_info = FAR;

		if ((aw9610x->channel_use_flag >> i) & 0x01) {
			aw9610x->channels_arr[i].used = AW_TURE;
			aw9610x->channels_arr[i].input = input_allocate_device();
			if (aw9610x->channels_arr[i].input == NULL) {
				*err_num = i;
				goto exit_input;
			}
			aw9610x->channels_arr[i].input->name = aw9610x->channels_arr[i].name;
			__set_bit(EV_KEY, aw9610x->channels_arr[i].input->evbit);
			__set_bit(EV_SYN, aw9610x->channels_arr[i].input->evbit);
			__set_bit(KEY_F1, aw9610x->channels_arr[i].input->keybit);
			input_set_abs_params(aw9610x->channels_arr[i].input,
						ABS_DISTANCE, -1, 100, 0, 0);
			ret = input_register_device(aw9610x->channels_arr[i].input);
			if (ret) {
				AWLOGE(aw9610x->dev, "failed to register input device");
				goto exit_input;
			}
		} else {
			aw9610x->channels_arr[i].used = AW_FALSE;
			aw9610x->channels_arr[i].input = NULL;
		}
	}

	return AW_OK;

exit_input:
	for (j = 0; j < i; j++) {
		if(aw9610x->channels_arr[j].input != NULL) {
			input_free_device(aw9610x->channels_arr[j].input);
		}
	}
	return -AW_ERR;
}

static void aw9610x_channel_scan_start(struct aw9610x *aw9610x)
{
	AWLOGD(aw9610x->dev, "enter");
	if (aw9610x->pwprox_dete == true) {
		 aw9610x_power_on_prox_detection(aw9610x);
	} else {
		aw9610x_i2c_write(aw9610x, REG_CMD, AW9610X_ACTIVE_MODE);
	}

	aw9610x_i2c_write(aw9610x, REG_IRQEN, aw9610x->hostirqen);
	aw9610x->mode = AW9610X_ACTIVE_MODE;
}

static void aw9610x_reg_version_comp(struct aw9610x *aw9610x,
					 struct aw_bin *aw_bin)
{
	uint8_t i = 0;
	uint32_t blfilt1_data = 0;
	uint32_t blfilt1_tmp = 0;

	if ((aw9610x->chip_name[7] == 'A') &&
		(aw_bin->header_info[0].chip_type[7] == '\0')) {
		AWLOGI(aw9610x->dev, "enter");
		for (i = 0; i < 6; i++) {
			aw9610x_i2c_read(aw9610x, REG_BLFILT_CH0 + (0x3c * i), &blfilt1_data);
			AWLOGD(aw9610x->dev, "addr 0x%04x val: 0x%08x", REG_BLFILT_CH0 + (0x3c * i), blfilt1_data);
			blfilt1_tmp = (blfilt1_data >> 25) & 0x1;
			if (blfilt1_tmp == 1) {
				AWLOGD(aw9610x->dev, "ablfilt1_tmp is 1");
				aw9610x_i2c_write_bits(aw9610x,
					REG_BLRSTRNG_CH0 + (0x3c * i), ~(0x3f), 1 << i);
			}
		}
	}
}

static void aw9610x_bin_valid_loaded(struct aw9610x *aw9610x,
						struct aw_bin *aw_bin_data_s)
{
	uint32_t i;
	int32_t ret = 0;
	uint16_t reg_addr;
	uint32_t reg_data;
	uint32_t start_addr = aw_bin_data_s->header_info[0].valid_data_addr;

	for (i = 0; i < aw_bin_data_s->header_info[0].valid_data_len;
						i += 6, start_addr += 6) {
		reg_addr = (aw_bin_data_s->info.data[start_addr]) |
				aw_bin_data_s->info.data[start_addr + 1] << 8;
		reg_data = aw_bin_data_s->info.data[start_addr + 2] |
			(aw_bin_data_s->info.data[start_addr + 3] << 8) |
			(aw_bin_data_s->info.data[start_addr + 4] << 16) |
			(aw_bin_data_s->info.data[start_addr + 5] << 24);
		if ((reg_addr == REG_EEDA0) || (reg_addr == REG_EEDA1))
			continue;
		if (reg_addr == REG_IRQEN) {
			aw9610x->hostirqen = reg_data;
			continue;
		}
		ret = aw9610x_i2c_write(aw9610x, reg_addr, reg_data);
		if (ret < 0)
			return ;

		AWLOGI(aw9610x->dev,
			"reg_addr = 0x%04x, reg_data = 0x%08x",
					reg_addr, reg_data);
	}
	AWLOGI(aw9610x->dev, "bin writen completely");

	aw9610x_reg_version_comp(aw9610x, aw_bin_data_s);

	aw9610x_channel_scan_start(aw9610x);

}

/***************************************************************************
* para loaded
****************************************************************************/
static int32_t aw9610x_para_loaded(struct aw9610x *aw9610x)
{
	int32_t i = 0;
	int32_t len = ARRAY_SIZE(aw9610x_reg_default);

	AWLOGD(aw9610x->dev, "start to download para!");

	for (i = 0; i < len; i = i + 2) {
		aw9610x_i2c_write(aw9610x, (uint16_t)aw9610x_reg_default[i], aw9610x_reg_default[i+1]);
		if (aw9610x_reg_default[i] == REG_IRQEN)
			aw9610x->hostirqen = aw9610x_reg_default[i+1];
		AWLOGI(aw9610x->dev, "reg_addr = 0x%04x, reg_data = 0x%08x",
						aw9610x_reg_default[i],
						aw9610x_reg_default[i+1]);
	}
	AWLOGI(aw9610x->dev, "para writen completely");

	aw9610x_channel_scan_start(aw9610x);

	return 0;
}

static void aw9610x_cfg_all_loaded(const struct firmware *cont, void *context)
{
	int32_t ret;
	struct aw_bin *aw_bin;
	struct aw9610x *aw9610x = context;

	AWLOGD(aw9610x->dev, "enter");

	if (!cont) {
		AWLOGE(aw9610x->dev, "%s request failed", aw9610x->cfg_name);
		release_firmware(cont);
		return;
	} else {
		AWLOGI(aw9610x->dev,
			"%s request successfully", aw9610x->cfg_name);
	}

	aw_bin = kzalloc(cont->size + sizeof(struct aw_bin), GFP_KERNEL);
	if (!aw_bin) {
		AWLOGE(aw9610x->dev, "failed to allcating memory!");
		goto err_resource_release_hand;
	}
	aw_bin->info.len = cont->size;
	memcpy(aw_bin->info.data, cont->data, cont->size);
	ret = aw9610x_parsing_bin_file(aw_bin);
	if (ret < 0) {
		AWLOGE(aw9610x->dev, "[:aw9610x parse bin fail! ret = %d", ret);
		goto err_resource_release_hand;
	}

	snprintf(aw9610x->chip_type, sizeof(aw9610x->chip_type), "%s", aw_bin->header_info[0].chip_type);
	AWLOGE(aw9610x->dev, "chip name: %s", aw9610x->chip_type);

	aw9610x_bin_valid_loaded(aw9610x, aw_bin);

	kfree(aw_bin);
	release_firmware(cont);
	return;

err_resource_release_hand:
	kfree(aw_bin);
	release_firmware(cont);
}

static int32_t aw9610x_cfg_update(struct aw9610x *aw9610x)
{
#ifdef AW9610X_SUPPORT_MTK
		uint32_t irq_status_temp = 0;
#endif
	AWLOGD(aw9610x->dev, "enter");

	if (aw9610x->firmware_flag == true) {
		snprintf(aw9610x->cfg_name, sizeof(aw9610x->cfg_name),
					"aw9610x_%d.bin", aw9610x->sar_num);

		request_firmware_nowait(THIS_MODULE, FW_ACTION_HOTPLUG,
							aw9610x->cfg_name,
							aw9610x->dev,
							GFP_KERNEL,
							aw9610x,
							aw9610x_cfg_all_loaded);
	} else {
		aw9610x_para_loaded(aw9610x);
	}
#ifdef AW9610X_SUPPORT_MTK
		disable_irq(aw9610x->to_irq);
		/* read clear interrupt*/
		aw9610x_i2c_read(aw9610x, REG_IRQSRC, &irq_status_temp);
		aw9610x_i2c_write(aw9610x, REG_CMD, AW9610X_SLEEP_MODE);
		aw9610x->mode = AW9610X_SLEEP_MODE;
#endif

	return AW_SAR_SUCCESS;
}

static void aw9610x_cfg_work_routine(struct work_struct *work)
{
	struct aw9610x
		*aw9610x = container_of(work, struct aw9610x, cfg_work.work);

	AWLOGD(aw9610x->dev, "enter");

	aw9610x_cfg_update(aw9610x);
}

static int32_t
aw9610x_sar_cfg_init(struct aw9610x *aw9610x, int32_t flag)
{
	uint32_t cfg_timer_val = 0;

	AWLOGD(aw9610x->dev, "enter");

	if (flag == AW_CFG_LOADED)
		cfg_timer_val = 20;
	else if (flag == AW_CFG_UNLOAD)
		cfg_timer_val = 5000;
	else
		return -AW_CFG_LOAD_TIME_FAILED;

	INIT_DELAYED_WORK(&aw9610x->cfg_work, aw9610x_cfg_work_routine);
	schedule_delayed_work(&aw9610x->cfg_work,
					msecs_to_jiffies(cfg_timer_val));

	return AW_SAR_SUCCESS;
}

/*****************************************************
 *
 * first irq clear
 *
 *****************************************************/
static int32_t aw9610x_init_irq_handle(struct aw9610x *aw9610x)
{
	uint8_t cnt = 20;
	uint32_t reg_data;

	AWLOGD(aw9610x->dev, "enter");

	while (cnt--) {
		aw9610x_i2c_read(aw9610x, REG_IRQSRC, &reg_data);
		aw9610x->first_irq_flag = reg_data & 0x01;
		if (aw9610x->first_irq_flag == 1) {
			AWLOGD(aw9610x->dev, "cnt = %d", cnt);
			return AW_SAR_SUCCESS;
		}
	}
	AWLOGE(aw9610x->dev, "hardware has trouble!");

	return -AW_IRQIO_FAILED;
}

/*****************************************************
 *
 * software reset
 *
 *****************************************************/
static void aw9610x_sw_reset(struct aw9610x *aw9610x)
{
	AWLOGD(aw9610x->dev, "enter");

	aw9610x_i2c_write(aw9610x, REG_RESET, 0);
	msleep(20);
}

static int32_t aw9610x_baseline_filter(struct aw9610x *aw9610x)
{
	int32_t ret = 0;
	uint8_t i = 0;
	uint32_t status0 = 0;
	uint32_t status1 = 0;

	ret = aw9610x_i2c_read(aw9610x, REG_STAT1, &status1);
	if (ret < 0)
		return ret;
	ret = aw9610x_i2c_read(aw9610x, REG_STAT0, &status0);
	if (ret < 0)
		return ret;

	for (i = 0; i < AW_CHANNEL_MAX; i++) {
		if (((status1 >> i) & 0x01) == 1) {
			if (aw9610x->satu_flag[i] == 0) {
				ret = aw9610x_i2c_read(aw9610x,
					REG_BLFILT_CH0 + i * AW_CL1SPE_DEAL_OS,
					&aw9610x->satu_data[i]);
				if (ret < 0)
					return ret;
				ret = aw9610x_i2c_write(aw9610x,
				REG_BLFILT_CH0 + i * AW_CL1SPE_DEAL_OS,
				((aw9610x->satu_data[i] | 0x1fc) & 0x3fffffff));
				if (ret < 0)
					return ret;
				aw9610x->satu_flag[i] = 1;
			}
		} else if (((status1 >> i) & 0x01) == 0) {
			if (aw9610x->satu_flag[i] == 1) {
				if (((status0 >> (i + 24)) & 0x01) == 0) {
					ret = aw9610x_i2c_write(aw9610x,
					REG_BLFILT_CH0 + i * AW_CL1SPE_DEAL_OS,
					aw9610x->satu_data[i]);
					if (ret < 0)
						return ret;
					aw9610x->satu_flag[i] = 0;
				}
			}
		}
	}

	return ret;
}

static void aw9610x_saturat_release_handle(struct aw9610x *aw9610x)
{
	uint32_t satu_irq = 0;
	uint8_t i = 0;
	int32_t ret = 0;
	uint32_t status0 = 0;

	AWLOGD(aw9610x->dev, "enter");

	satu_irq = (aw9610x->irq_status >> 7) & 0x01;
	if (satu_irq == 1) {
		ret = aw9610x_baseline_filter(aw9610x);
		if (ret < 0)
			return;
	} else {
		ret = aw9610x_i2c_read(aw9610x, REG_STAT0, &status0);
		if (ret < 0)
			return;
		for (i = 0; i < AW_CHANNEL_MAX; i++) {
			if (aw9610x->satu_flag[i] == 1) {
				if (((status0 >> (i + 24)) & 0x01) == 0) {
					ret = aw9610x_i2c_write(aw9610x,
					REG_BLFILT_CH0 + i * AW_CL1SPE_DEAL_OS,
					aw9610x->satu_data[i]);
					if (ret < 0)
						return;
					aw9610x->satu_flag[i] = 0;
				}
			}
		}
	}

	AWLOGI(aw9610x->dev, "satu_irq handle over!");
}

/******************************************************
 *
 * sys group attribute
 *
 ******************************************************/
static ssize_t aw9610x_set_reg(struct device *dev,
						struct device_attribute *attr,
						const char *buf, size_t count)
{
	struct aw9610x *aw9610x = dev_get_drvdata(dev);
	uint32_t databuf[2] = { 0, 0 };

	if (sscanf(buf, "%x %x", &databuf[0], &databuf[1]) == 2)
		aw9610x_i2c_write(aw9610x, (uint16_t)databuf[0],
							(uint32_t)databuf[1]);

	return count;
}

static ssize_t aw9610x_get_reg(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct aw9610x *aw9610x = dev_get_drvdata(dev);
	ssize_t len = 0;
	uint32_t i = 0;
	uint32_t reg_val = 0;
	uint32_t reg_num = 0;

	reg_num = ARRAY_SIZE(aw9610x_reg_access);
	for (i = 0; i < reg_num; i++) {
		if (aw9610x_reg_access[i].rw & REG_RD_ACCESS) {
			aw9610x_i2c_read(aw9610x, aw9610x_reg_access[i].reg,
								&reg_val);
			len += snprintf(buf + len, PAGE_SIZE - len,
						"reg:0x%04x=0x%08x\n",
						aw9610x_reg_access[i].reg,
						reg_val);
		}
	}

	return len;
}

static ssize_t aw9610x_diff_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct aw9610x *aw9610x = dev_get_drvdata(dev);
	ssize_t len = 0;
	uint8_t i = 0;
	int32_t reg_val = 0;

	for (i = 0; i < AW_CHANNEL_MAX; i++) {
		aw9610x_i2c_read(aw9610x, REG_DIFF_CH0 + i * 4, &reg_val);
		reg_val /= AW_DATA_PROCESS_FACTOR;
		len += snprintf(buf+len, PAGE_SIZE-len, "DIFF_CH%d = %d\n", i,
								reg_val);
	}

	return len;
}

static ssize_t aw9610x_parasitic_data_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct aw9610x *aw9610x = dev_get_drvdata(dev);
	ssize_t len = 0;
	uint8_t i = 0;
	uint32_t reg_val = 0;
	uint32_t coff_data = 0;
	uint32_t coff_data_int = 0;
	uint32_t coff_data_dec = 0;
	uint8_t temp_data[20] = { 0 };

	for (i = 0; i < AW_CHANNEL_MAX; i++) {
		aw9610x_i2c_read(aw9610x,
			REG_AFECFG1_CH0 + i * AW_CL1SPE_CALI_OS, &reg_val);
		coff_data = (reg_val >> 24) * 900 +
						((reg_val >> 16) & 0xff) * 13;
		coff_data_int = coff_data / 1000;
		coff_data_dec = coff_data % 1000;
		snprintf(temp_data, sizeof(temp_data), "%d.%d", coff_data_int,
								coff_data_dec);
		len += snprintf(buf+len, PAGE_SIZE-len,
				"PARASITIC_DATA_CH%d = %s pf\n", i, temp_data);
	}

	return len;
}

static ssize_t aw9610x_awrw_get(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct aw9610x *aw9610x = dev_get_drvdata(dev);
	uint8_t reg_data[228] = { 0 };
	uint8_t i = 0;
	ssize_t len = 0;
	uint8_t reg_num = aw9610x->aw_i2c_package.reg_num;
	uint8_t data_bytes = aw9610x->aw_i2c_package.data_bytes;

	i2c_read_seq(aw9610x, reg_data);
	for (i = 0; i < reg_num * data_bytes; i++)
		len += snprintf(buf + len, PAGE_SIZE - len,
						"0x%02x,", reg_data[i]);

	len += snprintf(buf + len - 1, PAGE_SIZE - len, "\n");

	return len - 1;
}

static ssize_t aw9610x_factory_cali_set(struct device *dev,
						struct device_attribute *attr,
						const char *buf, size_t count)
{
	struct aw9610x *aw9610x = dev_get_drvdata(dev);
	uint32_t databuf[1] = { 0 };

	if (sscanf(buf, "%d", &databuf[0]) == 1) {
		if ((databuf[0] == 1) && (aw9610x->pwprox_dete == true)) {
			aw9610x_get_calidata(aw9610x);
		} else {
			AWLOGE(aw9610x->dev, "aw_unsupport the pw_prox_dete=%d",
						aw9610x->pwprox_dete);
			return count;
		}
		aw9610x_sw_reset(aw9610x);
		aw9610x->cali_flag = AW_NO_CALI;
		aw9610x_sar_cfg_init(aw9610x, AW_CFG_LOADED);
	}

	return count;
}

static ssize_t aw9610x_power_prox_get(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct aw9610x *aw9610x = dev_get_drvdata(dev);
	ssize_t len = 0;

	if (aw9610x->pwprox_dete == false) {
		len += snprintf(buf + len, PAGE_SIZE - len,
							"unsupport powerprox!");
		return len;
	}

	len += snprintf(buf + len, PAGE_SIZE - len, "power_prox: ");
	len += snprintf(buf + len, PAGE_SIZE - len, "%d\n",
							aw9610x->power_prox);

	return len;
}

static ssize_t aw9610x_awrw_set(struct device *dev,
						struct device_attribute *attr,
						const char *buf, size_t count)
{
	struct aw9610x *aw9610x = dev_get_drvdata(dev);
	uint32_t datatype[3] = { 0 };

	if (sscanf(buf, "%d %d %d", &datatype[0], &datatype[1],
							&datatype[2]) == 3) {
		aw9610x->aw_i2c_package.addr_bytes = (uint8_t)datatype[0];
		aw9610x->aw_i2c_package.data_bytes = (uint8_t)datatype[1];
		aw9610x->aw_i2c_package.reg_num = (uint8_t)datatype[2];

		aw9610x_addrblock_load(dev, buf);
		if (count > 7 + 5 * aw9610x->aw_i2c_package.addr_bytes)
			aw9610x_datablock_load(dev, buf);
	}

	return count;
}

static ssize_t aw9610x_set_update(struct device *dev,
						struct device_attribute *attr,
						const char *buf, size_t count)
{
	ssize_t ret;
	uint32_t state;
	int32_t cfg_timer_val = 10;
	struct aw9610x *aw9610x = dev_get_drvdata(dev);

	ret = kstrtouint(buf, 10, &state);
	if (ret) {
		AWLOGE(aw9610x->dev, "fail to set update");
		return ret;
	}
	if (state) {
		aw9610x_i2c_write(aw9610x, REG_IRQEN, 0);
		aw9610x_sw_reset(aw9610x);
		schedule_delayed_work(&aw9610x->cfg_work,
					msecs_to_jiffies(cfg_timer_val));
	}

	return count;
}

static ssize_t aw9610x_aot_cali_set(struct device *dev,
						struct device_attribute *attr,
						const char *buf, size_t count)
{
	ssize_t ret;
	uint32_t state;
	uint32_t data_en = 0;
	struct aw9610x *aw9610x = dev_get_drvdata(dev);

	ret = kstrtouint(buf, 10, &state);
	if (ret) {
		AWLOGE(aw9610x->dev, "fail to set aot cali");
		return ret;
	}
	aw9610x_i2c_read(aw9610x, REG_SCANCTRL0, &data_en);

	if (state != 0)
		aw9610x_i2c_write_bits(aw9610x, REG_SCANCTRL0, ~(0x3f << 8),
							(data_en & 0x3f) << 8);
	else
		AWLOGE(aw9610x->dev, "fail to set aot cali");

	return count;
}

static ssize_t aw9610x_get_satu(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct aw9610x *aw9610x = dev_get_drvdata(dev);
	ssize_t len = 0;

	if (aw9610x->satu_release != 0)
		len += snprintf(buf + len, PAGE_SIZE - len,
			"satu_ralease function is supporting! the flag = %d\n",
							aw9610x->satu_release);
	else
		len += snprintf(buf + len, PAGE_SIZE - len,
			"satu_ralease function unsupport! the flag = %d\n",
							aw9610x->satu_release);

	return len;
}

static ssize_t aw9610x_set_satu(struct device *dev,
						struct device_attribute *attr,
						const char *buf, size_t count)
{
	ssize_t ret;
	uint32_t state;
	struct aw9610x *aw9610x = dev_get_drvdata(dev);

	ret = kstrtouint(buf, 10, &state);
	if (ret) {
		AWLOGE(aw9610x->dev, "fail to set satu");
		return ret;
	}
	if (state && (aw9610x->vers == AW9610X)) {
		aw9610x_saturat_release_handle(aw9610x);
		aw9610x->satu_release = AW9610X_FUNC_ON;
	} else {
		aw9610x->satu_release = AW9610X_FUNC_OFF;
	}

	return count;
}

static ssize_t aw9610x_operation_mode_set(struct device *dev,
						struct device_attribute *attr,
						const char *buf, size_t count)
{
	ssize_t ret = 0;
	uint32_t irq_status_temp = 0;
	struct aw9610x *aw9610x = dev_get_drvdata(dev);

	ret = kstrtouint(buf, 10, &aw9610x->mode);
	if (ret) {
		AWLOGE(aw9610x->dev, "fail to set operation mode");
		return ret;
	}

	if (aw9610x->mode == AW9610X_ACTIVE_MODE &&
				aw9610x->old_mode != AW9610X_ACTIVE_MODE) {
		if (aw9610x->old_mode == AW9610X_DEEPSLEEP_MODE) {
			aw9610x_i2c_write(aw9610x, REG_OSCEN,
							AW9610X_CPU_WORK_MASK);
			enable_irq(aw9610x->to_irq);
		}
		if (aw9610x->old_mode == AW9610X_SLEEP_MODE)
			enable_irq(aw9610x->to_irq);
		aw9610x_i2c_write(aw9610x, REG_CMD, AW9610X_ACTIVE_MODE);
	} else if (aw9610x->mode == AW9610X_SLEEP_MODE &&
				aw9610x->old_mode != AW9610X_SLEEP_MODE) {
		if (aw9610x->old_mode == AW9610X_DEEPSLEEP_MODE) {
			aw9610x_i2c_write(aw9610x, REG_OSCEN,
							AW9610X_CPU_WORK_MASK);
		} else {
			disable_irq(aw9610x->to_irq);
			/***interrupt read clear ***/
			aw9610x_i2c_read(aw9610x, REG_IRQSRC, &irq_status_temp);
		}
		aw9610x_i2c_write(aw9610x, REG_CMD, AW9610X_SLEEP_MODE);
	} else if ((aw9610x->mode == AW9610X_DEEPSLEEP_MODE) &&
		  aw9610x->old_mode != AW9610X_DEEPSLEEP_MODE &&
				 ((aw9610x->vers == AW9610XA) ||
					(aw9610x->vers == AW9610XB))) {
		if (aw9610x->old_mode != AW9610X_SLEEP_MODE) {
			disable_irq(aw9610x->to_irq);
			aw9610x_i2c_read(aw9610x, REG_IRQSRC, &irq_status_temp);
		}
		if (aw9610x->vers == AW9610XB)
			aw9610x_i2c_write(aw9610x, REG_CMD, AW9610XB_DEEPSLEEP_MODE);
		else
			aw9610x_i2c_write(aw9610x, REG_CMD, AW9610X_DEEPSLEEP_MODE);
	} else {
		AWLOGE(aw9610x->dev, "failed to operation mode!");
		return count;
	}
	aw9610x->old_mode = aw9610x->mode;

	return count;
}

static ssize_t aw9610x_operation_mode_get(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct aw9610x *aw9610x = dev_get_drvdata(dev);
	ssize_t len = 0;

	if (aw9610x->mode == AW9610X_ACTIVE_MODE)
		len += snprintf(buf + len, PAGE_SIZE - len,
						"operation mode: Active\n");
	else if (aw9610x->mode == AW9610X_SLEEP_MODE)
		len += snprintf(buf + len, PAGE_SIZE - len,
						"operation mode: Sleep\n");
	else if ((aw9610x->mode == AW9610X_DEEPSLEEP_MODE) &&
				((aw9610x->vers == AW9610XA) ||
					(aw9610x->vers == AW9610XB)))
		len += snprintf(buf + len, PAGE_SIZE - len,
						"operation mode: DeepSleep\n");
	else
		len += snprintf(buf + len, PAGE_SIZE - len,
					"operation mode: Unconfirmed\n");

	return len;
}
static ssize_t aw9610x_chip_info_get(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct aw9610x *aw9610x = dev_get_drvdata(dev);
	ssize_t len = 0;
	uint32_t reg_data = 0;

	len += snprintf(buf + len, PAGE_SIZE - len, "sar%d , driver version %s\n", aw9610x->sar_num, AW9610X_DRIVER_VERSION);
	len += snprintf(buf + len, PAGE_SIZE - len, "The driver supports UI\n");

	aw9610x_i2c_read(aw9610x, REG_CHIPID, &reg_data);
	len += snprintf(buf + len, PAGE_SIZE - len, "chipid is 0x%08x\n", reg_data);

	aw9610x_i2c_read(aw9610x, REG_IRQEN, &reg_data);
	len += snprintf(buf + len, PAGE_SIZE - len, "REG_HOSTIRQEN is 0x%08x\n", reg_data);

	len += snprintf(buf + len, PAGE_SIZE - len, "chip_name:%s bin_prase_chip_name:%s\n",
							aw9610x->chip_name, aw9610x->chip_type);

	return len;
}

static DEVICE_ATTR(reg, 0664, aw9610x_get_reg, aw9610x_set_reg);
static DEVICE_ATTR(diff, 0664, aw9610x_diff_show, NULL);
static DEVICE_ATTR(parasitic_data, 0664, aw9610x_parasitic_data_show, NULL);
static DEVICE_ATTR(factory_cali, 0664, NULL, aw9610x_factory_cali_set);
static DEVICE_ATTR(aot_cali, 0664, NULL, aw9610x_aot_cali_set);
static DEVICE_ATTR(awrw, 0664, aw9610x_awrw_get, aw9610x_awrw_set);
static DEVICE_ATTR(update, 0644, NULL, aw9610x_set_update);
static DEVICE_ATTR(satu, 0644, aw9610x_get_satu, aw9610x_set_satu);
static DEVICE_ATTR(prox, 0644, aw9610x_power_prox_get, NULL);
static DEVICE_ATTR(operation_mode, 0644, aw9610x_operation_mode_get,
						aw9610x_operation_mode_set);
static DEVICE_ATTR(chip_info, 0664, aw9610x_chip_info_get, NULL);

static struct attribute *aw9610x_sar_attributes[] = {
	&dev_attr_reg.attr,
	&dev_attr_diff.attr,
	&dev_attr_parasitic_data.attr,
	&dev_attr_awrw.attr,
	&dev_attr_factory_cali.attr,
	&dev_attr_aot_cali.attr,
	&dev_attr_update.attr,
	&dev_attr_satu.attr,
	&dev_attr_prox.attr,
	&dev_attr_operation_mode.attr,
	&dev_attr_chip_info.attr,
	NULL
};

static struct attribute_group aw9610x_sar_attribute_group = {
	.attrs = aw9610x_sar_attributes
};

/*****************************************************
*
* irq init
*
*****************************************************/
static void aw9610x_irq_handle(struct aw9610x *aw9610x)
{
	uint8_t i = 0;
	uint32_t curr_status = 0;
	uint32_t curr_status_val = 0;
#ifdef AW9610X_SUPPORT_MTK
	static int32_t sarData[AW_CHANNEL_MAX] = {0};
#endif

	AWLOGD(aw9610x->dev, "enter");

	aw9610x_i2c_read(aw9610x, REG_STAT0, &curr_status_val);
	AWLOGD(aw9610x->dev, "channel = 0x%08x", curr_status_val);

	if (aw9610x->channels_arr == NULL) {
		AWLOGE(aw9610x->dev, "input err!!!");
		return;
	}

	for (i = 0; i < AW_CHANNEL_MAX; i++) {
		curr_status =
			(((uint8_t)(curr_status_val >> (24 + i)) & 0x1))
#ifdef AW_INPUT_TRIGGER_TH1
			| (((uint8_t)(curr_status_val >> (16 + i)) & 0x1) << 1)
#endif
#ifdef AW_INPUT_TRIGGER_TH2
			| (((uint8_t)(curr_status_val >> (8 + i)) & 0x1) << 2)
#endif
#ifdef AW_INPUT_TRIGGER_TH3
			| (((uint8_t)(curr_status_val >> (i)) & 0x1) << 3)
#endif
			;
		AWLOGI(aw9610x->dev, "curr_state[%d] = 0x%x", i, curr_status);

		if (aw9610x->channels_arr[i].used == AW_FALSE) {
			AWLOGD(aw9610x->dev, "channels_arr[%d] no user", i);
			continue;
		}
		if (aw9610x->channels_arr[i].last_channel_info == curr_status) {
			continue;
		}

		switch (curr_status) {
		case FAR:
			input_report_abs(aw9610x->channels_arr[i].input, ABS_DISTANCE, 0);
#ifdef AW9610X_SUPPORT_MTK
			sarData[i] = 0;
#endif
			break;
		case TRIGGER_TH0:
			input_report_abs(aw9610x->channels_arr[i].input, ABS_DISTANCE, 1);
#ifdef AW9610X_SUPPORT_MTK
			sarData[i] = 1;
#endif
			break;
#ifdef AW_INPUT_TRIGGER_TH1
		case TRIGGER_TH1:
			input_report_abs(aw9610x->channels_arr[i].input, ABS_DISTANCE, 2);
			break;
#endif
#ifdef AW_INPUT_TRIGGER_TH2
		case TRIGGER_TH2:
			input_report_abs(aw9610x->channels_arr[i].input, ABS_DISTANCE, 3);
			break;
#endif
#ifdef AW_INPUT_TRIGGER_TH3
		case TRIGGER_TH3:
			input_report_abs(aw9610x->channels_arr[i].input, ABS_DISTANCE, 4);
			break;
#endif
		default:
			AWLOGE(aw9610x->dev, "error abs distance");
			return;
		}
		input_sync(aw9610x->channels_arr[i].input);

		aw9610x->channels_arr[i].last_channel_info = curr_status;
	}
#ifdef AW9610X_SUPPORT_MTK
	sarValue[0] = sarData[AW_CHANNEL0];	//BoraG use CH0
	sarValue[1] = sarData[AW_CHANNEL1];	//BoraG use CH1
	sarValue[2] = sarData[AW_CHANNEL3];	//BoraG use CH3

	sar_data_report(sarValue);
#endif
}

static void aw9610x_farirq_handle(struct aw9610x *aw9610x)
{
	uint8_t th0_far = 0;

	th0_far = (aw9610x->irq_status >> 2) & 0x1;
	if (th0_far == 1)
		aw9610x->power_prox = AW9610X_FUNC_OFF;
}

static void aw9610x_irq_multiple_sar_select(struct aw9610x *aw9610x)
{
	/* multiple sar handle IO */
	switch (aw9610x->sar_num) {
	case AW_SAR0:
		break;
	case AW_SAR1:
		break;
	default:
		return;
	}
}

static void aw9610x_version_aw9610x_private(struct aw9610x *aw9610x)
{
	AWLOGD(aw9610x->dev, "AW9610X enter");

	if (aw9610x->satu_release == AW9610X_FUNC_ON)
		aw9610x_saturat_release_handle(aw9610x);
}

static void aw9610x_version_aw9610xA_private(struct aw9610x *aw9610x)
{
	AWLOGD(aw9610x->dev, "AW9610XA enter");
}

static void aw9610x_interrupt_clear(struct aw9610x *aw9610x)
{
	int32_t ret = 0;

	AWLOGD(aw9610x->dev, "enter");

	ret = aw9610x_i2c_read(aw9610x, REG_IRQSRC, &aw9610x->irq_status);
	if (ret < 0) {
		AWLOGE(aw9610x->dev, "i2c IO error");
		return;
	}
	AWLOGI(aw9610x->dev, "IRQSRC = 0x%x", aw9610x->irq_status);

	if (aw9610x->pwprox_dete == true)
		aw9610x_farirq_handle(aw9610x);

	switch (aw9610x->vers) {
	case AW9610X:
		aw9610x_version_aw9610x_private(aw9610x);
		break;
	case AW9610XA:
		aw9610x_version_aw9610xA_private(aw9610x);
		break;
	default:
		return;
	}

	aw9610x_irq_multiple_sar_select(aw9610x);

	aw9610x_irq_handle(aw9610x);
}

#ifdef AW_HEADSET_PLUG_CALI
void aw9610x_plug_headset_cali(int val)
{
	uint32_t data_en = 0;
	int ret = 0;
	if (PLUG_HEADSET == val) {
		AWLOGI(h_aw9610x->dev, "Headset plug,going to force calibrate");
	} else {
		AWLOGI(h_aw9610x->dev, "Headset unplug,going to force calibrate");
	}
	aw9610x_i2c_read(h_aw9610x, REG_SCANCTRL0, &data_en);
	ret = aw9610x_i2c_write_bits(h_aw9610x, REG_SCANCTRL0, ~(0x3f << 8),
							(data_en & 0x3f) << 8);
	if (ret < 0){
		AWLOGE(h_aw9610x->dev, "Headset insert,calibrate sar sensor failed");
		return;
	}
}
EXPORT_SYMBOL(aw9610x_plug_headset_cali);
#endif

static irqreturn_t aw9610x_irq(int32_t irq, void *data)
{
	struct aw9610x *aw9610x = data;
	AWLOGD(aw9610x->dev, "enter");
	aw9610x_interrupt_clear(aw9610x);
	AWLOGD(aw9610x->dev, "exit");

	return IRQ_HANDLED;
}

#ifdef AW_PINCTRL_ON
void aw9610x_int_output(struct aw9610x *aw9610x, int32_t level)
{
	pr_info("%s enter aw9610x int level:%d\n", __func__, level);
	if (level == 0) {
		if (aw9610x->pinctrl.pinctrl) {
			pinctrl_select_state(aw9610x->pinctrl.pinctrl,
						aw9610x->pinctrl.int_out_low);
		} else {
			pr_info("%s Failed set int pin output low\n", __func__);
		}
	} else if (level == 1) {
		if (aw9610x->pinctrl.pinctrl) {
			pinctrl_select_state(aw9610x->pinctrl.pinctrl,
						aw9610x->pinctrl.int_out_high);
		} else {
			pr_info("%s Failed set int pin output high\n", __func__);
		}
	}
}

static int32_t aw9610x_pinctrl_init(struct aw9610x *aw9610x)
{
	struct aw9610x_pinctrl *pinctrl = &aw9610x->pinctrl;
	uint8_t pin_default_name[50] = { 0 };
	uint8_t pin_output_low_name[50] = { 0 };
	uint8_t pin_output_high_name[50] = { 0 };

	AWLOGD(aw9610x->dev, "enter");

	pinctrl->pinctrl = devm_pinctrl_get(aw9610x->dev);
	if (IS_ERR_OR_NULL(pinctrl->pinctrl)) {
		pr_info("%s:No pinctrl found\n", __func__);
		pinctrl->pinctrl = NULL;
		return -EINVAL;
	}

	snprintf(pin_default_name, sizeof(pin_default_name),
					"aw_default_sar%d", aw9610x->sar_num);
	AWLOGD(aw9610x->dev, "pin_default_name = %s", pin_default_name);
	pinctrl->default_sta = pinctrl_lookup_state(pinctrl->pinctrl,
							pin_default_name);
	if (IS_ERR_OR_NULL(pinctrl->default_sta)) {
		AWLOGE(aw9610x->dev, "Failed get pinctrl state:default state");
		goto exit_pinctrl_init;
	}

	snprintf(pin_output_high_name, sizeof(pin_output_high_name),
				"aw_int_output_high_sar%d", aw9610x->sar_num);
	AWLOGD(aw9610x->dev, "pin_output_high_name = %s", pin_output_high_name);
	pinctrl->int_out_high = pinctrl_lookup_state(pinctrl->pinctrl,
							pin_output_high_name);
	if (IS_ERR_OR_NULL(pinctrl->int_out_high)) {
		AWLOGE(aw9610x->dev, "Failed get pinctrl state:output_high");
		goto exit_pinctrl_init;
	}

	snprintf(pin_output_low_name, sizeof(pin_output_low_name),
				"aw_int_output_low_sar%d", aw9610x->sar_num);
	AWLOGD(aw9610x->dev, "pin_output_low_name = %s", pin_output_low_name);
	pinctrl->int_out_low = pinctrl_lookup_state(pinctrl->pinctrl,
							pin_output_low_name);
	if (IS_ERR_OR_NULL(pinctrl->int_out_low)) {
		AWLOGE(aw9610x->dev, "Failed get pinctrl state:output_low");
		goto exit_pinctrl_init;
	}

	pr_info("%s: Success init pinctrl\n", __func__);
	return 0;
exit_pinctrl_init:
	devm_pinctrl_put(pinctrl->pinctrl);
	pinctrl->pinctrl = NULL;
	return -EINVAL;
}

static void aw9610x_pinctrl_deinit(struct aw9610x *aw9610x)
{
	if (aw9610x->pinctrl.pinctrl)
		devm_pinctrl_put(aw9610x->pinctrl.pinctrl);
}
#endif

static int32_t aw9610x_interrupt_init(struct aw9610x *aw9610x)
{
	int32_t irq_flags = 0;
	int32_t ret = 0;
	uint8_t i = 0;
	int8_t irq_gpio_name[100] = { 0 };

	AWLOGD(aw9610x->dev, "enter");

	for (i = 0; i < AW_CHANNEL_MAX; i++)
		aw9610x->satu_flag[i] = 0;

	snprintf(irq_gpio_name, sizeof(irq_gpio_name),
					"aw9610x_irq_gpio%d", aw9610x->sar_num);

	if (gpio_is_valid(aw9610x->irq_gpio)) {
		aw9610x->to_irq = gpio_to_irq(aw9610x->irq_gpio);

		ret = devm_gpio_request_one(aw9610x->dev,
					aw9610x->irq_gpio,
					GPIOF_DIR_IN | GPIOF_INIT_HIGH,
					irq_gpio_name);

		if (ret) {
			AWLOGE(aw9610x->dev,
				"request irq gpio failed, ret = %d", ret);
			ret = -AW_IRQIO_FAILED;
		} else {
			/* register irq handler */
			irq_flags = IRQF_TRIGGER_FALLING | IRQF_ONESHOT;
			ret = devm_request_threaded_irq(&aw9610x->i2c->dev,
							aw9610x->to_irq, NULL,
							aw9610x_irq, irq_flags,
							"aw9610x_irq", aw9610x);
			if (ret != 0) {
				AWLOGE(aw9610x->dev,
						"failed to request IRQ %d: %d",
						aw9610x->to_irq, ret);
				ret = -AW_IRQ_REQUEST_FAILED;
			} else {
				AWLOGI(aw9610x->dev,
					"IRQ request successfully!");
				ret = AW_SAR_SUCCESS;
			}
		}
	} else {
		AWLOGE(aw9610x->dev, "irq gpio invalid!");
		return -AW_IRQIO_FAILED;
	}

	return ret;
}

/*****************************************************
 *
 * parse dts
 *
 *****************************************************/
static int32_t aw9610x_parse_dt(struct device *dev, struct aw9610x *aw9610x,
			   struct device_node *np)
{
	uint32_t val = 0;

	val = of_property_read_u32(np, "sar-num", &aw9610x->sar_num);
	if (val != 0) {
		AWLOGE(aw9610x->dev, "multiple sar failed!");
		return -AW_MULTIPLE_SAR_FAILED;
	} else {
		AWLOGI(aw9610x->dev, "sar num = %d", aw9610x->sar_num);
	}

	aw9610x->irq_gpio = of_get_named_gpio(np, "irq-gpio", 0);
	if (aw9610x->irq_gpio < 0) {
		aw9610x->irq_gpio = -1;
		AWLOGE(aw9610x->dev, "no irq gpio provided.");
		return -AW_IRQGPIO_FAILED;
	} else {
		AWLOGI(aw9610x->dev, "irq gpio provided ok.");
	}

	aw9610x->firmware_flag =
			of_property_read_bool(np, "aw9610x,using-firmware");
	AWLOGI(aw9610x->dev, "firmware_flag = <%d>", aw9610x->firmware_flag);

	aw9610x->pwprox_dete =
		of_property_read_bool(np, "aw9610x,using-pwon-prox-dete");
	AWLOGI(aw9610x->dev, "pwprox_dete = <%d>", aw9610x->pwprox_dete);

	aw9610x->satu_release =
		of_property_read_bool(np, "aw9610x,using-satu");
	AWLOGI(aw9610x->dev, "satu_release = <%d>", aw9610x->satu_release);

	val = of_property_read_u32(np, "channel_use_flag", &aw9610x->channel_use_flag);
	if (val != 0) {
		AWLOGE(aw9610x->dev, "channel_use_flag failed!");
		return -AW_ERR;
	} else {
		AWLOGI(aw9610x->dev, "channel_use_flag = 0x%x", aw9610x->channel_use_flag);
	}
	return AW_SAR_SUCCESS;
}

#ifdef AW_POWER_ON
static int32_t aw9610x_power_init(struct aw9610x *aw9610x)
{
	int32_t rc;
	uint8_t vcc_name[20] = { 0 };

	AWLOGD(aw9610x->dev, "aw9610x power init enter");

	snprintf(vcc_name, sizeof(vcc_name), "vcc%d", aw9610x->sar_num);
	AWLOGD(aw9610x->dev, "vcc_name = %s", vcc_name);

	aw9610x->vcc = regulator_get(aw9610x->dev, vcc_name);
	if (IS_ERR(aw9610x->vcc)) {
		rc = PTR_ERR(aw9610x->vcc);
		AWLOGE(aw9610x->dev, "regulator get failed vcc rc = %d", rc);
		return rc;
	}

	if (regulator_count_voltages(aw9610x->vcc) > 0) {
		rc = regulator_set_voltage(aw9610x->vcc,
					AW_VCC_MIN_UV, AW_VCC_MAX_UV);
		if (rc) {
			AWLOGE(aw9610x->dev,
				"regulator set vol failed rc = %d", rc);
			goto reg_vcc_put;
		}
	}

	return rc;

reg_vcc_put:
	regulator_put(aw9610x->vcc);
	return rc;
}

static void aw9610x_power_enable(struct aw9610x *aw9610x, bool on)
{
	int32_t rc = 0;

	AWLOGD(aw9610x->dev, "aw9610x power enable enter");

	if (on) {
		rc = regulator_enable(aw9610x->vcc);
		if (rc) {
			AWLOGE(aw9610x->dev,
				"regulator_enable vol failed rc = %d", rc);
		} else {
			aw9610x->power_enable = true;
			msleep(20);
		}
	} else {
		rc = regulator_disable(aw9610x->vcc);
		if (rc)
			AWLOGE(aw9610x->dev,
				"regulator_disable vol failed rc = %d", rc);
		else
			aw9610x->power_enable = false;
	}
}

static int32_t regulator_is_get_voltage(struct aw9610x *aw9610x)
{
	uint32_t cnt = 10;
	int32_t voltage_val = 0;

	AWLOGD(aw9610x->dev, "enter");

	while(cnt--) {
		voltage_val = regulator_get_voltage(aw9610x->vcc);
		AWLOGD(aw9610x->dev, "aw9610x voltage is : %d uv", voltage_val);
		if (voltage_val >= AW9610X_CHIP_MIN_VOLTAGE)
			return AW_SAR_SUCCESS;
		mdelay(1);
	}

	return -AW_VERS_ERR;
}

static int32_t aw9610x_wait_chip_init(struct aw9610x *aw9610x)
{
	uint32_t cnt = 20;
	uint32_t reg_data = 0;
	uint32_t chip_init_flag = 0;

	AWLOGD(aw9610x->dev, "enter");

	while (cnt--) {
		aw9610x_i2c_read(aw9610x, REG_IRQSRC, &reg_data);
		AWLOGE(aw9610x->dev, "REG_IRQSRC = 0x%x", reg_data);
		chip_init_flag = reg_data & 0x01;
		if (chip_init_flag == 1) {
			AWLOGE(aw9610x->dev, "chip init success cnt = %d", cnt);
			return AW_SAR_SUCCESS;
		}
		mdelay(1);
	}
	AWLOGE(aw9610x->dev, "hardware has trouble!");

	return -AW_IRQIO_FAILED;
}

#endif

/*****************************************************
 *
 * check chip id
 *
 *****************************************************/
static int32_t aw9610x_read_chipid(struct aw9610x *aw9610x)
{
	int32_t ret = -1;
	uint8_t cnt = 0;
	uint32_t reg_val = 0;

	while (cnt < AW_READ_CHIPID_RETRIES) {
		ret = aw9610x_i2c_read(aw9610x, REG_CHIPID, &reg_val);
		if (ret < 0) {
			AWLOGE(aw9610x->dev, "read CHIP ID failed: %d", ret);
		} else {
			reg_val = reg_val >> 16;
			break;
		}

		cnt++;
		usleep_range(2000, 3000);
	}

	if (reg_val == AW9610X_CHIP_ID) {
		AWLOGI(aw9610x->dev, "aw9610x detected, 0x%08x", reg_val);
		return AW_SAR_SUCCESS;
	} else {
		AWLOGE(aw9610x->dev,
			"unsupport dev, chipid is (0x%04x)", reg_val);
	}

	return -AW_CHIPID_FAILED;
}

static void aw9610x_i2c_set(struct i2c_client *i2c,
						struct aw9610x *aw9610x)
{
	aw9610x->dev = &i2c->dev;
	aw9610x->i2c = i2c;
	i2c_set_clientdata(i2c, aw9610x);
}

static int32_t aw9610x_version_init(struct aw9610x *aw9610x)
{
	uint32_t fw_ver = 0;
	int32_t ret = 0;
	uint32_t firmvers = 0;

	aw9610x_i2c_read(aw9610x, REG_FWVER, &firmvers);
	AWLOGD(aw9610x->dev, "REG_FWVER = 0x%08x", firmvers);

	ret = aw9610x_i2c_read(aw9610x, REG_FWVER2, &fw_ver);
	if (ret < 0) {
		AWLOGE(aw9610x->dev, "read REG_FWVER2 err!");
		return AW_ERR;
	}
	snprintf(aw9610x->chip_name, sizeof(aw9610x->chip_name),
						"AW9610X");
	memcpy(aw9610x->chip_name, "AW9610X", strlen("AW9610X") + 1);

	AWLOGD(aw9610x->dev, "REG_FWVER2 : 0x%08x", fw_ver);
	if (fw_ver == AW_CHIP_AW9610XA) {
		aw9610x->vers = AW9610XA;
		memcpy(aw9610x->chip_name + strlen(aw9610x->chip_name), "A", 2);
	} else {
		aw9610x->vers = AW9610X;
		aw9610x->chip_name[7] = '\0';
	}
	AWLOGI(aw9610x->dev, "the IC is = %s", aw9610x->chip_name);
	return AW_OK;
}

#ifdef AW_USB_PLUG_CALI
static void aw9610x_ps_notify_callback_work(struct work_struct *work)
{
	struct aw9610x *aw9610x = container_of(work, struct aw9610x, ps_notify_work);
	uint32_t data_en = 0;
	int ret = 0;

	AWLOGI(aw9610x->dev, "Usb insert,going to force calibrate");
	aw9610x_i2c_read(aw9610x, REG_SCANCTRL0, &data_en);
	ret = aw9610x_i2c_write_bits(aw9610x, REG_SCANCTRL0, ~(0x3f << 8),
							(data_en & 0x3f) << 8);
	if (ret < 0){
		AWLOGE(aw9610x->dev, "Usb insert,calibrate sar sensor failed");
		return;
	}
}

static int aw9610x_ps_get_state(struct aw9610x *aw9610x, struct power_supply *psy, bool *present)
{
	union power_supply_propval pval = { 0 };
	int retval;

#ifdef CONFIG_AW9610X_MTK_CHARGER
	retval = power_supply_get_property(psy, POWER_SUPPLY_PROP_ONLINE,
			&pval);
#else
	retval = power_supply_get_property(psy, POWER_SUPPLY_PROP_PRESENT,
			&pval);
#endif
	if (retval) {
		AWLOGE(aw9610x->dev, "%s psy get property failed", psy->desc->name);
		return retval;
	}
	*present = (pval.intval) ? true : false;
	AWLOGI(aw9610x->dev, "%s is %s", psy->desc->name,
			(*present) ? "present" : "not present");
	return 0;
}

static int aw9610x_ps_notify_callback(struct notifier_block *self,
		unsigned long event, void *p)
{
	struct aw9610x *aw9610x = container_of(self, struct aw9610x, ps_notif);
	struct power_supply *psy = p;
	bool present;
	int retval;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,9,0)
	if (event == PSY_EVENT_PROP_CHANGED
#else
	if (event == PSY_EVENT_PROP_CHANGED
#endif
			&& psy && psy->desc->get_property && psy->desc->name &&
			!strncmp(psy->desc->name, USB_POWER_SUPPLY_NAME, sizeof(USB_POWER_SUPPLY_NAME))){
		AWLOGI(aw9610x->dev, "ps notification: event = %lu", event);
		retval = aw9610x_ps_get_state(aw9610x, psy, &present);
		if (retval) {
			AWLOGE(aw9610x->dev, "psy get property failed");
			return retval;
		}
		if (event == PSY_EVENT_PROP_CHANGED) {
			if (aw9610x->ps_is_present == present) {
				AWLOGE(aw9610x->dev, "ps present state not change");
				return 0;
			}
		}
		aw9610x->ps_is_present = present;
		schedule_work(&aw9610x->ps_notify_work);
	}
	return 0;
}

static int aw9610x_ps_notify_init(struct aw9610x *aw9610x)
{
	struct power_supply *psy = NULL;
	int ret = 0;

	AWLOGD(aw9610x->dev, "enter");
	INIT_WORK(&aw9610x->ps_notify_work, aw9610x_ps_notify_callback_work);
	aw9610x->ps_notif.notifier_call = aw9610x_ps_notify_callback;
	ret = power_supply_reg_notifier(&aw9610x->ps_notif);
	if (ret) {
		AWLOGE(aw9610x->dev,"Unable to register ps_notifier: %d", ret);
		return -AW_ERR;
	}
	psy = power_supply_get_by_name(USB_POWER_SUPPLY_NAME);
	if (psy) {
		ret = aw9610x_ps_get_state(aw9610x, psy, &aw9610x->ps_is_present);
		if (ret) {
			AWLOGE(aw9610x->dev, "psy get property failed rc=%d",
				ret);
			goto free_ps_notifier;
		}
	}
	else
	{
		AWLOGE(aw9610x->dev,"power_supply_get_by_name get fail.");
	}
	return AW_OK;
free_ps_notifier:
	power_supply_unreg_notifier(&aw9610x->ps_notif);
	return -AW_ERR;
}
#endif

#ifdef AW9610X_SUPPORT_MTK
static int aw9610x_situation_open_report_data(int open)
{
	uint32_t irq_status_temp = 0;

	AW9610X_FUN();
    
    if (!open)
    {
		AW9610X_LOG("%s close report data, line:%d.\n", __func__, __LINE__);
		if (g_aw9610x) {
			disable_irq(g_aw9610x->to_irq);
			/* read clear interrupt*/
			aw9610x_i2c_read(g_aw9610x, REG_IRQSRC, &irq_status_temp);
			aw9610x_i2c_write(g_aw9610x, REG_CMD, AW9610X_SLEEP_MODE);
			g_aw9610x->mode = AW9610X_SLEEP_MODE;
		}
	}
	else
	{
		AW9610X_LOG("%s open report data, line:%d.\n", __func__, __LINE__);
		if (g_aw9610x) {
			aw9610x_i2c_write(g_aw9610x, REG_CMD, AW9610X_ACTIVE_MODE);
			g_aw9610x->mode = AW9610X_ACTIVE_MODE;
			enable_irq(g_aw9610x->to_irq);
		}
	}
    
    return 0;
}

static int aw9610x_situation_batch(int flag, int64_t samplingPeriodNs, int64_t maxBatchReportLatencyNs)
{
    AW9610X_FUN();
	return 0;
}

static int aw9610x_situation_flush(void)
{
	AW9610X_FUN();
	return situation_flush_report(ID_SAR);
}

static int aw9610x_situation_get_data(int *probability, int *status)
{
	AW9610X_FUN();
	return 0;
}

static int sar_factory_enable_sensor(bool enabledisable,
                                         int64_t sample_periods_ms)
{
	AW9610X_FUN();
	return 0;
}

static int sar_factory_get_data(int32_t sensor_data[3])
{
	AW9610X_FUN();
	return 0;
}

static int sar_factory_enable_calibration(void)
{
	uint32_t data_en = 0;
	AW9610X_FUN();

	aw9610x_i2c_read(g_aw9610x, REG_SCANCTRL0, &data_en);
	aw9610x_i2c_write_bits(g_aw9610x, REG_SCANCTRL0, ~(0x3f << 8),
							(data_en & 0x3f) << 8);
    
	return 0;
}

static int sar_factory_get_cali(int32_t data[3])
{
	AW9610X_FUN();
	return 0;
}

static struct sar_factory_fops aw9610x_factory_fops = {
	.enable_sensor = sar_factory_enable_sensor,
	.get_data = sar_factory_get_data,
    .enable_calibration = sar_factory_enable_calibration,
    .get_cali = sar_factory_get_cali,
};

static struct sar_factory_public aw9610x_factory_device = {
	.gain = 1,
	.sensitivity = 1,
    .fops = &aw9610x_factory_fops,
};

static int aw9610x_situation_init(void)
{
	struct situation_control_path ctl = {0};
	struct situation_data_path data = {0};
	int err = 0;

	ctl.open_report_data = aw9610x_situation_open_report_data;
	ctl.batch = aw9610x_situation_batch;
	ctl.flush = aw9610x_situation_flush;
	ctl.is_support_wake_lock = false;
	ctl.is_support_batch = false;
	err = situation_register_control_path(&ctl, ID_SAR);
	if (err) {
		goto exit;
	}

	data.get_data = aw9610x_situation_get_data;
	err = situation_register_data_path(&data, ID_SAR);
	if (err) {
		goto exit;
	}

	err = sar_factory_device_register(&aw9610x_factory_device);
    if (err) {
		goto exit;
    }

	return 0;

exit:
	return err;
}
#endif

static int32_t
aw9610x_i2c_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
{
	struct aw9610x *aw9610x;
	struct device_node *np = i2c->dev.of_node;
	int32_t ret = 0;
	uint8_t err_num = 0;
	uint8_t i = 0;

	if (!i2c_check_functionality(i2c->adapter, I2C_FUNC_I2C)) {
		AWLOGE(&i2c->dev, "check_functionality failed");
		return -EIO;
	}

	aw9610x = devm_kzalloc(&i2c->dev, sizeof(struct aw9610x), GFP_KERNEL);
	if (aw9610x == NULL) {
		AWLOGE(&i2c->dev, "failed to malloc memory!");
		ret = -AW_MALLOC_FAILED;
		goto err_malloc;
	}

	aw9610x_i2c_set(i2c, aw9610x);

#ifdef AW_POWER_ON
	/* aw9610x power init */
	ret = aw9610x_power_init(aw9610x);
	if (ret)
		AWLOGE(&i2c->dev, "aw9610x power init failed");
	else
		aw9610x_power_enable(aw9610x, true);

	ret = regulator_is_get_voltage(aw9610x);
	if (ret != AW_SAR_SUCCESS) {
		AWLOGE(aw9610x->dev, "get_voltage failed");
		goto err_get_voltage;
	}

	ret = aw9610x_wait_chip_init(aw9610x);
	if (ret != AW_SAR_SUCCESS) {
		AWLOGE(aw9610x->dev, "_wait_chip_inite failed");
		goto err_wait_chip_init;
	}
#endif
	/* aw9610x chip id */
	ret = aw9610x_read_chipid(aw9610x);
	if (ret != AW_SAR_SUCCESS) {
		AWLOGE(aw9610x->dev, "read chipid failed, ret=%d", ret);
		goto err_chipid;
	}

	ret = aw9610x_parse_dt(&i2c->dev, aw9610x, np);
	if (ret != AW_SAR_SUCCESS) {
		AWLOGE(aw9610x->dev, "irq gpio error!, ret = %d", ret);
		goto err_pase_dt;
	}
	ret = aw9610x_input_init(aw9610x, &err_num);
	if (ret < 0) {
		AWLOGE(aw9610x->dev, "input init err");
		goto err_input_init;
	}
	ret = sysfs_create_group(&i2c->dev.kobj, &aw9610x_sar_attribute_group);
	if (ret < 0) {
		AWLOGE(aw9610x->dev, "error creating sysfs attr files");
		goto err_sysfs;
	}

	ret = aw9610x_version_init(aw9610x);
	if (ret < 0) {
		AWLOGE(aw9610x->dev, "read version failed, ret=%d", ret);
		goto err_vers_load;
	}

	aw9610x_sw_reset(aw9610x);

	ret = aw9610x_init_irq_handle(aw9610x);
	if (ret != AW_SAR_SUCCESS) {
		AWLOGE(aw9610x->dev, "the trouble ret = %d", ret);
		goto err_first_irq;
	}

#ifdef AW_PINCTRL_ON
	ret = aw9610x_pinctrl_init(aw9610x);
	if (ret < 0) {
		/* if define pinctrl must define the following state
		 * to let int-pin work normally: default, int_output_high,
		 * int_output_low, int_input
		 */
		pr_err("%s: Failed get wanted pinctrl state\n", __func__);
		goto err_pinctrl;
	}

	aw9610x_int_output(aw9610x, 1);
#endif
	ret = aw9610x_interrupt_init(aw9610x);
	if (ret == -AW_IRQ_REQUEST_FAILED) {
		AWLOGE(aw9610x->dev, "request irq failed ret = %d", ret);
		goto err_requst_irq;
	}

#ifdef AW_USB_PLUG_CALI
	ret = aw9610x_ps_notify_init(aw9610x);
	if (ret < 0) {
		AWLOGE(aw9610x->dev, "error creating power supply notify");
		goto err_ps_notify;
	}
#endif

#ifdef AW_HEADSET_PLUG_CALI
	h_aw9610x = aw9610x;
#endif

	ret = aw9610x_sar_cfg_init(aw9610x, AW_CFG_UNLOAD);
	if (ret < 0) {
		AWLOGE(aw9610x->dev, "cfg situation not confirmed!");
		goto err_cfg;
	}

	g_aw9610x = aw9610x;
	aw9610x_situation_init();

	return AW_SAR_SUCCESS;

err_cfg:
#ifdef AW_USB_PLUG_CALI
err_ps_notify:
#endif
#ifdef AW_USB_PLUG_CALI
	power_supply_unreg_notifier(&aw9610x->ps_notif);
#endif
err_requst_irq:
	if (gpio_is_valid(aw9610x->irq_gpio))
		devm_gpio_free(&i2c->dev, aw9610x->irq_gpio);
#ifdef AW_PINCTRL_ON
err_pinctrl:
	aw9610x_pinctrl_deinit(aw9610x);
#endif
err_first_irq:
err_vers_load:

err_sysfs:
	sysfs_remove_group(&i2c->dev.kobj, &aw9610x_sar_attribute_group);
err_input_init:
	for (i = 0; i < err_num; i++) {
		input_unregister_device(aw9610x->channels_arr[i].input);
	}
	for (i = 0; i < AW_CHANNEL_MAX; i++) {
		if (aw9610x->channels_arr[i].input != NULL)
			input_free_device(aw9610x->channels_arr[i].input);
	}
err_pase_dt:
err_chipid:
#ifdef AW_POWER_ON
err_wait_chip_init:
err_get_voltage:
	if (aw9610x->power_enable) {
		regulator_disable(aw9610x->vcc);
		regulator_put(aw9610x->vcc);
	}
#endif
err_malloc:
	return ret;
}

static int32_t aw9610x_i2c_remove(struct i2c_client *i2c)
{
	struct aw9610x *aw9610x = i2c_get_clientdata(i2c);
	uint32_t i = 0;

#ifdef AW_POWER_ON
	if (aw9610x->power_enable) {
		regulator_disable(aw9610x->vcc);
		regulator_put(aw9610x->vcc);
	}
#endif
#ifdef AW_PINCTRL_ON
		aw9610x_pinctrl_deinit(aw9610x);
#endif
	for (i = 0; i < AW_CHANNEL_MAX; i++) {
		input_unregister_device(aw9610x->channels_arr[i].input);
		if (aw9610x->channels_arr[i].input != NULL)
			input_free_device(aw9610x->channels_arr[i].input);
	}
	devm_kfree(aw9610x->dev, aw9610x->channels_arr);

	sysfs_remove_group(&i2c->dev.kobj, &aw9610x_sar_attribute_group);
	return 0;
}

static int aw9610x_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct aw9610x *aw9610x = i2c_get_clientdata(client);

	AWLOGD(aw9610x->dev, "suspend enter");

	return 0;
}

static int aw9610x_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct aw9610x *aw9610x = i2c_get_clientdata(client);

	AWLOGD(aw9610x->dev, "resume enter");

	return 0;
}

static void aw9610x_i2c_shutdown(struct i2c_client *i2c)
{
	struct aw9610x *aw9610x = i2c_get_clientdata(i2c);
	uint32_t irq_status_temp = 0;

	pr_info("%s enter", __func__);

	disable_irq(aw9610x->to_irq);
	/* read clear interrupt*/
	aw9610x_i2c_read(aw9610x, REG_IRQSRC, &irq_status_temp);
	aw9610x_i2c_write(aw9610x, REG_CMD, AW9610X_SLEEP_MODE);
}

static const struct dev_pm_ops aw9610x_pm_ops = {
	.suspend = aw9610x_suspend,
	.resume = aw9610x_resume,
};

static const struct of_device_id aw9610x_dt_match[] = {
	{ .compatible = "awinic,aw9610x_sar" },
	{ },
};

static const struct i2c_device_id aw9610x_i2c_id[] = {
	{ AW9610X_I2C_NAME, 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, aw9610x_i2c_id);

static struct i2c_driver aw9610x_i2c_driver = {
	.driver = {
		.name = AW9610X_I2C_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(aw9610x_dt_match),
		.pm = &aw9610x_pm_ops,
	},
	.probe = aw9610x_i2c_probe,
	.remove = aw9610x_i2c_remove,
	.shutdown = aw9610x_i2c_shutdown,
	.id_table = aw9610x_i2c_id,
};

static int aw9610x_local_init(void)
{
	pr_info("%s enter", __func__);
	
	if(i2c_add_driver(&aw9610x_i2c_driver)) {
		pr_err("add aw9610x driver error\n");
		return -1;
	}
	
	return 0;
}

static int aw9610x_local_uninit(void)
{
	pr_info("%s enter", __func__);
	i2c_del_driver(&aw9610x_i2c_driver);

	return 0;
}

static struct situation_init_info aw9610x_init_info = {
	.name = "aw9610x",
	.init = aw9610x_local_init,
	.uninit = aw9610x_local_uninit,
};

static int __init aw9610x_init(void)
{
	pr_info("aw9610x_init enter.\n");
	situation_driver_add(&aw9610x_init_info, ID_SAR);
	return 0;
}

static void __exit aw9610x_exit(void)
{
	pr_info("aw9610x_exit enter\n");
}

module_init(aw9610x_init);
module_exit(aw9610x_exit);
MODULE_DESCRIPTION("AW9610X SAR Driver");
MODULE_LICENSE("GPL");
MODULE_LICENSE("GPL v2");
