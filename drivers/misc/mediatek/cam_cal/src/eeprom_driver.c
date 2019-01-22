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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/of.h>
#include "cam_cal.h"
#include "cam_cal_define.h"
#include "cam_cal_list.h"
/*#include <asm/system.h>  // for SM*/
#include <linux/dma-mapping.h>
#ifdef CONFIG_COMPAT
/* 64 bit */
#include <linux/fs.h>
#include <linux/compat.h>
#endif

/*#define CAM_CALGETDLT_DEBUG*/
#define CAM_CAL_DEBUG
#define PFX "CAM_CAL_DRV"
#ifdef CAM_CAL_DEBUG
/*#include <linux/log.h>*/
#define PK_INF(format, args...)     pr_debug(PFX "[%s] " format, __func__, ##args)
#define PK_DBG(format, args...)     pr_debug(PFX "[%s] " format, __func__, ##args)
#define PK_ERR(format, args...)     pr_debug(PFX "[%s] " format, __func__, ##args)
#else
#define PK_INF(format, args...)
#define PK_DBG(format, args...)
#define PK_ERR(format, args...)     pr_debug(PFX "[%s] " format, __func__, ##args)
#endif

#define CAM_CAL_DRV_NAME "CAM_CAL_DRV"
#define CAM_CAL_DEV_MAJOR_NUMBER 226

#define CAM_CAL_I2C_MAX_BUSNUM 2
#define CAM_CAL_I2C_MAX_SENSOR 4
#define CAM_CAL_MAX_BUF_SIZE 65536/*For Safety, Can Be Adjested*/

#define CAM_CAL_I2C_DEV1_NAME CAM_CAL_DRV_NAME
#define CAM_CAL_I2C_DEV2_NAME "CAM_CAL_DEV2"
#define CAM_CAL_I2C_DEV3_NAME "CAM_CAL_DEV3"
#define CAM_CAL_I2C_DEV4_NAME "CAM_CAL_DEV4"

static struct i2c_board_info i2cDev1 = { I2C_BOARD_INFO(CAM_CAL_I2C_DEV1_NAME, 0xA2 >> 1)};
static struct i2c_board_info i2cDev2 = { I2C_BOARD_INFO(CAM_CAL_I2C_DEV2_NAME, 0xA2 >> 1)};
static struct i2c_board_info i2cDev3 = { I2C_BOARD_INFO(CAM_CAL_I2C_DEV3_NAME, 0xA2 >> 1)};
static struct i2c_board_info i2cDev4 = { I2C_BOARD_INFO(CAM_CAL_I2C_DEV4_NAME, 0xA2 >> 1)};

static dev_t g_devNum = MKDEV(CAM_CAL_DEV_MAJOR_NUMBER, 0);
static struct cdev *g_charDrv;
static struct class *g_drvClass;
static unsigned int g_drvOpened;
static struct i2c_client *g_pstI2Cclient;
static struct i2c_client *g_pstI2Cclient2;




static DEFINE_SPINLOCK(g_spinLock); /*for SMP*/

typedef enum {
	I2C_DEV_1 = 0,
	I2C_DEV_2,
	I2C_DEV_3,
	I2C_DEV_4,
	I2C_DEV_MAX,
} CAM_CAL_DEV_ID;

typedef CAM_CAL_DEV_ID cam_cal_dev_id;

static struct i2c_board_info *g_i2c_info[I2C_DEV_MAX] = {&i2cDev1, &i2cDev2, &i2cDev3, &i2cDev4};
static cam_cal_dev_id g_curDevIdx = I2C_DEV_1;

typedef enum {
	BUS_ID_MAIN = 0,
	BUS_ID_SUB,
	BUS_ID_MAIN2,
	BUS_ID_SUB2,
	BUS_ID_MAX,
} CAM_CAL_BUS_ID;

typedef CAM_CAL_BUS_ID cam_cal_bus_id;

static unsigned int g_busNum[BUS_ID_MAX] = {0, 0, 0, 0};
static cam_cal_bus_id g_curBusIdx = BUS_ID_MAIN;
static struct i2c_client *g_Current_Client;


/*Note: Must Mapping to IHalSensor.h*/
enum {
	SENSOR_DEV_NONE = 0x00,
	SENSOR_DEV_MAIN = 0x01,
	SENSOR_DEV_SUB  = 0x02,
	SENSOR_DEV_PIP = 0x03,
	SENSOR_DEV_MAIN_2 = 0x04,
	SENSOR_DEV_MAIN_3D = 0x05,
	SENSOR_DEV_SUB_2 = 0x08,
	SENSOR_DEV_MAX = 0x50
};

static unsigned int g_lastDevID = SENSOR_DEV_NONE;

/*******************************************************************************
*
********************************************************************************/
typedef struct {
	unsigned int sensorID;
	unsigned int deviceID;
	struct i2c_client *client;
	cam_cal_cmd_func readCMDFunc;
	cam_cal_cmd_func writeCMDFunc;
} stCAM_CAL_CMD_INFO_STRUCT, *stPCAM_CAL_CMD_INFO_STRUCT;

static stCAM_CAL_CMD_INFO_STRUCT g_camCalDrvInfo[CAM_CAL_I2C_MAX_SENSOR];

/*******************************************************************************
*
********************************************************************************/

static int EEPROM_set_i2c_bus(unsigned int deviceID)
{
	switch (deviceID) {
	case SENSOR_DEV_MAIN:
		g_curBusIdx = BUS_ID_MAIN;
		g_curDevIdx = I2C_DEV_1;
		g_Current_Client = g_pstI2Cclient;
		break;
	case SENSOR_DEV_SUB:
		g_curBusIdx = BUS_ID_SUB;
		g_curDevIdx = I2C_DEV_2;
		g_Current_Client = g_pstI2Cclient2;
		break;
	case SENSOR_DEV_MAIN_2:
		g_curBusIdx = BUS_ID_MAIN2;
		g_curDevIdx = I2C_DEV_3;
		g_Current_Client = g_pstI2Cclient2;
		break;
	case SENSOR_DEV_SUB_2:
		g_curBusIdx = BUS_ID_SUB2;
		g_curDevIdx = I2C_DEV_4;
		g_Current_Client = g_pstI2Cclient;
		break;
	default:
		return 1;
	}
	PK_DBG("EEPROM_set_i2c_bus end! deviceID=%d g_curBusIdx=%d g_curDevIdx=%d\n",
		deviceID, g_curBusIdx, g_curDevIdx);

	return 0;

}


static int EEPROM_get_cmd_info(unsigned int sensorID, stCAM_CAL_CMD_INFO_STRUCT *cmdInfo)
{
	stCAM_CAL_LIST_STRUCT *pCamCalList = NULL;
	stCAM_CAL_FUNC_STRUCT *pCamCalFunc = NULL;
	int i = 0;

	cam_cal_get_sensor_list(&pCamCalList);
	cam_cal_get_func_list(&pCamCalFunc);
	if (pCamCalList != NULL && pCamCalFunc != NULL) {
		PK_DBG("pCamCalList!=NULL && pCamCalFunc!= NULL\n");
		for (i = 0; pCamCalList[i].sensorID != 0; i++) {
			if (pCamCalList[i].sensorID == sensorID) {
				(*g_i2c_info[g_curDevIdx]).addr = pCamCalList[i].slaveID >> 1;
				cmdInfo->client = g_Current_Client;

				PK_DBG("pCamCalList[%d].sensorID==%x\n", i, pCamCalList[i].sensorID);
				PK_DBG("g_i2c_info[%d].addr =%x\n", g_curDevIdx,
					  (*g_i2c_info[g_curDevIdx]).addr);
				PK_DBG("20 g_client =%p g_client2=%p Cur=%p\n",
					g_pstI2Cclient, g_pstI2Cclient2, g_Current_Client);

				if (pCamCalList[i].checkFunc(cmdInfo->client, pCamCalFunc[0].readCamCalData)) {
					PK_DBG("pCamCalList[%d].checkFunc ok!\n", i);
					cmdInfo->readCMDFunc = pCamCalFunc[0].readCamCalData;
					return 1;
				}
			}
		}
	}
	return 0;

}

static stCAM_CAL_CMD_INFO_STRUCT *EEPROM_get_cmd_info_ex(unsigned int sensorID,
	unsigned int deviceID)
{
	int i = 0;

	/* To check device ID */
	for (i = 0; i < CAM_CAL_I2C_MAX_SENSOR; i++) {
		if (g_camCalDrvInfo[i].deviceID == deviceID) {
			PK_DBG("g_camCalDrvInfo[%d].deviceID == deviceID == %x!\n", i, deviceID);
			break;
		}
	}
	/* To check cmd from Sensor ID */

	if (i == CAM_CAL_I2C_MAX_SENSOR) {
		for (i = 0; i < CAM_CAL_I2C_MAX_SENSOR; i++) {
			/* To Set Client */
			if (g_camCalDrvInfo[i].sensorID == 0) {
				PK_DBG("g_camCalDrvInfo[%d].sensorID == 0, start get_cmd_info!\n", i);
				EEPROM_get_cmd_info(sensorID, &g_camCalDrvInfo[i]);

				if (g_camCalDrvInfo[i].readCMDFunc != NULL) {
					g_camCalDrvInfo[i].sensorID = sensorID;
					g_camCalDrvInfo[i].deviceID = deviceID;
					PK_DBG("deviceID=%d, SensorID=%x, BusID=%d\n",
						deviceID, sensorID, g_busNum[g_curBusIdx]);
				}
				break;
			}
		}
	}

	if (i == CAM_CAL_I2C_MAX_SENSOR) { /*g_camCalDrvInfo is full*/
		return NULL;
	} else {
		return &g_camCalDrvInfo[i];
	}
}

/*******************************************************************************
* EEPROM_HW_i2c_probe
********************************************************************************/
static int EEPROM_HW_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    /* get sensor i2c client */
	spin_lock(&g_spinLock);
	g_pstI2Cclient = client;

    /* set I2C clock rate */
    #ifdef CONFIG_MTK_I2C_EXTENSION
	g_pstI2Cclient->timing = 100;/* 100k */
	g_pstI2Cclient->ext_flag &= ~I2C_POLLING_FLAG; /* No I2C polling busy waiting */
    #endif

	/* Default EEPROM Slave Address*/
	g_pstI2Cclient->addr = 0x50;
	spin_unlock(&g_spinLock);

	return 0;
}



/*******************************************************************************
* CAMERA_HW_i2c_remove
********************************************************************************/
static int EEPROM_HW_i2c_remove(struct i2c_client *client)
{
	return 0;
}

/*******************************************************************************
* EEPROM_HW_i2c_probe2
********************************************************************************/
static int EEPROM_HW_i2c_probe2(struct i2c_client *client, const struct i2c_device_id *id)
{
    /* get sensor i2c client */
	spin_lock(&g_spinLock);
	g_pstI2Cclient2 = client;

    /* set I2C clock rate */
    #ifdef CONFIG_MTK_I2C_EXTENSION
	g_pstI2Cclient2->timing = 100;/* 100k */
	g_pstI2Cclient2->ext_flag &= ~I2C_POLLING_FLAG; /* No I2C polling busy waiting */
    #endif

	/* Default EEPROM Slave Address*/
	g_pstI2Cclient2->addr = 0x50;
	spin_unlock(&g_spinLock);

	return 0;
}

/*******************************************************************************
* CAMERA_HW_i2c_remove2
********************************************************************************/
static int EEPROM_HW_i2c_remove2(struct i2c_client *client)
{
	return 0;
}

/*******************************************************************************
* I2C related variable
********************************************************************************/

static const struct i2c_device_id EEPROM_HW_i2c_id[] = {{CAM_CAL_DRV_NAME, 0}, {} };
static const struct i2c_device_id EEPROM_HW_i2c_id2[] = {{CAM_CAL_I2C_DEV2_NAME, 0}, {} };



#ifdef CONFIG_OF
	static const struct of_device_id EEPROM_HW_i2c_of_ids[] = {
	{ .compatible = "mediatek,camera_main_eeprom", },
	{}
	};
#endif

struct i2c_driver EEPROM_HW_i2c_driver = {
	.probe = EEPROM_HW_i2c_probe,
	.remove = EEPROM_HW_i2c_remove,
	.driver = {
	.name = CAM_CAL_DRV_NAME,
	.owner = THIS_MODULE,

#ifdef CONFIG_OF
	.of_match_table = EEPROM_HW_i2c_of_ids,
#endif
	},
	.id_table = EEPROM_HW_i2c_id,
};

/*******************************************************************************
* I2C Driver structure for Sub
********************************************************************************/
#ifdef CONFIG_OF
	static const struct of_device_id EEPROM_HW2_i2c_driver_of_ids[] = {
	{ .compatible = "mediatek,camera_sub_eeprom", },
	{}
	};
#endif

struct i2c_driver EEPROM_HW_i2c_driver2 = {
	.probe = EEPROM_HW_i2c_probe2,
	.remove = EEPROM_HW_i2c_remove2,
	.driver = {
	.name = CAM_CAL_I2C_DEV2_NAME,
	.owner = THIS_MODULE,
#ifdef CONFIG_OF
	.of_match_table = EEPROM_HW2_i2c_driver_of_ids,
#endif
	},
	.id_table = EEPROM_HW_i2c_id2,
};


/*******************************************************************************
* EEPROM_HW_probe
********************************************************************************/
static int EEPROM_HW_probe(struct platform_device *pdev)
{
	i2c_add_driver(&EEPROM_HW_i2c_driver2);
	return i2c_add_driver(&EEPROM_HW_i2c_driver);
}
/*******************************************************************************
* EEPROM_HW_remove()
********************************************************************************/
static int EEPROM_HW_remove(struct platform_device *pdev)
{
	i2c_del_driver(&EEPROM_HW_i2c_driver);
	i2c_del_driver(&EEPROM_HW_i2c_driver2);
	return 0;
}

/*******************************************************************************
*
********************************************************************************/
static struct platform_device g_platDev = {
	.name = CAM_CAL_DRV_NAME,
	.id = 0,
	.dev = {
	}
};


static struct platform_driver g_stEEPROM_HW_Driver = {
	.probe      = EEPROM_HW_probe,
	.remove     = EEPROM_HW_remove,
	.driver     = {
		.name   = CAM_CAL_DRV_NAME,
		.owner  = THIS_MODULE,
	}
};


/********************************************************************/

#ifdef CONFIG_COMPAT
static int compat_put_cal_info_struct(
	COMPAT_stCAM_CAL_INFO_STRUCT __user *data32,
	stCAM_CAL_INFO_STRUCT __user *data)
{
	compat_uptr_t p;
	compat_uint_t i;
	int err;

	err = get_user(i, &data->u4Offset);
	err |= put_user(i, &data32->u4Offset);
	err |= get_user(i, &data->u4Length);
	err |= put_user(i, &data32->u4Length);
	err |= get_user(i, &data->sensorID);
	err |= put_user(i, &data32->sensorID);
	err |= get_user(i, &data->deviceID);
	err |= put_user(i, &data32->deviceID);

	/* Assume pointer is not change */
#if 1
	err |= get_user(p, (compat_uptr_t *)&data->pu1Params);
	err |= put_user(p, &data32->pu1Params);
#endif
	return err;
}

static int EEPROM_compat_get_info(
	COMPAT_stCAM_CAL_INFO_STRUCT __user *data32,
	stCAM_CAL_INFO_STRUCT __user *data)
{
	compat_uptr_t p;
	compat_uint_t i;
	int err;

	err = get_user(i, &data32->u4Offset);
	err |= put_user(i, &data->u4Offset);
	err |= get_user(i, &data32->u4Length);
	err |= put_user(i, &data->u4Length);
	err |= get_user(i, &data32->sensorID);
	err |= put_user(i, &data->sensorID);
	err |= get_user(i, &data32->deviceID);
	err |= put_user(i, &data->deviceID);

	err |= get_user(p, &data32->pu1Params);
	err |= put_user(compat_ptr(p), &data->pu1Params);

	return err;
}

static long EEPROM_drv_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long ret;

	COMPAT_stCAM_CAL_INFO_STRUCT __user *data32;
	stCAM_CAL_INFO_STRUCT __user *data;
	int err;

	if (!filp->f_op || !filp->f_op->unlocked_ioctl)
		return -ENOTTY;

	switch (cmd) {

	case COMPAT_CAM_CALIOC_G_READ: {
		data32 = compat_ptr(arg);
		data = compat_alloc_user_space(sizeof(*data));
		if (data == NULL)
			return -EFAULT;

		err = EEPROM_compat_get_info(data32, data);
		if (err)
			return err;

		ret = filp->f_op->unlocked_ioctl(filp, CAM_CALIOC_G_READ,
						 (unsigned long)data);
		err = compat_put_cal_info_struct(data32, data);

		if (err != 0)
			PK_ERR("compat_put_acdk_sensor_getinfo_struct failed\n");

		return ret;
	}

	case COMPAT_CAM_CALIOC_S_WRITE: {/*Note: Write Command is Unverified!*/
		data32 = compat_ptr(arg);
		data = compat_alloc_user_space(sizeof(*data));
		if (data == NULL)
			return -EFAULT;

		err = EEPROM_compat_get_info(data32, data);
		if (err)
			return err;

		ret = filp->f_op->unlocked_ioctl(filp, CAM_CALIOC_S_WRITE,
						 (unsigned long)data);
		/*err = compat_put_cal_info_struct(data32, data);*/
		/*151122=write won't need*/

		if (err != 0)
			PK_ERR("compat_put_acdk_sensor_getinfo_struct failed\n");

		return ret;
	}
	default:
		return -ENOIOCTLCMD;
	}

}

#endif

#define NEW_UNLOCK_IOCTL
#ifndef NEW_UNLOCK_IOCTL
static int EEPROM_drv_ioctl(struct inode *a_pstInode,
			     struct file *a_pstFile,
			     unsigned int a_u4Command,
			     unsigned long a_u4Param)
#else
static long EEPROM_drv_ioctl(
	struct file *file,
	unsigned int a_u4Command,
	unsigned long a_u4Param
)
#endif
{

	int i4RetValue = 0;
	u8 *pBuff = NULL;
	u8 *pu1Params = NULL;
	/*u8 *tempP = NULL;*/
	stCAM_CAL_INFO_STRUCT *ptempbuf = NULL;
	stCAM_CAL_CMD_INFO_STRUCT *pcmdInf = NULL;

#ifdef CAM_CALGETDLT_DEBUG
	struct timeval ktv1, ktv2;
	unsigned long TimeIntervalUS;
#endif
	if (_IOC_DIR(a_u4Command) != _IOC_NONE) {
		pBuff = kmalloc(sizeof(stCAM_CAL_INFO_STRUCT), GFP_KERNEL);
		if (pBuff == NULL) {
			PK_DBG(" ioctl allocate pBuff mem failed\n");
			return -ENOMEM;
		}

		if (copy_from_user((u8 *) pBuff, (u8 *) a_u4Param, sizeof(stCAM_CAL_INFO_STRUCT))) {
			/*get input structure address*/
			kfree(pBuff);
			PK_DBG("ioctl copy from user failed\n");
			return -EFAULT;
		}

		ptempbuf = (stCAM_CAL_INFO_STRUCT *)pBuff;

		if ((ptempbuf->u4Length <= 0) || (ptempbuf->u4Length > CAM_CAL_MAX_BUF_SIZE)) {
			kfree(pBuff);
			PK_DBG("Buffer Length Error!\n");
			return -EFAULT;
		}

		pu1Params = kmalloc(ptempbuf->u4Length, GFP_KERNEL);

		if (pu1Params == NULL) {
			kfree(pBuff);
			PK_DBG("ioctl allocate pu1Params mem failed\n");
			return -ENOMEM;
		}

		if (copy_from_user((u8 *)pu1Params, (u8 *)ptempbuf->pu1Params, ptempbuf->u4Length)) {
			kfree(pBuff);
			kfree(pu1Params);
			PK_DBG("ioctl copy from user failed\n");
			return -EFAULT;
		}
	}
	if (ptempbuf == NULL) { /*It have to add */
		PK_DBG("ptempbuf is Null !!!");
		return -EFAULT;
	}
	switch (a_u4Command) {

	case CAM_CALIOC_S_WRITE:/*Note: Write Command is Unverified!*/
		PK_DBG("CAM_CALIOC_S_WRITE start!\n");
#ifdef CAM_CALGETDLT_DEBUG
		do_gettimeofday(&ktv1);
#endif

	if (g_lastDevID != ptempbuf->deviceID) {
		g_lastDevID = ptempbuf->deviceID;
		if (EEPROM_set_i2c_bus(ptempbuf->deviceID)) {
			PK_DBG("deviceID Error!\n");
			kfree(pBuff);
			kfree(pu1Params);
			return -EFAULT;
		}
	}

	pcmdInf = EEPROM_get_cmd_info_ex(ptempbuf->sensorID, ptempbuf->deviceID);

	if (pcmdInf != NULL) {
		if (pcmdInf->writeCMDFunc != NULL) {
			i4RetValue = pcmdInf->writeCMDFunc(pcmdInf->client,
				ptempbuf->u4Offset, pu1Params, ptempbuf->u4Length);
		} else
			PK_DBG("pcmdInf->writeCMDFunc == NULL\n");
	} else
		PK_DBG("pcmdInf == NULL\n");

#ifdef CAM_CALGETDLT_DEBUG
		do_gettimeofday(&ktv2);
		if (ktv2.tv_sec > ktv1.tv_sec)
			TimeIntervalUS = ktv1.tv_usec + 1000000 - ktv2.tv_usec;
		else
			TimeIntervalUS = ktv2.tv_usec - ktv1.tv_usec;

		PK_DBG("Write data %d bytes take %lu us\n", ptempbuf->u4Length, TimeIntervalUS);
#endif
		PK_DBG("CAM_CALIOC_S_WRITE End!\n");
		break;

	case CAM_CALIOC_G_READ:
		PK_DBG("CAM_CALIOC_G_READ start! offset=%d, length=%d\n", ptempbuf->u4Offset,
			  ptempbuf->u4Length);

#ifdef CAM_CALGETDLT_DEBUG
		do_gettimeofday(&ktv1);
#endif

		if (g_lastDevID != ptempbuf->deviceID) {
			g_lastDevID = ptempbuf->deviceID;
			EEPROM_set_i2c_bus(ptempbuf->deviceID);
		}
		PK_DBG("SensorID=%x DeviceID=%x\n", ptempbuf->sensorID, ptempbuf->deviceID);
		pcmdInf = EEPROM_get_cmd_info_ex(ptempbuf->sensorID, ptempbuf->deviceID);

		if (pcmdInf != NULL) {
			if (pcmdInf->readCMDFunc != NULL)
				i4RetValue = pcmdInf->readCMDFunc(pcmdInf->client,
					ptempbuf->u4Offset, pu1Params, ptempbuf->u4Length);
			else {
				PK_DBG("pcmdInf->readCMDFunc == NULL\n");
			}
		}

#ifdef CAM_CALGETDLT_DEBUG
		do_gettimeofday(&ktv2);
		if (ktv2.tv_sec > ktv1.tv_sec)
			TimeIntervalUS = ktv1.tv_usec + 1000000 - ktv2.tv_usec;
		else
			TimeIntervalUS = ktv2.tv_usec - ktv1.tv_usec;

		PK_DBG("Read data %d bytes take %lu us\n", ptempbuf->u4Length, TimeIntervalUS);
#endif
		break;

	default:
		PK_DBG("No CMD\n");
		i4RetValue = -EPERM;
		break;
	}

	if (_IOC_READ & _IOC_DIR(a_u4Command)) {
		/*copy data to user space buffer, keep other input paremeter unchange.*/
		if (copy_to_user((u8 __user *) ptempbuf->pu1Params, (u8 *)pu1Params, ptempbuf->u4Length)) {
			kfree(pBuff);
			kfree(pu1Params);
			PK_DBG("ioctl copy to user failed\n");
			return -EFAULT;
		}
	}

	kfree(pBuff);
	kfree(pu1Params);
	return i4RetValue;
}

static int EEPROM_drv_open(struct inode *a_pstInode, struct file *a_pstFile)
{
	int ret = 0;

	PK_DBG("EEPROM_drv_open start\n");
	spin_lock(&g_spinLock);
	if (g_drvOpened) {
		spin_unlock(&g_spinLock);
		PK_DBG("Opened, return -EBUSY\n");
		ret = -EBUSY;
	} else {
		g_drvOpened = 1;
		spin_unlock(&g_spinLock);
	}
	mdelay(2);

	return ret;
}

static int EEPROM_drv_release(struct inode *a_pstInode, struct file *a_pstFile)
{
	spin_lock(&g_spinLock);
	g_drvOpened = 0;
	spin_unlock(&g_spinLock);

	return 0;
}

static const struct file_operations g_stCAM_CAL_fops1 = {
	.owner = THIS_MODULE,
	.open = EEPROM_drv_open,
	.release = EEPROM_drv_release,
	/*.ioctl = CAM_CAL_Ioctl*/
#ifdef CONFIG_COMPAT
	.compat_ioctl = EEPROM_drv_compat_ioctl,
#endif
	.unlocked_ioctl = EEPROM_drv_ioctl
};

/*******************************************************************************
*
********************************************************************************/

#define CAM_CAL_DYNAMIC_ALLOCATE_DEVNO 1
static inline int EEPROM_chrdev_register(void)
{
	struct device *device = NULL;

	PK_DBG("EEPROM_chrdev_register Start\n");

#if CAM_CAL_DYNAMIC_ALLOCATE_DEVNO
	if (alloc_chrdev_region(&g_devNum, 0, 1, CAM_CAL_DRV_NAME)) {
		PK_DBG("Allocate device no failed\n");
		return -EAGAIN;
	}
#else
	if (register_chrdev_region(g_devNum, 1, CAM_CAL_DRV_NAME)) {
		PK_DBG("Register device no failed\n");
		return -EAGAIN;
	}
#endif

	g_charDrv = cdev_alloc();

	if (g_charDrv == NULL) {
		unregister_chrdev_region(g_devNum, 1);
		PK_DBG("Allocate mem for kobject failed\n");
		return -ENOMEM;
	}

	cdev_init(g_charDrv, &g_stCAM_CAL_fops1);
	g_charDrv->owner = THIS_MODULE;

	if (cdev_add(g_charDrv, g_devNum, 1)) {
		PK_DBG("Attatch file operation failed\n");
		unregister_chrdev_region(g_devNum, 1);
		return -EAGAIN;
	}

	g_drvClass = class_create(THIS_MODULE, "CAM_CALdrv1");
	if (IS_ERR(g_drvClass)) {
		int ret = PTR_ERR(g_drvClass);

		PK_DBG("Unable to create class, err = %d\n", ret);
		return ret;
	}
	device = device_create(g_drvClass, NULL, g_devNum, NULL, CAM_CAL_DRV_NAME);
	PK_DBG("EEPROM_chrdev_register End\n");

	return 0;
}

static void EEPROM_chrdev_unregister(void)
{
	/*Release char driver*/

	class_destroy(g_drvClass);

	device_destroy(g_drvClass, g_devNum);

	cdev_del(g_charDrv);

	unregister_chrdev_region(g_devNum, 1);
}

/*******************************************************************************
*
********************************************************************************/

static int __init EEPROM_drv_init(void)
{
	PK_DBG("EEPROM_drv_init Start!\n");

	if (platform_driver_register(&g_stEEPROM_HW_Driver)) {
		PK_ERR("failed to register EEPROM driver i2C main\n");
		return -ENODEV;
	}

	if (platform_device_register(&g_platDev)) {
		PK_ERR("failed to register EEPROM device");
		return -ENODEV;
	}

	EEPROM_chrdev_register();

	PK_DBG("EEPROM_drv_init End!\n");
	return 0;
}

static void __exit EEPROM_drv_exit(void)
{

	platform_device_unregister(&g_platDev);
	platform_driver_unregister(&g_stEEPROM_HW_Driver);

	EEPROM_chrdev_unregister();
}

module_init(EEPROM_drv_init);
module_exit(EEPROM_drv_exit);

MODULE_DESCRIPTION("EEPROM Driver");
MODULE_AUTHOR("MM3_SW2");
MODULE_LICENSE("GPL");


