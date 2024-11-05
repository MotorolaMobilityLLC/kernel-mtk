/* SPDX-License-Identifier: GPL-2.0
 *
 * aw87xxx_device.h  aw87xxx pa module
 *
 * Copyright (c) 2021 AWINIC Technology CO., LTD
 *
 * Author: Barry <zhaozhongbo@awinic.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#ifndef __AW87XXX_DEVICE_H__
#define __AW87XXX_DEVICE_H__
#include <linux/version.h>
#include <linux/kernel.h>
#include <sound/control.h>
#include <sound/soc.h>
#include "aw87xxx_acf_bin.h"


#define AW_PRODUCT_NAME_LEN		(8)

#define AW_GPIO_HIGHT_LEVEL		(1)
#define AW_GPIO_LOW_LEVEL		(0)

#define AW_I2C_RETRIES			(5)
#define AW_I2C_RETRY_DELAY		(2)
#define AW_I2C_READ_MSG_NUM		(2)

#define AW_READ_CHIPID_RETRIES		(5)
#define AW_READ_CHIPID_RETRY_DELAY	(2)

#define AW_DEV_REG_INVALID_MASK		(0xff)

#define AW_NO_RESET_GPIO		(-1)

#define AW_PID_9B_BIN_REG_CFG_COUNT	(10)

#define AW87XXX_DELAY_REG_ADDR		(0xFE)
#define AW87XXX_REG_DELAY_TIME		(1000)

#define AW_BOOST_VOLTAGE_MIN		(0x00)

#define AW_REG_NONE		(0xFF)

/********************************************
 *
 * aw87xxx register attributes
 *
 *******************************************/
#define AW87XXX_CHIPIDL_REG (0x00)
#define AW87XXX_SW_RESET_PASSWORD (0xAA)
#define AW87XXX_CHIPIDH_REG (0x01)
#define AW87XXX_SYSCTRL_REG (0x04)
#define AW87XXX_EN_RCV_START (7)
#define AW87XXX_EN_RCV_LEN (1)
#define AW87XXX_EN_RCV_MASK \
	(~(((1 << AW87XXX_EN_RCV_LEN) - 1) << AW87XXX_EN_RCV_START))
#define AW87XXX_EN_RCV_DISABLE (0)
#define AW87XXX_EN_RCV_ENABLE (1)
#define AW87XXX_EN_SW_START (6)
#define AW87XXX_EN_SW_LEN (1)
#define AW87XXX_EN_SW_MASK \
	(~(((1 << AW87XXX_EN_SW_LEN) - 1) << AW87XXX_EN_SW_START))
#define AW87XXX_EN_SW_DISABLE (0)
#define AW87XXX_EN_SW_DISABLE_VALUE \
	(AW87XXX_EN_SW_DISABLE << AW87XXX_EN_SW_START)
#define AW87XXX_EN_SW_ENABLE (1)
#define AW87XXX_EN_SW_ENABLE_VALUE \
	(AW87XXX_EN_SW_ENABLE << AW87XXX_EN_SW_START)
#define AW87XXX_CPOVP_REG (0x05)
#define AW87XXX_BSTCTRL_REG (0x06)
#define AW87XXX_BST_IPEAK_START (0)
#define AW87XXX_BST_IPEAK_LEN (4)
#define AW87XXX_BST_IPEAK_MASK \
	(~(((1 << AW87XXX_BST_IPEAK_LEN) - 1) << AW87XXX_BST_IPEAK_START))
#define AW87XXX_ESD_REG (0x64)
#define AW87XXX_VCINL_REG (0x7C)
#define AW87XXX_VCINH_REG (0x7D)
#define AW87XXX_VCOUTL_REG (0x7E)
#define AW87XXX_VCOUTH_REG (0x7F)

#define AW87XXX_PID_9B_POWER_ON_DELAY_MS (0)
#define AW87XXX_PID_9B_POWER_OFF_DELAY_MS (0)
#define AW87XXX_PID_9B_REG_MAX (0x63)
#define AW87XXX_PID_9B_SYSCTRL_DEFAULT (0x03)
#define AW87XXX_PID_9B_SYSCTRL_REG (0x01)
#define AW87XXX_PID_9B_SPK_MODE_START_BIT (0)
#define AW87XXX_PID_9B_SPK_MODE_BITS_LEN (1)
#define AW87XXX_PID_9B_SPK_MODE_MASK \
	(~(((1<<AW87XXX_PID_9B_SPK_MODE_BITS_LEN)-1) << AW87XXX_PID_9B_SPK_MODE_START_BIT))
#define AW87XXX_PID_9B_SPK_MODE_DISABLE	(0)
#define AW87XXX_PID_9B_SPK_MODE_DISABLE_VALUE \
	(AW87XXX_PID_9B_SPK_MODE_DISABLE << AW87XXX_PID_9B_SPK_MODE_START_BIT)
#define AW87XXX_PID_9B_SPK_MODE_ENABLE (1)
#define AW87XXX_PID_9B_SPK_MODE_ENABLE_VALUE \
	(AW87XXX_PID_9B_SPK_MODE_ENABLE << AW87XXX_PID_9B_SPK_MODE_START_BIT)
#define AW87XXX_PID_9B_REG_EN_SW_START_BIT (2)
#define AW87XXX_PID_9B_REG_EN_SW_BITS_LEN (1)
#define AW87XXX_PID_9B_REG_EN_SW_MASK \
	(~(((1<<AW87XXX_PID_9B_REG_EN_SW_BITS_LEN)-1) << AW87XXX_PID_9B_REG_EN_SW_START_BIT))
#define AW87XXX_PID_9B_REG_EN_SW_DISABLE (0)
#define AW87XXX_PID_9B_REG_EN_SW_DISABLE_VALUE \
	(AW87XXX_PID_9B_REG_EN_SW_DISABLE << AW87XXX_PID_9B_REG_EN_SW_START_BIT)
#define AW87XXX_PID_9B_REG_EN_SW_ENABLE (1)
#define AW87XXX_PID_9B_REG_EN_SW_ENABLE_VALUE \
	(AW87XXX_PID_9B_REG_EN_SW_ENABLE << AW87XXX_PID_9B_REG_EN_SW_START_BIT)
#define AW87XXX_PID_9B_ENCRYPTION_REG (0x64)
#define AW87XXX_PID_9B_ENCRYPTION_BOOST_OUTPUT_SET (0x2C)

#define AW87XXX_PID_18_POWER_ON_DELAY_MS (0)
#define AW87XXX_PID_18_POWER_OFF_DELAY_MS (0)
#define AW87XXX_PID_18_REG_MAX (0x66)
#define AW87XXX_PID_18_SYSCTRL_REG (0x03)
#define AW87XXX_PID_18_REG_REC_MODE_START_BIT (1)
#define AW87XXX_PID_18_REG_REC_MODE_BITS_LEN (1)
#define AW87XXX_PID_18_REG_REC_MODE_MASK \
	(~(((1<<AW87XXX_PID_18_REG_REC_MODE_BITS_LEN)-1) << AW87XXX_PID_18_REG_REC_MODE_START_BIT))
#define AW87XXX_PID_18_REG_REC_MODE_DISABLE	(0)
#define AW87XXX_PID_18_REG_REC_MODE_DISABLE_VALUE \
	(AW87XXX_PID_18_REG_REC_MODE_DISABLE << AW87XXX_PID_18_REG_REC_MODE_START_BIT)
#define AW87XXX_PID_18_REG_REC_MODE_ENABLE (1)
#define AW87XXX_PID_18_REG_REC_MODE_ENABLE_VALUE \
	(AW87XXX_PID_18_REG_REC_MODE_ENABLE << AW87XXX_PID_18_REG_REC_MODE_START_BIT)
#define AW87XXX_PID_18_REG_EN_SW_START_BIT (6)
#define AW87XXX_PID_18_REG_EN_SW_BITS_LEN (1)
#define AW87XXX_PID_18_REG_EN_SW_MASK \
	(~(((1<<AW87XXX_PID_18_REG_EN_SW_BITS_LEN)-1) << AW87XXX_PID_18_REG_EN_SW_START_BIT))
#define AW87XXX_PID_18_REG_EN_SW_DISABLE (0)
#define AW87XXX_PID_18_REG_EN_SW_DISABLE_VALUE \
	(AW87XXX_PID_18_REG_EN_SW_DISABLE << AW87XXX_PID_18_REG_EN_SW_START_BIT)
#define AW87XXX_PID_18_REG_EN_SW_ENABLE	(1)
#define AW87XXX_PID_18_REG_EN_SW_ENABLE_VALUE \
	(AW87XXX_PID_18_REG_EN_SW_ENABLE << AW87XXX_PID_18_REG_EN_SW_START_BIT)
#define AW87XXX_PID_18_CLASSD_REG (0x05)
#define AW87XXX_PID_18_CLASSD_DEFAULT (0x10)
#define AW87XXX_PID_18_CPOC_REG (0x04)

#define AW87XXX_PID_39_POWER_ON_DELAY_MS (0)
#define AW87XXX_PID_39_POWER_OFF_DELAY_MS (0)
#define AW87XXX_PID_39_REG_MAX (0x64)
#define AW87XXX_PID_39_REG_MODECTRL (0x02)
#define AW87XXX_PID_39_MODECTRL_DEFAULT (0xa0)
#define AW87XXX_PID_39_REC_MODE_START_BIT (3)
#define AW87XXX_PID_39_REC_MODE_BITS_LEN (1)
#define AW87XXX_PID_39_REC_MODE_MASK \
	(~(((1<<AW87XXX_PID_39_REC_MODE_BITS_LEN)-1) << AW87XXX_PID_39_REC_MODE_START_BIT))
#define AW87XXX_PID_39_REC_MODE_DISABLE	(0)
#define AW87XXX_PID_39_REC_MODE_DISABLE_VALUE \
	(AW87XXX_PID_39_REC_MODE_DISABLE << AW87XXX_PID_39_REC_MODE_START_BIT)
#define AW87XXX_PID_39_REC_MODE_ENABLE (1)
#define AW87XXX_PID_39_REC_MODE_ENABLE_VALUE \
	(AW87XXX_PID_39_REC_MODE_ENABLE << AW87XXX_PID_39_REC_MODE_START_BIT)
#define AW87XXX_PID_39_REG_CPOVP (0x03)

#define AW87XXX_PID_59_5X9_POWER_ON_DELAY_MS (0)
#define AW87XXX_PID_59_5X9_POWER_OFF_DELAY_MS (0)
#define AW87XXX_PID_59_5X9_REG_MAX (0x69)
#define AW87XXX_PID_59_5X9_REG_SYSCTRL (0x01)
#define AW87XXX_PID_59_5X9_REC_MODE_START_BIT (3)
#define AW87XXX_PID_59_5X9_REC_MODE_BITS_LEN (1)
#define AW87XXX_PID_59_5X9_REC_MODE_MASK \
	(~(((1<<AW87XXX_PID_59_5X9_REC_MODE_BITS_LEN)-1) << AW87XXX_PID_59_5X9_REC_MODE_START_BIT))
#define AW87XXX_PID_59_5X9_REC_MODE_DISABLE	(0)
#define AW87XXX_PID_59_5X9_REC_MODE_DISABLE_VALUE \
	(AW87XXX_PID_59_5X9_REC_MODE_DISABLE << AW87XXX_PID_59_5X9_REC_MODE_START_BIT)
#define AW87XXX_PID_59_5X9_REC_MODE_ENABLE (1)
#define AW87XXX_PID_59_5X9_REC_MODE_ENABLE_VALUE	\
	(AW87XXX_PID_59_5X9_REC_MODE_ENABLE << AW87XXX_PID_59_5X9_REC_MODE_START_BIT)
#define AW87XXX_PID_59_5X9_REG_ENCR (0x69)
#define AW87XXX_PID_59_5X9_ENCRY_DEFAULT (0x00)

#define AW87XXX_PID_59_3X9_POWER_ON_DELAY_MS (0)
#define AW87XXX_PID_59_3X9_POWER_OFF_DELAY_MS (0)
#define AW87XXX_PID_59_3X9_REG_MAX (0x70)
#define AW87XXX_PID_59_3X9_REG_MDCRTL (0x02)
#define AW87XXX_PID_59_3X9_SPK_MODE_START_BIT (2)
#define AW87XXX_PID_59_3X9_SPK_MODE_BITS_LEN (1)
#define AW87XXX_PID_59_3X9_SPK_MODE_MASK \
	(~(((1<<AW87XXX_PID_59_3X9_SPK_MODE_BITS_LEN)-1) << AW87XXX_PID_59_3X9_SPK_MODE_START_BIT))
#define AW87XXX_PID_59_3X9_SPK_MODE_DISABLE	(0)
#define AW87XXX_PID_59_3X9_SPK_MODE_DISABLE_VALUE \
	(AW87XXX_PID_59_3X9_SPK_MODE_DISABLE << AW87XXX_PID_59_3X9_SPK_MODE_START_BIT)
#define AW87XXX_PID_59_3X9_SPK_MODE_ENABLE (1)
#define AW87XXX_PID_59_3X9_SPK_MODE_ENABLE_VALUE	\
	(AW87XXX_PID_59_3X9_SPK_MODE_ENABLE << AW87XXX_PID_59_3X9_SPK_MODE_START_BIT)
#define AW87XXX_PID_59_3X9_REG_CPOVP (0x03)
#define AW87XXX_PID_59_3X9_REG_ENCR (0x70)
#define AW87XXX_PID_59_3X9_ENCR_DEFAULT (0x00)

#define AW87XXX_PID_5A_POWER_ON_DELAY_MS (0)
#define AW87XXX_PID_5A_POWER_OFF_DELAY_MS (0)
#define AW87XXX_PID_5A_REG_MAX (0x77)
#define AW87XXX_PID_5A_REG_SYSCTRL_REG (0x01)
#define AW87XXX_PID_5A_REG_RCV_MODE_START_BIT (2)
#define AW87XXX_PID_5A_REG_RCV_MODE_BITS_LEN (1)
#define AW87XXX_PID_5A_REG_RCV_MODE_MASK \
	(~(((1<<AW87XXX_PID_5A_REG_RCV_MODE_BITS_LEN)-1) << AW87XXX_PID_5A_REG_RCV_MODE_START_BIT))
#define AW87XXX_PID_5A_REG_RCV_MODE_DISABLE	(0)
#define AW87XXX_PID_5A_REG_RCV_MODE_DISABLE_VALUE \
	(AW87XXX_PID_5A_REG_RCV_MODE_DISABLE << AW87XXX_PID_5A_REG_RCV_MODE_START_BIT)
#define AW87XXX_PID_5A_REG_RCV_MODE_ENABLE (1)
#define AW87XXX_PID_5A_REG_RCV_MODE_ENABLE_VALUE	\
	(AW87XXX_PID_5A_REG_RCV_MODE_ENABLE << AW87XXX_PID_5A_REG_RCV_MODE_START_BIT)
#define AW87XXX_PID_5A_REG_DFT3R_REG (0x62)
#define AW87XXX_PID_5A_DFT3R_DEFAULT (0x02)
#define AW87XXX_PID_5A_REG_BSTCPR2_REG (0x05)
#define AW87XXX_PID_5A_REG_BST_IPEAK_START_BIT (0)
#define AW87XXX_PID_5A_REG_BST_IPEAK_BITS_LEN (4)
#define AW87XXX_PID_5A_REG_BST_IPEAK_MASK \
	(~(((1<<AW87XXX_PID_5A_REG_BST_IPEAK_BITS_LEN)-1) << AW87XXX_PID_5A_REG_BST_IPEAK_START_BIT))

#define AW87XXX_PID_76_POWER_ON_DELAY_MS (0)
#define AW87XXX_PID_76_POWER_OFF_DELAY_MS (0)
#define AW87XXX_PID_76_REG_MAX (0x78)
#define AW87XXX_PID_76_MDCTRL_REG (0x02)
#define AW87XXX_PID_76_EN_SPK_START_BIT (2)
#define AW87XXX_PID_76_EN_SPK_BITS_LEN (1)
#define AW87XXX_PID_76_EN_SPK_MASK \
	(~(((1<<AW87XXX_PID_76_EN_SPK_BITS_LEN)-1) << AW87XXX_PID_76_EN_SPK_START_BIT))
#define AW87XXX_PID_76_EN_SPK_DISABLE (0)
#define AW87XXX_PID_76_EN_SPK_DISABLE_VALUE	\
	(AW87XXX_PID_76_EN_SPK_DISABLE << AW87XXX_PID_76_EN_SPK_START_BIT)
#define AW87XXX_PID_76_EN_SPK_ENABLE (1)
#define AW87XXX_PID_76_EN_SPK_ENABLE_VALUE	\
	(AW87XXX_PID_76_EN_SPK_ENABLE << AW87XXX_PID_76_EN_SPK_START_BIT)
#define AW87XXX_PID_76_CPOVP_REG (0x03)
#define AW87XXX_PID_76_DFT_ADP1_REG (0x67)
#define AW87XXX_PID_76_DFT_ADP1_CHECK (0x04)

#define AW87XXX_PID_60_POWER_ON_DELAY_MS (0)
#define AW87XXX_PID_60_POWER_OFF_DELAY_MS (0)
#define AW87XXX_PID_60_REG_MAX (0x7C)
#define AW87XXX_PID_60_SYSCTRL_REG (0x01)
#define AW87XXX_PID_60_RCV_MODE_START_BIT (1)
#define AW87XXX_PID_60_RCV_MODE_BITS_LEN (1)
#define AW87XXX_PID_60_RCV_MODE_MASK \
	(~(((1<<AW87XXX_PID_60_RCV_MODE_BITS_LEN)-1) << AW87XXX_PID_60_RCV_MODE_START_BIT))
#define AW87XXX_PID_60_RCV_MODE_DISABLE	(0)
#define AW87XXX_PID_60_RCV_MODE_DISABLE_VALUE \
	(AW87XXX_PID_60_RCV_MODE_DISABLE << AW87XXX_PID_60_RCV_MODE_START_BIT)
#define AW87XXX_PID_60_RCV_MODE_ENABLE	(1)
#define AW87XXX_PID_60_RCV_MODE_ENABLE_VALUE \
	(AW87XXX_PID_60_RCV_MODE_ENABLE << AW87XXX_PID_60_RCV_MODE_START_BIT)
#define AW87XXX_PID_60_NG3_REG (0x76)
#define AW87XXX_PID_60_ESD_REG_VAL (0x91)

#define AW87XXX_PID_C1_POWER_ON_DELAY_MS (0)
#define AW87XXX_PID_C1_POWER_OFF_DELAY_MS (0)
#define AW87XXX_PID_C1_REG_MAX (0x7F)
#define AW87XXX_PID_C1_SYSCTRL_REG (0x01)
#define AW87XXX_PID_C1_EN_SPK_START_BIT (3)
#define AW87XXX_PID_C1_EN_SPK_BITS_LEN (1)
#define AW87XXX_PID_C1_EN_SPK_MASK \
	(~(((1<<AW87XXX_PID_C1_EN_SPK_BITS_LEN)-1) << AW87XXX_PID_C1_EN_SPK_START_BIT))
#define AW87XXX_PID_C1_EN_SPK_SPK_MODE_DISABLE (0)
#define AW87XXX_PID_C1_EN_SPK_SPK_MODE_DISABLE_VALUE \
	(AW87XXX_PID_C1_EN_SPK_SPK_MODE_DISABLE << AW87XXX_PID_C1_EN_SPK_START_BIT)
#define AW87XXX_PID_C1_EN_SPK_SPK_MODE_ENABLE (1)
#define AW87XXX_PID_C1_EN_SPK_SPK_MODE_ENABLE_VALUE	\
	(AW87XXX_PID_C1_EN_SPK_SPK_MODE_ENABLE << AW87XXX_PID_C1_EN_SPK_START_BIT)
#define AW87XXX_PID_C1_DFT_THGEN1_REG (0x64)
#define AW87XXX_PID_C1_DFT_THGEN1_CHECK (0x0a)
#define AW87XXX_PID_C1_TESTIN1_REG (0x7C)
#define AW87XXX_PID_C1_TESTIN2_REG (0x7D)
#define AW87XXX_PID_C1_TESTOUT1_REG (0x7E)
#define AW87XXX_PID_C1_TESTOUT2_REG (0x7F)

#define AW87XXX_PID_C2_POWER_ON_DELAY_MS (3)
#define AW87XXX_PID_C2_POWER_OFF_DELAY_MS (5)
#define AW87XXX_PID_C2_REG_MAX (0x7F)
#define AW87XXX_PID_C2_SYSCTRL_REG (0x01)
#define AW87XXX_PID_C2_RCV_MODE_START_BIT (1)
#define AW87XXX_PID_C2_RCV_MODE_BITS_LEN (1)
#define AW87XXX_PID_C2_RCV_MODE_MASK \
	(~(((1<<AW87XXX_PID_C2_RCV_MODE_BITS_LEN)-1) << AW87XXX_PID_C2_RCV_MODE_START_BIT))
#define AW87XXX_PID_C2_RCV_MODE_DISABLE	(0)
#define AW87XXX_PID_C2_RCV_MODE_DISABLE_VALUE \
	(AW87XXX_PID_C2_RCV_MODE_DISABLE << AW87XXX_PID_C2_RCV_MODE_START_BIT)
#define AW87XXX_PID_C2_RCV_MODE_ENABLE (1)
#define AW87XXX_PID_C2_RCV_MODE_ENABLE_VALUE \
	(AW87XXX_PID_C2_RCV_MODE_ENABLE << AW87XXX_PID_C2_RCV_MODE_START_BIT)
#define AW87XXX_PID_C2_PEAKLIMIT_REG (0x03)
#define AW87XXX_PID_C2_BST_IPEAK_START_BIT (0)
#define AW87XXX_PID_C2_BST_IPEAK_BITS_LEN (4)
#define AW87XXX_PID_C2_BST_IPEAK_MASK \
	(~(((1<<AW87XXX_PID_C2_BST_IPEAK_BITS_LEN)-1) << AW87XXX_PID_C2_BST_IPEAK_START_BIT))
#define AW87XXX_PID_C2_CP_REG (0x21)
#define AW87XXX_PID_C2_CP_CHECK (0x77)
#define AW87XXX_PID_C2_CRCOUT0_REG (0x37)
#define AW87XXX_PID_C2_CRCOUT1_REG (0x38)
#define AW87XXX_PID_C2_TESTIN1_REG (0x3E)
#define AW87XXX_PID_C2_TESTIN2_REG (0x3F)
/********************************************
 *
 * aw87xxx devices attributes
 *
 *******************************************/
struct aw_device;

/*#define AW_ALGO_AUTH_DSP*/
extern int g_algo_auth_st;

enum AW_ALGO_AUTH_MODE {
	AW_ALGO_AUTH_DISABLE = 0,
	AW_ALGO_AUTH_MODE_MAGIC_ID,
	AW_ALGO_AUTH_MODE_REG_CRC,
};

enum AW_ALGO_AUTH_ID {
	AW_ALGO_AUTH_MAGIC_ID = 0x4157,
};

enum AW_ALGO_AUTH_STATUS {
	AW_ALGO_AUTH_WAIT = 0,
	AW_ALGO_AUTH_OK = 1,
};

#define AW_IOCTL_MAGIC_S			'w'
#define AW_IOCTL_GET_ALGO_AUTH			_IOWR(AW_IOCTL_MAGIC_S, 1, struct algo_auth_data)
#define AW_IOCTL_SET_ALGO_AUTH			_IOWR(AW_IOCTL_MAGIC_S, 2, struct algo_auth_data)

struct algo_auth_data {
	int32_t auth_mode;  /* 0: disable  1 : chip ID  2 : reg crc */
	int32_t reg_crc;
	int32_t random;
	int32_t chip_id;
	int32_t check_result;   /* 0 failed 1 success */
};

struct aw_device_ops {
	int (*pwr_on_func)(struct aw_device *aw_dev, struct aw_data_container *data);
	int (*pwr_off_func)(struct aw_device *aw_dev, struct aw_data_container *data);
};

enum aw_dev_chipid {
	AW_DEV_CHIPID_18 = 0x18,
	AW_DEV_CHIPID_39 = 0x39,
	AW_DEV_CHIPID_59 = 0x59,
	AW_DEV_CHIPID_69 = 0x69,
	AW_DEV_CHIPID_5A = 0x5A,
	AW_DEV_CHIPID_9A = 0x9A,
	AW_DEV_CHIPID_9B = 0x9B,
	AW_DEV_CHIPID_76 = 0x76,
	AW_DEV_CHIPID_60 = 0x60,
	AW_DEV_CHIPID_C1 = 0xC1,
	AW_DEV_CHIPID_C2 = 0xC2,
};

enum aw_dev_hw_status {
	AW_DEV_HWEN_OFF = 0,
	AW_DEV_HWEN_ON,
	AW_DEV_HWEN_INVALID,
	AW_DEV_HWEN_STATUS_MAX,
};

enum aw_dev_soft_off_enable {
	AW_DEV_SOFT_OFF_DISENABLE = 0,
	AW_DEV_SOFT_OFF_ENABLE = 1,
};

enum aw_reg_receiver_mode {
	AW_NOT_REC_MODE = 0,
	AW_IS_REC_MODE = 1,
};

enum aw_reg_voltage_status {
	AW_VOLTAGE_LOW = 0,
	AW_VOLTAGE_HIGH,
};

typedef int (*dev_init_func)(struct aw_device *aw_dev);

struct aw_dev_property {
	uint8_t product_cnt;
	uint8_t max_addr;
	uint8_t esd_default;
	uint8_t sw_enabled;
	uint8_t ipeak_enabled;
	uint8_t vol_enabled;
	uint8_t auth_enabled;
	uint8_t soft_off_enabled;
	uint8_t power_on_delay_ms;
	uint8_t power_off_delay_ms;
	int id;
	const char **product;
	dev_init_func dev_init_func;
	struct aw_device_ops ops;
};

struct aw_delay_desc {
	uint8_t power_on_delay_ms;
	uint8_t power_off_delay_ms;
};

struct aw_mute_desc {
	uint8_t addr;
	uint8_t enable;
	uint8_t disable;
	uint8_t mask;
};

struct aw_esd_check_desc {
	uint8_t first_update_reg_addr;
	uint8_t first_update_reg_val;
};

struct aw_rec_mode_desc {
	uint8_t addr;
	uint8_t enable;
	uint8_t disable;
	uint8_t mask;
};

struct aw_voltage_desc {
	uint8_t addr;
	uint8_t vol_max;
	uint8_t vol_min;
};

struct aw_auth_desc {
	uint8_t reg_in_l;
	uint8_t reg_in_h;
	uint8_t reg_out_l;
	uint8_t reg_out_h;
	int32_t auth_mode;
	int32_t reg_crc;
	int32_t random;
	int32_t chip_id;
	int32_t check_result;
};

struct aw_ipeak_desc {
	unsigned int reg;
	unsigned int mask;
};

struct aw_device {
	uint8_t i2c_addr;
	uint8_t soft_off_enable;
	uint8_t is_rec_mode;
	int chipid;
	int hwen_status;
	int i2c_bus;
	int rst_gpio;
	int reg_max_addr;
	int product_cnt;
	const char **product_tab;

	struct device *dev;
	struct i2c_client *i2c;
	struct aw_delay_desc delay_desc;
	struct aw_mute_desc mute_desc;
	struct aw_esd_check_desc esd_desc;
	struct aw_rec_mode_desc rec_desc;
	struct aw_voltage_desc vol_desc;
	struct aw_auth_desc auth_desc;
	struct aw_ipeak_desc ipeak_desc;

	struct aw_device_ops ops;
};


int aw87xxx_dev_i2c_write_byte(struct aw_device *aw_dev,
			uint8_t reg_addr, uint8_t reg_data);
int aw87xxx_dev_i2c_read_byte(struct aw_device *aw_dev,
			uint8_t reg_addr, uint8_t *reg_data);
int aw87xxx_dev_i2c_read_msg(struct aw_device *aw_dev,
	uint8_t reg_addr, uint8_t *data_buf, uint32_t data_len);
int aw87xxx_dev_i2c_write_bits(struct aw_device *aw_dev,
	uint8_t reg_addr, uint8_t mask, uint8_t reg_data);
void aw87xxx_dev_soft_reset(struct aw_device *aw_dev);
void aw87xxx_dev_hw_pwr_ctrl(struct aw_device *aw_dev, bool enable);
int aw87xxx_dev_default_pwr_on(struct aw_device *aw_dev,
			struct aw_data_container *profile_data);
int aw87xxx_dev_default_pwr_off(struct aw_device *aw_dev,
			struct aw_data_container *profile_data);
int aw87xxx_dev_esd_reg_status_check(struct aw_device *aw_dev);
int aw87xxx_dev_check_reg_is_rec_mode(struct aw_device *aw_dev);
int aw87xxx_dev_init(struct aw_device *aw_dev);
int aw87xxx_dev_algo_auth_mode(struct aw_device *aw_dev, struct algo_auth_data *algo_data);
#ifdef AW_ALGO_AUTH_DSP
void aw87xxx_dev_algo_authentication(struct aw_device *aw_dev);
#endif

#endif
