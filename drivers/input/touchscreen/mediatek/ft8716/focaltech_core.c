/*
 *
 * FocalTech ftxxxx TouchScreen driver.
 *
 * Copyright (c) 2010-2015, Focaltech Ltd. All rights reserved.
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

/*******************************************************************************
*
* File Name: Ftxxxx_ts.c
*
*    Author: Tsai HsiangYu
*
*   Created: 2015-03-02
*
*  Abstract:
*
* Reference:
*
*******************************************************************************/

/*******************************************************************************
* 1.Included header files
*******************************************************************************/
/*
///user defined include header files
#include <linux/i2c.h>
#include <linux/input.h>
//#include <linux/earlysuspend.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/unistd.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/input/mt.h>
#include <linux/switch.h>
#include <linux/gpio.h>
*/

/*
 *#include "tpd.h"
 *#include "tpd_custom_fts.h"
 */

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/sched.h>
#include <linux/kthread.h>
//#include <linux/rtpm_prio.h> //lixh10
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/dma-mapping.h>
#include <linux/seq_file.h>

#include "focaltech_core.h"
#include <linux/of.h>
#include <linux/of_irq.h>
#ifdef FTS_INCALL_TOUCH_REJECTION
#include <mach/battery_common.h>
#include "../../../../misc/mediatek/alsps/epl8801/epl8801.h"
#endif
#include "../../../../misc/mediatek/include/mt-plat/mt_boot_common.h"
/*******************************************************************************
* 2.Private constant and macro definitions using #define
*******************************************************************************/
/*register define*/
#define FTS_RESET_PIN							FT_ESD_PROTECT
#define TPD_OK 									0
#define DEVICE_MODE 							0x00
#define GEST_ID 									0x01
#define TD_STATUS 								0x02
#define TOUCH1_XH 								0x03
#define TOUCH1_XL 								0x04
#define TOUCH1_YH 								0x05
#define TOUCH1_YL 								0x06
#define TOUCH2_XH 								0x09
#define TOUCH2_XL 								0x0A
#define TOUCH2_YH 								0x0B
#define TOUCH2_YL 								0x0C
#define TOUCH3_XH 								0x0F
#define TOUCH3_XL 								0x10
#define TOUCH3_YH 								0x11
#define TOUCH3_YL 								0x12
#define TPD_MAX_RESET_COUNT 					3
/*if need these function, pls enable this MACRO*/

/*
 *#define TPD_PROXIMITY
 */


#define FTS_CTL_IIC
#define SYSFS_DEBUG
#define FTS_APK_DEBUG


#if FT_ESD_PROTECT
#define TPD_ESD_CHECK_CIRCLE        	200
static struct delayed_work gtp_esd_check_work;
static struct workqueue_struct *gtp_esd_check_workqueue = NULL;
static void gtp_esd_check_func(struct work_struct *);

/*
 *add for esd
 */
static int count_irq = 0;
static unsigned long esd_check_circle = TPD_ESD_CHECK_CIRCLE;
static u8 run_check_91_register = 0;
u8 esd_running;
/*
 *spinlock esd_lock;
 */
 int  apk_debug_flag ;  

#endif

int tp_button_flag = 0;
/*
 *#ifdef FTS_CTL_IIC
 *	#include "focaltech_ctl.h"
 *#endif
 *#ifdef SYSFS_DEBUG
 *	#include "focaltech_ex_fun.h"
 *#endif
 */

/*PROXIMITY*/
#ifdef TPD_PROXIMITY
#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>
#endif

#ifdef TPD_PROXIMITY
#define APS_ERR(fmt,arg...)           	printk("<<proximity>> "fmt"\n",##arg)
#define TPD_PROXIMITY_DEBUG(fmt,arg...) printk("<<proximity>> "fmt"\n",##arg)
#define TPD_PROXIMITY_DMESG(fmt,arg...) printk("<<proximity>> "fmt"\n",##arg)
static u8 tpd_proximity_flag 			= 0;
/*
 *add for tpd_proximity by wangdongfang
 */
static u8 tpd_proximity_flag_one 		= 0;
/*
 *0-->close ; 1--> far away
 */
static u8 tpd_proximity_detect 		= 1;
#endif

#if FTS_ACCDET
extern int get_accdet_current_status(void);
static int fts_accdet_status = 0;
int fts_accdet(bool status);
#endif
/*dma declare, allocate and release*/
#define __MSG_DMA_MODE__
#ifdef __MSG_DMA_MODE__
u8 *g_dma_buff_va;
dma_addr_t g_dma_buff_pa;
#endif

#ifdef __MSG_DMA_MODE__

static void msg_dma_alloct(struct i2c_client *client)
{
	g_dma_buff_va = (u8 *)dma_alloc_coherent(&client->dev, 4096, &g_dma_buff_pa, GFP_KERNEL);/*DMA size 4096 for customer */
	if (!g_dma_buff_va) {
		FTS_DBG("[DMA][Error] Allocate DMA I2C Buffer failed!\n");
	}
}
static void msg_dma_release(struct i2c_client *client)
{
	if (g_dma_buff_va) {
		dma_free_coherent(&client->dev, 4096, g_dma_buff_va, g_dma_buff_pa);
		g_dma_buff_va = NULL;
		/*
		 *g_dma_buff_pa = NULL;
		 */
		printk("[DMA][release] Allocate DMA I2C Buffer release!\n");
	}
}
#endif
#ifdef TPD_HAVE_BUTTON
static int tpd_keys_local[TPD_KEY_COUNT] = TPD_KEYS;
static int tpd_keys_dim_local[TPD_KEY_COUNT][4] = TPD_KEYS_DIM;
#endif
#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))
static int tpd_wb_start_local[TPD_WARP_CNT] = TPD_WARP_START;
static int tpd_wb_end_local[TPD_WARP_CNT]   = TPD_WARP_END;
#endif
#if (defined(TPD_HAVE_CALIBRATION) && !defined(TPD_CUSTOM_CALIBRATION))
static int tpd_calmat_local[8]     = TPD_CALIBRATION_MATRIX;
static int tpd_def_calmat_local[8] = TPD_CALIBRATION_MATRIX;
#endif
/*******************************************************************************
* 3.Private enumerations, structures and unions using typedef
*******************************************************************************/

/*touch info*/
struct touch_info {
	int y[10];
	int x[10];
	int p[10];
	int id[10];
	int count;
};

/*******************************************************************************
* 4.Static variables
*******************************************************************************/
struct i2c_client *fts_i2c_client = NULL;
struct input_dev *fts_input_dev = NULL;
struct fts_touch_info fts_touch_info ;
struct task_struct *thread = NULL;
static unsigned int touch_irq = 0;
/*
 *static u8 last_touchpoint=0;
 *add by lixh10 start
 */
#ifdef FTS_INCALL_TOUCH_REJECTION
extern kal_bool g_call_state; /*lixh10 */
extern int epl_sensor_show_ps_status_for_tp() ;
#endif
#if  FTS_GESTRUE_EN
static int fts_wakeup_flag = 0;
static bool  fts_gesture_status = false;
int fts_gesture_letter = 0 ;
#endif
#if  FTS_GLOVE_EN
static bool  fts_glove_flag = false;
static int  fts_enter_glove(bool status);
#endif
#ifdef LENOVO_CTP_EAGE_LIMIT
static int eage_x = 5;
static int eage_y = 5;
#endif
static bool  fts_power_down = true;

#ifdef  TPD_AUTO_UPGRADE
struct workqueue_struct *touch_wq;
struct work_struct fw_update_work;
#endif

#ifdef LENOVO_CTP_TEST_FLUENCY
#define LCD_X 1080
#define LCD_Y 1920
static struct hrtimer tpd_test_fluency_timer;
#define TIMER_MS_TO_NS(x) (x * 1000 * 1000)

static int test_x = 0;
static int test_y = 0;
static int test_w = 0;
static int test_id = 0;
static int coordinate_interval = 10;
static int 	delay_time = 10;
static struct completion report_point_complete;

#endif
static bool  is_fw_upgrate = false;
#if FT_ESD_PROTECT
static bool  is_turnoff_checkesd = false;
#endif 
/*
 *add by lixh10 end
 */
#ifdef  GTP_PROC_TPINFO
#define TP_INFO_LENGTH_UINT    50
static const char fts_proc_name[] = "tp_info";
#endif

int up_flag = 0, up_count = 0;
static int tpd_flag = 0;
static int tpd_halt = 0;

static DECLARE_WAIT_QUEUE_HEAD(waiter);
/*
 *static DEFINE_MUTEX(i2c_access);
 */
static DEFINE_MUTEX(i2c_rw_access);

static irqreturn_t tpd_eint_interrupt_handler(unsigned irq, struct irq_desc *desc);
static int __devinit tpd_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int __devexit tpd_remove(struct i2c_client *client);
static int touch_event_handler(void *unused);
extern void mt_eint_mask(unsigned int eint_num);
extern void mt_eint_unmask(unsigned int eint_num);
extern void mt_eint_ack(unsigned int eint_num);
extern void mt_eint_set_hw_debounce(unsigned int eint_num, unsigned int ms);
extern void mt_eint_set_polarity(unsigned int eint_num, unsigned int pol);
extern unsigned int mt_eint_set_sens(unsigned int eint_num, unsigned int sens);
extern void mt_eint_registration(unsigned int eint_num, unsigned int flow, void (EINT_FUNC_PTR)(void), unsigned int is_auto_umask);
extern void lcm_power_off(void);
static int fts_read_touchinfo(void);
/*register driver and device info*/
static const struct i2c_device_id fts_tpd_id[] = {{"fts", 0}, {}};

static const struct of_device_id tpd_of_match[] = {
	{.compatible = "mediatek,cap_touch_cust"},
	{},
};

unsigned short force[] = {3, 0x70 >> 1, I2C_CLIENT_END, I2C_CLIENT_END};
static const unsigned short *const forces[] = { force, NULL };

static struct i2c_driver tpd_i2c_driver = {
	.probe = tpd_probe,
	.remove = __devexit_p(tpd_remove),
	.id_table = fts_tpd_id,
	.driver = {
		.name = "fts",
		.of_match_table = tpd_of_match,
	},
	.address_list = (const unsigned short *) forces,
};

/*
 *add by lixh10 start
 */

#ifdef LENOVO_CTP_TEST_FLUENCY
enum hrtimer_restart tpd_test_fluency_handler(struct hrtimer *timer)
{

	complete(&report_point_complete);
	return HRTIMER_NORESTART;
}

static int  fts_test_cfg(char *buf)
{
	int error;
	unsigned int  val[5] = {0,};
	error = sscanf(buf, "%d%d%d", &val[0], &val[1], &val[2]);
	/*
	 *dev_err(&fts_i2c_client->dev, "ahe fts_test_fluency para %d  %d  %d ! \n",val[0],val[1],val[2]);
	 */

	return val[0];

}
static ssize_t fts_test_fluency(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i, j;
	int count ;
	int startpoint;

	if (tpd_halt)
		return 0;

	j =  fts_test_cfg(buf);
	test_x =	LCD_X / 2 ; /*10 ;// LCD_Y/2 ;  //160--680 */
	test_y = 300 ;/*LCD_Y/2; */
	startpoint = test_y;
	test_w = 30;
	test_id = 1;
	count = (LCD_Y - test_y - 50) / coordinate_interval;
	
	disable_irq(touch_irq);
	dev_err(&fts_i2c_client->dev, "ahe fts_test_fluency start %d ! \n", j);
	for (; j > 0; j--) {
		for (i = 0; i < 2 * count; i++) {
			if (i < count) {
				test_y += coordinate_interval;
			} else {
				if (i == count)
					test_x += 10;
				test_y -= coordinate_interval;
			}

		
			//tpd_down(test_x, test_y, test_w);

			input_mt_slot(tpd->dev, test_id);
			input_mt_report_slot_state(tpd->dev, MT_TOOL_FINGER, true);
			input_report_key(tpd->dev, BTN_TOUCH, 1);

			input_report_abs(tpd->dev, ABS_MT_PRESSURE, test_w); /*0x3f  data->pressure[i] */
			input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 0x05); /*0x05  data->area[i] */
			input_report_abs(tpd->dev, ABS_MT_POSITION_X, test_x);
			input_report_abs(tpd->dev, ABS_MT_POSITION_Y, test_y);
			input_sync(tpd->dev);

			init_completion(&report_point_complete);
			hrtimer_start(&tpd_test_fluency_timer, ktime_set(delay_time / 1000, (delay_time % 1000) * 1000000), HRTIMER_MODE_REL);
			wait_for_completion(&report_point_complete);
		}
		/*
		 *tpd_up(test_x, test_y,NULL);
		 */
		input_mt_slot(tpd->dev, test_id);
		input_mt_report_slot_state(tpd->dev, MT_TOOL_FINGER, false);
		input_report_key(tpd->dev, BTN_TOUCH, 0);
		input_sync(tpd->dev);
		mdelay(50);
	}
	enable_irq(touch_irq);

	dev_err(&fts_i2c_client->dev, "ahe fts_test_fluency end  !!! \n");

	return 1;
}

static DEVICE_ATTR(fts_test_fluency, 0664,
		   NULL, fts_test_fluency);

static ssize_t fts_test_fluency_interval_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", coordinate_interval);
}

static ssize_t fts_test_fluency_interval_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int val;
	int error;

	error = sscanf(buf, "%d", &val);
	if (error != 1)
		return error;
	coordinate_interval = val;
	return count;
}


static DEVICE_ATTR(fts_test_interval, 0664,
		   fts_test_fluency_interval_show, fts_test_fluency_interval_store);


static ssize_t fts_test_fluency_delay_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", delay_time);
}

static ssize_t fts_test_fluency_delay_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int val;
	int error;

	error = sscanf(buf, "%d", &val);
	if (error != 1)
		return error;
	delay_time = val;
	return count;
}

static DEVICE_ATTR(fts_test_delay, 0664,
		   fts_test_fluency_delay_show, fts_test_fluency_delay_store);

#endif


#ifdef  TPD_AUTO_UPGRADE
static void fts_fw_update_work_func(struct work_struct *work)
{
	int ret ;
	bool upgrate_loop = true ;
	int  upgrate_cnt = 0 ;
	printk("********************FTS Enter CTP Auto Upgrade ********************\n");
	disable_irq(touch_irq);

	is_fw_upgrate = true ;
#if FT_ESD_PROTECT
	esd_switch(0);
	apk_debug_flag = 1;
#endif
	do{
		ret = fts_ctpm_auto_upgrade(fts_i2c_client);
		if(ret <0 && upgrate_cnt<2){
			printk("********************FTS  Auto Upgrade times %d********************\n",upgrate_cnt);
			upgrate_loop = true;
		}
		else
			upgrate_loop=false;
		upgrate_cnt++;
	}while(upgrate_loop);
	
	mdelay(10);
	is_fw_upgrate = false;
#if FT_ESD_PROTECT
	esd_switch(1);
	apk_debug_flag = 0;
#endif
#if FTS_USB_DETECT
	printk("mtk_tpd[fts] usb reg write \n");
	fts_usb_insert((bool) upmu_is_chr_det());
#endif

#if FTS_ACCDET
	fts_accdet(get_accdet_current_status());
#endif

	enable_irq(touch_irq);
	/*update  touch info after fw update */
	fts_read_touchinfo();
}
#endif
static int fts_read_touchinfo(void)
{
	unsigned char uc_reg_addr;
	unsigned char uc_reg_value[10]={0,};
	int retval =0;
	
	if(tpd_halt){
		dev_err(&fts_i2c_client->dev,"ahe cancel info reading  while suspend!\n");
		return retval;
	}
	uc_reg_addr = FTS_REG_CHIP_ID;
	retval = fts_i2c_write(fts_i2c_client, &uc_reg_addr, 1);
	 if(retval<0)
	 	goto I2C_FAIL; 
	retval = fts_i2c_read(fts_i2c_client, &uc_reg_addr, 0, uc_reg_value, 6);
	 if(retval<0)
	 	goto I2C_FAIL; 

	  fts_touch_info.ic_version= uc_reg_value[0];
	 fts_touch_info.fw_version = uc_reg_value[3];
	 fts_touch_info.vendor =  uc_reg_value[5];
	 dev_err(&fts_i2c_client->dev,"ahe [FTS] vendor = %02x \n", fts_touch_info.vendor);

 I2C_FAIL:
	return retval;
}	 	
static int fts_get_touchinfo(char *buf){
	int data_len;
	static char* vendor_id;
	
	switch (fts_touch_info.vendor){
	case 0x0a:
		vendor_id = "tianma";
		break;
	case 0x51:
		vendor_id = "ofilm";
		break;
	case 0xa0:
		vendor_id = "toptouch";
		break;
	case 0x90:
		vendor_id = "eachopto";
		break;
	case 0x3b:
		vendor_id = "biel";
		break;
	case 0x55:
		vendor_id = "laibao";
		break;
	default:
		vendor_id = "unknown";
	}
	
	data_len =  snprintf(buf, PAGE_SIZE, "fts - ic_version:%d; vendor_id:%s; fw_version:%d", fts_touch_info.ic_version,
			vendor_id, fts_touch_info.fw_version);
	
	return data_len;
	
}


static ssize_t fts_information_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return fts_get_touchinfo(buf);

}
static DEVICE_ATTR(touchpanel_info, 0664,
		   fts_information_show,
		   NULL);


#ifdef  GTP_PROC_TPINFO
static int fts_tpinfo_proc_show(struct seq_file *m, void *v)
{
	char buf[TP_INFO_LENGTH_UINT]={0,};
	
	 fts_get_touchinfo(buf);
	seq_printf(m, "%s\n",buf);
	
	return 0 ;
	
}

static int fts_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, fts_tpinfo_proc_show, inode->i_private);
}

static const struct file_operations fts_proc_tool_fops = {
	.open = fts_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};
#endif
#if FTS_GLOVE_EN
static ssize_t fts_glove_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", fts_glove_flag);
}

static ssize_t fts_glove_store(struct device *dev,
			       struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long val;
	int error;
	int ret;
	error = sstrtoul(buf, 10, &val);
	if (error)
		return error;
	if (val == 1) {
		if (!fts_glove_flag) {
			if (tpd_halt) {
				dev_err(&fts_i2c_client->dev, "fts Glove Send config error, power down.");
				return count;
			}
			fts_glove_flag = true;
			ret = fts_enter_glove(true);

		} else
			return count;
	} else {
		if (fts_glove_flag) {
			if (tpd_halt) {
				dev_err(&fts_i2c_client->dev, "fts Glove Send config error, power down.");
				return count;
			}
			fts_glove_flag = false;
			ret = fts_enter_glove(false);
		} else
			return count;
	}
	return count;
}

static DEVICE_ATTR(tpd_glove_status, 0664,
		   fts_glove_show,
		   fts_glove_store);
#endif


#if FTS_GESTRUE_EN
static int get_array_flag(void)
{
	return fts_gesture_letter;
}
static ssize_t lenovo_gesture_flag_show(struct device *dev,
					struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", get_array_flag());
}
static DEVICE_ATTR(lenovo_tpd_info, 0664,
		   lenovo_gesture_flag_show,
		   NULL);
static ssize_t lenovo_gesture_wakeup_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", fts_wakeup_flag);
}

static ssize_t lenovo_gesture_wakeup_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long val;
	int error;
	error = sstrtoul(buf, 10, &val);
	if (error)
		return error;
	if (val == 1) {
		if (!fts_wakeup_flag) {
			fts_wakeup_flag = 1;
			/*
			 *enable_irq_wake(ts->client->irq);
			 */
			dev_err(&fts_i2c_client->dev, "ahe %s,gesture flag is  = %ld", __func__, val);
			printk("ahe %s,gesture flag is  = %ld", __func__, val);
		} else
			return count;
	} else {
		if (fts_wakeup_flag) {
			fts_wakeup_flag = 0;
			/*
			 *disable_irq_wake(ts->client->irq);
			 */
			dev_err(&fts_i2c_client->dev, "ahe %s,gesture flag is  = %ld", __func__, val);
		} else
			return count;
	}
	return count;
}

static DEVICE_ATTR(tpd_suspend_status, 0664,
		   lenovo_gesture_wakeup_show,
		   lenovo_gesture_wakeup_store);
#endif

static struct attribute *fts_touch_attrs[] = {
	&dev_attr_touchpanel_info.attr,
#if FTS_GESTRUE_EN
	&dev_attr_tpd_suspend_status.attr,
	&dev_attr_lenovo_tpd_info.attr,
#endif
#if FTS_GLOVE_EN
	&dev_attr_tpd_glove_status.attr,
#endif
#ifdef LENOVO_CTP_TEST_FLUENCY
	&dev_attr_fts_test_fluency.attr,
	&dev_attr_fts_test_delay.attr,
	&dev_attr_fts_test_interval.attr,
#endif
	NULL,
};

static struct attribute_group fts_touch_group = {
	.attrs = fts_touch_attrs,
};


#ifdef LENOVO_CTP_EAGE_LIMIT
static int  tpd_edge_filter(int x, int y)
{
	int ret = 0;
	if (y > TPD_RES_Y) { /*by pass key */
		if (x == 270 || x == 540 || x == 810) {
			ret = 0;
		} else
			ret = 1;
		return ret ;
	}
	if (x < eage_x || x > (TPD_RES_X - eage_x) || y <  eage_y  || y > (TPD_RES_Y - eage_y)) {
		dev_err(&fts_i2c_client->dev, "ahe  edge_filter :  (%d , %d ) dropped!\n", x, y);
		ret = 1;
	}
	return ret ;
}
#endif

static void fts_release_all_finger(void)
{
	unsigned int finger_count = 0;

#ifndef MT_PROTOCOL_B
	input_mt_sync(tpd->dev);
#else
	for (finger_count = 0; finger_count < MT_MAX_TOUCH_POINTS; finger_count++) {
		input_mt_slot(tpd->dev, finger_count);
		input_mt_report_slot_state(tpd->dev, MT_TOOL_FINGER, false);
	}
#endif
	input_sync(tpd->dev);

}

#if FTS_GESTRUE_EN
static int  fts_enter_gesture(void)
{
	int ret = 0;
	ret = fts_write_reg(fts_i2c_client, 0xd0, 0x01);
	if (ret < 0)
		dev_err(&fts_i2c_client->dev, "[fts][ahe] enter gesture write  fail 0xd0:  ");
	if (fts_updateinfo_curr.CHIP_ID == 0x87 ) {
		ret = fts_write_reg(fts_i2c_client, 0xd1, 0x33);
		if (ret < 0)
			dev_err(&fts_i2c_client->dev, "[fts][ahe] enter gesture write  fail 0xd1:  ");
		ret = fts_write_reg(fts_i2c_client, 0xd2, 0x01);
		if (ret < 0)
			dev_err(&fts_i2c_client->dev, "[fts][ahe] enter gesture write  fail 0xd2:  ");
		/*
		 *fts_write_reg(fts_i2c_client, 0xd5, 0xff);
		 */
		ret = fts_write_reg(fts_i2c_client, 0xd6, 0x10);
		if (ret < 0)
			dev_err(&fts_i2c_client->dev, "[fts][ahe] enter gesture write  fail 0xd3:  ");
		/*
		 *fts_write_reg(fts_i2c_client, 0xd7, 0xff);
		 *fts_write_reg(fts_i2c_client, 0xd8, 0xff);
		 */
	}
	return ret ;
}
#endif

static int  fts_enter_glove(bool status)
{
	int ret = 0;
	static u8 buf_addr[2] = { 0 };
	static u8 buf_value[2] = { 0 };
	buf_addr[0] = 0xC0; /*glove control */

	if (status)
		buf_value[0] = 0x01;
	else
		buf_value[0] = 0x00;

	ret = fts_write_reg(fts_i2c_client, buf_addr[0], buf_value[0]);
	if (ret < 0) {
		dev_err(&fts_i2c_client->dev, "[fts][Touch] fts_enter_glove write value fail \n");
	}
	dev_err(&fts_i2c_client->dev, "[fts][Touch] glove status :  %d \n", status);

	return ret ;

}

#if FTS_ACCDET
int fts_accdet(bool status)
{
	int ret = 0;
	u8 buf_addr[2] = { 0 };
	u8 buf_value[2] = { 0 };
	u8 tp_state = 0x03;

	if (status)
		fts_accdet_status = 1;
	else
		fts_accdet_status = 0;

	if (tpd_load_status == 0)
		return ret ;
	if ((fts_i2c_client == NULL) || (tpd_halt == 1)) {
		dev_err(&fts_i2c_client->dev, "[fts][Touch] %s return :  %d \n", __func__, status);
		return ret;
	}
	buf_addr[0] = 0xC3; /*accdet plug ? */
	if (status)
		buf_value[0] = 0x01;
	else
		buf_value[0] = 0x00;

	ret = fts_read_reg(fts_i2c_client, 0xC3, &tp_state);

	if (tp_state == buf_value[0]) {
		dev_err(&fts_i2c_client->dev, "[fts][Touch] %s status cancel update  :  (before=%d, after=%d) \n", __func__, buf_value[0], status);
		return ret;
	}

	ret = fts_write_reg(fts_i2c_client, buf_addr[0], buf_value[0]);
	if (ret < 0) {
		dev_err(&fts_i2c_client->dev, "[fts][Touch] %s write value fail \n", __func__);
	} else
		dev_err(&fts_i2c_client->dev, "[fts][Touch] %s status update success  ~~~~~~ :  (before=%d, after=%d) \n", __func__, tp_state, status);
	return ret ;
}
EXPORT_SYMBOL(fts_usb_insert);
#endif

#ifdef  FTS_USB_DETECT
int  fts_usb_insert(bool status)
{
	int ret = 0;
	u8 buf_addr[2] = { 0 };
	u8 buf_value[2] = { 0 };
	u8 tp_state = 0x03;
	return 0; //just return 
	if (tpd_load_status == 0)
		return ret ;
	if ((fts_i2c_client == NULL) || (tpd_halt == 1)) {
		/*
		 *dev_err(&fts_i2c_client->dev, "[fts][Touch] fts_usb_insert return XXXX :  %d \n",status);
		 */
		return ret;
	}

	buf_addr[0] = 0x8B; /*usb plug ? */
	if (status)
		buf_value[0] = 0x01;
	else
		buf_value[0] = 0x00;

	ret = fts_read_reg(fts_i2c_client, 0x8B, &tp_state);
	/*
	 *ret = fts_read_reg(fts_i2c_client, buf_addr[0], tp_value[0]);
	 *dev_err(&fts_i2c_client->dev, "[fts][Touch] fts_usb_insert status update 11111 :  (%d,%d) \n",tp_state,status);
	 */
	if (tp_state == buf_value[0]) {
		/*
		 *dev_err(&fts_i2c_client->dev, "[fts][Touch] fts_usb_insert status cancel update  :  (%d,%d) \n",tp_value[0],status);
		 */
		return ret;
	}

	ret = fts_write_reg(fts_i2c_client, buf_addr[0], buf_value[0]);
	if (ret < 0) {
		dev_err(&fts_i2c_client->dev, "[fts][Touch] fts_usb_insert write value fail \n");
	} else
		dev_err(&fts_i2c_client->dev, "[fts][Touch] fts_usb_insert status update success  ~~~~~~ :  (%d,%d) \n", tp_state, status);


	return ret ;

}
#endif

static int  fts_power_ctrl(bool en)
{
	int ret = 0;
	return 0; //remove power control 
	if (en) {
		if (fts_power_down) {

			ret = regulator_enable(tpd->reg);	/*enable regulator*/
			if (ret)
				printk("%s: ahe regulator_enable() failed!\n", __func__);

			fts_power_down = false;
		}
	} else {
		if (!fts_power_down) {

			ret = regulator_disable(tpd->reg);	/*disable regulator*/
			if (ret)
				printk("%s: ahe regulator_disable() failed!\n", __func__);

			fts_power_down = true;
		}
	}
	return 0;
}
#ifdef TPD_AUTO_UPGRADE
static int fts_workqueue_init(void)
{
	/*
	 *TPD_FUN ();
	 */

	touch_wq = create_singlethread_workqueue("touch_wq");
	if (touch_wq) {
		INIT_WORK(&fw_update_work, fts_fw_update_work_func);
	} else {
		goto err_workqueue_init;
	}
	return 0;

err_workqueue_init:
	printk("create_singlethread_workqueue failed\n");
	return -1;
}
#endif
/*
 *add by lixh10 end
 */


/************************************************************************
* Name: fts_i2c_read
* Brief: i2c read
* Input: i2c info, write buf, write len, read buf, read len
* Output: get data in the 3rd buf
* Return: fail <0
***********************************************************************/
int fts_i2c_read(struct i2c_client *client, char *writebuf, int writelen, char *readbuf, int readlen)
{
	int ret = 0;
	int retry = 0 ;

	/*
	 * for DMA I2c transfer
	 */

	mutex_lock(&i2c_rw_access);

	if (writelen != 0) {
		/*
		 *DMA Write
		 */
		memcpy(g_dma_buff_va, writebuf, writelen);
		client->addr = ((client->addr) & (I2C_MASK_FLAG)) | (I2C_DMA_FLAG);
		for (retry = 0; retry < I2C_MAX_TRY; retry++) { /*add by lixh10 */
			if ((ret = i2c_master_send(client, (unsigned char *)g_dma_buff_pa, writelen)) != writelen) {
				/*
				 *dev_err(&client->dev, "###%s i2c write len=%x,buffaddr=%x\n", __func__,ret,*g_dma_buff_pa);
				 */
				dev_err(&fts_i2c_client->dev, "ahe r i2c_master_send failed retry %d\n", retry);
				msleep(20);
			} else
				break;
			if (tpd_halt)
				break ;
		}

		client->addr = client->addr & I2C_MASK_FLAG & (~ I2C_DMA_FLAG);
	}

	/*
	 *DMA Read
	 */
	if (readlen != 0)

	{
		client->addr = ((client->addr) & (I2C_MASK_FLAG)) | (I2C_DMA_FLAG);
		for (retry = 0; retry < I2C_MAX_TRY; retry++) { /*add by lixh10 */
			if ((ret = i2c_master_recv(client, (unsigned char *)g_dma_buff_pa, readlen)) != readlen) {
				dev_err(&fts_i2c_client->dev, "ahe r  i2c_master_recv  failed retry %d\n", retry - 1);
				msleep(20);
			} else
				break ;
			if (tpd_halt)
				break ;
		}
		memcpy(readbuf, g_dma_buff_va, readlen);
		client->addr = client->addr & I2C_MASK_FLAG & (~ I2C_DMA_FLAG);
	}

	mutex_unlock(&i2c_rw_access);

	return ret;

	/*
	int ret,i;


	if (writelen > 0) {
		struct i2c_msg msgs[] = {
			{
			 .addr = client->addr,
			 .flags = 0,
			 .len = writelen,
			 .buf = writebuf,
			 },
			{
			 .addr = client->addr,
			 .flags = I2C_M_RD,
			 .len = readlen,
			 .buf = readbuf,
			 },
		};
		ret = i2c_transfer(client->adapter, msgs, 2);
		if (ret < 0)
			dev_err(&client->dev, "f%s: i2c read error.\n",
				__func__);
	}
	else {
		struct i2c_msg msgs[] = {
			{
			 .addr = client->addr,
			 .flags = I2C_M_RD,
			 .len = readlen,
			 .buf = readbuf,
			 },
		};
		ret = i2c_transfer(client->adapter, msgs, 1);
		if (ret < 0)
			dev_err(&client->dev, "%s:i2c read error.\n", __func__);
	}
	return ret;
	*/

}

/************************************************************************
* Name: fts_i2c_write
* Brief: i2c write
* Input: i2c info, write buf, write len
* Output: no
* Return: fail <0
***********************************************************************/
int fts_i2c_write(struct i2c_client *client, char *writebuf, int writelen)
{
	int ret;
	/*
	 *int i = 0;
	 */
	int retry = 0;

	mutex_lock(&i2c_rw_access);

	/*
	 *client->addr = client->addr & I2C_MASK_FLAG;
	 */

	/*
	 *ret = i2c_master_send(client, writebuf, writelen);
	 */
	memcpy(g_dma_buff_va, writebuf, writelen);
	client->addr = ((client->addr) & (I2C_MASK_FLAG)) | (I2C_DMA_FLAG);
	for (retry = 0; retry < I2C_MAX_TRY; retry++) { /*add by lixh10 */
		if ((ret = i2c_master_send(client, (unsigned char *)g_dma_buff_pa, writelen)) != writelen) {
			dev_err(&fts_i2c_client->dev, "ahe i2c_master_send failed retry :%d\n", retry - 1);
			msleep(20);
		} else
			break;

		if (tpd_halt)
			break ;
	}

	client->addr = client->addr & I2C_MASK_FLAG & (~ I2C_DMA_FLAG);

	mutex_unlock(&i2c_rw_access);

	return ret;

	/*
	int ret;
	int i = 0;


	client->addr = client->addr & I2C_MASK_FLAG;


	struct i2c_msg msg[] = {
		{
		 .addr = client->addr,
		 .flags = 0,
		 .len = writelen,
		 .buf = writebuf,
		 },
	};

	ret = i2c_transfer(client->adapter, msg, 1);
	if (ret < 0)
		dev_err(&client->dev, "%s i2c write error.\n", __func__);
	return ret;
	*/
}
/************************************************************************
* Name: fts_write_reg
* Brief: write register
* Input: i2c info, reg address, reg value
* Output: no
* Return: fail <0
***********************************************************************/
int fts_write_reg(struct i2c_client *client, u8 regaddr, u8 regvalue)
{
	unsigned char buf[2] = {0};

	buf[0] = regaddr;
	buf[1] = regvalue;

	return fts_i2c_write(client, buf, sizeof(buf));
}
/************************************************************************
* Name: fts_read_reg
* Brief: read register
* Input: i2c info, reg address, reg value
* Output: get reg value
* Return: fail <0
***********************************************************************/
int fts_read_reg(struct i2c_client *client, u8 regaddr, u8 *regvalue)
{

	return fts_i2c_read(client, &regaddr, 1, regvalue, 1);

}

#if 0
/************************************************************************
* Name: tpd_down
* Brief: down info
* Input: x pos, y pos, id number
* Output: no
* Return: no
***********************************************************************/
static void tpd_down(int x, int y, int p)
{

	if (x > TPD_RES_X) {
		TPD_DEBUG("warning: IC have sampled wrong value.\n");;
		return;
	}
	/*
	 *dev_err(&fts_i2c_client->dev,"\n [zax] tpd_down (x=%d, y=%d,id=%d ) \n", x,y,p);
	 */
	input_report_key(tpd->dev, BTN_TOUCH, 1);
	input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 20);
	input_report_abs(tpd->dev, ABS_MT_PRESSURE, 0x3f);
	input_report_abs(tpd->dev, ABS_MT_POSITION_X, x);
	input_report_abs(tpd->dev, ABS_MT_POSITION_Y, y);


	/*
	 *printk("ahe tpd:D[%4d %4d %4d] ", x, y, p);
	 *track id Start 0
	 *input_report_abs(tpd->dev, ABS_MT_TRACKING_ID, p);
	 */
	input_mt_sync(tpd->dev);
	if (FACTORY_BOOT == get_boot_mode() || RECOVERY_BOOT == get_boot_mode()) {
		tpd_button(x, y, 1);
	}
	if (y > TPD_RES_Y) { /*virtual key debounce to avoid android ANR issue */
		/*
		 *msleep(50);
		 *dev_err(&fts_i2c_client->dev,"fts D virtual key \n");
		 */
	}
	TPD_EM_PRINT(x, y, x, y, p - 1, 1);

}
/************************************************************************
* Name: tpd_up
* Brief: up info
* Input: x pos, y pos, count
* Output: no
* Return: no
***********************************************************************/
static  void tpd_up(int x, int y, int *count)
{

	input_report_key(tpd->dev, BTN_TOUCH, 0);
	/*
	 *dev_err(&fts_i2c_client->dev,"\n [zax] tpd_up (x=%d, y=%d,id=%d\n) ", x,y,*count);
	 */
	input_mt_sync(tpd->dev);
	TPD_EM_PRINT(x, y, x, y, 0, 0);

	if (FACTORY_BOOT == get_boot_mode() || RECOVERY_BOOT == get_boot_mode()) {
		tpd_button(x, y, 0);
	}
	if (y > TPD_RES_Y) { /*virtual key debounce to avoid android ANR issue */
		/*
		 *msleep(50);
		 */
		dev_err(&fts_i2c_client->dev, "fts U virtual key \n");
	}

}
/************************************************************************
* Name: tpd_touchinfo
* Brief: touch info
* Input: touch info point, no use
* Output: no
* Return: success nonzero
***********************************************************************/

static int tpd_touchinfo(struct touch_info *cinfo, struct touch_info *pinfo, struct touch_info *ptest)
{
	int i = 0;
	char data[128] = {0};
	u16 high_byte, low_byte, reg;
	u8 report_rate = 0;
	p_point_num = point_num;
	if (tpd_halt) {
		FTS_DBG("tpd_touchinfo return ..\n");
		return false;
	}
	/*
	 *mutex_lock(&i2c_access);
	 */

	reg = 0x00;
	fts_i2c_read(fts_i2c_client, &reg, 1, data, 64);
	/*
	 *mutex_unlock(&i2c_access);
	 */

	/*get the number of the touch points*/

	point_num = data[2] & 0x0f;
	if (up_flag == 2) {
		up_flag = 0;
		for (i = 0; i < fts_updateinfo_curr.TPD_MAX_POINTS; i++) {
			cinfo->p[i] = data[3 + 6 * i] >> 6; /*event flag */

			cinfo->id[i] = data[3 + 6 * i + 2] >> 4; /*touch id */
			/*get the X coordinate, 2 bytes*/
			high_byte = data[3 + 6 * i];
			high_byte <<= 8;
			high_byte &= 0x0f00;
			low_byte = data[3 + 6 * i + 1];
			cinfo->x[i] = high_byte | low_byte;
			high_byte = data[3 + 6 * i + 2];
			high_byte <<= 8;
			high_byte &= 0x0f00;
			low_byte = data[3 + 6 * i + 3];
			cinfo->y[i] = high_byte | low_byte;


			if (point_num >= i + 1)
				continue;
			if (up_count == 0)
				continue;
			cinfo->p[i] = ptest->p[i - point_num]; /*event flag */


			cinfo->id[i] = ptest->id[i - point_num]; /*touch id */

			cinfo->x[i] = ptest->x[i - point_num];

			cinfo->y[i] = ptest->y[i - point_num];
			/*
			 *dev_err(&fts_i2c_client->dev," zax add two x = %d, y = %d, evt = %d,id=%d\n", cinfo->x[i], cinfo->y[i], cinfo->p[i], cinfo->id[i]);
			 */
			up_count--;


		}

		return true;
	}
	up_count = 0;
	for (i = 0; i < fts_updateinfo_curr.TPD_MAX_POINTS; i++) {

		cinfo->p[i] = data[3 + 6 * i] >> 6; /*event flag */

		if (0 == cinfo->p[i]) {
			/*
			 *dev_err(&fts_i2c_client->dev,"\n  zax enter add   \n");
			 */
			up_flag = 1;
		}
		cinfo->id[i] = data[3 + 6 * i + 2] >> 4; /*touch id */
		/*get the X coordinate, 2 bytes*/
		high_byte = data[3 + 6 * i];
		high_byte <<= 8;
		high_byte &= 0x0f00;
		low_byte = data[3 + 6 * i + 1];
		cinfo->x[i] = high_byte | low_byte;
		high_byte = data[3 + 6 * i + 2];
		high_byte <<= 8;
		high_byte &= 0x0f00;
		low_byte = data[3 + 6 * i + 3];
		cinfo->y[i] = high_byte | low_byte;

		if (up_flag == 1 && 1 == cinfo->p[i]) {
			up_flag = 2;
			point_num++;
			ptest->x[up_count] = cinfo->x[i];
			ptest->y[up_count] = cinfo->y[i];
			ptest->id[up_count] = cinfo->id[i];
			ptest->p[up_count] = cinfo->p[i];
			/*
			 *dev_err(&fts_i2c_client->dev," zax add x = %d, y = %d, evt = %d,id=%d\n", ptest->x[j], ptest->y[j], ptest->p[j], ptest->id[j]);
			 */
			cinfo->p[i] = 2;
			up_count++;
		}
	}
	if (up_flag == 1)
		up_flag = 0;
	/*
	 *printk(" tpd cinfo->x[0] = %d, cinfo->y[0] = %d, cinfo->p[0] = %d\n", cinfo->x[0], cinfo->y[0], cinfo->p[0]);
	 */
	return true;

};
#endif

/************************************************************************
* Name: fts_read_Touchdata
* Brief: report the point information
* Input: event info
* Output: get touch data in pinfo
* Return: success is zero
***********************************************************************/
/*
 *zax static unsigned int buf_count_add=0;
 *zax static unsigned int buf_count_neg=0;
 *unsigned int buf_count_add1;
 *unsigned int buf_count_neg1;
 *u8 buf_touch_data[30*POINT_READ_BUF] = { 0 };//0xFF
 */
static int fts_read_Touchdata(struct ts_event *data)//(struct ts_event *pinfo)
{
	u8 buf[POINT_READ_BUF] = { 0 };/*0xFF */
	int ret = -1;
	int i = 0;
	u8 pointid = FTS_MAX_ID;

	if (tpd_halt) {
		FTS_DBG("fts while suspend cancel read touchdata, return ..\n");
		return ret;
	}

	/*
	 *mutex_lock(&i2c_access);
	 */
	ret = fts_i2c_read(fts_i2c_client, buf, 1, buf, POINT_READ_BUF);
	if (ret < 0) {
		dev_err(&fts_i2c_client->dev, "%s read touchdata failed.\n", __func__);
		/*
		 *mutex_unlock(&i2c_access);
		 */
		return ret;
	}
#if 0
	for (i = 0; i < POINT_READ_BUF; i++) {
		dev_err(&fts_i2c_client->dev, "\n [fts] zax buf %d =(0x%02x)  \n", i, buf[i]);
	}
#endif
	/*
	 *mutex_unlock(&i2c_access);
	 */
	memset(data, 0, sizeof(struct ts_event));
	data->touch_point = 0;

	data->touch_point_num = buf[FT_TOUCH_POINT_NUM] & 0x0F;
	/*
	 *printk("tpd  fts_updateinfo_curr.TPD_MAX_POINTS=%d fts_updateinfo_curr.chihID=%d \n", fts_updateinfo_curr.TPD_MAX_POINTS,fts_updateinfo_curr.CHIP_ID);
	 */
	for (i = 0; i < fts_updateinfo_curr.TPD_MAX_POINTS; i++) {
		pointid = (buf[FTS_TOUCH_ID_POS + FTS_TOUCH_STEP * i]) >> 4;
		if (pointid >= FTS_MAX_ID)
			break;
		else
			data->touch_point++;
		data->au16_x[i] =
			(s16)(buf[FTS_TOUCH_X_H_POS + FTS_TOUCH_STEP * i] & 0x0F) <<
			8 | (s16) buf[FTS_TOUCH_X_L_POS + FTS_TOUCH_STEP * i];
		data->au16_y[i] =
			(s16)(buf[FTS_TOUCH_Y_H_POS + FTS_TOUCH_STEP * i] & 0x0F) <<
			8 | (s16) buf[FTS_TOUCH_Y_L_POS + FTS_TOUCH_STEP * i];
		data->au8_touch_event[i] =
			buf[FTS_TOUCH_EVENT_POS + FTS_TOUCH_STEP * i] >> 6;
		data->au8_finger_id[i] =
			(buf[FTS_TOUCH_ID_POS + FTS_TOUCH_STEP * i]) >> 4;

		data->pressure[i] =
			(buf[FTS_TOUCH_XY_POS + FTS_TOUCH_STEP * i]);/*cannot constant value */
		data->area[i] =
			(buf[FTS_TOUCH_MISC + FTS_TOUCH_STEP * i]) >> 4;
		if ((data->au8_touch_event[i] == 0 || data->au8_touch_event[i] == 2) && ((data->touch_point_num == 0) || (data->pressure[i] == 0 && data->area[i] == 0)))
			return ret;
		/*
		 *dev_err(&fts_i2c_client->dev,"\n [fts] zax data (id= %d ,x=%d,y= %d)\n ", data->au8_finger_id[i],data->au16_x[i],data->au16_y[i]);
		 */
		/*if(0==data->pressure[i])
		{
			data->pressure[i]=0x08;
		}
		if(0==data->area[i])
		{
			data->area[i]=0x08;
		}
		*/
		/*
		 *if ( pinfo->au16_x[i]==0 && pinfo->au16_y[i] ==0)
		 *	pt00f++;
		 */
	}
	ret = 0;
	/*
	 *zax buf_count_add++;
	 *buf_count_add1=buf_count_add;
	 *zax memcpy( buf_touch_data+(((buf_count_add-1)%30)*POINT_READ_BUF), buf, sizeof(u8)*POINT_READ_BUF );
	 */
	return ret;
}

/************************************************************************
* Name: fts_report_value
* Brief: report the point information
* Input: event info
* Output: no
* Return: success is zero
***********************************************************************/

static int fts_report_value(struct ts_event *data)
{
	/*
	 *struct ts_event *event = NULL;
	 */
	int i = 0, j = 0;
	int up_point = 0;
	int touchs = 0;


#if FT_ESD_PROTECT/*change by fts 0708 */
	if (!is_turnoff_checkesd) {
		esd_switch(0);
		apk_debug_flag = 1;
		is_turnoff_checkesd = true;
	}
#endif

	for (i = 0; i < data->touch_point; i++) {
		input_mt_slot(tpd->dev, data->au8_finger_id[i]);

		if (data->au8_touch_event[i] == 0 || data->au8_touch_event[i] == 2) {
			/*
			 *down
			 */
#ifdef LENOVO_CTP_EAGE_LIMIT
			if (tpd_edge_filter(data->au16_x[i], data->au16_y[i])) {
				continue;
			}
#endif
#ifdef FTS_INCALL_TOUCH_REJECTION
			if (g_call_state == CALL_ACTIVE) {
				/*
				 *dev_err(&fts_i2c_client->dev,"[fts] Incall  ~~~~~~");
				 */
				if (0 == epl_sensor_show_ps_status_for_tp()) {
					dev_err(&fts_i2c_client->dev, "[fts] Incall with P-sensor  ,so cancel report ~~~~~~");
					continue;
				}
			}
#endif
			input_mt_report_slot_state(tpd->dev, MT_TOOL_FINGER, true);
			input_report_abs(tpd->dev, ABS_MT_PRESSURE, data->pressure[i]); /*0x3f  data->pressure[i] */
			input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, data->area[i]); /*0x05  data->area[i] */
			input_report_abs(tpd->dev, ABS_MT_POSITION_X, data->au16_x[i]);
			input_report_abs(tpd->dev, ABS_MT_POSITION_Y, data->au16_y[i]);
			touchs |= BIT(data->au8_finger_id[i]);
			data->touchs |= BIT(data->au8_finger_id[i]);
			//dev_err(&fts_i2c_client->dev,"[fts] D ID (%d ,%d, %d) ", data->au8_finger_id[i],data->au16_x[i],data->au16_y[i]);
		} else {
			/*
			 *up
			 */
			//dev_err(&fts_i2c_client->dev,"[fts] U  ID: %d ", data->au8_finger_id[i]);
			up_point++;
			input_mt_report_slot_state(tpd->dev, MT_TOOL_FINGER, false);
			data->touchs &= ~BIT(data->au8_finger_id[i]);
		}

	}
	if (unlikely(data->touchs ^ touchs)) {
		for (i = 0; i < fts_updateinfo_curr.TPD_MAX_POINTS; i++) {
			if (BIT(i) & (data->touchs ^ touchs)) {
				up_point++; /*fts change 2015-0701 for no up event. */
				/*
				 *dev_err(&fts_i2c_client->dev,"[fts] ~~U  ID: %d ", i);
				 */
				input_mt_slot(tpd->dev, i);
				input_mt_report_slot_state(tpd->dev, MT_TOOL_FINGER, false);
			}
		}
	}
	data->touchs = touchs;
	/*fts change 2015-0707 for no up event start */
	if ((data->touch_point_num == 0)) {
		for (j = 0; j < fts_updateinfo_curr.TPD_MAX_POINTS; j++) {
			input_mt_slot(tpd->dev, j);
			input_mt_report_slot_state(tpd->dev, MT_TOOL_FINGER, false);
		}
		/*
		 *last_touchpoint=0;
		 */
		data->touchs = 0;
		up_point = fts_updateinfo_curr.TPD_MAX_POINTS;
		data->touch_point = up_point;
	}
	/*fts change 2015-0707 for no up event end */

	if (data->touch_point == up_point) {
		input_report_key(tpd->dev, BTN_TOUCH, 0);

#if FT_ESD_PROTECT  /*change by fts 0708 */
		if (is_turnoff_checkesd) {
			esd_switch(1);
			apk_debug_flag = 0;
			is_turnoff_checkesd = false;
		}
#endif
	} else {
		input_report_key(tpd->dev, BTN_TOUCH, 1);
	}

	input_sync(tpd->dev);

	/*
	 *last_touchpoint=data->touch_point_num;
	 */
	return 0;
	/*
	 *printk("tpd D x =%d,y= %d",event->au16_x[0],event->au16_y[0]);
	 */
}

#ifdef TPD_PROXIMITY
/************************************************************************
* Name: tpd_read_ps
* Brief: read proximity value
* Input: no
* Output: no
* Return: 0
***********************************************************************/
int tpd_read_ps(void)
{
	tpd_proximity_detect;
	return 0;
}
/************************************************************************
* Name: tpd_get_ps_value
* Brief: get proximity value
* Input: no
* Output: no
* Return: 0
***********************************************************************/
static int tpd_get_ps_value(void)
{
	return tpd_proximity_detect;
}
/************************************************************************
* Name: tpd_enable_ps
* Brief: enable proximity
* Input: enable or not
* Output: no
* Return: 0
***********************************************************************/
static int tpd_enable_ps(int enable)
{
	u8 state;
	int ret = -1;

	ret = fts_read_reg(fts_i2c_client, 0xB0, &state);
	if (ret < 0) {
		printk("[Focal][Touch] read value fail");
		/*
		 *return ret;
		 */
	}

	printk("[proxi_fts]read: 999 0xb0's value is 0x%02X\n", state);

	if (enable) {
		state |= 0x01;
		tpd_proximity_flag = 1;
		TPD_PROXIMITY_DEBUG("[proxi_fts]ps function is on\n");
	} else {
		state &= 0x00;
		tpd_proximity_flag = 0;
		TPD_PROXIMITY_DEBUG("[proxi_fts]ps function is off\n");
	}

	ret = fts_write_reg(fts_i2c_client, 0xB0, state);
	if (ret < 0) {
		printk("[Focal][Touch] write value fail");
		/*
		 *return ret;
		 */
	}
	TPD_PROXIMITY_DEBUG("[proxi_fts]write: 0xB0's value is 0x%02X\n", state);
	return 0;
}
/************************************************************************
* Name: tpd_ps_operate
* Brief: operate function for proximity
* Input: point, which operation, buf_in , buf_in len, buf_out , buf_out len, no use
* Output: buf_out
* Return: fail <0
***********************************************************************/
int tpd_ps_operate(void *self, uint32_t command, void *buff_in, int size_in,

		   void *buff_out, int size_out, int *actualout)
{
	int err = 0;
	int value;
	hwm_sensor_data *sensor_data;
	TPD_DEBUG("[proxi_fts]command = 0x%02X\n", command);

	switch (command) {
	case SENSOR_DELAY:
		if ((buff_in == NULL) || (size_in < sizeof(int))) {
			APS_ERR("Set delay parameter error!\n");
			err = -EINVAL;
		}
		/*
		 * Do nothing
		 */
		break;
	case SENSOR_ENABLE:
		if ((buff_in == NULL) || (size_in < sizeof(int))) {
			APS_ERR("Enable sensor parameter error!\n");
			err = -EINVAL;
		} else {
			value = *(int *)buff_in;
			if (value) {
				if ((tpd_enable_ps(1) != 0)) {
					APS_ERR("enable ps fail: %d\n", err);
					return -1;
				}
			} else {
				if ((tpd_enable_ps(0) != 0)) {
					APS_ERR("disable ps fail: %d\n", err);
					return -1;
				}
			}
		}
		break;
	case SENSOR_GET_DATA:
		if ((buff_out == NULL) || (size_out < sizeof(hwm_sensor_data))) {
			APS_ERR("get sensor data parameter error!\n");
			err = -EINVAL;
		} else {
			sensor_data = (hwm_sensor_data *)buff_out;
			if ((err = tpd_read_ps())) {
				err = -1;;
			} else {
				sensor_data->values[0] = tpd_get_ps_value();
				TPD_PROXIMITY_DEBUG("huang sensor_data->values[0] 1082 = %d\n", sensor_data->values[0]);
				sensor_data->value_divide = 1;
				sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
			}
		}
		break;
	default:
		APS_ERR("proxmy sensor operate function no this parameter %d!\n", command);
		err = -1;
		break;
	}
	return err;
}
#endif

/************************************************************************
* Name: touch_event_handler
* Brief: interrupt event from TP, and read/report data to Android system
* Input: no use
* Output: no
* Return: 0
***********************************************************************/
static int touch_event_handler(void *unused)
{
	struct touch_info;
#ifdef MT_PROTOCOL_B
	struct ts_event pevent;
#endif

	int ret = 0;
#ifdef TPD_PROXIMITY
	int err;
	hwm_sensor_data sensor_data;
	u8 proximity_status;
#endif
	u8 state;
	struct sched_param param = { .sched_priority = 4}; /*lixh10 just set to 4 instead REG_RT_PRIO(4)*/
	sched_setscheduler(current, SCHED_RR, &param);

	do {
		set_current_state(TASK_INTERRUPTIBLE);
		wait_event_interruptible(waiter, tpd_flag != 0);
		tpd_flag = 0;
		set_current_state(TASK_RUNNING);

#if FTS_GESTRUE_EN
		if (fts_gesture_status) {

			ret = fts_read_reg(fts_i2c_client, 0xd0, &state);
			if (ret < 0) {
				printk("[Focal][Touch] read value fail");
			}

			if (state == 1) {
				fts_read_Gestruedata();
				continue;
			}
		}
#endif

#ifdef TPD_PROXIMITY

		if (tpd_proximity_flag == 1) {
			ret = fts_read_reg(fts_i2c_client, 0xB0, &state);
			if (ret < 0) {
				printk("[Focal][Touch] read value fail");
				/*
				 *return ret;
				 */
			}
			TPD_PROXIMITY_DEBUG("proxi_fts 0xB0 state value is 1131 0x%02X\n", state);
			if (!(state & 0x01)) {
				tpd_enable_ps(1);
			}
			ret = fts_read_reg(fts_i2c_client, 0x01, &proximity_status);
			if (ret < 0) {
				printk("[Focal][Touch] read value fail");
				/*
				 *return ret;
				 */
			}
			TPD_PROXIMITY_DEBUG("proxi_fts 0x01 value is 1139 0x%02X\n", proximity_status);
			if (proximity_status == 0xC0) {
				tpd_proximity_detect = 0;
			} else if (proximity_status == 0xE0) {
				tpd_proximity_detect = 1;
			}

			TPD_PROXIMITY_DEBUG("tpd_proximity_detect 1149 = %d\n", tpd_proximity_detect);
			if ((err = tpd_read_ps())) {
				TPD_PROXIMITY_DMESG("proxi_fts read ps data 1156: %d\n", err);
			}
			sensor_data.values[0] = tpd_get_ps_value();
			sensor_data.value_divide = 1;
			sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
			/*
			 *if ((err = hwmsen_get_interrupt_data(ID_PROXIMITY, &sensor_data)))
			 *{
			 *	TPD_PROXIMITY_DMESG(" proxi_5206 call hwmsen_get_interrupt_data failed= %d\n", err);
			 *}
			 */
		}

#endif

#ifdef MT_PROTOCOL_B
		{
#if FT_ESD_PROTECT/*change by fts 0708 */
			esd_switch(0);
			apk_debug_flag = 1;
#endif

			ret = fts_read_Touchdata(&pevent);
			if (ret == 0)
				fts_report_value(&pevent);

#if FT_ESD_PROTECT  /*change by fts 0708 */
			esd_switch(1);
			apk_debug_flag = 0;
#endif
		}
#else
		{
			if (tpd_touchinfo(&cinfo, &pinfo, &ptest)) {
				printk("tpd point_num = %d\n", point_num);
				TPD_DEBUG_SET_TIME;
				if (point_num > 0) {
					for (i = 0; i < point_num; i++) { /*only support 3 point */
						/*
#ifdef LENOVO_CTP_EAGE_LIMIT
						    	if(tpd_edge_filter(cinfo.x[i] ,cinfo.y[i]))
						   	 {
						    		//printk("ahe edge_filter :  (%d , %d ) dropped!\n",cinfo.x[i],cinfo.y[i]);
						   		continue;
						    	}
#endif
						*/
						tpd_down(cinfo.x[i], cinfo.y[i], cinfo.id[i]);
					}
					input_sync(tpd->dev);
				} else {
					tpd_up(cinfo.x[0], cinfo.y[0], &cinfo.id[0]);
					/*
					 *TPD_DEBUG("release --->\n");
					 */
					input_sync(tpd->dev);
				}
			}
		}
#endif
	} while (!kthread_should_stop());
	return 0;
}
/************************************************************************
* Name: fts_reset_tp
* Brief: reset TP
* Input: pull low or high
* Output: no
* Return: 0
***********************************************************************/
void fts_reset_tp(int HighOrLow)
{

	if (HighOrLow) {
		tpd_gpio_output(GTP_RST_PORT, 1);
	} else {
		tpd_gpio_output(GTP_RST_PORT, 0);
	}

}

#if 0
/************************************************************************
* Name: tpd_detect
* Brief: copy device name
* Input: i2c info, board info
* Output: no
* Return: 0
***********************************************************************/
static int tpd_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	strcpy(info->type, TPD_DEVICE);
	return 0;
}
#endif
/************************************************************************
* Name: tpd_eint_interrupt_handler
* Brief: deal with the interrupt event
* Input: no
* Output: no
* Return: no
***********************************************************************/
static irqreturn_t tpd_eint_interrupt_handler(unsigned irq, struct irq_desc *desc)
{

	TPD_DEBUG_PRINT_INT;
	tpd_flag = 1;
#if FT_ESD_PROTECT
	count_irq ++;
#endif

	wake_up_interruptible(&waiter);
	return IRQ_HANDLED;
}
/************************************************************************
* Name: fts_init_gpio_hw
* Brief: initial gpio
* Input: no
* Output: no
* Return: 0
***********************************************************************/
static int fts_init_gpio_hw(void)
{

	int ret = 0;


	tpd_gpio_output(GTP_RST_PORT, 1);

	return ret;
}
/************************************************************************
* Name: tpd_probe
* Brief: driver entrance function for initial/power on/create channel
* Input: i2c info, device id
* Output: no
* Return: 0
***********************************************************************/
static int __devinit tpd_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int retval = TPD_OK;
	char data;
	int err = 0;
	int reset_count = 0;
#ifdef TPD_PROXIMITY
	int err;
	struct hwmsen_object obj_ps;
#endif

	struct device_node *node = NULL;
	u32 ints[2] = { 0, 0 };
reset_proc:
	fts_i2c_client = client;
	fts_input_dev = tpd->dev;

	/*
	 *reset should be pull down before power on/off    lixh10
	 */
	//fts_power_ctrl(false);
	tpd_gpio_output(GTP_RST_PORT, 0);
	msleep(10);

	/*
	 *power on, need confirm with SA
	 */
	tpd_gpio_as_int(GTP_INT_PORT);
	fts_power_ctrl(true);

	tpd_gpio_output(GTP_RST_PORT, 0);
	msleep(1);
	printk("  fts do probe  reset \n");
	tpd_gpio_output(GTP_RST_PORT, 1);
	
	/*
	 *mt_set_gpio_pull_select(GPIO_CTP_EINT_PIN, GPIO_PULL_DOWN);
	 */
	msleep(200);
	err = i2c_smbus_read_i2c_block_data(fts_i2c_client, 0x00, 1, &data);
	/*
	 *if auto upgrade fail, it will not read right value next upgrade.
	 */
	//printk("ahe  i2c addr : %d \n", fts_i2c_client->addr);
	printk("fts i2c test :ret %d,data:%d\n", err, data);
	if (err < 0 || data != 0) { /* reg0 data running state is 0; other state is not 0 */
		printk("fts I2C  test transfer error, line: %d\n", __LINE__);

		if (++reset_count < TPD_MAX_RESET_COUNT) {
			goto reset_proc;
		}

		fts_power_ctrl(false);

        			return -1; 
   	         }


	msg_dma_alloct(client);

	fts_init_gpio_hw();

	node = of_find_matching_node(node, touch_of_match);
	if (node) {
		of_property_read_u32_array(node, "debounce", ints, ARRAY_SIZE(ints));
		gpio_set_debounce(ints[0], ints[1]);

		touch_irq = irq_of_parse_and_map(node, 0);

		retval = request_irq(touch_irq, (irq_handler_t) tpd_eint_interrupt_handler, IRQF_TRIGGER_FALLING, "focal-eint", NULL);
		if (retval > 0) {
			retval = -1;
			printk("tpd request_irq IRQ LINE NOT AVAILABLE!.");
		}
	} else {
		printk("tpd request_irq can not find touch eint device node!.");
		retval = -1;
	}
	printk("[wj][%s]irq:%d, debounce:%d-%d:", __func__, touch_irq, ints[0], ints[1]);

#ifdef SYSFS_DEBUG
	fts_create_sysfs(fts_i2c_client);
#endif
	hidi2c_to_stdi2c(fts_i2c_client);
	fts_get_upgrade_array();
#ifdef FTS_CTL_IIC
	if (fts_rw_iic_drv_init(fts_i2c_client) < 0)
		dev_err(&client->dev, "%s:[FTS] create fts control iic driver failed\n", __func__);
#endif

#ifdef FTS_APK_DEBUG
	fts_create_apk_debug_channel(fts_i2c_client);
#endif

#ifdef TPD_PROXIMITY
	{
		obj_ps.polling = 1; /*0--interrupt mode;1--polling mode; */
		obj_ps.sensor_operate = tpd_ps_operate;
		if ((err = hwmsen_attach(ID_PROXIMITY, &obj_ps))) {
			TPD_DEBUG("hwmsen attach fail, return:%d.", err);
		}
	}
#endif
#if FT_ESD_PROTECT
	INIT_DELAYED_WORK(&gtp_esd_check_work, gtp_esd_check_func);
	gtp_esd_check_workqueue = create_workqueue("gtp_esd_check");
	queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work, TPD_ESD_CHECK_CIRCLE);
#endif

#ifdef LENOVO_CTP_TEST_FLUENCY
	hrtimer_init(&tpd_test_fluency_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	tpd_test_fluency_timer.function = tpd_test_fluency_handler;
#endif


#if FTS_GESTRUE_EN
	fts_Gesture_init(tpd->dev);
#endif
#ifdef MT_PROTOCOL_B
	/*
	 *#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0))
	 */
	input_mt_init_slots(tpd->dev, MT_MAX_TOUCH_POINTS, 0);
	/*
	 *#endif
	 */
#endif
	input_set_abs_params(tpd->dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(tpd->dev, ABS_MT_WIDTH_MAJOR,	0, 255, 0, 0);
	input_set_abs_params(tpd->dev, ABS_MT_POSITION_X, 0, TPD_RES_X, 0, 0);
	input_set_abs_params(tpd->dev, ABS_MT_POSITION_Y, 0, TPD_RES_Y, 0, 0);
	input_set_abs_params(tpd->dev, ABS_MT_PRESSURE, 0, 255, 0, 0);
	/*input_set_abs_params(tpd->dev, ABS_MT_TRACKING_ID, 0, 255, 0, 0);//add by lixh10 */
	/*
	 *add by lixh10
	 */

	thread = kthread_run(touch_event_handler, 0, TPD_DEVICE);
	if (IS_ERR(thread)) {
		retval = PTR_ERR(thread);
		FTS_DBG(TPD_DEVICE " failed to create kernel thread: %d\n", retval);
	}
#ifdef TPD_AUTO_UPGRADE
	if((KERNEL_POWER_OFF_CHARGING_BOOT == get_boot_mode()) || (LOW_POWER_OFF_CHARGING_BOOT == get_boot_mode()))
	{
		printk("[wj]Due to the power off charging mode,skip the auto f/w upgrade process.\n");
	}
	else
	{
		err = fts_workqueue_init ();
		if ( err != 0 )
		{
			printk( "fts_workqueue_init failed\n" );
			//goto err_probing;
		}else
			queue_work ( touch_wq, &fw_update_work );	
	}
#endif
	 
	if (sysfs_create_group(&fts_i2c_client->dev.kobj , &fts_touch_group)) {
		dev_err(&client->dev, "failed to create sysfs group\n");
		return -EAGAIN;
	}
	if (sysfs_create_link(NULL, &fts_i2c_client->dev.kobj , "lenovo_tp_gestures")) {
		dev_err(&client->dev, "failed to create sysfs symlink\n");
		return -EAGAIN;
	}
#ifdef  GTP_PROC_TPINFO
	if (proc_create(fts_proc_name, 0664, NULL, &fts_proc_tool_fops) == NULL) {
		dev_err(&client->dev,"fts create_proc_entry %s failed", fts_proc_name);
		return -1;
	}
	dev_err(&client->dev,"fts create_proc_entry %s success", fts_proc_name);
#endif

	tpd_load_status = 1;

#ifndef TPD_AUTO_UPGRADE
#if FTS_USB_DETECT
	printk("mtk_tpd[fts] usb reg write \n");
	fts_usb_insert((bool) upmu_is_chr_det());
#endif

#if FTS_ACCDET
	fts_accdet(get_accdet_current_status());
#endif
#endif
	/*read touch info from touch ic*/
	fts_read_touchinfo(); 
	printk("fts Touch Panel Device Probe %s\n", (retval < TPD_OK) ? "FAIL" : "PASS");
	return 0;

}
/************************************************************************
* Name: tpd_remove
* Brief: remove driver/channel
* Input: i2c info
* Output: no
* Return: 0
***********************************************************************/
static int __devexit tpd_remove(struct i2c_client *client)

{
	msg_dma_release(client);
	if (tpd_load_status) { /*add by lix10 */
#ifdef FTS_CTL_IIC
		fts_rw_iic_drv_exit();
#endif
#ifdef SYSFS_DEBUG
		fts_remove_sysfs(client);
#endif
#if FT_ESD_PROTECT
		destroy_workqueue(gtp_esd_check_workqueue);
#endif

#ifdef FTS_APK_DEBUG
		fts_release_apk_debug_channel();
#endif
#ifdef TPD_AUTO_UPGRADE
		cancel_work_sync(&fw_update_work);
#endif
#ifdef  GTP_PROC_TPINFO
	remove_proc_entry(fts_proc_name,  NULL);
#endif
	}
	TPD_DEBUG("TPD removed\n");

	return 0;
}
#if FT_ESD_PROTECT
void esd_switch(s32 on)
{
	/*
	 *spin_lock(&esd_lock);
	 */
	if (1 == on) { // switch on esd
		/*
		 *lixh10 change
		 */
		if (is_fw_upgrate == false)
			queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work, esd_check_circle);

	} else { /* switch off esd */
		/*
		 * if (esd_running)
		 * {
		 *   esd_running = 0;
		 *spin_unlock(&esd_lock);
		 *printk("\n zax Esd cancell \n");
		 */
		cancel_delayed_work(&gtp_esd_check_work);
		/*
		 * }
		 * else
		 * {
		 *    spin_unlock(&esd_lock);
		 *}
		 */
	}
}
/************************************************************************
* Name: force_reset_guitar
* Brief: reset
* Input: no
* Output: no
* Return: 0
***********************************************************************/
static void force_reset_guitar(void)
{

	disable_irq(touch_irq);
	tpd_gpio_output(GTP_RST_PORT, 0);
	msleep(5);
	FTS_DBG("force_reset_guitar\n");

	fts_power_ctrl(false);
	mdelay(100);

	fts_power_ctrl(true);

	mdelay(10);
	FTS_DBG(" fts ic reset\n");
	tpd_gpio_output(GTP_RST_PORT, 1);

#ifdef TPD_PROXIMITY
	if (FT_PROXIMITY_ENABLE == tpd_proximity_flag) {
		tpd_enable_ps(FT_PROXIMITY_ENABLE);
	}
#endif
	mdelay(50);
	enable_irq(touch_irq);

}

#define RESET_91_REGVALUE_SAMECOUNT 				5
static u8 g_old_91_Reg_Value = 0x00;
static u8 g_first_read_91 = 0x01;
static u8 g_91value_same_count = 0;
/************************************************************************
* Name: gtp_esd_check_func
* Brief: esd check function
* Input: struct work_struct
* Output: no
* Return: 0
***********************************************************************/
static void gtp_esd_check_func(struct work_struct *work)
{
	int i;
	int ret = -1;
	u8 data;
	//u8 flag_error = 0;
	int reset_flag = 0;
	/*
	 *dev_err(&fts_i2c_client->dev,"fts esd polling ~~~~~");
	 */
	if (tpd_halt) {
		return;
	}
	if (apk_debug_flag) {
		/*
		 *queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work, esd_check_circle);
		 *printk("zax ESD  flag=%d",apk_debug_flag);
		 */
		return;
	}
	/*
	 *printk("zax enetr ESD  flag=%d",apk_debug_flag);
	 */
	run_check_91_register = 0;
	for (i = 0; i < 3; i++) {
		ret = fts_read_reg(fts_i2c_client, 0xA3, &data);
		if (ret < 0) {
			printk("[Focal][Touch] read value fail");
			/*
			 *return ret;
			 */
		}
		if (ret == 1 && A3_REG_VALUE == data) {
			break;
		}
	}

	if (i >= 3) {
		force_reset_guitar(); 
		dev_err(&fts_i2c_client->dev,"focal--tpd reset. i >= 3  ret = %d	A3_Reg_Value = 0x%02x\n ", ret, data);
		reset_flag = 1;
		goto FOCAL_RESET_A3_REGISTER;
	}
#if 0
	/*
	 *esd check for count
	 */
	ret = fts_read_reg(fts_i2c_client, 0x8F, &data);
	if (ret < 0) {
		printk("[Focal][Touch] read value fail");
		/*
		 *return ret;
		 */
	}
	//dev_err(&fts_i2c_client->dev,"0x8F:%d, count_irq is %d\n", data, count_irq);

	flag_error = 0;
	if ((count_irq - data) > 10) {
		if ((data + 200) > (count_irq + 10)) {
			flag_error = 1;
		}
	}

	if ((data - count_irq) > 10) {
		flag_error = 1;
	}

	if (1 == flag_error) {
		dev_err(&fts_i2c_client->dev,"focal--tpd reset.1 == flag_error...data=%d	count_irq: %d\n ", data, count_irq);
		force_reset_guitar();
		reset_flag = 1;
		goto FOCAL_RESET_INT;
	}
#endif 
	run_check_91_register = 1;
	ret = fts_read_reg(fts_i2c_client, 0x91, &data);
	if (ret < 0) {
		printk("[Focal][Touch] read value fail");
		/*
		 *return ret;
		 */
	}
	printk("focal---------91 register value = 0x%02x	old value = 0x%02x\n",	data, g_old_91_Reg_Value);
	if (0x01 == g_first_read_91) {
		g_old_91_Reg_Value = data;
		g_first_read_91 = 0x00;
	} else {
		if (g_old_91_Reg_Value == data) {
			g_91value_same_count++;
			printk("focal 91 value ==============, g_91value_same_count=%d\n", g_91value_same_count);
			if (RESET_91_REGVALUE_SAMECOUNT == g_91value_same_count) {
				force_reset_guitar();
				dev_err(&fts_i2c_client->dev,"focal--tpd reset. g_91value_same_count = 5\n");
				g_91value_same_count = 0;
				reset_flag = 1;
			}

			/*
			 *run_check_91_register = 1;
			 */
			esd_check_circle = TPD_ESD_CHECK_CIRCLE / 2;
			g_old_91_Reg_Value = data;
		} else {
			g_old_91_Reg_Value = data;
			g_91value_same_count = 0;
			/*
			 *run_check_91_register = 0;
			 */
			esd_check_circle = TPD_ESD_CHECK_CIRCLE;
		}
	}
FOCAL_RESET_A3_REGISTER:
	count_irq = 0;
	data = 0;
	/*
	 *fts_i2c_smbus_write_i2c_block_data(i2c_client, 0x8F, 1, &data);
	 */
	ret = fts_write_reg(fts_i2c_client, 0x8F, data);
	if (ret < 0) {
		printk("[Focal][Touch] write value fail");
		/*
		 *return ret;
		 */
	}
	if (0 == run_check_91_register) {
		g_91value_same_count = 0;
	}
#ifdef TPD_PROXIMITY
	if ((1 == reset_flag) && (FT_PROXIMITY_ENABLE == tpd_proximity_flag)) {
		if ((tpd_enable_ps(FT_PROXIMITY_ENABLE) != 0)) {
			APS_ERR("enable ps fail\n");
			return -1;
		}
	}
#endif
	/*
	 *end esd check for count
	 */

	if (!tpd_halt) {
		/*
		 *queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work, TPD_ESD_CHECK_CIRCLE);
		 */
		queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work, esd_check_circle);
	}

	return;
}
#endif

/************************************************************************
* Name: tpd_local_init
* Brief: add driver info
* Input: no
* Output: no
* Return: fail <0
***********************************************************************/
static int tpd_local_init(void)
{
	printk("Focaltech fts I2C Touchscreen Driver (Built @)\n");

#if 0  //lixh10 remove power control 
	printk("[ahe ]open vldo2V8.\n");
	tpd->reg = regulator_get(tpd->tpd_dev, "vtouch");
	ret = regulator_set_voltage(tpd->reg, 2800000, 2800000);	/*set 2.8v*/
	if (ret) {
		printk("ahe regulator_set_voltage(%d) failed!\n", ret);
		return -1;
	}
#endif 
	if (i2c_add_driver(&tpd_i2c_driver) != 0) {
		printk("fts unable to add i2c driver.\n");
		return -1;
	}
	if (tpd_load_status == 0) {
		printk("fts add  touch panel driver error.\n");
		i2c_del_driver(&tpd_i2c_driver);
		return -1;
	}
	/*
	 *TINNO_TOUCH_TRACK_IDS <--- finger number
	 *TINNO_TOUCH_TRACK_IDS	5
	 */
#if (LINUX_VERSION_CODE > KERNEL_VERSION(3, 8, 0))
	/*
	 *for linux 3.8
	 */
	input_set_abs_params(tpd->dev, ABS_MT_TRACKING_ID, 0, (TPD_MAX_POINTS_5 - 1), 0, 0);
#endif


#ifdef TPD_HAVE_BUTTON
	/*
	 * initialize tpd button data
	 */
	tpd_button_setting(TPD_KEY_COUNT, tpd_keys_local, tpd_keys_dim_local);
#endif

#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))
	TPD_DO_WARP = 1;
	memcpy(tpd_wb_start, tpd_wb_start_local, TPD_WARP_CNT * 4);
	memcpy(tpd_wb_end, tpd_wb_start_local, TPD_WARP_CNT * 4);
#endif

#if (defined(TPD_HAVE_CALIBRATION) && !defined(TPD_CUSTOM_CALIBRATION))
	memcpy(tpd_calmat, tpd_def_calmat_local, 8 * 4);
	memcpy(tpd_def_calmat, tpd_def_calmat_local, 8 * 4);
#endif
	printk("end %s, %d\n", __FUNCTION__, __LINE__);
	tpd_type_cap = 1;
	return 0;
}
/************************************************************************
* Name: tpd_resume
* Brief: system wake up
* Input: no use
* Output: no
* Return: no
***********************************************************************/
static void tpd_resume(struct device *dev)
{
	/*
	 *int i=0,ret = 0;
	 */
	dev_err(&fts_i2c_client->dev, "fts wake up~~~~\n");

#ifdef TPD_PROXIMITY
	if (tpd_proximity_flag == 1) {
		if (tpd_proximity_flag_one == 1) {
			tpd_proximity_flag_one = 0;
			dev_err(&fts_i2c_client->dev, " fts tpd_proximity_flag_one \n");
			return;
		}
	}
#endif
#if FTS_GESTRUE_EN
	if (fts_gesture_status) {
		/*fts_write_reg(fts_i2c_client,0xD0,0x00); //only reset can exit */
		fts_gesture_status = false; /*reset ic so exit */
		dev_err(&fts_i2c_client->dev, "[fts][ahe] exit  gesture $$$$$$$ ");
	}
#endif
#ifdef TPD_CLOSE_POWER_IN_SLEEP
	tpd_gpio_output(GTP_RST_PORT, 0);
	mdelay(10);
	fts_power_ctrl(true);
#else
	/*
	 *just do reset change by lixh10
	 */
	tpd_gpio_output(GTP_RST_PORT, 1);
	mdelay(10);
	tpd_gpio_output(GTP_RST_PORT, 0);
	mdelay(10);
#endif
	tpd_gpio_output(GTP_RST_PORT, 1);
	mdelay(150);/*why delay ?? */
	tpd_halt = 0;
#ifdef FTS_GLOVE_EN
	if (fts_glove_flag)
		fts_enter_glove(true);
#endif

#if FTS_USB_DETECT
	fts_usb_insert((bool) upmu_is_chr_det());
#endif
	enable_irq(touch_irq);

	fts_release_all_finger();

#if FT_ESD_PROTECT
	count_irq = 0;
	queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work, TPD_ESD_CHECK_CIRCLE);
#endif

#if FTS_ACCDET
	fts_accdet(fts_accdet_status);
#endif
	dev_err(&fts_i2c_client->dev, "fts wake up done\n");

}
/************************************************************************
* Name: tpd_suspend
* Brief: system sleep
* Input: no use
* Output: no
* Return: no
***********************************************************************/
static void tpd_suspend(struct device *dev)
{
	int ret = 0;
#ifndef TPD_CLOSE_POWER_IN_SLEEP
	char data =  0x00 ;
#endif 

	dev_err(&fts_i2c_client->dev, "fts  enter sleep~~~~\n");
#ifdef TPD_PROXIMITY
	if (tpd_proximity_flag == 1) {
		tpd_proximity_flag_one = 1;
		return;
	}
#endif

#if FT_ESD_PROTECT

	cancel_delayed_work_sync(&gtp_esd_check_work);
#endif
#if FTS_GESTRUE_EN
	if (fts_wakeup_flag && (!fts_gesture_status)) {
		fts_gesture_status = true ;
	}
#endif
	/*
	 *mutex_lock(&i2c_access);
	 */
#if FTS_GESTRUE_EN
	if (fts_gesture_status) {
		ret = fts_enter_gesture();
		if (ret >= 0)
			dev_err(&fts_i2c_client->dev, "[fts][ahe] enter gesture $$$$$$$ ");
#ifdef FTS_GLOVE_EN
		if (fts_glove_flag)
			fts_enter_glove(false);
#endif
	} else {
#endif
		dev_err(&fts_i2c_client->dev, "[fts][ahe] no gesture will enter normal sleep ~~~~ ");
		tpd_halt = 1;
		disable_irq(touch_irq);

#ifdef TPD_CLOSE_POWER_IN_SLEEP
		tpd_gpio_output(GTP_RST_PORT, 0);
		mdelay(10)
		fts_power_ctrl(false);
#else
		if ((fts_updateinfo_curr.CHIP_ID == 0x59)) {
			ret = fts_write_reg(fts_i2c_client, 0xA5, data);
			if (ret < 0) {
				printk("[Focal][Touch] write value fail");
				/*
				 *return ret;
				 */
			}
		} else {
			data = 0x03;/*change by lixh10 */
			ret = fts_write_reg(fts_i2c_client, 0xA5, data);
			if (ret < 0) {
				printk("[Focal][Touch] write value fail");
				/*
				 *return ret;
				 */
			}else
			dev_err(&fts_i2c_client->dev, "[fts][ahe] suspend enter low power mode !!  ");
				
		dev_err(&fts_i2c_client->dev, "[fts] ft8716 cut lcm power off   !! ");
		}
#endif
		lcm_power_off();
		tpd_gpio_output(GTP_RST_PORT, 0);

#if FTS_GESTRUE_EN
	}
#endif
	/*
	 *mutex_unlock(&i2c_access);
	 */
	fts_release_all_finger();
	dev_err(&fts_i2c_client->dev, "fts fts enter sleep done\n");

}


static struct tpd_driver_t tpd_device_driver = {
	.tpd_device_name = "fts",
	.tpd_local_init = tpd_local_init,
	.suspend = tpd_suspend,
	.resume = tpd_resume,

#ifdef TPD_HAVE_BUTTON
	.tpd_have_button = 1,
#else
	.tpd_have_button = 0,
#endif

};

/************************************************************************
* Name: tpd_suspend
* Brief:  called when loaded into kernel
* Input: no
* Output: no
* Return: 0
***********************************************************************/
static int __init tpd_driver_init(void)
{
	printk("MediaTek fts touch panel driver init\n");
	tpd_get_dts_info();
	if (tpd_driver_add(&tpd_device_driver) < 0)
		printk("add fts driver failed\n");
	return 0;
}


/************************************************************************
* Name: tpd_driver_exit
* Brief:  should never be called
* Input: no
* Output: no
* Return: 0
***********************************************************************/
static void __exit tpd_driver_exit(void)
{
	printk("MediaTek fts touch panel driver exit\n");
	/*
	 *input_unregister_device(tpd->dev);
	 */
	tpd_driver_remove(&tpd_device_driver);
}

module_init(tpd_driver_init);
module_exit(tpd_driver_exit);
