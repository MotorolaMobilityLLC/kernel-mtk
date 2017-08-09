/*
 * Driver for CAM_CAL
 *
 *
 */

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
#include "imx219_eeprom.h"
/* #include <asm/system.h>  // for SMP */
#include <linux/dma-mapping.h>
#ifdef CONFIG_COMPAT
/* 64 bit */
#include <linux/fs.h>
#include <linux/compat.h>
#endif

#define PFX "IMX219_EEPROM_FMT"


/* #define CAM_CALGETDLT_DEBUG */
#define CAM_CAL_DEBUG
#ifdef CAM_CAL_DEBUG


#define CAM_CALINF(fmt, arg...)    pr_debug("[%s] " fmt, __func__, ##arg)
#define CAM_CALDB(fmt, arg...)     pr_debug("[%s] " fmt, __func__, ##arg)
#define CAM_CALERR(fmt, arg...)    pr_err("[%s] " fmt, __func__, ##arg)
#else
#define CAM_CALINF(x, ...)
#define CAM_CALDB(x, ...)
#define CAM_CALERR(fmt, arg...)    pr_err("[%s] " fmt, __func__, ##arg)
#endif

static DEFINE_SPINLOCK(g_CAM_CALLock); /* for SMP */


#define USHORT             unsigned short
#define BYTE               unsigned char
#define Sleep(ms) mdelay(ms)

/*******************************************************************************
*
********************************************************************************/
#define CAM_CAL_DRVNAME "CAM_CAL_DRV"
#define CAM_CAL_I2C_GROUP_ID 0
/*******************************************************************************
*
********************************************************************************/


static dev_t g_CAM_CALdevno = MKDEV(CAM_CAL_DEV_MAJOR_NUMBER, 0);
static struct cdev *g_pCAM_CAL_CharDrv;


static struct class *CAM_CAL_class;
static atomic_t g_CAM_CALatomic;

#define I2C_SPEED 100
#define MAX_LSC_SIZE 1024
#define MAX_OTP_SIZE 1100
static int imx219_eeprom_read;

struct mtk_format {
#if 0
	u16    ChipInfo; /* chip id, lot Id, Chip No. Etc */
	u8     IdGroupWrittenFlag;
	/* "Bit[7:6]: Flag of WB_Group_0  00:empty  01: valid group 11 or 10: invalid group" */
	u8     ModuleInfo; /* MID, 0x02 for truly */
	u8     Year;
	u8     Month;
	u8     Day;
	u8     LensInfo;
	u8     VcmInfo;
	u8     DriverIcInfo;
	u8     LightTemp;
#endif
	u8     flag;
	u32    CaliVer;/* 0xff000b01 */
	u16    SerialNum;
	u8     Version;/* 0x01 */
	u8     AwbAfInfo;/* 0xF */
	u8     UnitAwbR;
	u8     UnitAwbGr;
	u8     UnitAwbGb;
	u8     UnitAwbB;
	u8     GoldenAwbR;
	u8     GoldenAwbGr;
	u8     GoldenAwbGb;
	u8     GoldenAwbB;
	u16    AfInfinite;
	u16    AfMacro;
	u16    LscSize;
	u8   Lsc[MAX_LSC_SIZE];
} OTP_MTK_TYPE;

union OTP_DATA {
	u8 Data[MAX_OTP_SIZE];
	OTP_MTK_TYPE       MtkOtpData;
};

#if 0
void otp_clear_flag(void)
{
	spin_lock(&g_CAM_CALLock);
	_otp_read = 0;
	spin_unlock(&g_CAM_CALLock);
}
#endif

union OTP_DATA imx219_eeprom_data = {{0} };

/*LukeHu--150706=For Kernel coding style.
extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 *a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
extern void kdSetI2CSpeed(u16 i2cSpeed);
*/


static int read_cmos_sensor(kal_uint16 slave_id, kal_uint32 addr, u8 *data)
{
	char pu_send_cmd[2] = {(char)(addr & 0xFF) };

	kdSetI2CSpeed(I2C_SPEED);
	return iReadRegI2C(pu_send_cmd, 1, data, 1, slave_id);/* 0 for good */
}



int read_imx219_eeprom(u8 slv_id, u16 offset, u8 *data)
{
	int ret = 0;

	ret = read_cmos_sensor(slv_id, offset, data);
	CAM_CALDB("OTP read slv_id 0x%x offset 0x%x  data 0x%x\n", slv_id, offset, *data);

	return ret;
}

int read_imx219_eeprom_size(u8 slv_id, u16 offset, u8 *data, int size)
{
	int i = 0;

	for (i = 0; i < size; i++) {
		if (read_imx219_eeprom(slv_id, offset + i, data + i) != 0)
			return -1;
	}
	return 0;
}

#define CAL_VERSION_MAGIC ""
int read_imx219_eeprom_mtk_fmt(void)
{
	int i = 0;
	int offset = 0;

	CAM_CALINF("OTP readed =%d\n", imx219_eeprom_read);
	if (1 == imx219_eeprom_read) {
		CAM_CALDB("OTP readed ! skip\n");
		return 1;
	}
	spin_lock(&g_CAM_CALLock);
	imx219_eeprom_read = 1;
	spin_unlock(&g_CAM_CALLock);
#if 0
	read_imx219_eeprom_size(0xA0, 0x00, &imx219_eeprom_data.Data[0x00], 1);
#endif

	/* read calibration version 0xff000b01 */
	if (read_imx219_eeprom_size(0xA0, 0x01, &imx219_eeprom_data.Data[0x01], 4) != 0) {
		CAM_CALERR("read imx219_eeprom GT24C16 i2c fail !?\n");
		return -1;
	}

	/* read serial number */
	read_imx219_eeprom_size(0xA0, 0x05, &imx219_eeprom_data.Data[0x05], 2);

	/* read AF config */
	read_imx219_eeprom_size(0xA0, 0x07, &imx219_eeprom_data.Data[0x07], 2);

	/* read AWB */
	read_imx219_eeprom_size(0xA0, 0x09, &imx219_eeprom_data.Data[0x09], 8);

	/* read AF calibration */
	read_imx219_eeprom_size(0xA0, 0x11, &imx219_eeprom_data.Data[0x011], 4);

	/* read LSC size */
	read_imx219_eeprom_size(0xA0, 0x15, &imx219_eeprom_data.Data[0x015], 2);
#if 0
	int size = 0;

	size = imx219_eeprom_data.Data[0x015] + imx219_eeprom_data.Data[0x016] << 4;
#endif


	/* for lsc data */
	read_imx219_eeprom_size(0xA0, 0x17, &imx219_eeprom_data.Data[0x017], (0xFF - 0X17 + 1));
	offset = 256;
	for (i = 0xA2; i < 0xA6; i += 2) {
		read_imx219_eeprom_size(i, 0x00, &imx219_eeprom_data.Data[offset], 256);
		offset += 256;
	}
	read_imx219_eeprom_size(0xA6, 0x00, &imx219_eeprom_data.Data[offset], 0xBA - 0 + 1);
	CAM_CALDB("final offset offset %d !\n", offset + 0xBA);
#if 0
	CAM_CALDB("size %d readed %d!\n", size, offset + 0xBA - 0x17 + 1);
	u8 data[9];

	read_imx219_eeprom_size(0xAA, 0xE0, &data[0], 8);
#endif

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
	err |= get_user(p, &data->pu1Params);
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

static long imx219eeprom_Ioctl_Compat(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long ret;

	COMPAT_stCAM_CAL_INFO_STRUCT __user *data32;
	stCAM_CAL_INFO_STRUCT __user *data;
	int err;

	CAM_CALDB("[CAMERA SENSOR] imx219_eeprom_DEVICE_ID,%p %p %x ioc size %d\n",
	filp->f_op , filp->f_op->unlocked_ioctl, cmd, _IOC_SIZE(cmd));

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

		ret = filp->f_op->unlocked_ioctl(filp, CAM_CALIOC_G_READ, (unsigned long)data);
		err = compat_put_cal_info_struct(data32, data);


		if (err != 0)
			CAM_CALERR("[CAMERA SENSOR] compat_put_acdk_sensor_getinfo_struct failed\n");
		return ret;
	}
	default:
		return -ENOIOCTLCMD;
	}
}


#endif


static int selective_read_region(u32 offset, BYTE *data, u16 i2c_id, u32 size)
{
	memcpy((void *)data, (void *)&imx219_eeprom_data.Data[offset], size);
	CAM_CALDB("selective_read_region offset =%x size %d data read = %d\n", offset, size, *data);
	return size;
}



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
	stCAM_CAL_INFO_STRUCT *ptempbuf;
#ifdef CAM_CALGETDLT_DEBUG
	struct timeval ktv1, ktv2;
	unsigned long TimeIntervalUS;
#endif
/*
	if (_IOC_NONE == _IOC_DIR(a_u4Command)) {
	} else {
*/
	if (_IOC_NONE != _IOC_DIR(a_u4Command)) {
		pBuff = kmalloc(sizeof(stCAM_CAL_INFO_STRUCT), GFP_KERNEL);

		if (NULL == pBuff) {
			CAM_CALERR(" ioctl allocate mem failed\n");
			return -ENOMEM;
		}

		if (_IOC_WRITE & _IOC_DIR(a_u4Command)) {
			if (copy_from_user((u8 *) pBuff , (u8 *) a_u4Param, sizeof(stCAM_CAL_INFO_STRUCT))) {
				/* get input structure address */
				kfree(pBuff);
				CAM_CALERR("ioctl copy from user failed\n");
				return -EFAULT;
			}
		}
	}

	ptempbuf = (stCAM_CAL_INFO_STRUCT *)pBuff;
	pu1Params = kmalloc(ptempbuf->u4Length, GFP_KERNEL);
	if (NULL == pu1Params) {
		kfree(pBuff);
		CAM_CALERR("ioctl allocate mem failed\n");
		return -ENOMEM;
	}


	if (copy_from_user((u8 *)pu1Params , (u8 *)ptempbuf->pu1Params, ptempbuf->u4Length)) {
		kfree(pBuff);
		kfree(pu1Params);
		CAM_CALERR(" ioctl copy from user failed\n");
		return -EFAULT;
	}

	switch (a_u4Command) {
	case CAM_CALIOC_S_WRITE:
		CAM_CALDB("Write CMD\n");
#ifdef CAM_CALGETDLT_DEBUG
		do_gettimeofday(&ktv1);
#endif
		i4RetValue = 0;/* iWriteData((u16)ptempbuf->u4Offset, ptempbuf->u4Length, pu1Params); */
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
		CAM_CALDB("[CAM_CAL] Read CMD\n");
#ifdef CAM_CALGETDLT_DEBUG
		do_gettimeofday(&ktv1);
#endif
		i4RetValue = selective_read_region(ptempbuf->u4Offset, pu1Params,
		IMX219_EEPROM_DEVICE_ID, ptempbuf->u4Length);

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
		CAM_CALINF("[CAM_CAL] No CMD\n");
		i4RetValue = -EPERM;
		break;
	}

	if (_IOC_READ & _IOC_DIR(a_u4Command)) {
		/* copy data to user space buffer, keep other input paremeter unchange. */
		if (copy_to_user((u8 __user *) ptempbuf->pu1Params , (u8 *)pu1Params , ptempbuf->u4Length)) {
			kfree(pBuff);
			kfree(pu1Params);
			CAM_CALERR("[CAM_CAL] ioctl copy to user failed\n");
			return -EFAULT;
		}
	}

	kfree(pBuff);
	kfree(pu1Params);
	return i4RetValue;
}


static u32 g_u4Opened;
/* #define */
/* Main jobs: */
/* 1.check for device-specified errors, device not ready. */
/* 2.Initialize the device if it is opened for the first time. */
static int CAM_CAL_Open(struct inode *a_pstInode, struct file *a_pstFile)
{
	CAM_CALDB("CAM_CAL_Open\n");
	spin_lock(&g_CAM_CALLock);
	if (g_u4Opened) {
		spin_unlock(&g_CAM_CALLock);
		CAM_CALERR("Opened, return -EBUSY\n");
		return -EBUSY;
	} /*else {*//*LukeHu--150720=For check fo*/
	if (!g_u4Opened) {/*LukeHu++150720=For check fo*/
		g_u4Opened = 1;
		atomic_set(&g_CAM_CALatomic, 0);
	}
	spin_unlock(&g_CAM_CALLock);
	return 0;
}

/* Main jobs: */
/* 1.Deallocate anything that "open" allocated in private_data. */
/* 2.Shut down the device on last close. */
/* 3.Only called once on last time. */
/* Q1 : Try release multiple times. */
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
	/* .ioctl = CAM_CAL_Ioctl */
#ifdef CONFIG_COMPAT
	.compat_ioctl = imx219eeprom_Ioctl_Compat,
#endif
	.unlocked_ioctl = CAM_CAL_Ioctl
};

#define CAM_CAL_DYNAMIC_ALLOCATE_DEVNO 1
/* #define CAM_CAL_DYNAMIC_ALLOCATE_DEVNO 1 */

static inline  int RegisterCAM_CALCharDrv(void)
{
	struct device *CAM_CAL_device = NULL;

	CAM_CALDB("RegisterCAM_CALCharDrv\n");
#if CAM_CAL_DYNAMIC_ALLOCATE_DEVNO
	if (alloc_chrdev_region(&g_CAM_CALdevno, 0, 1, CAM_CAL_DRVNAME)) {
		CAM_CALERR(" Allocate device no failed\n");

		return -EAGAIN;
	}
#else
	if (register_chrdev_region(g_CAM_CALdevno , 1 , CAM_CAL_DRVNAME)) {
		CAM_CALERR(" Register device no failed\n");

		return -EAGAIN;
	}
#endif

	/* Allocate driver */
	g_pCAM_CAL_CharDrv = cdev_alloc();

	if (NULL == g_pCAM_CAL_CharDrv) {
		unregister_chrdev_region(g_CAM_CALdevno, 1);

		CAM_CALERR(" Allocate mem for kobject failed\n");

		return -ENOMEM;
	}

	/* Attatch file operation. */
	cdev_init(g_pCAM_CAL_CharDrv, &g_stCAM_CAL_fops);

	g_pCAM_CAL_CharDrv->owner = THIS_MODULE;

	/* Add to system */
	if (cdev_add(g_pCAM_CAL_CharDrv, g_CAM_CALdevno, 1)) {
		CAM_CALERR(" Attatch file operation failed\n");

		unregister_chrdev_region(g_CAM_CALdevno, 1);

		return -EAGAIN;
	}

	CAM_CAL_class = class_create(THIS_MODULE, "CAM_CALdrv");
	if (IS_ERR(CAM_CAL_class)) {
		int ret = PTR_ERR(CAM_CAL_class);

		CAM_CALERR("Unable to create class, err = %d\n", ret);
		return ret;
	}
	CAM_CAL_device = device_create(CAM_CAL_class, NULL, g_CAM_CALdevno, NULL, CAM_CAL_DRVNAME);

	return 0;
}

static inline void UnregisterCAM_CALCharDrv(void)
{
	/* Release char driver */
	cdev_del(g_pCAM_CAL_CharDrv);

	unregister_chrdev_region(g_CAM_CALdevno, 1);

	device_destroy(CAM_CAL_class, g_CAM_CALdevno);
	class_destroy(CAM_CAL_class);
}

static int CAM_CAL_probe(struct platform_device *pdev)
{

	return 0;/* i2c_add_driver(&CAM_CAL_i2c_driver); */
}

static int CAM_CAL_remove(struct platform_device *pdev)
{
	/* i2c_del_driver(&CAM_CAL_i2c_driver); */
	return 0;
}

/* platform structure */
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

static int __init CAM_CAL_init(void)
{
	int i4RetValue = 0;

	CAM_CALDB("CAM_CAL_i2C_init\n");
	/* Register char driver */
	i4RetValue = RegisterCAM_CALCharDrv();
	if (i4RetValue) {
		CAM_CALDB(" register char device failed!\n");
		return i4RetValue;
	}
	CAM_CALDB(" Attached!!\n");

	/* i2c_register_board_info(CAM_CAL_I2C_BUSNUM, &kd_cam_cal_dev, 1); */
	if (platform_driver_register(&g_stCAM_CAL_Driver)) {
		CAM_CALERR("failed to register imx219_eeprom driver\n");
		return -ENODEV;
	}

	if (platform_device_register(&g_stCAM_CAL_Device)) {
		CAM_CALERR("failed to register imx219_eeprom driver, 2nd time\n");
		return -ENODEV;
	}

	return 0;
}

static void __exit CAM_CAL_exit(void)
{
	platform_driver_unregister(&g_stCAM_CAL_Driver);
}

module_init(CAM_CAL_init);
module_exit(CAM_CAL_exit);

MODULE_DESCRIPTION("CAM_CAL driver");
MODULE_AUTHOR("Sean Lin <Sean.Lin@Mediatek.com>");
MODULE_LICENSE("GPL");


