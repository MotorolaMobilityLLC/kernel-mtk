/*******************************************************************************
 * AW86006VCMYOVAAF.c
 *
 * Copyright (c) 2021 AWINIC Technology CO., LTD
 *
 * Author: liangqing <liangqing@awinic.com>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of  the GNU General  Public License as published by the Free
 * Software Foundation; either version 2 of the  License, or (at your option)
 * any later version.
 ******************************************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/printk.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <uapi/asm-generic/errno-base.h>
#include "lens_info.h"
#include "aw86006_ois.h"
//#include "AW86006VCMYOVAAF.h"

#define SOC_OIS_I2C_ADDR		0x69

#ifndef OIS_MAIN_H
#define OIS_MAIN_H

#define AF_DRVNAME		"AW86006VCMYOVAAF_DRV"

/* register address */
#define REG_AF_CODE	(0x0009)

/* AF init position */
#define INIT_POS_H	(0x02)
#define INIT_POS_L	(0x00)

#endif

/* Log Format */
#ifdef AW_LOGI
#undef AW_LOGI
#define AW_LOGI(format, ...) \
	pr_info("[%s][%04d]%s: " format "\n", AF_DRVNAME, __LINE__, __func__, \
								##__VA_ARGS__)
#endif
#ifdef AW_LOGD
#undef AW_LOGD
#define AW_LOGD(format, ...) \
	pr_debug("[%s][%04d]%s: " format "\n", AF_DRVNAME, __LINE__, \
							__func__, ##__VA_ARGS__)
#endif
#ifdef AW_LOGE
#undef AW_LOGE
#define AW_LOGE(format, ...) \
	pr_err("[%s][%04d]%s: " format "\n", AF_DRVNAME, __LINE__, __func__, \
								##__VA_ARGS__)
#endif

static struct i2c_client *g_pstAF_I2Cclient;
static int *g_pAF_Opened;
static spinlock_t *g_pAF_SpinLock;
static int hea_test_runing = 0;

static unsigned long g_u4CurrPosition = 512;
static unsigned long g_u4AF_MACRO = 1023;
static unsigned long g_u4AF_INF;

extern int gyro_offset_cali_run(struct motOISGOffsetResult *param);
extern int gyro_offset_cali_set(struct motOISGOffsetResult *param);
extern int aw86006_set_ois_mode(uint8_t flag);
extern int run_aw86006ois_drawcircle(motOISHeaParam *param);
/*******************************************************************************
 * I2c read/write
 ******************************************************************************/
static int aw86006_i2c_writes(uint32_t a_u4Addr, uint8_t a_Addr_uSize,
					uint8_t *a_puData, uint16_t a_uSize)
{
	int ret = 0;
	uint8_t r_shift = 0;
	uint8_t *puSendData = NULL;

	puSendData = kzalloc(a_uSize + a_Addr_uSize, GFP_KERNEL);
	if (puSendData == NULL) {
		AW_LOGE("Allocate Buffer error, ret = %d", ret);
		return -ENOMEM;
	}

	r_shift = a_Addr_uSize;
	while (a_Addr_uSize--) {
		*(puSendData + a_Addr_uSize) = (uint8_t)(a_u4Addr >>
					(8 * (r_shift - a_Addr_uSize - 1)));
	}
	memcpy(puSendData + r_shift, a_puData, a_uSize);

	ret = i2c_master_send(g_pstAF_I2Cclient, puSendData, a_uSize + r_shift);
	if (ret < 0)
		AW_LOGE("Send data error, ret = %d", ret);

	kfree(puSendData);
	puSendData = NULL;

	return ret;
}

static inline int AF_init(void)
{
	int ret = 0;
	uint8_t data[1] = {0x1};
	uint8_t dataSac[2] = {0x5, 0x3};
	ret = aw86006_i2c_writes(0x0001, 2, data, 1);
	if(ret < 0) {
		AW_LOGE("Set OIS ON error, ret = %d", ret);
	} else {
		AW_LOGE("Set OIS ON success , ret = %d", ret);
	}
	ret = aw86006_i2c_writes(0x000B, 2, dataSac, 2);
	if(ret < 0) {
		AW_LOGE("Set SAC error, ret = %d", ret);
	} else {
		AW_LOGI("Set SAC success , ret = %d", ret);
	}
	/* add AF init start */
	AW_LOGD("AF init");
	/* add AF init end */

	return 0;
}

static inline int SetPos(unsigned long a_u4Position)
{
	int ret = 0;
	uint8_t pos[2] = { 0 };

	AW_LOGI("Start");
	if(hea_test_runing) {
		AW_LOGI("can not setPos when runing hea test");
		return 0;
	}
	pos[0] = (uint8_t)(a_u4Position >> 8);
	pos[1] = (uint8_t)(a_u4Position & 0xff);

	AW_LOGD("reg[0x0009]: 0x%02x, reg[0x000a]: 0x%02x", pos[0], pos[1]);

	ret = aw86006_i2c_writes(REG_AF_CODE, 2, pos, 2);
	if (ret < 0) {
		AW_LOGE("Set AF CODE error, ret = %d", ret);
		return -EINVAL;
	}

	return 0;
}

static inline int AW86006VCMYOVAAF_MoveVCM(unsigned long a_u4Position)
{
	AW_LOGI("Start");

	if ((a_u4Position > g_u4AF_MACRO) || (a_u4Position < g_u4AF_INF)) {
		AW_LOGE("Target position out of range");
		return -EINVAL;
	}

	if (*g_pAF_Opened == 1) {
		AF_init();
		spin_lock(g_pAF_SpinLock);
		*g_pAF_Opened = 2;
		spin_unlock(g_pAF_SpinLock);
	} else if (*g_pAF_Opened == 0) {
		AW_LOGE("Please open device");
		return -EIO;
	}

	if (*g_pAF_Opened == 2) {
		if (SetPos(a_u4Position) == 0) {
			spin_lock(g_pAF_SpinLock);
			g_u4CurrPosition = a_u4Position;
			spin_unlock(g_pAF_SpinLock);
		} else {
			AW_LOGE("Move vcm error!");
			return -EAGAIN;
		}
	}

	return 0;
}

static inline int AW86006VCMYOVAAF_SetMacro(unsigned long a_u4Param)
{
	AW_LOGI("Start");

	spin_lock(g_pAF_SpinLock);
	g_u4AF_MACRO = a_u4Param;
	spin_unlock(g_pAF_SpinLock);

	AW_LOGD("g_u4AF_MACRO = 0x%04lx", g_u4AF_MACRO);

	return 0;
}

static inline int AW86006VCMYOVAAF_SetInf(unsigned long a_u4Param)
{
	AW_LOGI("Start");

	spin_lock(g_pAF_SpinLock);
	g_u4AF_INF = a_u4Param;
	spin_unlock(g_pAF_SpinLock);

	AW_LOGD("g_u4AF_INF = 0x%04lx", g_u4AF_INF);

	return 0;
}

static inline int AW86006VCMYOVAAF_GetVCMInfo(
				__user struct stAF_MotorInfo *pstMotorInfo)
{
	struct stAF_MotorInfo stMotorInfo;

	AW_LOGI("Start");

	if (pstMotorInfo == NULL) {
		AW_LOGE("struct pointer pstMotorInfo error: NULL");
		return -EPERM;
	}

	stMotorInfo.u4MacroPosition = g_u4AF_MACRO;
	stMotorInfo.u4InfPosition = g_u4AF_INF;
	stMotorInfo.u4CurrentPosition = g_u4CurrPosition;
	stMotorInfo.bIsSupportSR = 1;

	stMotorInfo.bIsMotorMoving = 1;

	if (*g_pAF_Opened >= 1)
		stMotorInfo.bIsMotorOpen = 1;
	else
		stMotorInfo.bIsMotorOpen = 0;

	if (copy_to_user(pstMotorInfo, &stMotorInfo,
						sizeof(struct stAF_MotorInfo)))
		AW_LOGE("Get VCM information error!");

	/* debug info */
	AW_LOGD("u4MacroPosition=0x%04x", stMotorInfo.u4MacroPosition);
	AW_LOGD("u4InfPosition=0x%04x", stMotorInfo.u4InfPosition);
	AW_LOGD("u4CurrentPosition=0x%04x", stMotorInfo.u4CurrentPosition);
	AW_LOGD("bIsSupportSR=%d", stMotorInfo.bIsSupportSR);
	AW_LOGD("bIsMotorMoving=%d", stMotorInfo.bIsMotorMoving);
	AW_LOGD("bIsMotorOpen=%d", stMotorInfo.bIsMotorOpen);

	return 0;
}

long MOT_CANCUNF_AW86006VCMYOVAAF_Ioctl(struct file *a_pstFile, uint32_t a_u4Command,
							unsigned long a_u4Param)
{
	long ret = 0;

	void *pBuff = NULL;
	AW_LOGI("Start");

	if (_IOC_DIR(a_u4Command) == _IOC_NONE)
		return -EFAULT;

	pBuff = kzalloc(_IOC_SIZE(a_u4Command), GFP_KERNEL);
	if (pBuff == NULL)
		return -ENOMEM;
	memset(pBuff, 0, _IOC_SIZE(a_u4Command));

	/*if (_IOC_WRITE & _IOC_DIR(a_u4Command)) {
		 ret = copy_from_user(pBuff, (void *)a_u4Param, _IOC_SIZE(a_u4Command));
		//kfree(pBuff);
		AW_LOGI("ioctl copy from user failed or no need, %d, %d", (_IOC_WRITE & _IOC_DIR(a_u4Command)), ret);
		//return -EFAULT;
	}*/

	switch (a_u4Command) {
	case AFIOC_G_MOTORINFO:
		ret = AW86006VCMYOVAAF_GetVCMInfo(
				(__user struct stAF_MotorInfo *) (a_u4Param));
		break;
	case AFIOC_T_MOVETO:
		ret = AW86006VCMYOVAAF_MoveVCM(a_u4Param);
		break;
	case AFIOC_T_SETMACROPOS:
		ret = AW86006VCMYOVAAF_SetMacro(a_u4Param);
		break;
	case AFIOC_T_SETINFPOS:
		ret = AW86006VCMYOVAAF_SetInf(a_u4Param);
		break;
	case OISIOC_G_GYRO_OFFSET_CALI:
		ret = gyro_offset_cali_run((struct motOISGOffsetResult *) a_u4Param);
		break;
	case OISIOC_G_GYRO_OFFSET_SET:
		copy_from_user(pBuff, (void *)a_u4Param, _IOC_SIZE(a_u4Command));
		ret = gyro_offset_cali_set((struct motOISGOffsetResult *) pBuff);
		break;
	case OISIOC_G_HEA:
		copy_from_user(pBuff, (void *)a_u4Param, _IOC_SIZE(a_u4Command));
		hea_test_runing = 1;
		ret = run_aw86006ois_drawcircle((motOISHeaParam *)pBuff);
		hea_test_runing = 0;
		break;
	case OISIOC_T_OISMODE:
		copy_from_user(pBuff, (void *)a_u4Param, _IOC_SIZE(a_u4Command));
		ret = aw86006_set_ois_mode( *((uint8_t *)pBuff));
		break;
	default:
		AW_LOGE("NO CMD!\n");
		ret = -EPERM;
		break;
	}

	AW_LOGI("End");

	kfree(pBuff);
	return ret;
}

/* Main jobs: */
/* 1.Deallocate anything that "open" allocated in private_data. */
/* 2.Shut down the device on last close. */
/* 3.Only called once on last time. */
/* Q1 : Try release multiple times. */
int MOT_CANCUNF_AW86006VCMYOVAAF_Release(struct inode *a_pstInode, struct file *a_pstFile)
{
	AW_LOGI("Start");

	if (*g_pAF_Opened == 2) {
		/* AF restore init position */
		unsigned long init_pos;

		init_pos = (unsigned long)((INIT_POS_H << 8) | INIT_POS_L);
		SetPos(init_pos);
	}

	if (*g_pAF_Opened) {
		AW_LOGD("Free");
		spin_lock(g_pAF_SpinLock);
		*g_pAF_Opened = 0;
		spin_unlock(g_pAF_SpinLock);
	}

	AW_LOGI("End");

	return 0;
}

int MOT_CANCUNF_AW86006VCMYOVAAF_PowerDown(struct i2c_client *pstAF_I2Cclient, int *pAF_Opened)
{
	AW_LOGI("Start");

	if ((pstAF_I2Cclient == NULL) || (pAF_Opened == NULL)) {
		AW_LOGE("struct pointer error: NULL");
		return -EPERM;
	}

	g_pstAF_I2Cclient = pstAF_I2Cclient;
	g_pAF_Opened = pAF_Opened;

	if (*g_pAF_Opened > 0)
		*g_pAF_Opened = 0;

	AW_LOGD("g_pAF_Opened = %d", *g_pAF_Opened);
	AW_LOGD("Set PowerDown mode OK!");

	return 0;
}

int MOT_CANCUNF_AW86006VCMYOVAAF_SetI2Cclient(struct i2c_client *pstAF_I2Cclient,
				spinlock_t *pAF_SpinLock, int *pAF_Opened)
{
	AW_LOGI("Start");

	if ((pstAF_I2Cclient == NULL) || (pAF_SpinLock == NULL) ||
							(pAF_Opened == NULL)) {
		AW_LOGE("struct pointer error: NULL");
		return -EPERM;
	}

	g_pstAF_I2Cclient = pstAF_I2Cclient;
	g_pAF_SpinLock = pAF_SpinLock;
	g_pAF_Opened = pAF_Opened;

	spin_lock(g_pAF_SpinLock);
	g_pstAF_I2Cclient->addr = SOC_OIS_I2C_ADDR;
	spin_unlock(g_pAF_SpinLock);

	AW_LOGI("End");

	return 1;
}

int MOT_CANCUNF_AW86006VCMYOVAAF_GetFileName(uint8_t *pFileName)
{
	#if SUPPORT_GETTING_LENS_FOLDER_NAME
	char FilePath[256];
	char *FileString;
	sprintf(FilePath, "%s", __FILE__);
	FileString = strrchr(FilePath, '/');
	*FileString = '\0';
	FileString = (strrchr(FilePath, '/') + 1);
	strncpy(pFileName, FileString, AF_MOTOR_NAME);
	AW_LOGD("FileName : %s", pFileName);
	#else
	pFileName[0] = '\0';
	#endif
	return 1;
}

MODULE_AUTHOR("<liangqing@awinic.com>");
MODULE_DESCRIPTION("AW86006VCMYOVAAF Driver");
MODULE_LICENSE("GPL v2");

