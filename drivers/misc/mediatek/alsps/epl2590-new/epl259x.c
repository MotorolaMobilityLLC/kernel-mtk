/* drivers/hwmon/mt6516/amit/epl259x.c - EPL259x ALS/PS driver
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
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <linux/atomic.h>
#include <linux/io.h>
#include "epl259x.h"
#include <linux/input/mt.h>
#include <alsps.h>
#include "upmu_sw.h"
#include "upmu_common.h"
#include <linux/gpio.h>
#include <linux/of_irq.h>

#include <linux/wakelock.h>
#include <linux/sched.h>
#if defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
#endif
#include <alsps.h>
/******************************************************************************
 * driver info
*******************************************************************************/
#define COMPATIABLE_NAME "mediatek,epl2590"
#define DRIVER_VERSION          "3.0.0"
#define EPL_DEV_NAME   		    "EPL259x"
/******************************************************************************
 * ALS / PS sensor structure
*******************************************************************************/
// MTK_LTE is chose structure hwmsen_object or control_path, data_path
// MTK_LTE = 1, is control_path, data_path
// MTK_LTE = 0 hwmsen_object
#if 0
#ifdef CONFIG_MTK_AUTO_DETECT_ALSPS
#define MTK_LTE         1
#else
#define MTK_LTE         0
#endif
#else
#define MTK_LTE         1
#endif

#if MTK_LTE
#include <alsps.h>
#endif
/******************************************************************************
 *  ALS / PS define
 ******************************************************************************/
#define PS_GES          0   // fake gesture
#define PS_DYN_K        1   // PS Auto K
#define ALS_DYN_INTT    0   // ALS Auto INTT
#define HS_ENABLE       0   // heart beat

#define EINT_API    1


#define LUX_PER_COUNT	400  //ALS lux per count
/*ALS interrupt table*/
unsigned long als_lux_intr_level[] = {15, 39, 63, 316, 639, 4008, 5748, 10772, 14517, 65535};
unsigned long als_adc_intr_level[10] = {0};

int als_frame_time = 0;
int ps_frame_time = 0;
/******************************************************************************
 *  factory setting
 ******************************************************************************/
static const char ps_cal_file[]="/data/data/com.eminent.ps.calibration/ps.dat";  //ps calibration file path
static const char als_cal_file[]="/data/data/com.eminent.ps.calibration/als.dat";  //als calibration file path
static struct alsps_hw alsps_cust;
static struct alsps_hw *hw = &alsps_cust;
static int PS_h_offset = 3000;  //factory high threshold offset
static int PS_l_offset = 2000;  //factory low threshold offset
static int PS_MAX_XTALK = 30000;  //factory max crosstalk, if real crosstalk > max crosstalk, return fail

/******************************************************************************
 *I2C function define
*******************************************************************************/
#define TXBYTES 				2
#define PACKAGE_SIZE 			48
#define I2C_RETRY_COUNT 		2
int i2c_max_count=8;
static const struct i2c_device_id epl_sensor_i2c_id[] = {{EPL_DEV_NAME,0},{}};
//static struct i2c_board_info __initdata i2c_epl_sensor= { I2C_BOARD_INFO(EPL_DEV_NAME, (0x92>>1))};
/******************************************************************************
 * extern functions
*******************************************************************************/
#if !EINT_API
#if defined(MT6575) || defined(MT6571) || defined(MT6589)
extern void mt65xx_eint_unmask(unsigned int line);
extern void mt65xx_eint_mask(unsigned int line);
extern void mt65xx_eint_set_polarity(kal_uint8 eintno, kal_bool ACT_Polarity);
extern void mt65xx_eint_set_hw_debounce(kal_uint8 eintno, kal_uint32 ms);
extern kal_uint32 mt65xx_eint_set_sens(kal_uint8 eintno, kal_bool sens);
extern void mt65xx_eint_registration(kal_uint8 eintno, kal_bool Dbounce_En,
                                     kal_bool ACT_Polarity, void (EINT_FUNC_PTR)(void),
                                     kal_bool auto_umask);
#else
extern void mt_eint_unmask(unsigned int line);
extern void mt_eint_mask(unsigned int line);
extern void mt_eint_set_polarity(kal_uint8 eintno, kal_bool ACT_Polarity);
extern void mt_eint_set_hw_debounce(kal_uint8 eintno, kal_uint32 ms);
extern kal_uint32 mt_eint_set_sens(kal_uint8 eintno, kal_bool sens);
extern void mt_eint_registration(unsigned int eint_num, unsigned int flow, void (EINT_FUNC_PTR)(void), unsigned int is_auto_umask);
#endif
#endif

/******************************************************************************
 *  configuration
 ******************************************************************************/
#ifndef CUST_EINT_ALS_TYPE
#define CUST_EINT_ALS_TYPE  8
#endif
#define POWER_NONE_MACRO MT65XX_POWER_NONE

struct hwmsen_object *ps_hw, * als_hw;
static struct epl_sensor_priv *epl_sensor_obj = NULL;
#if !MTK_LTE
static struct platform_driver epl_sensor_alsps_driver;
#endif
static struct wake_lock ps_lock;
static struct mutex sensor_mutex;
static epl_optical_sensor epl_sensor;
static struct i2c_client *epl_sensor_i2c_client = NULL;

typedef struct _epl_raw_data
{
    u8 raw_bytes[PACKAGE_SIZE];
} epl_raw_data;
static epl_raw_data	gRawData;

#define APS_TAG                 	  	"[ALS/PS] "
#define APS_FUN(f)              	  	printk(KERN_INFO APS_TAG"%s\n", __FUNCTION__)
#define APS_ERR(fmt, args...)    	    printk(KERN_ERR  APS_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define APS_LOG(fmt, args...)    	    printk(KERN_INFO APS_TAG fmt, ##args)
#define APS_DBG(fmt, args...)    	    printk(KERN_INFO fmt, ##args)

static int epl_sensor_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int epl_sensor_i2c_remove(struct i2c_client *client);
//static int epl_sensor_i2c_detect(struct i2c_client *client, struct i2c_board_info *info);
static int epl_sensor_i2c_suspend(struct i2c_client *client, pm_message_t msg);
static int epl_sensor_i2c_resume(struct i2c_client *client);
static irqreturn_t epl_sensor_eint_func(int irq, void *desc);
static int set_psensor_intr_threshold(uint16_t low_thd, uint16_t high_thd);
static int set_lsensor_intr_threshold(uint16_t low_thd, uint16_t high_thd);
void epl_sensor_update_mode(struct i2c_client *client);
void epl_sensor_fast_update(struct i2c_client *client);
int epl_sensor_read_ps(struct i2c_client *client);
static int ps_sensing_time(int intt, int adc, int cycle);
static int als_sensing_time(int intt, int adc, int cycle);
static int als_intr_update_table(struct epl_sensor_priv *epld);
int factory_ps_data(void);
int factory_als_data(void);
static struct epl_sensor_priv *g_epl2590_ptr;
static long long int_top_time;
#if !MTK_LTE
int epl_sensor_ps_operate(void* self, uint32_t command, void* buff_in, int size_in, void* buff_out, int size_out, int* actualout);
int epl_sensor_als_operate(void* self, uint32_t command, void* buff_in, int size_in, void* buff_out, int size_out, int* actualout);
#endif
#if PS_DYN_K
void epl_sensor_restart_dynk_polling(void);
#endif

typedef enum
{
    CMC_BIT_ALS   	= 1,
    CMC_BIT_PS     	= 2,
#if HS_ENABLE
    CMC_BIT_HS     	= 3,
#endif
#if PS_GES
    CMC_BIT_GES     = 4,
#endif
} CMC_BIT;

typedef enum
{
    CMC_BIT_RAW   			= 0x0,
    CMC_BIT_PRE_COUNT     	= 0x1,
    CMC_BIT_DYN_INT			= 0x2,
    CMC_BIT_TABLE			= 0x3,
    CMC_BIT_INTR_LEVEL		= 0x4,
} CMC_ALS_REPORT_TYPE;

struct epl_sensor_i2c_addr      /*define a series of i2c slave address*/
{
    u8  write_addr;
};

struct epl_sensor_priv
{
    struct alsps_hw  *hw;
    struct i2c_client *client;
    struct delayed_work  eint_work;
#if PS_DYN_K
    struct delayed_work  dynk_thd_polling_work;
#endif
#if HS_ENABLE || PS_GES
	struct delayed_work  polling_work;
#endif
    struct input_dev *gs_input_dev;
    /*i2c address group*/
    struct epl_sensor_i2c_addr  addr;
    /*misc*/
    atomic_t   	als_suspend;
    atomic_t    ps_suspend;
#if HS_ENABLE
	atomic_t   	hs_suspend;
#endif
#if PS_GES
	atomic_t   	ges_suspend;
#endif
    /*data*/
    u16		    lux_per_count;
    ulong       enable;         	/*record HAL enalbe status*/
    /*data*/
    u16         als_level_num;
    u16         als_value_num;
    u32         als_level[C_CUST_ALS_LEVEL-1];
    u32         als_value[C_CUST_ALS_LEVEL];
    /*als interrupt*/
    int als_intr_level;
    int als_intr_lux;
    /*early suspend*/
#if defined(CONFIG_HAS_EARLYSUSPEND)
    struct early_suspend    early_drv;
#endif
#if EINT_API
    struct device_node *irq_node;
    int		irq;
#endif
};
#ifdef CONFIG_OF
static const struct of_device_id alsps_of_match[] = {
	{.compatible = "mediatek,alsps"},
	{},
};
#endif
static struct i2c_driver epl_sensor_i2c_driver =
{
    .probe     	= epl_sensor_i2c_probe,
    .remove     = epl_sensor_i2c_remove,
    //.detect     = epl_sensor_i2c_detect,
    .suspend    = epl_sensor_i2c_suspend,
    .resume     = epl_sensor_i2c_resume,
    .id_table   = epl_sensor_i2c_id,
    //.address_data = &epl_sensor_addr_data,
    .driver = {
        //.owner  = THIS_MODULE,
        .name   = EPL_DEV_NAME,
#ifdef CONFIG_OF
			.of_match_table = alsps_of_match,
#endif
    },
};

#if MTK_LTE
static int alsps_init_flag =-1; // 0<==>OK -1 <==> fail
static int alsps_local_init(void);
static int alsps_remove(void);
static struct alsps_init_info epl_sensor_init_info = {
		.name = EPL_DEV_NAME,
		.init = alsps_local_init,
		.uninit = alsps_remove,
};

extern struct alsps_context *alsps_context_obj;
#endif
/******************************************************************************
 *  PS_DYN_K
 ******************************************************************************/
#if PS_DYN_K
static int dynk_polling_delay = 200;
int dynk_min_ps_raw_data = 0xffff;
int dynk_max_ir_data;
u32 dynk_thd_low = 0;
u32 dynk_thd_high = 0;
int dynk_low_offset;
int dynk_high_offset;
#endif  /*PS_DYN_K*/
/******************************************************************************
 *  ALS_DYN_INTT
 ******************************************************************************/
#if ALS_DYN_INTT
//Dynamic INTT
int dynamic_intt_idx;
int dynamic_intt_init_idx = 0;	//initial dynamic_intt_idx
int c_gain;
int dynamic_intt_lux = 0;
uint16_t dynamic_intt_high_thr;
uint16_t dynamic_intt_low_thr;
uint32_t dynamic_intt_max_lux = 17000;
uint32_t dynamic_intt_min_lux = 0;
uint32_t dynamic_intt_min_unit = 1000;
static int als_dynamic_intt_intt[] = {EPL_ALS_INTT_1024, EPL_ALS_INTT_1024};
static int als_dynamic_intt_value[] = {1024, 1024}; //match als_dynamic_intt_intt table
static int als_dynamic_intt_gain[] = {EPL_GAIN_MID, EPL_GAIN_LOW};
static int als_dynamic_intt_high_thr[] = {60000, 60000};
static int als_dynamic_intt_low_thr[] = {200, 200};
static int als_dynamic_intt_intt_num =  sizeof(als_dynamic_intt_value)/sizeof(int);
#endif /*ALS_DYN_INTT*/
/******************************************************************************
 *  HS_ENABLE
 ******************************************************************************/
#if HS_ENABLE
static struct mutex hs_sensor_mutex;
static bool hs_enable_flag = false;
#endif /*HS_ENABLE*/
/******************************************************************************
 *  PS_GES
 ******************************************************************************/
#if PS_GES
static bool ps_ges_enable_flag = false;
u16 ges_threshold_low = 500;
u16 ges_threshold_high = 800;
#define KEYCODE_LEFT			KEY_LEFTALT
u8 last_ges_state = 0;
#endif /*PS_GES*/

static int epl_sensor_I2C_Write_Block(struct i2c_client *client, u8 addr, u8 *data, u8 len)
{   /*because address also occupies one byte, the maximum length for write is 7 bytes*/
    int err, idx, num;
    char buf[C_I2C_FIFO_SIZE];
    err =0;

    if (!client)
    {
        return -EINVAL;
    }
    else if (len >= C_I2C_FIFO_SIZE)
    {
        APS_ERR(" length %d exceeds %d\n", len, C_I2C_FIFO_SIZE);
        return -EINVAL;
    }
    num = 0;
    buf[num++] = addr;
    for (idx = 0; idx < len; idx++)
    {
        buf[num++] = data[idx];
    }
    err = i2c_master_send(client, buf, num);
    if (err < 0)
    {
        APS_ERR("send command error!!\n");

        return -EFAULT;
    }

    return err;
}

static int epl_sensor_I2C_Write_Cmd(struct i2c_client *client, uint8_t regaddr, uint8_t data, uint8_t txbyte)
{
    uint8_t buffer[2];
    int ret = 0;
    int retry;

    buffer[0] = regaddr;
    buffer[1] = data;

    for(retry = 0; retry < I2C_RETRY_COUNT; retry++)
    {
        ret = i2c_master_send(client, buffer, txbyte);

        if (ret == txbyte)
        {
            break;
        }

        APS_ERR("i2c write error,TXBYTES %d\n",ret);
    }
    if(retry>=I2C_RETRY_COUNT)
    {
        APS_ERR("i2c write retry over %d\n", I2C_RETRY_COUNT);
        return -EINVAL;
    }

    return ret;
}

static int epl_sensor_I2C_Write(struct i2c_client *client, uint8_t regaddr, uint8_t data)
{
    int ret = 0;
    ret = epl_sensor_I2C_Write_Cmd(client, regaddr, data, 0x02);
    return ret;
}

static int epl_sensor_I2C_Read(struct i2c_client *client, uint8_t regaddr, uint8_t bytecount)
{
    int ret = 0;
    int retry;
    int read_count=0, rx_count=0;
    while(bytecount>0)
    {
        epl_sensor_I2C_Write_Cmd(client, regaddr+read_count, 0x00, 0x01);
        for(retry = 0; retry < I2C_RETRY_COUNT; retry++)
        {
            rx_count = bytecount>i2c_max_count?i2c_max_count:bytecount;
            ret = i2c_master_recv(client, &gRawData.raw_bytes[read_count], rx_count);

            if (ret == rx_count)
                break;

            APS_ERR("i2c read error %d\r\n",ret);
        }
        if(retry>=I2C_RETRY_COUNT)
        {
            APS_ERR("i2c read retry over %d\n", I2C_RETRY_COUNT);
            return -EINVAL;
        }
        bytecount-=rx_count;
        read_count+=rx_count;
    }

    return ret;
}

#if PS_GES
static void epl_sensor_notify_event(void)
{
    struct epl_sensor_priv *epld = epl_sensor_obj;
    struct input_dev *idev = epld->gs_input_dev;

    APS_LOG("  --> LEFT\n\n");
    input_report_key(idev, KEYCODE_LEFT, 1);
    input_report_key(idev,  KEYCODE_LEFT, 0);
    input_sync(idev);
}
#endif

static void set_als_ps_intr_type(struct i2c_client *client, bool ps_polling, bool als_polling)
{
    //set als / ps interrupt control mode and interrupt type
	switch((ps_polling << 1) | als_polling)
	{
		case 0: // ps and als interrupt
			epl_sensor.interrupt_control = 	EPL_INT_CTRL_ALS_OR_PS;
			epl_sensor.als.interrupt_type = EPL_INTTY_ACTIVE;
			epl_sensor.ps.interrupt_type = EPL_INTTY_ACTIVE;
		break;
		case 1: //ps interrupt and als polling
			epl_sensor.interrupt_control = 	EPL_INT_CTRL_PS;
			epl_sensor.als.interrupt_type = EPL_INTTY_DISABLE;
			epl_sensor.ps.interrupt_type = EPL_INTTY_ACTIVE;
		break;
		case 2: // ps polling and als interrupt
			epl_sensor.interrupt_control = 	EPL_INT_CTRL_ALS;
			epl_sensor.als.interrupt_type = EPL_INTTY_ACTIVE;
			epl_sensor.ps.interrupt_type = EPL_INTTY_DISABLE;
		break;
		case 3: //ps and als polling
			epl_sensor.interrupt_control = 	EPL_INT_CTRL_ALS_OR_PS;
			epl_sensor.als.interrupt_type = EPL_INTTY_DISABLE;
			epl_sensor.ps.interrupt_type = EPL_INTTY_DISABLE;
		break;
	}
}

static void write_global_variable(struct i2c_client *client)
{
    u8 buf;
#if HS_ENABLE || PS_GES
    struct epl_sensor_priv *obj = epl_sensor_obj;
#if HS_ENABLE
    bool enable_hs = test_bit(CMC_BIT_HS, &obj->enable) && atomic_read(&obj->hs_suspend)==0;
#endif
#if PS_GES
    bool enable_ges = test_bit(CMC_BIT_GES, &obj->enable) && atomic_read(&obj->ges_suspend)==0;
#endif
#endif   /*HS_ENABLE || PS_GES*/

    epl_sensor_I2C_Write(client, 0x11, EPL_POWER_OFF | EPL_RESETN_RESET);
    //wake up chip
    //buf = epl_sensor.reset | epl_sensor.power;
    //epl_sensor_I2C_Write(client, 0x11, buf);

    /* read revno*/
    epl_sensor_I2C_Read(client, 0x20, 2);
    epl_sensor.revno = gRawData.raw_bytes[0] | gRawData.raw_bytes[1] << 8;

    /*chip refrash*/
    epl_sensor_I2C_Write(client, 0xfd, 0x8e);
    epl_sensor_I2C_Write(client, 0xfe, 0x22);
    epl_sensor_I2C_Write(client, 0xfe, 0x02);
    epl_sensor_I2C_Write(client, 0xfd, 0x00);
    epl_sensor_I2C_Write(client, 0xfc, EPL_A_D | EPL_NORMAL| EPL_GFIN_ENABLE | EPL_VOS_ENABLE | EPL_DOC_ON);

#if HS_ENABLE
	if(enable_hs)
	{
	    epl_sensor.mode = EPL_MODE_PS;

        set_als_ps_intr_type(client, 1, epl_sensor.als.polling_mode);
	    buf = epl_sensor.interrupt_control | epl_sensor.ps.persist |epl_sensor.ps.interrupt_type;
        epl_sensor_I2C_Write(client, 0x06, buf);
        buf = epl_sensor.als.interrupt_channel_select | epl_sensor.als.persist | epl_sensor.als.interrupt_type;
        epl_sensor_I2C_Write(client, 0x07, buf);

		/*hs setting*/
		buf = epl_sensor.hs.integration_time | epl_sensor.hs.gain;
		epl_sensor_I2C_Write(client, 0x03, buf);

		buf = epl_sensor.hs.adc | epl_sensor.hs.cycle;
		epl_sensor_I2C_Write(client, 0x04, buf);

		buf = epl_sensor.hs.ir_on_control | epl_sensor.hs.ir_mode | epl_sensor.hs.ir_driver;
		epl_sensor_I2C_Write(client, 0x05, buf);

		buf = epl_sensor.hs.compare_reset | epl_sensor.hs.lock;
		epl_sensor_I2C_Write(client, 0x1b, buf);
	}
#if !PS_GES	/*PS_GES*/
	else
#elif PS_GES
    else if(enable_ges)
#endif  /*PS_GES*/
#endif /*HS_ENABLE*/
#if PS_GES
#if !HS_ENABLE /*HS_ENABLE*/
    if(enable_ges)
#endif  /*HS_ENABLE*/
    {
        set_als_ps_intr_type(client, epl_sensor.ps.polling_mode, epl_sensor.als.polling_mode);
	    buf = epl_sensor.interrupt_control | epl_sensor.ps.persist |epl_sensor.ps.interrupt_type;
        epl_sensor_I2C_Write(client, 0x06, buf);
        buf = epl_sensor.als.interrupt_channel_select | epl_sensor.als.persist | epl_sensor.als.interrupt_type;
        epl_sensor_I2C_Write(client, 0x07, buf);

        /*ges setting*/
        buf = epl_sensor.ges.integration_time | epl_sensor.ges.gain;
        epl_sensor_I2C_Write(client, 0x03, buf);

        buf = epl_sensor.ges.adc | epl_sensor.ges.cycle;
        epl_sensor_I2C_Write(client, 0x04, buf);

        buf = epl_sensor.ges.ir_on_control | epl_sensor.ges.ir_mode | epl_sensor.ges.ir_drive;
        epl_sensor_I2C_Write(client, 0x05, buf);

        buf = epl_sensor.ges.compare_reset | epl_sensor.ges.lock;
        epl_sensor_I2C_Write(client, 0x1b, buf);
        epl_sensor_I2C_Write(client, 0x22, (u8)(epl_sensor.ges.cancelation& 0xff));
        epl_sensor_I2C_Write(client, 0x23, (u8)((epl_sensor.ges.cancelation & 0xff00) >> 8));
        set_psensor_intr_threshold(epl_sensor.ges.low_threshold, epl_sensor.ges.high_threshold);
    }
    else
#endif /*PS_GES*/
    {
        //set als / ps interrupt control mode and trigger type
        set_als_ps_intr_type(client, epl_sensor.ps.polling_mode, epl_sensor.als.polling_mode);

        /*ps setting*/
        buf = epl_sensor.ps.integration_time | epl_sensor.ps.gain;
        epl_sensor_I2C_Write(client, 0x03, buf);
        buf = epl_sensor.ps.adc | epl_sensor.ps.cycle;
        epl_sensor_I2C_Write(client, 0x04, buf);
        buf = epl_sensor.ps.ir_on_control | epl_sensor.ps.ir_mode | epl_sensor.ps.ir_drive;
        epl_sensor_I2C_Write(client, 0x05, buf);
        buf = epl_sensor.interrupt_control | epl_sensor.ps.persist |epl_sensor.ps.interrupt_type;
        epl_sensor_I2C_Write(client, 0x06, buf);
        buf = epl_sensor.ps.compare_reset | epl_sensor.ps.lock;
        epl_sensor_I2C_Write(client, 0x1b, buf);
        epl_sensor_I2C_Write(client, 0x22, (u8)(epl_sensor.ps.cancelation& 0xff));
        epl_sensor_I2C_Write(client, 0x23, (u8)((epl_sensor.ps.cancelation & 0xff00) >> 8));
        set_psensor_intr_threshold(epl_sensor.ps.low_threshold, epl_sensor.ps.high_threshold);

        /*als setting*/
        buf = epl_sensor.als.integration_time | epl_sensor.als.gain;
        epl_sensor_I2C_Write(client, 0x01, buf);
        buf = epl_sensor.als.adc | epl_sensor.als.cycle;
        epl_sensor_I2C_Write(client, 0x02, buf);
        buf = epl_sensor.als.interrupt_channel_select | epl_sensor.als.persist | epl_sensor.als.interrupt_type;
        epl_sensor_I2C_Write(client, 0x07, buf);
        buf = epl_sensor.als.compare_reset | epl_sensor.als.lock;
        epl_sensor_I2C_Write(client, 0x12, buf);
        set_lsensor_intr_threshold(epl_sensor.als.low_threshold, epl_sensor.als.high_threshold);
	}

    //set mode and wait
    buf = epl_sensor.wait | epl_sensor.mode;
    epl_sensor_I2C_Write(client, 0x00, buf);
}

static int write_factory_calibration(struct epl_sensor_priv *epl_data, char* ps_data, int ps_cal_len)
{
    struct file *fp_cal;
	mm_segment_t fs;
	loff_t pos;

	APS_FUN();
    pos = 0;
	fp_cal = filp_open(ps_cal_file, O_CREAT|O_RDWR|O_TRUNC, 0755/*S_IRWXU*/);
	if (IS_ERR(fp_cal))
	{
		APS_ERR("[ELAN]create file_h error\n");
		return -1;
	}
    fs = get_fs();
	set_fs(KERNEL_DS);
	vfs_write(fp_cal, ps_data, ps_cal_len, &pos);
    filp_close(fp_cal, NULL);
	set_fs(fs);

	return 0;
}

static bool read_factory_calibration(void)
{
    struct epl_sensor_priv *obj = epl_sensor_obj;
    struct file *fp;
    mm_segment_t fs;
    loff_t pos;
    char buffer[100]= {0};

    if(epl_sensor.ps.factory.calibration_enable && !epl_sensor.ps.factory.calibrated)
    {
		fp = filp_open(ps_cal_file, O_RDWR, S_IRUSR);
        if (IS_ERR(fp))
        {
            APS_ERR("NO calibration file\n");
            epl_sensor.ps.factory.calibration_enable =  false;
        }
        else
        {
            int ps_cancelation = 0, ps_hthr = 0, ps_lthr = 0;
            pos = 0;
            fs = get_fs();
            set_fs(KERNEL_DS);
            vfs_read(fp, buffer, sizeof(buffer), &pos);
            filp_close(fp, NULL);

            sscanf(buffer, "%d,%d,%d", &ps_cancelation, &ps_hthr, &ps_lthr);
			epl_sensor.ps.factory.cancelation = ps_cancelation;
			epl_sensor.ps.factory.high_threshold = ps_hthr;
			epl_sensor.ps.factory.low_threshold = ps_lthr;
            set_fs(fs);

            epl_sensor.ps.high_threshold = epl_sensor.ps.factory.high_threshold;
            epl_sensor.ps.low_threshold = epl_sensor.ps.factory.low_threshold;
            epl_sensor.ps.cancelation = epl_sensor.ps.factory.cancelation;
        }
        epl_sensor_I2C_Write(obj->client,0x22, (u8)(epl_sensor.ps.cancelation& 0xff));
        epl_sensor_I2C_Write(obj->client,0x23, (u8)((epl_sensor.ps.cancelation & 0xff00) >> 8));
        set_psensor_intr_threshold(epl_sensor.ps.low_threshold, epl_sensor.ps.high_threshold);

        epl_sensor.ps.factory.calibrated = true;
    }

    if(epl_sensor.als.factory.calibration_enable && !epl_sensor.als.factory.calibrated)
    {
        fp = filp_open(ps_cal_file, O_RDWR, S_IRUSR);
        if (IS_ERR(fp))
        {
            APS_ERR("NO calibration file\n");
            epl_sensor.als.factory.calibration_enable =  false;
        }
        else
        {
            int als_lux_per_count = 0;
            pos = 0;
            fs = get_fs();
            set_fs(KERNEL_DS);
            vfs_read(fp, buffer, sizeof(buffer), &pos);
            filp_close(fp, NULL);
            sscanf(buffer, "%d", &als_lux_per_count);
			epl_sensor.als.factory.lux_per_count = als_lux_per_count;
            set_fs(fs);
        }
        epl_sensor.als.factory.calibrated = true;
    }

    return true;
}

static int epl_run_ps_calibration(struct epl_sensor_priv *epl_data)
{
    struct epl_sensor_priv *epld = epl_data;
    u16 ch1=0;
    int ps_hthr=0, ps_lthr=0, ps_cancelation=0, ps_cal_len = 0;
    char ps_calibration[20];

    if(PS_MAX_XTALK < 0)
    {
        APS_ERR("[%s]:Failed: PS_MAX_XTALK < 0 \r\n", __func__);
        return -EINVAL;
    }
	switch(epl_sensor.mode)
	{
		case EPL_MODE_PS:
		case EPL_MODE_ALS_PS:
            ch1 = factory_ps_data();
        break;
	}
    if(ch1 > PS_MAX_XTALK)
    {
        APS_ERR("[%s]:Failed: ch1 > max_xtalk(%d) \r\n", __func__, ch1);
        return -EINVAL;
    }
    else if(ch1 < 0)
    {
        APS_ERR("[%s]:Failed: ch1 < 0\r\n", __func__);
        return -EINVAL;
    }
    ps_hthr = ch1 + PS_h_offset;
    ps_lthr = ch1 + PS_l_offset;
    ps_cal_len = sprintf(ps_calibration, "%d,%d,%d", ps_cancelation, ps_hthr, ps_lthr);

    if(write_factory_calibration(epld, ps_calibration, ps_cal_len) < 0)
    {
        APS_ERR("[%s] create file error \n", __func__);
        return -EINVAL;
    }
    epl_sensor.ps.low_threshold = ps_lthr;
	epl_sensor.ps.high_threshold = ps_hthr;
	set_psensor_intr_threshold(epl_sensor.ps.low_threshold, epl_sensor.ps.high_threshold);
	APS_LOG("[%s]: ch1 = %d\n", __func__, ch1);

	return ch1;
}

static void initial_global_variable(struct i2c_client *client, struct epl_sensor_priv *obj)
{
    //general setting
    epl_sensor.power = EPL_POWER_ON;
    epl_sensor.reset = EPL_RESETN_RUN;
    epl_sensor.mode = EPL_MODE_IDLE;
    epl_sensor.wait = EPL_WAIT_0_MS;
    epl_sensor.osc_sel = EPL_OSC_SEL_1MHZ;

    //als setting
    epl_sensor.als.polling_mode = obj->hw->polling_mode_als;
    epl_sensor.als.integration_time = EPL_ALS_INTT_1024;
    epl_sensor.als.gain = EPL_GAIN_LOW;
    epl_sensor.als.adc = EPL_PSALS_ADC_11;
    epl_sensor.als.cycle = EPL_CYCLE_16;
    epl_sensor.als.interrupt_channel_select = EPL_ALS_INT_CHSEL_1;
    epl_sensor.als.persist = EPL_PERIST_1;
    epl_sensor.als.compare_reset = EPL_CMP_RUN;
    epl_sensor.als.lock = EPL_UN_LOCK;
    epl_sensor.als.report_type = CMC_BIT_RAW; //CMC_BIT_DYN_INT;
    epl_sensor.als.high_threshold = obj->hw->als_threshold_high;
    epl_sensor.als.low_threshold = obj->hw->als_threshold_low;
    //als factory
    epl_sensor.als.factory.calibration_enable =  false;
    epl_sensor.als.factory.calibrated = false;
    epl_sensor.als.factory.lux_per_count = LUX_PER_COUNT;
    //update als intr table
    if(epl_sensor.als.polling_mode == 0)
        als_intr_update_table(obj);

#if ALS_DYN_INTT
    if(epl_sensor.als.report_type == CMC_BIT_DYN_INT)
    {
        dynamic_intt_idx = dynamic_intt_init_idx;
        epl_sensor.als.integration_time = als_dynamic_intt_intt[dynamic_intt_idx];
        epl_sensor.als.gain = als_dynamic_intt_gain[dynamic_intt_idx];
        dynamic_intt_high_thr = als_dynamic_intt_high_thr[dynamic_intt_idx];
        dynamic_intt_low_thr = als_dynamic_intt_low_thr[dynamic_intt_idx];
    }
    c_gain = 300; // 300/1000=0.3 /*Lux per count*/
#endif

    //ps setting
    epl_sensor.ps.polling_mode = obj->hw->polling_mode_ps;
    epl_sensor.ps.integration_time = EPL_PS_INTT_272;
    epl_sensor.ps.gain = EPL_GAIN_LOW;
    epl_sensor.ps.adc = EPL_PSALS_ADC_11;
    epl_sensor.ps.cycle = EPL_CYCLE_16;
    epl_sensor.ps.persist = EPL_PERIST_1;
    epl_sensor.ps.ir_on_control = EPL_IR_ON_CTRL_ON;
    epl_sensor.ps.ir_mode = EPL_IR_MODE_CURRENT;
    epl_sensor.ps.ir_drive = EPL_IR_DRIVE_100;
    epl_sensor.ps.compare_reset = EPL_CMP_RUN;
    epl_sensor.ps.lock = EPL_UN_LOCK;
    epl_sensor.ps.high_threshold = obj->hw->ps_threshold_high;
    epl_sensor.ps.low_threshold = obj->hw->ps_threshold_low;
    //ps factory
    epl_sensor.ps.factory.calibration_enable =  false;
    epl_sensor.ps.factory.calibrated = false;
    epl_sensor.ps.factory.cancelation= 0;
#if PS_DYN_K
    dynk_max_ir_data = 50000; // ps max ch0
    dynk_low_offset = 500;
    dynk_high_offset = 800;
#endif /*PS_DYN_K*/

#if HS_ENABLE
	//hs setting
    epl_sensor.hs.integration_time = EPL_PS_INTT_80;
	epl_sensor.hs.integration_time_max = EPL_PS_INTT_272;
	epl_sensor.hs.integration_time_min = EPL_PS_INTT_32;
    epl_sensor.hs.gain = EPL_GAIN_LOW;
    epl_sensor.hs.adc = EPL_PSALS_ADC_11;
    epl_sensor.hs.cycle = EPL_CYCLE_4;
    epl_sensor.hs.ir_on_control = EPL_IR_ON_CTRL_ON;
    epl_sensor.hs.ir_mode = EPL_IR_MODE_CURRENT;
    epl_sensor.hs.ir_driver = EPL_IR_DRIVE_200;
    epl_sensor.hs.compare_reset = EPL_CMP_RUN;
    epl_sensor.hs.lock = EPL_UN_LOCK;
	epl_sensor.hs.low_threshold = 6400;
	epl_sensor.hs.mid_threshold = 25600;
	epl_sensor.hs.high_threshold = 60800;
#endif

#if PS_GES
    //ps setting
    epl_sensor.ges.polling_mode = obj->hw->polling_mode_ps;
    epl_sensor.ges.integration_time = EPL_PS_INTT_272;
    epl_sensor.ges.gain = EPL_GAIN_LOW;
    epl_sensor.ges.adc = EPL_PSALS_ADC_11;
    epl_sensor.ges.cycle = EPL_CYCLE_2;
    epl_sensor.ges.persist = EPL_PERIST_1;
    epl_sensor.ges.ir_on_control = EPL_IR_ON_CTRL_ON;
    epl_sensor.ges.ir_mode = EPL_IR_MODE_CURRENT;
    epl_sensor.ges.ir_drive = EPL_IR_DRIVE_100;
    epl_sensor.ges.compare_reset = EPL_CMP_RUN;
    epl_sensor.ges.lock = EPL_UN_LOCK;
    epl_sensor.ges.high_threshold = ges_threshold_high;
    epl_sensor.ges.low_threshold = ges_threshold_low;
#endif
    //write setting to sensor
    write_global_variable(client);
}

#if ALS_DYN_INTT
long raw_convert_to_lux(u16 raw_data)
{
    long lux = 0;
    long dyn_intt_raw = 0;
    int gain_value = 0;

    if(epl_sensor.als.gain == EPL_GAIN_MID)
    {
        gain_value = 8;
    }
    else if (epl_sensor.als.gain == EPL_GAIN_LOW)
    {
        gain_value = 1;
    }
    dyn_intt_raw = (raw_data * 10) / (10 * gain_value * als_dynamic_intt_value[dynamic_intt_idx] / als_dynamic_intt_value[1]); //float calculate
    APS_LOG("[%s]: dyn_intt_raw=%ld \r\n", __func__, dyn_intt_raw);

    if(dyn_intt_raw > 0xffff)
        epl_sensor.als.dyn_intt_raw = 0xffff;
    else
        epl_sensor.als.dyn_intt_raw = dyn_intt_raw;

    lux = c_gain * epl_sensor.als.dyn_intt_raw;
    APS_LOG("[%s]:raw_data=%d, epl_sensor.als.dyn_intt_raw=%d, lux=%ld\r\n", __func__, raw_data, epl_sensor.als.dyn_intt_raw, lux);

    if(lux >= (dynamic_intt_max_lux*dynamic_intt_min_unit)){
        APS_LOG("[%s]:raw_convert_to_lux: change max lux\r\n", __func__);
        lux = dynamic_intt_max_lux * dynamic_intt_min_unit;
    }
    else if(lux <= (dynamic_intt_min_lux*dynamic_intt_min_unit)){
        APS_LOG("[%s]:raw_convert_to_lux: change min lux\r\n", __func__);
        lux = dynamic_intt_min_lux * dynamic_intt_min_unit;
    }

    return lux;
}
#endif

static int epl_sensor_get_als_value(struct epl_sensor_priv *obj, u16 als)
{
    int idx;
    int invalid = 0;
    int level=0;
	long lux;
#if ALS_DYN_INTT
	long now_lux=0, lux_tmp=0;
    bool change_flag = false;
#endif

    switch(epl_sensor.als.report_type)
    {
        case CMC_BIT_RAW:
            return als;
        break;
        case CMC_BIT_PRE_COUNT:
            return (als * epl_sensor.als.factory.lux_per_count)/1000;
        break;
        case CMC_BIT_TABLE:
            for(idx = 0; idx < obj->als_level_num; idx++)
            {
                if(als < obj->hw->als_level[idx])
                {
                    break;
                }
            }

            if(idx >= obj->als_value_num)
            {
                APS_ERR("exceed range\n");
                idx = obj->als_value_num - 1;
            }

            if(!invalid)
            {
                APS_DBG("ALS: %05d => %05d\n", als, obj->hw->als_value[idx]);
                return obj->hw->als_value[idx];
            }
            else
            {
                APS_ERR("ALS: %05d => %05d (-1)\n", als, obj->hw->als_value[idx]);
                return als;
            }
        break;
#if ALS_DYN_INTT
		case CMC_BIT_DYN_INT:

            APS_LOG("[%s]: dynamic_intt_idx=%d, als_dynamic_intt_value=%d, dynamic_intt_gain=%d, als=%d \r\n",
                                    __func__, dynamic_intt_idx, als_dynamic_intt_value[dynamic_intt_idx], als_dynamic_intt_gain[dynamic_intt_idx], als);

            if(als > dynamic_intt_high_thr)
        	{
          		if(dynamic_intt_idx == (als_dynamic_intt_intt_num - 1)){
                    //als = dynamic_intt_high_thr;
          		    lux_tmp = raw_convert_to_lux(als);
        	      	APS_LOG(">>>>>>>>>>>>>>>>>>>>>>>> INTT_MAX_LUX\r\n");
          		}
                else{
                    change_flag = true;
        			als  = dynamic_intt_high_thr;
              		lux_tmp = raw_convert_to_lux(als);
                    dynamic_intt_idx++;
                    APS_LOG(">>>>>>>>>>>>>>>>>>>>>>>>change INTT high: %d, raw: %d \r\n", dynamic_intt_idx, als);
                }
            }
            else if(als < dynamic_intt_low_thr)
            {
                if(dynamic_intt_idx == 0){
                    //als = dynamic_intt_low_thr;
                    lux_tmp = raw_convert_to_lux(als);
                    APS_LOG(">>>>>>>>>>>>>>>>>>>>>>>> INTT_MIN_LUX\r\n");
                }
                else{
                    change_flag = true;
        			als  = dynamic_intt_low_thr;
                	lux_tmp = raw_convert_to_lux(als);
                    dynamic_intt_idx--;
                    APS_LOG(">>>>>>>>>>>>>>>>>>>>>>>>change INTT low: %d, raw: %d \r\n", dynamic_intt_idx, als);
                }
            }
            else
            {
            	lux_tmp = raw_convert_to_lux(als);
            }

            now_lux = lux_tmp;
            dynamic_intt_lux = now_lux/dynamic_intt_min_unit;

            if(change_flag == true)
            {
                APS_LOG("[%s]: ALS_DYN_INTT:Chang Setting \r\n", __func__);
                epl_sensor.als.integration_time = als_dynamic_intt_intt[dynamic_intt_idx];
                epl_sensor.als.gain = als_dynamic_intt_gain[dynamic_intt_idx];
                dynamic_intt_high_thr = als_dynamic_intt_high_thr[dynamic_intt_idx];
                dynamic_intt_low_thr = als_dynamic_intt_low_thr[dynamic_intt_idx];
                epl_sensor_fast_update(obj->client);

                mutex_lock(&sensor_mutex);
                //epl_sensor_I2C_Write(obj->client, 0x11, EPL_POWER_OFF | EPL_RESETN_RESET);    // setting with epl_sensor_fast_update()
                //epl_sensor_I2C_Write(obj->client, 0x01, epl_sensor.als.integration_time | epl_sensor.als.gain); //setting with epl_sensor_fast_update()
                epl_sensor_I2C_Write(obj->client, 0x00, epl_sensor.wait | epl_sensor.mode);
                epl_sensor_I2C_Write(obj->client, 0x11, EPL_POWER_ON | EPL_RESETN_RUN);
                mutex_unlock(&sensor_mutex);
            }

            if(epl_sensor.als.polling_mode == 0)
            {
                lux = dynamic_intt_lux;
                if (lux > 0xFFFF)
    				lux = 0xFFFF;

    		    for (idx = 0; idx < 10; idx++)
    		    {
        			if (lux <= (*(als_lux_intr_level + idx))) {
        				level = idx;
        				if (*(als_lux_intr_level + idx))
        					break;
        			}
        			if (idx == 9) {
        				level = idx;
        				break;
        			}
        		}
                obj->als_intr_level = level;
                obj->als_intr_lux = lux;
                return level;
            }
            else
            {
                return dynamic_intt_lux;
            }
		break;
#endif
        case CMC_BIT_INTR_LEVEL:
            lux = (als * epl_sensor.als.factory.lux_per_count)/1000;
            if (lux > 0xFFFF)
				lux = 0xFFFF;

		    for (idx = 0; idx < 10; idx++)
		    {
    			if (lux <= (*(als_lux_intr_level + idx))) {
    				level = idx;
    				if (*(als_lux_intr_level + idx))
    					break;
    			}
    			if (idx == 9) {
    				level = idx;
    				break;
    			}
    		}
            obj->als_intr_level = level;
            obj->als_intr_lux = lux;
            return level;

        break;
    }

    return 0;
}


static int set_psensor_intr_threshold(uint16_t low_thd, uint16_t high_thd)
{
    struct epl_sensor_priv *epld = epl_sensor_obj;
    struct i2c_client *client = epld->client;
    uint8_t high_msb ,high_lsb, low_msb, low_lsb;
    u8 buf[4];

    buf[3] = high_msb = (uint8_t) (high_thd >> 8);
    buf[2] = high_lsb = (uint8_t) (high_thd & 0x00ff);
    buf[1] = low_msb  = (uint8_t) (low_thd >> 8);
    buf[0] = low_lsb  = (uint8_t) (low_thd & 0x00ff);
    mutex_lock(&sensor_mutex);
    epl_sensor_I2C_Write_Block(client, 0x0c, buf, 4);
    mutex_unlock(&sensor_mutex);
    APS_LOG("%s: low_thd = %d, high_thd = %d \n", __FUNCTION__, low_thd, high_thd);
    return 0;
}

static int set_lsensor_intr_threshold(uint16_t low_thd, uint16_t high_thd)
{
    struct epl_sensor_priv *epld = epl_sensor_obj;
    struct i2c_client *client = epld->client;
    uint8_t high_msb ,high_lsb, low_msb, low_lsb;
    u8 buf[4];

    buf[3] = high_msb = (uint8_t) (high_thd >> 8);
    buf[2] = high_lsb = (uint8_t) (high_thd & 0x00ff);
    buf[1] = low_msb  = (uint8_t) (low_thd >> 8);
    buf[0] = low_lsb  = (uint8_t) (low_thd & 0x00ff);
    mutex_lock(&sensor_mutex);
    epl_sensor_I2C_Write_Block(client, 0x08, buf, 4);
    mutex_unlock(&sensor_mutex);
    APS_LOG("%s: low_thd = %d, high_thd = %d \n", __FUNCTION__, low_thd, high_thd);
    return 0;
}


/*----------------------------------------------------------------------------*/
static void epl_sensor_dumpReg(struct i2c_client *client)
{
    APS_LOG("chip id REG 0x00 value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x00));
    APS_LOG("chip id REG 0x01 value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x01));
    APS_LOG("chip id REG 0x02 value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x02));
    APS_LOG("chip id REG 0x03 value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x03));
    APS_LOG("chip id REG 0x04 value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x04));
    APS_LOG("chip id REG 0x05 value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x05));
    APS_LOG("chip id REG 0x06 value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x06));
    APS_LOG("chip id REG 0x07 value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x07));
    APS_LOG("chip id REG 0x11 value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x11));
    APS_LOG("chip id REG 0x12 value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x12));
    APS_LOG("chip id REG 0x1B value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x1B));
    APS_LOG("chip id REG 0x20 value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x20));
    APS_LOG("chip id REG 0x21 value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x21));
    APS_LOG("chip id REG 0x24 value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x24));
    APS_LOG("chip id REG 0x25 value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x25));
    APS_LOG("chip id REG 0xfb value = 0x%x\n", i2c_smbus_read_byte_data(client, 0xfb));
    APS_LOG("chip id REG 0xfc value = 0x%x\n", i2c_smbus_read_byte_data(client, 0xfc));
}

int hw8k_init_device(struct i2c_client *client)
{
    APS_LOG("hw8k_init_device.........\r\n");
    epl_sensor_i2c_client = client;
    APS_LOG(" I2C Addr==[0x%x],line=%d\n", epl_sensor_i2c_client->addr,__LINE__);

    return 0;
}

int epl_sensor_get_addr(struct alsps_hw *hw, struct epl_sensor_i2c_addr *addr)
{
    if(!hw || !addr)
    {
        return -EFAULT;
    }
    addr->write_addr= hw->i2c_addr[0];

    return 0;
}

static void epl_sensor_power(struct alsps_hw *hw, unsigned int on)
{
    static unsigned int power_on = 0;
    //APS_LOG("power %s\n", on ? "on" : "off");
    if(hw->power_id != POWER_NONE_MACRO)
    {
        if(power_on == on)
        {
            APS_LOG("ignore power control: %d\n", on);
        }
        else if(on)
        {
            if(!hwPowerOn(hw->power_id, hw->power_vol, EPL_DEV_NAME))
            {
                APS_ERR("power on fails!!\n");
            }
        }
        else
        {
            if(!hwPowerDown(hw->power_id, EPL_DEV_NAME))
            {
                APS_ERR("power off fail!!\n");
            }
        }
    }
    power_on = on;
}


#if PS_GES
int epl_sensor_report_ges_status(u8 now_ges_state)
{
    struct epl_sensor_priv *obj = epl_sensor_obj;
    bool enable_ges = test_bit(CMC_BIT_GES, &obj->enable) && atomic_read(&obj->ges_suspend)==0;

    APS_LOG("[%s]:now_ges_state=%d \r\n", __func__, now_ges_state);
    if(enable_ges == 1 && epl_sensor.ges.polling_mode == 1)
    {
        if( now_ges_state == 0 && (last_ges_state == 1) && epl_sensor.ps.saturation == 0)
            epl_sensor_notify_event();
    }
    else if(enable_ges == 1 && epl_sensor.ges.polling_mode == 0)
    {
        if(now_ges_state == 0 && epl_sensor.ps.saturation == 0)
           epl_sensor_notify_event();
    }

    return 0;
}
#endif

static void epl_sensor_report_ps_status(void)
{
    struct epl_sensor_priv *epld = epl_sensor_obj;
    int err;
#if !MTK_LTE
    hwm_sensor_data sensor_data;
#endif
    APS_LOG("[%s]: epl_sensor.ps.data.data=%d, ps status=%d \r\n", __func__, epl_sensor.ps.data.data, (epl_sensor.ps.compare_low >> 3));

#if MTK_LTE
    err = ps_report_interrupt_data((epl_sensor.ps.compare_low >> 3));
    if(err != 0)  //if report status is fail, write unlock again.
    {
        APS_ERR("epl_sensor_eint_work err: %d\n", err);
    	epl_sensor_I2C_Write(epld->client,0x1b, EPL_CMP_RESET | EPL_UN_LOCK);
    }
#else
    sensor_data.values[0] = epl_sensor.ps.compare_low >> 3;
	sensor_data.values[1] = epl_sensor.ps.data.data;
	sensor_data.value_divide = 1;
	sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
	if((err = hwmsen_get_interrupt_data(ID_PROXIMITY, &sensor_data))) //if report status is fail, write unlock again.
	{
	    APS_ERR("get interrupt data failed\n");
	    APS_ERR("call hwmsen_get_interrupt_data fail = %d\n", err);
    	epl_sensor_I2C_Write(epld->client,0x1b, EPL_CMP_RESET | EPL_UN_LOCK);
	}
#endif
}

static void epl_sensor_intr_als_report_lux(void)
{
    struct epl_sensor_priv *obj = epl_sensor_obj;
    u16 als;
    int err = 0;
//#if ALS_DYN_INTT
//    int last_dynamic_intt_idx = dynamic_intt_idx;
//#endif
#if !MTK_LTE
    hwm_sensor_data sensor_data;
#endif

    APS_LOG("[%s]: IDEL MODE \r\n", __func__);
    epl_sensor_I2C_Write(obj->client, 0x00, epl_sensor.wait | EPL_MODE_IDLE);

    als = epl_sensor_get_als_value(obj, epl_sensor.als.data.channels[1]);

    mutex_lock(&sensor_mutex);
    epl_sensor_I2C_Write(obj->client, 0x12, EPL_CMP_RESET | EPL_UN_LOCK);
    epl_sensor_I2C_Write(obj->client, 0x11, EPL_POWER_OFF | EPL_RESETN_RESET);  //After runing CMP_RESET, dont clean interrupt_flag
    mutex_unlock(&sensor_mutex);

//#if ALS_DYN_INTT
//    if(dynamic_intt_idx == last_dynamic_intt_idx) //if dark to sun, it will dont report (L=xxxx, H=65535)
//#endif
    {
        APS_LOG("[%s]: report als = %d \r\n", __func__, als);
#if MTK_LTE
        if((err = als_data_report(alsps_context_obj->idev, als, SENSOR_STATUS_ACCURACY_MEDIUM)))
        {
           APS_ERR("epl_sensor call als_data_report fail = %d\n", err);
        }
#else
        sensor_data.values[0] = als;
    	sensor_data.values[1] = epl_sensor.als.data.channels[1];
    	sensor_data.value_divide = 1;
    	sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
    	if((err = hwmsen_get_interrupt_data(ID_LIGHT, &sensor_data)))
    	{
    	    APS_ERR("get interrupt data failed\n");
    	    APS_ERR("call hwmsen_get_interrupt_data fail = %d\n", err);
    	}
#endif
    }

    if(epl_sensor.als.report_type == CMC_BIT_INTR_LEVEL || epl_sensor.als.report_type == CMC_BIT_DYN_INT)
    {
#if ALS_DYN_INTT
        if(epl_sensor.als.report_type == CMC_BIT_DYN_INT)
        {
            if(dynamic_intt_idx == 0)
            {
                int gain_value = 0;
                int normal_value = 0;
                long low_thd = 0, high_thd = 0;

                if(epl_sensor.als.gain == EPL_GAIN_MID)
                {
                    gain_value = 8;
                }
                else if (epl_sensor.als.gain == EPL_GAIN_LOW)
                {
                    gain_value = 1;
                }
                normal_value = gain_value * als_dynamic_intt_value[dynamic_intt_idx] / als_dynamic_intt_value[1];

                if((als == 0)) //Note: als =0 is nothing
                    low_thd = 0;
                else
                    low_thd = (*(als_adc_intr_level + (als-1)) + 1) * normal_value;
                high_thd = (*(als_adc_intr_level + als)) * normal_value;

                if(low_thd > 0xfffe)
                    low_thd = 65533;
                if(high_thd >= 0xffff)
                    high_thd = 65534;

                epl_sensor.als.low_threshold = low_thd;
                epl_sensor.als.high_threshold = high_thd;
            }
            else
            {
                epl_sensor.als.low_threshold = *(als_adc_intr_level + (als-1)) + 1;
                epl_sensor.als.high_threshold = *(als_adc_intr_level + als);

                if(epl_sensor.als.low_threshold == 0)
                    epl_sensor.als.low_threshold = 1;
            }
            APS_LOG("[%s]:dynamic_intt_idx=%d, thd(%d/%d) \r\n", __func__, dynamic_intt_idx, epl_sensor.als.low_threshold, epl_sensor.als.high_threshold);
        }
        else
#endif
        {
            epl_sensor.als.low_threshold = *(als_adc_intr_level + (als-1)) + 1;
            epl_sensor.als.high_threshold = *(als_adc_intr_level + als);
            if( (als == 0) || (epl_sensor.als.data.channels[1] == 0) )
            {
                epl_sensor.als.low_threshold = 0;
            }
        }
    }
    else
    {
        APS_LOG("[%s]: ALS dont support this als_report_type(%d) \r\n", __func__, epl_sensor.als.report_type );
    }
	//write new threshold
	set_lsensor_intr_threshold(epl_sensor.als.low_threshold, epl_sensor.als.high_threshold);

    mutex_lock(&sensor_mutex);
    epl_sensor_I2C_Write(obj->client, 0x00, epl_sensor.wait | epl_sensor.mode);
    epl_sensor_I2C_Write(obj->client, 0x11, EPL_POWER_ON | EPL_RESETN_RUN);
    mutex_unlock(&sensor_mutex);
    APS_LOG("[%s]: MODE=0x%x \r\n", __func__, epl_sensor.mode);
}
/*----------------------------------------------------------------------------*/

int epl_sensor_read_als(struct i2c_client *client)
{
    struct epl_sensor_priv *obj = i2c_get_clientdata(client);
    u8 buf[5];

    if(client == NULL)
    {
        APS_DBG("CLIENT CANN'T EQUL NULL\n");
        return -1;
    }
    mutex_lock(&sensor_mutex);
    epl_sensor_I2C_Read(obj->client, 0x12, 5);
    buf[0] = gRawData.raw_bytes[0];
    buf[1] = gRawData.raw_bytes[1];
    buf[2] = gRawData.raw_bytes[2];
    buf[3] = gRawData.raw_bytes[3];
    buf[4] = gRawData.raw_bytes[4];
    mutex_unlock(&sensor_mutex);

    epl_sensor.als.saturation = (buf[0] & 0x20);
    epl_sensor.als.compare_high = (buf[0] & 0x10);
    epl_sensor.als.compare_low = (buf[0] & 0x08);
    epl_sensor.als.interrupt_flag = (buf[0] & 0x04);
    epl_sensor.als.compare_reset = (buf[0] & 0x02);
    epl_sensor.als.lock = (buf[0] & 0x01);
    epl_sensor.als.data.channels[0] = (buf[2]<<8) | buf[1];
    epl_sensor.als.data.channels[1] = (buf[4]<<8) | buf[3];

	APS_LOG("als: ~~~~ ALS ~~~~~ \n");
	APS_LOG("als: buf = 0x%x\n", buf[0]);
	APS_LOG("als: sat = 0x%x\n", epl_sensor.als.saturation);
	APS_LOG("als: cmp h = 0x%x, l = %d\n", epl_sensor.als.compare_high, epl_sensor.als.compare_low);
	APS_LOG("als: int_flag = 0x%x\n",epl_sensor.als.interrupt_flag);
	APS_LOG("als: cmp_rstn = 0x%x, lock = 0x%0x\n", epl_sensor.als.compare_reset, epl_sensor.als.lock);
    APS_LOG("read als channel 0 = %d\n", epl_sensor.als.data.channels[0]);
    APS_LOG("read als channel 1 = %d\n", epl_sensor.als.data.channels[1]);
    return 0;
}

/*----------------------------------------------------------------------------*/
int epl_sensor_read_ps(struct i2c_client *client)
{
    u8 buf[5];

    if(client == NULL)
    {
        APS_DBG("CLIENT CANN'T EQUL NULL\n");
        return -1;
    }
    mutex_lock(&sensor_mutex);
    epl_sensor_I2C_Read(client,0x1b, 5);
    buf[0] = gRawData.raw_bytes[0];
    buf[1] = gRawData.raw_bytes[1];
    buf[2] = gRawData.raw_bytes[2];
    buf[3] = gRawData.raw_bytes[3];
    buf[4] = gRawData.raw_bytes[4];
    mutex_unlock(&sensor_mutex);

    epl_sensor.ps.saturation = (buf[0] & 0x20);
    epl_sensor.ps.compare_high = (buf[0] & 0x10);
    epl_sensor.ps.compare_low = (buf[0] & 0x08);
    epl_sensor.ps.interrupt_flag = (buf[0] & 0x04);
    epl_sensor.ps.compare_reset = (buf[0] & 0x02);
    epl_sensor.ps.lock= (buf[0] & 0x01);
    epl_sensor.ps.data.ir_data = (buf[2]<<8) | buf[1];
    epl_sensor.ps.data.data = (buf[4]<<8) | buf[3];

	APS_LOG("ps: ~~~~ PS ~~~~~ \n");
	APS_LOG("ps: buf = 0x%x\n", buf[0]);
	APS_LOG("ps: sat = 0x%x\n", epl_sensor.ps.saturation);
	APS_LOG("ps: cmp h = 0x%x, l = 0x%x\n", epl_sensor.ps.compare_high, epl_sensor.ps.compare_low);
	APS_LOG("ps: int_flag = 0x%x\n",epl_sensor.ps.interrupt_flag);
	APS_LOG("ps: cmp_rstn = 0x%x, lock = %x\n", epl_sensor.ps.compare_reset, epl_sensor.ps.lock);
	APS_LOG("[%s]: data = %d\n", __func__, epl_sensor.ps.data.data);
	APS_LOG("[%s]: ir data = %d\n", __func__, epl_sensor.ps.data.ir_data);
    return 0;
}

#if HS_ENABLE
int epl_sensor_read_hs(struct i2c_client *client)
{
    u8 buf;

    if(client == NULL)
    {
        APS_DBG("CLIENT CANN'T EQUL NULL\n");
        return -1;
    }
    mutex_lock(&hs_sensor_mutex);
    epl_sensor_I2C_Read(client,0x1e, 2);
    epl_sensor.hs.raw = (gRawData.raw_bytes[1]<<8) | gRawData.raw_bytes[0];
    if(epl_sensor.hs.dynamic_intt == true && epl_sensor.hs.raw>epl_sensor.hs.high_threshold && epl_sensor.hs.integration_time > epl_sensor.hs.integration_time_min)
    {
		epl_sensor.hs.integration_time -= 4;
		buf = epl_sensor.hs.integration_time | epl_sensor.hs.gain;
		epl_sensor_I2C_Write(client, 0x03, buf);
    }
    else if(epl_sensor.hs.dynamic_intt == true && epl_sensor.hs.raw>epl_sensor.hs.low_threshold && epl_sensor.hs.raw <epl_sensor.hs.mid_threshold && epl_sensor.hs.integration_time <  epl_sensor.hs.integration_time_max)
	{
		epl_sensor.hs.integration_time += 4;
		buf = epl_sensor.hs.integration_time | epl_sensor.hs.gain;
		epl_sensor_I2C_Write(client, 0x03, buf);
	}
	mutex_unlock(&hs_sensor_mutex);

    if(epl_sensor.hs.raws_count<200)
    {
		epl_sensor.hs.raws[epl_sensor.hs.raws_count] = epl_sensor.hs.raw;
        epl_sensor.hs.raws_count++;
    }
    return 0;
}
#endif

int factory_ps_data(void)
{
    struct epl_sensor_priv *epld = epl_sensor_obj;
    bool enable_ps = test_bit(CMC_BIT_PS, &epld->enable) && atomic_read(&epld->ps_suspend)==0;

#if !PS_DYN_K
    if(enable_ps == 1)
    {
        epl_sensor_read_ps(epld->client);
    }
    else
    {
        APS_LOG("[%s]: ps is disabled \r\n", __func__);
    }
#else
    if(enable_ps == 0)
    {
        APS_LOG("[%s]: ps is disabled \r\n", __func__);
    }
#endif
    APS_LOG("[%s]: enable_ps=%d, ps_raw=%d \r\n", __func__, enable_ps, epl_sensor.ps.data.data);

    return epl_sensor.ps.data.data;
}

int factory_als_data(void)
{
    struct epl_sensor_priv *epld = epl_sensor_obj;
    u16 als_raw = 0;
    bool enable_als = test_bit(CMC_BIT_ALS, &epld->enable) && atomic_read(&epld->als_suspend)==0;

    if(enable_als == 1)
    {
        epl_sensor_read_als(epld->client);

        if(epl_sensor.als.report_type == CMC_BIT_DYN_INT)
        {
            int als_lux=0;
            als_lux = epl_sensor_get_als_value(epld, epl_sensor.als.data.channels[1]);
        }
    }
    else
    {
        APS_LOG("[%s]: als is disabled \r\n", __func__);
    }

    if(epl_sensor.als.report_type == CMC_BIT_DYN_INT)
    {
        als_raw = epl_sensor.als.dyn_intt_raw;
        APS_LOG("[%s]: ALS_DYN_INTT: als_raw=%d \r\n", __func__, als_raw);
    }
    else
    {
        als_raw = epl_sensor.als.data.channels[1];
        APS_LOG("[%s]: als_raw=%d \r\n", __func__, als_raw);
    }

    return als_raw;
}

void epl_sensor_enable_ps(int enable)
{
    struct epl_sensor_priv *epld = epl_sensor_obj;
    bool enable_ps = test_bit(CMC_BIT_PS, &epld->enable);
#if HS_ENABLE
    bool enable_hs = test_bit(CMC_BIT_HS, &epld->enable) && atomic_read(&epld->hs_suspend)==0;
#endif
#if PS_GES
    bool enable_ges = test_bit(CMC_BIT_GES, &epld->enable) && atomic_read(&epld->ges_suspend)==0;
#endif

    APS_LOG("[%s]: ps enable = %d\r\n", __func__, enable);
#if HS_ENABLE
    if(enable_hs == 1 && enable == 1)
    {
        clear_bit(CMC_BIT_HS, &epld->enable);
        if(hs_enable_flag == true)
        {
            set_bit(CMC_BIT_ALS, &epld->enable);
            hs_enable_flag = false;
        }
        write_global_variable(epld->client);
        APS_LOG("[%s] Disable HS and recover ps setting \r\n", __func__);
    }
#endif
#if PS_GES
    if(enable_ges == 1 && enable == 1)
    {
        clear_bit(CMC_BIT_GES, &epld->enable);
        write_global_variable(epld->client);
        ps_ges_enable_flag = true;
        APS_LOG("[%s] Disable GES and recover ps setting \r\n", __func__);
    }
    else if (ps_ges_enable_flag == true && enable == 0)
    {
        set_bit(CMC_BIT_GES, &epld->enable);
        write_global_variable(epld->client);
        ps_ges_enable_flag = false;
        APS_LOG("[%s] enable GES and recover ges setting \r\n", __func__);
    }
#endif

    if(enable_ps != enable)
    {
        if(enable)
        {
            //wake_lock(&ps_lock);
            set_bit(CMC_BIT_PS, &epld->enable);
#if PS_DYN_K
            set_psensor_intr_threshold(65534, 65535);   //dont use first ps status
            dynk_min_ps_raw_data = 0xffff;
            epl_sensor.ps.compare_low = 0x08; //reg0x1b=0x08, FAR
#endif
        }
        else
        {
            clear_bit(CMC_BIT_PS, &epld->enable);
#if PS_DYN_K
            cancel_delayed_work(&epld->dynk_thd_polling_work);
#endif
            //wake_unlock(&ps_lock);
        }
        epl_sensor_fast_update(epld->client);
        epl_sensor_update_mode(epld->client);
    }

}

void epl_sensor_enable_als(int enable)
{
    struct epl_sensor_priv *epld = epl_sensor_obj;
    bool enable_als = test_bit(CMC_BIT_ALS, &epld->enable);

    APS_LOG("[%s]: als enable = %d\r\n", __func__, enable);
    if(enable_als != enable)
    {
        if(enable)
        {
            set_bit(CMC_BIT_ALS, &epld->enable);
#if ALS_DYN_INTT
            if(epl_sensor.als.report_type == CMC_BIT_DYN_INT)
            {
                dynamic_intt_idx = dynamic_intt_init_idx;
                epl_sensor.als.integration_time = als_dynamic_intt_intt[dynamic_intt_idx];
                epl_sensor.als.gain = als_dynamic_intt_gain[dynamic_intt_idx];
                dynamic_intt_high_thr = als_dynamic_intt_high_thr[dynamic_intt_idx];
                dynamic_intt_low_thr = als_dynamic_intt_low_thr[dynamic_intt_idx];
            }
#endif
            epl_sensor_fast_update(epld->client);
        }
        else
        {
            clear_bit(CMC_BIT_ALS, &epld->enable);
        }
        epl_sensor_update_mode(epld->client);
    }
}

#if PS_DYN_K
void epl_sensor_restart_dynk_polling(void)
{
    struct epl_sensor_priv *obj = epl_sensor_obj;
    cancel_delayed_work(&obj->dynk_thd_polling_work);
    schedule_delayed_work(&obj->dynk_thd_polling_work, msecs_to_jiffies(2*dynk_polling_delay));
}

void epl_sensor_dynk_thd_polling_work(struct work_struct *work)
{
    struct epl_sensor_priv *obj = epl_sensor_obj;
    bool enable_ps = test_bit(CMC_BIT_PS, &obj->enable) && atomic_read(&obj->ps_suspend)==0;
    bool enable_als = test_bit(CMC_BIT_ALS, &obj->enable) && atomic_read(&obj->als_suspend)==0;
#if HS_ENABLE
    bool enable_hs = test_bit(CMC_BIT_HS, &obj->enable) && atomic_read(&obj->hs_suspend)==0;
#endif
#if PS_GES
    bool enable_ges = test_bit(CMC_BIT_GES, &obj->enable) && atomic_read(&obj->ges_suspend)==0;
    APS_LOG("[%s]:als / ps / ges enable: %d / %d / %d \n", __func__,enable_als, enable_ps, enable_ges);
#else
    APS_LOG("[%s]:als / ps enable: %d / %d\n", __func__,enable_als, enable_ps);
#endif

#if PS_GES && HS_ENABLE
    if((enable_ps == true  || enable_ges == true) && enable_hs == false)
#elif PS_GES
    if(enable_ps == true  || enable_ges == true)
#else
    if(enable_ps == true)
#endif
    {
        epl_sensor_read_ps(obj->client);
#if PS_GES
        if(enable_ges == true && epl_sensor.ges.polling_mode == 1)
        {
            epl_sensor_report_ges_status((epl_sensor.ps.compare_low >> 3));
            last_ges_state = (epl_sensor.ps.compare_low >> 3);
        }
#endif
        if( (dynk_min_ps_raw_data > epl_sensor.ps.data.data)
            && (epl_sensor.ps.saturation == 0)
            && (epl_sensor.ps.data.ir_data < dynk_max_ir_data) )
        {
            dynk_min_ps_raw_data = epl_sensor.ps.data.data;
            if(enable_ps == true)
            {
                dynk_thd_low = dynk_min_ps_raw_data + dynk_low_offset;
    		    dynk_thd_high = dynk_min_ps_raw_data + dynk_high_offset;
    		    if(dynk_thd_low>65534)
                    dynk_thd_low = 65534;
                if(dynk_thd_high>65535)
                    dynk_thd_high = 65535;
		    }
#if PS_GES
            else
            {
                dynk_thd_low = dynk_min_ps_raw_data + (dynk_low_offset / 2);
                dynk_thd_high = dynk_min_ps_raw_data + (dynk_high_offset / 2);
                if(dynk_thd_low>65534)
                    dynk_thd_low = 65534;
                if(dynk_thd_high>65535)
                    dynk_thd_high = 65535;
                epl_sensor.ges.low_threshold = (u16)dynk_thd_low;
                epl_sensor.ges.high_threshold = (u16)dynk_thd_high;
            }
#endif
		    set_psensor_intr_threshold((u16)dynk_thd_low, (u16)dynk_thd_high);
		    APS_LOG("[%s]:dyn ps raw = %d, min = %d, ir_data = %d\n", __func__, epl_sensor.ps.data.data, dynk_min_ps_raw_data, epl_sensor.ps.data.ir_data);
		    APS_LOG("[%s]:dyn k thre_l = %d, thre_h = %d\n", __func__, (u16)dynk_thd_low, (u16)dynk_thd_high);
        }
        schedule_delayed_work(&obj->dynk_thd_polling_work, msecs_to_jiffies(dynk_polling_delay));
    }
}
#endif /*PS_DYN_K*/

//for 3637
static int als_sensing_time(int intt, int adc, int cycle)
{
    long sensing_us_time;
    int sensing_ms_time;
    int als_intt, als_adc, als_cycle;

    als_intt = als_intt_value[intt>>2];
    als_adc = adc_value[adc>>3];
    als_cycle = cycle_value[cycle];
    APS_LOG("ALS: INTT=%d, ADC=%d, Cycle=%d \r\n", als_intt, als_adc, als_cycle);

    sensing_us_time = (als_intt + als_adc*2*2) * 2 * als_cycle;
    sensing_ms_time = sensing_us_time / 1000;
    APS_LOG("[%s]: sensing=%d ms \r\n", __func__, sensing_ms_time);
    return (sensing_ms_time + 5);
}

static int ps_sensing_time(int intt, int adc, int cycle)
{
    long sensing_us_time;
    int sensing_ms_time;
    int ps_intt, ps_adc, ps_cycle;

    ps_intt = ps_intt_value[intt>>2];
    ps_adc = adc_value[adc>>3];
    ps_cycle = cycle_value[cycle];
    APS_LOG("PS: INTT=%d, ADC=%d, Cycle=%d \r\n", ps_intt, ps_adc, ps_cycle);

    sensing_us_time = (ps_intt*3 + ps_adc*2*3) * ps_cycle;
    sensing_ms_time = sensing_us_time / 1000;
    APS_LOG("[%s]: sensing=%d ms\r\n", __func__, sensing_ms_time);
    return (sensing_ms_time + 5);
}

#if HS_ENABLE || PS_GES
void epl_sensor_restart_polling(void)
{
    struct epl_sensor_priv *obj = epl_sensor_obj;

    cancel_delayed_work(&obj->polling_work);
    schedule_delayed_work(&obj->polling_work, msecs_to_jiffies(50));
}

void epl_sensor_polling_work(struct work_struct *work)
{
    struct epl_sensor_priv *obj = epl_sensor_obj;
#if HS_ENABLE
    bool enable_hs = test_bit(CMC_BIT_HS, &obj->enable) && atomic_read(&obj->hs_suspend)==0;
#endif
#if PS_GES
    bool enable_ges = test_bit(CMC_BIT_GES, &obj->enable) && atomic_read(&obj->ges_suspend)==0;
#endif

#if HS_ENABLE
    APS_LOG("[%s]: enable_hs=%d \r\n", __func__, enable_hs);
#endif
#if PS_GES
    APS_LOG("[%s]: enable_ges=%d \r\n", __func__, enable_ges);
#endif
    cancel_delayed_work(&obj->polling_work);

#if HS_ENABLE
    if(enable_hs)
    {
		schedule_delayed_work(&obj->polling_work, msecs_to_jiffies(20));
        epl_sensor_read_hs(obj->client);
    }
#if PS_GES
    else if (enable_ges && epl_sensor.ges.polling_mode==1)
#endif /*PS_GES*/
#endif /*HS_ENABLE*/

#if PS_GES
#if !HS_ENABLE /*HS_ENABLE*/
    if (enable_ges && epl_sensor.ges.polling_mode==1)
#endif  /*HS_ENABLE*/
    {
        schedule_delayed_work(&obj->polling_work, msecs_to_jiffies(200));
        epl_sensor_read_ps(obj->client);
        epl_sensor_report_ges_status((epl_sensor.ps.compare_low >> 3));
        last_ges_state = (epl_sensor.ps.compare_low >> 3);

    }
#endif
}
#endif  /*HS_ENABLE || PS_GES*/

void epl_sensor_fast_update(struct i2c_client *client)
{
    int als_fast_time = 0;

    APS_FUN();
    mutex_lock(&sensor_mutex);
    als_fast_time = als_sensing_time(epl_sensor.als.integration_time, epl_sensor.als.adc, EPL_CYCLE_1);

    epl_sensor_I2C_Write(client, 0x11, EPL_POWER_OFF | EPL_RESETN_RESET);
    epl_sensor_I2C_Write(client, 0x02, epl_sensor.als.adc | EPL_CYCLE_1);
    epl_sensor_I2C_Write(client, 0x01, epl_sensor.als.integration_time | epl_sensor.als.gain);
    epl_sensor_I2C_Write(client, 0x00, epl_sensor.wait | EPL_MODE_ALS);
    epl_sensor_I2C_Write(client, 0x11, EPL_POWER_ON | EPL_RESETN_RUN);
    mutex_unlock(&sensor_mutex);

    msleep(als_fast_time);
    APS_LOG("[%s]: msleep(%d)\r\n", __func__, als_fast_time);

    mutex_lock(&sensor_mutex);
    if(epl_sensor.als.polling_mode == 0)
    {
        //fast_mode is already ran one frame, so must to reset CMP bit for als intr mode
        //IDLE mode and CMMP reset
        epl_sensor_I2C_Write(client, 0x00, epl_sensor.wait | EPL_MODE_IDLE);
        epl_sensor_I2C_Write(client, 0x12, EPL_CMP_RESET | EPL_UN_LOCK);
        epl_sensor_I2C_Write(client, 0x12, EPL_CMP_RUN | EPL_UN_LOCK);
    }

    epl_sensor_I2C_Write(client, 0x11, EPL_POWER_OFF | EPL_RESETN_RESET);
    epl_sensor_I2C_Write(client, 0x02, epl_sensor.als.adc | epl_sensor.als.cycle);
    //epl_sensor_I2C_Write(client, 0x11, EPL_POWER_ON | EPL_RESETN_RUN);
    mutex_unlock(&sensor_mutex);
}

void epl_sensor_update_mode(struct i2c_client *client)
{
    struct epl_sensor_priv *obj = epl_sensor_obj;
    int als_time = 0, ps_time = 0;
    bool enable_ps = test_bit(CMC_BIT_PS, &obj->enable) && atomic_read(&obj->ps_suspend)==0;
    bool enable_als = test_bit(CMC_BIT_ALS, &obj->enable) && atomic_read(&obj->als_suspend)==0;
#if HS_ENABLE
	bool enable_hs = test_bit(CMC_BIT_HS, &obj->enable) && atomic_read(&obj->hs_suspend)==0;
#endif
#if PS_GES
    bool enable_ges = test_bit(CMC_BIT_GES, &obj->enable) && atomic_read(&obj->ges_suspend)==0;
#endif
    als_frame_time = als_time = als_sensing_time(epl_sensor.als.integration_time, epl_sensor.als.adc, epl_sensor.als.cycle);
    ps_frame_time = ps_time = ps_sensing_time(epl_sensor.ps.integration_time, epl_sensor.ps.adc, epl_sensor.ps.cycle);

    mutex_lock(&sensor_mutex);
#if PS_GES
    APS_LOG("mode selection =0x%x\n", (enable_ps|enable_ges) | (enable_als << 1));
#else
	APS_LOG("mode selection =0x%x\n", enable_ps | (enable_als << 1));
#endif

#if HS_ENABLE
    if(enable_hs)
    {
        APS_LOG("[%s]: HS mode \r\n", __func__);
        epl_sensor_I2C_Write(client, 0x11, EPL_POWER_ON | EPL_RESETN_RUN);
        epl_sensor_restart_polling();
    }
	else
#endif
	{   /*PS_GES and HS_ENABLE*/
#if PS_GES
        //**** mode selection ****
        switch((enable_als << 1) | (enable_ps|enable_ges))
#else
        //**** mode selection ****
        switch((enable_als << 1) | enable_ps)
#endif
        {
            case 0: //disable all
                epl_sensor.mode = EPL_MODE_IDLE;
                break;
            case 1: //als = 0, ps = 1
                epl_sensor.mode = EPL_MODE_PS;
             break;
            case 2: //als = 1, ps = 0
                epl_sensor.mode = EPL_MODE_ALS;
                break;
            case 3: //als = 1, ps = 1
                epl_sensor.mode = EPL_MODE_ALS_PS;
                break;
        }
        epl_sensor_I2C_Write(client, 0x11, EPL_POWER_OFF | EPL_RESETN_RESET);
        // initial factory calibration variable
        read_factory_calibration();

        epl_sensor_I2C_Write(obj->client, 0x00, epl_sensor.wait | epl_sensor.mode);

        if(epl_sensor.mode != EPL_MODE_IDLE)    // if mode isnt IDLE, PWR_ON and RUN
            epl_sensor_I2C_Write(client, 0x11, EPL_POWER_ON | EPL_RESETN_RUN);

        //**** check setting ****
        if(enable_ps == 1)
        {
            APS_LOG("[%s] PS:low_thd = %d, high_thd = %d \n",__func__, epl_sensor.ps.low_threshold, epl_sensor.ps.high_threshold);
        }
        if(enable_als == 1 && epl_sensor.als.polling_mode == 0)
        {
            APS_LOG("[%s] ALS:low_thd = %d, high_thd = %d \n",__func__, epl_sensor.als.low_threshold, epl_sensor.als.high_threshold);
        }
#if PS_GES
        if(enable_ges)
        {
            APS_LOG("[%s] GES:low_thd = %d, high_thd = %d \n",__func__, epl_sensor.ges.low_threshold, epl_sensor.ges.high_threshold);

        }
#endif
    	APS_LOG("[%s] reg0x00= 0x%x\n", __func__,  epl_sensor.wait | epl_sensor.mode);
    	APS_LOG("[%s] reg0x07= 0x%x\n", __func__, epl_sensor.als.interrupt_channel_select | epl_sensor.als.persist | epl_sensor.als.interrupt_type);
    	APS_LOG("[%s] reg0x06= 0x%x\n", __func__, epl_sensor.interrupt_control | epl_sensor.ps.persist |epl_sensor.ps.interrupt_type);
    	APS_LOG("[%s] reg0x11= 0x%x\n", __func__, epl_sensor.power | epl_sensor.reset);
    	APS_LOG("[%s] reg0x12= 0x%x\n", __func__, epl_sensor.als.compare_reset | epl_sensor.als.lock);
    	APS_LOG("[%s] reg0x1b= 0x%x\n", __func__, epl_sensor.ps.compare_reset | epl_sensor.ps.lock);
    }   /*PS_GES and HS_ENABLE*/

#if PS_DYN_K
#if !PS_GES
    if(enable_ps == 1)
#elif PS_GES
    if(enable_ps == 1 || enable_ges == 1)
#endif
    {
        epl_sensor_restart_dynk_polling();
    }
#endif

#if (PS_GES && !PS_DYN_K)
    if(enable_ges == 1 && epl_sensor.ges.polling_mode == 1)
    {
        epl_sensor_restart_polling();
    }
#endif
    mutex_unlock(&sensor_mutex);
}


/*----------------------------------------------------------------------------*/
static irqreturn_t epl_sensor_eint_func(int irq, void *desc)
{
    struct epl_sensor_priv *obj = g_epl2590_ptr;

    // APS_LOG(" interrupt fuc\n");

    int_top_time = sched_clock();

    if(!obj)
    {
        return IRQ_HANDLED;
    }

    disable_irq_nosync(epl_sensor_obj->irq);

    schedule_delayed_work(&obj->eint_work, 0);

    return IRQ_HANDLED;
}

/*----------------------------------------------------------------------------*/
static void epl_sensor_eint_work(struct work_struct *work)
{
    struct epl_sensor_priv *obj = epl_sensor_obj;
    bool enable_ps = test_bit(CMC_BIT_PS, &obj->enable) && atomic_read(&obj->ps_suspend)==0;
#if PS_GES
    bool enable_ges = test_bit(CMC_BIT_GES, &obj->enable) && atomic_read(&obj->ges_suspend)==0;
#endif

    APS_LOG("xxxxxxxxxxx\n\n");

    epl_sensor_read_ps(obj->client);
    epl_sensor_read_als(obj->client);
    if(epl_sensor.ps.interrupt_flag == EPL_INT_TRIGGER)
    {
#if PS_GES
        if(enable_ges) //epl_sensor_report_ges_status();
        {
            epl_sensor_report_ges_status( (epl_sensor.ps.compare_low >> 3) );
        }
#endif
        if(enable_ps)
        {
            wake_lock_timeout(&ps_lock, 2*HZ);
            epl_sensor_report_ps_status();
        }
        //PS unlock interrupt pin and restart chip
		mutex_lock(&sensor_mutex);
		epl_sensor_I2C_Write(obj->client,0x1b, EPL_CMP_RUN | EPL_UN_LOCK);
		mutex_unlock(&sensor_mutex);
    }

    if(epl_sensor.als.interrupt_flag == EPL_INT_TRIGGER)
    {
        epl_sensor_intr_als_report_lux();
        //ALS unlock interrupt pin and restart chip
    	mutex_lock(&sensor_mutex);
    	epl_sensor_I2C_Write(obj->client,0x12, EPL_CMP_RUN | EPL_UN_LOCK);
    	mutex_unlock(&sensor_mutex);
    }

#if EINT_API
#if defined(CONFIG_OF)
	enable_irq(obj->irq);
#endif
#else/*EINT_API*/
#if defined(MT6575) || defined(MT6571) || defined(MT6589)
    mt65xx_eint_unmask(CUST_EINT_ALS_NUM);
#else
    mt_eint_unmask(CUST_EINT_ALS_NUM);
#endif
#endif/*EINT_API*/
}

int epl_sensor_setup_eint(struct i2c_client *client)
{
    #ifdef CUSTOM_KERNEL_SENSORHUB
	int err = 0;

	err = SCP_sensorHub_rsp_registration(ID_PROXIMITY, alsps_irq_handler);
    #else				/* #ifdef CUSTOM_KERNEL_SENSORHUB */
	struct epl_sensor_priv *obj = i2c_get_clientdata(client);
	int ret;
	u32 ints[2] = { 0, 0 };
	struct pinctrl *pinctrl;
	struct pinctrl_state *pins_default;
	struct pinctrl_state *pins_cfg;
	struct platform_device *alsps_pdev = get_alsps_platformdev();

	APS_LOG("epl_sensor_setup_eint\n");

       g_epl2590_ptr = obj;
	   /*configure to GPIO function, external interrupt */
	 #ifndef FPGA_EARLY_PORTING
/* gpio setting */
	pinctrl = devm_pinctrl_get(&alsps_pdev->dev);
	if (IS_ERR(pinctrl)) {
		ret = PTR_ERR(pinctrl);
		APS_ERR("Cannot find alsps pinctrl!\n");
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

	}
	pinctrl_select_state(pinctrl, pins_cfg);

	if (epl_sensor_obj->irq_node) {
		of_property_read_u32_array(epl_sensor_obj->irq_node, "debounce", ints,
					   ARRAY_SIZE(ints));
		gpio_request(ints[0], "p-sensor");
		gpio_set_debounce(ints[0], ints[1]);
		APS_LOG("ints[0] = %d, ints[1] = %d!!\n", ints[0], ints[1]);

		epl_sensor_obj->irq = irq_of_parse_and_map(epl_sensor_obj->irq_node, 0);
		APS_LOG("epl2590_obj->irq = %d\n", epl_sensor_obj->irq);
		if (!epl_sensor_obj->irq) {
			APS_ERR("irq_of_parse_and_map fail!!\n");
			return -EINVAL;
		}
		if (request_irq
		    (epl_sensor_obj->irq, epl_sensor_eint_func, IRQF_TRIGGER_NONE, "ALS-eint", NULL)) {
			APS_ERR("IRQ LINE NOT AVAILABLE!!\n");
			return -EINVAL;
		}
		enable_irq(epl_sensor_obj->irq);
	}
	else
	{
		APS_ERR("null irq node!!\n");
		return -EINVAL;
	}

	enable_irq(epl_sensor_obj->irq);
#endif				/* #ifndef FPGA_EARLY_PORTING */
#endif				/* #ifdef CUSTOM_KERNEL_SENSORHUB */

	return 0;

	
}

static int epl_sensor_init_client(struct i2c_client *client)
{
    struct epl_sensor_priv *obj = i2c_get_clientdata(client);
    int err=0;
    /*  interrupt mode */
    APS_LOG("I2C Addr==[0x%x],line=%d\n", epl_sensor_i2c_client->addr, __LINE__);

    if(obj->hw->polling_mode_ps == 0)
    {
#if !EINT_API
#if defined(MT6575) || defined(MT6571) || defined(MT6589)
        mt65xx_eint_unmask(CUST_EINT_ALS_NUM);
#else
        mt_eint_unmask(CUST_EINT_ALS_NUM);
#endif
#endif/*EINT_API*/
        if((err = epl_sensor_setup_eint(client)))
        {
            APS_ERR("setup eint: %d\n", err);
            return err;
        }
        APS_LOG("epl_sensor interrupt setup\n");
    }

    if((err = hw8k_init_device(client)) != 0)
    {
        APS_ERR("init dev: %d\n", err);
        return err;
    }

    return err;
}


/*----------------------------------------------------------------------------*/
static ssize_t epl_sensor_show_reg(struct device_driver *ddri, char *buf)
{
    ssize_t len = 0;
    struct i2c_client *client = epl_sensor_obj->client;

    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x00 value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x00));
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x01 value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x01));
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x02 value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x02));
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x03 value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x03));
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x04 value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x04));
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x05 value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x05));
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x06 value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x06));
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x07 value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x07));
    if(epl_sensor.als.polling_mode == 0)
    {
        len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x08 value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x08));
        len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x09 value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x09));
        len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x0A value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x0A));
        len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x0B value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x0B));
    }
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x0C value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x0C));
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x0D value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x0D));
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x0E value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x0E));
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x0F value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x0F));
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x11 value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x11));
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x12 value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x12));
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x13 value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x13));
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x14 value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x14));
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x15 value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x15));
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x16 value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x16));
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x1B value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x1B));
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x1C value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x1C));
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x1D value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x1D));
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x1E value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x1E));
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x1F value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x1F));
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x22 value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x22));
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x23 value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x23));
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x24 value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x24));
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0x25 value = 0x%x\n", i2c_smbus_read_byte_data(client, 0x25));
    len += snprintf(buf+len, PAGE_SIZE-len, "chip id REG 0xFC value = 0x%x\n", i2c_smbus_read_byte_data(client, 0xFC));
    return len;
}

static ssize_t epl_sensor_show_status(struct device_driver *ddri, char *buf)
{
    ssize_t len = 0;
    struct epl_sensor_priv *epld = epl_sensor_obj;
    bool enable_ps = test_bit(CMC_BIT_PS, &epld->enable) && atomic_read(&epld->ps_suspend)==0;
    bool enable_als = test_bit(CMC_BIT_ALS, &epld->enable) && atomic_read(&epld->als_suspend)==0;
#if HS_ENABLE
	bool enable_hs = test_bit(CMC_BIT_HS, &epld->enable) && atomic_read(&epld->hs_suspend)==0;
#endif

    if(!epl_sensor_obj)
    {
        APS_ERR("epl_sensor_obj is null!!\n");
        return 0;
    }

    len += snprintf(buf+len, PAGE_SIZE-len, "chip is %s, ver is %s \n", EPL_DEV_NAME, DRIVER_VERSION);
    len += snprintf(buf+len, PAGE_SIZE-len, "als/ps polling is %d-%d\n", epl_sensor.als.polling_mode, epl_sensor.ps.polling_mode);
    len += snprintf(buf+len, PAGE_SIZE-len, "wait = %d, mode = %d\n",epl_sensor.wait >> 4, epl_sensor.mode);
    len += snprintf(buf+len, PAGE_SIZE-len, "interrupt control = %d\n", epl_sensor.interrupt_control >> 4);
    len += snprintf(buf+len, PAGE_SIZE-len, "frame time ps=%dms, als=%dms\n", ps_frame_time, als_frame_time);
#if HS_ENABLE
    if(enable_hs)
    {
        len += snprintf(buf+len, PAGE_SIZE-len, "hs adc= %d\n", epl_sensor.hs.adc>>3);
        len += snprintf(buf+len, PAGE_SIZE-len, "hs int_time= %d\n", epl_sensor.hs.integration_time>>2);
        len += snprintf(buf+len, PAGE_SIZE-len, "hs cycle= %d\n", epl_sensor.hs.cycle);
        len += snprintf(buf+len, PAGE_SIZE-len, "hs gain= %d\n", epl_sensor.hs.gain);
        len += snprintf(buf+len, PAGE_SIZE-len, "hs ch1 raw= %d\n", epl_sensor.hs.raw);
    }
#endif
    if(enable_ps)
    {
        len += snprintf(buf+len, PAGE_SIZE-len, "PS: \n");
        len += snprintf(buf+len, PAGE_SIZE-len, "INTEG = %d, gain = %d\n", epl_sensor.ps.integration_time >> 2, epl_sensor.ps.gain);
        len += snprintf(buf+len, PAGE_SIZE-len, "ADC = %d, cycle = %d, ir drive = %d\n", epl_sensor.ps.adc >> 3, epl_sensor.ps.cycle, epl_sensor.ps.ir_drive);
        len += snprintf(buf+len, PAGE_SIZE-len, "saturation = %d, int flag = %d\n", epl_sensor.ps.saturation >> 5, epl_sensor.ps.interrupt_flag >> 2);
        len += snprintf(buf+len, PAGE_SIZE-len, "Thr(L/H) = (%d/%d)\n", epl_sensor.ps.low_threshold, epl_sensor.ps.high_threshold);
#if PS_DYN_K
        len += snprintf(buf+len, PAGE_SIZE-len, "Dyn thr(L/H) = (%d/%d)\n", (u16)dynk_thd_low, (u16)dynk_thd_high);
#endif
        len += snprintf(buf+len, PAGE_SIZE-len, "pals data = %d, data = %d\n", epl_sensor.ps.data.ir_data, epl_sensor.ps.data.data);
    }
    if(enable_als)
    {
        len += snprintf(buf+len, PAGE_SIZE-len, "ALS: \n");
        len += snprintf(buf+len, PAGE_SIZE-len, "INTEG = %d, gain = %d\n", epl_sensor.als.integration_time >> 2, epl_sensor.als.gain);
        len += snprintf(buf+len, PAGE_SIZE-len, "ADC = %d, cycle = %d\n", epl_sensor.als.adc >> 3, epl_sensor.als.cycle);
#if ALS_DYN_INTT
    if(epl_sensor.als.report_type == CMC_BIT_DYN_INT)
    {
        len += snprintf(buf+len, PAGE_SIZE-len, "c_gain = %d\n", c_gain);
        len += snprintf(buf+len, PAGE_SIZE-len, "dyn_intt_raw=%d, dynamic_intt_lux=%d\n", epl_sensor.als.dyn_intt_raw, dynamic_intt_lux);
    }
#endif
    if(epl_sensor.als.polling_mode == 0)
        len += snprintf(buf+len, PAGE_SIZE-len, "Thr(L/H) = (%d/%d)\n", epl_sensor.als.low_threshold, epl_sensor.als.high_threshold);
    len += snprintf(buf+len, PAGE_SIZE-len, "ch0 = %d, ch1 = %d\n", epl_sensor.als.data.channels[0], epl_sensor.als.data.channels[1]);
    }

    return len;
}

static ssize_t epl_sensor_store_als_enable(struct device_driver *ddri, const char *buf, size_t count)
{
    uint16_t mode=0;

    APS_FUN();
    sscanf(buf, "%hu", &mode);
    APS_LOG("[%s]: als enable=%d \r\n", __func__, mode);
    epl_sensor_enable_als(mode);

    return count;
}

static ssize_t epl_sensor_store_ps_enable(struct device_driver *ddri, const char *buf, size_t count)
{
    uint16_t mode=0;
    APS_FUN();

    sscanf(buf, "%hu", &mode);

    APS_LOG("[%s]: ps enable=%d \r\n", __func__, mode);
    epl_sensor_enable_ps(mode);

    return count;
}

static ssize_t epl_sensor_show_cal_raw(struct device_driver *ddri, char *buf)
{
    struct epl_sensor_priv *obj = epl_sensor_obj;
    u16 ch1=0;
    size_t len = 0;
    if(!obj)
    {
        APS_ERR("epl_sensor_obj is null!!\n");
        return 0;
    }

    switch(epl_sensor.mode)
    {
        case EPL_MODE_PS:
            ch1 = factory_ps_data();
        break;

        case EPL_MODE_ALS:
            ch1 = factory_als_data();
            break;
    }
    APS_LOG("cal_raw = %d \r\n" , ch1);
    len += snprintf(buf + len, PAGE_SIZE - len, "%d \r\n", ch1);

    return  len;
}


/*----------------------------------------------------------------------------*/
static ssize_t epl_sensor_store_threshold(struct device_driver *ddri,const char *buf, size_t count)
{
    struct epl_sensor_priv *obj = epl_sensor_obj;
    int low, high;

    APS_FUN();
    if(!obj)
    {
        APS_ERR("epl_sensor_obj is null!!\n");
        return 0;
    }
    sscanf(buf, "%d,%d", &low, &high);

    switch(epl_sensor.mode)
    {
        case EPL_MODE_PS:
            obj->hw->ps_threshold_low = low;
            obj->hw->ps_threshold_high = high;
            epl_sensor.ps.low_threshold = low;
            epl_sensor.ps.high_threshold = high;
            set_psensor_intr_threshold(epl_sensor.ps.low_threshold, epl_sensor.ps.high_threshold);
            break;

        case EPL_MODE_ALS:
            obj->hw->als_threshold_low = low;
            obj->hw->als_threshold_high = high;
            epl_sensor.als.low_threshold = low;
            epl_sensor.als.high_threshold = high;
            set_lsensor_intr_threshold(epl_sensor.als.low_threshold, epl_sensor.als.high_threshold);
            break;

    }
    return  count;
}

static ssize_t epl_sensor_show_threshold(struct device_driver *ddri, char *buf)
{
    struct epl_sensor_priv *obj = epl_sensor_obj;
    ssize_t len = 0;

    if(!obj)
    {
        APS_ERR("epl_sensor_obj is null!!\n");
        return 0;
    }

    switch(epl_sensor.mode)
    {
        case EPL_MODE_PS:
            len += snprintf(buf + len, PAGE_SIZE - len, "obj->hw->ps_threshold_low=%d \r\n", obj->hw->ps_threshold_low);
            len += snprintf(buf + len, PAGE_SIZE - len, "obj->hw->ps_threshold_high=%d \r\n", obj->hw->ps_threshold_high);
            break;

        case EPL_MODE_ALS:
            len += snprintf(buf + len, PAGE_SIZE - len, "obj->hw->als_threshold_low=%d \r\n", obj->hw->als_threshold_low);
            len += snprintf(buf + len, PAGE_SIZE - len, "obj->hw->als_threshold_high=%d \r\n", obj->hw->als_threshold_high);
            break;

    }
    return  len;

}

/*----------------------------------------------------------------------------*/
static ssize_t epl_sensor_store_wait_time(struct device_driver *ddri, const char *buf, size_t count)
{
    int val;

    sscanf(buf, "%d",&val);
    epl_sensor.wait = (val & 0xf) << 4;

    return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t epl_sensor_store_gain(struct device_driver *ddri, const char *buf, size_t count)
{
	struct epl_sensor_priv *epld = epl_sensor_obj;
	int value = 0;

    APS_FUN();
    sscanf(buf, "%d", &value);
    value = value & 0x03;

	switch (epl_sensor.mode)
	{
        case EPL_MODE_PS:
            epl_sensor.ps.gain = value;
	        epl_sensor_I2C_Write(epld->client, 0x03, epl_sensor.ps.integration_time | epl_sensor.ps.gain);
		break;

        case EPL_MODE_ALS: //als
            epl_sensor.als.gain = value;
	        epl_sensor_I2C_Write(epld->client, 0x01, epl_sensor.als.integration_time | epl_sensor.als.gain);
		break;

    }
	epl_sensor_update_mode(epld->client);

	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t epl_sensor_store_mode(struct device_driver *ddri, const char *buf, size_t count)
{
    struct epl_sensor_priv *obj = epl_sensor_obj;
    int value=0;

    APS_FUN();
    clear_bit(CMC_BIT_PS, &obj->enable);
    clear_bit(CMC_BIT_ALS, &obj->enable);

    sscanf(buf, "%d",&value);
    switch (value)
    {
        case 0:
            epl_sensor.mode = EPL_MODE_IDLE;
            break;

        case 1:
            //set_bit(CMC_BIT_ALS, &obj->enable);
            epl_sensor.mode = EPL_MODE_ALS;
            break;

        case 2:
            //set_bit(CMC_BIT_PS, &obj->enable);
            epl_sensor.mode = EPL_MODE_PS;
            break;

        case 3:
            //set_bit(CMC_BIT_ALS, &obj->enable);
            //set_bit(CMC_BIT_PS, &obj->enable);
            epl_sensor.mode = EPL_MODE_ALS_PS;
            break;
    }

    //epl_sensor_update_mode(obj->client);
    return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t epl_sensor_store_ir_mode(struct device_driver *ddri, const char *buf, size_t count)
{
    int value=0;
    struct epl_sensor_priv *obj = epl_sensor_obj;

    APS_FUN();
    sscanf(buf, "%d", &value);

    switch (epl_sensor.mode)
    {
        case EPL_MODE_PS:
            switch(value)
            {
                case 0:
                    epl_sensor.ps.ir_mode = EPL_IR_MODE_CURRENT;
                    break;

                case 1:
                    epl_sensor.ps.ir_mode = EPL_IR_MODE_VOLTAGE;
                    break;
            }

            epl_sensor_I2C_Write(obj->client, 0x05, epl_sensor.ps.ir_on_control | epl_sensor.ps.ir_mode | epl_sensor.ps.ir_drive);
         break;
    }
    epl_sensor_I2C_Write(obj->client, 0x00, epl_sensor.wait | epl_sensor.mode);

    return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t epl_sensor_store_ir_contrl(struct device_driver *ddri, const char *buf, size_t count)
{
    int value=0;
    uint8_t  data;
    struct epl_sensor_priv *obj = epl_sensor_obj;

    APS_FUN();
    sscanf(buf, "%d",&value);

    switch (epl_sensor.mode)
    {
        case EPL_MODE_PS:
            switch(value)
            {
                case 0:
                    epl_sensor.ps.ir_on_control = EPL_IR_ON_CTRL_OFF;
                    break;

                case 1:
                    epl_sensor.ps.ir_on_control = EPL_IR_ON_CTRL_ON;
                    break;
            }

            data = epl_sensor.ps.ir_on_control | epl_sensor.ps.ir_mode | epl_sensor.ps.ir_drive;
            APS_LOG("%s: 0x05 = 0x%x\n", __FUNCTION__, data);

            epl_sensor_I2C_Write(obj->client, 0x05, epl_sensor.ps.ir_on_control | epl_sensor.ps.ir_mode | epl_sensor.ps.ir_drive);
        break;
    }
    epl_sensor_I2C_Write(obj->client, 0x00, epl_sensor.wait | epl_sensor.mode);

    return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t epl_sensor_store_ir_drive(struct device_driver *ddri, const char *buf, size_t count)
{
    int value=0;
    struct epl_sensor_priv *obj = epl_sensor_obj;

    APS_FUN();
    sscanf(buf, "%d", &value);

    switch(epl_sensor.mode)
    {
        case EPL_MODE_PS:
        epl_sensor.ps.ir_drive = (value & 0x03);
        epl_sensor_I2C_Write(obj->client, 0x05, epl_sensor.ps.ir_on_control | epl_sensor.ps.ir_mode | epl_sensor.ps.ir_drive);
        break;
    }
    epl_sensor_I2C_Write(obj->client, 0x00, epl_sensor.wait | epl_sensor.mode);

    return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t epl_sensor_store_interrupt_type(struct device_driver *ddri, const char *buf, size_t count)
{
    int value=0;
    struct epl_sensor_priv *obj = epl_sensor_obj;

    APS_FUN();
    sscanf(buf, "%d",&value);

    switch (epl_sensor.mode)
    {
        case EPL_MODE_PS:
            if(!obj->hw->polling_mode_ps)
            {
                epl_sensor.ps.interrupt_type = value & 0x03;
                epl_sensor_I2C_Write(obj->client, 0x06, epl_sensor.interrupt_control | epl_sensor.ps.persist |epl_sensor.ps.interrupt_type);
                APS_LOG("%s: 0x06 = 0x%x\n", __FUNCTION__, epl_sensor.interrupt_control | epl_sensor.ps.persist |epl_sensor.ps.interrupt_type);
            }
            break;

        case EPL_MODE_ALS: //als
            if(!obj->hw->polling_mode_als)
            {
                epl_sensor.als.interrupt_type = value & 0x03;
                epl_sensor_I2C_Write(obj->client, 0x07, epl_sensor.als.interrupt_channel_select | epl_sensor.als.persist | epl_sensor.als.interrupt_type);
                APS_LOG("%s: 0x07 = 0x%x\n", __FUNCTION__, epl_sensor.als.interrupt_channel_select | epl_sensor.als.persist | epl_sensor.als.interrupt_type);
            }
            break;
    }
    epl_sensor_update_mode(obj->client);

    return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t epl_sensor_store_integration(struct device_driver *ddri, const char *buf, size_t count)
{
    int value=0;
#if ALS_DYN_INTT
    int value1=0;
#endif
    struct epl_sensor_priv *obj = epl_sensor_obj;

    APS_FUN();
    sscanf(buf, "%d",&value);

    switch (epl_sensor.mode)
    {
        case EPL_MODE_PS:
            epl_sensor.ps.integration_time = (value & 0xf) << 2;
            epl_sensor_I2C_Write(obj->client, 0x03, epl_sensor.ps.integration_time | epl_sensor.ps.gain);
            epl_sensor_I2C_Read(obj->client, 0x03, 1);
            APS_LOG("%s: 0x03 = 0x%x (0x%x)\n", __FUNCTION__, epl_sensor.ps.integration_time | epl_sensor.ps.gain, gRawData.raw_bytes[0]);
            break;

        case EPL_MODE_ALS: //als
#if ALS_DYN_INTT
            if(epl_sensor.als.report_type == CMC_BIT_DYN_INT)
            {
                sscanf(buf, "%d,%d",&value, &value1);

                als_dynamic_intt_intt[0] = (value & 0xf) << 2;
                als_dynamic_intt_value[0] = als_intt_value[value];

                als_dynamic_intt_intt[1] = (value1 & 0xf) << 2;
                als_dynamic_intt_value[1] = als_intt_value[value1];
                APS_LOG("[%s]: als_dynamic_intt_value=%d,%d \r\n", __func__, als_dynamic_intt_value[0], als_dynamic_intt_value[1]);

                dynamic_intt_idx = dynamic_intt_init_idx;
                epl_sensor.als.integration_time = als_dynamic_intt_intt[dynamic_intt_idx];
                epl_sensor.als.gain = als_dynamic_intt_gain[dynamic_intt_idx];
                dynamic_intt_high_thr = als_dynamic_intt_high_thr[dynamic_intt_idx];
                dynamic_intt_low_thr = als_dynamic_intt_low_thr[dynamic_intt_idx];
            }
            else
#endif
            {
                epl_sensor.als.integration_time = (value & 0xf) << 2;
                epl_sensor_I2C_Write(obj->client, 0x01, epl_sensor.als.integration_time | epl_sensor.als.gain);
                epl_sensor_I2C_Read(obj->client, 0x01, 1);
                APS_LOG("%s: 0x01 = 0x%x (0x%x)\n", __FUNCTION__, epl_sensor.als.integration_time | epl_sensor.als.gain, gRawData.raw_bytes[0]);
            }
            break;

    }
    epl_sensor_update_mode(obj->client);

    return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t epl_sensor_store_adc(struct device_driver *ddri, const char *buf, size_t count)
{
    int value=0;
    struct epl_sensor_priv *obj = epl_sensor_obj;

    APS_FUN();
    sscanf(buf, "%d",&value);

    switch (epl_sensor.mode)
    {
        case EPL_MODE_PS:
            epl_sensor.ps.adc = (value & 0x3) << 3;
            epl_sensor_I2C_Write(obj->client, 0x04, epl_sensor.ps.adc | epl_sensor.ps.cycle);
            epl_sensor_I2C_Read(obj->client, 0x04, 1);
            APS_LOG("%s: 0x04 = 0x%x (0x%x)\n", __FUNCTION__, epl_sensor.ps.adc | epl_sensor.ps.cycle, gRawData.raw_bytes[0]);
            break;

        case EPL_MODE_ALS: //als
            epl_sensor.als.adc = (value & 0x3) << 3;
            epl_sensor_I2C_Write(obj->client, 0x02, epl_sensor.als.adc | epl_sensor.als.cycle);
            epl_sensor_I2C_Read(obj->client, 0x02, 1);
            APS_LOG("%s: 0x02 = 0x%x (0x%x)\n", __FUNCTION__, epl_sensor.als.adc | epl_sensor.als.cycle, gRawData.raw_bytes[0]);
            break;
    }
    epl_sensor_update_mode(obj->client);

    return count;
}

static ssize_t epl_sensor_store_cycle(struct device_driver *ddri, const char *buf, size_t count)
{
    int value=0;
    struct epl_sensor_priv *obj = epl_sensor_obj;

    APS_FUN();
    sscanf(buf, "%d",&value);

    switch (epl_sensor.mode)
    {
        case EPL_MODE_PS:
            epl_sensor.ps.cycle = (value & 0x7);
            epl_sensor_I2C_Write(obj->client, 0x04, epl_sensor.ps.adc | epl_sensor.ps.cycle);
            epl_sensor_I2C_Read(obj->client, 0x04, 1);
            APS_LOG("%s: 0x04 = 0x%x (0x%x)\n", __FUNCTION__, epl_sensor.ps.adc | epl_sensor.ps.cycle, gRawData.raw_bytes[0]);
            break;

        case EPL_MODE_ALS: //als
            epl_sensor.als.cycle = (value & 0x7);
            epl_sensor_I2C_Write(obj->client, 0x02, epl_sensor.als.adc | epl_sensor.als.cycle);
            epl_sensor_I2C_Read(obj->client, 0x02, 1);
            APS_LOG("%s: 0x02 = 0x%x (0x%x)\n", __FUNCTION__, epl_sensor.als.adc | epl_sensor.als.cycle, gRawData.raw_bytes[0]);
            break;
    }
    epl_sensor_update_mode(obj->client);

    return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t epl_sensor_store_als_report_type(struct device_driver *ddri, const char *buf, size_t count)
{
    int value=0;
    struct epl_sensor_priv *epld = epl_sensor_obj;

    APS_FUN();
    sscanf(buf, "%d", &value);
    epl_sensor.als.report_type = value & 0xf;

    if(epl_sensor.als.polling_mode == 0)
        als_intr_update_table(epld);

    return count;
}


static ssize_t epl_sensor_store_ps_polling_mode(struct device_driver *ddri, const char *buf, size_t count)
{
    struct epl_sensor_priv *epld = epl_sensor_obj;
#if !MTK_LTE
    struct hwmsen_object obj_ps;
#endif

    sscanf(buf, "%d", &epld->hw->polling_mode_ps);
    APS_LOG("epld->hw->polling_mode_ps=%d \r\n", epld->hw->polling_mode_ps);
    epl_sensor.ps.polling_mode = epld->hw->polling_mode_ps;

#if !MTK_LTE
    hwmsen_detach(ID_PROXIMITY);
    obj_ps.self = epl_sensor_obj;
    obj_ps.polling = epld->hw->polling_mode_ps;
    obj_ps.sensor_operate = epl_sensor_ps_operate;

    if(hwmsen_attach(ID_PROXIMITY, &obj_ps))
    {
        APS_ERR("[%s]: attach fail !\n", __FUNCTION__);
    }
#else
    alsps_context_obj->ps_ctl.is_polling_mode = epl_sensor.ps.polling_mode==1? true:false;
    alsps_context_obj->ps_ctl.is_report_input_direct = epl_sensor.ps.polling_mode==0? true:false;
#endif  /*!MTK_LTE*/

#if PS_GES
    epl_sensor.ges.polling_mode = epld->hw->polling_mode_ps;
#endif
    set_als_ps_intr_type(epld->client, epl_sensor.ps.polling_mode, epl_sensor.als.polling_mode);
    epl_sensor_I2C_Write(epld->client, 0x06, epl_sensor.interrupt_control | epl_sensor.ps.persist |epl_sensor.ps.interrupt_type);
    epl_sensor_I2C_Write(epld->client, 0x07, epl_sensor.als.interrupt_channel_select | epl_sensor.als.persist | epl_sensor.als.interrupt_type);

    epl_sensor_fast_update(epld->client);
    epl_sensor_update_mode(epld->client);

    return count;
}

static ssize_t epl_sensor_store_als_polling_mode(struct device_driver *ddri, const char *buf, size_t count)
{
    struct epl_sensor_priv *epld = epl_sensor_obj;
#if !MTK_LTE
    struct hwmsen_object obj_als;
#endif

    sscanf(buf, "%d",&epld->hw->polling_mode_als);
    APS_LOG("epld->hw->polling_mode_als=%d \r\n", epld->hw->polling_mode_als);
    epl_sensor.als.polling_mode = epld->hw->polling_mode_als;

#if !MTK_LTE
    hwmsen_detach(ID_LIGHT);
    obj_als.self = epl_sensor_obj;
    obj_als.polling = epld->hw->polling_mode_als;
    obj_als.sensor_operate = epl_sensor_als_operate;

    if(hwmsen_attach(ID_LIGHT, &obj_als))
    {
        APS_ERR("[%s]: attach fail !\n", __FUNCTION__);
    }
#else
    alsps_context_obj->als_ctl.is_polling_mode = epl_sensor.als.polling_mode==1? true:false;
    alsps_context_obj->als_ctl.is_report_input_direct = epl_sensor.als.polling_mode==0? true:false;
#endif /*!MTK_LTE*/

    //update als intr table
    if(epl_sensor.als.polling_mode == 0)
        als_intr_update_table(epld);

    set_als_ps_intr_type(epld->client, epl_sensor.ps.polling_mode, epl_sensor.als.polling_mode);
    epl_sensor_I2C_Write(epld->client, 0x06, epl_sensor.interrupt_control | epl_sensor.ps.persist |epl_sensor.ps.interrupt_type);
    epl_sensor_I2C_Write(epld->client, 0x07, epl_sensor.als.interrupt_channel_select | epl_sensor.als.persist | epl_sensor.als.interrupt_type);

    epl_sensor_fast_update(epld->client);
    epl_sensor_update_mode(epld->client);

    return count;
}
static ssize_t epl_sensor_store_ps_w_calfile(struct device_driver *ddri, const char *buf, size_t count)
{
	struct epl_sensor_priv *epld = epl_sensor_obj;
    int ps_hthr=0, ps_lthr=0, ps_cancelation=0;
    int ps_cal_len = 0;
    char ps_calibration[20];

	APS_FUN();
	if(!epl_sensor_obj)
    {
        APS_ERR("epl_obj is null!!\n");
        return 0;
    }
    sscanf(buf, "%d,%d,%d",&ps_cancelation, &ps_hthr, &ps_lthr);

    ps_cal_len = sprintf(ps_calibration, "%d,%d,%d",  ps_cancelation, ps_hthr, ps_lthr);

    write_factory_calibration(epld, ps_calibration, ps_cal_len);

	return count;
}
/*----------------------------------------------------------------------------*/

static ssize_t epl_sensor_store_unlock(struct device_driver *ddri, const char *buf, size_t count)
{
	struct epl_sensor_priv *epld = epl_sensor_obj;
    int mode;

    APS_FUN();
    sscanf(buf, "%d",&mode);

    APS_LOG("mode = %d \r\n", mode);
	switch (mode)
	{
		case 0: //PS unlock and run
        	epl_sensor.ps.compare_reset = EPL_CMP_RUN;
        	epl_sensor.ps.lock = EPL_UN_LOCK;
        	epl_sensor_I2C_Write(epld->client,0x1b, epl_sensor.ps.compare_reset |epl_sensor.ps.lock);
		break;

		case 1: //PS unlock and reset
        	epl_sensor.ps.compare_reset = EPL_CMP_RESET;
        	epl_sensor.ps.lock = EPL_UN_LOCK;
        	epl_sensor_I2C_Write(epld->client,0x1b, epl_sensor.ps.compare_reset |epl_sensor.ps.lock);
		break;

		case 2: //ALS unlock and run
        	epl_sensor.als.compare_reset = EPL_CMP_RUN;
        	epl_sensor.als.lock = EPL_UN_LOCK;
        	epl_sensor_I2C_Write(epld->client,0x12, epl_sensor.als.compare_reset | epl_sensor.als.lock);
		break;

        case 3: //ALS unlock and reset
    		epl_sensor.als.compare_reset = EPL_CMP_RESET;
        	epl_sensor.als.lock = EPL_UN_LOCK;
        	epl_sensor_I2C_Write(epld->client,0x12, epl_sensor.als.compare_reset | epl_sensor.als.lock);
        break;

		case 4: //ps+als
		    //PS unlock and run
        	epl_sensor.ps.compare_reset = EPL_CMP_RUN;
        	epl_sensor.ps.lock = EPL_UN_LOCK;
        	epl_sensor_I2C_Write(epld->client,0x1b, epl_sensor.ps.compare_reset |epl_sensor.ps.lock);

			//ALS unlock and run
        	epl_sensor.als.compare_reset = EPL_CMP_RUN;
        	epl_sensor.als.lock = EPL_UN_LOCK;
        	epl_sensor_I2C_Write(epld->client,0x12, epl_sensor.als.compare_reset | epl_sensor.als.lock);
		break;
	}
    /*double check PS or ALS lock*/

	return count;
}

static ssize_t epl_sensor_store_als_ch_sel(struct device_driver *ddri, const char *buf, size_t count)
{
	struct epl_sensor_priv *epld = epl_sensor_obj;
    int ch_sel;

    APS_FUN();
    sscanf(buf, "%d",&ch_sel);
    APS_LOG("channel selection = %d \r\n", ch_sel);

	switch (ch_sel)
	{
		case 0: //ch0
		    epl_sensor.als.interrupt_channel_select = EPL_ALS_INT_CHSEL_0;
		break;

		case 1: //ch1
        	epl_sensor.als.interrupt_channel_select = EPL_ALS_INT_CHSEL_1;
		break;
	}
    epl_sensor_I2C_Write(epld->client,0x07, epl_sensor.als.interrupt_channel_select | epl_sensor.als.persist | epl_sensor.als.interrupt_type);
    epl_sensor_update_mode(epld->client);

	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t epl_sensor_store_als_lux_per_count(struct device_driver *ddri, const char *buf, size_t count)
{
    int lux_per_count = 0;

    sscanf(buf, "%d",&lux_per_count);
    epl_sensor.als.factory.lux_per_count = lux_per_count;

    return count;
}

static ssize_t epl_sensor_store_ps_cancelation(struct device_driver *ddri, const char *buf, size_t count)
{
	struct epl_sensor_priv *epld = epl_sensor_obj;
    int cancelation;

    APS_FUN();
    sscanf(buf, "%d",&cancelation);
    epl_sensor.ps.cancelation = cancelation;
    APS_LOG("epl_sensor.ps.cancelation = %d \r\n", epl_sensor.ps.cancelation);

    epl_sensor_I2C_Write(epld->client,0x22, (u8)(epl_sensor.ps.cancelation& 0xff));
    epl_sensor_I2C_Write(epld->client,0x23, (u8)((epl_sensor.ps.cancelation & 0xff00) >> 8));

	return count;
}

static ssize_t epl_sensor_show_ps_polling(struct device_driver *ddri, char *buf)
{
    u16 *tmp = (u16*)buf;

    tmp[0]= epl_sensor.ps.polling_mode;

    return 2;
}

static ssize_t epl_sensor_show_als_polling(struct device_driver *ddri, char *buf)
{
    u16 *tmp = (u16*)buf;

    tmp[0]= epl_sensor.als.polling_mode;

    return 2;
}

static ssize_t epl_sensor_show_ps_run_cali(struct device_driver *ddri, char *buf)
{
	struct epl_sensor_priv *epld = epl_sensor_obj;
	ssize_t len = 0;
    int ret;

    APS_FUN();
    ret = epl_run_ps_calibration(epld);
    len += snprintf(buf+len, PAGE_SIZE-len, "ret = %d\r\n", ret);

	return len;
}

static ssize_t epl_sensor_show_pdata(struct device_driver *ddri, char *buf)
{
    ssize_t len = 0;
    int ps_raw;

    ps_raw = factory_ps_data();
    APS_LOG("[%s]: ps_raw=%d \r\n", __func__, ps_raw);
    len += snprintf(buf + len, PAGE_SIZE - len, "%d", ps_raw);

    return len;
}

static ssize_t epl_sensor_show_als_data(struct device_driver *ddri, char *buf)
{
    ssize_t len = 0;
    u16 als_raw = 0;

    als_raw = factory_als_data();
    APS_LOG("[%s]: als_raw=%d \r\n", __func__, als_raw);
    len += snprintf(buf + len, PAGE_SIZE - len, "%d", als_raw);

    return len;
}

#if PS_DYN_K
static ssize_t epl_sensor_store_dyn_offset(struct device_driver *ddri, const char *buf, size_t count)
{
    int dyn_h,dyn_l;

    APS_FUN();
    sscanf(buf, "%d,%d",&dyn_l, &dyn_h);
    dynk_low_offset = dyn_l;
    dynk_high_offset = dyn_h;

    return count;
}
static ssize_t epl_sensor_store_dyn_max_ir_data(struct device_driver *ddri, const char *buf, size_t count)
{
    int max_ir_data;

    APS_FUN();
    sscanf(buf, "%d",&max_ir_data);
    dynk_max_ir_data = max_ir_data;

    return count;
}
#endif

#if ALS_DYN_INTT
static ssize_t epl_sensor_store_c_gain(struct device_driver *ddri, const char *buf, size_t count)
{
    int als_c_gain;

    APS_FUN();
    sscanf(buf, "%d",&als_c_gain);
    c_gain = als_c_gain;
    APS_LOG("c_gain = %d \r\n", c_gain);

	return count;
}
#endif

static ssize_t epl_sensor_store_reg_write(struct device_driver *ddri, const char *buf, size_t count)
{
    struct epl_sensor_priv *epld = epl_sensor_obj;
    int reg;
    int data;

    APS_FUN();
    sscanf(buf, "%x,%x",&reg, &data);
    APS_LOG("[%s]: reg=0x%x, data=0x%x", __func__, reg, data);

    if(reg == 0x00 && ((data & 0x0f) == EPL_MODE_PS || (data & 0x0f) == EPL_MODE_ALS_PS))
    {
        set_bit(CMC_BIT_PS, &epld->enable);
    }
    else
    {
        clear_bit(CMC_BIT_PS, &epld->enable);
    }
    epl_sensor_I2C_Write(epld->client, reg, data);

    return count;
}


static ssize_t epl_sensor_show_renvo(struct device_driver *ddri, char *buf)
{
    ssize_t len = 0;

    APS_FUN();
    APS_LOG("gRawData.renvo=0x%x \r\n", epl_sensor.revno);
    len += snprintf(buf+len, PAGE_SIZE-len, "%x", epl_sensor.revno);

    return len;
}
#if HS_ENABLE
static ssize_t epl_sensor_store_hs_enable(struct device_driver *ddri, const char *buf, size_t count)
{
    uint16_t mode=0;
    struct epl_sensor_priv *obj = epl_sensor_obj;
	bool enable_als = test_bit(CMC_BIT_ALS, &obj->enable) && atomic_read(&obj->als_suspend)==0;

    APS_FUN();
    sscanf(buf, "%hu",&mode);

    if(mode > 0)
	{
	    if(enable_als == 1)
	    {
            clear_bit(CMC_BIT_ALS, &obj->enable);
            hs_enable_flag = true;
	    }
		epl_sensor.hs.integration_time = epl_sensor.hs.integration_time_max;
		epl_sensor.hs.raws_count=0;
        set_bit(CMC_BIT_HS, &obj->enable);

        if(mode == 2)
        {
            epl_sensor.hs.dynamic_intt = false;
        }
        else
        {
            epl_sensor.hs.dynamic_intt = true;
        }
	}
    else
	{
        clear_bit(CMC_BIT_HS, &obj->enable);
        if(hs_enable_flag == true)
        {
            set_bit(CMC_BIT_ALS, &obj->enable);
            hs_enable_flag = false;
        }

	}
	write_global_variable(obj->client);
    epl_sensor_update_mode(obj->client);

    return count;
}

static ssize_t epl_sensor_store_hs_int_time(struct device_driver *ddri, const char *buf, size_t count)
{
    struct epl_sensor_priv *obj = epl_sensor_obj;
	int value;
	u8 intt_buf;

    sscanf(buf, "%d",&value);

	mutex_lock(&hs_sensor_mutex);
	epl_sensor.hs.integration_time = value<<2;
	intt_buf = epl_sensor.hs.integration_time | epl_sensor.hs.gain;
	epl_sensor_I2C_Write(obj->client, 0x03, intt_buf);
	mutex_unlock(&hs_sensor_mutex);

    return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t epl_sensor_show_hs_raws(struct device_driver *ddri, char *buf)
{
    u16 *tmp = (u16*)buf;
    int byte_count=2+epl_sensor.hs.raws_count*2;
    int i=0;

    APS_FUN();
    mutex_lock(&hs_sensor_mutex);
    tmp[0]= epl_sensor.hs.raws_count;

    for(i=0; i<epl_sensor.hs.raws_count; i++){
        tmp[i+1] = epl_sensor.hs.raws[i];
    }
    epl_sensor.hs.raws_count=0;
    mutex_unlock(&hs_sensor_mutex);

    return byte_count;
}
#endif

#if PS_GES
static ssize_t epl_sensor_store_ges_enable(struct device_driver *ddri, const char *buf, size_t count)
{
    int ges_enable=0;
    struct epl_sensor_priv *obj = epl_sensor_obj;
    bool enable_ps = test_bit(CMC_BIT_PS, &obj->enable) && atomic_read(&obj->ps_suspend)==0;

    APS_FUN();
    sscanf(buf, "%d",&ges_enable);
    APS_LOG("[%s]: enable_ps=%d, ges_enable=%d \r\n", __func__, enable_ps, ges_enable);

    if(enable_ps == 0)
    {
        if(ges_enable == 1)
        {
            set_bit(CMC_BIT_GES, &obj->enable);
#if PS_DYN_K
            dynk_min_ps_raw_data = 0xffff;
#endif
        }
        else
        {
            clear_bit(CMC_BIT_GES, &obj->enable);
        }
    }
    write_global_variable(obj->client);
    epl_sensor_update_mode(obj->client);

    return count;
}

static ssize_t epl_sensor_store_ges_polling_mode(struct device_driver *ddri, const char *buf, size_t count)
{
    int ges_mode=0;
    struct epl_sensor_priv *obj = epl_sensor_obj;

    APS_FUN();
    sscanf(buf, "%d",&ges_mode);
    APS_LOG("[%s]: ges_mode=%d \r\n", __func__, ges_mode);

    epl_sensor.ges.polling_mode = ges_mode;
    set_als_ps_intr_type(obj->client, epl_sensor.ges.polling_mode, epl_sensor.als.polling_mode);
    epl_sensor_I2C_Write(obj->client, 0x06, epl_sensor.interrupt_control | epl_sensor.ps.persist |epl_sensor.ps.interrupt_type);

    return count;
}

static ssize_t epl_sensor_store_ges_thd(struct device_driver *ddri, const char *buf, size_t count)
{
    int ges_l=0, ges_h=0;
    struct epl_sensor_priv *obj = epl_sensor_obj;

    APS_FUN();
    sscanf(buf, "%d,%d",&ges_l, &ges_h);
    epl_sensor.ges.low_threshold = ges_l;
    epl_sensor.ges.high_threshold = ges_h;
    APS_LOG("[%s]: ges_thd=%d,%d \r\n", __func__, epl_sensor.ges.low_threshold, epl_sensor.ges.high_threshold);

    write_global_variable(obj->client);
    epl_sensor_update_mode(obj->client);

    return count;
}

#endif


static ssize_t epl_sensor_show_als_intr_table(struct device_driver *ddri, char *buf)
{
    ssize_t len = 0;
    int idx = 0;

    len += snprintf(buf+len, PAGE_SIZE-len, "ALS Lux Table\n");
    for (idx = 0; idx < 10; idx++)
    {
        len += snprintf(buf+len, PAGE_SIZE-len, "[%d]: %ld", idx, (*(als_lux_intr_level + idx)));
	}

    len += snprintf(buf+len, PAGE_SIZE-len, "\n ALS ADC Table\n");
    for (idx = 0; idx < 10; idx++)
    {
        len += snprintf(buf+len, PAGE_SIZE-len, "[%d]: %ld", idx, (*(als_adc_intr_level + idx)));
	}

    return len;
}

/*CTS --> S_IWUSR | S_IRUGO*/
/*----------------------------------------------------------------------------*/
static DRIVER_ATTR(elan_status, S_IWUSR | S_IRUGO, epl_sensor_show_status, NULL);
static DRIVER_ATTR(elan_reg, S_IWUSR | S_IRUGO, epl_sensor_show_reg,NULL);
static DRIVER_ATTR(als_enable, S_IWUSR | S_IRUGO, NULL, epl_sensor_store_als_enable);
static DRIVER_ATTR(als_report_type, S_IWUSR | S_IRUGO, NULL, epl_sensor_store_als_report_type);
static DRIVER_ATTR(als_polling_mode,	 S_IWUSR | S_IRUGO, epl_sensor_show_als_polling, epl_sensor_store_als_polling_mode);
static DRIVER_ATTR(als_lux_per_count, S_IWUSR | S_IRUGO, NULL, epl_sensor_store_als_lux_per_count	);
static DRIVER_ATTR(ps_enable, S_IWUSR | S_IRUGO, NULL, epl_sensor_store_ps_enable);
static DRIVER_ATTR(ps_polling_mode,	S_IWUSR | S_IRUGO, epl_sensor_show_ps_polling, epl_sensor_store_ps_polling_mode);
static DRIVER_ATTR(ir_mode, S_IWUSR | S_IRUGO, NULL, epl_sensor_store_ir_mode);
static DRIVER_ATTR(ir_drive, S_IWUSR | S_IRUGO, NULL, epl_sensor_store_ir_drive);
static DRIVER_ATTR(ir_on, S_IWUSR | S_IRUGO, NULL, epl_sensor_store_ir_contrl);
static DRIVER_ATTR(interrupt_type, S_IWUSR | S_IRUGO, NULL, epl_sensor_store_interrupt_type);
static DRIVER_ATTR(integration, S_IWUSR | S_IRUGO, NULL, epl_sensor_store_integration);
static DRIVER_ATTR(gain, S_IWUSR | S_IRUGO, NULL, epl_sensor_store_gain);
static DRIVER_ATTR(adc, S_IWUSR | S_IRUGO, NULL, epl_sensor_store_adc);
static DRIVER_ATTR(cycle, S_IWUSR | S_IRUGO, NULL, epl_sensor_store_cycle);
static DRIVER_ATTR(mode, S_IWUSR | S_IRUGO, NULL, epl_sensor_store_mode);
static DRIVER_ATTR(wait_time, S_IWUSR | S_IRUGO, NULL, epl_sensor_store_wait_time);
static DRIVER_ATTR(set_threshold, S_IWUSR | S_IRUGO, epl_sensor_show_threshold, epl_sensor_store_threshold);
static DRIVER_ATTR(cal_raw, S_IWUSR | S_IRUGO, epl_sensor_show_cal_raw, NULL);
static DRIVER_ATTR(unlock, S_IWUSR | S_IRUGO, NULL, epl_sensor_store_unlock);
static DRIVER_ATTR(als_ch, S_IWUSR | S_IRUGO, NULL, epl_sensor_store_als_ch_sel);
static DRIVER_ATTR(ps_cancel, S_IWUSR | S_IRUGO, NULL, epl_sensor_store_ps_cancelation);
static DRIVER_ATTR(run_ps_cali, S_IWUSR | S_IRUGO, epl_sensor_show_ps_run_cali, NULL);
static DRIVER_ATTR(pdata, S_IWUSR | S_IRUGO, epl_sensor_show_pdata, NULL);
static DRIVER_ATTR(als_data, S_IWUSR | S_IRUGO, epl_sensor_show_als_data, NULL);
static DRIVER_ATTR(ps_w_calfile, S_IWUSR | S_IRUGO, NULL, epl_sensor_store_ps_w_calfile);
#if PS_DYN_K
static DRIVER_ATTR(dyn_offset, S_IWUSR | S_IRUGO, NULL, epl_sensor_store_dyn_offset);
static DRIVER_ATTR(dyn_max_ir_data, S_IWUSR | S_IRUGO, NULL, epl_sensor_store_dyn_max_ir_data);
#endif
#if ALS_DYN_INTT
static DRIVER_ATTR(als_dyn_c_gain, S_IWUSR | S_IRUGO, NULL, epl_sensor_store_c_gain);
#endif
static DRIVER_ATTR(i2c_w, S_IWUSR | S_IRUGO, NULL, epl_sensor_store_reg_write);
static DRIVER_ATTR(elan_renvo, S_IWUSR | S_IRUGO, epl_sensor_show_renvo, NULL);
#if HS_ENABLE
static DRIVER_ATTR(hs_enable, S_IWUSR | S_IRUGO, NULL, epl_sensor_store_hs_enable);
static DRIVER_ATTR(hs_int_time, S_IWUSR | S_IRUGO, NULL, epl_sensor_store_hs_int_time);
static DRIVER_ATTR(hs_raws, S_IWUSR | S_IRUGO, epl_sensor_show_hs_raws, NULL);
#endif
#if PS_GES
static DRIVER_ATTR(ges_enable, S_IWUSR | S_IRUGO, NULL, epl_sensor_store_ges_enable);
static DRIVER_ATTR(ges_polling_mode, S_IWUSR | S_IRUGO, NULL, epl_sensor_store_ges_polling_mode);
static DRIVER_ATTR(ges_thd, S_IWUSR | S_IRUGO, NULL, epl_sensor_store_ges_thd);
#endif
static DRIVER_ATTR(als_intr_table, S_IWUSR | S_IRUGO, epl_sensor_show_als_intr_table, NULL);


/*----------------------------------------------------------------------------*/
static struct driver_attribute * epl_sensor_attr_list[] =
{
    &driver_attr_elan_status,
    &driver_attr_elan_reg,
    &driver_attr_als_enable,
    &driver_attr_als_report_type,
    &driver_attr_als_polling_mode,
    &driver_attr_als_lux_per_count,
    &driver_attr_ps_enable,
    &driver_attr_ps_polling_mode,
    &driver_attr_elan_renvo,
#if HS_ENABLE
    &driver_attr_hs_enable,
	&driver_attr_hs_int_time,
    &driver_attr_hs_raws,
#endif
    &driver_attr_mode,
    &driver_attr_ir_mode,
    &driver_attr_ir_drive,
    &driver_attr_ir_on,
    &driver_attr_interrupt_type,
    &driver_attr_cal_raw,
    &driver_attr_set_threshold,
    &driver_attr_wait_time,
    &driver_attr_integration,
    &driver_attr_gain,
    &driver_attr_adc,
    &driver_attr_cycle,
    &driver_attr_unlock,
    &driver_attr_ps_cancel,
    &driver_attr_als_ch,
    &driver_attr_run_ps_cali,
    &driver_attr_pdata,
    &driver_attr_als_data,
    &driver_attr_ps_w_calfile,
#if PS_DYN_K
    &driver_attr_dyn_offset,
    &driver_attr_dyn_max_ir_data,
#endif
#if ALS_DYN_INTT
    &driver_attr_als_dyn_c_gain,
#endif
    &driver_attr_i2c_w,
#if PS_GES
    &driver_attr_ges_enable,
    &driver_attr_ges_polling_mode,
    &driver_attr_ges_thd,
#endif
    &driver_attr_als_intr_table,
};

/*----------------------------------------------------------------------------*/
static int epl_sensor_create_attr(struct device_driver *driver)
{
    int idx, err = 0;
    int num = (int)(sizeof(epl_sensor_attr_list)/sizeof(epl_sensor_attr_list[0]));
    if (driver == NULL)
    {
        return -EINVAL;
    }
    for(idx = 0; idx < num; idx++)
    {
        if((err = driver_create_file(driver, epl_sensor_attr_list[idx])))
        {
            APS_ERR("driver_create_file (%s) = %d\n", epl_sensor_attr_list[idx]->attr.name, err);
            break;
        }
    }
    return err;
}



/*----------------------------------------------------------------------------*/
static int epl_sensor_delete_attr(struct device_driver *driver)
{
    int idx ,err = 0;
    int num = (int)(sizeof(epl_sensor_attr_list)/sizeof(epl_sensor_attr_list[0]));

    if (!driver)
        return -EINVAL;

    for (idx = 0; idx < num; idx++)
    {
        driver_remove_file(driver, epl_sensor_attr_list[idx]);
    }

    return err;
}

/******************************************************************************
 * Function Configuration
******************************************************************************/
static int epl_sensor_open(struct inode *inode, struct file *file)
{
    file->private_data = epl_sensor_i2c_client;

    APS_FUN();

    if (!file->private_data)
    {
        APS_ERR("null pointer!!\n");
        return -EINVAL;
    }

    return nonseekable_open(inode, file);
}

/*----------------------------------------------------------------------------*/
static int epl_sensor_release(struct inode *inode, struct file *file)
{
    APS_FUN();
    file->private_data = NULL;
    return 0;
}

/*----------------------------------------------------------------------------*/
static long epl_sensor_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct i2c_client *client = (struct i2c_client*)file->private_data;
    struct epl_sensor_priv *obj = i2c_get_clientdata(client);
    int err=0, ps_result=0, threshold[2], als_data=0;
    void __user *ptr = (void __user*) arg;
    int dat;
    uint32_t enable;
#if 0
    int ps_cali;
#endif
    APS_LOG("%s cmd = 0x%04x", __FUNCTION__, cmd);
    switch (cmd)
    {
        case ALSPS_SET_PS_MODE:
            if(copy_from_user(&enable, ptr, sizeof(enable)))
            {
                err = -EFAULT;
                goto err_out;
            }

            APS_LOG("[%s]: ps enable=%d \r\n", __func__, enable);
            epl_sensor_enable_ps(enable);
        break;

        case ALSPS_GET_PS_MODE:
            enable=test_bit(CMC_BIT_PS, &obj->enable);
            if(copy_to_user(ptr, &enable, sizeof(enable)))
            {
                err = -EFAULT;
                goto err_out;
            }
        break;

        case ALSPS_GET_PS_DATA:

            factory_ps_data();
            dat = epl_sensor.ps.compare_low >> 3;

            APS_LOG("ioctl ps state value = %d \n", dat);

            if(copy_to_user(ptr, &dat, sizeof(dat)))
            {
                err = -EFAULT;
                goto err_out;
            }
        break;

        case ALSPS_GET_PS_RAW_DATA:

            dat = factory_ps_data();

            APS_LOG("ioctl ps raw value = %d \n", dat);

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
            APS_LOG("[%s]: als enable=%d \r\n", __func__, enable);
            epl_sensor_enable_als(enable);
        break;

        case ALSPS_GET_ALS_MODE:
            enable=test_bit(CMC_BIT_ALS, &obj->enable);
            if(copy_to_user(ptr, &enable, sizeof(enable)))
            {
                err = -EFAULT;
                goto err_out;
            }
        break;

        case ALSPS_GET_ALS_DATA:
            als_data = factory_als_data();
            dat = epl_sensor_get_als_value(obj, als_data);

            APS_LOG("ioctl get als data = %d\n", dat);

            if(copy_to_user(ptr, &dat, sizeof(dat)))
            {
                err = -EFAULT;
                goto err_out;
            }
        break;

        case ALSPS_GET_ALS_RAW_DATA:
            dat = factory_als_data();
            APS_LOG("ioctl get als raw data = %d\n", dat);

            if(copy_to_user(ptr, &dat, sizeof(dat)))
            {
                err = -EFAULT;
                goto err_out;
            }
        break;
/*----------------------------------for factory mode test---------------------------------------*/
		case ALSPS_GET_PS_TEST_RESULT:

            dat = factory_ps_data();
            if(dat > obj->hw->ps_threshold_high)
			{
			    ps_result = 0;
			}
			else
			    ps_result = 1;

			APS_LOG("[%s] ps_result = %d \r\n", __func__, ps_result);

			if(copy_to_user(ptr, &ps_result, sizeof(ps_result)))
			{
				err = -EFAULT;
				goto err_out;
			}
		break;
#if 0 //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

		case ALSPS_IOCTL_CLR_CALI:
#if 0
			if(copy_from_user(&dat, ptr, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}
			if(dat == 0)
				obj->ps_cali = 0;
#else

            APS_LOG("[%s]: ALSPS_IOCTL_CLR_CALI \r\n", __func__);
#endif
			break;

		case ALSPS_IOCTL_GET_CALI:
#if 0
			ps_cali = obj->ps_cali ;
			APS_ERR("%s set ps_calix%x\n", __func__, obj->ps_cali);
			if(copy_to_user(ptr, &ps_cali, sizeof(ps_cali)))
			{
				err = -EFAULT;
				goto err_out;
			}
#else
            APS_LOG("[%s]: ALSPS_IOCTL_GET_CALI \r\n", __func__);
#endif
			break;

		case ALSPS_IOCTL_SET_CALI:
#if 0
			if(copy_from_user(&ps_cali, ptr, sizeof(ps_cali)))
			{
				err = -EFAULT;
				goto err_out;
			}

			obj->ps_cali = ps_cali;
			APS_ERR("%s set ps_calix%x\n", __func__, obj->ps_cali);
#else
            APS_LOG("[%s]: ALSPS_IOCTL_SET_CALI \r\n", __func__);
#endif
			break;

		case ALSPS_SET_PS_THRESHOLD:
			if(copy_from_user(threshold, ptr, sizeof(threshold)))
			{
				err = -EFAULT;
				goto err_out;
			}
#if 0
			APS_ERR("%s set threshold high: 0x%x, low: 0x%x\n", __func__, threshold[0],threshold[1]);
			atomic_set(&obj->ps_thd_val_high,  (threshold[0]+obj->ps_cali));
			atomic_set(&obj->ps_thd_val_low,  (threshold[1]+obj->ps_cali));//need to confirm
			set_psensor_threshold(obj->client);
#else
            APS_LOG("[%s] set threshold high: %d, low: %d\n", __func__, threshold[0],threshold[1]);
            obj->hw->ps_threshold_high = threshold[0];
            obj->hw->ps_threshold_low = threshold[1];
            set_psensor_intr_threshold(obj->hw->ps_threshold_low, obj->hw->ps_threshold_high);
#endif
			break;
#endif //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
		case ALSPS_GET_PS_THRESHOLD_HIGH:
#if 0
			APS_ERR("%s get threshold high before cali: 0x%x\n", __func__, atomic_read(&obj->ps_thd_val_high));
			threshold[0] = atomic_read(&obj->ps_thd_val_high) - obj->ps_cali;
			APS_ERR("%s set ps_calix%x\n", __func__, obj->ps_cali);
			APS_ERR("%s get threshold high: 0x%x\n", __func__, threshold[0]);
#else
            threshold[0] = obj->hw->ps_threshold_high;
            APS_LOG("[%s] get threshold high: %d\n", __func__, threshold[0]);
#endif
			if(copy_to_user(ptr, &threshold[0], sizeof(threshold[0])))
			{
				err = -EFAULT;
				goto err_out;
			}
		break;

		case ALSPS_GET_PS_THRESHOLD_LOW:
#if 0
			APS_ERR("%s get threshold low before cali: 0x%x\n", __func__, atomic_read(&obj->ps_thd_val_low));
			threshold[0] = atomic_read(&obj->ps_thd_val_low) - obj->ps_cali;
			APS_ERR("%s set ps_calix%x\n", __func__, obj->ps_cali);
			APS_ERR("%s get threshold low: 0x%x\n", __func__, threshold[0]);
#else
            threshold[0] = obj->hw->ps_threshold_low;
            APS_LOG("[%s] get threshold low: %d\n", __func__, threshold[0]);
#endif
			if(copy_to_user(ptr, &threshold[0], sizeof(threshold[0])))
			{
				err = -EFAULT;
				goto err_out;
			}
		break;
		/*------------------------------------------------------------------------------------------*/
        default:
            APS_ERR("%s not supported = 0x%04x", __FUNCTION__, cmd);
            err = -ENOIOCTLCMD;
        break;
    }

err_out:
    return err;
}
/*----------------------------------------------------------------------------*/
static struct file_operations epl_sensor_fops =
{
    .owner = THIS_MODULE,
    .open = epl_sensor_open,
    .release = epl_sensor_release,
    .unlocked_ioctl = epl_sensor_unlocked_ioctl,
};
/*----------------------------------------------------------------------------*/
static struct miscdevice epl_sensor_device =
{
    .minor = MISC_DYNAMIC_MINOR,
    .name = "als_ps",
    .fops = &epl_sensor_fops,
};
/*----------------------------------------------------------------------------*/
static int epl_sensor_i2c_suspend(struct i2c_client *client, pm_message_t msg)
{
    struct epl_sensor_priv *obj = i2c_get_clientdata(client);

    APS_FUN();
    if(!obj)
    {
        APS_ERR("[%s]: null pointer!!\n", __func__);
        return -EINVAL;
    }

    return 0;

}
/*----------------------------------------------------------------------------*/
static int epl_sensor_i2c_resume(struct i2c_client *client)
{
    struct epl_sensor_priv *obj = i2c_get_clientdata(client);

    APS_FUN();
    if(!obj)
    {
        APS_ERR("[%s]: null pointer!!\n", __func__);
        return -EINVAL;
    }

    return 0;
}

#if defined(CONFIG_HAS_EARLYSUSPEND)
/*----------------------------------------------------------------------------*/
static void epl_sensor_early_suspend(struct early_suspend *h)
{
    /*early_suspend is only applied for ALS*/
    struct epl_sensor_priv *obj = container_of(h, struct epl_sensor_priv, early_drv);
    bool enable_ps = test_bit(CMC_BIT_PS, &obj->enable);
    bool enable_als = test_bit(CMC_BIT_ALS, &obj->enable);

    APS_FUN();
    if(!obj)
    {
        APS_ERR("null pointer!!\n");
        return;
    }

    atomic_set(&obj->als_suspend, 1);
#if HS_ENABLE
    atomic_set(&obj->hs_suspend, 1);
#endif
#if PS_GES
    atomic_set(&obj->ges_suspend, 1);
#endif
    APS_LOG("[%s]: enable_ps(%d), enable_als(%d) \r\n", __func__, enable_ps, enable_als);


    if(enable_als == 1 && epl_sensor.als.polling_mode == 0)
    {
        APS_LOG("[%s]: check ALS interrupt_flag............ \r\n", __func__);
        epl_sensor_read_als(obj->client);
        if(epl_sensor.als.interrupt_flag == EPL_INT_TRIGGER)
        {
            APS_LOG("[%s]: epl_sensor.als.interrupt_flag = %d \r\n", __func__, epl_sensor.als.interrupt_flag);
            //ALS unlock interrupt pin and restart chip
        	epl_sensor_I2C_Write(obj->client, 0x12, EPL_CMP_RESET | EPL_UN_LOCK);
        }
    }
    epl_sensor_update_mode(obj->client);

}
/*----------------------------------------------------------------------------*/
static void epl_sensor_late_resume(struct early_suspend *h)
{
    /*late_resume is only applied for ALS*/
    struct epl_sensor_priv *obj = container_of(h, struct epl_sensor_priv, early_drv);
    bool enable_ps = test_bit(CMC_BIT_PS, &obj->enable);
    bool enable_als = test_bit(CMC_BIT_ALS, &obj->enable);

    APS_FUN();

    if(!obj)
    {
        APS_ERR("null pointer!!\n");
        return;
    }

    atomic_set(&obj->als_suspend, 0);
    atomic_set(&obj->ps_suspend, 0);
#if HS_ENABLE
    atomic_set(&obj->hs_suspend, 0);
#endif
#if PS_GES
    atomic_set(&obj->ges_suspend, 0);
#endif

    APS_LOG("[%s]: enable_ps(%d), enable_als(%d) \r\n", __func__, enable_ps, enable_als);
    if(enable_als == 1)
        epl_sensor_fast_update(obj->client);
    epl_sensor_update_mode(obj->client);

}
#endif

#if MTK_LTE /*MTK_LTE start .................*/
/*--------------------------------------------------------------------------------*/
static int als_open_report_data(int open)
{
	//should queuq work to report event if  is_report_input_direct=true
	return 0;
}
/*--------------------------------------------------------------------------------*/
// if use  this typ of enable , Gsensor only enabled but not report inputEvent to HAL
static int als_enable_nodata(int en)
{
    struct epl_sensor_priv *epld = epl_sensor_obj;

	if(!epld)
	{
		APS_ERR("obj is null!!\n");
		return -1;
	}

	APS_LOG("[%s]: als enable=%d \r\n", __func__, en);
    epl_sensor_enable_als(en);

	return 0;
}
/*--------------------------------------------------------------------------------*/
static int als_set_delay(u64 ns)
{
	return 0;
}
/*--------------------------------------------------------------------------------*/
static int als_get_data(int* value, int* status)
{
	int err = 0;
	u16 report_lux = 0;
	struct epl_sensor_priv *obj = epl_sensor_obj;
	if(!obj)
	{
		APS_ERR("obj is null!!\n");
		return -1;
	}

    epl_sensor_read_als(obj->client);

    report_lux = epl_sensor_get_als_value(obj, epl_sensor.als.data.channels[1]);

    *value = report_lux;
    *status = SENSOR_STATUS_ACCURACY_MEDIUM;
    APS_LOG("[%s]:*value = %d\n", __func__, *value);

	return err;
}
/*--------------------------------------------------------------------------------*/
// if use  this typ of enable , Gsensor should report inputEvent(x, y, z ,stats, div) to HAL
static int ps_open_report_data(int open)
{
	//should queuq work to report event if  is_report_input_direct=true
	return 0;
}
/*--------------------------------------------------------------------------------*/
// if use  this typ of enable , Gsensor only enabled but not report inputEvent to HAL
static int ps_enable_nodata(int en)
{
    struct epl_sensor_priv *epld = epl_sensor_obj;
    if(!epld)
	{
		APS_ERR("obj is null!!\n");
		return -1;
	}

    APS_LOG("[%s]: ps enable=%d \r\n", __func__, en);
    epl_sensor_enable_ps(en);

	return 0;
}
/*--------------------------------------------------------------------------------*/
static int ps_set_delay(u64 ns)
{
	return 0;
}
/*--------------------------------------------------------------------------------*/
static int ps_get_data(int* value, int* status)
{

    int err = 0;
#if !PS_DYN_K
    struct epl_sensor_priv *obj = epl_sensor_obj;
#endif

    APS_LOG("---SENSOR_GET_DATA---\n\n");
#if !PS_DYN_K
    epl_sensor_read_ps(obj->client);
#endif

    *value = epl_sensor.ps.compare_low >> 3;
    *status = SENSOR_STATUS_ACCURACY_MEDIUM;

    APS_LOG("[%s]:*value = %d\n", __func__, *value);

	return err;
}
/*----------------------------------------------------------------------------*/
#else   /*MTK_LTE*/
/*----------------------------------------------------------------------------*/
int epl_sensor_ps_operate(void* self, uint32_t command, void* buff_in, int size_in,
                       void* buff_out, int size_out, int* actualout)
{
    int err = 0;
    int value;
    hwm_sensor_data* sensor_data;
#if !PS_DYN_K
    struct epl_sensor_priv *obj = (struct epl_sensor_priv *)self;
#endif

    APS_LOG("epl_sensor_ps_operate command = %x\n",command);
    switch (command)
    {
        case SENSOR_DELAY:
            if((buff_in == NULL) || (size_in < sizeof(int)))
            {
                APS_ERR("Set delay parameter error!\n");
                err = -EINVAL;
            }
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
                APS_LOG("[%s]: ps enable=%d \r\n", __func__, value);
                epl_sensor_enable_ps(value);
            }

            break;

        case SENSOR_GET_DATA:
            APS_LOG(" get ps data !!!!!!\n");
            if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
            {
                APS_ERR("get sensor data parameter error!\n");
                err = -EINVAL;
            }
            else
            {
                APS_LOG("---SENSOR_GET_DATA---\n\n");

#if !PS_DYN_K
                epl_sensor_read_ps(obj->client);
#endif
                sensor_data = (hwm_sensor_data *)buff_out;
                sensor_data->values[0] = epl_sensor.ps.compare_low >> 3;
                sensor_data->values[1] = epl_sensor.ps.data.data;
                sensor_data->value_divide = 1;
                sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;

            }
            break;

        default:
            APS_ERR("proxmy sensor operate function no this parameter %d!\n", command);
            err = -1;
            break;
    }

    return err;
}



int epl_sensor_als_operate(void* self, uint32_t command, void* buff_in, int size_in,
                        void* buff_out, int size_out, int* actualout)
{
    int err = 0;
    int value = 0;
    hwm_sensor_data* sensor_data;
    struct epl_sensor_priv *obj = (struct epl_sensor_priv *)self;
#if HS_ENABLE
    bool enable_hs = test_bit(CMC_BIT_HS, &obj->enable) && atomic_read(&obj->hs_suspend)==0;
#endif
    u16 report_lux = 0;

    APS_LOG("epl_sensor_als_operate command = %x\n",command);

    switch (command)
    {
        case SENSOR_DELAY:
            if((buff_in == NULL) || (size_in < sizeof(int)))
            {
                APS_ERR("Set delay parameter error!\n");
                err = -EINVAL;
            }
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
                APS_LOG("[%s]: als enable=%d \r\n", __func__, value);
                epl_sensor_enable_als(value);
            }
            break;


        case SENSOR_GET_DATA:
            APS_LOG("get als data !!!!!!\n");
#if HS_ENABLE
            if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)) || enable_hs == true)
#else
            if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
#endif
            {
                APS_ERR("get sensor data parameter error!\n");
                err = -EINVAL;
            }
            else
            {
                epl_sensor_read_als(obj->client);

                report_lux = epl_sensor_get_als_value(obj, epl_sensor.als.data.channels[1]);

                sensor_data = (hwm_sensor_data *)buff_out;
                sensor_data->values[0] = report_lux;
                if(epl_sensor.als.report_type == CMC_BIT_DYN_INT)
                    sensor_data->values[1] = epl_sensor.als.dyn_intt_raw;
                else
                    sensor_data->values[1] = epl_sensor.als.data.channels[1];
                sensor_data->value_divide = 1;
                sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
                APS_LOG("get als data->values[0] = %d\n", sensor_data->values[0]);
            }
            break;

        default:
            APS_ERR("light sensor operate function no this parameter %d!\n", command);
            err = -1;
            break;
    }

    return err;

}
#endif /*MTK_LTE end .................*/
/*----------------------------------------------------------------------------*/
static int als_intr_update_table(struct epl_sensor_priv *epld)
{
	int i;
	for (i = 0; i < 10; i++) {
		if ( als_lux_intr_level[i] < 0xFFFF )
		{
#if ALS_DYN_INTT
		    if(epl_sensor.als.report_type == CMC_BIT_DYN_INT)
		    {
                als_adc_intr_level[i] = als_lux_intr_level[i] * 1000 / c_gain;
		    }
		    else
#endif
		    {
                als_adc_intr_level[i] = als_lux_intr_level[i] * 1000 / epl_sensor.als.factory.lux_per_count;
		    }


            if(i != 0 && als_adc_intr_level[i] <= als_adc_intr_level[i-1])
    		{
                als_adc_intr_level[i] = 0xffff;
    		}
		}
		else {
			als_adc_intr_level[i] = als_lux_intr_level[i];
		}
		APS_LOG(" [%s]:als_adc_intr_level: [%d], %ld \r\n", __func__, i, als_adc_intr_level[i]);
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
//static int epl_sensor_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
//{
    //strcpy(info->type, EPL_DEV_NAME);
    //return 0;
//}
/*----------------------------------------------------------------------------*/
static int epl_sensor_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct epl_sensor_priv *obj;
#if MTK_LTE
    struct als_control_path als_ctl={0};
	struct als_data_path als_data={0};
	struct ps_control_path ps_ctl={0};
	struct ps_data_path ps_data={0};
#else
    struct hwmsen_object obj_ps, obj_als;
#endif
    int err = 0;

    APS_FUN();
    epl_sensor_dumpReg(client);

    if((i2c_smbus_read_byte_data(client, 0x21)) != EPL_REVNO){ //check chip
        APS_LOG("elan ALS/PS sensor is failed. \n");
        err = -1;
        goto exit;
    }

    client->timing = 400;

    if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
    {
        err = -ENOMEM;
        goto exit;
    }

    memset(obj, 0, sizeof(*obj));

    epl_sensor_obj = obj;
    obj->hw =hw;

    epl_sensor_get_addr(obj->hw, &obj->addr);

    obj->als_level_num = sizeof(obj->hw->als_level)/sizeof(obj->hw->als_level[0]);
    obj->als_value_num = sizeof(obj->hw->als_value)/sizeof(obj->hw->als_value[0]);
    BUG_ON(sizeof(obj->als_level) != sizeof(obj->hw->als_level));
    memcpy(obj->als_level, obj->hw->als_level, sizeof(obj->als_level));
    BUG_ON(sizeof(obj->als_value) != sizeof(obj->hw->als_value));
    memcpy(obj->als_value, obj->hw->als_value, sizeof(obj->als_value));
#if PS_GES
    atomic_set(&obj->ges_suspend, 0);
    obj->gs_input_dev = input_allocate_device();
    set_bit(EV_KEY, obj->gs_input_dev->evbit);
    set_bit(EV_REL, obj->gs_input_dev->evbit);
    set_bit(EV_ABS, obj->gs_input_dev->evbit);
    obj->gs_input_dev->evbit[0] |= BIT_MASK(EV_REP);
    obj->gs_input_dev->keycodemax = 500;
    obj->gs_input_dev->name ="elan_gesture";
    obj->gs_input_dev->keybit[BIT_WORD(KEYCODE_LEFT)] |= BIT_MASK(KEYCODE_LEFT);
    if (input_register_device(obj->gs_input_dev))
        APS_ERR("register input error\n");
#endif

    INIT_DELAYED_WORK(&obj->eint_work, epl_sensor_eint_work);
#if PS_DYN_K
    INIT_DELAYED_WORK(&obj->dynk_thd_polling_work, epl_sensor_dynk_thd_polling_work);
#endif
#if HS_ENABLE
    atomic_set(&obj->hs_suspend, 0);
	INIT_DELAYED_WORK(&obj->polling_work, epl_sensor_polling_work);
	mutex_init(&hs_sensor_mutex);
#endif
    obj->client = client;

    mutex_init(&sensor_mutex);
    wake_lock_init(&ps_lock, WAKE_LOCK_SUSPEND, "ps wakelock");

    i2c_set_clientdata(client, obj);

    atomic_set(&obj->als_suspend, 0);
    atomic_set(&obj->ps_suspend, 0);
#if EINT_API
    obj->irq_node = of_find_compatible_node(NULL, NULL, "mediatek, ALS-eint");
#endif
    obj->enable = 0;

    epl_sensor_i2c_client = client;

    //initial global variable and write to senosr
    initial_global_variable(client, obj);

    if((err = epl_sensor_init_client(client)))
    {
        goto exit_init_failed;
    }

    if((err = misc_register(&epl_sensor_device)))
    {
        APS_ERR("epl_sensor_device register failed\n");
        goto exit_misc_device_register_failed;
    }
#if MTK_LTE /*MTK_LTE start .................*/
    if((err = epl_sensor_create_attr(&epl_sensor_init_info.platform_diver_addr->driver)))
    {
        APS_ERR("create attribute err = %d\n", err);
        goto exit_create_attr_failed;
    }
    als_ctl.open_report_data= als_open_report_data;
	als_ctl.enable_nodata = als_enable_nodata;
	als_ctl.set_delay  = als_set_delay;
	als_ctl.is_report_input_direct = epl_sensor.als.polling_mode==0? true:false;
	als_ctl.is_polling_mode = epl_sensor.als.polling_mode==1? true:false;
#ifdef CUSTOM_KERNEL_SENSORHUB
	als_ctl.is_support_batch = true;
#else
    als_ctl.is_support_batch = false;
#endif

	err = als_register_control_path(&als_ctl);
	if(err)
	{
		APS_ERR("register fail = %d\n", err);
		goto exit_create_attr_failed;
	}

	als_data.get_data = als_get_data;
	als_data.vender_div = 100;
	err = als_register_data_path(&als_data);
	if(err)
	{
		APS_ERR("tregister fail = %d\n", err);
		goto exit_create_attr_failed;
	}

	ps_ctl.open_report_data= ps_open_report_data;
	ps_ctl.enable_nodata = ps_enable_nodata;
	ps_ctl.set_delay  = ps_set_delay;
	ps_ctl.is_report_input_direct = epl_sensor.ps.polling_mode==0? true:false;
	ps_ctl.is_polling_mode = epl_sensor.ps.polling_mode==1? true:false;
#ifdef CUSTOM_KERNEL_SENSORHUB
	ps_ctl.is_support_batch = true;
#else
    ps_ctl.is_support_batch = false;
#endif

	err = ps_register_control_path(&ps_ctl);
	if(err)
	{
		APS_ERR("register fail = %d\n", err);
		goto exit_create_attr_failed;
	}

	ps_data.get_data = ps_get_data;
	ps_data.vender_div = 100;
	err = ps_register_data_path(&ps_data);
	if(err)
	{
		APS_ERR("tregister fail = %d\n", err);
		goto exit_create_attr_failed;
	}

#else /*MTK_LTE */
    if((err = epl_sensor_create_attr(&epl_sensor_alsps_driver.driver)))
    {
        APS_ERR("create attribute err = %d\n", err);
        goto exit_create_attr_failed;
    }

    obj_ps.self = epl_sensor_obj;
    ps_hw = &obj_ps;

    if( obj->hw->polling_mode_ps)
    {
        obj_ps.polling = 1;
        APS_LOG("ps_interrupt == false\n");
    }
    else
    {
        obj_ps.polling = 0;//interrupt mode
        APS_LOG("ps_interrupt == true\n");
    }

    obj_ps.sensor_operate = epl_sensor_ps_operate;

    if((err = hwmsen_attach(ID_PROXIMITY, &obj_ps)))
    {
        APS_ERR("attach fail = %d\n", err);
        goto exit_create_attr_failed;
    }

    obj_als.self = epl_sensor_obj;
    als_hw = &obj_als;

    if( obj->hw->polling_mode_als)
    {
        obj_als.polling = 1;
        APS_LOG("ps_interrupt == false\n");
    }
    else
    {
        obj_als.polling = 0;//interrupt mode
        APS_LOG("ps_interrupt == true\n");
    }

    obj_als.sensor_operate = epl_sensor_als_operate;
    APS_LOG("als polling mode\n");

    if((err = hwmsen_attach(ID_LIGHT, &obj_als)))
    {
        APS_ERR("attach fail = %d\n", err);
        goto exit_create_attr_failed;
    }
#endif  /*MTK_LTE end .................*/

#if defined(CONFIG_HAS_EARLYSUSPEND)
    obj->early_drv.level    = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1,
    obj->early_drv.suspend  = epl_sensor_early_suspend,
    obj->early_drv.resume   = epl_sensor_late_resume,
    register_early_suspend(&obj->early_drv);
#endif

    if(obj->hw->polling_mode_ps == 0 || obj->hw->polling_mode_als == 0)
        epl_sensor_setup_eint(client);

#if MTK_LTE
    alsps_init_flag = 0;
#endif

    APS_LOG("%s: OK\n", __FUNCTION__);
    return 0;

exit_create_attr_failed:
    misc_deregister(&epl_sensor_device);
exit_misc_device_register_failed:
exit_init_failed:
    //i2c_detach_client(client);
//	exit_kfree:
    kfree(obj);
exit:
    epl_sensor_i2c_client = NULL;
#if MTK_LTE
    alsps_init_flag = -1;
#endif
    APS_ERR("%s: err = %d\n", __FUNCTION__, err);
    return err;
}



/*----------------------------------------------------------------------------*/
static int epl_sensor_i2c_remove(struct i2c_client *client)
{
    int err;
#if MTK_LTE
    if((err = epl_sensor_delete_attr(&epl_sensor_init_info.platform_diver_addr->driver)))
    {
        APS_ERR("epl_sensor_delete_attr fail: %d\n", err);
    }
#else
    if((err = epl_sensor_delete_attr(&epl_sensor_i2c_driver.driver)))
    {
        APS_ERR("epl_sensor_delete_attr fail: %d\n", err);
    }
#endif
    if((err = misc_deregister(&epl_sensor_device)))
    {
        APS_ERR("misc_deregister fail: %d\n", err);
    }

    epl_sensor_i2c_client = NULL;
    i2c_unregister_device(client);
    kfree(i2c_get_clientdata(client));

    return 0;
}


#if !MTK_LTE
/*----------------------------------------------------------------------------*/
static int epl_sensor_probe(struct platform_device *pdev)
{
    //struct alsps_hw *hw = get_cust_alsps_hw();

    epl_sensor_power(hw, 1);

    if(i2c_add_driver(&epl_sensor_i2c_driver))
    {
        APS_ERR("add driver error\n");
        return -1;
    }

    return 0;
}



/*----------------------------------------------------------------------------*/
static int epl_sensor_remove(struct platform_device *pdev)
{
    //struct alsps_hw *hw = get_cust_alsps_hw();
    APS_FUN();
    epl_sensor_power(hw, 0);

    APS_ERR("EPL259x remove \n");
    i2c_del_driver(&epl_sensor_i2c_driver);

    return 0;
}


#ifdef CONFIG_OF
static const struct of_device_id alsps_of_match[] = {
	{ .compatible = "mediatek,als_ps", },
	{},
};
#endif
/*----------------------------------------------------------------------------*/
static struct platform_driver epl_sensor_alsps_driver =
{
    .probe      = epl_sensor_probe,
    .remove     = epl_sensor_remove,
    .driver     = {
        .name  = "als_ps",
        //.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = alsps_of_match,
#endif
    }
};
#endif

#if MTK_LTE
/*----------------------------------------------------------------------------*/
static int alsps_local_init(void)
{
    //struct alsps_hw *hw = get_cust_alsps_hw();
	//printk("fwq loccal init+++\n");

	epl_sensor_power(hw, 1);
	if(i2c_add_driver(&epl_sensor_i2c_driver))
	{
		APS_ERR("add driver error\n");
		return -1;
	}

	if(-1 == alsps_init_flag)
	{
	   return -1;
	}
	//printk("fwq loccal init---\n");

	return 0;
}
/*----------------------------------------------------------------------------*/
static int alsps_remove(void)
{
    //struct alsps_hw *hw = get_cust_alsps_hw();

    APS_FUN();
    epl_sensor_power(hw, 0);
    APS_ERR("epl_sensor remove \n");

    i2c_del_driver(&epl_sensor_i2c_driver);

    return 0;
}
#endif

/*----------------------------------------------------------------------------*/
static int __init epl_sensor_init(void)
{
    const char *name="mediatek,epl2590";
    APS_FUN();

    hw = get_alsps_dts_func(name, hw);

    if (!hw)
    {
    	APS_ERR("get dts info fail\n");
    }

#if MTK_LTE 
	alsps_driver_add(&epl_sensor_init_info);
#else

    if(platform_driver_register(&epl_sensor_alsps_driver))
    {
        APS_ERR("failed to register driver");
        return -ENODEV;
    }
#endif 
    return 0;
}

/*----------------------------------------------------------------------------*/
static void __exit epl_sensor_exit(void)
{
    APS_FUN();
#if !MTK_LTE
    platform_driver_unregister(&epl_sensor_alsps_driver);
#endif
}
/*----------------------------------------------------------------------------*/
module_init(epl_sensor_init);
module_exit(epl_sensor_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("renato.pan@eminent-tek.com");
MODULE_DESCRIPTION("EPL259x ALPsr driver");
MODULE_LICENSE("GPL");

