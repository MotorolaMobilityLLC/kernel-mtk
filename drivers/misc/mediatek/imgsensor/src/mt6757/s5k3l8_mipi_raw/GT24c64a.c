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
#include <linux/module.h>
#include "kd_camera_hw.h"
#include "cam_cal.h"
#include "cam_cal_define.h"
#include "GT24c64a.h"
//#include <asm/system.h>  // for SMP
#include <linux/dma-mapping.h>
#ifdef CONFIG_COMPAT
/* 64 bit */
#include <linux/fs.h>
#include <linux/compat.h>
#endif

#define AWB_ADDR	0x001D
#define AF_ADDR		0x0015
#define LSC_ADDR	0x002B
#define LSC_SIZE	0x74C
#define MID_ADDR 0x0401

//#define CAM_CALGETDLT_DEBUG
#define CAM_CAL_DEBUG
#ifdef CAM_CAL_DEBUG
//#include <linux/xlog.h>
#define PFX "GT24c64_otp"

#define CAM_CALINF(fmt, arg...)     pr_err(PFX "[%s] " fmt, __FUNCTION__, ##arg)
#define CAM_CALDB(fmt, arg...)    pr_err(PFX "[%s] " fmt, __FUNCTION__, ##arg)
#define CAM_CALERR(fmt, arg...)     pr_err(PFX "[%s] " fmt, __FUNCTION__, ##arg)
#else
#define CAM_CALDB(x,...)
#endif
#define PAGE_SIZE_ 256
#define BUFF_SIZE 8

u8 LSCdatabuf[LSC_SIZE+2];

static int is_firstboot = 1;
static DEFINE_SPINLOCK(g_CAM_CALLock); // for SMP
#define CAM_CAL_I2C_BUSNUM 2  //lenovo.sw wangsx3 change fro K5
extern u8 OTPData[];
extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iReadRegI2C_gt24c64(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
/*******************************************************************************
*
********************************************************************************/
#define CAM_CAL_ICS_REVISION 1 //seanlin111208
/*******************************************************************************
*
********************************************************************************/
#define GT24c64_CAM_CAL_DRVNAME "GT24c64_CAM_CAL_DRV"
#define CAM_CAL_I2C_GROUP_ID 0
/*******************************************************************************
*
********************************************************************************/
//static struct i2c_board_info __initdata kd_cam_cal_dev={ I2C_BOARD_INFO(GT24c64_CAM_CAL_DRVNAME, (GT24c64_DEVICE_ID >> 1))};

static struct i2c_client * g_pstI2Cclient = NULL;

//81 is used for V4L driver
static dev_t g_CAM_CALdevno = MKDEV(CAM_CAL_DEV_MAJOR_NUMBER,0);
static struct cdev * g_pCAM_CAL_CharDrv = NULL;
//static spinlock_t g_CAM_CALLock;
//spin_lock(&g_CAM_CALLock);
//spin_unlock(&g_CAM_CALLock);

static struct class *CAM_CAL_class = NULL;
static atomic_t g_CAM_CALatomic;
//static DEFINE_SPINLOCK(kdcam_cal_drv_lock);
//spin_lock(&kdcam_cal_drv_lock);
//spin_unlock(&kdcam_cal_drv_lock);


 

#define EEPROM_I2C_SPEED 400
//#define LSCOTPDATASIZE 0x03c4 //964
//static kal_uint8 lscotpdata[LSCOTPDATASIZE];
#if 0
static void kdSetI2CSpeed(u32 i2cSpeed)
{
    spin_lock(&g_CAM_CALLock);
    g_pstI2Cclient->timing = i2cSpeed;
    spin_unlock(&g_CAM_CALLock);

}
#endif

/*******************************************************************************
* iWriteReg
********************************************************************************/
#if 0
static int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId)
{
    int  i4RetValue = 0;
    int u4Index = 0;
    u8 * puDataInBytes = (u8 *)&a_u4Data;
    int retry = 3;

    char puSendCmd[6] = {(char)(a_u2Addr >> 8) , (char)(a_u2Addr & 0xFF) ,
        0 , 0 , 0 , 0};

    spin_lock(&g_CAM_CALLock);


        g_pstI2Cclient->addr = (i2cId >> 1);
        g_pstI2Cclient->ext_flag = (g_pstI2Cclient->ext_flag)&(~I2C_DMA_FLAG);

    spin_unlock(&g_CAM_CALLock);


    if(a_u4Bytes > 2)
    {
        CAM_CALERR(" exceed 2 bytes \n");
        return -1;
    }

    if(a_u4Data >> (a_u4Bytes << 3))
    {
        CAM_CALERR(" warning!! some data is not sent!! \n");
    }

    for(u4Index = 0 ; u4Index < a_u4Bytes ; u4Index += 1 )
    {
        puSendCmd[(u4Index + 2)] = puDataInBytes[(a_u4Bytes - u4Index-1)];
    }
    do {
            i4RetValue = i2c_master_send(g_pstI2Cclient, puSendCmd, (a_u4Bytes + 2));

        if (i4RetValue != (a_u4Bytes + 2)) {
        CAM_CALERR(" I2C send failed addr = 0x%x, data = 0x%x !! \n", a_u2Addr, a_u4Data);
        }
        else {
            break;
        }
        mdelay(5);
    } while ((retry --) > 0);
    return 0;
}
#endif

bool selective_read_byte(u32 addr, unsigned char* data,u16 i2c_id)
{
//	CAM_CALDB("selective_read_byte\n");

    u8 page = addr/PAGE_SIZE_; /* size of page was 256 */
	u8 offset = addr%PAGE_SIZE_;
	//kdSetI2CSpeed(EEPROM_I2C_SPEED);

	if(iReadRegI2C(&offset, 1, (u8*)data, 1, i2c_id+(page<<1))<0) {
		CAM_CALERR("fail selective_read_byte addr =0x%x data = 0x%x,page %d, offset 0x%x", addr, *data,page,offset);
		return false;
	}
	//CAM_CALDB("selective_read_byte addr =0x%x data = 0x%x,page %d, offset 0x%x", addr, *data,page,offset);
    return true;
}

int selective_read_region(u32 addr, unsigned char* data,u16 i2c_id,u32 size)
{
  //  u32 page = addr/PAGE_SIZE; /* size of page was 256 */
	//u32 offset = addr%PAGE_SIZE;
	unsigned char* buff = data;
	u32 size_to_read = size;
	//kdSetI2CSpeed(EEPROM_I2C_SPEED);
	int ret = 0;

    while(size_to_read>0) {
		if(selective_read_byte(addr,(u8*)buff,GT24c64_DEVICE_ID)){
			addr+=1;
			buff+=1;
			size_to_read-=1;
    		ret+=1;
		} else {
			break;

		}
#if 0
		if(size_to_read > BUFF_SIZE) {
			CAM_CALDB("offset =%x size %d\n", offset,BUFF_SIZE);
			if(iReadRegI2C(&offset, 1, (u8*)buff, BUFF_SIZE, i2c_id+(page<<1))<0)
				break;
			ret += BUFF_SIZE;
			buff += BUFF_SIZE;
			offset +=BUFF_SIZE;
			size_to_read -= BUFF_SIZE;
			page += offset/PAGE_SIZE_;

		} else {
		    CAM_CALDB("offset =%x size %d\n", offset,size_to_read);
			if(iReadRegI2C(&offset, 1, (u8*)buff, (u16) size_to_read, i2c_id+(page<<1))<0)
				break;
			ret += size_to_read;
			size_to_read =0;
		}
#endif
    }
	CAM_CALDB("selective_read_region addr =%x size %d data read = %d\n", addr,size, ret);
    return ret;
}




//Burst Write Data
static int iWriteData(unsigned int  ui4_offset, unsigned int  ui4_length, unsigned char * pinputdata)
{
	CAM_CALDB("not implemented!");
	return 0;
}




#ifdef CONFIG_COMPAT
static int compat_put_cal_info_struct(
            COMPAT_stCAM_CAL_INFO_STRUCT __user *data32,
            stCAM_CAL_INFO_STRUCT __user *data)
{
#if 1
    compat_uptr_t p;
    compat_uint_t i;
    u32 err;

    err = get_user(i, (u32 *)&data->u4Offset);
    err |= put_user(i, (u32 *)&data32->u4Offset);
    err |= get_user(i, (u32 *)&data->u4Length);
    err |= put_user(i, (u32 *)&data32->u4Length);
    /* Assume pointer is not change */
#if 1
    err |= get_user(p, (u32 *)&data->pu1Params);
    err |= put_user(p, (u32 *)&data32->pu1Params);
#endif
    return err;
#else
	return 0;
#endif
}
static int compat_get_cal_info_struct(
            COMPAT_stCAM_CAL_INFO_STRUCT __user *data32,
            stCAM_CAL_INFO_STRUCT __user *data)
{
#if 1
    compat_uptr_t p;
    compat_uint_t i;
    u32 err;

    err = get_user(i, &data32->u4Offset);
    err |= put_user(i, &data->u4Offset);
    err |= get_user(i, &data32->u4Length);
    err |= put_user(i, &data->u4Length);
    err |= get_user(p, &data32->pu1Params);
    err |= put_user(compat_ptr(p), &data->pu1Params);

    return err;
#else
    return 0;
#endif
}

static long GT24c64_Ioctl_Compat(struct file *filp, unsigned int cmd, unsigned long arg)
{
    long ret;
    COMPAT_stCAM_CAL_INFO_STRUCT __user *data32;
    stCAM_CAL_INFO_STRUCT __user *data;
    int err;
    CAM_CALDB("[CAMERA SENSOR] GT24c64_Ioctl_Compat,%p %p %x ioc size %d\n",filp->f_op ,filp->f_op->unlocked_ioctl,cmd,_IOC_SIZE(cmd) );


    if (!filp->f_op || !filp->f_op->unlocked_ioctl)
        return -ENOTTY;
CAM_CALDB("GT24c64_Ioctl_Compat compat cmd = %ud , cmd = %lu,cmd =%lu\n",cmd,COMPAT_CAM_CALIOC_G_READ,CAM_CALIOC_G_READ);
    switch (cmd) {

    case COMPAT_CAM_CALIOC_G_READ:
    {
		
        data32 = compat_ptr(arg);
		CAM_CALDB("GT24c64_Ioctl_Compat 222222\n");
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
		CAM_CALDB("GT24c64_Ioctl_Compat 1111111\n");
        return -ENOIOCTLCMD;
    }
}


#endif

static bool selective_read_eeprom(unsigned short addr, unsigned char * data)
{
	char pu_send_cmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
	
	if(iReadRegI2C_gt24c64(pu_send_cmd, 2, (u8*)data, 1, GT24c64_DEVICE_ID)<0)
		return false;
    return true;
}
//lenovo.sw wuyt3 add for read MID K5 begin
unsigned char Read_MID_form_eeprom(void)
{

	unsigned char data[1];

	if(!selective_read_eeprom(MID_ADDR, &data[0])){
		return -1;
	}else{
		CAM_CALDB("[wuyt3]Read_MID_form_eeprom data[0]=%x\n", data[0]);
	}
	return data[0];
}
//end
static int Read3L8AWBData(unsigned short ui4_offset, unsigned int  ui4_length, unsigned char * pinputdata)
{
	int i = 0;
	int addr = ui4_offset;
	unsigned int sum = 0;
	u8 AWBdatabuf[14];

	CAM_CALDB("Read3L8AWBData\n");
	
	for(i = 0; i < 14; i++) {  // AWB data 30 byte
		if(!selective_read_eeprom(addr, &AWBdatabuf[i])){
			return -1;
		}
		CAM_CALDB("read_eeprom 0x%0x %d\n",addr, AWBdatabuf[i]);
		addr++;
	}

	if( AWBdatabuf[0] != 0x1)
	{
		CAM_CALDB("AWBdatabuf[0]:0x%x\n", AWBdatabuf[0]);
		CAM_CALDB("3L8_OTP AWB data invalid!\n");
	}

	for(i = 1; i < 13; i++)
	{
		sum += AWBdatabuf[i];
	}
	sum %= 0xFF;
	sum++;
	
	if(sum != AWBdatabuf[13])
	{
		CAM_CALDB("3L8_OTP AWB check sum failed!\n");
	}

	for(i = 1; i < 9; i++)
	{
		pinputdata[i-1] = AWBdatabuf[i];
	}
	
	return 0;
}

static int Read3L8AFData(unsigned short ui4_offset, unsigned int  ui4_length, unsigned char * pinputdata)
{
	int i = 0;
	int addr = ui4_offset;
	unsigned int sum = 0;
	u8 AFdatabuf[8];

	CAM_CALDB("Read3L8AFData\n");
	
	for(i = 0; i < 8; i++) {  // AWB data 30 byte
		if(!selective_read_eeprom(addr, &AFdatabuf[i])){
			return -1;
		}
		CAM_CALDB("read_eeprom 0x%0x %d\n",addr, AFdatabuf[i]);
		addr++;
	}

	if( AFdatabuf[0] != 0x1)
	{
		CAM_CALDB("AFdatabuf[0]:0x%x\n", AFdatabuf[0]);
		CAM_CALDB("3L8_OTP AF data invalid!\n");
	}

	for(i = 1; i < 5; i++)
	{
		sum += AFdatabuf[i];
	}
	sum %= 0xFF;
	sum++;
	
	if(sum != AFdatabuf[7])
	{
		CAM_CALDB("3L8_OTP AF check sum failed!\n");
	}

	for(i = 1; i < 5; i++)
	{
		pinputdata[i-1] = AFdatabuf[i];
	}
	
	return 0;
}

static int Read3L8LSCData(unsigned short ui4_offset, unsigned int  ui4_length, unsigned char * pinputdata)
{
	int i = 0;
	int addr = ui4_offset;
	unsigned int sum = 0;

	CAM_CALDB("Read3L8LSCData\n");
	
	for(i = 0; i < LSC_SIZE + 2; i++) {  // LSC data 1868 + 2 byte
		if(!selective_read_eeprom(addr, &LSCdatabuf[i])){
			return -1;
		}
		/* huangsh4 to print lsc data with kungfu */
		//CAM_CALDB("read_eeprom 0x%0x %d\n",addr, LSCdatabuf[i]);
		addr++;
	}

	if( LSCdatabuf[0] != 0x1)
	{
		CAM_CALDB("LSCdatabuf[0]:0x%x\n", LSCdatabuf[0]);
		CAM_CALDB("3L8_OTP LSC data invalid!\n");
	}

	for(i = 1; i < LSC_SIZE + 1; i++)
	{
		sum += LSCdatabuf[i];
	}
	sum %= 0xFF;
	sum++;
	
	if(sum != LSCdatabuf[LSC_SIZE+1])
	{
		CAM_CALDB("3L8_OTP LSC check sum failed!\n");
	}

	for(i = 1; i < LSC_SIZE + 1; i++)
	{
		pinputdata[i-1] = LSCdatabuf[i];
	}
	
	return 0;
}


static int iReadData(unsigned short ui4_offset, unsigned int  ui4_length, unsigned char * pinputdata)
{
	int i = 0;
	int addr = ui4_offset;
	for(i = 0; i < ui4_length; i++) {
		if(!selective_read_eeprom(addr, &pinputdata[i])){
			return false;
		}
		CAM_CALDB("read_eeprom 0x%0x %d\n",addr, pinputdata[i]);
		addr++;
	}	

	return 0;
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
	CAM_CALDB("[GT24c64_CAL] ioctl\n");

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
            CAM_CALDB(" ioctl allocate mem failed\n");
            return -ENOMEM;
        }

        if(_IOC_WRITE & _IOC_DIR(a_u4Command))
        {
            if(copy_from_user((u8 *) pBuff , (u8 *) a_u4Param, sizeof(stCAM_CAL_INFO_STRUCT)))
            {    //get input structure address
                kfree(pBuff);
                CAM_CALDB("[GT24c64_CAL] ioctl copy from user failed\n");
                return -EFAULT;
            }
        }
    }

    ptempbuf = (stCAM_CAL_INFO_STRUCT *)pBuff;
    pu1Params = (u8*)kmalloc(ptempbuf->u4Length,GFP_KERNEL);
    if(NULL == pu1Params)
    {
        kfree(pBuff);
        CAM_CALDB("ioctl allocate mem failed\n");
        return -ENOMEM;
    }
     CAM_CALDB(" init Working buffer address 0x%p  command is 0x%x\n", pu1Params, a_u4Command);


    if(copy_from_user((u8*)pu1Params ,  (u8*)ptempbuf->pu1Params, ptempbuf->u4Length))
    {
        kfree(pBuff);
        kfree(pu1Params);
        CAM_CALDB("[GT24c64_CAL] ioctl copy from user failed\n");
        return -EFAULT;
    }

	if(is_firstboot == 1)
	{
		mdelay(300);
	}	

	spin_lock(&g_CAM_CALLock);
	is_firstboot = 0;
	spin_unlock(&g_CAM_CALLock);
	
    switch(a_u4Command)
    {
        case CAM_CALIOC_S_WRITE:
            CAM_CALDB("[GT24c64_CAL] Write CMD \n");
#ifdef CAM_CALGETDLT_DEBUG
            do_gettimeofday(&ktv1);
#endif
            i4RetValue = iWriteData((u16)ptempbuf->u4Offset, ptempbuf->u4Length, pu1Params);
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
            CAM_CALDB("[GT24c64_CAL] Read CMD \n");
#ifdef CAM_CALGETDLT_DEBUG
            do_gettimeofday(&ktv1);
#endif
            CAM_CALDB("[CAM_CAL] offset %d \n", ptempbuf->u4Offset);
            CAM_CALDB("[CAM_CAL] length %d \n", ptempbuf->u4Length);
            //CAM_CALDB("[CAM_CAL] Before read Working buffer address 0x%p \n", pu1Params);

            //i4RetValue = selective_read_region(ptempbuf->u4Offset, pu1Params, GT24c64_DEVICE_ID,ptempbuf->u4Length);  
            if((ptempbuf->u4Offset == AWB_ADDR) && (ptempbuf->u4Length == 8) )
            {
            	i4RetValue = Read3L8AWBData((u16)ptempbuf->u4Offset, ptempbuf->u4Length, pu1Params);
            }
            else if((ptempbuf->u4Offset == AF_ADDR) && (ptempbuf->u4Length == 4) )
            {
            	i4RetValue = Read3L8AFData((u16)ptempbuf->u4Offset, ptempbuf->u4Length, pu1Params);
            }   
            
            else if((ptempbuf->u4Offset == LSC_ADDR) && (ptempbuf->u4Length == LSC_SIZE) )
            {
            	i4RetValue = Read3L8LSCData((u16)ptempbuf->u4Offset, ptempbuf->u4Length, pu1Params);
            }   
            
            else
            {
            	i4RetValue = iReadData((u16)ptempbuf->u4Offset, ptempbuf->u4Length, pu1Params);
            }
            
            CAM_CALDB("[GT24c64_CAL] After read Working buffer data  0x%x \n", *pu1Params);


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
      	     CAM_CALDB("[GT24c64_CAL] No CMD \n");
            i4RetValue = -EPERM;
        break;
    }

    if(_IOC_READ & _IOC_DIR(a_u4Command))
    {
        //copy data to user space buffer, keep other input paremeter unchange.
        CAM_CALDB("[GT24c64_CAL] to user length %d \n", ptempbuf->u4Length);
        CAM_CALDB("[GT24c64_CAL] to user  Working buffer address 0x%p \n", pu1Params);
        if(copy_to_user((u8 __user *) ptempbuf->pu1Params , (u8 *)pu1Params , ptempbuf->u4Length))
        {
            kfree(pBuff);
            kfree(pu1Params);
            CAM_CALDB("[GT24c64_CAL] ioctl copy to user failed\n");
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
    CAM_CALDB("[GT24c64_CAL] CAM_CAL_Open\n");
    printk("[GT24c64_CAL]camera  CAM_CAL_Open\n");
    spin_lock(&g_CAM_CALLock);
    if(g_u4Opened)
    {
        spin_unlock(&g_CAM_CALLock);
		CAM_CALDB("[GT24c64_CAL] Opened, return -EBUSY\n");
        return -EBUSY;
    }
    else
    {
        g_u4Opened = 1;
        atomic_set(&g_CAM_CALatomic,0);
    }
    spin_unlock(&g_CAM_CALLock);
    mdelay(2);
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
    .compat_ioctl = GT24c64_Ioctl_Compat,
#endif
    .unlocked_ioctl = CAM_CAL_Ioctl
};

#define CAM_CAL_DYNAMIC_ALLOCATE_DEVNO 1
inline static int RegisterCAM_CALCharDrv(void)
{
    struct device* CAM_CAL_device = NULL;

#if CAM_CAL_DYNAMIC_ALLOCATE_DEVNO
    if( alloc_chrdev_region(&g_CAM_CALdevno, 0, 1,GT24c64_CAM_CAL_DRVNAME) )
    {
        CAM_CALDB("[GT24c64_CAL] Allocate device no failed\n");

        return -EAGAIN;
    }
#else
    if( register_chrdev_region(  g_CAM_CALdevno , 1 , GT24c64_CAM_CAL_DRVNAME) )
    {
        CAM_CALDB("[GT24c64_CAL] Register device no failed\n");

        return -EAGAIN;
    }
#endif

    //Allocate driver
    g_pCAM_CAL_CharDrv = cdev_alloc();

    if(NULL == g_pCAM_CAL_CharDrv)
    {
        unregister_chrdev_region(g_CAM_CALdevno, 1);

        CAM_CALDB("[GT24c64_CAL] Allocate mem for kobject failed\n");

        return -ENOMEM;
    }

    //Attatch file operation.
    cdev_init(g_pCAM_CAL_CharDrv, &g_stCAM_CAL_fops);

    g_pCAM_CAL_CharDrv->owner = THIS_MODULE;

    //Add to system
    if(cdev_add(g_pCAM_CAL_CharDrv, g_CAM_CALdevno, 1))
    {
        CAM_CALDB("[GT24c64_CAL] Attatch file operation failed\n");

        unregister_chrdev_region(g_CAM_CALdevno, 1);

        return -EAGAIN;
    }

    CAM_CAL_class = class_create(THIS_MODULE, "GT24c64_CAM_CALdrv");
    if (IS_ERR(CAM_CAL_class)) {
        int ret = PTR_ERR(CAM_CAL_class);
        CAM_CALDB("Unable to create class, err = %d\n", ret);
        return ret;
    }
    CAM_CAL_device = device_create(CAM_CAL_class, NULL, g_CAM_CALdevno, NULL, GT24c64_CAM_CAL_DRVNAME);

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


//////////////////////////////////////////////////////////////////////
#ifndef CAM_CAL_ICS_REVISION
static int CAM_CAL_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);
#elif 0
static int CAM_CAL_i2c_detect(struct i2c_client *client, struct i2c_board_info *info);
#else
#endif
static int CAM_CAL_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int CAM_CAL_i2c_remove(struct i2c_client *);

static const struct i2c_device_id CAM_CAL_i2c_id[] = {{GT24c64_CAM_CAL_DRVNAME,0},{}};

 

static const struct of_device_id GT24c64_of_match[] = {
	{.compatible = "mediatek,GT24c64a"},
	{},
}; 


static struct i2c_driver CAM_CAL_i2c_driver = {
    .driver = {
		 .name = GT24c64_CAM_CAL_DRVNAME,

		 .of_match_table = GT24c64_of_match,
 
		},
    .probe = CAM_CAL_i2c_probe,
    .remove = CAM_CAL_i2c_remove,
//   .detect = CAM_CAL_i2c_detect,
    .id_table = CAM_CAL_i2c_id,
};

#ifndef CAM_CAL_ICS_REVISION
static int CAM_CAL_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info) {
    strcpy(info->type, GT24c64_CAM_CAL_DRVNAME);
    return 0;
}
#endif
static int CAM_CAL_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id) {
int i4RetValue = 0;
    CAM_CALDB("[GT24c64_CAL] Attach I2C \n");
//    spin_lock_init(&g_CAM_CALLock);

    //get sensor i2c client
    spin_lock(&g_CAM_CALLock); //for SMP
    g_pstI2Cclient = client;
    g_pstI2Cclient->addr = GT24c64_DEVICE_ID>>1;
    spin_unlock(&g_CAM_CALLock); // for SMP

    CAM_CALDB("[CAM_CAL] g_pstI2Cclient->addr = 0x%x \n",g_pstI2Cclient->addr);
    //Register char driver
    i4RetValue = RegisterCAM_CALCharDrv();

    if(i4RetValue){
        CAM_CALDB("[GT24c64_CAL] register char device failed!\n");
        return i4RetValue;
    }


    CAM_CALDB("[GT24c64_CAL] Attached!! \n");
    return 0;
}

static int CAM_CAL_i2c_remove(struct i2c_client *client)
{
    return 0;
}

static int CAM_CAL_probe(struct platform_device *pdev)
{
    return i2c_add_driver(&CAM_CAL_i2c_driver);
}

static int CAM_CAL_remove(struct platform_device *pdev)
{
    i2c_del_driver(&CAM_CAL_i2c_driver);
    return 0;
}

// platform structure
static struct platform_driver g_stCAM_CAL_Driver = {
    .probe		= CAM_CAL_probe,
    .remove	= CAM_CAL_remove,
    .driver		= {
        .name	= GT24c64_CAM_CAL_DRVNAME,
        .owner	= THIS_MODULE,
    }
};
 

static struct platform_device g_stCAM_CAL_Device = {
    .name = GT24c64_CAM_CAL_DRVNAME,
    .id = 0,
    .dev = {
    }
};

static int __init CAM_CAL_i2C_init(void)
{
   // i2c_register_board_info(CAM_CAL_I2C_BUSNUM, &kd_cam_cal_dev, 1);
    if(platform_driver_register(&g_stCAM_CAL_Driver)){
        CAM_CALDB("failed to register GT24c64_CAL driver\n");
        return -ENODEV;
    }

    if (platform_device_register(&g_stCAM_CAL_Device))
    {
        CAM_CALDB("failed to register GT24c64_CAL driver, 2nd time\n");
        return -ENODEV;
    }

    return 0;
}

static void __exit CAM_CAL_i2C_exit(void)
{
	platform_driver_unregister(&g_stCAM_CAL_Driver);
}

module_init(CAM_CAL_i2C_init);
module_exit(CAM_CAL_i2C_exit);

//MODULE_DESCRIPTION("CAM_CAL driver");
//MODULE_AUTHOR("Sean Lin <Sean.Lin@Mediatek.com>");
//MODULE_LICENSE("GPL");


