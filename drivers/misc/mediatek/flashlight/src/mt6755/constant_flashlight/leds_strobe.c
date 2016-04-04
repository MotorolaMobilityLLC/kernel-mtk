/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifdef CONFIG_COMPAT

#include <linux/fs.h>
#include <linux/compat.h>

#endif
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/time.h>
#include "kd_flashlight.h"
#include <asm/io.h>
#include <asm/uaccess.h>
#include "kd_camera_typedef.h"
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/version.h>
#include <mt_pwm_hal.h>
#include <mt_pwm.h>
#include <linux/mutex.h>
#include <linux/i2c.h>
#include <linux/leds.h>

//#include "kd_camera_hw.h"  //wuyt3 add ofr K52M


/*
// flash current vs index
    0       1      2       3    4       5      6       7    8       9     10
93.74  140.63  187.5  281.25  375  468.75  562.5  656.25  750  843.75  937.5
     11    12       13      14       15    16
1031.25  1125  1218.75  1312.5  1406.25  1500mA
*/
/******************************************************************************
 * Debug configuration
******************************************************************************/
/* availible parameter */
/* ANDROID_LOG_ASSERT */
/* ANDROID_LOG_ERROR */
/* ANDROID_LOG_WARNING */
/* ANDROID_LOG_INFO */
/* ANDROID_LOG_DEBUG */
/* ANDROID_LOG_VERBOSE */

 static unsigned int times = 16;
 module_param(times,int,0644);
  static unsigned int debug = 0;
 module_param(debug,int,0644);
  static unsigned int f_duty = 10;
 module_param(f_duty,int,0644);
#define TAG_NAME "[leds_strobe.c]"
#define PK_DBG_NONE(fmt, arg...)    do {} while (0)
#define PK_DBG_FUNC(fmt, arg...)    pr_debug(TAG_NAME "%s: " fmt, __func__ , ##arg)

#undef PK_ERR
#define PK_ERR PK_DBG
#define DEBUG_LEDS_STROBE
#ifdef DEBUG_LEDS_STROBE
#define PK_DBG PK_DBG_FUNC
#else
#define PK_DBG(a, ...)
#endif

enum
{
	e_DutyNum = 9,
};
static int flashDuty[e_DutyNum]=     {0,16,15,14,13,12};//lenovo.sw 20160318wangsx3 change for K52a40,decrease flash current
/******************************************************************************
 * local variables
******************************************************************************/

static DEFINE_SPINLOCK(g_strobeSMPLock);	/* cotta-- SMP proection */


static u32 strobe_Res = 0;
static u32 strobe_Timeus = 0;
static BOOL g_strobe_On = 0;

static int g_duty=-1;
static int g_timeOutTimeMs=0;
static int g_timeTorchTimeMs=0;



#define FLASHLIGHT_DEVNAME            "SGM3785"
#define FLASH_SGM3785
struct flash_chip_data {
	struct led_classdev cdev_flash;
	struct led_classdev cdev_torch;

	struct mutex lock;    

	int mode;
	int torch_level;
};

static struct flash_chip_data chipconf;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
static DEFINE_MUTEX(g_strobeSem);
#else
static DECLARE_MUTEX(g_strobeSem);
#endif


#define STROBE_DEVICE_ID 0x60


static struct work_struct workTimeOut;
static struct work_struct workTimeTorch;

/*****************************************************************************
Functions
*****************************************************************************/
#define GPIO_ENF  9
#define GPIO_ENT 8


#define GPIO_OUT_ONE 1
#define GPIO_OUT_ZERO 0
#define GPIO_MODE_03 3
    /*CAMERA-FLASH-EN */

#ifndef FLASHLIGHT_ENT
#define FLASHLIGHT_ENT 10
#endif

#ifndef FLASHLIGHT_ENF
#define FLASHLIGHT_ENF  11
#endif

extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
static void work_timeOutFunc(struct work_struct *data);
extern int mtkcam_gpio_set(int PinIdx, int PwrType, int Val); //wuyt3 add for k52M

#if defined(FLASH_BD7710)

#elif		defined(FLASH_SGM3785)

//#define GPIO_ENF 				(GPIO12|0x80000000)
//#define GPIO_ENT				(GPIO106|0x80000000)
#define TORCH_BRIGHTNESS 		0
#define FLASH_BRIGHTNESS 		5///lenovo.sw 20160318wangsx3 change for K52a40,decrease flash current
#define FLASH_BRIGHTNESS_MAX	15
#define FLASH_ENT_PIN_M_GPIO		GPIO_MODE_00
#define FLASH_ENT_PIN_M_PWM		GPIO_MODE_03
#define FLASH_ENT_PIN_GPIO_H		(1)
#define FLASH_ENT_PIN_GPIO_L		(0)
struct 	pwm_spec_config	pwm_setting={
						.pwm_no=PWM2,
						.mode=PWM_MODE_OLD,
						.clk_div=CLK_DIV1,//lenovo.sw wangsx3 change for AIO
						.clk_src=PWM_CLK_OLD_MODE_32K,//lenovo.sw wangsx3 change for AIO
						.pmic_pad=false,
						.PWM_MODE_OLD_REGS.IDLE_VALUE=0,
						.PWM_MODE_OLD_REGS.GUARD_VALUE=0,
						.PWM_MODE_OLD_REGS.GDURATION=0,
						.PWM_MODE_OLD_REGS.WAVE_NUM=0,
						.PWM_MODE_OLD_REGS.DATA_WIDTH=10,//leovo.sw wangsx change pwm period
						.PWM_MODE_OLD_REGS.THRESH=5};


void FL_PWM_SetTorchBegin(void){

	pwm_setting.PWM_MODE_OLD_REGS.IDLE_VALUE=1;
	pwm_setting.PWM_MODE_OLD_REGS.WAVE_NUM=1;
	pwm_setting.PWM_MODE_OLD_REGS.THRESH=255;
	pwm_set_spec_config(&pwm_setting);
	msleep(5);
}
void FL_PWM_SetFlashBegin(void){

	pwm_setting.PWM_MODE_OLD_REGS.IDLE_VALUE=0;
	pwm_setting.PWM_MODE_OLD_REGS.WAVE_NUM=0;
	pwm_setting.PWM_MODE_OLD_REGS.THRESH=0;
}
void FL_PWM_SetTorchEnd(void){

	pwm_setting.PWM_MODE_OLD_REGS.IDLE_VALUE=0;
	pwm_setting.PWM_MODE_OLD_REGS.WAVE_NUM=1;
	pwm_setting.PWM_MODE_OLD_REGS.THRESH=1;
	pwm_set_spec_config(&pwm_setting);
}



/*wuyt3 add for K52M*/
void  	FL_ENT_SETMODE_GPIO(UINT8  mLevele){
	if(mLevele == 0){
		if(mtkcam_gpio_set(0,FLASHLIGHT_ENT,0))
			PK_ERR("[CAMERA FLASHLIGHT_ENT] set gpio failed!!\n");
	}else if(mLevele == 1){
		if(mtkcam_gpio_set(0,FLASHLIGHT_ENT,1))
			PK_ERR("[CAMERA FLASHLIGHT_ENT] set gpio failed!!\n");
	}else if(mLevele == 2){				//set to PWM mode
		if(mtkcam_gpio_set(0,FLASHLIGHT_ENT,2))
			PK_ERR("[CAMERA FLASHLIGHT_ENT] set gpio failed!!\n");
	}
}
void  	FL_ENF_SETMODE_GPIO(UINT8  mLevele){
	if(mLevele == 0){
		if(mtkcam_gpio_set(0,FLASHLIGHT_ENF,0))
			PK_ERR("[CAMERA FLASHLIGHT_ENF] set gpio failed!!\n");
	}else if(mLevele == 1){
		if(mtkcam_gpio_set(0,FLASHLIGHT_ENF,1))
			PK_ERR("[CAMERA FLASHLIGHT_ENF] set gpio failed!!\n");
	}
}
void 	FL_ENT_SETMODE_PWM(void){

	FL_ENT_SETMODE_GPIO(2);
}

//wuyt3 added because the previous code use so many this function, so midify it for temp solution as haven't enough time 
static void mt_set_gpio_out(u32 pin , u32 data){
	if(pin == GPIO_ENT)
		FL_ENT_SETMODE_GPIO(data);
	else if(pin == GPIO_ENF)
		FL_ENF_SETMODE_GPIO(data);
}
//end
int FL_PWM_SetBrightness(UINT32 mBrightness){

	pwm_setting.PWM_MODE_OLD_REGS.THRESH=mBrightness;
	pwm_set_spec_config(&pwm_setting);
	return 0;
}
/*
static void workTimeTorchFunc(struct work_struct *data)
{
	FL_ENT_SETMODE_PWM();
	FL_PWM_SetBrightness(125);
	PK_DBG("%s\n",__func__);
}
*/
enum hrtimer_restart TorchTimeOutCallback(struct hrtimer *timer)
{
   
    schedule_work(&workTimeTorch); 
    PK_DBG("%s\n",__func__);
    return HRTIMER_NORESTART;
}
static struct hrtimer g_timeTorchTimer;
void Torch_timerInit(void)
{
	g_timeTorchTimeMs=6; //1s
	hrtimer_init( &g_timeTorchTimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL );
	g_timeTorchTimer.function=TorchTimeOutCallback;

}
int FL_Enable(void)
{
	//int brightness = 0;
	int i = 0;
	PK_DBG("Suny duty=%d\r\n",g_duty);
	if(g_duty == 0)//torch
	{
	#if 0
		brightness=10;//lenovo.sw wangsx change torch pwm duty
		//ktime_t kTorchtime;
		//g_timeTorchTimeMs=6;
		mt_set_gpio_out(GPIO_ENF,GPIO_OUT_ZERO);
		FL_ENT_SETMODE_GPIO(FLASH_ENT_PIN_GPIO_H);
		mdelay(4);
		FL_ENT_SETMODE_PWM();
		FL_PWM_SetBrightness(brightness);
		//kTorchtime = ktime_set( 0, g_timeTorchTimeMs*1000000 );
		//hrtimer_start( &g_timeTorchTimer, kTorchtime, HRTIMER_MODE_REL );
		PK_DBG(" Suny FL_enable  torch brightness=%d g_duty=%d line=%d\n",brightness,g_duty,__LINE__);
	#endif
		FL_ENT_SETMODE_GPIO(FLASH_ENT_PIN_GPIO_H);
		mdelay(5);
		FL_ENF_SETMODE_GPIO(FLASH_ENT_PIN_GPIO_L);
		PK_DBG(" Suny FL_enable  torchg_duty=%d line=%d\n",g_duty,__LINE__);
	}
	else if((g_duty >= (TORCH_BRIGHTNESS+1))&&(g_duty <= FLASH_BRIGHTNESS))
	{
	#if 0
		brightness=(g_duty*10)/(FLASH_BRIGHTNESS);////lenovo.sw wangsx3 change for aio flash
		mt_set_gpio_out(GPIO_ENF,GPIO_OUT_ZERO);
		FL_ENT_SETMODE_PWM();
		FL_PWM_SetBrightness(brightness);
		udelay(50);
		mt_set_gpio_out(GPIO_ENF,GPIO_OUT_ONE);
		PK_DBG("Suny FL_enable flash brightness=%d g_duty=%d line=%d\n",brightness,g_duty,__LINE__);
	#endif
		for (i =1;i<=flashDuty[g_duty];i++)
			{			
			mt_set_gpio_out(GPIO_ENT,GPIO_OUT_ZERO);
			udelay(8);
		         FL_ENT_SETMODE_GPIO(FLASH_ENT_PIN_GPIO_H);
		 	 udelay(8);
			}
		mt_set_gpio_out(GPIO_ENF,GPIO_OUT_ONE);
		PK_DBG("Suny FL_enable flash g_duty=%d line=%d\n",g_duty,__LINE__);
	}
    	return 0;
}

int FL_Disable(void)
{
#if 0
	int brightness=0;
	PK_DBG("Suny SGM %s\r\n",__func__);
	mt_set_gpio_out(GPIO_ENF,GPIO_OUT_ZERO);
	FL_ENT_SETMODE_PWM();
	mt_pwm_disable(pwm_setting.pwm_no,pwm_setting.pmic_pad);

    	return 0;
#endif
	mt_set_gpio_out(GPIO_ENT,GPIO_OUT_ZERO);
	mt_set_gpio_out(GPIO_ENF,GPIO_OUT_ZERO);
	PK_DBG(" FL_Disable line=%d\n",__LINE__);
    return 0;
}

int FL_dim_duty(kal_uint32 duty)
{
	g_duty=duty;
	PK_DBG(" Suny FL_dim_duty=%d line=%d\n",duty,__LINE__);
    	return 0;
}


int FL_Init(void)
{
	PK_DBG("Suny SGM %s\r\n",__func__);
	mt_set_gpio_out(GPIO_ENT, GPIO_OUT_ZERO);
	INIT_WORK(&workTimeOut, work_timeOutFunc);	
	//INIT_WORK(&workTimeTorch,workTimeTorchFunc);
   	return 0;
}


int FL_Uninit(void)
{
	PK_DBG("Suny SGM %s\r\n",__func__);
	FL_Disable();
    	return 0;
}
#else
int FL_Enable(void)
{
	if(g_duty==0)
	{
		mt_set_gpio_out(GPIO_ENT,GPIO_OUT_ONE);
		mt_set_gpio_out(GPIO_ENF,GPIO_OUT_ZERO);
		PK_DBG(" FL_Enable line=%d\n",__LINE__);
	}
	else
	{
		mt_set_gpio_out(GPIO_ENT,GPIO_OUT_ZERO);
		mt_set_gpio_out(GPIO_ENF,GPIO_OUT_ONE);
		PK_DBG(" FL_Enable line=%d\n",__LINE__);
	}

    return 0;
}



int FL_Disable(void)
{

	mt_set_gpio_out(GPIO_ENT,GPIO_OUT_ZERO);
	mt_set_gpio_out(GPIO_ENF,GPIO_OUT_ZERO);
	PK_DBG(" FL_Disable line=%d\n",__LINE__);
    return 0;
}

int FL_dim_duty(kal_uint32 duty)
{
	g_duty=duty;
	PK_DBG(" FL_dim_duty line=%d\n",__LINE__);
    return 0;
}


int FL_Init(void)
{

/*wuyt3 add for K52M*/
	FL_ENT_SETMODE_GPIO(0); 
    	FL_ENF_SETMODE_GPIO(0);
/*wuyt3 add end*/
	INIT_WORK(&workTimeOut, work_timeOutFunc);
    PK_DBG(" FL_Init line=%d\n",__LINE__);
    return 0;
}


int FL_Uninit(void)
{
	FL_Disable();
	return 0;
}
#endif
/*****************************************************************************
User interface
*****************************************************************************/

static void work_timeOutFunc(struct work_struct *data)
{
	FL_Disable();
	PK_DBG("ledTimeOut_callback\n");
}



enum hrtimer_restart ledTimeOutCallback(struct hrtimer *timer)
{
	schedule_work(&workTimeOut);
	return HRTIMER_NORESTART;
}

static struct hrtimer g_timeOutTimer;
void timerInit(void)
{
	g_timeOutTimeMs=1000; //1s
	hrtimer_init( &g_timeOutTimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL );
	g_timeOutTimer.function=ledTimeOutCallback;

}



static int constant_flashlight_ioctl(unsigned int  cmd, unsigned long  arg)
{
	int i4RetValue = 0;
	int ior_shift;
	int iow_shift;
	int iowr_shift;
	ior_shift = cmd - (_IOR(FLASHLIGHT_MAGIC,0, int));
	iow_shift = cmd - (_IOW(FLASHLIGHT_MAGIC,0, int));
	iowr_shift = cmd - (_IOWR(FLASHLIGHT_MAGIC,0, int));
	PK_DBG("constant_flashlight_ioctl() line=%d ior_shift=%d, iow_shift=%d iowr_shift=%d arg=%d\n",__LINE__, ior_shift, iow_shift, iowr_shift, (int)arg);
    switch(cmd)
    {

	case FLASH_IOC_SET_TIME_OUT_TIME_MS:
		PK_DBG("FLASH_IOC_SET_TIME_OUT_TIME_MS: %d\n", (int)arg);
		g_timeOutTimeMs = arg;
		break;


	case FLASH_IOC_SET_DUTY:
		PK_DBG("FLASHLIGHT_DUTY: %d\n", (int)arg);
		FL_dim_duty(arg);
		break;


	case FLASH_IOC_SET_STEP:
		PK_DBG("FLASH_IOC_SET_STEP: %d\n", (int)arg);

		break;

	case FLASH_IOC_SET_ONOFF:
		PK_DBG("FLASHLIGHT_ONOFF: %d\n", (int)arg);
		if (arg == 1) {
			if (g_timeOutTimeMs != 0) {
				ktime_t ktime;

				ktime = ktime_set(0, g_timeOutTimeMs * 1000000);
				hrtimer_start(&g_timeOutTimer, ktime, HRTIMER_MODE_REL);
			}
			FL_Enable();
		} else {
			FL_Disable();
			hrtimer_cancel(&g_timeOutTimer);
		}
		break;
	default:
		PK_DBG(" No such command\n");
		i4RetValue = -EPERM;
		break;
	}
	return i4RetValue;
}




static int constant_flashlight_open(void *pArg)
{
	int i4RetValue = 0;

	PK_DBG("constant_flashlight_open line=%d\n", __LINE__);

	if (0 == strobe_Res) {
		FL_Init();
		timerInit();
	}
	PK_DBG("constant_flashlight_open line=%d\n", __LINE__);
	spin_lock_irq(&g_strobeSMPLock);


	if (strobe_Res) {
		PK_DBG(" busy!\n");
		i4RetValue = -EBUSY;
	} else {
		strobe_Res += 1;
	}


	spin_unlock_irq(&g_strobeSMPLock);
	PK_DBG("constant_flashlight_open line=%d\n", __LINE__);

	return i4RetValue;

}


static int constant_flashlight_release(void *pArg)
{
	PK_DBG(" constant_flashlight_release\n");

	if (strobe_Res) {
		spin_lock_irq(&g_strobeSMPLock);

		strobe_Res = 0;
		strobe_Timeus = 0;

		/* LED On Status */
		g_strobe_On = FALSE;

		spin_unlock_irq(&g_strobeSMPLock);

		FL_Uninit();
	}

	PK_DBG(" Done\n");

	return 0;

}


FLASHLIGHT_FUNCTION_STRUCT constantFlashlightFunc = {
	constant_flashlight_open,
	constant_flashlight_release,
	constant_flashlight_ioctl
};


MUINT32 constantFlashlightInit(PFLASHLIGHT_FUNCTION_STRUCT *pfFunc)
{
	if (pfFunc != NULL)
		*pfFunc = &constantFlashlightFunc;
	return 0;
}



/* LED flash control for high current capture mode*/
ssize_t strobe_VDIrq(void)
{

	return 0;
}
EXPORT_SYMBOL(strobe_VDIrq);


/***************                   *******************/
#if 1//def CONFIG_LENOVO
static void chip_torch_brightness_set(struct led_classdev *cdev,
				  enum led_brightness brightness)
{
	//int i, cc;
	struct flash_chip_data *chip = &chipconf;
	//u8 tmp4,tmp5;
	PK_DBG("[flashchip] torch brightness = %d\n",brightness);
	#if defined(FLASH_BD7710)
	if(brightness == 0)
	{
		BD7710_write_reg(BD7710_i2c_client, 0x02,0x88); //75ma torch output_en'
		BD7710_write_reg(BD7710_i2c_client, 0x01, 0xC0); //750ma flash output_en
		udelay(50);
		mt_set_gpio_out(GPIO_ENF, GPIO_OUT_ONE);
	    mt_set_gpio_out(RESET_PIN, GPIO_OUT_ZERO);
		mdelay(1);
	    mt_set_gpio_out(EN_PIN, GPIO_OUT_ZERO);

		return;
	}
	else
	{
		mt_set_gpio_out(EN_PIN, GPIO_OUT_ONE);
		mdelay(1);
		mt_set_gpio_out(RESET_PIN, GPIO_OUT_ONE);
		mdelay(1);
		BD7710_write_reg(BD7710_i2c_client, 0x02, 0x88);
		BD7710_write_reg(BD7710_i2c_client, 0x01, 0x50); //75ma torch output_en'
		udelay(50);
		mt_set_gpio_out(GPIO_ENF, GPIO_OUT_ZERO);
	}
	#elif defined(FLASH_RT9387)
	if(brightness == 0)
	{
		mt_set_gpio_out(GPIO_ENF,GPIO_OUT_ZERO);
		mt_set_gpio_out(GPIO_ENT,GPIO_OUT_ZERO);
		mdelay(4);
		chip->torch_level = 0;
		chip->mode = 0;
		PK_ERR("[flashchip] level = 0\n");

		return;
	}
	else
	{
		mt_set_gpio_out(GPIO_ENF,GPIO_OUT_ZERO);
		mt_set_gpio_out(GPIO_ENT,GPIO_OUT_ONE);
		mdelay(4);
		chip->torch_level = 0;
		chip->mode = 0;	
	}
	/*lenovo-sw sunliang add for SGM3785 2014_3_31 begin*/
	#elif defined(FLASH_SGM3785)
	if(brightness == 0)
	{
		mt_set_gpio_out(GPIO_ENF,GPIO_OUT_ZERO);
		mt_set_gpio_out(GPIO_ENT,GPIO_OUT_ZERO);
		#if 0
		FL_ENT_SETMODE_PWM();
		mt_pwm_disable(pwm_setting.pwm_no,pwm_setting.pmic_pad);
		#endif
		chip->torch_level = 0;
		chip->mode = 0;
		PK_DBG("[flashchip] level = 0\n");
	}
	else
	{
		//brightness=5;
		//ktime_t kTorchtime;
		//g_timeTorchTimeMs=6;

		FL_ENT_SETMODE_GPIO(FLASH_ENT_PIN_GPIO_H);
		mdelay(5);
		FL_ENF_SETMODE_GPIO(FLASH_ENT_PIN_GPIO_L);
		//mt_set_gpio_out(GPIO_ENF,GPIO_OUT_ZERO);
		if (debug) {
			FL_ENT_SETMODE_GPIO(2);
		//FL_ENT_SETMODE_PWM();
		FL_PWM_SetBrightness(f_duty);
		chip->torch_level = 0;
		chip->mode = 0;	
	}else {
		chip->torch_level = 0;
		chip->mode = 0;	
	}
	}
	/*lenovo-sw sunliang add for SGM3785 2014_3_31 end*/
	#endif
}


static void chip_flash_brightness_set(struct led_classdev *cdev,
				  enum led_brightness brightness)
{
	struct flash_chip_data *chip = &chipconf;
//	u8 tmp4,tmp5;
	u8 i;
	PK_ERR("[flashchip] flash brightness = %d\n",brightness);
	#if defined(FLASH_BD7710)
	if(brightness == 0)
	{
		BD7710_write_reg(BD7710_i2c_client, 0x02,0x88); //75ma torch output_en'
		BD7710_write_reg(BD7710_i2c_client, 0x01, 0xC0); //750ma flash output_en
		udelay(50);
		mt_set_gpio_out(GPIO_ENF, GPIO_OUT_ONE);
	    mt_set_gpio_out(RESET_PIN, GPIO_OUT_ZERO);
		mdelay(1);
	    mt_set_gpio_out(EN_PIN, GPIO_OUT_ZERO);
	}
	else
	{
		mt_set_gpio_out(EN_PIN, GPIO_OUT_ONE);
		mdelay(1);
		mt_set_gpio_out(RESET_PIN, GPIO_OUT_ONE);
		mdelay(1);
		BD7710_write_reg(BD7710_i2c_client, 0x02,0x88); //75ma torch output_en'
		BD7710_write_reg(BD7710_i2c_client, 0x01, 0x9A); //750ma flash output_en
		udelay(50);
		mt_set_gpio_out(GPIO_ENF, GPIO_OUT_ZERO);
		PK_ERR("[flashchip] flash level = 1\n");
	}
	#elif defined(FLASH_RT9387)
	if(brightness == 0)
	{
		mt_set_gpio_out(GPIO_ENF,GPIO_OUT_ZERO);
		mt_set_gpio_out(GPIO_ENT,GPIO_OUT_ZERO);
		mdelay(4);
		chip->torch_level = 0;
		chip->mode = 0;
		PK_ERR("[flashchip] flash level = 0\n");
	}
	else
	{
		mt_set_gpio_out(GPIO_ENF,GPIO_OUT_ONE);
		mt_set_gpio_out(GPIO_ENT,GPIO_OUT_ZERO);
		//mdelay(4);
		chip->torch_level = 0;
		chip->mode = 2;
		PK_ERR("[flashchip] flash level = 1\n");
	}
	/*lenovo-sw sunliang add for SGM3785 2014_3_31 begin*/
	#elif defined(FLASH_SGM3785)
	if(brightness == 0)
	{
		mt_set_gpio_out(GPIO_ENF,GPIO_OUT_ZERO);
		mt_set_gpio_out(GPIO_ENT,GPIO_OUT_ZERO);
		FL_ENT_SETMODE_PWM();
		mt_pwm_disable(pwm_setting.pwm_no,pwm_setting.pmic_pad);
		chip->torch_level = 0;
		chip->mode = 0;
		PK_ERR("[flashchip] level = 0\n");
	}
	else
	{
	#if 0
		FL_ENT_SETMODE_GPIO(FLASH_ENT_PIN_GPIO_H);
		mdelay(5);
		FL_ENT_SETMODE_PWM();
		FL_PWM_SetBrightness(10);
		udelay(50);
		mt_set_gpio_out(GPIO_ENF,GPIO_OUT_ONE);
		chip->torch_level = 0;
		chip->mode = 2;	
         #endif

		 
		for (i =1;i<=times;i++)
			{
			
			mt_set_gpio_out(GPIO_ENT,GPIO_OUT_ZERO);
			udelay(8);
		         FL_ENT_SETMODE_GPIO(FLASH_ENT_PIN_GPIO_H);
		 	 udelay(8);
			}
		mt_set_gpio_out(GPIO_ENF,GPIO_OUT_ONE);
	//	mt_set_gpio_out(GPIO_ENT,GPIO_OUT_ZERO);
		chip->torch_level = 0;
		chip->mode = 2;	
	}
	/*lenovo-sw sunliang add for SGM3785 2014_3_31 end*/
	#endif
	return;
}


static int flashchip_probe(struct platform_device *dev)
{
	struct flash_chip_data *chip;

	PK_ERR("[flashchip_probe] start\n");
	chip = &chipconf;
	chip->mode = 0;
	chip->torch_level = 0;
	mutex_init(&chip->lock);

	//flash
	chip->cdev_flash.name="flash";
	chip->cdev_flash.max_brightness = 1;
	chip->cdev_flash.brightness_set = chip_flash_brightness_set;
	if(led_classdev_register((struct device *)&dev->dev,&chip->cdev_flash)<0)
		goto err_create_flash_file;	
	//torch
	chip->cdev_torch.name="torch";
	chip->cdev_torch.max_brightness = 16;
	chip->cdev_torch.brightness_set = chip_torch_brightness_set;
	if(led_classdev_register((struct device *)&dev->dev,&chip->cdev_torch)<0)
		goto err_create_torch_file;

    PK_ERR("[flashchip_probe] Done\n");
    return 0;

err_create_torch_file:
	led_classdev_unregister(&chip->cdev_flash);
err_create_flash_file:	
	printk(KERN_ERR "[flashchip_probe] is failed !\n");
	return -ENODEV;



}

static int flashchip_remove(struct platform_device *dev)
{
	struct flash_chip_data *chip = &chipconf;
    PK_DBG("[flashchip_remove] start\n");

	led_classdev_unregister(&chip->cdev_torch);
	led_classdev_unregister(&chip->cdev_flash);


    PK_DBG("[flashchip_remove] Done\n");
    return 0;
}


static struct platform_driver flashchip_platform_driver =
{
    .probe      = flashchip_probe,
    .remove     = flashchip_remove,
    .driver     = {
        .name = FLASHLIGHT_DEVNAME,
		.owner	= THIS_MODULE,
    },
};



static struct platform_device flashchip_platform_device = {
    .name = FLASHLIGHT_DEVNAME,
    .id = 0,
    .dev = {
//    	.platform_data	= &chip,
    }
};

static int __init flashchip_init(void)
{
    int ret = 0;
    printk("[flashchip_init] start\n");

	ret = platform_device_register (&flashchip_platform_device);
	if (ret) {
        PK_ERR("[flashchip_init] platform_device_register fail\n");
        return ret;
	}

    ret = platform_driver_register(&flashchip_platform_driver);
	if(ret){
		PK_ERR("[flashchip_init] platform_driver_register fail\n");
		return ret;
	}

	printk("[flashchip_init] done!\n");
    return ret;
}

static void __exit flashchip_exit(void)
{
    printk("[flashchip_exit] start\n");
    platform_driver_unregister(&flashchip_platform_driver);
    printk("[flashchip_exit] done!\n");
}

/*****************************************************************************/
module_init(flashchip_init);
module_exit(flashchip_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zhangjiano@lenovo.com>");
MODULE_DESCRIPTION("Factory mode flash control Driver");
#endif


