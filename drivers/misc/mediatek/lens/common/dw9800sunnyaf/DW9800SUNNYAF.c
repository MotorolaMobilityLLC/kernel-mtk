/*
 * DW9714AF voice coil motor driver
 *
 *
 */

#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/fs.h>

#include "lens_info.h"


#define AF_DRVNAME "DW9800wSUNNYAF"
#define AF_I2C_SLAVE_ADDR        0x18
#define TRUE 1
#define AF_DEBUG
#ifdef AF_DEBUG
#define LOG_INF(format, args...) pr_debug(AF_DRVNAME " [%s] " format, __func__, ##args)
#else
#define LOG_INF(format, args...)
#endif

#define LENOVO_K5_M

static struct i2c_client *g_pstAF_I2Cclient;
static int *g_pAF_Opened;
static int g_dw9800_Opened =1;
static spinlock_t *g_pAF_SpinLock;

static int g_sr = 3;
static long g_i4Dir;
static long g_i4MotorStatus;
static unsigned long g_u4AF_INF;
static unsigned long g_u4AF_MACRO = 1023;
static unsigned long g_u4TargetPosition;
static unsigned long g_u4CurrPosition;

static int i2c_read(u8 a_u2Addr , u8 * a_puBuff)
{
    int  i4RetValue = 0;
    char puReadCmd[1] = {(char)(a_u2Addr)};
	i4RetValue = i2c_master_send(g_pstAF_I2Cclient, puReadCmd, 1);
	if (i4RetValue < 0) {
	    LOG_INF(" I2C write failed!! \n");
	    return -1;
	}
	//
	i4RetValue = i2c_master_recv(g_pstAF_I2Cclient, (char *)a_puBuff, 1);
	if (i4RetValue < 0) {
	    LOG_INF(" I2C read failed!! \n");
	    return -1;
	}
   
    return 0;
}


static u8 read_data(u8 addr)
{
	u8 get_byte=0;
    i2c_read( addr ,&get_byte);
    LOG_INF("[AF]  get_byte %d \n",  get_byte);
    return get_byte;
}
static int s4AF_ReadReg(unsigned short *a_pu2Result)
{
	/* int  i4RetValue = 0; */
	/* char pBuff[2]; */

	*a_pu2Result = (read_data(0x03) << 8) + (read_data(0x04) & 0xff);

	LOG_INF("[DW9800AF]  s4DW9800AF_ReadReg %d\n", *a_pu2Result);
	return 0;
}


static int  initdrv(void)
{
    int  i4RetValue = 0;

    char puSendCmd2[2] = {(char)(0x02) , (char)(0x02)};
    char puSendCmd3[2] = {(char)(0x06) , (char)(0x40)};
    char puSendCmd4[2] = {(char)(0x07) , (char)(0x79)};

  //  LOG_INF("g_sr %d, write %d \n", g_sr, a_u2Data);
    //g_pstDW9800AF_I2Cclient->ext_flag |= I2C_A_FILTER_MSG;
    i4RetValue = i2c_master_send(g_pstAF_I2Cclient, puSendCmd2, 2);
    i4RetValue = i2c_master_send(g_pstAF_I2Cclient, puSendCmd3, 2);
    i4RetValue = i2c_master_send(g_pstAF_I2Cclient, puSendCmd4, 2);
#if 0
	char puSendCmd2[2] = { 0x01, 0x39 };
	char puSendCmd3[2] = { 0x05, 0x65 };
	i2c_master_send(g_pstDW9800AF_I2Cclient, puSendCmd2, 2);
	i2c_master_send(g_pstDW9800AF_I2Cclient, puSendCmd3, 2);
#endif
	return 0;
}
//DW9800 add
static int WriteSensorReg(u8 reg, u8 data)
{
	int i4RetValue = 0;

	char puSendCmd[2] = { reg, data };

	LOG_INF("[DW9800AF]  write reg %x , data %x\n", reg, data);

	g_pstAF_I2Cclient->ext_flag |= I2C_A_FILTER_MSG;
	i4RetValue = i2c_master_send(g_pstAF_I2Cclient, puSendCmd, 2);

	if (i4RetValue < 0) {
		LOG_INF("[DW9800AF] I2C send failed!!\n");
		return -1;
	}

	return 0;
}

static inline int getAFInfo(__user stAF_MotorInfo *pstMotorInfo)
{
	stAF_MotorInfo stMotorInfo;
	stMotorInfo.u4MacroPosition = g_u4AF_MACRO;
	stMotorInfo.u4InfPosition = g_u4AF_INF;
	stMotorInfo.u4CurrentPosition = g_u4CurrPosition;
	stMotorInfo.bIsSupportSR = TRUE;

	if (g_i4MotorStatus == 1) {
		stMotorInfo.bIsMotorMoving = 1;
	} else {
		stMotorInfo.bIsMotorMoving = 0;
	}

	if (*g_pAF_Opened >= 1) {
		stMotorInfo.bIsMotorOpen = 1;
	} else {
		stMotorInfo.bIsMotorOpen = 0;
	}

	if (copy_to_user(pstMotorInfo, &stMotorInfo, sizeof(stAF_MotorInfo))) {
		LOG_INF("[DW9800AF] copy to user failed when getting motor information\n");
	}
	return 0;
}

static inline int moveAF(unsigned long a_u4Position)
{
	int ret = 0;
         int AFValueH, AFValueL;
        LOG_INF("[DW9800AF]  write moveAF\n");
	if ((a_u4Position > g_u4AF_MACRO) || (a_u4Position < g_u4AF_INF)) {
		LOG_INF("out of range\n");
		return -EINVAL;
	}

	if (*g_pAF_Opened == 1) {
		unsigned short InitPos;

		ret = s4AF_ReadReg(&InitPos);

		if (ret == 0) {
			LOG_INF("Init Pos %6d\n", InitPos);

			spin_lock(g_pAF_SpinLock);
			g_u4CurrPosition = (unsigned long)InitPos;
			spin_unlock(g_pAF_SpinLock);

		} else {
			spin_lock(g_pAF_SpinLock);
			g_u4CurrPosition = 0;
			spin_unlock(g_pAF_SpinLock);
		}

		spin_lock(g_pAF_SpinLock);
		*g_pAF_Opened = 2;
		spin_unlock(g_pAF_SpinLock);
	}
	
	if (g_u4CurrPosition < a_u4Position) {
		spin_lock(g_pAF_SpinLock);
		g_i4Dir = 1;
		spin_unlock(g_pAF_SpinLock);
	} else if (g_u4CurrPosition > a_u4Position) {
		spin_lock(g_pAF_SpinLock);
		g_i4Dir = -1;
		spin_unlock(g_pAF_SpinLock);
	} else {
		return 0;
	}

	
	if (g_u4CurrPosition == a_u4Position)
		return 0;
	/*lenovo-sw huangsh4  begin add g_dw9800_Opened flag to avoid the abnormal sound of vcm when first enter camera*/
         if (g_dw9800_Opened  == 1) {
	spin_lock(g_pAF_SpinLock);
	g_u4TargetPosition = g_u4CurrPosition;
	spin_unlock(g_pAF_SpinLock);
	g_dw9800_Opened = 2;
         	}else {
	spin_lock(g_pAF_SpinLock);
	g_u4TargetPosition = a_u4Position;
	spin_unlock(g_pAF_SpinLock);
         		}
        /*lenovo-sw huangsh4  end add g_dw9800_Opened flag to avoid the abnormal sound of vcm when first enter camera*/
	spin_lock(g_pAF_SpinLock);
	g_sr = 3;
	g_i4MotorStatus = 0;
	spin_unlock(g_pAF_SpinLock);

	AFValueH = ((unsigned short)g_u4TargetPosition>>8);
	AFValueL = ((unsigned short)g_u4TargetPosition & 0xff); 
	/* LOG_INF("move [curr] %ld [target] %ld\n", g_u4CurrPosition, g_u4TargetPosition); */
	//DW9800 add
	//if (s4DW9800AF_WriteReg((unsigned short)g_u4TargetPosition) == 0) {
	if (WriteSensorReg(0x03, AFValueH) == 0 && WriteSensorReg(0x04, AFValueL) == 0) {
		spin_lock(g_pAF_SpinLock);
		g_u4CurrPosition = (unsigned long)g_u4TargetPosition;
		spin_unlock(g_pAF_SpinLock);
	} else {
		LOG_INF("[DW9800AF] set I2C failed when moving the motor\n");
		spin_lock(g_pAF_SpinLock);
		g_i4MotorStatus = -1;
		spin_unlock(g_pAF_SpinLock);
	}

	return 0;
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
long DW9800wSUNNYAF_Ioctl(struct file *a_pstFile, unsigned int a_u4Command, unsigned long a_u4Param)
{
	long i4RetValue = 0;

	switch (a_u4Command) {
	case AFIOC_G_MOTORINFO:
		  LOG_INF("[DW9800AF]  write enter  getAFInfo\n");
		i4RetValue = getAFInfo((__user stAF_MotorInfo *) (a_u4Param));
		break;

	case AFIOC_T_MOVETO:
		LOG_INF("[DW9800AF]  write enter  moveAF\n");
		printk("[DW9800AF]  write enter  moveAF\n");
		i4RetValue = moveAF(a_u4Param);
		break;

	case AFIOC_T_SETINFPOS:
				LOG_INF("[DW9800AF]  write enter  setAFInf\n");
		printk("[DW9800AF]  write enter  setAFInf\n");
		i4RetValue = setAFInf(a_u4Param);
		break;

	case AFIOC_T_SETMACROPOS:
				LOG_INF("[DW9800AF]  write enter  setAFMacro\n");
		printk("[DW9800AF]  write enter  setAFMacro\n");
		i4RetValue = setAFMacro(a_u4Param);
		break;

	default:
		LOG_INF("No CMD\n");
		i4RetValue = -EPERM;
		break;
	}

	return i4RetValue;
}

//DW9800 add
void  DriverICReset_new(void) 
{
	printk("%s,%d\n", __func__, __LINE__);
	initdrv();
	//WriteSensorReg(0x02, 0x01);   //Power Down
	//msleep(30);
	WriteSensorReg(0x02, 0x02);    //Power On
	msleep(2);
	WriteSensorReg(0x03, 0x02);
	WriteSensorReg(0x04, 0x00);
	msleep(10);
}


/* Main jobs: */
/* 1.Deallocate anything that "open" allocated in private_data. */
/* 2.Shut down the device on last close. */
/* 3.Only called once on last time. */
/* Q1 : Try release multiple times. */
int DW9800wSUNNYAF_Release(struct inode *a_pstInode, struct file *a_pstFile)
{
	LOG_INF("[DW9800AF] DW9800AF_Release - Start\n");

    if (*g_pAF_Opened == 2)
    {
		g_sr = 5;    
    }

	if (*g_pAF_Opened) {
		LOG_INF("[DW9800AF] feee\n");
		//DW9800 add
		WriteSensorReg(0x02, 0x01);   //Power Down
		msleep(30);

		spin_lock(g_pAF_SpinLock);
		*g_pAF_Opened = 0;
		spin_unlock(g_pAF_SpinLock);

	}
	LOG_INF("[DW9800AF] DW9800AF_Release - End\n");


	return 0;
}

void DW9800wSUNNYAF_SetI2Cclient(struct i2c_client *pstAF_I2Cclient, spinlock_t *pAF_SpinLock, int *pAF_Opened)
{
	g_pstAF_I2Cclient = pstAF_I2Cclient;
	g_pAF_SpinLock = pAF_SpinLock;
	g_pAF_Opened = pAF_Opened;
	//motor_init();  //wuyt3 add  for k5 camera
	DriverICReset_new();
}
