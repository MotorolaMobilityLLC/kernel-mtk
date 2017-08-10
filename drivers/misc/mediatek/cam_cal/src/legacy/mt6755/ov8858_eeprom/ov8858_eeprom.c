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
/*
 * Driver for CAM_CAL
 *
 *
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
#include "kd_camera_hw.h"
#include "cam_cal.h"
#include "cam_cal_define.h"
#include "ov8858_eeprom.h"
/*#include <asm/system.h>  // for SM*/
#include <linux/dma-mapping.h>
#ifdef CONFIG_COMPAT
/* 64 bit */
#include <linux/fs.h>
#include <linux/compat.h>
#endif


/*#define CAM_CALGETDLT_DEBUG*/
/*#define CAM_CAL_DEBUG*/
#ifdef CAM_CAL_DEBUG
#include <linux/xlog.h>
#include <linux/kern_levels.h>

#define PFX "ov8858_eeprom"
/*pr_err("[%s] " fmt, __FUNCTION__, ##arg)*/

#define CAM_CALINF(format, args...)    pr_info(PFX "[%s] " format, __func__, ##args)
#define CAM_CALDB(format, args...)     pr_debug(PFX "[%s] " format, __func__, ##args)
#define CAM_CALERR(format, args...)    printk(KERN_ERR format, ##args)
#else
#define CAM_CALINF(x, ...)
#define CAM_CALDB(x, ...)
#define CAM_CALERR(x, ...)
#endif

#define PAGE_SIZE_ 32
#define BUFF_SIZE 8

static DEFINE_SPINLOCK(g_CAM_CALLock); /* for SMP*/
#define CAM_CAL_I2C_BUSNUM 0

/*******************************************************************************
*
********************************************************************************/
#define CAM_CAL_ICS_REVISION 1 /*seanlin111208*/
/*******************************************************************************
*
********************************************************************************/
#define CAM_CAL_DRVNAME "CAM_CAL_DRV2"
#define CAM_CAL_I2C_GROUP_ID     0
#define CAM_CAL_DEV_MAJOR_NUMBER 226
#define OV8858_DEVICE_ID         0xA2

/*******************************************************************************
*
********************************************************************************/
static struct i2c_board_info kd_cam_cal_dev __initdata = { I2C_BOARD_INFO(CAM_CAL_DRVNAME, 0xA2 >> 1)};

static struct i2c_client *g_pstI2Cclient;

/*81 is used for V4L driver*/
static dev_t g_CAM_CALdevno = MKDEV(CAM_CAL_DEV_MAJOR_NUMBER, 0);
static struct cdev *g_pCAM_CAL_CharDrv;
/*static spinlock_t g_CAM_CALLock;*/
/*spin_lock(&g_CAM_CALLock);*/
/*spin_unlock(&g_CAM_CALLock);*/

static struct class *CAM_CAL_class;
static atomic_t g_CAM_CALatomic;
/*static DEFINE_SPINLOCK(kdcam_cal_drv_lock);*/
/*spin_lock(&kdcam_cal_drv_lock);*/
/*spin_unlock(&kdcam_cal_drv_lock);*/

static int selective_read_region(u32 addr, u8 *data, u16 i2c_id, u32 size);


#define EEPROM_I2C_SPEED 100


static void kdSetI2CSpeed(u32 i2cSpeed)
{
	spin_lock(&g_CAM_CALLock);
	g_pstI2Cclient->timing = i2cSpeed;
	spin_unlock(&g_CAM_CALLock);

}


/*******************************************************************************
* iWriteRegI2C
********************************************************************************/
/*
int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId)
{
    int  i4RetValue = 0;
    int retry = 3;

//    PK_DBG("Addr : 0x%x,Val : 0x%x\n",a_u2Addr,a_u4Data);

    //KD_IMGSENSOR_PROFILE_INIT();
    spin_lock(&g_CAM_CALLock);
    g_pstI2Cclient->addr = (i2cId >> 1);
    g_pstI2Cclient->ext_flag = (g_pstI2Cclient->ext_flag)&(~I2C_DMA_FLAG);
	spin_unlock(&g_CAM_CALLock);
    //

    do {
		i4RetValue = i2c_master_send(g_pstI2Cclient, a_pSendData, a_sizeSendData);
	if (i4RetValue != a_sizeSendData) {
	    CAM_CALERR("[CAMERA SENSOR] I2C send failed!!, Addr = 0x%x, Data = 0x%x\n",
	    a_pSendData[0], a_pSendData[1] );
	}
	else {
	    break;
	}
	udelay(50);
    } while ((retry--) > 0);
    //KD_IMGSENSOR_PROFILE("iWriteRegI2C");
    return 0;
}
*/


/*******************************************************************************
* iWriteReg
********************************************************************************/
#if 0
static int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId)
{
	int  i4RetValue = 0;
	int u4Index = 0;
	u8 *puDataInBytes = (u8 *)&a_u4Data;
	int retry = 3;

	char puSendCmd[6] = {(char)(a_u2Addr >> 8) , (char)(a_u2Addr & 0xFF) ,
			     0 , 0 , 0 , 0
			    };

	spin_lock(&g_CAM_CALLock);


	g_pstI2Cclient->addr = (i2cId >> 1);
	g_pstI2Cclient->ext_flag = (g_pstI2Cclient->ext_flag) & (~I2C_DMA_FLAG);

	spin_unlock(&g_CAM_CALLock);


	if (a_u4Bytes > 2) {
		CAM_CALERR(" exceed 2 bytes\n");
		return -1;
	}

	if (a_u4Data >> (a_u4Bytes << 3))
		CAM_CALERR(" warning!! some data is not sent!!\n");

	for (u4Index = 0; u4Index < a_u4Bytes; u4Index += 1)
		puSendCmd[(u4Index + 2)] = puDataInBytes[(a_u4Bytes - u4Index - 1)];
	do {
		i4RetValue = i2c_master_send(g_pstI2Cclient, puSendCmd, (a_u4Bytes + 2));

		if (i4RetValue != (a_u4Bytes + 2))
			CAM_CALERR(" I2C send failed addr = 0x%x, data = 0x%x !!\n", a_u2Addr, a_u4Data);
		else
			break;
		mdelay(5);
	} while ((retry--) > 0);
	return 0;
}
#endif

/*
static bool selective_read_byte(u32 addr, BYTE* data,u16 i2c_id)
{
//      CAM_CALDB("selective_read_byte\n");

    u8 page = addr/PAGE_SIZE_; size of page was 256
	u8 offset = addr%PAGE_SIZE_;
	kdSetI2CSpeed(EEPROM_I2C_SPEED);

	if(iReadRegI2C(&offset, 1, (u8*)data, 1, i2c_id+(page<<1))<0) {
		CAM_CALERR("[CAM_CAL] fail selective_read_byte addr =0x%x data = 0x%x,page %d, offset 0x%x",
		addr, *data,page,offset);
		return false;
	}
	//CAM_CALDB("selective_read_byte addr =0x%x data = 0x%x,page %d, offset 0x%x", addr, *data,page,
	offset);
    return true;
}
*/
static bool byteread_cmos_sensor(unsigned char SLAVEID, unsigned short addr, unsigned char *data)
{
	/* To call your IIC function here*/
	/*char puSendCmd[1] = {(char)(addr & 0xFF) };*/
	/*if(iReadRegI2C(puSendCmd , sizeof(puSendCmd), data, 1, SLAVEID)<0) {*/
	char puSendCmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };

	kdSetI2CSpeed(EEPROM_I2C_SPEED);

	if (iReadRegI2C(puSendCmd , 2, data, 1, SLAVEID) < 0) {
		CAM_CALERR("[CAM_CAL2] fail ov8858_byteread_cmos_sensor addr =0x%x, data = 0x%x", addr, *data);
		return false;
	}
	/*CAM_CALDB("selective_read_byte addr =0x%x data = 0x%x,page %d, offset 0x%x", addr,
	*data,page,offset);*/
	CAM_CALDB("[CAM_CAL2] ov8858_cmos_sensor addr =0x%x, data = 0x%x", addr, *data);
	return true;

}


static int selective_read_region(u32 addr, u8 *data, u16 i2c_id, u32 size)
{
	/*u32 page = addr/PAGE_SIZE; // size of page was 256 */
	/*u32 offset = addr%PAGE_SIZE;*/
	unsigned short curAddr = (unsigned short)addr;
	u8 *buff = data;
	u32 size_to_read = size;
	/*kdSetI2CSpeed(EEPROM_I2C_SPEED);*/
	int ret = 0;

	CAM_CALDB("Before byteread_cmos_sensor curAddr =%x count=%d buffData=%x\n", curAddr,
	size - size_to_read, *buff);
	while (size_to_read > 0) {
		/*if(selective_read_byte(addr,(u8*)buff,OV8858_DEVICE_ID)){*/
		if (byteread_cmos_sensor(0xA2, curAddr, buff)) {
			CAM_CALDB("after byteread_cmos_sensor curAddr =%x count=%d buffData=%x\n", curAddr,
			size - size_to_read, *buff);
			curAddr += 1;
			buff += 1;
			size_to_read -= 1;
			ret += 1;
		} else {
			break;

		}
#if 0
		if (size_to_read > BUFF_SIZE) {
			CAM_CALDB("offset =%x size %d\n", offset, BUFF_SIZE);
			if (iReadRegI2C(&offset, 1, (u8 *)buff, BUFF_SIZE, i2c_id + (page << 1)) < 0)
				break;
			ret += BUFF_SIZE;
			buff += BUFF_SIZE;
			offset += BUFF_SIZE;
			size_to_read -= BUFF_SIZE;
			page += offset / PAGE_SIZE_;

		} else {
			CAM_CALDB("offset =%x size %d\n", offset, size_to_read);
			if (iReadRegI2C(&offset, 1, (u8 *)buff, (u16) size_to_read, i2c_id + (page << 1)) < 0)
				break;
			ret += size_to_read;
			size_to_read = 0;
		}
#endif
	}
	CAM_CALDB("selective_read_region addr =%x size %d readSize = %d\n", addr, size, ret);
	return ret;
}


/* Burst Write Data */
static int iWriteData(unsigned int  ui4_offset, unsigned int  ui4_length,
unsigned char *pinputdata)
{
	CAM_CALDB("[CAM_CAL2] not implemented!");
	return 0;
}




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
	/* Assume pointer is not change */
#if 1
	err |= get_user(p, (compat_uptr_t *)&data->pu1Params);
	err |= put_user(p, &data32->pu1Params);
#endif
	return err;
}
static int compat_get_cal_info_struct(
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
	err |= get_user(p, &data32->pu1Params);
	err |= put_user(compat_ptr(p), &data->pu1Params);

	return err;
}

static long ov8858_Ioctl_Compat(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long ret;
	COMPAT_stCAM_CAL_INFO_STRUCT __user *data32;
	stCAM_CAL_INFO_STRUCT __user *data;
	int err;

	CAM_CALDB("[CAMERA SENSOR] ov8858_Ioctl_Compat,%p %p %x ioc size %d\n", filp->f_op ,
	filp->f_op->unlocked_ioctl, cmd, _IOC_SIZE(cmd));


	if (!filp->f_op || !filp->f_op->unlocked_ioctl)
		return -ENOTTY;

	switch (cmd) {

	case COMPAT_CAM_CALIOC_G_READ: {
		data32 = compat_ptr(arg);
		data = compat_alloc_user_space(sizeof(*data));
		if (data == NULL)
			return -EFAULT;

		err = compat_get_cal_info_struct(data32, data);
		if (err)
			return err;

		ret = filp->f_op->unlocked_ioctl(filp, CAM_CALIOC_G_READ,
		(unsigned long)data);
		err = compat_put_cal_info_struct(data32, data);


		if (err != 0)
			CAM_CALERR("[CAM_CAL2] compat_put_acdk_sensor_getinfo_struct failed\n");
		return ret;
	}
	default:
		return -ENOIOCTLCMD;
	}
}


#endif


/*******************************************************************************
*
********************************************************************************/
#define NEW_UNLOCK_IOCTL
#ifndef NEW_UNLOCK_IOCTL
static int CAM_CAL_Ioctl(struct inode *a_pstInode,
			 struct file *a_pstFile,
			 unsigned int a_u4Command,
			 unsigned long a_u4Param)
#else
static long CAM_CAL_Ioctl(
	struct file *file,
	unsigned int a_u4Command,
	unsigned long a_u4Param
)
#endif
{
	int i4RetValue = 0;
	u8 *pBuff = NULL;
	u8 *pu1Params = NULL;
	stCAM_CAL_INFO_STRUCT *ptempbuf = NULL;/*LukeHu++160201=Fix Code Defect.*/

	CAM_CALDB("[CAM_CAL2] ioctl\n");

#ifdef CAM_CALGETDLT_DEBUG
	struct timeval ktv1, ktv2;
	unsigned long TimeIntervalUS;
#endif

	/*if (_IOC_NONE == _IOC_DIR(a_u4Command)) {
	} else {*/
	if (_IOC_NONE != _IOC_DIR(a_u4Command)) {
		pBuff = kmalloc(sizeof(stCAM_CAL_INFO_STRUCT), GFP_KERNEL);

		if (NULL == pBuff) {
			CAM_CALDB(" ioctl allocate mem failed\n");
			return -ENOMEM;
		}

		if (_IOC_WRITE & _IOC_DIR(a_u4Command)) {
			if (copy_from_user((u8 *) pBuff , (u8 *) a_u4Param, sizeof(stCAM_CAL_INFO_STRUCT))) {
				/*get input structure address*/
				kfree(pBuff);
				CAM_CALDB("[CAM_CAL2] ioctl copy from user failed\n");
				return -EFAULT;
			}
		}
	}

	ptempbuf = (stCAM_CAL_INFO_STRUCT *)pBuff;
	if (NULL == pBuff) /* Dream add for non-null at 2016.02.29 */
		return -ENOMEM;	
	if (ptempbuf->u4Length <= 0 || ptempbuf->u4Length > 65535) {
		kfree(pBuff);
		CAM_CALDB("ptempbuf->u4Length range is failed\n");
		return -ENOMEM;
	}
	pu1Params = kmalloc(ptempbuf->u4Length, GFP_KERNEL);
	if (NULL == pu1Params) {
		kfree(pBuff);
		CAM_CALDB("[CAM_CAL2] ioctl allocate mem failed\n");
		return -ENOMEM;
	}
	CAM_CALDB(" init Working buffer address 0x%p  command is 0x%x\n", pu1Params, a_u4Command);

	if (ptempbuf->u4Length > 65535) {
		kfree(pBuff);
		kfree(pu1Params);
		CAM_CALDB("ptempbuf->u4Length is large\n");
		return -EFAULT;
	}

	if (copy_from_user((u8 *)pu1Params, (u8 *)ptempbuf->pu1Params, ptempbuf->u4Length)) {
		kfree(pBuff);
		kfree(pu1Params);
		CAM_CALDB("[CAM_CAL2] ioctl copy from user failed\n");
		return -EFAULT;
	}

	switch (a_u4Command) {
	case CAM_CALIOC_S_WRITE:
		CAM_CALDB("[CAM_CAL2] Write CMD\n");
#ifdef CAM_CALGETDLT_DEBUG
		do_gettimeofday(&ktv1);
#endif
		i4RetValue = iWriteData((u16)ptempbuf->u4Offset, ptempbuf->u4Length, pu1Params);
#ifdef CAM_CALGETDLT_DEBUG
		do_gettimeofday(&ktv2);
		if (ktv2.tv_sec > ktv1.tv_sec)
			TimeIntervalUS = ktv1.tv_usec + 1000000 - ktv2.tv_usec;
		else
			TimeIntervalUS = ktv2.tv_usec - ktv1.tv_usec;

		CAM_CALDB("Write data %d bytes take %lu us\n", ptempbuf->u4Length, TimeIntervalUS);
#endif
		break;
	case CAM_CALIOC_G_READ:
		CAM_CALDB("[CAM_CAL2] Read CMD\n");

#ifdef CAM_CALGETDLT_DEBUG
		do_gettimeofday(&ktv1);
#endif
		CAM_CALDB("[CAM_CAL2] offset %d\n", ptempbuf->u4Offset);
		CAM_CALDB("[CAM_CAL2] length %d\n", ptempbuf->u4Length);
		/**pu1Params = 0;*/
		i4RetValue = selective_read_region(ptempbuf->u4Offset, pu1Params, OV8858_DEVICE_ID, ptempbuf->u4Length);
#ifdef CAM_CALGETDLT_DEBUG
		do_gettimeofday(&ktv2);
		if (ktv2.tv_sec > ktv1.tv_sec)
			TimeIntervalUS = ktv1.tv_usec + 1000000 - ktv2.tv_usec;
		else
			TimeIntervalUS = ktv2.tv_usec - ktv1.tv_usec;

		CAM_CALDB("Read data %d bytes take %lu us\n", ptempbuf->u4Length, TimeIntervalUS);
#endif

		break;
	default:
		CAM_CALDB("[CAM_CAL2] No CMD\n");
		i4RetValue = -EPERM;
		break;
	}
	if (_IOC_READ & _IOC_DIR(a_u4Command)) {
		/*copy data to user space buffer, keep other input paremeter unchange.*/
		CAM_CALDB("[CAM_CAL2] to user length %d\n", ptempbuf->u4Length);
		if (ptempbuf->u4Length > 65535) {
			kfree(pBuff);
			kfree(pu1Params);
			CAM_CALDB("ptempbuf->u4Length is large!\n");
			return -EFAULT;
		}

		if (copy_to_user((u8 __user *) ptempbuf->pu1Params , (u8 *)pu1Params , ptempbuf->u4Length)) {
			kfree(pBuff);
			kfree(pu1Params);
			CAM_CALDB("[CAM_CAL2] ioctl copy to user failed\n");
			return -EFAULT;
		}
	}

	kfree(pBuff);
	kfree(pu1Params);
	return i4RetValue;
}


static u32 g_u4Opened;
/*#define
//Main jobs:
// 1.check for device-specified errors, device not ready.
// 2.Initialize the device if it is opened for the first time.*/
static int CAM_CAL_Open(struct inode *a_pstInode, struct file *a_pstFile)
{
	CAM_CALDB("[CAM_CAL2] CAM_CAL_Open\n");
	spin_lock(&g_CAM_CALLock);
	if (g_u4Opened) {
		spin_unlock(&g_CAM_CALLock);
		CAM_CALDB("[CAM_CAL2] Opened, return -EBUSY\n");
		return -EBUSY;
	} else {
		g_u4Opened = 1;
		atomic_set(&g_CAM_CALatomic, 0);
	}
	spin_unlock(&g_CAM_CALLock);
	mdelay(2);
	return 0;
}

/*Main jobs:
// 1.Deallocate anything that "open" allocated in private_data.
// 2.Shut down the device on last close.
// 3.Only called once on last time.
// Q1 : Try release multiple times.*/
static int CAM_CAL_Release(struct inode *a_pstInode, struct file *a_pstFile)
{
	spin_lock(&g_CAM_CALLock);

	g_u4Opened = 0;

	atomic_set(&g_CAM_CALatomic, 0);

	spin_unlock(&g_CAM_CALLock);

	return 0;
}

static const struct file_operations g_stCAM_CAL_fops = {
	.owner = THIS_MODULE,
	.open = CAM_CAL_Open,
	.release = CAM_CAL_Release,
	/*.ioctl = CAM_CAL_Ioctl*/
#ifdef CONFIG_COMPAT
	.compat_ioctl = ov8858_Ioctl_Compat,
#endif
	.unlocked_ioctl = CAM_CAL_Ioctl
};

#define CAM_CAL_DYNAMIC_ALLOCATE_DEVNO 1
static inline int RegisterCAM_CALCharDrv(void)
{
	struct device *CAM_CAL_device = NULL;

	CAM_CALDB("[CAM_CAL2] RegisterCAM_CALCharDrv Start\n");

#if CAM_CAL_DYNAMIC_ALLOCATE_DEVNO
	if (alloc_chrdev_region(&g_CAM_CALdevno, 0, 1, CAM_CAL_DRVNAME)) {
		CAM_CALDB("[CAM_CAL2] Allocate device no failed\n");

		return -EAGAIN;
	}
#else
	if (register_chrdev_region(g_CAM_CALdevno , 1 , CAM_CAL_DRVNAME)) {
		CAM_CALDB("[CAM_CAL2] Register device no failed\n");

		return -EAGAIN;
	}
#endif

	/*Allocate driver*/
	g_pCAM_CAL_CharDrv = cdev_alloc();

	if (NULL == g_pCAM_CAL_CharDrv) {
		unregister_chrdev_region(g_CAM_CALdevno, 1);

		CAM_CALDB("[CAM_CAL2] Allocate mem for kobject failed\n");

		return -ENOMEM;
	}

	/*Attatch file operation.*/
	cdev_init(g_pCAM_CAL_CharDrv, &g_stCAM_CAL_fops);

	g_pCAM_CAL_CharDrv->owner = THIS_MODULE;

	/*Add to system*/
	if (cdev_add(g_pCAM_CAL_CharDrv, g_CAM_CALdevno, 1)) {
		CAM_CALDB("[CAM_CAL2] Attatch file operation failed\n");

		unregister_chrdev_region(g_CAM_CALdevno, 1);

		return -EAGAIN;
	}

	CAM_CAL_class = class_create(THIS_MODULE, "CAM_CALdrv2");
	if (IS_ERR(CAM_CAL_class)) {
		int ret = PTR_ERR(CAM_CAL_class);

		CAM_CALDB("[CAM_CAL2] Unable to create class, err = %d\n", ret);
		return ret;
	}
	CAM_CAL_device = device_create(CAM_CAL_class, NULL, g_CAM_CALdevno, NULL, CAM_CAL_DRVNAME);
	CAM_CALDB("[CAM_CAL2] RegisterCAM_CALCharDrv End\n");

	return 0;
}

static inline void UnregisterCAM_CALCharDrv(void)
{
	/*Release char driver*/
	cdev_del(g_pCAM_CAL_CharDrv);

	unregister_chrdev_region(g_CAM_CALdevno, 1);

	device_destroy(CAM_CAL_class, g_CAM_CALdevno);
	class_destroy(CAM_CAL_class);
}


/* //////////////////////////////////////////////////////////////////// */
#ifndef CAM_CAL_ICS_REVISION
static int CAM_CAL_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);
#elif 0
static int CAM_CAL_i2c_detect(struct i2c_client *client, struct i2c_board_info *info);
#else
#endif
static int CAM_CAL_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int CAM_CAL_i2c_remove(struct i2c_client *);

static const struct i2c_device_id CAM_CAL_i2c_id[] = {{CAM_CAL_DRVNAME, 0}, {} };



static struct i2c_driver CAM_CAL_i2c_driver = {
	.probe = CAM_CAL_i2c_probe,
	.remove = CAM_CAL_i2c_remove,
	/*   .detect = CAM_CAL_i2c_detect,*/
	.driver.name = CAM_CAL_DRVNAME,
	.id_table = CAM_CAL_i2c_id,
};

#ifndef CAM_CAL_ICS_REVISION
static int CAM_CAL_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info)
{
	strcpy(info->type, CAM_CAL_DRVNAME);
	return 0;
}
#endif
static int CAM_CAL_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int i4RetValue = 0;

	CAM_CALDB("[CAM_CAL2] CAM_CAL_i2c_probe Start!\n");
	/*    spin_lock_init(&g_CAM_CALLock);*/

	/*get sensor i2c client*/
	spin_lock(&g_CAM_CALLock); /*for SMP*/
	g_pstI2Cclient = client;
	g_pstI2Cclient->addr = OV8858_DEVICE_ID >> 1;
	spin_unlock(&g_CAM_CALLock); /* for SMP*/

	CAM_CALDB("[CAM_CAL2] g_pstI2Cclient->addr = 0x%x\n", g_pstI2Cclient->addr);
	/*Register char driver*/
	i4RetValue = RegisterCAM_CALCharDrv();

	if (i4RetValue) {
		CAM_CALDB("[CAM_CAL2] register char device failed!\n");
		return i4RetValue;
	}

	CAM_CALDB("[CAM_CAL2] CAM_CAL_i2c_probe End!\n");
	return 0;
}

static int CAM_CAL_i2c_remove(struct i2c_client *client)
{
	return 0;
}

static int CAM_CAL_probe(struct platform_device *pdev)
{
	CAM_CALDB("[CAM_CAL2] CAM_CAL_probe start!\n");
	return i2c_add_driver(&CAM_CAL_i2c_driver);
}

static int CAM_CAL_remove(struct platform_device *pdev)
{
	i2c_del_driver(&CAM_CAL_i2c_driver);
	return 0;
}

/*platform structure*/
static struct platform_driver g_stCAM_CAL_Driver = {
	.probe              = CAM_CAL_probe,
	.remove     = CAM_CAL_remove,
	.driver             = {
		.name   = CAM_CAL_DRVNAME,
		.owner  = THIS_MODULE,
	}
};


static struct platform_device g_stCAM_CAL_Device = {
	.name = CAM_CAL_DRVNAME,
	.id = 0,
	.dev = {
	}
};

static int __init CAM_CAL2_i2C_init(void)
{
	CAM_CALDB("[CAM_CAL2] CAM_CAL2_i2C_init Start!\n");
	i2c_register_board_info(CAM_CAL_I2C_BUSNUM, &kd_cam_cal_dev, 1);
	if (platform_driver_register(&g_stCAM_CAL_Driver)) {
		CAM_CALDB("[CAM_CAL] failed to register CAM_CAL driver\n");
		return -ENODEV;
	}

	if (platform_device_register(&g_stCAM_CAL_Device)) {
		CAM_CALDB("[CAM_CAL2] failed to register CAM_CAL driver, 2nd time\n");
		return -ENODEV;
	}
	CAM_CALDB("[CAM_CAL2] CAM_CAL2_i2C_init End!\n");
	return 0;
}

static void __exit CAM_CAL2_i2C_exit(void)
{
	platform_driver_unregister(&g_stCAM_CAL_Driver);
}

module_init(CAM_CAL2_i2C_init);
module_exit(CAM_CAL2_i2C_exit);

MODULE_DESCRIPTION("ov8858 CAM_CAL2 driver");
MODULE_AUTHOR("DreamYeh <Dream.Yeh@Mediatek.com>");
