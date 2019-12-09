/*
 * Copyright (C) 2018 MediaTek Inc.
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
#define PFX "CAM_CAL"
#define pr_fmt(fmt) PFX "[%s] " fmt, __func__


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
#include "eeprom_i2c_dev.h"
#include "eeprom_i2c_common_driver.h"
#include <linux/dma-mapping.h>
#ifdef CONFIG_COMPAT
/* 64 bit */
#include <linux/fs.h>
#include <linux/compat.h>
#endif

#include "kd_imgsensor.h"

#define CAM_CAL_DRV_NAME "CAM_CAL_DRV"
#define CAM_CAL_DEV_MAJOR_NUMBER 226

#define CAM_CAL_MAX_BUF_SIZE 65536	/*For Safety, Can Be Adjested */

#define CAM_CAL_I2C_DEV1_NAME CAM_CAL_DRV_NAME
#define CAM_CAL_I2C_DEV2_NAME "CAM_CAL_DEV2"
#define CAM_CAL_I2C_DEV3_NAME "CAM_CAL_DEV3"
#define CAM_CAL_I2C_DEV4_NAME "CAM_CAL_DEV4"

static dev_t g_devNum = MKDEV(CAM_CAL_DEV_MAJOR_NUMBER, 0);
static struct cdev *g_charDrv;
static struct class *g_drvClass;
static unsigned int g_drvOpened;
static struct i2c_client *g_pstI2Cclients[I2C_DEV_IDX_MAX] = { NULL };
static struct class *ontim_dual_cam_otp_class;
u32 dual_main_sensorid = 0;


static DEFINE_SPINLOCK(g_spinLock);	/*for SMP */


/*Note: Must Mapping to IHalSensor.h*/
enum {
	SENSOR_DEV_NONE = 0x00,
	SENSOR_DEV_MAIN = 0x01,
	SENSOR_DEV_SUB = 0x02,
	SENSOR_DEV_PIP = 0x03,
	SENSOR_DEV_MAIN_2 = 0x04,
	SENSOR_DEV_MAIN_3D = 0x05,
	SENSOR_DEV_SUB_2 = 0x08,
	SENSOR_DEV_MAIN_3 = 0x10,
	SENSOR_DEV_MAX = 0x50
};

static unsigned int g_lastDevID = SENSOR_DEV_NONE;

/***********************************************************
 *
 ***********************************************************/
struct stCAM_CAL_CMD_INFO_STRUCT {
	unsigned int sensorID;
	unsigned int deviceID;
	unsigned int i2cAddr;
	struct i2c_client *client;
	cam_cal_cmd_func readCMDFunc;
	cam_cal_cmd_func writeCMDFunc;
};

static struct stCAM_CAL_CMD_INFO_STRUCT g_camCalDrvInfo[CAM_CAL_SENSOR_IDX_MAX];

static int ontim_write_dual_cam_otp_data(void);
int ontim_check_otp_data(u32  sensorid, u8 * p_buf, u32 Length);

/********************************************************************
 * EEPROM_set_i2c_bus()
 * To i2c client and slave id
 ********************************************************************/

static int EEPROM_set_i2c_bus(unsigned int deviceID,
			      struct stCAM_CAL_CMD_INFO_STRUCT *cmdInfo)
{
	enum CAM_CAL_SENSOR_IDX idx;
	struct i2c_client *client;

	switch (deviceID) {
	case SENSOR_DEV_MAIN:
		idx = CAM_CAL_SENSOR_IDX_MAIN;
		break;
	case SENSOR_DEV_SUB:
		idx = CAM_CAL_SENSOR_IDX_SUB;
		break;
	case SENSOR_DEV_MAIN_2:
		idx = CAM_CAL_SENSOR_IDX_MAIN2;
		break;
	case SENSOR_DEV_SUB_2:
		idx = CAM_CAL_SENSOR_IDX_SUB2;
		break;
	case SENSOR_DEV_MAIN_3:
		idx = CAM_CAL_SENSOR_IDX_MAIN3;
		break;
	default:
		return -EFAULT;
	}
	client = g_pstI2Cclients[get_i2c_dev_sel(idx)];
	pr_debug("%s end! deviceID=%d index=%u client=%p\n",
		 __func__, deviceID, idx, client);

	if (client == NULL) {
		pr_err("i2c client is NULL");
		return -EFAULT;
	}

	if (cmdInfo != NULL) {
		client->addr = cmdInfo->i2cAddr;
		cmdInfo->client = client;
	}

	return 0;
}



/*************************************************
 * EEPROM_get_cmd_info function
 *************************************************/

static int EEPROM_get_cmd_info(unsigned int sensorID,
	struct stCAM_CAL_CMD_INFO_STRUCT *cmdInfo)
{
	struct stCAM_CAL_LIST_STRUCT *pCamCalList = NULL;
	int i = 0;

	cam_cal_get_sensor_list(&pCamCalList);
	if (pCamCalList != NULL) {
		pr_debug("pCamCalList!=NULL && pCamCalFunc!= NULL\n");
		for (i = 0; pCamCalList[i].sensorID != 0; i++) {
			if (pCamCalList[i].sensorID == sensorID) {
				pr_debug("pCamCalList[%d].sensorID==%x\n", i,
				       pCamCalList[i].sensorID);

				cmdInfo->i2cAddr = pCamCalList[i].slaveID >> 1;
				cmdInfo->readCMDFunc =
					pCamCalList[i].readCamCalData;

				return 1;
			}
		}
	}
	return 0;

}

static struct stCAM_CAL_CMD_INFO_STRUCT *EEPROM_get_cmd_info_ex
	(unsigned int sensorID, unsigned int deviceID)
{
	int i = 0;

	/* To check device ID */
	for (i = 0; i < CAM_CAL_SENSOR_IDX_MAX; i++) {
		if (g_camCalDrvInfo[i].deviceID == deviceID)
			break;
	}
	/* To check cmd from Sensor ID */

	if (i == CAM_CAL_SENSOR_IDX_MAX) {
		for (i = 0; i < CAM_CAL_SENSOR_IDX_MAX; i++) {
			/* To Set Client */
			if (g_camCalDrvInfo[i].sensorID == 0) {
				pr_debug("Start get_cmd_info!\n");
				EEPROM_get_cmd_info(sensorID,
					&g_camCalDrvInfo[i]);

				if (g_camCalDrvInfo[i].readCMDFunc != NULL) {
					g_camCalDrvInfo[i].sensorID = sensorID;
					g_camCalDrvInfo[i].deviceID = deviceID;
					pr_debug("deviceID=%d, SensorID=%x\n",
						deviceID, sensorID);
				}
				break;
			}
		}
	}

	if (i == CAM_CAL_SENSOR_IDX_MAX) {	/*g_camCalDrvInfo is full */
		return NULL;
	} else {
		return &g_camCalDrvInfo[i];
	}
}

/**************************************************
 * EEPROM_HW_i2c_probe
 **************************************************/
static int EEPROM_HW_i2c_probe
	(struct i2c_client *client, const struct i2c_device_id *id)
{
	/* get sensor i2c client */
	spin_lock(&g_spinLock);
	g_pstI2Cclients[I2C_DEV_IDX_1] = client;

	/* set I2C clock rate */
#ifdef CONFIG_MTK_I2C_EXTENSION
	g_pstI2Cclients[I2C_DEV_IDX_1]->timing = gi2c_dev_timing[I2C_DEV_IDX_1];
	g_pstI2Cclients[I2C_DEV_IDX_1]->ext_flag &= ~I2C_POLLING_FLAG;
#endif

	/* Default EEPROM Slave Address Main= 0xa0 */
	g_pstI2Cclients[I2C_DEV_IDX_1]->addr = 0x50;
	spin_unlock(&g_spinLock);

	return 0;
}



/**********************************************
 * CAMERA_HW_i2c_remove
 **********************************************/
static int EEPROM_HW_i2c_remove(struct i2c_client *client)
{
	return 0;
}

/***********************************************
 * EEPROM_HW_i2c_probe2
 ***********************************************/
static int EEPROM_HW_i2c_probe2
	(struct i2c_client *client, const struct i2c_device_id *id)
{
	/* get sensor i2c client */
	spin_lock(&g_spinLock);
	g_pstI2Cclients[I2C_DEV_IDX_2] = client;

	/* set I2C clock rate */
#ifdef CONFIG_MTK_I2C_EXTENSION
	g_pstI2Cclients[I2C_DEV_IDX_2]->timing = gi2c_dev_timing[I2C_DEV_IDX_2];
	g_pstI2Cclients[I2C_DEV_IDX_2]->ext_flag &= ~I2C_POLLING_FLAG;
#endif

	/* Default EEPROM Slave Address sub = 0xa8 */
	g_pstI2Cclients[I2C_DEV_IDX_2]->addr = 0x54;
	spin_unlock(&g_spinLock);

	return 0;
}

/********************************************************
 * CAMERA_HW_i2c_remove2
 ********************************************************/
static int EEPROM_HW_i2c_remove2(struct i2c_client *client)
{
	return 0;
}

/********************************************************
 * EEPROM_HW_i2c_probe3
 ********************************************************/
static int EEPROM_HW_i2c_probe3
	(struct i2c_client *client, const struct i2c_device_id *id)
{
	/* get sensor i2c client */
	spin_lock(&g_spinLock);
	g_pstI2Cclients[I2C_DEV_IDX_3] = client;

	/* set I2C clock rate */
#ifdef CONFIG_MTK_I2C_EXTENSION
	g_pstI2Cclients[I2C_DEV_IDX_3]->timing = gi2c_dev_timing[I2C_DEV_IDX_3];
	g_pstI2Cclients[I2C_DEV_IDX_3]->ext_flag &= ~I2C_POLLING_FLAG;
#endif

	/* Default EEPROM Slave Address Main2 = 0xa4 */
	g_pstI2Cclients[I2C_DEV_IDX_3]->addr = 0x52;
	spin_unlock(&g_spinLock);

	return 0;
}

/*************************************************************
 * CAMERA_HW_i2c_remove3
 *************************************************************/
static int EEPROM_HW_i2c_remove3(struct i2c_client *client)
{
	return 0;
}

/*************************************************************
 * I2C related variable
 *************************************************************/


static const struct i2c_device_id
	EEPROM_HW_i2c_id[] = { {CAM_CAL_DRV_NAME, 0}, {} };
static const struct i2c_device_id
	EEPROM_HW_i2c_id2[] = { {CAM_CAL_I2C_DEV2_NAME, 0}, {} };
static const struct i2c_device_id
	EEPROM_HW_i2c_id3[] = { {CAM_CAL_I2C_DEV3_NAME, 0}, {} };

#ifdef CONFIG_OF
static const struct of_device_id EEPROM_HW_i2c_of_ids[] = {
	{.compatible = "mediatek,camera_main_eeprom",},
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

/*********************************************************
 * I2C Driver structure for Sub
 *********************************************************/
#ifdef CONFIG_OF
static const struct of_device_id EEPROM_HW2_i2c_driver_of_ids[] = {
	{.compatible = "mediatek,camera_sub_eeprom",},
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

/**********************************************************
 * I2C Driver structure for Main2
 **********************************************************/
#ifdef CONFIG_OF
static const struct of_device_id EEPROM_HW3_i2c_driver_of_ids[] = {
	{.compatible = "mediatek,camera_main_two_eeprom",},
	{.compatible = "mediatek,camera_main_three_eeprom",},
	{}
};
#endif

struct i2c_driver EEPROM_HW_i2c_driver3 = {
	.probe = EEPROM_HW_i2c_probe3,
	.remove = EEPROM_HW_i2c_remove3,
	.driver = {
		   .name = CAM_CAL_I2C_DEV3_NAME,
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
		   .of_match_table = EEPROM_HW3_i2c_driver_of_ids,
#endif
		   },
	.id_table = EEPROM_HW_i2c_id3,
};

static ssize_t ontim_dual_cam_otp_show_node(struct class *class,
  		struct class_attribute *attr, char *buf)
{
    pr_info("enter, node: %s\n", attr->attr.name);
    ontim_check_otp_data(dual_main_sensorid,NULL,0);
    return sprintf(buf, "%s: %s\n", attr->attr.name, "ontim_dual_cam_otp");
}

static ssize_t ontim_dual_cam_otp_store_node(struct class *class,
  		struct class_attribute *attr, const char *buf, size_t count)
{
    pr_info("enter, node: %s\n", attr->attr.name);

    ontim_write_dual_cam_otp_data();

    return count;
}

static CLASS_ATTR(ontim_dual_cam_otp, S_IRUSR|S_IWUSR, ontim_dual_cam_otp_show_node, ontim_dual_cam_otp_store_node);

/*******************************************************
 * EEPROM_HW_probe
 *******************************************************/
static int EEPROM_HW_probe(struct platform_device *pdev)
{
	int ret = 0;

	ontim_dual_cam_otp_class = class_create(THIS_MODULE, "dual_cam_otp");
	ret = class_create_file(ontim_dual_cam_otp_class,&class_attr_ontim_dual_cam_otp);
	if (ret < 0)
	{
		class_destroy(ontim_dual_cam_otp_class);
  		ontim_dual_cam_otp_class = NULL;
  		pr_info("Creat ontim_dual_cam_otp_class fail\n");
	}
	i2c_add_driver(&EEPROM_HW_i2c_driver2);
	i2c_add_driver(&EEPROM_HW_i2c_driver3);
	return i2c_add_driver(&EEPROM_HW_i2c_driver);
}

/*******************************************************
 * EEPROM_HW_remove()
 *******************************************************/
static int EEPROM_HW_remove(struct platform_device *pdev)
{
	i2c_del_driver(&EEPROM_HW_i2c_driver);
	i2c_del_driver(&EEPROM_HW_i2c_driver2);
	i2c_del_driver(&EEPROM_HW_i2c_driver3);
	return 0;
}

/******************************************************
 *
 ******************************************************/
static struct platform_device g_platDev = {
	.name = CAM_CAL_DRV_NAME,
	.id = 0,
	.dev = {
		}
};


static struct platform_driver g_stEEPROM_HW_Driver = {
	.probe = EEPROM_HW_probe,
	.remove = EEPROM_HW_remove,
	.driver = {
		   .name = CAM_CAL_DRV_NAME,
		   .owner = THIS_MODULE,
		}
};


/************************************************/

#ifdef CONFIG_COMPAT
static int compat_put_cal_info_struct
	(struct COMPAT_stCAM_CAL_INFO_STRUCT __user *data32,
		struct stCAM_CAL_INFO_STRUCT __user *data)
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
	err |= get_user(p, (compat_uptr_t *) &data->pu1Params);
	err |= put_user(p, &data32->pu1Params);
#endif
	return err;
}

static int EEPROM_compat_get_info
	(struct COMPAT_stCAM_CAL_INFO_STRUCT __user *data32,
		struct stCAM_CAL_INFO_STRUCT __user *data)
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

/*************************************************
 * ioctl
 *************************************************/

static long EEPROM_drv_compat_ioctl
	(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long ret;

	struct COMPAT_stCAM_CAL_INFO_STRUCT __user *data32;
	struct stCAM_CAL_INFO_STRUCT __user *data;
	int err;

	if (!filp->f_op || !filp->f_op->unlocked_ioctl)
		return -ENOTTY;

	switch (cmd) {

	case COMPAT_CAM_CALIOC_G_READ:{
			data32 = compat_ptr(arg);
			data = compat_alloc_user_space(sizeof(*data));
			if (data == NULL || data32 == NULL)
				return -EFAULT;

			err = EEPROM_compat_get_info(data32, data);
			if (err)
				return err;

			ret = filp->f_op->unlocked_ioctl(filp,
				CAM_CALIOC_G_READ, (unsigned long)data);
			err = compat_put_cal_info_struct(data32, data);

			if (err != 0)
				pr_debug("getinfo_struct failed\n");

			return ret;
		}

	case COMPAT_CAM_CALIOC_S_WRITE:{
				/*Note: Write Command is Unverified! */
			data32 = compat_ptr(arg);
			data = compat_alloc_user_space(sizeof(*data));
			if (data == NULL || data32 == NULL)
				return -EFAULT;

			err = EEPROM_compat_get_info(data32, data);
			if (err)
				return err;

			ret = filp->f_op->unlocked_ioctl(filp,
				CAM_CALIOC_S_WRITE, (unsigned long)data);
			if (err != 0)
				pr_debug("getinfo_struct failed\n");

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
			    unsigned int a_u4Command, unsigned long a_u4Param)
#else
static long EEPROM_drv_ioctl(struct file *file,
	unsigned int a_u4Command, unsigned long a_u4Param)
#endif
{

	int i4RetValue = 0;
	u8 *pBuff = NULL;
	u8 *pu1Params = NULL;
	/*u8 *tempP = NULL; */
	struct stCAM_CAL_INFO_STRUCT *ptempbuf = NULL;
	struct stCAM_CAL_CMD_INFO_STRUCT *pcmdInf = NULL;

#ifdef CAM_CALGETDLT_DEBUG
	struct timeval ktv1, ktv2;
	unsigned long TimeIntervalUS;
#endif
	if (_IOC_DIR(a_u4Command) != _IOC_NONE) {
		pBuff = kmalloc(sizeof(struct stCAM_CAL_INFO_STRUCT),
					GFP_KERNEL);
		if (pBuff == NULL) {

			pr_debug("ioctl allocate pBuff mem failed\n");
			return -ENOMEM;
		}

		if (copy_from_user
		    ((u8 *) pBuff, (u8 *) a_u4Param,
				sizeof(struct stCAM_CAL_INFO_STRUCT))) {
			/*get input structure address */
			kfree(pBuff);
			pr_debug("ioctl copy from user failed\n");
			return -EFAULT;
		}

		ptempbuf = (struct stCAM_CAL_INFO_STRUCT *)pBuff;

		if ((ptempbuf->u4Length <= 0) ||
			(ptempbuf->u4Length > CAM_CAL_MAX_BUF_SIZE)) {
			kfree(pBuff);
			pr_debug("Buffer Length Error!\n");
			return -EFAULT;
		}

		pu1Params = kmalloc(ptempbuf->u4Length, GFP_KERNEL);

		if (pu1Params == NULL) {
			kfree(pBuff);
			pr_debug("ioctl allocate pu1Params mem failed\n");
			return -ENOMEM;
		}

		if (copy_from_user
		    ((u8 *) pu1Params, (u8 *) ptempbuf->pu1Params,
		    ptempbuf->u4Length)) {
			kfree(pBuff);
			kfree(pu1Params);
			pr_debug("ioctl copy from user failed\n");
			return -EFAULT;
		}
	}
	if (ptempbuf == NULL) {	/*It have to add */
		pr_debug("ptempbuf is Null !!!");
		return -EFAULT;
	}
	switch (a_u4Command) {

	case CAM_CALIOC_S_WRITE:	/*Note: Write Command is Unverified! */
		pr_debug("CAM_CALIOC_S_WRITE start!\n");
#ifdef CAM_CALGETDLT_DEBUG
		do_gettimeofday(&ktv1);
#endif

		pcmdInf = EEPROM_get_cmd_info_ex(ptempbuf->sensorID,
			ptempbuf->deviceID);

		if (pcmdInf != NULL && g_lastDevID != ptempbuf->deviceID) {
			if (EEPROM_set_i2c_bus(ptempbuf->deviceID,
					       pcmdInf) != 0) {
				pr_debug("deviceID Error!\n");
				kfree(pBuff);
				kfree(pu1Params);
				return -EFAULT;
			}
			g_lastDevID = ptempbuf->deviceID;
		}

		if (pcmdInf != NULL) {
			if (pcmdInf->writeCMDFunc != NULL) {
				i4RetValue = pcmdInf->writeCMDFunc(
					pcmdInf->client,
					ptempbuf->u4Offset, pu1Params,
					ptempbuf->u4Length);
			} else
				pr_debug("pcmdInf->writeCMDFunc == NULL\n");
		} else
			pr_debug("pcmdInf == NULL\n");

#ifdef CAM_CALGETDLT_DEBUG
		do_gettimeofday(&ktv2);
		if (ktv2.tv_sec > ktv1.tv_sec)
			TimeIntervalUS = ktv1.tv_usec + 1000000 - ktv2.tv_usec;
		else
			TimeIntervalUS = ktv2.tv_usec - ktv1.tv_usec;

		pr_debug("Write data %d bytes take %lu us\n",
			ptempbuf->u4Length, TimeIntervalUS);
#endif
		pr_debug("CAM_CALIOC_S_WRITE End!\n");
		break;

	case CAM_CALIOC_G_READ:
		pr_debug("CAM_CALIOC_G_READ start! offset=%d, length=%d\n",
			ptempbuf->u4Offset, ptempbuf->u4Length);

#ifdef CAM_CALGETDLT_DEBUG
		do_gettimeofday(&ktv1);
#endif

		pr_debug("SensorID=%x DeviceID=%x\n",
			ptempbuf->sensorID, ptempbuf->deviceID);
		pcmdInf = EEPROM_get_cmd_info_ex(
			ptempbuf->sensorID,
			ptempbuf->deviceID);

		if (pcmdInf != NULL && g_lastDevID != ptempbuf->deviceID) {
			if (EEPROM_set_i2c_bus(ptempbuf->deviceID,
					       pcmdInf) != 0) {
				pr_debug("deviceID Error!\n");
				kfree(pBuff);
				kfree(pu1Params);
				return -EFAULT;
			}
			g_lastDevID = ptempbuf->deviceID;
		}

		if (pcmdInf != NULL) {
			if (pcmdInf->readCMDFunc != NULL)
				i4RetValue =
					pcmdInf->readCMDFunc(pcmdInf->client,
							  ptempbuf->u4Offset,
							  pu1Params,
							  ptempbuf->u4Length);
			else {
				pr_debug("pcmdInf->readCMDFunc == NULL\n");
				kfree(pBuff);
				kfree(pu1Params);
				return -EFAULT;
			}
		}
#ifdef CAM_CALGETDLT_DEBUG
		do_gettimeofday(&ktv2);
		if (ktv2.tv_sec > ktv1.tv_sec)
			TimeIntervalUS = ktv1.tv_usec + 1000000 - ktv2.tv_usec;
		else
			TimeIntervalUS = ktv2.tv_usec - ktv1.tv_usec;

		pr_debug("Read data %d bytes take %lu us\n",
			ptempbuf->u4Length, TimeIntervalUS);
#endif
		break;

	default:
		pr_debug("No CMD\n");
		i4RetValue = -EPERM;
		break;
	}

	if (_IOC_READ & _IOC_DIR(a_u4Command)) {
		if (copy_to_user
		    ((u8 __user *) ptempbuf->pu1Params, (u8 *) pu1Params,
				ptempbuf->u4Length)) {
			kfree(pBuff);
			kfree(pu1Params);
			pr_debug("ioctl copy to user failed\n");
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

	pr_debug("%s start\n", __func__);
	spin_lock(&g_spinLock);
	if (g_drvOpened) {
		spin_unlock(&g_spinLock);
		pr_debug("Opened, return -EBUSY\n");
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


int ontim_get_otp_data(u32  sensorid, u8 * p_buf, u32 Length)
{

    const char * str_ov13855_path = "/data/vendor/camera_dump/seansons_ov13855.data";
    const char * str_ov16a10_path = "/data/vendor/camera_dump/seansons_ov16a10.data";
    const char * str_s5k3l6_path  = "/data/vendor/camera_dump/qtech_s5k3l6.data";
    const char * str_s5k5e9yx_path  = "/data/vendor/camera_dump/qunhui_s5k5e9yx.data";
    //const char * str_hi556_path   = "/data/vendor/camera_dump/seasons_hi556.data";
    const char * str_ar1337_path  = "/data/vendor/camera_dump/qunhui_ar1337.data";
    const char * str_s5k3p9sx_TXD_path  = "/data/vendor/camera_dump/TXD_s5k3p9sx.data";
    const char * str_s5k3p9sx_TSP_path  = "/data/vendor/camera_dump/TSP_s5k3p9sx.data";
    const char * str_gc8034_holitech_path  = "/data/vendor/camera_dump/holitech_gc8034.data";
    const char * str_blackjack_tsp_gc2375h_path  = "/data/vendor/camera_dump/blackjack_tsp_gc2375h.data";
    const char * str_blackjack_jsl_gc2375h_path  = "/data/vendor/camera_dump/blackjack_jsl_gc2375h.data";
    const char * str_gc5035_path  = "/data/vendor/camera_dump/seansons_gc5035.data";
    const char * str_hi846_path  = "/data/vendor/camera_dump/blackjack_txd_hi846.data";
    const char * str_mt9d015_path  = "/data/vendor/camera_dump/blackjack_sea_mt9d015.data";
    const char * str_dump_path = NULL;
    
	struct stCAM_CAL_CMD_INFO_STRUCT *pcmdInf = NULL;
	u32 u4Offset;
	u32 u4Length;
	u32 deviceid = 1;
	u8 *pu1Params = NULL;
	int i4RetValue = 0;
    
	static mm_segment_t oldfs;
	struct file *fp;
	loff_t pos;
        pr_debug("ontim_get_otp_data sensorid= %x  p_buf=%p  Length=%d\n", sensorid, p_buf, Length);
    
    switch(sensorid)
    {

        case OV13855_SENSOR_ID:
        {
            u4Offset = 0;
            u4Length = 0x1FFF;
            str_dump_path = str_ov13855_path;
            break;
        }
	case BLACKJACK_HLT_OV16A10_SENSOR_ID:
        {
            u4Offset = 0;
            u4Length = 0x0ECA;
            str_dump_path = str_ov16a10_path;
            break;
        }
        case S5K3L6_SENSOR_ID:
        {
            u4Offset = 0;
            u4Length = 0x0cfa;
            str_dump_path = str_s5k3l6_path;
            break;
        }
        #if 0
        case HI556_SENSOR_ID:
        {
            if((p_buf == NULL)|| (Length == 0))
            {
                pr_err("eeprom_driver.c[%s](%d)  error  p_buf=%p  Length=%d\n",
                __FUNCTION__, __LINE__, p_buf, Length);
                return -1;
            }
            pu1Params = p_buf;
            u4Length = Length;
            str_dump_path = str_hi556_path;
            break;
        }
		#endif
        case S5K5E9YX_SENSOR_ID:
        {
            if((p_buf == NULL)|| (Length == 0))
            {
                pr_err("eeprom_driver.c[%s](%d)  error  p_buf=%p  Length=%d\n",
                __FUNCTION__, __LINE__, p_buf, Length);
                return -1;
            }
            pu1Params = p_buf;
            u4Length = Length;
            str_dump_path = str_s5k5e9yx_path;
            break;
        }    
        case GC5035_SENSOR_ID:
        {
            if((p_buf == NULL)|| (Length == 0))
            {
                pr_err("eeprom_driver.c[%s](%d)  error  p_buf=%p  Length=%d\n",
                __FUNCTION__, __LINE__, p_buf, Length);
                return -1;
            }
            pu1Params = p_buf;
            u4Length = Length;
            str_dump_path = str_gc5035_path;
            break;
        }
        case AR1337_SENSOR_ID:
        {
            u4Offset = 0;
            u4Length = 0xcee;
            str_dump_path = str_ar1337_path;
            break;
        }
        case S5K3P9SXT_SENSOR_ID:
        {
            u4Offset = 0;
            u4Length = 0x0D09;
            str_dump_path = str_s5k3p9sx_TXD_path;
            break;
        }
        case S5K3P9SX_SENSOR_ID:
        {
            u4Offset = 0;
            u4Length = 0x0DC9;
            str_dump_path = str_s5k3p9sx_TSP_path;
            break;
        }
        case GC8034_SENSOR_ID:
        {
            pu1Params = 0;
            u4Length = 0x0773;
            str_dump_path = str_gc8034_holitech_path;
            deviceid = 0x02;
            break;
        }
        case BLACKJACK_TSP_GC2375H_SENSOR_ID:
        {
            u4Offset = 0;
            u4Length = 0x0806;
            str_dump_path = str_blackjack_tsp_gc2375h_path;
            deviceid = 0x10;
            break;
        }
        case BLACKJACK_JSL_GC2375H_SENSOR_ID:
        {
            u4Offset = 0;
            u4Length = 0x0774;
            str_dump_path = str_blackjack_jsl_gc2375h_path;
            deviceid = 0x10;
            break;
        }
        case BLACKJACK_SEA_MT9D015_SENSOR_ID:
        {
            u4Offset = 0;
            u4Length = 0x0773;
            str_dump_path = str_mt9d015_path;
            deviceid = 0x10;
            break;
        }
        case BLACKJACK_TXD_HI846_SENSOR_ID:
        {
            if((p_buf == NULL)|| (Length == 0))
            {
                pr_err("eeprom_driver.c[%s](%d)  error  p_buf=%p  Length=%d\n",
                __FUNCTION__, __LINE__, p_buf, Length);
                return -1;
            }
            pu1Params = p_buf;
            u4Length = Length;
            str_dump_path = str_hi846_path;
            break;
        }
        
        default:
            pr_err("eeprom_driver.c[%s](%d)  sensorid=0x%x \n",
            __FUNCTION__, __LINE__,  sensorid);
            if(p_buf != NULL)
            {
				kfree(p_buf);
            }
            return -1;
    }
    
    
    if((sensorid == OV13855_SENSOR_ID) || (sensorid == BLACKJACK_TSP_GC2375H_SENSOR_ID) ||
		(sensorid == BLACKJACK_JSL_GC2375H_SENSOR_ID) || (sensorid == AR1337_SENSOR_ID) || (sensorid == S5K3L6_SENSOR_ID) ||
	(sensorid == S5K3P9SXT_SENSOR_ID) || (sensorid == S5K3P9SX_SENSOR_ID) || (sensorid == BLACKJACK_SEA_MT9D015_SENSOR_ID) ||
	(sensorid == GC8034_SENSOR_ID) || (sensorid ==BLACKJACK_HLT_OV16A10_SENSOR_ID))
   {
	pu1Params = kmalloc(u4Length, GFP_KERNEL);
	if (pu1Params == NULL) 
        {
            pr_err("eeprom_driver.c[%s](%d)  kmalloc error   pu1Params == NULL  \n",
            __FUNCTION__, __LINE__);
            return -1;
        }

	pcmdInf = EEPROM_get_cmd_info_ex(sensorid, deviceid);
	if (pcmdInf != NULL && g_lastDevID != deviceid)
	{
		if (EEPROM_set_i2c_bus(pcmdInf->deviceID, pcmdInf) != 0)
		{
			pr_debug("deviceID Error!\n");
			kfree(pu1Params);
			return -1;
		}
		g_lastDevID = deviceid;
	}

	if (pcmdInf != NULL)
        {
	    if (pcmdInf->readCMDFunc != NULL)
            {
                i4RetValue = pcmdInf->readCMDFunc(pcmdInf->client,
                u4Offset,
                pu1Params,
                u4Length);

                pr_err("eeprom_driver.c[%s](%d)  readCMDFunc   i4RetValue=0x%x \n",
                __FUNCTION__, __LINE__, i4RetValue);
                pr_err("eeprom_driver.c[%s](%d)  0x%x  0x%x ... 0x%x 0x%x \n",
                __FUNCTION__, __LINE__,
                *pu1Params, *(pu1Params+1), *(pu1Params+u4Length - 2), *(pu1Params+u4Length - 1));

            }
	    else
            {
                pr_err("eeprom_driver.c[%s](%d)  pcmdInf->readCMDFunc == NULL \n",
                __FUNCTION__, __LINE__);
				kfree(pu1Params);
				return -1;
			}
		}
   }
    
    
    oldfs = get_fs();
    set_fs(KERNEL_DS);
    fp = filp_open(str_dump_path, O_RDWR | O_CREAT, 0644);
    if (IS_ERR(fp))
    {
        pr_err("eeprom_driver.c[%s](%d)file open %s error",
         __FUNCTION__, __LINE__, str_dump_path);
    }
    else
    {
        pos = 0;
        vfs_write(fp, pu1Params, u4Length, &pos);
        filp_close(fp, NULL);
    }
    set_fs(oldfs);
    
    kfree(pu1Params);
    
    
    return 0;
}

int ontim_check_otp_data(u32  sensorid, u8 * p_buf, u32 Length)
{

    const char * str_hi556_path   = "/data/vendor/camera_dump/dual_cam_otp_dump.data";
    const char * str_dump_path = NULL;
    
	struct stCAM_CAL_CMD_INFO_STRUCT *pcmdInf = NULL;
	u32 u4Offset;
	u32 u4Length;
	u8 *pu1Params = NULL;
	int i4RetValue = 0;
    
	static mm_segment_t oldfs;
	struct file *fp;
	loff_t pos;
        pr_debug("ontim_get_otp_data sensorid= %x  p_buf=%p  Length=%d\n", sensorid, p_buf, Length);
    
    switch(sensorid)
    {
        case AR1337_SENSOR_ID:
        {
            u4Offset = 0x0776;
            u4Length = 130;
            str_dump_path = str_hi556_path;
            break;
        }
        case OV13855_SENSOR_ID:
        {
            u4Offset = 0x0776;
            u4Length = 130;
            str_dump_path = str_hi556_path;
            break;
        }
        
        default:
            pr_err("eeprom_driver.c[%s](%d)  sensorid=0x%x \n",
            __FUNCTION__, __LINE__,  sensorid);
            if(p_buf != NULL)
            {
				kfree(p_buf);
            }
            return -1;
    }
    
    
       if((sensorid == OV13855_SENSOR_ID) ||
       (sensorid == AR1337_SENSOR_ID))
   {
	pu1Params = kmalloc(u4Length, GFP_KERNEL);
	if (pu1Params == NULL) 
        {
            pr_err("eeprom_driver.c[%s](%d)  kmalloc error   pu1Params == NULL  \n",
            __FUNCTION__, __LINE__);
            return -1;
        }
        
	pcmdInf = EEPROM_get_cmd_info_ex(sensorid, 1);
	if (pcmdInf != NULL && g_lastDevID != 1)
        {
		if (EEPROM_set_i2c_bus(1, pcmdInf) != 0) 
		{
			pr_debug("deviceID Error!\n");
			kfree(pu1Params);
			return -1;
		}
		g_lastDevID = 1;
	}

	if (pcmdInf != NULL)
        {
	    if (pcmdInf->readCMDFunc != NULL)
            {
                i4RetValue = pcmdInf->readCMDFunc(pcmdInf->client,
                u4Offset,
                pu1Params,
                u4Length);

                pr_err("eeprom_driver.c[%s](%d)  readCMDFunc   i4RetValue=0x%x \n",
                __FUNCTION__, __LINE__, i4RetValue);
                pr_err("eeprom_driver.c[%s](%d)  0x%x  0x%x ... 0x%x 0x%x \n",
                __FUNCTION__, __LINE__,
                *pu1Params, *(pu1Params+1), *(pu1Params+u4Length - 2), *(pu1Params+u4Length - 1));

            }
	    else
            {
                pr_err("eeprom_driver.c[%s](%d)  pcmdInf->readCMDFunc == NULL \n",
                __FUNCTION__, __LINE__);
				kfree(pu1Params);
				return -1;
			}
		}
   }
    
    
    oldfs = get_fs();
    set_fs(KERNEL_DS);
    fp = filp_open(str_dump_path, O_RDWR | O_CREAT, 0644);
    if (IS_ERR(fp))
    {
        pr_err("eeprom_driver.c[%s](%d)file open %s error",
         __FUNCTION__, __LINE__, str_dump_path);
    }
    else
    {
        pos = 0;
        vfs_write(fp, pu1Params, u4Length, &pos);
        filp_close(fp, NULL);
    }
    set_fs(oldfs);
    
    kfree(pu1Params);
    
    
    return 0;
}
static int ontim_write_dual_cam_otp_data(void)
{
    const char * str_ar1337_path  = "/data/vendor/camera_dump/TXD_ar1337_dual_otp.data";
    const char * str_ov13855_path  = "/data/vendor/camera_dump/seasons_13855_dual_otp.data";
    const char * str_dump_path = NULL;
    
    struct stCAM_CAL_CMD_INFO_STRUCT *pcmdInf = NULL;
    u32 u4Offset;
    u32 u4Length;
    u8 *pu1Params = NULL;
    int i4RetValue = 0;

    static mm_segment_t oldfs;
    struct file *fp;
    loff_t pos;
    pr_err("eeprom_driver.c[%s](%d)ontim_write_dual_cam_otp_data enter\n",
	__FUNCTION__, __LINE__);

    switch(dual_main_sensorid)
    {
        case AR1337_SENSOR_ID:
        {
            u4Offset = 0x0776;
            u4Length = 130;
            str_dump_path = str_ar1337_path;
            break;
        }
        
       case OV13855_SENSOR_ID:
        {
            u4Offset = 0x0776;
            u4Length = 130;
            str_dump_path = str_ov13855_path;
            break;
        }

        default:
            pr_err("eeprom_driver.c[%s](%d)  dual_main_sensorid=0x%x \n",
            __FUNCTION__, __LINE__,  dual_main_sensorid);
            return -1;
    }

    if((dual_main_sensorid == AR1337_SENSOR_ID) || (dual_main_sensorid == AR1337_SENSOR_ID))
    {
        pu1Params = kmalloc(u4Length, GFP_KERNEL);
        if (pu1Params == NULL)
        {
            pr_err("eeprom_driver.c[%s](%d)  kmalloc error   pu1Params == NULL  \n",
            __FUNCTION__, __LINE__);
            return -1;
        }

	oldfs = get_fs();
	set_fs(KERNEL_DS);
	fp = filp_open(str_dump_path, O_RDWR | O_CREAT, 0644);
	if (IS_ERR(fp))
	{
		pr_err("eeprom_driver.c[%s](%d)file open %s error",
		 __FUNCTION__, __LINE__, str_dump_path);
	}
	else
	{
		pos = 0;
		vfs_read(fp, pu1Params, u4Length, &pos);
		filp_close(fp, NULL);
	}
	set_fs(oldfs);

	pcmdInf = EEPROM_get_cmd_info_ex(dual_main_sensorid, 1);
        if (EEPROM_set_i2c_bus(1, pcmdInf) != 0) 
        {
                pr_debug("deviceID Error!\n");
                kfree(pu1Params);
                return -1;
        }
        i4RetValue = Common_write_region(pcmdInf->client,
        u4Offset,
        pu1Params,
        u4Length);

        pr_err("eeprom_driver.c[%s](%d)  writeCMDFunc   i4RetValue=0x%x \n",
        __FUNCTION__, __LINE__, i4RetValue);
    }

    kfree(pu1Params);

    return 0;
}


static const struct file_operations g_stCAM_CAL_fops1 = {
	.owner = THIS_MODULE,
	.open = EEPROM_drv_open,
	.release = EEPROM_drv_release,
	/*.ioctl = CAM_CAL_Ioctl */
#ifdef CONFIG_COMPAT
	.compat_ioctl = EEPROM_drv_compat_ioctl,
#endif
	.unlocked_ioctl = EEPROM_drv_ioctl
};

/***********************************************
 *
 ***********************************************/

#define CAM_CAL_DYNAMIC_ALLOCATE_DEVNO 1
static inline int EEPROM_chrdev_register(void)
{
	struct device *device = NULL;

	pr_debug("EEPROM_chrdev_register Start\n");

#if CAM_CAL_DYNAMIC_ALLOCATE_DEVNO
	if (alloc_chrdev_region(&g_devNum, 0, 1, CAM_CAL_DRV_NAME)) {
		pr_debug("Allocate device no failed\n");
		return -EAGAIN;
	}
#else
	if (register_chrdev_region(g_devNum, 1, CAM_CAL_DRV_NAME)) {
		pr_debug("Register device no failed\n");
		return -EAGAIN;
	}
#endif

	g_charDrv = cdev_alloc();

	if (g_charDrv == NULL) {
		unregister_chrdev_region(g_devNum, 1);
		pr_debug("Allocate mem for kobject failed\n");
		return -ENOMEM;
	}

	cdev_init(g_charDrv, &g_stCAM_CAL_fops1);
	g_charDrv->owner = THIS_MODULE;

	if (cdev_add(g_charDrv, g_devNum, 1)) {
		pr_debug("Attatch file operation failed\n");
		unregister_chrdev_region(g_devNum, 1);
		return -EAGAIN;
	}

	g_drvClass = class_create(THIS_MODULE, "CAM_CALdrv1");
	if (IS_ERR(g_drvClass)) {
		int ret = PTR_ERR(g_drvClass);

		pr_debug("Unable to create class, err = %d\n", ret);
		return ret;
	}
	device = device_create(g_drvClass, NULL, g_devNum, NULL,
		CAM_CAL_DRV_NAME);
	pr_debug("EEPROM_chrdev_register End\n");

	return 0;
}

static void EEPROM_chrdev_unregister(void)
{
	/*Release char driver */

	class_destroy(g_drvClass);

	device_destroy(g_drvClass, g_devNum);

	cdev_del(g_charDrv);

	unregister_chrdev_region(g_devNum, 1);
}

/***********************************************
 *
 ***********************************************/

static int __init EEPROM_drv_init(void)
{
	pr_debug("%s Start!\n", __func__);

	if (platform_driver_register(&g_stEEPROM_HW_Driver)) {
		pr_debug("failed to register EEPROM driver i2C main\n");
		return -ENODEV;
	}

	if (platform_device_register(&g_platDev)) {
		pr_debug("failed to register EEPROM device");
		return -ENODEV;
	}

	EEPROM_chrdev_register();

	pr_debug("%s End!\n", __func__);
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
