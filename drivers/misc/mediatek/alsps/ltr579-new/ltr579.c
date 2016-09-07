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

/*******************************************************************************
 * Filename:ltr559.c
 *
 * Author:
 * -------
 * ZhaoHongguang, hongguang.zhao@wecorp.com.cn oct/10/2015
 * 
 *******************************************************************************/

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/gpio.h>
#include "cust_alsps.h"
#include "ltr579.h"
#include "alsps.h"

#define GN_MTK_BSP_PS_DYNAMIC_CALI
#define POWER_NONE_MACRO MT65XX_POWER_NONE

/******************************************************************************
 * configuration
*******************************************************************************/
/*----------------------------------------------------------------------------*/

#define LTR579_DEV_NAME   "LTR_579ALS"

/*----------------------------------------------------------------------------*/
#define APS_TAG                  "[ALS/PS] "
#define APS_FUN(f)               pr_debug(APS_TAG"%s\n", __func__)
#define APS_ERR(fmt, args...)    pr_err(APS_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#define APS_LOG(fmt, args...)    pr_info(APS_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#define APS_DBG(fmt, args...)    pr_debug(APS_TAG"%s %d : "fmt, __func__, __LINE__, ##args)

/*----------------------------------------------------------------------------*/

static struct i2c_client *ltr579_i2c_client = NULL;
static int isadjust=0;
static int dynamic_cali = 0;
/*---------------------CONFIG_OF START------------------*/

struct alsps_hw alsps_cust;
static struct alsps_hw *hw = &alsps_cust;
struct platform_device *alspsPltFmDev;
static unsigned long long int_top_time;
/* For alsp driver get cust info */
struct alsps_hw *get_cust_alsps(void)
{
	return &alsps_cust;
}
/*---------------------CONFIG_OF	END------------------*/


/*----------------------------------------------------------------------------*/
static const struct i2c_device_id ltr579_i2c_id[] = {{LTR579_DEV_NAME,0},{}};
/*the adapter id & i2c address will be available in customization*/
//static struct i2c_board_info __initdata i2c_ltr579={ I2C_BOARD_INFO("LTR_579ALS", 0x53)};

//static unsigned short ltr579_force[] = {0x00, 0x46, I2C_CLIENT_END, I2C_CLIENT_END};
//static const unsigned short *const ltr579_forces[] = { ltr579_force, NULL };
//static struct i2c_client_address_data ltr579_addr_data = { .forces = ltr579_forces,};
/*----------------------------------------------------------------------------*/
static int ltr579_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id); 
static int ltr579_i2c_remove(struct i2c_client *client);
static int ltr579_i2c_detect(struct i2c_client *client, struct i2c_board_info *info);
/*----------------------------------------------------------------------------*/
static int ltr579_i2c_suspend(struct i2c_client *client, pm_message_t msg);
static int ltr579_i2c_resume(struct i2c_client *client);
static int ltr579_ps_enable(void);
static int ltr579_ps_disable(void);
static int ltr579_als_disable(void);
static int ltr579_als_enable(int gainrange);
static int als_get_data(int* value, int* status);
extern struct alsps_context *alsps_context_obj;

static int als_gainrange;

static int final_prox_val;
static int final_lux_val;

/*----------------------------------------------------------------------------*/
static DEFINE_MUTEX(read_lock);

/*----------------------------------------------------------------------------*/
static int ltr579_als_read(int gainrange);
static int ltr579_ps_read(void);

static int ltr579_local_init(void);
static int ltr579_local_uninit(void);
/*----------------------------------------------------------------------------*/
static struct alsps_init_info ltr579_init_info = {
		.name = "ltr579",
		.init = ltr579_local_init,
		.uninit = ltr579_local_uninit,
};



typedef enum {
    CMC_BIT_ALS    = 1,
    CMC_BIT_PS     = 2,
} CMC_BIT;

/*----------------------------------------------------------------------------*/
struct ltr579_i2c_addr {    /*define a series of i2c slave address*/
    u8  write_addr;  
    u8  ps_thd;     /*PS INT threshold*/
};

/*----------------------------------------------------------------------------*/

struct ltr579_priv {
    struct alsps_hw  *hw;
    struct i2c_client *client;
    struct work_struct  eint_work;
    struct mutex lock;
	/*i2c address group*/
    struct ltr579_i2c_addr  addr;
	struct device_node *irq_node;
	int irq;

     /*misc*/
    u16		    als_modulus;
    atomic_t    i2c_retry;
    atomic_t    als_debounce;   /*debounce time after enabling als*/
    atomic_t    als_deb_on;     /*indicates if the debounce is on*/
    atomic_t    als_deb_end;    /*the jiffies representing the end of debounce*/
    atomic_t    ps_mask;        /*mask ps: always return far away*/
    atomic_t    ps_debounce;    /*debounce time after enabling ps*/
    atomic_t    ps_deb_on;      /*indicates if the debounce is on*/
    atomic_t    ps_deb_end;     /*the jiffies representing the end of debounce*/
    atomic_t    ps_suspend;
    atomic_t    als_suspend;

    /*data*/
    u16         als;
    u16          ps;
    u8          _align;
    u16         als_level_num;
    u16         als_value_num;
    u32         als_level[C_CUST_ALS_LEVEL-1];
    u32         als_value[C_CUST_ALS_LEVEL];

    atomic_t    als_cmd_val;    /*the cmd value can't be read, stored in ram*/
    atomic_t    ps_cmd_val;     /*the cmd value can't be read, stored in ram*/
    atomic_t    ps_thd_val;     /*the cmd value can't be read, stored in ram*/
	atomic_t    ps_thd_val_high;     /*the cmd value can't be read, stored in ram*/
	atomic_t    ps_thd_val_low;     /*the cmd value can't be read, stored in ram*/
    ulong       enable;         /*enable mask*/
    ulong       pending_intr;   /*pending interrupt*/

    /*early suspend*/
#if defined(CONFIG_HAS_EARLYSUSPEND)
    struct early_suspend    early_drv;
#endif     
};

 struct PS_CALI_DATA_STRUCT
{
    int close;
    int far_away;
    int valid;
} ;

static struct PS_CALI_DATA_STRUCT ps_cali={0,0,0};
static int intr_flag_value = 0;


static struct ltr579_priv *ltr579_obj = NULL;
//static struct platform_driver ltr579_alsps_driver;
#ifdef CONFIG_OF
static const struct of_device_id alsps_of_match[] = {
	{.compatible = "mediatek,alsps"},
	{},
};
#endif

/*----------------------------------------------------------------------------*/
static struct i2c_driver ltr579_i2c_driver = {	
	.probe      = ltr579_i2c_probe,
	.remove     = ltr579_i2c_remove,
	.detect     = ltr579_i2c_detect,
	.suspend    = ltr579_i2c_suspend,
	.resume     = ltr579_i2c_resume,
	.id_table   = ltr579_i2c_id,
	//.address_data = &ltr579_addr_data,
	.driver = {
		//.owner          = THIS_MODULE,
		.name           = LTR579_DEV_NAME,
		#ifdef CONFIG_OF
		.of_match_table = alsps_of_match,
		#endif
	},
};



//add by wangxiqiang
#ifdef CONFIG_SLT_DRV_DEVINFO_SUPPORT
#define SLT_DEVINFO_ALSPS_DEBUG
#include  <linux/dev_info.h>
//u8 ver_id;
//u8 ver_module;
//static int devinfo_first=0;
//static char* temp_ver;
static char* temp_comments;
struct devinfo_struct *s_DEVINFO_alsps;   //suppose 10 max lcm device 
//The followd code is for GTP style
static void devinfo_ctp_regchar(char *module,char * vendor,char *version,char *used, char* comments)
{

	s_DEVINFO_alsps =(struct devinfo_struct*) kmalloc(sizeof(struct devinfo_struct), GFP_KERNEL);	
	s_DEVINFO_alsps->device_type="alsps";
	s_DEVINFO_alsps->device_module=module;
	s_DEVINFO_alsps->device_vendor=vendor;
	s_DEVINFO_alsps->device_ic="LTR579";
	s_DEVINFO_alsps->device_info=DEVINFO_NULL;
	s_DEVINFO_alsps->device_version=version;
	s_DEVINFO_alsps->device_used=used;
	s_DEVINFO_alsps->device_comments=comments;
#ifdef SLT_DEVINFO_ALSPS_DEBUG
		printk("[DEVINFO accel sensor]registe CTP device! type:<%s> module:<%s> vendor<%s> ic<%s> version<%s> info<%s> used<%s> comment<%s>\n",
				s_DEVINFO_alsps->device_type,s_DEVINFO_alsps->device_module,s_DEVINFO_alsps->device_vendor,
				s_DEVINFO_alsps->device_ic,s_DEVINFO_alsps->device_version,s_DEVINFO_alsps->device_info,s_DEVINFO_alsps->device_used,s_DEVINFO_alsps->device_comments);
#endif
       DEVINFO_CHECK_DECLARE_COMMEN(s_DEVINFO_alsps->device_type,s_DEVINFO_alsps->device_module,s_DEVINFO_alsps->device_vendor,s_DEVINFO_alsps->device_ic,s_DEVINFO_alsps->device_version,s_DEVINFO_alsps->device_info,s_DEVINFO_alsps->device_used,s_DEVINFO_alsps->device_comments);
      //devinfo_check_add_device(s_DEVINFO_ctp);

}
#endif


/* 
 * #########
 * ## I2C ##
 * #########
 */

// I2C Read
static int ltr579_i2c_read_reg(u8 regnum)
{
    u8 buffer[1],reg_value[1];
	int res = 0;
	mutex_lock(&read_lock);
	
	buffer[0]= regnum;
	res = i2c_master_send(ltr579_obj->client, buffer, 0x1);
	if(res <= 0)	{
		APS_ERR("read reg send res = %d\n",res);
		mutex_unlock(&read_lock);
		return res;
	}
	res = i2c_master_recv(ltr579_obj->client, reg_value, 0x1);
	if(res <= 0)
	{
		APS_ERR("read reg recv res = %d\n",res);
		mutex_unlock(&read_lock);
		return res;
	}
	mutex_unlock(&read_lock);
	return reg_value[0];
}

// I2C Write
static int ltr579_i2c_write_reg(u8 regnum, u8 value)
{
	u8 databuf[2];    
	int res = 0;
   
	databuf[0] = regnum;   
	databuf[1] = value;
	res = i2c_master_send(ltr579_obj->client, databuf, 0x2);

	if (res < 0)
		{
			APS_ERR("wirte reg send res = %d\n",res);
		   	return res;
		}
		
	else
		return 0;
}

/*----------------------------------------------------------------------------*/
static ssize_t ltr579_show_als(struct device_driver *ddri, char *buf)
{
	int res;
	
	if(!ltr579_obj)
	{
		APS_ERR("ltr579_obj is null!!\n");
		return 0;
	}
	res = ltr579_als_read(als_gainrange);
    return snprintf(buf, PAGE_SIZE, "0x%04X\n", res);    
	
}
/*----------------------------------------------------------------------------*/

static ssize_t ltr579_show_ps(struct device_driver *ddri, char *buf)
{
	int  res;
	if(!ltr579_obj)
	{
		APS_ERR("ltr579_obj is null!!\n");
		return 0;
	}
	res = ltr579_ps_read();
    return snprintf(buf, PAGE_SIZE, "0x%04X\n", res);     
}
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
static ssize_t ltr579_show_status(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	
	if(!ltr579_obj)
	{
		APS_ERR("ltr579_obj is null!!\n");
		return 0;
	}
	
	if(ltr579_obj->hw)
	{
	
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: %d, (%d %d)\n", 
			ltr579_obj->hw->i2c_num, ltr579_obj->hw->power_id, ltr579_obj->hw->power_vol);
		
	}
	else
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: NULL\n");
	}


	len += snprintf(buf+len, PAGE_SIZE-len, "MISC: %d %d\n", atomic_read(&ltr579_obj->als_suspend), atomic_read(&ltr579_obj->ps_suspend));

	return len;
}

/*----------------------------------------------------------------------------*/
static ssize_t ltr579_store_status(struct device_driver *ddri, const char *buf, size_t count)
{
	int status1,ret;
	if(!ltr579_obj)
	{
		APS_ERR("ltr579_obj is null!!\n");
		return 0;
	}

#if 0 //sang
	if(1 == sscanf(buf, "%d ", &status1))
	{ 
	    ret=ltr579_ps_enable();
		APS_DBG("iret= %d \n", ret);
	}
	else
	{
		//APS_DBG("invalid content: '%s', length = %d\n", buf, count);
		APS_DBG("invalid content: '%s'\n", buf);
	}
#else
	if(1 == sscanf(buf, "%d ", &status1))
	{ 
	   if(status1==1)
	   	{
		    ret=ltr579_ps_enable();
	   		if (ret < 0)
				APS_DBG("iret= %d \n", ret);
	   	}
	   else if(status1==2)
	   	{
			ret = ltr579_als_enable(als_gainrange);
			if (ret < 0)
				APS_DBG("iret= %d \n", ret);
	   	}
	   else if(status1==0)
	   	{
			ret=ltr579_ps_disable();
	   		if (ret < 0)
				APS_DBG("iret= %d \n", ret);
			
			ret=ltr579_als_disable();
	   		if (ret < 0)
				APS_DBG("iret= %d \n", ret);
	   	}
	}
	else
	{
		//APS_DBG("invalid content: '%s', length = %d\n", buf, count);
		APS_DBG("invalid content: '%s'\n", buf);
	}
#endif
	return count;    
}


/*----------------------------------------------------------------------------*/
static ssize_t ltr579_show_reg(struct device_driver *ddri, char *buf)
{
	int i,len=0;
	int reg[]={0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0d,0x0e,0x0f,
				0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20,0x21,0x22,0x23,0x24,0x25,0x26};
	for(i=0;i<27;i++)
		{
		len += snprintf(buf+len, PAGE_SIZE-len, "reg:0x%04X value: 0x%04X\n", reg[i],ltr579_i2c_read_reg(reg[i]));	

	    }
	return len;
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr579_store_reg(struct device_driver *ddri,const char *buf, size_t count)
{
	int ret,value;
	unsigned int  reg;
	if(!ltr579_obj)
	{
		APS_ERR("ltr579_obj is null!!\n");
		return 0;
	}
	
	if(2 == sscanf(buf, "%x %x ", &reg,&value))
	{ 
		APS_DBG("before write reg: %x, reg_value = %x  write value=%x\n", reg,ltr579_i2c_read_reg(reg),value);
	    ret=ltr579_i2c_write_reg(reg,value);
		APS_DBG("after write reg: %x, reg_value = %x\n", reg,ltr579_i2c_read_reg(reg));
	}
	else
	{
		//APS_DBG("invalid content: '%s', length = %d\n", buf, count); //sang
		APS_DBG("invalid content: '%s'\n", buf);
	}
	return count;    
}

/*----------------------------------------------------------------------------*/
static ssize_t ltr579_show_config(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	
	if(!ltr579_obj)
	{
		APS_ERR("ltr579_obj is null!!\n");
		return 0;
	}

	res = scnprintf(buf,PAGE_SIZE,"(%d %d %d %d %d %d)\n",atomic_read(&ltr579_obj->i2c_retry),atomic_read(&ltr579_obj->als_debounce),atomic_read(&ltr579_obj->ps_mask),atomic_read(&ltr579_obj->ps_thd_val_high),atomic_read(&ltr579_obj->ps_thd_val_low),atomic_read(&ltr579_obj->ps_debounce));

	return res;

}
/*----------------------------------------------------------------------------*/
static ssize_t ltr579_store_config(struct device_driver *ddri, const char *buf, size_t count)
{
	int retry, als_deb, ps_deb, mask, hthres, lthres;
	//struct i2c_client *client;
	//client = ltr579_i2c_client;
	if(!ltr579_obj)
	{
		APS_ERR("ltr579_obj is null!!\n");
		return 0;
	}
	
	if(6 == sscanf(buf, "%d %d %d %d %d %d", &retry, &als_deb, &mask, &hthres, &lthres, &ps_deb))
	{ 
		atomic_set(&ltr579_obj->i2c_retry, retry);
		atomic_set(&ltr579_obj->als_debounce, als_deb);
		atomic_set(&ltr579_obj->ps_mask, mask);
		atomic_set(&ltr579_obj->ps_thd_val_high, hthres);    
		atomic_set(&ltr579_obj->ps_thd_val_low, lthres);        
		atomic_set(&ltr579_obj->ps_debounce, ps_deb);

	}
	else
	{
		APS_ERR("invalid content: '%s', length = %zu\n", buf, count);
	}
	return count;    
}

static int ltr579_get_ps_value(void);
static ssize_t ltr579_show_distance(struct device_driver *ddri, char *buf)
{
	
	//struct ltr579_priv *obj = i2c_get_clientdata(ltr579_i2c_client);  
	int dat = 0;
	
	if(!ltr579_obj)
	{
		APS_ERR("ltr579_obj is null!!\n");
		return 0;
	}

	
	dat = ltr579_get_ps_value();

	return scnprintf(buf,PAGE_SIZE,"%d\n",dat);

}

static ssize_t ltr579_show_reset(struct device_driver *ddri, char *buf)
{

	int dat = 0;

	if(!ltr579_obj)
	{
		APS_ERR("ltr579_obj is null!!\n");
		return 0;
	}

	dat = ltr579_i2c_write_reg(LTR579_MAIN_CTRL, 0x10); 

	return scnprintf(buf,PAGE_SIZE,"%d\n",dat);

}


#if 1
/*----------------------------------------------------------------------------*/
static DRIVER_ATTR(als,     S_IWUSR | S_IRUGO, ltr579_show_als,   NULL);
static DRIVER_ATTR(ps,      S_IWUSR | S_IRUGO, ltr579_show_ps,    NULL);
static DRIVER_ATTR(distance,S_IWUSR | S_IRUGO, ltr579_show_distance,    NULL);
static DRIVER_ATTR(config,  S_IWUSR | S_IRUGO, ltr579_show_config,ltr579_store_config);
static DRIVER_ATTR(status,  S_IWUSR | S_IRUGO, ltr579_show_status,  ltr579_store_status);
static DRIVER_ATTR(reg,     S_IWUSR | S_IRUGO, ltr579_show_reg,   ltr579_store_reg);
static DRIVER_ATTR(reset,     S_IWUSR | S_IRUGO, ltr579_show_reset,   NULL);

/*----------------------------------------------------------------------------*/
static struct driver_attribute *ltr579_attr_list[] = {
    &driver_attr_als,
    &driver_attr_ps,
    &driver_attr_distance,
    &driver_attr_config,
    &driver_attr_status,
    &driver_attr_reg,
    &driver_attr_reset,
};

/*----------------------------------------------------------------------------*/
static int ltr579_create_attr(struct device_driver *driver) 
{
	int idx, err = 0;
	int num = (int)(sizeof(ltr579_attr_list)/sizeof(ltr579_attr_list[0]));

	if (driver == NULL)
	{
		return -EINVAL;
	}

	for(idx = 0; idx < num; idx++)
	{
		if((err = (driver_create_file(driver, ltr579_attr_list[idx]))))
		{            
			APS_ERR("driver_create_file (%s) = %d\n", ltr579_attr_list[idx]->attr.name, err);
			break;
		}
	}    
	return err;
}
/*----------------------------------------------------------------------------*/
	static int ltr579_delete_attr(struct device_driver *driver)
	{
	int idx ,err = 0;
	int num = (int)(sizeof(ltr579_attr_list)/sizeof(ltr579_attr_list[0]));

	if (!driver)
	return -EINVAL;

	for (idx = 0; idx < num; idx++) 
	{
		driver_remove_file(driver, ltr579_attr_list[idx]);
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
static int ltr579_dynamic_calibrate(void)
{
	//int ret = 0;
	int i = 0;
	int j = 0;
	int data = 0;
	int noise = 0;
	//int len = 0;
	//int err = 0;
	int max = 0;
	//int idx_table = 0;
	unsigned long data_total = 0;
	int ps_thd_val_low ;
	int ps_thd_val_high ;
	struct ltr579_priv *obj = ltr579_obj;


	APS_FUN(f);
	if (!obj) goto err;

	mdelay(10);
	for (i = 0; i < 5; i++) {
		if (max++ > 5) {
			atomic_set(&obj->ps_thd_val_high,  2047);
			atomic_set(&obj->ps_thd_val_low, 2047);

			goto err;
		}
		mdelay(15);
		
		data = ltr579_ps_read();
		if(data == 0){
			j++;
		}	
		data_total += data;
	}
	noise = data_total/(5 - j);

	if(noise < (dynamic_cali + 300))
	{
				//dynamic_cali = noise;
				isadjust = 1;
				
				if(noise < 22){
						atomic_set(&obj->ps_thd_val_high,  noise+67);
						atomic_set(&obj->ps_thd_val_low, noise+33);
				}else if(noise < 28){
						atomic_set(&obj->ps_thd_val_high,  noise+79);
						atomic_set(&obj->ps_thd_val_low, noise+38);
				}else if(noise < 100){
						atomic_set(&obj->ps_thd_val_high,  noise+94);
						atomic_set(&obj->ps_thd_val_low, noise+43);
				}else {
						atomic_set(&obj->ps_thd_val_high,  noise+104);
						atomic_set(&obj->ps_thd_val_low, noise+48);
				}

	} else {
		atomic_set(&obj->ps_thd_val_high,  obj->hw->ps_threshold_high);
		atomic_set(&obj->ps_thd_val_low,  obj->hw->ps_threshold_low);
	}
	
	ps_thd_val_low = atomic_read(&obj->ps_thd_val_low);
	ps_thd_val_high = atomic_read(&obj->ps_thd_val_high);
	printk(" ltr579_dynamic_calibrate end:noise=%d, dynamic_cali = %d, isadjust= %d,  obj->ps_thd_val_low= %d , obj->ps_thd_val_high = %d\n",
		noise, dynamic_cali, isadjust, ps_thd_val_low, ps_thd_val_high);

	return 0;
err:
	APS_ERR("ltr579_dynamic_calibrate fail!!!\n");
	return -1;
}
#endif

static int ltr579_ps_set_thres(void)
{
	int res;
	u8 databuf[2];
	struct i2c_client *client = ltr579_obj->client;
	struct ltr579_priv *obj = ltr579_obj;
	APS_FUN();

	
	
	
		
	APS_LOG("ps_cali.valid: %d\n", ps_cali.valid);
	if(1 == ps_cali.valid)
	{
		databuf[0] = LTR579_PS_THRES_LOW_0; 
		databuf[1] = (u8)(ps_cali.far_away & 0x00FF);
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
			return LTR579_ERR_I2C;
		}
		databuf[0] = LTR579_PS_THRES_LOW_1; 
		databuf[1] = (u8)((ps_cali.far_away & 0xFF00) >> 8);
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
			return LTR579_ERR_I2C;
		}
		databuf[0] = LTR579_PS_THRES_UP_0;	
		databuf[1] = (u8)(ps_cali.close & 0x00FF);
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
			return LTR579_ERR_I2C;
		}
		databuf[0] = LTR579_PS_THRES_UP_1;	
		databuf[1] = (u8)((ps_cali.close & 0xFF00) >> 8);;
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
			return LTR579_ERR_I2C;
		}
	}
	else
	{
		databuf[0] = LTR579_PS_THRES_LOW_0; 
		databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_low)) & 0x00FF);
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
			return LTR579_ERR_I2C;
		}
		databuf[0] = LTR579_PS_THRES_LOW_1; 
		databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_low )>> 8) & 0x00FF);
		
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
			return LTR579_ERR_I2C;
		}
		databuf[0] = LTR579_PS_THRES_UP_0;	
		databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_high)) & 0x00FF);
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
			return LTR579_ERR_I2C;
		}
		databuf[0] = LTR579_PS_THRES_UP_1;	
		databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_high) >> 8) & 0x00FF);
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
			return LTR579_ERR_I2C;
		}
	
	}

	res = 0;
	return res;
	
	EXIT_ERR:
	APS_ERR("set thres: %d\n", res);
	return res;

}


static int ltr579_ps_enable(void)
{
	struct i2c_client *client = ltr579_obj->client;
	struct ltr579_priv *obj = ltr579_obj;
	u8 databuf[2];	
	int res;

	int error;
	int setctrl;
    APS_LOG("ltr579_ps_enable() ...start!\n");

	
	mdelay(WAKEUP_DELAY);
    
	/* =============== 
	 * ** IMPORTANT **
	 * ===============
	 * Other settings like timing and threshold to be set here, if required.
 	 * Not set and kept as device default for now.
 	 */
   	error = ltr579_i2c_write_reg(LTR579_PS_PULSES, 32); //32pulses 
	if(error<0)
    {
        APS_LOG("ltr579_ps_enable() PS Pulses error2\n");
	    return error;
	} 
	error = ltr579_i2c_write_reg(LTR579_PS_LED, 0x36); // 60khz & 100mA 
	if(error<0)
    {
        APS_LOG("ltr579_ps_enable() PS LED error...\n");
	    return error;
	}
		error = ltr579_i2c_write_reg(LTR579_PS_MEAS_RATE, 0x5C); // 11bits & 50ms time 
	if(error<0)
    {
        APS_LOG("ltr579_ps_enable() PS time error...\n");
	    return error;
	}


	/*for interrup work mode support -- by WeeLiat, Liteon 18.06.2015*/
		if(0 == obj->hw->polling_mode_ps)
		{		

			ltr579_ps_set_thres();
			
			databuf[0] = LTR579_INT_CFG;	
			databuf[1] = 0x01;
			res = i2c_master_send(client, databuf, 0x2);
			if(res <= 0)
			{
				goto EXIT_ERR;
				return LTR579_ERR_I2C;
			}
	
			databuf[0] = LTR579_INT_PST;	
			databuf[1] = 0x02;
			res = i2c_master_send(client, databuf, 0x2);
			if(res <= 0)
			{
				goto EXIT_ERR;
				return LTR579_ERR_I2C;
			}
			//mt_eint_unmask(CUST_EINT_ALS_NUM);
			//enable_irq(obj->irq);
			//APS_LOG("enable_irq\n");
		}
	
 	setctrl = ltr579_i2c_read_reg(LTR579_MAIN_CTRL);
	if((setctrl & 0x01) == 0)//Check for PS enable?
	{
		setctrl = setctrl | 0x01;
		error = ltr579_i2c_write_reg(LTR579_MAIN_CTRL, setctrl); 
		if(error<0)
		{
	    APS_ERR("ltr579_ps_enable() error1\n");
	    return error;
		}
	}

	set_bit(CMC_BIT_PS, &obj->enable);
	
	APS_LOG("ltr579_ps_enable ...OK!\n");
	
	#ifdef GN_MTK_BSP_PS_DYNAMIC_CALI
	ltr579_dynamic_calibrate();
	#endif
	ltr579_ps_set_thres();

	return error;

	EXIT_ERR:
	APS_ERR("set thres: %d\n", res);
	return res;
}

// Put PS into Standby mode
static int ltr579_ps_disable(void)
{
	int error;
	struct ltr579_priv *obj = ltr579_obj;
	int setctrl;
	
	setctrl = ltr579_i2c_read_reg(LTR579_MAIN_CTRL);
	
	setctrl = setctrl & 0xFE; 	
	
	
	error = ltr579_i2c_write_reg(LTR579_MAIN_CTRL, setctrl);  //sang
	if(error<0)
		APS_LOG("ltr579_ps_disable ...ERROR\n");
	else {
		clear_bit(CMC_BIT_PS, &obj->enable);
		APS_LOG("ltr579_ps_disable ...OK\n");
	}

	if(0 == obj->hw->polling_mode_ps)
	{
	    //cancel_work_sync(&obj->eint_work);
	    //disable_irq_nosync(obj->irq);
	    //APS_LOG("disable_irq_nosync\n");
	}
	
	return error;
}


static int ltr579_ps_read(void)
{
	int psval_lo, psval_hi, psdata;

	psval_lo = ltr579_i2c_read_reg(LTR579_PS_DATA_0);
	APS_DBG("ps_rawdata_psval_lo = %d\n", psval_lo);
	if (psval_lo < 0){
	    
	    APS_DBG("psval_lo error\n");
		psdata = psval_lo;
		goto out;
	}
	psval_hi = ltr579_i2c_read_reg(LTR579_PS_DATA_1);
    APS_DBG("ps_rawdata_psval_hi = %d\n", psval_hi);

	if (psval_hi < 0){
	    APS_DBG("psval_hi error\n");
		psdata = psval_hi;
		goto out;
	}
	
	psdata = ((psval_hi & 7)* 256) + psval_lo;
    //psdata = ((psval_hi&0x7)<<8) + psval_lo;
    APS_LOG("ps_rawdata = %d\n", psdata);
    
	out:
	final_prox_val = psdata;
	
	return psdata;
}

/* 
 * ################
 * ## ALS CONFIG ##
 * ################
 */
static int ltr579_als_enable(int gainrange)
{
	int error, ret;
	int setctrl;
	int als_value, status;

	APS_LOG("gainrange = %d\n",gainrange);

	switch (gainrange)
	{
		case ALS_RANGE_1:
			error = ltr579_i2c_write_reg(LTR579_ALS_GAIN, MODE_ALS_Range1);
			break;

		case ALS_RANGE_3:
			error = ltr579_i2c_write_reg(LTR579_ALS_GAIN, MODE_ALS_Range3);
			break;

		case ALS_RANGE_6:
			error = ltr579_i2c_write_reg(LTR579_ALS_GAIN, MODE_ALS_Range6);
			break;
			
		case ALS_RANGE_9:
			error = ltr579_i2c_write_reg(LTR579_ALS_GAIN, MODE_ALS_Range9);
			break;
			
		case ALS_RANGE_18:
			error = ltr579_i2c_write_reg(LTR579_ALS_GAIN, MODE_ALS_Range18);
			break;
		
		default:
			error = ltr579_i2c_write_reg(LTR579_ALS_GAIN, MODE_ALS_Range3);			
			APS_ERR("ALS sensor gainrange %d!\n", gainrange);
			break;
	}

	error = ltr579_i2c_write_reg(LTR579_ALS_GAIN, MODE_ALS_Range18);


	error = ltr579_i2c_write_reg(LTR579_ALS_MEAS_RATE, ALS_RESO_MEAS);// 18 bit & 100ms measurement rate		
	APS_ERR("ALS sensor resolution & measurement rate: %d!\n", ALS_RESO_MEAS );	

	setctrl = ltr579_i2c_read_reg(LTR579_MAIN_CTRL);
	setctrl = setctrl | 0x02;// Enable ALS
	
	error = ltr579_i2c_write_reg(LTR579_MAIN_CTRL, setctrl);	

	mdelay(150);

	ret = als_get_data(&als_value, &status);
	if (ret) {
		APS_ERR("get first alsps data fails!!\n");
	} else {
		if((ret = als_data_report(alsps_context_obj->idev_als, als_value, status)))
		{
			APS_ERR("ltr579_sensor call als_data_report fail = %d\n", ret);
		}else {
			APS_LOG("ltr579_als_enable:  report  first data after enable, als_value= %d, status=%d\n", als_value, status);
		}
	}

	/* =============== 
	 * ** IMPORTANT **
	 * ===============
	 * Other settings like timing and threshold to be set here, if required.
	 * Not set and kept as device default for now.
	 */
	if(error<0)
		APS_ERR("ltr579_als_enable ...ERROR\n");
	else
		APS_LOG("ltr579_als_enable ...OK\n");

	return error;
}


// Put ALS into Standby mode
static int ltr579_als_disable(void)
{
	int error;
	int setctrl;

	error = 0;
	
	setctrl = ltr579_i2c_read_reg(LTR579_MAIN_CTRL);
	if(((setctrl & 0x03) == 0x02) ){	// do not repeat disable ALS 
		setctrl = setctrl & 0xFD;

		error = ltr579_i2c_write_reg(LTR579_MAIN_CTRL, setctrl);
	}

	if(error<0)
 	    APS_LOG("ltr579_als_disable ...ERROR\n");
 	else
        APS_LOG("ltr579_als_disable ...OK\n");

	als_data_report(alsps_context_obj->idev_als, -2, SENSOR_STATUS_ACCURACY_MEDIUM);
	APS_LOG("ltr579_als_disable: report one fake als value -2 to make sure the first data after enable could be reported sucessfully\n");

	return error;
}

static int ltr579_als_read(int gainrange)
{
	int alsval_0, alsval_1, alsval_2, alsval;
	int luxdata_int;

	alsval_0 = ltr579_i2c_read_reg(LTR579_ALS_DATA_0);
	alsval_1 = ltr579_i2c_read_reg(LTR579_ALS_DATA_1);
	alsval_2 = ltr579_i2c_read_reg(LTR579_ALS_DATA_2);
	alsval = (alsval_2 * 256* 256) + (alsval_1 * 256) + alsval_0;
	APS_DBG("alsval_0 = %d,alsval_1=%d,alsval_2=%d,alsval=%d\n",alsval_0,alsval_1,alsval_2,alsval);

	if(alsval==0)
	{
		luxdata_int = 0;
		goto err;
	}

	APS_DBG("gainrange = %d\n",gainrange);
	
	luxdata_int = alsval;//*8/gainrange/10;//formula: ALS counts * 0.8/gain/int , int=1
	
	APS_LOG("als_value_lux = %d\n", luxdata_int);
	return luxdata_int;

	
err:
	final_lux_val = luxdata_int;
	APS_ERR("err als_value_lux = 0x%x\n", luxdata_int);
	return luxdata_int;
}



/*----------------------------------------------------------------------------*/
int ltr579_get_addr(struct alsps_hw *hw, struct ltr579_i2c_addr *addr)
{
	/***
	if(!hw || !addr)
	{
		return -EFAULT;
	}
	addr->write_addr= hw->i2c_addr[0];
	***/
	return 0;
}


/*-----------------------------------------------------------------------------*/
void ltr579_eint_func(void)
{
	struct ltr579_priv *obj = ltr579_obj;
	APS_FUN();
	APS_LOG("[ALS/PS]: eint func START!!\n");

	
	if(!obj)
	{
		return;
	}

	int_top_time = sched_clock();
	schedule_work(&obj->eint_work);
	APS_LOG("[ALS/PS]: eint func END !!\n");
	//schedule_delayed_work(&obj->eint_work);
}

#if defined(CONFIG_OF)
static irqreturn_t ltr579_eint_handler(int irq, void *desc)
{
	APS_LOG("ltr579_eint_handler!!\n");
	ltr579_eint_func();
	disable_irq_nosync(ltr579_obj->irq);
	APS_LOG("disable_irq_nosync\n");

	return IRQ_HANDLED;
}
#endif

/*----------------------------------------------------------------------------*/
/*for interrup work mode support -- by liaoxl.lenovo 12.08.2011*/

int ltr579_setup_eint(struct i2c_client *client)
{
	int ret;
	struct pinctrl *pinctrl;
	struct pinctrl_state *pins_default;
	struct pinctrl_state *pins_cfg;
	u32 ints[2] = {0, 0};

	alspsPltFmDev = get_alsps_platformdev();

/* gpio setting */

	pinctrl = devm_pinctrl_get(&alspsPltFmDev->dev);
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
	if (ltr579_obj->irq_node) {
		of_property_read_u32_array(ltr579_obj->irq_node, "debounce", ints, ARRAY_SIZE(ints));
		gpio_set_debounce(ints[0], ints[1]);
		APS_LOG("ints[0] = %d, ints[1] = %d!!\n", ints[0], ints[1]);

		ltr579_obj->irq = irq_of_parse_and_map(ltr579_obj->irq_node, 0);
		APS_LOG("ltr579_obj->irq = %d\n", ltr579_obj->irq);
		if (!ltr579_obj->irq) {
			APS_ERR("irq_of_parse_and_map fail!!\n");
			return -EINVAL;
		}

		if (request_irq(ltr579_obj->irq, ltr579_eint_handler, IRQF_TRIGGER_LOW, "ALS-eint", NULL)) {
			APS_ERR("IRQ LINE NOT AVAILABLE!!\n");
			return -EINVAL;
		}
		disable_irq(ltr579_obj->irq);
		APS_LOG("disable_irq\n");
		enable_irq(ltr579_obj->irq);
		APS_LOG("enable_irq\n");
	} else {
		APS_ERR("null irq node!!\n");
		return -EINVAL;
	}

	return 0;
}


/*----------------------------------------------------------------------------*/
static void ltr579_power(struct alsps_hw *hw, unsigned int on) 
{

}


static int ltr579_devinit(void)
{
	int res;

	struct i2c_client *client = ltr579_obj->client;

	APS_ERR("ltr579_devinit\n");
	
	mdelay(PON_DELAY);

	/*added by fully for software reset 20160712*/
	//ltr579_i2c_write_reg(LTR579_MAIN_CTRL, 0x10); 
	//mdelay(WAKEUP_DELAY);

	if((res = ltr579_setup_eint(client))!=0)
	{
		APS_ERR("setup eint: %d\n", res);
		return res;
	}
	
	res = 0;

	APS_ERR("init dev: %d\n", res);
	return res;

}
/*----------------------------------------------------------------------------*/


static int ltr579_get_als_value(struct ltr579_priv *obj, u16 als)
{
	int idx;
	int invalid = 0;
        int level_high;
        int level_low;
        int level_diff;
        int value_high;
        int value_low;
        int value_diff;
        int value;

	APS_DBG("als  = %d\n",als); 
	for(idx = 0; idx < obj->als_level_num; idx++)
	{
		if(als < obj->hw->als_level[idx])
		{
			break;
		}
	}
	
	if(idx >= obj->als_level_num)
	{
		APS_ERR("exceed range\n"); 
		idx = obj->als_level_num - 1;
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

APS_DBG("idx  = %d\n",idx); 

	if(!invalid)
	{
		APS_DBG("ALS: %05d => %05d\n", als, obj->hw->als_value[idx]);	

	        level_high = obj->hw->als_level[idx];
	        level_low = (idx > 0) ? obj->hw->als_level[idx - 1] : 0;
	        level_diff = level_high - level_low;
	        value_high = obj->hw->als_value[idx];
	        value_low = (idx > 0) ? obj->hw->als_value[idx - 1] : 0;
	        value_diff = value_high - value_low;
	        value = 0;

	        if ((level_low >= level_high) || (value_low >= value_high))
	            value = value_low;
	        else
	            value = (level_diff * value_low + (als - level_low) * value_diff + ((level_diff + 1) >> 1)) / level_diff;

	        APS_LOG("ALS: %d [%d, %d] => %d [%d, %d] \n\r", als, level_low, level_high, value, value_low, value_high);
	        return value;
	
	}
	else
	{
		APS_ERR("ALS: %05d => %05d (-1)\n", als, obj->hw->als_value[idx]);    
		return -1;
	}
}
/*----------------------------------------------------------------------------*/

static int ltr579_get_ps_value(void)
{
	int ps_flag;
	int val;
	int error=-1;
	int buffer=0;

	buffer = ltr579_i2c_read_reg(LTR579_MAIN_STATUS);
	APS_DBG("Main status = %d\n", buffer);
	if (buffer < 0){
	    
	    APS_DBG("MAIN status read error\n");
		return error;
	}

	ps_flag = buffer & 0x04;
	ps_flag = ps_flag >>2;
	if(ps_flag==1) //Near
	{
	 	intr_flag_value =1;
		val=0;
	}
	else if(ps_flag ==0) //Far
	{
		intr_flag_value =0;
		val=1;
	}
    APS_DBG("ps_flag = %d, val = %d\n", ps_flag,val);
    
	return val;
}

/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
/*for interrup work mode support */
static void ltr579_eint_work(struct work_struct *work)
{
	struct ltr579_priv *obj = (struct ltr579_priv *)container_of(work, struct ltr579_priv, eint_work);
	int res = 0;
	int err;
	int value = 1;

	//err = ltr579_check_intr(obj->client);
	//if (err < 0) {
	//	goto EXIT_INTR;
	//}
	//else
	{
		//get raw data
		obj->ps = ltr579_ps_read();
		if (obj->ps < 0)
		{
			err = -1;
			goto EXIT_INTR;
		}
				
		APS_DBG("ltr579_eint_work: rawdata ps=%d!\n",obj->ps);
		value = ltr579_get_ps_value();
		
		//let up layer to know
		res = ps_report_interrupt_data(value);
	}

EXIT_INTR:
	//ltr579_clear_intr(obj->client);
#ifdef CONFIG_OF
	enable_irq(obj->irq);
#endif
}



/****************************************************************************** 
 * Function Configuration
******************************************************************************/
static int ltr579_open(struct inode *inode, struct file *file)
{
	file->private_data = ltr579_i2c_client;

	if (!file->private_data)
	{
		APS_ERR("null pointer!!\n");
		return -EINVAL;
	}
	
	return nonseekable_open(inode, file);
}
/*----------------------------------------------------------------------------*/
static int ltr579_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}
/*----------------------------------------------------------------------------*/

//static int offset_value = 0;
static long ltr579_unlocked_ioctl(struct file *file, unsigned int cmd,
       unsigned long arg)       
{
	struct i2c_client *client = (struct i2c_client*)file->private_data;
	struct ltr579_priv *obj = i2c_get_clientdata(client);  
	int err = 0;
	void __user *ptr = (void __user*) arg;
	int dat;
	uint32_t enable;
	//uint32_t cali_value;
	APS_DBG("cmd= %d\n", cmd); 
	switch (cmd)
	{
#if 0
		case PA12200001_ALSPS_IOCTL_SET_CALI:
			if(copy_from_user(&cali_value, ptr, sizeof(cali_value)))
			{
				err = -EFAULT;
				goto err_out;
			}
			offset_value = cali_value;
			break;
#endif
		case ALSPS_SET_PS_MODE:
			if(copy_from_user(&enable, ptr, sizeof(enable)))
			{
				err = -EFAULT;
				goto err_out;
			}
			if(enable)
			{
			    err = ltr579_ps_enable();
				if(err < 0)
				{
					APS_ERR("enable ps fail: %d\n", err); 
					goto err_out;
				}
				set_bit(CMC_BIT_PS, &obj->enable);
			}
			else
			{
			    err = ltr579_ps_disable();
				if(err < 0)
				{
					APS_ERR("disable ps fail: %d\n", err); 
					goto err_out;
				}
				
				clear_bit(CMC_BIT_PS, &obj->enable);
			}
			break;

		case ALSPS_GET_PS_MODE:
			enable = test_bit(CMC_BIT_PS, &obj->enable) ? (1) : (0);
			if(copy_to_user(ptr, &enable, sizeof(enable)))
			{
				err = -EFAULT;
				goto err_out;
			}
			break;

		case ALSPS_GET_PS_DATA:
			APS_DBG("ALSPS_GET_PS_DATA\n"); 
		    obj->ps = ltr579_ps_read();
			if(obj->ps < 0)
			{
				goto err_out;
			}
			
			dat = ltr579_get_ps_value();
			if(copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}  
			break;

		case ALSPS_GET_PS_RAW_DATA:    
			obj->ps = ltr579_ps_read();
			if(obj->ps < 0)
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
			    err = ltr579_als_enable(als_gainrange);
				if(err < 0)
				{
					APS_ERR("enable als fail: %d\n", err); 
					goto err_out;
				}
				set_bit(CMC_BIT_ALS, &obj->enable);
			}
			else
			{
			    err = ltr579_als_disable();
				if(err < 0)
				{
					APS_ERR("disable als fail: %d\n", err); 
					goto err_out;
				}
				clear_bit(CMC_BIT_ALS, &obj->enable);
			}
			break;

		case ALSPS_GET_ALS_MODE:
			enable = test_bit(CMC_BIT_ALS, &obj->enable) ? (1) : (0);
			if(copy_to_user(ptr, &enable, sizeof(enable)))
			{
				err = -EFAULT;
				goto err_out;
			}
			break;

		case ALSPS_GET_ALS_DATA: 
		    obj->als = ltr579_als_read(als_gainrange);
			if(obj->als < 0)
			{
				goto err_out;
			}

			dat = ltr579_get_als_value(obj, obj->als);
			if(copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}              
			break;

		case ALSPS_GET_ALS_RAW_DATA:    
			obj->als = ltr579_als_read(als_gainrange);
			if(obj->als < 0)
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

		default:
			APS_ERR("%s not supported = 0x%04x", __FUNCTION__, cmd);
			err = -ENOIOCTLCMD;
			break;
	}

	err_out:
	return err;    
}

/*----------------------------------------------------------------------------*/
static struct file_operations ltr579_fops = {
	//.owner = THIS_MODULE,
	.open = ltr579_open,
	.release = ltr579_release,
	.unlocked_ioctl = ltr579_unlocked_ioctl,

};
/*----------------------------------------------------------------------------*/
static struct miscdevice ltr579_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "als_ps",
	.fops = &ltr579_fops,
};

static int ltr579_i2c_suspend(struct i2c_client *client, pm_message_t msg) 
{
	//struct ltr579_priv *obj = i2c_get_clientdata(client);    
	//int err;
	APS_FUN();    
#if 0
	if(msg.event == PM_EVENT_SUSPEND)
	{   
		if(!obj)
		{
			APS_ERR("null pointer!!\n");
			return -EINVAL;
		}
		
		atomic_set(&obj->als_suspend, 1);
		err = ltr579_als_disable();
		if(err < 0)
		{
			APS_ERR("disable als: %d\n", err);
			return err;
		}
		atomic_set(&obj->ps_suspend, 1);

		if(test_bit(CMC_BIT_PS,  &obj->enable))
		{
			err = ltr579_ps_disable();
			if(err < 0)
			{
				APS_ERR("disable ps:  %d\n", err);
				return err;
			}
		}
		
		ltr579_power(obj->hw, 0);

	}
#endif
	return 0;
}
/*----------------------------------------------------------------------------*/
static int ltr579_i2c_resume(struct i2c_client *client)
{
	struct ltr579_priv *obj = i2c_get_clientdata(client);        
	//int err;
	APS_FUN();
	if(!obj)
	{
		APS_ERR("null pointer!!\n");
		return -EINVAL;
	}

	ltr579_power(obj->hw, 1);
	
#if 0
/*	err = ltr579_devinit();
	if(err < 0)
	{
		APS_ERR("initialize client fail!!\n");
		return err;        
	}*/
	atomic_set(&obj->als_suspend, 0);
	if(test_bit(CMC_BIT_ALS, &obj->enable))
	{
	    err = ltr579_als_enable(als_gainrange);
	    if (err < 0)
		{
			APS_ERR("enable als fail: %d\n", err);        
		}
	}

	atomic_set(&obj->ps_suspend, 0);
	if(test_bit(CMC_BIT_PS,  &obj->enable))
	{
		err = ltr579_ps_enable();
	    if (err < 0)
		{
			APS_ERR("enable ps fail: %d\n", err);                
		}
	}
#endif
	return 0;
}
#ifdef CONFIG_HAS_EARLYSUSPEND
static void ltr579_early_suspend(struct early_suspend *h) 
{   /*early_suspend is only applied for ALS*/
	struct ltr579_priv *obj = container_of(h, struct ltr579_priv, early_drv);   
	int err;
	APS_FUN();    

	if(!obj)
	{
		APS_ERR("null pointer!!\n");
		return;
	}
	
	atomic_set(&obj->als_suspend, 1); 
	err = ltr579_als_disable();
	if(err < 0)
	{
		APS_ERR("disable als fail: %d\n", err); 
	}
}

static void ltr579_late_resume(struct early_suspend *h)
{   /*early_suspend is only applied for ALS*/
	struct ltr579_priv *obj = container_of(h, struct ltr579_priv, early_drv);         
	int err;
	APS_FUN();

	if(!obj)
	{
		APS_ERR("null pointer!!\n");
		return;
	}

	atomic_set(&obj->als_suspend, 0);
	if(test_bit(CMC_BIT_ALS, &obj->enable))
	{
	    err = ltr579_als_enable(als_gainrange);
		if(err < 0)
		{
			APS_ERR("enable als fail: %d\n", err);        

		}
	}
}
#endif
int ltr579_ps_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	struct hwm_sensor_data* sensor_data;
	struct ltr579_priv *obj = (struct ltr579_priv *)self;
	
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
				    err = ltr579_ps_enable();
					if(err < 0)
					{
						APS_ERR("enable ps fail: %d\n", err); 
						return -1;
					}
					set_bit(CMC_BIT_PS, &obj->enable);
				}
				else
				{
				    err = ltr579_ps_disable();
					if(err < 0)
					{
						APS_ERR("disable ps fail: %d\n", err); 
						return -1;
					}
					clear_bit(CMC_BIT_PS, &obj->enable);
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
				APS_ERR("get sensor ps data !\n");
				sensor_data = (struct hwm_sensor_data *)buff_out;
				value = ltr579_get_ps_value();
				obj->ps = ltr579_ps_read();
    			if(obj->ps < 0)
    			{
    				err = -1;
    				break;
    			}
				sensor_data->values[0] = value;
				sensor_data->values[1] = ltr579_ps_read();		
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

int ltr579_als_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	struct hwm_sensor_data* sensor_data;
	struct ltr579_priv *obj = (struct ltr579_priv *)self;

 printk("ltr579_als_operate: cmd=%d\n",command);
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
				printk("ltr579_als_operate: value=%d\n",value);
				if(value)
				{
				    err = ltr579_als_enable(als_gainrange);
					if(err < 0)
					{
						APS_ERR("enable als fail: %d\n", err); 
						return -1;
					}
					set_bit(CMC_BIT_ALS, &obj->enable);
				}
				else
				{
				    err = ltr579_als_disable();
					if(err < 0)
					{
						APS_ERR("disable als fail: %d\n", err); 
						return -1;
					}
					clear_bit(CMC_BIT_ALS, &obj->enable);
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
				APS_ERR("get sensor als data !\n");
				sensor_data = (struct hwm_sensor_data *)buff_out;
				obj->als = ltr579_als_read(als_gainrange);
                #if defined(MTK_AAL_SUPPORT)
				sensor_data->values[0] = obj->als;
				#else
				sensor_data->values[0] = ltr579_get_als_value(obj, obj->als);
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


static int als_open_report_data(int open)
{
	APS_LOG("als_open_report_data!!\n");
	return 0;
}

static int als_enable_nodata(int en)
{
	int res = 0;
#ifdef CUSTOM_KERNEL_SENSORHUB
    SCP_SENSOR_HUB_DATA req;
    int len;
#endif //#ifdef CUSTOM_KERNEL_SENSORHUB

    APS_LOG("ltr579_obj als enable value = %d\n", en);

#ifdef CUSTOM_KERNEL_SENSORHUB
    req.activate_req.sensorType = ID_LIGHT;
    req.activate_req.action = SENSOR_HUB_ACTIVATE;
    req.activate_req.enable = en;
    len = sizeof(req.activate_req);
    res = SCP_sensorHub_req_send(&req, &len, 1);
#else //#ifdef CUSTOM_KERNEL_SENSORHUB
	if(!ltr579_obj)
	{
		APS_ERR("ltr579_obj is null!!\n");
		return -1;
	}
	APS_LOG("ltr579_obj als enable value = %d\n", en);

/*	if(intr_flag_value == 1)
	{
		APS_ERR("near, skip als enable (%d)\n", en);
		return -1;
	}*/

	if(en)
	{
		if((res = ltr579_als_enable(als_gainrange)))
		{
			APS_ERR("enable als fail: %d\n", res); 
			return -1;
		}
		set_bit(CMC_BIT_ALS, &ltr579_obj->enable);
	}
	else
	{
		if((res = ltr579_als_disable()))
		{
			APS_ERR("disable als fail: %d\n", res); 
			return -1;
		}
		clear_bit(CMC_BIT_ALS, &ltr579_obj->enable);
	}
 #endif
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

static int als_get_data(int* value, int* status)
{
	int err = 0;
#ifdef CUSTOM_KERNEL_SENSORHUB
    SCP_SENSOR_HUB_DATA req;
    int len;
#else
    struct ltr579_priv *obj = NULL;
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

    if(atomic_read(&ltr579_obj->trace) & CMC_TRC_PS_DATA)
	{
        APS_LOG("value = %d\n", *value);
        //show data
	}
#else //#ifdef CUSTOM_KERNEL_SENSORHUB
	if(!ltr579_obj)
	{
		APS_ERR("ltr579_obj is null!!\n");
		return -1;
	}
	obj = ltr579_obj;
	obj->als = ltr579_als_read(als_gainrange);
	if( obj->als < 0)
	{
		err = -1;
	}
	else
	{
		*value = ltr579_get_als_value(obj, obj->als);
		*status = SENSOR_STATUS_ACCURACY_MEDIUM;
	}
#endif //#ifdef CUSTOM_KERNEL_SENSORHUB

	return err;
}

static int ps_open_report_data(int open)
{
	APS_LOG("ps_open_report_data!!\n");
	return 0;
}

// if use  this typ of enable , Gsensor only enabled but not report inputEvent to HAL

static int ps_enable_nodata(int en)
{
	int res = 0;
#ifdef CUSTOM_KERNEL_SENSORHUB
    SCP_SENSOR_HUB_DATA req;
    int len;
#endif //#ifdef CUSTOM_KERNEL_SENSORHUB

    APS_LOG("ltr579_obj als enable value = %d\n", en);

#ifdef CUSTOM_KERNEL_SENSORHUB
    req.activate_req.sensorType = ID_PROXIMITY;
    req.activate_req.action = SENSOR_HUB_ACTIVATE;
    req.activate_req.enable = en;
    len = sizeof(req.activate_req);
    res = SCP_sensorHub_req_send(&req, &len, 1);
#else //#ifdef CUSTOM_KERNEL_SENSORHUB
	if(!ltr579_obj)
	{
		APS_ERR("ltr579_obj is null!!\n");
		return -1;
	}
	APS_LOG("ltr579_obj ps  enable value = %d\n", en);

    if(en)
	{
		if((res = ltr579_ps_enable()))
		{
			APS_ERR("enable ps fail: %d\n", res); 
			return -1;
		}
		set_bit(CMC_BIT_PS, &ltr579_obj->enable);
	}
	else
	{
		if((res = ltr579_ps_disable()))
		{
			APS_ERR("disable ps fail: %d\n", res); 
			return -1;
		}
		clear_bit(CMC_BIT_PS, &ltr579_obj->enable);
	}
#endif
	if(res){
		APS_ERR("als_enable_nodata is failed!!\n");
		return -1;
	}
	return 0;

}

static int ps_set_delay(u64 ns)
{
	APS_LOG("ps_set_delay!!\n");
	return 0;
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

    if(atomic_read(&ltr579_obj->trace) & CMC_TRC_PS_DATA)
	{
        APS_LOG("value = %d\n", *value);
        //show data
	}
#else //#ifdef CUSTOM_KERNEL_SENSORHUB
    if(!ltr579_obj)
	{
		APS_ERR("ltr579_obj is null!!\n");
		return -1;
	}
    ltr579_obj->ps = ltr579_ps_read();
    if(ltr579_obj->ps < 0)
    {
        err = -1;;
    }
    else
    {
        *value = ltr579_get_ps_value();
        *status = SENSOR_STATUS_ACCURACY_MEDIUM;
    }
#endif //#ifdef CUSTOM_KERNEL_SENSORHUB
    
	return 0;
}
/*----------------------------------------------------------------------------*/
static int ltr579_i2c_detect(struct i2c_client *client, struct i2c_board_info *info) 
{    
	APS_ERR("ltr579_i2c_detect\n");
	strcpy(info->type, LTR579_DEV_NAME);
	return 0;
}
/*----------------------------------------------------------------------------*/
static int ltr579_check_DeviceID(struct i2c_client *client)
{
	int res = 0;

		APS_LOG("ltr579_check_DeviceID\n");

		res = ltr579_i2c_read_reg(LTR579_PART_ID);

		if(res != 0xB1)
		{
			APS_LOG("ltr579_check_DeviceID main status reg is not default value 0x%x!!!!\n", res );
			goto EXIT_ERR;
		}
		else 
		{
			res = 0;
		}
	
		return res;
	
	EXIT_ERR:
		APS_ERR("ltr579_check_DeviceID fail\n");
		return -1;
}
/*----------------------------------------------------------------------------*/
static int ltr579_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ltr579_priv *obj;

	struct als_control_path als_ctl={0};
	struct als_data_path als_data={0};
	struct ps_control_path ps_ctl={0};
	struct ps_data_path ps_data={0};
	
	//struct hwmsen_object obj_ps, obj_als;
	int err = 0;
	int retry = 0;

	APS_FUN();
	
	if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
	{
		err = -ENOMEM;
		goto exit;
	}
	memset(obj, 0, sizeof(*obj));
	ltr579_obj = obj;

	obj->hw = hw;
	ltr579_get_addr(obj->hw, &obj->addr);

	INIT_WORK(&obj->eint_work, ltr579_eint_work);
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
	//atomic_set(&obj->als_cmd_val, 0xDF);
	//atomic_set(&obj->ps_cmd_val,  0xC1);
	atomic_set(&obj->ps_thd_val,  obj->hw->ps_threshold);
	obj->enable = 0;
	obj->pending_intr = 0;
	obj->als_level_num = sizeof(obj->hw->als_level)/sizeof(obj->hw->als_level[0]);
	obj->als_value_num = sizeof(obj->hw->als_value)/sizeof(obj->hw->als_value[0]);   
	obj->als_modulus = (400*100)/(16*150);//(1/Gain)*(400/Tine), this value is fix after init ATIME and CONTROL register value
										//(400)/16*2.72 here is amplify *100
	obj->irq_node = of_find_compatible_node(NULL, NULL, "mediatek, als-eint");
	obj->enable = 0;
	BUG_ON(sizeof(obj->als_level) != sizeof(obj->hw->als_level));
	memcpy(obj->als_level, obj->hw->als_level, sizeof(obj->als_level));
	BUG_ON(sizeof(obj->als_value) != sizeof(obj->hw->als_value));
	memcpy(obj->als_value, obj->hw->als_value, sizeof(obj->als_value));
	atomic_set(&obj->i2c_retry, 3);
	//set_bit(CMC_BIT_ALS, &obj->enable);
	//set_bit(CMC_BIT_PS, &obj->enable);

	APS_LOG("ltr579_devinit() start...!\n");

	
	ltr579_i2c_client = client;

	for (retry = 0; retry < 3; retry++) {
		err = ltr579_check_DeviceID(ltr579_i2c_client);
		if(err != 0)
		{
			APS_LOG("ltr579_i2c_probe() check ltr579 device ID fail and will retry %d times!\n", (3-retry));
			continue ;
		}
		else
		{
			APS_LOG("ltr579_i2c_probe() check ltr579 device ID OK!!!!!!!!!!!!\n");
			break;
		}
	}
	if(err)
	{
		APS_LOG("ltr579_i2c_probe() check ltr579 device ID fail finally, So we do not register ctl path!!!!!!!!!!!!\n");
		goto exit_init_failed;
	}


	if((err = (ltr579_devinit())))
	{
		goto exit_init_failed;
	}
	APS_LOG("ltr579_devinit() ...OK!\n");

	//printk("@@@@@@ Part ID:%x\n",ltr579_i2c_read_reg(0x06));

	if((err = (misc_register(&ltr579_device))))
	{
		APS_ERR("ltr579_device register failed\n");
		goto exit_misc_device_register_failed;
	}
    als_ctl.is_use_common_factory =false;
	ps_ctl.is_use_common_factory = false;

	
	/* Register sysfs attribute */
	if((err = (ltr579_create_attr(&(ltr579_init_info.platform_diver_addr->driver)))))
	{
		printk(KERN_ERR "create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}

	als_ctl.open_report_data= als_open_report_data;
	als_ctl.enable_nodata = als_enable_nodata;
	als_ctl.set_delay  = als_set_delay;
	als_ctl.is_report_input_direct = false;
#ifdef CUSTOM_KERNEL_SENSORHUB
	als_ctl.is_support_batch = true;
#else
    als_ctl.is_support_batch = false;
#endif
	
	err = als_register_control_path(&als_ctl);
	APS_LOG("register als control path:%d ...OK!\n", err);
	if (err) {
		APS_ERR("register fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}

	als_data.get_data = als_get_data;
	als_data.vender_div = 100;
	err = als_register_data_path(&als_data);
	APS_LOG("register als data path:%d ...OK!\n", err);
	if (err) {
		APS_ERR("tregister fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}

	
	ps_ctl.open_report_data= ps_open_report_data;
	ps_ctl.enable_nodata = ps_enable_nodata;
	ps_ctl.set_delay  = ps_set_delay;
	ps_ctl.is_report_input_direct = true;
#ifdef CUSTOM_KERNEL_SENSORHUB
	ps_ctl.is_support_batch = true;
#else
		ps_ctl.is_support_batch = false;
#endif
		
	err = ps_register_control_path(&ps_ctl);
	APS_LOG("register ps control path:%d ...OK!\n", err);
	if (err) {
		APS_ERR("register fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}

	ps_data.get_data = ps_get_data;
	ps_data.vender_div = 100;
	err = ps_register_data_path(&ps_data);
	APS_LOG("register ps data path:%d ...OK!\n", err);
	if (err) {
		APS_ERR("tregister fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}
	
		err = batch_register_support_info(ID_LIGHT,als_ctl.is_support_batch, 100, 0);
		if(err)
		{
			APS_ERR("register light batch support err = %d\n", err);
		}
		
		err = batch_register_support_info(ID_PROXIMITY,ps_ctl.is_support_batch, 100, 0);
		if(err)
		{
			APS_ERR("register proximity batch support err = %d\n", err);
		}

//add by wangxiqiang
   #ifdef CONFIG_SLT_DRV_DEVINFO_SUPPORT
   
	temp_comments=(char*) kmalloc(20, GFP_KERNEL);
	sprintf(temp_comments,"ALPS:LTR579 BOE"); 
	devinfo_ctp_regchar("unknown","BOE","1.0",DEVINFO_USED,temp_comments);

   #endif
   //end of add

#if defined(CONFIG_HAS_EARLYSUSPEND)
	obj->early_drv.level    = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1,
	obj->early_drv.suspend  = ltr579_early_suspend,
	obj->early_drv.resume   = ltr579_late_resume,    
	register_early_suspend(&obj->early_drv);
#endif

	APS_LOG("%s: OK\n", __func__);
	return 0;

	exit_sensor_obj_attach_fail:
	exit_create_attr_failed:
	misc_deregister(&ltr579_device);
	exit_misc_device_register_failed:
	exit_init_failed:
	//i2c_detach_client(client);
	//exit_kfree:
	kfree(obj);
	exit:
	ltr579_i2c_client = NULL;           
//	MT6516_EINTIRQMask(CUST_EINT_ALS_NUM);  /*mask interrupt if fail*/
	APS_ERR("%s: err = %d\n", __func__, err);
	return err;
}

/*----------------------------------------------------------------------------*/

static int ltr579_i2c_remove(struct i2c_client *client)
{
	int err;	
	APS_ERR("ltr579_i2c_remove\n");
	if((err = (ltr579_delete_attr(&ltr579_i2c_driver.driver))))
	{
		APS_ERR("ltr579_delete_attr fail: %d\n", err);
	} 

	if((err = (misc_deregister(&ltr579_device))))
	{
		APS_ERR("misc_deregister fail: %d\n", err);    
	}
	
	ltr579_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));

	return 0;
}

/*-----------------------------------------------------------------------------*/
static int ltr579_local_init(void)
{
	APS_FUN();
	ltr579_power(hw, 1);
	if(i2c_add_driver(&ltr579_i2c_driver))
	{
		APS_ERR("add driver error\n");
		return -1;
	}
	return 0;
}
/*-----------------------------------------------------------------------------*/
static int ltr579_local_uninit(void)
{
	APS_FUN();	  
	ltr579_power(hw, 0);	
	i2c_del_driver(&ltr579_i2c_driver);
	return 0;
}


/*----------------------------------------------------------------------------*/



/*----------------------------------------------------------------------------*/
static int __init ltr579_init(void)
{
	const char *name = "mediatek,ltr579-new";

	APS_FUN();
	hw = get_alsps_dts_func(name, hw);
	if (!hw) {
		APS_ERR("get dts info fail\n");
		return -1;
	}
	
	alsps_driver_add(&ltr579_init_info);// hwmsen_alsps_add(&ltr579_init_info);
	return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit ltr579_exit(void)
{
	APS_FUN();
}
/*----------------------------------------------------------------------------*/
module_init(ltr579_init);
module_exit(ltr579_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("XX Xx");
MODULE_DESCRIPTION("LTR-579ALS Driver");
MODULE_LICENSE("GPL");
/* Driver version v1.0 */

