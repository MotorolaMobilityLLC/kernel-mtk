// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

/*
 * MOT_NAPLES_AK7377AF voice coil motor driver
 *
 *
 */

#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>

#include "lens_info.h"

#define AF_DRVNAME "MOT_NAPLES_AK7377AF_DRV"
#define AF_I2C_SLAVE_ADDR 0x18

#define AF_DEBUG
#ifdef AF_DEBUG
#define LOG_INF(format, args...)                                               \
	pr_debug(AF_DRVNAME " [%s] " format, __func__, ##args)
#else
#define LOG_INF(format, args...)
#endif

#if IS_ENABLED (CONFIG_MOT_DRV_AK7377_HALL_TEST)
typedef struct {
       int max_val;
       int min_val;
} motAfTestData;

static int gethall_mode = -1;
#define AF_HALL_I2C_ADDR 0x84
#define AFIOC_G_AFPOS _IOWR('A', 35, int)
#endif

static struct i2c_client *g_pstAF_I2Cclient;
static int *g_pAF_Opened;
static spinlock_t *g_pAF_SpinLock;

static unsigned long g_u4AF_INF;
static unsigned long g_u4AF_MACRO = 1023;
static unsigned long g_u4CurrPosition;

static int s4AF_WriteReg(u16 a_u2Addr, u16 a_u2Data)
{
	int i4RetValue = 0;

	char puSendCmd[2] = {(char)a_u2Addr, (char)a_u2Data};

	g_pstAF_I2Cclient->addr = AF_I2C_SLAVE_ADDR;

	g_pstAF_I2Cclient->addr = g_pstAF_I2Cclient->addr >> 1;

	i4RetValue = i2c_master_send(g_pstAF_I2Cclient, puSendCmd, 2);

	if (i4RetValue < 0) {
		LOG_INF("I2C write failed!!\n");
		return -1;
	}

	return 0;
}

static inline int getAFInfo(__user struct stAF_MotorInfo *pstMotorInfo)
{
	struct stAF_MotorInfo stMotorInfo;

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
		LOG_INF("copy to user failed when getting motor information\n");

	return 0;
}

/* initAF include driver initialization and standby mode */
static int initAF(void)
{
	LOG_INF("+\n");

	if (*g_pAF_Opened == 1) {

		int ret = 0;

		/* 00:active mode , 10:Standby mode , x1:Sleep mode */
		ret = s4AF_WriteReg(0x02, 0x00);

		spin_lock(g_pAF_SpinLock);
		*g_pAF_Opened = 2;
		spin_unlock(g_pAF_SpinLock);
	}

	LOG_INF("-\n");

	return 0;
}

#if IS_ENABLED (CONFIG_MOT_DRV_AK7377_HALL_TEST)
#define AK7377A_SET_POSITION_ADDR 0x00
#define AK7377A_MOVE_DELAY_US 8400
static inline int ak7377_hall_test_set_positon(u16 pos)
{
	int retry = 3;
	int ret;
	g_pstAF_I2Cclient->addr = AF_I2C_SLAVE_ADDR;
	g_pstAF_I2Cclient->addr = g_pstAF_I2Cclient->addr >> 1;
	while (--retry > 0) {
		ret = i2c_smbus_write_word_data(g_pstAF_I2Cclient, AK7377A_SET_POSITION_ADDR,
					 swab16(pos << 4));
		if (ret < 0) {
			usleep_range(AK7377A_MOVE_DELAY_US,
				     AK7377A_MOVE_DELAY_US + 1000);
		} else {
			break;
		}
	}

	return 0;
}

static inline int ak7377_hall_test_getresult(int mode, unsigned char read_addr)
{
	// int i4RetValue = 0;
	int retry = 10;
	int ret;
	int val;
	int val_L = 0;
	int val_H = 0;
	int max_val =0;
	int min_val=0;

	msleep(2);
	while (retry > 0)
	{
		// ret = i2c_master_send(g_pstAF_I2Cclient, &read_addr, 2);
		ret = i2c_smbus_read_word_data(g_pstAF_I2Cclient, read_addr);
		val_L=(ret & 0xf000) >> 12;
		val_H = (ret & 0x00ff) << 4;
		val = val_L | val_H;
		LOG_INF("[af_hall_test] value =%d",val);
		msleep(2);
		if(mode == 0)
		{
			if(val  > max_val)
			{
				max_val=val;
			}
		}else{
			if(val  < min_val)
			{
				min_val=val;
			}
		}
		retry--;
	}
	if(mode == 0)
	{
		return max_val;
	}else{
		return min_val;
	}
}
#endif

static inline int setVCMPos(unsigned long a_u4Position)
{
	int i4RetValue = 0;

	i4RetValue = s4AF_WriteReg(0x0, (u16)((a_u4Position >> 2) & 0xff));

	if (i4RetValue < 0)
		return -1;

	i4RetValue = s4AF_WriteReg(0x1, (u16)((a_u4Position & 0x3) << 6));

	return i4RetValue;
}

/* moveAF only use to control moving the motor */
static inline int moveAF(unsigned long a_u4Position)
{
	int ret = 0;

	if (setVCMPos(a_u4Position) == 0) {
		g_u4CurrPosition = a_u4Position;
		ret = 0;
	} else {
		LOG_INF("set I2C failed when moving the motor\n");
		ret = -1;
	}

	return ret;
}

static inline int setAFInf(unsigned long a_u4Position)
{
	spin_lock(g_pAF_SpinLock);
	g_u4AF_INF = a_u4Position;
	spin_unlock(g_pAF_SpinLock);
	return 0;
}

static inline int setAFMacro(unsigned long a_u4Position)
{
	spin_lock(g_pAF_SpinLock);
	g_u4AF_MACRO = a_u4Position;
	spin_unlock(g_pAF_SpinLock);
	return 0;
}

/* ////////////////////////////////////////////////////////////// */
long MOT_NAPLES_AK7377AF_Ioctl(struct file *a_pstFile, unsigned int a_u4Command,
		    unsigned long a_u4Param)
{
	long i4RetValue = 0;
#if IS_ENABLED (CONFIG_MOT_DRV_AK7377_HALL_TEST)
	motAfTestData afdata = {0};
#endif

	switch (a_u4Command) {
	case AFIOC_G_MOTORINFO:
		i4RetValue =
			getAFInfo((__user struct stAF_MotorInfo *)(a_u4Param));
		break;

	case AFIOC_T_MOVETO:
#if IS_ENABLED (CONFIG_MOT_DRV_AK7377_HALL_TEST)
		if(gethall_mode >=0)
		break;
#endif
		i4RetValue = moveAF(a_u4Param);
		break;

	case AFIOC_T_SETINFPOS:
		i4RetValue = setAFInf(a_u4Param);
		break;

	case AFIOC_T_SETMACROPOS:
		i4RetValue = setAFMacro(a_u4Param);
		break;

#if IS_ENABLED (CONFIG_MOT_DRV_AK7377_HALL_TEST)
	case AFIOC_G_AFPOS:
		gethall_mode = 0;
		i4RetValue = ak7377_hall_test_set_positon(0x0FFF);              //4095
		afdata.max_val = ak7377_hall_test_getresult(gethall_mode, 0x84);           //mode: 0 marco
		gethall_mode = 1;
		i4RetValue = ak7377_hall_test_set_positon(0x0000);              //0
		afdata.min_val = ak7377_hall_test_getresult(gethall_mode, 0x84);           //mode: 1 inf
		gethall_mode = -1;
		i4RetValue =(afdata.max_val << 16) | afdata.min_val;
		LOG_INF("[af_hall_test] pos max=%x, min=%x, return%lx",afdata.max_val, afdata.min_val, i4RetValue);
		break;
#endif

	default:
		LOG_INF("No CMD\n");
		i4RetValue = -EPERM;
		break;
	}

	return i4RetValue;
}

/* Main jobs: */
/* 1.Deallocate anything that "open" allocated in private_data. */
/* 2.Shut down the device on last close. */
/* 3.Only called once on last time. */
/* Q1 : Try release multiple times. */
int MOT_NAPLES_AK7377AF_Release(struct inode *a_pstInode, struct file *a_pstFile)
{
	LOG_INF("Start\n");

	if (*g_pAF_Opened == 2) {
		LOG_INF("Wait\n");
		s4AF_WriteReg(0x02, 0x40);
		msleep(20);
	}

	if (*g_pAF_Opened) {
		LOG_INF("Free\n");

		spin_lock(g_pAF_SpinLock);
		*g_pAF_Opened = 0;
		spin_unlock(g_pAF_SpinLock);
	}

	LOG_INF("End\n");

	return 0;
}

int MOT_NAPLES_AK7377AF_PowerDown(struct i2c_client *pstAF_I2Cclient,
			int *pAF_Opened)
{
	g_pstAF_I2Cclient = pstAF_I2Cclient;
	g_pAF_Opened = pAF_Opened;

	LOG_INF("+\n");
	mdelay(7);
	if (*g_pAF_Opened == 0) {
		LOG_INF("Set power donw +\n");
		LOG_INF("Set power donw -\n");
	}
	LOG_INF("-\n");

	return 0;
}

int MOT_NAPLES_AK7377AF_SetI2Cclient(struct i2c_client *pstAF_I2Cclient,
			  spinlock_t *pAF_SpinLock, int *pAF_Opened)
{
	g_pstAF_I2Cclient = pstAF_I2Cclient;
	g_pAF_SpinLock = pAF_SpinLock;
	g_pAF_Opened = pAF_Opened;

	initAF();

	return 1;
}

int MOT_NAPLES_AK7377AF_GetFileName(unsigned char *pFileName)
{
	#if SUPPORT_GETTING_LENS_FOLDER_NAME
	char FilePath[256];
	char *FileString;

	sprintf(FilePath, "%s", __FILE__);
	FileString = strrchr(FilePath, '/');
	*FileString = '\0';
	FileString = (strrchr(FilePath, '/') + 1);
	strncpy(pFileName, FileString, AF_MOTOR_NAME);
	LOG_INF("FileName : %s\n", pFileName);
	#else
	pFileName[0] = '\0';
	#endif
	return 1;
}
