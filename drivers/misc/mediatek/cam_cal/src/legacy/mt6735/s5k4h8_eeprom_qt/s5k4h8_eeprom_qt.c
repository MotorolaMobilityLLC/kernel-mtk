/*
	wangkangmin@wind-mobi.com add for bb otp 20161123 begin
*/
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
#include "s5k4h8_eeprom_qt.h"
#include <linux/module.h>
//#include <asm/system.h>  // for SMP
#include <linux/dma-mapping.h>
#ifdef CONFIG_COMPAT
/* 64 bit */
#include <linux/fs.h>
#include <linux/compat.h>
#endif

#include <linux/types.h>

#include "kd_camera_typedef.h"
#define PFX "S5K4H8_EEPROM_FMT_qt"


//#define CAM_CALGETDLT_DEBUG
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

static DEFINE_SPINLOCK(g_CAM_CALLock); // for SMP


#define USHORT             unsigned short
#define BYTE               unsigned char
#define Sleep(ms) mdelay(ms)

/*******************************************************************************
*
********************************************************************************/
#define CAM_CAL_DRVNAME "CAM_CAL_DRV3"
#define CAM_CAL_I2C_GROUP_ID 0
/*******************************************************************************
*
********************************************************************************/


static dev_t g_CAM_CALdevno = MKDEV(CAM_CAL_DEV_MAJOR_NUMBER,0);
static struct cdev * g_pCAM_CAL_CharDrv = NULL;


static struct class *CAM_CAL_class = NULL;
static atomic_t g_CAM_CALatomic;

#define I2C_SPEED 100
#define MAX_LSC_SIZE 1868
#define MAX_OTP_SIZE 4096
#define GAIN_DEFAULT       	     0x0100

static int s5k4h8_eeprom_qt_read = 0;
//int read_s5k4h8_eeprom_size(u8 slv_id, u16 offset, u8* data,int size);
//kal_uint16 s5k4h8_read_cmos_sensor_eeprom(kal_uint32 addr);
typedef struct {
#if 0
	u16	   ChipInfo; //chip id, lot Id, Chip No. Etc
	u8     IdGroupWrittenFlag; //"Bit[7:6]: Flag of WB_Group_0  00: empty  01: valid group 11 or 10: invalid group"
	u8     ModuleInfo; //MID, 0x02 for truly
	u8     Year;
	u8     Month;
	u8     Day;
	u8     LensInfo;
	u8     VcmInfo;
	u8     DriverIcInfo;
	u8     LightTemp;
#endif
    u8     flag;
	u32    CaliVer;//0xff000b01
	u16    SerialNum;
	u8     Version;//0x01
	u8     AwbAfInfo;//0xF
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
}OTP_MTK_TYPE;

typedef union {
        u8 Data[MAX_OTP_SIZE];
        OTP_MTK_TYPE       MtkOtpData;
} OTP_DATA;

#if 0
void otp_clear_flag(void){
	spin_lock(&g_CAM_CALLock);
	_otp_read = 0;
	spin_unlock(&g_CAM_CALLock);
}
#endif

OTP_DATA s5k4h8_eeprom_qt_data = {{0}};
//u8 af_inf_data=0;

extern kal_uint16 qt_R_gain,qt_B_gain,qt_gd_R_gain,qt_gd_B_gain,qt_in_af,qt_ma_af; 
extern kal_uint8  qt_vendor_id;
#define CAL_VERSION_MAGIC ""
int read_s5k4h8_eeprom_qt_mtk_fmt(void)
{
#if 1
	//int i = 0;
	int offset = 0;
	//kal_uint32 checksum1 = 0;	
	//kal_uint32	checksum2 = 0;
	CAM_CALINF("OTP readed =%d \n",s5k4h8_eeprom_qt_read);
	if(1 == s5k4h8_eeprom_qt_read ) {
		CAM_CALDB("OTP readed ! skip\n");
		return 1;
	}
	spin_lock(&g_CAM_CALLock);
	s5k4h8_eeprom_qt_read = 1;
	spin_unlock(&g_CAM_CALLock);
	offset = 0;
	s5k4h8_eeprom_qt_data.Data[2] = qt_vendor_id;
	s5k4h8_eeprom_qt_data.Data[3] = qt_R_gain & 0xFF; 
	s5k4h8_eeprom_qt_data.Data[4] = qt_R_gain >> 8;
	s5k4h8_eeprom_qt_data.Data[5] = qt_B_gain & 0xFF;
	s5k4h8_eeprom_qt_data.Data[6] = qt_B_gain >> 8;
	s5k4h8_eeprom_qt_data.Data[7] = qt_gd_R_gain & 0xFF; 
	s5k4h8_eeprom_qt_data.Data[8] = qt_gd_R_gain >> 8;
	s5k4h8_eeprom_qt_data.Data[9] = qt_gd_B_gain & 0xFF;
	s5k4h8_eeprom_qt_data.Data[10] = qt_gd_B_gain >> 8;
	s5k4h8_eeprom_qt_data.Data[11] = qt_in_af & 0xFF;
	s5k4h8_eeprom_qt_data.Data[12] = qt_in_af >> 8;
	s5k4h8_eeprom_qt_data.Data[13] = qt_ma_af & 0xFF;
	s5k4h8_eeprom_qt_data.Data[14] = qt_ma_af >> 8;
	CAM_CALINF("luminjie s5k4h8 vendor_id =%x\n",qt_vendor_id);
	CAM_CALINF("luminjie s5k4h8 R_gain_h =%x,R_gain_l=%x\n",s5k4h8_eeprom_qt_data.Data[4],s5k4h8_eeprom_qt_data.Data[3]);
	CAM_CALINF("luminjie s5k4h8 B_gain_h =%x,B_gain_l=%x\n",s5k4h8_eeprom_qt_data.Data[6],s5k4h8_eeprom_qt_data.Data[5]);
	CAM_CALINF("luminjie s5k4h8 in_af_h =%x,in_af_l=%x\n",s5k4h8_eeprom_qt_data.Data[12],s5k4h8_eeprom_qt_data.Data[12]);
	CAM_CALINF("luminjie s5k4h8 R_gain_h =%x,B_gain_l=%x\n",s5k4h8_eeprom_qt_data.Data[14],s5k4h8_eeprom_qt_data.Data[13]);
#endif
	return 0;
}
#if 0
static u8 read_s5k4h8_eeprom_qt_vendor_id(void)
{
  CAM_CALINF("wangkangmin s5k4h8 vendor id =%x \n",s5k4h8_eeprom_qt_data.Data[3956]);
  return s5k4h8_eeprom_qt_data.Data[3956];
}
#endif
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
    err |= get_user(p, (compat_uptr_t __user *)&data->pu1Params);
    err |= put_user(p, (compat_uptr_t __user *)&data32->pu1Params);
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

static long s5k4h8eeprom_Ioctl_Compat(struct file *filp, unsigned int cmd, unsigned long arg)
{
    long ret;
    COMPAT_stCAM_CAL_INFO_STRUCT __user *data32;
    stCAM_CAL_INFO_STRUCT __user *data;
    int err;
	CAM_CALDB("[CAMERA SENSOR] s5k4h8_EEPROM_DEVICE_ID_QT,%p %p %x ioc size %d\n",filp->f_op ,filp->f_op->unlocked_ioctl,cmd,_IOC_SIZE(cmd) );

    if (!filp->f_op || !filp->f_op->unlocked_ioctl)
        return -ENOTTY;

    switch (cmd) {
    case COMPAT_CAM_CALIOC_G_READ:
    {
        data32 = compat_ptr(arg);
        data = compat_alloc_user_space(sizeof(*data));
        if (data == NULL)
            return -EFAULT;

        err = compat_get_cal_info_struct(data32, data);
        if (err)
            return err;

        ret = filp->f_op->unlocked_ioctl(filp, CAM_CALIOC_G_READ,(unsigned long)data);
        err = compat_put_cal_info_struct(data32, data);


        if(err != 0)
            CAM_CALERR("[CAMERA SENSOR] compat_put_acdk_sensor_getinfo_struct failed\n");
        return ret;
    }
    default:
        return -ENOIOCTLCMD;
    }
}


#endif


static int selective_read_region(u32 offset, BYTE* data,u16 i2c_id,u32 size)
{
    memcpy((void *)data,(void *)&s5k4h8_eeprom_qt_data.Data[offset],size);
	CAM_CALDB("selective_read_region offset =%x size %d data read = %d\n", offset,size, *data);
    return size;
}



/*******************************************************************************
*
********************************************************************************/
#define NEW_UNLOCK_IOCTL
#ifndef NEW_UNLOCK_IOCTL
static int CAM_CAL_Ioctl(struct inode * a_pstInode,
struct file * a_pstFile,
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
    u8 * pBuff = NULL;
    u8 * pu1Params = NULL;
    stCAM_CAL_INFO_STRUCT *ptempbuf;
#ifdef CAM_CALGETDLT_DEBUG
    struct timeval ktv1, ktv2;
    unsigned long TimeIntervalUS;
#endif
    if(_IOC_NONE == _IOC_DIR(a_u4Command))
    {
    }
    else
    {
        pBuff = (u8 *)kmalloc(sizeof(stCAM_CAL_INFO_STRUCT),GFP_KERNEL);

        if(NULL == pBuff)
        {
            CAM_CALERR(" ioctl allocate mem failed\n");
            return -ENOMEM;
        }

        if(_IOC_WRITE & _IOC_DIR(a_u4Command))
        {
            if(copy_from_user((u8 *) pBuff , (u8 *) a_u4Param, sizeof(stCAM_CAL_INFO_STRUCT)))
            {    //get input structure address
                kfree(pBuff);
                CAM_CALERR("ioctl copy from user failed\n");
                return -EFAULT;
            }
        }
    }

    ptempbuf = (stCAM_CAL_INFO_STRUCT *)pBuff;
    pu1Params = (u8*)kmalloc(ptempbuf->u4Length,GFP_KERNEL);
    if(NULL == pu1Params)
    {
        kfree(pBuff);
        CAM_CALERR("ioctl allocate mem failed\n");
        return -ENOMEM;
    }


    if(copy_from_user((u8*)pu1Params ,  (u8*)ptempbuf->pu1Params, ptempbuf->u4Length))
    {
        kfree(pBuff);
        kfree(pu1Params);
        CAM_CALERR(" ioctl copy from user failed\n");
        return -EFAULT;
    }
	CAM_CALDB("wangkangmin a_u4Command %d \n",a_u4Command);
    switch(a_u4Command)
    {
        case CAM_CALIOC_S_WRITE:
            CAM_CALDB("Write CMD \n");
#ifdef CAM_CALGETDLT_DEBUG
            do_gettimeofday(&ktv1);
#endif
            i4RetValue = 0;//iWriteData((u16)ptempbuf->u4Offset, ptempbuf->u4Length, pu1Params);
#ifdef CAM_CALGETDLT_DEBUG
            do_gettimeofday(&ktv2);
            if(ktv2.tv_sec > ktv1.tv_sec)
            {
                TimeIntervalUS = ktv1.tv_usec + 1000000 - ktv2.tv_usec;
            }
            else
            {
                TimeIntervalUS = ktv2.tv_usec - ktv1.tv_usec;
            }
            CAM_CALDB("Write data %d bytes take %lu us\n",ptempbuf->u4Length, TimeIntervalUS);
#endif
            break;
        case CAM_CALIOC_G_READ:
            CAM_CALDB("[CAM_CAL] Read CMD \n");
#ifdef CAM_CALGETDLT_DEBUG
            do_gettimeofday(&ktv1);
#endif
            CAM_CALDB("[wangkangmin CAM_CAL] offset %d \n", ptempbuf->u4Offset);
            CAM_CALDB("[wangkangmin CAM_CAL] length %d \n", ptempbuf->u4Length);
            i4RetValue = selective_read_region(ptempbuf->u4Offset, pu1Params, S5K4H8_EEPROM_DEVICE_ID_QT,ptempbuf->u4Length);
            CAM_CALDB("[wangkangmin CAM_CAL] After read Working buffer data  0x%x \n", *pu1Params);

#ifdef CAM_CALGETDLT_DEBUG
            do_gettimeofday(&ktv2);
            if(ktv2.tv_sec > ktv1.tv_sec)
            {
                TimeIntervalUS = ktv1.tv_usec + 1000000 - ktv2.tv_usec;
            }
            else
            {
                TimeIntervalUS = ktv2.tv_usec - ktv1.tv_usec;
            }
            CAM_CALDB("Read data %d bytes take %lu us\n",ptempbuf->u4Length, TimeIntervalUS);
#endif

            break;
        default :
      	     CAM_CALINF("[CAM_CAL] No CMD \n");
            i4RetValue = -EPERM;
        break;
    }

    if(_IOC_READ & _IOC_DIR(a_u4Command))
    {
        //copy data to user space buffer, keep other input paremeter unchange.
        CAM_CALDB("[wangkangmin CAM_CAL] to user length %d \n", ptempbuf->u4Length);
        CAM_CALDB("[wangkangmin CAM_CAL] to user  Working buffer address 0x%p \n", pu1Params);
        if(copy_to_user((u8 __user *) ptempbuf->pu1Params , (u8 *)pu1Params , ptempbuf->u4Length))
        {
            kfree(pBuff);
            kfree(pu1Params);
            CAM_CALERR("[wangkangmin CAM_CAL] ioctl copy to user failed\n");
            return -EFAULT;
        }
    }

    kfree(pBuff);
    kfree(pu1Params);
    return i4RetValue;
}


static u32 g_u4Opened = 0;
//#define
//Main jobs:
// 1.check for device-specified errors, device not ready.
// 2.Initialize the device if it is opened for the first time.
static int CAM_CAL_Open(struct inode * a_pstInode, struct file * a_pstFile)
{
    CAM_CALDB("wangkangmin CAM_CAL_Open g_u4Opened %d \n",g_u4Opened);
    spin_lock(&g_CAM_CALLock);
    if(g_u4Opened)
    {
        spin_unlock(&g_CAM_CALLock);
		CAM_CALERR("wangkangmin Opened, return -EBUSY\n");
        return -EBUSY;
    }
    else
    {
        g_u4Opened = 1;
        atomic_set(&g_CAM_CALatomic,0);
    }
    spin_unlock(&g_CAM_CALLock);
    return 0;
}

//Main jobs:
// 1.Deallocate anything that "open" allocated in private_data.
// 2.Shut down the device on last close.
// 3.Only called once on last time.
// Q1 : Try release multiple times.
static int CAM_CAL_Release(struct inode * a_pstInode, struct file * a_pstFile)
{
    spin_lock(&g_CAM_CALLock);

    g_u4Opened = 0;

    atomic_set(&g_CAM_CALatomic,0);

    spin_unlock(&g_CAM_CALLock);

    return 0;
}

static const struct file_operations g_stCAM_CAL_fops =
{
    .owner = THIS_MODULE,
    .open = CAM_CAL_Open,
    .release = CAM_CAL_Release,
    //.ioctl = CAM_CAL_Ioctl
#ifdef CONFIG_COMPAT
    .compat_ioctl = s5k4h8eeprom_Ioctl_Compat,
#endif
    .unlocked_ioctl = CAM_CAL_Ioctl
};

#define CAM_CAL_DYNAMIC_ALLOCATE_DEVNO 1
//#define CAM_CAL_DYNAMIC_ALLOCATE_DEVNO 1

inline static int RegisterCAM_CALCharDrv(void)
{
    struct device* CAM_CAL_device = NULL;
    CAM_CALDB("RegisterCAM_CALCharDrv\n");
#if CAM_CAL_DYNAMIC_ALLOCATE_DEVNO
    if( alloc_chrdev_region(&g_CAM_CALdevno, 0, 1,CAM_CAL_DRVNAME) )
    {
        CAM_CALERR(" Allocate device no failed\n");

        return -EAGAIN;
    }
#else
    if( register_chrdev_region(  g_CAM_CALdevno , 1 , CAM_CAL_DRVNAME) )
    {
        CAM_CALERR(" Register device no failed\n");

        return -EAGAIN;
    }
#endif

    //Allocate driver
    g_pCAM_CAL_CharDrv = cdev_alloc();

    if(NULL == g_pCAM_CAL_CharDrv)
    {
        unregister_chrdev_region(g_CAM_CALdevno, 1);

        CAM_CALERR(" Allocate mem for kobject failed\n");

        return -ENOMEM;
    }

    //Attatch file operation.
    cdev_init(g_pCAM_CAL_CharDrv, &g_stCAM_CAL_fops);

    g_pCAM_CAL_CharDrv->owner = THIS_MODULE;

    //Add to system
    if(cdev_add(g_pCAM_CAL_CharDrv, g_CAM_CALdevno, 1))
    {
        CAM_CALERR(" Attatch file operation failed\n");

        unregister_chrdev_region(g_CAM_CALdevno, 1);

        return -EAGAIN;
    }

	CAM_CAL_class = class_create(THIS_MODULE, "CAM_CALdrv3");
	if (IS_ERR(CAM_CAL_class)) {
		int ret = PTR_ERR(CAM_CAL_class);

		CAM_CALERR("Unable to create class, err = %d\n", ret);
		return ret;
	}
	CAM_CAL_device = device_create(CAM_CAL_class, NULL, g_CAM_CALdevno, NULL, CAM_CAL_DRVNAME);

    return 0;
}

inline static void UnregisterCAM_CALCharDrv(void)
{
    //Release char driver
    cdev_del(g_pCAM_CAL_CharDrv);

    unregister_chrdev_region(g_CAM_CALdevno, 1);

    device_destroy(CAM_CAL_class, g_CAM_CALdevno);
    class_destroy(CAM_CAL_class);
}

static int CAM_CAL_probe(struct platform_device *pdev)
{

    return 0;//i2c_add_driver(&CAM_CAL_i2c_driver);
}

static int CAM_CAL_remove(struct platform_device *pdev)
{
    //i2c_del_driver(&CAM_CAL_i2c_driver);
    return 0;
}

// platform structure
static struct platform_driver g_stCAM_CAL_Driver = {
    .probe		= CAM_CAL_probe,
    .remove	= CAM_CAL_remove,
    .driver		= {
        .name	= CAM_CAL_DRVNAME,
        .owner	= THIS_MODULE,
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
   //Register char driver
	i4RetValue = RegisterCAM_CALCharDrv();
    if(i4RetValue){
 	   CAM_CALDB(" register char device failed!\n");
	   return i4RetValue;
	}
	CAM_CALDB(" Attached!! \n");

  //  i2c_register_board_info(CAM_CAL_I2C_BUSNUM, &kd_cam_cal_dev, 1);
    if(platform_driver_register(&g_stCAM_CAL_Driver)){
        CAM_CALERR("failed to register s5k4h8_eeprom driver\n");
        return -ENODEV;
    }

    if (platform_device_register(&g_stCAM_CAL_Device))
    {
        CAM_CALERR("failed to register s5k4h8_eeprom driver, 2nd time\n");
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
/*
	wangkangmin@wind-mobi.com add for bb otp 20161123 end
*/
