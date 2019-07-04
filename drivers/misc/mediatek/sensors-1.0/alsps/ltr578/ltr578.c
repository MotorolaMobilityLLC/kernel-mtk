/*******************************************************************************
 *
 *Copyright (c) 2014-2015, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *******************************************************************************/


#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include "cust_alsps.h"
#include "ltr578.h"
#include "alsps.h"
/**********  it`s only for compile            ****************/
/* TODO: it`s only for compile  by wangyang for smt */
#define CUST_EINT_ALS_NUM			11
#define CUST_EINT_ALS_DEBOUNCE_CN	1
/**************************************************/

//+ add by fanxzh
//#define GN_MTK_BSP_PS_DYNAMIC_CALI
//- add by fanxzh
//#define PSENSOR_CALIBRATION

#ifdef PSENSOR_CALIBRATION
#define MAX_PS_NOISE 1330  // 65% 0f max ps_data
#define AVG_PS_NOISE 20      //average of this project
#endif

#define POWER_NONE_MACRO MT65XX_POWER_NONE
#define CUST_EINT_ALS_TYPE 8

/******************************************************************************
 * configuration
 *******************************************************************************/
/*----------------------------------------------------------------------------*/

#define LTR578_DEV_NAME   "LTR_578ALS"

/*----------------------------------------------------------------------------*/
#define LTR578_DEBUG   1
#define APS_TAG                  "[ALS/PS llssyy] "
#define APS_FUN(f)               pr_err(APS_TAG"%s\n", __func__)
#define APS_ERR(fmt, args...)    pr_err(APS_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#ifdef LTR578_DEBUG
//#define APS_LOG(fmt, args...)    pr_err(APS_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
//#define APS_DBG(fmt, args...)    pr_err(APS_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#define APS_LOG(fmt, args...)    pr_err(APS_TAG fmt, ##args)
#define APS_DBG(fmt, args...)    pr_err(APS_TAG fmt, ##args)
#else
#define APS_LOG(fmt, args...)    pr_debug(APS_TAG fmt, ##args)
#define APS_DBG(fmt, args...)    pr_debug(APS_TAG fmt, ##args)
#endif

//#define CHECK_FAIL_WORK

extern void mt_eint_mask(unsigned int eint_num);
extern void mt_eint_unmask(unsigned int eint_num);
extern void mt_eint_set_hw_debounce(unsigned int eint_num, unsigned int ms);
extern void mt_eint_set_polarity(unsigned int eint_num, unsigned int pol);
extern unsigned int mt_eint_set_sens(unsigned int eint_num, unsigned int sens);
extern void mt_eint_registration(unsigned int eint_num, unsigned int flow, void (EINT_FUNC_PTR)(void), unsigned int is_auto_umask);
extern void mt_eint_print_status(void);
/*----------------------------------------------------------------------------*/

static struct i2c_client *ltr578_i2c_client = NULL;
static int ltr578_init_flag =  -1;


#ifdef GN_MTK_BSP_PS_DYNAMIC_CALI
static int isadjust;
#endif
static int dynamic_cali = 2047;
#define MTK_AUTO_DETECT_ALSPS
/*---------------------CONFIG_OF START------------------*/

struct alsps_hw alsps_cust_ltr;
static struct alsps_hw *hw = &alsps_cust_ltr;


/* For alsp driver get cust info */
struct alsps_hw *get_cust_alsps_ltr(void)
{
		return &alsps_cust_ltr;
}
/*---------------------CONFIG_OF	END------------------*/


/*----------------------------------------------------------------------------*/
static const struct i2c_device_id ltr578_i2c_id[] = {{LTR578_DEV_NAME, 0}, {} };
/*the adapter id & i2c address will be available in customization*/
//static struct i2c_board_info __initdata i2c_ltr578={ I2C_BOARD_INFO("LTR_578ALS", 0x53)}; 

/* static unsigned short ltr578_force[] = {0x00, 0x46, I2C_CLIENT_END, I2C_CLIENT_END}; */
/* static const unsigned short *const ltr578_forces[] = { ltr578_force, NULL }; */
/* static struct i2c_client_address_data ltr578_addr_data = { .forces = ltr578_forces,}; */
/*----------------------------------------------------------------------------*/
static int ltr578_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int ltr578_i2c_remove(struct i2c_client *client);
static int ltr578_i2c_detect(struct i2c_client *client, struct i2c_board_info *info);
/*----------------------------------------------------------------------------*/
static int ltr578_i2c_suspend(struct device *dev);
static int ltr578_i2c_resume(struct device *dev);
static int ltr578_ps_enable(void);
static int ltr578_ps_disable(void);
static int ltr578_als_disable(void);
static int ltr578_als_enable(int gainrange);
#ifdef PSENSOR_CALIBRATION
static int ltr578_psensor_calibrate(void);
static int ltr578_get_init_noise(void);
#endif

static int als_gainrange = 18;

static int final_prox_val;

/*----------------------------------------------------------------------------*/
static DEFINE_MUTEX(read_lock);

/*----------------------------------------------------------------------------*/
static int ltr578_als_read(int gainrange);
static int ltr578_ps_read(void);

static int ltr578_local_init(void);
static int ltr578_local_uninit(void);
/*----------------------------------------------------------------------------*/
static struct alsps_init_info ltr578_init_info = {
		//.name = "ltr578-new",
		.name = LTR578_DEV_NAME,
		//	.name = "LTR_578ALS",
		.init = ltr578_local_init,
		.uninit = ltr578_local_uninit,
};



typedef enum {
		CMC_BIT_ALS    = 1,
		CMC_BIT_PS	   = 2,
} CMC_BIT;

/*----------------------------------------------------------------------------*/
struct ltr578_i2c_addr {	/*define a series of i2c slave address*/
		u8	write_addr;
		u8	ps_thd;	/*PS INT threshold*/
};

/*----------------------------------------------------------------------------*/

struct ltr578_priv {
		struct alsps_hw  *hw;
		struct i2c_client *client;
		struct work_struct	eint_work;
		struct mutex lock;
		/*i2c address group*/
		struct ltr578_i2c_addr	addr;
		struct device_node *irq_node;
		int irq;
#ifdef CONFIG_HQ_CTP_GESTRUE_ANTI_FALSE_TOUCH
		atomic_t	ps_state;			// add by 20160130
#endif
		/*misc*/
		u16			als_modulus;
		atomic_t	i2c_retry;
		atomic_t	als_debounce;	/*debounce time after enabling als*/
		atomic_t	als_deb_on;	/*indicates if the debounce is on*/
		atomic_t	als_deb_end;	/*the jiffies representing the end of debounce*/
		atomic_t	ps_mask;		/*mask ps: always return far away*/
		atomic_t	ps_debounce;	/*debounce time after enabling ps*/
		atomic_t	ps_deb_on;		/*indicates if the debounce is on*/
		atomic_t	ps_deb_end;	/*the jiffies representing the end of debounce*/
		atomic_t	ps_suspend;
		atomic_t	als_suspend;

		/*data*/
		u16		als;
		u16		 ps;
		u8			_align;
		u16		als_level_num;
		u16		als_value_num;
		u32		als_level[TP_COUNT][TEMP_COUNT][C_CUST_ALS_LEVEL];
		u32		als_value[C_CUST_ALS_LEVEL];
		int			ps_cali;

		atomic_t	als_cmd_val;	/*the cmd value can't be read, stored in ram*/
		atomic_t	ps_cmd_val;	/*the cmd value can't be read, stored in ram*/
		atomic_t	ps_thd_val;	/*the cmd value can't be read, stored in ram*/
		atomic_t	ps_thd_val_high;	 /*the cmd value can't be read, stored in ram*/
		atomic_t	ps_thd_val_low;	/*the cmd value can't be read, stored in ram*/
		ulong		enable;		/*enable mask*/
		ulong		pending_intr;	/*pending interrupt*/
#ifdef PSENSOR_CALIBRATION	
		int 		init_noise;
		int 		current_xtalk;
#endif
		/*early suspend*/
#if defined(CONFIG_HAS_EARLYSUSPEND)
		struct early_suspend	early_drv;
#endif
};

static unsigned int current_tp = 0;
static unsigned int current_color_temp=CWF_TEMP;

struct PS_CALI_DATA_STRUCT {
		int close;
		int far_away;
		int valid;
};

static struct PS_CALI_DATA_STRUCT ps_cali = {0, 0, 0};
static int intr_flag_value;


static struct ltr578_priv *ltr578_obj = NULL;
/* static struct platform_driver ltr578_alsps_driver; */
#ifdef CONFIG_OF
static const struct of_device_id alsps_of_match[] = {
		{.compatible = "mediatek,alsps"},
		{},
};
#endif

/*----------------------------------------------------------------------------*/
#ifdef CONFIG_PM_SLEEP
static const struct dev_pm_ops ltr578_pm_ops = {
		SET_SYSTEM_SLEEP_PM_OPS(ltr578_i2c_suspend, ltr578_i2c_resume)
};
#endif

static struct i2c_driver ltr578_i2c_driver = {
		.probe		= ltr578_i2c_probe,
		.remove	= ltr578_i2c_remove,
		.detect	= ltr578_i2c_detect,
		.id_table	= ltr578_i2c_id,
		/* .address_data = &ltr578_addr_data, */
		.driver = {
				/* .owner		   = THIS_MODULE, */
				.name			= LTR578_DEV_NAME,
#ifdef CONFIG_PM_SLEEP
				.pm   = &ltr578_pm_ops,
#endif
#ifdef CONFIG_OF
				.of_match_table = alsps_of_match,
#endif
		},
};

//+add by hzb for ontim debug
#ifdef ONTIM_CALI
static int ltr578_prox_set_noice(int noice);
#endif
#include <ontim/ontim_dev_dgb.h>
static char ltr578_prox_version[]="ltr578_mtk_1.0";
static char ltr578_prox_vendor_name[20]="LITE-ON-ltr578";
DEV_ATTR_DECLARE(als_prox)
		DEV_ATTR_DEFINE("version",ltr578_prox_version)
		DEV_ATTR_DEFINE("vendor",ltr578_prox_vendor_name)
#ifdef ONTIM_CALI
		DEV_ATTR_EXEC_DEFINE("set_ps_noice",&ltr578_prox_set_noice)
#endif
		DEV_ATTR_DECLARE_END;
		ONTIM_DEBUG_DECLARE_AND_INIT(als_prox,als_prox,8);
		//-add by hzb for ontim debug

		/*
		 * #########
		 * ## I2C ##
		 * #########
		 */

		/* I2C Read */
static int ltr578_i2c_read_reg(u8 regnum)
{
		u8 buffer[1], reg_value[1];
		int res = 0;

		mutex_lock(&read_lock);

		buffer[0] = regnum;
		res = i2c_master_send(ltr578_obj->client, buffer, 0x1);
		if (res <= 0)	{
				mutex_unlock(&read_lock);
				APS_ERR("read reg send res = %d\n", res);
				return res;
		}
		res = i2c_master_recv(ltr578_obj->client, reg_value, 0x1);
		if (res <= 0) {
				mutex_unlock(&read_lock);
				APS_ERR("read reg recv res = %d\n", res);
				return res;
		}
		mutex_unlock(&read_lock);
		return reg_value[0];
}

/* I2C Write */
static int ltr578_i2c_write_reg(u8 regnum, u8 value)
{
		u8 databuf[2];
		int res = 0;

		databuf[0] = regnum;
		databuf[1] = value;
		res = i2c_master_send(ltr578_obj->client, databuf, 0x2);

		if (res < 0) {
				APS_ERR("wirte reg send res = %d\n", res);
				return res;
		}

		else
				return 0;
}

#ifdef ONTIM_CALI
#if 1
static int ltr578_prox_set_noice(int noice)
{
		if ((noice > -1 ) &&  (noice < 1600))
		{
				ps_cali.noice = noice;
		}
		else
		{
				ps_cali.noice = -1;
		}
		printk(KERN_ERR "%s: noice = %d; Set noice to %d\n",__func__,noice,ps_cali.noice);
		return 0;
}
#endif
#endif

/*----------------------------------------------------------------------------*/
static ssize_t ltr578_show_als(struct device_driver *ddri, char *buf)
{
		int res;

		if (!ltr578_obj) {
				APS_ERR("ltr578_obj is null!!\n");
				return 0;
		}
		res = ltr578_als_read(als_gainrange);
		return snprintf(buf, PAGE_SIZE, "0x%04X\n", res);

}
/*----------------------------------------------------------------------------*/

static ssize_t ltr578_show_ps(struct device_driver *ddri, char *buf)
{
		int  res;

		if (!ltr578_obj) {
				APS_ERR("ltr578_obj is null!!\n");
				return 0;
		}
		res = ltr578_ps_read();

#ifdef PSENSOR_CALIBRATION
		if (res > MAX_PS_NOISE)
				return sprintf(buf,"device_name:%s @FAIL@ psensor_noise = %d\n", LTR578_DEV_NAME,res);
		else
				return sprintf(buf,"device_name:%s @PASS@ psensor_noise = %d\n", LTR578_DEV_NAME,res);
#else
		return snprintf(buf, PAGE_SIZE, "0x%04X\n", res);
#endif
}
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/

static ssize_t ltr578_show_ps_info(struct device_driver *ddri, char *buf)
{
		if (!ltr578_obj) {
				APS_ERR("ltr578_obj is null!!\n");
				return 0;
		}
		return snprintf(buf, PAGE_SIZE, "PS_THRED:%d -> %d,PS_CALI:%d\n", \
						atomic_read(&ltr578_obj->ps_thd_val_low), atomic_read(&ltr578_obj->ps_thd_val_high), dynamic_cali);
}
/*----------------------------------------------------------------------------*/



/*----------------------------------------------------------------------------*/
static ssize_t ltr578_show_status(struct device_driver *ddri, char *buf)
{
		ssize_t len = 0;

		if (!ltr578_obj) {
				APS_ERR("ltr578_obj is null!!\n");
				return 0;
		}

		if (ltr578_obj->hw) {
				len += snprintf(buf+len, PAGE_SIZE-len, "CUST: %d, (%d %d)\n",
								ltr578_obj->hw->i2c_num, ltr578_obj->hw->power_id, ltr578_obj->hw->power_vol);
		} else{
				len += snprintf(buf+len, PAGE_SIZE-len, "CUST: NULL\n");
		}


		len += snprintf(buf+len, PAGE_SIZE-len, "MISC: %d %d\n", atomic_read(&ltr578_obj->als_suspend), atomic_read(&ltr578_obj->ps_suspend));

		return len;
}

/*----------------------------------------------------------------------------*/
static ssize_t ltr578_store_status(struct device_driver *ddri, const char *buf, size_t count)
{
		int status1, ret;

		if (!ltr578_obj) {
				APS_ERR("ltr578_obj is null!!\n");
				return 0;
		}

#if 0 /* sang */
		if (1 == sscanf(buf, "%d ", &status1)) {
				ret = ltr578_ps_enable();
				APS_DBG("iret= %d\n", ret);
		} else{
				/* APS_DBG("invalid content: '%s', length = %d\n", buf, count); */
				APS_DBG("invalid content: '%s'\n", buf);
		}
#else
		if (1 == sscanf(buf, "%d ", &status1)) {
				if (status1 == 1) {
						ret = ltr578_ps_enable();
						if (ret < 0)
								APS_DBG("iret= %d\n", ret);
				} else if (status1 == 2) {
						ret = ltr578_als_enable(als_gainrange);
						if (ret < 0)
								APS_DBG("iret= %d\n", ret);
				} else if (status1 == 0) {
						ret = ltr578_ps_disable();
						if (ret < 0)
								APS_DBG("iret= %d\n", ret);

						ret = ltr578_als_disable();
						if (ret < 0)
								APS_DBG("iret= %d\n", ret);
				}
		} else{
				/* APS_DBG("invalid content: '%s', length = %d\n", buf, count); */
				APS_DBG("invalid content: '%s'\n", buf);
		}
#endif
		return count;
}


/*----------------------------------------------------------------------------*/
static ssize_t ltr578_show_reg(struct device_driver *ddri, char *buf)
{
		int i, len = 0;
		int reg[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0d, 0x0e, 0x0f,
				0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26
		};
		for (i = 0; i < 27; i++) {
				len += snprintf(buf+len, PAGE_SIZE-len, "reg:0x%04X value: 0x%04X\n", reg[i], ltr578_i2c_read_reg(reg[i]));

		}
		return len;
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr578_store_reg(struct device_driver *ddri, const char *buf, size_t count)
{
		int ret, value;
		unsigned int  reg;

		if (!ltr578_obj) {
				APS_ERR("ltr578_obj is null!!\n");
				return 0;
		}

		if (2 == sscanf(buf, "%x %x ", &reg, &value)) {
				APS_DBG("before write reg: %x, reg_value = %x  write value=%x\n", reg, ltr578_i2c_read_reg(reg), value);
				ret = ltr578_i2c_write_reg(reg, value);
				APS_DBG("after write reg: %x, reg_value = %x\n", reg, ltr578_i2c_read_reg(reg));
		} else{
				/* APS_DBG("invalid content: '%s', length = %d\n", buf, count); //sang */
				APS_DBG("invalid content: '%s'\n", buf);
		}
		return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t ltr578_show_config(struct device_driver *ddri, char *buf)
{
		ssize_t res;

		if (!ltr578_obj) {
				APS_ERR("ltr578_obj is null!!\n");
				return 0;
		}

		res = scnprintf(buf, PAGE_SIZE, "(%d %d %d %d %d %d)\n", atomic_read(&ltr578_obj->i2c_retry), atomic_read(&ltr578_obj->als_debounce), atomic_read(&ltr578_obj->ps_mask), atomic_read(&ltr578_obj->ps_thd_val_high), atomic_read(&ltr578_obj->ps_thd_val_low), atomic_read(&ltr578_obj->ps_debounce));

		return res;

}
/*----------------------------------------------------------------------------*/
static ssize_t ltr578_store_config(struct device_driver *ddri, const char *buf, size_t count)
{
		int retry, als_deb, ps_deb, mask, hthres, lthres;
		/* struct i2c_client *client; */
		/* client = ltr578_i2c_client; */
		if (!ltr578_obj) {
				APS_ERR("ltr578_obj is null!!\n");
				return 0;
		}

		if (6 == sscanf(buf, "%d %d %d %d %d %d", &retry, &als_deb, &mask, &hthres, &lthres, &ps_deb)) {
				atomic_set(&ltr578_obj->i2c_retry, retry);
				atomic_set(&ltr578_obj->als_debounce, als_deb);
				atomic_set(&ltr578_obj->ps_mask, mask);
				atomic_set(&ltr578_obj->ps_thd_val_high, hthres);
				atomic_set(&ltr578_obj->ps_thd_val_low, lthres);
				atomic_set(&ltr578_obj->ps_debounce, ps_deb);

		} else{
				APS_ERR("invalid content: '%s', length = %zu\n", buf, count);
		}
		return count;
}

static int ltr578_get_ps_value(void);
static ssize_t ltr578_show_distance(struct device_driver *ddri, char *buf)
{

		/* struct ltr578_priv *obj = i2c_get_clientdata(ltr578_i2c_client); */
		int dat = 0;

		if (!ltr578_obj) {
				APS_ERR("ltr578_obj is null!!\n");
				return 0;
		}


		dat = ltr578_get_ps_value();

		return scnprintf(buf, PAGE_SIZE, "%d\n", dat);

}

#if 1
/*----------------------------------------------------------------------------*/
static DRIVER_ATTR(als, S_IRUGO, ltr578_show_als,   NULL);
static DRIVER_ATTR(Huaqin_show_val, S_IRUGO, ltr578_show_ps,	  NULL);
static DRIVER_ATTR(ps_info, S_IRUGO, ltr578_show_ps_info,	NULL);
static DRIVER_ATTR(distance, S_IRUGO, ltr578_show_distance,	 NULL);
static DRIVER_ATTR(config,	S_IWUSR | S_IRUGO, ltr578_show_config, ltr578_store_config);
static DRIVER_ATTR(status,	S_IWUSR | S_IRUGO, ltr578_show_status,	ltr578_store_status);
static DRIVER_ATTR(reg,	S_IWUSR | S_IRUGO, ltr578_show_reg,   ltr578_store_reg);

/*----------------------------------------------------------------------------*/
static struct driver_attribute *ltr578_attr_list[] = {
		&driver_attr_als,
		&driver_attr_Huaqin_show_val,
		&driver_attr_ps_info,
		&driver_attr_distance,
		&driver_attr_config,
		&driver_attr_status,
		&driver_attr_reg,
};

/*----------------------------------------------------------------------------*/
static int ltr578_create_attr(struct device_driver *driver)
{
		int idx, err = 0;
		int num = (int)(sizeof(ltr578_attr_list)/sizeof(ltr578_attr_list[0]));

		if (driver == NULL) {
				return -EINVAL;
		}

		for (idx = 0; idx < num; idx++) {
				err = (driver_create_file(driver, ltr578_attr_list[idx]));
				if (err) {
						APS_ERR("driver_create_file (%s) = %d\n", ltr578_attr_list[idx]->attr.name, err);
						break;
				}
		}
		return err;
}
/*----------------------------------------------------------------------------*/
static int ltr578_delete_attr(struct device_driver *driver)
{
		int idx , err = 0;
		int num = (int)(sizeof(ltr578_attr_list)/sizeof(ltr578_attr_list[0]));

		if (!driver)
				return -EINVAL;

		for (idx = 0; idx < num; idx++) {
				driver_remove_file(driver, ltr578_attr_list[idx]);
		}

		return err;
}
#endif
/*----------------------------------------------------------------------------*/

/*
 * ###############
 * ## PS CONFIG ##
 * ###############

 */
#ifdef GN_MTK_BSP_PS_DYNAMIC_CALI
static int ltr578_dynamic_calibrate(void)
{
		/* int ret = 0; */
		int i = 0;
		int j = 0;
		int data = 0;
		int noise = 0;
		/* int len = 0; */
		/* int err = 0; */
		int max = 0;
		/* int idx_table = 0; */
		unsigned long data_total = 0;
		int ps_thd_val_low;
		int ps_thd_val_high;
		struct ltr578_priv *obj = ltr578_obj;

		APS_FUN(f);
		if (!obj)
				goto err;

		ltr578_i2c_write_reg(LTR578_PS_MEAS_RATE, 0x5A); /* 11bits & 12.5ms time */

		mdelay(10);
		for (i = 0; i < 5; i++) {
				if (max++ > 5) {
						atomic_set(&obj->ps_thd_val_high,  2047);
						atomic_set(&obj->ps_thd_val_low, 2047);

						goto err;
				}
				mdelay(15);

				data = ltr578_ps_read();
				if (data == 0) {
						j++;
				}
				data_total += data;
		}
		noise = data_total/(5 - j);

		if (noise < (dynamic_cali + 200)) {
				dynamic_cali = noise;
				isadjust = 1;

				if (noise < 100) {
						atomic_set(&obj->ps_thd_val_high,  noise+273);
						atomic_set(&obj->ps_thd_val_low, noise+105);
				} else if (noise < 200) {
						atomic_set(&obj->ps_thd_val_high,  noise+273);
						atomic_set(&obj->ps_thd_val_low, noise+105);
				} else if (noise < 300) {
						atomic_set(&obj->ps_thd_val_high,  noise+273);
						atomic_set(&obj->ps_thd_val_low, noise+105);
				} else if (noise < 400) {
						atomic_set(&obj->ps_thd_val_high,  noise+273);
						atomic_set(&obj->ps_thd_val_low, noise+105);
				} else if (noise < 600) {
						atomic_set(&obj->ps_thd_val_high,  noise+273);
						atomic_set(&obj->ps_thd_val_low, noise+105);
				} else if (noise < 800) {
						atomic_set(&obj->ps_thd_val_high,  noise+273);
						atomic_set(&obj->ps_thd_val_low, noise+105);
				} else if (noise < 1000) {
						atomic_set(&obj->ps_thd_val_high,  noise+273);
						atomic_set(&obj->ps_thd_val_low, noise+105);
				} else{
						atomic_set(&obj->ps_thd_val_high,  1200);
						atomic_set(&obj->ps_thd_val_low, 1150);
						isadjust = 0;
						APS_DBG("ltr578 the proximity sensor ps raw data\n");
				}

		}

		/*	*/

		ps_thd_val_low = atomic_read(&obj->ps_thd_val_low);
		ps_thd_val_high = atomic_read(&obj->ps_thd_val_high);
		APS_DBG(" ltr578_dynamic_calibrate end:noise=%d, obj->ps_thd_val_low= %d , obj->ps_thd_val_high = %d\n", noise, ps_thd_val_low, ps_thd_val_high);
		/* / */

		return 0;
err:
		APS_ERR("ltr578_dynamic_calibrate fail!!!\n");
		return -1;
}
#endif

static int ltr578_ps_set_thres(void)
{
		int res;
		u8 databuf[2];
		struct i2c_client *client = ltr578_obj->client;
		struct ltr578_priv *obj = ltr578_obj;

		APS_FUN();

		APS_DBG("ps_cali.valid: %d\n", ps_cali.valid);
		if (1 == ps_cali.valid) {
				databuf[0] = LTR578_PS_THRES_LOW_0;
				databuf[1] = (u8)(ps_cali.far_away & 0x00FF);
				res = i2c_master_send(client, databuf, 0x2);
				if (res <= 0) {
						goto EXIT_ERR;
						return LTR578_ERR_I2C;
				}
				databuf[0] = LTR578_PS_THRES_LOW_1;
				databuf[1] = (u8)((ps_cali.far_away & 0xFF00) >> 8);
				res = i2c_master_send(client, databuf, 0x2);
				if (res <= 0) {
						goto EXIT_ERR;
						return LTR578_ERR_I2C;
				}
				databuf[0] = LTR578_PS_THRES_UP_0;
				databuf[1] = (u8)(ps_cali.close & 0x00FF);
				res = i2c_master_send(client, databuf, 0x2);
				if (res <= 0) {
						goto EXIT_ERR;
						return LTR578_ERR_I2C;
				}
				databuf[0] = LTR578_PS_THRES_UP_1;
				databuf[1] = (u8)((ps_cali.close & 0xFF00) >> 8);
				res = i2c_master_send(client, databuf, 0x2);
				if (res <= 0) {
						goto EXIT_ERR;
						return LTR578_ERR_I2C;
				}
		} else{
				databuf[0] = LTR578_PS_THRES_LOW_0;
				databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_low)) & 0x00FF);
				res = i2c_master_send(client, databuf, 0x2);
				if (res <= 0) {
						goto EXIT_ERR;
						return LTR578_ERR_I2C;
				}
				databuf[0] = LTR578_PS_THRES_LOW_1;
				databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_low) >> 8) & 0x00FF);

				res = i2c_master_send(client, databuf, 0x2);
				if (res <= 0) {
						goto EXIT_ERR;
						return LTR578_ERR_I2C;
				}
				databuf[0] = LTR578_PS_THRES_UP_0;
				databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_high)) & 0x00FF);
				res = i2c_master_send(client, databuf, 0x2);
				if (res <= 0) {
						goto EXIT_ERR;
						return LTR578_ERR_I2C;
				}
				databuf[0] = LTR578_PS_THRES_UP_1;
				databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_high) >> 8) & 0x00FF);
				res = i2c_master_send(client, databuf, 0x2);
				if (res <= 0) {
						goto EXIT_ERR;
						return LTR578_ERR_I2C;
				}

				APS_ERR("%s ps_thd_val_high=%d;ps_thd_val_low=%d;\n", __func__,atomic_read(&obj->ps_thd_val_high),
								atomic_read(&obj->ps_thd_val_low));
		}

		res = 0;
		return res;

EXIT_ERR:
		APS_ERR("set thres: %d\n", res);
		return res;

}

#ifdef CHECK_FAIL_WORK

static void ltr578_init_reg(void)
{
		//	int error = 0;
		ltr578_i2c_write_reg(LTR578_PS_MEAS_RATE, 0x5C); /* 11bits & 50ms time */
		ltr578_i2c_write_reg(LTR578_INT_CFG, 0x01 );
		ltr578_i2c_write_reg(LTR578_INT_PST, 0x02 );
		ltr578_ps_set_thres();
		mdelay(WAKEUP_DELAY);
}

struct delayed_work dwork;// add delaywork for check 0x00 status
static u16 g_ltr578_error_check_delay=500;
static u32 g_ltr578_error_cnt=0;
static void ltr578_delay_work(struct work_struct *work)
{
		int status1 = 0;


		APS_FUN();
		status1 = ltr578_i2c_read_reg(LTR578_MAIN_CTRL);

		if((status1 & 0x01) == 0x00 )
		{
				g_ltr578_error_cnt++; // calc the fail times

				ltr578_init_reg();

				ltr578_i2c_write_reg(LTR578_MAIN_CTRL,((status1 | 0x03) & 0X03) );	// reset the main ctrl

				APS_LOG("ltr578_delay_work g_ltr578_error_cnt = %d \n", g_ltr578_error_cnt);


				mdelay(WAKEUP_DELAY);

				if(g_ltr578_error_cnt>1000)
						g_ltr578_error_cnt=0;
		}

		schedule_delayed_work(&dwork,msecs_to_jiffies(g_ltr578_error_check_delay));
}

static void ltr578_enable_delay_work(u8 enable)
{
		if (enable) 
		{
				schedule_delayed_work(&dwork,msecs_to_jiffies(g_ltr578_error_check_delay));
		} 
		else 
		{
				cancel_delayed_work_sync(&dwork);
		}
}

#endif



static int ltr578_ps_enable(void)
{

		struct i2c_client *client = ltr578_obj->client;
		struct ltr578_priv *obj = ltr578_obj;
		u8 databuf[2];
		int res;

		int error;
		int setctrl;

		APS_LOG("ltr578_ps_enable() ...start!\n");


		mdelay(WAKEUP_DELAY);



		/* ===============
		 * ** IMPORTANT **
		 * ===============
		 * Other settings like timing and threshold to be set here, if required.
		 * Not set and kept as device default for now.
		 */
		error = ltr578_i2c_write_reg(LTR578_PS_PULSES, 32); /* 32pulses -->12 */
		if (error < 0) {
				APS_LOG("ltr578_ps_enable() PS Pulses error2\n");
				return error;
		}
		error = ltr578_i2c_write_reg(LTR578_PS_LED, 0x36); /* 60khz & 100mA */
		if (error < 0) {
				APS_LOG("ltr578_ps_enable() PS LED error...\n");
				return error;
		}
		error = ltr578_i2c_write_reg(LTR578_PS_MEAS_RATE, 0x5C); /* 11bits & 50ms time */
		if (error < 0) {
				APS_LOG("ltr578_ps_enable() PS time error...\n");
				return error;
		}


		/*for interrup work mode support -- by WeeLiat, Liteon 18.06.2015*/
		if (0 == obj->hw->polling_mode_ps) {

				ltr578_ps_set_thres();

				databuf[0] = LTR578_INT_CFG;
				databuf[1] = 0x01;
				res = i2c_master_send(client, databuf, 0x2);
				if (res <= 0) {
						goto EXIT_ERR;
						return LTR578_ERR_I2C;
				}

				databuf[0] = LTR578_INT_PST;
				databuf[1] = 0x02;
				res = i2c_master_send(client, databuf, 0x2);
				if (res <= 0) {
						goto EXIT_ERR;
						return LTR578_ERR_I2C;
				}
				/* mt_eint_unmask(CUST_EINT_ALS_NUM); */

				enable_irq(obj->irq);

		}

		setctrl = ltr578_i2c_read_reg(LTR578_MAIN_CTRL);
		if ((setctrl & 0x01) == 0)/* Check for PS enable */{
				setctrl = (setctrl | 0x03) & 0x03;
				error = ltr578_i2c_write_reg(LTR578_MAIN_CTRL, setctrl);
				if (error < 0) {
						APS_LOG("ltr578_ps_enable() error1\n");
						return error;
				}
		}

		APS_LOG("ltr578_ps_enable ...OK!\n");

#ifdef GN_MTK_BSP_PS_DYNAMIC_CALI
		ltr578_dynamic_calibrate();
#endif

		//ltr578_ps_set_thres();

#ifdef CHECK_FAIL_WORK

		ltr578_enable_delay_work(1);

#endif


		return error;

EXIT_ERR:
		APS_ERR("set thres: %d\n", res);
		return res;
}

#ifdef PSENSOR_CALIBRATION
static int ltr578_psensor_calibrate(void)
{
		int i = 0;
		int j = 0;
		int data = 0;
		unsigned long sum = 0;
		struct ltr578_priv *obj = ltr578_obj;

		APS_FUN(f);
		if (!obj) goto err;

		ltr578_i2c_write_reg(LTR578_PS_MEAS_RATE, 0x5a); 

		for (i = 0; i < 3; i++) {
				msleep(15);

				data = ltr578_ps_read();

				if(data == 0){
						j++;
				}	
				sum += data;
		}

		ltr578_i2c_write_reg(LTR578_PS_MEAS_RATE, 0x5c);

		if(j<3)
				sum = sum/(3 - j);
		else
				sum = 0;

		//current_noise = sum;

		if(sum < MAX_PS_NOISE){
				if(obj->init_noise == 0)
						obj->init_noise = sum;

				obj->current_xtalk = sum;
		}else{
				obj->current_xtalk = obj->init_noise;
		}

		atomic_set(&obj->ps_thd_val_high,obj->hw->ps_threshold_high+obj->current_xtalk);
		atomic_set(&obj->ps_thd_val_low,obj->hw->ps_threshold_low+obj->current_xtalk);

		APS_ERR("zqq calibrate:noise=%ld,low=%d ,high=%d\n",\
						sum,atomic_read(&obj->ps_thd_val_low),atomic_read(&obj->ps_thd_val_high));

		ltr578_ps_set_thres();

		return 0;
err:
		APS_ERR("zqq ltr578_psensor_calibrate fail!!!\n");
		return -1;
}

static int ltr578_get_init_noise(void)
{
		int i = 0;
		int j = 0;
		int data = 0;
		int setctrl;
		int ret;
		unsigned long sum = 0;
		struct ltr578_priv *obj = ltr578_obj;

		APS_FUN(f);
		if (!obj) goto error;
		//disable irq
		disable_irq_nosync(obj->irq);
		//enable
		setctrl = ltr578_i2c_read_reg(LTR578_MAIN_CTRL);
		if ((setctrl & 0x01) == 0)/* Check for PS enable */{
				setctrl = setctrl | 0x01;
				ret = ltr578_i2c_write_reg(LTR578_MAIN_CTRL, setctrl);
				if (ret < 0) {
						APS_ERR("ltr578_ps_enable error1\n");
						goto error;
				}
		}

		ret = ltr578_i2c_write_reg(LTR578_PS_MEAS_RATE, 0x5a); 
		if(ret<0)
		{
				APS_LOG("ltr578 set ps measurement rate error\n");
		}

		for (i = 0; i < 5; i++) {
				msleep(15);

				data = ltr578_ps_read();

				if(data == 0){
						j++;
				}	
				sum += data;
		}

		if(j<5)
				sum = sum/(5 - j);
		else
				sum = 0;

		//current_noise = sum;

		if(sum < MAX_PS_NOISE){
				obj->init_noise = sum;	
		}
		else{
				obj->init_noise = AVG_PS_NOISE;
		}

		ret = ltr578_i2c_write_reg(LTR578_PS_MEAS_RATE, 0x5c); 
		if(ret<0)
		{
				APS_LOG("ltr578 set ps measurement rate error\n");
		} 

		//disable 
		setctrl = ltr578_i2c_read_reg(LTR578_MAIN_CTRL);
		if ((setctrl & 0x01) == 1) {
				setctrl = setctrl & 0x02;

				ret = ltr578_i2c_write_reg(LTR578_MAIN_CTRL, setctrl);  

		}	
		//enable irq
		enable_irq(obj->irq);
		APS_ERR("zqq init_noise %d\n",obj->init_noise);

		return 0;
error:
		APS_ERR("zqq ltr578_get_init_noise fail!!!\n");
		return -1;
}
#endif

/* Put PS into Standby mode */
static int ltr578_ps_disable(void)
{
		int error;
		struct ltr578_priv *obj = ltr578_obj;
		int setctrl;

		setctrl = ltr578_i2c_read_reg(LTR578_MAIN_CTRL);
		if ((setctrl & 0x01) == 1) {
				setctrl = setctrl & 0x02;
		}

		error = ltr578_i2c_write_reg(LTR578_MAIN_CTRL, setctrl);  /* sang */
		if (error < 0)
				APS_LOG("ltr578_ps_disable ...ERROR\n");
		else
				APS_LOG("ltr578_ps_disable ...OK\n");

		if (0 == obj->hw->polling_mode_ps) {
				//cancel_work_sync(&obj->eint_work);
				/* mt_eint_mask(CUST_EINT_ALS_NUM); */
				disable_irq(obj->irq); 
		}

#ifdef CHECK_FAIL_WORK

		ltr578_enable_delay_work(0);

#endif

		return error;
}


static int ltr578_ps_read(void)
{
		int psval_lo, psval_hi, psdata;

		psval_lo = ltr578_i2c_read_reg(LTR578_PS_DATA_0);
		APS_DBG("ps_rawdata_psval_lo = %d\n", psval_lo);
		if (psval_lo < 0) {

				APS_DBG("psval_lo error\n");
				psdata = psval_lo;
				goto out;
		}
		psval_hi = ltr578_i2c_read_reg(LTR578_PS_DATA_1);
		APS_DBG("ps_rawdata_psval_hi = %d\n", psval_hi);

		if (psval_hi < 0) {
				APS_DBG("psval_hi error\n");
				psdata = psval_hi;
				goto out;
		}

		psdata = ((psval_hi & 7) * 256) + psval_lo;
		/* psdata = ((psval_hi&0x7)<<8) + psval_lo; */
		APS_DBG("ps_rawdata = %d\n", psdata);

out:
		final_prox_val = psdata;

		return psdata;
}

/*
 * ################
 * ## ALS CONFIG ##
 * ################
 */

static int ltr578_als_enable(int gainrange)
{
		int error;
		int setctrl;

		APS_LOG("gainrange = %d\n", gainrange);
		switch (gainrange) {
				case ALS_RANGE_1:
						error = ltr578_i2c_write_reg(LTR578_ALS_GAIN, MODE_ALS_Range1);
						break;

				case ALS_RANGE_3:
						error = ltr578_i2c_write_reg(LTR578_ALS_GAIN, MODE_ALS_Range3);
						break;

				case ALS_RANGE_6:
						error = ltr578_i2c_write_reg(LTR578_ALS_GAIN, MODE_ALS_Range6);
						break;

				case ALS_RANGE_9:
						error = ltr578_i2c_write_reg(LTR578_ALS_GAIN, MODE_ALS_Range9);
						break;

				case ALS_RANGE_18:
						error = ltr578_i2c_write_reg(LTR578_ALS_GAIN, MODE_ALS_Range18);
						break;

				default:
						error = ltr578_i2c_write_reg(LTR578_ALS_GAIN, MODE_ALS_Range3);
						APS_ERR("ALS sensor gainrange %d!\n", gainrange);
						break;
		}

		error = ltr578_i2c_write_reg(LTR578_ALS_MEAS_RATE, ALS_RESO_MEAS);/* 18 bit & 100ms measurement rate */
		APS_ERR("ALS sensor resolution & measurement rate: %d!\n", ALS_RESO_MEAS);

		setctrl = ltr578_i2c_read_reg(LTR578_MAIN_CTRL);

		if((setctrl & 0x02) == 0x00 ){
				setctrl = (setctrl | 0x02) & 0x03;/* Enable ALS */
				error = ltr578_i2c_write_reg(LTR578_MAIN_CTRL, setctrl);
		}
		mdelay(WAKEUP_DELAY);

		//error = ltr578_i2c_read_reg(LTR578_MAIN_CTRL);
		if (error < 0) {
				APS_DBG("ltr578_ALS_enablE() error1\n");
		}
		/* ===============
		 * ** IMPORTANT **
		 * ===============
		 * Other settings like timing and threshold to be set here, if required.
		 * Not set and kept as device default for now.
		 */
		if (error < 0){
				APS_LOG("ltr578_als_enable ...ERROR\n");
				return error;
		}
		else
		{
				APS_LOG("ltr578_als_enable ...OK\n");
				return 0;
		}
}


/* Put ALS into Standby mode */
static int ltr578_als_disable(void)
{
		int error;
		int setctrl;

		error = 0;

		setctrl = ltr578_i2c_read_reg(LTR578_MAIN_CTRL);
		if (setctrl < 0)
		{
				APS_LOG("ltr578_als_disable ...ERROR setctrl=%d;\n",setctrl);
				return setctrl;
		}		
		if (((setctrl & 0x03) == 0x02)) {	/* do not repeat disable ALS */
				setctrl = setctrl & 0x01;
				error = ltr578_i2c_write_reg(LTR578_MAIN_CTRL, setctrl);
		}
		if (error < 0)
				APS_LOG("ltr578_als_disable ...ERROR\n");
		else
				APS_LOG("ltr578_als_disable ...OK\n");
		return error;
}

static int ltr578_als_read(int gainrange)
{
		int alsval_0, alsval_1, alsval_2, alsval;

		int cleval_0, cleval_1, cleval_2, cleval;
		int luxdata_int;
		int als_time;
		int als_gain;
		int lsec=0; 

		alsval_0 = ltr578_i2c_read_reg(LTR578_ALS_DATA_0);
		alsval_1 = ltr578_i2c_read_reg(LTR578_ALS_DATA_1);
		alsval_2 = ltr578_i2c_read_reg(LTR578_ALS_DATA_2);
		alsval = (alsval_2 * 256* 256) + (alsval_1 * 256) + alsval_0;
		//	APS_DBG("alsval_0 = %d,alsval_1=%d,alsval_2=%d,alsval=%d\n",alsval_0,alsval_1,alsval_2,alsval);

		if(alsval==0)
		{
				luxdata_int = 0;
				goto exit;
		}


		cleval_0 = ltr578_i2c_read_reg(0x0a);
		cleval_1 = ltr578_i2c_read_reg(0x0b);
		cleval_2 = ltr578_i2c_read_reg(0x0c);

		cleval = cleval_0 | (cleval_1<<8) | (cleval_2<<16);
		lsec = (cleval/alsval)*10;


		//APS_DBG("gainrange = %d\n",gainrange);

		als_time = ltr578_i2c_read_reg(0x04);
		als_time = (als_time>>4)&0x07;


		//APS_DBG("als_time=%d\n", als_time);

		if (als_time == 0)
				als_time = 400;
		else if (als_time == 1)
				als_time = 200;
		else if (als_time == 2)
				als_time = 100;
		else if (als_time == 3)
				als_time = 50;
		else
				als_time = 25;



		als_gain = ltr578_i2c_read_reg(0x05);
		als_gain = als_gain&0x07;
		//APS_DBG("als_gain=%d\n", als_gain);

		if (als_gain == 0)
				als_gain = 1;
		else if (als_gain == 1)
				als_gain = 3;
		else if (als_gain == 2)
				als_gain = 6;
		else if (als_gain == 3)
				als_gain = 9;
		else
				als_gain = 18;

		if(lsec > 100)
				luxdata_int = (alsval*1872)/(als_time*als_gain); //incan
		else if(lsec > 40)
				luxdata_int = (alsval*2274)/(als_time*als_gain);	// D65
		else
				luxdata_int = (alsval*1624)/(als_time*als_gain); //cwf

exit:	
		//+add by fxz
//		if(luxdata_int == 0) luxdata_int = 1;
		//-add by fxz
		APS_DBG("als_value_lux = %d;%d;%d;\n", luxdata_int,lsec,alsval);
		return luxdata_int;


}



/*----------------------------------------------------------------------------*/
int ltr578_get_addr(struct alsps_hw *hw, struct ltr578_i2c_addr *addr)
{
		return 0;
}


/*-----------------------------------------------------------------------------*/
void ltr578_eint_func(void)
{
		struct ltr578_priv *obj = ltr578_obj;
		if (!obj) {
				return;
		}

		schedule_work(&obj->eint_work);
		/* schedule_delayed_work(&obj->eint_work); */
		/* disable_irq_nosync(obj->irq); */
}

#if defined(CONFIG_OF)
static irqreturn_t ltr578_eint_handler(int irq, void *desc)
{
		disable_irq_nosync(ltr578_obj->irq);
		ltr578_eint_func();

		return IRQ_HANDLED;
}
#endif

/*----------------------------------------------------------------------------*/
/*for interrup work mode support -- by liaoxl.lenovo 12.08.2011*/

int ltr578_setup_eint(struct i2c_client *client)
{
		int ret;
		struct pinctrl *pinctrl;
		struct pinctrl_state *pins_default;
		struct pinctrl_state *pins_cfg;
		u32 ints[2] = {0, 0};

		/* gpio setting */
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
		if (ltr578_obj->irq_node) {
#ifndef CONFIG_MTK_EIC
				/*upstream code*/
				ints[0] = of_get_named_gpio(ltr578_obj->irq_node, "deb-gpios", 0);
				if (ints[0] < 0) {
						pr_err("debounce gpio not found\n");
				} else{
						ret = of_property_read_u32(ltr578_obj->irq_node, "debounce", &ints[1]);
						if (ret < 0)
								pr_err("debounce time not found\n");
						else
								gpio_set_debounce(ints[0], ints[1]);
						APS_LOG("ints[0] = %d, ints[1] = %d!!\n", ints[0], ints[1]);
				}
#else
				ret = of_property_read_u32_array(ltr578_obj->irq_node, "debounce", ints, ARRAY_SIZE(ints));
				if (ret) {
						APS_ERR("of_property_read_u32_array fail, ret = %d\n", ret);
						return ret;
				}
				gpio_set_debounce(ints[0], ints[1]);
				APS_LOG("ints[0] = %d, ints[1] = %d!!\n", ints[0], ints[1]);
#endif

				ltr578_obj->irq = irq_of_parse_and_map(ltr578_obj->irq_node, 0);
				APS_LOG("ltr578_obj->irq = %d\n", ltr578_obj->irq);
				if (!ltr578_obj->irq) {
						APS_ERR("irq_of_parse_and_map fail!!\n");
						return -EINVAL;
				}

				if (request_irq(ltr578_obj->irq, ltr578_eint_handler, IRQF_TRIGGER_LOW, "ALS-eint", NULL)) {
						APS_ERR("IRQ LINE NOT AVAILABLE!!\n");
						return -EINVAL;
				}

				disable_irq(ltr578_obj->irq);
		} else {
				APS_ERR("null irq node!!\n");
				return -EINVAL;
		}

		return 0;
}



/*----------------------------------------------------------------------------*/
/*for interrup work mode support -- by liaoxl.lenovo 12.08.2011*/

static int ltr578_check_intr(struct i2c_client *client)
{
		int res, intp, intl;
		u8 buffer[2];

		APS_FUN();



		/* if (mt_get_gpio_in(GPIO_ALS_EINT_PIN) == 1)*/
		/* return 0; */

		buffer[0] = ltr578_i2c_read_reg(LTR578_MAIN_STATUS);

		res = 1;
		intp = 0;
		intl = 0;
		if (0 != (buffer[0] & 0x02)) {
				res = 0;
				intp = 1;
		}
		if (0 != (buffer[0] & 0x10)) {
				res = 0;
				intl = 1;
		}

		return res;

}

static int ltr578_clear_intr(struct i2c_client *client)
{
		int res = 0;
		u8 buffer[2];

		APS_FUN();

		buffer[0] = ltr578_i2c_read_reg(LTR578_MAIN_STATUS);

		APS_DBG("ltr578_clear_intr buffer[0] = %d\n", buffer[0]);

#if 0
		buffer[1] = buffer[0] & 0x01;
		buffer[0] = LTR578_MAIN_STATUS;

		res = i2c_master_send(client, buffer, 0x2);
		if (res <= 0) {
				goto EXIT_ERR;
		} else{
				res = 0;
		}
#endif

		return res;

}




static int ltr578_devinit(void)
{
		int res=0;

		struct i2c_client *client = ltr578_obj->client;


		res = ltr578_als_disable();
		if (res < 0)
				goto EXIT_ERR;


		res = ltr578_setup_eint(client);
		if (res) {
				APS_ERR("setup eint: %d\n", res);
		}

EXIT_ERR:
		return res;

}
/*----------------------------------------------------------------------------*/


static int ltr578_get_als_value(struct ltr578_priv *obj, u16 als)
{
		int idx;
		int invalid = 0;
		static u16 last_als;

		if (last_als != als)
				APS_DBG("als  = %d\n", als);
		return als;

		for (idx = 0; idx < obj->als_level_num; idx++) {
				if (als < obj->hw->als_level[current_tp][current_color_temp][idx]) {
						break;
				}
		}

		if (idx >= obj->als_level_num) {
				APS_ERR("exceed range\n");
				idx = obj->als_level_num - 1;
		}

		if (1 == atomic_read(&obj->als_deb_on)) {
				unsigned long endt = atomic_read(&obj->als_deb_end);

				if (time_after(jiffies, endt)) {
						atomic_set(&obj->als_deb_on, 0);
				}

				if (1 == atomic_read(&obj->als_deb_on)) {
						invalid = 1;
				}
		}
		if (last_als != als)
				APS_DBG("idx  = %d\n", idx);

		if (!invalid) {
				if (last_als != als)
						APS_DBG("ALS: %05d => %05d\n", als, obj->hw->als_value[idx]);
				else
						APS_DBG("ALS:%05d, 0x00:%05d\n", als, ltr578_i2c_read_reg(LTR578_MAIN_CTRL));

				last_als = als;
				return obj->hw->als_value[idx];
		} else{
				APS_ERR("ALS: %05d => %05d (-1)\n", als, obj->hw->als_value[idx]);
				return -1;
		}
}
/*----------------------------------------------------------------------------*/

static int ltr578_get_ps_value(void)
{
		int ps_flag;
		int val;
		int error =  -1;
		int buffer = 0;

		buffer = ltr578_i2c_read_reg(LTR578_MAIN_STATUS);
		APS_DBG("Main status = %d\n", buffer);
		if (buffer < 0) {

				APS_DBG("MAIN status read error\n");
				return error;
		}

		ps_flag = buffer & 0x04;
		ps_flag = ps_flag >> 2;
		if (ps_flag == 1) {
				intr_flag_value = 1;
				val = 0;
		} else if (ps_flag == 0) {
				intr_flag_value = 0;
				val = 1;
		}
		APS_DBG("ps_flag = %d, val = %d\n", ps_flag, val);

		return val;
}

/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
/*for interrup work mode support */
static void ltr578_eint_work(struct work_struct *work)
{
		struct ltr578_priv *obj = (struct ltr578_priv *)container_of(work, struct ltr578_priv, eint_work);
		int err;
		struct hwm_sensor_data sensor_data;
		int ps_value;
		int noise;

		u8 databuf[2];
		int res = 0;

		APS_FUN();

		err = ltr578_check_intr(obj->client);
		if (err < 0) {
				APS_ERR("ltr578_eint_work check intrs: %d\n", err);
		} else{
				/* get ps flag */
				ps_value = ltr578_get_ps_value();
				if (ps_value < 0) {
						err = -1;
				}

				/* clear ps interrupt */
				obj->ps = ltr578_ps_read();
				if (obj->ps < 0) {
						err = -1;
						enable_irq(obj->irq);
						return;
				}
#ifdef PSENSOR_CALIBRATION
				APS_ERR("zqq int:init=%d,noise=%d,low=%d,high=%d\n",obj->init_noise,obj->current_xtalk,\
								atomic_read(&obj->ps_thd_val_low),atomic_read(&obj->ps_thd_val_high));
#endif

				noise = obj->ps;

				APS_DBG("intr_flag_value=%d\n",intr_flag_value);
				if(intr_flag_value){

#if 1//def UPDATE_PS_THRESHOLD
						databuf[0] = LTR578_PS_THRES_LOW_0;	
						databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_low)) & 0x00FF);
						res = i2c_master_send(obj->client, databuf, 0x2);
						if(res <= 0)
						{
								goto EXIT_INTR;
						}
						databuf[0] = LTR578_PS_THRES_LOW_1;	
						databuf[1] = (u8)(((atomic_read(&obj->ps_thd_val_low)) & 0xFF00) >> 8);
						res = i2c_master_send(obj->client, databuf, 0x2);
						if(res <= 0)
						{
								goto EXIT_INTR;
						}
						databuf[0] = LTR578_PS_THRES_UP_0;	
						databuf[1] = (u8)(0x00FF);
						res = i2c_master_send(obj->client, databuf, 0x2);
						if(res <= 0)
						{
								goto EXIT_INTR;
						}
						databuf[0] = LTR578_PS_THRES_UP_1; 
						databuf[1] = (u8)((0xFF00) >> 8);;
						res = i2c_master_send(obj->client, databuf, 0x2);
						if(res <= 0)
						{
								goto EXIT_INTR;
						}
#endif


				}
				else{
#if 1//def UPDATE_PS_THRESHOLD
						databuf[0] = LTR578_PS_THRES_LOW_0;	
						databuf[1] = (u8)(0 & 0x00FF);
						//databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_low)) & 0x00FF);
						res = i2c_master_send(obj->client, databuf, 0x2);
						if(res <= 0)
						{
								goto EXIT_INTR;
						}
						databuf[0] = LTR578_PS_THRES_LOW_1;
						databuf[1] = (u8)((0 & 0xFF00) >> 8);
						//databuf[1] = (u8)(((atomic_read(&obj->ps_thd_val_low)) & 0xFF00) >> 8);
						res = i2c_master_send(obj->client, databuf, 0x2);
						if(res <= 0)
						{
								goto EXIT_INTR;
						}
						databuf[0] = LTR578_PS_THRES_UP_0;	
						databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_high)) & 0x00FF);
						res = i2c_master_send(obj->client, databuf, 0x2);
						if(res <= 0)
						{
								goto EXIT_INTR;
						}
						databuf[0] = LTR578_PS_THRES_UP_1; 
						databuf[1] = (u8)(((atomic_read(&obj->ps_thd_val_high)) & 0xFF00) >> 8);;
						res = i2c_master_send(obj->client, databuf, 0x2);
						if(res <= 0)
						{
								goto EXIT_INTR;
						}
#endif		
#ifdef GN_MTK_BSP_PS_DYNAMIC_CALI
						//if (ps_value == 1) {
						if (noise < (dynamic_cali - 100)) {
								dynamic_cali = noise;
								isadjust = 1;
								if (noise < 100) {
										atomic_set(&obj->ps_thd_val_high,  noise+65);
										atomic_set(&obj->ps_thd_val_low, noise+20);
								} else if (noise < 200) {
										atomic_set(&obj->ps_thd_val_high,  noise+80);
										atomic_set(&obj->ps_thd_val_low, noise+40);
								} else if (noise < 300) {
										atomic_set(&obj->ps_thd_val_high,  noise+110);
										atomic_set(&obj->ps_thd_val_low, noise+60);
								} else if (noise < 400) {
										atomic_set(&obj->ps_thd_val_high,  noise+150);
										atomic_set(&obj->ps_thd_val_low, noise+90);
								} else if (noise < 600) {
										atomic_set(&obj->ps_thd_val_high,  noise+170);
										atomic_set(&obj->ps_thd_val_low, noise+110);
								} else if (noise < 800) {
										atomic_set(&obj->ps_thd_val_high,  noise+180);
										atomic_set(&obj->ps_thd_val_low, noise+130);
								} else if (noise < 1000) {
										atomic_set(&obj->ps_thd_val_high,  noise+220);
										atomic_set(&obj->ps_thd_val_low, noise+100);
								} else{
										atomic_set(&obj->ps_thd_val_high,  1200);
										atomic_set(&obj->ps_thd_val_low, 1150);
										isadjust = 0;
										printk("ltr578 the proximity ps_raw_data\n");
								}

								ltr578_ps_set_thres();

						}


#endif
				}

EXIT_INTR:
				APS_DBG("ltr578_eint_work rawdata ps=%d als=%d!\n", obj->ps, obj->als);
				sensor_data.values[0] = ps_value;
				sensor_data.value_divide = 1;
				sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
				/*singal interrupt function add*/
				APS_DBG("intr_flag_value=%d\n", intr_flag_value);
				/*-------------------modified by hongguang@wecorp---------------------------------
				//let up layer to know
				if((err = hwmsen_get_interrupt_data(ID_PROXIMITY, &sensor_data)))
				{
				APS_ERR("call hwmsen_get_interrupt_data fail = %d\n", err);
				}
				------------------------------------------------------*/
				/*-------------------modified by hongguang@wecorp-------------------------------*/
				if (ps_report_interrupt_data(sensor_data.values[0])) {
						APS_ERR("call ps_report_interrupt_data fail\n");
				}
				}
				ltr578_clear_intr(obj->client);
				/* mt_eint_unmask(CUST_EINT_ALS_NUM); */
				enable_irq(obj->irq);
		}



		/******************************************************************************
		 * Function Configuration
		 ******************************************************************************/
		static int ltr578_open(struct inode *inode, struct file *file)
		{
				file->private_data = ltr578_i2c_client;

				if (!file->private_data) {
						APS_ERR("null pointer!!\n");
						return -EINVAL;
				}

				return nonseekable_open(inode, file);
		}
		/*----------------------------------------------------------------------------*/
		static int ltr578_release(struct inode *inode, struct file *file)
		{
				file->private_data = NULL;
				return 0;
		}
		/*----------------------------------------------------------------------------*/

		static int offset_value;
#ifdef CONFIG_COMPAT
		static long ltr578_compat_ioctl(struct file *file, unsigned int cmd,
						unsigned long arg)
		{
				long err = 0;
				void __user *arg32 = compat_ptr(arg);
				APS_ERR("ltr578_compat_ioctl cmd= %x\n", cmd); 
				if (!file->f_op || !file->f_op->unlocked_ioctl || !arg32)
						return -ENOTTY;

				APS_ERR("ltr578_compat_ioctl cmd= %x\n", cmd); 
				err = file->f_op->unlocked_ioctl(file, cmd, (unsigned long)arg32);

				return err;
		}
#endif
		static long ltr578_unlocked_ioctl(struct file *file, unsigned int cmd,
						unsigned long arg)
		{
				struct i2c_client *client = (struct i2c_client *)file->private_data;
				struct ltr578_priv *obj = i2c_get_clientdata(client);
				int err = 0;
				void __user *ptr = (void __user *) arg;
				int dat;
				uint32_t enable;
				uint32_t cali_value;
				int ps_result;
				int threshold[2];

				APS_DBG("cmd= %x\n", cmd);
				switch (cmd) {
//add by fanxzh for calibration begin
						case ALSPS_IOCTL_CLR_CALI:
							if(copy_from_user(&cali_value, ptr, sizeof(cali_value)))
							{
								err = -EFAULT;
								goto err_out;
							}
							if(cali_value == 0)
								obj->ps_cali = 0;
						break;
						case ALSPS_IOCTL_GET_CALI:
							cali_value = obj->ps_cali ;
							if(copy_to_user(ptr, &cali_value, sizeof(cali_value)))
							{
								err = -EFAULT;
								goto err_out;
							}
						break;
//add by fanxzh for calibration end
						case ALSPS_IOCTL_SET_CALI:
								if (copy_from_user(&cali_value, ptr, sizeof(cali_value))) {
										err = -EFAULT;
										goto err_out;
								}
								offset_value = cali_value;
								break;

						case ALSPS_SET_PS_MODE:
								if (copy_from_user(&enable, ptr, sizeof(enable))) {
										err = -EFAULT;
										goto err_out;
								}
								if (enable) {
										err = ltr578_ps_enable();
										if (err < 0) {
												APS_ERR("enable ps fail: %d\n", err);
												goto err_out;
										}
										set_bit(CMC_BIT_PS, &obj->enable);
								} else{
										err = ltr578_ps_disable();
										if (err < 0) {
												APS_ERR("disable ps fail: %d\n", err);
												goto err_out;
										}

										clear_bit(CMC_BIT_PS, &obj->enable);
								}
								break;

						case ALSPS_GET_PS_MODE:
								enable = test_bit(CMC_BIT_PS, &obj->enable) ? (1) : (0);
								if (copy_to_user(ptr, &enable, sizeof(enable))) {
										err = -EFAULT;
										goto err_out;
								}
								break;

						case ALSPS_GET_PS_DATA:
								APS_DBG("ALSPS_GET_PS_DATA\n");
								obj->ps = ltr578_ps_read();
								if (obj->ps < 0) {
										goto err_out;
								}

								dat = ltr578_get_ps_value();
								if (copy_to_user(ptr, &dat, sizeof(dat))) {
										err = -EFAULT;
										goto err_out;
								}
								break;

						case ALSPS_GET_PS_RAW_DATA:
								obj->ps = ltr578_ps_read();
								if (obj->ps < 0) {
										goto err_out;
								}
								dat = obj->ps;
								if (copy_to_user(ptr, &dat, sizeof(dat))) {
										err = -EFAULT;
										goto err_out;
								}
								break;

						case ALSPS_SET_ALS_MODE:
								if (copy_from_user(&enable, ptr, sizeof(enable))) {
										err = -EFAULT;
										goto err_out;
								}
								if (enable) {
										err = ltr578_als_enable(als_gainrange);
										if (err < 0) {
												APS_ERR("enable als fail: %d\n", err);
												goto err_out;
										}
										set_bit(CMC_BIT_ALS, &obj->enable);
								} else{
										err = ltr578_als_disable();
										if (err < 0) {
												APS_ERR("disable als fail: %d\n", err);
												goto err_out;
										}
										clear_bit(CMC_BIT_ALS, &obj->enable);
								}
								break;

						case ALSPS_GET_ALS_MODE:
								enable = test_bit(CMC_BIT_ALS, &obj->enable) ? (1) : (0);
								if (copy_to_user(ptr, &enable, sizeof(enable))) {
										err = -EFAULT;
										goto err_out;
								}
								break;

						case ALSPS_GET_ALS_DATA:
								obj->als = ltr578_als_read(als_gainrange);
								if (obj->als < 0) {
										goto err_out;
								}

								dat = ltr578_get_als_value(obj, obj->als);
								if (copy_to_user(ptr, &dat, sizeof(dat))) {
										err = -EFAULT;
										goto err_out;
								}
								break;

						case ALSPS_GET_ALS_RAW_DATA:
								obj->als = ltr578_als_read(als_gainrange);
								if (obj->als < 0) {
										goto err_out;
								}

								dat = obj->als;
								if (copy_to_user(ptr, &dat, sizeof(dat))) {
										err = -EFAULT;
										goto err_out;
								}
								break;
								/* factory test mode*/
						case ALSPS_GET_PS_TEST_RESULT:
								obj->ps = ltr578_ps_read();
								if(obj->ps < 0)
								{
										goto err_out;
								}
								if(obj->ps > atomic_read(&obj->ps_thd_val_low))
								{
										ps_result = 0;
								}
								else	ps_result = 1;

								if(copy_to_user(ptr, &ps_result, sizeof(ps_result)))
								{
										err = -EFAULT;
										goto err_out;
								}			   
								APS_ERR("%s CALI get_ps_test_result;%d;\n", __func__,ps_result); 

								break;
								/*
								   case ALSPS_IOCTL_CLR_CALI:

								   break;

								   case ALSPS_IOCTL_GET_CALI:

								   break;

								   case ALSPS_IOCTL_SET_CALI:

								   break;
								 */
						case ALSPS_SET_PS_THRESHOLD:
								if(copy_from_user(threshold, ptr, sizeof(threshold)))
								{
										err = -EFAULT;
										goto err_out;
								}
								APS_ERR("%s CALI set threshold high: %d, low: %d;%d;\n", __func__, threshold[0],threshold[1],obj->ps_cali); 
								atomic_set(&obj->ps_thd_val_high,  (threshold[0]+obj->ps_cali));
								atomic_set(&obj->ps_thd_val_low,  (threshold[1]+obj->ps_cali));//need to confirm

								//ps_cali.valid =1;
#ifdef ONTIM_CALI
								ps_cali.close = (threshold[0]+obj->ps_cali);
								ps_cali.far_away = (threshold[1]+obj->ps_cali);
#endif	
								ltr578_ps_set_thres();

								break;

						case ALSPS_GET_PS_THRESHOLD_HIGH:
#ifdef ONTIM_CALI
								if (ps_cali.noice > -1)
								{
										threshold[0] = ps_cali.close - obj->ps_cali;
								}
								else
								{
#endif
										threshold[0] = atomic_read(&obj->ps_thd_val_high) - obj->ps_cali;
#ifdef ONTIM_CALI
								}
#endif
								APS_ERR("%s CALI get threshold high: %d;%d;\n", __func__, threshold[0],obj->ps_cali); 
								if(copy_to_user(ptr, &threshold[0], sizeof(threshold[0])))
								{
										err = -EFAULT;
										goto err_out;
								}

								break;

						case ALSPS_GET_PS_THRESHOLD_LOW:
#ifdef ONTIM_CALI
								if (ps_cali.noice > -1)
								{
										threshold[0] = ps_cali.far_away;
								}
								else
								{
#endif
										threshold[0] = atomic_read(&obj->ps_thd_val_low) - obj->ps_cali;
#ifdef ONTIM_CALI
								}
#endif
								APS_ERR("%s CALI get threshold low: %d;%d;\n", __func__, threshold[0],obj->ps_cali); 
								if(copy_to_user(ptr, &threshold[0], sizeof(threshold[0])))
								{
										err = -EFAULT;
										goto err_out;
								}

								break;

						default:
								APS_ERR("%s not supported = 0x%04x", __func__, cmd);
								err = -ENOIOCTLCMD;
								break;
				}

err_out:
				return err;
		}

		/*----------------------------------------------------------------------------*/
		static struct file_operations ltr578_fops = {
				/* .owner = THIS_MODULE, */
				.open = ltr578_open,
				.release = ltr578_release,
				.unlocked_ioctl = ltr578_unlocked_ioctl,
#ifdef CONFIG_COMPAT
				.compat_ioctl = ltr578_compat_ioctl,
#endif

		};
		/*----------------------------------------------------------------------------*/
		static struct miscdevice ltr578_device = {
				.minor = MISC_DYNAMIC_MINOR,
				.name = "als_ps",
				.fops = &ltr578_fops,
		};

		static int ltr578_i2c_suspend(struct device *dev)
		{

				struct i2c_client *client = to_i2c_client(dev);
				struct ltr578_priv *obj = i2c_get_clientdata(client);
				int err;

				APS_FUN();

				if (!obj) {
						APS_ERR("null pointer!!\n");
						return -EINVAL;
				}

				atomic_set(&obj->als_suspend, 1);
				err = ltr578_als_disable();
				if (err < 0) {
						APS_ERR("disable als fail: %d\n", err);
				}

//+ add by fanxzh
				err = enable_irq_wake(obj->irq);
				if (err) {
					APS_ERR("enable_irq_wake(irq:%d) failed", obj->irq);
				}
//- add by fanxzh

				return 0;
		}
		/*----------------------------------------------------------------------------*/
		static int ltr578_i2c_resume(struct device *dev)
		{
				struct i2c_client *client = to_i2c_client(dev);
				struct ltr578_priv *obj = i2c_get_clientdata(client);
				int err;

				APS_FUN();

				if (!obj) {
						APS_ERR("null pointer!!\n");
						return -EINVAL;
				}

				atomic_set(&obj->als_suspend, 0);
				if (test_bit(CMC_BIT_ALS, &obj->enable)) {
						err = ltr578_als_enable(als_gainrange);
						if (err < 0) {
								APS_ERR("enable als fail: %d\n", err);

						}
				}
//+ add by fanxzh
				err = disable_irq_wake(obj->irq);
				if (err) {
					APS_ERR("disable_irq_wake(irq:%d) failed", obj->irq);
				}
//- add by fanxzh

				return 0;
		}
#ifdef CONFIG_HAS_EARLYSUSPEND
		static void ltr578_early_suspend(struct early_suspend *h)
		{	/*early_suspend is only applied for ALS*/
				struct ltr578_priv *obj = container_of(h, struct ltr578_priv, early_drv);
				int err;

				APS_FUN();

				if (!obj) {
						APS_ERR("null pointer!!\n");
						return;
				}

				atomic_set(&obj->als_suspend, 1);
				err = ltr578_als_disable();
				if (err < 0) {
						APS_ERR("disable als fail: %d\n", err);
				}
		}

		static void ltr578_late_resume(struct early_suspend *h)
		{	/*early_suspend is only applied for ALS*/
				struct ltr578_priv *obj = container_of(h, struct ltr578_priv, early_drv);
				int err;

				APS_FUN();

				if (!obj) {
						APS_ERR("null pointer!!\n");
						return;
				}
				atomic_set(&obj->als_suspend, 0);
				if (test_bit(CMC_BIT_ALS, &obj->enable)) {
						err = ltr578_als_enable(als_gainrange);
						if (err < 0) {
								APS_ERR("enable als fail: %d\n", err);

						}
				}
		}
#endif

		int ltr578_ps_operate(void *self,
						uint32_t command, void *buff_in, int size_in,
						void *buff_out, int size_out, int *actualout)
		{
				int err = 0;
				int value;
				struct hwm_sensor_data *sensor_data;
				struct ltr578_priv *obj = (struct ltr578_priv *)self;

				switch (command) {
						case SENSOR_DELAY:
								if ((buff_in == NULL) || (size_in < sizeof(int))) {
										APS_ERR("Set delay parameter error!\n");
										err = -EINVAL;
								}
								/* Do nothing */
								break;

						case SENSOR_ENABLE:
								if ((buff_in == NULL) || (size_in < sizeof(int))) {
										APS_ERR("Enable sensor parameter error!\n");
										err = -EINVAL;
								} else{
										value = *(int *)buff_in;
										if (value) {
												err = ltr578_ps_enable();
												if (err < 0) {
														APS_ERR("enable ps fail: %d\n", err);
														return -1;
												}
												set_bit(CMC_BIT_PS, &obj->enable);
										} else{
												err = ltr578_ps_disable();
												if (err < 0) {
														APS_ERR("disable ps fail: %d\n", err);
														return -1;
												}
												clear_bit(CMC_BIT_PS, &obj->enable);
										}
								}
								break;

						case SENSOR_GET_DATA:
								if ((buff_out == NULL) ||
												(size_out < sizeof(struct hwm_sensor_data))) {
										APS_ERR("get sensor data parameter error!\n");
										err = -EINVAL;
								} else{
										APS_ERR("get sensor ps data !\n");
										sensor_data = (struct hwm_sensor_data *)buff_out;
										value = ltr578_get_ps_value();
										obj->ps = ltr578_ps_read();
										if (obj->ps < 0) {
												err = -1;
												break;
										}
										sensor_data->values[0] = value;
										sensor_data->values[1] = ltr578_ps_read();
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

		int ltr578_als_operate(void *self,
						uint32_t command, void *buff_in, int size_in,
						void *buff_out, int size_out, int *actualout)
		{
				int err = 0;
				int value;
				struct hwm_sensor_data *sensor_data;
				struct ltr578_priv *obj = (struct ltr578_priv *)self;

				printk("ltr578_als_operate: cmd=%d\n", command);
				switch (command) {
						case SENSOR_DELAY:
								if ((buff_in == NULL) || (size_in < sizeof(int))) {
										APS_ERR("Set delay parameter error!\n");
										err = -EINVAL;
								}
								/* Do nothing */
								break;

						case SENSOR_ENABLE:
								if ((buff_in == NULL) || (size_in < sizeof(int))) {
										APS_ERR("Enable sensor parameter error!\n");
										err = -EINVAL;
								} else{
										value = *(int *)buff_in;
										printk("ltr578_als_operate: value=%d\n", value);
										if (value) {
												err = ltr578_als_enable(als_gainrange);
												if (err < 0) {
														APS_ERR("enable als fail: %d\n", err);
														return -1;
												}
												set_bit(CMC_BIT_ALS, &obj->enable);
										} else{
												err = ltr578_als_disable();
												if (err < 0) {
														APS_ERR("disable als fail: %d\n", err);
														return -1;
												}
												clear_bit(CMC_BIT_ALS, &obj->enable);
										}

								}
								break;

						case SENSOR_GET_DATA:
								if ((buff_out == NULL) ||
												(size_out < sizeof(struct hwm_sensor_data))) {
										APS_ERR("get sensor data parameter error!\n");
										err = -EINVAL;
								} else{
										APS_ERR("get sensor als data !\n");
										sensor_data = (struct hwm_sensor_data *)buff_out;
										obj->als = ltr578_als_read(als_gainrange);
#if defined(MTK_AAL_SUPPORT)
										sensor_data->values[0] = obj->als;
#else
										sensor_data->values[0] = ltr578_get_als_value(obj, obj->als);
#endif
										sensor_data->values[1] = 1000;

										sensor_data->value_divide = 1;
										sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
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

		static int ltr578_i2c_detect(struct i2c_client *client,
						struct i2c_board_info *info)
		{
				strcpy(info->type, LTR578_DEV_NAME);
				return 0;
		}

		static int als_open_report_data(int open)
		{
				/* should queuq work to report event
				   if  is_report_input_direct=true */
				return 0;
		}

		/* if use  this typ of enable ,
		   Gsensor only enabled but not report inputEvent to HAL */

		static int als_enable_nodata(int en)
		{
				int res = 0;
#ifdef CUSTOM_KERNEL_SENSORHUB
				SCP_SENSOR_HUB_DATA req;
				int len;
#endif /* #ifdef CUSTOM_KERNEL_SENSORHUB */

				APS_LOG("ltr578_obj als enable value = %d\n", en);

#ifdef CUSTOM_KERNEL_SENSORHUB
				req.activate_req.sensorType = ID_LIGHT;
				req.activate_req.action = SENSOR_HUB_ACTIVATE;
				req.activate_req.enable = en;
				len = sizeof(req.activate_req);
				res = SCP_sensorHub_req_send(&req, &len, 1);
#else /* #ifdef CUSTOM_KERNEL_SENSORHUB */
				if (!ltr578_obj) {
						APS_ERR("ltr578_obj is null!!\n");
						return -1;
				}

				if (en) {
						res = ltr578_als_enable(als_gainrange);
						if (res) {
								APS_ERR("enable als fail: %d\n", res);
								return -1;
						}
						set_bit(CMC_BIT_ALS, &ltr578_obj->enable);
				} else{
						res = ltr578_als_disable();
						if (res) {
								APS_ERR("disable als fail: %d\n", res);
								return -1;
						}
						clear_bit(CMC_BIT_ALS, &ltr578_obj->enable);
				}
#endif
				if (res) {
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

		static int als_get_data(int *value, int *status)
		{
				int err = 0;
#ifdef CUSTOM_KERNEL_SENSORHUB
				SCP_SENSOR_HUB_DATA req;
				int len;
#else
				struct ltr578_priv *obj = NULL;
#endif /* #ifdef CUSTOM_KERNEL_SENSORHUB */

#ifdef CUSTOM_KERNEL_SENSORHUB
				req.get_data_req.sensorType = ID_LIGHT;
				req.get_data_req.action = SENSOR_HUB_GET_DATA;
				len = sizeof(req.get_data_req);
				err = SCP_sensorHub_req_send(&req, &len, 1);
				if (err) {
						APS_ERR("SCP_sensorHub_req_send fail!\n");
				} else{
						*value = req.get_data_rsp.int16_Data[0];
						*status = SENSOR_STATUS_ACCURACY_MEDIUM;
				}

				if (atomic_read(&ltr578_obj->trace) & CMC_TRC_PS_DATA) {
						APS_LOG("value = %d\n", *value);
						/* show data */
				}
#else /* #ifdef CUSTOM_KERNEL_SENSORHUB */
				if (!ltr578_obj) {
						APS_ERR("ltr578_obj is null!!\n");
						return -1;
				}
				obj = ltr578_obj;
				obj->als = ltr578_als_read(als_gainrange);
				if (obj->als < 0) {
						err = -1;
				} else{
						*value = ltr578_get_als_value(obj, obj->als);
						*status = SENSOR_STATUS_ACCURACY_MEDIUM;
				}
#endif /* #ifdef CUSTOM_KERNEL_SENSORHUB */

				return err;
		}

		static int ps_open_report_data(int open)
		{
				/* should queuq work to report event
				   if  is_report_input_direct=true */
				return 0;
		}

		/* if use  this typ of enable , Gsensor
		   only enabled but not report inputEvent to HAL */

		static int ps_enable_nodata(int en)
		{
				int res = 0;
#ifdef CUSTOM_KERNEL_SENSORHUB
				SCP_SENSOR_HUB_DATA req;
				int len;
#endif /* #ifdef CUSTOM_KERNEL_SENSORHUB */

				APS_LOG("ltr578_obj ps enable value = %d\n", en);

#ifdef CUSTOM_KERNEL_SENSORHUB
				req.activate_req.sensorType = ID_PROXIMITY;
				req.activate_req.action = SENSOR_HUB_ACTIVATE;
				req.activate_req.enable = en;
				len = sizeof(req.activate_req);
				res = SCP_sensorHub_req_send(&req, &len, 1);
#else /* #ifdef CUSTOM_KERNEL_SENSORHUB */
				if (!ltr578_obj) {
						APS_ERR("ltr578_obj is null!!\n");
						return -1;
				}

				if (en) {
						res = ltr578_ps_enable();
						if (res) {
								APS_ERR("enable ps fail: %d\n", res);
								return -1;
						}
						set_bit(CMC_BIT_PS, &ltr578_obj->enable);
#ifdef PSENSOR_CALIBRATION
						ltr578_psensor_calibrate();
#endif
				} else{
						res = ltr578_ps_disable();
						if (res) {
								APS_ERR("disable ps fail: %d\n", res);
								return -1;
						}
						clear_bit(CMC_BIT_PS, &ltr578_obj->enable);
				}
#endif
				if (res) {
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

		static int ps_get_data(int *value, int *status)
		{
				int err = 0;
#ifdef CUSTOM_KERNEL_SENSORHUB
				SCP_SENSOR_HUB_DATA req;
				int len;
#endif /* #ifdef CUSTOM_KERNEL_SENSORHUB */

#ifdef CUSTOM_KERNEL_SENSORHUB
				req.get_data_req.sensorType = ID_PROXIMITY;
				req.get_data_req.action = SENSOR_HUB_GET_DATA;
				len = sizeof(req.get_data_req);
				err = SCP_sensorHub_req_send(&req, &len, 1);
				if (err) {
						APS_ERR("SCP_sensorHub_req_send fail!\n");
				} else{
						*value = req.get_data_rsp.int16_Data[0];
						*status = SENSOR_STATUS_ACCURACY_MEDIUM;
				}

				if (atomic_read(&ltr578_obj->trace) & CMC_TRC_PS_DATA) {
						APS_LOG("value = %d\n", *value);
						/* show data */
				}
#else /* #ifdef CUSTOM_KERNEL_SENSORHUB */
				if (!ltr578_obj) {
						APS_ERR("ltr578_obj is null!!\n");
						return -1;
				}
				ltr578_obj->ps = ltr578_ps_read();
				if (ltr578_obj->ps < 0) {
						err = -1;
				} else{
						*value = ltr578_get_ps_value();
						*status = SENSOR_STATUS_ACCURACY_MEDIUM;
				}
#endif /* #ifdef CUSTOM_KERNEL_SENSORHUB */

				return 0;
		}

		/*--------------------------------------
		  --------------------------------------*/
#ifdef CONFIG_HQ_CTP_GESTRUE_ANTI_FALSE_TOUCH
		/**********************************************************/
		//add by 20160130
		void ltr578_gesture_ps_get_data(int *value)
		{
				int data;
				ps_get_data(value, &data);
		}
		EXPORT_SYMBOL(ltr578_gesture_ps_get_data);

		void ltr578_gesture_ps_get_state(int *value,int suspend_flag)
		{
				struct ltr578_priv *obj = ltr578_obj;
				if (obj == NULL) {
						APS_ERR("ltr578_obj is null\n");
						return ;
				}

				if (suspend_flag == 0) {
						atomic_dec(&obj->ps_state);
						if (atomic_read(&obj->ps_state) <= 0) {
								atomic_set(&obj->ps_state, 0);
						}
				} else
						atomic_inc(&obj->ps_state);

				*value = atomic_read(&obj->ps_state);
		}
		EXPORT_SYMBOL(ltr578_gesture_ps_get_state);
#endif


		//extern char * g_alsps_name;
		static int ltr578_i2c_probe(struct i2c_client *client,
						const struct i2c_device_id *id)
		{
				struct ltr578_priv *obj;

				struct als_control_path als_ctl = {0};
				struct als_data_path als_data = {0};
				struct ps_control_path ps_ctl = {0};
				struct ps_data_path ps_data = {0};

				/* struct hwmsen_object obj_ps, obj_als; */
				int err = 0;

				//+add by hzb for ontim debug
				if(CHECK_THIS_DEV_DEBUG_AREADY_EXIT()==0)
				{ 
						return -EIO;
				}
				//-add by hzb for ontim debug
				APS_ERR("ltr578 probe start...!\n");
				obj = kzalloc(sizeof(*obj), GFP_KERNEL);
				if (!obj) {
						err = -ENOMEM;
						goto exit;
				}
				memset(obj, 0, sizeof(*obj));

				ltr578_obj=obj;
				obj->hw=hw ;

				err = get_alsps_dts_func(client->dev.of_node, obj->hw);
				if (err < 0) {
						APS_ERR("get customization info from dts failed\n");
						goto exit_init_failed;
				}

				APS_ERR("i2c_num=0x%x,I2C_ADDRS=0x%x;0x%x;\n",hw->i2c_num,hw->i2c_addr[0],client->addr);


				INIT_WORK(&obj->eint_work, ltr578_eint_work);

#ifdef CHECK_FAIL_WORK

				INIT_DELAYED_WORK(&dwork, ltr578_delay_work);//add delaywork

#endif

				obj->client = client;
				i2c_set_clientdata(client, obj);
				atomic_set(&obj->als_debounce, 300);
				atomic_set(&obj->als_deb_on, 0);
				atomic_set(&obj->als_deb_end, 0);
				atomic_set(&obj->ps_debounce, 300);
				atomic_set(&obj->ps_deb_on, 0);
				atomic_set(&obj->ps_deb_end, 0);
				atomic_set(&obj->ps_mask, 0);
				atomic_set(&obj->als_suspend, 0);
				atomic_set(&obj->ps_thd_val_high,  obj->hw->ps_threshold_high);
				atomic_set(&obj->ps_thd_val_low,  obj->hw->ps_threshold_low);
				/* atomic_set(&obj->als_cmd_val, 0xDF); */
				/* atomic_set(&obj->ps_cmd_val,  0xC1); */
				atomic_set(&obj->ps_thd_val,  obj->hw->ps_threshold);
				obj->enable = 0;
				obj->pending_intr = 0;
				obj->als_level_num = sizeof(obj->hw->als_level[0][0])/sizeof(obj->hw->als_level[0][0][0]);
				obj->als_value_num = sizeof(obj->hw->als_value)/sizeof(obj->hw->als_value[0]);
				obj->als_modulus = (400*100)/(16*150);
				/* (1/Gain)*(400/Tine), this value is fix
				   after init ATIME and CONTROL register value */
				/* (400)/16*2.72 here is amplify *100 */
				obj->irq_node = client->dev.of_node;
				obj->enable = 0;
				BUG_ON(sizeof(obj->als_level) != sizeof(obj->hw->als_level));
				memcpy(obj->als_level, obj->hw->als_level, sizeof(obj->als_level));
				BUG_ON(sizeof(obj->als_value) != sizeof(obj->hw->als_value));
				memcpy(obj->als_value, obj->hw->als_value, sizeof(obj->als_value));
				atomic_set(&obj->i2c_retry, 3);
				set_bit(CMC_BIT_ALS, &obj->enable);
				set_bit(CMC_BIT_PS, &obj->enable);
				APS_LOG("ltr578_devinit() start...!\n");

				ltr578_i2c_client = client;
				err = ltr578_devinit();
				if (err)
						goto exit_init_failed;

				APS_LOG("ltr578_devinit() ...OK!\n");

				/* printk("@@@@@@ Part ID:%x\n",ltr578_i2c_read_reg(0x06)); */
				err = misc_register(&ltr578_device);
				if (err) {
						APS_ERR("ltr578_device register failed\n");
						goto exit_misc_device_register_failed;
				}
				als_ctl.is_use_common_factory = false;
				ps_ctl.is_use_common_factory = false;


				/* Register sysfs attribute */
				err = (ltr578_create_attr(&(ltr578_init_info\
												.platform_diver_addr->driver)));
				if (err) {
						printk(KERN_ERR "create attribute err = %d\n", err);
						goto exit_create_attr_failed;
				}

				als_ctl.open_report_data = als_open_report_data;
				als_ctl.enable_nodata = als_enable_nodata;
				als_ctl.set_delay  = als_set_delay;
				als_ctl.batch = als_batch;
				als_ctl.flush = als_flush;
				als_ctl.is_report_input_direct = false;
#ifdef CUSTOM_KERNEL_SENSORHUB
				als_ctl.is_support_batch = true;
#else
				als_ctl.is_support_batch = false;
#endif

				err = als_register_control_path(&als_ctl);
				if (err) {
						APS_ERR("register fail = %d\n", err);
						goto exit_sensor_obj_attach_fail;
				}

				als_data.get_data = als_get_data;
				als_data.vender_div = 100;
				err = als_register_data_path(&als_data);
				if (err) {
						APS_ERR("tregister fail = %d\n", err);
						goto exit_sensor_obj_attach_fail;
				}


				ps_ctl.open_report_data = ps_open_report_data;
				ps_ctl.enable_nodata = ps_enable_nodata;
				ps_ctl.set_delay  = ps_set_delay;
				ps_ctl.batch = ps_batch;
				ps_ctl.flush = ps_flush;
				ps_ctl.is_report_input_direct = false;
#ifdef CUSTOM_KERNEL_SENSORHUB
				ps_ctl.is_support_batch = true;
#else
				ps_ctl.is_support_batch = false;
#endif

				err = ps_register_control_path(&ps_ctl);
				if (err) {
						APS_ERR("register fail = %d\n", err);
						goto exit_sensor_obj_attach_fail;
				}

				ps_data.get_data = ps_get_data;
				ps_data.vender_div = 100;
				err = ps_register_data_path(&ps_data);
				if (err) {
						APS_ERR("tregister fail = %d\n", err);
						goto exit_sensor_obj_attach_fail;
				}


#if defined(CONFIG_HAS_EARLYSUSPEND)
				obj->early_drv.level    = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1,
						obj->early_drv.suspend  = ltr578_early_suspend,
						obj->early_drv.resume   = ltr578_late_resume,
						register_early_suspend(&obj->early_drv);
#endif

#ifdef PSENSOR_CALIBRATION
				ltr578_get_init_noise();
#endif

				//+add by hzb for ontim debug
				REGISTER_AND_INIT_ONTIM_DEBUG_FOR_THIS_DEV();
				//-add by hzb for ontim debug
				//	g_alsps_name = LTR578_DEV_NAME;
				APS_LOG("%s: OK\n", __func__);
				ltr578_init_flag = 0;
				return 0;

exit_create_attr_failed:
				misc_deregister(&ltr578_device);
exit_misc_device_register_failed:
exit_sensor_obj_attach_fail:
exit_init_failed:
				/* i2c_detach_client(client); */
				/* exit_kfree: */
				kfree(obj);
exit:
				ltr578_i2c_client = NULL;
				/* MT6516_EINTIRQMask(CUST_EINT_ALS_NUM);*/
				APS_ERR("%s: err = %d\n", __func__, err);
				ltr578_init_flag = -1;
				return err;
		}

		/*--------------------------------------
		  --------------------------------------*/

static int ltr578_i2c_remove(struct i2c_client *client)
{
		int err;

		err = ltr578_delete_attr(&ltr578_i2c_driver.driver);
		if (err)
				APS_ERR("ltr578_delete_attr fail: %d\n", err);

		misc_deregister(&ltr578_device);

		ltr578_i2c_client = NULL;
		i2c_unregister_device(client);
		kfree(i2c_get_clientdata(client));

		return 0;
}

/*--------------------------------------
  ---------------------------------------*/
static int ltr578_local_init(void)
{
		APS_FUN();
		if (i2c_add_driver(&ltr578_i2c_driver)) {
				APS_ERR("add driver error\n");
				return -1;
		}
		if (ltr578_init_flag == -1) {
				APS_ERR("add driver error\n");
				return -1;
		}
		return 0;
}
/*--------------------------------------
  ---------------------------------------*/
static int ltr578_local_uninit(void)
{
		APS_FUN();
		i2c_del_driver(&ltr578_i2c_driver);
		return 0;
}


/*--------------------------------------
  --------------------------------------*/



/*--------------------------------------
  --------------------------------------*/
static int __init ltr578_init(void)
{
		//	const char *name = "mediatek,ltr578-new";

		APS_FUN();
		//	hw = get_alsps_dts_func(name, hw);
		//	APS_ERR(" i2c_num=%d,I2C_ADDRS=%d\n",hw->i2c_num,hw->i2c_addr[0]);

		//	i2c_register_board_info(hw->i2c_num, &i2c_ltr578, 1);

		//	if (!hw)
		//		APS_ERR("get dts info fail\n");

		if (alsps_driver_add(&ltr578_init_info))
				APS_ERR("alsps_driver_add fail\n");
		return 0;
}
/*--------------------------------------
  --------------------------------------*/
static void __exit ltr578_exit(void)
{
		APS_FUN();
}
/*--------------------------------------
  --------------------------------------*/
module_init(ltr578_init);
module_exit(ltr578_exit);
/*--------------------------------------
  --------------------------------------*/
MODULE_AUTHOR("XX Xx");
MODULE_DESCRIPTION("LTR-578ALS Driver");
MODULE_LICENSE("GPL");
/* Driver version v1.0 */
