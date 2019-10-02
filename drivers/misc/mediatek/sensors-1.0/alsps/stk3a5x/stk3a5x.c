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

  /* drivers/hwmon/mt6516/amit/stk3a5x.c - stk3a5x ALS/PS driver
   *
   * Author: MingHsien Hsieh <minghsien.hsieh@mediatek.com>
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
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
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
#include <asm/atomic.h>
#include <linux/version.h>
#include <linux/fs.h>
//#include <linux/wakelock.h>
#include <asm/io.h>
#include <linux/module.h>
   //#include <linux/hwmsen_helper.h>
   //#include "cust_eint.h"
//#include <linux/hwmsensor.h>
//#include <linux/sensors_io.h>
//#include <linux/hwmsen_dev.h>
//#include <stk_cust_alsps.h>
#include "cust_alsps.h"
#include "alsps.h"
#include "stk3a5x.h"

#define STK3A5X_DRIVER_VERSION          "v3 20190912 for android9.x"

#define STK_PS_POLLING_LOG
#define STK_PS_DEBUG
#define STK_TUNE0
#define CALI_EVERY_TIME
#define CTTRACKING
#define STK_ALS_THRESHOLD	30
#define MTK_AUTO_DETECT_ALSPS


#define STK_ALS_MID_FIR

#ifdef STK_ALS_MID_FIR
#define STK_ALS_MID_FIR_LEN   			 5
#define MAX_ALS_FIR_LEN 				 32
typedef struct
{
	uint16_t raw[STK_ALS_MID_FIR_LEN];
	uint32_t number;
	uint32_t index;
}stk3a5x_data_filter;
#endif

//extern struct alsps_hw* get_cust_alsps_hw(void);
extern struct platform_device *get_alsps_platformdev(void);

/******************************************************************************
 * configuration
*******************************************************************************/

/*----------------------------------------------------------------------------*/
#define stk3a5x_DEV_NAME     "stk3a5x"
/*----------------------------------------------------------------------------*/
#define APS_TAG                  "[ALS/PS] "
#define APS_FUN(f)               pr_err(APS_TAG"%s\n", __func__)
#define APS_ERR(fmt, args...)    pr_err(APS_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#define APS_LOG(fmt, args...)    pr_err(APS_TAG fmt, ##args)
#define APS_DBG(fmt, args...)    pr_err(APS_TAG fmt, ##args)
/******************************************************************************
 * extern functions
*******************************************************************************/
#ifdef STK_TUNE0
#define STK_MAX_MIN_DIFF		80 
#define STK_LT_N_CT				105 
#define STK_HT_N_CT				120 
#endif /* #ifdef STK_TUNE0 */

#define STK_H_PS				500
#define STK_H_HT				300
#define STK_H_LT				200

#define STK_ALS_THRESHOLD				30

/*----------------------------------------------------------------------------*/
static struct i2c_client *stk3a5x_i2c_client = NULL;
/*----------------------------------------------------------------------------*/
static const struct i2c_device_id stk3a5x_i2c_id[] = { {stk3a5x_DEV_NAME, 0}, {} };
//static struct regulator *stk3a5x_regulator;
/*----------------------------------------------------------------------------*/
static int stk3a5x_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int stk3a5x_i2c_remove(struct i2c_client *client);
static int stk3a5x_i2c_detect(struct i2c_client *client, struct i2c_board_info *info);
/*----------------------------------------------------------------------------*/
static int stk3a5x_i2c_suspend(struct device *dev);
static int stk3a5x_i2c_resume(struct device *dev);
//static struct stk3a5x_priv *g_stk3a5x_ptr = NULL;

#ifndef C_I2C_FIFO_SIZE
#define C_I2C_FIFO_SIZE     10
#endif
static DEFINE_MUTEX(STK3A5X_i2c_mutex);
static int	stk3a5x_init_flag = -1;	// 0<==>OK -1 <==> fail
static int	stk3a5x_calibration_flag = 0;
static int  stk3a5x_local_init(void);
static int  stk3a5x_local_uninit(void);
// static struct sensor_init_info stk3a5x_init_info = {
static struct alsps_init_info stk3a5x_init_info =
{
	.name = "stk3a5x",
	.init = stk3a5x_local_init,
	.uninit = stk3a5x_local_uninit,
};
#define ALS_LEVEL_TP_DEFAULT 	(0)
#define ALS_LEVEL_TEMP_DEFAULT 	(0)
static struct alsps_hw alsps_cust;
static struct alsps_hw *hw = &alsps_cust;
//struct platform_device *alspsPltFmDev;
/* For alsp driver get cust info */
//struct alsps_hw *get_cust_alsps(void)
//{
//	return &alsps_cust;
//}
/*----------------------------------------------------------------------------*/
typedef enum
{
	STK_TRC_ALS_DATA = 0x0001,
	STK_TRC_PS_DATA = 0x0002,
	STK_TRC_EINT = 0x0004,
	STK_TRC_IOCTL = 0x0008,
	STK_TRC_I2C = 0x0010,
	STK_TRC_CVT_ALS = 0x0020,
	STK_TRC_CVT_PS = 0x0040,
	STK_TRC_DEBUG = 0x8000,
} STK_TRC;
/*----------------------------------------------------------------------------*/
typedef enum
{
	STK_BIT_ALS = 1,
	STK_BIT_PS = 2,
} STK_BIT;
/*----------------------------------------------------------------------------*/
struct stk3a5x_i2c_addr
{
	/*define a series of i2c slave address*/
	u8  state;      	/* enable/disable state */
	u8  psctrl;     	/* PS control */
	u8  alsctrl;    	/* ALS control */
	u8  ledctrl;   		/* LED control */
	u8  intmode;    	/* INT mode */
	u8  wait;     		/* wait time */
	u8  thdh1_ps;   	/* PS INT threshold high 1 */
	u8	thdh2_ps;		/* PS INT threshold high 2 */
	u8  thdl1_ps;   	/* PS INT threshold low 1 */
	u8  thdl2_ps;   	/* PS INT threshold low 2 */
	u8  thdh1_als;   	/* ALS INT threshold high 1 */
	u8	thdh2_als;		/* ALS INT threshold high 2 */
	u8  thdl1_als;   	/* ALS INT threshold low 1 */
	u8  thdl2_als;   	/* ALS INT threshold low 2 */
	u8  flag;			/* int flag */
	u8  data1_ps;		/* ps data1 */
	u8  data2_ps;		/* ps data2 */
	u8  data1_als;		/* als data1 */
	u8  data2_als;		/* als data2 */
	u8  data1_offset;	/* offset data1 */
	u8  data2_offset;	/* offset data2 */
	u8  data1_ir;		/* ir data1 */
	u8  data2_ir;		/* ir data2 */
	u8  soft_reset;		/* software reset */
};
/*----------------------------------------------------------------------------*/
struct stk3a5x_priv
{
	struct alsps_hw  *hw;
	struct i2c_client *client;
	struct delayed_work  eint_work;

	/*i2c address group*/
	struct stk3a5x_i2c_addr  addr;

	/*misc*/
	atomic_t    trace;
	atomic_t    i2c_retry;
	atomic_t    als_suspend;
	atomic_t    als_debounce;   /*debounce time after enabling als*/
	atomic_t    als_deb_on;     /*indicates if the debounce is on*/
	atomic_t    als_deb_end;    /*the jiffies representing the end of debounce*/
	atomic_t    ps_mask;        /*mask ps: always return far away*/
	atomic_t    ps_debounce;    /*debounce time after enabling ps*/
	atomic_t    ps_deb_on;      /*indicates if the debounce is on*/
	atomic_t    ps_deb_end;     /*the jiffies representing the end of debounce*/
	atomic_t    ps_suspend;
	atomic_t    init_done;
	struct      device_node *irq_node;
	int         irq;

	/*data*/
	u16         als;
	u16         ps;
	u8          _align;
	u16         als_level_num;
	u16         als_value_num;
	//u32         als_level[C_CUST_ALS_LEVEL - 1];
	//u32         als_value[C_CUST_ALS_LEVEL];
	u32 		als_level[TP_COUNT][TEMP_COUNT][C_CUST_ALS_LEVEL];
	u32			als_value[C_CUST_ALS_LEVEL];

	u8 			wait_val;
	u8		 	int_val;
	u8		 	ledctrl_val;
	u8		 	intell_val;
	atomic_t	state_val;
	atomic_t 	psctrl_val;
	atomic_t 	alsctrl_val;
	atomic_t 	alsctrl2_val;

	atomic_t    ps_high_thd_val;     /*the cmd value can't be read, stored in ram*/
	atomic_t    ps_low_thd_val;     /*the cmd value can't be read, stored in ram*/
	ulong       enable;         /*enable mask*/
	ulong       pending_intr;   /*pending interrupt*/
	atomic_t	recv_reg;
	/*early suspend*/
#if defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend    early_drv;
#endif
	bool first_boot;
	struct hrtimer ps_tune0_timer;
	struct workqueue_struct *stk_ps_tune0_wq;
	struct work_struct stk_ps_tune0_work;
#ifdef STK_TUNE0
	uint16_t psa;
	uint16_t psi;
	uint16_t psi_set;
	uint16_t stk_max_min_diff;
	uint16_t stk_lt_n_ct;
	uint16_t stk_ht_n_ct;
	uint16_t crosstalk;
#ifdef CALI_EVERY_TIME
	uint16_t ps_high_thd_boot;
	uint16_t ps_low_thd_boot;
#endif
	uint16_t boot_cali;
#ifdef CTTRACKING
	bool ps_thd_update;
#endif

	ktime_t ps_tune0_delay;
	bool tune_zero_init_proc;
	uint32_t ps_stat_data[3];
	int data_count;
#endif

#ifdef STK_ALS_MID_FIR
	stk3a5x_data_filter     als_data_filter;
#endif // STK_ALS_MID_FIR

	uint16_t ir_code;
	uint16_t als_correct_factor;
	u16 als_last;
	bool re_enable_ps;
	bool re_enable_als;
	u16 ps_cali;

	uint8_t pid;
	uint32_t als_code_last;
	uint16_t als_data_index;
	uint8_t ps_distance_last;

	uint32_t als_debug_count;
};
/*----------------------------------------------------------------------------*/
#ifdef CONFIG_OF
static const struct of_device_id alsps_of_match[] =
{
	{.compatible = "mediatek,alsps"},
	{.compatible = "sensortek,stk3a5x"},
	{},
};
#endif

#ifdef CONFIG_PM_SLEEP
static const struct dev_pm_ops stk3a5x_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(stk3a5x_i2c_suspend, stk3a5x_i2c_resume)
};
#endif

static struct i2c_driver stk3a5x_i2c_driver =
{
	.probe = stk3a5x_i2c_probe,
	.remove = stk3a5x_i2c_remove,
	.detect = stk3a5x_i2c_detect,
	//.suspend = stk3a5x_i2c_suspend,
	//.resume = stk3a5x_i2c_resume,
	.id_table = stk3a5x_i2c_id,
	.driver = {
		.name = stk3a5x_DEV_NAME,
#ifdef CONFIG_PM_SLEEP
		.pm   = &stk3a5x_pm_ops,
#endif
#ifdef CONFIG_OF
		.of_match_table = alsps_of_match,
#endif
	},
};

static struct stk3a5x_priv *stk3a5x_obj = NULL;
static int stk3a5x_get_ps_value(struct stk3a5x_priv *obj, u16 ps);
static int stk3a5x_get_ps_value_only(struct stk3a5x_priv *obj, u16 ps);
static int stk3a5x_get_als_value(struct stk3a5x_priv *obj, u16 als);
static int stk3a5x_read_als(struct i2c_client *client, u16 *data);
static int stk3a5x_read_ps(struct i2c_client *client, u16 *data);
//static int stk3a5x_set_als_int_thd(struct i2c_client *client, u16 als_data_reg);
//static int32_t stk3a5x_get_ir_value(struct stk3a5x_priv *obj, int32_t als_it_reduce);
#ifdef STK_ALS_MID_FIR
static void stk3a5x_als_bubble_sort(uint16_t* sort_array, uint8_t size_n);
#endif // STK_ALS_MID_FIR
#ifdef STK_TUNE0
static int stk_ps_tune_zero_func_fae(struct stk3a5x_priv *obj);
#endif
static int stk3a5x_init_client(struct i2c_client *client);
//struct wake_lock mps_lock;
static DEFINE_MUTEX(run_cali_mutex);

struct pinctrl *pinctrl;
struct pinctrl_state *pins_v3venable;
struct pinctrl_state *pins_v3vdisable;

static unsigned int current_tp = 0;
static unsigned int current_color_temp=CWF_TEMP;
//static unsigned int current_color_temp_first = 0;
static unsigned int als_level[TP_COUNT][TEMP_COUNT][C_CUST_ALS_LEVEL];
static unsigned int als_value[C_CUST_ALS_LEVEL] = {0};


/*----------------------------------------------------------------------------*/
int stk3a5x_get_addr(struct stk3a5x_i2c_addr *addr)
{
	if (!addr)
	{
		return -EFAULT;
	}

	addr->state = STK_STATE_REG;
	addr->psctrl = STK_PSCTRL_REG;
	addr->alsctrl = STK_ALSCTRL_REG;
	addr->ledctrl = STK_LEDCTRL_REG;
	addr->intmode = STK_INT_REG;
	addr->wait = STK_WAIT_REG;
	addr->thdh1_ps = STK_THDH1_PS_REG;
	addr->thdh2_ps = STK_THDH2_PS_REG;
	addr->thdl1_ps = STK_THDL1_PS_REG;
	addr->thdl2_ps = STK_THDL2_PS_REG;
	addr->thdh1_als = STK_THDH1_ALS_REG;
	addr->thdh2_als = STK_THDH2_ALS_REG;
	addr->thdl1_als = STK_THDL1_ALS_REG;
	addr->thdl2_als = STK_THDL2_ALS_REG;
	addr->flag = STK_FLAG_REG;
	addr->data1_ps = STK_DATA1_PS_REG;
	addr->data2_ps = STK_DATA2_PS_REG;
	addr->data1_als = STK_DATA1_ALS_REG;
	addr->data2_als = STK_DATA2_ALS_REG;
	addr->data1_offset = STK_DATA1_OFFSET_REG;
	addr->data2_offset = STK_DATA2_OFFSET_REG;
	//addr->data1_ir = STK_DATA1_IR_REG;
	//addr->data2_ir = STK_DATA2_IR_REG;
	addr->soft_reset = STK_SW_RESET_REG;
	return 0;
}
/*----------------------------------------------------------------------------*/
int stk3a5x_hwmsen_read_block(struct i2c_client *client, u8 addr, u8 *data, u8 len)
{
	int err;
	u8 beg = addr;
	struct i2c_msg msgs[2] =
	{
		{
			.addr = client->addr,
			.flags = 0,
			.len = 1,
			.buf = &beg
		},
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = len,
			.buf = data,
		}
	};
	mutex_lock(&STK3A5X_i2c_mutex);

	if (!client)
	{
		mutex_unlock(&STK3A5X_i2c_mutex);
		return -EINVAL;
	}
	else if (len > C_I2C_FIFO_SIZE)
	{
		mutex_unlock(&STK3A5X_i2c_mutex);
		APS_LOG(" length %d exceeds %d\n", len, C_I2C_FIFO_SIZE);
		return -EINVAL;
	}

	err = i2c_transfer(client->adapter, msgs, sizeof(msgs) / sizeof(msgs[0]));
	mutex_unlock(&STK3A5X_i2c_mutex);

	if (err != 2)
	{
		APS_LOG("i2c_transfer error: (%d %p %d) %d\n", addr, data, len, err);
		err = -EIO;
	}
	else
	{
		err = 0;/*no error*/
	}

	return err;
}
/*----------------------------------------------------------------------------*/
int stk3a5x_get_timing(void)
{
	return 200;
}

/*----------------------------------------------------------------------------*/
int stk3a5x_master_recv(struct i2c_client *client, u16 addr, u8 *buf, int count)
{
	struct stk3a5x_priv *obj = i2c_get_clientdata(client);
	int ret = 0, retry = 0;
	int trc = atomic_read(&obj->trace);
	int max_try = atomic_read(&obj->i2c_retry);

	while (retry++ < max_try)
	{
		ret = stk3a5x_hwmsen_read_block(client, addr, buf, count);

		if (ret == 0)
			break;

		udelay(100);
	}

	if (unlikely(trc))
	{
		if ((retry != 1) && (trc & STK_TRC_DEBUG))
		{
			APS_LOG("(recv) %d/%d\n", retry - 1, max_try);
		}
	}

	/* If everything went ok (i.e. 1 msg transmitted), return #bytes
	transmitted, else error code. */
	return (ret == 0) ? count : ret;
}
/*----------------------------------------------------------------------------*/
int stk3a5x_master_send(struct i2c_client *client, u16 addr, u8 *buf, int count)
{
	int ret = 0, retry = 0;
	struct stk3a5x_priv *obj = i2c_get_clientdata(client);
	int trc = atomic_read(&obj->trace);
	int max_try = atomic_read(&obj->i2c_retry);

	while (retry++ < max_try)
	{
		ret = hwmsen_write_block(client, addr, buf, count);

		if (ret == 0)
			break;

		udelay(100);
	}

	if (unlikely(trc))
	{
		if ((retry != 1) && (trc & STK_TRC_DEBUG))
		{
			APS_LOG("(send) %d/%d\n", retry - 1, max_try);
		}
	}

	/* If everything went ok (i.e. 1 msg transmitted), return #bytes
	transmitted, else error code. */
	return (ret == 0) ? count : ret;
}
/*----------------------------------------------------------------------------*/
#if 0
static int stk3a5x_otp_read_byte_data(struct i2c_client *client, unsigned char command)
{
	int ret;
	int value;
	u8 data;
	data = 0x2;
	ret = stk3a5x_master_send(client, 0x0, &data, 1);

	if (ret < 0)
	{
		APS_ERR("write 0x0 = %d\n", ret);
		return -EFAULT;
	}

	data = command;
	ret = stk3a5x_master_send(client, 0x90, &data, 1);

	if (ret < 0)
	{
		APS_ERR("write 0x90 = %d\n", ret);
		return -EFAULT;
	}

	data = 0x82;
	ret = stk3a5x_master_send(client, 0x92, &data, 1);

	if (ret < 0)
	{
		APS_ERR("write 0x0 = %d\n", ret);
		return -EFAULT;
	}

	usleep_range(2000, 4000);
	ret = stk3a5x_master_recv(client, 0x91, &data, 1);

	if (ret < 0)
	{
		APS_DBG("error: %d\n", ret);
		return -EFAULT;
	}

	value = data;
	APS_LOG("%s: 0x%x=0x%x\n", __func__, command, value);
	data = 0x0;
	ret = stk3a5x_master_send(client, 0x0, &data, 1);

	if (ret < 0)
	{
		APS_ERR("write 0x0 = %d\n", ret);
		return -EFAULT;
	}

	return value;
}
#endif
/*----------------------------------------------------------------------------*/
int stk3a5x_write_led(struct i2c_client *client, u8 data)
{
	struct stk3a5x_priv *obj = i2c_get_clientdata(client);
	int ret = 0;
	ret = stk3a5x_master_send(client, obj->addr.ledctrl, &data, 1);

	if (ret < 0)
	{
		APS_ERR("write led = %d\n", ret);
		return -EFAULT;
	}

	return 0;
}

/*----------------------------------------------------------------------------*/
int stk3a5x_read_als(struct i2c_client *client, u16 *data)
{
	struct stk3a5x_priv *obj = i2c_get_clientdata(client);
	int ret = 0;
	u8 buf[2];
	u32 als_data;

	if (NULL == client)
	{
		return -EINVAL;
	}

	ret = stk3a5x_master_recv(client, obj->addr.data1_als, buf, 0x2);
	if (ret < 0)
	{
		APS_DBG("error: %d\n", ret);
		return -EFAULT;
	}

	als_data = (buf[0] << 8) | (buf[1]);

	obj->als_code_last = als_data;
#ifdef STK_ALS_MID_FIR
	if (obj->als_data_filter.number < STK_ALS_MID_FIR_LEN)
	{
		obj->als_data_filter.raw[obj->als_data_filter.number] = als_data;
		obj->als_data_filter.number++;
		obj->als_data_filter.index++;
	}
	else
	{
		uint8_t index;
		uint16_t mid_als;
		uint16_t cpraw[STK_ALS_MID_FIR_LEN] = { 0 };
		index = obj->als_data_filter.index % obj->als_data_filter.number;
		obj->als_data_filter.raw[index] = als_data;
		obj->als_data_filter.index++;

		memcpy(cpraw, obj->als_data_filter.raw, sizeof(obj->als_data_filter.raw));

		stk3a5x_als_bubble_sort(cpraw, sizeof(cpraw) / sizeof(cpraw[0]));
		mid_als = cpraw[STK_ALS_MID_FIR_LEN / 2];
		als_data = mid_als;
	}

#endif
	obj->als_code_last = als_data;
	*data = (u16)als_data;
	if (atomic_read(&obj->trace) & STK_TRC_ALS_DATA)
	{
		APS_DBG("ALS: 0x%04X\n", (u32)(*data));
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
int stk3a5x_write_als(struct i2c_client *client, u8 data)
{
	struct stk3a5x_priv *obj = i2c_get_clientdata(client);
	int ret = 0;
	ret = stk3a5x_master_send(client, obj->addr.alsctrl, &data, 1);

	if (ret < 0)
	{
		APS_ERR("write als = %d\n", ret);
		return -EFAULT;
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
int stk3a5x_read_state(struct i2c_client *client, u8 *data)
{
	//struct stk3a5x_priv *obj = i2c_get_clientdata(client);
	int ret = 0;
	u8 buf;

	if (NULL == client)
	{
		return -EINVAL;
	}

	ret = stk3a5x_master_recv(client, STK_STATE_REG, &buf, 0x01);

	if (ret < 0)
	{
		APS_DBG("error: %d\n", ret);
		return -EFAULT;
	}
	else
	{
		*data = buf;
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
int stk3a5x_read_flag(struct i2c_client *client, u8 *data)
{
	struct stk3a5x_priv *obj = i2c_get_clientdata(client);
	int ret = 0;
	u8 buf;

	if (NULL == client)
	{
		return -EINVAL;
	}

	ret = stk3a5x_master_recv(client, obj->addr.flag, &buf, 0x01);

	if (ret < 0)
	{
		APS_DBG("error: %d\n", ret);
		return -EFAULT;
	}
	else
	{
		*data = buf;
	}

	if (atomic_read(&obj->trace) & STK_TRC_ALS_DATA)
	{
		APS_DBG("PS NF flag: 0x%04X\n", (u32)(*data));
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
int stk3a5x_read_id(struct i2c_client *client)
{
	struct stk3a5x_priv *obj = i2c_get_clientdata(client);
	int ret = 0;
	u8 buf[2];

	if (NULL == client)
	{
		return -EINVAL;
	}

	ret = stk3a5x_master_recv(client, STK_PDT_ID_REG, buf, 0x02);

	if (ret < 0)
	{
		APS_DBG("error: %d\n", ret);
		return -EFAULT;
	}

	obj->pid = buf[0];
	APS_LOG("%s: PID=0x%x, VID=0x%x\n", __func__, buf[0], buf[1]);
	return 0;
}

int stk3a5x_get_ps_high_thd(struct i2c_client *client, u16 *data)
{
    struct stk3a5x_priv *obj = i2c_get_clientdata(client);
    int ret = 0;
    u8 buf[2];

    if (NULL == client)
    {
        APS_ERR("i2c client is NULL\n");
        return -EINVAL;
    }

    ret = stk3a5x_master_recv(client, obj->addr.thdh1_ps, buf, 0x02);

    if (ret < 0)
    {
        APS_DBG("error: %d\n", ret);
        return -EFAULT;
    }
    else
    {
        *data = (buf[0] << 8) | (buf[1]);
    }

    if (atomic_read(&obj->trace) & STK_TRC_ALS_DATA)
    {
        APS_DBG("PS: 0x%04X\n", (u32)(*data));
    }

    return 0;
}

int stk3a5x_get_ps_low_thd(struct i2c_client *client, u16 *data)
{
    struct stk3a5x_priv *obj = i2c_get_clientdata(client);
    int ret = 0;
    u8 buf[2];

    if (NULL == client)
    {
        APS_ERR("i2c client is NULL\n");
        return -EINVAL;
    }

    ret = stk3a5x_master_recv(client, obj->addr.thdl1_ps, buf, 0x02);

    if (ret < 0)
    {
        APS_DBG("error: %d\n", ret);
        return -EFAULT;
    }
    else
    {
        *data = (buf[0] << 8) | (buf[1]);
    }

    if (atomic_read(&obj->trace) & STK_TRC_ALS_DATA)
    {
        APS_DBG("PS: 0x%04X\n", (u32)(*data));
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
int stk3a5x_read_ps(struct i2c_client *client, u16 *data)
{
	struct stk3a5x_priv *obj = i2c_get_clientdata(client);
	int ret = 0;
	u8 buf[2];

	if (NULL == client)
	{
		APS_ERR("i2c client is NULL\n");
		return -EINVAL;
	}

	ret = stk3a5x_master_recv(client, obj->addr.data1_ps, buf, 0x02);

	if (ret < 0)
	{
		APS_DBG("error: %d\n", ret);
		return -EFAULT;
	}
	else
	{
		*data = (buf[0] << 8) | (buf[1]);
	}

	if (atomic_read(&obj->trace) & STK_TRC_ALS_DATA)
	{
		APS_DBG("PS: 0x%04X\n", (u32)(*data));
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
int stk3a5x_write_ps(struct i2c_client *client, u8 data)
{
	struct stk3a5x_priv *obj = i2c_get_clientdata(client);
	int ret = 0;
	ret = stk3a5x_master_send(client, obj->addr.psctrl, &data, 1);

	if (ret < 0)
	{
		APS_ERR("write ps = %d\n", ret);
		return -EFAULT;
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
int stk3a5x_write_wait(struct i2c_client *client, u8 data)
{
	struct stk3a5x_priv *obj = i2c_get_clientdata(client);
	int ret = 0;
	ret = stk3a5x_master_send(client, obj->addr.wait, &data, 1);

	if (ret < 0)
	{
		APS_ERR("write wait = %d\n", ret);
		return -EFAULT;
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
int stk3a5x_write_int(struct i2c_client *client, u8 data)
{
	struct stk3a5x_priv *obj = i2c_get_clientdata(client);
	int ret = 0;
	ret = stk3a5x_master_send(client, obj->addr.intmode, &data, 1);

	if (ret < 0)
	{
		APS_ERR("write intmode = %d\n", ret);
		return -EFAULT;
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
int stk3a5x_write_state(struct i2c_client *client, u8 data)
{
	struct stk3a5x_priv *obj = i2c_get_clientdata(client);
	int ret = 0;
	ret = stk3a5x_master_send(client, obj->addr.state, &data, 1);

	if (ret < 0)
	{
		APS_ERR("write state = %d\n", ret);
		return -EFAULT;
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
int stk3a5x_write_flag(struct i2c_client *client, u8 data)
{
	struct stk3a5x_priv *obj = i2c_get_clientdata(client);
	int ret = 0;
	ret = stk3a5x_master_send(client, obj->addr.flag, &data, 1);

	if (ret < 0)
	{
		APS_ERR("write ps = %d\n", ret);
		return -EFAULT;
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
int stk3a5x_write_sw_reset(struct i2c_client *client)
{
	struct stk3a5x_priv *obj = i2c_get_clientdata(client);
	u8 buf = 0, r_buf = 0;
	int ret = 0;
	APS_LOG("%s: In\n", __FUNCTION__);
	buf = 0x7F;
	ret = stk3a5x_master_send(client, obj->addr.wait, (char*)&buf, sizeof(buf));

	if (ret < 0)
	{
		APS_ERR("i2c write test error = %d\n", ret);
		return -EFAULT;
	}

	ret = stk3a5x_master_recv(client, obj->addr.wait, &r_buf, 1);

	if (ret < 0)
	{
		APS_ERR("i2c read test error = %d\n", ret);
		return -EFAULT;
	}

	if (buf != r_buf)
	{
		APS_ERR("i2c r/w test error, read-back value is not the same, write=0x%x, read=0x%x\n", buf, r_buf);
		return -EIO;
	}

	buf = 0;
	ret = stk3a5x_master_send(client, obj->addr.soft_reset, (char*)&buf, sizeof(buf));

	if (ret < 0)
	{
		APS_ERR("write software reset error = %d\n", ret);
		return -EFAULT;
	}

	msleep(50);
	return 0;
}
/*----------------------------------------------------------------------------*/
int stk3a5x_write_ps_high_thd(struct i2c_client *client, u16 thd)
{
	struct stk3a5x_priv *obj = i2c_get_clientdata(client);
	u8 buf[2];
	int ret = 0;
	buf[0] = (u8)((0xFF00 & thd) >> 8);
	buf[1] = (u8)(0x00FF & thd);
	ret = stk3a5x_master_send(client, obj->addr.thdh1_ps, &buf[0], 1);

	if (ret < 0)
	{
		APS_ERR("WARNING: %d\n", ret);
		return -EFAULT;
	}

	ret = stk3a5x_master_send(client, obj->addr.thdh2_ps, &(buf[1]), 1);

	if (ret < 0)
	{
		APS_ERR("WARNING: %d\n", ret);
		return -EFAULT;
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
int stk3a5x_write_ps_low_thd(struct i2c_client *client, u16 thd)
{
	struct stk3a5x_priv *obj = i2c_get_clientdata(client);
	u8 buf[2];
	int ret = 0;
	buf[0] = (u8)((0xFF00 & thd) >> 8);
	buf[1] = (u8)(0x00FF & thd);
	ret = stk3a5x_master_send(client, obj->addr.thdl1_ps, &buf[0], 1);

	if (ret < 0)
	{
		APS_ERR("WARNING: %s: %d\n", __func__, ret);
		return -EFAULT;
	}

	ret = stk3a5x_master_send(client, obj->addr.thdl2_ps, &(buf[1]), 1);

	if (ret < 0)
	{
		APS_ERR("WARNING: %s: %d\n", __func__, ret);
		return -EFAULT;
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
int stk3a5x_write_als_high_thd(struct i2c_client *client, u16 thd)
{
	struct stk3a5x_priv *obj = i2c_get_clientdata(client);
	u8 buf[2];
	int ret = 0;
	buf[0] = (u8)((0xFF00 & thd) >> 8);
	buf[1] = (u8)(0x00FF & thd);
	ret = stk3a5x_master_send(client, obj->addr.thdh1_als, &buf[0], 1);

	if (ret < 0)
	{
		APS_ERR("WARNING: %s: %d\n", __func__, ret);
		return -EFAULT;
	}

	ret = stk3a5x_master_send(client, obj->addr.thdh2_als, &(buf[1]), 1);

	if (ret < 0)
	{
		APS_ERR("WARNING: %s: %d\n", __func__, ret);
		return -EFAULT;
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
int stk3a5x_write_als_low_thd(struct i2c_client *client, u16 thd)
{
	struct stk3a5x_priv *obj = i2c_get_clientdata(client);
	u8 buf[2];
	int ret = 0;
	buf[0] = (u8)((0xFF00 & thd) >> 8);
	buf[1] = (u8)(0x00FF & thd);
	ret = stk3a5x_master_send(client, obj->addr.thdl1_als, &buf[0], 1);

	if (ret < 0)
	{
		APS_ERR("WARNING: %s: %d\n", __func__, ret);
		return -EFAULT;
	}

	ret = stk3a5x_master_send(client, obj->addr.thdl2_als, &(buf[1]), 1);

	if (ret < 0)
	{
		APS_ERR("WARNING: %s: %d\n", __func__, ret);
		return -EFAULT;
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
#if 1
static int show_allreg(void)
{
	int ret = 0;
	u8 reg[14] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x4F, 0xA0, 0xA1, 0xDB, 0xF4, 0xFA, 0x3E, 0x3F};
	u8 rbuf;
	int cnt;

	if (!stk3a5x_obj)
	{
		APS_ERR("stk3a5x_obj is null!!\n");
		return 0;
	}

	for(cnt = 0; cnt < 14; cnt++){
		ret = stk3a5x_master_recv(stk3a5x_obj->client, reg[cnt], &rbuf, 1);
		if (ret < 0)
		{
			APS_DBG("error: %d\n", ret);
		}else{
			APS_LOG("reg[0x%x]=0x%x\n", reg[cnt], rbuf);
		}		
	}
	
	return 0;
}
#endif
/*----------------------------------------------------------------------------*/
#if 0
static void stk3a5x_power(struct alsps_hw *hw, unsigned int on)
{
#if 0
	int ret;

	if (on == 1)
	{
		ret = regulator_enable(stk3a5x_regulator);	/*enable regulator*/

		if (ret)
			printk("regulator_enable() failed!\n");
	}
	else
	{
		ret = regulator_disable(stk3a5x_regulator);       /*enable regulator*/

		if (ret)
			printk("regulator_disable() failed!\n");
	}

#endif
}
#endif
/*----------------------------------------------------------------------------*/
static void stk_ps_report(struct stk3a5x_priv *obj, int nf)
{
	/*
		hwm_sensor_data sensor_data;

		sensor_data.values[0] = nf;
		sensor_data.value_divide = 1;
		sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
		if((nf = hwmsen_get_interrupt_data(ID_PROXIMITY, &sensor_data)))
		APS_ERR("call hwmsen_get_interrupt_data fail = %d\n", nf);
	*/
	if (ps_report_interrupt_data(nf))
		APS_ERR("call ps_report_interrupt_data fail\n");

	APS_LOG("%s:ps raw 0x%x -> value 0x%x \n", __FUNCTION__, obj->ps, nf);
	obj->ps_distance_last = nf;
}
/*----------------------------------------------------------------------------*/
static int stk3a5x_enable_als(struct i2c_client *client, int enable)
{
	struct stk3a5x_priv *obj = i2c_get_clientdata(client);
	int err, cur = 0, old = atomic_read(&obj->state_val);
	int trc = atomic_read(&obj->trace);
	APS_LOG("%s: enable=%d\n", __func__, enable);

	obj->als_debug_count = 0;
	cur = old & (~(STK_STATE_EN_ALS_MASK | STK_STATE_EN_WAIT_MASK));

	if (enable)
	{
		cur |= STK_STATE_EN_ALS_MASK;
	}
	else if (old & STK_STATE_EN_PS_MASK)
	{
		cur |= STK_STATE_EN_WAIT_MASK;
	}

	if (trc & STK_TRC_DEBUG)
	{
		APS_LOG("%s: %08X, %08X, %d\n", __func__, cur, old, enable);
	}

	if (0 == (cur ^ old))
	{
		return 0;
	}

	if (enable && obj->hw->polling_mode_als == 0)
	{
		stk3a5x_write_als_high_thd(client, 0x0);
		stk3a5x_write_als_low_thd(client, 0xFFFF);
	}

	err = stk3a5x_write_state(client, cur);

	if (err < 0)
		return err;
	else
		atomic_set(&obj->state_val, cur);

	if (enable)
	{
		obj->als_last = 0;

		if (obj->hw->polling_mode_als)
		{
			atomic_set(&obj->als_deb_on, 1);
			atomic_set(&obj->als_deb_end, jiffies + atomic_read(&obj->als_debounce)*HZ / 1000);
		}
		else
		{
			//set_bit(STK_BIT_ALS,  &obj->pending_intr);
			schedule_delayed_work(&obj->eint_work, 220 * HZ / 1000);
		}
	}

	if (trc & STK_TRC_DEBUG)
	{
		APS_LOG("enable als (%d)\n", enable);
	}

#if 0
	APS_LOG("%s:show all reg\n", __FUNCTION__);
	show_allreg(); //for debug
#endif
	return err;
}
/*----------------------------------------------------------------------------*/
static int stk3a5x_enable_ps_set_thd(struct stk3a5x_priv *obj)
{
	int err;
#ifdef STK_TUNE0
#ifdef CALI_EVERY_TIME

	if (obj->crosstalk > 0)
	{
		atomic_set(&obj->ps_high_thd_val, obj->ps_high_thd_boot);
		atomic_set(&obj->ps_low_thd_val, obj->ps_low_thd_boot);
	}
	else if (obj->boot_cali == 1 && obj->ps_low_thd_boot < 1000)
	{
		atomic_set(&obj->ps_high_thd_val, obj->ps_high_thd_boot);
		atomic_set(&obj->ps_low_thd_val, obj->ps_low_thd_boot);
	}

#endif

	if ((err = stk3a5x_write_ps_high_thd(obj->client, atomic_read(&obj->ps_high_thd_val))))
	{
		APS_ERR("write high thd error: %d\n", err);
		return err;
	}

	if ((err = stk3a5x_write_ps_low_thd(obj->client, atomic_read(&obj->ps_low_thd_val))))
	{
		APS_ERR("write low thd error: %d\n", err);
		return err;
	}

	APS_LOG("%s:in reg, HT=%d, LT=%d\n", __func__, atomic_read(&obj->ps_high_thd_val), atomic_read(&obj->ps_low_thd_val));
#else

	if ((err = stk3a5x_write_ps_high_thd(obj->client, atomic_read(&obj->ps_high_thd_val))))
	{
		APS_ERR("write high thd error: %d\n", err);
		return err;
	}

	if ((err = stk3a5x_write_ps_low_thd(obj->client, atomic_read(&obj->ps_low_thd_val))))
	{
		APS_ERR("write low thd error: %d\n", err);
		return err;
	}

	APS_LOG("%s:in reg, HT=%d, LT=%d\n", __func__, atomic_read(&obj->ps_high_thd_val), atomic_read(&obj->ps_low_thd_val));
#endif
	APS_LOG("%s:in driver, HT=%d, LT=%d\n", __func__, atomic_read(&obj->ps_high_thd_val), atomic_read(&obj->ps_low_thd_val));
	return 0;
}
/*----------------------------------------------------------------------------*/
static int stk3a5x_enable_ps(struct i2c_client *client, int enable, int validate_reg)
{
	struct stk3a5x_priv *obj = i2c_get_clientdata(client);
	int err, cur = 0, old = atomic_read(&obj->state_val);
	int trc = atomic_read(&obj->trace);
#if 1

	if (enable)
	{
		APS_LOG("%s: before enable ps, HT=%d, LT=%d\n", __func__, atomic_read(&obj->ps_high_thd_val), atomic_read(&obj->ps_low_thd_val));
		APS_LOG("%s: before enable ps, show all reg\n", __FUNCTION__);
		show_allreg(); //for debug
	}

#endif
	printk("%s: enable=%d\n", __FUNCTION__, enable);
	cur = old;
	cur &= (~(0x45));

	if (enable)
	{
		cur |= (STK_STATE_EN_PS_MASK|STK_STATE_EN_WAIT_MASK);

		if (!(old & STK_STATE_EN_ALS_MASK))
			cur |= STK_STATE_EN_WAIT_MASK;
	}

	if (0 == (cur ^ old))
	{
		printk("%s: repeat enable=%d, reg=0x%x\n", __FUNCTION__, enable, cur);
		return 0;
	}

#ifdef STK_TUNE0
#ifndef CTTRACKING

	if (!(obj->psi_set) && !enable)
	{
		hrtimer_cancel(&obj->ps_tune0_timer);
		cancel_work_sync(&obj->stk_ps_tune0_work);
	}

#endif
#endif

	if (obj->first_boot == true)
	{
		obj->first_boot = false;
	}

	if (enable)
	{
		stk3a5x_enable_ps_set_thd(obj);

		//if (1 == obj->hw->polling_mode_ps)
			//wake_lock(&mps_lock);
	}
	else
	{
		//if (1 == obj->hw->polling_mode_ps)
			//wake_unlock(&mps_lock);
	}

	if (trc & STK_TRC_DEBUG)
	{
		APS_LOG("%s: %08X, %08X, %d\n", __func__, cur, old, enable);
	}

	err = stk3a5x_write_state(client, cur);

	if (err < 0)
		return err;

	atomic_set(&obj->state_val, cur);

	if (enable)
	{
#ifdef STK_TUNE0
#ifdef CTTRACKING
		obj->ps_thd_update = false;
#endif
#ifndef CALI_EVERY_TIME

		if (!(obj->psi_set))
			hrtimer_start(&obj->ps_tune0_timer, obj->ps_tune0_delay, HRTIMER_MODE_REL);

#else
		obj->psi_set = 0;
		obj->psa = 0;
		obj->psi = 0xFFFF;
		hrtimer_start(&obj->ps_tune0_timer, obj->ps_tune0_delay, HRTIMER_MODE_REL);
#endif
#endif

		if (obj->hw->polling_mode_ps)
		{
			atomic_set(&obj->ps_deb_on, 1);
			atomic_set(&obj->ps_deb_end, jiffies + atomic_read(&obj->ps_debounce)*HZ / 1000);
		}
		else
		{
			{
				msleep(5);

				if ((err = stk3a5x_read_ps(obj->client, &obj->ps)))
				{
					APS_ERR("stk3a5x read ps data: %d\n", err);
					return err;
				}

				err = stk3a5x_get_ps_value_only(obj, obj->ps);

				if (err < 0)
				{
					APS_ERR("stk3a5x get ps value: %d\n", err);
					return err;
				}
				else if (stk3a5x_obj->hw->polling_mode_ps == 0)
				{
					stk_ps_report(obj, err);
				}

#if 1
				APS_LOG("%s: after stk_ps_report, show all reg\n", __FUNCTION__);
				show_allreg(); //for debug
#endif
			}
		}
		pinctrl_select_state(pinctrl, pins_v3venable);
	}
	else
	{
#ifdef CTTRACKING
		hrtimer_cancel(&obj->ps_tune0_timer);
#endif
		pinctrl_select_state(pinctrl, pins_v3vdisable);
	}

	if (trc & STK_TRC_DEBUG)
	{
		APS_LOG("enable ps  (%d)\n", enable);
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
static int stk3a5x_ps_calibration(struct i2c_client *client)
{
	struct stk3a5x_priv *obj = i2c_get_clientdata(client);
	u8 reg;
	int err = 0;
	int org_state_reg = atomic_read(&obj->state_val);
	bool re_en_ps = false, re_en_als = false;
	int counter = 0;
	uint32_t ps_sum = 0;
	const int PS_CALI_COUNT = 5;

	if (org_state_reg & STK_STATE_EN_PS_MASK)
	{
		APS_LOG("%s: force disable PS\n", __func__);
		stk3a5x_enable_ps(obj->client, 0, 1);
		re_en_ps = true;
	}

	if (org_state_reg & STK_STATE_EN_ALS_MASK)
	{
		APS_LOG("%s: force disable ALS\n", __func__);
		stk3a5x_enable_als(obj->client, 0);
		re_en_als = true;
	}

#if defined(CONFIG_OF)
	disable_irq(obj->irq);
#elif ((defined MT6573) || (defined MT6575) || (defined MT6577) || (defined MT6589) || (defined MT6572))
	mt65xx_eint_mask(CUST_EINT_ALS_NUM);
#else
	mt_eint_mask(CUST_EINT_ALS_NUM);
#endif
	reg = STK_STATE_EN_WAIT_MASK | STK_STATE_EN_PS_MASK;
	err = stk3a5x_write_state(client, reg);

	if (err < 0)
		goto err_out;

	while (counter < PS_CALI_COUNT)
	{
		msleep(60);

		if ((err = stk3a5x_read_ps(obj->client, &obj->ps)))
			goto err_out;

		//#ifdef STK_PS_POLLING_LOG
		APS_LOG("%s: ps=%d\n", __func__, obj->ps);
		//#endif
		ps_sum += obj->ps;
		counter++;
	}

	obj->ps_cali = (ps_sum / PS_CALI_COUNT);
#ifdef STK_TUNE0
	obj->crosstalk = (ps_sum / PS_CALI_COUNT);
	if (obj->crosstalk == 0)
	{
		obj->crosstalk = obj->hw->ps_threshold_low;
		stk3a5x_calibration_flag = 0;
	}
	else
	{
		stk3a5x_calibration_flag = 1;
	}

#endif
#ifdef CALI_EVERY_TIME
	obj->ps_high_thd_boot = obj->crosstalk + obj->stk_ht_n_ct;
	obj->ps_low_thd_boot = obj->crosstalk + obj->stk_lt_n_ct;
	printk("%s: crosstalk=%d, high thd=%d, low thd=%d\n", __func__, obj->crosstalk, obj->ps_high_thd_boot, obj->ps_low_thd_boot);
#endif
err_out:
#if defined(CONFIG_OF)
	disable_irq(obj->irq);
#elif ((defined MT6573) || (defined MT6575) || (defined MT6577) || (defined MT6589) || (defined MT6572))
	mt65xx_eint_mask(CUST_EINT_ALS_NUM);
#else
	mt_eint_mask(CUST_EINT_ALS_NUM);
#endif

	if (re_en_als)
	{
		APS_LOG("%s: re-enable ALS\n", __func__);
		stk3a5x_enable_als(obj->client, 1);
	}

	if (re_en_ps)
	{
		APS_LOG("%s: re-enable PS\n", __func__);
		stk3a5x_enable_ps(obj->client, 1, 1);
	}

	return err;
}
/*----------------------------------------------------------------------------*/
#ifdef STK_ALS_MID_FIR
static void stk3a5x_als_bubble_sort(uint16_t* sort_array, uint8_t size_n)
{
	int i, j, tmp;

	for (i = 1; i < size_n; i++)
	{
		tmp = sort_array[i];
		j = i - 1;

		while (j >= 0 && sort_array[j] > tmp)
		{
			sort_array[j + 1] = sort_array[j];
			j = j - 1;
		}

		sort_array[j + 1] = tmp;
	}
}
#endif
/*----------------------------------------------------------------------------*/
static int stk3a5x_check_intr(struct i2c_client *client, u8 *status)
{
	struct stk3a5x_priv *obj = i2c_get_clientdata(client);
	int err;
	//if (mt_get_gpio_in(GPIO_ALS_EINT_PIN) == 1) /*skip if no interrupt*/
	//    return 0;
	err = stk3a5x_read_flag(client, status);

	if (err < 0)
	{
		APS_ERR("WARNING: read flag reg error: %d\n", err);
		return -EFAULT;
	}

	APS_LOG("%s: read status reg: 0x%x\n", __func__, *status);

	if (*status & STK_FLG_ALSINT_MASK)
	{
		set_bit(STK_BIT_ALS, &obj->pending_intr);
	}
	else
	{
		clear_bit(STK_BIT_ALS, &obj->pending_intr);
	}

	if (*status & STK_FLG_PSINT_MASK)
	{
		set_bit(STK_BIT_PS, &obj->pending_intr);
	}
	else
	{
		clear_bit(STK_BIT_PS, &obj->pending_intr);
	}

	if (atomic_read(&obj->trace) & STK_TRC_DEBUG)
	{
		APS_LOG("check intr: 0x%02X => 0x%08lX\n", *status, obj->pending_intr);
	}

	return 0;
}
static int stk3a5x_clear_intr(struct i2c_client *client, u8 status, u8 disable_flag)
{
	int err = 0;
	status = status | (STK_FLG_ALSINT_MASK | STK_FLG_PSINT_MASK);
	status &= (~disable_flag);

	//APS_LOG(" set flag reg: 0x%x\n", status);
	if ((err = stk3a5x_write_flag(client, status)))
		APS_ERR("stk3a5x_write_flag failed, err=%d\n", err);

	return err;
}

/*----------------------------------------------------------------------------*/
#if 0
static int stk3a5x_set_als_int_thd(struct i2c_client *client, u16 als_data_reg)
{
	s32 als_thd_h, als_thd_l;
	als_thd_h = als_data_reg + STK_ALS_CODE_CHANGE_THD;
	als_thd_l = als_data_reg - STK_ALS_CODE_CHANGE_THD;

	if (als_thd_h >= (1 << 16))
		als_thd_h = (1 << 16) - 1;

	if (als_thd_l < 0)
		als_thd_l = 0;

	APS_LOG("stk3a5x_set_als_int_thd:als_thd_h:%d,als_thd_l:%d\n", als_thd_h, als_thd_l);
	stk3a5x_write_als_high_thd(client, als_thd_h);
	stk3a5x_write_als_low_thd(client, als_thd_l);
	return 0;
}
#endif

/*----------------------------------------------------------------------------*/
static int stk3a5x_ps_val(void)
{
	u8 ps_invalid_flag, i;
	u8 bgir_raw_data[4]={0};
	int ret;
	u8 bgir_thd = 3;
	uint16_t psoff_thd = 1000;
	uint16_t psoff[4]={0};
	u8 buf[2];

	ret = stk3a5x_master_recv(stk3a5x_obj->client, 0xA7, &ps_invalid_flag, 1);
	if (ret < 0)
	{
		APS_ERR("%s fail, err=0x%x", __FUNCTION__, ret);
		return ret;
	}

	ret = stk3a5x_master_recv(stk3a5x_obj->client, 0x34, bgir_raw_data, 4);

	if (ret < 0)
	{
		APS_ERR("%s fail, err=0x%x", __FUNCTION__, ret);
		return ret;
	}

	for(i = 0; i < 4;i++){
		ret = stk3a5x_master_recv(stk3a5x_obj->client, (0x24 + i*2), buf, 2);
		if (ret < 0)
		{
			APS_ERR("%s fail, err=0x%x", __func__, ret);
			return ret;
		}
		psoff[i] = (buf[0] << 8) | buf[1];	
	}

	if((stk3a5x_obj->data_count%10)==3){
		APS_LOG("%s: bgir0=%d, 1=%d, 2=%d, 3=%d\n", __func__, bgir_raw_data[0], bgir_raw_data[1], bgir_raw_data[2], bgir_raw_data[3]);
		APS_LOG("%s: psoff=%d, 1=%d, 2=%d, 3=%d\n", __func__, psoff[0], psoff[1], psoff[2], psoff[3]);
	}

	if (((ps_invalid_flag >> 5) & 0x1) || ((bgir_raw_data[0] & 0x7f) >= bgir_thd) ||
		((bgir_raw_data[1] & 0x7f) >= bgir_thd) || ((bgir_raw_data[2] & 0x7f) >= bgir_thd) || ((bgir_raw_data[3] & 0x7f) >= bgir_thd))
	{
		return 0xFFFF;
	}

	if ((psoff[0] >= psoff_thd) || (psoff[1] >= psoff_thd) ||
		(psoff[2] >= psoff_thd) || (psoff[3] >= psoff_thd))
	{
		return 0xFFFF;
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
#ifdef STK_TUNE0
static int stk_ps_tune_zero_final(struct stk3a5x_priv *obj)
{
	int err;
	obj->tune_zero_init_proc = false;

	if ((err = stk3a5x_write_int(obj->client, obj->int_val)))
	{
		APS_ERR("write int mode error: %d\n", err);
		return err;
	}

	if ((err = stk3a5x_write_state(obj->client, atomic_read(&obj->state_val))))
	{
		APS_ERR("write stete error: %d\n", err);
		return err;
	}

	if (obj->data_count == -1)
	{
		APS_LOG("%s: exceed limit\n", __func__);
		hrtimer_cancel(&obj->ps_tune0_timer);
		return 0;
	}

	obj->psa = obj->ps_stat_data[0];
	obj->psi = obj->ps_stat_data[2];
#ifndef CALI_EVERY_TIME
	atomic_set(&obj->ps_high_thd_val, obj->ps_stat_data[1] + obj->stk_ht_n_ct);
	atomic_set(&obj->ps_low_thd_val, obj->ps_stat_data[1] + obj->stk_lt_n_ct);
#else
	obj->ps_high_thd_boot = obj->ps_stat_data[1] + obj->stk_ht_n_ct;
	obj->ps_low_thd_boot = obj->ps_stat_data[1] + obj->stk_lt_n_ct;
	atomic_set(&obj->ps_high_thd_val, obj->ps_high_thd_boot);
	atomic_set(&obj->ps_low_thd_val, obj->ps_low_thd_boot);
#endif
	//	obj->crosstalk = obj->ps_stat_data[1];

	if ((err = stk3a5x_write_ps_high_thd(obj->client, atomic_read(&obj->ps_high_thd_val))))
	{
		APS_ERR("write high thd error: %d\n", err);
		return err;
	}

	if ((err = stk3a5x_write_ps_low_thd(obj->client, atomic_read(&obj->ps_low_thd_val))))
	{
		APS_ERR("write low thd error: %d\n", err);
		return err;
	}

	obj->boot_cali = 1;
	APS_LOG("%s: set HT=%d,LT=%d\n", __func__, atomic_read(&obj->ps_high_thd_val), atomic_read(&obj->ps_low_thd_val));
	hrtimer_cancel(&obj->ps_tune0_timer);
	return 0;
}
/*----------------------------------------------------------------------------*/
static int32_t stk_tune_zero_get_ps_data(struct stk3a5x_priv *obj)
{
	int err;
	err = stk3a5x_ps_val();

	if (err == 0xFFFF)
	{
		obj->data_count = -1;
		stk_ps_tune_zero_final(obj);
		return 0;
	}

	if ((err = stk3a5x_read_ps(obj->client, &obj->ps)))
	{
		APS_ERR("stk3a5x read ps data: %d\n", err);
		return err;
	}

	APS_LOG("%s: ps #%d=%d\n", __func__, obj->data_count, obj->ps);
	obj->ps_stat_data[1] += obj->ps;

	if (obj->ps > obj->ps_stat_data[0])
		obj->ps_stat_data[0] = obj->ps;

	if (obj->ps < obj->ps_stat_data[2])
		obj->ps_stat_data[2] = obj->ps;

	obj->data_count++;

	if (obj->data_count == 5)
	{
		obj->ps_stat_data[1] /= obj->data_count;
		stk_ps_tune_zero_final(obj);
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
static int stk_ps_tune_zero_init(struct stk3a5x_priv *obj)
{
	u8 w_state_reg;
	int err;
	obj->psa = 0;
	obj->psi = 0xFFFF;
	obj->psi_set = 0;
	obj->tune_zero_init_proc = true;
	obj->ps_stat_data[0] = 0;
	obj->ps_stat_data[2] = 9999;
	obj->ps_stat_data[1] = 0;
	obj->data_count = 0;
	obj->boot_cali = 0;
#ifdef CALI_EVERY_TIME
	obj->ps_high_thd_boot = obj->hw->ps_threshold_high;
	obj->ps_low_thd_boot = obj->hw->ps_threshold_low;

	if (obj->ps_high_thd_boot <= 0)
	{
		obj->ps_high_thd_boot = obj->stk_ht_n_ct * 3;
		obj->ps_low_thd_boot = obj->stk_lt_n_ct * 3;
	}

#endif

	if ((err = stk3a5x_write_int(obj->client, 0)))
	{
		APS_ERR("write int mode error: %d\n", err);
		return err;
	}

	w_state_reg = (STK_STATE_EN_PS_MASK | STK_STATE_EN_WAIT_MASK);

	if ((err = stk3a5x_write_state(obj->client, w_state_reg)))
	{
		APS_ERR("write stete error: %d\n", err);
		return err;
	}

	hrtimer_start(&obj->ps_tune0_timer, obj->ps_tune0_delay, HRTIMER_MODE_REL);
	return 0;
}
/*----------------------------------------------------------------------------*/
static int stk_ps_tune_zero_func_fae(struct stk3a5x_priv *obj)
{
#ifdef CTTRACKING
	uint16_t ct_value = 0;
#endif
	int32_t word_data;
	u8 flag;
	bool ps_enabled = false;
	u8 buf[2];
	int ret, diff;
	uint16_t high_thd, low_thd;
	ps_enabled = (atomic_read(&obj->state_val) & STK_STATE_EN_PS_MASK) ? true : false;
#ifndef CALI_EVERY_TIME
	if (obj->psi_set || !(ps_enabled))
#else
	if (!(ps_enabled))
#endif
	{
		return 0;
	}


	obj->data_count++;
	if(obj->data_count < 3){
		APS_ERR("%s:count=%d\n", __func__, obj->data_count);
		return 0;
	}

	if(obj->data_count > 50000){
		obj->data_count = 10;
	}else if((obj->data_count%10)==6){
		ret = stk3a5x_read_flag(obj->client, &flag);

		if (ret < 0)
		{
			APS_ERR("%s: get flag failed, err=0x%x\n", __func__, ret);
			return ret;
		}

		ret = stk3a5x_master_recv(obj->client, 0x11, buf, 2);
		if (ret < 0)
		{
			APS_ERR("%s fail, err=0x%x", __func__, ret);
			return ret;
		}
		word_data = (buf[0] << 8) | buf[1];	
		APS_LOG("%s: cur ps=%d, flag=0x%x, dis=%d\n", __func__, word_data, flag, obj->ps_distance_last);
		APS_LOG("%s: psi_set=%d, psi=%d, smudge=%d\n", __func__, obj->psi_set, obj->psi, obj->ps_thd_update);	
	}else if((obj->data_count%10)==9){
        ret = stk3a5x_get_ps_high_thd(obj->client, &high_thd);
        if (ret < 0)
        {
            APS_DBG("fail, error: %d\n", ret);
            return ret;
        }

        ret = stk3a5x_get_ps_low_thd(obj->client, &low_thd);
        if (ret < 0)
        {
            APS_DBG("fail, error: %d\n", ret);
            return ret;
        }
		APS_LOG("%s: cur ht=%d, lt=%d\n", __func__, high_thd, low_thd);
	}else if((obj->data_count%50)==8){
		show_allreg();
	}

#ifdef CTTRACKING

	if ((obj->psi_set != 0))
	{
		if ((obj->ps_distance_last == 1))
		{
			ret = stk3a5x_ps_val();

			if (ret == 0)
			{
				ret = stk3a5x_master_recv(obj->client, 0x11, buf, 2);

				if (ret < 0)
				{
					APS_ERR("%s fail, err=0x%x", __func__, ret);
					return ret;
				}

				word_data = (buf[0] << 8) | buf[1];

				if (word_data > 0)
				{
					ct_value = atomic_read(&obj->ps_high_thd_val) - stk3a5x_obj->stk_ht_n_ct;
					ret = stk3a5x_read_flag(obj->client, &flag);

					if ((word_data < ct_value) && ((ct_value - word_data) > 10)
						&& ((flag & STK_FLG_PSINT_MASK) == 0)
						&& (obj->ps_distance_last == 1)
						&& (obj->ps_thd_update == false))
					{
						atomic_set(&obj->ps_high_thd_val, word_data + stk3a5x_obj->stk_ht_n_ct);
						atomic_set(&obj->ps_low_thd_val, word_data + stk3a5x_obj->stk_lt_n_ct);
						stk3a5x_write_ps_high_thd(obj->client, atomic_read(&obj->ps_high_thd_val));
						stk3a5x_write_ps_low_thd(obj->client, atomic_read(&obj->ps_low_thd_val));
						APS_LOG("%s: CTTRACKING set HT=%d, LT=%d\n", __func__,
							atomic_read(&obj->ps_high_thd_val),
							atomic_read(&obj->ps_low_thd_val));
						obj->psi = word_data;
						APS_LOG("%s: CTTRACKING update psi=%d\n",
							__func__, obj->psi);

						if ((atomic_read(&obj->ps_high_thd_val) + 100) < (obj->ps_high_thd_boot))
						{
							obj->ps_high_thd_boot =
								word_data + obj->stk_ht_n_ct + STK_H_LT;
							obj->ps_low_thd_boot =
								word_data + obj->stk_lt_n_ct + STK_H_LT;
							obj->boot_cali = 1;
							APS_LOG("%s: update boot HT=%d, LT=%d\n",
								__func__,
								obj->ps_high_thd_boot, obj->ps_low_thd_boot);
						}
					}
				}
			}
		}

		return 0;
	}

#endif
	ret = stk3a5x_read_flag(obj->client, &flag);

	if (ret < 0)
	{
		APS_ERR("%s: get flag failed, err=0x%x\n", __func__, ret);
		return ret;
	}

	if (!(flag & STK_FLG_PSDR_MASK))
	{
		return 0;
	}

	if ((flag & STK_FLG_NF_MASK) == 0)
	{
		APS_ERR("%s: Nearby State!\n", __func__);
		return 0;
	}

	ret = stk3a5x_ps_val();

	if (ret == 0)
	{
		ret = stk3a5x_master_recv(obj->client, 0x11, buf, 2);

		if (ret < 0)
		{
			APS_ERR("%s fail, err=0x%x", __func__, ret);
			return ret;
		}

		word_data = (buf[0] << 8) | buf[1];
		//APS_LOG("%s: word_data=%d\n", __func__, word_data);

		if (word_data == 0)
		{
			//APS_ERR( "%s: incorrect word data (0)\n", __func__);
			return 0xFFFF;
		}

		if (word_data > obj->psa)
		{
			obj->psa = word_data;
			APS_LOG("%s: update psa: psa=%d,psi=%d\n", __func__, obj->psa, obj->psi);
		}

		if (word_data < obj->psi)
		{
			obj->psi = word_data;
			APS_LOG("%s: update psi: psa=%d,psi=%d\n", __func__, obj->psa, obj->psi);
		}
	}

	diff = obj->psa - obj->psi;

	if (diff > obj->stk_max_min_diff)
	{
		obj->psi_set = obj->psi;
		atomic_set(&obj->ps_high_thd_val, obj->psi + obj->stk_ht_n_ct);
		atomic_set(&obj->ps_low_thd_val, obj->psi + obj->stk_lt_n_ct);
#ifdef CALI_EVERY_TIME

		if ((atomic_read(&obj->ps_high_thd_val) > (obj->ps_high_thd_boot + 500)) && (obj->boot_cali == 1))
		{
			atomic_set(&obj->ps_high_thd_val, obj->ps_high_thd_boot);
			atomic_set(&obj->ps_low_thd_val, obj->ps_low_thd_boot);
			APS_LOG("%s: change THD to boot HT=%d, LT=%d\n", __func__,
				atomic_read(&obj->ps_high_thd_val),
				atomic_read(&obj->ps_low_thd_val));
		}
		else
		{
			if ((atomic_read(&obj->ps_high_thd_val) + 100) < (obj->ps_high_thd_boot))
			{
				obj->ps_high_thd_boot = obj->psi + obj->stk_ht_n_ct + STK_H_LT;
				obj->ps_low_thd_boot = obj->psi + obj->stk_lt_n_ct + STK_H_LT;
				obj->boot_cali = 1;
				APS_LOG("%s: update boot HT=%d, LT=%d\n",
					__func__,
					obj->ps_high_thd_boot,
					obj->ps_low_thd_boot);
			}
		}

#endif

		if ((ret = stk3a5x_write_ps_high_thd(obj->client, atomic_read(&obj->ps_high_thd_val))))
		{
			APS_ERR("write high thd error: %d\n", ret);
			return ret;
		}

		if ((ret = stk3a5x_write_ps_low_thd(obj->client, atomic_read(&obj->ps_low_thd_val))))
		{
			APS_ERR("write low thd error: %d\n", ret);
			return ret;
		}

#ifdef STK_DEBUG_PRINTF
		APS_LOG("%s: FAE tune0 psa-psi(%d) > DIFF found\n", __func__, diff);
#endif
		APS_LOG("%s: set HT=%d, LT=%d\n", __func__,
			atomic_read(&obj->ps_high_thd_val),
			atomic_read(&obj->ps_low_thd_val));
#ifndef CTTRACKING
		hrtimer_cancel(&obj->ps_tune0_timer);
#endif
	}

	return 0;
}
#endif	/*#ifdef STK_TUNE0	*/
static void stk_ps_tune0_work_func(struct work_struct *work)
{
	struct stk3a5x_priv *obj = container_of(work, struct stk3a5x_priv, stk_ps_tune0_work);
#ifdef STK_TUNE0

	if (obj->tune_zero_init_proc)
		stk_tune_zero_get_ps_data(obj);
	else
		stk_ps_tune_zero_func_fae(obj);

#endif
	return;
}
/*----------------------------------------------------------------------------*/
#ifdef STK_TUNE0
static enum hrtimer_restart stk_ps_tune0_timer_func(struct hrtimer *timer)
{
	struct stk3a5x_priv *obj = container_of(timer, struct stk3a5x_priv, ps_tune0_timer);
	queue_work(obj->stk_ps_tune0_wq, &obj->stk_ps_tune0_work);
	hrtimer_forward_now(&obj->ps_tune0_timer, obj->ps_tune0_delay);
	return HRTIMER_RESTART;
}
#endif
/*----------------------------------------------------------------------------*/
static void stk_ps_int_handle(struct stk3a5x_priv *obj, int nf_state)
{
	struct hwm_sensor_data sensor_data;
	memset(&sensor_data, 0, sizeof(sensor_data));
	sensor_data.values[0] = nf_state;
	sensor_data.value_divide = 1;
	sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
	APS_LOG("%s:ps raw 0x%x -> value 0x%x \n", __FUNCTION__, obj->ps, sensor_data.values[0]);

	//let up layer to know
	/*
		if((err = hwmsen_get_interrupt_data(ID_PROXIMITY, &sensor_data)))
		{
			APS_ERR("call hwmsen_get_interrupt_data fail = %d\n", err);
		}
	*/
	if (ps_report_interrupt_data(sensor_data.values[0]))
		APS_ERR("call ps_report_interrupt_data fail\n");

	obj->ps_distance_last = sensor_data.values[0];
}
/*----------------------------------------------------------------------------*/
#if 0
static int stk_als_int_handle(struct stk3a5x_priv *obj, u16 als_reading)
{
	struct hwm_sensor_data sensor_data;
	memset(&sensor_data, 0, sizeof(sensor_data));
	stk3a5x_set_als_int_thd(obj->client, obj->als);
	sensor_data.values[0] = stk3a5x_get_als_value(obj, obj->als);
	sensor_data.value_divide = 1;
	sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
	APS_LOG("%s:als raw 0x%x -> value 0x%x \n", __FUNCTION__, obj->als, sensor_data.values[0]);
	//let up layer to know
	obj->als_code_last = als_reading;
	/*
	if((err = hwmsen_get_interrupt_data(ID_LIGHT, &sensor_data)))
	{
		APS_ERR("call hwmsen_get_interrupt_data fail = %d\n", err);
	}
	*/
	// no available command such as als_report_interrupt_data to report data
	/*
	if(als_report_interrupt_data(sensor_data.values[0]))
	{
		APS_ERR("call ps_report_interrupt_data fail\n");
	}
	*/
	return 0;
}
/*----------------------------------------------------------------------------*/
#endif 
void stk3a5x_eint_func(void)
{
	struct stk3a5x_priv *obj = stk3a5x_obj;
	APS_LOG(" interrupt fuc\n");

	if (!obj)
	{
		return;
	}

	//schedule_work(&obj->eint_work);
	if (obj->hw->polling_mode_ps == 0 || obj->hw->polling_mode_als == 0)
		schedule_delayed_work(&obj->eint_work, 0);

	if (atomic_read(&obj->trace) & STK_TRC_EINT)
	{
		APS_LOG("eint: als/ps intrs\n");
	}
}
/*----------------------------------------------------------------------------*/
static void stk3a5x_eint_work(struct work_struct *work)
{
	struct stk3a5x_priv *obj = stk3a5x_obj;
	int err;
	u8 flag_reg, disable_flag = 0;
	u8 near_far_state = 0;
	APS_LOG(" eint work\n");

	if ((err = stk3a5x_check_intr(obj->client, &flag_reg)))
	{
		APS_ERR("stk3a5x_check_intr fail: %d\n", err);
		goto err_i2c_rw;
	}

	APS_LOG(" &obj->pending_intr =%lx\n", obj->pending_intr);

	if (((1 << STK_BIT_ALS) & obj->pending_intr) && (obj->hw->polling_mode_als == 0))
	{
		//get raw data
		APS_LOG("stk als change\n");
		disable_flag |= STK_FLG_ALSINT_MASK;

		if ((err = stk3a5x_read_als(obj->client, &obj->als)))
		{
			APS_ERR("stk3a5x_read_als failed %d\n", err);
			goto err_i2c_rw;
		}
#if 0
		err = stk_als_int_handle(obj, obj->als);

		if (err < 0)
			goto err_i2c_rw;
#endif
	}

	if (((1 << STK_BIT_PS) &  obj->pending_intr) && (obj->hw->polling_mode_ps == 0))
	{
		APS_LOG("stk ps change\n");
		disable_flag |= STK_FLG_PSINT_MASK;

		if ((err = stk3a5x_read_ps(obj->client, &obj->ps)))
		{
			APS_ERR("stk3a5x read ps data: %d\n", err);
			goto err_i2c_rw;
		}

		near_far_state = flag_reg & STK_FLG_NF_MASK ? 1 : 0;
#ifdef CTTRACKING

		if (near_far_state == 0)
		{
			if (obj->ps > (obj->psi + STK_H_PS))
			{
				atomic_set(&obj->ps_high_thd_val, obj->psi + STK_H_HT);
				atomic_set(&obj->ps_low_thd_val, obj->psi + STK_H_LT);

				if ((err = stk3a5x_write_ps_high_thd(obj->client, atomic_read(&obj->ps_high_thd_val))))
				{
					APS_ERR("write high thd error: %d\n", err);
				}

				if ((err = stk3a5x_write_ps_low_thd(obj->client, atomic_read(&obj->ps_low_thd_val))))
				{
					APS_ERR("write high thd error: %d\n", err);
				}

				obj->ps_thd_update = true;
				APS_LOG("%s:ps update ps = 0x%x, psi = 0x%x\n", __FUNCTION__, obj->ps, obj->psi);
				APS_LOG("%s: update HT=%d, LT=%d\n", __func__, atomic_read(&obj->ps_high_thd_val), atomic_read(&obj->ps_low_thd_val));
			}
		}
		else
		{
			//APS_LOG("%s:ps thd update %d, ret %d\n",__FUNCTION__, obj->ps_thd_update, ret);
			if (obj->ps_thd_update)
			{
				err = stk3a5x_ps_val();

				if (err == 0)
				{
					atomic_set(&obj->ps_high_thd_val, obj->ps + stk3a5x_obj->stk_ht_n_ct);
					atomic_set(&obj->ps_low_thd_val, obj->ps + stk3a5x_obj->stk_lt_n_ct);

					if ((err = stk3a5x_write_ps_high_thd(obj->client, atomic_read(&obj->ps_high_thd_val))))
					{
						APS_ERR("write high thd error: %d\n", err);
					}

					if ((err = stk3a5x_write_ps_low_thd(obj->client, atomic_read(&obj->ps_low_thd_val))))
					{
						APS_ERR("write low thd error: %d\n", err);
					}

					APS_LOG("%s: Set HT=%d, LT=%d\n", __func__,
						atomic_read(&obj->ps_high_thd_val),
						atomic_read(&obj->ps_low_thd_val));
				}

				obj->ps_thd_update = false;
			}
		}

#endif
		stk_ps_int_handle(obj, near_far_state);
#if 1
		APS_LOG("%s: ps interrupt, show all reg\n", __FUNCTION__);
		show_allreg(); //for debug
#endif
	}

	if (disable_flag)
	{
		if ((err = stk3a5x_clear_intr(obj->client, flag_reg, disable_flag)))
		{
			APS_ERR("fail: %d\n", err);
			goto err_i2c_rw;
		}
	}

	msleep(1);
#if defined(CONFIG_OF)
	enable_irq(obj->irq);
#endif
	return;
err_i2c_rw:
	msleep(30);
#if defined(CONFIG_OF)
	enable_irq(obj->irq);
#endif
	return;
}

#if defined(CONFIG_OF)
static irqreturn_t stk3a5x_eint_handler(int irq, void *desc)
{
	printk("----------------stk3a5x_eint_handler-----------------------\n");
	disable_irq_nosync(stk3a5x_obj->irq);
	stk3a5x_eint_func();
	return IRQ_HANDLED;
}
#endif

/*----------------------------------------------------------------------------*/
int stk3a5x_setup_eint(struct i2c_client *client)
{
	int ret;
	//struct pinctrl *pinctrl;
	struct pinctrl_state *pins_default;
	struct pinctrl_state *pins_cfg;
	u32 ints[2] = { 0, 0 };
	//alspsPltFmDev = get_alsps_platformdev();
	/* gpio setting */
	//pinctrl = devm_pinctrl_get(&alspsPltFmDev->dev);
	pinctrl = devm_pinctrl_get(&client->dev);

	if (IS_ERR(pinctrl))
	{
		ret = PTR_ERR(pinctrl);
		APS_ERR("Cannot find alsps pinctrl!\n");
	}

	pins_default = pinctrl_lookup_state(pinctrl, "pin_default");

	if (IS_ERR(pins_default))
	{
		ret = PTR_ERR(pins_default);
		APS_ERR("Cannot find alsps pinctrl default!\n");
	}

	pins_cfg = pinctrl_lookup_state(pinctrl, "pin_cfg");

	if (IS_ERR(pins_cfg))
	{
		ret = PTR_ERR(pins_cfg);
		APS_ERR("Cannot find alsps pinctrl pin_cfg!\n");
	}
	
	pins_v3venable = pinctrl_lookup_state(pinctrl, "v3v_enable");

        if (IS_ERR(pins_v3venable))
        {
                ret = PTR_ERR(pins_v3venable);
                APS_ERR("Cannot find alsps pinctrl pin_v3venable!\n");
        }

	pins_v3vdisable = pinctrl_lookup_state(pinctrl, "v3v_disable");

        if (IS_ERR(pins_v3vdisable))
        {
                ret = PTR_ERR(pins_v3vdisable);
                APS_ERR("Cannot find alsps pinctrl pin_v3vdisable!\n");
        }

	//stk3a5x_obj->irq_node = client->dev.of_node;
		
	/* eint request */
	if (stk3a5x_obj->irq_node)
	{
		//node = of_find_matching_node(node, touch_of_match);
		pinctrl_select_state(pinctrl, pins_cfg);
#ifndef CONFIG_MTK_EIC
		/*upstream code*/
		ints[0] = of_get_named_gpio(stk3a5x_obj->irq_node, "deb-gpios", 0);
		if (ints[0] < 0) {
			APS_ERR("debounce gpio not found\n");
		} else{
			ret = of_property_read_u32(stk3a5x_obj->irq_node, "debounce", &ints[1]);
			if (ret < 0)
				APS_ERR("debounce time not found\n");
			else
				gpio_set_debounce(ints[0], ints[1]);
			APS_LOG("ints[0] = %d, ints[1] = %d!!\n", ints[0], ints[1]);
		}
#else
		ret = of_property_read_u32_array(stk3a5x_obj->irq_node, "debounce", ints, ARRAY_SIZE(ints));
		if (ret) {
			APS_ERR("of_property_read_u32_array fail, ret = %d\n", ret);
			return ret;
		}
		gpio_set_debounce(ints[0], ints[1]);
		pinctrl_select_state(pinctrl, pins_cfg);
		APS_LOG("ints[0] = %d, ints[1] = %d!!\n", ints[0], ints[1]);
#endif
		stk3a5x_obj->irq = irq_of_parse_and_map(stk3a5x_obj->irq_node, 0);
		APS_LOG("stk3a5x_obj->irq = %d\n", stk3a5x_obj->irq);

		if (!stk3a5x_obj->irq)
		{
			APS_ERR("irq_of_parse_and_map fail!!\n");
			return -EINVAL;
		}

		if (request_irq(stk3a5x_obj->irq, stk3a5x_eint_handler, IRQF_TRIGGER_LOW, "ALS-eint", NULL))
		{
			APS_ERR("IRQ LINE NOT AVAILABLE!!\n");
			return -EINVAL;
		}

		enable_irq_wake(stk3a5x_obj->irq);
	}
	else
	{
		APS_ERR("null irq node!!\n");
		return -EINVAL;
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
static int stk3a5x_init_client(struct i2c_client *client)
{
	struct stk3a5x_priv *obj = i2c_get_clientdata(client);
	int err;
	int ps_ctrl;
	//uint8_t buffer = 0x0;
	//u8 int_status;
	APS_LOG("%s: In\n", __FUNCTION__);

	if ((err = stk3a5x_write_sw_reset(client)))
	{
		APS_ERR("software reset error, err=%d", err);
		return err;
	}

	if ((err = stk3a5x_read_id(client)))
	{
		APS_ERR("stk3a5x_read_id error, err=%d", err);
		return err;
	}

	if (obj->first_boot == true)
	{
		if (obj->hw->polling_mode_ps == 0 || obj->hw->polling_mode_als == 0)
		{
			if ((err = stk3a5x_setup_eint(client)))
			{
				APS_ERR("setup eint error: %d\n", err);
				return err;
			}
		}
	}

	if ((err = stk3a5x_write_state(client, atomic_read(&obj->state_val))))
	{
		APS_ERR("write stete error: %d\n", err);
		return err;
	}

	/*
		if((err = stk3a5x_check_intr(client, &int_status)))
		{
			APS_ERR("check intr error: %d\n", err);
			return err;
		}

		if((err = stk3a5x_clear_intr(client, int_status, STK_FLG_PSINT_MASK | STK_FLG_ALSINT_MASK)))
		{
			APS_ERR("clear intr error: %d\n", err);
			return err;
		}
	*/
	ps_ctrl = atomic_read(&obj->psctrl_val);

	if (obj->hw->polling_mode_ps == 1)
		ps_ctrl &= 0x3F;

	if ((err = stk3a5x_write_ps(client, ps_ctrl)))
	{
		APS_ERR("write ps error: %d\n", err);
		return err;
	}

	if ((err = stk3a5x_write_als(client, atomic_read(&obj->alsctrl_val))))
	{
		APS_ERR("write als error: %d\n", err);
		return err;
	}

	if ((err = stk3a5x_write_led(client, obj->ledctrl_val)))
	{
		APS_ERR("write led error: %d\n", err);
		return err;
	}

	if ((err = stk3a5x_write_wait(client, obj->wait_val)))
	{
		APS_ERR("write wait error: %d\n", err);
		return err;
	}

	//disable by zhuhui, 20191002
#if 0
	//INTEL PEERS
	stk3a5x_master_send(client, STK_INTELLI_WAIT_PS_REG, &obj->intell_val, 1);

	//A GAIN default
	//stk3a5x_master_send(client, 0xDB, 0x15, 1);

	//PS INT mode
	buffer = 0x01;
	stk3a5x_master_send(client, 0xFA, &buffer, 1);

	buffer = 0x06;
	stk3a5x_master_send(client, 0x4E, &buffer, 1);

	//BGIR
	buffer = 0x10;
	stk3a5x_master_send(client, 0xA0, &buffer, 1);
	buffer = 0x64;
	stk3a5x_master_send(client, 0xAA, &buffer, 1);

	//Other
	buffer = 0x0F|0x70;
	stk3a5x_master_send(client, 0xA1, &buffer, 1);

	buffer = 0x01;
	stk3a5x_master_send(client, 0xDB, &buffer, 1);

	buffer = 0x82;
	stk3a5x_master_send(client, 0xF6, &buffer, 1);
#endif
#ifndef STK_TUNE0

	if ((err = stk3a5x_write_ps_high_thd(client, atomic_read(&obj->ps_high_thd_val))))
	{
		APS_ERR("write high thd error: %d\n", err);
		return err;
	}

	if ((err = stk3a5x_write_ps_low_thd(client, atomic_read(&obj->ps_low_thd_val))))
	{
		APS_ERR("write low thd error: %d\n", err);
		return err;
}

#endif


	if ((err = stk3a5x_write_int(client, obj->int_val)))
	{
		APS_ERR("write int mode error: %d\n", err);
		return err;
	}

	/*
	u8 data;
	data = 0x60;
	err = stk3a5x_master_send(client, 0x87, &data, 1);
	if (err < 0)
	{
		APS_ERR("write 0x87 = %d\n", err);
		return -EFAULT;
	}
	*/
#ifdef STK_ALS_MID_FIR
	memset(&obj->als_data_filter, 0x00, sizeof(obj->als_data_filter));
#endif
#ifdef STK_TUNE0

	if (obj->first_boot == true)
		stk_ps_tune_zero_init(obj);

#endif
	obj->re_enable_ps = false;
	obj->re_enable_als = false;
	obj->als_code_last = 100;

	obj->ps_distance_last = -1;
	obj->als_code_last = 100;
	return 0;
}

/******************************************************************************
 * Sysfs attributes
*******************************************************************************/
static ssize_t stk3a5x_show_config(struct device_driver *ddri, char *buf)
{
	ssize_t res;

	if (!stk3a5x_obj)
	{
		APS_ERR("stk3a5x_obj is null!!\n");
		return 0;
	}

	res = scnprintf(buf, PAGE_SIZE, "(%d %d %d %d %d %d)\n",
		atomic_read(&stk3a5x_obj->i2c_retry),
		atomic_read(&stk3a5x_obj->als_debounce),
		atomic_read(&stk3a5x_obj->ps_mask),
		atomic_read(&stk3a5x_obj->ps_high_thd_val),
		atomic_read(&stk3a5x_obj->ps_low_thd_val),
		atomic_read(&stk3a5x_obj->ps_debounce));
	return res;
}
/*----------------------------------------------------------------------------*/
static ssize_t stk3a5x_store_config(struct device_driver *ddri, const char *buf, size_t count)
{
	int retry, als_deb, ps_deb, mask, hthres, lthres, err;
	struct i2c_client *client;
	client = stk3a5x_i2c_client;

	if (!stk3a5x_obj)
	{
		APS_ERR("stk3a5x_obj is null!!\n");
		return 0;
	}

	if (6 == sscanf(buf, "%d %d %d %d %d %d", &retry, &als_deb, &mask, &hthres, &lthres, &ps_deb))
	{
		atomic_set(&stk3a5x_obj->i2c_retry, retry);
		atomic_set(&stk3a5x_obj->als_debounce, als_deb);
		atomic_set(&stk3a5x_obj->ps_mask, mask);
		atomic_set(&stk3a5x_obj->ps_high_thd_val, hthres);
		atomic_set(&stk3a5x_obj->ps_low_thd_val, lthres);
		atomic_set(&stk3a5x_obj->ps_debounce, ps_deb);

		if ((err = stk3a5x_write_ps_high_thd(client, atomic_read(&stk3a5x_obj->ps_high_thd_val))))
		{
			APS_ERR("write high thd error: %d\n", err);
			return err;
		}

		if ((err = stk3a5x_write_ps_low_thd(client, atomic_read(&stk3a5x_obj->ps_low_thd_val))))
		{
			APS_ERR("write low thd error: %d\n", err);
			return err;
		}
	}
	else
	{
		APS_ERR("invalid content: '%s'\n", buf);
	}

	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t stk3a5x_show_trace(struct device_driver *ddri, char *buf)
{
	ssize_t res;

	if (!stk3a5x_obj)
	{
		APS_ERR("stk3a5x_obj is null!!\n");
		return 0;
	}

	res = scnprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&stk3a5x_obj->trace));
	return res;
}
/*----------------------------------------------------------------------------*/
static ssize_t stk3a5x_store_trace(struct device_driver *ddri, const char *buf, size_t count)
{
	int trace;

	if (!stk3a5x_obj)
	{
		APS_ERR("stk3a5x_obj is null!!\n");
		return 0;
	}

	if (1 == sscanf(buf, "0x%x", &trace))
	{
		atomic_set(&stk3a5x_obj->trace, trace);
	}
	else
	{
		APS_ERR("invalid content: '%s', length = %d\n", buf, (int)count);
	}

	return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t stk3a5x_show_als(struct device_driver *ddri, char *buf)
{
	int res;

	if (!stk3a5x_obj)
	{
		APS_ERR("stk3a5x_obj is null!!\n");
		return 0;
	}

	if (stk3a5x_obj->hw->polling_mode_als == 0)
	{
		if ((res = stk3a5x_read_als(stk3a5x_obj->client, &stk3a5x_obj->als)))
			return scnprintf(buf, PAGE_SIZE, "ERROR: %d\n", res);
		else
			return scnprintf(buf, PAGE_SIZE, "%d\n", stk3a5x_obj->als);
	}

	return scnprintf(buf, PAGE_SIZE, "%d\n", stk3a5x_obj->als_code_last);
}
/*----------------------------------------------------------------------------*/
static ssize_t stk3a5x_show_ps(struct device_driver *ddri, char *buf)
{
	int res;

	if (!stk3a5x_obj)
	{
		APS_ERR("stk3a5x_obj is null!!\n");
		return 0;
	}

	if ((res = stk3a5x_read_ps(stk3a5x_obj->client, &stk3a5x_obj->ps)))
	{
		return scnprintf(buf, PAGE_SIZE, "ERROR: %d\n", res);
	}
	else
	{
		return scnprintf(buf, PAGE_SIZE, "%d\n", stk3a5x_obj->ps);
	}
}
/*----------------------------------------------------------------------------*/
static ssize_t stk3a5x_show_reg(struct device_driver *ddri, char *buf)
{
	u8 int_status;

	if (!stk3a5x_obj)
	{
		APS_ERR("stk3a5x_obj is null!!\n");
		return 0;
	}

	/*read*/
	stk3a5x_check_intr(stk3a5x_obj->client, &int_status);
	//stk3a5x_clear_intr(stk3a5x_obj->client, int_status, 0x0);
	stk3a5x_read_ps(stk3a5x_obj->client, &stk3a5x_obj->ps);
	stk3a5x_read_als(stk3a5x_obj->client, &stk3a5x_obj->als);
	/*write*/
	stk3a5x_write_als(stk3a5x_obj->client, atomic_read(&stk3a5x_obj->alsctrl_val));
	stk3a5x_write_ps(stk3a5x_obj->client, atomic_read(&stk3a5x_obj->psctrl_val));
	stk3a5x_write_ps_high_thd(stk3a5x_obj->client, atomic_read(&stk3a5x_obj->ps_high_thd_val));
	stk3a5x_write_ps_low_thd(stk3a5x_obj->client, atomic_read(&stk3a5x_obj->ps_low_thd_val));
	return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t stk3a5x_show_send(struct device_driver *ddri, char *buf)
{
	return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t stk3a5x_store_send(struct device_driver *ddri, const char *buf, size_t count)
{
	int addr, cmd;
	u8 dat;

	if (!stk3a5x_obj)
	{
		APS_ERR("stk3a5x_obj is null!!\n");
		return 0;
	}
	else if (2 != sscanf(buf, "%x %x", &addr, &cmd))
	{
		APS_ERR("invalid format: '%s'\n", buf);
		return 0;
	}

	dat = (u8)cmd;
	APS_LOG("send(%02X, %02X) = %d\n", addr, cmd,
		stk3a5x_master_send(stk3a5x_obj->client,
		(u16)addr, &dat, sizeof(dat)));
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t stk3a5x_show_recv(struct device_driver *ddri, char *buf)
{
	if (!stk3a5x_obj)
	{
		APS_ERR("stk3a5x_obj is null!!\n");
		return 0;
	}

	return scnprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&stk3a5x_obj->recv_reg));
}
/*----------------------------------------------------------------------------*/
static ssize_t stk3a5x_store_recv(struct device_driver *ddri, const char *buf, size_t count)
{
	int addr;
	u8 dat;

	if (!stk3a5x_obj)
	{
		APS_ERR("stk3a5x_obj is null!!\n");
		return 0;
	}
	else if (1 != sscanf(buf, "%x", &addr))
	{
		APS_ERR("invalid format: '%s'\n", buf);
		return 0;
	}

	APS_LOG("recv(%02X) = %d, 0x%02X\n", addr,
		stk3a5x_master_recv(stk3a5x_obj->client, (u16)addr, (char*)&dat, sizeof(dat)), dat);
	atomic_set(&stk3a5x_obj->recv_reg, dat);
	return count;
}
/*----------------------------------------------------------------------------*/

static ssize_t stk3a5x_show_allreg(struct device_driver *ddri, char *buf)
{
	int ret = 0;
	u8 rbuf[0x22];
	int cnt;
	int len = 0;
	memset(rbuf, 0, sizeof(rbuf));

	if (!stk3a5x_obj)
	{
		APS_ERR("stk3a5x_obj is null!!\n");
		return 0;
	}

	ret = stk3a5x_master_recv(stk3a5x_obj->client, 0, &rbuf[0], 7);

	if (ret < 0)
	{
		APS_DBG("error: %d\n", ret);
		return -EFAULT;
	}

	ret = stk3a5x_master_recv(stk3a5x_obj->client, 7, &rbuf[7], 7);

	if (ret < 0)
	{
		APS_DBG("error: %d\n", ret);
		return -EFAULT;
	}

	ret = stk3a5x_master_recv(stk3a5x_obj->client, 14, &rbuf[14], 7);

	if (ret < 0)
	{
		APS_DBG("error: %d\n", ret);
		return -EFAULT;
	}

	ret = stk3a5x_master_recv(stk3a5x_obj->client, 21, &rbuf[21], 7);

	if (ret < 0)
	{
		APS_DBG("error: %d\n", ret);
		return -EFAULT;
	}

	ret = stk3a5x_master_recv(stk3a5x_obj->client, 28, &rbuf[28], 4);

	if (ret < 0)
	{
		APS_DBG("error: %d\n", ret);
		return -EFAULT;
	}

	ret = stk3a5x_master_recv(stk3a5x_obj->client, STK_PDT_ID_REG, &rbuf[32], 2);

	if (ret < 0)
	{
		APS_DBG("error: %d\n", ret);
		return -EFAULT;
	}

	for (cnt = 0; cnt < 0x20; cnt++)
	{
		APS_LOG("reg[0x%x]=0x%x\n", cnt, rbuf[cnt]);
		len += scnprintf(buf + len, PAGE_SIZE - len, "[%2X]%2X,", cnt, rbuf[cnt]);
	}

	APS_LOG("reg[0x3E]=0x%x\n", rbuf[cnt]);
	APS_LOG("reg[0x3F]=0x%x\n", rbuf[cnt++]);
	len += scnprintf(buf + len, PAGE_SIZE - len, "[0x3E]%2X,[0x3F]%2X\n",
		rbuf[cnt - 1], rbuf[cnt]);
	return len;
	/*
	return scnprintf(buf, PAGE_SIZE, "[0]%2X [1]%2X [2]%2X [3]%2X [4]%2X [5]%2X [6/7 HTHD]%2X,%2X [8/9 LTHD]%2X, %2X [A]%2X [B]%2X [C]%2X [D]%2X [E/F Aoff]%2X,%2X,[10]%2X [11/12 PS]%2X,%2X [13]%2X [14]%2X [15/16 Foff]%2X,%2X [17]%2X [18]%2X [3E]%2X [3F]%2X\n",
		rbuf[0], rbuf[1], rbuf[2], rbuf[3], rbuf[4], rbuf[5], rbuf[6], rbuf[7], rbuf[8],
		rbuf[9], rbuf[10], rbuf[11], rbuf[12], rbuf[13], rbuf[14], rbuf[15], rbuf[16], rbuf[17],
		rbuf[18], rbuf[19], rbuf[20], rbuf[21], rbuf[22], rbuf[23], rbuf[24], rbuf[25], rbuf[26]);
	*/
}
/*----------------------------------------------------------------------------*/
static ssize_t stk3a5x_show_status(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	u8 rbuf[25];
	int ret = 0;

	if (!stk3a5x_obj)
	{
		APS_ERR("stk3a5x_obj is null!!\n");
		return 0;
	}

	//if (stk3a5x_obj->hw)
	//{
	len += scnprintf(buf + len, PAGE_SIZE - len,
		"CUST: %d, (%d %d) (%02X) (%02X %02X %02X) (%02X %02X %02X %02X)\n",
		stk3a5x_obj->hw->i2c_num,
		stk3a5x_obj->hw->power_id,
		stk3a5x_obj->hw->power_vol,
		stk3a5x_obj->addr.flag,
		stk3a5x_obj->addr.alsctrl,
		stk3a5x_obj->addr.data1_als,
		stk3a5x_obj->addr.data2_als,
		stk3a5x_obj->addr.psctrl,
		stk3a5x_obj->addr.data1_ps,
		stk3a5x_obj->addr.data2_ps,
		stk3a5x_obj->addr.thdh1_ps);
	//}
	//else
	//{
	//	len += scnprintf(buf + len, PAGE_SIZE - len, "CUST: NULL\n");
	//}

	len += scnprintf(buf + len, PAGE_SIZE - len,
		"REGS: %02X %02X %02X %02X %02X %02X %02X %02X %02lX %02lX\n",
		atomic_read(&stk3a5x_obj->state_val),
		atomic_read(&stk3a5x_obj->psctrl_val),
		atomic_read(&stk3a5x_obj->alsctrl_val),
		stk3a5x_obj->ledctrl_val, stk3a5x_obj->int_val,
		stk3a5x_obj->wait_val,
		atomic_read(&stk3a5x_obj->ps_high_thd_val),
		atomic_read(&stk3a5x_obj->ps_low_thd_val),
		stk3a5x_obj->enable,
		stk3a5x_obj->pending_intr);
#ifdef MT6516
	len += scnprintf(buf + len, PAGE_SIZE - len, "EINT: %d (%d %d %d %d)\n",
		mt_get_gpio_in(GPIO_ALS_EINT_PIN),
		CUST_EINT_ALS_NUM,
		CUST_EINT_ALS_POLARITY,
		CUST_EINT_ALS_DEBOUNCE_EN,
		CUST_EINT_ALS_DEBOUNCE_CN);
	len += scnprintf(buf + len, PAGE_SIZE - len, "GPIO: %d (%d %d %d %d)\n",
		GPIO_ALS_EINT_PIN,
		mt_get_gpio_dir(GPIO_ALS_EINT_PIN),
		mt_get_gpio_mode(GPIO_ALS_EINT_PIN),
		mt_get_gpio_pull_enable(GPIO_ALS_EINT_PIN),
		mt_get_gpio_pull_select(GPIO_ALS_EINT_PIN));
#endif
	len += scnprintf(buf + len, PAGE_SIZE - len, "MISC: %d %d\n",
		atomic_read(&stk3a5x_obj->als_suspend),
		atomic_read(&stk3a5x_obj->ps_suspend));
	len += scnprintf(buf + len, PAGE_SIZE - len, "VER.: %s\n",
		STK3A5X_DRIVER_VERSION);
	memset(rbuf, 0, sizeof(rbuf));
	ret = stk3a5x_master_recv(stk3a5x_obj->client, 0, &rbuf[0], 7);

	if (ret < 0)
	{
		APS_DBG("error: %d\n", ret);
		return -EFAULT;
	}

	ret = stk3a5x_master_recv(stk3a5x_obj->client, 7, &rbuf[7], 7);

	if (ret < 0)
	{
		APS_DBG("error: %d\n", ret);
		return -EFAULT;
	}

	ret = stk3a5x_master_recv(stk3a5x_obj->client, 14, &rbuf[14], 7);

	if (ret < 0)
	{
		APS_DBG("error: %d\n", ret);
		return -EFAULT;
	}

	/*
	ret = stk3a5x_master_recv(stk3a5x_obj->client, 21, &rbuf[21], 4);
	if(ret < 0)
	{
		APS_DBG("error: %d\n", ret);
		return -EFAULT;
	}
	*/
	len += scnprintf(buf + len, PAGE_SIZE - len, "[PS=%2X] [ALS=%2X] [WAIT=%4Xms] [EN_ASO=%2X] [EN_AK=%2X] [NEAR/FAR=%2X] [FLAG_OUI=%2X] [FLAG_PSINT=%2X] [FLAG_ALSINT=%2X]\n",
		rbuf[0] & 0x01, (rbuf[0] & 0x02) >> 1, ((rbuf[0] & 0x04) >> 2) * rbuf[5] * 6, (rbuf[0] & 0x20) >> 5,
		(rbuf[0] & 0x40) >> 6, rbuf[16] & 0x01, (rbuf[16] & 0x04) >> 2, (rbuf[16] & 0x10) >> 4, (rbuf[16] & 0x20) >> 5);
	return len;
}
/*----------------------------------------------------------------------------*/
#define IS_SPACE(CH) (((CH) == ' ') || ((CH) == '\n'))
/*----------------------------------------------------------------------------*/
static int read_int_from_buf(struct stk3a5x_priv *obj, const char* buf, size_t count,
	u32 data[], int len)
{
	int idx = 0;
	char *cur = (char*)buf, *end = (char*)(buf + count);

	while (idx < len)
	{
		while ((cur < end) && IS_SPACE(*cur))
		{
			cur++;
		}

		if (1 != sscanf(cur, "%d", &data[idx]))
		{
			break;
		}

		idx++;

		while ((cur < end) && !IS_SPACE(*cur))
		{
			cur++;
		}
	}

	return idx;
}
/*----------------------------------------------------------------------------*/
static ssize_t stk3a5x_show_alslv(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	int idx;

	if (!stk3a5x_obj)
	{
		APS_ERR("stk3a5x_obj is null!!\n");
		return 0;
	}

	for (idx = 0; idx < C_CUST_ALS_LEVEL/*stk3a5x_obj->als_level_num*/; idx++)
	{
		//len += scnprintf(buf + len, PAGE_SIZE - len, "%d ", stk3a5x_obj->hw->als_level[idx]);
		len += scnprintf(buf + len, PAGE_SIZE - len, "%d ", stk3a5x_obj->hw->als_level[ALS_LEVEL_TEMP_DEFAULT][ALS_LEVEL_TEMP_DEFAULT][idx]);
	}

	len += scnprintf(buf + len, PAGE_SIZE - len, "\n");
	return len;
}
/*----------------------------------------------------------------------------*/
static ssize_t stk3a5x_store_alslv(struct device_driver *ddri, const char *buf, size_t count)
{
	if (!stk3a5x_obj)
	{
		APS_ERR("stk3a5x_obj is null!!\n");
		return 0;
	}
/*
	else if (!strcmp(buf, "def"))
	{
		memcpy(stk3a5x_obj->als_level, stk3a5x_obj->hw->als_level, sizeof(stk3a5x_obj->als_level));
	}
	else if (stk3a5x_obj->als_level_num != read_int_from_buf(stk3a5x_obj, buf, count,
		stk3a5x_obj->hw->als_level[ALS_LEVEL_TEMP_DEFAULT][ALS_LEVEL_TEMP_DEFAULT], stk3a5x_obj->als_level_num))
	{
		APS_ERR("invalid format: '%s'\n", buf);
	}
*/
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t stk3a5x_show_alsval(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	int idx;

	if (!stk3a5x_obj)
	{
		APS_ERR("stk3a5x_obj is null!!\n");
		return 0;
	}

	for (idx = 0; idx < stk3a5x_obj->als_value_num; idx++)
	{
		len += scnprintf(buf + len, PAGE_SIZE - len, "%d ", stk3a5x_obj->hw->als_value[idx]);
	}

	len += scnprintf(buf + len, PAGE_SIZE - len, "\n");
	return len;
}
/*----------------------------------------------------------------------------*/
static ssize_t stk3a5x_store_alsval(struct device_driver *ddri, const char *buf, size_t count)
{
	if (!stk3a5x_obj)
	{
		APS_ERR("stk3a5x_obj is null!!\n");
		return 0;
	}
	else if (!strcmp(buf, "def"))
	{
		memcpy(stk3a5x_obj->als_value, stk3a5x_obj->hw->als_value, sizeof(stk3a5x_obj->als_value));
	}
	else if (stk3a5x_obj->als_value_num != read_int_from_buf(stk3a5x_obj, buf, count,
		stk3a5x_obj->hw->als_value, stk3a5x_obj->als_value_num))
	{
		APS_ERR("invalid format: '%s'\n", buf);
	}

	return count;
}

#ifdef STK_TUNE0
static ssize_t stk3a5x_show_cali(struct device_driver *ddri, char *buf)
{
	int32_t word_data;
	u8 r_buf[2];
	int ret;

	if (!stk3a5x_obj)
	{
		APS_ERR("stk3a5x_obj is null!!\n");
		return 0;
	}

	ret = stk3a5x_master_recv(stk3a5x_obj->client, 0x20, r_buf, 2);

	if (ret < 0)
	{
		APS_ERR("%s fail, err=0x%x", __FUNCTION__, ret);
		return ret;
	}

	word_data = (r_buf[0] << 8) | r_buf[1];
	ret = stk3a5x_master_recv(stk3a5x_obj->client, 0x22, r_buf, 2);

	if (ret < 0)
	{
		APS_ERR("%s fail, err=0x%x", __FUNCTION__, ret);
		return ret;
	}

	word_data += (r_buf[0] << 8) | r_buf[1];
	APS_LOG("%s: psi_set=%d, psa=%d,psi=%d, word_data=%d\n", __FUNCTION__,
		stk3a5x_obj->psi_set, stk3a5x_obj->psa, stk3a5x_obj->psi, word_data);
#ifdef CALI_EVERY_TIME
	APS_LOG("%s: boot HT=%d, LT=%d\n", __func__, stk3a5x_obj->ps_high_thd_boot, stk3a5x_obj->ps_low_thd_boot);
#endif
	return scnprintf(buf, PAGE_SIZE, "%5d\n", stk3a5x_obj->psi_set);
	//return 0;
}

static ssize_t stk3a5x_ps_maxdiff_store(struct device_driver *ddri, const char *buf, size_t count)
{
	unsigned long value = 0;
	int ret;

	if ((ret = kstrtoul(buf, 10, &value)) < 0)
	{
		printk(KERN_ERR "%s:kstrtoul failed, ret=0x%x\n", __func__, ret);
		return ret;
	}

	stk3a5x_obj->stk_max_min_diff = (int)value;
	return count;
}


static ssize_t stk3a5x_ps_maxdiff_show(struct device_driver *ddri, char *buf)
{
	if (!stk3a5x_obj)
	{
		APS_ERR("stk3a5x_obj is null!!\n");
		return 0;
	}

	return scnprintf(buf, PAGE_SIZE, "%d\n", stk3a5x_obj->stk_max_min_diff);
}

static ssize_t stk3a5x_ps_ltnct_store(struct device_driver *ddri, const char *buf, size_t count)
{
	unsigned long value = 0;
	int ret;

	if ((ret = kstrtoul(buf, 10, &value)) < 0)
	{
		printk(KERN_ERR "%s:kstrtoul failed, ret=0x%x\n", __func__, ret);
		return ret;
	}

	stk3a5x_obj->stk_lt_n_ct = (int)value;
	return count;
}

static ssize_t stk3a5x_ps_ltnct_show(struct device_driver *ddri, char *buf)
{
	if (!stk3a5x_obj)
	{
		APS_ERR("stk3a5x_obj is null!!\n");
		return 0;
	}

	return scnprintf(buf, PAGE_SIZE, "%d\n", stk3a5x_obj->stk_lt_n_ct);
}

static ssize_t stk3a5x_ps_htnct_store(struct device_driver *ddri, const char *buf, size_t count)
{
	unsigned long value = 0;
	int ret;

	if ((ret = kstrtoul(buf, 10, &value)) < 0)
	{
		printk(KERN_ERR "%s:kstrtoul failed, ret=0x%x\n", __func__, ret);
		return ret;
	}

	stk3a5x_obj->stk_ht_n_ct = (int)value;
	return count;
}

static ssize_t stk3a5x_ps_htnct_show(struct device_driver *ddri, char *buf)
{
	if (!stk3a5x_obj)
	{
		APS_ERR("stk3a5x_obj is null!!\n");
		return 0;
	}

	return scnprintf(buf, PAGE_SIZE, "%d\n", stk3a5x_obj->stk_ht_n_ct);
}

#endif	// #ifdef STK_TUNE0

/*---Offset At-------------------------------------------------------------------------*/
static ssize_t stk3a5x_show_ps_offset(struct device_driver *ddri, char *buf)
{
	if (!stk3a5x_obj)
	{
		APS_ERR("stk3a5x_obj is null!!\n");
		return 0;
	}

#ifdef STK_TUNE0
	return snprintf(buf, PAGE_SIZE, "%d\n", stk3a5x_obj->crosstalk);
#else
	return 0;
#endif
}
/*----------------------------------------------------------------------------*/
static ssize_t stk3a5x_set_ps_offset(struct device_driver *ddri, const char *buf, size_t count)
{
	int ret;
	ret = stk3a5x_ps_calibration(stk3a5x_obj->client);
	return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t stk3a5x_show_pthreshold_calibration(struct device_driver *ddri, char *buf)
{
	// struct stk3a5x_priv *data = i2c_get_clientdata(stk3a5x_obj->client);
#ifdef CALI_EVERY_TIME
	return snprintf(buf, PAGE_SIZE, "Low threshold = %d , High threshold = %d\n",
		stk3a5x_obj->ps_low_thd_boot, stk3a5x_obj->ps_high_thd_boot);
#else
	return 0;
#endif
}

static ssize_t stk3a5x_store_ps_offset(struct device_driver *ddri, const char *buf, size_t count)
{
#ifdef STK_TUNE0
	struct stk3a5x_priv *obj = stk3a5x_obj;
	int ps_cali_val = 0;
	//int threshold[2] = {0};
	sscanf(buf, "%d", &ps_cali_val);
	obj->crosstalk = ps_cali_val;
	if (obj->crosstalk == 0)
	{
		obj->crosstalk = obj->hw->ps_threshold_low;
		stk3a5x_calibration_flag = 0;
	}
	else
	{
		stk3a5x_calibration_flag = 1;
	}

	mutex_lock(&run_cali_mutex);
#ifdef CALI_EVERY_TIME
	obj->ps_high_thd_boot = obj->crosstalk + obj->stk_ht_n_ct;
	obj->ps_low_thd_boot = obj->crosstalk + obj->stk_lt_n_ct;
	printk("%s: crosstalk=%d, high thd=%d, low thd=%d\n",
		__func__, obj->crosstalk,
		obj->ps_high_thd_boot, obj->ps_low_thd_boot);
#endif
	mutex_unlock(&run_cali_mutex);
#endif
	return count;
}

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static DRIVER_ATTR(als, S_IWUSR | S_IRUGO, stk3a5x_show_als, NULL);
static DRIVER_ATTR(ps, S_IWUSR | S_IRUGO, stk3a5x_show_ps, NULL);
static DRIVER_ATTR(config, S_IWUSR | S_IRUGO, stk3a5x_show_config, stk3a5x_store_config);
static DRIVER_ATTR(alslv, S_IWUSR | S_IRUGO, stk3a5x_show_alslv, stk3a5x_store_alslv);
static DRIVER_ATTR(alsval, S_IWUSR | S_IRUGO, stk3a5x_show_alsval, stk3a5x_store_alsval);
static DRIVER_ATTR(trace, S_IWUSR | S_IRUGO, stk3a5x_show_trace, stk3a5x_store_trace);
static DRIVER_ATTR(status, S_IWUSR | S_IRUGO, stk3a5x_show_status, NULL);
static DRIVER_ATTR(send, S_IWUSR | S_IRUGO, stk3a5x_show_send, stk3a5x_store_send);
static DRIVER_ATTR(recv, S_IWUSR | S_IRUGO, stk3a5x_show_recv, stk3a5x_store_recv);
static DRIVER_ATTR(reg, S_IWUSR | S_IRUGO, stk3a5x_show_reg, NULL);
static DRIVER_ATTR(allreg, S_IWUSR | S_IRUGO, stk3a5x_show_allreg, NULL);
static DRIVER_ATTR(pscalibration, S_IWUSR | S_IRUGO, stk3a5x_show_ps_offset, stk3a5x_set_ps_offset);
static DRIVER_ATTR(pthredcalibration, S_IWUSR | S_IRUGO, stk3a5x_show_pthreshold_calibration, NULL);
#ifdef STK_TUNE0
static DRIVER_ATTR(cali, S_IWUSR | S_IRUGO, stk3a5x_show_cali, NULL);
static DRIVER_ATTR(maxdiff, S_IWUSR | S_IRUGO, stk3a5x_ps_maxdiff_show, stk3a5x_ps_maxdiff_store);
static DRIVER_ATTR(ltnct, S_IWUSR | S_IRUGO, stk3a5x_ps_ltnct_show, stk3a5x_ps_ltnct_store);
static DRIVER_ATTR(htnct, S_IWUSR | S_IRUGO, stk3a5x_ps_htnct_show, stk3a5x_ps_htnct_store);
#endif
static DRIVER_ATTR(ps_offset, S_IWUSR | S_IRUGO, NULL, stk3a5x_store_ps_offset);
/*----------------------------------------------------------------------------*/
static struct driver_attribute *stk3a5x_attr_list[] =
{
	&driver_attr_als,
	&driver_attr_ps,
	&driver_attr_trace,        /*trace log*/
	&driver_attr_config,
	&driver_attr_alslv,
	&driver_attr_alsval,
	&driver_attr_status,
	&driver_attr_send,
	&driver_attr_recv,
	&driver_attr_allreg,
	&driver_attr_reg,
	&driver_attr_pscalibration,
	&driver_attr_pthredcalibration,
#ifdef STK_TUNE0
	&driver_attr_cali,
	&driver_attr_maxdiff,
	&driver_attr_ltnct,
	&driver_attr_htnct,
#endif
	&driver_attr_ps_offset,
};

/*----------------------------------------------------------------------------*/
static int stk3a5x_create_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(sizeof(stk3a5x_attr_list) / sizeof(stk3a5x_attr_list[0]));

	if (driver == NULL)
	{
		return -EINVAL;
	}

	for (idx = 0; idx < num; idx++)
	{
		if ((err = driver_create_file(driver, stk3a5x_attr_list[idx])))
		{
			APS_ERR("driver_create_file (%s) = %d\n", stk3a5x_attr_list[idx]->attr.name, err);
			break;
		}
	}

	return err;
}
/*----------------------------------------------------------------------------*/
static int stk3a5x_delete_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(sizeof(stk3a5x_attr_list) / sizeof(stk3a5x_attr_list[0]));

	if (!driver)
		return -EINVAL;

	for (idx = 0; idx < num; idx++)
	{
		driver_remove_file(driver, stk3a5x_attr_list[idx]);
	}

	return err;
}

/******************************************************************************
 * Function Configuration
******************************************************************************/

static int stk3a5x_get_als_value(struct stk3a5x_priv *obj, u16 als)
{
	int idx;
	int invalid = 0;
	unsigned int lum;

#if 0
	for (idx = 0; idx < obj->als_level_num; idx++)
	{
		if (als < obj->hw->als_level[ALS_LEVEL_TEMP_DEFAULT][ALS_LEVEL_TEMP_DEFAULT][idx])
		{
			break;
		}
	}

	if (idx >= obj->als_value_num)
	{
		APS_ERR("exceed range\n");
		idx = obj->als_value_num - 1;
	}

	if (1 == atomic_read(&obj->als_deb_on))
	{
		unsigned long endt = atomic_read(&obj->als_deb_end);

		if (time_after(jiffies, endt))
		{
			atomic_set(&obj->als_deb_on, 0);
		}

		if (1 == atomic_read(&obj->als_deb_on))
		{
			invalid = 1;
		}
	}

	if (!invalid)
	{
		if (atomic_read(&obj->trace) & STK_TRC_CVT_ALS)
		{
			APS_DBG("ALS: %05d => %05d\n", als, obj->hw->als_value[idx]);
		}

		return obj->hw->als_value[idx];
	}
	else
	{
		if (atomic_read(&obj->trace) & STK_TRC_CVT_ALS)
		{
			APS_DBG("ALS: %05d => %05d (-1)\n", als, obj->hw->als_value[idx]);
		}

		return -1;
	}
#else
	for(idx = 0; idx < C_CUST_ALS_LEVEL; idx++)
	{
		if(als < als_level[current_tp][current_color_temp][idx])
		{
			//APS_DBG("als=%d ,als_level = %d\n",als,als_level[current_tp][current_color_temp][idx]);
			break;
		}
	}

	if(idx >= C_CUST_ALS_LEVEL)
	{
		APS_ERR("exceed range  idx =%d\n",idx);
		idx = C_CUST_ALS_LEVEL - 1;
	}

	if(1 == atomic_read(&obj->als_deb_on))
	{
		unsigned long endt = atomic_read(&obj->als_deb_end);
		if(time_after(jiffies, endt))
		{
			atomic_set(&obj->als_deb_on, 0);
		}

		if(1 == atomic_read(&obj->als_deb_on))
		{
			invalid = 1;
		}
	}

	if(!invalid)
	{
		lum=(als_value[idx]-als_value[idx-1])*(als-als_level[current_tp][current_color_temp][idx-1])/
                        (als_level[current_tp][current_color_temp][idx]-als_level[current_tp][current_color_temp][idx-1]);
		lum += als_value[idx-1];

		//APS_DBG("ALS: %05d => %05d\n", als, lum);

		return lum;
	}
	else
	{
		if(atomic_read(&obj->trace) & STK_TRC_CVT_ALS)
		{
			APS_ERR("ALS: %05d => %05d (-1)\n", als, als_value[idx]);//obj->hw->als_value[idx]
		}
		return -1;
	}
#endif
}
/*----------------------------------------------------------------------------*/
static int stk3a5x_get_ps_value_only(struct stk3a5x_priv *obj, u16 ps)
{
	int mask = atomic_read(&obj->ps_mask);
	int invalid = 0, val;
	int err;
	u8 flag;
	err = stk3a5x_read_flag(obj->client, &flag);

	if (err)
		return err;

	val = (flag & STK_FLG_NF_MASK) ? 1 : 0;

	if (atomic_read(&obj->ps_suspend))
	{
		invalid = 1;
	}
	else if (1 == atomic_read(&obj->ps_deb_on))
	{
		unsigned long endt = atomic_read(&obj->ps_deb_end);

		if (time_after(jiffies, endt))
		{
			atomic_set(&obj->ps_deb_on, 0);
		}

		if (1 == atomic_read(&obj->ps_deb_on))
		{
			invalid = 1;
		}
	}

	if (!invalid)
	{
		if (unlikely(atomic_read(&obj->trace) & STK_TRC_CVT_PS))
		{
			if (mask)
			{
				APS_DBG("PS:  %05d => %05d [M] \n", ps, val);
			}
			else
			{
				APS_DBG("PS:  %05d => %05d\n", ps, val);
			}
		}

		return val;
	}
	else
	{
		APS_ERR(" ps value is invalid, PS:  %05d => %05d\n", ps, val);

		if (unlikely(atomic_read(&obj->trace) & STK_TRC_CVT_PS))
		{
			APS_DBG("PS:  %05d => %05d (-1)\n", ps, val);
		}

		return -1;
	}
}
/*----------------------------------------------------------------------------*/
static int stk3a5x_get_ps_value(struct stk3a5x_priv *obj, u16 ps)
{
	int mask = atomic_read(&obj->ps_mask);
	int invalid = 0, val;
	int err;
	u8 flag;
	err = stk3a5x_read_flag(obj->client, &flag);

	if (err)
		return err;

	val = (flag & STK_FLG_NF_MASK) ? 1 : 0;
	APS_LOG("%s: read_flag = 0x%x, val = %d\n", __FUNCTION__, flag, val);

	/*
	if((err = stk3a5x_clear_intr(obj->client, flag, STK_FLG_OUI_MASK)))
	{
		APS_ERR("fail: %d\n", err);
		return err;
	}
	*/
	if (atomic_read(&obj->ps_suspend))
	{
		invalid = 1;
	}
	else if (1 == atomic_read(&obj->ps_deb_on))
	{
		unsigned long endt = atomic_read(&obj->ps_deb_end);

		if (time_after(jiffies, endt))
		{
			atomic_set(&obj->ps_deb_on, 0);
		}

		if (1 == atomic_read(&obj->ps_deb_on))
		{
			invalid = 1;
		}
	}

	if (!invalid)
	{
		if (unlikely(atomic_read(&obj->trace) & STK_TRC_CVT_PS))
		{
			if (mask)
			{
				APS_DBG("PS:  %05d => %05d [M] \n", ps, val);
			}
			else
			{
				APS_DBG("PS:  %05d => %05d\n", ps, val);
			}
		}

		return val;
	}
	else
	{
		APS_ERR(" ps value is invalid, PS:  %05d => %05d\n", ps, val);

		if (unlikely(atomic_read(&obj->trace) & STK_TRC_CVT_PS))
		{
			APS_DBG("PS:  %05d => %05d (-1)\n", ps, val);
		}

		return -1;
	}
}


/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static int stk3a5x_i2c_suspend(struct device *dev) 
{
	int err;
	struct i2c_client *client = to_i2c_client(dev);
	struct stk3a5x_priv *obj = i2c_get_clientdata(client);
	

	APS_LOG("stk3a5x_i2c_suspend\n");

	if (!obj) {
		APS_ERR("null pointer!!\n");
		return 0;
	}

	atomic_set(&obj->als_suspend, 1);
	err = stk3a5x_enable_als(obj->client, 0);
	if (err)
		APS_ERR("disable als fail: %d\n", err);

	return 0;
}
/*----------------------------------------------------------------------------*/
static int stk3a5x_i2c_resume(struct device *dev)
{
	int err;
	struct i2c_client *client = to_i2c_client(dev);
   	struct stk3a5x_priv *obj = i2c_get_clientdata(client);
	
	APS_LOG("stk3a5x_i2c_resume\n");
	if (!obj) {
		APS_ERR("null pointer!!\n");
		return 0;
	}

	atomic_set(&obj->als_suspend, 0);
	if (test_bit(STK_BIT_ALS, &obj->enable)) {
		err = stk3a5x_enable_als(obj->client, 1);
		if (err)
			APS_ERR("enable als fail: %d\n", err);
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
static int stk3a5x_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	strcpy(info->type, stk3a5x_DEV_NAME);
	return 0;
}

static int als_open_report_data(int open)
{
	//should queuq work to report event if  is_report_input_direct=true
	return 0;
}

// if use  this typ of enable , Gsensor only enabled but not report inputEvent to HAL

static int als_enable_nodata(int en)
{
	int res = 0;
#ifdef CUSTOM_KERNEL_SENSORHUB
	SCP_SENSOR_HUB_DATA req;
	int len;
#endif //#ifdef CUSTOM_KERNEL_SENSORHUB
	APS_LOG("stk3a5x_obj als enable value = %d\n", en);
#ifdef CUSTOM_KERNEL_SENSORHUB
	req.activate_req.sensorType = ID_LIGHT;
	req.activate_req.action = SENSOR_HUB_ACTIVATE;
	req.activate_req.enable = en;
	len = sizeof(req.activate_req);
	res = SCP_sensorHub_req_send(&req, &len, 1);
#else //#ifdef CUSTOM_KERNEL_SENSORHUB

	if (!stk3a5x_obj)
	{
		APS_ERR("stk3a5x_obj is null!!\n");
		return -1;
	}

	res = stk3a5x_enable_als(stk3a5x_obj->client, en);
#endif //#ifdef CUSTOM_KERNEL_SENSORHUB

	if (res)
	{
		APS_ERR("als_enable_nodata is failed!!\n");
		return -1;
	}

	return 0;
}

static int als_set_delay(u64 ns)
{
	return 0;
}

static int als_batch(int flag, int64_t samplingPeriodNs, int64_t maxBatchReportLatencyNs)
{
	return als_set_delay(samplingPeriodNs);
}

static int als_flush(void)
{
	return als_flush_report();
}

static int als_get_data(int* value, int* status)
{
	int err = 0;
#ifdef CUSTOM_KERNEL_SENSORHUB
	SCP_SENSOR_HUB_DATA req;
	int len;
#else
	struct stk3a5x_priv *obj = NULL;
#endif //#ifdef CUSTOM_KERNEL_SENSORHUB
#ifdef CUSTOM_KERNEL_SENSORHUB
	req.get_data_req.sensorType = ID_LIGHT;
	req.get_data_req.action = SENSOR_HUB_GET_DATA;
	len = sizeof(req.get_data_req);
	err = SCP_sensorHub_req_send(&req, &len, 1);

	if (err)
	{
		APS_ERR("SCP_sensorHub_req_send fail!\n");
	}
	else
	{
		*value = req.get_data_rsp.int16_Data[0];
		*status = SENSOR_STATUS_ACCURACY_MEDIUM;
	}

	if (atomic_read(&stk3a5x_obj->trace) & CMC_TRC_PS_DATA)
	{
		APS_LOG("value = %d\n", *value);
		//show data
	}

#else //#ifdef CUSTOM_KERNEL_SENSORHUB

	if (!stk3a5x_obj)
	{
		APS_ERR("stk3a5x_obj is null!!\n");
		return -1;
	}

	obj = stk3a5x_obj;

	if ((err = stk3a5x_read_als(obj->client, &obj->als)))
	{
		err = -1;
	}
	else
	{
		if (obj->als < 3)
		{
			obj->als_last = obj->als;
			*value = stk3a5x_get_als_value(obj, 0);
		}
		else if (abs(obj->als - obj->als_last) >= STK_ALS_CODE_CHANGE_THD)
		{
			obj->als_last = obj->als;
			*value = stk3a5x_get_als_value(obj, obj->als);
		}
		else
		{
			*value = stk3a5x_get_als_value(obj, obj->als_last);
		}

		*status = SENSOR_STATUS_ACCURACY_MEDIUM;
	}

	obj->als_debug_count ++;
	if(obj->als_debug_count > 50000){
		obj->als_debug_count = 0;
	}else if((obj->als_debug_count%10)==5){
		APS_ERR("ALS: als=%d, lux=%d\n", obj->als, (*value));
	}
#endif //#ifdef CUSTOM_KERNEL_SENSORHUB
	return err;
}

static int ps_open_report_data(int open)
{
	//should queuq work to report event if  is_report_input_direct=true
	return 0;
}

// if use  this typ of enable , Gsensor only enabled but not report inputEvent to HAL

static int ps_enable_nodata(int en)
{
	int res = 0;
	//struct stk3a5x_priv *obj = i2c_get_clientdata(client);
#ifdef CUSTOM_KERNEL_SENSORHUB
	SCP_SENSOR_HUB_DATA req;
	int len;
#endif //#ifdef CUSTOM_KERNEL_SENSORHUB
	APS_LOG("stk3a5x_obj als enable value = %d\n", en);
#ifdef CUSTOM_KERNEL_SENSORHUB
	req.activate_req.sensorType = ID_PROXIMITY;
	req.activate_req.action = SENSOR_HUB_ACTIVATE;
	req.activate_req.enable = en;
	len = sizeof(req.activate_req);
	res = SCP_sensorHub_req_send(&req, &len, 1);
#else //#ifdef CUSTOM_KERNEL_SENSORHUB

	if (!stk3a5x_obj)
	{
		APS_ERR("stk3a5x_obj is null!!\n");
		return -1;
	}

	res = stk3a5x_enable_ps(stk3a5x_obj->client, en, 1);

	if (res)
	{
		APS_ERR("als_enable_nodata is failed!!\n");
		return -1;
	}

#endif //#ifdef CUSTOM_KERNEL_SENSORHUB
	return 0;
}

static int ps_set_delay(u64 ns)
{
	return 0;
}

static int ps_batch(int flag, int64_t samplingPeriodNs, int64_t maxBatchReportLatencyNs)
{
	return 0;
}

static int ps_flush(void)
{
	return ps_flush_report();
}

static int ps_get_data(int* value, int* status)
{
	int err = 0;
#ifdef CUSTOM_KERNEL_SENSORHUB
	SCP_SENSOR_HUB_DATA req;
	int len;
#endif //#ifdef CUSTOM_KERNEL_SENSORHUB
#ifdef CUSTOM_KERNEL_SENSORHUB
	req.get_data_req.sensorType = ID_PROXIMITY;
	req.get_data_req.action = SENSOR_HUB_GET_DATA;
	len = sizeof(req.get_data_req);
	err = SCP_sensorHub_req_send(&req, &len, 1);

	if (err)
	{
		APS_ERR("SCP_sensorHub_req_send fail!\n");
	}
	else
	{
		*value = req.get_data_rsp.int16_Data[0];
		*status = SENSOR_STATUS_ACCURACY_MEDIUM;
	}

	if (atomic_read(&stk3a5x_obj->trace) & CMC_TRC_PS_DATA)
	{
		APS_LOG("value = %d\n", *value);
		//show data
	}

#else //#ifdef CUSTOM_KERNEL_SENSORHUB

	if (!stk3a5x_obj)
	{
		APS_ERR("stk3a5x_obj is null!!\n");
		return -1;
	}

	if ((err = stk3a5x_read_ps(stk3a5x_obj->client, &stk3a5x_obj->ps)))
	{
		err = -1;;
	}
	else
	{
		*value = stk3a5x_get_ps_value(stk3a5x_obj, stk3a5x_obj->ps);
		*status = SENSOR_STATUS_ACCURACY_MEDIUM;
	}

#endif //#ifdef CUSTOM_KERNEL_SENSORHUB
	return 0;
}


static int stk3a5x_als_factory_enable_sensor(bool enable_disable, int64_t sample_periods_ms)
{
	int err = 0;
	APS_FUN();
	
	err = als_enable_nodata(enable_disable ? 1 : 0);
	if (err) {
		APS_ERR("%s:%s failed\n", __func__, enable_disable ? "enable" : "disable");
		return -1;
	}
	err = als_batch(0, sample_periods_ms * 1000000, 0);
	if (err) {
		APS_ERR("%s set_batch failed\n", __func__);
		return -1;
	}
	return 0;
}

static int stk3a5x_als_factory_get_data(int32_t *data)
{
	int status;
	
	APS_FUN();
	return als_get_data(data, &status);
}

static int stk3a5x_als_factory_get_raw_data(int32_t *data)
{
	int err = 0;
	struct stk3a5x_priv *obj = stk3a5x_obj;

	APS_FUN();

	if (!obj) {
		APS_ERR("obj is null!!\n");
		return -1;
	}

	err = stk3a5x_read_als(obj->client, &obj->als);
	if (err) {
		APS_ERR("%s failed\n", __func__);
		return -1;
	}
	*data = stk3a5x_obj->als;

	return 0;
}
static int stk3a5x_als_factory_enable_calibration(void)
{
	APS_FUN();
	return 0;
}
static int stk3a5x_als_factory_clear_cali(void)
{
	APS_FUN();
	return 0;
}
static int stk3a5x_als_factory_set_cali(int32_t offset)
{
	APS_FUN();
	return 0;
}
static int stk3a5x_als_factory_get_cali(int32_t *offset)
{
	APS_FUN();
	return 0;
}
static int stk3a5x_ps_factory_enable_sensor(bool enable_disable, int64_t sample_periods_ms)
{
	int err = 0;
	
	APS_FUN();
	err = ps_enable_nodata(enable_disable ? 1 : 0);
	if (err) {
		APS_ERR("%s:%s failed\n", __func__, enable_disable ? "enable" : "disable");
		return -1;
	}
	err = ps_batch(0, sample_periods_ms * 1000000, 0);
	if (err) {
		APS_ERR("%s set_batch failed\n", __func__);
		return -1;
	}
	return err;
}
static int stk3a5x_ps_factory_get_data(int32_t *data)
{
	int err = 0, status = 0;

	APS_FUN();
	err = ps_get_data(data, &status);
	if (err < 0)
		return -1;
	return 0;
}
static int stk3a5x_ps_factory_get_raw_data(int32_t *data)
{
	int err = 0;
	struct stk3a5x_priv *obj = stk3a5x_obj;

	APS_FUN();
	err = stk3a5x_read_ps(obj->client, &obj->ps);
	if (err) {
		APS_ERR("%s failed\n", __func__);
		return -1;
	}
	*data = stk3a5x_obj->ps;
	return 0;
}
static int stk3a5x_ps_factory_enable_calibration(void)
{
	APS_FUN();
	return 0;
}
static int stk3a5x_ps_factory_clear_cali(void)
{
	struct stk3a5x_priv *obj = stk3a5x_obj;

	APS_FUN();
	obj->ps_cali = 0;
	return 0;
}
static int stk3a5x_ps_factory_set_cali(int32_t offset)
{
	struct stk3a5x_priv *obj = stk3a5x_obj;

	APS_FUN();
	obj->ps_cali = offset;
	return 0;
}
static int stk3a5x_ps_factory_get_cali(int32_t *offset)
{
	struct stk3a5x_priv *obj = stk3a5x_obj;

	APS_FUN();
	*offset = obj->ps_cali;
	return 0;
}
#if 0
static int stk3a5x_ps_factory_set_threashold(int32_t threshold[2])
{
	int err = 0;
	struct stk3a5x_priv *obj = stk3a5x_obj;

	APS_FUN();
	APS_ERR("%s set threshold high: 0x%x, low: 0x%x, ps_cali=%d\n", __func__, threshold[0], threshold[1], obj->ps_cali);
	atomic_set(&obj->ps_high_thd_val, (threshold[0] + obj->ps_cali));
	atomic_set(&obj->ps_low_thd_val, (threshold[1] + obj->ps_cali));
	err = stk3a5x_write_ps_high_thd(obj->client, atomic_read(&obj->ps_high_thd_val));
	err = stk3a5x_write_ps_low_thd(obj->client, atomic_read(&obj->ps_low_thd_val));
	//err = set_psensor_threshold(obj->client);

	if (err < 0) {
		APS_ERR("set_psensor_threshold fail\n");
		return -1;
	}
	return 0;
}
static int stk3a5x_ps_factory_get_threashold(int32_t threshold[2])
{
	struct stk3a5x_priv *obj = stk3a5x_obj;

	APS_FUN();
	threshold[0] = atomic_read(&obj->ps_high_thd_val) - obj->ps_cali;
	threshold[1] = atomic_read(&obj->ps_low_thd_val) - obj->ps_cali;
	return 0;
}
#endif
static struct alsps_factory_fops stk3a5x_factory_fops = {
	.als_enable_sensor = stk3a5x_als_factory_enable_sensor,
	.als_get_data = stk3a5x_als_factory_get_data,
	.als_get_raw_data = stk3a5x_als_factory_get_raw_data,
	.als_enable_calibration = stk3a5x_als_factory_enable_calibration,
	.als_clear_cali = stk3a5x_als_factory_clear_cali,
	.als_set_cali = stk3a5x_als_factory_set_cali,
	.als_get_cali = stk3a5x_als_factory_get_cali,

	.ps_enable_sensor = stk3a5x_ps_factory_enable_sensor,
	.ps_get_data = stk3a5x_ps_factory_get_data,
	.ps_get_raw_data = stk3a5x_ps_factory_get_raw_data,
	.ps_enable_calibration = stk3a5x_ps_factory_enable_calibration,
	.ps_clear_cali = stk3a5x_ps_factory_clear_cali,
	.ps_set_cali = stk3a5x_ps_factory_set_cali,
	.ps_get_cali = stk3a5x_ps_factory_get_cali,
	//.ps_set_threashold = stk3a5x_ps_factory_set_threashold,
	//.ps_get_threashold = stk3a5x_ps_factory_get_threashold,
};

static struct alsps_factory_public stk3a5x_factory_device = {
	.gain = 1,
	.sensitivity = 1,
	.fops = &stk3a5x_factory_fops,
};

static int stk3a5x_init_reg(struct i2c_client *client)
{
	struct stk3a5x_priv *obj;
	uint8_t i2c_config_reg = 0;

	obj = stk3a5x_obj;

	obj->client = client;
	i2c_set_clientdata(client, obj);
	atomic_set(&obj->als_debounce, 55);//reference IT
	atomic_set(&obj->als_deb_on, 0);
	atomic_set(&obj->als_deb_end, 0);
	atomic_set(&obj->ps_debounce, 10);
	atomic_set(&obj->ps_deb_on, 0);
	atomic_set(&obj->ps_deb_end, 0);
	atomic_set(&obj->ps_mask, 0);
	atomic_set(&obj->trace, 0x00);
	atomic_set(&obj->als_suspend, 0);
	atomic_set(&obj->init_done, 0);
	//obj->irq_node = of_find_matching_node(obj->irq_node, alsps_of_match);
	//obj->irq_node = of_find_compatible_node(NULL, NULL, "mediatek, ALS-eint");
	//obj->irq_node = of_find_compatible_node(NULL, NULL, "mediatek,als_ps");
	obj->irq_node = client->dev.of_node;
	atomic_set(&obj->state_val, 0);

	i2c_config_reg = STK_PS_PRS2 | STK_PS_GAIN8 | STK_PS_IT100;
	atomic_set(&obj->psctrl_val, i2c_config_reg);

	i2c_config_reg = STK_ALS_PRS1 | STK_ALS_GAIN4 | STK_ALS_IT50;
	atomic_set(&obj->alsctrl_val, i2c_config_reg);

	i2c_config_reg = STK_ALSC_GAIN4;
	atomic_set(&obj->alsctrl2_val, i2c_config_reg);

	obj->ledctrl_val = STK_LED_100mA; // 0x60 = 100mA
	
	obj->wait_val = 0x1F; //STK_WAIT50;
	obj->int_val = STK_INT_PS_MODE1;
	obj->intell_val = STK_INTELL_25;



	obj->first_boot = true;
	obj->als_correct_factor = 1000;
	atomic_set(&obj->ps_high_thd_val, obj->hw->ps_threshold_high);//  obj->hw.ps_high_thd_val
	atomic_set(&obj->ps_low_thd_val, obj->hw->ps_threshold_low);
	atomic_set(&obj->recv_reg, 0);

	return 0;
}

/*----------------------------------------------------------------------------*/
static int stk3a5x_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct stk3a5x_priv *obj;
	int err = 0;
	struct als_control_path als_ctl = { 0 };
	struct als_data_path als_data = { 0 };
	struct ps_control_path ps_ctl = { 0 };
	struct ps_data_path ps_data = { 0 };
	APS_LOG("%s: driver version: %s\n", __FUNCTION__, STK3A5X_DRIVER_VERSION);



	if (!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
	{
		err = -ENOMEM;
		goto exit;
	}

	memset(obj, 0, sizeof(*obj));
	
	err = get_alsps_dts_func(client->dev.of_node, hw);
	if (err < 0)
	{
		APS_ERR("get customization info from dts failed\n");
		goto exit_init_failed;
	}

	stk3a5x_obj = obj;
	obj->hw = hw;
	stk3a5x_get_addr(&obj->addr);
	INIT_DELAYED_WORK(&obj->eint_work, stk3a5x_eint_work);
	printk("stk client->addr======%x\n",client->addr);
	stk3a5x_init_reg(client);

#ifdef CTTRACKING
	obj->ps_thd_update = false;
#endif
#ifdef STK_TUNE0
	obj->crosstalk = 0;
	obj->stk_max_min_diff = STK_MAX_MIN_DIFF;
	obj->stk_lt_n_ct = STK_LT_N_CT;
	obj->stk_ht_n_ct = STK_HT_N_CT;
#ifdef CALI_EVERY_TIME
	obj->ps_high_thd_boot = obj->hw->ps_threshold_high;
	obj->ps_low_thd_boot = obj->hw->ps_threshold_low;

	if (obj->ps_high_thd_boot <= 0)
	{
		obj->ps_high_thd_boot = obj->stk_ht_n_ct * 3;
		obj->ps_low_thd_boot = obj->stk_lt_n_ct * 3;
	}

#endif
#endif

	if (obj->hw->polling_mode_ps == 0)
	{
		APS_LOG("%s: enable PS interrupt\n", __FUNCTION__);
	}else{
		APS_LOG("%s: enable PS polling\n", __FUNCTION__);
	}

	obj->int_val |= STK_INT_PS_MODE1;

	if (obj->hw->polling_mode_als == 0)
	{
		obj->int_val |= STK_INT_ALS;
		APS_LOG("%s: enable ALS interrupt\n", __FUNCTION__);
	}

	obj->enable = 0;
	obj->pending_intr = 0;
/*	
	obj->als_level_num = sizeof(obj->hw->als_level) / sizeof(obj->hw->als_level[0]);
	obj->als_value_num = sizeof(obj->hw->als_value) / sizeof(obj->hw->als_value[0]);
	BUG_ON(sizeof(obj->als_level) != sizeof(obj->hw->als_level));
	memcpy(obj->als_level, obj->hw->als_level, sizeof(obj->als_level));
	BUG_ON(sizeof(obj->als_value) != sizeof(obj->hw->als_value));
	memcpy(obj->als_value, obj->hw->als_value, sizeof(obj->als_value));
*/
	BUG_ON(sizeof(als_level) != sizeof(obj->hw->als_level));
	memcpy(als_level, obj->hw->als_level, sizeof(als_level));
	BUG_ON(sizeof(als_value) != sizeof(obj->hw->als_value));
	memcpy(als_value, obj->hw->als_value, sizeof(als_value));
	
	atomic_set(&obj->i2c_retry, 3);

	if (atomic_read(&obj->state_val) & STK_STATE_EN_ALS_MASK)
	{
		set_bit(STK_BIT_ALS, &obj->enable);
	}

	if (atomic_read(&obj->state_val) & STK_STATE_EN_PS_MASK)
	{
		set_bit(STK_BIT_PS, &obj->enable);
	}

	stk3a5x_i2c_client = client;
	obj->stk_ps_tune0_wq = create_singlethread_workqueue("stk_ps_tune0_wq");
	INIT_WORK(&obj->stk_ps_tune0_work, stk_ps_tune0_work_func);
	hrtimer_init(&obj->ps_tune0_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
#ifdef STK_TUNE0
	obj->ps_tune0_delay = ns_to_ktime(60 * NSEC_PER_MSEC);
	obj->ps_tune0_timer.function = stk_ps_tune0_timer_func;
#endif

	if ((err = stk3a5x_init_client(client)))
	{
		goto exit_init_failed;
	}

	APS_LOG("stk3a5x_init_client() OK!\n");
	err = alsps_factory_device_register(&stk3a5x_factory_device);
	if(err)
	{
		APS_ERR("stk3a5x_device register failed\n");
		goto exit_misc_device_register_failed;
	}


	if ((err = stk3a5x_create_attr(&(stk3a5x_init_info.platform_diver_addr->driver))))
		//if((err = stk3a5x_create_attr(&stk3a5x_alsps_driver.driver)))
	{
		APS_ERR("create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}

	als_ctl.open_report_data = als_open_report_data;
	als_ctl.enable_nodata = als_enable_nodata;
	als_ctl.set_delay = als_set_delay;
	als_ctl.batch = als_batch;
	als_ctl.flush = als_flush;
	als_ctl.is_report_input_direct = false;
	als_ctl.is_use_common_factory = false;

	if (1 == obj->hw->polling_mode_als)
	{
		als_ctl.is_polling_mode = true;
	}
	else
	{
		als_ctl.is_polling_mode = false;
	}

#ifdef CUSTOM_KERNEL_SENSORHUB
	als_ctl.is_support_batch = obj->hw.is_batch_supported_als;
#else
	als_ctl.is_support_batch = false;
#endif
	err = als_register_control_path(&als_ctl);

	if (err)
	{
		APS_ERR("als_control register fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}

	als_data.get_data = als_get_data;
	als_data.vender_div = 100;
	err = als_register_data_path(&als_data);

	if (err)
	{
		APS_ERR("als_data register fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}

	ps_ctl.open_report_data = ps_open_report_data;
	ps_ctl.enable_nodata = ps_enable_nodata;
	ps_ctl.set_delay = ps_set_delay;
	ps_ctl.batch = ps_batch;
	ps_ctl.flush = ps_flush;

	ps_ctl.is_use_common_factory = false;
	ps_ctl.is_support_batch = false;

	if (1 == obj->hw->polling_mode_ps)
	{
		ps_ctl.is_polling_mode = true;
		ps_ctl.is_report_input_direct = false;
		//wake_lock_init(&mps_lock, WAKE_LOCK_SUSPEND, "ps wakelock");
	}
	else
	{
		ps_ctl.is_polling_mode = false;
		ps_ctl.is_report_input_direct = true;
	}

#ifdef CUSTOM_KERNEL_SENSORHUB
	ps_ctl.is_support_batch = obj->hw.is_batch_supported_ps;
#else
	ps_ctl.is_support_batch = false;
#endif
	err = ps_register_control_path(&ps_ctl);

	if (err)
	{
		APS_ERR("ps_control register fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}

	ps_data.get_data = ps_get_data;
	ps_data.vender_div = 100;
	err = ps_register_data_path(&ps_data);

	if (err)
	{
		APS_ERR("ps_data register fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}

	stk3a5x_init_flag = 0;
	APS_LOG("%s: state_val=0x%x, psctrl_val=0x%x, alsctrl_val=0x%x, ledctrl_val=0x%x, wait_val=0x%x, int_val=0x%x\n",
		__FUNCTION__, atomic_read(&obj->state_val), atomic_read(&obj->psctrl_val), atomic_read(&obj->alsctrl_val),
		obj->ledctrl_val, obj->wait_val, obj->int_val);
	APS_LOG("stk3a5x_i2c_probe() OK!\n");
	APS_LOG("%s: OK\n", __FUNCTION__);
	return 0;
exit_sensor_obj_attach_fail:
exit_create_attr_failed:
	//misc_deregister(&stk3a5x_device);
exit_misc_device_register_failed:
exit_init_failed:
	//i2c_detach_client(client);
	//	exit_kfree:
	kfree(obj);
exit:
	stk3a5x_i2c_client = NULL;
#ifdef MT6516
	MT6516_EINTIRQMask(CUST_EINT_ALS_NUM);  /*mask interrupt if fail*/
#endif
	stk3a5x_init_flag = -1;
	APS_ERR("%s: err = %d\n", __FUNCTION__, err);
	return err;
	}
/*----------------------------------------------------------------------------*/
static int stk3a5x_i2c_remove(struct i2c_client *client)
{
	int err;
	struct stk3a5x_priv *obj = i2c_get_clientdata(client);
#ifdef STK_TUNE0
	destroy_workqueue(obj->stk_ps_tune0_wq);
#endif

	if ((err = stk3a5x_delete_attr(&(stk3a5x_init_info.platform_diver_addr->driver))))
	{
		APS_ERR("stk3a5x_delete_attr fail: %d\n", err);
	}

	alsps_factory_device_deregister(&stk3a5x_factory_device);


	stk3a5x_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));
	return 0;
}
/*----------------------------------------------------------------------------*/
static int  stk3a5x_local_uninit(void)
{
	APS_FUN();
	//stk3a5x_power(hw, 0);
	i2c_del_driver(&stk3a5x_i2c_driver);
	stk3a5x_i2c_client = NULL;
	return 0;
}
/*----------------------------------------------------------------------------*/
static int stk3a5x_local_init(void)
{
	//struct stk3a5x_i2c_addr addr;
	APS_FUN();
	//stk3a5x_power(hw, 1);
	//stk3a5x_get_addr(&addr);

	if (i2c_add_driver(&stk3a5x_i2c_driver))
	{
		APS_ERR("add driver error\n");
		return -1;
	}

	if (-1 == stk3a5x_init_flag)
	{
		APS_ERR("stk3a5x_local_init fail with stk3a5x_init_flag=%d\n", stk3a5x_init_flag);
		return -1;
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
static int __init stk3a5x_init(void)
{
	APS_FUN();
	printk("stk3a5x_init In");
	alsps_driver_add(&stk3a5x_init_info);// hwmsen_alsps_add(&stk3a5x_init_info);
#if 0
	stk3a5x_regulator = regulator_get(&alspsPltFmDev->dev, "vstk3a5x");

	if (!stk3a5x_regulator)
		printk("stk3a5x_regulator is NULL\n");

	ret = regulator_set_voltage(stk3a5x_regulator, 2800000, 2800000); /*set 2.8v*/

	if (ret)
	{
		printk("regulator_set_voltage() failed!\n");
	}

	msleep(3);
#endif
	printk("stk3a5x_init Out");
	return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit stk3a5x_exit(void)
{
	APS_FUN();
}
/*----------------------------------------------------------------------------*/
module_init(stk3a5x_init);
module_exit(stk3a5x_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("Sensortek");
MODULE_DESCRIPTION("SensorTek stk3a5x proximity and light sensor driver");
MODULE_LICENSE("GPL");
