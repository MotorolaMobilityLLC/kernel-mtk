//tuwenzan@wind-mobi.com modify at 20170331 begin
/* 
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

#include "cust_alsps.h"
#include "ltr778.h"
#include "alsps.h"

#define GN_MTK_BSP_PS_DYNAMIC_CALI
//#define DEMO_BOARD

/******************************************************************************
 * configuration
*******************************************************************************/
/*----------------------------------------------------------------------------*/
#define LTR778_DEV_NAME			"ltr778"

/*----------------------------------------------------------------------------*/
#define APS_TAG					"[ALS/PS] "
#define APS_FUN(f)              printk(KERN_INFO 	APS_TAG"%s\n", __FUNCTION__)
#define APS_ERR(fmt, args...)   printk(KERN_ERR  	APS_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
//liujinzhou@wind-mobi.com modify at 20161205 begin
#define APS_LOG(fmt, args...)   printk(KERN_ERR		APS_TAG fmt, ##args)
//liujinzhou@wind-mobi.com modify at 20161205 end
#define APS_DBG(fmt, args...)   printk(KERN_ERR 	APS_TAG fmt, ##args)

/*----------------------------------------------------------------------------*/
static const struct i2c_device_id ltr778_i2c_id[] = {{LTR778_DEV_NAME,0},{}};
static unsigned long long int_top_time;
struct alsps_hw alsps_cust;
static struct alsps_hw *hw = &alsps_cust;
struct platform_device *alspsPltFmDev;

/*----------------------------------------------------------------------------*/
static int ltr778_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id); 
static int ltr778_i2c_remove(struct i2c_client *client);
static int ltr778_i2c_detect(struct i2c_client *client, struct i2c_board_info *info);
static int ltr778_i2c_suspend(struct i2c_client *client, pm_message_t msg);
static int ltr778_i2c_resume(struct i2c_client *client);
//tuwenzan@wind-mobi.com add at 20170424 begin
static int ltr778_als_enable(struct i2c_client *client, int enable);
static int ltr778_als_read(struct i2c_client *client, u16* data);
//tuwenzan@wind-mobi.com add at 20170424 end
static int	als_integration_factor;
static int	als_gain_factor;

static int final_prox_val;
static int final_lux_val;
//liujinzhou@wind-mobi.com add at 20161205 begin
static int winfac_1;  // modified by steven
static int winfac_2;
//liujinzhou@wind-mobi.com add at 20161205 end
//tuwenzan@wind-mobi.com add at 20170313 begin
#ifdef CONFIG_MOTO_AOD_BASE_ON_AP_SENSORS
static bool ps_stowed_start = false;
#endif
//tuwenzan@wind-mobi.com add at 20170313 end
/*----------------------------------------------------------------------------*/
typedef enum {
    CMC_BIT_ALS    = 1,
    CMC_BIT_PS     = 2,
} CMC_BIT;

/*----------------------------------------------------------------------------*/
struct ltr778_priv {
	struct alsps_hw  *hw;
	struct i2c_client *client;
	struct work_struct	eint_work;

	/*misc*/
	u16 		als_modulus;
	atomic_t	i2c_retry;
	atomic_t	als_suspend;
	atomic_t	als_debounce;	/*debounce time after enabling als*/
	atomic_t	als_deb_on; 	/*indicates if the debounce is on*/
	atomic_t	als_deb_end;	/*the jiffies representing the end of debounce*/
	atomic_t	ps_mask;		/*mask ps: always return far away*/
	atomic_t	ps_debounce;	/*debounce time after enabling ps*/
	atomic_t	ps_deb_on;		/*indicates if the debounce is on*/
	atomic_t	ps_deb_end; 	/*the jiffies representing the end of debounce*/
	atomic_t	ps_suspend;
	atomic_t 	trace;

#ifdef CONFIG_OF
	struct device_node *irq_node;
	int		irq;
#endif

	/*data*/
	u16			als;
	u16 		ps;
	u8			_align;
	u16			als_level_num;
	u16			als_value_num;
	u32			als_level[C_CUST_ALS_LEVEL-1];
	u32			als_value[C_CUST_ALS_LEVEL];
	int			ps_cali;
	
	atomic_t	als_cmd_val;	/*the cmd value can't be read, stored in ram*/
	atomic_t	ps_cmd_val; 	/*the cmd value can't be read, stored in ram*/
	atomic_t	ps_thd_val_high;	 /*the cmd value can't be read, stored in ram*/
	atomic_t	ps_thd_val_low; 	/*the cmd value can't be read, stored in ram*/
	//liujinzhou@wind-mobi.com add at 20161205 begin
	atomic_t	ps_persist_val_high;
	atomic_t	ps_persist_val_low;
	//liujinzhou@wind-mobi.com add at 20161205 end
	atomic_t	ps_thd_val_high_stowed;
	atomic_t	ps_thd_val_low_stowed;
	atomic_t	als_thd_val_high;	 /*the cmd value can't be read, stored in ram*/
	atomic_t	als_thd_val_low; 	/*the cmd value can't be read, stored in ram*/
	atomic_t	ps_thd_val;
	ulong		enable; 		/*enable mask*/
	ulong		pending_intr;	/*pending interrupt*/	

	int			fault_detect_level;
};

static int ltr778_i2c_read_reg(u8 regnum);
static int ltr778_i2c_write_reg(u8 regnum, u8 value);
static void ltr778_enable_ps_delay_work(u8 enable);

struct PS_CALI_DATA_STRUCT
{
    int close;
    int far_away;
    int valid;
} ;

static struct PS_CALI_DATA_STRUCT ps_cali={0,0,0};
static int intr_flag_value = 0;

static struct ltr778_priv *ltr778_obj = NULL;
static struct i2c_client *ltr778_i2c_client = NULL;

static DEFINE_MUTEX(ltr778_mutex);

static int ltr778_local_init(void);
static int ltr778_remove(void);


#define MAX_ELM_als 6
static unsigned int record_als[MAX_ELM_als];
static int rct_als=0,full_als=0;
static int lux_calc=0;


/*----------------modified by hongguang for avglux function-----------------------*/		
static int get_stable_lux(unsigned int lux)
{
	int lux_stalbe;
	if(rct_als >= MAX_ELM_als)
		full_als=1;

	if(full_als){
		rct_als %= MAX_ELM_als;

		if(lux == record_als[rct_als]){
			lux_calc ++;
		}else{
			lux_calc = 0;
		}		
	}else{
		if(rct_als>=1){
			if(lux == record_als[rct_als-1]){
				lux_calc ++;
			}else{
				lux_calc = 0;
			}
		}
	}
		
	record_als[rct_als]=lux;
	//APS_DBG("ltr778_get_stable_lux rct = %d ,record[rct] = %d, lux_calc=%d, full= %d \n",rct_als,record_als[rct_als],lux_calc,full_als);
	rct_als++;
	if(lux_calc >= MAX_ELM_als ){
		lux_stalbe = 1;
	}else{
		lux_stalbe = 0;
	}
	
	return lux_stalbe;
}




static int ltr778_init_flag = -1;
#define MAX_ELM_ALS 3
static unsigned int record_ALS[MAX_ELM_ALS];
static int rct_ALS=0,full_ALS=0;
static long lux_sum_ALS=0;
static u8 ps_en_flag_1 = 0;
static u8 ps_en_flag = 0;




/*----------------modified by hongguang for avglux function-----------------------*/		
static int get_avg_lux(unsigned int lux)
{
	int lux_a;

	
	int als_data_s;

	int err;
	int regdata;

	
	if(rct_ALS >= MAX_ELM_ALS)
		full_ALS=1;

	if(full_ALS){
		rct_ALS %= MAX_ELM_ALS;
		lux_sum_ALS -= record_ALS[rct_ALS];
	}
	lux_sum_ALS += lux;
	record_ALS[rct_ALS]=lux;
	rct_ALS++;
	
	als_data_s = get_stable_lux(lux);

	if(als_data_s == 1){
		regdata = ltr778_i2c_read_reg(LTR778_PS_CONTR);
		regdata |= 0x02;
		err = ltr778_i2c_write_reg(LTR778_PS_CONTR, regdata);
		if (err < 0)
		{
			APS_ERR("PS: enable ps err: %d  \n", err);
		}
		
		ps_en_flag_1 = 1;

	}else{

		if(ps_en_flag == 0 && ps_en_flag_1 == 1){
			regdata = ltr778_i2c_read_reg(LTR778_PS_CONTR);
			regdata &= 0xfd;
			
			err = ltr778_i2c_write_reg(LTR778_PS_CONTR, regdata);
			if (err < 0)
			{
				APS_ERR("PS: enable ps err: %d	\n", err);
			}

			ps_en_flag_1 = 0;
		}

	}
	//APS_DBG("ltr778_als_read als_data_s = %d\n", als_data_s);
	if(full_ALS){
	lux_a = lux_sum_ALS / MAX_ELM_ALS;
	}else{
	lux_a = lux_sum_ALS /rct_ALS;
	}
	return lux_a;
}



//tuwenzan@wind-mobi.com add for solve oil bug at 20170307 begin
#define MAX_ELM_PS 2 	//tuwenzan@wind-mobi.com modify at 20170424
static unsigned int record_ps[MAX_ELM_PS];
static int rct_ps=0,full_ps=0;
static long ps_sum=0;

static int als_enable_flag = 0;
static int ps_enable_als = 0;
static int ps_stowed_enf = 0;


static int get_avg_ps(unsigned int ps_data_c)
{
	int ps_d;
	if(rct_ps >= MAX_ELM_PS)
		full_ps=1;

	if(full_ps){
		rct_ps %= MAX_ELM_PS;
		ps_sum -= record_ps[rct_ps];
	}
	ps_sum += ps_data_c;
	record_ps[rct_ps]=ps_data_c;
	rct_ps++;

	if(full_ps){

		ps_d = ps_sum / MAX_ELM_PS;
			
	}else{	
	ps_d = ps_sum / rct_ps;
		}

	if(rct_ps == MAX_ELM_PS ){
		return ps_d;
	}else{
		return 2048;
	}
}

#define PS_NUM 8
static unsigned int ps_data[PS_NUM];
static unsigned int ps_rct_n = 0,ps_cal_full = 0, j_ps = 0;

static int get_stable_ps(unsigned int ps_count)
{
	int ps_avg = 0;
	int ps_avg_h;
	int ps_avg_l;
	int i;
	long ps_data_sum = 0;
	
	if(ps_rct_n >= PS_NUM)
		ps_cal_full=1;

	if(ps_cal_full){
		ps_rct_n %= PS_NUM;
	}	

	ps_data[ps_rct_n]=ps_count;

	
	if(ps_cal_full){
		
		for(i=0;i<PS_NUM;i++){
			APS_LOG("LTR778 %s:ps_data[%d] = %d \n", __func__, i,ps_data[i]);
			ps_data_sum += ps_data[i];
		}
		
		ps_avg = ps_data_sum / PS_NUM;
		
		ps_avg_h = ps_avg + 20; // 25->20
		
		ps_avg_l = ps_avg - 20;
		
		for(i=0;i<PS_NUM;i++)
		{
			if(ps_data[i]< ps_avg_h  && ps_data[i]>ps_avg_l )
				j_ps++;
			else 
				j_ps = 0;
		}
		
		
	}else{
		j_ps = 0;
	}
	
	ps_rct_n++;

	
	if(j_ps >= PS_NUM-1){
		return 1;
	}else{
		return 0;
	}


}


static int clear_stable(void)
{
		ps_rct_n = 0;
		ps_cal_full = 0;
		j_ps = 0;
		
		APS_LOG("LTR778 %s: j_ps %d \n", __func__,j_ps );

		return 0;		
}

//tuwenzan@wind-mobi.com add for solve oil bug at 20170307 end

static struct alsps_init_info ltr778_init_info = {
		.name = "ltr778",
		.init = ltr778_local_init,
		.uninit = ltr778_remove,
};

#ifdef CONFIG_OF
static const struct of_device_id alsps_of_match[] = {
	{.compatible = "mediatek,driver-ltr778"},
	{},
};
#endif

static struct i2c_driver ltr778_i2c_driver = {	
	.probe      = ltr778_i2c_probe,
	.remove     = ltr778_i2c_remove,
	.detect     = ltr778_i2c_detect,
	.suspend    = ltr778_i2c_suspend,
	.resume     = ltr778_i2c_resume,
	.id_table   = ltr778_i2c_id,
	.driver = {
		.name           = LTR778_DEV_NAME,
#ifdef CONFIG_OF
		.of_match_table = alsps_of_match,
#endif
	},
};

/*----------------------------------------------------------------------------*/
#ifdef GN_MTK_BSP_PS_DYNAMIC_CALI
static int ltr778_dynamic_calibrate(void);
//liujinzhou@wind-mobi.com modify at 20161205 begin
static int dynamic_calibrate = 2047;
//liujinzhou@wind-mobi.com modify at 20161205 end
#endif
//tuwenzan@wind-mobi.com add for solve oil bug at 20170307 begin
#define MAX_ELM 3
static unsigned int record[MAX_ELM];
static int rct=0,full=0;
static long lux_sum=0;


/*----------------modified by hongguang for avgps function-----------------------*/		
static int get_ps_avg(unsigned int lux)
{
	int lux_a;
	if(rct >= MAX_ELM)
		full=1;

	if(full){
		rct %= MAX_ELM;
		lux_sum -= record[rct];
	}
	lux_sum += lux;
	record[rct]=lux;
	rct++;
	if(full){
	lux_a = lux_sum / MAX_ELM;
	}else{
	lux_a = lux_sum /rct;
	}
	return lux_a;
}
//tuwenzan@wind-mobi.com add for solve oil bug at 20170307 end
/*-----------------------------------------------------------------------------*/

/* 
 * #########
 * ## I2C ##
 * #########
 */

// I2C Read
static int ltr778_i2c_read_reg(u8 regnum)
{
    u8 buffer[1],reg_value[1];
	int res = 0;

	mutex_lock(&ltr778_mutex);
	buffer[0]= regnum;
	res = i2c_master_send(ltr778_obj->client, buffer, 0x1);
	if(res <= 0)
	{	   
	   APS_ERR("read reg send res = %d\n",res);
	   //liujinzhou@wind-mobi.com add at 20161205 begin
		mutex_unlock(&ltr778_mutex);
	   //liujinzhou@wind-mobi.com add at 20161205 end
		return res;
	}
	res = i2c_master_recv(ltr778_obj->client, reg_value, 0x1);
	if(res <= 0)
	{
		APS_ERR("read reg recv res = %d\n",res);
	   //liujinzhou@wind-mobi.com add at 20161205 begin
		mutex_unlock(&ltr778_mutex);
	   //liujinzhou@wind-mobi.com add at 20161205 end
		return res;
	}
	mutex_unlock(&ltr778_mutex);
	return reg_value[0];
}

// I2C Write
static int ltr778_i2c_write_reg(u8 regnum, u8 value)
{
	u8 databuf[2];    
	int res = 0;
   
	mutex_lock(&ltr778_mutex);
	databuf[0] = regnum;   
	databuf[1] = value;
	res = i2c_master_send(ltr778_obj->client, databuf, 0x2);
	mutex_unlock(&ltr778_mutex);

	if (res < 0)
	{
		APS_ERR("wirte reg send res = %d\n",res);
	   	return res;
	}		
	else
		return 0;
}

/*----------------------------------------------------------------------------*/
static void ltr778_power(struct alsps_hw *hw, unsigned int on) 
{
#ifdef DEMO_BOARD
	static unsigned int power_on = 0;	

	if(hw->power_id != POWER_NONE_MACRO)
	{
		if(power_on == on)
		{
			APS_LOG("ignore power control: %d\n", on);
		}
		else if(on)
		{
			if(!hwPowerOn(hw->power_id, hw->power_vol, "ltr778")) 
			{
				APS_ERR("power on fails!!\n");
			}
		}
		else
		{
			if(!hwPowerDown(hw->power_id, "ltr778")) 
			{
				APS_ERR("power off fail!!\n");   
			}
		}
	}
	power_on = on;
#endif
}

/********************************************************************/
/* 
 * ###############
 * ## PS CONFIG ##
 * ###############

 */
static int ltr778_ps_set_thres(void)
{
	int res;
	u8 databuf[2];
	
	struct i2c_client *client = ltr778_obj->client;
	struct ltr778_priv *obj = ltr778_obj;

	APS_FUN();

	APS_DBG("ps_cali.valid: %d\n", ps_cali.valid);

	if(1 == ps_cali.valid)
	{
		databuf[0] = LTR778_PS_THRES_LOW_0; 
		databuf[1] = (u8)(ps_cali.far_away & 0x00FF);
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{			
			goto EXIT_ERR;
		}
		databuf[0] = LTR778_PS_THRES_LOW_1; 
		databuf[1] = (u8)((ps_cali.far_away & 0xFF00) >> 8);
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
		}
		databuf[0] = LTR778_PS_THRES_UP_0;	
		databuf[1] = (u8)(ps_cali.close & 0x00FF);
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
		}
		databuf[0] = LTR778_PS_THRES_UP_1;	
		databuf[1] = (u8)((ps_cali.close & 0xFF00) >> 8);
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
		}
	}
	else
	{
		databuf[0] = LTR778_PS_THRES_LOW_0; 
		databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_low)) & 0x00FF);
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
		}
		databuf[0] = LTR778_PS_THRES_LOW_1; 
		databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_low )>> 8) & 0x00FF);
		
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
		}
		databuf[0] = LTR778_PS_THRES_UP_0;	
		databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_high)) & 0x00FF);
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
		}
		databuf[0] = LTR778_PS_THRES_UP_1;	
		databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_high) >> 8) & 0x00FF);
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
		}	
	}

	res = 0;
	return res;
	
EXIT_ERR:
	APS_ERR("set thres: %d\n", res);
	res = LTR778_ERR_I2C;
	return res;
}
//tuwenzan@wind-mobi.com add at 20170313 begin
#ifdef CONFIG_MOTO_AOD_BASE_ON_AP_SENSORS

static int ltr778_ps_stowed_enable(struct i2c_client *client, int enable)
{
	u8 regdata;
	int err;
	int res = 0;
	struct ltr778_priv *obj = ltr778_obj;
	

	APS_LOG("ltr778_ps_stowed_enable(%d) ...start!\n",enable);

	// modified by steven
	res = ltr778_i2c_write_reg( LTR778_PS_THRES_LOW_0, 0x00);
	if(res < 0)
	{
		APS_ERR("PS: enable ps res: %d en: %d \n", res, enable);
		return res;
	}

	res = ltr778_i2c_write_reg( LTR778_PS_THRES_LOW_1, 0x00);
	if(res < 0)
	{
		APS_ERR("PS: enable ps res: %d en: %d \n", res, enable);
		return res;
	}

	res = ltr778_i2c_write_reg( LTR778_PS_THRES_UP_0, 0xFF);
	if(res < 0)
	{
		APS_ERR("PS: enable ps res: %d en: %d \n", res, enable);
		return res;
	}

	res = ltr778_i2c_write_reg( LTR778_PS_THRES_UP_1, 0x07);
	if(res < 0)
	{
		APS_ERR("PS: enable ps res: %d en: %d \n", res, enable);
		return res;
	}

	ltr778_i2c_write_reg(LTR778_PS_MEAS_RATE, 0x05);// 200ms measurement time
	ltr778_i2c_write_reg(LTR778_INTERRUPT_PRST, 0xE0);// 15 persist

	regdata = ltr778_i2c_read_reg(LTR778_PS_CONTR);
	if (enable != 0) {
		APS_LOG("PS: stowed enable ps only \n");
		regdata = 0xC2; //c2
		ps_stowed_start = 1;
		ps_en_flag = 1; //tuwenzan@wind-mobi.com add at 20170414
		ps_stowed_enf = 1;	//tuwenzan@wind-mobi.com add at 20170421
	}
	else {
		APS_LOG("PS: stowed disable ps only \n");
		regdata = 0x00;
		ps_stowed_start = 0;
		ps_en_flag = 0; //tuwenzan@wind-mobi.com add at 20170414
		ps_stowed_enf = 0;	//tuwenzan@wind-mobi.com add at 20170421
	}

	err = ltr778_i2c_write_reg(LTR778_PS_CONTR, regdata);
	if (err < 0)
	{
		APS_ERR("PS: enable ps err: %d en: %d \n", err, enable);
		return err;
	}

	mdelay(WAKEUP_DELAY);

	if (0 == ltr778_obj->hw->polling_mode_ps && ((regdata & 0x02) == 0x02))
	{
	atomic_set(&obj->ps_thd_val_high_stowed, atomic_read(&obj->ps_thd_val_high) + 50);
	atomic_set(&obj->ps_thd_val_low_stowed, atomic_read(&obj->ps_thd_val_low) + 20);
//#ifdef GN_MTK_BSP_PS_DYNAMIC_CALI
#if 0
		err = ltr778_dynamic_calibrate();
		if (err < 0)
		{
			APS_LOG("ltr778_dynamic_calibrate() failed\n");
		}
#endif
	ltr778_i2c_write_reg( LTR778_PS_THRES_LOW_0,(u8)((atomic_read(&obj->ps_thd_val_low_stowed)) & 0x00FF) );
	ltr778_i2c_write_reg( LTR778_PS_THRES_LOW_1, (u8)(((atomic_read(&obj->ps_thd_val_low_stowed)) & 0x7F00) >> 8));
	ltr778_i2c_write_reg( LTR778_PS_THRES_UP_0, (u8)((atomic_read(&obj->ps_thd_val_high_stowed)) & 0x00FF) );
	ltr778_i2c_write_reg( LTR778_PS_THRES_UP_1, (u8)(((atomic_read(&obj->ps_thd_val_high_stowed)) & 0x7F00) >> 8));
	}
	else if (0 == ltr778_obj->hw->polling_mode_ps && ((regdata & 0x02) == 0x00))
	{
		cancel_work_sync(&ltr778_obj->eint_work);
	}

	APS_LOG("ltr778_ps_stowed_enable(%d) ...done!\n",enable);

	return err;
}
#endif
static int ltr778_ps_enable(struct i2c_client *client, int enable)
{
	u8 regdata;
	int err;
	//liujinzhou@wind-mobi.com modify at 20161215 begin
	int res;
	
	APS_LOG("ltr778_ps_enable(%d) ...start!\n",enable);

	// modified by steven	
	res = ltr778_i2c_write_reg( LTR778_PS_THRES_LOW_0, 0x00);
			if(res < 0)
			{
				return res;
			}
			
			res = ltr778_i2c_write_reg( LTR778_PS_THRES_LOW_1, 0x00);
			if(res < 0)
			{
				return res;
			}
			
			res = ltr778_i2c_write_reg( LTR778_PS_THRES_UP_0, 0xFF);
			if(res < 0)
			{
				return res;
			}
			
			res = ltr778_i2c_write_reg( LTR778_PS_THRES_UP_1, 0x07);
			if(res < 0)
			{
				return res;
			}
	//liujinzhou@wind-mobi.com modify at 20161215 end
#ifdef CONFIG_MOTO_AOD_BASE_ON_AP_SENSORS
	ltr778_i2c_write_reg(LTR778_PS_MEAS_RATE, 0x02);// 25ms measurement time
	ltr778_i2c_write_reg(LTR778_INTERRUPT_PRST, 0x20);// 2 persist
#endif
	regdata = ltr778_i2c_read_reg(LTR778_PS_CONTR);
	if (enable != 0) {
		APS_LOG("PS: enable ps only \n");
		regdata = 0xC2;  //regdata |= 0x02;
		ps_en_flag = 1;
	}
	else {
		APS_LOG("PS: disable ps only \n");
		regdata = 0x00; //regdata &= 0xfd;
		ps_en_flag = 0;
	}
	//tuwenzan@wind-mobi.com add at 20170313 end


//tuwenzan@wind-mobic.com add at 20170424 begin	
	if (enable != 0) {
		APS_LOG("PS: enable als als_enable_flag=%d \n",als_enable_flag);
		if(als_enable_flag == 0)
			{
				ltr778_als_enable(client, 1);
				ps_enable_als = 1;
			}else{
				ps_enable_als = 0;
			}
	}else {
		APS_LOG("PS: disable als ps_enable_als =%d\n",ps_enable_als);
		if(ps_enable_als == 1)
			{
				ltr778_als_enable(client, 0);
			}
	}
//tuwenzan@wind-mobic.com add at 20170424 end
	err = ltr778_i2c_write_reg(LTR778_PS_CONTR, regdata);
	if (err < 0)
	{
		APS_ERR("PS: enable ps err: %d en: %d \n", err, enable);
		return err;
	}

	mdelay(WAKEUP_DELAY);	
	//liujinzhou@wind-mobi.com modify at 20161205 begin	
	regdata = ltr778_i2c_read_reg(LTR778_PS_CONTR);
    

	if (0 == ltr778_obj->hw->polling_mode_ps && ((regdata & 0x02) == 0x02))
	//liujinzhou@wind-mobi.com modify at 20161205 end
	{
#ifdef GN_MTK_BSP_PS_DYNAMIC_CALI
		err = ltr778_dynamic_calibrate();
		if (err < 0)
		{
			APS_LOG("ltr778_dynamic_calibrate() failed\n");			
		}
#endif
		ltr778_ps_set_thres();
	}
	//liujinzhou@wind-mobi.com modify at 20161205 begin
	else if (0 == ltr778_obj->hw->polling_mode_ps && ((regdata & 0x02) == 0x00))
	{
		cancel_work_sync(&ltr778_obj->eint_work);
	}
	
	APS_LOG("ltr778_ps_enable(%d) ...done!\n",enable);
	//liujinzhou@wind-mobi.com modify at 20161205 end

	return err;
}

/********************************************************************/
//tuwenzan@wind-mobi.com modify for solve oil bug at 20170307 begin
static int ps_en = 0;
static int ps_offen = 0;
static int ltr778_ps_read(struct i2c_client *client, u16 *data)
{
	int psval_lo, psval_hi, psdata;
	int ps_offd , ps_offdl, ps_offdh;
	int ps_offr , ps_offrl, ps_offrh;
	psval_lo = ltr778_i2c_read_reg(LTR778_PS_DATA_0);
	//APS_DBG("ps_rawdata_psval_lo = %d\n", psval_lo);
	if (psval_lo < 0){	    
	    APS_DBG("psval_lo error\n");
		psdata = psval_lo;
		goto out;
	}

	psval_hi = ltr778_i2c_read_reg(LTR778_PS_DATA_1);
    //APS_DBG("ps_rawdata_psval_hi = %d\n", psval_hi);
	if (psval_hi < 0){
	    APS_DBG("psval_hi error\n");
		psdata = psval_hi;
		goto out;
	}
	/* by steven   decress   power */	
	psdata = ((psval_hi & 7)* 256) + psval_lo;
	if(ps_offen == 0){
		ps_offen = 1; //tuwenzan@wind-mobi.com add at 20170424
		if(ps_en == 1 && psdata > 80 && psdata < 1024){
			

			ps_offd = psdata-50;
			
			APS_DBG("ltr778_ps_read:  ps_offd= %d\n", ps_offd);


			ps_offdl = ps_offd & 0x00ff;
			ps_offdh = (ps_offd & 0x0700)/256;

			ltr778_i2c_write_reg(0x99, ps_offdl);
			ltr778_i2c_write_reg(0x9A, ps_offdh);

		}else if(ps_en == 1 && psdata >= 1024 && psdata < 1400){
			
			ps_offen = 1;
			
			ltr778_i2c_write_reg(0x99, 0xff);
			ltr778_i2c_write_reg(0x9A, 0x03);			
			
		}
	}


	
	psval_lo = ltr778_i2c_read_reg(LTR778_PS_DATA_0);
	APS_DBG("ps_rawdata_psval_lo = %d\n", psval_lo);
	if (psval_lo < 0){	    
	    APS_DBG("psval_lo error\n");
		psdata = psval_lo;
		goto out;
	}

	psval_hi = ltr778_i2c_read_reg(LTR778_PS_DATA_1);
    APS_DBG("ps_rawdata_psval_hi = %d\n", psval_hi);
	if (psval_hi < 0){
	    APS_DBG("psval_hi error\n");
		psdata = psval_hi;
		goto out;
	}
	psdata = ((psval_hi & 7)* 256) + psval_lo;
	
	if(get_avg_ps(psdata) == 0  && ps_offen == 1 )
	{
		
		ps_offrl = ltr778_i2c_read_reg(0x99);
		ps_offrh = ltr778_i2c_read_reg(0x9A);
		ps_offr = ((ps_offrh & 7)* 256) + ps_offrl;
		
		APS_DBG("ltr778_ps_read:  ps_offr= %d\n", ps_offr);
		
		if(ps_offr < 100){
		ltr778_i2c_write_reg(0x99, 0x00);
		ltr778_i2c_write_reg(0x9A, 0x00);
			
		}else{
		
			ps_offd = ps_offr-25;
			
			APS_DBG("ltr778_ps_read:  ps_offd= %d\n", ps_offd);

			ps_offdl = ps_offd & 0x00ff;
			ps_offdh = (ps_offd & 0x0700)/256;

			ltr778_i2c_write_reg(0x99, ps_offdl);
			ltr778_i2c_write_reg(0x9A, ps_offdh);
			
		}
		
			psval_lo = ltr778_i2c_read_reg(LTR778_PS_DATA_0);
			//APS_DBG("ps_rawdata_psval_lo1 = %d\n", psval_lo);
			psval_hi = ltr778_i2c_read_reg(LTR778_PS_DATA_1);
			//APS_DBG("ps_rawdata_psval_hi1 = %d\n", psval_hi);
			if (psval_hi < 0){
				APS_DBG("psval_hi error\n");
				psdata = psval_hi;
				goto out;
			}
			psdata = ((psval_hi & 7)* 256) + psval_lo;

	}
//tuwenzan@wind-mobi.com modify for solve oil bug at 20170307 end	
	*data = psdata;
    APS_DBG("ltr778_ps_read: ps_rawdata = %d\n", psdata);
    
out:
	final_prox_val = psdata;	
	return psdata;
}

//tuwenzan@wind-mobi.com modify at 20170323 begin
static unsigned int psensor_first_cal = 0;

#ifdef GN_MTK_BSP_PS_DYNAMIC_CALI
static int ltr778_dynamic_calibrate(void)
{
	int i = 0;
	int data;
	int data_total = 0;
	int noise = 0;
	int count = 5;
	int ps_thd_val_low, ps_thd_val_high;
	//liujinzhou@wind-mobi.com modify at 20161205 begin
	int ps_persist_val_low, ps_persist_val_high;
	struct ltr778_priv *obj = ltr778_obj;
	ps_thd_val_low = ps_thd_val_high = ps_persist_val_low = ps_persist_val_high =0;
	//liujinzhou@wind-mobi.com modify at 20161205 end
	ltr778_i2c_write_reg(LTR778_PS_MEAS_RATE, 0x01);	// 12.25 ms time 
	if (!ltr778_obj)
	{
		APS_ERR("ltr778_obj is null!!\n");
		return -1;
	}

	for(i = 0; i < 3 ; i++)
		{		
		msleep(13);
		ltr778_ps_read(ltr778_obj->client, &ltr778_obj->ps);
		}

	for (i = 0; i < count; i++) {
		// wait for ps value be stable
		ps_en = 1;
		msleep(13);

		data = ltr778_ps_read(ltr778_obj->client, &ltr778_obj->ps);
		if (data < 0) {
			i--;
			continue;
		}				

		data_total += data;
	}
	ps_en = 0;

	noise = data_total / count;

	
	APS_LOG("%s:noise1 = %d\n", __func__, noise);
//tuwenzan@wind-mobic.com add at 20170424 begin	
	obj->als = ltr778_als_read(obj->client, &obj->als);
	
	
	if(obj->als < 10 && noise > atomic_read(&obj->ps_persist_val_low) && psensor_first_cal == 1){

		noise = 3000;  // not meet the calibration condition -- noise < dynamic_calibrate + 450

	}
	psensor_first_cal = 1; // first time  must run dynamic calibrate
	
	APS_LOG("%s:noise2 = %d\n", __func__, noise);
//tuwenzan@wind-mobic.com add at 20170424 end


	
    //liujinzhou@wind-mobi.com modify at 20161205 begin
	if(noise < dynamic_calibrate + 450)  // modified by steven
	{
		dynamic_calibrate = noise;

		if (noise < 100) {
		//liujinzhou@wind-mobi.com modify at 20161215 begin
			ps_thd_val_high = noise + 30;
			ps_thd_val_low  = noise + 15;
			ps_persist_val_high = 2046;// modified by steven
			ps_persist_val_low  = noise + 100;
		}
		else if (noise < 200) {
			ps_thd_val_high = noise + 30;
			ps_thd_val_low  = noise + 15;
			ps_persist_val_high = noise + 1700;
			ps_persist_val_low  = noise + 120;
		//liujinzhou@wind-mobi.com modify at 20161215 end
		}
		else if (noise < 300) {
			ps_thd_val_high = noise + 30;
			ps_thd_val_low  = noise + 15;
			ps_persist_val_high = noise + 1600;
			ps_persist_val_low  = noise + 160;
		}
		else if (noise < 400) {
			ps_thd_val_high = noise + 30;
			ps_thd_val_low  = noise + 15;
			ps_persist_val_high = noise + 1600;
			ps_persist_val_low  = noise + 200;
		}
		else if (noise < 600) {
			ps_thd_val_high = noise + 47;
			ps_thd_val_low  = noise + 30;
			ps_persist_val_high = noise + 1400;
			ps_persist_val_low  = noise + 220;
		}
		else if (noise < 1500) {
			ps_thd_val_high = noise + 47;
			ps_thd_val_low  = noise + 30;
			ps_persist_val_high = 2047;
			ps_persist_val_low  = noise + 250;
		}
		else {
			ps_thd_val_high = 1900;
			ps_thd_val_low  = 1700;	
		ps_persist_val_high = 2047;
		ps_persist_val_low  = 1900;		
		}
	

	atomic_set(&obj->ps_thd_val_high, ps_thd_val_high);
	atomic_set(&obj->ps_thd_val_low, ps_thd_val_low);
	
	atomic_set(&obj->ps_persist_val_high, ps_persist_val_high);
	atomic_set(&obj->ps_persist_val_low, ps_persist_val_low);

	}
	
	APS_LOG("%s:obj->ps_thd_val_high = %d\n", __func__, atomic_read(&obj->ps_thd_val_high));
	APS_LOG("%s:obj->ps_thd_val_low = %d\n", __func__, atomic_read(&obj->ps_thd_val_low));
	APS_LOG("%s:obj->ps_persist_val_high = %d\n", __func__, atomic_read(&obj->ps_persist_val_high));
	APS_LOG("%s:obj->ps_persist_val_low = %d\n", __func__, atomic_read(&obj->ps_persist_val_low));
    //liujinzhou@wind-mobi.com modify at 20161205 end
	ltr778_i2c_write_reg(LTR778_PS_MEAS_RATE, 0x03);	// 50ms time 
	
	ltr778_i2c_write_reg(LTR778_INTERRUPT_PRST, 0x00);// 0 persist

	return 0;
}
#endif
//tuwenzan@wind-mobi.com modify at 20170323 end

/********************************************************************/
/* 
 * ################
 * ## ALS CONFIG ##
 * ################
 */
 
//tuwenzan@wind-mobi.com modify at 20170222 begin
static int ltr778_als_enable(struct i2c_client *client, int enable)
{
	int err = 0;
	u8 regdata = 0;
	u8 regvalue = 0;
	//liujinzhou@wind-mobi.com modify at 20161205 begin
	//regdata = ltr778_i2c_read_reg(LTR778_ALS_CONTR);
	if (enable != 0) {
		APS_LOG("ALS(1): enable als only \n");
		regdata = 0x01;
		als_enable_flag = 1;
	}
	else {
		APS_LOG("ALS(1): disable als only \n");
		regdata = 0x00;
		als_enable_flag = 0;
		
	}
    //liujinzhou@wind-mobi.com modify at 20161205 end
	err = ltr778_i2c_write_reg(LTR778_ALS_CONTR, regdata);
	if (err < 0)
	{
		APS_ERR("ALS: enable als err: %d en: %d \n", err, enable);
		return err;
	}

	mdelay(WAKEUP_DELAY);

	regvalue = ltr778_i2c_read_reg(LTR778_ALS_CONTR);
	if(regvalue == 0x01){
		printk("twz test regvalue = %x\n",regvalue);
	}else{
		printk("twz test  regvalue = %x\n",regvalue);
	}
	return 0;
}

static int ltr778_als_read(struct i2c_client *client, u16* data)
{
	int alsval_ch0_lo, alsval_ch0_hi, alsval_ch0;
	int alsval_ch1_lo, alsval_ch1_hi, alsval_ch1;
	int luxdata_int;
	int ratio;
	int ch0_coeff;
	int ch1_coeff;
	//u8 status_value = 0;
	//liujinzhou@wind-mobi.com modify at 20161205 begin
	int coeff_factor = 10000;
    //liujinzhou@wind-mobi.com modify at 20161205 end
   // status_value = ltr778_i2c_read_reg(LTR778_ALS_PS_STATUS);
	//printk("twz test status_value = %x\n",status_value);
	alsval_ch1_lo = ltr778_i2c_read_reg(LTR778_ALS_DATA_CH1_0);
	alsval_ch1_hi = ltr778_i2c_read_reg(LTR778_ALS_DATA_CH1_1);
	alsval_ch1 = (alsval_ch1_hi * 256) + alsval_ch1_lo;
	//APS_DBG("alsval_ch1_lo = %d,alsval_ch1_hi=%d,alsval_ch1=%d\n",alsval_ch1_lo,alsval_ch1_hi,alsval_ch1);

	alsval_ch0_lo = ltr778_i2c_read_reg(LTR778_ALS_DATA_CH0_0);
	alsval_ch0_hi = ltr778_i2c_read_reg(LTR778_ALS_DATA_CH0_1);
	alsval_ch0 = (alsval_ch0_hi * 256) + alsval_ch0_lo;
	//APS_DBG("alsval_ch0_lo = %d,alsval_ch0_hi=%d,alsval_ch0=%d\n",alsval_ch0_lo,alsval_ch0_hi,alsval_ch0);

    if(alsval_ch0 == 0)
    {
        luxdata_int = 0;
        goto out;
    }

	ratio = alsval_ch1 * 100 / alsval_ch0;
	//APS_DBG("ratio = %d\n", ratio);

	if (ratio < 14)
	{
		ch0_coeff = 0;
		ch1_coeff = 0;
	//liujinzhou@wind-mobi.com modify at 20161205 begin
		winfac_1 = 3; // 1
		winfac_2 = 1;
	}
	else if ((ratio >= 14) && (ratio < 18)) // modified by steven
	{//A light
		ch0_coeff = 379;
		ch1_coeff = 1520;
		winfac_1 = 5; // 1
		winfac_2 = 3;
	}
	else if ((ratio >= 18) && (ratio < 30))  
	{// D6500K 
		ch0_coeff = 379;
		ch1_coeff = 1520;
		winfac_1 = 21; //tuwenzan@wind-mobi.com modify 32->21 at 20170302
		winfac_2 = 7;
	}
	else if ((ratio >= 30) && (ratio < 49))
	{//CWF
		ch0_coeff = -4910;
		ch1_coeff = 19950;
		winfac_1 = 24;
		winfac_2 = 3;
		
	}
	else if ((ratio >= 49) && (ratio < 53))
	{
		ch0_coeff = 8000;
		ch1_coeff = -5760;
		winfac_1 = 6;
		winfac_2 = 1;
	
	}
	//liujinzhou@wind-mobi.com modify at 20170210 begin
	else if ((ratio >= 53) && (ratio < 90))
	{
		ch0_coeff = 8000;
		ch1_coeff = -5760;
		winfac_1 = 9;
		winfac_2 = 1;
	
	}
	else if (ratio >= 90 && ratio <130)
	{
		ch0_coeff = 8000;
		ch1_coeff = -5760;
		winfac_1 = 13;
		winfac_2 = 4;
	}else if(ratio >= 130 && ratio < 200)
	{
		ch0_coeff = 8000;
		ch1_coeff = -3200;
		winfac_1 = 13;
		winfac_2 = 4;
	}else{
		ch0_coeff = 8000;
		ch1_coeff = -800;
		winfac_1 = 13;
		winfac_2 = 4;
	}
	//liujinzhou@wind-mobi.com modify at 20170210 end
	luxdata_int = ((ch0_coeff * alsval_ch0) + (ch1_coeff * alsval_ch1))* WIN_FACTOR*winfac_1/winfac_2 / coeff_factor / als_gain_factor / als_integration_factor;
	//liujinzhou@wind-mobi.com modify at 20161205 end
	//APS_DBG("ltr778_als_read als_value_lux = %d\n", luxdata_int);
out:
	luxdata_int = get_avg_lux(luxdata_int);//huyunge@wubd-mobi.com modify 20170411	
	*data = luxdata_int;
	final_lux_val = luxdata_int;
	return luxdata_int;
}
//tuwenzan@wind-mobi.com modify at 20170222 end
//tuwenzan@wind-mobi.com modify for solve oil bug at 20170307 begin
//liujinzhou@wind-mobi.com add at 20161205 begin
static int oil_far_cal = 0;
static int oil_close = 0;
//liujinzhou@wind-mobi.com add at 20161205 end
/********************************************************************/

static int ltr778_stowed_get_ps_value(struct ltr778_priv *obj, u16 ps)
{
	int val;
	int invalid = 0;

	static int val_temp = 1;
	if((ps > atomic_read(&obj->ps_thd_val_high_stowed)))
	{
		val = 0;  /*close*/
		val_temp = 0;
		intr_flag_value = 1;
	}
	else if((ps < atomic_read(&obj->ps_thd_val_low_stowed)))
	{
		val = 1;  /*far away*/
		val_temp = 1;
		intr_flag_value = 0;
	}
	else
		val = val_temp;			
	
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
	else if (obj->als > 50000)
	{
		//invalid = 1;
		APS_DBG("ligh too high will result to failt proximiy\n");
		return 1;  /*far away*/
	}

	if(!invalid)
	{
		APS_DBG("stowed PS:  %05d => %05d\n", ps, val);
		return val;
	}	
	else
	{
		return -1;
	}	
}


static int ltr778_get_ps_value(struct ltr778_priv *obj, u16 ps)
{
	int val = 0; //tuwenzan@wind-mobi.com add at 20170424
	int invalid = 0;

	static int val_temp = 1;
	//liujinzhou@wind-mobi.com modify at 20161205 begin
	APS_DBG("ALS/PS ltr778_get_ps_value oil_close= %d\n", oil_close);
	if((ps >= atomic_read(&obj->ps_persist_val_high)))  // modified by steven
	{
		val = 2;  /* persist oil close*/
		val_temp = 2;
		intr_flag_value = 2;
		oil_far_cal = 0;
		oil_close = 1;
			clear_stable();
	}
	else if((ps >= atomic_read(&obj->ps_thd_val_high)))
	{
		if(oil_close == 0)
			{
				val = 0;  /*close*/
				val_temp = 0;
				intr_flag_value = 1;
				oil_far_cal = 0;
			clear_stable();
			}
		//tuwenzan@wind-mobi.com modify at 20170424 begin
		if((ps <= (atomic_read(&obj->ps_persist_val_low)-10)) && (oil_close == 1) )
			{
				val = 3;  /* persist oil far away*/
				val_temp = 3;
				intr_flag_value = 3;
			}else if((ps > (atomic_read(&obj->ps_persist_val_low)+15)) && (oil_close == 1)){
				val = 2;  //oil close
				val_temp = 2;
				intr_flag_value = 2;
			clear_stable();
			}else{
				val = val_temp;
			clear_stable();
			}
	}
		//tuwenzan@wind-mobi.com modify at 20170424 end	
	else if((ps <= atomic_read(&obj->ps_thd_val_low)))
	{
		val = 1;  /*far away*/
		val_temp = 1;
		intr_flag_value = 0;
		oil_far_cal = 0;
			clear_stable();
		oil_close = 0;
	}
	else if(oil_close == 1)
	{	

		val = 3;  /* persist oil far away*/
		val_temp = 3;
		intr_flag_value = 3;
	}
	else
	{
		val = val_temp;		
		oil_far_cal = 0;
			clear_stable();
	}

	

	if(val == 3  && oil_far_cal <= (PS_NUM+10))  // modified by steven stable data
	{		
		oil_far_cal ++;
		val = 2;  /* persist oil close*/
		val_temp = 2;
		intr_flag_value = 2;

		if(get_stable_ps(ps) == 1){
			clear_stable();
			
			val = 3;  /* persist oil far away*/
			val_temp = 3;
			intr_flag_value = 3;

			APS_DBG("ALS/PS ltr778_get_ps_value get_stable_ps\n");
			
		}
	}
	//liujinzhou@wind-mobi.com modify at 20161205 end

	
	
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
	else if (obj->als > 50000)
	{
		//invalid = 1;
		APS_DBG("ligh too high will result to failt proximiy\n");
		return 1;  /*far away*/
	}

	if(!invalid)
	{
		APS_DBG("PS  %05d => %05d\n", ps, val);
		return val;
	}	
	else
	{
		return -1;
	}	
}
//tuwenzan@wind-mobi.com modify for solve oil bug at 20170307 end
/********************************************************************/
static int ltr778_get_als_value(struct ltr778_priv *obj, u16 als)
{
	int idx;
	int invalid = 0;
	APS_DBG("als  = %d\n",als); 
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
    //liujinzhou@wind-mobi.com modify at 20161205 begin
	if(!invalid)
	{
		APS_DBG("ALS %05d => %05d\n", als, obj->hw->als_value[idx-1]);	
		return obj->hw->als_value[idx-1];
	}
	else
	{
		APS_ERR("ALS %05d => %05d (-1)\n", als, obj->hw->als_value[idx]);    
		return -1;
	}
	//liujinzhou@wind-mobi.com modify at 20161205 end
}

static int force_fault_detection_check(void)
{
	int res;
	u8 databuf[2];

	struct i2c_client *client = ltr778_obj->client;
	struct ltr778_priv *obj = ltr778_obj;

	APS_FUN();

	if (obj->fault_detect_level == 2)
		databuf[1] = 0x10;
	else
		databuf[1] = 0x11;

	databuf[0] = LTR778_FAULT_DET_SETTING;
	res = i2c_master_send(client, databuf, 0x2);
	if (res <= 0)
	{
		goto EXIT_ERR;
	}

	databuf[0] = LTR778_ALS_CONTR;
	databuf[1] = 0x20;
	res = i2c_master_send(client, databuf, 0x2);
	if (res <= 0)
	{
		goto EXIT_ERR;
	}
	msleep(5);	// 5ms delay

	/* Read fault detection status */
	databuf[0] = ltr778_i2c_read_reg(LTR778_FAULT_DET_STATUS);
	if (databuf[0] < 0) {
		goto EXIT_ERR;
	}
	else {
		if (databuf[0] != 0) {
			res = ltr778_ps_enable(obj->client, 0);
			if (res < 0)
			{
				APS_ERR("disable ps:  %d\n", res);
			}
			goto EXIT_ERR;
		}
	}	

	if (obj->fault_detect_level == 2)
		databuf[1] = 0x00;
	else
		databuf[1] = 0x01;

	databuf[0] = LTR778_FAULT_DET_SETTING;
	res = i2c_master_send(client, databuf, 0x2);
	if (res <= 0)
	{
		goto EXIT_ERR;
	}

	databuf[0] = LTR778_ALS_CONTR;
	databuf[1] = 0x00;
	res = i2c_master_send(client, databuf, 0x2);
	if (res <= 0)
	{
		goto EXIT_ERR;
	}

	res = 0;
	return res;

EXIT_ERR:
	APS_ERR("set thres: %d\n", res);
	res = LTR778_ERR_I2C;
	return res;
}
/*-------------------------------attribute file for debugging----------------------------------*/

/******************************************************************************
 * Sysfs attributes
*******************************************************************************/
static ssize_t ltr778_show_config(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	
	if(!ltr778_obj)
	{
		APS_ERR("ltr778_obj is null!!\n");
		return 0;
	}
	
	res = snprintf(buf, PAGE_SIZE, "(%d %d %d %d %d)\n", 
		atomic_read(&ltr778_obj->i2c_retry), atomic_read(&ltr778_obj->als_debounce), 
		atomic_read(&ltr778_obj->ps_mask), atomic_read(&ltr778_obj->ps_thd_val), atomic_read(&ltr778_obj->ps_debounce));     
	return res;    
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr778_store_config(struct device_driver *ddri, const char *buf, size_t count)
{
	int retry, als_deb, ps_deb, mask, thres;
	if(!ltr778_obj)
	{
		APS_ERR("ltr778_obj is null!!\n");
		return 0;
	}
	
	if(5 == sscanf(buf, "%d %d %d %d %d", &retry, &als_deb, &mask, &thres, &ps_deb))
	{ 
		atomic_set(&ltr778_obj->i2c_retry, retry);
		atomic_set(&ltr778_obj->als_debounce, als_deb);
		atomic_set(&ltr778_obj->ps_mask, mask);
		atomic_set(&ltr778_obj->ps_thd_val, thres);        
		atomic_set(&ltr778_obj->ps_debounce, ps_deb);
	}
	else
	{
		APS_ERR("invalid content: '%s', length = %zu\n", buf, count);
	}
	return count;    
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr778_show_trace(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	if(!ltr778_obj)
	{
		APS_ERR("ltr778_obj is null!!\n");
		return 0;
	}

	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&ltr778_obj->trace));     
	return res;    
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr778_store_trace(struct device_driver *ddri, const char *buf, size_t count)
{
    int trace;
    if(!ltr778_obj)
	{
		APS_ERR("ltr778_obj is null!!\n");
		return 0;
	}
	
	if(1 == sscanf(buf, "0x%x", &trace))
	{
		atomic_set(&ltr778_obj->trace, trace);
	}
	else 
	{
		APS_ERR("invalid content: '%s', length = %zu\n", buf, count);
	}
	return count;    
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr778_show_als(struct device_driver *ddri, char *buf)
{
	int res;
		
	if(!ltr778_obj)
	{
		APS_ERR("ltr778_obj is null!!\n");
		return 0;
	}
	res = ltr778_als_read(ltr778_obj->client, &ltr778_obj->als);
	return snprintf(buf, PAGE_SIZE, "0x%04X(%d)\n", res, res);
	
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr778_show_ps(struct device_driver *ddri, char *buf)
{
	int  res;
	if(!ltr778_obj)
	{
		APS_ERR("ltr778_obj is null!!\n");
		return 0;
	}
	res = ltr778_ps_read(ltr778_obj->client, &ltr778_obj->ps);
    return snprintf(buf, PAGE_SIZE, "0x%04X(%d)\n", res, res);
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr778_show_reg(struct device_driver *ddri, char *buf)
{
	int i,len=0;
	int reg[]={0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,
		0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x9b,0x9c,0x9d,0x9e,0x9f,0xa0};
	for(i=0;i<31;i++)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "reg:0x%04X value: 0x%04X\n", reg[i],ltr778_i2c_read_reg(reg[i]));	
	}
	return len;
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr778_show_send(struct device_driver *ddri, char *buf)
{
    return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr778_store_send(struct device_driver *ddri, const char *buf, size_t count)
{
	int addr, cmd;
	u8 dat;

	if(!ltr778_obj)
	{
		APS_ERR("ltr778_obj is null!!\n");
		return 0;
	}
	else if(2 != sscanf(buf, "%x %x", &addr, &cmd))
	{
		APS_ERR("invalid format: '%s'\n", buf);
		return 0;
	}

	dat = (u8)cmd;
	//****************************
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr778_show_recv(struct device_driver *ddri, char *buf)
{
    return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr778_store_recv(struct device_driver *ddri, const char *buf, size_t count)
{
	int addr;
	
	if(!ltr778_obj)
	{
		APS_ERR("ltr778_obj is null!!\n");
		return 0;
	}
	else if(1 != sscanf(buf, "%x", &addr))
	{
		APS_ERR("invalid format: '%s'\n", buf);
		return 0;
	}

	//****************************
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr778_show_status(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	
	if(!ltr778_obj)
	{
		APS_ERR("ltr778_obj is null!!\n");
		return 0;
	}
	
	if(ltr778_obj->hw)
	{	
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: %d, (%d %d)\n", 
			ltr778_obj->hw->i2c_num, ltr778_obj->hw->power_id, ltr778_obj->hw->power_vol);		
	}
	else
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: NULL\n");
	}


	len += snprintf(buf+len, PAGE_SIZE-len, "MISC: %d %d\n", atomic_read(&ltr778_obj->als_suspend), atomic_read(&ltr778_obj->ps_suspend));

	return len;
}
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
#define IS_SPACE(CH) (((CH) == ' ') || ((CH) == '\n'))
/*----------------------------------------------------------------------------*/
static int read_int_from_buf(struct ltr778_priv *obj, const char* buf, size_t count, u32 data[], int len)
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
/*----------------------------------------------------------------------------*/
static ssize_t ltr778_show_alslv(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	int idx;
	if(!ltr778_obj)
	{
		APS_ERR("ltr778_obj is null!!\n");
		return 0;
	}
	
	for(idx = 0; idx < ltr778_obj->als_level_num; idx++)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "%d ", ltr778_obj->hw->als_level[idx]);
	}
	len += snprintf(buf+len, PAGE_SIZE-len, "\n");
	return len;    
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr778_store_alslv(struct device_driver *ddri, const char *buf, size_t count)
{
	if(!ltr778_obj)
	{
		APS_ERR("ltr778_obj is null!!\n");
		return 0;
	}
	else if(!strcmp(buf, "def"))
	{
		memcpy(ltr778_obj->als_level, ltr778_obj->hw->als_level, sizeof(ltr778_obj->als_level));
	}
	else if(ltr778_obj->als_level_num != read_int_from_buf(ltr778_obj, buf, count, 
			ltr778_obj->hw->als_level, ltr778_obj->als_level_num))
	{
		APS_ERR("invalid format: '%s'\n", buf);
	}    
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr778_show_alsval(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	int idx;
	if(!ltr778_obj)
	{
		APS_ERR("ltr778_obj is null!!\n");
		return 0;
	}
	
	for(idx = 0; idx < ltr778_obj->als_value_num; idx++)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "%d ", ltr778_obj->hw->als_value[idx]);
	}
	len += snprintf(buf+len, PAGE_SIZE-len, "\n");
	return len;    
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr778_store_alsval(struct device_driver *ddri, const char *buf, size_t count)
{
	if(!ltr778_obj)
	{
		APS_ERR("ltr778_obj is null!!\n");
		return 0;
	}
	else if(!strcmp(buf, "def"))
	{
		memcpy(ltr778_obj->als_value, ltr778_obj->hw->als_value, sizeof(ltr778_obj->als_value));
	}
	else if(ltr778_obj->als_value_num != read_int_from_buf(ltr778_obj, buf, count, 
			ltr778_obj->hw->als_value, ltr778_obj->als_value_num))
	{
		APS_ERR("invalid format: '%s'\n", buf);
	}    
	return count;
}
#ifdef CONFIG_MOTO_AOD_BASE_ON_AP_SENSORS
static ssize_t ltr778_store_stowed(struct device_driver *ddri, const char *buf, size_t count)
{
	uint16_t mode = 0;
	int ret = 0;

	ret = sscanf(buf, "%hu", &mode);
	if (ret != 1)
		return -EINVAL;

	ltr778_ps_stowed_enable(ltr778_i2c_client, mode?true:false);

	return count;
}
#endif
/*---------------------------------------------------------------------------------------*/
static DRIVER_ATTR(als,     S_IWUSR | S_IRUGO, ltr778_show_als,		NULL);
static DRIVER_ATTR(ps,      S_IWUSR | S_IRUGO, ltr778_show_ps,		NULL);
static DRIVER_ATTR(config,  S_IWUSR | S_IRUGO, ltr778_show_config,	ltr778_store_config);
static DRIVER_ATTR(alslv,   S_IWUSR | S_IRUGO, ltr778_show_alslv,	ltr778_store_alslv);
static DRIVER_ATTR(alsval,  S_IWUSR | S_IRUGO, ltr778_show_alsval,	ltr778_store_alsval);
static DRIVER_ATTR(trace,   S_IWUSR | S_IRUGO, ltr778_show_trace,	ltr778_store_trace);
static DRIVER_ATTR(status,  S_IWUSR | S_IRUGO, ltr778_show_status,	NULL);
static DRIVER_ATTR(send,    S_IWUSR | S_IRUGO, ltr778_show_send,	ltr778_store_send);
static DRIVER_ATTR(recv,    S_IWUSR | S_IRUGO, ltr778_show_recv,	ltr778_store_recv);
static DRIVER_ATTR(reg,     S_IWUSR | S_IRUGO, ltr778_show_reg,		NULL);
#ifdef CONFIG_MOTO_AOD_BASE_ON_AP_SENSORS
static DRIVER_ATTR(stowed,    S_IWUSR | S_IRUGO, NULL,				ltr778_store_stowed);
#endif

/*----------------------------------------------------------------------------*/
static struct driver_attribute *ltr778_attr_list[] = {
    &driver_attr_als,
    &driver_attr_ps,    
    &driver_attr_trace,        /*trace log*/
    &driver_attr_config,
    &driver_attr_alslv,
    &driver_attr_alsval,
    &driver_attr_status,
    &driver_attr_send,
    &driver_attr_recv,
    &driver_attr_reg,
#ifdef CONFIG_MOTO_AOD_BASE_ON_AP_SENSORS
    &driver_attr_stowed,
#endif
};
//tuwenzan@wind-mobi.com add at 20170313 end
/*----------------------------------------------------------------------------*/
static int ltr778_create_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(sizeof(ltr778_attr_list)/sizeof(ltr778_attr_list[0]));

	if (driver == NULL)
	{
		return -EINVAL;
	}

	for(idx = 0; idx < num; idx++)
	{
		if((err = driver_create_file(driver, ltr778_attr_list[idx])))
		{            
			APS_ERR("driver_create_file (%s) = %d\n", ltr778_attr_list[idx]->attr.name, err);
			break;
		}
	}    
	return err;
}
/*----------------------------------------------------------------------------*/
static int ltr778_delete_attr(struct device_driver *driver)
{
	int idx ,err = 0;
	int num = (int)(sizeof(ltr778_attr_list)/sizeof(ltr778_attr_list[0]));

	if (!driver)
		return -EINVAL;

	for (idx = 0; idx < num; idx++) 
	{
		driver_remove_file(driver, ltr778_attr_list[idx]);
	}
	
	return err;
}
/*----------------------------------------------------------------------------*/

/*----------------------------------interrupt functions--------------------------------*/
#ifndef CUSTOM_KERNEL_SENSORHUB
static int ltr778_check_intr(struct i2c_client *client) 
{
	int res,intp,intl;
	u8 buffer[2];

	APS_FUN();	

	buffer[0] = LTR778_ALS_PS_STATUS;
	res = i2c_master_send(client, buffer, 0x1);
	if(res <= 0)
	{
		goto EXIT_ERR;
	}
	res = i2c_master_recv(client, buffer, 0x1);
	if(res <= 0)
	{
		goto EXIT_ERR;
	}
	
	res = 0;
	intp = 0;
	intl = 0;
	if (0 != (buffer[0] & 0x02))
	{
		res = 1;	//PS int
		intp = 1;
	}
	if (0 != (buffer[0] & 0x08))
	{
		res = 2;	//ALS int
		intl = 1;		
	}
	if ((intp == 1) && (intl == 1))
	{
		res = 4;	//ALS & PS int		
	}

	return res;

EXIT_ERR:
	APS_ERR("ltr778_check_intr fail\n");
	return 0;
}
#endif //#ifndef CUSTOM_KERNEL_SENSORHUB
/*----------------------------------------------------------------------------*/
/*modified by steven for light test*/
//tuwenzan@wind-mobi.com modify at 20170425 begin
static u8 ps_normal_farcal = 0;
struct delayed_work dpswork;
static u16 ltr778_ps_check_delay=50;
static u8 ps_cali_time = 0;
static u8 ps_delay_work_flag = 0;

static void ltr778_ps_delay_work(struct work_struct *work)
{
	
	struct ltr778_priv *obj = ltr778_obj;

	obj->als = ltr778_als_read(obj->client, &obj->als);
	
	APS_DBG("ltr778_delay_work obj-> als  = %d \n",obj->als);
	obj->ps = ltr778_ps_read(obj->client, &obj->ps);
	
	if(get_stable_ps(obj->ps) == 1 && ps_cali_time == 0 && (obj->ps < dynamic_calibrate)){
		clear_stable();

		ps_cali_time = 1;

        		if(obj->ps < 100){			
        			atomic_set(&obj->ps_thd_val_high,  obj->ps+30);
        			atomic_set(&obj->ps_thd_val_low, obj->ps+15);
        			atomic_set(&obj->ps_persist_val_high,  obj->ps+2046);
        			atomic_set(&obj->ps_persist_val_low, obj->ps+100);
        		}else if(obj->ps < 200){
        			atomic_set(&obj->ps_thd_val_high,  obj->ps+30);
        			atomic_set(&obj->ps_thd_val_low, obj->ps+15);
        			atomic_set(&obj->ps_persist_val_high,  obj->ps+1700);
        			atomic_set(&obj->ps_persist_val_low, obj->ps+120);
        		}else if(obj->ps < 300){
        			atomic_set(&obj->ps_thd_val_high,  obj->ps+30);
        			atomic_set(&obj->ps_thd_val_low, obj->ps+15);
        			atomic_set(&obj->ps_persist_val_high,  obj->ps+1600);
        			atomic_set(&obj->ps_persist_val_low, obj->ps+160);
        		}else if(obj->ps < 400){
        			atomic_set(&obj->ps_thd_val_high,  obj->ps+30);
        			atomic_set(&obj->ps_thd_val_low, obj->ps+15);
        			atomic_set(&obj->ps_persist_val_high,  obj->ps+1600);
        			atomic_set(&obj->ps_persist_val_low, obj->ps+200);
        		}else if(obj->ps < 600){
        			atomic_set(&obj->ps_thd_val_high,  obj->ps+47);
        			atomic_set(&obj->ps_thd_val_low, obj->ps+30);
        			atomic_set(&obj->ps_persist_val_high,  obj->ps+1400);
        			atomic_set(&obj->ps_persist_val_low, obj->ps+220);
        		}else if(obj->ps < 1500){
        			atomic_set(&obj->ps_thd_val_high,  obj->ps+47);
        			atomic_set(&obj->ps_thd_val_low, obj->ps+30);
        			atomic_set(&obj->ps_persist_val_high,  2047);
        			atomic_set(&obj->ps_persist_val_low, obj->ps+250);
        		}
        		else{
        			atomic_set(&obj->ps_thd_val_high,  1900);
        			atomic_set(&obj->ps_thd_val_low, 1700);
        			atomic_set(&obj->ps_persist_val_high, 2047 );
        			atomic_set(&obj->ps_persist_val_low, 1900);
        		}
				atomic_set(&obj->ps_thd_val_high_stowed, atomic_read(&obj->ps_thd_val_high) + 50);
				atomic_set(&obj->ps_thd_val_low_stowed, atomic_read(&obj->ps_thd_val_low) + 20);
		
        		dynamic_calibrate = obj->ps;

		
		}else{
			ps_delay_work_flag++;
		}

	if(ps_cali_time == 0 && ps_delay_work_flag < 200) //tuwenzan@wind-mobi.com add at 20170421
		schedule_delayed_work(&dpswork,msecs_to_jiffies(ltr778_ps_check_delay));
	

}

static void ltr778_enable_ps_delay_work(u8 enable)
{

	APS_DBG("ltr778_enable_ps_delay_work enable = %d \n",enable);
	if (enable) 
	{
		schedule_delayed_work(&dpswork,msecs_to_jiffies(ltr778_ps_check_delay));
		
		ps_cali_time = 0;
		ps_delay_work_flag = 0;
	} 
//tuwenzan@wind-mobi.com modify at 20170425 end
	else 
	{
		cancel_delayed_work_sync(&dpswork);
		
		ps_cali_time = 0;
	}


	
	if (enable != 0) {
		APS_LOG("PS: enable als als_enable_flag=%d \n",als_enable_flag);
		if(als_enable_flag == 0)
			{
				ltr778_als_enable(ltr778_obj->client, 1);
				ps_enable_als = 1;
			}else{
				ps_enable_als = 0;
			}
	}else {
		APS_LOG("PS: disable als ps_enable_als =%d\n",ps_enable_als);
		if(ps_enable_als == 1)
			{
				ltr778_als_enable(ltr778_obj->client, 0);
			}
	}

	
}


static void ltr778_eint_work(struct work_struct *work)
{
	struct ltr778_priv *obj = (struct ltr778_priv *)container_of(work, struct ltr778_priv, eint_work);
	//liujinzhou@wind-mobi.com modify at 20161205 begin
	//u8 databuf[2];
	//liujinzhou@wind-mobi.com modify at 20161205 end
	int res = 0;
	int err;
	int value = 1;
	//liujinzhou@wind-mobi.com modify at 20161205 begin
	APS_FUN();

	/* Read fault detection status */  // modified by steven
	
	value = ltr778_i2c_read_reg(LTR778_FAULT_DET_STATUS);	

//APS_ERR("ltr778_eint_work  value:  %d\n", value);	

	if (value < 0) {
	//liujinzhou@wind-mobi.com modify at 20161205 end
		goto EXIT_INTR;
	}
	else {
		if (value != 0) {
			
			err = ltr778_i2c_write_reg(LTR778_PS_CONTR, 0x00);
			if (err < 0)
			{
				APS_ERR("PS: eint_work disable ps err: %d \n", err);
			}			
			goto EXIT_INTR;
		}
	}
	
	err = ltr778_check_intr(obj->client);
	if (err < 0) {
		goto EXIT_INTR;
	}
	else
	{
		//get raw data
		obj->ps = ltr778_ps_read(obj->client, &obj->ps);
		if (obj->ps < 0)
		{
			err = -1;
			goto EXIT_INTR;
		}
				
		APS_DBG("ltr778_eint_work rawdata ps=%d!\n",obj->ps);
		//liujinzhou@wind-mobi.com modify at 20161205 begin
		if( ps_stowed_enf == 0){
		value = ltr778_get_ps_value(obj, obj->ps);
		} else {
			value = ltr778_stowed_get_ps_value(obj, obj->ps);
		}
		if(value == 0 || value == 2)
			value = 0;
		else
			value = 1;
#ifdef CONFIG_MOTO_AOD_BASE_ON_AP_SENSORS
		if(ps_stowed_start == 1)
		{
			if(value == 1){
				APS_DBG("stowed: set 3000 mS persist");
				ltr778_i2c_write_reg(LTR778_PS_MEAS_RATE, 0x05);// 200ms measurement time
				ltr778_i2c_write_reg(LTR778_INTERRUPT_PRST, 0xE0);// 15 persist
			}else{
				APS_DBG("stowed: set 100 mS persist");
				ltr778_i2c_write_reg(LTR778_PS_MEAS_RATE, 0x03);// 50ms measurement time
				ltr778_i2c_write_reg(LTR778_INTERRUPT_PRST, 0x20);// 2 persist
			}
			value += 2;//stowed:2,unstowed:3
		}
#endif
		APS_DBG("intr_flag_value=%d    value = %d \n",intr_flag_value, value);

		/* 	modyfied by steven turn if...else to switch...case
			1--> normal near  
			0--> normal far    
			2--> oil near    
			3--> oil far
		*/
		switch(intr_flag_value){
			case 1://normal near
			{

				if(ps_normal_farcal == 1){
					ltr778_enable_ps_delay_work(0);
					ps_normal_farcal = 0;
					}
				
				if( ps_stowed_enf == 0){
					res = ltr778_i2c_write_reg( LTR778_PS_THRES_LOW_0,(u8)((atomic_read(&obj->ps_thd_val_low)) & 0x00FF) );
					if(res < 0)
					{
						goto EXIT_INTR;
					}
					
					res = ltr778_i2c_write_reg( LTR778_PS_THRES_LOW_1, (u8)(((atomic_read(&obj->ps_thd_val_low)) & 0x7F00) >> 8));
					if(res < 0)
					{
						goto EXIT_INTR;
					}
					
					res = ltr778_i2c_write_reg( LTR778_PS_THRES_UP_0,   (u8)((atomic_read(&obj->ps_persist_val_high)) & 0x00FF) );
					if(res < 0)
					{
						goto EXIT_INTR;
					}
					
					res = ltr778_i2c_write_reg( LTR778_PS_THRES_UP_1, (u8)(((atomic_read(&obj->ps_persist_val_high)) & 0x7F00) >> 8));
					if(res < 0)
					{
						goto EXIT_INTR;
					}
				}else{
				
					res = ltr778_i2c_write_reg( LTR778_PS_THRES_LOW_0,(u8)((atomic_read(&obj->ps_thd_val_low_stowed)) & 0x00FF) );
					if(res < 0)
					{
						goto EXIT_INTR;
					}
					
					res = ltr778_i2c_write_reg( LTR778_PS_THRES_LOW_1, (u8)(((atomic_read(&obj->ps_thd_val_low_stowed)) & 0x7F00) >> 8));
					if(res < 0)
					{
						goto EXIT_INTR;
					}
					
					res = ltr778_i2c_write_reg( LTR778_PS_THRES_UP_0, 0xFF );
					if(res < 0)
					{
						goto EXIT_INTR;
					}
					
					res = ltr778_i2c_write_reg( LTR778_PS_THRES_UP_1, 0X07 );
					if(res < 0)
					{
						goto EXIT_INTR;
					}
				}
			}
			break;
			
			case 0://normal far
			{

				if(obj->ps > 20 && obj->ps < (dynamic_calibrate - 50)){
					ps_normal_farcal = 1;
					ltr778_enable_ps_delay_work(1);
				}

				res = ltr778_i2c_write_reg( LTR778_PS_THRES_LOW_0, 0x00);
				if(res < 0)
				{
					goto EXIT_INTR;
				}
				
				res = ltr778_i2c_write_reg( LTR778_PS_THRES_LOW_1, 0x00);
				if(res < 0)
				{
					goto EXIT_INTR;
				}
				
				if( ps_stowed_enf == 0){
						res = ltr778_i2c_write_reg( LTR778_PS_THRES_UP_0, (u8)((atomic_read(&obj->ps_thd_val_high)) & 0x00FF));
						if(res < 0)
						{
							goto EXIT_INTR;
						}
						
						res = ltr778_i2c_write_reg( LTR778_PS_THRES_UP_1, (u8)(((atomic_read(&obj->ps_thd_val_high)) & 0x7F00) >> 8));
						if(res < 0)
						{
							goto EXIT_INTR;
						}
				}else{
					
						res = ltr778_i2c_write_reg( LTR778_PS_THRES_UP_0, (u8)((atomic_read(&obj->ps_thd_val_high_stowed)) & 0x00FF));
						if(res < 0)
						{
							goto EXIT_INTR;
						}
						
						res = ltr778_i2c_write_reg( LTR778_PS_THRES_UP_1, (u8)(((atomic_read(&obj->ps_thd_val_high_stowed)) & 0x7F00) >> 8));
						if(res < 0)
						{
							goto EXIT_INTR;
						}
				}
			}
			break;
			
			case 2:	//oil near
			{
				
				if(ps_normal_farcal == 1){
					ltr778_enable_ps_delay_work(0);
					ps_normal_farcal = 0;
					}
					res = ltr778_i2c_write_reg( LTR778_PS_THRES_LOW_0,(u8)((atomic_read(&obj->ps_persist_val_low)) & 0x00FF) );
					//APS_ERR();
					if(res < 0)
					{
						goto EXIT_INTR;
					}
					
					res = ltr778_i2c_write_reg( LTR778_PS_THRES_LOW_1, (u8)(((atomic_read(&obj->ps_persist_val_low)) & 0x7F00) >> 8));
					//APS_ERR();

					if(res < 0)
					{
						goto EXIT_INTR;
					}
					
					res = ltr778_i2c_write_reg( LTR778_PS_THRES_UP_0, 0xFF );
					//APS_ERR();

					if(res < 0)
					{
						goto EXIT_INTR;
					}
					
					res = ltr778_i2c_write_reg( LTR778_PS_THRES_UP_1, 0x07 );
					//APS_ERR();
					if(res < 0)
					{
						goto EXIT_INTR;
					}
				}
			break;
			
			case 3:	//oil far
			{
			//tuwenzan@wind-mobi.com remove oil calibration at 20170421 begin
			#if 0
			if(obj->ps > 25 ){ 
				 if(obj->ps < 50){
        			atomic_set(&obj->ps_thd_val_high,  obj->ps+75);
        			atomic_set(&obj->ps_thd_val_low, obj->ps+45);
					
        			atomic_set(&obj->ps_persist_val_high,  obj->ps+1700);
        			atomic_set(&obj->ps_persist_val_low, obj->ps+190);
        		}else if(obj->ps < 80){
        			atomic_set(&obj->ps_thd_val_high,  obj->ps+80);
        			atomic_set(&obj->ps_thd_val_low, obj->ps+50);
					
        			atomic_set(&obj->ps_persist_val_high,  obj->ps+1600);
        			atomic_set(&obj->ps_persist_val_low, obj->ps+230);
        		}else if(obj->ps < 300){
        			atomic_set(&obj->ps_thd_val_high,  obj->ps+90);
        			atomic_set(&obj->ps_thd_val_low, obj->ps+60);
					
        			atomic_set(&obj->ps_persist_val_high,  obj->ps+1200);
        			atomic_set(&obj->ps_persist_val_low, obj->ps+312);
        		}else if(obj->ps < 1400){
        			atomic_set(&obj->ps_thd_val_high,  obj->ps+200);
        			atomic_set(&obj->ps_thd_val_low, obj->ps+100);
					
        			atomic_set(&obj->ps_persist_val_high,  2000);
        			atomic_set(&obj->ps_persist_val_low, obj->ps+400);
        		}
        		else{
        			atomic_set(&obj->ps_thd_val_high,  1900);
        			atomic_set(&obj->ps_thd_val_low, 1800);
					
        			atomic_set(&obj->ps_persist_val_high, 2000 );
        			atomic_set(&obj->ps_persist_val_low, 1950);
        		}
        		
        		dynamic_calibrate = obj->ps;
        	}	        
			#endif
			//tuwenzan@wind-mobi.com remove oil calibration at 20170421 end
				res = ltr778_i2c_write_reg( LTR778_PS_THRES_LOW_0,(u8)((atomic_read(&obj->ps_thd_val_low)) & 0x00FF) );
				if(res < 0)
				{
					goto EXIT_INTR;
				}
				
				res = ltr778_i2c_write_reg( LTR778_PS_THRES_LOW_1, (u8)(((atomic_read(&obj->ps_thd_val_low)) & 0x7F00) >> 8));
				if(res < 0)
				{
					goto EXIT_INTR;
				}
				
				res = ltr778_i2c_write_reg( LTR778_PS_THRES_UP_0, (u8)((atomic_read(&obj->ps_thd_val_high)) & 0x00FF) );
				if(res < 0)
				{
					goto EXIT_INTR;
				}
				
				res = ltr778_i2c_write_reg( LTR778_PS_THRES_UP_1, (u8)(((atomic_read(&obj->ps_thd_val_high)) & 0x7F00) >> 8) );
				if(res < 0)
				{
					goto EXIT_INTR;
				}
			}
			break;

			default: 
				APS_DBG("ltr778_eint_work  switch case error intr_flag_value=%d!\n",intr_flag_value);

		}
		//let up layer to know
		res = ps_report_interrupt_data(value);
		//APS_DBG("ltr778_eint_work ps_report_interrupt_data 111 intr_flag_value=%d!\n",intr_flag_value);

	}
		APS_DBG("ltr778_eint_work ps_report_interrupt_data 222 intr_flag_value=%d!\n",intr_flag_value);


EXIT_INTR:	
#ifdef CONFIG_OF
	APS_DBG("ltr778_eint_work ps_report_interrupt_data 333 intr_flag_value=%d!\n",intr_flag_value);
	//liujinzhou@wind-mobi.com modify at 20161205 end
	enable_irq(obj->irq);
#endif
}
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static void ltr778_eint_func(void)
{
	struct ltr778_priv *obj = ltr778_obj;
	if(!obj)
	{
		return;
	}
	//liujinzhou@wind-mobi.com modify at 20161205 begin
	//APS_FUN();
	//liujinzhou@wind-mobi.com modify at 20161205 end
	int_top_time = sched_clock();
	schedule_work(&obj->eint_work);
}

#ifdef CONFIG_OF
static irqreturn_t ltr778_eint_handler(int irq, void *desc)
{
	disable_irq_nosync(ltr778_obj->irq);
	ltr778_eint_func();
	
	return IRQ_HANDLED;
}
#endif
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
int ltr778_setup_eint(struct i2c_client *client)
{
	int ret;
	struct pinctrl *pinctrl;
	struct pinctrl_state *pins_cfg;
	u32 ints[2] = { 0, 0 };	

	APS_FUN();

	alspsPltFmDev = get_alsps_platformdev();
	ltr778_obj->irq_node = of_find_compatible_node(NULL, NULL, "mediatek, als-eint");
	
	/* gpio setting */
	pinctrl = devm_pinctrl_get(&alspsPltFmDev->dev);
	if (IS_ERR(pinctrl)) {
		ret = PTR_ERR(pinctrl);
		APS_ERR("Cannot find alsps pinctrl!\n");
	}
	
	pins_cfg = pinctrl_lookup_state(pinctrl, "pin_cfg");
	if (IS_ERR(pins_cfg)) {
		ret = PTR_ERR(pins_cfg);
		APS_ERR("Cannot find alsps pinctrl pin_cfg!\n");
	}
	
	/* eint request */
	if (ltr778_obj->irq_node) {
		of_property_read_u32_array(ltr778_obj->irq_node, "debounce", ints, ARRAY_SIZE(ints));
		gpio_request(ints[0], "p-sensor");
		gpio_set_debounce(ints[0], ints[1]);
		pinctrl_select_state(pinctrl, pins_cfg);
		APS_LOG("ints[0] = %d, ints[1] = %d!!\n", ints[0], ints[1]);

		ltr778_obj->irq = irq_of_parse_and_map(ltr778_obj->irq_node, 0);
		APS_LOG("ltr778_obj->irq = %d\n", ltr778_obj->irq);
		if (!ltr778_obj->irq) {
			APS_ERR("irq_of_parse_and_map fail!!\n");
			return -EINVAL;
		}
		//APS_ERR("irq to gpio = %d \n", irq_to_gpio(ltr778_obj->irq));
		if (request_irq(ltr778_obj->irq, ltr778_eint_handler, IRQF_TRIGGER_LOW, "ALS-eint", NULL)) {
			APS_ERR("IRQ LINE NOT AVAILABLE!!\n");
			return -EINVAL;
		}
		printk("twz request_irq 12\n");
		enable_irq(ltr778_obj->irq);
	}
	else {
		APS_ERR("null irq node!!\n");
		return -EINVAL;
	}

	return 0;
}
/**********************************************************************************************/

/*-------------------------------MISC device related------------------------------------------*/
static int ltr778_open(struct inode *inode, struct file *file)
{
	file->private_data = ltr778_i2c_client;

	if (!file->private_data)
	{
		APS_ERR("null pointer!!\n");
		return -EINVAL;
	}
	return nonseekable_open(inode, file);
}
/************************************************************/
static int ltr778_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}
/************************************************************/
static long ltr778_unlocked_ioctl(struct file *file, unsigned int cmd,
       unsigned long arg)       
{
	struct i2c_client *client = (struct i2c_client*)file->private_data;
	struct ltr778_priv *obj = i2c_get_clientdata(client);  
	int err = 0;
	void __user *ptr = (void __user*) arg;
	int dat;
	uint32_t enable;
	int ps_cali;
	int threshold[2];
	APS_DBG("cmd= %d\n", cmd); 
	switch (cmd)
	{
		case ALSPS_SET_PS_MODE:
			if(copy_from_user(&enable, ptr, sizeof(enable)))
			{
				err = -EFAULT;
				goto err_out;
			}
			err = ltr778_ps_enable(obj->client, enable);
			if (err < 0)
			{
				APS_ERR("enable ps fail: %d en: %d\n", err, enable);
				goto err_out;
			}
			if (enable)
				set_bit(CMC_BIT_PS, &obj->enable);
			else
				clear_bit(CMC_BIT_PS, &obj->enable);				
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
			obj->ps = ltr778_ps_read(obj->client, &obj->ps);
			if (obj->ps < 0)
			{
				goto err_out;
			}
			
			dat = ltr778_get_ps_value(obj, obj->ps);
			//liujinzhou@wind-mobi.com add at 20161205 begin
			if(dat == 0 || dat ==2)
				dat = 0;
			else
				dat = 1;
			//liujinzhou@wind-mobi.com add at 20161205 end
			if (copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}
			break;

		case ALSPS_GET_PS_RAW_DATA:    
			obj->ps = ltr778_ps_read(obj->client, &obj->ps);
			if (obj->ps < 0)
			{
				goto err_out;
			}
			dat = obj->ps;
			if (copy_to_user(ptr, &dat, sizeof(dat)))
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
			err = ltr778_als_enable(obj->client, enable);
			if (err < 0)
			{
				APS_ERR("enable als fail: %d en: %d\n", err, enable);
				goto err_out;
			}
			if (enable)
				set_bit(CMC_BIT_ALS, &obj->enable);
			else
				clear_bit(CMC_BIT_ALS, &obj->enable);
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
			obj->als = ltr778_als_read(obj->client, &obj->als);
			if (obj->als < 0)
			{
				goto err_out;
			}

			dat = ltr778_get_als_value(obj, obj->als);
			if (copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}
			break;

		case ALSPS_GET_ALS_RAW_DATA:    
			obj->als = ltr778_als_read(obj->client, &obj->als);
			if (obj->als < 0)
			{
				goto err_out;
			}

			dat = obj->als;
			if (copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}
			break;

/*----------------------------------for factory mode test---------------------------------------*/
		case ALSPS_GET_PS_TEST_RESULT:
			obj->ps = ltr778_ps_read(obj->client, &obj->ps);
			if (obj->ps < 0)
			{
				goto err_out;
			}
			if(obj->ps > atomic_read(&obj->ps_thd_val_low))
				dat = 1;
			else	
				dat = 0;
			if(copy_to_user(ptr, &dat, sizeof(dat)))
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
			break;

		case ALSPS_IOCTL_GET_CALI:
			ps_cali = obj->ps_cali ;
			if(copy_to_user(ptr, &ps_cali, sizeof(ps_cali)))
			{
				err = -EFAULT;
				goto err_out;
			}
			break;

		case ALSPS_IOCTL_SET_CALI:
			if(copy_from_user(&ps_cali, ptr, sizeof(ps_cali)))
			{
				err = -EFAULT;
				goto err_out;
			}
			obj->ps_cali = ps_cali;
			break;

		case ALSPS_SET_PS_THRESHOLD:
			if(copy_from_user(threshold, ptr, sizeof(threshold)))
			{
				err = -EFAULT;
				goto err_out;
			}
			atomic_set(&obj->ps_thd_val_high,  (threshold[0]+obj->ps_cali));
			atomic_set(&obj->ps_thd_val_low,  (threshold[1]+obj->ps_cali));//need to confirm

			ltr778_ps_set_thres();
			break;
				
		case ALSPS_GET_PS_THRESHOLD_HIGH:
			threshold[0] = atomic_read(&obj->ps_thd_val_high) - obj->ps_cali;
			if(copy_to_user(ptr, &threshold[0], sizeof(threshold[0])))
			{
				err = -EFAULT;
				goto err_out;
			}
			break;
				
		case ALSPS_GET_PS_THRESHOLD_LOW:
			threshold[0] = atomic_read(&obj->ps_thd_val_low) - obj->ps_cali;
			if(copy_to_user(ptr, &threshold[0], sizeof(threshold[0])))
			{
				err = -EFAULT;
				goto err_out;
			}
			break;
/*------------------------------------------------------------------------------------------*/

		default:
			err = -ENOIOCTLCMD;
			break;
	}

	err_out:
	return err;    
}
/********************************************************************/
/*------------------------------misc device related operation functions------------------------------------*/
static struct file_operations ltr778_fops = {
	.owner = THIS_MODULE,
	.open = ltr778_open,
	.release = ltr778_release,
	.unlocked_ioctl = ltr778_unlocked_ioctl,
};

static struct miscdevice ltr778_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "als_ps",
	.fops = &ltr778_fops,
};

/*--------------------------------------------------------------------------------*/
static int ltr778_init_client(void)
{
	int res = 0;
	u8 databuf[2];
	int als_gainrange;

	struct i2c_client *client = ltr778_obj->client;

	struct ltr778_priv *obj = ltr778_obj;

	mdelay(PON_DELAY);

	if (force_fault_detection_check() < 0) {
		APS_LOG("ltr778 Fault detection check Fail...\n");
		goto EXIT_ERR;
	}

	res = ltr778_i2c_write_reg(LTR778_PS_LED, 0x53);		// 3mA 
	if (res<0)
	{
		APS_LOG("ltr778 set ps led error\n");
		goto EXIT_ERR;
	}
    //liujinzhou@wind-mobi.com modify at 20161205 begin
	res = ltr778_i2c_write_reg(LTR778_PS_PULSES, 0x08);		// 48 pulses
	//liujinzhou@wind-mobi.com modify at 20161205 end
	if (res<0)
	{
		APS_LOG("ltr778 set ps pulse error\n");
		goto EXIT_ERR;
	}

	res = ltr778_i2c_write_reg(LTR778_PS_MEAS_RATE, 0x03);	// 3 -> 50ms time  1-> 12.5ms
	if (res<0)
	{
		APS_LOG("ltr778 set ps meas rate error\n");
		goto EXIT_ERR;
	}

	res = ltr778_i2c_write_reg(LTR778_ALS_MEAS_RATE, 0x09);	// 100ms time 
	if (res<0)
	{
		APS_LOG("ltr778 set als meas rate error\n");
		goto EXIT_ERR;
	}

	/*for interrup work mode support */
	if (0 == obj->hw->polling_mode_ps)
	{
		APS_LOG("eint enable");
		
		databuf[0] = LTR778_INTERRUPT;
		databuf[1] = 0x01;
		res = i2c_master_send(client, databuf, 0x2);
		if (res <= 0)
		{
			goto EXIT_ERR;			
		}

		databuf[0] = LTR778_INTERRUPT_PRST;
		databuf[1] = 0x10;
		res = i2c_master_send(client, databuf, 0x2);
		if (res <= 0)
		{
			goto EXIT_ERR;
		}
	}

	//res = ltr778_ps_enable(client, 1); //tuwenzan@wind-mobi.com modify for solve oil bug at 20170307
	if (res < 0)
	{
		APS_ERR("enable ps fail: %d\n", res);
		goto EXIT_ERR;
	}

	// Enable ALS to Full Range at startup
	als_gainrange = ALS_RANGE_1;	
	APS_ERR("ALS sensor gainrange %d!\n", als_gainrange);

	switch (als_gainrange)
	{
	case ALS_RANGE_1:
		res = ltr778_i2c_write_reg(LTR778_ALS_CONTR, MODE_ALS_Range1);
		break;

	case ALS_RANGE_4:
		res = ltr778_i2c_write_reg(LTR778_ALS_CONTR, MODE_ALS_Range2);
		break;

	case ALS_RANGE_16:
		res = ltr778_i2c_write_reg(LTR778_ALS_CONTR, MODE_ALS_Range3);
		break;

	case ALS_RANGE_64:
		res = ltr778_i2c_write_reg(LTR778_ALS_CONTR, MODE_ALS_Range4);
		break;

	case ALS_RANGE_128:
		res = ltr778_i2c_write_reg(LTR778_ALS_CONTR, MODE_ALS_Range5);
		break;

	case ALS_RANGE_256:
		res = ltr778_i2c_write_reg(LTR778_ALS_CONTR, MODE_ALS_Range6);
		break;

	default:
		res = ltr778_i2c_write_reg(LTR778_ALS_CONTR, MODE_ALS_Range1);
		break;
	}
	//tuwenzan@wind-mobi.com modify for solve oil bug at 20170307 begin
	/*
	res = ltr778_als_enable(client, 1);
	if (res < 0)
	{
		APS_ERR("enable als fail: %d\n", res);
		goto EXIT_ERR;
	}
   */
   //tuwenzan@wind-mobi.com modify for solve oil bug at 20170307 end
	als_gain_factor = als_gainrange;
	als_integration_factor = 1;

	if ((res = ltr778_setup_eint(client)) != 0)
	{
		APS_ERR("setup eint: %d\n", res);
		goto EXIT_ERR;
	}	

	return 0;

EXIT_ERR:
	APS_ERR("init dev: %d\n", res);
	return 1;
}
/*--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/
// if use  this typ of enable , Gsensor should report inputEvent(x, y, z ,stats, div) to HAL
static int als_open_report_data(int open)
{
	//should queuq work to report event if  is_report_input_direct=true
	return 0;
}

// if use  this typ of enable , Gsensor only enabled but not report inputEvent to HAL
static int als_enable_nodata(int en)
{
	int res = 0;
	APS_LOG("ltr778_obj als enable value = %d\n", en);

	if(!ltr778_obj)
	{
		APS_ERR("ltr778_obj is null!!\n");
		return -1;
	}

	mutex_lock(&ltr778_mutex);
	if (en)
		set_bit(CMC_BIT_ALS, &ltr778_obj->enable);
	else
		clear_bit(CMC_BIT_ALS, &ltr778_obj->enable);
	mutex_unlock(&ltr778_mutex);
	ps_enable_als = 0; //tuwenzan@wind-mobi.com add at 20170502  clear ps control als
	res = ltr778_als_enable(ltr778_obj->client, en);
	if (res) {
		APS_ERR("als_enable_nodata is failed!!\n");
		return -1;
	}
	return 0;
}

static int als_set_delay(u64 ns)
{
	// Do nothing
	return 0;
}

static int als_get_data(int* value, int* status)
{
	int err = 0;
	
	if(!ltr778_obj)
	{
		APS_ERR("ltr778_obj is null!!\n");
		return -1;
	}

	ltr778_obj->als = ltr778_als_read(ltr778_obj->client, &ltr778_obj->als);
	if (ltr778_obj->als < 0)
		err = -1;
	else {
	//	*value = ltr778_get_als_value(ltr778_obj, ltr778_obj->als);
		*value = ltr778_obj->als;
		if (*value < 0)
			err = -1;
		*status = SENSOR_STATUS_ACCURACY_MEDIUM;
	}

	return err;
}

// if use  this typ of enable , Gsensor should report inputEvent(x, y, z ,stats, div) to HAL
static int ps_open_report_data(int open)
{
	//should queuq work to report event if  is_report_input_direct=true
	return 0;
}

// if use  this typ of enable , Gsensor only enabled but not report inputEvent to HAL
static int ps_enable_nodata(int en)
{
	int res = 0;
	APS_LOG("ltr778_obj ps enable value = %d\n", en);

	if(!ltr778_obj)
	{
		APS_ERR("ltr778_obj is null!!\n");
		return -1;
	}
	
	mutex_lock(&ltr778_mutex);
	if (en)
		set_bit(CMC_BIT_PS, &ltr778_obj->enable);
	else
		clear_bit(CMC_BIT_PS, &ltr778_obj->enable);
	mutex_unlock(&ltr778_mutex);

	res = ltr778_ps_enable(ltr778_obj->client, en);
	if (res < 0) {
		APS_ERR("als_enable_nodata is failed!!\n");
		return -1;
	}
	return 0;
}

static int ps_set_delay(u64 ns)
{
	// Do nothing
	return 0;
}

static int ps_get_data(int* value, int* status)
{
    int err = 0;

    if(!ltr778_obj)
	{
		APS_ERR("ltr778_obj is null!!\n");
		return -1;
	}
    
	ltr778_obj->ps = ltr778_ps_read(ltr778_obj->client, &ltr778_obj->ps);
	ltr778_obj->ps = get_ps_avg(ltr778_obj->ps);//add steven //tuwenzan@wind-mobi.com modify for solve oil bug at 20170307
	if (ltr778_obj->ps < 0)
		err = -1;
	else {
		*value = ltr778_get_ps_value(ltr778_obj, ltr778_obj->ps);
		if (*value < 0)
			err = -1;
		//liujinzhou@wind-mobi.com modify at 20161205 begin
		if(*value == 0 || *value == 2)
			*value = 0;
		else
			*value = 1;
		//liujinzhou@wind-mobi.com modify at 20161205 end		
		*status = SENSOR_STATUS_ACCURACY_MEDIUM;
	}
    
	return err;
}
/*-----------------------------------------------------------------------------------*/

/*-----------------------------------i2c operations----------------------------------*/
static int ltr778_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ltr778_priv *obj = NULL;
	struct als_control_path als_ctl={0};
	struct als_data_path als_data={0};
	struct ps_control_path ps_ctl={0};
	struct ps_data_path ps_data={0};
	int err = 0;

	APS_FUN();
	printk("twz enter ltr778_i2c_probe addr = 0x%x\n",client->addr);
	if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
	{
		err = -ENOMEM;
		goto exit;
	}
	memset(obj, 0, sizeof(*obj));
	ltr778_obj = obj;
	
	obj->hw = hw;
	INIT_WORK(&obj->eint_work, ltr778_eint_work);
	INIT_DELAYED_WORK(&dpswork, ltr778_ps_delay_work);//modified by steven
	obj->client = client;
	i2c_set_clientdata(client, obj);	
	
	/*-----------------------------value need to be confirmed-----------------------------------------*/
	atomic_set(&obj->als_debounce, 300);
	atomic_set(&obj->als_deb_on, 0);
	atomic_set(&obj->als_deb_end, 0);
	atomic_set(&obj->ps_debounce, 300);
	atomic_set(&obj->ps_deb_on, 0);
	atomic_set(&obj->ps_deb_end, 0);
	atomic_set(&obj->ps_mask, 0);
	atomic_set(&obj->als_suspend, 0);
	//atomic_set(&obj->als_cmd_val, 0xDF);
	//atomic_set(&obj->ps_cmd_val,  0xC1);
	atomic_set(&obj->ps_thd_val_high,  obj->hw->ps_threshold_high);
	atomic_set(&obj->ps_thd_val_low,  obj->hw->ps_threshold_low);
	atomic_set(&obj->ps_thd_val,  obj->hw->ps_threshold);
	atomic_set(&obj->als_thd_val_high,  obj->hw->als_threshold_high);
	atomic_set(&obj->als_thd_val_low,  obj->hw->als_threshold_low);
	
	obj->enable = 0;
	obj->pending_intr = 0;
	obj->ps_cali = 0;
	obj->als_level_num = sizeof(obj->hw->als_level)/sizeof(obj->hw->als_level[0]);
	obj->als_value_num = sizeof(obj->hw->als_value)/sizeof(obj->hw->als_value[0]);
	obj->als_modulus = (400*100)/(16*150);//(1/Gain)*(400/Tine), this value is fix after init ATIME and CONTROL register value
										//(400)/16*2.72 here is amplify *100	
	/*-----------------------------value need to be confirmed-----------------------------------------*/
	
	BUG_ON(sizeof(obj->als_level) != sizeof(obj->hw->als_level));
	memcpy(obj->als_level, obj->hw->als_level, sizeof(obj->als_level));
	BUG_ON(sizeof(obj->als_value) != sizeof(obj->hw->als_value));
	memcpy(obj->als_value, obj->hw->als_value, sizeof(obj->als_value));
	atomic_set(&obj->i2c_retry, 3);
	set_bit(CMC_BIT_ALS, &obj->enable);
	set_bit(CMC_BIT_PS, &obj->enable);

	APS_LOG("ltr778_init_client() start...!\n");
	ltr778_i2c_client = client;
	err = ltr778_init_client();
	if(err)
	{
		goto exit_init_failed;
	}
	APS_LOG("ltr778_init_client() OK!\n");
	
	err = misc_register(&ltr778_device);
	if(err)
	{
		APS_ERR("ltr778_device register failed\n");
		goto exit_misc_device_register_failed;
	}

    als_ctl.is_use_common_factory =false;
	ps_ctl.is_use_common_factory = false;
	
	/*------------------------ltr778 attribute file for debug--------------------------------------*/
	//err = ltr778_create_attr(&(ltr778_init_info.platform_diver_addr->driver));
	err = ltr778_create_attr(&(ltr778_i2c_driver.driver));
	if(err)
	{
		APS_ERR("create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}
	/*------------------------ltr778 attribute file for debug--------------------------------------*/
	
	als_ctl.open_report_data= als_open_report_data;
	als_ctl.enable_nodata = als_enable_nodata;
	als_ctl.set_delay  = als_set_delay;
	als_ctl.is_report_input_direct = false;
	als_ctl.is_support_batch = false;
 	
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
		APS_ERR("register fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}
	
	ps_ctl.open_report_data= ps_open_report_data;
	ps_ctl.enable_nodata = ps_enable_nodata;
	ps_ctl.set_delay  = ps_set_delay;
	ps_ctl.is_report_input_direct = false;
	ps_ctl.is_support_batch = false;
	ps_ctl.is_polling_mode = hw->polling_mode_ps;
		
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

	err = batch_register_support_info(ID_LIGHT,als_ctl.is_support_batch, 1, 0);
	if(err)
	{
		APS_ERR("register light batch support err = %d\n", err);
	}
	
	err = batch_register_support_info(ID_PROXIMITY,ps_ctl.is_support_batch, 1, 0);
	if(err)
	{
		APS_ERR("register proximity batch support err = %d\n", err);
	}

	ltr778_init_flag =0;
	APS_LOG("%s: OK\n", __func__);
	return 0;

exit_create_attr_failed:
exit_sensor_obj_attach_fail:
exit_misc_device_register_failed:
	misc_deregister(&ltr778_device);
exit_init_failed:
	kfree(obj);
exit:
	ltr778_i2c_client = NULL;           
	APS_ERR("%s: err = %d\n", __func__, err);
	ltr778_init_flag =-1;
	return err;
}

static int ltr778_i2c_remove(struct i2c_client *client)
{
	int err;

	//err = ltr579_delete_attr(&(ltr579_init_info.platform_diver_addr->driver));
	err = ltr778_delete_attr(&(ltr778_i2c_driver.driver));
	if(err)
	{
		APS_ERR("ltr778_delete_attr fail: %d\n", err);
	}

	err = misc_deregister(&ltr778_device);
	if(err)
	{
		APS_ERR("misc_deregister fail: %d\n", err);    
	}
		
	ltr778_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));

	return 0;
}

static int ltr778_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	strcpy(info->type, LTR778_DEV_NAME);
	return 0;
}

static int ltr778_i2c_suspend(struct i2c_client *client, pm_message_t msg) 
{
	struct ltr778_priv *obj = i2c_get_clientdata(client);    
	int err;
	APS_FUN();    

	if(msg.event == PM_EVENT_SUSPEND)
	{   
		if(!obj)
		{
			APS_ERR("null pointer!!\n");
			return -EINVAL;
		}
		
		atomic_set(&obj->als_suspend, 1);
		err = ltr778_als_enable(obj->client, 0);
		if(err < 0)
		{
			APS_ERR("disable als: %d\n", err);
			return err;
		}
//liujinzhou@wind-mobi.com modify at 20161205 begin
#if 0  // modified by steven
		atomic_set(&obj->ps_suspend, 1);
		err = ltr778_ps_enable(obj->client, 0);
		if(err < 0)
		{
			APS_ERR("disable ps:  %d\n", err);
			return err;
		}
		
		ltr778_power(obj->hw, 0);
#endif
//liujinzhou@wind-mobi.com modify at 20161205 end
	}
	return 0;
}

static int ltr778_i2c_resume(struct i2c_client *client)
{
	struct ltr778_priv *obj = i2c_get_clientdata(client);        
	int err;
	APS_FUN();

	if(!obj)
	{
		APS_ERR("null pointer!!\n");
		return -EINVAL;
	}

	ltr778_power(obj->hw, 1);

	atomic_set(&obj->als_suspend, 0);
	if(test_bit(CMC_BIT_ALS, &obj->enable))
	{
		err = ltr778_als_enable(obj->client, 1);
	    if (err < 0)
		{
			APS_ERR("enable als fail: %d\n", err);        
		}
	}
//liujinzhou@wind-mobi.com modify at 20161205 begin
#if 0  // modified by steven
	atomic_set(&obj->ps_suspend, 0);
	if(test_bit(CMC_BIT_PS,  &obj->enable))
	{
		err = ltr778_ps_enable(obj->client, 1);
	    if (err < 0)
		{
			APS_ERR("enable ps fail: %d\n", err);                
		}
	}
#endif
//liujinzhou@wind-mobi.com modify at 20161205
	return 0;
}
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static int ltr778_remove(void)
{
	APS_FUN();

	ltr778_power(hw, 0);	
	i2c_del_driver(&ltr778_i2c_driver);

	return 0;
}
/*----------------------------------------------------------------------------*/
static int  ltr778_local_init(void)
{
	APS_FUN();

	ltr778_power(hw, 1);
	
	if(i2c_add_driver(&ltr778_i2c_driver))
	{
		APS_ERR("add driver error\n");
		return -1;
	}

	if(-1 == ltr778_init_flag)
	{
	   return -1;
	}
	
	return 0;
}
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static int __init ltr778_init(void)
{
    const char *name = "mediatek,ltr778";

	APS_FUN();

    hw = get_alsps_dts_func(name, hw);
	if (!hw)
		APS_ERR("get dts info fail\n");

	alsps_driver_add(&ltr778_init_info);
	return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit ltr778_exit(void)
{
	APS_FUN();
}
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
module_init(ltr778_init);
module_exit(ltr778_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("Liteon");
MODULE_DESCRIPTION("LTR-778ALSPS Driver");
MODULE_LICENSE("GPL");
//tuwenzan@wind-mobi.com modify at 20170331 end

