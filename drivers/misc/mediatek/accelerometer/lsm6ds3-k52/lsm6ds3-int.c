/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

/* ST LSM6DS3 Accelerometer and Gyroscope sensor driver
 *
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <sensors_io.h>
#include <linux/kernel.h>
//#include <mach/mt_pm_ldo.h>
//#include <cust_eint.h>
//#include <mach/eint.h>
#include <linux/dma-mapping.h>
/*lenovo-sw caoyi modify for eint API 20151021 begin*/
#include <linux/of.h>
#include <linux/of_irq.h>
/*lenovo-sw caoyi modify for eint API 20151021 end*/
#include <mt-plat/mt_gpio.h>
#include <mach/gpio_const.h>
#include <linux/gpio.h>
#include <mach/upmu_sw.h>
#include "lsm6ds3-int.h"



#define POWER_NONE_MACRO MT65XX_POWER_NONE

#define LSM6DS3_NEW_ARCH				//kk and L compatialbe
#define LSM6DS3_M
#define LSM6DS3_STEP_COUNTER 			//it depends on the MACRO LSM6DS3_NEW_ARCH
//#define LSM6DS3_TILT_FUNC 				//dependency on LSM6DS3_STEP_COUNTER
//#define LSM6DS3_SIGNIFICANT_MOTION  	//dependency on LSM6DS3_STEP_COUNTER
//#define LSM6DS3_FIFO_SUPPORT

/*lenovo-sw caoyi1 add for Gsensor APK calibration 20150512 begin*/
//#define LSM6DS3_MTK_CALIB                             //CALIB USED MTK fatctory MODE
#define LSM6DS3_CALIB_HAL                             // CALIB FROM LENOVO HAL
/*lenovo-sw caoyi1 add for Gsensor APK calibration 20150512 end*/

#ifndef LSM6DS3_NEW_ARCH		//new sensor type depend on new arch
#undef LSM6DS3_STEP_COUNTER
#undef LSM6DS3_TILT_FUNC
#undef LSM6DS3_SIGNIFICANT_MOTION
#endif


#ifndef LSM6DS3_STEP_COUNTER		//significant_motion depend on step_counter
#undef LSM6DS3_SIGNIFICANT_MOTION
#endif


#include <cust_acc.h>

#ifdef LSM6DS3_NEW_ARCH
#include <accel.h>
#ifdef LSM6DS3_STEP_COUNTER //step counter
#include <step_counter.h>
#endif

#ifdef LSM6DS3_TILT_FUNC //tilt detector
#include <tilt_detector.h>
#endif

#endif
/****************************************************************/
#if (defined(LSM6DS3_TILT_FUNC) || defined(LSM6DS3_SIGNIFICANT_MOTION)  || defined(LSM6DS3_STEP_COUNTER))
/*lenovo-sw caoyi1 modify for GPIO pin 20150325 begin*/
#define GPIO_LSM6DS3_EINT_PIN GPIO_ACCEL_GYRO_INT1
#define GPIO_LSM6DS3_EINT_PIN_M_EINT GPIO_ACCEL_GYRO_INT1_M_EINT
#define CUST_EINT_LSM6DS3_NUM CUST_EINT_ACCEL_GYRO_INT1_NUM
/*lenovo-sw caoyi1 modify for GPIO pin 20150325 end*/
#define CUST_EINT_LSM6DS3_DEBOUNCE_CN CUST_EINT_ACCEL_GYRO_INT1_DEBOUNCE_CN //debounce time
#define CUST_EINT_LSM6DS3_TYPE CUST_EINT_ACCEL_GYRO_INT1_TYPE	//eint trigger type

/* lenovo-sw liaoxl add for int2 pin of lsm6ds3  20150505 start */
#define GPIO_LSM6DS3_EINT2_PIN GPIO_ACCEL_GYRO_INT2
#define GPIO_LSM6DS3_EINT2_PIN_M_EINT GPIO_ACCEL_GYRO_INT2_M_EINT
#define CUST_EINT2_LSM6DS3_NUM CUST_EINT_ACCEL_GYRO_INT2_NUM
#define CUST_EINT2_LSM6DS3_DEBOUNCE_CN CUST_EINT_ACCEL_GYRO_INT2_DEBOUNCE_CN //debounce time
#define CUST_EINT2_LSM6DS3_TYPE CUST_EINT_ACCEL_GYRO_INT2_TYPE	//eint trigger type
/* lenovo-sw liaoxl add for int2 pin of lsm6ds3  20150505 start */
#endif

/*lenovo-sw caoyi1 add for selftest function 20150527 begin*/
#define LSM6DS3_FTM_ENABLE 1
#ifdef LSM6DS3_FTM_ENABLE
#define ABS_ST(X) ((X) < 0 ? (-1 * (X)) : (X))
#define MIN_ST   90
#define MAX_ST  1700
#endif
/*lenovo-sw caoyi1 add for selftest function 20150527 end*/

/*---------------------------------------------------------------------------*/
#define DEBUG 1
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
#define LSM6DS3_ACC_DATA_LEN        6   
#define LSM6DS3_GYRO_DATA_LEN       6   
#define LSM6DS3_ACC_DEV_NAME        "LSM6DS3_ACCEL"
/*----------------------------------------------------------------------------*/
struct acc_hw lsm6ds3_accel_cust;
static struct acc_hw *hw = &lsm6ds3_accel_cust;

/*For driver get cust info*/
struct acc_hw *get_cust_acc_hw(void)
{
	return &lsm6ds3_accel_cust;
}

static const struct i2c_device_id lsm6ds3_i2c_id[] = {{LSM6DS3_ACC_DEV_NAME,0},{}};
/*lenovo-sw caoyi1 modify A+G sensro iic address 20150417 begin*/
//static struct i2c_board_info __initdata i2c_lsm6ds3={ I2C_BOARD_INFO(LSM6DS3_ACC_DEV_NAME, (0xD5>>1))};
/*lenovo-sw caoyi1 modify A+G sensro iic address 20150417 end*/

/*----------------------------------------------------------------------------*/
static int lsm6ds3_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int lsm6ds3_i2c_remove(struct i2c_client *client);
//static int lsm6ds3_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);
static int LSM6DS3_init_client(struct i2c_client *client, bool enable);

static int LSM6DS3_acc_SetPowerMode(struct i2c_client *client, bool enable);

static int LSM6DS3_ReadAccRawData(struct i2c_client *client, s16 data[LSM6DS3_ACC_AXES_NUM]);
#ifndef CONFIG_HAS_EARLYSUSPEND

static int lsm6ds3_acc_suspend(struct i2c_client *client, pm_message_t msg);
static int lsm6ds3_acc_resume(struct i2c_client *client);
#endif
static int LSM6DS3_acc_SetSampleRate(struct i2c_client *client, u8 sample_rate);

//static int LSM6DS3_acc_Enable_Func(struct i2c_client *client, LSM6DS3_ACC_GYRO_FUNC_EN_t newValue);
//static int LSM6DS3_Int_Ctrl(struct i2c_client *client, LSM6DS3_ACC_GYRO_INT_ACTIVE_t int_act, LSM6DS3_ACC_GYRO_INT_LATCH_CTL_t int_latch);

#ifdef LSM6DS3_STEP_COUNTER //step counter
static int LSM6DS3_acc_Enable_Pedometer_Func(struct i2c_client *client, bool enable);

static int LSM6DS3_Write_PedoThreshold(struct i2c_client *client, u8 newValue);
static int LSM6DS3_Reset_Pedo_Data(struct i2c_client *client, LSM6DS3_ACC_GYRO_PEDO_RST_STEP_t newValue);

static int LSM6DS3_acc_Enable_Func(struct i2c_client *client, LSM6DS3_ACC_GYRO_FUNC_EN_t newValue);
static int LSM6DS3_Int_Ctrl(struct i2c_client *client, LSM6DS3_ACC_GYRO_INT_ACTIVE_t int_act, LSM6DS3_ACC_GYRO_INT_LATCH_CTL_t int_latch);
#ifdef LSM6DS3_SIGNIFICANT_MOTION

static int LSM6DS3_Enable_SigMotion_Func(struct i2c_client *client, LSM6DS3_ACC_GYRO_SIGN_MOT_t newValue);
#endif
#endif
#ifdef LSM6DS3_TILT_FUNC //tilt detector

static int LSM6DS3_Enable_Tilt_Func(struct i2c_client *client, bool enable);
static int LSM6DS3_Enable_Tilt_Func_On_Int(struct i2c_client *client, LSM6DS3_ACC_GYRO_ROUNT_INT_t tilt_int, bool enable);


#endif

#ifdef LSM6DS3_FIFO_SUPPORT
int lsm6ds3_reconfigure_fifo(struct lsm6ds3_i2c_data *cdata,
						bool disable_irq_and_flush);
#endif
static int lsm6ds3_setup_eint(void);



/*----------------------------------------------------------------------------*/
#ifdef CONFIG_OF
static const struct of_device_id accel_of_match[] = {
	{.compatible = "mediatek,gsensor"},
	{}
};
#endif

static struct i2c_driver lsm6ds3_i2c_driver = {
    .driver = {
        .owner          = THIS_MODULE,
        .name           = LSM6DS3_ACC_DEV_NAME,
#ifdef CONFIG_OF
	.of_match_table = accel_of_match,
#endif
    },
	.probe      		= lsm6ds3_i2c_probe,
	.remove    			= lsm6ds3_i2c_remove,
#if !defined(CONFIG_HAS_EARLYSUSPEND)    
    .suspend            = lsm6ds3_acc_suspend,
    .resume             = lsm6ds3_acc_resume,
#endif
	.id_table = lsm6ds3_i2c_id,
};
#ifdef LSM6DS3_NEW_ARCH
static int lsm6ds3_local_init(void);
static int lsm6ds3_local_uninit(void);
static int lsm6ds3_acc_init_flag = -1;
static unsigned long lsm6ds3_init_flag_test = 0; //initial state
static DEFINE_MUTEX(lsm6ds3_init_mutex);
static DEFINE_MUTEX(lsm6ds3_acc_mutex);
typedef enum {
	CFG_LSM6DS3_ACC = 1,
	CFG_LSM6DS3_STEP_C = 2,
	CFG_LSM6DS3_TILT = 3,
}LSM6DS3_INIT_TYPE;
static struct acc_init_info  lsm6ds3_init_info = {
	.name   = LSM6DS3_ACC_DEV_NAME,
	.init   = lsm6ds3_local_init,
	.uninit = lsm6ds3_local_uninit,
};

#ifdef LSM6DS3_STEP_COUNTER
static int lsm6ds3_step_c_local_init(void);
static int lsm6ds3_step_c_local_uninit(void);
static struct step_c_init_info  lsm6ds3_step_c_init_info = {
	.name   = "LSM6DS3_STEP_C",
	.init   = lsm6ds3_step_c_local_init,
	.uninit = lsm6ds3_step_c_local_uninit,
};
#endif
#ifdef LSM6DS3_TILT_FUNC
static int lsm6ds3_tilt_local_init(void);
static int lsm6ds3_tilt_local_uninit(void);
static struct tilt_init_info  lsm6ds3_tilt_init_info = {
	.name   = "LSM6DS3_TILT",
	.init   = lsm6ds3_tilt_local_init,
	.uninit = lsm6ds3_tilt_local_uninit,
};
#endif

#endif
/*----------------------------------------------------------------------------*/
static struct i2c_client *lsm6ds3_i2c_client = NULL;

#ifndef LSM6DS3_NEW_ARCH
static struct platform_driver lsm6ds3_driver;
#endif

static struct lsm6ds3_i2c_data *obj_i2c_data = NULL;
static bool sensor_power = false;
static bool enable_status = false;
static bool pedo_enable_status = false;
static bool tilt_enable_status = false;

static bool sigm_enable_status = false;  /* add for significant motioon sensor -- by liaoxl.lenovo 5.12.2015 */
static bool stepd_enable_status = false; /* add for step detector sensor -- by liaoxl.lenovo 7.12.2015 */

/*lenovo-sw caoyi1 add for Gsensor APK calibration 20150512 begin*/
#ifdef LSM6DS3_CALIB_HAL
static int calib_offset[3] = {0,0,0};                  // ug unit £¬ used for lenovo hal calib
#endif
/*lenovo-sw caoyi1 add for Gsensor APK calibration 20150512 end*/

/*----------------------------------------------------------------------------*/

#define GSE_TAG                  "[accel] "

#define GSE_FUN(f)               printk(KERN_INFO GSE_TAG" - %s called...\n", __FUNCTION__)
#define GSE_ERR(fmt, args...)    printk(KERN_ERR GSE_TAG " %s %d : " fmt, __FUNCTION__, __LINE__, ##args)
#define GSE_LOG(fmt, args...)    printk(KERN_INFO GSE_TAG " %s %d : " fmt, __FUNCTION__, __LINE__, ##args)

/*----------------------------------------------------------------------------*/
#define IIC_MAX_TRANSFER_SIZE         8
#define IIC_DMA_MAX_TRANSFER_SIZE     250
#define LSM6DS3_ADDR_LENGTH 1
#define I2C_MASTER_CLOCK 400
static u8 *gpDMABuf_va = NULL;
static dma_addr_t gpDMABuf_pa = 0;
struct mutex dma_mutex;
DEFINE_MUTEX(dma_mutex);


static int lsm6ds3_i2c_read(struct lsm6ds3_i2c_data *cdata, u8 reg_addr, int len,
							u8 *data, bool b_lock)
{
	s32 ret = 0;
	s32 pos = 0;
	struct i2c_client *client = cdata->client;
	s32 transfer_length;
	u8 addr_buf[LSM6DS3_ADDR_LENGTH] = { 0 };

	if (!client)
		return -EINVAL;

	if(len > 8)
	{
		struct i2c_msg msgs[2] = {
			{
			 .flags = 0,	//!I2C_M_RD,
			 .addr = (client->addr & I2C_MASK_FLAG),
			 .timing = I2C_MASTER_CLOCK,
			 .len = LSM6DS3_ADDR_LENGTH,
			 .buf = addr_buf,
			 },
			{
			 .flags = I2C_M_RD,
			 .ext_flag = (client->ext_flag | I2C_ENEXT_FLAG | I2C_DMA_FLAG),
			 .addr = (client->addr & I2C_MASK_FLAG),
			 .timing = I2C_MASTER_CLOCK,
			 .buf = (u8 *) gpDMABuf_pa,
			 },
		};

		mutex_lock(&dma_mutex);
		while (pos != len) {
			if (len - pos > IIC_DMA_MAX_TRANSFER_SIZE) {
				transfer_length = IIC_DMA_MAX_TRANSFER_SIZE;
			} else {
				transfer_length = len - pos;
			}

			msgs[0].buf[0] = reg_addr;
			msgs[1].len = transfer_length;

			ret = i2c_transfer(client->adapter, msgs, 2);
			if (ret != 2) {
				GSE_ERR("I2C Transfer error! (%d)\n", ret);
				ret = -EIO;
				break;
			}
			memcpy(&data[pos], gpDMABuf_va, transfer_length);
			pos += transfer_length;
		};
		mutex_unlock(&dma_mutex);
		if(2 == ret)
		{
			ret = 0;
		}
	}
	else
	{
		struct i2c_msg msgs[2] = {
			{
				.addr = client->addr,	.flags = 0,
				.len = 1,	.buf = addr_buf,
			},
			{
				.addr = client->addr,	.flags = I2C_M_RD,
				.len = len,	.buf = data,
			}
		};
		
		msgs[0].buf[0] = reg_addr;
		mutex_lock(&dma_mutex);
		ret = i2c_transfer(client->adapter, msgs, sizeof(msgs)/sizeof(msgs[0]));
		mutex_unlock(&dma_mutex);
		if (ret != 2) {
			GSE_ERR("I2C Transfer error! (%d)\n", ret);
			ret = -EIO;
		} else {
			ret = 0;
		}
	}

	return ret;
}


static int lsm6ds3_i2c_write(struct lsm6ds3_i2c_data *cdata, u8 reg_addr, int len,
							u8 *data, bool b_lock)
{
	s32 ret = 0;
	s32 pos = 0;
	s32 transfer_length;
	struct i2c_client *client = cdata->client;

	if(len > 8)
	{
		struct i2c_msg msg = {
			.flags = !I2C_M_RD,
			.ext_flag = (client->ext_flag | I2C_ENEXT_FLAG | I2C_DMA_FLAG),
			.addr = (client->addr & I2C_MASK_FLAG),
			.timing = I2C_MASTER_CLOCK,
			.buf = (u8 *) gpDMABuf_pa,
		};

		mutex_lock(&dma_mutex);
		while (pos != len) {
			if (len - pos > (IIC_DMA_MAX_TRANSFER_SIZE - LSM6DS3_ADDR_LENGTH)) {
				transfer_length = IIC_DMA_MAX_TRANSFER_SIZE - LSM6DS3_ADDR_LENGTH;
			} else {
				transfer_length = len - pos;
			}

			gpDMABuf_va[0] = reg_addr;
			memcpy(&gpDMABuf_va[LSM6DS3_ADDR_LENGTH], &data[pos], transfer_length);

			msg.len = transfer_length + LSM6DS3_ADDR_LENGTH;

			ret = i2c_transfer(client->adapter, &msg, 1);
			if (ret != 1) {
				GSE_ERR("I2c Transfer error! (%d)\n", ret);
				ret = -EIO;
				break;
			}
			//ret = 0;
			pos += transfer_length;
		}
		mutex_unlock(&dma_mutex);
		if(ret ==1)
		{
			ret = 0;
		}
	}
	else
	{
		u8 buf[8];
		int num, idx;

	    num = 0;
	    buf[num++] = reg_addr;
    	for (idx = 0; idx < len; idx++)
        	buf[num++] = data[idx];
		mutex_lock(&dma_mutex);
	    ret = i2c_master_send(client, buf, num);
	    mutex_unlock(&dma_mutex);
    	if (ret < 0) {
        	GSE_ERR("send command error!!\n");
        	return -EFAULT;
    	} else {
	        ret = 0;    /*no error*/
    	}
	}

	return ret;
}


static const struct lsm6ds3_transfer_function lsm6ds3_tf_i2c = {
	.write = lsm6ds3_i2c_write,
	.read = lsm6ds3_i2c_read,
};


static void LSM6DS3_dumpReg(struct i2c_client *client)
{
/*lenovo-sw caoyi1 modify for debug 20150325 begin*/
	int i=0;
	u8 addr = 0x0C;
	u8 regdata=0;
	struct lsm6ds3_i2c_data *obj = i2c_get_clientdata(client);

	for(i=0; i<35 ; i++)
	{
		//dump all
		//hwmsen_read_byte(client,addr,&regdata);
		obj->tf->read(obj, addr, 1, &regdata, true);
		GSE_LOG("Reg addr=%x regdata=%x\n",addr,regdata);
		addr++;	
	}

	addr = 0x49;
	for(i=0x49; i<0x60; i++)
	{
		//dump all
		//hwmsen_read_byte(client,addr,&regdata);
		obj->tf->read(obj, addr, 1, &regdata, true);
		GSE_LOG("Reg addr=%x regdata=%x\n",addr,regdata);
		addr++;
	}
/*lenovo-sw caoyi1 modify for debug 20150325 end*/
}

static void LSM6DS3_power(struct acc_hw *hw, unsigned int on) 
{
#if 0
	static unsigned int power_on = 0;

	if (hw->power_id != POWER_NONE_MACRO)		// have externel LDO
	{        
		GSE_LOG("power %s\n", on ? "on" : "off");
		if (power_on == on)	// power status not change
		{
			GSE_LOG("ignore power control: %d\n", on);
		}
		else if(on)	// power on
		{
			if (!hwPowerOn((MT65XX_POWER)hw->power_id, hw->power_vol, "LSM6DS3"))
			{
				GSE_ERR("power on fails!!\n");
			}
		}
		else	// power off
		{
			if (!hwPowerDown(hw->power_id, "LSM6DS3"))
			{
				GSE_ERR("power off fail!!\n");
			}			  
		}
	}
	power_on = on;    
#endif
}
/*----------------------------------------------------------------------------*/

static int LSM6DS3_acc_write_rel_calibration(struct lsm6ds3_i2c_data *obj, int dat[LSM6DS3_GYRO_AXES_NUM])
{
    obj->cali_sw[LSM6DS3_AXIS_X] = obj->cvt.sign[LSM6DS3_AXIS_X]*dat[obj->cvt.map[LSM6DS3_AXIS_X]];
    obj->cali_sw[LSM6DS3_AXIS_Y] = obj->cvt.sign[LSM6DS3_AXIS_Y]*dat[obj->cvt.map[LSM6DS3_AXIS_Y]];
    obj->cali_sw[LSM6DS3_AXIS_Z] = obj->cvt.sign[LSM6DS3_AXIS_Z]*dat[obj->cvt.map[LSM6DS3_AXIS_Z]];
#if DEBUG		
		if(atomic_read(&obj->trace) & ACCEL_TRC_CALI)
		{
			GSE_LOG("test  (%5d, %5d, %5d) ->(%5d, %5d, %5d)->(%5d, %5d, %5d))\n", 
				obj->cvt.sign[LSM6DS3_AXIS_X],obj->cvt.sign[LSM6DS3_AXIS_Y],obj->cvt.sign[LSM6DS3_AXIS_Z],
				dat[LSM6DS3_AXIS_X], dat[LSM6DS3_AXIS_Y], dat[LSM6DS3_AXIS_Z],
				obj->cvt.map[LSM6DS3_AXIS_X],obj->cvt.map[LSM6DS3_AXIS_Y],obj->cvt.map[LSM6DS3_AXIS_Z]);
			GSE_LOG("write gyro calibration data  (%5d, %5d, %5d)\n", 
				obj->cali_sw[LSM6DS3_AXIS_X],obj->cali_sw[LSM6DS3_AXIS_Y],obj->cali_sw[LSM6DS3_AXIS_Z]);
		}
#endif
    return 0;
}

/*----------------------------------------------------------------------------*/
static int LSM6DS3_acc_ResetCalibration(struct i2c_client *client)
{
	struct lsm6ds3_i2c_data *obj = i2c_get_clientdata(client);	

	memset(obj->cali_sw, 0x00, sizeof(obj->cali_sw));
	return 0;    
}

/*----------------------------------------------------------------------------*/
static int LSM6DS3_acc_ReadCalibration(struct i2c_client *client, int dat[LSM6DS3_GYRO_AXES_NUM])
{
    struct lsm6ds3_i2c_data *obj = i2c_get_clientdata(client);

    dat[obj->cvt.map[LSM6DS3_AXIS_X]] = obj->cvt.sign[LSM6DS3_AXIS_X]*obj->cali_sw[LSM6DS3_AXIS_X];
    dat[obj->cvt.map[LSM6DS3_AXIS_Y]] = obj->cvt.sign[LSM6DS3_AXIS_Y]*obj->cali_sw[LSM6DS3_AXIS_Y];
    dat[obj->cvt.map[LSM6DS3_AXIS_Z]] = obj->cvt.sign[LSM6DS3_AXIS_Z]*obj->cali_sw[LSM6DS3_AXIS_Z];

#if DEBUG		
		if(atomic_read(&obj->trace) & ACCEL_TRC_CALI)
		{
			GSE_LOG("Read gyro calibration data  (%5d, %5d, %5d)\n", 
				dat[LSM6DS3_AXIS_X],dat[LSM6DS3_AXIS_Y],dat[LSM6DS3_AXIS_Z]);
		}
#endif
                                       
    return 0;
}
/*----------------------------------------------------------------------------*/

static int LSM6DS3_acc_WriteCalibration(struct i2c_client *client, int dat[LSM6DS3_GYRO_AXES_NUM])
{
	struct lsm6ds3_i2c_data *obj = i2c_get_clientdata(client);
	int err = 0;
	int cali[LSM6DS3_GYRO_AXES_NUM];

	GSE_FUN();
	if(!obj || ! dat)
	{
		GSE_ERR("null ptr!!\n");
		return -EINVAL;
	}
	else
	{        		
		cali[obj->cvt.map[LSM6DS3_AXIS_X]] = obj->cvt.sign[LSM6DS3_AXIS_X]*obj->cali_sw[LSM6DS3_AXIS_X];
		cali[obj->cvt.map[LSM6DS3_AXIS_Y]] = obj->cvt.sign[LSM6DS3_AXIS_Y]*obj->cali_sw[LSM6DS3_AXIS_Y];
		cali[obj->cvt.map[LSM6DS3_AXIS_Z]] = obj->cvt.sign[LSM6DS3_AXIS_Z]*obj->cali_sw[LSM6DS3_AXIS_Z]; 
		cali[LSM6DS3_AXIS_X] += dat[LSM6DS3_AXIS_X];
		cali[LSM6DS3_AXIS_Y] += dat[LSM6DS3_AXIS_Y];
		cali[LSM6DS3_AXIS_Z] += dat[LSM6DS3_AXIS_Z];
#if DEBUG		
		if(atomic_read(&obj->trace) & ACCEL_TRC_CALI)
		{
			GSE_LOG("write gyro calibration data  (%5d, %5d, %5d)-->(%5d, %5d, %5d)\n", 
				dat[LSM6DS3_AXIS_X], dat[LSM6DS3_AXIS_Y], dat[LSM6DS3_AXIS_Z],
				cali[LSM6DS3_AXIS_X],cali[LSM6DS3_AXIS_Y],cali[LSM6DS3_AXIS_Z]);
		}
#endif
		return LSM6DS3_acc_write_rel_calibration(obj, cali);
	} 

	return err;
}
/*----------------------------------------------------------------------------*/
static int LSM6DS3_CheckDeviceID(struct i2c_client *client)
{
	u8 databuf[2];    
	int res;
	struct lsm6ds3_i2c_data *obj = i2c_get_clientdata(client);

	databuf[0] = LSM6DS3_FIXED_DEVID;
	databuf[1] = 0;
	//res = hwmsen_read_byte(client,LSM6DS3_WHO_AM_I,databuf);
	res = 	obj->tf->read(obj, LSM6DS3_WHO_AM_I, 1, databuf, true);
    GSE_LOG(" LSM6DS3  LSM6DS3_CheckDeviceID = 0x%x ret=%d!\n", databuf[0], res);
	if(databuf[0]!=LSM6DS3_FIXED_DEVID)
	{
		return LSM6DS3_ERR_IDENTIFICATION;
	}

	if (res < 0)
	{
		return LSM6DS3_ERR_I2C;
	}
	
	return LSM6DS3_SUCCESS;
}

#ifdef LSM6DS3_TILT_FUNC //tilt detector
static int LSM6DS3_enable_tilt(struct i2c_client *client, bool enable)
{
	int res = 0;
	struct lsm6ds3_i2c_data *obj = i2c_get_clientdata(client);//obj_i2c_data;
	
	if(enable)
	{
		//set ODR to 26 hz		
		//res = LSM6DS3_acc_SetSampleRate(client, LSM6DS3_ACC_ODR_26HZ);
		res = LSM6DS3_acc_SetSampleRate(client, obj->sample_rate);
		if(LSM6DS3_SUCCESS == res)
		{
			GSE_LOG(" %s set 26hz odr to acc\n", __func__);
		}			
		
		res = LSM6DS3_Enable_Tilt_Func(client, enable);
		if(res != LSM6DS3_SUCCESS)
		{
			GSE_LOG(" LSM6DS3_Enable_Tilt_Func failed!\n");
			return LSM6DS3_ERR_STATUS;
		}
		
		res = LSM6DS3_acc_Enable_Func(client, LSM6DS3_ACC_GYRO_FUNC_EN_ENABLED);	
		if(res != LSM6DS3_SUCCESS)
		{
			GSE_LOG(" LSM6DS3_acc_Enable_Func failed!\n");
			return LSM6DS3_ERR_STATUS;
		}		
		
		res = LSM6DS3_Enable_Tilt_Func_On_Int(client, LSM6DS3_ACC_GYRO_INT1, true);  //default route to INT1 	
		if(res != LSM6DS3_SUCCESS)
		{
			GSE_LOG(" LSM6DS3_Enable_Tilt_Func_On_Int failed!\n");
			return LSM6DS3_ERR_STATUS;
		}
/*Lenovo-sw weimh1 mod 2016-4-20 begin: just enable without condition*/
#if 0
		if(0 == atomic_read(&obj->int1_request_num))
#endif	
/*Lenovo-sw weimh1 mod 2016-4-20 end*/	
		{
/*lenovo-sw caoyi modify for eint API 20151021 begin*/
#if defined(CONFIG_OF)
			enable_irq(obj_i2c_data->irq);
#else
			mt_eint_unmask(CUST_EINT_LSM6DS3_NUM);
#endif
/*lenovo-sw caoyi modify for eint API 20151021 end*/
		}
		atomic_inc(&obj->int1_request_num);
	}
	else
	{
		res = LSM6DS3_Enable_Tilt_Func(client, enable);
		if(res != LSM6DS3_SUCCESS)
		{
			GSE_LOG(" LSM6DS3_Enable_Tilt_Func failed!\n");
			return LSM6DS3_ERR_STATUS;
		}
		if(!enable_status && !pedo_enable_status && !sigm_enable_status)
		{
			res = LSM6DS3_acc_SetPowerMode(client, false);
			if(res != LSM6DS3_SUCCESS)
			{
				GSE_LOG(" LSM6DS3_acc_SetPowerMode failed!\n");
				return LSM6DS3_ERR_STATUS;
			}
		}
		atomic_dec(&obj->int1_request_num);
		if(0 >= atomic_read(&obj->int1_request_num))
		{
			atomic_set(&obj->int1_request_num, 0);
			cancel_work_sync(&obj->eint_work);
/*lenovo-sw caoyi modify for eint API 20151021 begin*/
#if defined(CONFIG_OF)
//			disable_irq(obj_i2c_data->irq);
#else
			mt_eint_mask(CUST_EINT_LSM6DS3_NUM);
#endif
/*lenovo-sw caoyi modify for eint API 20151021 end*/
		}
	}
//	tilt_enable_status = enable;
	return LSM6DS3_SUCCESS;
}
#endif

#ifdef LSM6DS3_STEP_COUNTER //step counter
#ifdef LSM6DS3_FIFO_SUPPORT
static int lsm6ds3_write_data_with_mask(struct lsm6ds3_i2c_data *cdata,
				u8 reg_addr, u8 mask, u8 data, bool b_lock)
{
	int err;
	u8 new_data = 0x00, old_data = 0x00;

	err = cdata->tf->read(cdata, reg_addr, 1, &old_data, b_lock);
	if (err < 0)
		return err;

	new_data = ((old_data & (~mask)) | ((data << __ffs(mask)) & mask));

	if (new_data == old_data)
		return 1;

	return cdata->tf->write(cdata, reg_addr, 1, &new_data, b_lock);
}
#endif

static int LSM6DS3_enable_pedo_On_Int(struct i2c_client *client, bool enable)
{
	u8 databuf[2] = {0}; 
	int res = 0;
	struct lsm6ds3_i2c_data *obj = i2c_get_clientdata(client);
	GSE_FUN();    

	/* route INT2 to INT1 pin */
	if(obj->tf->read(obj, LSM6DS3_CTRL4_C, 1, databuf, true))
	{
		GSE_ERR("%s read LSM6DS3_CTRL4_C register err!\n", __func__);
		return LSM6DS3_ERR_I2C;
	}
	else
	{
		GSE_LOG("read  LSM6DS3_CTRL4_C register: 0x%x\n", databuf[0]);
	}
	databuf[1] = databuf[0] | 0x20;
	databuf[0] = LSM6DS3_CTRL4_C;
	res = obj->tf->write(obj, databuf[0], 1, &databuf[1], true);
	if(res < 0)
	{
		GSE_ERR("write LSM6DS3_CTRL4_C register err!\n");
		return LSM6DS3_ERR_I2C;
	}

	//if(hwmsen_read_byte(client, LSM6DS3_TAP_CFG, databuf))
	if(obj->tf->read(obj, LSM6DS3_TAP_CFG, 1, databuf, true))
	{
		GSE_ERR("read LSM6DS3_TAP_CFG register err!\n");
		return LSM6DS3_ERR_I2C;
	}
	else
	{
		GSE_LOG("read LSM6DS3_TAP_CFG register: 0x%x\n", databuf[0]);
	}
	if(enable)
	{
		databuf[0] &= ~LSM6DS3_TIMER_EN;//clear 
		databuf[0] |= LSM6DS3_TIMER_EN_ENABLED;
	}
	else
	{
		databuf[0] &= ~LSM6DS3_TIMER_EN;//clear 
		databuf[0] |= LSM6DS3_TIMER_EN_DISABLED;
	}
	databuf[1] = databuf[0];
	databuf[0] = LSM6DS3_TAP_CFG;
	//res = i2c_master_send(client, databuf, 0x2);
	res = obj->tf->write(obj, databuf[0], 1, &databuf[1], true);
	if(res < 0)
	{
		GSE_ERR("write LSM6DS3_TAP_CFG func register err!\n");
		return LSM6DS3_ERR_I2C;
	}

	if(enable)
	{
		/* enable step count delta func */
		/* step2 TIMER_HR bit = 0 */
		//if(hwmsen_read_byte(client, LSM6DS3_WAKE_UP_DUR, databuf))
		if(obj->tf->read(obj, LSM6DS3_WAKE_UP_DUR, 1, databuf, true))
		{
			GSE_ERR("read acc wakeup dur register err!\n");
			return LSM6DS3_ERR_I2C;
		}
		else
		{
			GSE_LOG("read  acc wakeup dur register: 0x%x\n", databuf[0]);
		}
		databuf[0] &= ~0x10;
		databuf[1] = databuf[0];
		databuf[0] = LSM6DS3_WAKE_UP_DUR; 	
		//res = i2c_master_send(client, databuf, 0x2);
		res = obj->tf->write(obj, databuf[0], 1, &databuf[1], true);
		if(res < 0)
		{
			GSE_ERR("write acc wakeup func register err!\n");
			return LSM6DS3_ERR_I2C;
		}

		/* step3 STEP_COUNT_DELTA value, 1LSB = 1.6384S */
		/* treat as batch timeout value */
		databuf[1] = 0x80;
		databuf[0] = LSM6DS3_FUNC_CFG_ACCESS; 	
		//res = i2c_master_send(client, databuf, 0x2);
		res = obj->tf->write(obj, databuf[0], 1, &databuf[1], true);
		if(res < 0)
		{
			GSE_ERR("%s turnon embedded func access err1!\n", __func__);
			return LSM6DS3_ERR_I2C;
		}
		databuf[1] = 3;  // 3 steps for test
		databuf[0] = LSM6DS3_STEP_COUNT_DELTA; 
		//res = i2c_master_send(client, databuf, 0x2);
		res = obj->tf->write(obj, databuf[0], 1, &databuf[1], true);
		if(res < 0)
		{
			GSE_ERR("%s write LSM6DS3_STEP_COUNT_DELTA register err2!\n", __func__);
			return LSM6DS3_ERR_I2C;
		}
		databuf[1] = 0x00;
		databuf[0] = LSM6DS3_FUNC_CFG_ACCESS;
		//res = i2c_master_send(client, databuf, 0x2);
		res = obj->tf->write(obj, databuf[0], 1, &databuf[1], true);
		if(res < 0)
		{
			GSE_ERR("%s turnoff embedded func access err!\n", __func__);
			return LSM6DS3_ERR_I2C;
		}

#ifdef LSM6DS3_FIFO_SUPPORT
		/* step4 step counter& timestamp as 4th FIFO data set */
		/*       step detecter as FIFO DRDY */
		res = lsm6ds3_write_data_with_mask(obj,
							LSM6DS3_FIFO_PEDO_E_ADDR,
							LSM6DS3_FIFO_PEDO_E_MASK,
							0x03, true);
		if(res < 0)
		{
			GSE_ERR("%s write LSM6DS3_FIFO_PEDO_E_ADDR register err!\n", __func__);
			return LSM6DS3_ERR_I2C;
		}

		res = lsm6ds3_reconfigure_fifo(obj, true);
		if(res != LSM6DS3_SUCCESS)
		{
			GSE_LOG(" lsm6ds3_reconfigure_fifo failed!\n");
			return LSM6DS3_ERR_STATUS;
		}
#endif
	}
	else
	{
		/* anything else? */
	}

	/*enable INT2_STEP_DELTA/INT2_STEP_COUNT_OV/INT2_FULL_FLAG/INT2_FIFO_OVR/INT2_FTH bit*/
	//if(hwmsen_read_byte(client, LSM6DS3_INT2_CTRL, databuf))
	if(obj->tf->read(obj, LSM6DS3_INT2_CTRL, 1, databuf, true))
	{
		GSE_ERR("%s read LSM6DS3_INT2_CTRL register err!\n", __func__);
		return LSM6DS3_ERR_I2C;
	}
	else
	{
		GSE_LOG("read  LSM6DS3_INT2_CTRL register: 0x%x\n", databuf[0]);
	}
#ifdef LSM6DS3_FIFO_SUPPORT
	if(enable)
	{
		databuf[0] |= 0xF8;
	}
	else
	{
		databuf[0] &= ~0xF8;//clear 
	}
	/* turn off step counter delta intterrupt */
	if(atomic_read(&obj->suspend))
	{
		databuf[0] &= ~0x80;//clear 
	}
	else
	{
		databuf[0] |= 0x80;
	}
#else
	if(enable)
	{
		databuf[0] |= 0x40;
	}
	else
	{
		databuf[0] &= ~0x40;//clear 
	}
	/* turn off step counter delta intterrupt */
	if(atomic_read(&obj->suspend))
	{
		databuf[0] &= ~0x80;//clear 
	}
	else
	{
		databuf[0] |= 0x80;
	}
#endif
	databuf[1] = databuf[0];
	databuf[0] = LSM6DS3_INT2_CTRL;
	//res = i2c_master_send(client, databuf, 0x2);
	res = obj->tf->write(obj, databuf[0], 1, &databuf[1], true);
	if(res < 0)
	{
		GSE_ERR("write LSM6DS3_INT2_CTRL register err!\n");
		return LSM6DS3_ERR_I2C;
	}

	res = LSM6DS3_Int_Ctrl(client, LSM6DS3_ACC_GYRO_INT_ACTIVE_LOW, LSM6DS3_ACC_GYRO_INT_LATCH);
	if(res < 0)
	{
		GSE_ERR(" call LSM6DS3_Int_Ctrl func return err!\n");
		return LSM6DS3_ERR_I2C;
	}

	if(true == enable)
	{
/*Lenovo-sw weimh1 mod 2016-4-20 begin: just enable without condition*/
#if 0
		if(0 == atomic_read(&obj->int1_request_num))
#endif		
		{
/*lenovo-sw caoyi modify for eint API 20151021 begin*/
#if defined(CONFIG_OF)
			enable_irq(obj_i2c_data->irq);
#else
			mt_eint_unmask(CUST_EINT_LSM6DS3_NUM);
#endif
			GSE_LOG("enable step counter eint\n");
		}
		atomic_inc(&obj->int1_request_num);
#ifdef LSM6DS3_FIFO_SUPPORT
/*lenovo-sw caoyi modify for eint API 20151021 begin*/
#if defined(CONFIG_OF)
		enable_irq(obj_i2c_data->irq2);
#else
		mt_eint_unmask(CUST_EINT2_LSM6DS3_NUM);
#endif
/*lenovo-sw caoyi modify for eint API 20151021 end*/
#endif
	}
	else
	{
		atomic_dec(&obj->int1_request_num);
		if(0 >= atomic_read(&obj->int1_request_num))
		{
			atomic_set(&obj->int1_request_num, 0);
			cancel_work_sync(&obj->eint_work);
/*lenovo-sw caoyi modify for eint API 20151021 begin*/
#if defined(CONFIG_OF)
			//disable_irq(obj_i2c_data->irq);
#else
			mt_eint_mask(CUST_EINT_LSM6DS3_NUM);
#endif
			GSE_LOG("disable step counter eint\n");
		}
#ifdef LSM6DS3_FIFO_SUPPORT
/*lenovo-sw caoyi modify for eint API 20151021 begin*/
#if defined(CONFIG_OF)
		disable_irq(obj_i2c_data->irq2);
#else
		mt_eint_mask(CUST_EINT2_LSM6DS3_NUM);
#endif
/*lenovo-sw caoyi modify for eint API 20151021 end*/
#endif
	}

	return LSM6DS3_SUCCESS;
}


static int LSM6DS3_enable_pedo(struct i2c_client *client, bool enable)
{
//	u8 databuf[2] = {0};    
	int res = 0;
	struct lsm6ds3_i2c_data *obj = i2c_get_clientdata(client);//obj_i2c_data;
	GSE_FUN();

	if(true == enable)
	{	
		//software reset
		//set ODR to 26 hz		
		//res = LSM6DS3_acc_SetSampleRate(client, LSM6DS3_ACC_ODR_26HZ);
		res = LSM6DS3_acc_SetSampleRate(client, obj->sample_rate);
		if(LSM6DS3_SUCCESS == res)
		{
			GSE_LOG(" %s set 26hz odr to acc\n", __func__);
		}
		
		res = LSM6DS3_acc_Enable_Func(client, LSM6DS3_ACC_GYRO_FUNC_EN_ENABLED);	
		if(res != LSM6DS3_SUCCESS)
		{
			GSE_LOG(" LSM6DS3_acc_Enable_Func failed!\n");
			return LSM6DS3_ERR_STATUS;
		}
		/* change reg value from 0x1B to 0x16 for step counter debounce on bus env --modified by liaoxl.lenovo 7.6.2015 */
		res = LSM6DS3_Write_PedoThreshold(client, 0x19);  // set threshold to a certain value here
		if(res != LSM6DS3_SUCCESS)
		{
			GSE_LOG(" LSM6DS3_Write_PedoThreshold failed!\n");
			return LSM6DS3_ERR_STATUS;
		}

/* remove step counter reset operation to prevent data losing when system sleeping --modified by liaoxl.lenovo 7.28.2015 start */
		if(obj->boot_deb == 2)
		{
			obj->boot_deb = 1;
			res = LSM6DS3_Reset_Pedo_Data(client, LSM6DS3_ACC_GYRO_PEDO_RST_STEP_ENABLED);
			if(res != LSM6DS3_SUCCESS)
			{
				GSE_LOG(" LSM6DS3_Reset_Pedo_Data failed!\n");
				return LSM6DS3_ERR_STATUS;
			}
		}
/* remove step counter reset operation to prevent data losing when system sleeping --modified by liaoxl.lenovo 7.28.2015 end */

		/* config batch mode & wakeup relateds */
		res = LSM6DS3_enable_pedo_On_Int(client, true);
		if(res != LSM6DS3_SUCCESS)
		{
			GSE_LOG(" LSM6DS3_enable_pedo_On_Int failed!\n");
			return LSM6DS3_ERR_STATUS;
		}		

		//enable tilt feature and pedometer feature
		res = LSM6DS3_acc_Enable_Pedometer_Func(client, enable);
		if(res != LSM6DS3_SUCCESS)
		{
			GSE_LOG(" LSM6DS3_acc_Enable_Pedometer_Func failed!\n");
			return LSM6DS3_ERR_STATUS;
		}

	}
	else
	{
		res = LSM6DS3_enable_pedo_On_Int(client, false);
		if(res != LSM6DS3_SUCCESS)
		{
			GSE_LOG(" LSM6DS3_enable_pedo_On_Int failed!\n");
			return LSM6DS3_ERR_STATUS;
		}		

		res = LSM6DS3_acc_Enable_Pedometer_Func(client, enable);
		if(res != LSM6DS3_SUCCESS)
		{
			GSE_LOG(" LSM6DS3_acc_Enable_Func failed at disable pedo!\n");
			return LSM6DS3_ERR_STATUS;
		}
		/*do not turn off the func when other related func working, modified by liaoxl.lenovo 7.6.2015 */
		if(!enable_status && !tilt_enable_status && !sigm_enable_status)
		{
			res = LSM6DS3_acc_SetPowerMode(client,false);
			if(res != LSM6DS3_SUCCESS)
			{
				GSE_LOG(" LSM6DS3_acc_SetPowerMode failed at disable pedo!\n");
				return LSM6DS3_ERR_STATUS;
			}
		}
	}
	return LSM6DS3_SUCCESS;
}
#endif

static int LSM6DS3_acc_SetPowerMode(struct i2c_client *client, bool enable)
{
	u8 databuf[2] = {0};    
	int res = 0;
	struct lsm6ds3_i2c_data *obj = i2c_get_clientdata(client);//obj_i2c_data;

	if(enable == sensor_power)
	{
		GSE_LOG("Sensor power status is newest!\n");
		return LSM6DS3_SUCCESS;
	}

	//if(hwmsen_read_byte(client, LSM6DS3_CTRL1_XL, databuf))
	if(obj->tf->read(obj, LSM6DS3_CTRL1_XL, 1, databuf, true))
	{
		GSE_ERR("read lsm6ds3 power ctl register err!\n");
		return LSM6DS3_ERR_I2C;
	}
	GSE_LOG("LSM6DS3_CTRL1_XL:databuf[0] =  %x!\n", databuf[0]);


	if(true == enable)
	{
		databuf[0] &= ~LSM6DS3_ACC_ODR_MASK;//clear lsm6ds3 gyro ODR bits
		databuf[0] |= obj->sample_rate;//LSM6DS3_ACC_ODR_104HZ; //default set 100HZ for LSM6DS3 acc
	}
	else
	{
		// do nothing
		databuf[0] &= ~LSM6DS3_ACC_ODR_MASK;//clear lsm6ds3 acc ODR bits
		databuf[0] |= LSM6DS3_ACC_ODR_POWER_DOWN;
	}
	databuf[1] = databuf[0];
	databuf[0] = LSM6DS3_CTRL1_XL;    
	//res = i2c_master_send(client, databuf, 0x2);
	res = obj->tf->write(obj, databuf[0], 1, &databuf[1], true);
	if(res < 0)
	{
		GSE_LOG("LSM6DS3 set power mode: ODR 100hz failed!\n");
		return LSM6DS3_ERR_I2C;
	}	
	else
	{
		GSE_LOG("set LSM6DS3 gyro power mode:ODR 100HZ ok %d!\n", enable);
	}	

	sensor_power = enable;
	
	return LSM6DS3_SUCCESS;    
}


/*----------------------------------------------------------------------------*/
static int LSM6DS3_acc_SetFullScale(struct i2c_client *client, u8 acc_fs)
{
	u8 databuf[2] = {0};    
	int res = 0;
	struct lsm6ds3_i2c_data *obj = i2c_get_clientdata(client);
	
	GSE_FUN();     
	
	//if(hwmsen_read_byte(client, LSM6DS3_CTRL1_XL, databuf))
	if(obj->tf->read(obj, LSM6DS3_CTRL1_XL, 1, databuf, true))
	{
		GSE_ERR("read LSM6DS3_CTRL1_XL err!\n");
		return LSM6DS3_ERR_I2C;
	}
	else
	{
		GSE_LOG("read  LSM6DS3_CTRL1_XL register: 0x%x\n", databuf[0]);
	}

	databuf[0] &= ~LSM6DS3_ACC_RANGE_MASK;//clear 
	databuf[0] |= acc_fs;
	
	databuf[1] = databuf[0];
	databuf[0] = LSM6DS3_CTRL1_XL; 
	
	//res = i2c_master_send(client, databuf, 0x2);
	res = obj->tf->write(obj, databuf[0], 1, &databuf[1], true);
	if(res < 0)
	{
		GSE_ERR("write full scale register err!\n");
		return LSM6DS3_ERR_I2C;
	}
	switch(acc_fs)
	{
		case LSM6DS3_ACC_RANGE_2g:
			obj->sensitivity = LSM6DS3_ACC_SENSITIVITY_2G;
			break;
		case LSM6DS3_ACC_RANGE_4g:
			obj->sensitivity = LSM6DS3_ACC_SENSITIVITY_4G;
			break;
		case LSM6DS3_ACC_RANGE_8g:
			obj->sensitivity = LSM6DS3_ACC_SENSITIVITY_8G;
			break;
		case LSM6DS3_ACC_RANGE_16g:
			obj->sensitivity = LSM6DS3_ACC_SENSITIVITY_16G;
			break;
		default:
			obj->sensitivity = LSM6DS3_ACC_SENSITIVITY_2G;
			break;
	}

	//if(hwmsen_read_byte(client, LSM6DS3_CTRL9_XL, databuf))
	if(obj->tf->read(obj, LSM6DS3_CTRL9_XL, 1, databuf, true))
	{
		GSE_ERR("read LSM6DS3_CTRL9_XL err!\n");
		return LSM6DS3_ERR_I2C;
	}
	else
	{
		GSE_LOG("read  LSM6DS3_CTRL9_XL register: 0x%x\n", databuf[0]);
	}

	databuf[0] &= ~LSM6DS3_ACC_ENABLE_AXIS_MASK;//clear 
	databuf[0] |= LSM6DS3_ACC_ENABLE_AXIS_X | LSM6DS3_ACC_ENABLE_AXIS_Y| LSM6DS3_ACC_ENABLE_AXIS_Z;
	
	databuf[1] = databuf[0];
	databuf[0] = LSM6DS3_CTRL9_XL; 
	
	//res = i2c_master_send(client, databuf, 0x2);
	res = obj->tf->write(obj, databuf[0], 1, &databuf[1], true);
	if(res < 0)
	{
		GSE_ERR("write full scale register err!\n");
		return LSM6DS3_ERR_I2C;
	}

	return LSM6DS3_SUCCESS;    
}

/*----------------------------------------------------------------------------*/
// set the acc sample rate
static int LSM6DS3_acc_SetSampleRate(struct i2c_client *client, u8 sample_rate)
{
	u8 databuf[2] = {0}; 
	int res = 0;
	struct lsm6ds3_i2c_data *obj = i2c_get_clientdata(client);
	GSE_FUN();    

	res = LSM6DS3_acc_SetPowerMode(client, true);	//set Sample Rate will enable power and should changed power status
	if(res != LSM6DS3_SUCCESS)	
	{
		return res;
	}

	//if(hwmsen_read_byte(client, LSM6DS3_CTRL1_XL, databuf))
	if(obj->tf->read(obj, LSM6DS3_CTRL1_XL, 1, databuf, true))
	{
		GSE_ERR("read acc data format register err!\n");
		return LSM6DS3_ERR_I2C;
	}
	else
	{
		GSE_LOG("read  acc data format register: 0x%x\n", databuf[0]);
	}

	databuf[0] &= ~LSM6DS3_ACC_ODR_MASK;//clear 
	databuf[0] |= sample_rate;
	
	databuf[1] = databuf[0];
	databuf[0] = LSM6DS3_CTRL1_XL; 
	
	//res = i2c_master_send(client, databuf, 0x2);
	res = obj->tf->write(obj, databuf[0], 1, &databuf[1], true);
	if(res < 0)
	{
		GSE_ERR("write sample rate register err!\n");
		return LSM6DS3_ERR_I2C;
	}

	return LSM6DS3_SUCCESS;    
}

#ifdef LSM6DS3_TILT_FUNC //tilt detector
static int LSM6DS3_Enable_Tilt_Func(struct i2c_client *client, bool enable)
{
	u8 databuf[2] = {0}; 
	int res = 0;
	struct lsm6ds3_i2c_data *obj = i2c_get_clientdata(client);
	GSE_FUN();    
	
	//if(hwmsen_read_byte(client, LSM6DS3_TAP_CFG, databuf))
	if(obj->tf->read(obj, LSM6DS3_TAP_CFG, 1, databuf, true))
	{
		GSE_ERR("read acc data format register err!\n");
		return LSM6DS3_ERR_I2C;
	}
	else
	{
		GSE_LOG("read  acc data format register: 0x%x\n", databuf[0]);
	}

	if(enable)
	{
		databuf[0] &= ~LSM6DS3_TILT_EN_MASK;//clear 
		databuf[0] |= LSM6DS3_ACC_GYRO_TILT_EN_ENABLED;			
	}
	else
	{
		databuf[0] &= ~LSM6DS3_TILT_EN_MASK;//clear 
		databuf[0] |= LSM6DS3_ACC_GYRO_TILT_EN_DISABLED;		
	}
	
	databuf[1] = databuf[0];
	databuf[0] = LSM6DS3_TAP_CFG; 	
	//res = i2c_master_send(client, databuf, 0x2);
	res = obj->tf->write(obj, databuf[0], 1, &databuf[1], true);
	if(res < 0)
	{
		GSE_ERR("write enable tilt func register err!\n");
		return LSM6DS3_ERR_I2C;
	}

	return LSM6DS3_SUCCESS; 
}
#endif

#ifdef LSM6DS3_SIGNIFICANT_MOTION
static int LSM6DS3_Enable_SigMotion_Func_On_Int(struct i2c_client *client, bool enable)
{
	u8 databuf[2] = {0}; 
	int res = 0;
	u8 op_reg = 0;
	struct lsm6ds3_i2c_data *obj = i2c_get_clientdata(client);
	LSM6DS3_ACC_GYRO_FUNC_EN_t func_enable;
	LSM6DS3_ACC_GYRO_SIGN_MOT_t sigm_enable;
	GSE_FUN();    
	
	if(enable)
	{
		func_enable = LSM6DS3_ACC_GYRO_FUNC_EN_ENABLED;
		sigm_enable = LSM6DS3_ACC_GYRO_SIGN_MOT_ENABLED;
		
		res = LSM6DS3_acc_Enable_Func(client, func_enable);	
		if(res != LSM6DS3_SUCCESS)
		{
			GSE_LOG(" LSM6DS3_acc_Enable_Func failed!\n");
			return LSM6DS3_ERR_STATUS;
		}	
	}
	else
	{
		//func_enable = LSM6DS3_ACC_GYRO_FUNC_EN_DISABLED;
		sigm_enable = LSM6DS3_ACC_GYRO_SIGN_MOT_DISABLED;
	}		
	
	res = LSM6DS3_Enable_SigMotion_Func(client, sigm_enable);	
	if(res != LSM6DS3_SUCCESS)
	{
		GSE_LOG(" LSM6DS3_acc_Enable_Func failed!\n");
		return LSM6DS3_ERR_STATUS;
	}	

     // configure WU for sig int
	op_reg = LSM6DS3_MD1_CFG;
   
	if(obj->tf->read(obj, op_reg, 1, databuf, true))
	{
		GSE_ERR("%s read data format register err!\n", __func__);
		return LSM6DS3_ERR_I2C;
	}
	else
	{
		GSE_LOG("read  acc data format register: 0x%x\n", databuf[0]);
	}

	if(enable)
	{
		databuf[0] &= ~LSM6DS3_ACC_GYRO_MD1_WU_CFG;//clear 
		databuf[0] |= LSM6DS3_ACC_GYRO_MD1_WU_CFG_ENABLE;			
	}
	else
	{
		databuf[0] &= ~LSM6DS3_ACC_GYRO_MD1_WU_CFG;//clear 
		databuf[0] |= LSM6DS3_ACC_GYRO_MD1_WU_CFG_DISABLE;		
	}
	
	databuf[1] = databuf[0];
	databuf[0] = op_reg; 	
	//res = i2c_master_send(client, databuf, 0x2);
	res = obj->tf->write(obj, databuf[0], 1, &databuf[1], true);
	if(res < 0)
	{
		GSE_ERR("write enable tilt func register err!\n");
		return LSM6DS3_ERR_I2C;
	}
	
	//Config interrupt for significant motion

	op_reg = LSM6DS3_INT1_CTRL;
	
	//if(hwmsen_read_byte(client, op_reg, databuf))
	if(obj->tf->read(obj, op_reg, 1, databuf, true))
	{
		GSE_ERR("%s read data format register err!\n", __func__);
		return LSM6DS3_ERR_I2C;
	}
	else
	{
		GSE_LOG("read  acc data format register: 0x%x\n", databuf[0]);
	}

	if(enable)
	{
		databuf[0] &= ~LSM6DS3_ACC_GYRO_INT_SIGN_MOT_MASK;//clear 
		databuf[0] |= LSM6DS3_ACC_GYRO_INT_SIGN_MOT_ENABLED;			
	}
	else
	{
		databuf[0] &= ~LSM6DS3_ACC_GYRO_INT_SIGN_MOT_MASK;//clear 
		databuf[0] |= LSM6DS3_ACC_GYRO_INT_SIGN_MOT_DISABLED;		
	}
	
	databuf[1] = databuf[0];
	databuf[0] = op_reg; 	
	//res = i2c_master_send(client, databuf, 0x2);
	res = obj->tf->write(obj, databuf[0], 1, &databuf[1], true);
	if(res < 0)
	{
		GSE_ERR("write enable tilt func register err!\n");
		return LSM6DS3_ERR_I2C;
	}	
	res = LSM6DS3_Int_Ctrl(client, LSM6DS3_ACC_GYRO_INT_ACTIVE_LOW, LSM6DS3_ACC_GYRO_INT_LATCH);
	if(res < 0)
	{
		GSE_ERR("write enable tilt func register err!\n");
		return LSM6DS3_ERR_I2C;
	}	
	return LSM6DS3_SUCCESS; 
}
#endif

#ifdef LSM6DS3_STEP_COUNTER
static int LSM6DS3_Int_Ctrl(struct i2c_client *client, LSM6DS3_ACC_GYRO_INT_ACTIVE_t int_act, LSM6DS3_ACC_GYRO_INT_LATCH_CTL_t int_latch)
{
	u8 databuf[2] = {0}; 
	int res = 0;
	u8 op_reg = 0;
	struct lsm6ds3_i2c_data *obj = i2c_get_clientdata(client);
	GSE_FUN();    

	//config latch int or no latch
	op_reg = LSM6DS3_TAP_CFG;
	//if(hwmsen_read_byte(client, op_reg, databuf))
	if(obj->tf->read(obj, op_reg, 1, databuf, true))
	{
		GSE_ERR("%s read data format register err!\n", __func__);
		return LSM6DS3_ERR_I2C;
	}
	else
	{
		GSE_LOG("read  acc data format register: 0x%x\n", databuf[0]);
	}

	databuf[0] &= ~LSM6DS3_ACC_GYRO_INT_LATCH_CTL_MASK;//clear 
	databuf[0] |= int_latch;			
		
	databuf[1] = databuf[0];
	databuf[0] = op_reg; 	
	//res = i2c_master_send(client, databuf, 0x2);
	res = obj->tf->write(obj, databuf[0], 1, &databuf[1], true);
	if(res < 0)
	{
		GSE_ERR("write enable tilt func register err!\n");
		return LSM6DS3_ERR_I2C;
	}
	// config high or low active
	op_reg = LSM6DS3_CTRL3_C;
	//if(hwmsen_read_byte(client, op_reg, databuf))
	if(obj->tf->read(obj, op_reg, 1, databuf, true))
	{
		GSE_ERR("%s read data format register err!\n", __func__);
		return LSM6DS3_ERR_I2C;
	}
	else
	{
		GSE_LOG("read  acc data format register: 0x%x\n", databuf[0]);
	}

	databuf[0] &= ~LSM6DS3_ACC_GYRO_INT_ACTIVE_MASK;//clear 
	databuf[0] |= int_act;			
		
	databuf[1] = databuf[0];
	databuf[0] = op_reg; 	
	//res = i2c_master_send(client, databuf, 0x2);
	res = obj->tf->write(obj, databuf[0], 1, &databuf[1], true);
	if(res < 0)
	{
		GSE_ERR("write enable tilt func register err!\n");
		return LSM6DS3_ERR_I2C;
	}

	return LSM6DS3_SUCCESS; 
}
#endif

#ifdef LSM6DS3_TILT_FUNC //tilt detector
static int LSM6DS3_Enable_Tilt_Func_On_Int(struct i2c_client *client, LSM6DS3_ACC_GYRO_ROUNT_INT_t tilt_int, bool enable)
{
	u8 databuf[2] = {0}; 
	int res = 0;
	u8 op_reg = 0;
	struct lsm6ds3_i2c_data *obj = i2c_get_clientdata(client);
	GSE_FUN();    
	
	if(LSM6DS3_ACC_GYRO_INT1 == tilt_int)
	{
		op_reg = LSM6DS3_MD1_CFG;
	}
	else if(LSM6DS3_ACC_GYRO_INT2 == tilt_int)
	{
		op_reg = LSM6DS3_MD2_CFG;
	}
	
	//if(hwmsen_read_byte(client, op_reg, databuf))
	if(obj->tf->read(obj, op_reg, 1, databuf, true))
	{
		GSE_ERR("%s read data format register err!\n", __func__);
		return LSM6DS3_ERR_I2C;
	}
	else
	{
		GSE_LOG("read  acc data format register: 0x%x\n", databuf[0]);
	}

	if(enable)
	{
		databuf[0] &= ~LSM6DS3_ACC_GYRO_INT_TILT_MASK;//clear 
		databuf[0] |= LSM6DS3_ACC_GYRO_INT_TILT_ENABLED;			
	}
	else
	{
		databuf[0] &= ~LSM6DS3_ACC_GYRO_INT_TILT_MASK;//clear 
		databuf[0] |= LSM6DS3_ACC_GYRO_INT_TILT_DISABLED;		
	}
	
	databuf[1] = databuf[0];
	databuf[0] = op_reg; 	
	//res = i2c_master_send(client, databuf, 0x2);
	res = obj->tf->write(obj, databuf[0], 1, &databuf[1], true);
	if(res < 0)
	{
		GSE_ERR("write enable tilt func register err!\n");
		return LSM6DS3_ERR_I2C;
	}
	res = LSM6DS3_Int_Ctrl(client, LSM6DS3_ACC_GYRO_INT_ACTIVE_LOW, LSM6DS3_ACC_GYRO_INT_LATCH);
	if(res < 0)
	{
		GSE_ERR("write enable tilt func register err!\n");
		return LSM6DS3_ERR_I2C;
	}

	return LSM6DS3_SUCCESS; 
}
#endif

#ifdef LSM6DS3_STEP_COUNTER //step counter
static int LSM6DS3_acc_Enable_Pedometer_Func(struct i2c_client *client, bool enable)
{
	u8 databuf[2] = {0}; 
	int res = 0;
	struct lsm6ds3_i2c_data *obj = i2c_get_clientdata(client);
	GSE_FUN();    
	
	//if(hwmsen_read_byte(client, LSM6DS3_TAP_CFG, databuf))
	if(obj->tf->read(obj, LSM6DS3_TAP_CFG, 1, databuf, true))
	{
		GSE_ERR("read LSM6DS3_TAP_CFG register err!\n");
		return LSM6DS3_ERR_I2C;
	}
	else
	{
		GSE_LOG("read LSM6DS3_TAP_CFG register: 0x%x\n", databuf[0]);
	}

	if(enable)
	{
		databuf[0] &= ~LSM6DS3_PEDO_EN_MASK;//clear 
		databuf[0] |= LSM6DS3_ACC_GYRO_PEDO_EN_ENABLED;
	}
	else
	{
		databuf[0] &= ~LSM6DS3_PEDO_EN_MASK;//clear 
		databuf[0] |= LSM6DS3_ACC_GYRO_PEDO_EN_DISABLED;
	}
	databuf[1] = databuf[0];
	databuf[0] = LSM6DS3_TAP_CFG; 	
	//res = i2c_master_send(client, databuf, 0x2);
	res = obj->tf->write(obj, databuf[0], 1, &databuf[1], true);
	if(res < 0)
	{
		GSE_ERR("write enable pedometer func register err!\n");
		return LSM6DS3_ERR_I2C;
	}

	return LSM6DS3_SUCCESS;    
}

#ifdef LSM6DS3_SIGNIFICANT_MOTION
static int LSM6DS3_Set_SigMotion_Threshold(struct i2c_client *client, u8 SigMotion_Threshold)
{
	u8 databuf[2] = {0}; 
	int res = 0;
	struct lsm6ds3_i2c_data *obj = i2c_get_clientdata(client);
	GSE_FUN();    
	
	//if(hwmsen_read_byte(client, LSM6DS3_FUNC_CFG_ACCESS, databuf))
	if(obj->tf->read(obj, LSM6DS3_FUNC_CFG_ACCESS, 1, databuf, true))
	{
		GSE_ERR("%s read LSM6DS3_CTRL10_C register err!\n", __func__);
		return LSM6DS3_ERR_I2C;
	}
	else
	{
		GSE_LOG("%s read acc data format register: 0x%x\n", __func__, databuf[0]);
	}
	databuf[0] = 0x80;
	
	databuf[1] = databuf[0];
	databuf[0] = LSM6DS3_FUNC_CFG_ACCESS; 	
	//res = i2c_master_send(client, databuf, 0x2);
	res = obj->tf->write(obj, databuf[0], 1, &databuf[1], true);
	if(res < 0)
	{
		GSE_ERR("%s write LSM6DS3_CTRL10_C register err!\n", __func__);
		return LSM6DS3_ERR_I2C;
	}
	
	databuf[1] = SigMotion_Threshold;
	databuf[0] = LSM6DS3_SM_THS; 
	
	//res = i2c_master_send(client, databuf, 0x2);
	res = obj->tf->write(obj, databuf[0], 1, &databuf[1], true);
	if(res < 0)
	{
		GSE_ERR("%s write LSM6DS3_CTRL10_C register err!\n", __func__);
		return LSM6DS3_ERR_I2C;
	}
	
	databuf[1] = 0x00;
	databuf[0] = LSM6DS3_FUNC_CFG_ACCESS;
	//res = i2c_master_send(client, databuf, 0x2);
	res = obj->tf->write(obj, databuf[0], 1, &databuf[1], true);
	if(res < 0)
	{
		GSE_ERR("%s write LSM6DS3_CTRL10_C register err!\n", __func__);
		return LSM6DS3_ERR_I2C;
	}

     // read WU TH	
	if(obj->tf->read(obj, LSM6DS3_WAKE_UP_THS, 1, databuf, true))
	{
		GSE_ERR("%s read LSM6DS3_CTRL10_C register err!\n", __func__);
		return LSM6DS3_ERR_I2C;
	}
	else
	{
		GSE_LOG("%s read acc data format register: 0x%x\n", __func__, databuf[0]);
	}
	databuf[0] &= ~LSM6DS3_ACC_WU_TH_MASK;//clear 
	databuf[0] |= 0x0a;

	// set Wu TH
	databuf[1] = databuf[0];
	databuf[0] = LSM6DS3_WAKE_UP_THS; 	
	//res = i2c_master_send(client, databuf, 0x2);
	res = obj->tf->write(obj, databuf[0], 1, &databuf[1], true);
	if(res < 0)
	{
		GSE_ERR("%s write LSM6DS3_WAKE_UP_THS register err!\n", __func__);
		return LSM6DS3_ERR_I2C;
	}

    // read WU DUR

	if(obj->tf->read(obj, LSM6DS3_WAKE_UP_DUR, 1, databuf, true))
		{
			GSE_ERR("read acc wakeup dur register err!\n");
			return LSM6DS3_ERR_I2C;
		}
		else
		{
			GSE_LOG("read  acc wakeup dur register: 0x%x\n", databuf[0]);
		}
		databuf[0] &= ~LSM6DS3_ACC_WU_DUR_MASK;
		databuf[0] |= 0x20;

		//set WU DUR
		databuf[1] = databuf[0];
		databuf[0] = LSM6DS3_WAKE_UP_DUR; 	
		//res = i2c_master_send(client, databuf, 0x2);
		res = obj->tf->write(obj, databuf[0], 1, &databuf[1], true);
		if(res < 0)
		{
			GSE_ERR("write enable LSM6DS3_WAKE_UP_DUR register err!\n");
			return LSM6DS3_ERR_I2C;
		}

	// read LSM6DS3_TAP_CFG
	if(obj->tf->read(obj, LSM6DS3_TAP_CFG, 1, databuf, true))
		{
			GSE_ERR("read LSM6DS3_TAP_CFG register err!\n");
			return LSM6DS3_ERR_I2C;
		}
		else
		{
			GSE_LOG("read LSM6DS3_TAP_CFG register: 0x%x\n", databuf[0]);
		}
	
	 // set TAP_CFG  SLOPE ENABLE
		databuf[0] &= ~LSM6DS3_ACC_GYRO_SLOPE_FDS_MASK;//clear 
		databuf[0] |= LSM6DS3_ACC_GYRO_SLOPE_FDS_ENABLE_HF_DISABLE;

		databuf[1] = databuf[0];
		databuf[0] = LSM6DS3_TAP_CFG;	
		//res = i2c_master_send(client, databuf, 0x2);
		res = obj->tf->write(obj, databuf[0], 1, &databuf[1], true);
		if(res < 0)
		{
			GSE_ERR("write enable pedometer func register err!\n");
			return LSM6DS3_ERR_I2C;
		}
	return LSM6DS3_SUCCESS;    
}
static int LSM6DS3_Enable_SigMotion_Func(struct i2c_client *client, LSM6DS3_ACC_GYRO_SIGN_MOT_t newValue)
{
	u8 databuf[2] = {0}; 
	int res = 0;
	struct lsm6ds3_i2c_data *obj = i2c_get_clientdata(client);
	GSE_FUN();    
	
	//if(hwmsen_read_byte(client, LSM6DS3_CTRL10_C, databuf))
	if(obj->tf->read(obj, LSM6DS3_CTRL10_C, 1, databuf, true))
	{
		GSE_ERR("%s read LSM6DS3_CTRL10_C register err!\n", __func__);
		return LSM6DS3_ERR_I2C;
	}
	else
	{
		GSE_LOG("%s read acc data format register: 0x%x\n", __func__, databuf[0]);
	}
/* do not reset step counter -- modified by liaoxl.lenovo 5.12.2015 start */
	databuf[0] &= ~(LSM6DS3_ACC_GYRO_SIGN_MOT_MASK | LSM6DS3_PEDO_RST_STEP_MASK);//clear 
	databuf[0] |= newValue;
/* do not reset step counter -- modified by liaoxl.lenovo 5.12.2015 end */
	
	databuf[1] = databuf[0];
	databuf[0] = LSM6DS3_CTRL10_C; 	
	//res = i2c_master_send(client, databuf, 0x2);
	res = obj->tf->write(obj, databuf[0], 1, &databuf[1], true);
	if(res < 0)
	{
		GSE_ERR("%s write LSM6DS3_CTRL10_C register err!\n", __func__);
		return LSM6DS3_ERR_I2C;
	}

	return LSM6DS3_SUCCESS;    
}
#endif
#endif

#ifdef LSM6DS3_STEP_COUNTER 
static int LSM6DS3_acc_Enable_Func(struct i2c_client *client, LSM6DS3_ACC_GYRO_FUNC_EN_t newValue)
{
	u8 databuf[2] = {0}; 
	int res = 0;
	struct lsm6ds3_i2c_data *obj = i2c_get_clientdata(client);
	GSE_FUN();    
	
	//if(hwmsen_read_byte(client, LSM6DS3_CTRL10_C, databuf))
	if(obj->tf->read(obj, LSM6DS3_CTRL10_C, 1, databuf, true))
	{
		GSE_ERR("%s read LSM6DS3_CTRL10_C register err!\n", __func__);
		return LSM6DS3_ERR_I2C;
	}
	else
	{
		GSE_LOG("%s read acc data format register: 0x%x\n", __func__, databuf[0]);
	}
/* do not reset step counter -- modified by liaoxl.lenovo 5.12.2015 start */
	databuf[0] &= ~(LSM6DS3_ACC_GYRO_FUNC_EN_MASK | LSM6DS3_PEDO_RST_STEP_MASK);//clear 
/* do not reset step counter -- modified by liaoxl.lenovo 5.12.2015 start */
	databuf[0] |= newValue;
	
	databuf[1] = databuf[0];
	databuf[0] = LSM6DS3_CTRL10_C; 	
	//res = i2c_master_send(client, databuf, 0x2);
	res = obj->tf->write(obj, databuf[0], 1, &databuf[1], true);
	if(res < 0)
	{
		GSE_ERR("%s write LSM6DS3_CTRL10_C register err!\n", __func__);
		return LSM6DS3_ERR_I2C;
	}

	return LSM6DS3_SUCCESS;    
}
#endif

#ifdef LSM6DS3_STEP_COUNTER //step counter
static int LSM6DS3_W_Open_RAM_Page(struct i2c_client *client, LSM6DS3_ACC_GYRO_RAM_PAGE_t newValue)
{
	u8 databuf[2] = {0}; 
	int res = 0;
	struct lsm6ds3_i2c_data *obj = i2c_get_clientdata(client);
	GSE_FUN();    
	
	//if(hwmsen_read_byte(client, LSM6DS3_RAM_ACCESS, databuf))
	if(obj->tf->read(obj, LSM6DS3_RAM_ACCESS, 1, databuf, true))
	{
		GSE_ERR("%s read LSM6DS3_RAM_ACCESS register err!\n", __func__);
		return LSM6DS3_ERR_I2C;
	}
	else
	{
		GSE_LOG("%s read acc data format register: 0x%x\n", __func__, databuf[0]);
	}
	databuf[0] &= ~LSM6DS3_RAM_PAGE_MASK;//clear 
	databuf[0] |= newValue;
	
	databuf[1] = databuf[0];
	databuf[0] = LSM6DS3_RAM_ACCESS; 	
	//res = i2c_master_send(client, databuf, 0x2);
	res = obj->tf->write(obj, databuf[0], 1, &databuf[1], true);
	if(res < 0)
	{
		GSE_ERR("%s write LSM6DS3_RAM_ACCESS register err!\n", __func__);
		return LSM6DS3_ERR_I2C;
	}

	return LSM6DS3_SUCCESS;
}

static int LSM6DS3_Write_PedoThreshold(struct i2c_client *client, u8 newValue)
{
	u8 databuf[2] = {0}; 
	int res = 0;
	struct lsm6ds3_i2c_data *obj = i2c_get_clientdata(client);
	GSE_FUN();    
	
	res = LSM6DS3_W_Open_RAM_Page(client, LSM6DS3_ACC_GYRO_RAM_PAGE_ENABLED);
	if(LSM6DS3_SUCCESS != res)
	{
		return res;
	}
	//if(hwmsen_read_byte(client, LSM6DS3_CONFIG_PEDO_THS_MIN, databuf))
	if(obj->tf->read(obj, LSM6DS3_CONFIG_PEDO_THS_MIN, 1, databuf, true))
	{
		GSE_ERR("%s read LSM6DS3_CTRL10_C register err!\n", __func__);
		return LSM6DS3_ERR_I2C;
	}
	else
	{
		GSE_LOG("%s read acc data format register: 0x%x\n", __func__, databuf[0]);
	}
	
	databuf[0] &= ~0x1F; 
	databuf[0] |= (newValue & 0x1F);
	
	databuf[1] = databuf[0];
	databuf[0] = LSM6DS3_CONFIG_PEDO_THS_MIN; 	
	//res = i2c_master_send(client, databuf, 0x2);
	res = obj->tf->write(obj, databuf[0], 1, &databuf[1], true);
	if(res < 0)
	{
		GSE_ERR("%s write LSM6DS3_CTRL10_C register err!\n", __func__);
		return LSM6DS3_ERR_I2C;
	}
	
	databuf[0] = 0x14;
	databuf[1] = 0x6e; 
	//res = i2c_master_send(client, databuf, 0x2);
	res = obj->tf->write(obj, databuf[0], 1, &databuf[1], true);
	if(res < 0)
	{
		GSE_ERR("%s write LSM6DS3_CTRL10_C register err!\n", __func__);
		return LSM6DS3_ERR_I2C;
	}
	res = LSM6DS3_W_Open_RAM_Page(client, LSM6DS3_ACC_GYRO_RAM_PAGE_DISABLED);
	if(LSM6DS3_SUCCESS != res)
	{
		GSE_ERR("%s write LSM6DS3_W_Open_RAM_Page failed!\n", __func__);
		return res;
	}
	
	return LSM6DS3_SUCCESS; 
}


static int LSM6DS3_Reset_Pedo_Data(struct i2c_client *client, LSM6DS3_ACC_GYRO_PEDO_RST_STEP_t newValue)
{
	u8 databuf[2] = {0}; 
	int res = 0;
	struct lsm6ds3_i2c_data *obj = i2c_get_clientdata(client);
	GSE_FUN();    
	
	//if(hwmsen_read_byte(client, LSM6DS3_CTRL10_C, databuf))
	if(obj->tf->read(obj, LSM6DS3_CTRL10_C, 1, databuf, true))
	{
		GSE_ERR("%s read LSM6DS3_CTRL10_C register err!\n", __func__);
		return LSM6DS3_ERR_I2C;
	}
	else
	{
		GSE_LOG("%s read acc LSM6DS3_CTRL10_C data format register: 0x%x\n", __func__, databuf[0]);
	}
	databuf[0] &= ~LSM6DS3_PEDO_RST_STEP_MASK;//clear 
	databuf[0] |= newValue;
	
	databuf[1] = databuf[0];
	databuf[0] = LSM6DS3_CTRL10_C; 	
	//res = i2c_master_send(client, databuf, 0x2);
	res = obj->tf->write(obj, databuf[0], 1, &databuf[1], true);
	if(res < 0)
	{
		GSE_ERR("%s write LSM6DS3_CTRL10_C register err!\n", __func__);
		return LSM6DS3_ERR_I2C;
	}

	msleep(20);
	databuf[1] &= ~LSM6DS3_PEDO_RST_STEP_MASK;
	res = obj->tf->write(obj, databuf[0], 1, &databuf[1], true);
	if(res < 0)
	{
		GSE_ERR("%s write LSM6DS3_CTRL10_C register err!\n", __func__);
		return LSM6DS3_ERR_I2C;
	}

	return LSM6DS3_SUCCESS;
}
static int LSM6DS3_Get_Pedo_DataReg(struct i2c_client *client, u16 *Value)
{
	u8 databuf[2] = {0};
	struct lsm6ds3_i2c_data *obj = (struct lsm6ds3_i2c_data*)i2c_get_clientdata(client);
	//GSE_FUN();    
	
	//if(hwmsen_read_block(client, LSM6DS3_STEP_COUNTER_L, databuf, 2))
	if(obj->tf->read(obj, LSM6DS3_STEP_COUNTER_L, 2, databuf, true))
	{
		GSE_ERR("LSM6DS3 read acc data  error\n");
		return -2;
	}

	*Value = (databuf[1]<<8)|databuf[0];	
//	GSE_LOG("read step count=%d \n", *Value);

	return LSM6DS3_SUCCESS;
}
#endif
/*----------------------------------------------------------------------------*/
static int LSM6DS3_ReadAccData(struct i2c_client *client, char *buf, int bufsize)
{
	struct lsm6ds3_i2c_data *obj = (struct lsm6ds3_i2c_data*)i2c_get_clientdata(client);
	u8 databuf[20];
	int acc[LSM6DS3_ACC_AXES_NUM];
	int res = 0;
	memset(databuf, 0, sizeof(u8)*10);

	if(NULL == buf)
	{
		return -1;
	}
	if(NULL == client)
	{
		*buf = 0;
		return -2;
	}

	if(sensor_power == false)
	{
		res = LSM6DS3_acc_SetPowerMode(client, true);
		if(res)
		{
			GSE_ERR("Power on lsm6ds3 error %d!\n", res);
		}
		msleep(20);
	}

	res = LSM6DS3_ReadAccRawData(client, obj->data);
	if(res < 0)
	{        
		GSE_ERR("I2C error: ret value=%d", res);
		return -3;
	}
	else
	{
	#if 1
		obj->data[LSM6DS3_AXIS_X] = (long)(obj->data[LSM6DS3_AXIS_X]) * obj->sensitivity*GRAVITY_EARTH_1000/(1000*1000); //NTC
		obj->data[LSM6DS3_AXIS_Y] = (long)(obj->data[LSM6DS3_AXIS_Y]) * obj->sensitivity*GRAVITY_EARTH_1000/(1000*1000);
		obj->data[LSM6DS3_AXIS_Z] = (long)(obj->data[LSM6DS3_AXIS_Z]) * obj->sensitivity*GRAVITY_EARTH_1000/(1000*1000);

/*lenovo-sw caoyi1 add for Gsensor APK calibration 20150512 begin*/
#ifdef LSM6DS3_MTK_CALIB
		obj->data[LSM6DS3_AXIS_X] += obj->cali_sw[LSM6DS3_AXIS_X];
		obj->data[LSM6DS3_AXIS_Y] += obj->cali_sw[LSM6DS3_AXIS_Y];
		obj->data[LSM6DS3_AXIS_Z] += obj->cali_sw[LSM6DS3_AXIS_Z];
#endif
/*lenovo-sw caoyi1 add for Gsensor APK calibration 20150512 end*/

		/*remap coordinate*/
		acc[obj->cvt.map[LSM6DS3_AXIS_X]] = obj->cvt.sign[LSM6DS3_AXIS_X]*obj->data[LSM6DS3_AXIS_X];
		acc[obj->cvt.map[LSM6DS3_AXIS_Y]] = obj->cvt.sign[LSM6DS3_AXIS_Y]*obj->data[LSM6DS3_AXIS_Y];
		acc[obj->cvt.map[LSM6DS3_AXIS_Z]] = obj->cvt.sign[LSM6DS3_AXIS_Z]*obj->data[LSM6DS3_AXIS_Z];

		//GSE_LOG("Mapped gsensor data: %d, %d, %d!\n", acc[LSM6DS3_AXIS_X], acc[LSM6DS3_AXIS_Y], acc[LSM6DS3_AXIS_Z]);

		//Out put the mg
		/*
		acc[LSM6DS3_AXIS_X] = acc[LSM6DS3_AXIS_X] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
		acc[LSM6DS3_AXIS_Y] = acc[LSM6DS3_AXIS_Y] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
		acc[LSM6DS3_AXIS_Z] = acc[LSM6DS3_AXIS_Z] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;		
		*/
	#endif

/*lenovo-sw caoyi1 add for Gsensor APK calibration 20150512 begin*/
#ifdef LSM6DS3_CALIB_HAL
	        acc[LSM6DS3_AXIS_X] =  acc[LSM6DS3_AXIS_X] - calib_offset[LSM6DS3_AXIS_X];
	        acc[LSM6DS3_AXIS_Y] =  acc[LSM6DS3_AXIS_Y] - calib_offset[LSM6DS3_AXIS_Y];
		acc[LSM6DS3_AXIS_Z] =  acc[LSM6DS3_AXIS_Z] - calib_offset[LSM6DS3_AXIS_Z];

#endif
/*lenovo-sw caoyi1 add for Gsensor APK calibration 20150512 end*/

		sprintf(buf, "%04x %04x %04x", acc[LSM6DS3_AXIS_X], acc[LSM6DS3_AXIS_Y], acc[LSM6DS3_AXIS_Z]);
	
		if(atomic_read(&obj->trace) & ADX_TRC_IOCTL)//atomic_read(&obj->trace) & ADX_TRC_IOCTL
		{
			//GSE_LOG("gsensor data: %s!\n", buf);
			GSE_LOG("raw data:obj->data:%04x %04x %04x\n", obj->data[LSM6DS3_AXIS_X], obj->data[LSM6DS3_AXIS_Y], obj->data[LSM6DS3_AXIS_Z]);
			GSE_LOG("acc:%04x %04x %04x\n", acc[LSM6DS3_AXIS_X], acc[LSM6DS3_AXIS_Y], acc[LSM6DS3_AXIS_Z]);
	
			//LSM6DS3_dumpReg(client);
		}
	}
	
	return 0;
}
static int LSM6DS3_ReadAccRawData(struct i2c_client *client, s16 data[LSM6DS3_ACC_AXES_NUM])
{
	int err = 0;
	char databuf[6] = {0};
	struct lsm6ds3_i2c_data *obj = (struct lsm6ds3_i2c_data*)i2c_get_clientdata(client);

	if(NULL == client)
	{
		err = -EINVAL;
	}	
	else
	{
		//if(hwmsen_read_block(client, LSM6DS3_OUTX_L_XL, databuf, 6))
		if(obj->tf->read(obj, LSM6DS3_OUTX_L_XL, 6, databuf, true))
		{
			GSE_ERR("LSM6DS3 read acc data  error\n");
			return -2;
		}
		else
		{
			data[LSM6DS3_AXIS_X] = (s16)((databuf[LSM6DS3_AXIS_X*2+1] << 8) | (databuf[LSM6DS3_AXIS_X*2]));
			data[LSM6DS3_AXIS_Y] = (s16)((databuf[LSM6DS3_AXIS_Y*2+1] << 8) | (databuf[LSM6DS3_AXIS_Y*2]));
			data[LSM6DS3_AXIS_Z] = (s16)((databuf[LSM6DS3_AXIS_Z*2+1] << 8) | (databuf[LSM6DS3_AXIS_Z*2]));	
		}      
	}
	return err;
}

/*lenovo-sw caoyi1 add for selftest function 20150527 begin*/
#ifdef LSM6DS3_FTM_ENABLE
static int lsm303d_acc_get_ST_data(struct i2c_client *client, int *xyz)
{
	int i;
	s16 acc_data[6];
	int hw_d[3] = {0};
	u8 buf[2];
        char databuf[6] = {0};
	int xyz_sum[3] = {0};

	if(hwmsen_read_block(client, LSM6DS3_OUTX_L_XL, databuf, 6))
		{
			GSE_ERR("LSM6DS3 read acc data  error\n");
			return -2;
		}
	
	for(i=0; i<5;)
	{
		if(hwmsen_read_byte(client, LSM6DS3_STATUS_REG, buf))
		{
			GSE_ERR("read acc data format register err!\n");
			return LSM6DS3_ERR_I2C;
		}
		else
		{
			GSE_LOG("read  acc status  register: 0x%x\n", buf[0]);
		}

		if(buf[0] & 0x01)                                                       // checking status reg  0x01 bit)
		{
			i++;
			if(hwmsen_read_block(client, LSM6DS3_OUTX_L_XL, databuf, 6))
	        	{
			GSE_ERR("LSM6DS3 read acc data  error\n");
			return -2;
	        	}

			acc_data[0] = (s16)((databuf[LSM6DS3_AXIS_X*2+1] << 8) | (databuf[LSM6DS3_AXIS_X*2]));
			acc_data[1] = (s16)((databuf[LSM6DS3_AXIS_Y*2+1] << 8) | (databuf[LSM6DS3_AXIS_Y*2]));
			acc_data[2] = (s16)((databuf[LSM6DS3_AXIS_Z*2+1] << 8) | (databuf[LSM6DS3_AXIS_Z*2]));

			GSE_ERR("raw-- x %d, y%d,z %d ,i %d \n",acc_data[0],acc_data[1],acc_data[2],i);	
	
			hw_d[0] = acc_data[0] * LSM6DS3_ACC_SENSITIVITY_2G;
			hw_d[1] = acc_data[1] * LSM6DS3_ACC_SENSITIVITY_2G;
			hw_d[2] = acc_data[2] * LSM6DS3_ACC_SENSITIVITY_2G;

		        GSE_ERR("x %d, y%d,z %d ,i %d \n",hw_d[0],hw_d[1],hw_d[2],i);	
					 
			xyz_sum[0] =  hw_d[0] + xyz_sum[0] ;	  
			xyz_sum[1] =  hw_d[1] + xyz_sum[1] ;	  
			xyz_sum[2] =  hw_d[2] + xyz_sum[2] ;	 
		}else{
	               msleep(25);
			}
	}

         xyz[0] = xyz_sum[0];
	 xyz[1] = xyz_sum[1];
	 xyz[2] = xyz_sum[2]; 

	 GSE_ERR("sum x %d, y%d,z %d ,i %d \n",xyz[0], xyz[1], xyz[2] ,i);	

	 return 0;
}
#endif
/*lenovo-sw caoyi1 add for selftest function 20150527 begin*/

/*----------------------------------------------------------------------------*/
static int LSM6DS3_ReadChipInfo(struct i2c_client *client, char *buf, int bufsize)
{
	u8 databuf[10];    

	memset(databuf, 0, sizeof(u8)*10);

	if((NULL == buf)||(bufsize<=30))
	{
		return -1;
	}
	
	if(NULL == client)
	{
		*buf = 0;
		return -2;
	}

	sprintf(buf, "LSM6DS3 Chip");
	return 0;
}

/*----------------------------------------------------------------------------*/
static ssize_t show_chipinfo_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = lsm6ds3_i2c_client;
	char strbuf[LSM6DS3_BUFSIZE];
	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	
	LSM6DS3_ReadChipInfo(client, strbuf, LSM6DS3_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);        
}

static ssize_t show_chipid_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = lsm6ds3_i2c_client;
	char strbuf[LSM6DS3_BUFSIZE] = "unkown chipid";
	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	
	//LSM6DS3_ReadChipInfo(client, strbuf, LSM6DS3_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);        
}

/*----------------------------------------------------------------------------*/
static ssize_t show_sensordata_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = lsm6ds3_i2c_client;
	char strbuf[LSM6DS3_BUFSIZE];
	int x,y,z;
	
	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	
	LSM6DS3_ReadAccData(client, strbuf, LSM6DS3_BUFSIZE);
	sscanf(strbuf, "%x %x %x", &x, &y, &z);	
	return snprintf(buf, PAGE_SIZE, "%d, %d, %d\n", x,y,z);            
}
static ssize_t show_sensorrawdata_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = lsm6ds3_i2c_client;
	s16 data[LSM6DS3_ACC_AXES_NUM] = {0};
	
	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	
	LSM6DS3_ReadAccRawData(client, data);
	return snprintf(buf, PAGE_SIZE, "%x,%x,%x\n", data[0],data[1],data[2]);            
}

/*----------------------------------------------------------------------------*/
static ssize_t show_trace_value(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	struct lsm6ds3_i2c_data *obj = obj_i2c_data;
	
	if (obj == NULL)
	{
		GSE_ERR("i2c_data obj is null!!\n");
		return 0;
	}
	
	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->trace));     
	return res;    
}
/*----------------------------------------------------------------------------*/
static ssize_t store_trace_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct lsm6ds3_i2c_data *obj = obj_i2c_data;
	int trace;
	if (obj == NULL)
	{
		GSE_ERR("i2c_data obj is null!!\n");
		return count;
	}
	
	if(1 == sscanf(buf, "0x%x", &trace))
	{
		atomic_set(&obj->trace, trace);
	}	
	else
	{
		GSE_ERR("invalid content: '%s', length = %zu\n", buf, count);
	}
	
	return count;    
}
static ssize_t show_chipinit_value(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	struct lsm6ds3_i2c_data *obj = obj_i2c_data;
	if (obj == NULL)
	{
		GSE_ERR("i2c_data obj is null!!\n");
		return 0;
	}
	
	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->trace));     
	return res;    
}
/*----------------------------------------------------------------------------*/
static ssize_t store_chipinit_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct lsm6ds3_i2c_data *obj = obj_i2c_data;

	if (obj == NULL)
	{
		GSE_ERR("i2c_data obj is null!!\n");
		return count;
	}
	
	LSM6DS3_init_client(obj->client, true);
	LSM6DS3_dumpReg(obj->client);
	
	return count;    
}
/*----------------------------------------------------------------------------*/
static ssize_t show_status_value(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;    
	struct lsm6ds3_i2c_data *obj = obj_i2c_data;
	if (obj == NULL)
	{
		GSE_ERR("i2c_data obj is null!!\n");
		return 0;
	}	
	
	if(obj->hw)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: i2c_num=%d, direction=%d, sensitivity = %d,(power_id=%d, power_vol=%d)\n", 
	            obj->hw->i2c_num, obj->hw->direction, obj->sensitivity, obj->hw->power_id, obj->hw->power_vol);   
		LSM6DS3_dumpReg(obj->client);
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: pedo=%d tilt=%d sigm=%d\n", pedo_enable_status, tilt_enable_status, sigm_enable_status);
	}
	else
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: NULL\n");
	}
	return len;    
}
static ssize_t show_layout_value(struct device_driver *ddri, char *buf)
{
	struct lsm6ds3_i2c_data *data = obj_i2c_data;
	if(NULL == data)
	{
		printk(KERN_ERR "lsm6ds3_i2c_data is null!!\n");
		return -1;
	}

	return sprintf(buf, "(%d, %d)\n[%+2d %+2d %+2d]\n[%+2d %+2d %+2d]\n",
		data->hw->direction,atomic_read(&data->layout),	data->cvt.sign[0], data->cvt.sign[1],
		data->cvt.sign[2],data->cvt.map[0], data->cvt.map[1], data->cvt.map[2]);
}
/*----------------------------------------------------------------------------*/
static ssize_t store_layout_value(struct device_driver *ddri, const char *buf, size_t count)
{
	int layout = 0;
	struct lsm6ds3_i2c_data *data = obj_i2c_data;

	if(NULL == data)
	{
		printk(KERN_ERR "lsm6ds3_i2c_data is null!!\n");
		return count;
	}

	

	if(1 == sscanf(buf, "%d", &layout))
	{
		atomic_set(&data->layout, layout);
		if(!hwmsen_get_convert(layout, &data->cvt))
		{
			printk(KERN_ERR "HWMSEN_GET_CONVERT function error!\r\n");
		}
		else if(!hwmsen_get_convert(data->hw->direction, &data->cvt))
		{
			printk(KERN_ERR "invalid layout: %d, restore to %d\n", layout, data->hw->direction);
		}
		else
		{
			printk(KERN_ERR "invalid layout: (%d, %d)\n", layout, data->hw->direction);
			hwmsen_get_convert(0, &data->cvt);
		}
	}
	else
	{
		printk(KERN_ERR "invalid format = '%s'\n", buf);
	}

	return count;
}

/*lenovo-sw caoyi1 add for Gsensor APK calibration 20150526 begin*/
 #ifdef LSM6DS3_CALIB_HAL
static ssize_t show_cali_x_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = lsm6ds3_i2c_client;
        int data;

	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	data = calib_offset[LSM6DS3_AXIS_X];

	return snprintf(buf, PAGE_SIZE, "%d \n", data);
}

static ssize_t store_cali_x_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct lsm6ds3_i2c_data *obj = obj_i2c_data;
	int cali_x;

	if (obj == NULL)
	{
		GSE_ERR("i2c_data obj is null!!\n");
		return count;
	}

	if(1 == sscanf(buf, "%d", &cali_x))
	{
		calib_offset[LSM6DS3_AXIS_X] = cali_x;
	}
	else
	{
		GSE_ERR("invalid content: '%s', length = %zu\n", buf, count);
	}

	return count;
}

static ssize_t show_cali_y_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = lsm6ds3_i2c_client;
        int data;

	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	data = calib_offset[LSM6DS3_AXIS_Y];

	return snprintf(buf, PAGE_SIZE, "%d \n", data);
}

static ssize_t store_cali_y_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct lsm6ds3_i2c_data *obj = obj_i2c_data;
	int cali_y;
	if (obj == NULL)
	{
		GSE_ERR("i2c_data obj is null!!\n");
		return count;
	}

	if(1 == sscanf(buf, "%d", &cali_y))
	{
		calib_offset[LSM6DS3_AXIS_Y] = cali_y;
	}
	else
	{
		GSE_ERR("invalid content: '%s', length = %zu\n", buf, count);
	}

	return count;
}

static ssize_t show_cali_z_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = lsm6ds3_i2c_client;
        int data;

	if(NULL == client)
	{
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	data = calib_offset[LSM6DS3_AXIS_Z];

	return snprintf(buf, PAGE_SIZE, "%d \n", data);
}

static ssize_t store_cali_z_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct lsm6ds3_i2c_data *obj = obj_i2c_data;
	int cali_z;
	if (obj == NULL)
	{
		GSE_ERR("i2c_data obj is null!!\n");
		return count;
	}

	if(1 == sscanf(buf, "%d", &cali_z))
	{
		calib_offset[LSM6DS3_AXIS_Z] = cali_z;
	}
	else
	{
		GSE_ERR("invalid content: '%s', length = %zu\n", buf, count);
	}

	return count;
}

#endif
/*lenovo-sw caoyi1 add for Gsensor APK calibration 20150512 end*/

/*lenovo-sw caoyi1 add for selftest function 20150527 begin*/
#ifdef LSM6DS3_FTM_ENABLE

static int back_ST_reg_config(struct i2c_client  *client )
{
	struct lsm6ds3_i2c_data *priv = obj_i2c_data;
	u8 databuf[2];
   
	//res = hwmsen_read_byte(client, LSM6DS3_CTRL1_XL, databuf);
	if(obj_i2c_data->tf->read(priv, LSM6DS3_CTRL1_XL, 1, (u8 *)&databuf, true))
	{
		GSE_ERR("read LSM6DS3_CTRL1_XL err!\n");
		return LSM6DS3_ERR_I2C;
	}
         priv ->resume_LSM6DS3_CTRL1_XL = databuf[0];
		 
	//res = hwmsen_read_byte(client, LSM6DS3_CTRL2_G, databuf);
	if(obj_i2c_data->tf->read(priv, LSM6DS3_CTRL2_G, 1, (u8 *)&databuf, true))
	{
		GSE_ERR("read LSM6DS3_CTRL2_G err!\n");
		return LSM6DS3_ERR_I2C;
	}
         priv ->resume_LSM6DS3_CTRL2_G = databuf[0];
		 

	//res = hwmsen_read_byte(client, LSM6DS3_CTRL3_C, databuf);
	if(obj_i2c_data->tf->read(priv, LSM6DS3_CTRL3_C, 1, (u8 *)&databuf, true))
	{
		GSE_ERR("read LSM6DS3_CTRL3_C err!\n");
		return LSM6DS3_ERR_I2C;
	}
         priv ->resume_LSM6DS3_CTRL3_C = databuf[0];
	
	
	//res = hwmsen_read_byte(client, LSM6DS3_CTRL4_C, databuf);
	if(obj_i2c_data->tf->read(priv, LSM6DS3_CTRL4_C, 1, (u8 *)&databuf, true))
	{
		GSE_ERR("read LSM6DS3_CTRL4_C err!\n");
		return LSM6DS3_ERR_I2C;
	}
         priv ->resume_LSM6DS3_CTRL4_C = databuf[0];

	//res = hwmsen_read_byte(client, LSM6DS3_CTRL5_C, databuf);
	if(obj_i2c_data->tf->read(priv, LSM6DS3_CTRL5_C, 1, (u8 *)&databuf, true))
	{
		GSE_ERR("read LSM6DS3_CTRL5_C err!\n");
		return LSM6DS3_ERR_I2C;
	}
         priv ->resume_LSM6DS3_CTRL5_C = databuf[0];

	
	//res = hwmsen_read_byte(client, LSM6DS3_CTRL6_C, databuf);
	if(obj_i2c_data->tf->read(priv, LSM6DS3_CTRL6_C, 1, (u8 *)&databuf, true))
	{
		GSE_ERR("read LSM6DS3_CTRL6_C err!\n");
		return LSM6DS3_ERR_I2C;
	}
         priv ->resume_LSM6DS3_CTRL6_C = databuf[0];

       
	//res = hwmsen_read_byte(client, LSM6DS3_CTRL7_G, databuf);
	if(obj_i2c_data->tf->read(priv, LSM6DS3_CTRL7_G, 1, (u8 *)&databuf, true))
	{
		GSE_ERR("read LSM6DS3_CTRL7_G err!\n");
		return LSM6DS3_ERR_I2C;
	}
         priv ->resume_LSM6DS3_CTRL7_G = databuf[0];

	//res = hwmsen_read_byte(client, LSM6DS3_CTRL8_XL, databuf);
	if(obj_i2c_data->tf->read(priv, LSM6DS3_CTRL8_XL, 1, (u8 *)&databuf, true))
	{
		GSE_ERR("read LSM6DS3_CTRL8_XL err!\n");
		return LSM6DS3_ERR_I2C;
	}
         priv ->resume_LSM6DS3_CTRL8_XL = databuf[0];

	//res = hwmsen_read_byte(client, LSM6DS3_CTRL9_XL, databuf);
	if(obj_i2c_data->tf->read(priv, LSM6DS3_CTRL9_XL, 1, (u8 *)&databuf, true))
	{
		GSE_ERR("read LSM6DS3_CTRL9_XL err!\n");
		return LSM6DS3_ERR_I2C;
	}
         priv ->resume_LSM6DS3_CTRL9_XL = databuf[0];

       
	//res = hwmsen_read_byte(client, LSM6DS3_CTRL10_C, databuf);
	if(obj_i2c_data->tf->read(priv, LSM6DS3_CTRL10_C, 1, (u8 *)&databuf, true))
	{
		GSE_ERR("read LSM6DS3_CTRL10_C err!\n");
		return LSM6DS3_ERR_I2C;
	}
         priv ->resume_LSM6DS3_CTRL10_C= databuf[0];

	return 0;
}


static int recover_ST_reg_config(struct i2c_client  *client )
{
         struct lsm6ds3_i2c_data *priv = obj_i2c_data;
        u8 databuf[2];
	int res;

        databuf[1] = priv ->resume_LSM6DS3_CTRL1_XL  ;
	databuf[0] = LSM6DS3_CTRL1_XL; 
	
	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
		GSE_ERR("write LSM6DS3_CTRL1_XL err!\n");
		return LSM6DS3_ERR_I2C;
	}

	databuf[1] =  priv ->resume_LSM6DS3_CTRL2_G;
	databuf[0] = LSM6DS3_CTRL2_G; 
	
	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
		GSE_ERR("write LSM6DS3_CTRL2_G err!\n");
		return LSM6DS3_ERR_I2C;
	}
	
	databuf[1] =  priv ->resume_LSM6DS3_CTRL3_C;
	databuf[0] = LSM6DS3_CTRL3_C; 
	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
		GSE_ERR("write LSM6DS3_CTRL3_C err!\n");
		return LSM6DS3_ERR_I2C;
	}
	
	databuf[1] = priv ->resume_LSM6DS3_CTRL4_C;
	databuf[0] = LSM6DS3_CTRL4_C; 
	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
		GSE_ERR("write LSM6DS3_CTRL4_C err!\n");
		return LSM6DS3_ERR_I2C;
	}

	databuf[1] =  priv ->resume_LSM6DS3_CTRL5_C;
	databuf[0] = LSM6DS3_CTRL5_C; 
	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
		GSE_ERR("write LSM6DS3_CTRL5_C err!\n");
		return LSM6DS3_ERR_I2C;
	}

	databuf[1] = priv ->resume_LSM6DS3_CTRL6_C;
	databuf[0] = LSM6DS3_CTRL6_C; 
	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
		GSE_ERR("write LSM6DS3_CTRL6_C err!\n");
		return LSM6DS3_ERR_I2C;
	}

    databuf[1] = priv ->resume_LSM6DS3_CTRL7_G;
	databuf[0] = LSM6DS3_CTRL7_G; 
	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
		GSE_ERR("write LSM6DS3_CTRL7_G err!\n");
		return LSM6DS3_ERR_I2C;
	}

    databuf[1] = priv ->resume_LSM6DS3_CTRL8_XL;
	databuf[0] = LSM6DS3_CTRL8_XL; 
	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
		GSE_ERR("write LSM6DS3_CTRL8_XL err!\n");
		return LSM6DS3_ERR_I2C;
	}

	databuf[1] = priv ->resume_LSM6DS3_CTRL9_XL;
	databuf[0] = LSM6DS3_CTRL9_XL; 
	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
		GSE_ERR("write LSM6DS3_CTRL9_XL err!\n");
		return LSM6DS3_ERR_I2C;
	}

    databuf[1] = priv ->resume_LSM6DS3_CTRL10_C;
	databuf[0] = LSM6DS3_CTRL10_C; 
	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
		GSE_ERR("write LSM6DS3_CTRL10_C err!\n");
		return LSM6DS3_ERR_I2C;
	}

	return 0;

}
static ssize_t show_acc_self_data(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = lsm6ds3_i2c_client;
	
	int temp[3] ;
	int OUTX_NOST,OUTY_NOST,OUTZ_NOST;
        int OUTX_ST,OUTY_ST,OUTZ_ST;
	int ABS_OUTX,ABS_OUTY,ABS_OUTZ;
	int res;
	u8 databuf[2];
	u8 result = -1;

	res = back_ST_reg_config(client); 

	if(res < 0)
	{
		GSE_ERR("back ST_reg err!\n");
		return LSM6DS3_ERR_I2C;
	}

	databuf[1] = 0x30 ;
	databuf[0] = LSM6DS3_CTRL1_XL; 
	
	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
		GSE_ERR("write LSM6DS3_CTRL1_XL err!\n");
		return LSM6DS3_ERR_I2C;
	}

	databuf[1] = 0x00 ;
	databuf[0] = LSM6DS3_CTRL2_G; 
	
	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
		GSE_ERR("write LSM6DS3_CTRL2_G err!\n");
		return LSM6DS3_ERR_I2C;
	}
	
	databuf[1] = 0x44 ;
	databuf[0] = LSM6DS3_CTRL3_C; 
	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
		GSE_ERR("write LSM6DS3_CTRL3_C err!\n");
		return LSM6DS3_ERR_I2C;
	}
	
	databuf[1] = 0x00 ;
	databuf[0] = LSM6DS3_CTRL4_C; 
	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
		GSE_ERR("write LSM6DS3_CTRL4_C err!\n");
		return LSM6DS3_ERR_I2C;
	}

	databuf[1] = 0x00 ;
	databuf[0] = LSM6DS3_CTRL5_C; 
	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
		GSE_ERR("write LSM6DS3_CTRL5_C err!\n");
		return LSM6DS3_ERR_I2C;
	}

	databuf[1] = 0x00 ;
	databuf[0] = LSM6DS3_CTRL6_C; 
	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
		GSE_ERR("write LSM6DS3_CTRL6_C err!\n");
		return LSM6DS3_ERR_I2C;
	}

        databuf[1] = 0x00 ;
	databuf[0] = LSM6DS3_CTRL7_G; 
	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
		GSE_ERR("write LSM6DS3_CTRL7_G err!\n");
		return LSM6DS3_ERR_I2C;
	}

        databuf[1] = 0x00 ;
	databuf[0] = LSM6DS3_CTRL8_XL; 
	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
		GSE_ERR("write LSM6DS3_CTRL8_XL err!\n");
		return LSM6DS3_ERR_I2C;
	}

	databuf[1] = 0x38 ;
	databuf[0] = LSM6DS3_CTRL9_XL; 
	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
		GSE_ERR("write LSM6DS3_CTRL9_XL err!\n");
		return LSM6DS3_ERR_I2C;
	}

        databuf[1] = 0x00 ;
	databuf[0] = LSM6DS3_CTRL10_C; 
	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
		GSE_ERR("write LSM6DS3_CTRL10_C err!\n");
		return LSM6DS3_ERR_I2C;
	}

	LSM6DS3_dumpReg(client);
		 
	 msleep(200);

	GSE_ERR("setting ST!\n");
	
	lsm303d_acc_get_ST_data(client, temp);

	OUTX_NOST =  temp[0] /5000;                                 // ug transfer to mg
	OUTY_NOST =  temp[1] /5000;	
	OUTZ_NOST =  temp[2] /5000;
        GSE_ERR("OUTX_NOST %d, OUTY_NOST %d,OUTZ_NOST %d \n",OUTX_NOST,OUTY_NOST,OUTZ_NOST);

        databuf[1] = 0x01 ;
	databuf[0] = LSM6DS3_CTRL5_C; 
	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
		GSE_ERR("write LSM6DS3_CTRL5_C err!\n");
		return LSM6DS3_ERR_I2C;
	}

	LSM6DS3_dumpReg(client);
	
	msleep(200);

	lsm303d_acc_get_ST_data(client, temp);

	OUTX_ST =  temp[0] /5000;
	OUTY_ST =  temp[1] /5000;	
	OUTZ_ST =  temp[2] /5000;

       GSE_ERR("OUTX_ST %d, OUTY_ST %d,OUTZ_ST %d \n",OUTX_ST,OUTY_ST,OUTZ_ST);


        ABS_OUTX = ABS_ST(OUTX_ST - OUTX_NOST) ;
        ABS_OUTY = ABS_ST(OUTY_ST - OUTY_NOST);
        ABS_OUTZ = ABS_ST(OUTZ_ST - OUTZ_NOST);

	 if(((ABS_OUTX >= MIN_ST)&&(ABS_OUTX <= MAX_ST))
	 	&& ((ABS_OUTY >= MIN_ST)&&(ABS_OUTY <= MAX_ST))
	        && ((ABS_OUTZ >= MIN_ST)&&(ABS_OUTZ <= MAX_ST)))
	{
		GSE_ERR(" SELF TEST SUCCESS ABS_OUTX,ABS_OUTY,ABS_OUTZ:%d ,%d,%d\n", ABS_OUTX,ABS_OUTY,ABS_OUTZ);
		result = 1;
	}
	else
	{
		GSE_ERR(" SELF TEST FAIL ABS_OUTX,ABS_OUTY,ABS_OUTZ:%d ,%d,%d\n", ABS_OUTX,ABS_OUTY,ABS_OUTZ);
              result = 0;
	 }

      /*  databuf[1] = 0x00 ;
	databuf[0] = LSM6DS3_CTRL1_XL; 
	
	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
		GSE_ERR("write LSM6DS3_CTRL1_XL err!\n");
		return LSM6DS3_ERR_I2C;
	}
			 
	databuf[1] = 0x00 ;
	databuf[0] = LSM6DS3_CTRL5_C; 
	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
		GSE_ERR("write LSM6DS3_CTRL5_C err!\n");
		return LSM6DS3_ERR_I2C;
	}

	LSM6DS3_init_client(client, false); */

	 res = recover_ST_reg_config(client); 

	  if(res < 0)
	{
		GSE_ERR("recover ST_reg err!\n");
		return LSM6DS3_ERR_I2C;
	}
		   
	return sprintf(buf, "%d\n", result);
}
#endif
/*lenovo-sw caoyi1 add for selftest function 20150527 end*/

/*----------------------------------------------------------------------------*/

static DRIVER_ATTR(chipinfo,             S_IRUGO, show_chipinfo_value,      NULL);
static DRIVER_ATTR(chipid,             S_IRUGO, show_chipid_value,      NULL);
static DRIVER_ATTR(sensorrawdata,           S_IRUGO, show_sensorrawdata_value,    NULL);
static DRIVER_ATTR(sensordata,           S_IRUGO, show_sensordata_value,    NULL);
static DRIVER_ATTR(trace,      S_IWUSR | S_IRUGO, show_trace_value,         store_trace_value);
static DRIVER_ATTR(chipinit,      S_IWUSR | S_IRUGO, show_chipinit_value,         store_chipinit_value);
static DRIVER_ATTR(status,               S_IRUGO, show_status_value,        NULL);
static DRIVER_ATTR(layout,      S_IRUGO | S_IWUSR, show_layout_value, store_layout_value);
/*lenovo-sw caoyi1 add for Gsensor APK calibration 20150512 begin*/
#ifdef LSM6DS3_CALIB_HAL
static DRIVER_ATTR(gsensor_cali_val_x,      S_IRUGO | S_IWUSR, show_cali_x_value, store_cali_x_value);
static DRIVER_ATTR(gsensor_cali_val_y,      S_IRUGO | S_IWUSR, show_cali_y_value, store_cali_y_value);
static DRIVER_ATTR(gsensor_cali_val_z,      S_IRUGO | S_IWUSR, show_cali_z_value, store_cali_z_value);
#endif
/*lenovo-sw caoyi1 add for Gsensor APK calibration 20150512 end*/
/*lenovo-sw caoyi1 add for selftest function 20150527 begin*/
#ifdef LSM6DS3_FTM_ENABLE
static DRIVER_ATTR(selftest,  S_IRUGO , show_acc_self_data, NULL);
#endif
/*lenovo-sw caoyi1 add for selftest function 20150527 end*/
/*----------------------------------------------------------------------------*/
static struct driver_attribute *LSM6DS3_attr_list[] = {
	&driver_attr_chipinfo,     /*chip information*/
	&driver_attr_chipid,       /*chip id*/
	&driver_attr_sensordata,   /*dump sensor data*/	
	&driver_attr_sensorrawdata,   /*dump sensor raw data*/	
	&driver_attr_trace,        /*trace log*/
	&driver_attr_status,  
	&driver_attr_chipinit,
	&driver_attr_layout,
/*lenovo-sw caoyi1 add for Gsensor APK calibration 20150512 begin*/
#ifdef LSM6DS3_CALIB_HAL
	&driver_attr_gsensor_cali_val_x ,   /* add for lenovo hal calib syspoint by jonny */
	&driver_attr_gsensor_cali_val_y ,   /* add for lenovo hal calib syspoint by jonny */
	&driver_attr_gsensor_cali_val_z ,   /* add for lenovo hal calib syspoint by jonny */
#endif
/*lenovo-sw caoyi1 add for Gsensor APK calibration 20150512 end*/
/*lenovo-sw caoyi1 add for selftest function 20150527 begin*/
#ifdef LSM6DS3_FTM_ENABLE
	&driver_attr_selftest,
#endif
/*lenovo-sw caoyi1 add for selftest function 20150527 end*/
};
/*----------------------------------------------------------------------------*/
static int lsm6ds3_create_attr(struct device_driver *driver) 
{
	int idx, err = 0;
	int num = (int)(sizeof(LSM6DS3_attr_list)/sizeof(LSM6DS3_attr_list[0]));
	if (driver == NULL)
	{
		return -EINVAL;
	}

	for(idx = 0; idx < num; idx++)
	{
		if(0 != (err = driver_create_file(driver,  LSM6DS3_attr_list[idx])))
		{            
			GSE_ERR("driver_create_file (%s) = %d\n",  LSM6DS3_attr_list[idx]->attr.name, err);
			break;
		}
	}    
	return err;
}
/*----------------------------------------------------------------------------*/
static int lsm6ds3_delete_attr(struct device_driver *driver)
{
	int idx ,err = 0;
	int num = (int)(sizeof( LSM6DS3_attr_list)/sizeof( LSM6DS3_attr_list[0]));

	if(driver == NULL)
	{
		return -EINVAL;
	}	

	for(idx = 0; idx < num; idx++)
	{
		driver_remove_file(driver,  LSM6DS3_attr_list[idx]);
	}
	return err;
}
static int LSM6DS3_Set_RegInc(struct i2c_client *client, bool inc)
{
	u8 databuf[2] = {0};    
	int res = 0;
	struct lsm6ds3_i2c_data *obj = i2c_get_clientdata(client);
	//GSE_FUN();     
	
	//if(hwmsen_read_byte(client, LSM6DS3_CTRL3_C, databuf))
	if(obj->tf->read(obj, LSM6DS3_CTRL3_C, 1, databuf, true))
	{
		GSE_ERR("read LSM6DS3_CTRL3_XL err!\n");
		return LSM6DS3_ERR_I2C;
	}
	else
	{
		GSE_LOG("read  LSM6DS3_CTRL3_C register: 0x%x\n", databuf[0]);
	}
	if(inc)
	{
		databuf[0] |= LSM6DS3_CTRL3_C_IFINC;
		
		databuf[1] = databuf[0];
		databuf[0] = LSM6DS3_CTRL3_C; 
		
		//res = i2c_master_send(client, databuf, 0x2);
		res = obj->tf->write(obj, databuf[0], 1, &databuf[1], true);
		if(res < 0)
		{
			GSE_ERR("write full scale register err!\n");
			return LSM6DS3_ERR_I2C;
		}
	}

	return LSM6DS3_SUCCESS;    
}

/*----------------------------------------------------------------------------*/
/* software reset, executed when internal osc is turned on  -- add by liaoxl.lenovo 8.27.2005 start */
static int LSM6DS3_Soft_Reset(struct i2c_client *client)
{
	u8 databuf[2] = {0};    
	int res = 0;
	struct lsm6ds3_i2c_data *obj = i2c_get_clientdata(client);
	//GSE_FUN();     
	
	//if(hwmsen_read_byte(client, LSM6DS3_CTRL3_C, databuf))
	if(obj->tf->read(obj, LSM6DS3_CTRL3_C, 1, databuf, true))
	{
		GSE_ERR("read LSM6DS3_CTRL3_XL err!\n");
		return LSM6DS3_ERR_I2C;
	}
	else
	{
		GSE_LOG("read  LSM6DS3_CTRL3_C register: 0x%x\n", databuf[0]);
	}

	databuf[0] |= 0x01; /* software reset */
	databuf[1] = databuf[0];
	databuf[0] = LSM6DS3_CTRL3_C; 
	
	//res = i2c_master_send(client, databuf, 0x2);
	res = obj->tf->write(obj, databuf[0], 1, &databuf[1], true);
	if(res < 0)
	{
		GSE_ERR("write soft reset register err!\n");
		return LSM6DS3_ERR_I2C;
	}

	return LSM6DS3_SUCCESS;    
}
/* software reset, executed when internal osc is turned on  -- add by liaoxl.lenovo 8.27.2005 end */


static int LSM6DS3_init_client(struct i2c_client *client, bool enable)
{
	struct lsm6ds3_i2c_data *obj = i2c_get_clientdata(client);
	int res = 0;
	GSE_FUN();	
    GSE_LOG(" lsm6ds3 addr %x!\n",client->addr);
/* do reset to ensure hardware inital state -- add by liaoxl.lenovo 8.27.2005 start */
    res = LSM6DS3_acc_SetPowerMode(client, true);
    if(res != LSM6DS3_SUCCESS)
    {
    	return res;
    }
    res = LSM6DS3_Soft_Reset(client);
    if(res != LSM6DS3_SUCCESS)
    {
    	return res;
    }
    msleep(30);
/* do reset to ensure hardware inital state -- add by liaoxl.lenovo 8.27.2005 end */

	res = LSM6DS3_CheckDeviceID(client);
	if(res != LSM6DS3_SUCCESS)
	{
		return res;
	}

	res = LSM6DS3_Set_RegInc(client, true);
	if(res != LSM6DS3_SUCCESS)
	{
		return res;
	}
	
	res = LSM6DS3_acc_SetFullScale(client,LSM6DS3_ACC_RANGE_2g);//we have only this choice
	if(res != LSM6DS3_SUCCESS) 
	{
		return res;
	}

	//res = LSM6DS3_acc_SetSampleRate(client, LSM6DS3_ACC_ODR_104HZ);
	res = LSM6DS3_acc_SetSampleRate(client, obj->sample_rate);
	if(res != LSM6DS3_SUCCESS ) 
	{
		return res;
	}

	res = LSM6DS3_acc_SetPowerMode(client, enable);
	if(res != LSM6DS3_SUCCESS)
	{
		return res;
	}

	GSE_LOG("LSM6DS3_init_client OK!\n");
	//acc setting
	
	return LSM6DS3_SUCCESS;
}
/*----------------------------------------------------------------------------*/
#ifdef LSM6DS3_NEW_ARCH
static int lsm6ds3_open_report_data(int open)
{
    //should queuq work to report event if  is_report_input_direct=true
	
    return 0;
}

// if use  this typ of enable , Gsensor only enabled but not report inputEvent to HAL

static int lsm6ds3_enable_nodata(int en)
{
	int value = en;
	int err = 0;
	struct lsm6ds3_i2c_data *priv = obj_i2c_data;

	if(priv == NULL)
	{
		GSE_ERR("obj_i2c_data is NULL!\n");
		return -1;
	}

	if(value == 1)
	{
		enable_status = true;
	}
	else
	{
		enable_status = false;
		priv->sample_rate = LSM6DS3_ACC_ODR_104HZ; //default rate
	}
	GSE_LOG("enable value=%d, sensor_power =%d\n",value,sensor_power);
	
	if(((value == 0) && (sensor_power == false)) ||((value == 1) && (sensor_power == true)))
	{
		GSE_LOG("Gsensor device have updated!\n");
	}
	else if(!pedo_enable_status && !tilt_enable_status & !sigm_enable_status)
	{
		err = LSM6DS3_acc_SetPowerMode( priv->client, enable_status);					
	}

    GSE_LOG("%s OK!\n",__FUNCTION__);
    return err;
}

static int lsm6ds3_set_delay(u64 ns)
{
    int value =0;
	int err = 0;
	int sample_delay;
	struct lsm6ds3_i2c_data *priv = obj_i2c_data;

	if(priv == NULL)
	{
		GSE_ERR("obj_i2c_data is NULL!\n");
		return -1;
	}
	
    value = (int)(ns/1000/1000);
	if(value <= 5)
	{
		sample_delay = LSM6DS3_ACC_ODR_208HZ;
	}
	else if(value <= 10)
	{
		sample_delay = LSM6DS3_ACC_ODR_104HZ;
	}
	else
	{
		sample_delay = LSM6DS3_ACC_ODR_52HZ;
	}
	priv->sample_rate = sample_delay;
	err = LSM6DS3_acc_SetSampleRate(priv->client, sample_delay);
	if(err != LSM6DS3_SUCCESS ) 
	{
		GSE_ERR("Set delay parameter error!\n");
	}

    GSE_LOG("%s (%d), chip only use 1024HZ \n",__FUNCTION__, value);
    return 0;
}

static int lsm6ds3_get_data(int* x ,int* y,int* z, int* status)
{
    char buff[LSM6DS3_BUFSIZE];
	struct lsm6ds3_i2c_data *priv = obj_i2c_data;
	
	if(priv == NULL)
	{
		GSE_ERR("obj_i2c_data is NULL!\n");
		return -1;
	}
	if(atomic_read(&priv->trace) & ACCEL_TRC_DATA)
	{
		GSE_LOG("%s (%d),  \n",__FUNCTION__,__LINE__);
	}
	memset(buff, 0, sizeof(buff));
	LSM6DS3_ReadAccData(priv->client, buff, LSM6DS3_BUFSIZE);
	
	sscanf(buff, "%x %x %x", x, y, z);				
	*status = SENSOR_STATUS_ACCURACY_MEDIUM;				

    return 0;
}
#ifdef LSM6DS3_TILT_FUNC
static int lsm6ds3_tilt_open_report_data(int open)
{
	int res = 0;
	struct lsm6ds3_i2c_data *priv = obj_i2c_data;
	
	if(1 == open)
	{
		tilt_enable_status = true;
		res = LSM6DS3_enable_tilt(priv->client, true);
		if(LSM6DS3_SUCCESS != res)
		{
			GSE_ERR("%s run LSM6DS3_enable_tilt to true failed!\n", __func__);
		}
	}
	else if(0 == open)
	{
		tilt_enable_status = false;
		res = LSM6DS3_enable_tilt(priv->client, false);
		if(LSM6DS3_SUCCESS != res)
		{
			GSE_ERR("%s run LSM6DS3_enable_tilt to false failed!\n", __func__);
		}
	}
	
	return res;
}
#endif

#ifdef LSM6DS3_SIGNIFICANT_MOTION
static int lsm6ds3_step_c_enable_significant(int en)
{
	int res =0;
	struct lsm6ds3_i2c_data *priv = obj_i2c_data;
	GSE_FUN();
	
	if(1 == en)
	{
		sigm_enable_status = true;
		res = LSM6DS3_Set_SigMotion_Threshold(priv->client, 0x08);
		if(LSM6DS3_SUCCESS != res)
		{
			GSE_ERR("%s run LSM6DS3_Set_SigMotion_Threshold to fail!\n", __func__);
		}
		//res = LSM6DS3_acc_SetSampleRate(priv->client, LSM6DS3_ACC_ODR_26HZ);
		res = LSM6DS3_acc_SetSampleRate(priv->client, priv->sample_rate);
		if(LSM6DS3_SUCCESS != res)
		{
			GSE_ERR("%s run LSM6DS3_Set_SigMotion_Threshold to fail!\n", __func__);
		}
		res = LSM6DS3_Enable_SigMotion_Func_On_Int(priv->client, true); //default route to INT2
		if(LSM6DS3_SUCCESS != res)
		{
			GSE_ERR("%s run LSM6DS3_Enable_SigMotion_Func_On_Int to fail!\n", __func__);
		}
		
		res = LSM6DS3_acc_SetFullScale(priv->client, LSM6DS3_ACC_RANGE_2g);
		if(LSM6DS3_SUCCESS != res)
		{
			GSE_ERR("%s run LSM6DS3_Enable_SigMotion_Func_On_Int to fail!\n", __func__);
		}
		GSE_LOG(" sigMotion enable with1 int1_count=%d\n", atomic_read(&priv->int1_request_num));
/*Lenovo-sw weimh1 mod 2016-4-20 begin: just enable without condition*/
#if 0
		if(0 == atomic_read(&priv->int1_request_num))
#endif		
/*Lenovo-sw weimh1 mod 2016-4-20 end*/
/*lenovo-sw caoyi modify for eint API 20151021 begin*/
		{
#if defined(CONFIG_OF)
			enable_irq(obj_i2c_data->irq);
#else
			mt_eint_unmask(CUST_EINT_LSM6DS3_NUM);
#endif
		}
/*lenovo-sw caoyi modify for eint API 20151021 end*/
		atomic_inc(&priv->int1_request_num);
		GSE_LOG(" sigMotion enable with2 int1_count=%d\n", atomic_read(&priv->int1_request_num));
	}
	else if(0 == en)
	{
		sigm_enable_status = false;
		res = LSM6DS3_Enable_SigMotion_Func_On_Int(priv->client, false);
		if(LSM6DS3_SUCCESS != res)
		{
			GSE_ERR("%s run LSM6DS3_Enable_SigMotion_Func_On_Int to fail!\n", __func__);
		}
		/* do not turn off the func when other related func working, modified by liaoxl.lenovo 7.6.2015 */
		if(!enable_status && !tilt_enable_status && !pedo_enable_status)
		{
			res = LSM6DS3_acc_SetPowerMode(priv->client, false);
			if(LSM6DS3_SUCCESS != res)
			{
				GSE_ERR("%s run LSM6DS3_acc_SetPowerMode to fail!\n", __func__);
			}
		}
		GSE_LOG(" sigMotion disable with1 int1_count=%d\n", atomic_read(&priv->int1_request_num));
		atomic_dec(&priv->int1_request_num);
		if(0 >= atomic_read(&priv->int1_request_num))
		{
			atomic_set(&priv->int1_request_num, 0);
			cancel_work_sync(&priv->eint_work);
/*lenovo-sw caoyi modify for eint API 20151021 begin*/
#if defined(CONFIG_OF)
//			disable_irq(obj_i2c_data->irq);
#else
			mt_eint_mask(CUST_EINT_LSM6DS3_NUM);
#endif
/*lenovo-sw caoyi modify for eint API 20151021 end*/
		}
		GSE_LOG(" sigMotion disable with2 int1_count=%d\n", atomic_read(&priv->int1_request_num));
	}
	
	return res;
}
#endif

#ifdef LSM6DS3_STEP_COUNTER //step counter
static int lsm6ds3_step_c_open_report_data(int open)
{
	
	return LSM6DS3_SUCCESS;
}
static int lsm6ds3_step_c_enable_nodata(int en)
{
	int res =0;
	int value = en;
	int err = 0;
	struct lsm6ds3_i2c_data *priv = obj_i2c_data;
	GSE_FUN();

	if(priv == NULL)
	{
		GSE_ERR("%s obj_i2c_data is NULL!\n", __func__);
		return -1;
	}

	if(value == 1)
	{
		pedo_enable_status = true;
		res = LSM6DS3_enable_pedo(priv->client, true);
		if(LSM6DS3_SUCCESS != res)
		{
			GSE_LOG("LSM6DS3_enable_pedo failed at open action!\n");
			return res;
		}

		step_notify(TYPE_STEP_COUNTER);/* for data init of step counter, add by liaoxl.lenovo 8.17.2015 */
	}
	else
	{
		pedo_enable_status = false;
		res = LSM6DS3_enable_pedo(priv->client, false);
		if(LSM6DS3_SUCCESS != res)
		{
			GSE_LOG("LSM6DS3_enable_pedo failed at close action!\n");
			return res;
		}

	}
	
	GSE_LOG("lsm6ds3_step_c_enable_nodata en=%d OK!\n", en);
    return err;
}
static int lsm6ds3_step_c_enable_step_detect(int en)
{
	int ret = 0;
	u8 databuf[2] = {0}; 
	struct lsm6ds3_i2c_data *obj = obj_i2c_data;
	GSE_FUN();

	/* enable INT1_STEP_DETECTOR */
	//if(hwmsen_read_byte(client, LSM6DS3_INT1_CTRL, databuf))
	if(obj->tf->read(obj, LSM6DS3_INT1_CTRL, 1, databuf, true))
	{
		GSE_ERR("%s read LSM6DS3_INT1_CTRL register err!\n", __func__);
		return LSM6DS3_ERR_I2C;
	}
	else
	{
		GSE_LOG("read  LSM6DS3_INT1_CTRL register: 0x%x\n", databuf[0]);
	}
	if(1 == en)
	{
		databuf[0] |= 0x80;			
	}
	else
	{
		databuf[0] &= ~0x80;//clear 
	}
	databuf[1] = databuf[0];
	databuf[0] = LSM6DS3_INT1_CTRL; 	
	//res = i2c_master_send(client, databuf, 0x2);
	ret = obj->tf->write(obj, databuf[0], 1, &databuf[1], true);
	if(ret < 0)
	{
		GSE_ERR("write LSM6DS3_INT1_CTRL register err!\n");
		return LSM6DS3_ERR_I2C;
	}

	if(0 == en)
	{
		if(false == pedo_enable_status)
		{
			ret = LSM6DS3_enable_pedo(obj->client, en);
		}
		else
		{
			/* step counter remind working */
		}
		atomic_dec(&obj->int1_request_num);
		if(0 >= atomic_read(&obj->int1_request_num))
		{
			atomic_set(&obj->int1_request_num, 0);
			cancel_work_sync(&obj->eint_work);
/*lenovo-sw caoyi modify for eint API 20151021 begin*/
#if defined(CONFIG_OF)
//			disable_irq(obj_i2c_data->irq);
#else
			mt_eint_mask(CUST_EINT_LSM6DS3_NUM);
#endif
/*lenovo-sw caoyi modify for eint API 20151021 end*/
		}
		stepd_enable_status = false;  /* add for step detector sensor -- by liaoxl.lenovo 7.12.2015 */
		GSE_LOG(" disable detect with pedo=%d count=%d\n", pedo_enable_status, atomic_read(&obj->int1_request_num));
	}
	else if(1 == en)
	{
		if(false == pedo_enable_status)
		{
			ret = LSM6DS3_enable_pedo(obj->client, en);
		}
/*Lenovo-sw weimh1 mod 2016-4-20 begin: just enable without condition*/
#if 0
		if(0 == atomic_read(&obj->int1_request_num))
#endif		
/*Lenovo-sw weimh1 mod 2016-4-20 end*/
/*lenovo-sw caoyi modify for eint API 20151021 begin*/
		{
#if defined(CONFIG_OF)
			enable_irq(obj_i2c_data->irq);
#else
			mt_eint_unmask(CUST_EINT_LSM6DS3_NUM);
#endif
		}
/*lenovo-sw caoyi modify for eint API 20151021 end*/
		atomic_inc(&obj->int1_request_num);
		stepd_enable_status = true;  /* add for step detector sensor -- by liaoxl.lenovo 7.12.2015 */
		GSE_LOG(" enable detect with pedo=%d count=%d\n", pedo_enable_status, atomic_read(&obj->int1_request_num));
	}

	return ret;
}

static int lsm6ds3_step_c_set_delay(u64 delay)
{
	u8 databuf[2] = {0}, delta = 0;
	int res = 0, mre, mod;
	struct lsm6ds3_i2c_data *obj = obj_i2c_data;

	mre = (int)delay / 1600;
	mod = (delay % 1600);
	if(mre > 200)
		delta = 200;
	else
		delta = (u8)mre;
	if(mod > 800)
		delta += 1;

	if(obj->step_c_delay != delta)
	{
		obj->step_c_delay = delta;
		GSE_LOG(" now lsm6ds3_step_c_set_delay = %llums\n", delay);
		/* STEP_COUNT_DELTA value, 1LSB = 1.6384S */
		/* treat as batch timeout value */
		databuf[1] = 0x80;
		databuf[0] = LSM6DS3_FUNC_CFG_ACCESS;
		//res = i2c_master_send(client, databuf, 0x2);
		res = obj->tf->write(obj, databuf[0], 1, &databuf[1], true);
		if(res < 0)
		{
			GSE_ERR("%s turnon embedded func access err1!\n", __func__);
			return LSM6DS3_ERR_I2C;
		}
		databuf[1] = delta;  // 3 steps for test
		databuf[0] = LSM6DS3_STEP_COUNT_DELTA;
		//res = i2c_master_send(client, databuf, 0x2);
		res = obj->tf->write(obj, databuf[0], 1, &databuf[1], true);
		if(res < 0)
		{
			GSE_ERR("%s write LSM6DS3_STEP_COUNT_DELTA register err2!\n", __func__);
			return LSM6DS3_ERR_I2C;
		}
		databuf[1] = 0x00;
		databuf[0] = LSM6DS3_FUNC_CFG_ACCESS;
		//res = i2c_master_send(client, databuf, 0x2);
		res = obj->tf->write(obj, databuf[0], 1, &databuf[1], true);
		if(res < 0)
		{
			GSE_ERR("%s turnoff embedded func access err!\n", __func__);
			return LSM6DS3_ERR_I2C;
		}
	}

	return 0;
}
static int lsm6ds3_step_c_get_data(u64 *value, int *status)
{
	int err = 0;
	u16 pedo_data = 0;
	struct lsm6ds3_i2c_data *priv = obj_i2c_data;
	int srst = atomic_read(&priv->reset_sc);

	err = LSM6DS3_Get_Pedo_DataReg(priv->client, &pedo_data);
	if(LSM6DS3_SUCCESS == err)
	{
		/* step counter overflow occus just now */
		if(0 != srst)
		{
			atomic_set(&priv->reset_sc, 0);
			priv->steps_c = pedo_data;
		}
		/* none reset operaton,do debounce check */
		else
		{
			/* lsm6ds data reg overflow result to this error? */
			if(pedo_data < priv->steps_c)
			{
				/* use old steps */
				if(0 == priv->bounce_count)
				{
					priv->bounce_count = 1;
					priv->last_bounce_step = pedo_data;
				}
				else
				{
					if(pedo_data > priv->last_bounce_step)
					{
						priv->bounce_count = 0;
						priv->bounce_steps += priv->steps_c;
						priv->steps_c = pedo_data;
					}
					else
					{
						priv->bounce_count = 0;
					}
				}
			}
			else
			{
				priv->steps_c = pedo_data;
			}
		}

		*value = priv->steps_c + priv->bounce_steps + ((priv->overflow) << 16);
		*status = SENSOR_STATUS_ACCURACY_MEDIUM;
		if(atomic_read(&priv->trace) & ACCEL_TRC_DATA)
		{
			GSE_LOG(" now lsm6ds3_step_c_get_data steps reg=%d report=%d bounce=%u of=%d rst=%d\n",
						pedo_data, priv->steps_c, priv->bounce_steps, priv->overflow, srst);
		}
	}
	
	return err;
}
static int lsm6ds3_step_c_get_data_step_d(u64 *value, int *status)
{
	return 0;
}
static int lsm6ds3_step_c_get_data_significant(u64 *value, int *status)
{
	return 0;
}
#endif
#else
static int LSM6DS3_acc_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value, sample_delay;	
	struct lsm6ds3_i2c_data *priv = (struct lsm6ds3_i2c_data*)self;
	struct hwm_sensor_data* gsensor_data;
	char buff[LSM6DS3_BUFSIZE];
	
	//GSE_FUN(f);
	switch (command)
	{
		case SENSOR_DELAY:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				GSE_ERR("Set delay parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				value = *(int *)buff_in;
				if(value <= 5)
				{
					sample_delay = LSM6DS3_ACC_ODR_208HZ;
				}
				else if(value <= 10)
				{
					sample_delay = LSM6DS3_ACC_ODR_104HZ;
				}
				else
				{
					sample_delay = LSM6DS3_ACC_ODR_52HZ;
				}
				
				priv->sample_rate = sample_delay;
				LSM6DS3_acc_SetSampleRate(priv->client, sample_delay);
				if(err != LSM6DS3_SUCCESS ) 
				{
					GSE_ERR("Set delay parameter error!\n");
				}

				if(value >= 50)
				{
					atomic_set(&priv->filter, 0);
				}
				else
				{					
					priv->fir.num = 0;
					priv->fir.idx = 0;
					priv->fir.sum[LSM6DS3_AXIS_X] = 0;
					priv->fir.sum[LSM6DS3_AXIS_Y] = 0;
					priv->fir.sum[LSM6DS3_AXIS_Z] = 0;
					atomic_set(&priv->filter, 1);
				}
			}
			break;

		case SENSOR_ENABLE:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				GSE_ERR("Enable sensor parameter error!\n");
				err = -EINVAL;
			}
			else
			{
			    
				value = *(int *)buff_in;
				if(value == 1)
				{
					enable_status = true;
				}
				else
				{
					enable_status = false;
					priv->sample_rate = LSM6DS3_ACC_ODR_104HZ; //default rate
				}
				GSE_LOG("enable value=%d, sensor_power =%d\n",value,sensor_power);
				
				if(((value == 0) && (sensor_power == false)) ||((value == 1) && (sensor_power == true)))
				{
					GSE_LOG("Gsensor device have updated!\n");
				}
				else if(!pedo_enable_status && !tilt_enable_status)
				{
					err = LSM6DS3_acc_SetPowerMode( priv->client, enable_status);					
				}

			}
			break;

		case SENSOR_GET_DATA:
			if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
			{
				GSE_ERR("get sensor data parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				gsensor_data = (hwm_sensor_data *)buff_out;
				LSM6DS3_ReadAccData(priv->client, buff, LSM6DS3_BUFSIZE);
				
				sscanf(buff, "%x %x %x", &gsensor_data->values[0], 
					&gsensor_data->values[1], &gsensor_data->values[2]);				
				gsensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;				
				gsensor_data->value_divide = 1000;
			}
			break;
		default:
			GSE_ERR("gsensor operate function no this parameter %d!\n", command);
			err = -1;
			break;
	}
	
	return err;
}
#endif

/****************************************************************************** 
 * Function Configuration
******************************************************************************/
static int lsm6ds3_open(struct inode *inode, struct file *file)
{
	file->private_data = lsm6ds3_i2c_client;

	if(file->private_data == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return -EINVAL;
	}
	return nonseekable_open(inode, file);
}
/*----------------------------------------------------------------------------*/
static int lsm6ds3_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}
/*----------------------------------------------------------------------------*/
static long lsm6ds3_acc_unlocked_ioctl(struct file *file, unsigned int cmd,
       unsigned long arg)
{
	struct i2c_client *client = (struct i2c_client*)file->private_data;
	struct lsm6ds3_i2c_data *obj = (struct lsm6ds3_i2c_data*)i2c_get_clientdata(client);	
	char strbuf[LSM6DS3_BUFSIZE];
	void __user *data;
	struct SENSOR_DATA sensor_data;
	int err = 0;
	int cali[3];

	//GSE_FUN(f);
	if(_IOC_DIR(cmd) & _IOC_READ)
	{
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	}
	else if(_IOC_DIR(cmd) & _IOC_WRITE)
	{
		err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	}

	if(err)
	{
		GSE_ERR("access error: %08X, (%2d, %2d)\n", cmd, _IOC_DIR(cmd), _IOC_SIZE(cmd));
		return -EFAULT;
	}

	switch(cmd)
	{
		case GSENSOR_IOCTL_INIT:			
			break;

		case GSENSOR_IOCTL_READ_CHIPINFO:
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}
			
			LSM6DS3_ReadChipInfo(client, strbuf, LSM6DS3_BUFSIZE);
			
			if(copy_to_user(data, strbuf, strlen(strbuf)+1))
			{
				err = -EFAULT;
				break;
			}				 
			break;	  

		case GSENSOR_IOCTL_READ_SENSORDATA:
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}
			
			LSM6DS3_ReadAccData(client, strbuf, LSM6DS3_BUFSIZE);
			
			if(copy_to_user(data, strbuf, strlen(strbuf)+1))
			{
				err = -EFAULT;
				break;	  
			}				 
			break;

		case GSENSOR_IOCTL_READ_GAIN:
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}			

			break;

		case GSENSOR_IOCTL_READ_OFFSET:
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}

			break;

		case GSENSOR_IOCTL_READ_RAW_DATA:
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}
			
			LSM6DS3_ReadAccRawData(client, (s16 *)strbuf);
			if(copy_to_user(data, strbuf, strlen(strbuf)+1))
			{
				err = -EFAULT;
				break;	  
			}
			break;	  

		case GSENSOR_IOCTL_SET_CALI:
			data = (void __user*)arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}
			if(copy_from_user(&sensor_data, data, sizeof(sensor_data)))
			{
				err = -EFAULT;
				break;	  
			}
			if(atomic_read(&obj->suspend))
			{
				GSE_ERR("Perform calibration in suspend state!!\n");
				err = -EINVAL;
			}
			else
			{
		#if 0
			cali[LSM6DS3_AXIS_X] = (s64)(sensor_data.x) * 1000*1000/(obj->sensitivity*GRAVITY_EARTH_1000); //NTC
			cali[LSM6DS3_AXIS_Y] = (s64)(sensor_data.y) * 1000*1000/(obj->sensitivity*GRAVITY_EARTH_1000);
			cali[LSM6DS3_AXIS_Z] = (s64)(sensor_data.z) * 1000*1000/(obj->sensitivity*GRAVITY_EARTH_1000);
		#else
			cali[LSM6DS3_AXIS_X] = (s64)(sensor_data.x);
			cali[LSM6DS3_AXIS_Y] = (s64)(sensor_data.y);	
			cali[LSM6DS3_AXIS_Z] = (s64)(sensor_data.z);	
		#endif
				err = LSM6DS3_acc_WriteCalibration(client, cali);			 
			}
			break;

		case GSENSOR_IOCTL_CLR_CALI:
			err = LSM6DS3_acc_ResetCalibration(client);
			break;

		case GSENSOR_IOCTL_GET_CALI:
			data = (void __user*)arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}
			err = LSM6DS3_acc_ReadCalibration(client, cali);
			if(err < 0)
			{
				break;
			}
					
		#if 0
			sensor_data.x = (s64)(cali[LSM6DS3_AXIS_X]) * obj->sensitivity*GRAVITY_EARTH_1000/(1000*1000); //NTC
			sensor_data.y = (s64)(cali[LSM6DS3_AXIS_Y]) * obj->sensitivity*GRAVITY_EARTH_1000/(1000*1000);
			sensor_data.z = (s64)(cali[LSM6DS3_AXIS_Z]) * obj->sensitivity*GRAVITY_EARTH_1000/(1000*1000);
		#else
			sensor_data.x = (s64)(cali[LSM6DS3_AXIS_X]);
			sensor_data.y = (s64)(cali[LSM6DS3_AXIS_Y]);
			sensor_data.z = (s64)(cali[LSM6DS3_AXIS_Z]);
		#endif
			if(copy_to_user(data, &sensor_data, sizeof(sensor_data)))
			{
				err = -EFAULT;
				break;
			}		
			break;
		
		default:
			GSE_ERR("unknown IOCTL: 0x%08x\n", cmd);
			err = -ENOIOCTLCMD;
			break;
			
	}

	return err;
}
#ifdef CONFIG_COMPAT
static long lsm6ds3_acc_compat_ioctl(struct file *file, unsigned int cmd,
       unsigned long arg)
{
    long err = 0;

	void __user *arg32 = compat_ptr(arg);
	
	if (!file->f_op || !file->f_op->unlocked_ioctl)
		return -ENOTTY;
	
    switch (cmd)
    {
        case COMPAT_GSENSOR_IOCTL_READ_SENSORDATA:
            if (arg32 == NULL)
            {
                err = -EINVAL;
                break;    
            }
		
		    err = file->f_op->unlocked_ioctl(file, GSENSOR_IOCTL_READ_SENSORDATA, (unsigned long)arg32);
		    if (err){
		        GSE_ERR("GSENSOR_IOCTL_READ_SENSORDATA unlocked_ioctl failed.");
		        return err;
		    }
			break;
			
        case COMPAT_GSENSOR_IOCTL_SET_CALI:
            if (arg32 == NULL)
            {
                err = -EINVAL;
                break;    
            }
		
		    err = file->f_op->unlocked_ioctl(file, GSENSOR_IOCTL_SET_CALI, (unsigned long)arg32);
		    if (err){
		        GSE_ERR("GSENSOR_IOCTL_SET_CALI unlocked_ioctl failed.");
		        return err;
		    }
			break;
			
        case COMPAT_GSENSOR_IOCTL_GET_CALI:
            if (arg32 == NULL)
            {
                err = -EINVAL;
                break;    
            }
		
		    err = file->f_op->unlocked_ioctl(file, GSENSOR_IOCTL_GET_CALI, (unsigned long)arg32);
		    if (err){
		        GSE_ERR("GSENSOR_IOCTL_GET_CALI unlocked_ioctl failed.");
		        return err;
		    }
			break;
			
        case COMPAT_GSENSOR_IOCTL_CLR_CALI:
            if (arg32 == NULL)
            {
                err = -EINVAL;
                break;    
            }
		
		    err = file->f_op->unlocked_ioctl(file, GSENSOR_IOCTL_CLR_CALI, (unsigned long)arg32);
		    if (err){
		        GSE_ERR("GSENSOR_IOCTL_CLR_CALI unlocked_ioctl failed.");
		        return err;
		    }
			break;

        default:
            GSE_ERR("unknown IOCTL: 0x%08x\n", cmd);
            err = -ENOIOCTLCMD;
        break;

    }

    return err;
}
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
/* lenovo-sw liaoxl add for reduce power consume of lsm6ds3  20150715 start */
static int lsm6ds3_eint_pause(struct lsm6ds3_i2c_data *obj)
{
	u8 databuf[2] = {0}; 
	int res = 0;

	/* step counter as non-wakup sensor, need to mask data report interrupt */
	if(pedo_enable_status)
	{
		/*disable INT2_STEP_DELTA bit*/
		if(obj->tf->read(obj, LSM6DS3_INT2_CTRL, 1, databuf, true))
		{
			GSE_ERR("%s read LSM6DS3_INT2_CTRL register err!\n", __func__);
			return LSM6DS3_ERR_I2C;
		}
		else
		{
			GSE_LOG("read  LSM6DS3_INT2_CTRL register: 0x%x\n", databuf[0]);
		}
		databuf[0] &= ~0x80;//clear 
		databuf[1] = databuf[0];
		databuf[0] = LSM6DS3_INT2_CTRL;
		//res = i2c_master_send(client, databuf, 0x2);
		res = obj->tf->write(obj, databuf[0], 1, &databuf[1], true);
		if(res < 0)
		{
			GSE_ERR("write LSM6DS3_INT2_CTRL register err!\n");
			return LSM6DS3_ERR_I2C;
		}		
	}
	/* step detector is disable @ uplayer, no need to do mask here */
	/* signification motion is wakeup sensor, remain as before */
	/* tilt detector is wakeup sensor, remain as before */

	return res;
}


static int lsm6ds3_eint_resume(struct lsm6ds3_i2c_data *obj)
{
	u8 databuf[2] = {0}; 
	int res = 0;

	/* step counter as non-wakup sensor, need to unmask data report interrupt */
	if(pedo_enable_status)
	{
		/*disable INT2_STEP_DELTA bit*/
		if(obj->tf->read(obj, LSM6DS3_INT2_CTRL, 1, databuf, true))
		{
			GSE_ERR("%s read LSM6DS3_INT2_CTRL register err!\n", __func__);
			return LSM6DS3_ERR_I2C;
		}
		else
		{
			GSE_LOG("read  LSM6DS3_INT2_CTRL register: 0x%x\n", databuf[0]);
		}
		databuf[0] |= 0x80;
		databuf[1] = databuf[0];
		databuf[0] = LSM6DS3_INT2_CTRL;
		//res = i2c_master_send(client, databuf, 0x2);
		res = obj->tf->write(obj, databuf[0], 1, &databuf[1], true);
		if(res < 0)
		{
			GSE_ERR("write LSM6DS3_INT2_CTRL register err!\n");
			return LSM6DS3_ERR_I2C;
		}		
	}
	/* step detector is disable @ uplayer, no need to do mask here */
	/* signification motion is wakeup sensor, remain as before */
	/* tilt detector is wakeup sensor, remain as before */

	return res;
}
/* lenovo-sw liaoxl add for reduce power consume of lsm6ds3  20150715 end */
#endif

/*----------------------------------------------------------------------------*/
static struct file_operations lsm6ds3_acc_fops = {
	.owner = THIS_MODULE,
	.open = lsm6ds3_open,
	.release = lsm6ds3_release,
	.unlocked_ioctl = lsm6ds3_acc_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = lsm6ds3_acc_compat_ioctl,
#endif
};
/*----------------------------------------------------------------------------*/
static struct miscdevice lsm6ds3_acc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "gsensor",
	.fops = &lsm6ds3_acc_fops,
};
/*----------------------------------------------------------------------------*/
#ifndef CONFIG_HAS_EARLYSUSPEND
/*----------------------------------------------------------------------------*/
static int lsm6ds3_acc_suspend(struct i2c_client *client, pm_message_t msg) 
{
	struct lsm6ds3_i2c_data *obj = i2c_get_clientdata(client);       
	int err = 0;
	GSE_FUN(); 
	
	if(msg.event == PM_EVENT_SUSPEND)
	{   
		if(obj == NULL)
		{
			GSE_ERR("null pointer!!\n");
			msleep(1000);
			return -EINVAL;
		}
		atomic_set(&obj->suspend, 1);

		if(pedo_enable_status  || tilt_enable_status || sigm_enable_status)
		{
			return 0;
		}
		mutex_lock(&lsm6ds3_acc_mutex);
		err = LSM6DS3_acc_SetPowerMode(obj->client, false);
		mutex_unlock(&lsm6ds3_acc_mutex);
		if(err)
		{
			GSE_ERR("write power control fail!!\n");
			return err;
		}
		
		sensor_power = false;
		
		LSM6DS3_power(obj->hw, 0);

	}
	return err;
}
/*----------------------------------------------------------------------------*/
static int lsm6ds3_acc_resume(struct i2c_client *client)
{
	struct lsm6ds3_i2c_data *obj = i2c_get_clientdata(client);        
	int err;
	GSE_FUN();

	if(obj == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return -1;
	}

	if(pedo_enable_status  || tilt_enable_status || sigm_enable_status)
	{
		atomic_set(&obj->suspend, 0);
		return 0;
	}
	LSM6DS3_power(obj->hw, 1);
	err = LSM6DS3_acc_SetPowerMode(obj->client, enable_status);
	if(err)
	{
		GSE_ERR("initialize client fail! err code %d!\n", err);
		return err ;        
	}
	atomic_set(&obj->suspend, 0);  

	return 0;
}
/*----------------------------------------------------------------------------*/
#else /*CONFIG_HAS_EARLY_SUSPEND is defined*/
/*----------------------------------------------------------------------------*/
static void lsm6ds3_early_suspend(struct early_suspend *h) 
{
	struct lsm6ds3_i2c_data *obj = container_of(h, struct lsm6ds3_i2c_data, early_drv);   
	int err;
	GSE_FUN();    

	if(obj == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return;
	}
	atomic_set(&obj->suspend, 1);
	
	if(pedo_enable_status  || tilt_enable_status || sigm_enable_status)
	{
		lsm6ds3_eint_pause(obj);
		return;
	}
	err = LSM6DS3_acc_SetPowerMode(obj->client, false);
	if(err)
	{
		GSE_ERR("write power control fail!!\n");
		return;
	}

	sensor_power = false;
	
	LSM6DS3_power(obj->hw, 0);
}
/*----------------------------------------------------------------------------*/
static void lsm6ds3_late_resume(struct early_suspend *h)
{
	struct lsm6ds3_i2c_data *obj = container_of(h, struct lsm6ds3_i2c_data, early_drv);         
	int err;
	GSE_FUN();

	if(obj == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return;
	}

	if(pedo_enable_status  || tilt_enable_status || sigm_enable_status)
	{
		atomic_set(&obj->suspend, 0);
		lsm6ds3_eint_resume(obj);
		return;
	}

	LSM6DS3_power(obj->hw, 1);
	
	err = LSM6DS3_acc_SetPowerMode(obj->client, enable_status);

	if(err)
	{
		GSE_ERR("initialize client fail! err code %d!\n", err);
		return;        
	}
	atomic_set(&obj->suspend, 0);    
}
#endif /*CONFIG_HAS_EARLYSUSPEND*/


/* fifo operations  */
#ifdef LSM6DS3_FIFO_SUPPORT
static DEFINE_MUTEX(lsm6ds3_fifo_mutex);

static inline s64 lsm6ds3_get_time_ns(void)
{
	struct timespec ts;
	/*
	 * calls getnstimeofday.
	 * If hrtimers then up to ns accurate, if not microsecond.
	 */
	ktime_get_real_ts(&ts);

	return timespec_to_ns(&ts);
}

static void lsm6ds3_push_data_with_timestamp(struct lsm6ds3_sensor_data *sdata,
							u16 offset, s64 timestamp)
{
	u8 *fifo_data_buffer;
	s32 data[3];

	if(obj_i2c_data == NULL)
	{
		return;
	}
	fifo_data_buffer = obj_i2c_data->fifo_data_buffer;

	data[0] = (s32)((s16)(fifo_data_buffer[offset] |
			(fifo_data_buffer[offset + 1] << 8)));
	data[1] = (s32)((s16)(fifo_data_buffer[offset + 2] |
			(fifo_data_buffer[offset + 3] << 8)));
	data[2] = (s32)((s16)(fifo_data_buffer[offset + 4] |
			(fifo_data_buffer[offset + 5] << 8)));

	data[0] *= sdata->c_gain;
	data[1] *= sdata->c_gain;
	data[2] *= sdata->c_gain;

//	lsm6ds3_report_3axes_event(sdata, data, timestamp);
}

static void lsm6ds3_report_single_event(struct lsm6ds3_sensor_data *sdata,
							s32 data, s64 timestamp)
{

}

static void lsm6ds3_parse_fifo_data(struct lsm6ds3_i2c_data *cdata, u16 read_len)
{
	u16 fifo_offset = 0, steps_c = 0;
	u8 gyro_sip, accel_sip, step_c_sip;

	while (fifo_offset < read_len) {
		gyro_sip = cdata->sensors[LSM6DS3_GYRO_IDX].sample_in_pattern;
		accel_sip = cdata->sensors[LSM6DS3_ACCEL_IDX].sample_in_pattern;
		step_c_sip =
			cdata->sensors[LSM6DS3_STEP_COUNTER_IDX].sample_in_pattern;

		do {
			if (gyro_sip > 0) {
				if (cdata->sensors[LSM6DS3_GYRO_IDX].sample_to_discard > 0)
					cdata->sensors[LSM6DS3_GYRO_IDX].sample_to_discard--;
				else
					lsm6ds3_push_data_with_timestamp(
						&cdata->sensors[LSM6DS3_GYRO_IDX],
						fifo_offset,
						cdata->sensors[LSM6DS3_GYRO_IDX].timestamp);

				cdata->sensors[LSM6DS3_GYRO_IDX].timestamp +=
					cdata->sensors[LSM6DS3_GYRO_IDX].deltatime;
				fifo_offset += LSM6DS3_FIFO_ELEMENT_LEN_BYTE;
				gyro_sip--;
			}

			if (accel_sip > 0) {
				if (cdata->sensors[LSM6DS3_ACCEL_IDX].sample_to_discard > 0)
					cdata->sensors[LSM6DS3_ACCEL_IDX].sample_to_discard--;
				else
					lsm6ds3_push_data_with_timestamp(
						&cdata->sensors[LSM6DS3_ACCEL_IDX],
						fifo_offset,
						cdata->sensors[LSM6DS3_ACCEL_IDX].timestamp);

				cdata->sensors[LSM6DS3_ACCEL_IDX].timestamp +=
					cdata->sensors[LSM6DS3_ACCEL_IDX].deltatime;
				fifo_offset += LSM6DS3_FIFO_ELEMENT_LEN_BYTE;
				accel_sip--;
			}

			if (step_c_sip > 0) {
				steps_c = cdata->fifo_data_buffer[fifo_offset + 4] |
					(cdata->fifo_data_buffer[fifo_offset + 5] << 8);
				if (cdata->steps_c != steps_c) {
					lsm6ds3_report_single_event(
						&cdata->sensors[LSM6DS3_STEP_COUNTER_IDX],
						steps_c,
						cdata->sensors[LSM6DS3_STEP_COUNTER_IDX].timestamp);
					cdata->steps_c = steps_c;
				}
				cdata->sensors[LSM6DS3_STEP_COUNTER_IDX].timestamp +=
						cdata->sensors[LSM6DS3_STEP_COUNTER_IDX].deltatime;
				fifo_offset += LSM6DS3_FIFO_ELEMENT_LEN_BYTE;
				step_c_sip--;
			}
		} while ((accel_sip > 0) || (gyro_sip > 0) || (step_c_sip > 0));
	}

	return;
}

void lsm6ds3_read_fifo(struct lsm6ds3_i2c_data *cdata, bool check_fifo_len)
{
	int err;
	u16 read_len = cdata->fifo_threshold * 2;

	if (!cdata->fifo_data_buffer)
		return;

	if (check_fifo_len) {
		err = cdata->tf->read(cdata, LSM6DS3_FIFO_DIFF_L, 2, (u8 *)&read_len,
									true);
		if (err < 0)
		{
			GSE_ERR("read LSM6DS3_FIFO_DIFF_L err=%d\n", err);
			return;
		}

		if (read_len & LSM6DS3_FIFO_DATA_OVR_2REGS) {
			GSE_ERR("data fifo overrun, failed to read it.\n");

			return;
		}

		read_len &= LSM6DS3_FIFO_DIFF_MASK;
		read_len *= LSM6DS3_FIFO_BYTE_FOR_CHANNEL;
		GSE_LOG("get length=%d of data in FIFO vs thd=%d\n", read_len, (cdata->fifo_threshold * 2));

		if (read_len > (cdata->fifo_threshold * 2))
			read_len = cdata->fifo_threshold;
	}

	if (read_len == 0)
		return;

	err = cdata->tf->read(cdata, LSM6DS3_FIFO_DATA_OUT_L, read_len,
						cdata->fifo_data_buffer, true);
	if (err < 0)
	{
		GSE_ERR("read LSM6DS3_FIFO_DATA_OUT_L err=%d\n", err);
		return;
	}

	//lsm6ds3_parse_fifo_data(cdata, read_len);
	{
		u8 *p = cdata->fifo_data_buffer;
		u16 size = read_len;

		while(read_len > 0)
		{
			if(read_len > 6)
			{
				GSE_LOG(" dma read idx[%d] data=%*ph\n", (size - read_len), 6, p);
				read_len -= 6;
				p += 6;
			}
			else
			{
				GSE_LOG(" dma read idx[%d] data=%*ph\n", (size - read_len), read_len, p);
				read_len = 0;
			}
		}
	}
}

static int lsm6ds3_set_fifo_enable(struct lsm6ds3_i2c_data *cdata, bool status)
{
	int err;
	u8 reg_value;

	if (status)
		reg_value = LSM6DS3_FIFO_ODR_MAX;
	else
		reg_value = LSM6DS3_FIFO_ODR_OFF;

	err = lsm6ds3_write_data_with_mask(cdata,
					LSM6DS3_FIFO_ODR_ADDR,
					LSM6DS3_FIFO_ODR_MASK,
					reg_value, true);
	if (err < 0)
		return err;

	cdata->sensors[LSM6DS3_ACCEL_IDX].timestamp = lsm6ds3_get_time_ns();
	cdata->sensors[LSM6DS3_GYRO_IDX].timestamp =
					cdata->sensors[LSM6DS3_ACCEL_IDX].timestamp;
	cdata->sensors[LSM6DS3_STEP_COUNTER_IDX].timestamp =
					cdata->sensors[LSM6DS3_ACCEL_IDX].timestamp;

	return 0;
}

int lsm6ds3_set_fifo_mode(struct lsm6ds3_i2c_data *cdata, enum fifo_mode fm)
{
	int err;
	u8 reg_value;
	bool enable_fifo;

	switch (fm) {
	case BYPASS:
		reg_value = LSM6DS3_FIFO_MODE_BYPASS;
		enable_fifo = false;
		break;
	case CONTINUOS:
		reg_value = LSM6DS3_FIFO_MODE_CONTINUOS;
		enable_fifo = true;
		break;
	default:
		return -EINVAL;
	}

	err = lsm6ds3_set_fifo_enable(cdata, enable_fifo);
	if (err < 0)
		return err;

	return lsm6ds3_write_data_with_mask(cdata, LSM6DS3_FIFO_MODE_ADDR,
				LSM6DS3_FIFO_MODE_MASK, reg_value, true);
}

int lsm6ds3_set_fifo_decimators_and_threshold(struct lsm6ds3_i2c_data *cdata)
{
	int err;
	unsigned int min_odr = 416, max_odr = 0;
	u8 decimator = 0;
	struct lsm6ds3_sensor_data *sdata_accel, *sdata_gyro, *sdata_step_c;
	u16 fifo_len = 0, fifo_threshold;
	u16 min_num_pattern, num_pattern;

	min_num_pattern = LSM6DS3_MAX_FIFO_SIZE / LSM6DS3_FIFO_ELEMENT_LEN_BYTE;
	sdata_accel = &cdata->sensors[LSM6DS3_ACCEL_IDX];
	sdata_gyro = &cdata->sensors[LSM6DS3_GYRO_IDX];
	sdata_step_c = &cdata->sensors[LSM6DS3_STEP_COUNTER_IDX];
	sdata_accel->sample_in_pattern = 0;
	sdata_gyro->sample_in_pattern = 0;
	err = lsm6ds3_write_data_with_mask(cdata,
					LSM6DS3_FIFO_CTRL3_ADDR,
					LSM6DS3_FIFO_ACCEL_DECIMATOR_MASK|LSM6DS3_FIFO_GYRO_DECIMATOR_MASK,
					0, true);
	if (err < 0)
		return err;

	sdata_step_c->sample_in_pattern = 1;  // set 1 for test
	decimator = 0x01;
	err = lsm6ds3_write_data_with_mask(cdata,
					LSM6DS3_FIFO_CTRL4_ADDR,
					LSM6DS3_FIFO_STEP_C_DECIMATOR_MASK,
					decimator, true);
	if (err < 0)
		return err;

	fifo_len = 8 * LSM6DS3_FIFO_ELEMENT_LEN_BYTE; // set 8 for test

	if (fifo_len > 0) {
		fifo_threshold = fifo_len;

		err = cdata->tf->write(cdata,
					LSM6DS3_FIFO_THR_L_ADDR,
					1,
					(u8 *)&fifo_threshold, true);
		if (err < 0)
			return err;

		err = lsm6ds3_write_data_with_mask(cdata,
					LSM6DS3_FIFO_THR_H_ADDR,
					LSM6DS3_FIFO_THR_H_MASK,
					*(((u8 *)&fifo_threshold) + 1), true);
		if (err < 0)
			return err;

		cdata->fifo_threshold = fifo_len;
	}
	if (cdata->fifo_data_buffer) {
		kfree(cdata->fifo_data_buffer);
		cdata->fifo_data_buffer = 0;
	}

	if (fifo_len > 0) {
		cdata->fifo_data_buffer = kzalloc((cdata->fifo_threshold * 2), GFP_KERNEL);
		if (!cdata->fifo_data_buffer)
			return -ENOMEM;
	}

	return fifo_len;
}

int lsm6ds3_reconfigure_fifo(struct lsm6ds3_i2c_data *cdata,
						bool disable_irq_and_flush)
{
	int err, fifo_len;

	if (disable_irq_and_flush) {
//		disable_irq(cdata->irq);
//		lsm6ds3_flush_works();
	}

//	mutex_lock(&cdata->fifo_lock);
	mutex_lock(&lsm6ds3_fifo_mutex);

	lsm6ds3_read_fifo(cdata, true);

	err = lsm6ds3_set_fifo_mode(cdata, BYPASS);
	if (err < 0)
		goto reconfigure_fifo_irq_restore;

	fifo_len = lsm6ds3_set_fifo_decimators_and_threshold(cdata);
	if (fifo_len < 0) {
		err = fifo_len;
		goto reconfigure_fifo_irq_restore;
	}

	if (fifo_len > 0) {
		err = lsm6ds3_set_fifo_mode(cdata, CONTINUOS);
		if (err < 0)
			goto reconfigure_fifo_irq_restore;
	}

reconfigure_fifo_irq_restore:
//	mutex_unlock(&cdata->fifo_lock);
	mutex_unlock(&lsm6ds3_fifo_mutex);

	if (disable_irq_and_flush)
	{
//		enable_irq(cdata->irq);
	}

	return err;
}


static ssize_t get_fifo_length(struct device *dev,
					struct device_attribute *attr, char *buf)
{
	struct lsm6ds3_sensor_data *sdata = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", sdata->fifo_length);
}

static ssize_t set_fifo_length(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t count)
{
	int err;
	unsigned int fifo_length;
	struct lsm6ds3_sensor_data *sdata = dev_get_drvdata(dev);

	err = kstrtoint(buf, 10, &fifo_length);
	if (err < 0)
		return err;

//	mutex_lock(&sdata->input_dev->mutex);
	sdata->fifo_length = fifo_length;
//	mutex_unlock(&sdata->input_dev->mutex);

	lsm6ds3_reconfigure_fifo(obj_i2c_data, true);

	return count;
}

static ssize_t get_hw_fifo_lenght(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", LSM6DS3_MAX_FIFO_LENGHT);
}

static ssize_t flush_fifo(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t size)
{
	//struct lsm6ds3_sensor_data *sdata = dev_get_drvdata(dev);

	//disable_irq(sdata->cdata->irq);
	//lsm6ds3_flush_works();

//	mutex_lock(&obj_i2c_data->fifo_lock);
	mutex_lock(&lsm6ds3_fifo_mutex);
	lsm6ds3_read_fifo(obj_i2c_data, true);
//	mutex_unlock(&obj_i2c_data->fifo_lock);
	mutex_unlock(&lsm6ds3_fifo_mutex);

	//enable_irq(sdata->cdata->irq);

	return size;
}
#endif

#if (defined(LSM6DS3_TILT_FUNC) || defined(LSM6DS3_SIGNIFICANT_MOTION) || defined(LSM6DS3_STEP_COUNTER))
static void lsm6ds3_eint_work(struct work_struct *work)
{
	u8 src_value = 0, src_fifo = 0;	
#ifdef LSM6DS3_SIGNIFICANT_MOTION
	u8 src_wu_value =0;
#endif
	struct lsm6ds3_i2c_data *obj = obj_i2c_data;

	if(obj == NULL)
	{
		GSE_ERR("obj_i2c_data is null pointer!!\n");
		goto lsm6ds3_eint_work_exit;
	}	
	
	//if(hwmsen_read_byte(obj->client, LSM6DS3_FUNC_SRC, &src_value))
	if(obj->tf->read(obj, LSM6DS3_FUNC_SRC, 1, &src_value, true))
	{
		GSE_ERR("%s read LSM6DS3_FUNC_SRC register err!\n", __func__);
		goto lsm6ds3_eint_work_exit;
	}
#ifdef LSM6DS3_FIFO_SUPPORT
	//if(hwmsen_read_byte(obj->client, LSM6DS3_FIFO_STATUS2, &src_fifo))
	if(obj->tf->read(obj, LSM6DS3_FIFO_STATUS2, 1, &src_fifo, true))
	{
		GSE_ERR("%s read LSM6DS3_FIFO_STATUS2 register err!\n", __func__);
		goto lsm6ds3_eint_work_exit;
	}
#endif
	if(atomic_read(&obj->trace) & ACCEL_TRC_DATA)
	{
		GSE_LOG("%s read int src-fifo register: 0x%x src_func=0x%x\n", __func__, src_fifo, src_value);
	}

#ifdef LSM6DS3_STEP_COUNTER
	#ifdef LSM6DS3_FIFO_SUPPORT
		if(LSM6DS3_FIFO_DATA_AVL & src_fifo)
		{
			if(LSM6DS3_FIFO_OVER_RUN & src_fifo)
			{
				/* fifo overrun, reset fifo? */
			}
			else if(LSM6DS3_FIFO_FULL & src_fifo)
			{
				/* fifo full, readback & send to uplayer */
			}
			else
			{
				/* anything to do?? */
				mutex_lock(&lsm6ds3_fifo_mutex);
				lsm6ds3_read_fifo(obj, true);
				mutex_unlock(&lsm6ds3_fifo_mutex);
			}
		}
	#endif

	/* 16bit step count overflow, need to reset */
	if(LSM6DS3_STEP_OVERFLOW & src_value)
	{
		int res;
		u16 steps = 0;

		res = LSM6DS3_Get_Pedo_DataReg(obj->client, &steps);
		if(res == LSM6DS3_SUCCESS)
		{
			if(obj->boot_deb == 1)
			{
				/* state debounce */
				obj->boot_deb = 0;
				GSE_ERR("%s overflow intr first boot time with steps=%d.\n", __func__, steps);
			}
			else if((steps == 0) || (steps == 65535))
			{
				res = LSM6DS3_Reset_Pedo_Data(obj->client, LSM6DS3_ACC_GYRO_PEDO_RST_STEP_ENABLED);
				if(res != LSM6DS3_SUCCESS)
				{
					GSE_ERR(" LSM6DS3_Reset_Pedo_Data failed!\n");
				}
				else
				{
					obj->overflow += 1;
					atomic_set(&obj->reset_sc, 1);
					GSE_LOG("%s overflow intr done steps=%d en=%d.\n", __func__, steps, pedo_enable_status);
				}
			}
		}
	}
	/* at least 1 step recognized in delta time, readback & send to uplayer */
	if(LSM6DS3_STEP_COUNT_DELTA_IA & src_value)
	{
		if(0 == atomic_read(&obj->suspend))
		{
			step_notify(TYPE_STEP_COUNTER);
		}
		obj->boot_deb = 0;
	}


	if((stepd_enable_status == true) && (LSM6DS3_STEP_DETECT_INT_STATUS & src_value))
	{
		//add the action when receive step detection interrupt
		step_notify(TYPE_STEP_DETECTOR);
	}
#endif

#ifdef LSM6DS3_SIGNIFICANT_MOTION
	if(sigm_enable_status == true)
	{
		if(obj->tf->read(obj, LSM6DS3_WAKE_UP_SRC, 1, &src_wu_value, true))
		{
			GSE_ERR("%s read LSM6DS3_CTRL10_C register err!\n", __func__);
			goto lsm6ds3_eint_work_exit;
		}
		
		if((LSM6DS3_SIGNICANT_MOTION_INT_WU_STATUS & src_wu_value)|(LSM6DS3_SIGNICANT_MOTION_INT_STATUS & src_value))
		{
			//add the action when receive the significant motion
			step_notify(TYPE_SIGNIFICANT);
		}
	}
#endif

#ifdef LSM6DS3_TILT_FUNC
	if(LSM6DS3_TILT_INT_STATUS & src_value)
	{
		//add the action when receive the tilt interrupt
		tilt_notify();
	}
#endif

lsm6ds3_eint_work_exit:
	if(0 != atomic_read(&obj->int1_request_num))
	{
/*lenovo-sw caoyi modify for eint API 20151021 begin*/
#if defined(CONFIG_OF)
		enable_irq(obj_i2c_data->irq);
#else
		mt_eint_unmask(CUST_EINT_LSM6DS3_NUM);
#endif
/*lenovo-sw caoyi modify for eint API 20151021 end*/
	}
#ifdef LSM6DS3_FIFO_SUPPORT
	if(0 != atomic_read(&obj->int2_request_num))
	{
/*lenovo-sw caoyi modify for eint API 20151021 begin*/
#if defined(CONFIG_OF)
		enable_irq(obj_i2c_data->irq2);
#else
		mt_eint_unmask(CUST_EINT2_LSM6DS3_NUM);
#endif
/*lenovo-sw caoyi modify for eint API 20151021 end*/
	}
#endif
}
#endif

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static int lsm6ds3_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct i2c_client *new_client;
	struct lsm6ds3_i2c_data *obj;

#ifdef LSM6DS3_NEW_ARCH

#else
	struct hwmsen_object gyro_sobj;
	struct hwmsen_object acc_sobj;
#endif
	int err = 0;

	GSE_FUN();
    
	if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
	{
		err = -ENOMEM;
		goto exit;
	}
	
	memset(obj, 0, sizeof(struct lsm6ds3_i2c_data));

#if(defined(LSM6DS3_TILT_FUNC) || defined(LSM6DS3_SIGNIFICANT_MOTION) || defined(LSM6DS3_STEP_COUNTER))
	INIT_WORK(&obj->eint_work, lsm6ds3_eint_work);
#endif

	obj->hw = get_cust_acc_hw(); 
	obj->sample_rate = LSM6DS3_ACC_ODR_104HZ;
	
	atomic_set(&obj->layout, obj->hw->direction);
	err = hwmsen_get_convert(obj->hw->direction, &obj->cvt);
	if(err)
	{
		GSE_ERR("invalid direction: %d\n", obj->hw->direction);
		goto exit_kfree;
	}

	/* Lenovo-sw, weimh1, 2016-3-30 add begin: mod real addr*/
	client->addr = 0x6A;
	/* Lenovo-sw, weimh1, 2016-3-30 add end: mod real addr*/
	
	obj_i2c_data = obj;
	obj->client = client;
	new_client = obj->client;
	i2c_set_clientdata(new_client,obj);
	
	atomic_set(&obj->trace, 0);
	atomic_set(&obj->suspend, 0);

/* init step counter intterupt vars  --add by liaoxl.lenovo 7.28.2015 start */
	obj->boot_deb = 2;
	obj->overflow = 0;
	obj->steps_c = 0;
	obj->step_c_delay = 3;
	obj->bounce_steps = 0;
	obj->bounce_count = 0;
	obj->last_bounce_step = 0;
	obj->tf = &lsm6ds3_tf_i2c;
	atomic_set(&obj->reset_sc, 0);
	atomic_set(&obj->int1_request_num, 0);
	atomic_set(&obj->int2_request_num, 0);
/* init step counter intterupt vars  --add by liaoxl.lenovo 7.28.2015  end */
/*lenovo-sw caoyi modify for eint API 20151021 begin*/
	obj->irq_node = of_find_compatible_node(NULL, NULL, "mediatek, ACCEL_GYRO_INT1-eint");
	obj->irq_node2 = of_find_compatible_node(NULL, NULL, "mediatek, ACCEL_GYRO_INT2-eint");
/*lenovo-sw caoyi modify for eint API 20151021 end*/

	/* for DMA transfer */
	gpDMABuf_va = (u8 *) dma_alloc_coherent(&client->dev, IIC_DMA_MAX_TRANSFER_SIZE, (dma_addr_t *)&gpDMABuf_pa, GFP_KERNEL);
	if (!gpDMABuf_va) {
		GSE_ERR("Allocate DMA I2C Buffer failed!");
		return -1;
	}
	memset(gpDMABuf_va, 0, IIC_DMA_MAX_TRANSFER_SIZE);

	lsm6ds3_i2c_client = new_client;	
	err = LSM6DS3_init_client(new_client, false);
	if(err)
	{
		goto exit_init_failed;
	}
	
	err = misc_register(&lsm6ds3_acc_device);
	if(err)
	{
		GSE_ERR("lsm6ds3_gyro_device misc register failed!\n");
		goto exit_misc_device_register_failed;
	}
	
#ifdef LSM6DS3_NEW_ARCH

#else
	err = lsm6ds3_create_attr(&lsm6ds3_driver.driver);
#endif
	if(err)
	{
		GSE_ERR("lsm6ds3 create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}	

#ifdef LSM6DS3_NEW_ARCH

#else
	acc_sobj.self = obj;
    acc_sobj.polling = 1;
    acc_sobj.sensor_operate = LSM6DS3_acc_operate;
	err = hwmsen_attach(ID_ACCELEROMETER, &acc_sobj);
	if(err)
	{
		GSE_ERR("hwmsen_attach Accelerometer fail = %d\n", err);
		goto exit_kfree;
	}
#endif	

#ifdef CONFIG_HAS_EARLYSUSPEND
	obj->early_drv.level    = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1,
	obj->early_drv.suspend  = lsm6ds3_early_suspend,
	obj->early_drv.resume   = lsm6ds3_late_resume,    
	register_early_suspend(&obj->early_drv);
#endif 
#ifdef LSM6DS3_NEW_ARCH
	lsm6ds3_acc_init_flag = 0;
#endif

	lsm6ds3_setup_eint();

	GSE_LOG("%s: OK\n", __func__);    
	return 0;

exit_create_attr_failed:
	misc_deregister(&lsm6ds3_acc_device);
exit_misc_device_register_failed:
exit_init_failed:
	/* for DMA transfer */
    if (gpDMABuf_va)
    {
		(void)dma_free_coherent(&client->dev, IIC_DMA_MAX_TRANSFER_SIZE, gpDMABuf_va, gpDMABuf_pa);
		gpDMABuf_va = NULL;
		gpDMABuf_pa = 0;
    }
exit_kfree:
	kfree(obj);
exit:
#ifdef LSM6DS3_NEW_ARCH
	lsm6ds3_acc_init_flag = -1;
#endif
	GSE_ERR("%s: err = %d\n", __func__, err);        
	return err;
}

/*----------------------------------------------------------------------------*/
static int lsm6ds3_i2c_remove(struct i2c_client *client)
{
	int err = 0;	

#ifdef LSM6DS3_NEW_ARCH	
	if(test_bit(CFG_LSM6DS3_ACC, &lsm6ds3_init_flag_test))
	{
		err = lsm6ds3_delete_attr(&(lsm6ds3_init_info.platform_diver_addr->driver));
	}
	lsm6ds3_acc_init_flag = -1;
#else
	err = lsm6ds3_delete_attr(&lsm6ds3_driver.driver);
#endif
	if(err)
	{
		GSE_ERR("lsm6ds3_i2c_remove fail: %d\n", err);
	}

	err = misc_deregister(&lsm6ds3_acc_device);
	if(err)
	{
		GSE_ERR("misc_deregister lsm6ds3_gyro_device fail: %d\n", err);
	}

	lsm6ds3_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));
	/* for DMA transfer */
    if(gpDMABuf_va)
    {
		dma_free_coherent(&client->dev, IIC_DMA_MAX_TRANSFER_SIZE, gpDMABuf_va, gpDMABuf_pa);
		gpDMABuf_va = NULL;
		gpDMABuf_pa = 0;
    }

	return 0;
}
/*----------------------------------------------------------------------------*/
#ifdef LSM6DS3_NEW_ARCH
static int lsm6ds3_local_init_common(void)
{
	struct acc_hw *accel_hw = get_cust_acc_hw();
	
	GSE_FUN();

	LSM6DS3_power(accel_hw, 1);
	
	if(i2c_add_driver(&lsm6ds3_i2c_driver))
	{
		GSE_ERR("add driver error\n");
		return -1;
	}

	return 0;
}
static int lsm6ds3_local_init(void)
{
	int res = 0;
	struct acc_control_path ctl={0};
 	struct acc_data_path data={0};
	struct lsm6ds3_i2c_data *obj = NULL; 

	mutex_lock(&lsm6ds3_init_mutex);

	set_bit(CFG_LSM6DS3_ACC, &lsm6ds3_init_flag_test);

	if((0==test_bit(CFG_LSM6DS3_STEP_C, &lsm6ds3_init_flag_test)) \
		&& (0 == test_bit(CFG_LSM6DS3_TILT, &lsm6ds3_init_flag_test)))
	{
		res = lsm6ds3_local_init_common();
		if(res < 0)
		{
			goto lsm6ds3_local_init_failed;
		}
		
	}


	if(lsm6ds3_acc_init_flag == -1)
	{
		mutex_unlock(&lsm6ds3_init_mutex);
		GSE_ERR("%s init failed!\n", __FUNCTION__);
		return -1;
	}
	else
	{
		obj = obj_i2c_data;
		if(NULL == obj)
		{
			GSE_ERR("i2c_data obj is null!!\n");
			goto lsm6ds3_local_init_failed;
		}
		
		res = lsm6ds3_create_attr(&(lsm6ds3_init_info.platform_diver_addr->driver));
		if(res < 0)
		{
			goto lsm6ds3_local_init_failed;
		}
		ctl.open_report_data= lsm6ds3_open_report_data;
	    ctl.enable_nodata = lsm6ds3_enable_nodata;
	    ctl.set_delay  = lsm6ds3_set_delay;
	    ctl.is_report_input_direct = false;
	    ctl.is_support_batch = obj->hw->is_batch_supported;

	    res = acc_register_control_path(&ctl);
	    if(res)
	    {
	         GSE_ERR("register acc control path err\n");
			 goto lsm6ds3_local_init_failed;

	    }

	    data.get_data = lsm6ds3_get_data;
	    data.vender_div = 1000;
	    res = acc_register_data_path(&data);
	    if(res)
	    {
	        GSE_ERR("register acc data path err= %d\n", res);
			goto lsm6ds3_local_init_failed;

	    }
	}
	mutex_unlock(&lsm6ds3_init_mutex);
	return 0;
lsm6ds3_local_init_failed:
	GSE_ERR("%s init failed\n", __FUNCTION__);
	mutex_unlock(&lsm6ds3_init_mutex);
	return res;

}
static int lsm6ds3_local_uninit(void)
{
	struct acc_hw *accel_hw = get_cust_acc_hw();
	clear_bit(CFG_LSM6DS3_ACC, &lsm6ds3_init_flag_test);

    //GSE_FUN();
    LSM6DS3_power(accel_hw, 0);  	
    i2c_del_driver(&lsm6ds3_i2c_driver);

    return 0;
}

static void lsm6ds3_eint_func(void)
{
	struct lsm6ds3_i2c_data *priv = obj_i2c_data;
	//GSE_FUN();
	if(!priv)
	{
		return;
	}	
	schedule_work(&priv->eint_work);
}

static void lsm6ds3_eint2_func(void)
{
	struct lsm6ds3_i2c_data *priv = obj_i2c_data;
	//GSE_FUN();
	if(!priv)
	{
		return;
	}	
	schedule_work(&priv->eint_work);
}

/*lenovo-sw caoyi modify for eint API 20151021 begin*/
#if defined(CONFIG_OF)
static irqreturn_t lsm6ds3_eint_handler(int irq, void *desc)
{
	disable_irq_nosync(obj_i2c_data->irq);
	lsm6ds3_eint_func();
	
	return IRQ_HANDLED;
}

static irqreturn_t lsm6ds3_eint2_handler(int irq, void *desc)
{
	disable_irq_nosync(obj_i2c_data->irq2);
	lsm6ds3_eint2_func();
	
	return IRQ_HANDLED;
}
#endif
/*lenovo-sw caoyi modify for eint API 20151021 end*/

/*lenovo-sw caoyi modify for eint API 20151021 begin*/
static int lsm6ds3_setup_eint(void)
{
/*Lenovo-sw weimh1 add 2016-3-30 begin:add for M*/
#ifdef LSM6DS3_M
//	struct lsm6ds3_i2c_data *priv = obj_i2c_data;
	int ret;
	u32 ints[2] = { 0, 0 };
	struct pinctrl *pinctrl;
	struct pinctrl_state *pins_default;
	struct pinctrl_state *pins_cfg;
	struct platform_device *accel_pdev = get_accel_platformdev();

	GSE_LOG("lsm6ds3_setup_eint\n");

	/*configure to GPIO function, external interrupt */
	/* gpio setting */
	pinctrl = devm_pinctrl_get(&accel_pdev->dev);
	if (IS_ERR(pinctrl)) {
		ret = PTR_ERR(pinctrl);
		GSE_ERR("Cannot find stepcounter pinctrl!\n");
	}
	
	pins_default = pinctrl_lookup_state(pinctrl, "pin_default");
	if (IS_ERR(pins_default)) {
		ret = PTR_ERR(pins_default);
		GSE_ERR("Cannot find stepcounter pinctrl default!\n");
	}

	pins_cfg = pinctrl_lookup_state(pinctrl, "pin_cfg");
	if (IS_ERR(pins_cfg)) {
		ret = PTR_ERR(pins_cfg);
		GSE_ERR("Cannot find stepcounter pinctrl pin_cfg!\n");
	}
#else
	mt_set_gpio_dir(GPIO_LSM6DS3_EINT_PIN, GPIO_DIR_IN);
	mt_set_gpio_mode(GPIO_LSM6DS3_EINT_PIN, GPIO_LSM6DS3_EINT_PIN_M_EINT);
	mt_set_gpio_pull_enable(GPIO_LSM6DS3_EINT_PIN, true);
	mt_set_gpio_pull_select(GPIO_LSM6DS3_EINT_PIN, GPIO_PULL_UP);

	mt_set_gpio_dir(GPIO_LSM6DS3_EINT2_PIN, GPIO_DIR_IN);
	mt_set_gpio_mode(GPIO_LSM6DS3_EINT2_PIN, GPIO_LSM6DS3_EINT2_PIN_M_EINT);
	mt_set_gpio_pull_enable(GPIO_LSM6DS3_EINT2_PIN, true);
	mt_set_gpio_pull_select(GPIO_LSM6DS3_EINT2_PIN, GPIO_PULL_UP);
#endif
/*Lenovo-sw weimh1 add 2016-3-30 end:add for M*/

#if defined(CONFIG_OF)
	if (obj_i2c_data->irq_node)
	{
		of_property_read_u32_array(obj_i2c_data->irq_node, "debounce", ints, ARRAY_SIZE(ints));
		gpio_request(ints[0], "g-sensor");
		gpio_set_debounce(ints[0], ints[1]);
		GSE_LOG("ints[0] = %d, ints[1] = %d!!\n", ints[0], ints[1]);
		
		pinctrl_select_state(pinctrl, pins_cfg);
		obj_i2c_data->irq = irq_of_parse_and_map(obj_i2c_data->irq_node, 0);
		GSE_LOG("obj_i2c_data->irq = %d\n", obj_i2c_data->irq);
		if (!obj_i2c_data->irq)
		{
			GSE_ERR("irq_of_parse_and_map fail!!\n");
			return -EINVAL;
		}

		if (request_irq(obj_i2c_data->irq, lsm6ds3_eint_handler, IRQF_TRIGGER_LOW, "ACCEL_GYRO_INT1-eint", NULL)) {
			GSE_ERR("IRQ LINE NOT AVAILABLE!!\n");
			return -EINVAL;
		}
/*Lenovo-sw weimh1 mod 2016-4-20 begin: don't need disalble_irq here*/		
//		disable_irq(obj_i2c_data->irq);
/*Lenovo-sw weimh1 mod 2016-4-20 end*/
	}
	else
	{
		GSE_ERR("null irq node!!\n");
		return -EINVAL;
	}

	if (obj_i2c_data->irq_node2)
	{
		of_property_read_u32_array(obj_i2c_data->irq_node2, "debounce", ints, ARRAY_SIZE(ints));
		gpio_request(ints[0], "g-sensor");
		gpio_set_debounce(ints[0], ints[1]);
		GSE_LOG("ints[0] = %d, ints[1] = %d!!\n", ints[0], ints[1]);
		
		pinctrl_select_state(pinctrl, pins_cfg);
		
		obj_i2c_data->irq2 = irq_of_parse_and_map(obj_i2c_data->irq_node2, 0);
		GSE_LOG("obj_i2c_data->irq2 = %d\n", obj_i2c_data->irq2);
		if (!obj_i2c_data->irq2)
		{
			GSE_ERR("irq_of_parse_and_map fail!!\n");
			return -EINVAL;
		}
		
		if (request_irq(obj_i2c_data->irq2, lsm6ds3_eint2_handler, IRQF_TRIGGER_NONE, "ACCEL_GYRO_INT2-eint", NULL)) {
			GSE_ERR("IRQ LINE NOT AVAILABLE!!\n");
			return -EINVAL;
		}
/*Lenovo-sw weimh1 mod 2016-4-20 begin: don't need disalble_irq here*/			
//		disable_irq(obj_i2c_data->irq2);
/*Lenovo-sw weimh1 mod 2016-4-20 end*/	
	}
	else
	{
		GSE_ERR("null irq node!!\n");
		return -EINVAL;
	}
#else
	#ifdef CUST_EINT_LSM6DS3_TYPE
	mt_eint_set_hw_debounce(CUST_EINT_LSM6DS3_NUM, CUST_EINT_LSM6DS3_DEBOUNCE_CN);
	mt_eint_registration(CUST_EINT_LSM6DS3_NUM, CUST_EINT_LSM6DS3_TYPE, lsm6ds3_eint_func, 0);
	mt_eint_mask(CUST_EINT_LSM6DS3_NUM);

	mt_eint_set_hw_debounce(CUST_EINT2_LSM6DS3_NUM, CUST_EINT2_LSM6DS3_DEBOUNCE_CN);
	mt_eint_registration(CUST_EINT2_LSM6DS3_NUM, CUST_EINT2_LSM6DS3_TYPE, lsm6ds3_eint2_func, 0);
	mt_eint_mask(CUST_EINT2_LSM6DS3_NUM);
		
	#endif
#endif

	return 0;
}
/*lenovo-sw caoyi modify for eint API 20151021 end*/

#ifdef LSM6DS3_TILT_FUNC
static int lsm6ds3_tilt_get_data(u16 *value, int *status)
{
	return 0;
}

static int lsm6ds3_tilt_local_init(void)
{
	int res = 0;

	struct tilt_control_path tilt_ctl={0};
	struct tilt_data_path tilt_data={0};

	mutex_lock(&lsm6ds3_init_mutex);
	set_bit(CFG_LSM6DS3_TILT, &lsm6ds3_init_flag_test);
	
	if((0==test_bit(CFG_LSM6DS3_ACC, &lsm6ds3_init_flag_test)) \
		&& (0==test_bit(CFG_LSM6DS3_STEP_C, &lsm6ds3_init_flag_test)))
	{
		res = lsm6ds3_local_init_common();
		if(res < 0)
		{
			goto lsm6ds3_tilt_local_init_failed;
		}
	}

	if(lsm6ds3_acc_init_flag == -1)
	{
		mutex_unlock(&lsm6ds3_init_mutex);
		GSE_ERR("%s init failed!\n", __FUNCTION__);
		return -1;
	}
	else
	{
		tilt_ctl.open_report_data= lsm6ds3_tilt_open_report_data;	
		res = tilt_register_control_path(&tilt_ctl);

		tilt_data.get_data = lsm6ds3_tilt_get_data;
		res = tilt_register_data_path(&tilt_data);
	}
	mutex_unlock(&lsm6ds3_init_mutex);
	return 0;
	
lsm6ds3_tilt_local_init_failed:
	mutex_unlock(&lsm6ds3_init_mutex);
	GSE_ERR("%s init failed!\n", __FUNCTION__);
	return -1;
}

static int lsm6ds3_tilt_local_uninit(void)
{
	clear_bit(CFG_LSM6DS3_TILT, &lsm6ds3_init_flag_test);
    return 0;
}
#endif

#ifdef LSM6DS3_STEP_COUNTER
static int lsm6ds3_step_c_local_init(void)
{
	int res = 0;

	struct step_c_control_path step_ctl={0};
	struct step_c_data_path step_data={0};	
	
	mutex_lock(&lsm6ds3_init_mutex);
		
	set_bit(CFG_LSM6DS3_STEP_C, &lsm6ds3_init_flag_test);
	
	if((0==test_bit(CFG_LSM6DS3_ACC, &lsm6ds3_init_flag_test)) \
		&& (0 == test_bit(CFG_LSM6DS3_TILT, &lsm6ds3_init_flag_test)))
	{
		res = lsm6ds3_local_init_common();
		if(res < 0)
		{
			goto lsm6ds3_step_c_local_init_failed;
		}
			
	}

	if(lsm6ds3_acc_init_flag == -1)
	{
		mutex_unlock(&lsm6ds3_init_mutex);
		GSE_ERR("%s init failed!\n", __FUNCTION__);
		return -1;
	}
	else
	{
		step_ctl.open_report_data= lsm6ds3_step_c_open_report_data;
		step_ctl.enable_nodata = lsm6ds3_step_c_enable_nodata;
		step_ctl.enable_step_detect  = lsm6ds3_step_c_enable_step_detect;
		step_ctl.set_delay = lsm6ds3_step_c_set_delay;
		step_ctl.is_report_input_direct = false;
		step_ctl.is_support_batch = false;		
#ifdef LSM6DS3_SIGNIFICANT_MOTION
		step_ctl.enable_significant = lsm6ds3_step_c_enable_significant;
#endif

		res = step_c_register_control_path(&step_ctl);
		if(res)
		{
			 GSE_ERR("register step counter control path err\n");
			goto lsm6ds3_step_c_local_init_failed;
		}
	
		step_data.get_data = lsm6ds3_step_c_get_data;
		step_data.get_data_step_d = lsm6ds3_step_c_get_data_step_d;
		step_data.get_data_significant = lsm6ds3_step_c_get_data_significant;
		
		step_data.vender_div = 1;
		res = step_c_register_data_path(&step_data);
		if(res)
		{
			GSE_ERR("register step counter data path err= %d\n", res);
			goto lsm6ds3_step_c_local_init_failed;
		}
	}
	mutex_unlock(&lsm6ds3_init_mutex);
	return 0;
	
lsm6ds3_step_c_local_init_failed:
	mutex_unlock(&lsm6ds3_init_mutex);
	GSE_ERR("%s init failed!\n", __FUNCTION__);
	return res;

}
static int lsm6ds3_step_c_local_uninit(void)
{
	clear_bit(CFG_LSM6DS3_STEP_C, &lsm6ds3_init_flag_test);
    return 0;
}
#endif
#else
static int lsm6ds3_probe(struct platform_device *pdev) 
{
	struct acc_hw *accel_hw = get_cust_acc_hw();
	GSE_FUN();

	LSM6DS3_power(accel_hw, 1);
	
	if(i2c_add_driver(&lsm6ds3_i2c_driver))
	{
		GSE_ERR("add driver error\n");
		return -1;
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
static int lsm6ds3_remove(struct platform_device *pdev)
{
	struct acc_hw *accel_hw = get_cust_acc_hw();

    //GSE_FUN();    
    LSM6DS3_power(accel_hw, 0);  	
    i2c_del_driver(&lsm6ds3_i2c_driver);
    return 0;
}

/*----------------------------------------------------------------------------*/
#ifdef CONFIG_OF
static const struct of_device_id gsensor_of_match[] = {
	{ .compatible = "mediatek,gsensor", },
	{},
};
#endif

static struct platform_driver lsm6ds3_driver = {
	.probe      = lsm6ds3_probe,
	.remove     = lsm6ds3_remove,    
	.driver     = {
			.name  = "gsensor",
		//	.owner	= THIS_MODULE,
	#ifdef CONFIG_OF
			.of_match_table = gsensor_of_match,
	#endif
	}
};

#endif
/*----------------------------------------------------------------------------*/
static int __init lsm6ds3_init(void)
{
	//GSE_FUN();
#ifdef LSM6DS3_M
	const char *name = "mediatek,lsm6ds3_acc";

	hw = get_accel_dts_func(name, hw);
	if (!hw)
		GSE_ERR("get dts info fail\n");
#else
	struct acc_hw *hw = get_cust_acc_hw();
    GSE_LOG("%s: i2c_number=%d\n", __func__,hw->i2c_num);
    i2c_register_board_info(hw->i2c_num, &i2c_lsm6ds3, 1);
#endif

#ifdef LSM6DS3_NEW_ARCH
	acc_driver_add(&lsm6ds3_init_info);	
#ifdef LSM6DS3_STEP_COUNTER //step counter
	step_c_driver_add(&lsm6ds3_step_c_init_info); //step counter
#endif
#ifdef LSM6DS3_TILT_FUNC
	tilt_driver_add(&lsm6ds3_tilt_init_info);
#endif

#else
	if(platform_driver_register(&lsm6ds3_driver))
	{
		GSE_ERR("failed to register driver");
		return -ENODEV;
	}
#endif
    
	return 0;    
}
/*----------------------------------------------------------------------------*/
static void __exit lsm6ds3_exit(void)
{
	GSE_FUN();
#ifndef LSM6DS3_NEW_ARCH	
	platform_driver_unregister(&lsm6ds3_driver);
#endif

}
/*----------------------------------------------------------------------------*/
module_init(lsm6ds3_init);
module_exit(lsm6ds3_exit);
/*----------------------------------------------------------------------------*/
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("LSM6DS3 Accelerometer");
MODULE_AUTHOR("xj.wang@mediatek.com, darren.han@st.com");






/*----------------------------------------------------------------- LSM6DS3 ------------------------------------------------------------------*/
