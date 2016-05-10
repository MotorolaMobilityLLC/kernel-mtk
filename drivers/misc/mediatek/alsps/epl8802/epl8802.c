/* drivers/hwmon/mt6516/amit/epl8802.c - EPL8802 ALS/PS driver
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


#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/kobject.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#if defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
#endif
#include <linux/platform_device.h>
#include <linux/wakelock.h>

#include <cust_alsps.h>
#include "epl8802.h"

#define ANDR_M  1
// MTK_LTE is chose structure hwmsen_object or control_path, data_path
// MTK_LTE = 1, is control_path, data_path
// MTK_LTE = 0 hwmsen_object
#define MTK_LTE         1

#if ANDR_M
#include <linux/uaccess.h>
#include <linux/atomic.h>
#include <linux/io.h>
#include <linux/input/mt.h>
#include <alsps.h>
#include "upmu_sw.h"
#include "upmu_common.h"
#include <linux/gpio.h>
#include <linux/of_irq.h>
#include <linux/sched.h>
#include <alsps.h>
#else   /*ANDR_M*/
#include <linux/i2c.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/input.h>
#include <asm/atomic.h>
#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>
#include <asm/io.h>
#include <cust_eint.h>
#include <linux/input/mt.h>
#include <mach/devs.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>
#if MTK_LTE
#include <alsps.h>
#endif
#endif  /*ANDR_M*/

/******************************************************************************
 * driver info
*******************************************************************************/
#define EPL_DEV_NAME   		    "EPL8802"
#define DRIVER_VERSION          "3.0.0_Lenovo"
/******************************************************************************
 * ALS / PS sensor structure
*******************************************************************************/
#if ANDR_M
#define COMPATIABLE_NAME "mediatek,epl8802"
static struct alsps_hw alsps_cust;
static struct alsps_hw *hw = &alsps_cust;
static long long int_top_time;
#endif
/******************************************************************************
 *  ALS / PS define
 ******************************************************************************/
#define PS_DYN_K_ONE    1  // PS Auto K
#define ALS_DYN_INTT    1  // ALS Auto INTT
#define ALS_DYN_INTT_REPORT 1 //ALS_DYN report

#define LENOVO_SUSPEND_PATCH 1
#define LENOVO_CALI     1

#define PS_FIRST_REPORT 1

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

/******************************************************************************
 * extern functions
*******************************************************************************/

/*lenovo-sw caoyi modify for eint API 20151021 begin*/
#if !ANDR_M
#if 1//defined(MT6575) || defined(MT6571) || defined(MT6589)
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
/*lenovo-sw caoyi modify for eint API 20151021 end*/

/******************************************************************************
 *  configuration
 ******************************************************************************/
#ifndef CUST_EINT_ALS_TYPE
#define CUST_EINT_ALS_TYPE  8
#endif
#endif  /*ANDR_M*/

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
static const struct i2c_device_id epl_sensor_i2c_id[] = {{EPL_DEV_NAME,0},{}};
#if !ANDR_M
static struct i2c_board_info __initdata i2c_epl_sensor= { I2C_BOARD_INFO(EPL_DEV_NAME, (0x92>>1))};
#endif /*ANDR_M*/

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
//static int epl_sensor_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);
static int epl_sensor_i2c_suspend(struct i2c_client *client, pm_message_t msg);
static int epl_sensor_i2c_resume(struct i2c_client *client);
#if ANDR_M
static irqreturn_t epl_sensor_eint_func(int irq, void *desc);
#else
static void epl_sensor_eint_func(void);
#endif  /*ANDR_M*/
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
#if ALS_DYN_INTT_REPORT
int epl_sensor_als_dyn_report(bool first_flag);
#endif
#if MTK_LTE
static int ps_open_report_data(int open);
static int ps_enable_nodata(int en);
static int ps_set_delay(u64 ns);
static int ps_get_data(int* value, int* status);
static int als_open_report_data(int open);
static int als_enable_nodata(int en);
static int als_set_delay(u64 ns);
static int als_get_data(int* value, int* status);
#else
int epl_sensor_ps_operate(void* self, uint32_t command, void* buff_in, int size_in, void* buff_out, int size_out, int* actualout);
int epl_sensor_als_operate(void* self, uint32_t command, void* buff_in, int size_in, void* buff_out, int size_out, int* actualout);
#endif
static struct epl_sensor_priv *g_epl8802_ptr;
static long long int_top_time;
typedef enum
{
    CMC_BIT_ALS   	= 1,
    CMC_BIT_PS     	= 2,
} CMC_BIT;

typedef enum
{
    CMC_BIT_RAW   			= 0x0,
    CMC_BIT_PRE_COUNT     	= 0x1,
    CMC_BIT_DYN_INT			= 0x2,
    CMC_BIT_TABLE			= 0x3,
    CMC_BIT_INTR_LEVEL		= 0x4,
} CMC_ALS_REPORT_TYPE;

#if ALS_DYN_INTT
typedef enum
{
    CMC_BIT_LSRC_NON		= 0x0,
    CMC_BIT_LSRC_SCALE     	= 0x1,
    CMC_BIT_LSRC_SLOPE		= 0x2,
    CMC_BIT_LSRC_BOTH       = 0x3,
} CMC_LSRC_REPORT_TYPE;
#endif

struct epl_sensor_i2c_addr      /*define a series of i2c slave address*/
{
    u8  write_addr;
};

struct epl_sensor_priv
{
    struct alsps_hw  *hw;
    struct i2c_client *client;
    struct delayed_work  eint_work;
    struct input_dev *gs_input_dev;
    /*i2c address group*/
    struct epl_sensor_i2c_addr  addr;
    /*misc*/
    atomic_t   	als_suspend;
    atomic_t    ps_suspend;
/*lenovo-sw caoyi modify for eint API 20151021 begin*/
    struct device_node *irq_node;
    int		irq;
/*lenovo-sw caoyi modify for eint API 20151021 end*/
    /*data*/
    u16		    lux_per_count;
    ulong       enable;         	/*record HAL enalbe status*/
    /*data*/
    u16         als_level_num;
    u16         als_value_num;
    u32         als_level[C_CUST_ALS_LEVEL-1];
    u32         als_value[C_CUST_ALS_LEVEL];
#if ALS_DYN_INTT
    uint32_t ratio;
    uint32_t last_ratio;
    int c_gain_h; // fluorescent (C1)
    int c_gain_l; // incandescent (C2)
    uint32_t lsource_thd_high; //different light source boundary (N) fluorescent (C1)
    uint32_t lsource_thd_low; //different light source boundary (N) incandescent (C2)
#endif
    /*als interrupt*/
    int als_intr_level;
    int als_intr_lux;
    /*early suspend*/
#if defined(CONFIG_HAS_EARLYSUSPEND)
    struct early_suspend    early_drv;
#endif
};

/*lenovo-sw caoyi modify for eint API 20151021 begin*/
#ifdef CONFIG_OF
static const struct of_device_id alsps_of_match[] = {
	{.compatible = "mediatek,alsps"},
	{},
};
#endif
/*lenovo-sw caoyi modify for eint API 20151021 end*/

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
/*lenovo-sw caoyi modify for eint API 20151021 begin*/
#ifdef CONFIG_OF
        .of_match_table = alsps_of_match,
#endif
/*lenovo-sw caoyi modify for eint API 20151021 end*/
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


#if PS_DYN_K_ONE
static bool ps_dyn_flag = false;
#define PS_MAX_CT	10000
#define PS_MAX_IR	50000
#define PS_DYN_H_OFFSET 1121
#define PS_DYN_L_OFFSET 586
u32 dynk_thd_low = 0;
u32 dynk_thd_high = 0;
#endif

#if PS_FIRST_REPORT
static bool ps_frist_flag = true;
#endif

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



static void write_global_variable(struct i2c_client *client)
{
    u8 buf;
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
        int ps_cancelation = 0, ps_hthr = 0, ps_lthr = 0;
		fp = filp_open(ps_cal_file, O_RDWR, S_IRUSR);
        if (IS_ERR(fp))
        {
            APS_ERR("NO calibration file\n");
            epl_sensor.ps.factory.calibration_enable =  false;
        }
        else
        {
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
    u32 ch1_all=0;
    int count =1, i;
    int ps_hthr=0, ps_lthr=0, ps_cancelation=0, ps_cal_len = 0;
    char ps_calibration[20];


    if(PS_MAX_XTALK < 0)
    {
        APS_ERR("[%s]:Failed: PS_MAX_XTALK < 0 \r\n", __func__);
        return -EINVAL;
    }

    for(i=0; i<count; i++)
    {
        msleep(50);
    	switch(epl_sensor.mode)
    	{
    		case EPL_MODE_PS:
    		case EPL_MODE_ALS_PS:
    			ch1 = factory_ps_data();
		    break;
    	}

    	ch1_all = ch1_all + ch1;
    }


    ch1 = (u16)(ch1_all/count);

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
    epl_sensor.als.report_type = CMC_BIT_DYN_INT; //CMC_BIT_RAW; //CMC_BIT_DYN_INT;
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
    epl_sensor.als.lsrc_type = CMC_BIT_LSRC_NON;
    if(epl_sensor.als.report_type == CMC_BIT_DYN_INT)
    {
        dynamic_intt_idx = dynamic_intt_init_idx;
        epl_sensor.als.integration_time = als_dynamic_intt_intt[dynamic_intt_idx];
        epl_sensor.als.gain = als_dynamic_intt_gain[dynamic_intt_idx];
        dynamic_intt_high_thr = als_dynamic_intt_high_thr[dynamic_intt_idx];
        dynamic_intt_low_thr = als_dynamic_intt_low_thr[dynamic_intt_idx];
    }
/*lenovo-sw caoyi1 modify als gain 20151020 begin*/
    c_gain = 155;//300; // 300/1000=0.3 /*Lux per count*/
/*lenovo-sw caoyi1 modify als gain 20151020 end*/
    obj->lsource_thd_high = 1900; //different light source boundary (N) (1.9)
    obj->lsource_thd_low = 1500;   //1.5
    obj->c_gain_h = 300; //fluorescent (C1) (0.3)
    obj->c_gain_l = 214;  //incandescent (C2)   (0.214)

#endif

    //ps setting
    epl_sensor.ps.polling_mode = obj->hw->polling_mode_ps;
    epl_sensor.ps.integration_time = EPL_PS_INTT_80;
    epl_sensor.ps.gain = EPL_GAIN_MID;
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
    //set als / ps interrupt control mode and trigger type
    set_als_ps_intr_type(client, epl_sensor.ps.polling_mode, epl_sensor.als.polling_mode);
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
    dyn_intt_raw = (raw_data * 10) / (10 * gain_value * als_dynamic_intt_value[dynamic_intt_idx] / als_dynamic_intt_value[1]); //float calculate //Robert, overflow issue 20150708

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

            if(epl_sensor.als.lsrc_type != CMC_BIT_LSRC_NON)
            {
                long luxratio = 0;

                if (epl_sensor.als.data.channels[0] == 0)
                {
                   epl_sensor.als.data.channels[0] = 1;
                   APS_ERR("[%s]:read ch0 data is 0 \r\n", __func__);
                }

                luxratio = (long)((als*dynamic_intt_min_unit) / epl_sensor.als.data.channels[0]); //lux ratio (A=CH1/CH0)

                obj->ratio = luxratio;
                if((epl_sensor.als.saturation >> 5) == 0)
                {
                    if(epl_sensor.als.lsrc_type == CMC_BIT_LSRC_SCALE || epl_sensor.als.lsrc_type == CMC_BIT_LSRC_BOTH)
                    {
                        if(obj->ratio == 0){
                            obj->last_ratio = luxratio;
                        }
                        else{
                            obj->last_ratio = (luxratio + obj->last_ratio*9)  / 10;
                        }

                        if (obj->last_ratio >= obj->lsource_thd_high)  //fluorescent (C1)
                        {
                            c_gain = obj->c_gain_h;
                        }
                        else if (obj->last_ratio <= obj->lsource_thd_low)   //incandescent (C2)
                        {
                            c_gain = obj->c_gain_l;
                        }
                        else if(epl_sensor.als.lsrc_type == CMC_BIT_LSRC_BOTH)
                        {
                            int a = 0, b = 0, c = 0;
                            a = (obj->c_gain_h - obj->c_gain_l) * dynamic_intt_min_unit / (obj->lsource_thd_high - obj->lsource_thd_low);
                            b = (obj->c_gain_h) - ((a * obj->lsource_thd_high)/dynamic_intt_min_unit );
                            c = ((a * obj->last_ratio)/dynamic_intt_min_unit) + b;

                            if(c > obj->c_gain_h)
                                c_gain = obj->c_gain_h;
                            else if (c < obj->c_gain_l)
                                c_gain = obj->c_gain_l;
                            else
                                c_gain = c;
                        }
                    }
                    else
                    {
                        if (luxratio >= obj->lsource_thd_high)  //fluorescent (C1)
                        {
                            c_gain = obj->c_gain_h;
                        }
                        else if (luxratio <= obj->lsource_thd_low)   //incandescent (C2)
                        {
                            c_gain = obj->c_gain_l;
                        }
                        else{ /*mix*/
                            int a = 0, b = 0, c = 0;
                            a = (obj->c_gain_h - obj->c_gain_l) * dynamic_intt_min_unit / (obj->lsource_thd_high - obj->lsource_thd_low);
                            b = (obj->c_gain_h) - ((a * obj->lsource_thd_high)/dynamic_intt_min_unit );
                            c = ((a * luxratio)/dynamic_intt_min_unit) + b;

                            if(c > obj->c_gain_h)
                                c_gain = obj->c_gain_h;
                            else if (c < obj->c_gain_l)
                                c_gain = obj->c_gain_l;
                            else
                                c_gain = c;
                        }
                    }
                     APS_LOG("[%s]:ch0=%d, ch1=%d, c_gain=%d, obj->ratio=%d, obj->last_ratio=%d \r\n\n",
                                      __func__,epl_sensor.als.data.channels[0], als, c_gain, obj->ratio, obj->last_ratio);
                }
                else
                {
                    APS_LOG("[%s]: ALS saturation(%d) \r\n", __func__, (epl_sensor.als.saturation >> 5));
                }
            }
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

                //epl_sensor_I2C_Write(obj->client, 0x11, EPL_POWER_OFF | EPL_RESETN_RESET);
                //epl_sensor_I2C_Write(obj->client, 0x01, epl_sensor.als.integration_time | epl_sensor.als.gain);
                epl_sensor_I2C_Write(obj->client, 0x00, epl_sensor.wait | epl_sensor.mode);
                epl_sensor_I2C_Write(obj->client, 0x11, EPL_POWER_ON | EPL_RESETN_RUN);
#if ALS_DYN_INTT_REPORT
                dynamic_intt_lux = -1;
#endif
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
    epl_sensor_I2C_Write_Block(client, 0x0c, buf, 4);

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
    epl_sensor_I2C_Write_Block(client, 0x08, buf, 4);

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

static void epl_sensor_report_ps_status(void)
{
    struct epl_sensor_priv *epld = epl_sensor_obj;
    int err;
#if !MTK_LTE
    hwm_sensor_data sensor_data;
#endif
    APS_FUN();
    APS_LOG("[%s]: epl_sensor.ps.data.data=%d, ps status=%d \r\n", __func__, epl_sensor.ps.data.data, (epl_sensor.ps.compare_low >> 3));

#if MTK_LTE
    err = ps_report_interrupt_data((epl_sensor.ps.compare_low >> 3));
    if(err != 0)  //if report status is fail, write unlock again.
    {
        APS_ERR("epl_sensor_eint_work err: %d\n", err);
        epl_sensor.ps.compare_reset = EPL_CMP_RESET;
    	epl_sensor.ps.lock = EPL_UN_LOCK;
    	epl_sensor_I2C_Write(epld->client,0x1b, epl_sensor.ps.compare_reset | epl_sensor.ps.lock);
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
	    epl_sensor.ps.compare_reset = EPL_CMP_RESET;
    	epl_sensor.ps.lock = EPL_UN_LOCK;
    	epl_sensor_I2C_Write(epld->client,0x1b, epl_sensor.ps.compare_reset | epl_sensor.ps.lock);
	}
#endif
}

static void epl_sensor_intr_als_report_lux(void)
{
    struct epl_sensor_priv *obj = epl_sensor_obj;
    u16 als;
    int err = 0;
#if ALS_DYN_INTT
    int last_dynamic_intt_idx = dynamic_intt_idx;
#endif
#if !MTK_LTE
    hwm_sensor_data sensor_data;
#endif

    epl_sensor.als.compare_reset = EPL_CMP_RESET;
	epl_sensor.als.lock = EPL_UN_LOCK;
	epl_sensor_I2C_Write(obj->client,0x12, epl_sensor.als.compare_reset | epl_sensor.als.lock);

    APS_LOG("[%s]: IDEL MODE \r\n", __func__);
    //epl_sensor_I2C_Write(obj->client, 0x00, epl_sensor.wait | EPL_MODE_IDLE);
	epl_sensor_I2C_Write(obj->client, 0x11, EPL_POWER_OFF | EPL_RESETN_RESET);  //After runing CMP_RESET, dont clean interrupt_flag

    als = epl_sensor_get_als_value(obj, epl_sensor.als.data.channels[1]);
#if ALS_DYN_INTT
    if(dynamic_intt_idx == last_dynamic_intt_idx)
#endif
    {
        APS_LOG("[%s]: report als = %d \r\n", __func__, als);
#if MTK_LTE
        if((err = als_data_report(alsps_context_obj->idev_als, als, SENSOR_STATUS_ACCURACY_MEDIUM)))
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
    //Robert, need to check 20150708
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
                normal_value = gain_value * als_dynamic_intt_value[dynamic_intt_idx] / als_dynamic_intt_value[1];//Robert, overflow issue 20150708

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
        //set dynamic threshold
    	if(epl_sensor.als.compare_high >> 4)
    	{
    		epl_sensor.als.high_threshold = epl_sensor.als.high_threshold + 250;
    		epl_sensor.als.low_threshold = epl_sensor.als.low_threshold + 250;
    		if (epl_sensor.als.high_threshold > 60000)
    		{
    			epl_sensor.als.high_threshold = epl_sensor.als.high_threshold - 250;
    			epl_sensor.als.low_threshold = epl_sensor.als.low_threshold - 250;
    		}
    	}
    	if(epl_sensor.als.compare_low>> 3)
    	{
    		epl_sensor.als.high_threshold = epl_sensor.als.high_threshold - 250;
    		epl_sensor.als.low_threshold = epl_sensor.als.low_threshold - 250;
    		if (epl_sensor.als.high_threshold < 250)
    		{
    			epl_sensor.als.high_threshold = epl_sensor.als.high_threshold + 250;
    			epl_sensor.als.low_threshold = epl_sensor.als.low_threshold + 250;
    		}
    	}

        if(epl_sensor.als.high_threshold < epl_sensor.als.low_threshold)
    	{
    	    APS_LOG("[%s]:recover default setting \r\n", __FUNCTION__);
    	    epl_sensor.als.high_threshold = obj->hw->als_threshold_high;
    	    epl_sensor.als.low_threshold = obj->hw->als_threshold_low;
    	}
    }

	//write new threshold
	set_lsensor_intr_threshold(epl_sensor.als.low_threshold, epl_sensor.als.high_threshold);

    epl_sensor_I2C_Write(obj->client, 0x11, EPL_POWER_ON | EPL_RESETN_RUN);
    //epl_sensor_I2C_Write(obj->client, 0x00, epl_sensor.wait | epl_sensor.mode);
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

int factory_ps_data(void)
{
    struct epl_sensor_priv *epld = epl_sensor_obj;
    bool enable_ps = test_bit(CMC_BIT_PS, &epld->enable) && atomic_read(&epld->ps_suspend)==0;

    if(enable_ps == 0)
    {
        set_bit(CMC_BIT_PS, &epld->enable);
        epl_sensor_fast_update(epld->client);
        epl_sensor_update_mode(epld->client);
    }
    if(epl_sensor.ps.polling_mode == 0)
        epl_sensor_read_ps(epld->client);

    APS_LOG("[%s]: ps_raw=%d \r\n", __func__, epl_sensor.ps.data.data);
    return epl_sensor.ps.data.data;
}

int factory_als_data(void)
{
    struct epl_sensor_priv *epld = epl_sensor_obj;
    u16 als_raw = 0;

    bool enable_als = test_bit(CMC_BIT_ALS, &epld->enable) && atomic_read(&epld->als_suspend)==0;

    if(enable_als == 0)
    {
        set_bit(CMC_BIT_ALS, &epld->enable);
        epl_sensor_fast_update(epld->client);
        epl_sensor_update_mode(epld->client);
    }

    if(enable_als == true)
    {
        epl_sensor_read_als(epld->client);
        if(epl_sensor.als.report_type == CMC_BIT_DYN_INT)
        {
            int als_lux=0;
#if ALS_DYN_INTT_REPORT
            als_lux = epl_sensor_als_dyn_report(false);
#else
            als_lux = epl_sensor_get_als_value(epld, epl_sensor.als.data.channels[1]);
#endif
        }
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
#if ALS_DYN_INTT_REPORT
int epl_sensor_als_dyn_report(bool first_flag)
{
    struct epl_sensor_priv *epld = epl_sensor_obj;
#if !MTK_LTE
    hwm_sensor_data als_sensor_data;
#endif
    int als_value=-1, err=0, i=0;
    bool enable_ps = test_bit(CMC_BIT_PS, &epld->enable) && atomic_read(&epld->ps_suspend)==0;
    bool enable_als = test_bit(CMC_BIT_ALS, &epld->enable) && atomic_read(&epld->als_suspend)==0;
    int als_time = als_sensing_time(epl_sensor.als.integration_time, epl_sensor.als.adc, epl_sensor.als.cycle);
    int ps_time = ps_sensing_time(epl_sensor.ps.integration_time, epl_sensor.ps.adc, epl_sensor.ps.cycle);

    do
    {
        if(enable_ps == 0 && enable_als == 1)
        {
            msleep(als_time);
            APS_LOG("[%s]:%d ALS only msleep(%d)", __func__, i, als_time);
        }
        else if (enable_ps == 1 && enable_als == 1)
        {
            msleep(ps_time+als_time);
            APS_LOG("[%s]:%d ALS+PS msleep(%d)", __func__, i, (ps_time+als_time));
        }

        epl_sensor_read_als(epld->client);
        als_value = epl_sensor_get_als_value(epld, epl_sensor.als.data.channels[1]);

        APS_LOG("[%s]: als_value=%d \r\n", __func__, als_value);

        if(als_value != -1 && first_flag == true)
        {
            APS_LOG("[%s]: first_repot(%d) \r\n", __func__, als_value);
#if MTK_LTE
            if((err = als_data_report(alsps_context_obj->idev_als, als_value, SENSOR_STATUS_ACCURACY_MEDIUM)))
            {
               APS_ERR("epl_sensor call als_data_report fail = %d\n", err);
            }
#else
            als_sensor_data.values[0] = als_value;
        	als_sensor_data.values[1] = epl_sensor.als.dyn_intt_raw;
        	als_sensor_data.value_divide = 1;
        	als_sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
        	if((err = hwmsen_get_interrupt_data(ID_LIGHT, &als_sensor_data)))
        	{
        	    APS_ERR("get interrupt data failed\n");
        	    APS_ERR("[%s]:call hwmsen_get_interrupt_data fail = %d\n", __func__, err);
        	}
#endif
        }
        i++;
    }while(-1 == als_value);

    return als_value;
}
#endif
#if PS_DYN_K_ONE
void epl_sensor_do_ps_auto_k_one(void)
{
    struct epl_sensor_priv *epld = epl_sensor_obj;
    int ps_time = ps_sensing_time(epl_sensor.ps.integration_time, epl_sensor.ps.adc, epl_sensor.ps.cycle);

    if(ps_dyn_flag == true)
    {
#if !PS_FIRST_REPORT
        msleep(ps_time);
#endif
        APS_LOG("[%s]: msleep(%d)\r\n", __func__, ps_time);
        epl_sensor_read_ps(epld->client);

        if(epl_sensor.ps.data.data < PS_MAX_CT && (epl_sensor.ps.saturation == 0) && (epl_sensor.ps.data.ir_data < PS_MAX_IR))
        {
    	    APS_LOG("[%s]: epl_sensor.ps.data.data=%d \r\n", __func__, epl_sensor.ps.data.data);
    	    dynk_thd_low = epl_sensor.ps.data.data + PS_DYN_L_OFFSET;
    		dynk_thd_high = epl_sensor.ps.data.data + PS_DYN_H_OFFSET;
    		set_psensor_intr_threshold(dynk_thd_low, dynk_thd_high);
        }
        else
        {
            APS_LOG("[%s]:threshold is err; epl_sensor.ps.data.data=%d \r\n", __func__, epl_sensor.ps.data.data);
	        epl_sensor.ps.high_threshold = epld->hw->ps_threshold_high;
            epl_sensor.ps.low_threshold = epld->hw->ps_threshold_low;
            set_psensor_intr_threshold(epl_sensor.ps.low_threshold, epl_sensor.ps.high_threshold);
            dynk_thd_low = epl_sensor.ps.low_threshold;
            dynk_thd_high = epl_sensor.ps.high_threshold;
        }

        ps_dyn_flag = false;
        APS_LOG("[%s]: reset ps_dyn_flag ........... \r\n", __func__);
    }
}
#endif

void epl_sensor_fast_update(struct i2c_client *client)
{
    int als_fast_time = 0;

    APS_FUN();
    mutex_lock(&sensor_mutex);
    set_als_ps_intr_type(client, 1, 1);
    als_fast_time = als_sensing_time(epl_sensor.als.integration_time, epl_sensor.als.adc, EPL_CYCLE_1);

    epl_sensor_I2C_Write(client, 0x11, EPL_POWER_OFF | EPL_RESETN_RESET);

    epl_sensor_I2C_Write(client, 0x06, epl_sensor.interrupt_control | epl_sensor.ps.persist |epl_sensor.ps.interrupt_type);
    epl_sensor_I2C_Write(client, 0x07, epl_sensor.als.interrupt_channel_select | epl_sensor.als.persist | epl_sensor.als.interrupt_type);

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
        //ALS reset and run
        epl_sensor_I2C_Write(client, 0x12, EPL_CMP_RESET | EPL_UN_LOCK);
    	epl_sensor_I2C_Write(client, 0x12, EPL_CMP_RUN | EPL_UN_LOCK);
    }

    epl_sensor_I2C_Write(client, 0x11, EPL_POWER_OFF | EPL_RESETN_RESET);

    epl_sensor_I2C_Write(client, 0x02, epl_sensor.als.adc | epl_sensor.als.cycle);

    set_als_ps_intr_type(client, epl_sensor.ps.polling_mode, epl_sensor.als.polling_mode);
    epl_sensor_I2C_Write(client, 0x06, epl_sensor.interrupt_control | epl_sensor.ps.persist |epl_sensor.ps.interrupt_type);
    epl_sensor_I2C_Write(client, 0x07, epl_sensor.als.interrupt_channel_select | epl_sensor.als.persist | epl_sensor.als.interrupt_type);
    //epl_sensor_I2C_Write(client, 0x11, EPL_POWER_ON | EPL_RESETN_RUN);
    mutex_unlock(&sensor_mutex);
}

void epl_sensor_update_mode(struct i2c_client *client)
{
    struct epl_sensor_priv *obj = epl_sensor_obj;
    int als_time = 0, ps_time = 0;
    bool enable_ps = test_bit(CMC_BIT_PS, &obj->enable) && atomic_read(&obj->ps_suspend)==0;
    bool enable_als = test_bit(CMC_BIT_ALS, &obj->enable) && atomic_read(&obj->als_suspend)==0;
    als_frame_time = als_time = als_sensing_time(epl_sensor.als.integration_time, epl_sensor.als.adc, epl_sensor.als.cycle);
    ps_frame_time = ps_time = ps_sensing_time(epl_sensor.ps.integration_time, epl_sensor.ps.adc, epl_sensor.ps.cycle);

    mutex_lock(&sensor_mutex);

	APS_LOG("mode selection =0x%x\n", enable_ps | (enable_als << 1));

    //**** mode selection ****
    switch((enable_als << 1) | enable_ps)
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

    //**** write setting ****
    // step 1. set sensor at idle mode
    // step 2. uplock and reset als / ps status
    // step 3. set sensor at operation mode
    // step 4. delay sensing time
    // step 5. unlock and run als / ps status
    epl_sensor_I2C_Write(client, 0x11, EPL_POWER_OFF | EPL_RESETN_RESET);
    // initial factory calibration variable
    read_factory_calibration();

    epl_sensor_I2C_Write(obj->client, 0x00, epl_sensor.wait | epl_sensor.mode);

    if(epl_sensor.mode != EPL_MODE_IDLE)    // if mode isnt IDLE, PWR_ON and RUN    //check this
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

	APS_LOG("[%s] reg0x00= 0x%x\n", __func__,  epl_sensor.wait | epl_sensor.mode);
	APS_LOG("[%s] reg0x07= 0x%x\n", __func__, epl_sensor.als.interrupt_channel_select | epl_sensor.als.persist | epl_sensor.als.interrupt_type);
	APS_LOG("[%s] reg0x06= 0x%x\n", __func__, epl_sensor.interrupt_control | epl_sensor.ps.persist |epl_sensor.ps.interrupt_type);
	APS_LOG("[%s] reg0x11= 0x%x\n", __func__, epl_sensor.power | epl_sensor.reset);
	APS_LOG("[%s] reg0x12= 0x%x\n", __func__, epl_sensor.als.compare_reset | epl_sensor.als.lock);
	APS_LOG("[%s] reg0x1b= 0x%x\n", __func__, epl_sensor.ps.compare_reset | epl_sensor.ps.lock);

    mutex_unlock(&sensor_mutex);

#if PS_FIRST_REPORT
    if(ps_frist_flag == true)
    {
        APS_LOG("[%s]: PS CMP RESET \r\n", __func__);
        epl_sensor_I2C_Write(obj->client,0x1b, EPL_CMP_RESET | EPL_UN_LOCK);
        epl_sensor_I2C_Write(obj->client,0x1b, EPL_CMP_RUN | EPL_UN_LOCK);

        msleep(ps_time);
        APS_LOG("[%s] PS(%dms)\r\n", __func__, ps_time);
        epl_sensor_read_ps(obj->client);
        if((epl_sensor.ps.data.data<=epl_sensor.ps.high_threshold) && (epl_sensor.ps.data.data>=epl_sensor.ps.low_threshold))
        {
            APS_LOG("[%s]: direct report FAR(%d) \r\n", __func__, epl_sensor.ps.data.data);
            epl_sensor.ps.compare_low = 0x08;
            epl_sensor_report_ps_status();
        }
        ps_frist_flag = false;
    }
#endif
}


/*----------------------------------------------------------------------------*/
#if ANDR_M
static irqreturn_t epl_sensor_eint_func(int irq, void *desc)
{
    struct epl_sensor_priv *obj = epl_sensor_obj;

    int_top_time = sched_clock();

    if(!obj)
    {
        return IRQ_HANDLED;
    }

    disable_irq_nosync(obj->irq);

    schedule_delayed_work(&obj->eint_work, 0);

    return IRQ_HANDLED;
}
#else /*ANDR_M*/
void epl_sensor_eint_func(void)
{
    struct epl_sensor_priv *obj = epl_sensor_obj;
    // APS_LOG(" interrupt fuc\n");
    if(!obj)
    {
        return;
    }
/*lenovo-sw caoyi modify for eint API 20151021 begin*/
#if defined(CONFIG_OF)
	//disable_irq(obj->irq);
#else
	#if defined(MT6575) || defined(MT6571) || defined(MT6589)
    mt65xx_eint_mask(CUST_EINT_ALS_NUM);
	#else
    mt_eint_mask(CUST_EINT_ALS_NUM);
	#endif
#endif
/*lenovo-sw caoyi modify for eint API 20151021 end*/
    schedule_delayed_work(&obj->eint_work, 0);
}

/*lenovo-sw caoyi modify for eint API 20151021 begin*/
#if defined(CONFIG_OF)
static irqreturn_t epl_eint_handler(int irq, void *desc)
{
	disable_irq_nosync(epl_sensor_obj->irq);
	epl_sensor_eint_func();

	return IRQ_HANDLED;
}
#endif
#endif /*ANDR_M*/
/*lenovo-sw caoyi modify for eint API 20151021 end*/

/*----------------------------------------------------------------------------*/
static void epl_sensor_eint_work(struct work_struct *work)
{
    struct epl_sensor_priv *obj = epl_sensor_obj;
    bool enable_ps = test_bit(CMC_BIT_PS, &obj->enable) && atomic_read(&obj->ps_suspend)==0;

    APS_LOG("%s\n",__func__);

    epl_sensor_read_ps(obj->client);
    epl_sensor_read_als(obj->client);
    if(epl_sensor.ps.interrupt_flag == EPL_INT_TRIGGER)
    {
        if(enable_ps)
        {
            wake_lock_timeout(&ps_lock, 2*HZ);
            epl_sensor_report_ps_status();
        }
        //PS unlock interrupt pin and restart chip
		epl_sensor.ps.compare_reset = EPL_CMP_RUN;
		epl_sensor.ps.lock = EPL_UN_LOCK;
		epl_sensor_I2C_Write(obj->client,0x1b, epl_sensor.ps.compare_reset | epl_sensor.ps.lock);
    }

    if(epl_sensor.als.interrupt_flag == EPL_INT_TRIGGER)
    {
        epl_sensor_intr_als_report_lux();
        //ALS unlock interrupt pin and restart chip
    	epl_sensor.als.compare_reset = EPL_CMP_RUN;
    	epl_sensor.als.lock = EPL_UN_LOCK;
    	epl_sensor_I2C_Write(obj->client,0x12, epl_sensor.als.compare_reset | epl_sensor.als.lock);
    }

/*lenovo-sw caoyi modify for eint API 20151021 begin*/
#if defined(CONFIG_OF)
	enable_irq(obj->irq);
#else
	#if defined(MT6575) || defined(MT6571) || defined(MT6589)
    mt65xx_eint_unmask(CUST_EINT_ALS_NUM);
	#else
    mt_eint_unmask(CUST_EINT_ALS_NUM);
	#endif
#endif
/*lenovo-sw caoyi modify for eint API 20151021 end*/
}

int epl_sensor_setup_eint(struct i2c_client *client)
{
#if ANDR_M
    struct epl_sensor_priv *obj = i2c_get_clientdata(client);
	int ret;
	u32 ints[2] = { 0, 0 };
	struct pinctrl *pinctrl;
	struct pinctrl_state *pins_default;
	struct pinctrl_state *pins_cfg;
	struct platform_device *alsps_pdev = get_alsps_platformdev();

	APS_LOG("epl_sensor_setup_eint\n");

    g_epl8802_ptr = obj;
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

	if (epl_sensor_obj->irq_node) {
		of_property_read_u32_array(epl_sensor_obj->irq_node, "debounce", ints,
					   ARRAY_SIZE(ints));
		gpio_request(ints[0], "p-sensor");
		gpio_set_debounce(ints[0], ints[1]);
		APS_LOG("ints[0] = %d, ints[1] = %d!!\n", ints[0], ints[1]);

		pinctrl_select_state(pinctrl, pins_cfg);

		epl_sensor_obj->irq = irq_of_parse_and_map(epl_sensor_obj->irq_node, 0);
		APS_LOG("epl8802_obj->irq = %d\n", epl_sensor_obj->irq);
		if (!epl_sensor_obj->irq) {
			APS_ERR("irq_of_parse_and_map fail!!\n");
			return -EINVAL;
		}
		if (request_irq(epl_sensor_obj->irq, epl_sensor_eint_func, IRQF_TRIGGER_LOW, "ALS-eint", NULL)) {
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


#else
    APS_LOG("epl_sensor_setup_eint\n");
    /*configure to GPIO function, external interrupt*/

    mt_set_gpio_mode(GPIO_ALS_EINT_PIN, GPIO_ALS_EINT_PIN_M_EINT);
    mt_set_gpio_dir(GPIO_ALS_EINT_PIN, GPIO_DIR_IN);
    mt_set_gpio_pull_enable(GPIO_ALS_EINT_PIN, GPIO_PULL_ENABLE);
    mt_set_gpio_pull_select(GPIO_ALS_EINT_PIN, GPIO_PULL_UP);

#if defined(MT6575) || defined(MT6571) || defined(MT6589)
    mt65xx_eint_set_sens(CUST_EINT_ALS_NUM, CUST_EINT_EDGE_SENSITIVE);
    mt65xx_eint_set_polarity(CUST_EINT_ALS_NUM, CUST_EINT_ALS_POLARITY);
    mt65xx_eint_set_hw_debounce(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_CN);
    mt65xx_eint_registration(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_EN, CUST_EINT_POLARITY_LOW, epl_sensor_eint_func, 0);
    mt65xx_eint_unmask(CUST_EINT_ALS_NUM);
#endif

#if defined(MT6582) || defined(MT6592) || defined(MT6735) || defined(MT6752) || defined(MT6753)
	mt_eint_set_hw_debounce(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_CN);
	mt_eint_registration(CUST_EINT_ALS_NUM, CUST_EINT_ALS_TYPE, epl_sensor_eint_func, 0);

	mt_eint_unmask(CUST_EINT_ALS_NUM);
#endif
#endif /*EINT_API*/


    return 0;
}

static int epl_sensor_init_client(struct i2c_client *client)
{
#if !ANDR_M
    struct epl_sensor_priv *obj = i2c_get_clientdata(client);
#endif /*ANDR_M*/
    int err=0;
    /*  interrupt mode */
    APS_FUN();
    APS_LOG("I2C Addr==[0x%x],line=%d\n", epl_sensor_i2c_client->addr, __LINE__);
#if !ANDR_M
    if(obj->hw->polling_mode_ps == 0)
    {
/*lenovo-sw caoyi modify for eint API 20151021 begin*/
#if defined(CONFIG_OF)
	//enable_irq(obj->irq);
#else
	#if defined(MT6575) || defined(MT6571) || defined(MT6589)
        mt65xx_eint_unmask(CUST_EINT_ALS_NUM);
	#else
        mt_eint_unmask(CUST_EINT_ALS_NUM);
	#endif
#endif
/*lenovo-sw caoyi modify for eint API 20151021 end*/

        if((err = epl_sensor_setup_eint(client)))
        {
            APS_ERR("setup eint: %d\n", err);
            return err;
        }
        APS_LOG("epl_sensor interrupt setup\n");
    }
#endif /*ANDR_M*/
    if((err = hw8k_init_device(client)) != 0)
    {
        APS_ERR("init dev: %d\n", err);
        return err;
    }

    /*  interrupt mode */
    //if(obj->hw->polling_mode_ps == 0)
    //     mt65xx_eint_unmask(CUST_EINT_ALS_NUM);
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

    if(enable_ps)
    {
        len += snprintf(buf+len, PAGE_SIZE-len, "PS: \n");
        len += snprintf(buf+len, PAGE_SIZE-len, "INTEG = %d, gain = %d\n", epl_sensor.ps.integration_time >> 2, epl_sensor.ps.gain);
        len += snprintf(buf+len, PAGE_SIZE-len, "ADC = %d, cycle = %d, ir drive = %d\n", epl_sensor.ps.adc >> 3, epl_sensor.ps.cycle, epl_sensor.ps.ir_drive);
        len += snprintf(buf+len, PAGE_SIZE-len, "saturation = %d, int flag = %d\n", epl_sensor.ps.saturation >> 5, epl_sensor.ps.interrupt_flag >> 2);
        len += snprintf(buf+len, PAGE_SIZE-len, "Thr(L/H) = (%d/%d)\n", epl_sensor.ps.low_threshold, epl_sensor.ps.high_threshold);
#if PS_DYN_K_ONE
        len += snprintf(buf+len, PAGE_SIZE-len, "Dyn thr(L/H) = (%ld/%ld)\n", (long)dynk_thd_low, (long)dynk_thd_high);
#endif
        len += snprintf(buf+len, PAGE_SIZE-len, "pals data = %d, data = %d\n", epl_sensor.ps.data.ir_data, epl_sensor.ps.data.data);
    }
    if(enable_als)
    {
        len += snprintf(buf+len, PAGE_SIZE-len, "ALS: \n");
        len += snprintf(buf+len, PAGE_SIZE-len, "INTEG = %d, gain = %d\n", epl_sensor.als.integration_time >> 2, epl_sensor.als.gain);
        len += snprintf(buf+len, PAGE_SIZE-len, "ADC = %d, cycle = %d\n", epl_sensor.als.adc >> 3, epl_sensor.als.cycle);
#if ALS_DYN_INTT
    if(epl_sensor.als.lsrc_type != CMC_BIT_LSRC_NON)
    {
        len += snprintf(buf+len, PAGE_SIZE-len, "lsource_thd_low=%d, lsource_thd_high=%d \n", epld->lsource_thd_low, epld->lsource_thd_high);
        len += snprintf(buf+len, PAGE_SIZE-len, "saturation = %d\n", epl_sensor.als.saturation >> 5);

        if(epl_sensor.als.lsrc_type == CMC_BIT_LSRC_SCALE || epl_sensor.als.lsrc_type == CMC_BIT_LSRC_BOTH)
        {
            len += snprintf(buf+len, PAGE_SIZE-len, "real_ratio = %d\n", epld->ratio);
            len += snprintf(buf+len, PAGE_SIZE-len, "use_ratio = %d\n", epld->last_ratio);
        }
        else if(epl_sensor.als.lsrc_type == CMC_BIT_LSRC_SLOPE)
        {
            len += snprintf(buf+len, PAGE_SIZE-len, "ratio = %d\n", epld->ratio);
        }
    }
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
    struct epl_sensor_priv *obj = epl_sensor_obj;
    bool enable_als = test_bit(CMC_BIT_ALS, &obj->enable) && atomic_read(&obj->als_suspend)==0;
    APS_FUN();

    sscanf(buf, "%hu", &mode);
    if(enable_als != mode)
    {
        if(mode)
        {
            set_bit(CMC_BIT_ALS, &obj->enable);
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
            epl_sensor_fast_update(obj->client);
        }
        else
        {
            clear_bit(CMC_BIT_ALS, &obj->enable);
        }
        epl_sensor_update_mode(obj->client);
#if ALS_DYN_INTT_REPORT
        if(mode == 1)
            epl_sensor_als_dyn_report(true);
#endif
    }
    return count;
}

static ssize_t epl_sensor_store_ps_enable(struct device_driver *ddri, const char *buf, size_t count)
{
    uint16_t mode=0;
    struct epl_sensor_priv *obj = epl_sensor_obj;
    bool enable_ps = test_bit(CMC_BIT_PS, &obj->enable) && atomic_read(&obj->ps_suspend)==0;
    APS_FUN();

    sscanf(buf, "%hu", &mode);
    if(enable_ps != mode)
    {
        if(mode)
        {
            //wake_lock(&ps_lock);
            set_bit(CMC_BIT_PS, &obj->enable);
#if PS_FIRST_REPORT
            ps_frist_flag = true;
#endif
#if PS_DYN_K_ONE
            ps_dyn_flag = true;
#endif
        }
        else
        {
            clear_bit(CMC_BIT_PS, &obj->enable);
            //wake_unlock(&ps_lock);
        }
        epl_sensor_fast_update(obj->client); //if fast to on/off ALS, it read als for DOC_OFF setting for als_operate
        epl_sensor_update_mode(obj->client);
#if PS_DYN_K_ONE
        epl_sensor_do_ps_auto_k_one();
#endif
    }
    return count;
}

static ssize_t epl_sensor_show_cal_raw(struct device_driver *ddri, char *buf)
{
    u16 ch1=0;
    u32 ch1_all=0;
    int count =1;
    int i;
    ssize_t len = 0;

    if(!epl_sensor_obj)
    {
        APS_ERR("epl_sensor_obj is null!!\n");
        return 0;
    }

    for(i=0; i<count; i++)
    {
        msleep(50);
        switch(epl_sensor.mode)
        {
            case EPL_MODE_PS:
                ch1 = factory_ps_data();
                break;

            case EPL_MODE_ALS:

                ch1 = factory_als_data();
                break;
        }

        ch1_all = ch1_all + ch1;
    }

    ch1 = (u16)ch1_all/count;
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

    sscanf(buf, "%d",&epld->hw->polling_mode_ps);

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
    alsps_context_obj->ps_ctl.is_report_input_direct = epl_sensor.ps.polling_mode==0? true:false;
#endif  /*!MTK_LTE*/

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
    alsps_context_obj->als_ctl.is_report_input_direct = epl_sensor.als.polling_mode==0? true:false;
#endif /*!MTK_LTE*/

    //update als intr table
    if(epl_sensor.als.polling_mode == 0)
        als_intr_update_table(epld);

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

    len += snprintf(buf + len, PAGE_SIZE - len, "%d", ps_raw);
    return len;

}

static ssize_t epl_sensor_show_als_data(struct device_driver *ddri, char *buf)
{
    ssize_t len = 0;
    u16 als_raw = 0;

    als_raw = factory_als_data();

    len += snprintf(buf + len, PAGE_SIZE - len, "%d", als_raw);
    return len;

}
#if ALS_DYN_INTT
static ssize_t epl_sensor_store_c_gain(struct device_driver *ddri, const char *buf, size_t count)
{
    struct epl_sensor_priv *epld = epl_sensor_obj;
    int c_h,c_l;
    APS_FUN();

    if(epl_sensor.als.lsrc_type == CMC_BIT_LSRC_NON)
    {
        sscanf(buf, "%d",&c_h);
        c_gain = c_h;
        APS_LOG("c_gain = %d \r\n", c_gain);
    }
    else
    {
        sscanf(buf, "%d,%d",&c_l, &c_h);
        epld->c_gain_h = c_h;
        epld->c_gain_l = c_l;
    }

	return count;
}

static ssize_t epl_sensor_store_lsrc_type(struct device_driver *ddri, const char *buf, size_t count)
{
    int type;
    APS_FUN();
    sscanf(buf, "%d",&type);
    epl_sensor.als.lsrc_type = type;

    APS_LOG("epl_sensor.als.lsrc_type = %d \r\n", epl_sensor.als.lsrc_type);
	return count;
}

static ssize_t epl_sensor_store_lsrc_thd(struct device_driver *ddri, const char *buf, size_t count)
{
    int lsrc_thrl, lsrc_thrh;
    struct epl_sensor_priv *epld = epl_sensor_obj;
    APS_FUN();

    sscanf(buf, "%d,%d",&lsrc_thrl, &lsrc_thrh);

    epld->lsource_thd_low = lsrc_thrl;
    epld->lsource_thd_high = lsrc_thrh;

    APS_LOG("lsource_thd=(%d,%d) \r\n", epld->lsource_thd_low, epld->lsource_thd_high);

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


#if LENOVO_CALI
static ssize_t epl_sensor_store_ps_h_threshold(struct device_driver *ddri, const char *buf, size_t count)
{
    struct epl_sensor_priv *epld = epl_sensor_obj;
    int thr_h;
    APS_FUN();

    sscanf(buf, "%d",&thr_h);

    APS_LOG("[%s]: thr_h=%d", __func__, thr_h);

    epld->hw->ps_threshold_high = thr_h;
    epl_sensor.ps.high_threshold = thr_h;

    set_psensor_intr_threshold(epl_sensor.ps.low_threshold, epl_sensor.ps.high_threshold);
    return count;
}

static ssize_t epl_sensor_show_ps_h_threshold(struct device_driver *ddri, char *buf)
{
    ssize_t len = 0;

    APS_FUN();
    APS_LOG("[%s]: epl_sensor.ps.high_threshold=%d \r\n", __func__, epl_sensor.ps.high_threshold);

    len += snprintf(buf+len, PAGE_SIZE-len, "%d", epl_sensor.ps.high_threshold);

    return len;
}

static ssize_t epl_sensor_store_ps_l_threshold(struct device_driver *ddri, const char *buf, size_t count)
{
    struct epl_sensor_priv *epld = epl_sensor_obj;
    int thr_l;
    APS_FUN();

    sscanf(buf, "%d",&thr_l);

    APS_LOG("[%s]: thr_l=%d", __func__, thr_l);

    epld->hw->ps_threshold_low = thr_l;
    epl_sensor.ps.low_threshold = thr_l;

    set_psensor_intr_threshold(epl_sensor.ps.low_threshold, epl_sensor.ps.high_threshold);
    return count;
}

static ssize_t epl_sensor_show_ps_l_threshold(struct device_driver *ddri, char *buf)
{
    ssize_t len = 0;

    APS_FUN();
    APS_LOG("[%s]: epl_sensor.ps.low_threshold=%d \r\n", __func__, epl_sensor.ps.low_threshold);

    len += snprintf(buf+len, PAGE_SIZE-len, "%d", epl_sensor.ps.low_threshold);

    return len;
}


/*lenovo-sw caoyi1 add for Psensor calibration ATTR 20150511 end*/

/*lenovo-sw caoyi1 add for selftest function 20150715 begin*/
static ssize_t epl_sensor_show_selftest(struct device_driver *ddri, char *buf)
{
      struct epl_sensor_priv *epld = epl_sensor_obj;
      ssize_t len = 0;
      bool result;
      APS_FUN();

      if((i2c_smbus_read_byte_data(epld->client, 0x21)) != 0x91)
      {
            result = false;
      }
      else
      {
            result = true;
      }
      APS_LOG("[%s]: result=%d \r\n", __func__, result);

      len += snprintf(buf + len, PAGE_SIZE - len, "%d", result);
      return len;

}

static ssize_t epl_sensor_show_ps_status(struct device_driver *ddri, char *buf)
{
      ssize_t len = 0;
      int ps_status;
      APS_FUN();

      ps_status = epl_sensor.ps.compare_low >> 3;
      APS_LOG("[%s]: ps_status=%d \r\n", __func__, ps_status);

      len += snprintf(buf + len, PAGE_SIZE - len, "%d", ps_status);
      return len;

}

/*lenovo-sw caoyi1 add ps status for touchpanel 20151013 begin*/
#if 0
int epl_sensor_show_ps_status_for_tp()
{
	struct epl_sensor_priv *obj = epl_sensor_obj;

	if ( obj == NULL ) {
		APS_ERR("null pointer!!\n");
		return -1;
        }
	bool enable_ps = test_bit(CMC_BIT_PS, &obj->enable);
	APS_LOG("[%s]: enable_ps=%d,ps_status=%d \r\n", __func__, enable_ps,epl_sensor.ps.compare_low >> 3);

	if(enable_ps == 1)
		return epl_sensor.ps.compare_low >> 3;
	else
		return -1;

}
#endif
/*lenovo-sw caoyi1 add ps status for touchpanel 20151013 end*/

#endif
static ssize_t epl_sensor_show_als_enable(struct device_driver *ddri, char *buf)
{
      ssize_t len = 0;
      struct epl_sensor_priv *obj = epl_sensor_obj;
      bool enable_als = test_bit(CMC_BIT_ALS, &obj->enable);
      APS_FUN();

      APS_LOG("[%s]: enable_als=%d \r\n", __func__, enable_als);

      len += snprintf(buf + len, PAGE_SIZE - len, "%d", enable_als);
      return len;
}

static ssize_t epl_sensor_show_ps_enable(struct device_driver *ddri, char *buf)
{
      ssize_t len = 0;
      struct epl_sensor_priv *obj = epl_sensor_obj;
      bool enable_ps = test_bit(CMC_BIT_PS, &obj->enable);
      APS_FUN();

      APS_LOG("[%s]: enable_als=%d \r\n", __func__, enable_ps);

      len += snprintf(buf + len, PAGE_SIZE - len, "%d", enable_ps);
      return len;
}

/*CTS --> S_IWUSR | S_IRUGO*/
/*----------------------------------------------------------------------------*/
static DRIVER_ATTR(elan_status,					S_IWUSR  | S_IRUGO, epl_sensor_show_status,  	  		NULL										);
static DRIVER_ATTR(elan_reg,    				S_IWUSR  | S_IRUGO, epl_sensor_show_reg,   				NULL										);
static DRIVER_ATTR(als_enable,					S_IWUSR  | S_IRUGO, epl_sensor_show_als_enable,			epl_sensor_store_als_enable					);
static DRIVER_ATTR(als_report_type,				S_IWUSR  | S_IRUGO, NULL,								epl_sensor_store_als_report_type			);
static DRIVER_ATTR(als_polling_mode,			S_IWUSR  | S_IRUGO, epl_sensor_show_als_polling,   		epl_sensor_store_als_polling_mode			);
static DRIVER_ATTR(als_lux_per_count,			S_IWUSR  | S_IRUGO, NULL,   					 		epl_sensor_store_als_lux_per_count			);
static DRIVER_ATTR(ps_enable,					S_IWUSR  | S_IRUGO, epl_sensor_show_ps_enable,  		epl_sensor_store_ps_enable					);
static DRIVER_ATTR(ps_polling_mode,			    S_IWUSR  | S_IRUGO, epl_sensor_show_ps_polling,   		epl_sensor_store_ps_polling_mode			);
static DRIVER_ATTR(ir_mode,					    S_IWUSR  | S_IRUGO, NULL,   							epl_sensor_store_ir_mode					);
static DRIVER_ATTR(ir_drive,					S_IWUSR  | S_IRUGO, NULL,   							epl_sensor_store_ir_drive					);
static DRIVER_ATTR(ir_on,						S_IWUSR  | S_IRUGO, NULL,								epl_sensor_store_ir_contrl					);
static DRIVER_ATTR(interrupt_type,				S_IWUSR  | S_IRUGO, NULL,								epl_sensor_store_interrupt_type				);
static DRIVER_ATTR(integration,					S_IWUSR  | S_IRUGO, NULL,								epl_sensor_store_integration				);
static DRIVER_ATTR(gain,					    S_IWUSR  | S_IRUGO, NULL,								epl_sensor_store_gain					    );
static DRIVER_ATTR(adc,					        S_IWUSR  | S_IRUGO, NULL,								epl_sensor_store_adc						);
static DRIVER_ATTR(cycle,						S_IWUSR  | S_IRUGO, NULL,								epl_sensor_store_cycle						);
static DRIVER_ATTR(mode,						S_IWUSR  | S_IRUGO, NULL,   							epl_sensor_store_mode						);
static DRIVER_ATTR(wait_time,					S_IWUSR  | S_IRUGO, NULL,   					 		epl_sensor_store_wait_time					);
static DRIVER_ATTR(set_threshold,     			S_IWUSR  | S_IRUGO, epl_sensor_show_threshold,                epl_sensor_store_threshold			);
static DRIVER_ATTR(cal_raw, 					S_IWUSR  | S_IRUGO, epl_sensor_show_cal_raw, 	  		NULL										);
static DRIVER_ATTR(unlock,				        S_IWUSR  | S_IRUGO, NULL,			                    epl_sensor_store_unlock						);
static DRIVER_ATTR(als_ch,				        S_IWUSR  | S_IRUGO, NULL,			                    epl_sensor_store_als_ch_sel					);
static DRIVER_ATTR(ps_cancel,				    S_IWUSR  | S_IRUGO, NULL,			                    epl_sensor_store_ps_cancelation				);
static DRIVER_ATTR(run_ps_cali, 				S_IWUSR  | S_IRUGO, epl_sensor_show_ps_run_cali, 	  	NULL								    	);
static DRIVER_ATTR(pdata,                       S_IWUSR  | S_IRUGO, epl_sensor_show_pdata,              NULL                                        );
static DRIVER_ATTR(als_data,                    S_IWUSR  | S_IRUGO, epl_sensor_show_als_data,           NULL                                        );
static DRIVER_ATTR(ps_w_calfile,				S_IWUSR  | S_IRUGO, NULL,			                    epl_sensor_store_ps_w_calfile				);
#if ALS_DYN_INTT
static DRIVER_ATTR(als_dyn_c_gain,              S_IWUSR  | S_IRUGO, NULL,                               epl_sensor_store_c_gain                     );
static DRIVER_ATTR(als_dyn_lsrc_type,           S_IWUSR  | S_IRUGO, NULL,                               epl_sensor_store_lsrc_type                  );
static DRIVER_ATTR(als_dyn_lsrc_thd,            S_IWUSR  | S_IRUGO, NULL,                               epl_sensor_store_lsrc_thd                   );
#endif
static DRIVER_ATTR(i2c_w,                       S_IWUSR  | S_IRUGO, NULL,                               epl_sensor_store_reg_write                  );
static DRIVER_ATTR(elan_renvo,                  S_IWUSR  | S_IRUGO, epl_sensor_show_renvo,              NULL                                        );
static DRIVER_ATTR(als_intr_table,				S_IWUSR  | S_IRUGO, epl_sensor_show_als_intr_table,     NULL                                        );
#if LENOVO_CALI
static DRIVER_ATTR(cali_param_1,                S_IWUSR  | S_IRUGO, epl_sensor_show_ps_h_threshold,     epl_sensor_store_ps_h_threshold                  );
static DRIVER_ATTR(cali_param_2,                S_IWUSR  | S_IRUGO, epl_sensor_show_ps_l_threshold,     epl_sensor_store_ps_l_threshold                  );
static DRIVER_ATTR(selftest,                S_IWUSR  | S_IRUGO, epl_sensor_show_selftest,           NULL                  );
static DRIVER_ATTR(ps_status,                S_IWUSR  | S_IRUGO, epl_sensor_show_ps_status,           NULL                  );
#endif
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
#if ALS_DYN_INTT
    &driver_attr_als_dyn_c_gain,
    &driver_attr_als_dyn_lsrc_type,
    &driver_attr_als_dyn_lsrc_thd,
#endif
    &driver_attr_i2c_w,
    &driver_attr_als_intr_table,
#if LENOVO_CALI
    &driver_attr_cali_param_1,
    &driver_attr_cali_param_2,
    &driver_attr_selftest,
    &driver_attr_ps_status,
#endif
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
#if 0
static int epl_sensor_ps_average_val = 0;

static void epl_sensor_WriteCalibration(struct epl_sensor_priv *obj, HWMON_PS_STRUCT *data_cali)
{
	// struct PS_CALI_DATA_STRUCT *ps_data_cali;
	APS_LOG("le_WriteCalibration  1 %d," ,data_cali->close);
	APS_LOG("le_WriteCalibration  2 %d," ,data_cali->far_away);
	APS_LOG("le_WriteCalibration  3 %d,", data_cali->valid);
	//APS_LOG("le_WriteCalibration  4 %d,", data_cali->pulse);

	if(data_cali->valid == 1)
	{
		//atomic_set(&obj->ps_thd_val_high, data_cali->close);
		//atomic_set(&obj->ps_thd_val_low, data_cali->far_away);
		epl_sensor.ps.high_threshold = data_cali->close;
		epl_sensor.ps.low_threshold = data_cali->far_away;
	}else{
		/*lenovo-sw molg1 add default value for uncalibration 20141222 begin*/
		epl_sensor.ps.high_threshold = 4000;
		epl_sensor.ps.low_threshold = 3000;
		/*lenovo-sw molg1 add default value for uncalibration 20141222 end*/
	}
}

static int epl_sensor_read_data_for_cali(struct i2c_client *client, HWMON_PS_STRUCT *ps_data_cali)
{
	/*Lenovo-sw caoyi1 modify for proximity sensor cali raw data 20140717 begin*/
	//struct epl_sensor_priv *obj = i2c_get_clientdata(client);
	int als_time = 0, ps_time = 0;
	/*Lenovo-sw caoyi1 modify for proximity sensor cali raw data 20140717 end*/
	//u8 databuf[2];
	//u8 buffer[2];
	int i=0 ,res = 0;
	/*Lenovo huangdra 20130713 modify u16 will overflow,change to u32*/
	u16 data[32];
	u32 sum=0, data_cali=0;

	als_time = als_sensing_time(epl_sensor.als.integration_time, epl_sensor.als.adc, epl_sensor.als.cycle);
    ps_time = ps_sensing_time(epl_sensor.ps.integration_time, epl_sensor.ps.adc, epl_sensor.ps.cycle);

	for(i = 0; i < 10; i++)
	{
		mdelay(als_time+ps_time+wait_value[epl_sensor.wait>>4]);//50

        data[i] = factory_ps_data();

		if(res != 0)
		{
			APS_ERR("epl_sensor_read_data_for_cali fail: %d\n", i);
			break;
		}
		else
		{
			APS_LOG("[%d]sample = %d\n", i, data[i]);
			sum += data[i];
		}
	}


	if(i < 10)
	{
		res=  -1;

		return res;
	}
	else
	{
		data_cali = sum / 10;
		epl_sensor_ps_average_val = data_cali;
		APS_LOG("epl_sensor_read_data_for_cali data = %d",data_cali);
		/*Lenovo-sw caoyi1 modify for Aubest p-sensor calibration 2014-08-06 start*/
		if( data_cali>6000)
		{
			APS_ERR("epl_sensor_read_data_for_cali fail value to high: %d\n", data_cali);
			return -2;
		}
#if PS_DYN_K_ONE
		if((data_cali>0)&&(data_cali <=10))
		{
			ps_data_cali->close =data_cali + dynk_thd_high;
			ps_data_cali->far_away =data_cali + dynk_thd_low;
		}
		else if(data_cali <=100)
		{
			ps_data_cali->close =data_cali + dynk_thd_high;
			ps_data_cali->far_away =data_cali + dynk_thd_low;
		}
		else if(data_cali <=200)
		{
			ps_data_cali->close =data_cali + dynk_thd_high;
			ps_data_cali->far_away =data_cali + dynk_thd_low;
		}
		else if(data_cali<6000)
		{
			ps_data_cali->close =data_cali + dynk_thd_high;
			ps_data_cali->far_away =data_cali + dynk_thd_low;
		}
		else
		{
			APS_ERR("epl_sensor_read_data_for_cali IR current 0\n");
			return -2;
		}
#else
		if((data_cali>0)&&(data_cali <=10))
		{
			ps_data_cali->close =data_cali + 460;
			ps_data_cali->far_away =data_cali + 200;
		}
		else if(data_cali <=100)
		{
			ps_data_cali->close =data_cali + 460;
			ps_data_cali->far_away =data_cali + 200;
		}
		else if(data_cali <=200)
		{
			ps_data_cali->close =data_cali + 460;
			ps_data_cali->far_away =data_cali + 200;
		}
		else if(data_cali<6000)
		{
			ps_data_cali->close =data_cali + 460;
			ps_data_cali->far_away =data_cali + 200;
		}
		else
		{
			APS_ERR("epl_sensor_read_data_for_cali IR current 0\n");
			return -2;
		}
#endif
		ps_data_cali->valid = 1;
		/*Lenovo-sw caoyi1 modify for Aubest p-sensor calibration 2014-08-06 end*/
		APS_LOG("epl_sensor_read_data_for_cali close = %d,far_away = %d,valid = %d\n",ps_data_cali->close,ps_data_cali->far_away,ps_data_cali->valid);
		//APS_LOG("rpr400_read_data_for_cali avg=%d max=%d min=%d\n",data_cali, max, min);
	}

	return res;
}
#endif

/*----------------------------------------------------------------------------*/
static long epl_sensor_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct i2c_client *client = (struct i2c_client*)file->private_data;
    struct epl_sensor_priv *obj = i2c_get_clientdata(client);
    int err=0, ps_result=0, threshold[2], als_data=0;
    void __user *ptr = (void __user*) arg;
    int dat;
    uint32_t enable;
    bool enable_ps = test_bit(CMC_BIT_PS, &obj->enable) && atomic_read(&obj->ps_suspend)==0;
    bool enable_als = test_bit(CMC_BIT_ALS, &obj->enable) && atomic_read(&obj->als_suspend)==0;
#if LENOVO_CALI
    //int ps_cali;
    //HWMON_PS_STRUCT ps_cali_temp;
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

            if(enable_ps != enable)
            {
                if(enable)
                {
                    //wake_lock(&ps_lock);
                    set_bit(CMC_BIT_PS, &obj->enable);
#if PS_FIRST_REPORT
            ps_frist_flag = true;
#endif
#if PS_DYN_K_ONE
                    ps_dyn_flag = true;
#endif
                }
                else
                {
                    clear_bit(CMC_BIT_PS, &obj->enable);
                    //wake_unlock(&ps_lock);
                }
                epl_sensor_fast_update(obj->client); //if fast to on/off ALS, it read als for DOC_OFF setting for als_operate
                epl_sensor_update_mode(obj->client);
#if PS_DYN_K_ONE
                epl_sensor_do_ps_auto_k_one();
#endif
            }
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
            if(enable_als != enable)
            {
                if(enable)
                {
                    set_bit(CMC_BIT_ALS, &obj->enable);
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
                    epl_sensor_fast_update(obj->client);
                }
                else
                {
                    clear_bit(CMC_BIT_ALS, &obj->enable);
                }
                epl_sensor_update_mode(obj->client);
#if ALS_DYN_INTT_REPORT
                if(enable == 1)
                    epl_sensor_als_dyn_report(true);
#endif
            }
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
#if 1 //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

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
#endif //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
		/*------------------------------------------------------------------------------------------*/
/*lenovo-sw caoyi1 modify for PS calibration begin*/
#if LENOVO_CALI
            case ALSPS_SET_PS_CALI:
        			//if(copy_from_user(&ps_cali_temp, ptr, sizeof(ps_cali_temp)))
        			//{
        			//	APS_LOG("copy_from_user\n");
        			//	err = -EFAULT;
        			//	break;
        			//}
        			//epl_sensor_WriteCalibration(obj, &ps_cali_temp);
        			//APS_LOG(" ALSPS_SET_PS_CALI %d,%d,%d\t",ps_cali_temp.close,ps_cali_temp.far_away,ps_cali_temp.valid);
        	break;

            case ALSPS_GET_PS_RAW_DATA_FOR_CALI:
        			{

				//cancel_delayed_work_sync(&obj->eint_work);
				//disable_irq(epl_sensor_obj->irq);
        				//err = epl_sensor_read_data_for_cali(obj->client,&ps_cali_temp);
        				//if(err < 0 ){
        				//	goto err_out;
        				//}
        				//epl_sensor_WriteCalibration(obj, &ps_cali_temp);
        				//if(copy_to_user(ptr, &ps_cali_temp, sizeof(ps_cali_temp)))
        				//{
        				//	err = -EFAULT;
        				//	goto err_out;
        				//}
        			}
        	break;

            case ALSPS_GET_PS_AVERAGE:
        			//enable = epl_sensor_ps_average_val;
        			//if(copy_to_user(ptr, &enable, sizeof(enable)))
        			//{
        			//	err = -EFAULT;
        			//	goto err_out;
        			//}
        	break;
        			//lenovo-sw, shanghai, add by chenlj2, for geting ps average val, 2012-05-14 end
        			//lenovo-sw, shanghai, add by chenlj2, for AVAGO project, 2012-10-09 start
        		case ALSPS_GET_PS_FAR_THRESHOLD:
        			enable = epl_sensor.ps.low_threshold;
        			if(copy_to_user(ptr, &enable, sizeof(enable)))
        			{
        				err = -EFAULT;
        				goto err_out;
        			}
        			break;
        		case ALSPS_GET_PS_CLOSE_THRESHOLD:
        			enable = epl_sensor.ps.high_threshold;
        			if(copy_to_user(ptr, &enable, sizeof(enable)))
        			{
        				err = -EFAULT;
        				goto err_out;
        			}
        			break;
        			//lenovo-sw, shanghai, add by chenlj2, for AVAGO project, 2012-10-09 end
        			/*Lenovo-sw caoyi1 for proximity sensor cali 20140711 end*/

#endif
/*lenovo-sw caoyi1 modify for PS calibration end*/
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
    //bool enable_ps = test_bit(CMC_BIT_PS, &obj->enable) && atomic_read(&obj->ps_suspend)==0;
    //bool enable_als = test_bit(CMC_BIT_ALS, &obj->enable) && atomic_read(&obj->als_suspend)==0;
    APS_FUN();

    if(!obj)
    {
        APS_ERR("null pointer!!\n");
        return -EINVAL;
    }
#if 0
    if(msg.event == PM_EVENT_SUSPEND)
    {
        if(!obj)
        {
            APS_ERR("null pointer!!\n");
            return -EINVAL;
        }
        atomic_set(&obj->als_suspend, 1);
        if(enable_ps == 1){
            atomic_set(&obj->ps_suspend, 0);
            APS_LOG("[%s]: ps enable \r\n", __func__);
        }
        else{
            atomic_set(&obj->ps_suspend, 1);
            APS_LOG("[%s]: ps disable \r\n", __func__);
            epl_sensor_update_mode(obj->client);
        }

        epl_sensor_power(obj->hw, 0);
    }
#endif
    return 0;

}



/*----------------------------------------------------------------------------*/
static int epl_sensor_i2c_resume(struct i2c_client *client)
{
    struct epl_sensor_priv *obj = i2c_get_clientdata(client);
    //bool enable_ps = test_bit(CMC_BIT_PS, &obj->enable) && atomic_read(&obj->ps_suspend)==0;
    //bool enable_als = test_bit(CMC_BIT_ALS, &obj->enable) && atomic_read(&obj->als_suspend)==0;
    APS_FUN();
    if(!obj)
    {
        APS_ERR("null pointer!!\n");
        return -EINVAL;
    }
#if 0
    atomic_set(&obj->ps_suspend, 0);
    atomic_set(&obj->als_suspend, 0);
    if(enable_ps == 1)
    {
        APS_LOG("[%s]: ps enable \r\n", __func__);
    }
    else
    {
        APS_LOG("[%s]: ps disable \r\n", __func__);
        epl_sensor_update_mode(obj->client);
    }

    epl_sensor_power(obj->hw, 1);
#endif
    return 0;
}



/*----------------------------------------------------------------------------*/
#if defined(CONFIG_HAS_EARLYSUSPEND)
static void epl_sensor_early_suspend(struct early_suspend *h)
{
    /*early_suspend is only applied for ALS*/
    struct epl_sensor_priv *obj = container_of(h, struct epl_sensor_priv, early_drv);
    bool enable_ps = test_bit(CMC_BIT_PS, &obj->enable) && atomic_read(&obj->ps_suspend)==0;
    bool enable_als = test_bit(CMC_BIT_ALS, &obj->enable) && atomic_read(&obj->als_suspend)==0;

    APS_FUN();

    if(!obj)
    {
        APS_ERR("null pointer!!\n");
        return;
    }
#if LENOVO_SUSPEND_PATCH
    APS_LOG("[%s]: ps=%d, als=%d \r\n", __func__, enable_ps, enable_als);
#else
    atomic_set(&obj->als_suspend, 1);
    if(enable_ps == 1){
        //atomic_set(&obj->ps_suspend, 0);

        APS_LOG("[%s]: ps enable \r\n", __func__);
    }
    else{
        //atomic_set(&obj->ps_suspend, 1);
        APS_LOG("[%s]: ps disable \r\n", __func__);
        epl_sensor_update_mode(obj->client);
    }
#endif
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
#if LENOVO_SUSPEND_PATCH
    APS_LOG("[%s]: ps=%d, als=%d \r\n", __func__, enable_ps, enable_als);
#else
    atomic_set(&obj->als_suspend, 0);
    atomic_set(&obj->ps_suspend, 0);

    if(enable_ps == 1)
    {
        //atomic_set(&obj->ps_suspend, 0);
        APS_LOG("[%s]: ps enable \r\n", __func__);
    }
    else
    {
        APS_LOG("[%s]: ps disable \r\n", __func__);
        epl_sensor_fast_update(obj->client);
        epl_sensor_update_mode(obj->client);
    }
#endif
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
    struct epl_sensor_priv *obj = epl_sensor_obj;
    bool enable_als = test_bit(CMC_BIT_ALS, &obj->enable) && atomic_read(&obj->als_suspend)==0;
	if(!obj)
	{
		APS_ERR("obj is null!!\n");
		return -1;
	}
	APS_LOG("[%s] als enable en = %d\n", __func__, en);

    if(enable_als != en)
    {
        if(en)
        {
            set_bit(CMC_BIT_ALS, &obj->enable);
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
            epl_sensor_fast_update(obj->client);
        }
        else
        {
            clear_bit(CMC_BIT_ALS, &obj->enable);
        }
        epl_sensor_update_mode(obj->client);
#if ALS_DYN_INTT_REPORT
        if(en == 1)
            epl_sensor_als_dyn_report(true);
#endif
    }

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
#if ALS_DYN_INTT_REPORT
    report_lux = epl_sensor_als_dyn_report(false);
#else
    report_lux = epl_sensor_get_als_value(obj, epl_sensor.als.data.channels[1]);
#endif
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
	struct epl_sensor_priv *obj = epl_sensor_obj;
    struct i2c_client *client = obj->client;
    bool enable_ps = test_bit(CMC_BIT_PS, &obj->enable) && atomic_read(&obj->ps_suspend)==0;

    APS_LOG("ps enable = %d\n", en);

    if(enable_ps != en)
    {
        if(en)
        {
            //wake_lock(&ps_lock);
            set_bit(CMC_BIT_PS, &obj->enable);
#if PS_FIRST_REPORT
            ps_frist_flag = true;
#endif
#if PS_DYN_K_ONE
            ps_dyn_flag = true;
#endif
        }
        else
        {

            clear_bit(CMC_BIT_PS, &obj->enable);
            //wake_unlock(&ps_lock);
        }
        epl_sensor_fast_update(client); //if fast to on/off ALS, it read als for DOC_OFF setting for als_operate
        epl_sensor_update_mode(client);
#if PS_DYN_K_ONE
        epl_sensor_do_ps_auto_k_one();
#endif
    }

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
    struct epl_sensor_priv *obj = epl_sensor_obj;
    int err = 0;
	if(!obj)
	{
		APS_ERR("obj is null!!\n");
		return -1;
	}
    APS_LOG("---SENSOR_GET_DATA---\n\n");

    epl_sensor_read_ps(obj->client);

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
    struct epl_sensor_priv *obj = (struct epl_sensor_priv *)self;
    struct i2c_client *client = obj->client;
    bool enable_ps = test_bit(CMC_BIT_PS, &obj->enable) && atomic_read(&obj->ps_suspend)==0;

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
                APS_LOG("ps enable = %d\n", value);

                if(enable_ps != value)
                {
                    if(value)
                    {
                        //wake_lock(&ps_lock);
                        set_bit(CMC_BIT_PS, &obj->enable);
#if PS_FIRST_REPORT
                        ps_frist_flag = true;
#endif
#if PS_DYN_K_ONE
                        ps_dyn_flag = true;
#endif
                    }
                    else
                    {

                        clear_bit(CMC_BIT_PS, &obj->enable);
                        //wake_unlock(&ps_lock);
                    }
                    epl_sensor_fast_update(client); //if fast to on/off ALS, it read als for DOC_OFF setting for als_operate
                    epl_sensor_update_mode(client);
#if PS_DYN_K_ONE
                    epl_sensor_do_ps_auto_k_one();
#endif
                }
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

                epl_sensor_read_ps(client);

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
    struct i2c_client *client = obj->client;
    bool enable_als = test_bit(CMC_BIT_ALS, &obj->enable) && atomic_read(&obj->als_suspend)==0;
    int report_lux = 0;
    APS_FUN();
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
                APS_LOG("als enable = %d\n", value);

                value = *(int *)buff_in;
                if(enable_als != value)
                {
                    if(value)
                    {
                        set_bit(CMC_BIT_ALS, &obj->enable);
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
                        epl_sensor_fast_update(client);

                    }
                    else
                    {
                        clear_bit(CMC_BIT_ALS, &obj->enable);
                    }
                    epl_sensor_update_mode(client);
#if ALS_DYN_INTT_REPORT
                    if(value == 1)
                        epl_sensor_als_dyn_report(true);
#endif
                }
            }
            break;


        case SENSOR_GET_DATA:
            APS_LOG("get als data !!!!!!\n");

            if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
            {
                APS_ERR("get sensor data parameter error!\n");
                err = -EINVAL;
            }
            else
            {
                epl_sensor_read_als(client);
#if ALS_DYN_INTT_REPORT
                report_lux = epl_sensor_als_dyn_report(false);
#else
                report_lux = epl_sensor_get_als_value(obj, epl_sensor.als.data.channels[1]);
#endif
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


static int als_intr_update_table(struct epl_sensor_priv *epld)
{
	int i;
	for (i = 0; i < 10; i++) {
		if ( als_lux_intr_level[i] < 0xFFFF )
		{
#if ALS_DYN_INTT
		    if(epl_sensor.als.report_type == CMC_BIT_DYN_INT)
		    {
                als_adc_intr_level[i] = als_lux_intr_level[i] * 1000 / c_gain;   //Robert, overflow issue 20150708
		    }
		    else
#endif
		    {
                als_adc_intr_level[i] = als_lux_intr_level[i] * 1000 / epl_sensor.als.factory.lux_per_count;   //Robert, overflow issue 20150708
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
#if 0
static int epl_sensor_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info)
{
    strcpy(info->type, EPL_DEV_NAME);
    return 0;
}
#endif

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

    if((i2c_smbus_read_byte_data(client, 0x20)) != EPL_REVNO){ //check chip
        APS_LOG("elan ALS/PS sensor is failed. \n");
        err = -1;
        goto exit;
    }
#if !ANDR_M
    client->timing = 400;
#endif /*ANDR_M*/
    if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
    {
        err = -ENOMEM;
        goto exit;
    }

    memset(obj, 0, sizeof(*obj));

/*lenovo-sw caoyi1 modify begin*/
    //epl_sensor_obj = obj;
/*lenovo-sw caoyi1 modify end*/
#if ANDR_M
    obj->hw =hw;
#else
    obj->hw = get_cust_alsps_hw();
#endif /*ANDR_M*/
    epl_sensor_get_addr(obj->hw, &obj->addr);

    obj->als_level_num = sizeof(obj->hw->als_level)/sizeof(obj->hw->als_level[0]);
    obj->als_value_num = sizeof(obj->hw->als_value)/sizeof(obj->hw->als_value[0]);
    BUG_ON(sizeof(obj->als_level) != sizeof(obj->hw->als_level));
    memcpy(obj->als_level, obj->hw->als_level, sizeof(obj->als_level));
    BUG_ON(sizeof(obj->als_value) != sizeof(obj->hw->als_value));
    memcpy(obj->als_value, obj->hw->als_value, sizeof(obj->als_value));

    INIT_DELAYED_WORK(&obj->eint_work, epl_sensor_eint_work);
    obj->client = client;

    mutex_init(&sensor_mutex);
    wake_lock_init(&ps_lock, WAKE_LOCK_SUSPEND, "ps wakelock");

    i2c_set_clientdata(client, obj);

    atomic_set(&obj->als_suspend, 0);
    atomic_set(&obj->ps_suspend, 0);

/*lenovo-sw caoyi modify for eint API 20151021 begin*/
    obj->irq_node = of_find_compatible_node(NULL, NULL, "mediatek, ALS-eint");
/*lenovo-sw caoyi modify for eint API 20151021 end*/

    obj->enable = 0;
/*lenovo-sw caoyi1 modify begin*/
    epl_sensor_obj = obj;
/*lenovo-sw caoyi1 modify end*/
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
	als_ctl.is_report_input_direct = epl_sensor.als.polling_mode==0? true:false; //false;
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
	ps_ctl.is_report_input_direct = epl_sensor.ps.polling_mode==0? true:false; //false;
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

/*lenovo-sw caoyi1 modify begin*/
#if ANDR_M
    if(obj->hw->polling_mode_ps == 0 || obj->hw->polling_mode_als == 0)
        epl_sensor_setup_eint(client);
#endif
/*lenovo-sw caoyi1 modify end*/

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
#if !ANDR_M
    struct alsps_hw *hw = get_cust_alsps_hw();
#endif

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
#if !ANDR_M
    struct alsps_hw *hw = get_cust_alsps_hw();
#endif
    APS_FUN();
    epl_sensor_power(hw, 0);

    APS_ERR("EPL8802 remove \n");
    i2c_del_driver(&epl_sensor_i2c_driver);
    return 0;
}



/*----------------------------------------------------------------------------*/
static struct platform_driver epl_sensor_alsps_driver =
{
    .probe      = epl_sensor_probe,
    .remove     = epl_sensor_remove,
    .driver     = {
        .name  = "als_ps",
        //.owner = THIS_MODULE,
    }
};
#endif

#if MTK_LTE
/*----------------------------------------------------------------------------*/
static int alsps_local_init(void)
{
#if !ANDR_M
    struct alsps_hw *hw = get_cust_alsps_hw();
#endif
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
#if !ANDR_M
    struct alsps_hw *hw = get_cust_alsps_hw();
#endif
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
#if ANDR_M
    const char *name = COMPATIABLE_NAME;
#else
    struct alsps_hw *hw = get_cust_alsps_hw();
#endif

    APS_FUN();
#if ANDR_M
    hw = get_alsps_dts_func(name, hw);

    if (!hw)
    {
    	APS_ERR("get dts info fail\n");
    }
#else
    i2c_register_board_info(hw->i2c_num, &i2c_epl_sensor, 1);
#endif  /*ANDR_M*/
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
MODULE_DESCRIPTION("EPL8802 ALPsr driver");
MODULE_LICENSE("GPL");

