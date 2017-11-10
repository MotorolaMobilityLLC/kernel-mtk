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

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/sched.h>
#include <linux/kthread.h>
//#include <linux/rtpm_prio.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/gpio.h>
#include<linux/wakelock.h>
#include "tpd_ft5x0x_common.h"
#include <linux/workqueue.h>

#include "mt_boot_common.h"

#include "focaltech_core.h"
/* #include "ft5x06_ex_fun.h" */

#include "tpd.h"

#ifdef MT_PROTOCOL_B
#include <linux/input/mt.h>
#endif
#ifdef WT_CTP_INFORMATION
#include <linux/hardware_info.h>
static char tp_string_version[40];
#endif
/* #define TIMER_DEBUG */

//#define CONFIG_MTK_I2C_EXTENSION_ZX

#ifdef TIMER_DEBUG
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#endif

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>

#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
#include <mach/md32_ipi.h>
#include <mach/md32_helper.h>
#endif

#ifdef FTS_MCAP_TEST
#include "mcap_test_lib.h"
#endif


#define TPD_PROXIMITY //zhangwenkang.wt,add tp-proximity fun,20161011 close
#ifdef TPD_PROXIMITY
	#include <hwmsensor.h>
	#include <hwmsen_dev.h>
	#include <sensors_io.h>
#endif

static int tpd_halt = 0;
//static int up_count = 0;
//static int point_num = 0;
#if FT_ESD_PROTECT
extern int  fts_esd_protection_init(void);
extern int  fts_esd_protection_exit(void);
extern int  fts_esd_protection_notice(void);
extern int  fts_esd_protection_suspend(void);
extern int  fts_esd_protection_resume(void);

int fts_esd_test= 0;
u8 fts_iic_cnt =0;
int apk_debug_flag = 0; 
//int factory_mode_flag = 0;
//int  power_switch_gesture = 0;
#define TPD_ESD_CHECK_CIRCLE        		200
static struct delayed_work gtp_esd_check_work;
//static struct workqueue_struct *gtp_esd_check_workqueue = NULL;

 void fts_esd_check_func(struct work_struct *work);
static int count_irq = 0; //add for esd
//static unsigned long esd_check_circle = TPD_ESD_CHECK_CIRCLE;
static u8 run_check_91_register;
#endif


#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
enum DOZE_T {
	DOZE_DISABLED = 0,
	DOZE_ENABLED = 1,
	DOZE_WAKEUP = 2,
};
static DOZE_T doze_status = DOZE_DISABLED;
#endif

#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
static s8 ftp_enter_doze(struct i2c_client *client);

enum TOUCH_IPI_CMD_T {
	/* SCP->AP */
	IPI_COMMAND_SA_GESTURE_TYPE,
	/* AP->SCP */
	IPI_COMMAND_AS_CUST_PARAMETER,
	IPI_COMMAND_AS_ENTER_DOZEMODE,
	IPI_COMMAND_AS_ENABLE_GESTURE,
	IPI_COMMAND_AS_GESTURE_SWITCH,
};

struct Touch_Cust_Setting {
	u32 i2c_num;
	u32 int_num;
	u32 io_int;
	u32 io_rst;
};

struct Touch_IPI_Packet {
	u32 cmd;
	union {
		u32 data;
		Touch_Cust_Setting tcs;
	} param;
};

/* static bool tpd_scp_doze_en = FALSE; */
static bool tpd_scp_doze_en = TRUE;
DEFINE_MUTEX(i2c_access);
#endif

#define TPD_SUPPORT_POINTS		5


struct i2c_client *i2c_client = NULL;
struct task_struct *thread_tpd = NULL;
/*******************************************************************************
* 4.Static variables
*******************************************************************************/
struct i2c_client *fts_i2c_client 				= NULL;
struct input_dev *fts_input_dev				=NULL;
#ifdef TPD_AUTO_UPGRADE
static bool is_update = false;
#endif
#if 1//def CONFIG_FT_AUTO_UPGRADE_SUPPORT
u8 *tpd_i2c_dma_va = NULL;
dma_addr_t tpd_i2c_dma_pa = 0;
#endif

#if FTS_GESTRUE_EN	//wwm add//
struct wake_lock gesture_chrg_lock;
#endif			//wwm add//

static DECLARE_WAIT_QUEUE_HEAD(waiter);
extern int fts_gesture_sysfs(struct i2c_client * client);
//extern unsigned char  g_hq_ctp_module_fw_version;
static irqreturn_t tpd_eint_interrupt_handler(int irq, void *dev_id);


static int tpd_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int tpd_i2c_detect(struct i2c_client *client, struct i2c_board_info *info);
static int tpd_remove(struct i2c_client *client);
static int touch_event_handler(void *unused);
static void tpd_resume(struct device *h);
static void tpd_suspend(struct device *h);
static int tpd_flag;
/*static int point_num = 0;
static int p_point_num = 0;*/

#ifdef USB_CHARGE_DETECT
u8 b_usb_plugin = 0;
int ctp_is_probe = 0;
#endif


#if FTS_GESTRUE_EN	//modify for zal1519 20160506
static struct device *mx_tsp;  	
extern int gesture_onoff_fts;	
u8 fts_gesture_three_byte_one = 0;
u8 fts_gesture_three_byte_two = 0;
u8 fts_gesture_three_byte_three = 0;
u8 fts_gesture_three_byte_four = 0;
int fts_gesture_data=0;
int fts_gesture_ctrl=0;
int fts_gesture_mask = 0;
//static int gesture_data1=0;
//static int gesture_data2=0;
//static int gesture_data3=0;
#endif 
unsigned int tpd_rst_gpio_number = 0;
static unsigned int tpd_int_gpio_number = 1;
static unsigned int touch_irq = 0;
#define TPD_OK 0

/* Register define */
#define DEVICE_MODE	0x00
#define GEST_ID		0x01
#define TD_STATUS	0x02

#define TOUCH1_XH	0x03
#define TOUCH1_XL	0x04
#define TOUCH1_YH	0x05
#define TOUCH1_YL	0x06

#define TOUCH2_XH	0x09
#define TOUCH2_XL	0x0A
#define TOUCH2_YH	0x0B
#define TOUCH2_YL	0x0C

#define TOUCH3_XH	0x0F
#define TOUCH3_XL	0x10
#define TOUCH3_YH	0x11
#define TOUCH3_YL	0x12

#define TPD_RESET_ISSUE_WORKAROUND
#define TPD_MAX_RESET_COUNT	3



#ifdef TPD_PROXIMITY
	#define APS_ERR(fmt,arg...)           	printk("<<proximity>> "fmt"\n",##arg)
	#define TPD_PROXIMITY_DEBUG(fmt,arg...) printk("<<proximity>> "fmt"\n",##arg)
	#define TPD_PROXIMITY_DMESG(fmt,arg...) printk("<<proximity>> "fmt"\n",##arg)
	static u8 tpd_proximity_flag 			= 0;
	//add for tpd_proximity by wangdongfang
	static u8 tpd_proximity_flag_one 		= 0; 
	//0 -->close ; 10--> far away
	static u8 tpd_proximity_detect 		= 10;
#endif

#ifdef FTS_MCAP_TEST
struct i2c_client *g_focalclient = NULL;
extern int focal_i2c_Read(unsigned char *writebuf, int writelen, unsigned char *readbuf, int readlen);
extern int focal_i2c_Write(unsigned char *writebuf, int writelen);
#endif
//+bug 258645 zhangwenkang.wt 20170419, add headphone detection function
#define CONFIG_ENABLE_HEADPHONES_DETECTION
#ifdef CONFIG_ENABLE_HEADPHONES_DETECTION

static u8 bSuspendHeadphones = 0;
static struct hrtimer headphonesTimer;
static struct workqueue_struct *headphonesWq;
static struct work_struct headphonesWork;

void ftDrvCommonReadFile(char *pFilePath, u8 *pBuf, u16 nLength)
{
    struct file *pFile = NULL;
    mm_segment_t old_fs;
    ssize_t nReadBytes = 0;    

    old_fs = get_fs();
    set_fs(get_ds());

    pFile = filp_open(pFilePath, O_RDONLY, 0);
    if (IS_ERR(pFile)) {
        printk("Open file failed: %s\n", pFilePath);
        return;
    }

    pFile->f_op->llseek(pFile, 0, SEEK_SET);
    nReadBytes = pFile->f_op->read(pFile, pBuf, nLength, &pFile->f_pos);
   
    set_fs(old_fs);        
    filp_close(pFile, NULL);    
}

static u8 DrvGetHeadphonesStateFromSys(void)
{
                u8 szHeadphonesState[5] = {0};
     
                ftDrvCommonReadFile("/sys/class/switch/h2w/state", szHeadphonesState, 5);
                            
                if (strstr(szHeadphonesState, "1") != NULL || strstr(szHeadphonesState, "2") != NULL) 
                {
                    return 1;
                }
                else 
                {
                    return 0;
                }
 }

static void DrvSetHeadphonesStateToFw(u8 bPlugIn)
{

	 s32 rc = -1;
	 u8 reg_val = 0;

	 
	if(bPlugIn)
	{
		 reg_val = 0x0f;
	}
	else 
	{
		reg_val = 0x00;
	}

	 mdelay(10); 
         rc = fts_write_reg(fts_i2c_client, 0x8a,reg_val);
	if(rc < 0)
	{
	   printk("headphone write fail\n");
	}
	 mdelay(10);  

}

static enum hrtimer_restart headphones_timer_func(struct hrtimer *timer)
{   
	queue_work(headphonesWq, &headphonesWork);
	return HRTIMER_NORESTART;
}

 static void headphones_work_func(struct work_struct *work)
{
	static u8 lastHeadphonesState = 0;
	u8 currHeadphonesState = DrvGetHeadphonesStateFromSys();
	
	if(lastHeadphonesState!=currHeadphonesState)
	{
		DrvSetHeadphonesStateToFw(currHeadphonesState);
	}
	
 	lastHeadphonesState = currHeadphonesState;

        hrtimer_start(&headphonesTimer, ktime_set(0, 125000000), HRTIMER_MODE_REL);

}

static void headphones_init(void)
{
	headphonesWq = create_singlethread_workqueue("headphones detection");
	INIT_WORK(&headphonesWork, headphones_work_func);

	hrtimer_init(&headphonesTimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	headphonesTimer.function = headphones_timer_func;
	hrtimer_start(&headphonesTimer, ktime_set(1, 0), HRTIMER_MODE_REL);
}
#endif
//-bug 258645 zhangwenkang.wt 20170419, add headphone detection function
//Modify,@hubing1.wt 2016.7.25
#ifdef LCT_ADD_TP_LOCKDOWN_INFO
extern unsigned char ft5x46_ctpm_LockDownInfo_get_from_boot(  struct i2c_client *client,char *pProjectCode );
#define 	 CTP_PROC_LOCKDOWN_FILE "tp_lockdown_info"
static struct proc_dir_entry *ctp_lockdown_status_proc = NULL;
char tp_lockdown_info[128];

static int ctp_lockdown_proc_show(struct seq_file *file, void* data)
{
	char temp[40] = {0};

 	sprintf(temp, "%s\n", tp_lockdown_info);
	seq_printf(file, "%s\n", temp);
	
	return 0;	
}

static int ctp_lockdown_proc_open (struct inode* inode, struct file* file) 
{
	return single_open(file, ctp_lockdown_proc_show, inode->i_private);
}

static const struct file_operations ctp_lockdown_proc_fops = 
{
	.open = ctp_lockdown_proc_open,
	.read = seq_read,
};
#endif


#ifdef TIMER_DEBUG

static struct timer_list test_timer;

static void timer_func(unsigned long data)
{
	tpd_flag = 1;
	wake_up_interruptible(&waiter);

	mod_timer(&test_timer, jiffies + 100*(1000/HZ));
}

static int init_test_timer(void)
{
	memset((void *)&test_timer, 0, sizeof(test_timer));
	test_timer.expires  = jiffies + 100*(1000/HZ);
	test_timer.function = timer_func;
	test_timer.data     = 0;
	init_timer(&test_timer);
	add_timer(&test_timer);
	return 0;
}
#endif


#if defined(CONFIG_TPD_ROTATE_90) || defined(CONFIG_TPD_ROTATE_270) || defined(CONFIG_TPD_ROTATE_180)
/*
static void tpd_swap_xy(int *x, int *y)
{
	int temp = 0;

	temp = *x;
	*x = *y;
	*y = temp;
}
*/
/*
static void tpd_rotate_90(int *x, int *y)
{
//	int temp;

	*x = TPD_RES_X + 1 - *x;

	*x = (*x * TPD_RES_Y) / TPD_RES_X;
	*y = (*y * TPD_RES_X) / TPD_RES_Y;

	tpd_swap_xy(x, y);
}
*/
static void tpd_rotate_180(int *x, int *y)
{
	*y = TPD_RES_Y + 1 - *y;
	*x = TPD_RES_X + 1 - *x;
}
/*
static void tpd_rotate_270(int *x, int *y)
{
//	int temp;

	*y = TPD_RES_Y + 1 - *y;

	*x = (*x * TPD_RES_Y) / TPD_RES_X;
	*y = (*y * TPD_RES_X) / TPD_RES_Y;

	tpd_swap_xy(x, y);
}
*/
#endif
struct touch_info {
	int y[TPD_SUPPORT_POINTS];
	int x[TPD_SUPPORT_POINTS];
	int p[TPD_SUPPORT_POINTS];
	int id[TPD_SUPPORT_POINTS];
	int count;
	int eventnum;
};

/*dma declare, allocate and release*/
//#define __MSG_DMA_MODE__
#ifdef CONFIG_MTK_I2C_EXTENSION_FOCAL
	u8 *g_dma_buff_va = NULL;
	dma_addr_t g_dma_buff_pa = 0;
#endif

#ifdef CONFIG_MTK_I2C_EXTENSION_FOCAL

	static void msg_dma_alloct(void)
	{
	    if (NULL == g_dma_buff_va)
    		{
       		 tpd->dev->dev.coherent_dma_mask = DMA_BIT_MASK(32);
       		 g_dma_buff_va = (u8 *)dma_alloc_coherent(&tpd->dev->dev, 128, &g_dma_buff_pa, GFP_KERNEL);
    		}

	    	if(!g_dma_buff_va)
		{
	        	TPD_DMESG("[DMA][Error] Allocate DMA I2C Buffer failed!\n");
	    	}
	}
	static void msg_dma_release(void){
		if(g_dma_buff_va)
		{
	     		dma_free_coherent(NULL, 128, g_dma_buff_va, g_dma_buff_pa);
	        	g_dma_buff_va = NULL;
	        	g_dma_buff_pa = 0;
			TPD_DMESG("[DMA][release] Allocate DMA I2C Buffer release!\n");
	    	}
	}
#endif

static DEFINE_MUTEX(i2c_access);
static DEFINE_MUTEX(i2c_rw_access);

#if (defined(CONFIG_TPD_HAVE_CALIBRATION) && !defined(CONFIG_TPD_CUSTOM_CALIBRATION))
/* static int tpd_calmat_local[8]     = TPD_CALIBRATION_MATRIX; */
/* static int tpd_def_calmat_local[8] = TPD_CALIBRATION_MATRIX; */
static int tpd_def_calmat_local_normal[8]  = TPD_CALIBRATION_MATRIX_ROTATION_NORMAL;
static int tpd_def_calmat_local_factory[8] = TPD_CALIBRATION_MATRIX_ROTATION_FACTORY;
#endif

static const struct i2c_device_id ft5x0x_tpd_id[] = {{"FT6436U", 0}, {} };
static const struct of_device_id ft5x0x_dt_match[] = {
	{.compatible = "mediatek,cap_touch1"},
	{},
};
MODULE_DEVICE_TABLE(of, ft5x0x_dt_match);

static struct i2c_driver tpd_i2c_driver = {
	.driver = {
		.of_match_table = of_match_ptr(ft5x0x_dt_match),
		.name = "FT6436U",
	},
	.probe = tpd_probe,
	.remove = tpd_remove,
	.id_table = ft5x0x_tpd_id,
	.detect = tpd_i2c_detect,
};

static int of_get_ft5x0x_platform_data(struct device *dev)
{
	/*int ret, num;*/

	if (dev->of_node) {
		const struct of_device_id *match;

		match = of_match_device(of_match_ptr(ft5x0x_dt_match), dev);
		if (!match) {
			TPD_DMESG("Error: No device match found\n");
			return -ENODEV;
		}
	}
//	tpd_rst_gpio_number = of_get_named_gpio(dev->of_node, "rst-gpio", 0);
//	tpd_int_gpio_number = of_get_named_gpio(dev->of_node, "int-gpio", 0);
	/*ret = of_property_read_u32(dev->of_node, "rst-gpio", &num);
	if (!ret)
		tpd_rst_gpio_number = num;
	ret = of_property_read_u32(dev->of_node, "int-gpio", &num);
	if (!ret)
		tpd_int_gpio_number = num;
  */
	printk("g_vproc_en_gpio_number %d\n", tpd_rst_gpio_number);
	printk("g_vproc_vsel_gpio_number %d\n", tpd_int_gpio_number);
	return 0;
}

#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
static ssize_t show_scp_ctrl(struct device *dev, struct device_attribute *attr, char *buf)
{
	return 0;
}
static ssize_t store_scp_ctrl(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	u32 cmd;
	Touch_IPI_Packet ipi_pkt;

	if (kstrtoul(buf, 10, &cmd)) {
		TPD_DEBUG("[SCP_CTRL]: Invalid values\n");
		return -EINVAL;
	}

	TPD_DEBUG("SCP_CTRL: Command=%d", cmd);
	switch (cmd) {
	case 1:
	    /* make touch in doze mode */
	    tpd_scp_wakeup_enable(TRUE);
	    tpd_suspend(NULL);
	    break;
	case 2:
	    tpd_resume(NULL);
	    break;
		/*case 3:
	    // emulate in-pocket on
	    ipi_pkt.cmd = IPI_COMMAND_AS_GESTURE_SWITCH,
	    ipi_pkt.param.data = 1;
		md32_ipi_send(IPI_TOUCH, &ipi_pkt, sizeof(ipi_pkt), 0);
	    break;
	case 4:
	    // emulate in-pocket off
	    ipi_pkt.cmd = IPI_COMMAND_AS_GESTURE_SWITCH,
	    ipi_pkt.param.data = 0;
		md32_ipi_send(IPI_TOUCH, &ipi_pkt, sizeof(ipi_pkt), 0);
	    break;*/
	case 5:
		{
				Touch_IPI_Packet ipi_pkt;

				ipi_pkt.cmd = IPI_COMMAND_AS_CUST_PARAMETER;
			    ipi_pkt.param.tcs.i2c_num = TPD_I2C_NUMBER;
			ipi_pkt.param.tcs.int_num = CUST_EINT_TOUCH_PANEL_NUM;
				ipi_pkt.param.tcs.io_int = tpd_int_gpio_number;
			ipi_pkt.param.tcs.io_rst = tpd_rst_gpio_number;
			if (md32_ipi_send(IPI_TOUCH, &ipi_pkt, sizeof(ipi_pkt), 0) < 0)
				TPD_DEBUG("[TOUCH] IPI cmd failed (%d)\n", ipi_pkt.cmd);

			break;
		}
	default:
	    TPD_DEBUG("[SCP_CTRL] Unknown command");
	    break;
	}

	return size;
}
static DEVICE_ATTR(tpd_scp_ctrl, 0664, show_scp_ctrl, store_scp_ctrl);
#endif

static struct device_attribute *ft5x0x_attrs[] = {
#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
	&dev_attr_tpd_scp_ctrl,
#endif
};



#ifdef USB_CHARGE_DETECT
void tpd_usb_plugin(u8 plugin)
{
	int ret = -1;
	b_usb_plugin = plugin;

	if(!ctp_is_probe)
	{
		return;
	}
	printk("Fts usb detect: %s %d.\n",__func__,b_usb_plugin);

	ret = i2c_smbus_write_i2c_block_data(fts_i2c_client, 0x8B, 1, &b_usb_plugin);
	if ( ret < 0 )
	{
		printk("Fts usb detect write err: %s %d.\n",__func__,b_usb_plugin);
	}
}
EXPORT_SYMBOL(tpd_usb_plugin);
#endif


static void tpd_down(int x, int y, int p, int id)
{

#ifdef MT_PROTOCOL_B
	input_mt_slot(tpd->dev, id);
	input_mt_report_slot_state(tpd->dev, MT_TOOL_FINGER, 1);
#endif

	input_report_abs(tpd->dev, ABS_MT_PRESSURE, 0x3f);//add  2
	//input_report_abs(tpd->dev, ABS_MT_TRACKING_ID, id);
	printk("%s x:%d y:%d p:%d\n", __func__, x, y, p);
	//input_report_key(tpd->dev, BTN_TOUCH, 1);
	input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 1);
	input_report_abs(tpd->dev, ABS_MT_POSITION_X, x);
	input_report_abs(tpd->dev, ABS_MT_POSITION_Y, y);
	//input_mt_sync(tpd->dev);

#ifndef MT_PROTOCOL_B
	/* track id Start 0 */
	input_report_abs(tpd->dev, ABS_MT_TRACKING_ID, id);
	input_mt_sync(tpd->dev);
#endif
	
	if(FACTORY_BOOT == get_boot_mode() || RECOVERY_BOOT == get_boot_mode())
	{
		printk("[FTS]tpd_down,tpd_button\n");
		tpd_button(x, y, 1);
	}
}

static void tpd_up(int x, int y, int id)
{

#ifdef MT_PROTOCOL_B
	input_mt_slot(tpd->dev, id);
	input_mt_report_slot_state(tpd->dev, MT_TOOL_FINGER, 0);
#endif

	printk("%s x:%d y:%d\n", __func__, x, y);
	//input_report_key(tpd->dev, BTN_TOUCH, 0);
	//input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 0);
#ifndef MT_PROTOCOL_B
	input_mt_sync(tpd->dev);
#endif
	if (FACTORY_BOOT == get_boot_mode() || RECOVERY_BOOT == get_boot_mode())
	{
		printk("[FTS]tpd_up,tpd_button\n");
		tpd_button(x, y, 0);
	}
}

/*Coordination mapping*/
/*
static void tpd_calibrate_driver(int *x, int *y)
{
	int tx;

	tx = ((tpd_def_calmat[0] * (*x)) + (tpd_def_calmat[1] * (*y)) + (tpd_def_calmat[2])) >> 12;
	*y = ((tpd_def_calmat[3] * (*x)) + (tpd_def_calmat[4] * (*y)) + (tpd_def_calmat[5])) >> 12;
	*x = tx;
}
*/
static int tpd_touchinfo(struct touch_info *cinfo, struct touch_info *pinfo)
{
	int i = 0;
	char data[40] = {0};
	//u8 report_rate = 0;
	u16 high_byte, low_byte;
	char writebuf[10]={0};
	//u8 fwversion = 0;

	writebuf[0]=0x00;
	fts_i2c_read(i2c_client, writebuf,  1, data, 32);
#if 0

	fts_read_reg(i2c_client, 0xa6, &fwversion);
	fts_read_reg(i2c_client, 0x88, &report_rate);

	TPD_DEBUG("FW version=%x]\n", fwversion);
	TPD_DEBUG("received raw data from touch panel as following:\n");
	for (i = 0; i < 8; i++)
		TPD_DEBUG("data[%d] = 0x%02X ", i, data[i]);
	TPD_DEBUG("\n");
	for (i = 8; i < 16; i++)
		TPD_DEBUG("data[%d] = 0x%02X ", i, data[i]);
	TPD_DEBUG("\n");
	for (i = 16; i < 24; i++)
		TPD_DEBUG("data[%d] = 0x%02X ", i, data[i]);
	TPD_DEBUG("\n");
	for (i = 24; i < 32; i++)
		TPD_DEBUG("data[%d] = 0x%02X ", i, data[i]);
	TPD_DEBUG("\n");
#endif
	/*if (report_rate < 8) {
		report_rate = 0x8;
		if ((fts_write_reg(i2c_client, 0x88, report_rate)) < 0)
			TPD_DMESG("I2C write report rate error, line: %d\n", __LINE__);
	}
	*/
	/* Device Mode[2:0] == 0 :Normal operating Mode*/
	if ((data[0] & 0x70) != 0)
		return false;

	if(tpd_halt)//esd reset ,tpd suspend
		return false;
	memcpy(pinfo, cinfo, sizeof(struct touch_info));
	memset(cinfo, 0, sizeof(struct touch_info));
	for (i = 0; i < TPD_SUPPORT_POINTS; i++)
		cinfo->p[i] = 1;	/* Put up */
		//cinfo->p[i] = 0xff;	//event flag,default,invalid

	/*get the number of the touch points*/
	cinfo->count = data[2] & 0x0f;
	cinfo->eventnum = 0;
	printk("[fts]point_num = %d\n", cinfo->count);

	for (i = 0; i <TPD_SUPPORT_POINTS; i++) {				
		cinfo->p[i] = (data[3 + 6 * i] >> 6) & 0x0003; /* event flag */
		cinfo->id[i] = data[3+6*i+2]>>4; 	// touch id

		if(cinfo->id[i] > TPD_SUPPORT_POINTS)
			return true;

		/*get the X coordinate, 2 bytes*/
		high_byte = data[3 + 6 * i];
		high_byte <<= 8;
		high_byte &= 0x0F00;

		low_byte = data[3 + 6 * i + 1];
		low_byte &= 0x00FF;
		cinfo->x[i] = high_byte | low_byte;

		/*get the Y coordinate, 2 bytes*/
		high_byte = data[3 + 6 * i + 2];
		high_byte <<= 8;
		high_byte &= 0x0F00;

		low_byte = data[3 + 6 * i + 3];
		low_byte &= 0x00FF;
		cinfo->y[i] = high_byte | low_byte;

		cinfo->eventnum++;

		printk(" cinfo->x[%d] = %d, cinfo->y[%d] = %d, cinfo->p[%d] = %d, cinfo->id = %d count = %d \n", i,
	 cinfo->x[i], i, cinfo->y[i], i, cinfo->p[i], cinfo->id[i], cinfo->count);
	}

	return true;

};
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
	//tpd_proximity_detect;
	return 0;    
}
 /************************************************************************
* Name: tpd_get_ps_value
* Brief: get proximity value
* Input: no
* Output: no
* Return: 0
***********************************************************************/
 int tpd_get_ps_value(void)
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
 int tpd_enable_ps(int enable)
{
	u8 state;
	int ret = -1;
	
	//i2c_smbus_read_i2c_block_data(fts_i2c_client, 0xB0, 1, &state);

	ret = fts_read_reg(fts_i2c_client, 0xB0,&state);
	if (ret<0) 
	{
		printk("[Focal][Touch] read value fail");
		//return ret;
	}
	
	printk("[proxi_fts]read: 999 0xb0's value is 0x%02X\n", state);

	if (enable)
	{
		state |= 0x01;
		tpd_proximity_flag = 1;
		TPD_PROXIMITY_DEBUG("[proxi_fts]ps function is on\n");	
	}
	else
	{
		state &= 0x00;	
		tpd_proximity_flag = 0;
		TPD_PROXIMITY_DEBUG("[proxi_fts]ps function is off\n");
	}
	
	//ret = i2c_smbus_write_i2c_block_data(fts_i2c_client, 0xB0, 1, &state);
	ret = fts_write_reg(fts_i2c_client, 0xB0,state);
	if (ret<0) 
	{
		printk("[Focal][Touch] write value fail");
		//return ret;
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
int tpd_ps_operate(void* self, uint32_t command, void* buff_in, int size_in,

		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	struct hwm_sensor_data *sensor_data;
	TPD_DEBUG("[proxi_fts]command = 0x%02X\n", command);		
	
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
					if((tpd_enable_ps(1) != 0))
					{
						APS_ERR("enable ps fail: %d\n", err); 
						return -1;
					}
				}
				else
				{
					if((tpd_enable_ps(0) != 0))
					{
						APS_ERR("disable ps fail: %d\n", err); 
						return -1;
					}
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
				sensor_data = (struct hwm_sensor_data *)(buff_out);				
				if((err = tpd_read_ps()))
				{
					err = -1;
				}
				else
				{
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


#ifdef TPD_PROXIMITY
static long tp_ps_ioctl(struct file *file, unsigned int cmd,unsigned long arg)
{
	int ret = 0,  ps_value = 0;
	void __user *data = (void __user *)arg;
	unsigned int enable = 0;
	
	switch (cmd) { 
	case ALSPS_GET_PS_DATA:
		ps_value = tpd_get_ps_value();
		if (copy_to_user(data, &ps_value, sizeof(ps_value)))
		{
			printk("%s, copy to usr error\n", __func__);
			ret =  -EINVAL;
		}
		break;
	case ALSPS_SET_PS_MODE:
		if (copy_from_user(&enable, data, sizeof(enable))) 
		{
			ret = -EFAULT;
			break;
		}
		if(enable)
		{		
			if(tpd_enable_ps(1) < 0)
			{
				printk("enable ps fail\n"); 
				return -EFAULT;
			}
		}
		else
		{
			if(tpd_enable_ps(0) < 0)
			{
				printk("disable ps fail\n"); 
				return -EFAULT;
			}
		}
		break;
	default:
		ret = -EINVAL;
	}
	return ret;
}

static struct file_operations tp_ps_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = tp_ps_ioctl,
};

static struct miscdevice tp_ps_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "tp_emulate_ps",
	.fops = &tp_ps_fops,
};
#endif

#ifdef CONFIG_MTK_I2C_EXTENSION_FOCAL

int i2c_master_send_fts(const struct i2c_client *client, const char *buf, int count)
{
	int ret;
	struct i2c_adapter *adap = client->adapter;
	struct i2c_msg msg;

	msg.addr = client->addr;
	msg.flags = client->flags & I2C_M_TEN;
	msg.len = count;
	msg.buf = (char *)buf;
#ifdef CONFIG_MTK_I2C_EXTENSION
	msg.timing = 400 ; //client->timing;
	msg.ext_flag = client->ext_flag;
#endif

	ret = i2c_transfer(adap, &msg, 1);

	/*
	 * If everything went ok (i.e. 1 msg transmitted), return #bytes
	 * transmitted, else error code.
	 */
	return (ret == 1) ? count : ret;
}



int fts_i2c_read(struct i2c_client *client, char *writebuf,int writelen, char *readbuf, int readlen)
{
	int ret=0;

	// for DMA I2c transfer

	mutex_lock(&i2c_rw_access);

	if((NULL!=client) && (writelen>0) && (writelen<=128))
	{
		// DMA Write
		memcpy(g_dma_buff_va, writebuf, writelen);
		client->addr = (client->addr & I2C_MASK_FLAG) | I2C_DMA_FLAG;
		if((ret=i2c_master_send_fts(client, (unsigned char *)g_dma_buff_pa, writelen))!=writelen)
			//dev_err(&client->dev, "###%s i2c write len=%x,buffaddr=%x\n", __func__,ret,*g_dma_buff_pa);
			printk("i2c write failed\n");
		client->addr = (client->addr & I2C_MASK_FLAG) &(~ I2C_DMA_FLAG);
	}

	// DMA Read

	if((NULL!=client) && (readlen>0) && (readlen<=128))

	{
		client->addr = (client->addr & I2C_MASK_FLAG) | I2C_DMA_FLAG;

		ret = i2c_master_recv(client, (unsigned char *)g_dma_buff_pa, readlen);

		memcpy(readbuf, g_dma_buff_va, readlen);

		client->addr = (client->addr & I2C_MASK_FLAG) &(~ I2C_DMA_FLAG);
	}

	mutex_unlock(&i2c_rw_access);

	return ret;
}

int fts_i2c_write(struct i2c_client *client, char *writebuf, int writelen)
{
	int i = 0;
	int ret = 0;

	if (writelen <= 8) {
	    client->ext_flag = client->ext_flag & (~I2C_DMA_FLAG);
		return i2c_master_send_fts(client, writebuf, writelen);
	}
	else if((writelen > 8)&&(NULL != g_dma_buff_va))
	{
		for (i = 0; i < writelen; i++)
			g_dma_buff_va[i] = writebuf[i];

		client->addr = (client->addr & I2C_MASK_FLAG )| I2C_DMA_FLAG;
	    //client->ext_flag = client->ext_flag | I2C_DMA_FLAG;
	    ret = i2c_master_send_fts(client, (unsigned char *)g_dma_buff_pa, writelen);
	    client->addr = client->addr & I2C_MASK_FLAG & ~I2C_DMA_FLAG;
		//ret = i2c_master_send_fts(client, (u8 *)(uintptr_t)tpd_i2c_dma_pa, writelen);
	    //client->ext_flag = client->ext_flag & (~I2C_DMA_FLAG);
		return ret;
	}
	return 1;
}

#else

int fts_i2c_read(struct i2c_client *client, char *writebuf, int writelen, char *readbuf, int readlen)
{
	int ret = -1;

	mutex_lock(&i2c_rw_access);

	if(readlen > 0)
	{
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
				dev_err(&client->dev, "%s: i2c read error.\n", __func__);
		} else {
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
	}

	mutex_unlock(&i2c_rw_access);

	return ret;
}

int fts_i2c_write(struct i2c_client *client, char *writebuf, int writelen)
{
	int ret = -1;

	struct i2c_msg msgs[] = {
		{
			 .addr = client->addr,
			 .flags = 0,
			 .len = writelen,
			 .buf = writebuf,
		 },
	};
	mutex_lock(&i2c_rw_access);	
	if(writelen > 0)
	{
		ret = i2c_transfer(client->adapter, msgs, 1);
		if (ret < 0)
			dev_err(&client->dev, "%s: i2c write error.\n", __func__);
	}

	mutex_unlock(&i2c_rw_access);

	return ret;
}

#endif
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

static void fts_release_all_finger ( void )
{
	unsigned int finger_count=0;

#ifndef MT_PROTOCOL_B
	input_mt_sync ( tpd->dev );
#else
	for(finger_count = 0; finger_count < TPD_SUPPORT_POINTS; finger_count++)
	{
		input_mt_slot( tpd->dev, finger_count);
		input_mt_report_slot_state( tpd->dev, MT_TOOL_FINGER, false);
	}
	input_report_key(tpd->dev, BTN_TOUCH, 0);
#endif
	input_sync ( tpd->dev );
}

static int touch_event_handler(void *unused)
{
	int i = 0;
	#if FTS_GESTRUE_EN	//modify for zal1519 20160506
	int ret = 0;
	u8 state = 0;
	#endif
	
	#ifdef TPD_PROXIMITY
		int error,err;
		struct hwm_sensor_data sensor_data;
		u8 proximity_status;
		int ret = 0;
		u8 state = 0;
	#endif
	
	struct touch_info cinfo, pinfo, finfo;
//	struct sched_param param = { .sched_priority = RTPM_PRIO_TPD };
	struct sched_param param = { .sched_priority = 4};//zhangwenkang.wt 20161011 modify
	if (tpd_dts_data.use_tpd_button) {
		memset(&finfo, 0, sizeof(struct touch_info));
		for (i = 0; i < TPD_SUPPORT_POINTS; i++)
			finfo.p[i] = 1;
	}

	sched_setscheduler(current, SCHED_RR, &param);

	do {
		/*enable_irq(touch_irq);*/
		set_current_state(TASK_INTERRUPTIBLE);
		wait_event_interruptible(waiter, tpd_flag != 0);

		tpd_flag = 0;

		set_current_state(TASK_RUNNING);

		#if FT_ESD_PROTECT
		if(fts_esd_test)
		{
			fts_esd_test = 0;
			printk("[FTS][FAE]fts_esd_test modify\n");
			continue;
		}
#endif
		#if FTS_GESTRUE_EN	//modify for zal1519 20160506
			ret = fts_read_reg(fts_i2c_client, 0xd0,&state);		
			if (ret<0)
			{
				printk("[FTS][HQ] read value fail");
				//return ret;
			}
				printk("[FTS][HQ]fts_read_Gestruedata:state=%d <0:exit 1:enable>\n",state);
		     	if(state ==1)
		     	{
			        fts_read_Gestruedata();
			        continue;
		    	}
		 #endif
		 
		 	 #ifdef TPD_PROXIMITY

			 if (tpd_proximity_flag == 1)
			 {

				//i2c_smbus_read_i2c_block_data(fts_i2c_client, 0xB0, 1, &state);

				ret = fts_read_reg(fts_i2c_client, 0xB0,&state);
				if (ret<0) 
				{
					printk("[Focal][Touch] read value fail");
					//return ret;
				}
	           		TPD_PROXIMITY_DEBUG("proxi_fts 0xB0 state value is 1131 0x%02X\n", state);
				if(!(state&0x01))
				{
					tpd_enable_ps(1);
				}
				//i2c_smbus_read_i2c_block_data(fts_i2c_client, 0x01, 1, &proximity_status);
				ret = fts_read_reg(fts_i2c_client, 0x01,&proximity_status);
				if (ret<0) 
				{
					printk("[Focal][Touch] read value fail");
					//return ret;
				}
	            		TPD_PROXIMITY_DEBUG("proxi_fts 0x01 value is 1139 0x%02X\n", proximity_status);
				if (proximity_status == 0xC0)
				{
					tpd_proximity_detect = 0;	
				}
				else if(proximity_status == 0xE0)
				{
					tpd_proximity_detect = 10;// far wary
				}

				TPD_PROXIMITY_DEBUG("tpd_proximity_detect 1149 = %d\n", tpd_proximity_detect);
				if ((error = tpd_read_ps()))
				{
					TPD_PROXIMITY_DMESG("proxi_fts read ps data 1156: %d\n", error);	
				}
				sensor_data.values[0] = tpd_get_ps_value();
				sensor_data.value_divide = 1;
				sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
				if ((err = hwmsen_get_interrupt_data(ID_PROXIMITY, &sensor_data)))
				{
					TPD_PROXIMITY_DMESG(" proxi_5206 call hwmsen_get_interrupt_data failed= %d\n", err);	
				}
                           
			}  

		#endif


		if (tpd_touchinfo(&cinfo, &pinfo)) {

			if(cinfo.count > 0)
				input_report_key(tpd->dev, BTN_TOUCH, 1);

			for (i = 0; i < cinfo.eventnum; i++)
			{
				if(cinfo.p[i]==0 ||cinfo.p[i]==2)	
					tpd_down(cinfo.x[i], cinfo.y[i], i + 1, cinfo.id[i]);

				else if (cinfo.p[i] == 1)
					tpd_up(cinfo.x[i], cinfo.y[i],cinfo.id[i]);
			}
			
			if(cinfo.count <= 0)
			{
#ifdef MT_PROTOCOL_B
				fts_release_all_finger();
#else
				tpd_up(0,0,0);
#endif
			}
			input_sync(tpd->dev);
		}	
	} while (!kthread_should_stop());

	TPD_DEBUG("touch_event_handler exit\n");

	return 0;
}

static int tpd_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	strcpy(info->type, TPD_DEVICE);

	return 0;
}

static irqreturn_t tpd_eint_interrupt_handler(int irq, void *dev_id)
{
	TPD_DEBUG("TPD interrupt has been triggered\n");
	tpd_flag = 1;
	#if FT_ESD_PROTECT
	count_irq ++;
	#endif
	wake_up_interruptible(&waiter);
	return IRQ_HANDLED;
}
static int tpd_irq_registration(void)
{
	struct device_node *node = NULL;
	int ret = 0;
	u32 ints[2] = {0,0};

	node = of_find_matching_node(node, touch_of_match);
	if (node) {
		/*touch_irq = gpio_to_irq(tpd_int_gpio_number);*/
		of_property_read_u32_array(node,"debounce", ints, ARRAY_SIZE(ints));

//wwm start//
		TPD_DMESG("debounce:ints[0]=%d,ints[1]=%d", ints[0], ints[1]);
//wwm end//

		gpio_set_debounce(ints[0], ints[1]);

		touch_irq = irq_of_parse_and_map(node, 0);

//wwm start//
		TPD_DMESG("touch_irq=%d", touch_irq);
//wwm end//

		ret = request_irq(touch_irq, tpd_eint_interrupt_handler,
					IRQF_TRIGGER_FALLING, "TOUCH_PANEL-eint", NULL);
			if (ret > 0)
				TPD_DMESG("tpd request_irq IRQ LINE NOT AVAILABLE!.");
	} else {
		TPD_DMESG("[%s] tpd request_irq can not find touch eint device node!.", __func__);
	}
	return 0;
}
#if 0
int hidi2c_to_stdi2c(struct i2c_client * client)
{
	u8 auc_i2c_write_buf[5] = {0};
	int bRet = 0;

	auc_i2c_write_buf[0] = 0xeb;
	auc_i2c_write_buf[1] = 0xaa;
	auc_i2c_write_buf[2] = 0x09;

	fts_i2c_write(client, auc_i2c_write_buf, 3);

	msleep(10);

	auc_i2c_write_buf[0] = auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = 0;

	fts_i2c_read(client, auc_i2c_write_buf, 0, auc_i2c_write_buf, 3);

	if(0xeb==auc_i2c_write_buf[0] && 0xaa==auc_i2c_write_buf[1] && 0x08==auc_i2c_write_buf[2])
	{
		bRet = 1;
	}
	else
		bRet = 0;

	return bRet;

}
#endif

extern unsigned char ft5x46_ctpm_InkId_get_from_boot(  struct i2c_client *client );

//modify for zal1519 20160506

#if FTS_GESTRUE_EN	//modify for zal1519 20160506
static struct attribute_group fts_gesture_attribute_group;
#endif

static unsigned char tpd_read_fw_ver(struct i2c_client *client)
{
	unsigned char ver;

	i2c_smbus_read_i2c_block_data(client, 0xA6, 1, &ver);

	return ver;
}
 
static int tpd_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int retval = TPD_OK;
	//u8 report_rate = 0;
	//int reset_count = 0,InkId;
	int reset_count = 0;
	char data;
	int err = 0;
	unsigned char tp_fm_ver;
	
	#ifdef TPD_PROXIMITY
		int error;
		struct hwmsen_object obj_ps;
	#endif

	i2c_client = client;
	fts_i2c_client = client;
	
	#ifdef FTS_MCAP_TEST
	g_focalclient = client;
	#endif 
	fts_input_dev=tpd->dev;

	//printk("wwm:i2c_client->addr=0x%02x\n", i2c_client->addr);

	//Modify,@wangdongbo.wt_for_a9_tp
        if(i2c_client->addr != 0x38)
        {
                i2c_client->addr = 0x38;
        }

	of_get_ft5x0x_platform_data(&client->dev);
	/* configure the gpio pins */

	TPD_DMESG("mtk_tpd: tpd_probe ft5x0x\n");

//wwm start//
#if 0
	retval = regulator_enable(tpd->reg);

	if (retval != 0)
		TPD_DMESG("Failed to enable reg-vgp6: %d\n", retval);
#endif
//wwm end//

	/* set INT mode */

	tpd_gpio_as_int(tpd_int_gpio_number);


	msleep(10);//100 sven
	#ifdef CONFIG_MTK_I2C_EXTENSION_FOCAL
	
	msg_dma_alloct();
	#endif

#ifdef CONFIG_FT_AUTO_UPGRADE_SUPPORT

    if (NULL == tpd_i2c_dma_va)
    {
        tpd->dev->dev.coherent_dma_mask = DMA_BIT_MASK(32);
        tpd_i2c_dma_va = (u8 *)dma_alloc_coherent(&tpd->dev->dev, 250, &tpd_i2c_dma_pa, GFP_KERNEL);
    }
    if (!tpd_i2c_dma_va)
		TPD_DMESG("TPD dma_alloc_coherent error!\n");
	else
		TPD_DMESG("TPD dma_alloc_coherent success!\n");
#endif

#if FTS_GESTRUE_EN	//modify for zal1519 20160506
		wake_lock_init(&gesture_chrg_lock, WAKE_LOCK_SUSPEND, "gesture_wake_lock");
		fts_Gesture_init(tpd->dev);
#endif


reset_proc:
	/* Reset CTP */
	tpd_gpio_output(tpd_rst_gpio_number, 0);
	msleep(20);
	tpd_gpio_output(tpd_rst_gpio_number, 1);
	msleep(200);//400 sven
	
	err = fts_read_reg(i2c_client, 0x00, &data);

	TPD_DMESG("fts_i2c:err %d,data:%d\n", err,data);
	if(err< 0 || data!=0)// reg0 data running state is 0; other state is not 0
	{
		TPD_DMESG("I2C transfer error, line: %d\n", __LINE__);
#ifdef TPD_RESET_ISSUE_WORKAROUND
		if ( ++reset_count < TPD_MAX_RESET_COUNT )
		{
			goto reset_proc;
		}
#endif

//wwm start//
#if 1
		retval	= regulator_disable(tpd->reg); //disable regulator
		if(retval)
		{
			printk("focaltech tpd_probe regulator_disable() failed!\n");
		}

		regulator_put(tpd->reg);
#endif
//wwm end//


#if FTS_GESTRUE_EN	//wwm add//
		//wake_unlock(&gesture_chrg_lock);
	wake_lock_destroy(&gesture_chrg_lock);
#endif//wwm add//
		gpio_free(tpd_rst_gpio_number);
		gpio_free(tpd_int_gpio_number);
		return -1;
	}
	tpd_load_status = 1;
	tpd_irq_registration();

	#ifdef SYSFS_DEBUG
                fts_create_sysfs(fts_i2c_client);
	#endif
	//hidi2c_to_stdi2c(fts_i2c_client);
	fts_get_upgrade_array();
	#ifdef FTS_CTL_IIC
		 if (fts_rw_iic_drv_init(fts_i2c_client) < 0)
			 dev_err(&client->dev, "%s:[FTS] create fts control iic driver failed\n", __func__);
	#endif

	#ifdef FTS_APK_DEBUG
		fts_create_apk_debug_channel(fts_i2c_client);//include creat proc//*****ITO proc
	#endif

	#if FTS_GESTRUE_EN	//modify for zal1519 20160506
		mx_tsp=root_device_register("mx_tsp");
		err = sysfs_create_group(&mx_tsp->kobj,&fts_gesture_attribute_group);
	if (err < 0)
	{
	   printk("unable to create gesture attribute file\n");
	}
	#endif

	
	#if 0
	/* Reset CTP */

	tpd_gpio_output(tpd_rst_gpio_number, 0);
	msleep(20);
	tpd_gpio_output(tpd_rst_gpio_number, 1);
	msleep(400);
	#endif
	
	//Modify,@wangdongbo_close_upgrade_function
	#ifdef TPD_AUTO_UPGRADE
	
		printk("********************Enter CTP Auto Upgrade********************\n");//CTP Auto Upgrade**********
		is_update = true;
		fts_ctpm_auto_upgrade(fts_i2c_client);
		is_update = false;
	#endif

	//Modify,@wangdongbo
	//InkId = ft5x46_ctpm_InkId_get_from_boot(i2c_client);
	//printk("[FTS] 0x11=black, 0x12 = white, inkids =0x%x\n",InkId);

	#if 0
	/* Reset CTP */

	tpd_gpio_output(tpd_rst_gpio_number, 0);
	msleep(20);
	tpd_gpio_output(tpd_rst_gpio_number, 1);
	msleep(400);
	#endif
/*#ifdef CONFIG_FT_AUTO_UPGRADE_SUPPORT
	tpd_auto_upgrade(client);
#endif*/
	/* Set report rate 80Hz */   //********************************************limit report rate 80Hz********************
	/*report_rate = 0x8;
	if ((fts_write_reg(i2c_client, 0x88, report_rate)) < 0) {
		if ((fts_write_reg(i2c_client, 0x88, report_rate)) < 0)
			TPD_DMESG("I2C write report rate error, line: %d\n", __LINE__);
	}
	*/
	/* tpd_load_status = 1; */
#ifdef MT_PROTOCOL_B
	input_mt_init_slots(tpd->dev, TPD_SUPPORT_POINTS, INPUT_MT_DIRECT);
#endif


	thread_tpd = kthread_run(touch_event_handler, 0, TPD_DEVICE);
	if (IS_ERR(thread_tpd)) {
		retval = PTR_ERR(thread_tpd);
		TPD_DMESG(TPD_DEVICE " failed to create kernel thread_tpd: %d\n", retval);
	}

	TPD_DMESG("Touch Panel Device Probe %s\n", (retval < TPD_OK) ? "FAIL" : "PASS");

#ifdef TIMER_DEBUG
	init_test_timer();
#endif
	tp_fm_ver  = tpd_read_fw_ver(client);
	TPD_DMESG("[TPD] host_fm_ver = 0x%x\n",tp_fm_ver);

//Modify,@hubing1 2016.7.25
#ifdef LCT_ADD_TP_LOCKDOWN_INFO
		ft5x46_ctpm_LockDownInfo_get_from_boot(i2c_client, tp_lockdown_info);
		printk("tpd_probe, ft5x46_ctpm_LockDownInfo_get_from_boot, tp_lockdown_info=%s\n", tp_lockdown_info);
		ctp_lockdown_status_proc = proc_create(CTP_PROC_LOCKDOWN_FILE, 0644, NULL, &ctp_lockdown_proc_fops);
		if (ctp_lockdown_status_proc == NULL)
		{
			TPD_DMESG("tpd, create_proc_entry ctp_lockdown_status_proc failed\n");
		}
#endif	



	{
		u8 ver;

		fts_read_reg(client, 0xA6, &ver);

		TPD_DMESG(TPD_DEVICE " fts_read_reg version : %d\n", ver);

		//g_hq_ctp_module_fw_version = ver;
		tpd->dev->id.version = ver;
	}
#if FT_ESD_PROTECT
	//gtp_esd_check_work=NULL;
	INIT_DELAYED_WORK(&gtp_esd_check_work, fts_esd_check_func);
	schedule_delayed_work(&gtp_esd_check_work,2*HZ);
	//gtp_esd_check_workqueue = create_workqueue("gtp_esd_check");
//	queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work, TPD_ESD_CHECK_CIRCLE);
#endif
	#ifdef TPD_PROXIMITY
		{
			obj_ps.polling = 0; //0--interrupt mode;1--polling mode;
			obj_ps.sensor_operate = tpd_ps_operate;
			if ((error = hwmsen_attach(ID_PROXIMITY, &obj_ps)))
			{
				TPD_DEBUG("hwmsen attach fail, return:%d.", error);
			}
			misc_register(&tp_ps_device); //add tp emulate ps test for factory
		}
	#endif

#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
	int ret;

	ret = get_md32_semaphore(SEMAPHORE_TOUCH);
	if (ret < 0)
		pr_err("[TOUCH] HW semaphore reqiure timeout\n");
#endif

#ifdef USB_CHARGE_DETECT  //add by wangyang
		if(ctp_is_probe == 0)
		{
			 i2c_smbus_write_i2c_block_data(fts_i2c_client, 0x8B, 1, &b_usb_plugin);
			ctp_is_probe =1;
		}
#endif
#ifdef WT_CTP_OPEN_SHORT_TEST
	create_ctp_proc();
#endif
//+bug 258645 zhangwenkang.wt 20170419, add headphone detection function
#ifdef CONFIG_ENABLE_HEADPHONES_DETECTION
	headphones_init();
#endif
//-bug 258645 zhangwenkang.wt 20170419, add headphone detection function
#ifdef WT_CTP_INFORMATION
        sprintf(tp_string_version,"BOEN,FT6436U,V%x",  tp_fm_ver);
	hardwareinfo_set_prop(HARDWARE_TP, tp_string_version);
#endif

	return 0;
}

static int tpd_remove(struct i2c_client *client)
{
	TPD_DEBUG("TPD removed\n");
#ifdef CONFIG_CUST_FTS_APK_DEBUG
	//ft_rw_iic_drv_exit();
#endif

#ifdef CONFIG_FT_AUTO_UPGRADE_SUPPORT
	if (tpd_i2c_dma_va) {
		dma_free_coherent(NULL, 4096, tpd_i2c_dma_va, tpd_i2c_dma_pa);
		tpd_i2c_dma_va = NULL;
		tpd_i2c_dma_pa = 0;
	}
#endif
#ifdef WT_CTP_OPEN_SHORT_TEST
	remove_ctp_proc();
#endif

	#ifdef CONFIG_MTK_I2C_EXTENSION_FOCAL
		msg_dma_release();
	#endif

#if FTS_GESTRUE_EN	//wwm add//
	wake_lock_destroy(&gesture_chrg_lock);
#endif			//wwm add//
	gpio_free(tpd_rst_gpio_number);
	gpio_free(tpd_int_gpio_number);
	return 0;
}
#if FT_ESD_PROTECT
void esd_switch(s32 on)
{
	if(on == 1)// switch on esd 
	{
		schedule_delayed_work(&gtp_esd_check_work,2*HZ);
		printk("[FTS]apk_debug_flag=%d\n",apk_debug_flag);
		printk("[FTS]esd_mode_on\n");
	}
	else// switch off esd
	{
		cancel_delayed_work(&gtp_esd_check_work);
		printk("[FTS]apk_debug_flag=%d\n",apk_debug_flag);
		printk("[FTS]esd_mode_off\n");
	}
}
#endif

#if FT_ESD_PROTECT
 /************************************************************************
 * Name: force_reset_guitar
 * Brief: reset
 * Input: no
 * Output: no
 * Return: 0
 ***********************************************************************/
 static void force_reset_guitar(void)
 {
 

   int retval = TPD_OK;
   //u8 state= 0;
  // u8 data = 0;
	fts_esd_test=1;
	printk("[focal]guitar fts_esd_test =1\n");	
	disable_irq(touch_irq);

	if(apk_debug_flag||tpd_halt) //||factory_mode_flag
	{
		printk("focal--force_reset_guitar,return queue\n");
		return;
	}
	//fts_read_reg(fts_i2c_client, 0xd0,&state);
	//printk("foacal guitar state=%d\n",state);

	/*ret =  fts_read_reg(i2c_client, 0x00, &data);
	printk("focal--fts_esd_check_func-0x00:%x\n", factory_check);
	if(factory_check==0x40)*/
 		/* Reset CTP */
    retval = regulator_disable(tpd->reg);
	if (retval != 0)
		//TPD_DMESG("Failed to disable reg-vgp6: %d\n", retval);
		printk("Failed to disable reg-vgp6: %d\n", retval);
    msleep(400);

	tpd_gpio_output(tpd_rst_gpio_number, 0);
	msleep(5);

	
	retval = regulator_enable(tpd->reg);
	if (retval != 0)
	TPD_DMESG("Failed to enable reg-vgp6: %d\n", retval);
	msleep(5);
	tpd_gpio_output(tpd_rst_gpio_number, 1);
    msleep(200);
 #ifdef TPD_PROXIMITY
	if (FT_PROXIMITY_ENABLE == tpd_proximity_flag) 
	{
		tpd_enable_ps(FT_PROXIMITY_ENABLE);
	}
#endif

	enable_irq(touch_irq);
	
	//fts_esd_test=0;

	//tpd_up(0,0);
	//input_sync(tpd->dev);
	
	{
		fts_write_reg(i2c_client, 0x95,fts_iic_cnt);
		printk("[FTS]fts_iic_cnt =%x\n",fts_iic_cnt);
	}
	
	printk("focal--force_reset_guitar\n");

	
	/*
	tpd_gpio_output(tpd_rst_gpio_number, 0);
	msleep(20);
	tpd_gpio_output(tpd_rst_gpio_number, 1);
	msleep(400);

	printk("[FTS] fts ic reset\n");
	*/
}

//int apk_debug_flag = 0 ; //0 for no apk upgrade, 1 for apk upgrade
#define A3_REG_VALUE	0x54
static u8 g_old_91_Reg_Value = 0x00;
static u8 g_first_read_91 = 0x01;
static u8 g_91value_same_count = 0;
#define RESET_91_REGVALUE_SAMECOUNT	5

 
 void fts_esd_check_func(struct work_struct *work)
{
	//return;
	
	static unsigned long esd_check_circle = TPD_ESD_CHECK_CIRCLE;
	int i;
	int ret = -1;
	u8 data;//,factory_check;
#if _IRQ_CNT_COMPARE
	u8 flag_error = 0;
#endif
	int reset_flag = 0;
	//u8 check_91_reg_flag = 0;

	TPD_DMESG("MEIZU_ESDTEST\n");

	/*ret =  fts_read_reg(i2c_client, 0x00, &factory_check);
	printk("focal--fts_esd_check_func-0x00:%x\n", factory_check);
	if(factory_check==0x40)
		//factory_mode_flag = 1;*/

	if(apk_debug_flag||tpd_halt)//||factory_mode_flag
	{
		TPD_DEBUG("focal--fts_esd_check_func,return queue\n");
		
		//queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work, esd_check_circle);
			//schedule_delayed_work(&gtp_esd_check_work,2*HZ);
		return;
	}
	
	ret =  fts_read_reg(i2c_client, 0x95, &data);
	if(data!=0)
		fts_iic_cnt = data;
	printk("focal--Refresh base cnt-0x95:%x\n", fts_iic_cnt);

    run_check_91_register = 0;
	for (i = 0; i < 3; i++)
	{
		//ret = fts_i2c_smbus_read_i2c_block_data(i2c_client_ts, 0xA3, 1, &data);
		ret =  fts_read_reg(i2c_client, 0xA3, &data);
		printk("focal--fts_esd_check_func-0xA3:%x\n", data);
		if (ret==1 && A3_REG_VALUE==data) {
			break;
		}
	}

	if (i >= 3) {
		force_reset_guitar();
		printk("focal--tpd reset. i >= 3  ret = %d  A3_Reg_Value = 0x%02x\n ", ret, data);
		reset_flag = 1;
		goto FOCAL_RESET_A3_REGISTER;
	}
#if _IRQ_CNT_COMPARE
	//ret = fts_i2c_smbus_read_i2c_block_data(i2c_client_ts, 0x8f, 1, &data);
	ret =  fts_read_reg(i2c_client, 0x8f, &data);
	printk("focal--fts_esd_check_func-0x8F:%d, count_irq is %d\n", data, count_irq);

	flag_error = 0;
	if((count_irq - data) > 10) {
		if((data+200) > (count_irq+10) )
		{
			flag_error = 1;
		}
	}

	if((data - count_irq ) > 10) {
		flag_error = 1;
	}

	if(1 == flag_error) {	 
		printk("focal--tpd reset.1 == flag_error...data=%d  count_irq =%d\n ", data, count_irq);
		force_reset_guitar();
		reset_flag = 1;
		goto FOCAL_RESET_INT;
	}
#endif
	run_check_91_register = 1;
	//ret = fts_i2c_smbus_read_i2c_block_data(i2c_client_ts, 0x91, 1, &data);
	ret =  fts_read_reg(i2c_client, 0x91, &data);
	printk("focal 91 register value = 0x%02x,old value = 0x%02x\n",	data, g_old_91_Reg_Value);

	if(0x01 == g_first_read_91) {
		g_old_91_Reg_Value = data;
		g_first_read_91 = 0x00;
	} else {
	if(g_old_91_Reg_Value == data){
		g_91value_same_count++;
		printk("focal 91 register  value , g_91value_same_count=%d\n", g_91value_same_count);
		if(RESET_91_REGVALUE_SAMECOUNT == g_91value_same_count) {
			 force_reset_guitar();
			 printk("focal--tpd reset. g_91value_same_count = 5\n");
			 g_91value_same_count = 0;
			 reset_flag = 1;
		}

		//run_check_91_register = 1;
		esd_check_circle = TPD_ESD_CHECK_CIRCLE / 2;
		g_old_91_Reg_Value = data;
	} else {
		g_old_91_Reg_Value = data;
		g_91value_same_count = 0;
		//run_check_91_register = 0;
		esd_check_circle = TPD_ESD_CHECK_CIRCLE;
		}
	}
#if _IRQ_CNT_COMPARE
	FOCAL_RESET_INT:
#endif
	FOCAL_RESET_A3_REGISTER:
	count_irq=0;
	data=0;
	//i2c_smbus_write_i2c_block_data(i2c_client_ts, 0x8F, 1, &data);
	ret = fts_write_reg(i2c_client, 0x8F, 0);
	
	if (ret<0) 
	{
		printk("[Focal][Touch] write value fail");
	}

	if(0 == run_check_91_register)
		g_91value_same_count = 0;
	
	#ifdef TPD_PROXIMITY
	if( (1 == reset_flag) && ( FT_PROXIMITY_ENABLE == tpd_proximity_flag) )
	{
		if((tpd_enable_ps(FT_PROXIMITY_ENABLE) != 0))
		{
			APS_ERR("enable ps fail\n"); 
			return -1;
		}
	}
	#endif
if(!tpd_halt)
	schedule_delayed_work(&gtp_esd_check_work,2*HZ);
else
	printk("tpd_halt\n");	

	return;
}
#endif

static int tpd_local_init(void)
{
//wwm start//
#if 1
	int retval;
#endif
//wwm end//

	TPD_DMESG("Focaltech FT5x0x I2C Touchscreen Driver...\n");

//wwm start//
#if !defined CONFIG_MTK_LEGACY
	printk("Device Tree get regulator!\n");
	tpd->reg = regulator_get(tpd->tpd_dev, "vtouch");
	retval = regulator_set_voltage(tpd->reg, 2800000, 2800000);	/*set 2.8v*/
	if (retval) {
		TPD_DMESG("regulator_set_voltage(%d) failed!\n", retval);
		return -1;
	}
	retval = regulator_enable(tpd->reg);	/*enable regulator*/
	if (retval)
		TPD_DMESG("regulator_enable() failed!\n");
#endif
#ifdef TPD_POWER_SOURCE_CUSTOM
#ifdef CONFIG_ARCH_MT6580
        printk("Device Power on by PMIC\n");
	tpd->reg = regulator_get(tpd->tpd_dev, "VGP1");	/*get pointer to regulator structure*/
	if (IS_ERR(tpd->reg))
		TPD_DMESG("regulator_get() failed!\n");
#endif
#endif

#if 0
	tpd->reg = regulator_get(tpd->tpd_dev, "vtouch");
	retval = regulator_set_voltage(tpd->reg, 2800000, 2800000);
	if (retval != 0) {
		TPD_DMESG("Failed to set reg-vgp6 voltage: %d\n", retval);
		return -1;
	}
#endif
//wwm end//

	if (i2c_add_driver(&tpd_i2c_driver) != 0) {
		TPD_DMESG("unable to add i2c driver.\n");
		return -1;
	}
     /* tpd_load_status = 1; */
	if (tpd_dts_data.use_tpd_button) {
		tpd_button_setting(tpd_dts_data.tpd_key_num, tpd_dts_data.tpd_key_local,
		tpd_dts_data.tpd_key_dim_local);
	}
#ifndef MT_PROTOCOL_B //20160421
	input_set_abs_params(tpd->dev, ABS_MT_TRACKING_ID, 0, (CFG_MAX_TOUCH_POINTS-1), 0, 0);
#endif
#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))
	TPD_DO_WARP = 1;
	memcpy(tpd_wb_start, tpd_wb_start_local, TPD_WARP_CNT * 4);
	memcpy(tpd_wb_end, tpd_wb_start_local, TPD_WARP_CNT * 4);
#endif

#if (defined(CONFIG_TPD_HAVE_CALIBRATION) && !defined(CONFIG_TPD_CUSTOM_CALIBRATION))

	memcpy(tpd_calmat, tpd_def_calmat_local_factory, 8 * 4);
	memcpy(tpd_def_calmat, tpd_def_calmat_local_factory, 8 * 4);

	memcpy(tpd_calmat, tpd_def_calmat_local_normal, 8 * 4);
	memcpy(tpd_def_calmat, tpd_def_calmat_local_normal, 8 * 4);

#endif

	TPD_DMESG("end %s, %d\n", __func__, __LINE__);
	tpd_type_cap = 1;

	return 0;
}

#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
static s8 ftp_enter_doze(struct i2c_client *client)
{
	s8 ret = -1;
	s8 retry = 0;
	char gestrue_on = 0x01;
	char gestrue_data;
	int i;

	/* TPD_DEBUG("Entering doze mode..."); */
	pr_alert("Entering doze mode...");

	/* Enter gestrue recognition mode */
	ret = fts_write_reg(i2c_client, FT_GESTRUE_MODE_SWITCH_REG, gestrue_on);
	if (ret < 0) {
		/* TPD_DEBUG("Failed to enter Doze %d", retry); */
		pr_alert("Failed to enter Doze %d", retry);
		return ret;
	}
	msleep(30);

	for (i = 0; i < 10; i++) {
		fts_read_reg(i2c_client, FT_GESTRUE_MODE_SWITCH_REG, &gestrue_data);
		if (gestrue_data == 0x01) {
			doze_status = DOZE_ENABLED;
			/* TPD_DEBUG("FTP has been working in doze mode!"); */
			pr_alert("FTP has been working in doze mode!");
			break;
		}
		msleep(20);
		fts_write_reg(i2c_client, FT_GESTRUE_MODE_SWITCH_REG, gestrue_on);

	}

	return ret;
}
#endif

static void tpd_resume(struct device *h)
{
//wwm start//
#if 1
	int retval = TPD_OK;
#endif
//wwm end//

	TPD_DEBUG("TPD wake up\n");
	  #ifdef TPD_PROXIMITY	
		if (tpd_proximity_flag == 1)
		{
			if(tpd_proximity_flag_one == 1)
			{
				tpd_proximity_flag_one = 0;
				tpd_halt = 0;	
				TPD_DMESG(TPD_DEVICE " tpd_proximity_flag_one \n"); 
				return;
			}
		}
	#endif	

#if FT_ESD_PROTECT
	if(((fts_gesture_three_byte_one == 0) && ((fts_gesture_three_byte_two == 0) \
		&& (fts_gesture_three_byte_three == 0) && (fts_gesture_three_byte_four == 0)))){
	}else{
	//if(1){
		retval = regulator_disable(tpd->reg);
		if (retval != 0)
		//TPD_DMESG("Failed to disable reg-vgp6: %d\n", retval);
		printk("Failed to disable reg-vgp6: %d\n", retval);
		msleep(30);
	}
#endif

	tpd_gpio_output(tpd_rst_gpio_number, 0);
	msleep(5);

	retval = regulator_enable(tpd->reg);
	if (retval != 0)
	TPD_DMESG("Failed to enable reg-vgp6: %d\n", retval);
	msleep(5);
	tpd_gpio_output(tpd_rst_gpio_number, 1);
	msleep(400);


//wwm start//
#if 0
	retval = regulator_enable(tpd->reg);
	if (retval != 0)
		TPD_DMESG("Failed to enable reg-vgp6: %d\n", retval);

//wwm end//

	msleep(5);
	tpd_gpio_output(tpd_rst_gpio_number, 0);
	msleep(20);
	tpd_gpio_output(tpd_rst_gpio_number, 1);
	msleep(400);
#endif

#if FTS_GESTRUE_EN	//wwm add//
	gesture_onoff_fts = 0;

	wake_unlock(&gesture_chrg_lock);
#endif			//wwm add//
	
	fts_release_all_finger();
	tpd_halt = 0;
			
#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
	int ret;

	if (tpd_scp_doze_en) {
		ret = get_md32_semaphore(SEMAPHORE_TOUCH);
		if (ret < 0) {
			TPD_DEBUG("[TOUCH] HW semaphore reqiure timeout\n");
		} else {
			Touch_IPI_Packet ipi_pkt = {.cmd = IPI_COMMAND_AS_ENABLE_GESTURE, .param.data = 0};

			md32_ipi_send(IPI_TOUCH, &ipi_pkt, sizeof(ipi_pkt), 0);
		}
	}
#endif

#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
	doze_status = DOZE_DISABLED;
	/* tpd_halt = 0; */
	int data;

	data = 0x00;

	fts_write_reg(i2c_client, FT_GESTRUE_MODE_SWITCH_REG, data);
#else
	enable_irq(touch_irq);
#endif
//+bug 258645 zhangwenkang.wt 20170419, add headphone detection function
 #ifdef CONFIG_ENABLE_HEADPHONES_DETECTION
    if(bSuspendHeadphones)
    {
    	  DrvSetHeadphonesStateToFw(DrvGetHeadphonesStateFromSys());
         bSuspendHeadphones = 0;
	  hrtimer_start(&headphonesTimer, ktime_set(1, 0), HRTIMER_MODE_REL);
    }
#endif
//-bug 258645 zhangwenkang.wt 20170419, add headphone detection function
#ifdef USB_CHARGE_DETECT
	msleep(120);
	tpd_usb_plugin(b_usb_plugin);
#endif


#if FT_ESD_PROTECT
	count_irq = 0;
	schedule_delayed_work(&gtp_esd_check_work,2*HZ);
	fts_write_reg(i2c_client, 0x8c,1);
	
	//fts_read_reg(i2c_client, 0x8c, &state);
	//printk("[FTS]resume 0x8c=%d\n",state);
	
	//fts_esd_protection_resume();
#endif	
	//tpd_up(0,0);
	//input_sync(tpd->dev);

}

#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
void tpd_scp_wakeup_enable(bool en)
{
	tpd_scp_doze_en = en;
}

void tpd_enter_doze(void)
{

}
#endif

static void tpd_suspend(struct device *h)
{

//wwm start//
#if 1
	int retval = TPD_OK;
#endif
//wwm end//

	static char data = 0x3;

	#if FTS_GESTRUE_EN	//modify for zal1519 20160506
	int i = 0;
	u8 state = 0;
	#endif
	
	printk("[FTS]TPD enter sleep\n");
	tpd_halt = 1;
	fts_release_all_finger();
	

#if FT_ESD_PROTECT
	cancel_delayed_work_sync(&gtp_esd_check_work);	
	TPD_DMESG("suspend,esd_switch_off\n");
#endif
#ifdef TPD_PROXIMITY
	if (tpd_proximity_flag == 1)
	{
		tpd_proximity_flag_one = 1;	
		return;
	}
#endif
//+bug 258645 zhangwenkang.wt 20170419, add headphone detection function
#ifdef CONFIG_ENABLE_HEADPHONES_DETECTION
       cancel_work_sync(&headphonesWork);
	hrtimer_cancel(&headphonesTimer);
       bSuspendHeadphones = 1;
#endif
//-bug 258645 zhangwenkang.wt 20170419, add headphone detection function
	#if FTS_GESTRUE_EN		//modify for zal1519 20160506
	if(((fts_gesture_three_byte_one == 0) && ((fts_gesture_three_byte_two == 0) \
		&& (fts_gesture_three_byte_three == 0) && (fts_gesture_three_byte_four == 0)))){
	}else{
//	if(1){
	     	printk("[FTS] FTS_GESTRUE wakeup>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");			
			fts_write_reg(i2c_client, 0xd0, 0x01);
		  	fts_write_reg(i2c_client, 0xd1, 0xff);
			fts_write_reg(i2c_client, 0xd2, 0xff);
			fts_write_reg(i2c_client, 0xd5, 0xff);
		    fts_write_reg(i2c_client, 0xd6, 0xff);		
			fts_write_reg(i2c_client, 0xd7, 0xff);
			fts_write_reg(i2c_client, 0xd8, 0xff);
			msleep(10);

			for(i = 0; i < 10; i++)
			{
				TPD_DMESG("tpd_suspend4 %d",i);
			  	fts_read_reg(i2c_client, 0xd0, &state);

				if(state == 1)
				{
				    gesture_onoff_fts = 1;
					printk("[FTS]TPD gesture write 0x01\n");
	        			return;
				}
				else
				{
					fts_write_reg(i2c_client, 0xd0, 0x01);
					fts_write_reg(i2c_client, 0xd1, 0xff);
		 			fts_write_reg(i2c_client, 0xd2, 0xff);
				    fts_write_reg(i2c_client, 0xd5, 0xff);
					fts_write_reg(i2c_client, 0xd6, 0xff);
					fts_write_reg(i2c_client, 0xd7, 0xff);
				  	fts_write_reg(i2c_client, 0xd8, 0xff);
					msleep(10);
			}
		}

		if(i >= 9)
		{
			printk("[FTS]TPD gesture write 0x01 to d0 fail \n");
			return;
		}
	}
	#endif

#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
	int sem_ret;

	tpd_enter_doze();

	int ret;
	char gestrue_data;
	char gestrue_cmd = 0x03;
	static int scp_init_flag;

	/* TPD_DEBUG("[tpd_scp_doze]:init=%d en=%d", scp_init_flag, tpd_scp_doze_en); */

	mutex_lock(&i2c_access);

	sem_ret = release_md32_semaphore(SEMAPHORE_TOUCH);

	if (scp_init_flag == 0) {
		Touch_IPI_Packet ipi_pkt;

		ipi_pkt.cmd = IPI_COMMAND_AS_CUST_PARAMETER;
		ipi_pkt.param.tcs.i2c_num = TPD_I2C_NUMBER;
		ipi_pkt.param.tcs.int_num = CUST_EINT_TOUCH_PANEL_NUM;
		ipi_pkt.param.tcs.io_int = tpd_int_gpio_number;
		ipi_pkt.param.tcs.io_rst = tpd_rst_gpio_number;

		TPD_DEBUG("[TOUCH]SEND CUST command :%d ", IPI_COMMAND_AS_CUST_PARAMETER);

		ret = md32_ipi_send(IPI_TOUCH, &ipi_pkt, sizeof(ipi_pkt), 0);
		if (ret < 0)
			TPD_DEBUG(" IPI cmd failed (%d)\n", ipi_pkt.cmd);

		msleep(20); /* delay added between continuous command */
		/* Workaround if suffer MD32 reset */
		/* scp_init_flag = 1; */
	}

	if (tpd_scp_doze_en) {
		TPD_DEBUG("[TOUCH]SEND ENABLE GES command :%d ", IPI_COMMAND_AS_ENABLE_GESTURE);
		ret = ftp_enter_doze(i2c_client);
		if (ret < 0) {
			TPD_DEBUG("FTP Enter Doze mode failed\n");
	  } else {
			int retry = 5;
	    {
				/* check doze mode */
				fts_read_reg(i2c_client, FT_GESTRUE_MODE_SWITCH_REG, &gestrue_data);
				TPD_DEBUG("========================>0x%x", gestrue_data);
	    }

	    msleep(20);
			Touch_IPI_Packet ipi_pkt = {.cmd = IPI_COMMAND_AS_ENABLE_GESTURE, .param.data = 1};

			do {
				if (md32_ipi_send(IPI_TOUCH, &ipi_pkt, sizeof(ipi_pkt), 1) == DONE)
					break;
				msleep(20);
				TPD_DEBUG("==>retry=%d", retry);
			} while (retry--);

	    if (retry <= 0)
				TPD_DEBUG("############# md32_ipi_send failed retry=%d", retry);

			/*while(release_md32_semaphore(SEMAPHORE_TOUCH) <= 0) {
				//TPD_DEBUG("GTP release md32 sem failed\n");
				pr_alert("GTP release md32 sem failed\n");
			}*/

		}
		/* disable_irq(touch_irq); */
	}

	mutex_unlock(&i2c_access);
#else
	disable_irq(touch_irq);
	fts_write_reg(i2c_client, 0xA5, data);  /* TP enter sleep mode */

//wwm start//
#if 1
	retval = regulator_disable(tpd->reg);
	if (retval != 0)
		TPD_DMESG("Failed to disable reg-vgp6: %d\n", retval);
#endif
//wwm end//

#endif

}

static struct tpd_driver_t tpd_device_driver = {
	.tpd_device_name = "FT6436U",
	.tpd_local_init = tpd_local_init,
	.suspend = tpd_suspend,
	.resume = tpd_resume,
	.attrs = {
		.attr = ft5x0x_attrs,
		.num  = ARRAY_SIZE(ft5x0x_attrs),
	},
};

/* called when loaded into kernel */
static int __init tpd_driver_init(void)
{
	TPD_DMESG("MediaTek FT5x0x touch panel driver init\n");
	tpd_get_dts_info();
	if (tpd_driver_add(&tpd_device_driver) < 0)
		TPD_DMESG("add FT5x0x driver failed\n");

	return 0;
}

/* should never be called */
static void __exit tpd_driver_exit(void)
{
	TPD_DMESG("MediaTek FT5x0x touch panel driver exit\n");
	tpd_driver_remove(&tpd_device_driver);
	
#if FTS_GESTRUE_EN	//modify for zal1519 20160506
	sysfs_remove_group(&mx_tsp->kobj,&fts_gesture_attribute_group);
    root_device_unregister(mx_tsp);
#endif

}


#if FTS_GESTRUE_EN	//modify for zal1519 20160506
/**zal1519 gesture sys point init start**************************************************************************************/
static ssize_t gesture_data_store(struct device* dev, struct device_attribute *attr,
                                  const char *buf, size_t count)
{  
/*
	struct alsps_context *cxt = NULL;
	//int err =0;
	ALSPS_LOG("als_store_active buf=%s\n",buf);
	mutex_lock(&alsps_context_obj->alsps_op_mutex);
	cxt = alsps_context_obj;

    if (!strncmp(buf, "1", 1)) 
	{
      	als_enable_data(1);
    	} 
	else if (!strncmp(buf, "0", 1))
	{
       als_enable_data(0);
    	}
	else
	{
	  ALSPS_ERR(" alsps_store_active error !!\n");
	}
	mutex_unlock(&alsps_context_obj->alsps_op_mutex);
	ALSPS_LOG(" alsps_store_active done\n");
    return count;
 */
    return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t gesture_data_show(struct device* dev, 
                                 struct device_attribute *attr, char *buf) 
{
/*
	struct alsps_context *cxt = NULL;
	int div = 0;
	cxt = alsps_context_obj;
	div=cxt->als_data.vender_div;
	ALSPS_LOG("als vender_div value: %d\n", div);
	return snprintf(buf, PAGE_SIZE, "%d\n", div); 
*/
	printk("fts_gesture_data_show gesture_data=%x\n",fts_gesture_data);
	return snprintf(buf, PAGE_SIZE, "%u\n",fts_gesture_data);
}
/*----------------------------------------------------------------------------*/

static ssize_t gesture_control_node_store(struct device* dev, struct device_attribute *attr,
                                  const char *buf, size_t count)
{  
    u32 value=0,value1=0,value2=0;
    //sscanf(buf, "%x", &value);
    //fts_gesture_ctrl = value;
    //value1 = value&0x000000ff;
    //value2 = (value>>16)&0x000000ff;s
    
    int i = 0 ;
	char buff[4];
	
     //printk("yili: >> buf = %s\n",buf);
	/*
	the flyme upoper data flow:
	when gesture open,
	first data: x0x0		low bit -> hight bit
	end data  : 1010
	*/
	
	
	for(i=0;i<4;i++){
		buff[i]=*buf;
		buf++;
		}
    printk("[FTS] >> buff[0]= %d buff[1]= %d buff[2]= %d buff[3]= %d\n",buff[0],buff[1],buff[2],buff[3]);
	
	//sscanf(buf,"%d %d %d %d",&buff[0],&buff[1],&buff[2],&buff[3]);
  
	value1 = buff[0];
    value2 = buff[2];
   // printk("[FTS] >> value1= %d value2= %d\n",value1,value2);

    if(value2 == 1)
    {
    	fts_gesture_three_byte_one = value1;
		if(fts_gesture_three_byte_one ==1){
			if((fts_gesture_three_byte_two ==1)&&(fts_gesture_three_byte_three ==255)&&(fts_gesture_three_byte_four ==15))
				{fts_gesture_mask = 64;}
			else if((fts_gesture_three_byte_two ==0)&&(fts_gesture_three_byte_three ==0)&&(fts_gesture_three_byte_four ==0))
				{fts_gesture_mask = 0;}
			else{
			fts_gesture_mask = 128;}
			}
		else{
				fts_gesture_mask = 0;
			}	

    }
    else if(value2 == 2)
    {
    	fts_gesture_three_byte_two = value1;
		if(fts_gesture_three_byte_one ==1){
			if((fts_gesture_three_byte_two ==1)&&(fts_gesture_three_byte_three ==255)&&(fts_gesture_three_byte_four ==15))
				{fts_gesture_mask = 64;}
			else if((fts_gesture_three_byte_two ==0)&&(fts_gesture_three_byte_three ==0)&&(fts_gesture_three_byte_four ==0))
				{fts_gesture_mask = 0;}
			else{
			fts_gesture_mask = 128;}
			}
		else{
				fts_gesture_mask = 0;
			}		
    }
    else if(value2 == 3)
    {
    	fts_gesture_three_byte_three = value1;
		if(fts_gesture_three_byte_one ==1){
			if((fts_gesture_three_byte_two ==1)&&(fts_gesture_three_byte_three ==255)&&(fts_gesture_three_byte_four ==15))
				{fts_gesture_mask = 64;}
			else if((fts_gesture_three_byte_two ==0)&&(fts_gesture_three_byte_three ==0)&&(fts_gesture_three_byte_four ==0))
				{fts_gesture_mask = 0;}
			else{
			fts_gesture_mask = 128;}
			}
		else{
				fts_gesture_mask = 0;
			}		
    }
    else if(value2 == 4)
    {
    	fts_gesture_three_byte_four = value1;
		if(fts_gesture_three_byte_one ==1){
			if((fts_gesture_three_byte_two ==1)&&(fts_gesture_three_byte_three ==255)&&(fts_gesture_three_byte_four ==15))
				{fts_gesture_mask = 64;}
			else if((fts_gesture_three_byte_two ==0)&&(fts_gesture_three_byte_three ==0)&&(fts_gesture_three_byte_four ==0))
				{fts_gesture_mask = 0;}
			else{
			fts_gesture_mask = 128;}
			}
		else{
				fts_gesture_mask = 0;
			}		
    }
	else
	{
		if(fts_gesture_three_byte_one ==1){
			if((fts_gesture_three_byte_two ==1)&&(fts_gesture_three_byte_three ==255)&&(fts_gesture_three_byte_four ==15))
				{fts_gesture_mask = 64;}
			else if((fts_gesture_three_byte_two ==0)&&(fts_gesture_three_byte_three ==0)&&(fts_gesture_three_byte_four ==0))
				{fts_gesture_mask = 0;}
			else{
			fts_gesture_mask = 128;}
			}
		else{
				fts_gesture_mask = 0;
			}	
	}
	fts_gesture_ctrl = fts_gesture_mask;
    printk("[FTS]value = %x, value1 = %x, value2 = %x fts_gesture_ctrl =fts_gesture_mask = %x\n",value,value1,value2,fts_gesture_mask);
    printk("[FTS]fts_gesture_three_byte_one=%x,fts_gesture_three_byte_two=%x,fts_gesture_three_byte_three=%x,gesture_three_byte_four=%x\n",fts_gesture_three_byte_one,fts_gesture_three_byte_two,fts_gesture_three_byte_three,fts_gesture_three_byte_four);
    return count;
}

static ssize_t gesture_control_node_show(struct device* dev, 
                                 struct device_attribute *attr, char *buf) 
{
	return sprintf(buf,"%x\n",fts_gesture_ctrl);
}


static DEVICE_ATTR(gesture_data,     		S_IWUSR | S_IRUGO, gesture_data_show, gesture_data_store);
static DEVICE_ATTR(gesture_control,      	S_IWUSR | S_IRUGO, gesture_control_node_show,  gesture_control_node_store);

static struct attribute *gesture_attributes[] = {
	//&dev_attr_gesture_control_node.attr,
	&dev_attr_gesture_data.attr,
	&dev_attr_gesture_control.attr,
	NULL
};

static struct attribute_group fts_gesture_attribute_group = {
	.attrs = gesture_attributes
};
/**zal1519 gesture sys point init end**************************************************************************************************/
#endif


module_init(tpd_driver_init);//subsys_initcall
module_exit(tpd_driver_exit);

