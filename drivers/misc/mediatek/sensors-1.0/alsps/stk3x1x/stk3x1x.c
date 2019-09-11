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

/* drivers/hwmon/mt6516/amit/stk3x1x.c - stk3x1x ALS/PS driver
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
#if defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
#endif
#include <linux/platform_device.h>
#include <asm/atomic.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/wakelock.h>
#include <asm/io.h>
#include <linux/module.h>

#include "sensors_io.h"
#include <cust_alsps.h>
//#include <stk_cust_alsps.h>
#include <linux/rtc.h>
#include "stk3x1x.h"
#define DRIVER_VERSION          "3.5.2 20150915 20180409"
//#define STK_PS_POLLING_LOG
//#define STK_PS_DEBUG
//+ add by fanxzh
//#define STK_TUNE0
//- add by fanxzh
#define CALI_EVERY_TIME
#define STK_ALS_FIR
#define STK_IRS
//#define STK_CHK_REG
//#define STK_GES
#define IR_LED

/////
#include "cust_alsps.h"  //wangxiqiang
//extern struct alsps_hw* get_cust_alsps_hw_stk3x1x(void);

#include <alsps.h>



#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>




/******************************************************************************
 * configuration
*******************************************************************************/

/*----------------------------------------------------------------------------*/
#define stk3x1x_DEV_NAME     "stk3x1x"
/*----------------------------------------------------------------------------*/
#define APS_TAG                  "[ALS/PS]"
#define APS_FUN(f)               pr_err(APS_TAG"[%s]: enter.\n", __func__)
#define APS_ERR(fmt, args...)    pr_err(APS_TAG"[ERROR][%s](%d): "fmt, __func__, __LINE__, ##args)
#define APS_LOG(fmt, args...)    pr_err(APS_TAG"[INFO][%s](%d): "fmt, __func__, __LINE__, ##args)
#define APS_DBG(fmt, args...)    ontim_dev_dbg(7, APS_TAG"[DBG][%s][%d]: "fmt, __func__, __LINE__, ##args) //pr_err(APS_TAG"%s %d : "fmt, __func__, __LINE__, ##args)

/******************************************************************************
 * extern functions
*******************************************************************************/
#ifdef STK_TUNE0
	#define STK_MAX_MIN_DIFF	200
	#define STK_LT_N_CT	140 //100
	#define STK_HT_N_CT	160 //150
#endif /* #ifdef STK_TUNE0 */

#define STK_IRC_MAX_ALS_CODE		20000
#define STK_IRC_MIN_ALS_CODE		25
#define STK_IRC_MIN_IR_CODE		50
#define STK_IRC_ALS_DENOMI		2
#define STK_IRC_ALS_NUMERA		5
#define STK_IRC_ALS_CORREC		748
#define STK_IRC_ALS_DENOMI_LED		100
#define STK_IRC_ALS_NUMERA_LED		80
//#define STK_IRC_ALS_CORREC		1868	//1343
#define STK_IRC_ALS_CORREC_LED		1908


#define STK_IRS_IT_REDUCE				2
#define STK_ALS_READ_IRS_IT_REDUCE	5
#define STK_ALS_THRESHOLD			30

#define STK3310SA_PID		0x17
#define STK3311SA_PID		0x1E
#define STK3311WV_PID	0x1D
#define MTK_AUTO_DETECT_ALSPS
/*----------------------------------------------------------------------------*/
static struct i2c_client *stk3x1x_i2c_client = NULL;
/*----------------------------------------------------------------------------*/
static const struct i2c_device_id stk3x1x_i2c_id[] = {{stk3x1x_DEV_NAME,0},{}};
//static struct i2c_board_info __initdata i2c_stk3x1x={ I2C_BOARD_INFO("stk3x1x", (0x90>>1))};
/*----------------------------------------------------------------------------*/
static int stk3x1x_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int stk3x1x_i2c_remove(struct i2c_client *client);
static int stk3x1x_i2c_detect(struct i2c_client *client, struct i2c_board_info *info);
/*----------------------------------------------------------------------------*/
static int stk3x1x_i2c_suspend(struct device *dev);
static int stk3x1x_i2c_resume(struct device *dev);
static struct stk3x1x_priv *g_stk3x1x_ptr = NULL;
static int	stk3x1x_init_flag = -1;	// 0<==>OK -1 <==> fail
static int  stk3x1x_local_init(void);
static int  stk3x1x_local_uninit(void);
static int stk3x1x_local_update(void);
static int stk3x1x_read_cali_file(struct i2c_client *client);

#define HW_dynamic_cali


#ifdef HW_dynamic_cali

#define PS_READ_PROINIF_VALUE

  #ifdef PS_READ_PROINIF_VALUE
	#define PROINFO_CALI_DATA_OFFSET        2684
	#define PS_BUF_SIZE    256
	#define PS_CALI_DATA_LEN  6
	//static char backup_file_path[PS_BUF_SIZE] ="/dev/block/mmcblk0p13";  ///"/dev/block/platform/bootdevice/by-name/proinfo";
	static char backup_file_path[PS_BUF_SIZE] ="/dev/block/platform/bootdevice/by-name/proinfo";
//add by fanxzh for ps dynamic cali begin
	//static unsigned int ps_cali_factor = 68;
	static unsigned int ps_cali_factor_30 = 44;
	static unsigned int ps_cali_factor_45 = 75;
	static unsigned int ps_cali_per = 100;
//add by fanxzh for ps dynamic cali end
	static mm_segment_t oldfs;
	static unsigned int ps_nvraw_none_value = 0;
	static unsigned int ps_nvraw_40mm_value = 0;
	static unsigned int ps_nvraw_25mm_value = 0;
  #endif
	static unsigned int default_none_value = 50;
	static unsigned int between_40mm_none_value = 150;
	static unsigned int between_25mm_none_value = 200;

	static unsigned int pwindows_value = 0;
	static unsigned int pwave_value = 0;
	static unsigned int threshold_value = 0;
	static unsigned int min_proximity = 0;
	static unsigned int ps_threshold_l = 0;
	static unsigned int ps_threshold_h = 0;
	static unsigned int ps_detection = -1;
	static unsigned int ps_thd_val_hlgh_set = 0;
	static unsigned int ps_thd_val_low_set = 0;
	static unsigned int ps_threshold_h_tmp = 0;
	static unsigned int ps_threshold_l_tmp = 0;
	static unsigned char get_calib_flag = 0;


	#define MAX_ADC_PROX_VALUE 2047
	#define FAR_THRESHOLD(x) (min_proximity<(x)?(x):min_proximity)
	#define NRAR_THRESHOLD(x) ((FAR_THRESHOLD(x)+pwindows_value-1)>MAX_ADC_PROX_VALUE ? MAX_ADC_PROX_VALUE:(FAR_THRESHOLD(x)+pwindows_value-1))
#endif

//static struct sensor_init_info stk3x1x_init_info = {
static struct alsps_init_info stk3x1x_init_info = {
		.name = "stk3x1x",
		.init = stk3x1x_local_init,
		.uninit = stk3x1x_local_uninit,
		.update = stk3x1x_local_update,
};
/*----------------------------------------------------------------------------*/
typedef enum {
    STK_TRC_ALS_DATA= 0x0001,
    STK_TRC_PS_DATA = 0x0002,
    STK_TRC_EINT    = 0x0004,
    STK_TRC_IOCTL   = 0x0008,
    STK_TRC_I2C     = 0x0010,
    STK_TRC_CVT_ALS = 0x0020,
    STK_TRC_CVT_PS  = 0x0040,
    STK_TRC_DEBUG   = 0x8000,
} STK_TRC;
/*----------------------------------------------------------------------------*/
typedef enum {
    STK_BIT_ALS    = 1,
    STK_BIT_PS     = 2,
} STK_BIT;
/*----------------------------------------------------------------------------*/
struct stk3x1x_i2c_addr {
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
#ifdef STK_ALS_FIR
	#define STK_FIR_LEN	8
	#define MAX_FIR_LEN 32
struct data_filter {
    u16 raw[MAX_FIR_LEN];
    int sum;
    int num;
    int idx;
};
#endif

static struct alsps_hw alsps_cust;
static struct alsps_hw *hw = &alsps_cust;


#ifdef CONFIG_OF
static const struct of_device_id stk3x1x_of_match[] = {
	{ .compatible = "mediatek,alps_stk3x1x"},
	{},
};
#endif


struct stk3x1x_priv {
    struct alsps_hw  *hw;
    struct i2c_client *client;
    struct delayed_work  eint_work;

    /*i2c address group*/
    struct stk3x1x_i2c_addr  addr;

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

#ifdef CONFIG_OF
struct device_node *irq_node;
	int		irq;
#endif

    /*data*/
    u16         als;
    u16         ps;
    u8          _align;
    //u16         als_level_num;
    //u16         als_value_num;
    //u32         als_level[C_CUST_ALS_LEVEL];
    //u32         als_value[C_CUST_ALS_LEVEL];

	atomic_t	state_val;
	atomic_t 	psctrl_val;
	atomic_t 	alsctrl_val;
	u8 			wait_val;
	u8		 	ledctrl_val;
	u8		 	int_val;

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
#ifdef STK_TUNE0
	uint16_t psa;
	uint16_t psi;
	uint16_t psi_set;
	uint16_t stk_max_min_diff;
	uint16_t stk_lt_n_ct;
	uint16_t stk_ht_n_ct;

#ifdef CALI_EVERY_TIME
	uint16_t ps_high_thd_boot;
	uint16_t ps_low_thd_boot;
#endif
	uint16_t boot_cali;
#ifdef STK_TUNE0
	struct hrtimer ps_tune0_timer;
	struct workqueue_struct *stk_ps_tune0_wq;
    struct work_struct stk_ps_tune0_work;
	ktime_t ps_tune0_delay;
#endif
	bool tune_zero_init_proc;
	uint32_t ps_stat_data[3];
	int data_count;
#endif
#ifdef STK_ALS_FIR
	struct data_filter      fir;
	atomic_t                firlength;
#endif
	uint16_t ir_code;
	uint16_t als_correct_factor;
	u16 als_last;
	bool re_enable_ps;
	bool re_enable_als;
	u16 ps_cali;

	uint8_t pid;
	uint8_t	p_wv_r_bd_with_co;
	uint16_t p_wv_r_bd_ratio;
	uint32_t als_code_last;

};
/*----------------------------------------------------------------------------*/
#ifdef CONFIG_PM_SLEEP
static const struct dev_pm_ops stk3x1x_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(stk3x1x_i2c_suspend, stk3x1x_i2c_resume)
};
#endif

static struct i2c_driver stk3x1x_i2c_driver = {
	.probe      = stk3x1x_i2c_probe,
	.remove     = stk3x1x_i2c_remove,
	.detect     = stk3x1x_i2c_detect,
	.id_table   = stk3x1x_i2c_id,
	.driver = {
		.name           = stk3x1x_DEV_NAME,
#ifdef CONFIG_PM_SLEEP
		.pm   = &stk3x1x_pm_ops,
#endif
#ifdef CONFIG_OF
	.of_match_table = stk3x1x_of_match,
#endif
	},
};


static struct stk3x1x_priv *stk3x1x_obj = NULL;
static int stk3x1x_get_ps_value(struct stk3x1x_priv *obj, u16 ps);
static int stk3x1x_get_ps_value_only(struct stk3x1x_priv *obj, u16 ps);
static int stk3x1x_get_als_value(struct stk3x1x_priv *obj, u16 als);
static int stk3x1x_read_als(struct i2c_client *client, u16 *data);
static int stk3x1x_read_ps(struct i2c_client *client, u16 *data);
static int stk3x1x_set_als_int_thd(struct i2c_client *client, u16 als_data_reg);
static int32_t stk3x1x_get_ir_value(struct stk3x1x_priv *obj, int32_t als_it_reduce);
#ifdef STK_TUNE0
static int stk_ps_tune_zero_func_fae(struct stk3x1x_priv *obj);
#endif
static int stk3x1x_init_client(struct i2c_client *client);
struct wake_lock ps_lock;
//#define ONTIM_ALS_CALI
#ifdef ONTIM_ALS_CALI
struct als_cali_para
{
        int als_ch0;
        int als_ch1;
};
static struct als_cali_para als_cali;
static int als_ch0_expect = 0;
static int als_ch1_expect = 0;
static int    s_nIsCaliLoaded = false;
static int    s_CaliLoadEnable = false;
static int als_cali_enable=0;   // 0--stop calib; 1--start calib; 2--calib runing;
static int channel[2] = {0};
#endif
/*----------------------------------------------------------------------------*/
/*
#define COLOR_TEMP 3
typedef enum
{
        CWF_TEMP=0,
        TL84_TEMP,
        A_TEMP,
}color_t;
*/
static unsigned int current_tp = 0;
static unsigned int current_color_temp=CWF_TEMP;
static unsigned int current_color_temp_first = 0;
static unsigned int    als_level[TP_COUNT][TEMP_COUNT][C_CUST_ALS_LEVEL];
static unsigned int    als_value[C_CUST_ALS_LEVEL] = {0};
static unsigned int coeff_boyi[3] = {180,160,210};
static unsigned int coeff_txd[3] =  {180,160,210};

static int *coeff_als_for_tp = coeff_boyi;

/*
static unsigned int    als_level[COLOR_TEMP][C_CUST_ALS_LEVEL]=
{
	{0,  59 , 135  , 222 ,   447 , 877  , 1330 ,  1735},      //cwf
	{0,  59 , 135  , 222 ,   801 , 2128  , 3615 ,  4946},      //d65
   {0,  59 , 135  , 222 ,   447 , 2101  , 3277 ,  4329},      //a
};
static unsigned int    als_value[C_CUST_ALS_LEVEL] =
       {0, 133, 304, 502, 1004, 2005, 3058, 4000};
*/
//#define ONTIM_CALI
#ifdef ONTIM_CALI
#define CALI_HIGH_THD 1600
#define CALI_LOW_THD 10
static int stk_prox_set_noice(int noice);
static int dynamic_calibrate=0;
int stk3x1x_write_ps_high_thd(struct i2c_client *client, u16 thd);
int stk3x1x_write_ps_low_thd(struct i2c_client *client, u16 thd);
#endif

#define ontim_debug_info


#ifdef ontim_debug_info
#include <ontim/ontim_dev_dgb.h>
static char stk3x1x_prox_version[]="stk_ps_mtk_1.0";//"stk3x1x_mtk_1.0";
static char stk3x1x_prox_vendor_name[20]="stk_ps";//"stk3x1x";
    DEV_ATTR_DECLARE(als_prox)
    DEV_ATTR_DEFINE("version",stk3x1x_prox_version)
    DEV_ATTR_DEFINE("vendor",stk3x1x_prox_vendor_name)
#ifdef ONTIM_CALI
    DEV_ATTR_EXEC_DEFINE("set_ps_noice",&stk_prox_set_noice)
#endif
    DEV_ATTR_DECLARE_END;
    ONTIM_DEBUG_DECLARE_AND_INIT(als_prox,als_prox,8);
#endif

#ifdef ONTIM_CALI
 struct PS_CALI_DATA_STRUCT
{
    int noice;
    int close;
    int far_away;
    int valid;
} ;

static struct PS_CALI_DATA_STRUCT ps_cali={-1,0,0,0};

static int stk_prox_set_noice(int noice)
{
	if ((noice > -1 ) &&  (noice < 1600))
	{
		ps_cali.noice = noice;
	}
	else
	{
		ps_cali.noice = -1;
	}
	APS_LOG("noice = %d; Set noice to %d\n",noice,ps_cali.noice);
	return 0;
}

static ssize_t stk_dynamic_calibrate(struct stk3x1x_priv *obj)
{
	//int ret=0;
	int i=0;
	//int data;
	int data_total=0;
	ssize_t len = 0;
	int noise = 0;
	int count = 5;
	int max = 0;
	int err = 0;

	// wait for register to be stable
	msleep(15);


	for (i = 0; i < count; i++) {
		// wait for ps value be stable

		msleep(15);

		if((err = stk3x1x_read_ps(obj->client, &obj->ps)))
		{
			APS_ERR("stk3x1x read ps data: %d\n", err);
			return err;
		}
		APS_DBG("ps #%d=%d\n", count, obj->ps);

		if (obj->ps < 0) {
			i--;
			continue;
		}

		//if(obj->ps & 0x8000){
		//	noise = 0;
		//	break;
		//}else{
			noise=obj->ps;
		//}

		data_total+=obj->ps;

		if (max++ > 100) {
			//len = sprintf(buf,"adjust fail\n");
			return len;
		}
	}

	noise=data_total/count;

	dynamic_calibrate = noise;

	if ((ps_cali.noice > -1) && ( ps_cali.close > ps_cali.far_away)  )
	{
		int temp_high =  ps_cali.close  -ps_cali.noice + dynamic_calibrate;
		int temp_low =  ps_cali.far_away  -ps_cali.noice + dynamic_calibrate;
		if ( (temp_high < CALI_HIGH_THD) && (temp_low > CALI_LOW_THD) && ( dynamic_calibrate > ps_cali.noice )&& (dynamic_calibrate < ((ps_cali.close + ps_cali.far_away)/2)))
		{
			atomic_set(&obj->ps_high_thd_val, temp_high);
			atomic_set(&obj->ps_low_thd_val, temp_low);
		}
		else
		{
			atomic_set(&obj->ps_high_thd_val,  ps_cali.close);
			atomic_set(&obj->ps_low_thd_val,  ps_cali.far_away);
		}
		if ((err = stk3x1x_write_ps_high_thd(obj->client, atomic_read(&obj->ps_high_thd_val))))
        	{
                	APS_ERR("write high thd error: %d\n", err);
                	return err;
       		}
        	if ((err = stk3x1x_write_ps_low_thd(obj->client, atomic_read(&obj->ps_low_thd_val))))
        	{
                	APS_ERR("write low thd error: %d\n", err);
                	return err;
        	}
        	APS_DBG("set HT=%d, LT=%d\n", atomic_read(&obj->ps_high_thd_val), atomic_read(&obj->ps_low_thd_val));

	}
	APS_LOG("cali noice=%d; cali high=%d; cali low=%d; curr noice=%d; high=%d; low=%d\n",ps_cali.noice,ps_cali.close,ps_cali.far_away,dynamic_calibrate,atomic_read(&obj->ps_high_thd_val),atomic_read(&obj->ps_low_thd_val));

	return 0;
}
#endif

int stk3x1x_get_addr(struct alsps_hw *hw, struct stk3x1x_i2c_addr *addr)
{
	if(!hw || !addr)
	{
		return -EFAULT;
	}
	addr->state   = STK_STATE_REG;
	addr->psctrl   = STK_PSCTRL_REG;
	addr->alsctrl  = STK_ALSCTRL_REG;
	addr->ledctrl  = STK_LEDCTRL_REG;
	addr->intmode    = STK_INT_REG;
	addr->wait    = STK_WAIT_REG;
	addr->thdh1_ps    = STK_THDH1_PS_REG;
	addr->thdh2_ps    = STK_THDH2_PS_REG;
	addr->thdl1_ps = STK_THDL1_PS_REG;
	addr->thdl2_ps = STK_THDL2_PS_REG;
	addr->thdh1_als    = STK_THDH1_ALS_REG;
	addr->thdh2_als    = STK_THDH2_ALS_REG;
	addr->thdl1_als = STK_THDL1_ALS_REG ;
	addr->thdl2_als = STK_THDL2_ALS_REG;
	addr->flag = STK_FLAG_REG;
	addr->data1_ps = STK_DATA1_PS_REG;
	addr->data2_ps = STK_DATA2_PS_REG;
	addr->data1_als = STK_DATA1_ALS_REG;
	addr->data2_als = STK_DATA2_ALS_REG;
	addr->data1_offset = STK_DATA1_OFFSET_REG;
	addr->data2_offset = STK_DATA2_OFFSET_REG;
	addr->data1_ir = STK_DATA1_IR_REG;
	addr->data2_ir = STK_DATA2_IR_REG;
	addr->soft_reset = STK_SW_RESET_REG;

	return 0;
}
/*----------------------------------------------------------------------------*/
int stk3x1x_hwmsen_read_block(struct i2c_client *client, u8 addr, u8 *data, u8 len)
{
	u8 beg = addr;
	struct i2c_msg msgs[2] =
	{
		{
			.addr = client->addr,
			.flags = 0,
			.len = 1,
			.buf= &beg
		},
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = len,
			.buf = data,
		}
	};
	int err;

	if (!client)
		return -EINVAL;
	else if (len > C_I2C_FIFO_SIZE)
	{
		APS_ERR(" length %d exceeds %d\n", len, C_I2C_FIFO_SIZE);
		return -EINVAL;
	}

	err = i2c_transfer(client->adapter, msgs, sizeof(msgs)/sizeof(msgs[0]));
	if (err != 2)
	{
		APS_ERR("i2c_transfer error: (%d %p %d) %d\n", addr, data, len, err);
		err = -EIO;
	}
	else
	{
		err = 0;/*no error*/
	}
	return err;
}
/*----------------------------------------------------------------------------*/
int stk3x1x_get_timing(void)
{
	return 200;
}

/*----------------------------------------------------------------------------*/
int stk3x1x_master_recv(struct i2c_client *client, u16 addr, u8 *buf ,int count)
{
	struct stk3x1x_priv *obj = i2c_get_clientdata(client);
	int ret = 0, retry = 0;
	int trc = atomic_read(&obj->trace);
	int max_try = atomic_read(&obj->i2c_retry);

	while(retry++ < max_try)
	{
		ret = stk3x1x_hwmsen_read_block(client, addr, buf, count);
		if(ret == 0)
            break;
		udelay(100);
	}

	if(unlikely(trc))
	{
		if((retry != 1) && (trc & STK_TRC_DEBUG))
		{
			APS_LOG("(recv) %d/%d\n", retry-1, max_try);

		}
	}

	/* If everything went ok (i.e. 1 msg transmitted), return #bytes
	transmitted, else error code. */
	return (ret == 0) ? count : ret;
}
/*----------------------------------------------------------------------------*/
int stk3x1x_master_send(struct i2c_client *client, u16 addr, u8 *buf ,int count)
{
	int ret = 0, retry = 0;
	struct stk3x1x_priv *obj = i2c_get_clientdata(client);
	int trc = atomic_read(&obj->trace);
	int max_try = atomic_read(&obj->i2c_retry);


	while(retry++ < max_try)
	{
		ret = hwmsen_write_block(client, addr, buf, count);
		if (ret == 0)
		    break;
		udelay(100);
	}

	if(unlikely(trc))
	{
		if((retry != 1) && (trc & STK_TRC_DEBUG))
		{
			APS_LOG("(send) %d/%d\n", retry-1, max_try);
		}
	}
	/* If everything went ok (i.e. 1 msg transmitted), return #bytes
	transmitted, else error code. */
	return (ret == 0) ? count : ret;
}
/*----------------------------------------------------------------------------*/
int stk3x1x_write_led(struct i2c_client *client, u8 data)
{
	struct stk3x1x_priv *obj = i2c_get_clientdata(client);
	int ret = 0;

    ret = stk3x1x_master_send(client, obj->addr.ledctrl, &data, 1);
	if(ret < 0)
	{
		APS_ERR("write led = %d\n", ret);
		return -EFAULT;
	}

	return 0;
}
#ifdef ONTIM_ALS_CALI
#define BUF_SIZE    256
#define PROINFO_CALI_DATA_OFFSET        904
#define LSENSOR_CALI_NAME_LEN                           8
#define LSENSOR_CALI_DATA_LEN                           32
static char    file_path[BUF_SIZE]        = "/drvinfo/als-calib.txt";
static char    backup_file_path[BUF_SIZE] = "/dev/block/platform/mtk-msdc.0/11230000.msdc0/by-name/proinfo";
static struct file* fd_file = NULL;
static mm_segment_t oldfs;

/*****************************************
 *** openFile
 *****************************************/
static struct file *openFile(char *path,int flag,int mode)
{
    struct file *fp = NULL;

    fp = filp_open(path, flag, mode);

    if (IS_ERR(fp) || !fp->f_op)
    {
        APS_ERR("Calibration File filp_open return NULL\n");
        return NULL;
    }
    else
    {
        return fp;
    }
}


/*****************************************
 *** seekFile
      whence--- SEEK_END/SEEK_CUR/SEEK_SET
 *****************************************/
static int seekFile(struct file *fp,int offset,int whence)
{
    if (fp->f_op && fp->f_op->llseek)
        return fp->f_op->llseek(fp,(loff_t)offset, whence);
    else
        return -1;
}

/*****************************************
 *** readFile
 *****************************************/
static int readFile(struct file *fp,char *buf,int readlen)
{
    if (fp->f_op && fp->f_op->read)
        return fp->f_op->read(fp,buf,readlen, &fp->f_pos);
    else
        return -1;
}

/*****************************************
 *** writeFile
 *****************************************/
static int writeFile(struct file *fp,char *buf,int writelen)
{
    if (fp->f_op && fp->f_op->write)
        return fp->f_op->write(fp,buf,writelen, &fp->f_pos);
    else
        return -1;
}

/*****************************************
 *** closeFile
 *****************************************/
static int closeFile(struct file *fp)
{
    filp_close(fp,NULL);
    return 0;
}

/*****************************************
 *** initKernelEnv
 *****************************************/
static void initKernelEnv(void)
{
    oldfs = get_fs();
    set_fs(KERNEL_DS);
    APS_DBG("initKernelEnv\n");
}

static int als_read_cali_file(void)
{
    int err = 0;
    char buf[LSENSOR_CALI_DATA_LEN] = {0};

    APS_FUN();

    initKernelEnv();
    fd_file = openFile(file_path,O_RDONLY,0);

    if (fd_file == NULL)
    {
        APS_ERR("fail to open calibration file: %s\n", file_path);
        fd_file = openFile(backup_file_path,O_RDONLY,0);

        if(fd_file == NULL)
        {
            APS_ERR("fail to open proinfo file: %s\n", backup_file_path);
            set_fs(oldfs);
            return (-1);
        }
        else
        {
            APS_LOG("Open proinfo file successfully: %s\n", backup_file_path);
            if (seekFile(fd_file,PROINFO_CALI_DATA_OFFSET,SEEK_SET)<0)
            {
                APS_ERR("fail to seek proinfo file: %s;\n", backup_file_path);
				goto read_error;
            }
            else
            {
                if((err = readFile(fd_file, buf, LSENSOR_CALI_NAME_LEN)) > 0)
                {
                    if (strncmp(buf,"ALS_CALI",8))
		    {
                        APS_ERR("read name error, name is %s\n",buf);
		    	goto read_error;
		    }
                }
		else
		{
                    APS_ERR("read file error %d\n",err);
                    goto read_error;
		}
            }
        }
    }
    else
    {
        APS_LOG("Open calibration file successfully: %s\n", file_path);
    }

    memset(buf,0,sizeof(buf));
    if ((err = readFile(fd_file, buf, sizeof(buf))) > 0)

        APS_LOG("cali_file: buf:%s\n",buf);
    else
    {
        APS_ERR("read file error %d\n",err);
        goto read_error;
    }

    closeFile(fd_file);
    set_fs(oldfs);

    sscanf(buf, "%d %d",&als_cali.als_ch0, &als_cali.als_ch1);
    APS_DBG("cali_data: %d %d\n", als_cali.als_ch0, als_cali.als_ch1);

    return 0;

read_error:
    closeFile(fd_file);
    set_fs(oldfs);
    return (-1);
}

static void als_load_cali(void)
{
    static int read_loop=0;

    if ((false == s_nIsCaliLoaded) && (s_CaliLoadEnable))
    {
        APS_LOG("loading cali file...\n");

        if (0 == als_read_cali_file())
        {
            s_nIsCaliLoaded = true;
            read_loop=0;
        }
        else
        {
            read_loop++;
            if (read_loop>10) s_nIsCaliLoaded=true;
            APS_ERR("loading cali file fail!\n");
        }
    }
}
static void als_ResetCalibration(void)
{
	als_cali.als_ch0 = als_ch0_expect;
	als_cali.als_ch1 = als_ch1_expect;
}
static void als_SetCaliOffset(int alsval_ch0,int alsval_ch1)
{
	als_cali.als_ch0 = alsval_ch0;
	als_cali.als_ch1 = alsval_ch1;
}
static int als_write_cali_file(int alsval_ch0,int alsval_ch1)
{
    #define _WRT_LOG_DATA_BUFFER_SIZE    (LSENSOR_CALI_DATA_LEN)

    int err = 0;
    char _pszBuffer[_WRT_LOG_DATA_BUFFER_SIZE] = {0};
    int n = 0;
    struct file* fd_file = NULL;
    mm_segment_t oldfs;

    oldfs = get_fs();
    set_fs(KERNEL_DS);

    fd_file = openFile(file_path ,O_RDWR | O_CREAT,0644);
    if (fd_file == NULL)
    {
        APS_ERR("als_write_log_data fail to open\n");

       // fd_file = openFile(backup_file_path ,O_RDWR,0);
        fd_file = openFile(backup_file_path,O_RDWR,0);

        if(fd_file == NULL)
        {
            APS_ERR("%s:fail to open proinfo file: %s\n", __func__, backup_file_path);
            set_fs(oldfs);
            return (-1);
        }
        else
        {
            APS_LOG("Open proinfo file successfully: %s\n", backup_file_path);
            if (seekFile(fd_file,PROINFO_CALI_DATA_OFFSET,SEEK_SET)<0)
            {
                APS_ERR("%s:fail to seek proinfo file: %s;\n", __func__, backup_file_path);
                closeFile(fd_file);
                set_fs(oldfs);
                return (-1);
            }
            else
            {
                sprintf(_pszBuffer,"ALS_CALI");
                if ((err = writeFile(fd_file,_pszBuffer,LSENSOR_CALI_NAME_LEN)) > 0)
                {
                    APS_LOG("name:%s\n",_pszBuffer);
                    memset(_pszBuffer, 0, _WRT_LOG_DATA_BUFFER_SIZE);
                }
                else
                {
                    APS_ERR("write name error %d\n",err);
                    closeFile(fd_file);
                    set_fs(oldfs);
                    return (-1);
                }
            }
        }
    }

    n = sprintf(_pszBuffer, "%d %d\n", alsval_ch0 ,alsval_ch1);

    if ((err = writeFile(fd_file,_pszBuffer,_WRT_LOG_DATA_BUFFER_SIZE)) > 0)
        APS_LOG("buf:%s\n",_pszBuffer);
    else
        APS_ERR("write file error %d\n",err);


    closeFile(fd_file);
    set_fs(oldfs);

    return 0;
}
static int als_do_audo_cali(int alsval_ch0,int alsval_ch1)
{
	int ret=0;
        static int save_count=0;

      	als_SetCaliOffset(alsval_ch0,alsval_ch1);

	if (als_write_cali_file(alsval_ch0,alsval_ch1)==0)
   	{
		s_CaliLoadEnable = true;
		s_nIsCaliLoaded=false;
		als_cali_enable=0;
		save_count = 0;
	}
	else if (save_count < 5)
	{
		save_count++;
		ret=-1;
	}
	else
  	{
		APS_ERR("Save cali file fail  5 time, Disable read cali file!!!\n");
		s_CaliLoadEnable = false;
		als_cali_enable=0;
		save_count=0;
		ret=-1;
  	}

    return ret;
}
#endif
/*----------------------------------------------------------------------------*/
int stk3x1x_read_als(struct i2c_client *client, u16 *data)
{
	struct timeval tv = { 0 };
	struct rtc_time tm_android;
	struct timeval tv_android = { 0 };
	struct stk3x1x_priv *obj = i2c_get_clientdata(client);
	int ret = 0;
	u8 buf[2];
	int32_t als_comperator,als_comperator_led;
	u16 als_data;
	int32_t ir_data, tmp;
	u32 als_data_u32;
	int32_t als_comperator1,als_comperator_led1;

#ifdef STK_IRS
	const int ir_enlarge = 1 << (STK_ALS_READ_IRS_IT_REDUCE - STK_IRS_IT_REDUCE);
#endif

#ifdef STK_ALS_FIR
	int idx;
	int firlen = atomic_read(&obj->firlength);
#endif
	do_gettimeofday(&tv);
        tv_android = tv;
        tv_android.tv_sec -= sys_tz.tz_minuteswest * 60;
        rtc_time_to_tm(tv_android.tv_sec, &tm_android);

	if(NULL == client)
	{
		return -EINVAL;
	}
	ret = stk3x1x_master_recv(client, obj->addr.data1_als, buf, 0x02);
	if(ret < 0)
	{
		APS_ERR("error: %d\n", ret);
		return -EFAULT;
	}

	als_data = (buf[0] << 8) | (buf[1]);
	if((obj->p_wv_r_bd_with_co & 0b101) == 0b101)
	{
		tmp = als_data * obj->p_wv_r_bd_ratio / 1024;
		if(tmp > 65535)
			als_data = 65535;
		else
			als_data = tmp;
	}

	if(obj->p_wv_r_bd_with_co & 0b010)
	{
		if(als_data < STK_ALS_THRESHOLD && obj->als_code_last > 10000)
		{
			ir_data = stk3x1x_get_ir_value(obj, STK_ALS_READ_IRS_IT_REDUCE);
#ifdef STK_IRS
			if(ir_data > 0)
				obj->ir_code = ir_data * ir_enlarge;

			APS_DBG("%s:  als_data=%d, als_code_last=%d,ir_data=%d,ir_enlarge=%d\n",
					__func__,  als_data, obj->als_code_last, ir_data,ir_enlarge);
#endif
			if(ir_data > 1000)
			{
				//als_data = obj->als_code_last;
			}
		}
	}
	obj->als_code_last = als_data;
#ifdef ONTIM_ALS_CALI
	channel[0] = als_data;
        als_load_cali();
        if (als_cali_enable==1)
        {
                als_cali_enable =2;
                s_CaliLoadEnable = false;
                als_ResetCalibration();
        }
        als_data = als_data *  als_ch0_expect / als_cali.als_ch0;
#endif
        ir_data = stk3x1x_get_ir_value(obj, STK_ALS_READ_IRS_IT_REDUCE);
#ifdef ONTIM_ALS_CALI
        if (als_cali_enable==2)
        {
                als_do_audo_cali(als_data,ir_data);
        }
#endif
#ifdef STK_ALS_FIR
	if(obj->fir.num < firlen)
	{
		obj->fir.raw[obj->fir.num] = als_data;
		obj->fir.sum += als_data;
		obj->fir.num++;
		obj->fir.idx++;
	}
	else
	{
		idx = obj->fir.idx % firlen;
		obj->fir.sum -= obj->fir.raw[idx];
		obj->fir.raw[idx] = als_data;
		obj->fir.sum += als_data;
		obj->fir.idx++;
		als_data = (obj->fir.sum / firlen);
	}
#endif
	//ir_data = stk3x1x_get_ir_value(obj, STK_ALS_READ_IRS_IT_REDUCE);
	//APS_LOG("%s: %02d:%02d:%02d. Sensortek als=%d, ir_data=%d \n", __func__, tm_android.tm_hour, tm_android.tm_min, tm_android.tm_sec,als_data, ir_data);

	als_comperator1 = als_data * 90 / 100;
	als_comperator_led1 = als_data * 15 / 2;
	if(ir_data < als_comperator1)
		current_color_temp = CWF_TEMP;
	else if(ir_data > als_comperator_led1)
		current_color_temp = CWF_TEMP;//D65_TEMP;//A_TEMP
	else
		current_color_temp = CWF_TEMP;//D65_TEMP;

	if(obj->ir_code)
	{
		obj->als_correct_factor = 1000;
		if(als_data < STK_IRC_MAX_ALS_CODE && als_data > STK_IRC_MIN_ALS_CODE &&
			obj->ir_code > STK_IRC_MIN_IR_CODE)
		{
			als_comperator = als_data * STK_IRC_ALS_NUMERA / STK_IRC_ALS_DENOMI;
			als_comperator_led = als_data * STK_IRC_ALS_NUMERA_LED / STK_IRC_ALS_DENOMI_LED;
			if(obj->ir_code > als_comperator)
				obj->als_correct_factor = STK_IRC_ALS_CORREC;
#ifdef IR_LED
			else if(obj->ir_code < als_comperator_led)
				obj->als_correct_factor = STK_IRC_ALS_CORREC_LED;
#endif
			//if(obj->ir_code > als_comperator)
			//	obj->als_correct_factor = STK_IRC_ALS_CORREC;
		}
		APS_DBG("%s: %02d:%02d:%02d. Sensortek als=%d, ir=%d, als_correct_factor=%d", __func__, tm_android.tm_hour, tm_android.tm_min, tm_android.tm_sec,als_data, obj->ir_code, obj->als_correct_factor);
		obj->ir_code = 0;
	}
	als_data_u32 = als_data;
	//als_data_u32 = als_data_u32 * obj->als_correct_factor / 1000;
	*data = (u16)als_data_u32;

	APS_DBG("ALS: 0x%04X\n", (u32)(*data));

	return 0;
}
/*----------------------------------------------------------------------------*/
int stk3x1x_write_als(struct i2c_client *client, u8 data)
{
	struct stk3x1x_priv *obj = i2c_get_clientdata(client);
	int ret = 0;

    ret = stk3x1x_master_send(client, obj->addr.alsctrl, &data, 1);
	if(ret < 0)
	{
		APS_ERR("write als = %d\n", ret);
		return -EFAULT;
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
int stk3x1x_read_state(struct i2c_client *client, u8 *data)
{
	//struct stk3x1x_priv *obj = i2c_get_clientdata(client);
	int ret = 0;
	u8 buf;

	if(NULL == client)
	{
		return -EINVAL;
	}
	ret = stk3x1x_master_recv(client, STK_STATE_REG, &buf, 0x01);
	if(ret < 0)
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
int stk3x1x_read_flag(struct i2c_client *client, u8 *data)
{
	struct stk3x1x_priv *obj = i2c_get_clientdata(client);
	int ret = 0;
	u8 buf;

	if(NULL == client)
	{
		return -EINVAL;
	}
	ret = stk3x1x_master_recv(client, obj->addr.flag, &buf, 0x01);
	if(ret < 0)
	{
		APS_ERR("error: %d\n", ret);
		return -EFAULT;
	}
	else
	{
		*data = buf;
	}

	if(atomic_read(&obj->trace) & STK_TRC_ALS_DATA)
	{
		APS_LOG("PS NF flag: 0x%04X\n", (u32)(*data));
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
static int stk3x1x_read_otp25(struct i2c_client *client)
{
	// struct stk3x1x_priv *obj = i2c_get_clientdata(client);
	int ret, otp25;
	u8 data;

	data = 0x2;
    ret = stk3x1x_master_send(client, 0x0, &data, 1);
	if (ret < 0)
	{
		APS_ERR("write 0x0 = %d\n", ret);
		return -EFAULT;
	}

	data = 0x25;
    ret = stk3x1x_master_send(client, 0x90, &data, 1);
	if (ret < 0)
	{
		APS_ERR("write 0x90 = %d\n", ret);
		return -EFAULT;
	}

	data = 0x82;
    ret = stk3x1x_master_send(client, 0x92, &data, 1);
	if (ret < 0)
	{
		APS_ERR("write 0x0 = %d\n", ret);
		return -EFAULT;
	}
	usleep_range(1000, 5000);

	ret = stk3x1x_master_recv(client, 0x91, &data, 1);
	if(ret < 0)
	{
		APS_ERR("error: %d\n", ret);
		return -EFAULT;
	}
	otp25 = data;

	data = 0x0;
    ret = stk3x1x_master_send(client, 0x0, &data, 1);
	if (ret < 0)
	{
		APS_ERR("write 0x0 = %d\n", ret);
		return -EFAULT;
	}

	APS_DBG("%s: otp25=0x%x\n", __func__, otp25);
	if(otp25 & 0x80)
		return 1;

	return 0;
}

/*----------------------------------------------------------------------------*/
int stk3x1x_read_id(struct i2c_client *client)
{
	struct stk3x1x_priv *obj = i2c_get_clientdata(client);
	int ret = 0;
	u8 buf[2];
	u8 pid_msb;

	if(NULL == client)
	{
		return -EINVAL;
	}
	obj->p_wv_r_bd_with_co = 0;
	obj->p_wv_r_bd_ratio = 1024;

	ret = stk3x1x_master_recv(client, STK_PDT_ID_REG, buf, 0x02);
	if(ret < 0)
	{
		APS_ERR("error: %d\n", ret);
		return -EFAULT;
	}
	obj->pid = buf[0];
	APS_DBG("PID=0x%x, VID=0x%x\n", buf[0], buf[1]);

	if(obj->pid == STK3310SA_PID || obj->pid == STK3311SA_PID)
		obj->ledctrl_val &= 0x3F;

	if(buf[0] == STK3311WV_PID)
		obj->p_wv_r_bd_with_co |= 0b100;
	if(buf[1] == 0xC3)
		obj->p_wv_r_bd_with_co |= 0b010;

	if(stk3x1x_read_otp25(client) == 1)
	{
		obj->p_wv_r_bd_with_co |= 0b001;
		obj->p_wv_r_bd_ratio = 1024;
	}
	APS_DBG("%s: p_wv_r_bd_with_co = 0x%x\n", __func__, obj->p_wv_r_bd_with_co);

	if(buf[0] == 0)
	{
		APS_ERR( "PID=0x0, please make sure the chip is stk3x1x!\n");
		return -2;
	}

	pid_msb = buf[0] & 0xF0;
	switch(pid_msb)
	{
	case 0x10:
	case 0x20:
	case 0x30:
		return 0;
	default:
		APS_ERR( "invalid PID(%#x)\n", buf[0]);
		return -1;
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
int stk3x1x_read_ps(struct i2c_client *client, u16 *data)
{
	struct stk3x1x_priv *obj = i2c_get_clientdata(client);
	int ret = 0;
	u8 buf[2];

	if(NULL == client)
	{
		APS_ERR("i2c client is NULL\n");
		return -EINVAL;
	}
	ret = stk3x1x_master_recv(client, obj->addr.data1_ps, buf, 0x02);
	if(ret < 0)
	{
		APS_ERR("error: %d\n", ret);
		return -EFAULT;
	}
	else
	{
		*data = (buf[0] << 8) | (buf[1]);
	}

#ifndef SMT_MODE
	APS_LOG("rawdata:%d\n", (u32)(*data));
#endif

	return 0;
}
/*----------------------------------------------------------------------------*/
int stk3x1x_write_ps(struct i2c_client *client, u8 data)
{
	struct stk3x1x_priv *obj = i2c_get_clientdata(client);
	int ret = 0;

    ret = stk3x1x_master_send(client, obj->addr.psctrl, &data, 1);
	if (ret < 0)
	{
		APS_ERR("write ps = %d\n", ret);
		return -EFAULT;
	}
	return 0;
}

/*----------------------------------------------------------------------------*/
int stk3x1x_write_wait(struct i2c_client *client, u8 data)
{
	struct stk3x1x_priv *obj = i2c_get_clientdata(client);
	int ret = 0;

    ret = stk3x1x_master_send(client, obj->addr.wait, &data, 1);
	if (ret < 0)
	{
		APS_ERR("write wait = %d\n", ret);
		return -EFAULT;
	}
	return 0;
}

/*----------------------------------------------------------------------------*/
int stk3x1x_write_int(struct i2c_client *client, u8 data)
{
	struct stk3x1x_priv *obj = i2c_get_clientdata(client);
	int ret = 0;

    ret = stk3x1x_master_send(client, obj->addr.intmode, &data, 1);
	if (ret < 0)
	{
		APS_ERR("write intmode = %d\n", ret);
		return -EFAULT;
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
int stk3x1x_write_state(struct i2c_client *client, u8 data)
{
	struct stk3x1x_priv *obj = i2c_get_clientdata(client);
	int ret = 0;

    ret = stk3x1x_master_send(client, obj->addr.state, &data, 1);
	if (ret < 0)
	{
		APS_ERR("write state = %d\n", ret);
		return -EFAULT;
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
int stk3x1x_write_flag(struct i2c_client *client, u8 data)
{
	struct stk3x1x_priv *obj = i2c_get_clientdata(client);
	int ret = 0;

    ret = stk3x1x_master_send(client, obj->addr.flag, &data, 1);
	if (ret < 0)
	{
		APS_ERR("write ps = %d\n", ret);
		return -EFAULT;
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
int stk3x1x_write_sw_reset(struct i2c_client *client)
{
	struct stk3x1x_priv *obj = i2c_get_clientdata(client);
	u8 buf = 0, r_buf = 0;
	int ret = 0;

	buf = 0x7F;
    ret = stk3x1x_master_send(client, obj->addr.wait, (char*)&buf, sizeof(buf));
	if (ret < 0)
	{
		APS_ERR("i2c write test error = %d\n", ret);
		return -EFAULT;
	}

    ret = stk3x1x_master_recv(client, obj->addr.wait, &r_buf, 1);
	if (ret < 0)
	{
		APS_ERR("i2c read test error = %d\n", ret);
		return -EFAULT;
	}

	if(buf != r_buf)
	{
        APS_ERR("i2c r/w test error, read-back value is not the same, write=0x%x, read=0x%x\n", buf, r_buf);
		return -EIO;
	}

	buf = 0;
    ret = stk3x1x_master_send(client, obj->addr.soft_reset, (char*)&buf, sizeof(buf));
	if (ret < 0)
	{
		APS_ERR("write software reset error = %d\n", ret);
		return -EFAULT;
	}
	msleep(13);
	return 0;
}

/*----------------------------------------------------------------------------*/
int stk3x1x_write_ps_high_thd(struct i2c_client *client, u16 thd)
{
	struct stk3x1x_priv *obj = i2c_get_clientdata(client);
	u8 buf[2];
	int ret = 0;

    buf[0] = (u8) ((0xFF00 & thd) >> 8);
    buf[1] = (u8) (0x00FF & thd);

    ret = stk3x1x_master_send(client, obj->addr.thdh1_ps, &buf[0], 1);
	if (ret < 0)
	{
		APS_ERR("WARNING: %d\n",  ret);
		return -EFAULT;
	}

    ret = stk3x1x_master_send(client, obj->addr.thdh2_ps, &(buf[1]), 1);
	if (ret < 0)
	{
		APS_ERR("WARNING: %d\n", ret);
		return -EFAULT;
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
int stk3x1x_write_ps_low_thd(struct i2c_client *client, u16 thd)
{
	struct stk3x1x_priv *obj = i2c_get_clientdata(client);
	u8 buf[2];
	int ret = 0;

    buf[0] = (u8) ((0xFF00 & thd) >> 8);
    buf[1] = (u8) (0x00FF & thd);
    ret = stk3x1x_master_send(client, obj->addr.thdl1_ps, &buf[0], 1);
	if (ret < 0)
	{
		APS_ERR("WARNING: %s: %d\n", __func__, ret);
		return -EFAULT;
	}

    ret = stk3x1x_master_send(client, obj->addr.thdl2_ps, &(buf[1]), 1);
	if (ret < 0)
	{
		APS_ERR("WARNING: %s: %d\n", __func__, ret);
		return -EFAULT;
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
int stk3x1x_write_als_high_thd(struct i2c_client *client, u16 thd)
{
	struct stk3x1x_priv *obj = i2c_get_clientdata(client);
	u8 buf[2];
	int ret = 0;

    buf[0] = (u8) ((0xFF00 & thd) >> 8);
    buf[1] = (u8) (0x00FF & thd);
    ret = stk3x1x_master_send(client, obj->addr.thdh1_als, &buf[0], 1);
	if (ret < 0)
	{
		APS_ERR("WARNING: %s: %d\n", __func__, ret);
		return -EFAULT;
	}

    ret = stk3x1x_master_send(client, obj->addr.thdh2_als, &(buf[1]), 1);
	if (ret < 0)
	{
		APS_ERR("WARNING: %s: %d\n", __func__, ret);
		return -EFAULT;
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
int stk3x1x_write_als_low_thd(struct i2c_client *client, u16 thd)
{
	struct stk3x1x_priv *obj = i2c_get_clientdata(client);
	u8 buf[2];
	int ret = 0;

    buf[0] = (u8) ((0xFF00 & thd) >> 8);
    buf[1] = (u8) (0x00FF & thd);
    ret = stk3x1x_master_send(client, obj->addr.thdl1_als, &buf[0], 1);
	if (ret < 0)
	{
		APS_ERR("WARNING: %s: %d\n", __func__, ret);
		return -EFAULT;
	}

    ret = stk3x1x_master_send(client, obj->addr.thdl2_als, &(buf[1]), 1);
	if (ret < 0)
	{
		APS_ERR("WARNING: %s: %d\n", __func__, ret);
		return -EFAULT;
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
#if 0
int stk3x1x_write_foffset(struct i2c_client *client, u16 ofset)
{
	struct stk3x1x_priv *obj = i2c_get_clientdata(client);
	u8 buf[2];
	int ret = 0;

    buf[0] = (u8) ((0xFF00 & ofset) >> 8);
    buf[1] = (u8) (0x00FF & ofset);
    ret = stk3x1x_master_send(client, obj->addr.data1_offset, &buf[0], 1);
	if (ret < 0)
	{
		APS_ERR("WARNING: %s: %d\n", __func__, ret);
		return -EFAULT;
	}

    ret = stk3x1x_master_send(client, obj->addr.data2_offset, &(buf[1]), 1);
	if (ret < 0)
	{
		APS_ERR("WARNING: %s: %d\n", __func__, ret);
		return -EFAULT;
	}

	return 0;
}

/*----------------------------------------------------------------------------*/

int stk3x1x_write_aoffset(struct i2c_client *client,  u16 ofset)
{
	struct stk3x1x_priv *obj = i2c_get_clientdata(client);
	u8 buf[2];
	int ret = 0;
	u8 s_buf = 0, re_en;
    ret = stk3x1x_master_recv(client, obj->addr.state, &s_buf, 1);
	if (ret < 0)
	{
		APS_ERR("i2c read state error = %d\n", ret);
		return -EFAULT;
	}
	re_en = (s_buf & STK_STATE_EN_AK_MASK) ? 1: 0;
	if(re_en)
	{
		s_buf &= (~STK_STATE_EN_AK_MASK);
		ret = stk3x1x_master_send(client, obj->addr.state, &s_buf, 1);
		if (ret < 0)
		{
			APS_ERR("write state = %d\n", ret);
			return -EFAULT;
		}
		msleep(3);
	}

    buf[0] = (u8) ((0xFF00 & ofset) >> 8);
    buf[1] = (u8) (0x00FF & ofset);
    ret = stk3x1x_master_send(client, 0x0E, &buf[0], 1);
	if (ret < 0)
	{
		APS_ERR("WARNING: %s: %d\n", __func__, ret);
		return -EFAULT;
	}

    ret = stk3x1x_master_send(client, 0x0F, &(buf[1]), 1);
	if (ret < 0)
	{
		APS_ERR("WARNING: %s: %d\n", __func__, ret);
		return -EFAULT;
	}
	if(!re_en)
		return 0;
	s_buf |= STK_STATE_EN_AK_MASK;
	ret = stk3x1x_master_send(client, obj->addr.state, &s_buf, 1);
	if (ret < 0)
	{
		APS_ERR("write state = %d\n", ret);
		return -EFAULT;
	}
	return 0;
}
#endif

/*----------------------------------------------------------------------------*/
static int stk3x1x_enable_als(struct i2c_client *client, int enable)
{
	struct stk3x1x_priv *obj = i2c_get_clientdata(client);
	int err, cur = 0, old = atomic_read(&obj->state_val);
	int trc = atomic_read(&obj->trace);

	APS_DBG("%s: enable=%d\n", __func__, enable);

	cur = old & (~(STK_STATE_EN_ALS_MASK | STK_STATE_EN_WAIT_MASK));
	if(enable)
	{
		cur |= STK_STATE_EN_ALS_MASK;
	}
	else if (old & STK_STATE_EN_PS_MASK)
	{
		cur |= STK_STATE_EN_WAIT_MASK;
	}
	if(trc & STK_TRC_DEBUG)
	{
		APS_LOG("%s: %08X, %08X, %d\n", __func__, cur, old, enable);
	}

	if(0 == (cur ^ old))
	{
		return 0;
	}

#ifdef STK_IRS
	if(enable && !(old & STK_STATE_EN_PS_MASK))
	{
		err =  stk3x1x_get_ir_value(obj, STK_IRS_IT_REDUCE);
		if(err > 0)
			obj->ir_code = err;
	}
#endif

	if(enable && obj->hw->polling_mode_als == 0)
	{
		stk3x1x_write_als_high_thd(client, 0x0);
		stk3x1x_write_als_low_thd(client, 0xFFFF);
	}
	err = stk3x1x_write_state(client, cur);
	if(err < 0)
		return err;
	else
		atomic_set(&obj->state_val, cur);

	if(enable)
	{

		obj->als_last = 0;
		if(obj->hw->polling_mode_als)
		{
			atomic_set(&obj->als_deb_on, 1);
			atomic_set(&obj->als_deb_end, jiffies+atomic_read(&obj->als_debounce)*HZ/1000);
		}
		else
		{
			//set_bit(STK_BIT_ALS,  &obj->pending_intr);
			schedule_delayed_work(&obj->eint_work,220*HZ/1000);
		}
	}
#ifdef STK_ALS_FIR
	else
	{
        	memset(&obj->fir, 0x00, sizeof(obj->fir));
	}
#endif

	if(trc & STK_TRC_DEBUG)
	{
		APS_LOG("enable als (%d)\n", enable);
	}

	return err;
}


#ifdef HW_dynamic_cali

#ifdef PS_READ_PROINIF_VALUE
/*****************************************
 *** openFile
 *****************************************/
static struct file *openFile(char *path, int flag, int mode)
{
	struct file *fp = NULL;

	fp = filp_open(path, flag, mode);

	if (IS_ERR(fp) || !fp->f_op) {
		APS_ERR("Calibration File filp_open return NULL\n");
		return NULL;
	} else {
		return fp;
	}
}


/*****************************************
 *** seekFile
      whence--- SEEK_END/SEEK_CUR/SEEK_SET
 *****************************************/
static int seekFile(struct file *fp, int offset, int whence)
{
	if (fp->f_op && fp->f_op->llseek)
		return fp->f_op->llseek(fp, (loff_t) offset, whence);
	else
		return -1;
}

/*****************************************
 *** readFile
 *****************************************/
static int readFile(struct file *fp, char *buf, int readlen)
{
	if (fp && fp->f_op)
		return __vfs_read(fp, buf, readlen, &fp->f_pos);
	else
		return -1;
}

/*****************************************
 *** writeFile
 *****************************************/
#if 0
static int writeFile(struct file *fp, char *buf, int writelen)
{
	if (fp->f_op && fp->f_op->write)
		return fp->f_op->write(fp, buf, writelen, &fp->f_pos);
	else
		return -1;
}
#endif
/*****************************************
 *** closeFile
 *****************************************/
static int closeFile(struct file *fp)
{
	filp_close(fp, NULL);
	return 0;
}

/*****************************************
 *** initKernelEnv
 *****************************************/
static void initKernelEnv(void)
{
	oldfs = get_fs();
	set_fs(KERNEL_DS);
	APS_DBG("initKernelEnv\n");
}
/*****************************************
 *** stk3x1x_read_cali_file
 *****************************************/
static int stk3x1x_read_cali_file(struct i2c_client *client)
{
	int mRaw =0;
	int mRaw45 = 0;
	int mRaw30 = 0;
	int mRaw22 = 0;
	int err = 0;
	char buf[PS_CALI_DATA_LEN] = { 0 };  ///PS_CALI_DATA_LEN = 6

	struct file *fd_file = NULL;

	initKernelEnv();

	fd_file = openFile(backup_file_path, O_RDONLY, 0);
	if (fd_file == NULL) {
		APS_ERR("fail to open proinfo file: %s\n", backup_file_path);
		set_fs(oldfs);
		return (-1);
	}

	if (seekFile(fd_file, PROINFO_CALI_DATA_OFFSET, SEEK_SET) < 0) {
		APS_ERR("fail to seek proinfo file: %s;\n", backup_file_path);
		goto read_error;
	}

	memset(buf, 0, sizeof(buf));
	err = readFile(fd_file, buf, sizeof(buf));
	if (err > 0) {
		APS_DBG("cali_file: buf:%s\n", buf);
	} else {
		APS_ERR("read file error %d\n", err);
		goto read_error;
	}

	closeFile(fd_file);
	set_fs(oldfs);
//add by fanxzh for dynamic cali begin
	mRaw   = (int)(((int)buf[0])<<8|(0xFF&(int)buf[1]));
	//mRaw40 = (int)(((int)buf[2])<<8|(0xFF&(int)buf[3]));
	mRaw22 = (int)(((int)buf[4])<<8|(0xFF&(int)buf[5]));
	//mRaw40 = mRaw25 - ((ps_cali_factor * (mRaw25 - mRaw)) / ps_cali_per);
	mRaw30 = mRaw22 - ((ps_cali_factor_30 * (mRaw22 - mRaw)) / ps_cali_per);
	mRaw45 = mRaw22 - ((ps_cali_factor_45 * (mRaw22 - mRaw)) / ps_cali_per);
	APS_LOG("mRaw:%d   mRaw22:%d  mRaw30:%d  mRaw45:%d\n", mRaw, mRaw22, mRaw30, mRaw45);
//add by fanxzh for dynamic cali end

	if(((mRaw + 8) < mRaw45) && ((mRaw45 + 8) < mRaw30)){
		ps_nvraw_none_value = mRaw;
		ps_threshold_l_tmp = ps_nvraw_40mm_value = mRaw45;
		ps_threshold_h_tmp = ps_nvraw_25mm_value = mRaw30;
	}else{
		ps_nvraw_none_value = default_none_value;
		ps_threshold_l_tmp = ps_nvraw_40mm_value = between_40mm_none_value;
		ps_threshold_h_tmp = ps_nvraw_25mm_value = between_25mm_none_value;

		APS_LOG("none_value =  %d  \n", mRaw);
	}

	return 0;

read_error:
	closeFile(fd_file);
	set_fs(oldfs);
	return (-1);
}

static int stk3x1x_local_update(void)
{
	int ret = -1;

	ret = stk3x1x_read_cali_file(NULL);
	if( 0 != ret ) {
		APS_ERR("stk3x1x_read_cali_file, refresh califile failed, ret=%d\n", ret);
	}

	return ret;

}
#endif
/*****************************************/


static int stk3x1x_enable_ps(struct i2c_client *client, int enable, int validate_reg)
{
	struct stk3x1x_priv *obj = i2c_get_clientdata(client);
	int err, cur = 0, old = atomic_read(&obj->state_val);
	int trc = atomic_read(&obj->trace);
	int ps_ctrl;
	u8 temp = 0;
	int cali_err = -1;

#ifdef STK_PS_DEBUG
	if(enable)
	{
		APS_LOG("%s: before enable ps, HT=%d, LT=%d\n", __func__, atomic_read(&obj->ps_high_thd_val),  atomic_read(&obj->ps_low_thd_val));
		APS_LOG("%s: before enable ps, show all reg\n", __FUNCTION__);
		show_allreg(); //for debug
	}
#endif

	APS_DBG("enable=%d\n", enable);
	stk3x1x_read_flag(obj->client, &temp);
	APS_DBG("read_flag = 0x%x\n", temp);

	if(obj->first_boot == true)
	{
		obj->first_boot = false;
	}


	cur = old;
	cur &= (~(0x45));
	if(enable)
	{
		cur |= (STK_STATE_EN_PS_MASK);
		if(!(old & STK_STATE_EN_ALS_MASK))
			cur |= STK_STATE_EN_WAIT_MASK;
		if(1 == obj->hw->polling_mode_ps)
		{
		//	APS_LOG("%s: line=%d\n", __FUNCTION__, __LINE__);
			wake_lock(&ps_lock);
		}
	}
	else
	{
		if(1 == obj->hw->polling_mode_ps)
		{
		//	APS_LOG("%s: line=%d\n", __FUNCTION__, __LINE__);
			wake_unlock(&ps_lock);
		}
	}

	if(trc & STK_TRC_DEBUG)
	{
		APS_LOG("%s: %08X, %08X, %d\n", __func__, cur, old, enable);
	}

	if(0 == (cur ^ old))
	{
		APS_ERR("%s: error, cur = old = 0x%x\n", __FUNCTION__, cur);
		//return 0;
	}

	if(enable)
	{
		ps_ctrl = atomic_read(&obj->psctrl_val);
		if(obj->hw->polling_mode_ps == 1)
			ps_ctrl &= 0x3F;

		if((err = stk3x1x_write_ps(client, ps_ctrl)))
		{
			APS_ERR("write ps error: %d\n", err);
		}

		if((err = stk3x1x_write_wait(client, obj->wait_val)))
		{
			APS_ERR("write wait error: %d\n", err);
		}

		if((err = stk3x1x_write_int(client, obj->int_val)))
		{
			APS_ERR("write int mode error: %d\n", err);
		}

		err = stk3x1x_write_state(client, 0x02);

#ifdef PS_READ_PROINIF_VALUE
		cali_err = stk3x1x_read_cali_file(client);
		if (cali_err < 0)
		{
			get_calib_flag = 0;
			pwindows_value = between_25mm_none_value - between_40mm_none_value;
			pwave_value = between_40mm_none_value - default_none_value;
			threshold_value = default_none_value;
		}else{
			get_calib_flag = 1;
			pwindows_value =ps_nvraw_25mm_value - ps_nvraw_40mm_value;
			pwave_value = ps_nvraw_40mm_value - ps_nvraw_none_value;
			threshold_value = ps_nvraw_none_value;
		}
#else
		pwindows_value = between_25mm_none_value - between_40mm_none_value;
		pwave_value = between_40mm_none_value - default_none_value;
		threshold_value = default_none_value;
#endif
		min_proximity = obj->hw->ps_threshold_low;
		ps_threshold_l = min_proximity- pwave_value -1;
		ps_threshold_h = min_proximity- pwave_value ;

		ps_thd_val_low_set = ps_threshold_l;
		ps_thd_val_hlgh_set = ps_threshold_h;

	}

	err = stk3x1x_write_state(client, cur);
	if(err < 0)
		return err;
	else
		atomic_set(&obj->state_val, cur);

	if(enable)
	{
		if ((err = stk3x1x_write_ps_high_thd(obj->client, ps_thd_val_hlgh_set)))
		{
			APS_ERR("write high thd error: %d\n", err);
			return err;
		}
		if ((err = stk3x1x_write_ps_low_thd(obj->client, ps_thd_val_low_set)))
		{
			APS_ERR("write low thd error: %d\n", err);
			return err;
		}

		if(stk3x1x_obj->hw->polling_mode_ps == 0)
		{
			enable_irq_wake(obj->irq);
			enable_irq(obj->irq);
		}

	}
	else
	{
		if(stk3x1x_obj->hw->polling_mode_ps == 0){
			disable_irq_nosync(obj->irq);
			disable_irq_wake(obj->irq);
		}
	}

	if(trc & STK_TRC_DEBUG)
	{
		APS_LOG("enable ps  (%d)\n", enable);
	}

	if(err < 0)
		return err;
	else
		return 0;
}

#else

/*----------------------------------------------------------------------------*/
static int stk3x1x_enable_ps(struct i2c_client *client, int enable, int validate_reg)
{
	struct stk3x1x_priv *obj = i2c_get_clientdata(client);
	int err, cur = 0, old = atomic_read(&obj->state_val);
	int trc = atomic_read(&obj->trace);
	struct hwm_sensor_data sensor_data;
	int ps_ctrl;
	u8 temp = 0;

#ifdef STK_PS_DEBUG
	if(enable)
	{
		APS_LOG("%s: before enable ps, HT=%d, LT=%d\n", __func__, atomic_read(&obj->ps_high_thd_val),  atomic_read(&obj->ps_low_thd_val));
		APS_LOG("%s: before enable ps, show all reg\n", __FUNCTION__);
		show_allreg(); //for debug
	}
#endif

	APS_LOG("enable=%d\n", enable);
	stk3x1x_read_flag(obj->client, &temp);
	APS_DBG("read_flag = 0x%x\n", temp);

#ifdef STK_TUNE0
	if (!(obj->psi_set) && !enable)
	{
		hrtimer_cancel(&obj->ps_tune0_timer);
		cancel_work_sync(&obj->stk_ps_tune0_work);
	}
#endif

	if(obj->first_boot == true)
	{
		obj->first_boot = false;
	}

	cur = old;
	cur &= (~(0x45));
	if(enable)
	{
		cur |= (STK_STATE_EN_PS_MASK);
		if(!(old & STK_STATE_EN_ALS_MASK))
			cur |= STK_STATE_EN_WAIT_MASK;
		if(1 == obj->hw->polling_mode_ps)
		{
			wake_lock(&ps_lock);
		}
	}
	else
	{
		if(1 == obj->hw->polling_mode_ps)
		{
			wake_unlock(&ps_lock);
		}
	}

	if(trc & STK_TRC_DEBUG)
	{
		APS_LOG("%s: %08X, %08X, %d\n", __func__, cur, old, enable);
	}

	if(0 == (cur ^ old))
	{
		APS_ERR("%s: error, cur = old = 0x%x\n", __FUNCTION__, cur);
		//return 0;
	}

	if(enable)
	{
		ps_ctrl = atomic_read(&obj->psctrl_val);
		if(obj->hw->polling_mode_ps == 1)
			ps_ctrl &= 0x3F;

		if((err = stk3x1x_write_ps(client, ps_ctrl)))
		{
			APS_ERR("write ps error: %d\n", err);
		}

		if((err = stk3x1x_write_wait(client, obj->wait_val)))
		{
			APS_ERR("write wait error: %d\n", err);
		}

		if((err = stk3x1x_write_int(client, obj->int_val)))
		{
			APS_ERR("write int mode error: %d\n", err);
		}

		err = stk3x1x_write_state(client, 0x02);
	}

	err = stk3x1x_write_state(client, cur);
	if(err < 0)
		return err;
	else
		atomic_set(&obj->state_val, cur);

	if(enable)
	{
#ifdef STK_TUNE0
	#ifndef CALI_EVERY_TIME
		if (!(obj->psi_set))
			hrtimer_start(&obj->ps_tune0_timer, obj->ps_tune0_delay, HRTIMER_MODE_REL);

		if ((err = stk3x1x_write_ps_high_thd(obj->client, atomic_read(&obj->ps_high_thd_val))))
		{
			APS_ERR("write high thd error: %d\n", err);
			return err;
		}
		if ((err = stk3x1x_write_ps_low_thd(obj->client, atomic_read(&obj->ps_low_thd_val))))
		{
			APS_ERR("write low thd error: %d\n", err);
			return err;
		}
	//	APS_LOG("%s: set HT=%d, LT=%d\n", __func__, atomic_read(&obj->ps_high_thd_val), atomic_read(&obj->ps_low_thd_val));
	#else
		if(true)
		{
			obj->psi_set = 0;
			obj->psa = 0;
			obj->psi = 0xFFFF;

			if(obj->boot_cali == 1)
			{//	APS_LOG("%s: line=%d\n", __FUNCTION__, __LINE__);
				atomic_set(&obj->ps_high_thd_val, obj->ps_high_thd_boot);
				atomic_set(&obj->ps_low_thd_val, obj->ps_low_thd_boot);
			}
			else
			{
			//	atomic_set(&obj->ps_high_thd_val, 0xFFFF);
			//	atomic_set(&obj->ps_low_thd_val, 0xFFFF);
			}

			if ((err = stk3x1x_write_ps_high_thd(obj->client, atomic_read(&obj->ps_high_thd_val))))
			{
				APS_ERR("write high thd error: %d\n", err);
				return err;
			}
			if ((err = stk3x1x_write_ps_low_thd(obj->client, atomic_read(&obj->ps_low_thd_val))))
			{
				APS_ERR("write low thd error: %d\n", err);
				return err;
			}
			APS_DBG("%s: set HT=%d, LT=%d\n", __func__, atomic_read(&obj->ps_high_thd_val), atomic_read(&obj->ps_low_thd_val));

			hrtimer_start(&obj->ps_tune0_timer, obj->ps_tune0_delay, HRTIMER_MODE_REL);
		}
	#endif
#endif
		APS_DBG("%s: HT=%d, LT=%d\n", __func__, atomic_read(&obj->ps_high_thd_val), atomic_read(&obj->ps_low_thd_val));

		if((err = stk3x1x_write_wait(obj->client, obj->wait_val)))
        	{
                	APS_ERR("write wait error: %d\n", err);
                	return err;
        	}
#ifdef ONTIM_CALI
	stk_dynamic_calibrate(obj);
#endif
		if((err = stk3x1x_write_int(obj->client, obj->int_val)))
        	{
                	APS_ERR("write int mode error: %d\n", err);
                	return err;
        	}

		if(obj->hw->polling_mode_ps)
		{
			atomic_set(&obj->ps_deb_on, 1);
			atomic_set(&obj->ps_deb_end, jiffies+atomic_read(&obj->ps_debounce)*HZ/1000);
		}
		else
		{
				msleep(4);

				if((err = stk3x1x_read_ps(obj->client, &obj->ps)))
				{
					APS_ERR("stk3x1x read ps data: %d\n", err);
					return err;
				}

				err = stk3x1x_get_ps_value_only(obj, obj->ps);
				if(err < 0)
				{
					APS_ERR("stk3x1x get ps value: %d\n", err);
					return err;
				}
				else if(stk3x1x_obj->hw->polling_mode_ps == 0)
				{
					if(obj->ps == 0)
					{
						err = 1;
						APS_ERR("%s: ps = 0 error, Let val = 1\n", __FUNCTION__);
						stk3x1x_read_flag(obj->client, &temp);
						APS_DBG("%s: read_flag = 0x%x\n", __FUNCTION__, temp); //for debug
						stk3x1x_read_state(obj->client, &temp);
						APS_DBG("%s: read_state = 0x%x\n", __FUNCTION__, temp); //for debug
					}
					sensor_data.values[0] = err;
					sensor_data.value_divide = 1;
					sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
					APS_DBG("ps raw 0x%x -> value 0x%x \n", obj->ps,
									sensor_data.values[0]);
					if(ps_report_interrupt_data(sensor_data.values[0]))
					{
						APS_ERR("call ps_report_interrupt_data fail\n");
					}
					//	if((err = hwmsen_get_interrupt_data(ID_PROXIMITY, &sensor_data)))
					//	APS_ERR("call hwmsen_get_interrupt_data fail = %d\n", err);
					enable_irq_wake(obj->irq);
					enable_irq(obj->irq);
				}

			#ifdef STK_PS_DEBUG
				APS_LOG("%s: after ps_report_interrupt_data, show all reg\n", __FUNCTION__);
				show_allreg(); //for debug
			#endif
		}
	}
	else
	{
		if(stk3x1x_obj->hw->polling_mode_ps == 0){
			disable_irq_nosync(obj->irq);
			disable_irq_wake(obj->irq);
		}
	}

	if(trc & STK_TRC_DEBUG)
	{
		APS_LOG("enable ps  (%d)\n", enable);
	}

	if(err < 0)
		return err;
	else
		return 0;
}

#endif
/*----------------------------------------------------------------------------*/
static int stk3x1x_check_intr(struct i2c_client *client, u8 *status)
{
	struct stk3x1x_priv *obj = i2c_get_clientdata(client);
	int err;

	//if (mt_get_gpio_in(GPIO_ALS_EINT_PIN) == 1) /*skip if no interrupt*/
	//    return 0;

	err = stk3x1x_read_flag(client, status);
	if (err < 0)
	{
		APS_ERR("WARNING: read flag reg error: %d\n", err);
		return -EFAULT;
	}
	APS_DBG("%s: read status reg: 0x%x\n", __func__, *status);

	if(*status & STK_FLG_ALSINT_MASK)
	{
		set_bit(STK_BIT_ALS, &obj->pending_intr);
	}
	else
	{
	   clear_bit(STK_BIT_ALS, &obj->pending_intr);
	}

	if(*status & STK_FLG_PSINT_MASK)
	{
		set_bit(STK_BIT_PS,  &obj->pending_intr);
	}
	else
	{
	    clear_bit(STK_BIT_PS, &obj->pending_intr);
	}

	if(atomic_read(&obj->trace) & STK_TRC_DEBUG)
	{
		APS_LOG("check intr: 0x%02X => 0x%08lX\n", *status, obj->pending_intr);
	}

	return 0;
}


static int stk3x1x_clear_intr(struct i2c_client *client, u8 status, u8 disable_flag)
{
    int err = 0;

    status = status | (STK_FLG_ALSINT_MASK | STK_FLG_PSINT_MASK | STK_FLG_OUI_MASK | STK_FLG_IR_RDY_MASK);
    status &= (~disable_flag);
	//APS_LOG(" set flag reg: 0x%x\n", status);
	if((err = stk3x1x_write_flag(client, status)))
		APS_ERR("stk3x1x_write_flag failed, err=%d\n", err);
    return err;
}

/*----------------------------------------------------------------------------*/
static int stk3x1x_set_als_int_thd(struct i2c_client *client, u16 als_data_reg)
{
	s32 als_thd_h, als_thd_l;

    als_thd_h = als_data_reg + STK_ALS_CODE_CHANGE_THD;
    als_thd_l = als_data_reg - STK_ALS_CODE_CHANGE_THD;
    if (als_thd_h >= (1<<16))
        als_thd_h = (1<<16) -1;
    if (als_thd_l <0)
        als_thd_l = 0;
	APS_DBG("stk3x1x_set_als_int_thd:als_thd_h:%d,als_thd_l:%d\n", als_thd_h, als_thd_l);

	stk3x1x_write_als_high_thd(client, als_thd_h);
	stk3x1x_write_als_low_thd(client, als_thd_l);

	return 0;
}
//+ add by fanxzh
#if 0
static int stk3x1x_ps_val(void)
{
	int mode;
	int32_t word_data, lii;
	u8 buf[4];
	int ret;

	ret = stk3x1x_master_recv(stk3x1x_obj->client, 0x20, buf, 4);
	if(ret < 0)
	{
		APS_ERR("%s fail, err=0x%x", __FUNCTION__, ret);
		return ret;
	}
	word_data = (buf[0] << 8) | buf[1];
	word_data += (buf[2] << 8) | buf[3];

	mode = atomic_read(&stk3x1x_obj->psctrl_val) & 0x3F;
	if(mode == 0x30)
	{
		lii = 100;
	}
	else if (mode == 0x31)
	{
		lii = 200;
	}
	else if (mode == 0x32)
	{
		lii = 400;
	}
	else if (mode == 0x33)
	{
		lii = 800;
	}
	else
	{
		APS_ERR("%s: unsupported PS_IT(0x%x)\n", __FUNCTION__, mode);
		return -1;
	}

	if(word_data > lii)
	{
		APS_LOG( "%s: word_data=%d, lii=%d\n", __FUNCTION__, word_data, lii);
		return 0xFFFF;
	}
	return 0;
}
#endif
//- add by fanxzh

#ifdef STK_TUNE0

static int stk_ps_tune_zero_final(struct stk3x1x_priv *obj)
{
	int err;

	obj->tune_zero_init_proc = false;
	if((err = stk3x1x_write_int(obj->client, obj->int_val)))
	{
		APS_ERR("write int mode error: %d\n", err);
		return err;
	}

	if((err = stk3x1x_write_state(obj->client, atomic_read(&obj->state_val))))
	{
		APS_ERR("write stete error: %d\n", err);
		return err;
	}

	if(obj->data_count == -1)
	{
		APS_ERR("%s: exceed limit\n", __func__);
		hrtimer_cancel(&obj->ps_tune0_timer);
		return 0;
	}

	obj->psa = obj->ps_stat_data[0];
	obj->psi = obj->ps_stat_data[2];

#ifndef CALI_EVERY_TIME
	atomic_set(&obj->ps_high_thd_val, obj->ps_stat_data[1] + obj->stk_ht_n_ct);
	atomic_set(&obj->ps_low_thd_val, obj->ps_stat_data[1] + obj->stk_lt_n_ct);
#else
	obj->ps_high_thd_boot = obj->ps_stat_data[1] + (obj->stk_ht_n_ct * 2);
	obj->ps_low_thd_boot = obj->ps_stat_data[1] + (obj->stk_lt_n_ct * 2);
	atomic_set(&obj->ps_high_thd_val, obj->ps_high_thd_boot);
	atomic_set(&obj->ps_low_thd_val, obj->ps_low_thd_boot);
#endif

	if((err = stk3x1x_write_ps_high_thd(obj->client, atomic_read(&obj->ps_high_thd_val))))
	{
		APS_ERR("write high thd error: %d\n", err);
		return err;
	}
	if((err = stk3x1x_write_ps_low_thd(obj->client, atomic_read(&obj->ps_low_thd_val))))
	{
		APS_ERR("write low thd error: %d\n", err);
		return err;
	}
	obj->boot_cali = 1;

	APS_DBG("%s: set HT=%d,LT=%d\n", __func__, atomic_read(&obj->ps_high_thd_val),  atomic_read(&obj->ps_low_thd_val));
	hrtimer_cancel(&obj->ps_tune0_timer);
	return 0;
}

static int32_t stk_tune_zero_get_ps_data(struct stk3x1x_priv *obj)
{
	int err;

	err = stk3x1x_ps_val();
	if(err == 0xFFFF)
	{
		obj->data_count = -1;
		stk_ps_tune_zero_final(obj);
		return 0;
	}

	if((err = stk3x1x_read_ps(obj->client, &obj->ps)))
	{
		APS_ERR("stk3x1x read ps data: %d\n", err);
		return err;
	}
	APS_DBG("ps #%d=%d\n", obj->data_count, obj->ps);

	obj->ps_stat_data[1]  +=  obj->ps;
	if(obj->ps > obj->ps_stat_data[0])
		obj->ps_stat_data[0] = obj->ps;
	if(obj->ps < obj->ps_stat_data[2])
		obj->ps_stat_data[2] = obj->ps;
	obj->data_count++;

	if(obj->data_count == 5)
	{
		obj->ps_stat_data[1]  /= obj->data_count;
		stk_ps_tune_zero_final(obj);
	}

	return 0;
}

static int stk_ps_tune_zero_init(struct stk3x1x_priv *obj)
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
	if(obj->ps_high_thd_boot <= 0)
	{
		obj->ps_high_thd_boot = obj->stk_ht_n_ct*3;
		obj->ps_low_thd_boot = obj->stk_lt_n_ct*3;
	}
#endif

	if((err = stk3x1x_write_int(obj->client, 0)))
	{
		APS_ERR("write int mode error: %d\n", err);
		return err;
	}

	w_state_reg = (STK_STATE_EN_PS_MASK | STK_STATE_EN_WAIT_MASK);
	if((err = stk3x1x_write_state(obj->client, w_state_reg)))
	{
		APS_ERR("write stete error: %d\n", err);
		return err;
	}
	hrtimer_start(&obj->ps_tune0_timer, obj->ps_tune0_delay, HRTIMER_MODE_REL);
	return 0;
}


static int stk_ps_tune_zero_func_fae(struct stk3x1x_priv *obj)
{
	int32_t word_data;
	u8 flag;
	bool ps_enabled = false;
	u8 buf[2];
	int ret, diff;

	ps_enabled = (atomic_read(&obj->state_val) & STK_STATE_EN_PS_MASK) ? true : false;

#ifndef CALI_EVERY_TIME
	if(obj->psi_set || !(ps_enabled))
#else
	if(!(ps_enabled))
#endif
	{
		return 0;
	}

	ret = stk3x1x_read_flag(obj->client, &flag);
	if(ret < 0)
	{
		APS_ERR( "%s: get flag failed, err=0x%x\n", __func__, ret);
		return ret;
	}
	//if(!(flag&STK_FLG_PSDR_MASK))
	//{
	//	return 0;
	//}

	ret = stk3x1x_ps_val();
	if(ret == 0)
	{
		ret = stk3x1x_master_recv(obj->client, 0x11, buf, 2);
		if(ret < 0)
		{
			APS_ERR( "%s fail, err=0x%x", __func__, ret);
			return ret;
		}
		word_data = (buf[0] << 8) | buf[1];
		//APS_LOG("%s: word_data=%d\n", __func__, word_data);

		if((word_data == 0) && (!(flag&STK_FLG_PSDR_MASK)))
		{
			APS_ERR( "%s: incorrect word data (0)\n", __func__);
			return 0xFFFF;
		}

		if(word_data > obj->psa)
		{
			obj->psa = word_data;
			APS_LOG("%s: update psa: psa=%d,psi=%d\n", __func__, obj->psa, obj->psi);
		}
		if(word_data < obj->psi)
		{
			obj->psi = word_data;
			APS_LOG("%s: update psi: psa=%d,psi=%d\n", __func__, obj->psa, obj->psi);
		}
	}

	diff = obj->psa - obj->psi;
	if(diff > obj->stk_max_min_diff)
	{
		obj->psi_set = obj->psi;
		atomic_set(&obj->ps_high_thd_val, obj->psi + obj->stk_ht_n_ct);
		atomic_set(&obj->ps_low_thd_val, obj->psi + obj->stk_lt_n_ct);

#ifdef CALI_EVERY_TIME
		APS_DBG("%s: boot HT=%d, LT=%d\n", __func__, obj->ps_high_thd_boot, obj->ps_low_thd_boot);
		if( (atomic_read(&obj->ps_high_thd_val) > (obj->ps_high_thd_boot + 500)) && (obj->boot_cali == 1) )
		{
			atomic_set(&obj->ps_high_thd_val, obj->ps_high_thd_boot);
			atomic_set(&obj->ps_low_thd_val, obj->ps_low_thd_boot);
			APS_DBG("%s: change THD to boot HT=%d, LT=%d\n", __func__, atomic_read(&obj->ps_high_thd_val), atomic_read(&obj->ps_low_thd_val));
		}
		else
		{
			if((atomic_read(&obj->ps_high_thd_val) + 500) < (obj->ps_high_thd_boot))
			{
				obj->ps_high_thd_boot = obj->psi + (obj->stk_ht_n_ct * 2);
				obj->ps_low_thd_boot = obj->psi + (obj->stk_lt_n_ct * 2);
				obj->boot_cali = 1;
				APS_DBG("%s: update boot HT=%d, LT=%d\n", __func__, obj->ps_high_thd_boot, obj->ps_low_thd_boot);
			}
		}
#endif

		if((ret = stk3x1x_write_ps_high_thd(obj->client, atomic_read(&obj->ps_high_thd_val))))
		{
			APS_ERR("write high thd error: %d\n", ret);
			return ret;
		}
		if((ret = stk3x1x_write_ps_low_thd(obj->client, atomic_read(&obj->ps_low_thd_val))))
		{
			APS_ERR("write low thd error: %d\n", ret);
			return ret;
		}
#ifdef STK_DEBUG_PRINTF
		APS_LOG("%s: FAE tune0 psa-psi(%d) > DIFF found\n", __func__, diff);
#endif
		APS_LOG("%s: set HT=%d, LT=%d\n", __func__, atomic_read(&obj->ps_high_thd_val), atomic_read(&obj->ps_low_thd_val));
		hrtimer_cancel(&obj->ps_tune0_timer);
	}

	return 0;
}
#endif	/*#ifdef STK_TUNE0	*/
#ifdef STK_TUNE0
static void stk_ps_tune0_work_func(struct work_struct *work)
{
	struct stk3x1x_priv *obj = container_of(work, struct stk3x1x_priv, stk_ps_tune0_work);
#ifdef STK_GES
	if(obj->ges_enabled)
		stk_ges_poll_func(obj);
#endif
	if(obj->tune_zero_init_proc)
		stk_tune_zero_get_ps_data(obj);
	else
		stk_ps_tune_zero_func_fae(obj);
	return;
}


static enum hrtimer_restart stk_ps_tune0_timer_func(struct hrtimer *timer)
{
	struct stk3x1x_priv *obj = container_of(timer, struct stk3x1x_priv, ps_tune0_timer);
	queue_work(obj->stk_ps_tune0_wq, &obj->stk_ps_tune0_work);
	hrtimer_forward_now(&obj->ps_tune0_timer, obj->ps_tune0_delay);
	return HRTIMER_RESTART;
}
#endif
/*----------------------------------------------------------------------------*/


void stk3x1x_eint_func(void)
{
	struct stk3x1x_priv *obj = g_stk3x1x_ptr;

	APS_DBG(" enter.\n");
	if(!obj)
	{
		return;
	}
	//schedule_work(&obj->eint_work);
	if(obj->hw->polling_mode_ps == 0 || obj->hw->polling_mode_als == 0)
		schedule_delayed_work(&obj->eint_work,0);
	if(atomic_read(&obj->trace) & STK_TRC_EINT)
	{
		APS_LOG("eint: als/ps intrs\n");
	}
}

#ifdef CONFIG_OF
static irqreturn_t stk3x1x_irq_func(int irq, void *desc)
{
	disable_irq_nosync(stk3x1x_obj->irq);
	disable_irq_wake(stk3x1x_obj->irq);
	stk3x1x_eint_func();
	return IRQ_HANDLED;
}
#endif

/*----------------------------------------------------------------------------*/
#ifdef HW_dynamic_cali
static void stk3x1x_eint_work(struct work_struct *work)
{
	struct stk3x1x_priv *obj = g_stk3x1x_ptr;
	int err =0;
	int distance = 0;
	struct hwm_sensor_data sensor_data;
	u8 flag_reg, disable_flag = 0;

	memset(&sensor_data, 0, sizeof(sensor_data));

	APS_DBG(" enter.\n");

	if((err = stk3x1x_check_intr(obj->client, &flag_reg)))
	{
		APS_ERR("stk3x1x_check_intr fail: %d\n", err);
		goto err_i2c_rw;
	}

	APS_DBG(" &obj->pending_intr =%lx\n",obj->pending_intr);

	if(((1<<STK_BIT_ALS) & obj->pending_intr) && (obj->hw->polling_mode_als == 0))
	{
		//get raw data
		APS_DBG("stk als change\n");
		disable_flag |= STK_FLG_ALSINT_MASK;
		if((err = stk3x1x_read_als(obj->client, &obj->als)))
		{
			APS_ERR("stk3x1x_read_als failed %d\n", err);
			goto err_i2c_rw;

		}

		stk3x1x_set_als_int_thd(obj->client, obj->als);
		sensor_data.values[0] = stk3x1x_get_als_value(obj, obj->als);
		sensor_data.value_divide = 1;
		sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
		APS_DBG("%s:als raw 0x%x -> value 0x%x \n", __FUNCTION__, obj->als,sensor_data.values[0]);

		if(ps_report_interrupt_data(sensor_data.values[0]))
		{
			APS_ERR("call ps_report_interrupt_data fail\n");
		}
	}
	if(((1<<STK_BIT_PS) &  obj->pending_intr) && (obj->hw->polling_mode_ps == 0))
	{
		APS_DBG("stk ps change\n");
		disable_flag |= STK_FLG_PSINT_MASK;

		if((err = stk3x1x_read_ps(obj->client, &obj->ps)))
		{
			APS_ERR("stk3x1x read ps data: %d\n", err);
			goto err_i2c_rw;
		}
		distance = stk3x1x_get_ps_value_only(obj, obj->ps);
		if(distance < 0)
		{
			APS_ERR("stk3x1x get ps value: %d\n", err);
			return;
		}

#ifdef SMT_MODE
		ps_thd_val_low_set = ps_nvraw_40mm_value;
		ps_thd_val_hlgh_set = ps_nvraw_25mm_value;
#endif
		if ((err = stk3x1x_write_ps_high_thd(obj->client, ps_thd_val_hlgh_set)))
		{
        		APS_ERR("write high thd error: %d\n", err);
        		return ;
		}
		if ((err = stk3x1x_write_ps_low_thd(obj->client, ps_thd_val_low_set)))
		{
        		APS_ERR("write low thd error: %d\n", err);
        		return ;
		}
		APS_DBG("%s: set HT=%d, LT=%d\n", __func__, ps_thd_val_hlgh_set, ps_thd_val_low_set);


		sensor_data.values[0] = distance;
		sensor_data.value_divide = 1;
		sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
		APS_DBG("%s:ps raw 0x%x -> value 0x%x \n",__func__, obj->ps,sensor_data.values[0]);

		if(ps_report_interrupt_data(sensor_data.values[0]))
		{
			APS_ERR("call ps_report_interrupt_data fail\n");
		}

#ifdef STK_PS_DEBUG
		APS_DBG("%s: ps interrupt, show all reg\n", __FUNCTION__);
		show_allreg(); //for debug
#endif
	}
	if(disable_flag)
	{
		if((err = stk3x1x_clear_intr(obj->client, flag_reg, disable_flag)))
		{
			APS_ERR("fail: %d\n", err);
			goto err_i2c_rw;
		}
	}

	msleep(1);
	enable_irq_wake(obj->irq);
	enable_irq(obj->irq);

	return ;

err_i2c_rw:
	msleep(30);
	enable_irq_wake(obj->irq);
	enable_irq(obj->irq);
	return;
}

#else

static void stk3x1x_eint_work(struct work_struct *work)
{
	struct stk3x1x_priv *obj = g_stk3x1x_ptr;
	int err;
	struct hwm_sensor_data sensor_data;
	u8 flag_reg, disable_flag = 0;

	memset(&sensor_data, 0, sizeof(sensor_data));

	APS_DBG(" enter.\n");

	if((err = stk3x1x_check_intr(obj->client, &flag_reg)))
	{
		APS_ERR("stk3x1x_check_intr fail: %d\n", err);
		goto err_i2c_rw;
	}

	APS_DBG(" &obj->pending_intr =%lx\n",obj->pending_intr);

	if(((1<<STK_BIT_ALS) & obj->pending_intr) && (obj->hw->polling_mode_als == 0))
	{
		//get raw data
		APS_DBG("stk als change\n");
		disable_flag |= STK_FLG_ALSINT_MASK;
		if((err = stk3x1x_read_als(obj->client, &obj->als)))
		{
			APS_ERR("stk3x1x_read_als failed %d\n", err);
			goto err_i2c_rw;

		}

		stk3x1x_set_als_int_thd(obj->client, obj->als);
		sensor_data.values[0] = stk3x1x_get_als_value(obj, obj->als);
		sensor_data.value_divide = 1;
		sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
		APS_DBG("%s:als raw 0x%x -> value 0x%x \n", __FUNCTION__, obj->als,sensor_data.values[0]);
		//let up layer to know
/*
		if((err = hwmsen_get_interrupt_data(ID_LIGHT, &sensor_data)))
		{
			APS_ERR("call hwmsen_get_interrupt_data fail = %d\n", err);
		}
*/
		if(ps_report_interrupt_data(sensor_data.values[0]))
		{
			APS_ERR("call ps_report_interrupt_data fail\n");
		}
	}
	if(((1<<STK_BIT_PS) &  obj->pending_intr) && (obj->hw->polling_mode_ps == 0))
	{
		APS_DBG("stk ps change\n");
		disable_flag |= STK_FLG_PSINT_MASK;

		if((err = stk3x1x_read_ps(obj->client, &obj->ps)))
		{
			APS_ERR("stk3x1x read ps data: %d\n", err);
			goto err_i2c_rw;
		}
#ifdef ONTIM_CALI
		if ((ps_cali.noice > -1)&&( ps_cali.close > ps_cali.far_away) && (obj->ps < dynamic_calibrate) )
		{
			int temp_high =  ps_cali.close  -ps_cali.noice + obj->ps;
			int temp_low =  ps_cali.far_away  -ps_cali.noice + obj->ps;
			if ((temp_high < CALI_HIGH_THD) && (temp_low > CALI_LOW_THD) && (obj->ps > ps_cali.noice )  && (dynamic_calibrate < ((ps_cali.close +ps_cali.far_away)/2 )))
			{
				atomic_set(&obj->ps_high_thd_val,  temp_high);
				atomic_set(&obj->ps_low_thd_val,  temp_low);
			}
			else
			{
				atomic_set(&obj->ps_high_thd_val,  ps_cali.close);
				atomic_set(&obj->ps_low_thd_val,  ps_cali.far_away);
			}
			dynamic_calibrate = obj->ps;
			if ((err = stk3x1x_write_ps_high_thd(obj->client, atomic_read(&obj->ps_high_thd_val))))
    		{
            		APS_ERR("write high thd error: %d\n", err);
            		return ;
    		}
    		if ((err = stk3x1x_write_ps_low_thd(obj->client, atomic_read(&obj->ps_low_thd_val))))
    		{
            		APS_ERR("write low thd error: %d\n", err);
            		return ;
    		}
    		APS_DBG("%s: set HT=%d, LT=%d\n", __func__, atomic_read(&obj->ps_high_thd_val), atomic_read(&obj->ps_low_thd_val));
		}

		APS_DBG("cali noice=%d; cali high=%d; cali low=%d; curr noice=%d; high=%d; low=%d\n",ps_cali.noice,ps_cali.close,ps_cali.far_away,dynamic_calibrate,atomic_read(&obj->ps_high_thd_val),atomic_read(&obj->ps_low_thd_val));
#endif
		sensor_data.values[0] = (flag_reg & STK_FLG_NF_MASK)? 1 : 0;
		sensor_data.value_divide = 1;
		sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
		APS_DBG("%s:ps raw 0x%x -> value 0x%x \n",__FUNCTION__, obj->ps,sensor_data.values[0]);
		//let up layer to know
/*
		if((err = hwmsen_get_interrupt_data(ID_PROXIMITY, &sensor_data)))
		{
			APS_ERR("call hwmsen_get_interrupt_data fail = %d\n", err);
		}
*/
		if(ps_report_interrupt_data(sensor_data.values[0]))
		{
			APS_ERR("call ps_report_interrupt_data fail\n");
		}

#ifdef STK_PS_DEBUG
		APS_LOG("%s: ps interrupt, show all reg\n", __FUNCTION__);
		show_allreg(); //for debug
#endif
	}
	if(disable_flag)
	{
		if((err = stk3x1x_clear_intr(obj->client, flag_reg, disable_flag)))
		{
			APS_ERR("fail: %d\n", err);
			goto err_i2c_rw;
		}
	}

	msleep(1);
	enable_irq_wake(obj->irq);
	enable_irq(obj->irq);

	return;

err_i2c_rw:
	msleep(30);
	enable_irq_wake(obj->irq);
	enable_irq(obj->irq);
	return;
}
#endif
/*----------------------------------------------------------------------------*/
int stk3x1x_setup_eint(struct i2c_client *client)
{
	int ret;
        struct pinctrl *pinctrl;
        struct pinctrl_state *pins_default;
        struct pinctrl_state *pins_cfg;
        u32 ints[2] = {0, 0};
        //struct platform_device *alspsPltFmDev;
	struct stk3x1x_priv *obj = i2c_get_clientdata(client);

	g_stk3x1x_ptr = obj;
	/*configure to GPIO function, external interrupt*/

/* gpio setting */
	stk3x1x_obj->irq_node = of_find_compatible_node(NULL,NULL,"mediatek,alsps");
	pinctrl = devm_pinctrl_get(&client->dev);
	if (IS_ERR(pinctrl)) {
		ret = PTR_ERR(pinctrl);
		APS_ERR("Cannot find alsps pinctrl!\n");
		return ret;
	}
	pins_default = pinctrl_lookup_state(pinctrl, "pin_default");
	if (IS_ERR(pins_default)) {
		ret = PTR_ERR(pins_default);
		APS_ERR("Cannot find alsps pinctrl default!\n");

	}

	pins_cfg = pinctrl_lookup_state(pinctrl, "pin_cfg");
	if (IS_ERR(pins_cfg)) {
		ret = PTR_ERR(pins_cfg);
		APS_ERR("Cannot find alsps pinctrl pin_cfg!\n");
		return ret;

	}
	pinctrl_select_state(pinctrl, pins_cfg);
/* eint request */
	if (stk3x1x_obj->irq_node) {

		ret = of_property_read_u32_array(stk3x1x_obj->irq_node, "debounce", ints, ARRAY_SIZE(ints));
		if (ret) {
			APS_ERR("of_property_read_u32_array fail, ret = %d\n", ret);

		}
		gpio_set_debounce(ints[0], ints[1]);
		APS_DBG("ints[0] = %d, ints[1] = %d!!\n", ints[0], ints[1]);


		stk3x1x_obj->irq = irq_of_parse_and_map(stk3x1x_obj->irq_node, 0);
		APS_LOG("stk3x1x_obj->irq = %d\n", stk3x1x_obj->irq);
		if (!stk3x1x_obj->irq) {
			APS_ERR("irq_of_parse_and_map fail!!\n");
			return -EINVAL;
		}
		if (request_irq(stk3x1x_obj->irq, stk3x1x_irq_func, IRQF_TRIGGER_LOW, "ALS-eint", NULL)) {
			APS_ERR("IRQ LINE NOT AVAILABLE!!\n");
			return -EINVAL;
		}
		enable_irq_wake(stk3x1x_obj->irq);

		//disable_irq(stk3x1x_obj->irq);
	} else {
		APS_ERR("null irq node!!\n");
		return -EINVAL;
	}


    return 0;
}
/*----------------------------------------------------------------------------*/
static int stk3x1x_init_client(struct i2c_client *client)
{
	struct stk3x1x_priv *obj = i2c_get_clientdata(client);
	int err;
	int ps_ctrl;
	//u8 int_status;

	if((err = stk3x1x_write_sw_reset(client)))
	{
		APS_ERR("software reset error, err=%d", err);
		return err;
	}

	if((err = stk3x1x_read_id(client)))
	{
		APS_ERR("stk3x1x_read_id error, err=%d", err);
		return err;
	}

/*
	if(obj->first_boot == true)
	{
		if(obj->hw->polling_mode_ps == 0 || obj->hw->polling_mode_als == 0)
		{
			if((err = stk3x1x_setup_eint(client)))
			{
				APS_ERR("setup eint error: %d\n", err);
				return err;
			}
		}
	}
*/
	if((err = stk3x1x_write_state(client, atomic_read(&obj->state_val))))
	{
		APS_ERR("write stete error: %d\n", err);
		return err;
	}

/*
	if((err = stk3x1x_check_intr(client, &int_status)))
	{
		APS_ERR("check intr error: %d\n", err);
		return err;
	}

	if((err = stk3x1x_clear_intr(client, int_status, STK_FLG_PSINT_MASK | STK_FLG_ALSINT_MASK)))
	{
		APS_ERR("clear intr error: %d\n", err);
		return err;
	}
*/
	ps_ctrl = atomic_read(&obj->psctrl_val);
	if(obj->hw->polling_mode_ps == 1)
		ps_ctrl &= 0x3F;

	if((err = stk3x1x_write_ps(client, ps_ctrl)))
	{
		APS_ERR("write ps error: %d\n", err);
		return err;
	}

	if((err = stk3x1x_write_als(client, atomic_read(&obj->alsctrl_val))))
	{
		APS_ERR("write als error: %d\n", err);
		return err;
	}

	if((err = stk3x1x_write_led(client, obj->ledctrl_val)))
	{
		APS_ERR("write led error: %d\n", err);
		return err;
	}

	if((err = stk3x1x_write_wait(client, obj->wait_val)))
	{
		APS_ERR("write wait error: %d\n", err);
		return err;
	}
#ifndef STK_TUNE0
	if((err = stk3x1x_write_ps_high_thd(client, atomic_read(&obj->ps_high_thd_val))))
	{
		APS_ERR("write high thd error: %d\n", err);
		return err;
	}

	if((err = stk3x1x_write_ps_low_thd(client, atomic_read(&obj->ps_low_thd_val))))
	{
		APS_ERR("write low thd error: %d\n", err);
		return err;
	}
#endif
	if((err = stk3x1x_write_int(client, obj->int_val)))
	{
		APS_ERR("write int mode error: %d\n", err);
		return err;
	}

	/*
	u8 data;
	data = 0x60;
    err = stk3x1x_master_send(client, 0x87, &data, 1);
	if (err < 0)
	{
		APS_ERR("write 0x87 = %d\n", err);
		return -EFAULT;
	}
	*/
#ifdef STK_ALS_FIR
	memset(&obj->fir, 0x00, sizeof(obj->fir));
#endif
#ifdef STK_TUNE0
	if(obj->first_boot == true)
		stk_ps_tune_zero_init(obj);
#endif
	obj->re_enable_ps = false;
	obj->re_enable_als = false;
	obj->als_code_last = 500;

	return 0;
}

/******************************************************************************
 * Sysfs attributes
*******************************************************************************/
static ssize_t stk3x1x_show_config(struct device_driver *ddri, char *buf)
{
	ssize_t res;

	if(!stk3x1x_obj)
	{
		APS_ERR("stk3x1x_obj is null!!\n");
		return 0;
	}

	res = scnprintf(buf, PAGE_SIZE, "(%d %d %d %d %d %d)\n",
	atomic_read(&stk3x1x_obj->i2c_retry), atomic_read(&stk3x1x_obj->als_debounce),
	atomic_read(&stk3x1x_obj->ps_mask), atomic_read(&stk3x1x_obj->ps_high_thd_val),atomic_read(&stk3x1x_obj->ps_low_thd_val), atomic_read(&stk3x1x_obj->ps_debounce));
	return res;
}
/*----------------------------------------------------------------------------*/
static ssize_t stk3x1x_store_config(struct device_driver *ddri, const char *buf, size_t count)
{
	int retry, als_deb, ps_deb, mask, hthres, lthres, err;
	struct i2c_client *client;
	client = stk3x1x_i2c_client;
	if(!stk3x1x_obj)
	{
		APS_ERR("stk3x1x_obj is null!!\n");
		return 0;
	}

	if(6 == sscanf(buf, "%d %d %d %d %d %d", &retry, &als_deb, &mask, &hthres, &lthres, &ps_deb))
	{
		atomic_set(&stk3x1x_obj->i2c_retry, retry);
		atomic_set(&stk3x1x_obj->als_debounce, als_deb);
		atomic_set(&stk3x1x_obj->ps_mask, mask);
		atomic_set(&stk3x1x_obj->ps_high_thd_val, hthres);
		atomic_set(&stk3x1x_obj->ps_low_thd_val, lthres);
		atomic_set(&stk3x1x_obj->ps_debounce, ps_deb);

		if((err = stk3x1x_write_ps_high_thd(client, atomic_read(&stk3x1x_obj->ps_high_thd_val))))
		{
			APS_ERR("write high thd error: %d\n", err);
			return err;
		}

		if((err = stk3x1x_write_ps_low_thd(client, atomic_read(&stk3x1x_obj->ps_low_thd_val))))
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
static ssize_t stk3x1x_show_trace(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	if(!stk3x1x_obj)
	{
		APS_ERR("stk3x1x_obj is null!!\n");
		return 0;
	}

	res = scnprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&stk3x1x_obj->trace));
	return res;
}
/*----------------------------------------------------------------------------*/
static ssize_t stk3x1x_store_trace(struct device_driver *ddri, const char *buf, size_t count)
{
    int trace;
    if(!stk3x1x_obj)
	{
		APS_ERR("stk3x1x_obj is null!!\n");
		return 0;
	}

	if(1 == sscanf(buf, "0x%x", &trace))
	{
		atomic_set(&stk3x1x_obj->trace, trace);
	}
	else
	{
		APS_ERR("invalid content: '%s', length = %d\n", buf, (int)count);
	}
	return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t stk3x1x_show_ir(struct device_driver *ddri, char *buf)
{
    int32_t reading;

	if(!stk3x1x_obj)
	{
		APS_ERR("stk3x1x_obj is null!!\n");
		return 0;
	}
    reading = stk3x1x_get_ir_value(stk3x1x_obj, STK_IRS_IT_REDUCE);
	if(reading < 0)
		return scnprintf(buf, PAGE_SIZE, "ERROR: %d\n", reading);

	stk3x1x_obj->ir_code = reading;
	return scnprintf(buf, PAGE_SIZE, "0x%04X\n", stk3x1x_obj->ir_code);
}

/*----------------------------------------------------------------------------*/
static ssize_t stk3x1x_show_als(struct device_driver *ddri, char *buf)
{
	int res;

	if(!stk3x1x_obj)
	{
		APS_ERR("stk3x1x_obj is null!!\n");
		return 0;
	}

	if(stk3x1x_obj->hw->polling_mode_als == 0)
	{
		if((res = stk3x1x_read_als(stk3x1x_obj->client, &stk3x1x_obj->als)))
			return scnprintf(buf, PAGE_SIZE, "ERROR: %d\n", res);
		else
			return scnprintf(buf, PAGE_SIZE, "0x%04X\n", stk3x1x_obj->als);
	}
	return scnprintf(buf, PAGE_SIZE, "0x%04X\n", stk3x1x_obj->als_code_last);
}
/*----------------------------------------------------------------------------*/
static ssize_t stk3x1x_show_ps(struct device_driver *ddri, char *buf)
{
	int res;
	if(!stk3x1x_obj)
	{
		APS_ERR("stk3x1x_obj is null!!\n");
		return 0;
	}

	if((res = stk3x1x_read_ps(stk3x1x_obj->client, &stk3x1x_obj->ps)))
	{
		return scnprintf(buf, PAGE_SIZE, "ERROR: %d\n", res);
	}
	else
	{
		return scnprintf(buf, PAGE_SIZE, "0x%04X\n", stk3x1x_obj->ps);
	}
}
/*----------------------------------------------------------------------------*/
static ssize_t stk3x1x_show_reg(struct device_driver *ddri, char *buf)
{
	u8 int_status;
	if(!stk3x1x_obj)
	{
		APS_ERR("stk3x1x_obj is null!!\n");
		return 0;
	}

	/*read*/
	stk3x1x_check_intr(stk3x1x_obj->client, &int_status);
	//stk3x1x_clear_intr(stk3x1x_obj->client, int_status, 0x0);
	stk3x1x_read_ps(stk3x1x_obj->client, &stk3x1x_obj->ps);
	stk3x1x_read_als(stk3x1x_obj->client, &stk3x1x_obj->als);
	/*write*/
	stk3x1x_write_als(stk3x1x_obj->client, atomic_read(&stk3x1x_obj->alsctrl_val));
	stk3x1x_write_ps(stk3x1x_obj->client, atomic_read(&stk3x1x_obj->psctrl_val));
	stk3x1x_write_ps_high_thd(stk3x1x_obj->client, atomic_read(&stk3x1x_obj->ps_high_thd_val));
	stk3x1x_write_ps_low_thd(stk3x1x_obj->client, atomic_read(&stk3x1x_obj->ps_low_thd_val));
	return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t stk3x1x_show_send(struct device_driver *ddri, char *buf)
{
    return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t stk3x1x_store_send(struct device_driver *ddri, const char *buf, size_t count)
{
	int addr, cmd;
	u8 dat;

	if(!stk3x1x_obj)
	{
		APS_ERR("stk3x1x_obj is null!!\n");
		return 0;
	}
	else if(2 != sscanf(buf, "%x %x", &addr, &cmd))
	{
		APS_ERR("invalid format: '%s'\n", buf);
		return 0;
	}

	dat = (u8)cmd;
	APS_LOG("send(%02X, %02X) = %d\n", addr, cmd,
	stk3x1x_master_send(stk3x1x_obj->client, (u16)addr, &dat, sizeof(dat)));

	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t stk3x1x_show_recv(struct device_driver *ddri, char *buf)
{
	if(!stk3x1x_obj)
	{
		APS_ERR("stk3x1x_obj is null!!\n");
		return 0;
	}
	return scnprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&stk3x1x_obj->recv_reg));
}
/*----------------------------------------------------------------------------*/
static ssize_t stk3x1x_store_recv(struct device_driver *ddri, const char *buf, size_t count)
{
	int addr;
	u8 dat;
	if(!stk3x1x_obj)
	{
		APS_ERR("stk3x1x_obj is null!!\n");
		return 0;
	}
	else if(1 != sscanf(buf, "%x", &addr))
	{
		APS_ERR("invalid format: '%s'\n", buf);
		return 0;
	}

	APS_LOG("recv(%02X) = %d, 0x%02X\n", addr,
	stk3x1x_master_recv(stk3x1x_obj->client, (u16)addr, (char*)&dat, sizeof(dat)), dat);
	atomic_set(&stk3x1x_obj->recv_reg, dat);
	return count;
}
/*----------------------------------------------------------------------------*/
#ifdef STK_PS_DEBUG
static int show_allreg(void)
{
	int ret = 0;
	u8 rbuf[0x22];
	int cnt;
	int len = 0;

	memset(rbuf, 0, sizeof(rbuf));
	if(!stk3x1x_obj)
	{
		APS_ERR("stk3x1x_obj is null!!\n");
		return 0;
	}
	ret = stk3x1x_master_recv(stk3x1x_obj->client, 0, &rbuf[0], 7);
	if(ret < 0)
	{
		APS_ERR("error: %d\n", ret);
		return -EFAULT;
	}
	ret = stk3x1x_master_recv(stk3x1x_obj->client, 7, &rbuf[7], 7);
	if(ret < 0)
	{
		APS_ERR("error: %d\n", ret);
		return -EFAULT;
	}
	ret = stk3x1x_master_recv(stk3x1x_obj->client, 14, &rbuf[14], 7);
	if(ret < 0)
	{
		APS_ERR("error: %d\n", ret);
		return -EFAULT;
	}
	ret = stk3x1x_master_recv(stk3x1x_obj->client, 21, &rbuf[21], 7);
	if(ret < 0)
	{
		APS_ERR("error: %d\n", ret);
		return -EFAULT;
	}
	ret = stk3x1x_master_recv(stk3x1x_obj->client, 28, &rbuf[28], 4);
	if(ret < 0)
	{
		APS_ERR("error: %d\n", ret);
		return -EFAULT;
	}

	ret = stk3x1x_master_recv(stk3x1x_obj->client, STK_PDT_ID_REG, &rbuf[32], 2);
	if(ret < 0)
	{
		APS_ERR("error: %d\n", ret);
		return -EFAULT;
	}

	for(cnt=0;cnt<0x20;cnt++)
	{
		APS_LOG("reg[0x%x]=0x%x\n", cnt, rbuf[cnt]);
	}
	APS_LOG("reg[0x3E]=0x%x\n", rbuf[cnt]);
	APS_LOG("reg[0x3F]=0x%x\n", rbuf[cnt++]);

	return 0;
}
#endif
static ssize_t stk3x1x_show_allreg(struct device_driver *ddri, char *buf)
{
	int ret = 0;
	u8 rbuf[0x22];
	int cnt;
	int len = 0;

	memset(rbuf, 0, sizeof(rbuf));
	if(!stk3x1x_obj)
	{
		APS_ERR("stk3x1x_obj is null!!\n");
		return 0;
	}
	ret = stk3x1x_master_recv(stk3x1x_obj->client, 0, &rbuf[0], 7);
	if(ret < 0)
	{
		APS_ERR("error: %d\n", ret);
		return -EFAULT;
	}
	ret = stk3x1x_master_recv(stk3x1x_obj->client, 7, &rbuf[7], 7);
	if(ret < 0)
	{
		APS_ERR("error: %d\n", ret);
		return -EFAULT;
	}
	ret = stk3x1x_master_recv(stk3x1x_obj->client, 14, &rbuf[14], 7);
	if(ret < 0)
	{
		APS_ERR("error: %d\n", ret);
		return -EFAULT;
	}
	ret = stk3x1x_master_recv(stk3x1x_obj->client, 21, &rbuf[21], 7);
	if(ret < 0)
	{
		APS_ERR("error: %d\n", ret);
		return -EFAULT;
	}
	ret = stk3x1x_master_recv(stk3x1x_obj->client, 28, &rbuf[28], 4);
	if(ret < 0)
	{
		APS_ERR("error: %d\n", ret);
		return -EFAULT;
	}

	ret = stk3x1x_master_recv(stk3x1x_obj->client, STK_PDT_ID_REG, &rbuf[32], 2);
	if(ret < 0)
	{
		APS_ERR("error: %d\n", ret);
		return -EFAULT;
	}

	for(cnt=0;cnt<0x20;cnt++)
	{
		APS_LOG("reg[0x%x]=0x%x\n", cnt, rbuf[cnt]);
		len += scnprintf(buf+len, PAGE_SIZE-len, "[%2X]%2X,", cnt, rbuf[cnt]);
	}
	APS_LOG("reg[0x3E]=0x%x\n", rbuf[cnt]);
	APS_LOG("reg[0x3F]=0x%x\n", rbuf[cnt++]);
	len += scnprintf(buf+len, PAGE_SIZE-len, "[0x3E]%2X,[0x3F]%2X\n", rbuf[cnt-1], rbuf[cnt]);
	return len;
	/*
    return scnprintf(buf, PAGE_SIZE, "[0]%2X [1]%2X [2]%2X [3]%2X [4]%2X [5]%2X [6/7 HTHD]%2X,%2X [8/9 LTHD]%2X, %2X [A]%2X [B]%2X [C]%2X [D]%2X [E/F Aoff]%2X,%2X,[10]%2X [11/12 PS]%2X,%2X [13]%2X [14]%2X [15/16 Foff]%2X,%2X [17]%2X [18]%2X [3E]%2X [3F]%2X\n",
		rbuf[0], rbuf[1], rbuf[2], rbuf[3], rbuf[4], rbuf[5], rbuf[6], rbuf[7], rbuf[8],
		rbuf[9], rbuf[10], rbuf[11], rbuf[12], rbuf[13], rbuf[14], rbuf[15], rbuf[16], rbuf[17],
		rbuf[18], rbuf[19], rbuf[20], rbuf[21], rbuf[22], rbuf[23], rbuf[24], rbuf[25], rbuf[26]);
	*/
}
/*----------------------------------------------------------------------------*/
static ssize_t stk3x1x_show_status(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	u8 rbuf[25];
	int ret = 0;

	if(!stk3x1x_obj)
	{
		APS_ERR("stk3x1x_obj is null!!\n");
		return 0;
	}

	if(stk3x1x_obj->hw)
	{
		len += scnprintf(buf+len, PAGE_SIZE-len, "CUST: %d, (%d %d) (%02X) (%02X %02X %02X) (%02X %02X %02X %02X)\n",
		stk3x1x_obj->hw->i2c_num, stk3x1x_obj->hw->power_id, stk3x1x_obj->hw->power_vol, stk3x1x_obj->addr.flag,
		stk3x1x_obj->addr.alsctrl, stk3x1x_obj->addr.data1_als, stk3x1x_obj->addr.data2_als, stk3x1x_obj->addr.psctrl,
		stk3x1x_obj->addr.data1_ps, stk3x1x_obj->addr.data2_ps, stk3x1x_obj->addr.thdh1_ps);
	}
	else
	{
		len += scnprintf(buf+len, PAGE_SIZE-len, "CUST: NULL\n");
	}

	len += scnprintf(buf+len, PAGE_SIZE-len, "REGS: %02X %02X %02X %02X %02X %02X %02X %02X %02lX %02lX\n",
				atomic_read(&stk3x1x_obj->state_val), atomic_read(&stk3x1x_obj->psctrl_val), atomic_read(&stk3x1x_obj->alsctrl_val),
				stk3x1x_obj->ledctrl_val, stk3x1x_obj->int_val, stk3x1x_obj->wait_val,
				atomic_read(&stk3x1x_obj->ps_high_thd_val), atomic_read(&stk3x1x_obj->ps_low_thd_val),stk3x1x_obj->enable, stk3x1x_obj->pending_intr);

	len += scnprintf(buf+len, PAGE_SIZE-len, "MISC: %d %d\n", atomic_read(&stk3x1x_obj->als_suspend), atomic_read(&stk3x1x_obj->ps_suspend));
	len += scnprintf(buf+len, PAGE_SIZE-len, "VER.: %s\n", DRIVER_VERSION);

	memset(rbuf, 0, sizeof(rbuf));
	ret = stk3x1x_master_recv(stk3x1x_obj->client, 0, &rbuf[0], 7);
	if(ret < 0)
	{
		APS_ERR("error: %d\n", ret);
		return -EFAULT;
	}
	ret = stk3x1x_master_recv(stk3x1x_obj->client, 7, &rbuf[7], 7);
	if(ret < 0)
	{
		APS_ERR("error: %d\n", ret);
		return -EFAULT;
	}
	ret = stk3x1x_master_recv(stk3x1x_obj->client, 14, &rbuf[14], 7);
	if(ret < 0)
	{
		APS_ERR("error: %d\n", ret);
		return -EFAULT;
	}
	/*
	ret = stk3x1x_master_recv(stk3x1x_obj->client, 21, &rbuf[21], 4);
	if(ret < 0)
	{
		APS_DBG("error: %d\n", ret);
		return -EFAULT;
	}
	*/
    len += scnprintf(buf+len, PAGE_SIZE-len, "[PS=%2X] [ALS=%2X] [WAIT=%4Xms] [EN_ASO=%2X] [EN_AK=%2X] [NEAR/FAR=%2X] [FLAG_OUI=%2X] [FLAG_PSINT=%2X] [FLAG_ALSINT=%2X]\n",
		rbuf[0]&0x01,(rbuf[0]&0x02)>>1,((rbuf[0]&0x04)>>2)*rbuf[5]*6,(rbuf[0]&0x20)>>5,
		(rbuf[0]&0x40)>>6,rbuf[16]&0x01,(rbuf[16]&0x04)>>2,(rbuf[16]&0x10)>>4,(rbuf[16]&0x20)>>5);

	return len;
}
/*----------------------------------------------------------------------------*/
#define IS_SPACE(CH) (((CH) == ' ') || ((CH) == '\n'))
/*----------------------------------------------------------------------------*/
#if 0
static int read_int_from_buf(struct stk3x1x_priv *obj, const char* buf, size_t count,
                             u32 data[], int len)
{
	int idx = 0;
	char *cur = (char*)buf, *end = (char*)(buf+count);

	while(idx < len)
	{
		while((cur < end) && IS_SPACE(*cur))
		{
			cur++;
		}

		if(1 != sscanf(cur, "%d", &data[idx]))
		{
			break;
		}

		idx++;
		while((cur < end) && !IS_SPACE(*cur))
		{
			cur++;
		}
	}
	return idx;
}
#endif
/*----------------------------------------------------------------------------*/
static ssize_t stk3x1x_show_alslv(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	int idx;
	if(!stk3x1x_obj)
	{
		APS_ERR("stk3x1x_obj is null!!\n");
		return 0;
	}

	for(idx = 0; idx < C_CUST_ALS_LEVEL/*stk3x1x_obj->als_level_num*/; idx++)
	{
		len += scnprintf(buf+len, PAGE_SIZE-len, "%d ", als_level[current_tp][current_color_temp][idx]);//stk3x1x_obj->hw->als_level[idx]
	}
	len += scnprintf(buf+len, PAGE_SIZE-len, "\n");
	return len;
}
/*----------------------------------------------------------------------------*/
static ssize_t stk3x1x_store_alslv(struct device_driver *ddri, const char *buf, size_t count)
{
	if(!stk3x1x_obj)
	{
		APS_ERR("stk3x1x_obj is null!!\n");
		return 0;
	}
/*
	else if(!strcmp(buf, "def"))
	{
		memcpy(stk3x1x_obj->als_level, als_level[current_color_temp], sizeof(stk3x1x_obj->als_level));//stk3x1x_obj->hw->als_level
	}
	else if(stk3x1x_obj->als_level_num != read_int_from_buf(stk3x1x_obj, buf, count,
			stk3x1x_obj->hw->als_level, stk3x1x_obj->als_level_num))
	{
		APS_ERR("invalid format: '%s'\n", buf);
	}
*/
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t stk3x1x_show_alsval(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	int idx;
	if(!stk3x1x_obj)
	{
		APS_ERR("stk3x1x_obj is null!!\n");
		return 0;
	}

	for(idx = 0; idx < C_CUST_ALS_LEVEL/*stk3x1x_obj->als_value_num*/; idx++)
	{
		len += scnprintf(buf+len, PAGE_SIZE-len, "%d ", als_value[idx]);//stk3x1x_obj->hw->als_value[idx]);
	}
	len += scnprintf(buf+len, PAGE_SIZE-len, "\n");
	return len;
}
/*----------------------------------------------------------------------------*/
static ssize_t stk3x1x_store_alsval(struct device_driver *ddri, const char *buf, size_t count)
{
	if(!stk3x1x_obj)
	{
		APS_ERR("stk3x1x_obj is null!!\n");
		return 0;
	}
/*
	else if(!strcmp(buf, "def"))
	{
		memcpy(stk3x1x_obj->als_value, als_value, sizeof(stk3x1x_obj->als_value));//stk3x1x_obj->hw->als_value
	}
	else if(stk3x1x_obj->als_value_num != read_int_from_buf(stk3x1x_obj, buf, count,
			stk3x1x_obj->hw->als_value, stk3x1x_obj->als_value_num))
	{
		APS_ERR("invalid format: '%s'\n", buf);
	}
*/
	return count;
}

#ifdef STK_TUNE0
static ssize_t stk3x1x_show_cali(struct device_driver *ddri, char *buf)
{
	int32_t word_data;
	u8 r_buf[2];
	int ret;

	if(!stk3x1x_obj)
	{
		APS_ERR("stk3x1x_obj is null!!\n");
		return 0;
	}

	ret = stk3x1x_master_recv(stk3x1x_obj->client, 0x20, r_buf, 2);
	if(ret < 0)
	{
		APS_ERR("%s fail, err=0x%x", __FUNCTION__, ret);
		return ret;
	}
	word_data = (r_buf[0] << 8) | r_buf[1];

	ret = stk3x1x_master_recv(stk3x1x_obj->client, 0x22, r_buf, 2);
	if(ret < 0)
	{
		APS_ERR("%s fail, err=0x%x", __FUNCTION__, ret);
		return ret;
	}
	word_data += (r_buf[0] << 8) | r_buf[1];

	APS_LOG("psi_set=%d, psa=%d,psi=%d, word_data=%d\n",
		stk3x1x_obj->psi_set, stk3x1x_obj->psa, stk3x1x_obj->psi, word_data);
#ifdef CALI_EVERY_TIME
	APS_LOG("boot HT=%d, LT=%d\n", stk3x1x_obj->ps_high_thd_boot, stk3x1x_obj->ps_low_thd_boot);
#endif
	return scnprintf(buf, PAGE_SIZE, "%5d\n", stk3x1x_obj->psi_set);
	//return 0;
}

static ssize_t stk3x1x_ps_maxdiff_store(struct device_driver *ddri, const char *buf, size_t count)
{
    unsigned long value = 0;
	int ret;

	if((ret = kstrtoul(buf, 10, &value)) < 0)
	{
		printk(KERN_ERR "%s:kstrtoul failed, ret=0x%x\n", __func__, ret);
		return ret;
	}
	stk3x1x_obj->stk_max_min_diff = (int) value;
	return count;
}


static ssize_t stk3x1x_ps_maxdiff_show(struct device_driver *ddri, char *buf)
{
	if(!stk3x1x_obj)
	{
		APS_ERR("stk3x1x_obj is null!!\n");
		return 0;
	}

	return scnprintf(buf, PAGE_SIZE, "%d\n", stk3x1x_obj->stk_max_min_diff);
}

static ssize_t stk3x1x_ps_ltnct_store(struct device_driver *ddri, const char *buf, size_t count)
{
    unsigned long value = 0;
	int ret;

	if((ret = kstrtoul(buf, 10, &value)) < 0)
	{
		APS_ERR("kstrtoul failed, ret=0x%x\n", ret);
		return ret;
	}
	stk3x1x_obj->stk_lt_n_ct = (int) value;
	return count;
}

static ssize_t stk3x1x_ps_ltnct_show(struct device_driver *ddri, char *buf)
{
	if(!stk3x1x_obj)
	{
		APS_ERR("stk3x1x_obj is null!!\n");
		return 0;
	}
	return scnprintf(buf, PAGE_SIZE, "%d\n", stk3x1x_obj->stk_lt_n_ct);
}

static ssize_t stk3x1x_ps_htnct_store(struct device_driver *ddri, const char *buf, size_t count)
{
    unsigned long value = 0;
	int ret;

	if((ret = kstrtoul(buf, 10, &value)) < 0)
	{
		APS_ERR("kstrtoul failed, ret=0x%x\n", ret);
		return ret;
	}
	stk3x1x_obj->stk_ht_n_ct = (int) value;
	return count;
}

static ssize_t stk3x1x_ps_htnct_show(struct device_driver *ddri, char *buf)
{
	if(!stk3x1x_obj)
	{
		APS_ERR("stk3x1x_obj is null!!\n");
		return 0;
	}
	return scnprintf(buf, PAGE_SIZE, "%d\n", stk3x1x_obj->stk_ht_n_ct);
}

#endif

#ifdef STK_ALS_FIR
/*----------------------------------------------------------------------------*/
static ssize_t stk3x1x_show_firlen(struct device_driver *ddri, char *buf)
{
	int len = atomic_read(&stk3x1x_obj->firlength);

	if(!stk3x1x_obj)
	{
		APS_ERR("stk3x1x_obj is null!!\n");
		return 0;
	}

	APS_LOG("%s: len = %2d, idx = %2d\n", __func__, len, stk3x1x_obj->fir.idx);
	APS_LOG("%s: sum = %5d, ave = %5d\n", __func__, stk3x1x_obj->fir.sum, stk3x1x_obj->fir.sum/len);

	return scnprintf(buf, PAGE_SIZE, "%d\n", len);
}

/*----------------------------------------------------------------------------*/
static ssize_t stk3x1x_store_firlen(struct device_driver *ddri, const char *buf, size_t count)
{
	int value;

	if(!stk3x1x_obj)
	{
		APS_ERR("stk3x1x_obj is null!!\n");
		return 0;
	}
	else if(1 != sscanf(buf, "%d", &value))
	{
		APS_ERR("invalid format: '%s'\n", buf);
		return 0;
	}

	if(value > MAX_FIR_LEN)
	{
		APS_ERR("%s: firlen exceed maximum filter length\n", __func__);
	}
	else if (value < 1)
	{
		atomic_set(&stk3x1x_obj->firlength, 1);
		memset(&stk3x1x_obj->fir, 0x00, sizeof(stk3x1x_obj->fir));
	}
	else
	{
		atomic_set(&stk3x1x_obj->firlength, value);
		memset(&stk3x1x_obj->fir, 0x00, sizeof(stk3x1x_obj->fir));
	}

	return count;
}
#endif /* #ifdef STK_ALS_FIR */
#ifdef ONTIM_ALS_CALI
static ssize_t show_als_cali(struct device_driver *ddri, char *buf)
{
    int len=0;

    len += snprintf(buf+len, PAGE_SIZE-len, "als_cali_enable:%d \n", als_cali_enable);

    return len;
}

static ssize_t store_als_cali(struct device_driver *ddri, const char *buf, size_t count)
{
    int loop=0;

    als_cali_enable=1;
    while((loop<100) && (als_cali_enable))
    {
        msleep(20);
        loop++;
    }
    return count;
}

static ssize_t show_channel_value(struct device_driver *ddri, char *buf)
{
    int len=0;

    len += snprintf(buf+len, PAGE_SIZE-len, "channel0:%d,channel1:%d\n", channel[0],channel[1]);

    return len;
}
#endif
/*----------------------------------------------------------------------------*/
static DRIVER_ATTR(als,     S_IWUSR | S_IRUGO, stk3x1x_show_als,   NULL);
static DRIVER_ATTR(ps,      S_IWUSR | S_IRUGO, stk3x1x_show_ps,    NULL);
static DRIVER_ATTR(ir,      S_IWUSR | S_IRUGO, stk3x1x_show_ir,    NULL);
static DRIVER_ATTR(config,  S_IWUSR | S_IRUGO, stk3x1x_show_config,stk3x1x_store_config);
static DRIVER_ATTR(alslv,   S_IWUSR | S_IRUGO, stk3x1x_show_alslv, stk3x1x_store_alslv);
static DRIVER_ATTR(alsval,  S_IWUSR | S_IRUGO, stk3x1x_show_alsval,stk3x1x_store_alsval);
static DRIVER_ATTR(trace,   S_IWUSR | S_IRUGO, stk3x1x_show_trace, stk3x1x_store_trace);
static DRIVER_ATTR(status,  S_IWUSR | S_IRUGO, stk3x1x_show_status,  NULL);
static DRIVER_ATTR(send,    S_IWUSR | S_IRUGO, stk3x1x_show_send,  stk3x1x_store_send);
static DRIVER_ATTR(recv,    S_IWUSR | S_IRUGO, stk3x1x_show_recv,  stk3x1x_store_recv);
static DRIVER_ATTR(reg,     S_IWUSR | S_IRUGO, stk3x1x_show_reg,   NULL);
static DRIVER_ATTR(allreg,  S_IWUSR | S_IRUGO, stk3x1x_show_allreg,   NULL);
#ifdef STK_TUNE0
static DRIVER_ATTR(cali,    S_IWUSR | S_IRUGO, stk3x1x_show_cali,  NULL);
static DRIVER_ATTR(maxdiff,    S_IWUSR | S_IRUGO, stk3x1x_ps_maxdiff_show,  stk3x1x_ps_maxdiff_store);
static DRIVER_ATTR(ltnct,    S_IWUSR | S_IRUGO, stk3x1x_ps_ltnct_show,  stk3x1x_ps_ltnct_store);
static DRIVER_ATTR(htnct,    S_IWUSR | S_IRUGO, stk3x1x_ps_htnct_show,  stk3x1x_ps_htnct_store);
#endif
#ifdef STK_ALS_FIR
static DRIVER_ATTR(firlen,    S_IWUSR | S_IRUGO, stk3x1x_show_firlen,  stk3x1x_store_firlen);
#endif
#ifdef ONTIM_ALS_CALI
static DRIVER_ATTR(als_cali,     S_IWUSR | S_IRUGO, show_als_cali,   store_als_cali);
static DRIVER_ATTR(channel_value,     S_IWUSR | S_IRUGO, show_channel_value,   NULL);
#endif
/*----------------------------------------------------------------------------*/
static struct driver_attribute *stk3x1x_attr_list[] = {
    &driver_attr_als,
    &driver_attr_ps,
    &driver_attr_ir,
    &driver_attr_trace,        /*trace log*/
    &driver_attr_config,
    &driver_attr_alslv,
    &driver_attr_alsval,
    &driver_attr_status,
    &driver_attr_send,
    &driver_attr_recv,
    &driver_attr_allreg,
//    &driver_attr_i2c,
    &driver_attr_reg,
#ifdef STK_TUNE0
    &driver_attr_cali,
    &driver_attr_maxdiff,
    &driver_attr_ltnct,
    &driver_attr_htnct,
#endif
#ifdef STK_ALS_FIR
    &driver_attr_firlen,
#endif
#ifdef ONTIM_ALS_CALI
    &driver_attr_als_cali,
    &driver_attr_channel_value,
#endif
};

/*----------------------------------------------------------------------------*/
static int stk3x1x_create_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(sizeof(stk3x1x_attr_list)/sizeof(stk3x1x_attr_list[0]));
	if (driver == NULL)
	{
		return -EINVAL;
	}

	for(idx = 0; idx < num; idx++)
	{
		if((err = driver_create_file(driver, stk3x1x_attr_list[idx])))
		{
			APS_ERR("driver_create_file (%s) = %d\n", stk3x1x_attr_list[idx]->attr.name, err);
			break;
		}
	}
	return err;
}
/*----------------------------------------------------------------------------*/
static int stk3x1x_delete_attr(struct device_driver *driver)
{
	int idx ,err = 0;
	int num = (int)(sizeof(stk3x1x_attr_list)/sizeof(stk3x1x_attr_list[0]));

	if (!driver)
	return -EINVAL;

	for (idx = 0; idx < num; idx++)
	{
		driver_remove_file(driver, stk3x1x_attr_list[idx]);
	}

	return err;
}


/******************************************************************************
 * Function Configuration
******************************************************************************/
static int stk3x1x_get_als_value(struct stk3x1x_priv *obj, u16 als)
{
	int idx;
        int invalid = 0;
        unsigned int lum;
	struct timeval tv = { 0 };
	struct rtc_time tm_android;
	struct timeval tv_android = { 0 };
	do_gettimeofday(&tv);
	tv_android = tv;
	tv_android.tv_sec -= sys_tz.tz_minuteswest * 60;
	rtc_time_to_tm(tv_android.tv_sec, &tm_android);

	if(current_color_temp_first == 0)
        {
                mm_segment_t orgfs = 0;
                struct file *filep = NULL;
                char buf[15] = {0};

                current_color_temp_first = 1;

                orgfs = get_fs();
                /* set_fs(KERNEL_DS); */
                set_fs(get_ds());

                filep = filp_open("/sys/ontim_dev_debug/touch_screen/vendor", O_RDONLY, 0600);
                if (IS_ERR(filep)) {
                        APS_ERR("read,sys_open %s error!!.\n", "/sys/ontim_dev_debug/touch_screen/vendor");
                        set_fs(orgfs);
                } else {
                        memset(buf, 0, sizeof(buf));
                        filep->f_op->read(filep, buf,sizeof(buf), &filep->f_pos);
			if(!strncmp(buf, "boyi", 4))
                        {
                                if(!strncmp(&buf[5], "hx83102", 7))
                                {
                                current_tp = 0;
				coeff_als_for_tp = coeff_boyi;
                                }
                        }
                        else if(!strncmp(buf, "txd", 3))
                        {
                                if(!strncmp(&buf[4], "ft8006", 6))
                                {
                                current_tp = 1;
				coeff_als_for_tp = coeff_txd;
                                }
                        }else{
				current_tp = 0;
				coeff_als_for_tp = coeff_boyi;
			}
                        filp_close(filep, NULL);
                        set_fs(orgfs);
                }
        }
	APS_DBG("current_tp = %d \n",current_tp);
	for(idx = 0; idx < C_CUST_ALS_LEVEL/*obj->als_level_num*/; idx++)
	{
		//if(als < obj->hw->als_level[idx])
		if(als < als_level[current_tp][current_color_temp][idx])
		{
			APS_DBG("als=%d ,als_level = %d\n",als,als_level[current_tp][current_color_temp][idx]);
			break;
		}
	}

	if(idx >= C_CUST_ALS_LEVEL/*obj->als_value_num*/)
	{
		APS_ERR("exceed range  idx =%d\n",idx);
		idx = C_CUST_ALS_LEVEL/*obj->als_value_num*/ - 1;
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

		//add for accuracy by zhuhui
		lum = (lum * 263) / 100;

		APS_DBG("ALS: %05d => %05d\n", als, lum);//obj->hw->als_value[idx]);

		return lum;
		//return obj->hw->als_value[idx];
	}
	else
	{
		if(atomic_read(&obj->trace) & STK_TRC_CVT_ALS)
		{
			APS_ERR("ALS: %05d => %05d (-1)\n", als, als_value[idx]);//obj->hw->als_value[idx]
		}
		return -1;
	}
}

/*----------------------------------------------------------------------------*/

#ifdef HW_dynamic_cali

static int stk3x1x_get_ps_value_only(struct stk3x1x_priv *obj, u16 ps)
{
	int ps_data = ps;
	if(((ps_data+pwave_value)<min_proximity)&&(ps_data>0))
	{
		min_proximity = ps_data+pwave_value;

		if (get_calib_flag) {
			ps_threshold_l = ps_threshold_l_tmp;
			ps_threshold_h = ps_threshold_h_tmp;
		} else {
			ps_threshold_l = FAR_THRESHOLD(threshold_value);
			ps_threshold_h = NRAR_THRESHOLD(threshold_value);
		}

		ps_thd_val_low_set = ps_threshold_l;
		ps_thd_val_hlgh_set = ps_threshold_h;
	}
	if (ps_data >= ps_threshold_h){
		ps_detection = 0;
		ps_thd_val_hlgh_set = MAX_ADC_PROX_VALUE;
		ps_thd_val_low_set = ps_threshold_l;
		APS_DBG("ps_data= %d,ps_detection = %d,th_l =%d,th_h =%d\n", ps_data,ps_detection,ps_thd_val_low_set,ps_thd_val_hlgh_set);
	}
	else if (ps_data <= ps_threshold_h){
		ps_detection = 1;
		ps_thd_val_hlgh_set = ps_threshold_h;
		ps_thd_val_low_set = 0;
		APS_DBG("ps_data= %d,ps_detection = %d,th_l =%d,th_h =%d\n", ps_data,ps_detection,ps_thd_val_low_set,ps_thd_val_hlgh_set);
	}
	else{
		APS_DBG("ps_data= %d,min_proxi = %d,th_l =%d,th_h =%d\n", ps_data,min_proximity,ps_thd_val_low_set,ps_thd_val_hlgh_set);
	}
	return ps_detection;
}
#else

static int stk3x1x_get_ps_value_only(struct stk3x1x_priv *obj, u16 ps)
{
	int mask = atomic_read(&obj->ps_mask);
	int invalid = 0, val;
	int err;
	u8 flag;

	err = stk3x1x_read_flag(obj->client, &flag);
	if(err)
		return err;
	val = (flag & STK_FLG_NF_MASK)? 1 : 0;
	APS_DBG("read_flag = 0x%x, val = %d\n", flag, val);

	if(atomic_read(&obj->ps_suspend))
	{
		invalid = 1;
	}
	else if(1 == atomic_read(&obj->ps_deb_on))
	{
		unsigned long endt = atomic_read(&obj->ps_deb_end);
		if(time_after(jiffies, endt))
		{
			atomic_set(&obj->ps_deb_on, 0);
		}

		if (1 == atomic_read(&obj->ps_deb_on))
		{
			invalid = 1;
		}
	}

	if(!invalid)
	{
		if(unlikely(atomic_read(&obj->trace) & STK_TRC_CVT_PS))
		{
			if(mask)
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
		if(unlikely(atomic_read(&obj->trace) & STK_TRC_CVT_PS))
		{
			APS_DBG("PS:  %05d => %05d (-1)\n", ps, val);
		}
		return -1;
	}
}

#endif

/*----------------------------------------------------------------------------*/
static int stk3x1x_get_ps_value(struct stk3x1x_priv *obj, u16 ps)
{
	int mask = atomic_read(&obj->ps_mask);
	int invalid = 0, val;
	int err;
	u8 flag;

	err = stk3x1x_read_flag(obj->client, &flag);
	if(err)
		return err;

	val = (flag & STK_FLG_NF_MASK)? 1 : 0;

	APS_DBG("read_flag = 0x%x, val = %d\n", flag, val);

	if((err = stk3x1x_clear_intr(obj->client, flag, STK_FLG_OUI_MASK)))
	{
		APS_ERR("fail: %d\n", err);
		return err;
	}

	if(atomic_read(&obj->ps_suspend))
	{
		invalid = 1;
	}
	else if(1 == atomic_read(&obj->ps_deb_on))
	{
		unsigned long endt = atomic_read(&obj->ps_deb_end);
		if(time_after(jiffies, endt))
		{
			atomic_set(&obj->ps_deb_on, 0);
		}

		if (1 == atomic_read(&obj->ps_deb_on))
		{
			invalid = 1;
		}
	}


	if(!invalid)
	{
		if(unlikely(atomic_read(&obj->trace) & STK_TRC_CVT_PS))
		{
			if(mask)
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
		if(unlikely(atomic_read(&obj->trace) & STK_TRC_CVT_PS))
		{
			APS_DBG("PS:  %05d => %05d (-1)\n", ps, val);
		}
		return -1;
	}
}

/*----------------------------------------------------------------------------*/

static int32_t stk3x1x_set_irs_it_slp(struct stk3x1x_priv *obj, uint16_t *slp_time, int32_t ials_it_reduce)
{
	uint8_t irs_alsctrl;
	int32_t ret;

	irs_alsctrl = (atomic_read(&obj->alsctrl_val) & 0x0F) - ials_it_reduce;
	switch(irs_alsctrl)
	{
	case 2:
		*slp_time = 1;
		break;
	case 3:
		*slp_time = 2;
		break;
	case 4:
		*slp_time = 3;
		break;
	case 5:
		*slp_time = 6;
		break;
	case 6:
		*slp_time = 12;
		break;
	case 7:
		*slp_time = 24;
		break;
	case 8:
		*slp_time = 48;
		break;
	case 9:
		*slp_time = 96;
		break;
	default:
		APS_ERR( "%s: unknown ALS IT=0x%x\n", __func__, irs_alsctrl);
		ret = -EINVAL;
		return ret;
	}
	irs_alsctrl |= (atomic_read(&obj->alsctrl_val) & 0xF0);
	ret = i2c_smbus_write_byte_data(obj->client, STK_ALSCTRL_REG, irs_alsctrl);
	if (ret < 0)
	{
		APS_ERR( "%s: write i2c error\n", __func__);
		return ret;
	}
	return 0;
}

static int32_t stk3x1x_get_ir_value(struct stk3x1x_priv *obj, int32_t als_it_reduce)
{
    int32_t word_data, ret;
	uint8_t w_reg, retry = 0;
	uint16_t irs_slp_time = 100;
	//bool re_enable_ps = false;
	u8 flag;
	u8 buf[2];

/*	re_enable_ps = (atomic_read(&obj->state_val) & STK_STATE_EN_PS_MASK) ? true : false;
	if(re_enable_ps)
	{
		stk3x1x_enable_ps(obj->client, 0, 1);
	}
*/
	ret = stk3x1x_set_irs_it_slp(obj, &irs_slp_time, als_it_reduce);
	if(ret < 0)
		goto irs_err_i2c_rw;

	w_reg = atomic_read(&obj->state_val) | STK_STATE_EN_IRS_MASK;
    ret = i2c_smbus_write_byte_data(obj->client, STK_STATE_REG, w_reg);
    if (ret < 0)
	{
		APS_ERR( "%s: write i2c error\n", __func__);
		goto irs_err_i2c_rw;
	}
	msleep(irs_slp_time);

	do
	{
		msleep(3);
		ret = stk3x1x_read_flag(obj->client, &flag);
		if (ret < 0)
		{
			APS_ERR("WARNING: read flag reg error: %d\n", ret);
			goto irs_err_i2c_rw;
		}
		retry++;
	}while(retry < 10 && ((flag&STK_FLG_IR_RDY_MASK) == 0));

	if(retry == 10)
	{
		APS_ERR( "%s: ir data is not ready for a long time\n", __func__);
		ret = -EINVAL;
		goto irs_err_i2c_rw;
	}

	ret = stk3x1x_clear_intr(obj->client, flag, STK_FLG_IR_RDY_MASK);
    if (ret < 0)
	{
		APS_ERR( "%s: write i2c error\n", __func__);
		goto irs_err_i2c_rw;
	}

	ret = stk3x1x_master_recv(obj->client, STK_DATA1_IR_REG, buf, 2);
	if(ret < 0)
	{
		APS_ERR( "%s fail, ret=0x%x", __func__, ret);
		goto irs_err_i2c_rw;
	}
	word_data =  (buf[0] << 8) | buf[1];
#ifdef ONTIM_ALS_CALI
	channel[1] = word_data;
	word_data = word_data *  als_ch1_expect / als_cali.als_ch1;
#endif
	ret = i2c_smbus_write_byte_data(obj->client, STK_ALSCTRL_REG, atomic_read(&obj->alsctrl_val));
	if (ret < 0)
	{
		APS_ERR( "%s: write i2c error\n", __func__);
		goto irs_err_i2c_rw;
	}
	//if(re_enable_ps)
		//stk3x1x_enable_ps(obj->client, 1, 0);
	stk3x1x_master_recv(obj->client, STK_STATE_REG, &w_reg, 0x01);
	i2c_smbus_write_byte_data(obj->client, STK_STATE_REG, w_reg);
	stk3x1x_master_recv(obj->client, STK_DATA1_ALS_REG, buf, 2);

	return word_data;

irs_err_i2c_rw:
	//if(re_enable_ps)
	//	stk3x1x_enable_ps(obj->client, 1, 0);
	i2c_smbus_write_byte_data(obj->client, STK_ALSCTRL_REG, atomic_read(&obj->alsctrl_val));
	stk3x1x_master_recv(obj->client, STK_STATE_REG, &w_reg, 0x01);
	i2c_smbus_write_byte_data(obj->client, STK_STATE_REG, w_reg);
	stk3x1x_master_recv(obj->client, STK_DATA1_ALS_REG, buf, 2);
	return 0;
}

/******************************************************************************
 * Function Configuration
******************************************************************************/
static int stk3x1x_open(struct inode *inode, struct file *file)
{
	file->private_data = stk3x1x_i2c_client;

	if (!file->private_data)
	{
		APS_ERR("null pointer!!\n");
		return -EINVAL;
	}

	return nonseekable_open(inode, file);
}
/*----------------------------------------------------------------------------*/
static int stk3x1x_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}
/*----------------------------------------------------------------------------*/
#ifdef CONFIG_COMPAT
static long stk3x1x_compat_ioctl(struct file *file, unsigned int cmd,
       unsigned long arg)
{
        long err = 0;
        void __user *arg32 = compat_ptr(arg);
        if (!file->f_op || !file->f_op->unlocked_ioctl || !arg32)
                return -ENOTTY;

        APS_DBG("stk3x1x_compat_ioctl cmd= %x\n", cmd);
        err = file->f_op->unlocked_ioctl(file, cmd, (unsigned long)arg32);

        return err;
}
#endif
static long stk3x1x_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct i2c_client *client = (struct i2c_client*)file->private_data;
	struct stk3x1x_priv *obj = i2c_get_clientdata(client);
#if (LINUX_VERSION_CODE>=KERNEL_VERSION(2,6,36))
	long err = 0;
#else
	int err = 0;
#endif
	void __user *ptr = (void __user*) arg;
	int dat;
	uint32_t enable;
	int ps_result, ps_cali_result;
	int threshold[2];

	APS_DBG("cmd= %x\n", cmd);

	switch (cmd)
	{
		case ALSPS_SET_PS_MODE:
			if(copy_from_user(&enable, ptr, sizeof(enable)))
			{
				err = -EFAULT;
				goto err_out;
			}
			if(enable)
			{
				if((err = stk3x1x_enable_ps(obj->client, 1, 1)))
				{
#if (LINUX_VERSION_CODE>=KERNEL_VERSION(2,6,36))
					APS_ERR("enable ps fail: %ld\n", err);
#else
					APS_ERR("enable ps fail: %d\n", err);
#endif
					goto err_out;
				}

				set_bit(STK_BIT_PS, &obj->enable);
			}
			else
			{
				if((err = stk3x1x_enable_ps(obj->client, 0, 1)))
				{
#if (LINUX_VERSION_CODE>=KERNEL_VERSION(2,6,36))
					APS_ERR("disable ps fail: %ld\n", err);
#else
					APS_ERR("disable ps fail: %d\n", err);
#endif

					goto err_out;
				}

				clear_bit(STK_BIT_PS, &obj->enable);
			}
			break;

		case ALSPS_GET_PS_MODE:
			enable = test_bit(STK_BIT_PS, &obj->enable) ? (1) : (0);
			if(copy_to_user(ptr, &enable, sizeof(enable)))
			{
				err = -EFAULT;
				goto err_out;
			}
			break;

		case ALSPS_GET_PS_DATA:
			if((err = stk3x1x_read_ps(obj->client, &obj->ps)))
			{
				goto err_out;
			}

			dat = stk3x1x_get_ps_value(obj, obj->ps);
			if(dat < 0)
			{
				err = dat;
				goto err_out;
			}
#ifdef STK_PS_POLLING_LOG
			APS_LOG("%s:ps raw 0x%x -> value 0x%x \n",__FUNCTION__, obj->ps, dat);
#endif
			if(copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}
			break;

		case ALSPS_GET_PS_RAW_DATA:
			if((err = stk3x1x_read_ps(obj->client, &obj->ps)))
			{
				goto err_out;
			}

			dat = obj->ps;
			if(copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}
			break;

		case ALSPS_SET_ALS_MODE:
			if(copy_from_user(&enable, ptr, sizeof(enable)))
			{
				err = -EFAULT;
				goto err_out;
			}
			if(enable)
			{
				if((err = stk3x1x_enable_als(obj->client, 1)))
				{
#if (LINUX_VERSION_CODE>=KERNEL_VERSION(2,6,36))
					APS_ERR("enable als fail: %ld\n", err);
#else
					APS_ERR("enable als fail: %d\n", err);
#endif

					goto err_out;
				}
				set_bit(STK_BIT_ALS, &obj->enable);
			}
			else
			{
				if((err = stk3x1x_enable_als(obj->client, 0)))
				{
#if (LINUX_VERSION_CODE>=KERNEL_VERSION(2,6,36))
					APS_ERR("disable als fail: %ld\n", err);
#else
					APS_ERR("disable als fail: %d\n", err);
#endif

					goto err_out;
				}
				clear_bit(STK_BIT_ALS, &obj->enable);
			}
			break;

		case ALSPS_GET_ALS_MODE:
			enable = test_bit(STK_BIT_ALS, &obj->enable) ? (1) : (0);
			if(copy_to_user(ptr, &enable, sizeof(enable)))
			{
				err = -EFAULT;
				goto err_out;
			}
			break;

		case ALSPS_GET_ALS_DATA:
			if((err = stk3x1x_read_als(obj->client, &obj->als)))
			{
				goto err_out;
			}
			dat = stk3x1x_get_als_value(obj, obj->als);
			if(copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}
			break;

		case ALSPS_GET_ALS_RAW_DATA:
			if((err = stk3x1x_read_als(obj->client, &obj->als)))
			{
				goto err_out;
			}

			dat = obj->als;
			if(copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}
			break;

		case ALSPS_GET_PS_THRESHOLD_HIGH:
#ifdef ONTIM_CALI
	if (ps_cali.noice > -1)
			{
				dat = ps_cali.close - obj->ps_cali;
			}
			else
			{
#endif
			dat = atomic_read(&obj->ps_high_thd_val) - obj->ps_cali;
#ifdef ONTIM_CALI
			}
#endif
			APS_LOG("%s:ps_high_thd_val:%d\n",__func__,dat);
			if(copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}
			break;
		case ALSPS_GET_PS_THRESHOLD_LOW:
#ifdef ONTIM_CALI
	if (ps_cali.noice > -1)
			{
				dat = ps_cali.far_away;
			}
			else
			{
#endif
			dat = atomic_read(&obj->ps_low_thd_val) - obj->ps_cali;
#ifdef ONTIM_CALI
			}
#endif
			APS_LOG("%s:ps_low_thd_val:%d\n",__func__,dat);
			if(copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}
			break;

		case ALSPS_GET_PS_TEST_RESULT:
			if((err = stk3x1x_read_ps(obj->client, &obj->ps)))
			{
				goto err_out;
			}

			if(obj->ps > atomic_read(&obj->ps_high_thd_val))
				ps_result = 0;
			else
				ps_result = 1;
			APS_LOG("ALSPS_GET_PS_TEST_RESULT : ps_result = %d\n",ps_result);
			if(copy_to_user(ptr, &ps_result, sizeof(ps_result)))
			{
				err = -EFAULT;
				goto err_out;
			}
			break;

		case ALSPS_IOCTL_CLR_CALI:
			if(copy_from_user(&dat, ptr, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}
			if(dat == 0)
			obj->ps_cali = 0;
			//atomic_set(&obj->ps_high_thd_val,  obj->hw->ps_threshold_high);
			//atomic_set(&obj->ps_low_thd_val,  obj->hw->ps_threshold_low);  //obj->hw->ps_low_thd_val
			APS_LOG("ALSPS_IOCTL_CLR_CALI : obj->ps_cali:%d high:%d low:%d\n", obj->ps_cali,
													atomic_read(&obj->ps_high_thd_val),
													atomic_read(&obj->ps_low_thd_val));
			break;

		case ALSPS_IOCTL_GET_CALI:
			//stk3x1x_ps_calibration(obj->client);
			ps_cali_result = obj->ps_cali;
			APS_LOG("ALSPS_IOCTL_GET_CALI : ps_cali = %d\n",obj->ps_cali);
			if(copy_to_user(ptr, &ps_cali_result, sizeof(ps_cali_result)))
			{
				err = -EFAULT;
				goto err_out;
			}
			break;

		case ALSPS_IOCTL_SET_CALI:
			//1. libhwm.so calc value store in ps_cali;
			//2. nvram_daemon update ps_cali in driver
			if(copy_from_user(&ps_cali_result, ptr, sizeof(ps_cali_result)))
			{
				err = -EFAULT;
				goto err_out;
			}
			obj->ps_cali = ps_cali_result;
			/*
			atomic_set(&obj->ps_high_thd_val, obj->ps_cali + obj->stk_ht_n_ct);
			atomic_set(&obj->ps_low_thd_val,  obj->ps_cali + obj->stk_lt_n_ct);
			if((err = stk3x1x_write_ps_high_thd(obj->client, atomic_read(&obj->ps_high_thd_val))))
				goto err_out;
			if((err = stk3x1x_write_ps_low_thd(obj->client, atomic_read(&obj->ps_low_thd_val))))
				goto err_out;
			*/
			APS_DBG("ALSPS_IOCTL_SET_CALI : ps_cali_result = %d\n",ps_cali_result);
			//APS_LOG("ALSPS_IOCTL_SET_CALI : obj->ps_cali:%d high:%d low:%d\n", obj->ps_cali,
			//										atomic_read(&obj->ps_high_thd_val),
			//										atomic_read(&obj->ps_low_thd_val));
			break;

		case ALSPS_SET_PS_THRESHOLD:
                        if(copy_from_user(threshold, ptr, sizeof(threshold)))
                        {
                                err = -EFAULT;
                                goto err_out;
                        }
                        APS_LOG("%s CALI set threshold high: %d, low: %d;%d;\n", __func__, threshold[0],threshold[1],obj->ps_cali);
                        atomic_set(&obj->ps_high_thd_val,  (threshold[0]+obj->ps_cali));
                        atomic_set(&obj->ps_low_thd_val,  (threshold[1]+obj->ps_cali));//need to confirm
#ifdef ONTIM_CALI
			//ps_cali.valid =1;
			ps_cali.close = (threshold[0]+obj->ps_cali);
			ps_cali.far_away = (threshold[1]+obj->ps_cali);
#endif
			if((err = stk3x1x_write_ps_high_thd(obj->client, atomic_read(&obj->ps_high_thd_val))))
                                goto err_out;
                        if((err = stk3x1x_write_ps_low_thd(obj->client, atomic_read(&obj->ps_low_thd_val))))
                                goto err_out;

                        break;
		default:
			APS_ERR("%s not supported = 0x%04x", __FUNCTION__, cmd);
			err = -ENOIOCTLCMD;
			break;
	}

	err_out:
	return err;
}
/*----------------------------------------------------------------------------*/
static struct file_operations stk3x1x_fops = {
#if (LINUX_VERSION_CODE<KERNEL_VERSION(3,0,0))
	.owner = THIS_MODULE,
#endif
	.open = stk3x1x_open,
	.release = stk3x1x_release,
	.unlocked_ioctl = stk3x1x_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = stk3x1x_compat_ioctl,
#endif
};
/*----------------------------------------------------------------------------*/
static struct miscdevice stk3x1x_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "als_ps",
	.fops = &stk3x1x_fops,
};
/*----------------------------------------------------------------------------*/
static int stk3x1x_i2c_suspend(struct device *dev)
{
//+ add by fanxzh
	struct i2c_client *client = to_i2c_client(dev);
	struct stk3x1x_priv *obj = i2c_get_clientdata(client);
	int err;

	APS_FUN();

	if (!obj) {
		APS_ERR("null pointer!!\n");
		return -EINVAL;
	}

	err = enable_irq_wake(obj->irq);
	if (err) {
		APS_ERR("enable_irq_wake(irq:%d) failed", obj->irq);
	}
//- add by fanxzh

/*
	if(msg.event == PM_EVENT_SUSPEND)
	{
		if(!obj)
		{
			APS_ERR("null pointer!!\n");
			return -EINVAL;
		}

		atomic_set(&obj->als_suspend, 1);
		if((err = stk3x1x_enable_als(client, 0)))
		{
			APS_ERR("disable als: %d\n", err);
			return err;
		}

		atomic_set(&obj->ps_suspend, 1);
		if((err = stk3x1x_enable_ps(client, 0, 1)))
		{
			APS_ERR("disable ps:  %d\n", err);
			return err;
		}

		stk3x1x_power(obj->hw, 0);
	}

*/
	return 0;
}
/*----------------------------------------------------------------------------*/
static int stk3x1x_i2c_resume(struct device *dev)
{
//+ add by fanxzh
	struct i2c_client *client = to_i2c_client(dev);
	struct stk3x1x_priv *obj = i2c_get_clientdata(client);
	int err;

	APS_FUN();

	err = disable_irq_wake(obj->irq);
	if (err) {
		APS_ERR("disable_irq_wake(irq:%d) failed", obj->irq);
	}
//- add by fanxzh

/*
	if(!obj)
	{
		APS_ERR("null pointer!!\n");
		return -EINVAL;
	}

	stk3x1x_power(obj->hw, 1);
	if((err = stk3x1x_init_client(client)))
	{
		APS_ERR("initialize client fail!!\n");
		return err;
	}
	atomic_set(&obj->als_suspend, 0);
	if(test_bit(STK_BIT_ALS, &obj->enable))
	{
		if((err = stk3x1x_enable_als(client, 1)))
		{
			APS_ERR("enable als fail: %d\n", err);
		}
	}
	atomic_set(&obj->ps_suspend, 0);
	if(test_bit(STK_BIT_PS,  &obj->enable))
	{
		if((err = stk3x1x_enable_ps(client, 1, 1)))
		{
			APS_ERR("enable ps fail: %d\n", err);
		}
	}
*/
	return 0;
}
/*----------------------------------------------------------------------------*/
#if defined(CONFIG_HAS_EARLYSUSPEND)
static void stk3x1x_early_suspend(struct early_suspend *h)
{   /*early_suspend is only applied for ALS*/
	int err;
	struct stk3x1x_priv *obj = container_of(h, struct stk3x1x_priv, early_drv);
	int old = atomic_read(&obj->state_val);
	APS_FUN();

	if(!obj)
	{
		APS_ERR("null pointer!!\n");
		return;
	}

	if(old & STK_STATE_EN_ALS_MASK)
	{
		atomic_set(&obj->als_suspend, 1);
		if((err = stk3x1x_enable_als(obj->client, 0)))
		{
			APS_ERR("disable als fail: %d\n", err);
		}
	}
}
/*----------------------------------------------------------------------------*/
static void stk3x1x_late_resume(struct early_suspend *h)
{   /*early_suspend is only applied for ALS*/
	int err;
	struct stk3x1x_priv *obj = container_of(h, struct stk3x1x_priv, early_drv);
	APS_FUN();

	if(!obj)
	{
		APS_ERR("null pointer!!\n");
		return;
	}

	if(atomic_read(&obj->als_suspend))
	{
		atomic_set(&obj->als_suspend, 0);
		if(test_bit(STK_BIT_ALS, &obj->enable))
		{
			if((err = stk3x1x_enable_als(obj->client, 1)))
			{
				APS_ERR("enable als fail: %d\n", err);
			}
		}
	}
}
#endif
int stk3x1x_ps_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	struct hwm_sensor_data* sensor_data;
	struct stk3x1x_priv *obj = (struct stk3x1x_priv *)self;

	switch (command)
	{
		case SENSOR_DELAY:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				APS_ERR("Set delay parameter error!\n");
				err = -EINVAL;
			}
			// Do nothing
			break;

		case SENSOR_ENABLE:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				APS_ERR("Enable sensor parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				value = *(int *)buff_in;
				if(value)
				{
					if((err = stk3x1x_enable_ps(obj->client, 1, 1)))
					{
						APS_ERR("enable ps fail: %d\n", err);
						return -1;
					}
					set_bit(STK_BIT_PS, &obj->enable);
				}
				else
				{
					if((err = stk3x1x_enable_ps(obj->client, 0, 1)))
					{
						APS_ERR("disable ps fail: %d\n", err);
						return -1;
					}
					clear_bit(STK_BIT_PS, &obj->enable);
				}
			}
			break;

		case SENSOR_GET_DATA:
			if((buff_out == NULL) || (size_out< sizeof(struct hwm_sensor_data)))
			{
				APS_ERR("get sensor data parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				sensor_data = (struct hwm_sensor_data *)buff_out;

				if((err = stk3x1x_read_ps(obj->client, &obj->ps)))
				{
					err = -1;
				}
				else
				{
					value = stk3x1x_get_ps_value(obj, obj->ps);
					if(value < 0)
					{
						err = -1;
					}
					else
					{
						sensor_data->values[0] = value;
						sensor_data->value_divide = 1;
						sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
#ifdef STK_PS_POLLING_LOG
						APS_LOG("%s:ps raw 0x%x -> value 0x%x \n",__FUNCTION__, obj->ps, sensor_data->values[0]);
#endif
					}
				}
			}
			break;
		default:
			APS_ERR("proximity sensor operate function no this parameter %d!\n", command);
			err = -1;
			break;
	}

	return err;
}

int stk3x1x_als_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	struct hwm_sensor_data* sensor_data;
	struct stk3x1x_priv *obj = (struct stk3x1x_priv *)self;
	u8 flag;

	switch (command)
	{
		case SENSOR_DELAY:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				APS_ERR("Set delay parameter error!\n");
				err = -EINVAL;
			}
			// Do nothing
			break;

		case SENSOR_ENABLE:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				APS_ERR("Enable sensor parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				value = *(int *)buff_in;
				if(value)
				{
					if((err = stk3x1x_enable_als(obj->client, 1)))
					{
						APS_ERR("enable als fail: %d\n", err);
						return -1;
					}
					set_bit(STK_BIT_ALS, &obj->enable);
				}
				else
				{
					if((err = stk3x1x_enable_als(obj->client, 0)))
					{
						APS_ERR("disable als fail: %d\n", err);
						return -1;
					}
					clear_bit(STK_BIT_ALS, &obj->enable);
				}

			}
			break;

		case SENSOR_GET_DATA:
			if((buff_out == NULL) || (size_out< sizeof(struct hwm_sensor_data)))
			{
				APS_ERR("get sensor data parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				sensor_data = (struct hwm_sensor_data *)buff_out;
				err = stk3x1x_read_flag(obj->client, &flag);
				if(err)
					return err;

				if(!(flag & STK_FLG_ALSDR_MASK))
					return -1;

				if((err = stk3x1x_read_als(obj->client, &obj->als)))
				{
					err = -1;
				}
				else
				{
					/*
					if(obj->als < 3)
					{
						obj->als_last = obj->als;
						sensor_data->values[0] = stk3x1x_get_als_value(obj, 0);
					}
					else if(abs(obj->als - obj->als_last) >= STK_ALS_CODE_CHANGE_THD)
					{
						obj->als_last = obj->als;
						sensor_data->values[0] = stk3x1x_get_als_value(obj, obj->als);
					}
					else
					{
						sensor_data->values[0] = stk3x1x_get_als_value(obj, obj->als_last);
					}
					*/
					sensor_data->values[0] = stk3x1x_get_als_value(obj, obj->als);
					sensor_data->value_divide = 1;
					sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
				}
			}
			break;
		default:
			APS_ERR("light sensor operate function no this parameter %d!\n", command);
			err = -1;
			break;
	}

	return err;
}


/*----------------------------------------------------------------------------*/
static int stk3x1x_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	strcpy(info->type, stk3x1x_DEV_NAME);
	return 0;
}
/*----------------------------------------------------------------------------*/

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

    APS_DBG("stk3x1x_obj als enable value = %d\n", en);

#ifdef CUSTOM_KERNEL_SENSORHUB
    req.activate_req.sensorType = ID_LIGHT;
    req.activate_req.action = SENSOR_HUB_ACTIVATE;
    req.activate_req.enable = en;
    len = sizeof(req.activate_req);
    res = SCP_sensorHub_req_send(&req, &len, 1);
#else //#ifdef CUSTOM_KERNEL_SENSORHUB
	if(!stk3x1x_obj)
	{
		APS_ERR("stk3x1x_obj is null!!\n");
		return -1;
	}
	res=	stk3x1x_enable_als(stk3x1x_obj->client, en);
#endif //#ifdef CUSTOM_KERNEL_SENSORHUB
	if(res){
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
    struct stk3x1x_priv *obj = NULL;
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

    if(atomic_read(&stk3x1x_obj->trace) & CMC_TRC_PS_DATA)
	{
        APS_LOG("value = %d\n", *value);
        //show data
	}
#else //#ifdef CUSTOM_KERNEL_SENSORHUB
	if(!stk3x1x_obj)
	{
		APS_ERR("stk3x1x_obj is null!!\n");
		return -1;
	}
	obj = stk3x1x_obj;
	if((err = stk3x1x_read_als(obj->client, &obj->als)))
	{
		err = -1;
	}
	else
	{
		/*
		if(obj->als < 3)
		{
			obj->als_last = obj->als;
			*value = stk3x1x_get_als_value(obj, 0);
		}
		else if(abs(obj->als - obj->als_last) >= STK_ALS_CODE_CHANGE_THD)
		{
			obj->als_last = obj->als;
			*value = stk3x1x_get_als_value(obj, obj->als);
		}
		else
		{
			*value = stk3x1x_get_als_value(obj, obj->als_last);
		}
		*/
		*value = stk3x1x_get_als_value(obj, obj->als);
		*status = SENSOR_STATUS_ACCURACY_MEDIUM;
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
	//struct stk3x1x_priv *obj = i2c_get_clientdata(client);
#ifdef CUSTOM_KERNEL_SENSORHUB
    SCP_SENSOR_HUB_DATA req;
    int len;
#endif //#ifdef CUSTOM_KERNEL_SENSORHUB

    APS_DBG("stk3x1x_obj ps enable value = %d\n", en);

#ifdef CUSTOM_KERNEL_SENSORHUB
    req.activate_req.sensorType = ID_PROXIMITY;
    req.activate_req.action = SENSOR_HUB_ACTIVATE;
    req.activate_req.enable = en;
    len = sizeof(req.activate_req);
    res = SCP_sensorHub_req_send(&req, &len, 1);
#else //#ifdef CUSTOM_KERNEL_SENSORHUB
	if(!stk3x1x_obj)
	{
		APS_ERR("stk3x1x_obj is null!!\n");
		return -1;
	}
	res=	stk3x1x_enable_ps(stk3x1x_obj->client, en,1);
#endif //#ifdef CUSTOM_KERNEL_SENSORHUB

	if(res<0){
		APS_ERR("als_enable_nodata is failed!!\n");
		return -1;
	}
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

	APS_DBG(" enter.\n");

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

    if(atomic_read(&stk3x1x_obj->trace) & CMC_TRC_PS_DATA)
	{
        APS_LOG("value = %d\n", *value);
        //show data
	}
#else //#ifdef CUSTOM_KERNEL_SENSORHUB
    if(!stk3x1x_obj)
	{
		APS_ERR("stk3x1x_obj is null!!\n");
		return -1;
	}

    if((err = stk3x1x_read_ps(stk3x1x_obj->client, &stk3x1x_obj->ps)))
    {
        err = -1;;
    }
    else
    {
        *value = stk3x1x_get_ps_value(stk3x1x_obj, stk3x1x_obj->ps);
        *status = SENSOR_STATUS_ACCURACY_MEDIUM;
    }
#endif //#ifdef CUSTOM_KERNEL_SENSORHUB

	return 0;
}


/*----------------------------------------------------------------------------*/
static int stk3x1x_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct stk3x1x_priv *obj;
	//struct hwmsen_object obj_ps, obj_als;
	int err = 0;
	struct als_control_path als_ctl={0};
	struct als_data_path als_data={0};
	struct ps_control_path ps_ctl={0};
	struct ps_data_path ps_data={0};

	APS_FUN();

	if(CHECK_THIS_DEV_DEBUG_AREADY_EXIT()==0)
    {
        return -EIO;
    }
	err = get_alsps_dts_func(client->dev.of_node, hw);
	if (err < 0) {
		APS_ERR("get customization info from dts failed\n");
		goto exit_init_failed;
	}

	APS_DBG("i2c_num=%d,I2C_ADDRS=%d;\n",hw->i2c_num,hw->i2c_addr[0]);

	if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
	{
		err = -ENOMEM;
		goto exit;
	}
	memset(obj, 0, sizeof(*obj));

	stk3x1x_obj = obj;
        obj->hw = hw;

	stk3x1x_get_addr(obj->hw, &obj->addr);

	INIT_DELAYED_WORK(&obj->eint_work, stk3x1x_eint_work);
	obj->client = client;
	i2c_set_clientdata(client, obj);
	atomic_set(&obj->als_debounce, 200);
	atomic_set(&obj->als_deb_on, 0);
	atomic_set(&obj->als_deb_end, 0);
	atomic_set(&obj->ps_debounce, 10);
	atomic_set(&obj->ps_deb_on, 0);
	atomic_set(&obj->ps_deb_end, 0);
	atomic_set(&obj->ps_mask, 0);
	atomic_set(&obj->trace, 0x00);
	atomic_set(&obj->als_suspend, 0);

	atomic_set(&obj->state_val, obj->hw->state_val);
	atomic_set(&obj->psctrl_val, obj->hw->psctrl_val);
	atomic_set(&obj->alsctrl_val, obj->hw->alsctrl_val);
	obj->ledctrl_val = obj->hw->ledctrl_val;
	obj->wait_val = obj->hw->wait_val;
	obj->int_val = 0;
	obj->first_boot = true;
	obj->als_correct_factor = 1000;
#ifdef STK_TUNE0
	if((obj->hw->ps_threshold_high != 0) && (obj->hw->ps_threshold_low != 0))
	{
		atomic_set(&obj->ps_high_thd_val, obj->hw->ps_threshold_high + 500);//  obj->hw->ps_high_thd_val
		atomic_set(&obj->ps_low_thd_val, obj->hw->ps_threshold_low + 500);
	}
	else
	{
		atomic_set(&obj->ps_high_thd_val, 2200);//  obj->hw->ps_high_thd_val
		atomic_set(&obj->ps_low_thd_val, 2100);
	}
#else
	atomic_set(&obj->ps_high_thd_val, obj->hw->ps_threshold_high);//  obj->hw->ps_high_thd_val
	atomic_set(&obj->ps_low_thd_val, obj->hw->ps_threshold_low);
#endif
	atomic_set(&obj->recv_reg, 0);
#ifdef STK_ALS_FIR
	atomic_set(&obj->firlength, STK_FIR_LEN);
#endif

#ifdef STK_TUNE0
	obj->stk_max_min_diff = STK_MAX_MIN_DIFF;
	obj->stk_lt_n_ct = STK_LT_N_CT;
	obj->stk_ht_n_ct = STK_HT_N_CT;
#endif
	if(obj->hw->polling_mode_ps == 0)
	{
		APS_DBG("%s: enable PS interrupt\n", __FUNCTION__);
	}
	obj->int_val |= STK_INT_PS_MODE7;

	if(obj->hw->polling_mode_als == 0)
	{
	  obj->int_val |= STK_INT_ALS;
	  APS_DBG("%s: enable ALS interrupt\n", __FUNCTION__);
	}

	APS_DBG("%s: state_val=0x%x, psctrl_val=0x%x, alsctrl_val=0x%x, ledctrl_val=0x%x, wait_val=0x%x, int_val=0x%x\n",
		__FUNCTION__, atomic_read(&obj->state_val), atomic_read(&obj->psctrl_val), atomic_read(&obj->alsctrl_val),
		obj->ledctrl_val, obj->wait_val, obj->int_val);

	//stk3x1x_obj->irq_node = client->dev.of_node;
	obj->enable = 0;
	obj->pending_intr = 0;
	//obj->als_level_num = sizeof(obj->hw->als_level)/sizeof(obj->hw->als_level[0]);
	//obj->als_value_num = sizeof(obj->hw->als_value)/sizeof(obj->hw->als_value[0]);
	BUG_ON(sizeof(als_level) != sizeof(obj->hw->als_level));
	memcpy(als_level, obj->hw->als_level, sizeof(als_level));
	BUG_ON(sizeof(als_value) != sizeof(obj->hw->als_value));
	memcpy(als_value, obj->hw->als_value, sizeof(als_value));
	atomic_set(&obj->i2c_retry, 3);
	if(atomic_read(&obj->state_val) & STK_STATE_EN_ALS_MASK)
	{
		set_bit(STK_BIT_ALS, &obj->enable);
	}

	if(atomic_read(&obj->state_val) & STK_STATE_EN_PS_MASK)
	{
		set_bit(STK_BIT_PS, &obj->enable);
	}

	stk3x1x_i2c_client = client;
#ifdef STK_TUNE0
	obj->stk_ps_tune0_wq = create_singlethread_workqueue("stk_ps_tune0_wq");
	INIT_WORK(&obj->stk_ps_tune0_work, stk_ps_tune0_work_func);
	hrtimer_init(&obj->ps_tune0_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	obj->ps_tune0_delay = ns_to_ktime(60 * NSEC_PER_MSEC);
	obj->ps_tune0_timer.function = stk_ps_tune0_timer_func;
#endif

	if((err = stk3x1x_init_client(client)))
	{
		goto exit_init_failed;
	}
	APS_DBG("obj->hw->polling_mode_ps=0x%x,obj->hw->polling_mode_als=0x%x;\n",obj->hw->polling_mode_ps,obj->hw->polling_mode_als);
	if(obj->hw->polling_mode_ps == 0 || obj->hw->polling_mode_als == 0)
        {
            if((err = stk3x1x_setup_eint(client)))
            {
             	APS_ERR("setup eint error: %d\n", err);
                goto exit_init_failed;
            }
        }


	if((err = misc_register(&stk3x1x_device)))
	{
		APS_ERR("stk3x1x_device register failed\n");
		goto exit_misc_device_register_failed;
	}

	if((err = stk3x1x_create_attr(&(stk3x1x_init_info.platform_diver_addr->driver))))
	//if((err = stk3x1x_create_attr(&stk3x1x_alsps_driver.driver)))
	{
		APS_ERR("create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}

	/*
	obj_ps.self = stk3x1x_obj;
	if(1 == obj->hw->polling_mode_ps)
	{
		obj_ps.polling = 1;
		wake_lock_init(&ps_lock,WAKE_LOCK_SUSPEND,"ps wakelock");
	}
	else
	{
	  obj_ps.polling = 0;//PS interrupt mode
	}

	obj_ps.sensor_operate = stk3x1x_ps_operate;
	if((err = hwmsen_attach(ID_PROXIMITY, &obj_ps)))
	{
		APS_ERR("attach fail = %d\n", err);
		goto exit_create_attr_failed;
	}

	obj_als.self = stk3x1x_obj;
	if(1 == obj->hw->polling_mode_als)
	{
	  obj_als.polling = 1;
	}
	else
	{
	  obj_als.polling = 0;//ALS interrupt mode
	}
	obj_als.sensor_operate = stk3x1x_als_operate;
	if((err = hwmsen_attach(ID_LIGHT, &obj_als)))
	{
		APS_ERR("attach fail = %d\n", err);
		goto exit_create_attr_failed;
	}*/
	als_ctl.open_report_data= als_open_report_data;
	als_ctl.enable_nodata = als_enable_nodata;
	als_ctl.set_delay  = als_set_delay;
	als_ctl.batch = als_batch;
	als_ctl.flush = als_flush;
	als_ctl.is_report_input_direct = false;
#ifdef CUSTOM_KERNEL_SENSORHUB
	als_ctl.is_support_batch = obj->hw->is_batch_supported_als;
#else
    als_ctl.is_support_batch = false;
#endif

	err = als_register_control_path(&als_ctl);
	if(err)
	{
		APS_ERR("register fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}

	als_data.get_data = als_get_data;
	als_data.vender_div = 100;
	err = als_register_data_path(&als_data);
	if(err)
	{
		APS_ERR("tregister fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}


	ps_ctl.open_report_data= ps_open_report_data;
	ps_ctl.enable_nodata = ps_enable_nodata;
	ps_ctl.set_delay  = ps_set_delay;
	ps_ctl.batch = ps_batch;
	ps_ctl.flush = ps_flush;
	ps_ctl.is_report_input_direct = false;
#ifdef CUSTOM_KERNEL_SENSORHUB
	ps_ctl.is_support_batch = obj->hw->is_batch_supported_ps;
#else
    ps_ctl.is_support_batch = false;
#endif

	err = ps_register_control_path(&ps_ctl);
	if(err)
	{
		APS_ERR("register fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}

	ps_data.get_data = ps_get_data;
	ps_data.vender_div = 100;
	err = ps_register_data_path(&ps_data);
	if(err)
	{
		APS_ERR("tregister fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}



#if defined(CONFIG_HAS_EARLYSUSPEND)
	obj->early_drv.level    = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1,
	obj->early_drv.suspend  = stk3x1x_early_suspend,
	obj->early_drv.resume   = stk3x1x_late_resume,
	register_early_suspend(&obj->early_drv);
#endif

#if defined(MTK_AUTO_DETECT_ALSPS)
    stk3x1x_init_flag = 0;
#endif

#ifdef ontim_debug_info
    REGISTER_AND_INIT_ONTIM_DEBUG_FOR_THIS_DEV();
#endif

#ifdef ONTIM_ALS_CALI
    if(obj->hw->als_calib_enable)
    {
		s_CaliLoadEnable  = true;
    }
	als_ch0_expect = obj->hw->als_ch0_expect;
	als_ch1_expect = obj->hw->als_ch1_expect;
#endif
#ifdef ontim_debug_info
	REGISTER_AND_INIT_ONTIM_DEBUG_FOR_THIS_DEV();
#endif

	APS_LOG("probe successful.\n");
	return 0;

exit_sensor_obj_attach_fail:
exit_create_attr_failed:
	misc_deregister(&stk3x1x_device);
exit_misc_device_register_failed:
exit_init_failed:
//i2c_detach_client(client);
//	exit_kfree:
	kfree(obj);
//		obj = NULL;
exit:
	stk3x1x_i2c_client = NULL;
#if defined(MTK_AUTO_DETECT_ALSPS)
	stk3x1x_init_flag = -1;
#endif
	APS_ERR("err = %d\n", err);
	return err;
}
/*----------------------------------------------------------------------------*/
static int stk3x1x_i2c_remove(struct i2c_client *client)
{
	int err;
#ifdef STK_TUNE0
	struct stk3x1x_priv *obj = i2c_get_clientdata(client);
	destroy_workqueue(obj->stk_ps_tune0_wq);
#endif
	//if((err = stk3x1x_delete_attr(&stk3x1x_i2c_driver.driver)))
	if((err = stk3x1x_delete_attr(&(stk3x1x_init_info.platform_diver_addr->driver))))
	{
		APS_ERR("stk3x1x_delete_attr fail: %d\n", err);
	}

	misc_deregister(&stk3x1x_device);

	stk3x1x_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));

	return 0;
}


static int stk3x1x_local_uninit(void)
{
	//APS_FUN();
	i2c_del_driver(&stk3x1x_i2c_driver);
	stk3x1x_i2c_client = NULL;
	return 0;
}

/*----------------------------------------------------------------------------*/
static int stk3x1x_local_init(void)
{
	//APS_FUN();
	if(i2c_add_driver(&stk3x1x_i2c_driver))
	{
		APS_ERR("add driver error\n");
		return -1;
	}

    if(-1 == stk3x1x_init_flag)
    {
        return -1;
    }
	return 0;
}

/*----------------------------------------------------------------------------*/
static int __init stk3x1x_init(void)
{
	APS_FUN();

   	if (alsps_driver_add(&stk3x1x_init_info))
		APS_ERR("alsps_driver_add fail\n");
	return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit stk3x1x_exit(void)
{
	APS_FUN();
}
/*----------------------------------------------------------------------------*/
module_init(stk3x1x_init);
module_exit(stk3x1x_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("MingHsien Hsieh");
MODULE_DESCRIPTION("SensorTek stk3x1x proximity and light sensor driver");
MODULE_LICENSE("GPL");
