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
#include "cam_cal.h"
#include "cam_cal_define.h"
#include "imx219_otp.h"
//#include <asm/system.h>  // for SMP

//#define CAM_CALGETDLT_DEBUG
#define CAM_CAL_DEBUG  
#ifdef CAM_CAL_DEBUG
#define CAM_CALDB printk
#else
#define CAM_CALDB(x,...)
#endif

typedef unsigned char kal_uint8;
typedef unsigned int kal_uint32;
static DEFINE_SPINLOCK(g_CAM_CALLock); // for SMP
#define CAM_CAL_I2C_BUSNUM 2//

/*******************************************************************************
*
********************************************************************************/
#define CAM_CAL_ICS_REVISION 1 //seanlin111208
/*******************************************************************************
*
********************************************************************************/
#define CAM_CAL_DRVNAME "CAM_CAL_DRV"
#define CAM_CAL_I2C_GROUP_ID 0
/*******************************************************************************
*
********************************************************************************/
//static struct i2c_board_info __initdata kd_cam_cal_dev={ I2C_BOARD_INFO(CAM_CAL_DRVNAME, 0x19)};

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

extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);



/*******************************************************************************
*
********************************************************************************/
//Address: 2Byte, Data: 1Byte
int iWriteCAM_CAL(u16 a_u2Addr  , u32 a_u4Bytes, u8 puDataInBytes)
{
    int  i4RetValue = 0;
	int retry = 3;
    char puSendCmd[3] = {(char)(a_u2Addr >> 8) , (char)(a_u2Addr & 0xFF) ,puDataInBytes};
	do {
        CAM_CALDB("[CAM_CAL][iWriteCAM_CAL] Write 0x%x=0x%x \n",a_u2Addr, puDataInBytes);
		i4RetValue = i2c_master_send(g_pstI2Cclient, puSendCmd, 3);
        if (i4RetValue != 3) {
            CAM_CALDB("[CAM_CAL] I2C send failed!!\n");
        }
        else {
            break;
    	}
        mdelay(10);
    } while ((retry--) > 0);    
   //CAM_CALDB("[CAM_CAL] iWriteCAM_CAL done!! \n");
    return 0;
}


//Burst Write Data
static int iWriteData(unsigned int  ui4_offset, unsigned int  ui4_length, u8 * pinputdata)
{
   int  i4RetValue = 0;
   int  i4ResidueDataLength;
   u32 u4IncOffset = 0;
   u32 u4CurrentOffset;
   u8 * pBuff;
   CAM_CALDB("[imx219otp_CAL] iWriteData\n" );

   i4ResidueDataLength = (int)ui4_length;
   u4CurrentOffset = ui4_offset;
   pBuff = pinputdata;   

   CAM_CALDB("[CAM_CAL] iWriteData u4CurrentOffset is %d \n",u4CurrentOffset);   

   do 
   {
       CAM_CALDB("[CAM_CAL][iWriteData] Write 0x%x=0x%x \n",u4CurrentOffset, pBuff[0]);
       i4RetValue = iWriteCAM_CAL((u16)u4CurrentOffset, 1, pBuff[0]);
       if (i4RetValue != 0)
       {
            CAM_CALDB("[CAM_CAL] I2C iWriteData failed!! \n");
            return -1;
       }           
       u4IncOffset ++;
       i4ResidueDataLength --;
       u4CurrentOffset = ui4_offset + u4IncOffset;
       pBuff = pinputdata + u4IncOffset;
   }while (i4ResidueDataLength > 0);
   CAM_CALDB("[imx219otp_CAL] iWriteData done\n" );
 
   return 0;
}

#ifdef CONFIG_MTK_CAM_CAL_IMX219OTP_SUPPORT
extern kal_uint8 imx219_cam_cal_read_data(kal_uint32 offset,
        kal_uint32 lenth, kal_uint8 *pBuf);
#endif

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
    int  i4RetValue = 0;
    u8 * pBuff = NULL;
    int i = 0;
    u8 * pWorkingBuff = NULL;
    stCAM_CAL_INFO_STRUCT *ptempbuf;
	
	u8 * pOTP_write_OneByteBuff = NULL;
	pOTP_write_OneByteBuff = (u8 *)kmalloc(I2C_UNIT_SIZE, GFP_KERNEL);
	//Enable Reading OTP


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
            CAM_CALDB("[imx219otp_CAL] ioctl allocate mem failed\n");
            return -ENOMEM;
        }

        if(_IOC_WRITE & _IOC_DIR(a_u4Command))
        {
            if(copy_from_user((u8 *) pBuff , (u8 *) a_u4Param, sizeof(stCAM_CAL_INFO_STRUCT)))
            {    //get input structure address
                kfree(pBuff);
                CAM_CALDB("[imx219otp_CAL] ioctl copy from user failed\n");
                return -EFAULT;
            }
        }
    }

    ptempbuf = (stCAM_CAL_INFO_STRUCT *)pBuff;
    pWorkingBuff = (u8*)kmalloc(ptempbuf->u4Length,GFP_KERNEL); 
    if(NULL == pWorkingBuff)
    {
        kfree(pBuff);
        CAM_CALDB("[imx135otp_CAL] ioctl allocate mem failed\n");
        return -ENOMEM;
    }
     //CAM_CALDB("[imx219otp_CAL] init Working buffer address 0x%8x  command is 0x%8x\n", (u32)pWorkingBuff, (u32)a_u4Command);

 
    if(copy_from_user((u8*)pWorkingBuff ,  (u8*)ptempbuf->pu1Params, ptempbuf->u4Length))
    {
        kfree(pBuff);
        kfree(pWorkingBuff);
        CAM_CALDB("[imx219otp_CAL] ioctl copy from user failed\n");
        return -EFAULT;
    } 
    
    switch(a_u4Command)
    {
        case CAM_CALIOC_S_WRITE:    
            CAM_CALDB("[imx219otp_CAL] Write CMD \n");
#ifdef CAM_CALGETDLT_DEBUG
            do_gettimeofday(&ktv1);
#endif            
            i4RetValue = iWriteData((u16)ptempbuf->u4Offset, ptempbuf->u4Length, pWorkingBuff);
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
            printk("Write data %d bytes take %lu us\n",ptempbuf->u4Length, TimeIntervalUS);
#endif            
            break;
        case CAM_CALIOC_G_READ:
            CAM_CALDB("[imx219otp_CAL] Read CMD \n");
#ifdef CAM_CALGETDLT_DEBUG            
            do_gettimeofday(&ktv1);
#endif 
            CAM_CALDB("[CAM_CAL] offset %d \n", ptempbuf->u4Offset);
            CAM_CALDB("[CAM_CAL] length %d \n", ptempbuf->u4Length);
            //CAM_CALDB("[CAM_CAL] Before read Working buffer address 0x%8x \n", (u32)pWorkingBuff);

			printk("[CAM_CAL] offset %x \n", ptempbuf->u4Offset);
			printk("[CAM_CAL] length %d \n", ptempbuf->u4Length);
			//printk("[CAM_CAL] Before read Working buffer address 0x%8x \n", (u32)pWorkingBuff);		

#ifdef CONFIG_MTK_CAM_CAL_IMX219OTP_SUPPORT
CAM_CALDB("[imx219otp_CAL]enter IMX219_MIPI_RAW Read CMD \n");
	if(ptempbuf->u4Offset==0x00)
	{
		if (imx219_cam_cal_read_data(ptempbuf->u4Offset, ptempbuf->u4Length, pWorkingBuff) 
			!= ptempbuf->u4Length){
			i4RetValue = -1;
		} else {
			i4RetValue = 0;
		}
	}
	else if(ptempbuf->u4Offset==0x5a10) {
				if (imx219_cam_cal_read_data(ptempbuf->u4Offset, ptempbuf->u4Length, pWorkingBuff) 
			!= ptempbuf->u4Length){
			i4RetValue = -1;
		} else {
			for(i=0;i<=4;i=i+4)
		{
		CAM_CALDB("shen 0x%02x,   0x%02x,   0x%02x,  0x%02x,  0x%02x\n",
		pWorkingBuff[i],pWorkingBuff[i+1],pWorkingBuff[i+2],pWorkingBuff[i+3],pWorkingBuff[i+4]);			
		}
			i4RetValue = 0;
		}
	}
	else if(ptempbuf->u4Offset==0x5a20) {
				if (imx219_cam_cal_read_data(ptempbuf->u4Offset, ptempbuf->u4Length, pWorkingBuff) 
			!= ptempbuf->u4Length){
			i4RetValue = -1;
		} else {
		for(i=0;i<=20;i=i+16)
		{
		CAM_CALDB("shen 0x%02x,   0x%02x,   0x%02x,  0x%02x,  0x%02x,  0x%02x,  0x%02x,  0x%02x,  0x%02x,  0x%02x,  0x%02x,  0x%02x,  0x%02x,  0x%02x,  0x%02x,  0x%02x,\n",
		pWorkingBuff[i],pWorkingBuff[i+1],pWorkingBuff[i+2],pWorkingBuff[i+3],pWorkingBuff[i+4],pWorkingBuff[i+5],
		pWorkingBuff[i+6],pWorkingBuff[i+7],pWorkingBuff[i+8],pWorkingBuff[i+9],pWorkingBuff[i+10],pWorkingBuff[i+11],
		pWorkingBuff[i+12],pWorkingBuff[i+13],pWorkingBuff[i+14],pWorkingBuff[i+15]);			
		}
			i4RetValue = 0;
		}
	}	
	else if(ptempbuf->u4Offset==0x5a30) {
		if (imx219_cam_cal_read_data(ptempbuf->u4Offset, ptempbuf->u4Length, pWorkingBuff) 
			!= ptempbuf->u4Length){
			i4RetValue = -1;
		} else {
		for(i=0;i<=174;i=i+16)
		{
		CAM_CALDB("shen lsc 0x%02x,   0x%02x,   0x%02x,  0x%02x,  0x%02x,  0x%02x,  0x%02x,  0x%02x,  0x%02x,  0x%02x,  0x%02x,  0x%02x,  0x%02x,  0x%02x,  0x%02x,  0x%02x,\n",
		pWorkingBuff[i],pWorkingBuff[i+1],pWorkingBuff[i+2],pWorkingBuff[i+3],pWorkingBuff[i+4],pWorkingBuff[i+5],
		pWorkingBuff[i+6],pWorkingBuff[i+7],pWorkingBuff[i+8],pWorkingBuff[i+9],pWorkingBuff[i+10],pWorkingBuff[i+11],
		pWorkingBuff[i+12],pWorkingBuff[i+13],pWorkingBuff[i+14],pWorkingBuff[i+15]);			
		}
			i4RetValue = 0;
		}
	}
#endif
			//i4RetValue = iReadData(0x350a, ptempbuf->u4Length, pWorkingBuff);
			CAM_CALDB("[imx219otp_CAL] After read Working buffer data  0x%4x \n", pWorkingBuff[0]);			

  
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
            printk("Read data %d bytes take %lu us\n",ptempbuf->u4Length, TimeIntervalUS);
#endif            

            break;
        default :
      	     CAM_CALDB("[imx135otp_CAL] No CMD \n");
            i4RetValue = -EPERM;
        break;
    }

    if(_IOC_READ & _IOC_DIR(a_u4Command))
    {
        //copy data to user space buffer, keep other input paremeter unchange.
        CAM_CALDB("[imx219otp_CAL] to user length %d \n", ptempbuf->u4Length);
        //CAM_CALDB("[imx219otp_CAL] to user  Working buffer address 0x%8x \n", (u32)pWorkingBuff);
        if(copy_to_user((u8 __user *) ptempbuf->pu1Params , (u8 *)pWorkingBuff , ptempbuf->u4Length))
        {
            kfree(pBuff);
            kfree(pWorkingBuff);
            CAM_CALDB("[imx219otp_CAL] ioctl copy to user failed\n");
            return -EFAULT;
        }
    }

    kfree(pBuff);
    kfree(pWorkingBuff);
    return i4RetValue;
}


static u32 g_u4Opened = 0;
//#define
//Main jobs:
// 1.check for device-specified errors, device not ready.
// 2.Initialize the device if it is opened for the first time.
static int CAM_CAL_Open(struct inode * a_pstInode, struct file * a_pstFile)
{
    CAM_CALDB("[imx219_otp_CAL] CAM_CAL_Open\n");
    spin_lock(&g_CAM_CALLock);

    if(g_u4Opened)
    {
        spin_unlock(&g_CAM_CALLock);
		CAM_CALDB("[imx219otp_CAL] Opened, return -EBUSY\n");
        return -EBUSY;
    }
    else
    {
        g_u4Opened = 1;
        atomic_set(&g_CAM_CALatomic,0);
    }
    spin_unlock(&g_CAM_CALLock);

/*
    if(TRUE != hwPowerOn(MT65XX_POWER_LDO_VCAMA, VOL_2800, CAM_CAL_DRVNAME))
    {
        CAM_CALDB("[imx135otp_CAL] Fail to enable analog gain\n");
        return -EIO;
    }
*/
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
    .unlocked_ioctl = CAM_CAL_Ioctl
};

#define CAM_CAL_DYNAMIC_ALLOCATE_DEVNO 1
inline static int RegisterCAM_CALCharDrv(void)
{
    struct device* CAM_CAL_device = NULL;

#if CAM_CAL_DYNAMIC_ALLOCATE_DEVNO
    if( alloc_chrdev_region(&g_CAM_CALdevno, 0, 1,CAM_CAL_DRVNAME) )
    {
        CAM_CALDB("[imx219otp_CAL] Allocate device no failed\n");

        return -EAGAIN;
    }
#else
    if( register_chrdev_region(  g_CAM_CALdevno , 1 , CAM_CAL_DRVNAME) )
    {
        CAM_CALDB("[imx219otp_CAL] Register device no failed\n");

        return -EAGAIN;
    }
#endif

    //Allocate driver
    g_pCAM_CAL_CharDrv = cdev_alloc();

    if(NULL == g_pCAM_CAL_CharDrv)
    {
        unregister_chrdev_region(g_CAM_CALdevno, 1);

        CAM_CALDB("[imx219otp_CAL] Allocate mem for kobject failed\n");

        return -ENOMEM;
    }

    //Attatch file operation.
    cdev_init(g_pCAM_CAL_CharDrv, &g_stCAM_CAL_fops);

    g_pCAM_CAL_CharDrv->owner = THIS_MODULE;

    //Add to system
    if(cdev_add(g_pCAM_CAL_CharDrv, g_CAM_CALdevno, 1))
    {
        CAM_CALDB("[imx219otp_CAL] Attatch file operation failed\n");

        unregister_chrdev_region(g_CAM_CALdevno, 1);

        return -EAGAIN;
    }

    CAM_CAL_class = class_create(THIS_MODULE, "CAM_CALdrv");
    if (IS_ERR(CAM_CAL_class)) {
        int ret = PTR_ERR(CAM_CAL_class);
        CAM_CALDB("Unable to create class, err = %d\n", ret);
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


//////////////////////////////////////////////////////////////////////
#ifndef CAM_CAL_ICS_REVISION
static int CAM_CAL_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);
#elif 0
static int CAM_CAL_i2c_detect(struct i2c_client *client, struct i2c_board_info *info);
#else
#endif
//static int CAM_CAL_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
//static int CAM_CAL_i2c_remove(struct i2c_client *);

//static const struct i2c_device_id CAM_CAL_i2c_id[] = {{CAM_CAL_DRVNAME,0},{}};   
#if 0 //test110314 Please use the same I2C Group ID as Sensor
static unsigned short force[] = {CAM_CAL_I2C_GROUP_ID, IMX135OTP_DEVICE_ID, I2C_CLIENT_END, I2C_CLIENT_END};   
#else
//static unsigned short force[] = {IMG_SENSOR_I2C_GROUP_ID, IMX135OTP_DEVICE_ID, I2C_CLIENT_END, I2C_CLIENT_END};   
#endif
//static const unsigned short * const forces[] = { force, NULL };              
//static struct i2c_client_address_data addr_data = { .forces = forces,}; 

#if 0
static struct i2c_driver CAM_CAL_i2c_driver = {
    .probe = CAM_CAL_i2c_probe,                                   
    .remove = CAM_CAL_i2c_remove,                           
//   .detect = CAM_CAL_i2c_detect,                           
    .driver.name = CAM_CAL_DRVNAME,
    .id_table = CAM_CAL_i2c_id,                             
};
#endif
#ifndef CAM_CAL_ICS_REVISION
static int CAM_CAL_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info) {         
    strcpy(info->type, CAM_CAL_DRVNAME);
    return 0;
}
#endif
#if 0
static int CAM_CAL_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id) {             
int i4RetValue = 0;
    CAM_CALDB("[imx219otp_CAL] Attach I2C \n");

//    spin_lock_init(&g_CAM_CALLock);

    //get sensor i2c client
    spin_lock(&g_CAM_CALLock); //for SMP
    g_pstI2Cclient = client;
    g_pstI2Cclient->addr = IMX219OTP_DEVICE_ID>>1;
    spin_unlock(&g_CAM_CALLock); // for SMP    
    
    CAM_CALDB("[CAM_CAL] g_pstI2Cclient->addr = 0x%8x \n",g_pstI2Cclient->addr);
    //Register char driver
    i4RetValue = RegisterCAM_CALCharDrv();

    if(i4RetValue){
        CAM_CALDB("[imx219otp_CAL] register char device failed!\n");

        return i4RetValue;
    }

    CAM_CALDB("[imx219otp_CAL] Attached!! \n");
    return 0;                                                                                       
} 

static int CAM_CAL_i2c_remove(struct i2c_client *client)
{
    return 0;
}
#endif
static int CAM_CAL_probe(struct platform_device *pdev)
{
printk("shen imx219_otp CAM_CAL_probe\n ");
return 0;
  /*  return i2c_add_driver(&CAM_CAL_i2c_driver);*/
}

static int CAM_CAL_remove(struct platform_device *pdev)
{
   /* i2c_del_driver(&CAM_CAL_i2c_driver);*/
	
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

static int __init CAM_CAL_i2C_init(void)
{
	int i4RetValue = 0;
       CAM_CALDB("calling 0000 imx219otp_CAL driver\n");
	      printk("calling 11111register imx219otp_CAL driver\n");
           CAM_CALDB("CAM_CAL_i2C_init\n");
	/* Register char driver */
	i4RetValue = RegisterCAM_CALCharDrv();
	if (i4RetValue) {
		CAM_CALDB(" register char device failed!\n");
		return i4RetValue;
	}
 //   i2c_register_board_info(CAM_CAL_I2C_BUSNUM, &kd_cam_cal_dev, 1);

    if(platform_driver_register(&g_stCAM_CAL_Driver)){
        CAM_CALDB("failed to register imx219otp_CAL driver\n");
        return -ENODEV;
    }

    if (platform_device_register(&g_stCAM_CAL_Device))
    {
        CAM_CALDB("failed to register imx219otp_CAL driver, 2nd time\n");
        return -ENODEV;
    }	
   printk("calling ending  imx219otp_CAL driver\n");
    return 0;
}

static void __exit CAM_CAL_i2C_exit(void)
{
	platform_driver_unregister(&g_stCAM_CAL_Driver);
}

module_init(CAM_CAL_i2C_init);
module_exit(CAM_CAL_i2C_exit);



