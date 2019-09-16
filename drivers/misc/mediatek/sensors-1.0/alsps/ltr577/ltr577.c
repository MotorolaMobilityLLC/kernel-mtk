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
#include "ltr577.h"
#include "alsps.h"

#define GN_MTK_BSP_PS_DYNAMIC_CALI
//#define DELAYED_PS_CALI
//#define DEMO_BOARD
//#define LTR577_DEBUG
//#define SENSOR_DEFAULT_ENABLED
#define UPDATE_PS_THRESHOLD
#define NO_ALS_CTRL_WHEN_PS_ENABLED
//#define REPORT_PS_ONCE_WHEN_ALS_ENABLED
#define NO_PS_CTRL_WHEN_SUSPEND_RESUME

/******************************************************************************
 * configuration
*******************************************************************************/
/*----------------------------------------------------------------------------*/
#define LTR577_DEV_NAME			"ltr577"

/*----------------------------------------------------------------------------*/
#define APS_TAG					"[ALS/PS] "
#define APS_FUN(f)              pr_err(APS_TAG"%s\n", __FUNCTION__)
#define APS_ERR(fmt, args...)	pr_err(APS_TAG"[ERROR][%s][%d]: "fmt, __FUNCTION__, __LINE__, ##args)
#define APS_LOG(fmt, args...)	pr_err(APS_TAG"[INFO][%s][%d]:  "fmt, __FUNCTION__, __LINE__, ##args)
#define APS_DBG(fmt, args...)	ontim_dev_dbg(7, APS_TAG fmt, ##args)

/*----------------------------------------------------------------------------*/
static const struct i2c_device_id ltr577_i2c_id[] = {{LTR577_DEV_NAME,0},{}};
static unsigned long long int_top_time;
static struct alsps_hw alsps_cust;
static struct alsps_hw *hw = &alsps_cust;

static struct platform_device *alspsPltFmDev;
 // static struct i2c_board_info __initdata ltr577_i2c_board_info = { I2C_BOARD_INFO(LTR577_DEV_NAME, 0x53) }; 

static unsigned int coeff_boyi[3] = {180,160,210};
static unsigned int coeff_txd[3] =  {180,160,210};
static unsigned int current_tp = 0;
static unsigned int current_color_temp=CWF_TEMP;
static unsigned int current_color_temp_first = 0;
static unsigned int als_level[TP_COUNT][TEMP_COUNT][C_CUST_ALS_LEVEL];
static unsigned int  als_value[C_CUST_ALS_LEVEL] = {0};
static int ps_irq_use_old = 0;
static int *coeff_als_for_tp = coeff_boyi;
static unsigned int cureent_color_ratio = 0;
#define PS_PULSES 16

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
	static unsigned int ps_cali_factor_30 = 43;
	static unsigned int ps_cali_factor_45 = 74;

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


	#define MAX_ADC_PROX_VALUE 2047
	#define FAR_THRESHOLD(x) (min_proximity<(x)?(x):min_proximity)
	#define NRAR_THRESHOLD(x) ((FAR_THRESHOLD(x)+pwindows_value-1)>MAX_ADC_PROX_VALUE ? MAX_ADC_PROX_VALUE:(FAR_THRESHOLD(x)+pwindows_value-1))
#endif
///add by xi'an end

/*----------------------------------------------------------------------------*/
static int ltr577_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id); 
static int ltr577_i2c_remove(struct i2c_client *client);
static int ltr577_i2c_detect(struct i2c_client *client, struct i2c_board_info *info);
static int ltr577_i2c_suspend(struct device *dev);
static int ltr577_i2c_resume(struct device *dev);
static int ltr577_check_for_esd(void);

#ifdef LTR577_DEBUG
static int ltr577_dump_reg(void);
#endif

//static int ps_gainrange;
static int als_gainrange;

static int final_prox_val;
static int final_lux_val;

/*----------------------------------------------------------------------------*/
typedef enum {
    CMC_BIT_ALS    = 1,
    CMC_BIT_PS     = 2,
} CMC_BIT;

/*----------------------------------------------------------------------------*/
struct ltr577_priv {
	struct alsps_hw  *hw;
	struct i2c_client *client;
	struct work_struct	eint_work;
#ifdef REPORT_PS_ONCE_WHEN_ALS_ENABLED
	struct delayed_work check_ps_work;
#endif
#ifdef DELAYED_PS_CALI
	struct delayed_work cali_ps_work;
#endif

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
		u32		als_level[TP_COUNT][TEMP_COUNT][C_CUST_ALS_LEVEL];
	u32			als_value[C_CUST_ALS_LEVEL];
	int			ps_cali;
	
	atomic_t	als_cmd_val;	/*the cmd value can't be read, stored in ram*/
	atomic_t	ps_cmd_val; 	/*the cmd value can't be read, stored in ram*/
	atomic_t	ps_thd_val_high;	 /*the cmd value can't be read, stored in ram*/
	atomic_t	ps_thd_val_low; 	/*the cmd value can't be read, stored in ram*/
	atomic_t	als_thd_val_high;	 /*the cmd value can't be read, stored in ram*/
	atomic_t	als_thd_val_low; 	/*the cmd value can't be read, stored in ram*/
	atomic_t	ps_thd_val;
	ulong		enable; 		/*enable mask*/
	ulong		pending_intr;	/*pending interrupt*/
};

struct PS_CALI_DATA_STRUCT
{
    int close;
    int far_away;
    int valid;
} ;
#ifndef HW_dynamic_cali
static struct PS_CALI_DATA_STRUCT ps_cali={0,0,0};

#endif

static struct ltr577_priv *ltr577_obj = NULL;
static struct i2c_client *ltr577_i2c_client = NULL;

static DEFINE_MUTEX(ltr577_mutex);
static DEFINE_MUTEX(ltr577_i2c_mutex);

static int ltr577_local_init(void);
static int ltr577_remove(void);
static int ltr577_update(void);
static int ltr577_init_flag =-1; // 0<==>OK -1 <==> fail
static int ltr577_read_cali_file(struct i2c_client *client);

static int ps_enabled = 0;
static int als_enabled = 0;

//static int irq_enabled = 0;

static struct alsps_init_info ltr577_init_info = {
		.name = "ltr577",
		.init = ltr577_local_init,
		.uninit = ltr577_remove,	
		.update = ltr577_update,
};

#ifdef CONFIG_OF
static const struct of_device_id alsps_of_match[] = {
	{.compatible = "mediatek,alsps"},
	{},
};
#endif

#ifdef CONFIG_PM_SLEEP
static const struct dev_pm_ops ltr577_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(ltr577_i2c_suspend, ltr577_i2c_resume)
};
#endif

static struct i2c_driver ltr577_i2c_driver = {	
	.probe      = ltr577_i2c_probe,
	.remove     = ltr577_i2c_remove,
	.detect     = ltr577_i2c_detect,
	.id_table   = ltr577_i2c_id,
	.driver = {
		.name           = LTR577_DEV_NAME,
#ifdef CONFIG_OF
		.of_match_table = alsps_of_match,
#endif

#ifdef CONFIG_PM_SLEEP
		.pm = &ltr577_pm_ops,
#endif
	},
};

/*----------------------------------------------------------------------------*/
#ifdef GN_MTK_BSP_PS_DYNAMIC_CALI
#ifndef HW_dynamic_cali
	static int ltr577_dynamic_calibrate(void);
	static int dynamic_calibrate = 0;
#endif
#endif

#define ontim_debug_info

#ifdef ontim_debug_info
#include <ontim/ontim_dev_dgb.h>
static char ltr577_prox_version[]="ltr577_mtk_1.0";
static char ltr577_prox_vendor_name[20] = {0};
	DEV_ATTR_DECLARE(als_prox)
	DEV_ATTR_DEFINE("version",ltr577_prox_version)
	DEV_ATTR_DEFINE("vendor",ltr577_prox_vendor_name)
	DEV_ATTR_DECLARE_END;
	ONTIM_DEBUG_DECLARE_AND_INIT(als_prox,als_prox,8);

#endif


/*-----------------------------------------------------------------------------*/

/* 
 * #########
 * ## I2C ##
 * #########
 */
static int ltr577_i2c_read_block(struct i2c_client *client, u8 addr, u8 *data, u8 len)
{
	int err;
	u8 beg = addr;
	struct i2c_msg msgs[2] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = 1,
			.buf = &beg, },
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = len,
			.buf = data, }
	};

	mutex_lock(&ltr577_i2c_mutex);
	if (!client) {
		mutex_unlock(&ltr577_i2c_mutex);
		return -EINVAL;
	}
	else if (len > C_I2C_FIFO_SIZE) {
		mutex_unlock(&ltr577_i2c_mutex);
		APS_ERR(" length %d exceeds %d\n", len, C_I2C_FIFO_SIZE);
		return -EINVAL;
	}

	err = i2c_transfer(client->adapter, msgs, sizeof(msgs) / sizeof(msgs[0]));
	mutex_unlock(&ltr577_i2c_mutex);
	if (err != 2) {
		APS_ERR("i2c_transfer error: (%d %p %d) %d\n", addr, data, len, err);
		err = -EIO;
	}
	else {
		err = 0;	/*no error */
	}
	return err;
}

static int ltr577_i2c_write_block(struct i2c_client *client, u8 addr, u8 *data, u8 len)
{
	int err, idx, num;
	char buf[C_I2C_FIFO_SIZE];

	err = 0;
	mutex_lock(&ltr577_i2c_mutex);
	if (!client) {
		mutex_unlock(&ltr577_i2c_mutex);
		return -EINVAL;
	}
	else if (len >= C_I2C_FIFO_SIZE) {
		APS_ERR(" length %d exceeds %d\n", len, C_I2C_FIFO_SIZE);
		mutex_unlock(&ltr577_i2c_mutex);
		return -EINVAL;
	}

	num = 0;
	buf[num++] = addr;
	for (idx = 0; idx < len; idx++) {
		buf[num++] = data[idx];
	}

	err = i2c_master_send(client, buf, num);
	if (err < 0) {
		APS_ERR("send command error!!\n");
		mutex_unlock(&ltr577_i2c_mutex);
		return -EFAULT;
	}
	mutex_unlock(&ltr577_i2c_mutex);
	return err;
}

/*----------------------------------------------------------------------------*/
static int ltr577_master_recv(struct i2c_client *client, u16 addr, u8 *buf, int count)
{
	int ret = 0, retry = 0;
	int trc = atomic_read(&ltr577_obj->trace);
	int max_try = atomic_read(&ltr577_obj->i2c_retry);

	while (retry++ < max_try) {
		ret = ltr577_i2c_read_block(client, addr, buf, count);
		if (ret == 0)
			break;
		udelay(100);
	}

	if (unlikely(trc)) {
		if ((retry != 1) && (trc & 0x8000)) {
			APS_LOG("(recv) %d/%d\n", retry - 1, max_try);
		}
	}

	/* If everything went ok (i.e. 1 msg transmitted), return #bytes
	transmitted, else error code. */
	return (ret == 0) ? count : ret;
}

/*----------------------------------------------------------------------------*/
static int ltr577_master_send(struct i2c_client *client, u16 addr, u8 *buf, int count)
{
	int ret = 0, retry = 0;
	int trc = atomic_read(&ltr577_obj->trace);
	int max_try = atomic_read(&ltr577_obj->i2c_retry);

	while (retry++ < max_try) {
		ret = ltr577_i2c_write_block(client, addr, buf, count);
		if (ret == 0)
			break;
		udelay(100);
	}

	if (unlikely(trc)) {
		if ((retry != 1) && (trc & 0x8000)) {
			APS_LOG("(send) %d/%d\n", retry - 1, max_try);
		}
	}
	/* If everything went ok (i.e. 1 msg transmitted), return #bytes
	transmitted, else error code. */
	return (ret == 0) ? count : ret;
}

/*----------------------------------------------------------------------------*/
static void ltr577_power(struct alsps_hw *hw, unsigned int on) 
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
			if(!hwPowerOn(hw->power_id, hw->power_vol, "ltr577")) 
			{
				APS_ERR("power on fails!!\n");
			}
		}
		else
		{
			if(!hwPowerDown(hw->power_id, "ltr577")) 
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
#ifndef HW_dynamic_cali
static int ltr577_ps_set_thres(void)
{
	int res;
	u8 databuf[2];
	
	struct i2c_client *client = ltr577_obj->client;
	struct ltr577_priv *obj = ltr577_obj;

	APS_FUN();

	APS_DBG("ps_cali.valid: %d\n", ps_cali.valid);

	if(1 == ps_cali.valid)
	{
		databuf[0] = LTR577_PS_THRES_LOW_0; 
		databuf[1] = (u8)(ps_cali.far_away & 0x00FF);
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{			
			goto EXIT_ERR;
		}
		databuf[0] = LTR577_PS_THRES_LOW_1; 
		databuf[1] = (u8)((ps_cali.far_away & 0xFF00) >> 8);
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
		}
		databuf[0] = LTR577_PS_THRES_UP_0;	
		databuf[1] = (u8)(ps_cali.close & 0x00FF);
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
		}
		databuf[0] = LTR577_PS_THRES_UP_1;	
		databuf[1] = (u8)((ps_cali.close & 0xFF00) >> 8);;
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
		}
        ps_cali.valid = 0;
	}
	else
	{
		databuf[0] = LTR577_PS_THRES_LOW_0; 
		databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_low)) & 0x00FF);
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
		}
		databuf[0] = LTR577_PS_THRES_LOW_1; 
		databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_low )>> 8) & 0x00FF);
		
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
		}
		databuf[0] = LTR577_PS_THRES_UP_0;	
		databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_high)) & 0x00FF);
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
		}
		databuf[0] = LTR577_PS_THRES_UP_1;	
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
	res = LTR577_ERR_I2C;
	return res;
}
#endif

#ifdef HW_dynamic_cali

static int ltr577_ps_set_thres(void)
{
	int res;
	u8 databuf[2];
	struct i2c_client *client = ltr577_obj->client;
	//struct ltr577_priv *obj = ltr577_obj;

	APS_DBG("ps_thd_val_low_set=%d,ps_thd_val_hlgh_set=%d\n", ps_thd_val_low_set,ps_thd_val_hlgh_set);

	databuf[0] = LTR577_PS_THRES_LOW_0; 
	databuf[1] = (u8)(ps_thd_val_low_set & 0x00FF);
	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{			
		goto EXIT_ERR;
	}
	databuf[0] = LTR577_PS_THRES_LOW_1; 
	databuf[1] = (u8)((ps_thd_val_low_set & 0xFF00) >> 8);
	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
		goto EXIT_ERR;
	}
	databuf[0] = LTR577_PS_THRES_UP_0;	
	databuf[1] = (u8)(ps_thd_val_hlgh_set & 0x00FF);
	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
		goto EXIT_ERR;
	}
	databuf[0] = LTR577_PS_THRES_UP_1;	
	databuf[1] = (u8)((ps_thd_val_hlgh_set & 0xFF00) >> 8);;
	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
		goto EXIT_ERR;
	}
	
	res = 0;
	APS_DBG("%s:set thres: %d  end !!\n",__func__,res);

	return res;
	
EXIT_ERR:
	APS_ERR("%s:set thres failed, ret:%d\n",__func__,res);
	res = LTR577_ERR_I2C;
	return res;
}

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
 *** ltr577_read_cali_file
 *****************************************/
static int ltr577_update()
{
	int ret = -1;
	ret = ltr577_read_cali_file(NULL);
	if( 0 != ret ) {
		APS_ERR("ltr577_update, refresh califile failed, ret=%d\n", ret);
	}
	return ret;
}
static int ltr577_read_cali_file(struct i2c_client *client)
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
		ps_nvraw_40mm_value = mRaw45;
		ps_nvraw_25mm_value = mRaw30;

	}else{
		ps_nvraw_none_value = default_none_value;
		ps_nvraw_40mm_value = between_40mm_none_value;
		ps_nvraw_25mm_value = between_25mm_none_value;

		APS_ERR("none_value =  %d  \n", mRaw);
	}

	return 0;

read_error:
	closeFile(fd_file);
	set_fs(oldfs);
	return (-1);
}
#endif
/*****************************************/


static int ltr577_ps_enable(struct i2c_client *client, int enable)
{
	u8 regdata;
	int err;
#ifdef PS_READ_PROINIF_VALUE
	int cali_err;
#endif

	ltr577_check_for_esd();

	if (enable != 0 && ps_enabled == 1)
	{
		APS_DBG("PS: Already enabled \n");
		return 0;
	}

	if (enable == 0 && ps_enabled == 0)
	{
		APS_DBG("PS: Already disabled \n");
		return 0;
	}

	err = ltr577_master_recv(client, LTR577_MAIN_CTRL, &regdata, 0x01);
	if (err < 0) {
		APS_DBG("i2c error: %d\n", err);
	}

	regdata &= 0xEF;	// Clear reset bit
	
	if (enable != 0) {
		APS_DBG("PS: enable ps only \n");
		regdata |= 0x01;
	}
	else {
		APS_DBG("PS: disable ps only \n");
		regdata &= 0xFE;
	}

	err = ltr577_master_send(client, LTR577_MAIN_CTRL, (char *)&regdata, 1);
	if (err < 0)
	{
		APS_ERR("PS: enable ps err: %d en: %d \n", err, enable);
		return err;
	}
	mdelay(WAKEUP_DELAY);
	err = ltr577_master_recv(client, LTR577_MAIN_CTRL, &regdata, 0x01);
	if (err < 0) {
		APS_DBG("i2c error: %d\n", err);
	}

	if (0 == ltr577_obj->hw->polling_mode_ps && enable != 0)
	{
#ifdef PS_READ_PROINIF_VALUE
		cali_err = ltr577_read_cali_file(client);
		if (cali_err < 0)
		{
			pwindows_value = between_25mm_none_value - between_40mm_none_value;
			pwave_value = between_40mm_none_value - default_none_value;
			threshold_value = default_none_value;
		}else{
			pwindows_value =ps_nvraw_25mm_value - ps_nvraw_40mm_value;
			pwave_value = ps_nvraw_40mm_value - ps_nvraw_none_value;
			threshold_value = ps_nvraw_none_value;
		}
#else
		pwindows_value = between_25mm_none_value - between_40mm_none_value;
		pwave_value = between_40mm_none_value - default_none_value;
		threshold_value = default_none_value;
#endif
		min_proximity = ltr577_obj->hw->ps_threshold_low;
		ps_threshold_l = ps_nvraw_25mm_value -1;
		ps_threshold_h = ps_nvraw_25mm_value;

		ps_thd_val_low_set = ps_threshold_l;
		ps_thd_val_hlgh_set = ps_threshold_h;
		ltr577_ps_set_thres();
		ps_enabled = 1;
	}
	else if (0 == ltr577_obj->hw->polling_mode_ps && enable == 0)
	{
		//cancel_work_sync(&ltr577_obj->eint_work);
		ps_enabled = 0;
	}
/*
	if ((irq_enabled == 1) && (enable != 0))
	{
		irq_enabled = 2;
	}
*/
	return err;
}
#else
static int ltr577_ps_enable(struct i2c_client *client, int enable)
{
	u8 regdata;
	int err;

	if (enable != 0 && ps_enabled == 1)
	{
		APS_DBG("PS: Already enabled \n");
		return 0;
	}

	if (enable == 0 && ps_enabled == 0)
	{
		APS_DBG("PS: Already disabled \n");
		return 0;
	}

	err = ltr577_master_recv(client, LTR577_MAIN_CTRL, &regdata, 0x01);
	if (err < 0) {
		APS_ERR("i2c error: %d\n", err);
	}

	regdata &= 0xEF;	// Clear reset bit
	
	if (enable != 0) {
		APS_DBG("PS: enable ps only \n");
		regdata |= 0x01;
	}
	else {
		APS_DBG("PS: disable ps only \n");
		regdata &= 0xFE;
	}

	err = ltr577_master_send(client, LTR577_MAIN_CTRL, (char *)&regdata, 1);
	if (err < 0)
	{
		APS_ERR("PS: enable ps err: %d en: %d \n", err, enable);
		return err;
	}
	mdelay(WAKEUP_DELAY);
	err = ltr577_master_recv(client, LTR577_MAIN_CTRL, &regdata, 0x01);
	if (err < 0) {
		APS_ERR("i2c error: %d\n", err);
	}
	
	if (0 == ltr577_obj->hw->polling_mode_ps && enable != 0)
	{
#ifndef DELAYED_PS_CALI
#ifdef GN_MTK_BSP_PS_DYNAMIC_CALI
		err = ltr577_dynamic_calibrate();
		if (err < 0)
		{
			APS_ERR("ltr577_dynamic_calibrate() failed\n");
		}
#endif
		ltr577_ps_set_thres();
#else
		cancel_delayed_work(&ltr577_obj->cali_ps_work);
		schedule_delayed_work(&ltr577_obj->cali_ps_work, msecs_to_jiffies(200));
#endif
	}
	else if (0 == ltr577_obj->hw->polling_mode_ps && enable == 0)
	{
		//cancel_work_sync(&ltr577_obj->eint_work);		
	}

	if (enable != 0)
		ps_enabled = 1;
	else
		ps_enabled = 0;
/*
	if ((irq_enabled == 1) && (enable != 0))
	{
		irq_enabled = 2;
	}
*/
	return err;
}
#endif
/********************************************************************/
static int ltr577_ps_read(struct i2c_client *client, u16 *data)
{
	int psdata, ret = 0;
	u8 buf[2];

	ret = ltr577_master_recv(client, LTR577_PS_DATA_0, buf, 0x02);
	if (ret < 0) {
		APS_DBG("i2c error: %d\n", ret);
	}

	APS_DBG("ps_rawdata_lo = %d\n", buf[0]);	
    APS_DBG("ps_rawdata_hi = %d\n", buf[1]);	
	
	psdata = ((buf[1] & 0x07) << 8) | (buf[0]);
	*data = psdata;
#ifndef SMT_MODE
    APS_LOG("ps_rawdata = %d\n", psdata);
#endif
	final_prox_val = psdata;	
	return psdata;
}

#ifdef GN_MTK_BSP_PS_DYNAMIC_CALI
#ifndef HW_dynamic_cali
static int ltr577_dynamic_calibrate(void)
{
	int i = 0;
	int data;
	int data_total = 0;
	int noise = 0;
	int count = 5;
	int ps_thd_val_low, ps_thd_val_high;
	struct ltr577_priv *obj = ltr577_obj;

	if (!ltr577_obj)
	{
		APS_ERR("ltr577_obj is null!!\n");
		return -1;
	}	

	for (i = 0; i < count; i++) {
		// wait for ps value be stable
		msleep(60);

		data = ltr577_ps_read(ltr577_obj->client, &ltr577_obj->ps);
		if (data < 0) {
			i--;
			continue;
		}

		if (data & 0x0800) {
			break;
		}		

		data_total += data;
	}

	noise = data_total / count;
	dynamic_calibrate = noise;

	if (noise < 100) {
		ps_thd_val_high = noise + 100;
		ps_thd_val_low  = noise + 50;
	}
	else if (noise < 200) {
		ps_thd_val_high = noise + 150;
		ps_thd_val_low  = noise + 60;
	}
	else if (noise < 300) {
		ps_thd_val_high = noise + 150;
		ps_thd_val_low  = noise + 60;
	}
	else if (noise < 400) {
		ps_thd_val_high = noise + 150;
		ps_thd_val_low  = noise + 60;
	}
	else if (noise < 600) {
		ps_thd_val_high = noise + 180;
		ps_thd_val_low  = noise + 90;
	}
	else if (noise < 1000) {
		ps_thd_val_high = noise + 300;
		ps_thd_val_low  = noise + 180;
	}
	else {
		ps_thd_val_high = 1600;
		ps_thd_val_low  = 1400;		
	}

	atomic_set(&obj->ps_thd_val_high, ps_thd_val_high);
	atomic_set(&obj->ps_thd_val_low, ps_thd_val_low);
    ps_cali.valid = 1;
    ps_cali.far_away = ps_thd_val_low;
    ps_cali.close = ps_thd_val_high;
	
	APS_DBG("%s:noise = %d\n", __func__, noise);
	APS_DBG("%s:obj->ps_thd_val_high = %d\n", __func__, ps_thd_val_high);
	APS_DBG("%s:obj->ps_thd_val_low = %d\n", __func__, ps_thd_val_low);

	return 0;
}
#endif
#endif
/********************************************************************/
/* 
 * ################
 * ## ALS CONFIG ##
 * ################
 */

static int ltr577_als_enable(struct i2c_client *client, int enable)
{
	int err = 0;
	u8 regdata = 0;

	if (enable != 0 && als_enabled == 1)
	{
		APS_DBG("ALS: Already enabled \n");
		//return 0;
	}

	if (enable == 0 && als_enabled == 0)
	{
		APS_DBG("ALS: Already disabled \n");
		return 0;
	}
#ifdef NO_ALS_CTRL_WHEN_PS_ENABLED
	if ((ps_enabled == 1)&&(enable == 0))
	{
		APS_DBG("ALS: PS enabled, do nothing \n");
		return 0;
	}
#endif
	err = ltr577_master_recv(client, LTR577_MAIN_CTRL, &regdata, 0x01);
	if (err < 0) {
		APS_DBG("i2c error: %d\n", err);
	}

	regdata &= 0xEF;	// Clear reset bit
	
	if (enable != 0) {
		APS_DBG("ALS(1): enable als only \n");
		regdata |= 0x02;
	}
	else {
		APS_DBG("ALS(1): disable als only \n");
		regdata &= 0xFD;
	}

	err = ltr577_master_send(client, LTR577_MAIN_CTRL, (char *)&regdata, 1);
	if (err < 0)
	{
		APS_ERR("ALS: enable als err: %d en: %d \n", err, enable);
		return err;
	}

	mdelay(WAKEUP_DELAY);

	if (enable != 0)
		als_enabled = 1;
	else
		als_enabled = 0;

	return 0;
}

static int ltr577_als_read(struct i2c_client *client, u16* data)
{
	int alsval = 0, clearval = 0;
	int luxdata_int;	
	u8 buf[3];
	int ret;
	int ratio= 0;

	ltr577_check_for_esd();

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
			goto als_defalut;
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



als_defalut:
	ret = ltr577_master_recv(client, LTR577_ALS_DATA_0, buf, 0x03);
	if (ret < 0) {
		APS_ERR("i2c error: %d\n", ret);
	}

	alsval = (buf[2] * 256 * 256) + (buf[1] * 256) + buf[0];
	APS_DBG("alsval_0 = %d,alsval_1=%d,alsval_2=%d,alsval=%d\n", buf[0], buf[1], buf[2], alsval);	

	ret = ltr577_master_recv(client, LTR577_CLEAR_DATA_0, buf, 0x03);
	if (ret < 0) {
		APS_ERR("i2c error: %d\n", ret);
	}

	clearval = (buf[2] * 256 * 256) + (buf[1] * 256) + buf[0];
	APS_DBG("clearval_0 = %d,clearval_1=%d,clearval_2=%d,clearval=%d\n", buf[0], buf[1], buf[2], clearval);

	if (alsval == 0)
	{
		luxdata_int = 0;
		goto out;
	}

	ratio = 10*clearval/alsval;
	cureent_color_ratio = ratio;
	
	if(ratio > 15){
		luxdata_int = (alsval*coeff_als_for_tp[0])/300;
		current_color_temp = A_TEMP;
	}
	else if(ratio > 58){
		luxdata_int = (alsval*coeff_als_for_tp[1])/300;
		current_color_temp = D65_TEMP;
	}else{
		luxdata_int = (alsval*coeff_als_for_tp[2])/300;
		current_color_temp = CWF_TEMP;
	}
	
	APS_DBG("ltr577_als_read: als_value_lux = %d,current_color_temp = %d\n", luxdata_int,current_color_temp);
out:
	*data = luxdata_int;
	final_lux_val = luxdata_int;
	return luxdata_int;
}
/********************************************************************/
#ifdef HW_dynamic_cali

static int ltr577_get_ps_value(struct ltr577_priv *obj, u16 ps)
{
	int ps_data = ps;
	if(((ps_data+pwave_value)<min_proximity)&&(ps_data>0))
	{
		min_proximity = ps_data+pwave_value;

		ps_threshold_l = FAR_THRESHOLD(threshold_value);
		ps_threshold_h = NRAR_THRESHOLD(threshold_value);

		ps_thd_val_low_set = ps_threshold_l;
		ps_thd_val_hlgh_set = ps_threshold_h;
		APS_DBG("ps_data= %d, min_proximity=%d  threshold_value=%d th_l =%d,th_h =%d\n", ps_data, min_proximity, threshold_value, ps_thd_val_low_set,ps_thd_val_hlgh_set);
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
static int intr_flag_value = 0;
static int ltr577_get_ps_value(struct ltr577_priv *obj, u16 ps)
{
	int val = 1;
	//int invalid = 0;
#if 1
	u8 buffer = 0;
	int ps_flag;
	int ret;

	ret = ltr577_master_recv(ltr577_obj->client, LTR577_MAIN_STATUS, &buffer, 0x01);
	if (ret < 0) {
		APS_ERR("i2c error: %d\n", ret);
		return -1;
	}	

	ps_flag = buffer & 0x04;
	ps_flag = ps_flag >> 2;
	if (ps_flag == 1) //Near
	{
		intr_flag_value = 1;
		val = 0;
	}
	else if (ps_flag == 0) //Far
	{
		intr_flag_value = 0;
		val = 1;
	}

	return val;
#else
	if((ps > atomic_read(&obj->ps_thd_val_high)))
	{
		val = 0;  /*close*/
		intr_flag_value = 1;
	}
	else if((ps < atomic_read(&obj->ps_thd_val_low)))
	{
		val = 1;  /*far away*/
		intr_flag_value = 0;
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
	else if (obj->als > 50000)
	{
		//invalid = 1;
		APS_DBG("ligh too high will result to failt proximiy\n");
		return 1;  /*far away*/
	}

	if(!invalid)
	{
		APS_DBG("PS:  %05d => %05d\n", ps, val);
		return val;
	}	
	else
	{
		return -1;
	}
#endif
}
#endif
/********************************************************************/
static int ltr577_get_als_value(struct ltr577_priv *obj, u16 als)
{
	int idx =0;
	int invalid = 0;
 	unsigned int lum;
	APS_DBG("als  = %d\n",als); 
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
		APS_ERR(" %d ,exceed range\n",__LINE__);
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

	APS_DBG("%s, lum =%d \n",__func__,lum);
		return lum;
		//return obj->hw->als_value[idx];
	}
	else
	{
	//	APS_ERR("ALS: %05d => %05d (-1)\n", als, obj->hw->als_value[idx]);    
		return -1;
	}
}
/*-------------------------------attribute file for debugging----------------------------------*/

/******************************************************************************
 * Sysfs attributes
*******************************************************************************/
static ssize_t ltr577_show_config(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	
	if(!ltr577_obj)
	{
		APS_ERR("ltr577_obj is null!!\n");
		return 0;
	}
	
	res = snprintf(buf, PAGE_SIZE, "(%d %d %d %d %d)\n", 
		atomic_read(&ltr577_obj->i2c_retry), atomic_read(&ltr577_obj->als_debounce), 
		atomic_read(&ltr577_obj->ps_mask), atomic_read(&ltr577_obj->ps_thd_val), atomic_read(&ltr577_obj->ps_debounce));     
	return res;    
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr577_store_config(struct device_driver *ddri, const char *buf, size_t count)
{
	int retry, als_deb, ps_deb, mask, thres;
	if(!ltr577_obj)
	{
		APS_ERR("ltr577_obj is null!!\n");
		return 0;
	}
	
	if(5 == sscanf(buf, "%d %d %d %d %d", &retry, &als_deb, &mask, &thres, &ps_deb))
	{ 
		atomic_set(&ltr577_obj->i2c_retry, retry);
		atomic_set(&ltr577_obj->als_debounce, als_deb);
		atomic_set(&ltr577_obj->ps_mask, mask);
		atomic_set(&ltr577_obj->ps_thd_val, thres);        
		atomic_set(&ltr577_obj->ps_debounce, ps_deb);
	}
	else
	{
		APS_ERR("invalid content: '%s', length = %zu\n", buf, count);
	}
	return count;    
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr577_show_trace(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	if(!ltr577_obj)
	{
		APS_ERR("ltr577_obj is null!!\n");
		return 0;
	}

	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&ltr577_obj->trace));     
	return res;    
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr577_store_trace(struct device_driver *ddri, const char *buf, size_t count)
{
    int trace;
    if(!ltr577_obj)
	{
		APS_ERR("ltr577_obj is null!!\n");
		return 0;
	}
	
	if(1 == sscanf(buf, "0x%x", &trace))
	{
		atomic_set(&ltr577_obj->trace, trace);
	}
	else 
	{
		APS_ERR("invalid content: '%s', length = %zu\n", buf, count);
	}
	return count;    
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr577_show_als(struct device_driver *ddri, char *buf)
{
	int res;
		
	if(!ltr577_obj)
	{
		APS_ERR("ltr577_obj is null!!\n");
		return 0;
	}
	res = ltr577_als_read(ltr577_obj->client, &ltr577_obj->als);
	return snprintf(buf, PAGE_SIZE, "0x%04X(%d)\n", res, res);
	
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr577_show_ps(struct device_driver *ddri, char *buf)
{
	int  res;
	if(!ltr577_obj)
	{
		APS_ERR("ltr577_obj is null!!\n");
		return 0;
	}
	res = ltr577_ps_read(ltr577_obj->client, &ltr577_obj->ps);
    return snprintf(buf, PAGE_SIZE, "0x%04X(%d)\n", res, res);
}


/*----------------------------------------------------------------------------*/
#if 0
static ssize_t ltr578_show_ps_info(struct device_driver *ddri, char *buf)
{
		if (!ltr578_obj) {
				APS_ERR("ltr578_obj is null!!\n");
				return 0;
		}
		return snprintf(buf, PAGE_SIZE, "PS_THRED:%d -> %d,PS_CALI:%d\n", \
						atomic_read(&ltr578_obj->ps_thd_val_low), atomic_read(&ltr578_obj->ps_thd_val_high), dynamic_cali);
}
#endif
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
static ssize_t ltr577_show_reg(struct device_driver *ddri, char *buf)
{
	int i,len=0;
	int reg[]={0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0d,0x0e,0x0f,
			   0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20,0x21,0x22,0x23,0x24,0x25,0x26};
	int ret;
	u8 buffer;

	for(i=0;i<27;i++)
	{
		ret = ltr577_master_recv(ltr577_obj->client, reg[i], &buffer, 0x01);
		if (ret < 0) {
			APS_DBG("i2c error: %d\n", ret);
		}
		len += snprintf(buf + len, PAGE_SIZE - len, "reg:0x%04X value: 0x%04X\n", reg[i], buffer);
	}
	return len;
}

#ifdef LTR577_DEBUG
static int ltr577_dump_reg(void)
{
	int i=0;
	int reg[]={0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0d,0x0e,0x0f,
		       0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26};
	int ret;
	u8 buffer;

	for(i=0;i<27;i++)
	{
		ret = ltr577_master_recv(ltr577_obj->client, reg[i], &buffer, 0x01);
		if (ret < 0) {
			APS_DBG("i2c error: %d\n", ret);
		}

		APS_DBG("reg:0x%04X value: 0x%04X\n", reg[i], buffer);
	}
	return 0;
}
#endif

/*----------------------------------------------------------------------------*/
static ssize_t ltr577_show_send(struct device_driver *ddri, char *buf)
{
    return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr577_store_send(struct device_driver *ddri, const char *buf, size_t count)
{
	int addr, cmd;
	u8 dat;

	if(!ltr577_obj)
	{
		APS_ERR("ltr577_obj is null!!\n");
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
static ssize_t ltr577_show_recv(struct device_driver *ddri, char *buf)
{
    return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr577_store_recv(struct device_driver *ddri, const char *buf, size_t count)
{
	int addr;
	
	if(!ltr577_obj)
	{
		APS_ERR("ltr577_obj is null!!\n");
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
static ssize_t ltr577_show_status(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	
	if(!ltr577_obj)
	{
		APS_ERR("ltr577_obj is null!!\n");
		return 0;
	}
	
	if(ltr577_obj->hw)
	{	
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: %d, (%d %d)\n", 
			ltr577_obj->hw->i2c_num, ltr577_obj->hw->power_id, ltr577_obj->hw->power_vol);		
	}
	else
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: NULL\n");
	}


	len += snprintf(buf+len, PAGE_SIZE-len, "MISC: %d %d\n", atomic_read(&ltr577_obj->als_suspend), atomic_read(&ltr577_obj->ps_suspend));

	return len;
}
/*----------------------------------------------------------------------------*/
#if 0
#define IS_SPACE(CH) (((CH) == ' ') || ((CH) == '\n'))
/*----------------------------------------------------------------------------*/
static int read_int_from_buf(struct ltr577_priv *obj, const char* buf, size_t count, u32 data[], int len)
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
static ssize_t ltr577_show_alslv(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	int idx;
	if(!ltr577_obj)
	{
		APS_ERR("ltr577_obj is null!!\n");
		return 0;
	}
	
	//for(idx = 0; idx < ltr577_obj->als_level_num; idx++)
	for(idx = 0; idx < C_CUST_ALS_LEVEL; idx++)
	{
		//len += snprintf(buf+len, PAGE_SIZE-len, "%d ", ltr577_obj->hw->als_level[idx]);
		len += scnprintf(buf+len, PAGE_SIZE-len, "%d ", als_level[current_tp][current_color_temp][idx]);
	}
	len += snprintf(buf+len, PAGE_SIZE-len, "\n");
	return len;    
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr577_store_alslv(struct device_driver *ddri, const char *buf, size_t count)
{
	if(!ltr577_obj)
	{
		APS_ERR("ltr577_obj is null!!\n");
		return 0;
	}
/*
	else if(!strcmp(buf, "def"))
	{
		memcpy(ltr577_obj->als_level, ltr577_obj->hw->als_level, sizeof(ltr577_obj->als_level));
	}
	else if(ltr577_obj->als_level_num != read_int_from_buf(ltr577_obj, buf, count, 
			ltr577_obj->hw->als_level, ltr577_obj->als_level_num))
	{
		APS_ERR("invalid format: '%s'\n", buf);
	}    */
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr577_show_alsval(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	int idx;
	if(!ltr577_obj)
	{
		APS_ERR("ltr577_obj is null!!\n");
		return 0;
	}
	
	//for(idx = 0; idx < ltr577_obj->als_value_num; idx++)
	for(idx = 0; idx < C_CUST_ALS_LEVEL; idx++)
	{
		//len += snprintf(buf+len, PAGE_SIZE-len, "%d ", ltr577_obj->hw->als_value[idx]);
		len += scnprintf(buf+len, PAGE_SIZE-len, "%d ", als_value[idx]);
	}
	len += snprintf(buf+len, PAGE_SIZE-len, "\n");
	return len;    
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr577_store_alsval(struct device_driver *ddri, const char *buf, size_t count)
{
	if(!ltr577_obj)
	{
		APS_ERR("ltr577_obj is null!!\n");
		return 0;
	}
/*
	else if(!strcmp(buf, "def"))
	{
		memcpy(ltr577_obj->als_value, ltr577_obj->hw->als_value, sizeof(ltr577_obj->als_value));
	}
	else if(ltr577_obj->als_value_num != read_int_from_buf(ltr577_obj, buf, count, 
			ltr577_obj->hw->als_value, ltr577_obj->als_value_num))
	{
		APS_ERR("invalid format: '%s'\n", buf);
	}    */
	return count;
}
/*---------------------------------------------------------------------------------------*/
static DRIVER_ATTR(als,     S_IWUSR | S_IRUGO, ltr577_show_als,		NULL);
static DRIVER_ATTR(ps,      S_IWUSR | S_IRUGO, ltr577_show_ps,		NULL);
// static DRIVER_ATTR(ps_info, S_IRUGO, ltr578_show_ps_info,	NULL);
static DRIVER_ATTR(config,  S_IWUSR | S_IRUGO, ltr577_show_config,	ltr577_store_config);
static DRIVER_ATTR(alslv,   S_IWUSR | S_IRUGO, ltr577_show_alslv,	ltr577_store_alslv);
static DRIVER_ATTR(alsval,  S_IWUSR | S_IRUGO, ltr577_show_alsval,	ltr577_store_alsval);
static DRIVER_ATTR(trace,   S_IWUSR | S_IRUGO, ltr577_show_trace,	ltr577_store_trace);
static DRIVER_ATTR(status,  S_IWUSR | S_IRUGO, ltr577_show_status,	NULL);
static DRIVER_ATTR(send,    S_IWUSR | S_IRUGO, ltr577_show_send,	ltr577_store_send);
static DRIVER_ATTR(recv,    S_IWUSR | S_IRUGO, ltr577_show_recv,	ltr577_store_recv);
static DRIVER_ATTR(reg,     S_IWUSR | S_IRUGO, ltr577_show_reg,		NULL);
/*----------------------------------------------------------------------------*/
static struct driver_attribute *ltr577_attr_list[] = {
    &driver_attr_als,
    &driver_attr_ps,
//	&driver_attr_ps_info,    
    &driver_attr_trace,        /*trace log*/
    &driver_attr_config,
    &driver_attr_alslv,
    &driver_attr_alsval,
    &driver_attr_status,
    &driver_attr_send,
    &driver_attr_recv,
    &driver_attr_reg,
};

/*----------------------------------------------------------------------------*/
static int ltr577_create_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(sizeof(ltr577_attr_list)/sizeof(ltr577_attr_list[0]));

	if (driver == NULL)
	{
		return -EINVAL;
	}

	for(idx = 0; idx < num; idx++)
	{
		if((err = driver_create_file(driver, ltr577_attr_list[idx])))
		{            
			APS_ERR("driver_create_file (%s) = %d\n", ltr577_attr_list[idx]->attr.name, err);
			break;
		}
	}    
	return err;
}
/*----------------------------------------------------------------------------*/
static int ltr577_delete_attr(struct device_driver *driver)
{
	int idx ,err = 0;
	int num = (int)(sizeof(ltr577_attr_list)/sizeof(ltr577_attr_list[0]));

	if (!driver)
		return -EINVAL;

	for (idx = 0; idx < num; idx++) 
	{
		driver_remove_file(driver, ltr577_attr_list[idx]);
	}
	
	return err;
}
/*----------------------------------------------------------------------------*/

/*----------------------------------interrupt functions--------------------------------*/
#ifdef REPORT_PS_ONCE_WHEN_ALS_ENABLED
static void ltr577_check_ps_work(struct work_struct *work)
{
	struct ltr577_priv *obj = ltr577_obj;	
	struct hwm_sensor_data sensor_data;
	APS_FUN();

	if (test_bit(CMC_BIT_PS, &obj->enable)) {
		ltr577_ps_read(obj->client, &obj->ps);
		APS_LOG("ltr577_check_ps_work rawdata ps=%d high=%d low=%d\n", obj->ps, atomic_read(&obj->ps_thd_val_high), atomic_read(&obj->ps_thd_val_low));

		sensor_data.values[0] = ltr577_get_ps_value(obj, obj->ps);
		sensor_data.value_divide = 1;
		sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;

		if (ps_report_interrupt_data(sensor_data.values[0]))
		{
			APS_ERR("call ps_report_interrupt_data fail \n");
		}
	}

	return;
}
#endif

#ifdef DELAYED_PS_CALI
static void ltr577_cali_ps_work(struct work_struct *work)
{
	int err = 0;

#ifdef GN_MTK_BSP_PS_DYNAMIC_CALI
	err = ltr577_dynamic_calibrate();
	if (err < 0)
	{
		APS_ERR("ltr577_dynamic_calibrate() failed\n");
	}
#endif		
	ltr577_ps_set_thres();
}
#endif
/*----------------------------------------------------------------------------*/
#ifdef HW_dynamic_cali
#ifndef SMT_MODE
static void ltr577_clear_intr(struct i2c_client *client)
{
	int ret = 0;
	u8 regdata = 0;

	ret = ltr577_master_recv(client, LTR577_MAIN_STATUS, &regdata, 0x01);
	if (ret < 0)
	{
		APS_ERR("clear PS interrupt failed, ret = %d\n", ret);
	}
	APS_DBG("clear PS interrupt, reg:0x%x\n", regdata);

	return;
}
#endif //SMT_MODE

static void ltr577_eint_work(struct work_struct *work)
{
	struct ltr577_priv *obj = (struct ltr577_priv *)container_of(work, struct ltr577_priv, eint_work);
	int res = 0;

	int distance =-1;

	//clear interrupt for ltr578 by zhuhui,20190816
#ifndef SMT_MODE
	ltr577_clear_intr(obj->client);
#endif

	//get raw data
	obj->ps = ltr577_ps_read(obj->client, &obj->ps);
	if (obj->ps < 0)
	{
		goto EXIT_INTR;
	}

	APS_DBG("ltr577_eint_work: rawdata ps=%d!\n",obj->ps);
	distance = ltr577_get_ps_value(obj, obj->ps);

	APS_DBG("%s:let up distance=%d\n",__func__,distance);

#ifdef SMT_MODE
	ps_thd_val_low_set = ps_nvraw_40mm_value;
	ps_thd_val_hlgh_set = ps_nvraw_25mm_value;
#endif

	ltr577_ps_set_thres();

	//let up layer to know
	res = ps_report_interrupt_data(distance);

#ifdef SMT_MODE
	mdelay(300);
#endif

EXIT_INTR:
#ifdef CONFIG_OF
	enable_irq_wake(obj->irq);
	enable_irq(obj->irq);
#endif
}

#else
static void ltr577_eint_work(struct work_struct *work)
{
	struct ltr577_priv *obj = (struct ltr577_priv *)container_of(work, struct ltr577_priv, eint_work);
	int res = 0;
	int value = 1;
#ifdef UPDATE_PS_THRESHOLD
	u8 databuf[2];
#endif

	//get raw data
	obj->ps = ltr577_ps_read(obj->client, &obj->ps);
	if (obj->ps < 0)
	{
		goto EXIT_INTR;
	}
				
	APS_DBG("ltr577_eint_work: rawdata ps=%d!\n",obj->ps);
	value = ltr577_get_ps_value(obj, obj->ps);
	APS_DBG("intr_flag_value=%d\n",intr_flag_value);
	if(intr_flag_value){
#ifdef UPDATE_PS_THRESHOLD
		databuf[0] = LTR577_PS_THRES_LOW_0;	
		databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_low)) & 0x00FF);
		res = i2c_master_send(obj->client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_INTR;
		}
		databuf[0] = LTR577_PS_THRES_LOW_1;	
		databuf[1] = (u8)(((atomic_read(&obj->ps_thd_val_low)) & 0xFF00) >> 8);
		res = i2c_master_send(obj->client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_INTR;
		}
		databuf[0] = LTR577_PS_THRES_UP_0;	
		databuf[1] = (u8)(0x00FF);
		res = i2c_master_send(obj->client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_INTR;
		}
		databuf[0] = LTR577_PS_THRES_UP_1; 
		databuf[1] = (u8)((0xFF00) >> 8);;
		res = i2c_master_send(obj->client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_INTR;
		}
#endif
	}
	else{	
#ifndef DELAYED_PS_CALI
#ifdef GN_MTK_BSP_PS_DYNAMIC_CALI
		if(obj->ps > 20 && obj->ps < (dynamic_calibrate - 50)){ 
        	if(obj->ps < 100){			
        		atomic_set(&obj->ps_thd_val_high,  obj->ps+100);
        		atomic_set(&obj->ps_thd_val_low, obj->ps+50);
        	}else if(obj->ps < 200){
        		atomic_set(&obj->ps_thd_val_high,  obj->ps+150);
        		atomic_set(&obj->ps_thd_val_low, obj->ps+60);
        	}else if(obj->ps < 300){
        		atomic_set(&obj->ps_thd_val_high,  obj->ps+150);
        		atomic_set(&obj->ps_thd_val_low, obj->ps+60);
        	}else if(obj->ps < 400){
        		atomic_set(&obj->ps_thd_val_high,  obj->ps+150);
        		atomic_set(&obj->ps_thd_val_low, obj->ps+60);
        	}else if(obj->ps < 600){
        		atomic_set(&obj->ps_thd_val_high,  obj->ps+180);
        		atomic_set(&obj->ps_thd_val_low, obj->ps+90);
        	}else if(obj->ps < 1000){
        		atomic_set(&obj->ps_thd_val_high,  obj->ps+300);
        		atomic_set(&obj->ps_thd_val_low, obj->ps+180);	
        	}else if(obj->ps < 1250){
        		atomic_set(&obj->ps_thd_val_high,  obj->ps+400);
        		atomic_set(&obj->ps_thd_val_low, obj->ps+300);
        	}
        	else{
        		atomic_set(&obj->ps_thd_val_high,  1400);
        		atomic_set(&obj->ps_thd_val_low, 1000);        			
        	}
        		
        	dynamic_calibrate = obj->ps;
        }	        
#endif
#ifdef UPDATE_PS_THRESHOLD
		databuf[0] = LTR577_PS_THRES_LOW_0;	
		databuf[1] = (u8)(0 & 0x00FF);
		//databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_low)) & 0x00FF);
		res = i2c_master_send(obj->client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_INTR;
		}
		databuf[0] = LTR577_PS_THRES_LOW_1;
		databuf[1] = (u8)((0 & 0xFF00) >> 8);
		//databuf[1] = (u8)(((atomic_read(&obj->ps_thd_val_low)) & 0xFF00) >> 8);
		res = i2c_master_send(obj->client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_INTR;
		}
		databuf[0] = LTR577_PS_THRES_UP_0;	
		databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_high)) & 0x00FF);
		res = i2c_master_send(obj->client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_INTR;
		}
		databuf[0] = LTR577_PS_THRES_UP_1; 
		databuf[1] = (u8)(((atomic_read(&obj->ps_thd_val_high)) & 0xFF00) >> 8);;
		res = i2c_master_send(obj->client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_INTR;
		}
#endif
#else
		cancel_delayed_work(&ltr577_obj->cali_ps_work);
		schedule_delayed_work(&ltr577_obj->cali_ps_work, msecs_to_jiffies(2000));
#endif
	}
	//let up layer to know
	res = ps_report_interrupt_data(value);

EXIT_INTR:	
#ifdef CONFIG_OF
	enable_irq_wake(obj->irq);
	enable_irq(obj->irq);
#endif
}
#endif
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static void ltr577_eint_func(void)
{
	struct ltr577_priv *obj = ltr577_obj;
	if(!obj)
	{
		return;
	}	
	int_top_time = sched_clock();
	schedule_work(&obj->eint_work);
}
#ifdef CONFIG_OF
static irqreturn_t ltr577_eint_handler(int irq, void *desc)
{	
	//if (irq_enabled == 2)
	{
		disable_irq_nosync(ltr577_obj->irq);
		disable_irq_wake(ltr577_obj->irq);
		ltr577_eint_func();
	}

	return IRQ_HANDLED;
}
#endif
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
int ltr577_setup_eint(struct i2c_client *client)
{
	int ret;
        struct pinctrl *pinctrl;
        struct pinctrl_state *pins_default;
        struct pinctrl_state *pins_cfg;

	u32 ints[2] = { 0, 0 };	

//	struct ltr577_priv *obj = i2c_get_clientdata(client);


if (ps_irq_use_old){

	alspsPltFmDev = get_alsps_platformdev();
	ltr577_obj->irq_node = of_find_compatible_node(NULL,NULL,"mediatek,als-eint");

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

	/* eint request */
	if (ltr577_obj->irq_node) {

		of_property_read_u32_array(ltr577_obj->irq_node, "debounce", ints, ARRAY_SIZE(ints));

		gpio_request(ints[0], "p-sensor");

		gpio_set_debounce(ints[0], ints[1]);

		pinctrl_select_state(pinctrl, pins_cfg);
		APS_DBG("ints[0] = %d, ints[1] = %d!!\n", ints[0], ints[1]);

		ltr577_obj->irq = irq_of_parse_and_map(ltr577_obj->irq_node, 0);

		APS_LOG("ltr577_obj->irq = %d\n", ltr577_obj->irq);

		if (!ltr577_obj->irq) {
			APS_ERR("irq_of_parse_and_map fail!!\n");
			return -EINVAL;
		}
		if (request_irq(ltr577_obj->irq, ltr577_eint_handler, IRQF_TRIGGER_LOW, "ALS-eint", NULL)) {
			APS_ERR("IRQ LINE NOT AVAILABLE!!\n");
			return -EINVAL;
		}
		enable_irq_wake(ltr577_obj->irq);
		//enable_irq(ltr577_obj->irq);
		//irq_enabled = 1;
	}
	else {
		APS_ERR("null irq node!!\n");
		return -EINVAL;
	}
}else{

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
	if (ltr577_obj->irq_node) {

		ret = of_property_read_u32_array(ltr577_obj->irq_node, "debounce", ints, ARRAY_SIZE(ints));
		if (ret) {
			APS_ERR("of_property_read_u32_array fail, ret = %d\n", ret);

		}
		gpio_set_debounce(ints[0], ints[1]);
		APS_DBG("ints[0] = %d, ints[1] = %d!!\n", ints[0], ints[1]);

		ltr577_obj->irq = irq_of_parse_and_map(ltr577_obj->irq_node, 0);
		APS_ERR("ltr577_obj->irq = %d\n", ltr577_obj->irq);
		if (!ltr577_obj->irq) {
			APS_ERR("irq_of_parse_and_map fail!!\n");
			return -EINVAL;
		}
		if (request_irq(ltr577_obj->irq, ltr577_eint_handler, IRQF_TRIGGER_LOW, "ALS-eint", NULL)) {
			APS_ERR("IRQ LINE NOT AVAILABLE!!\n");
			return -EINVAL;
		}
		enable_irq_wake(ltr577_obj->irq);
		//enable_irq(ltr577_obj->irq);
		//irq_enabled = 1;
	//	disable_irq(ltr577_obj->irq);
	} else {
		APS_ERR("null irq node!!\n");
		return -EINVAL;
	}
	
   }
	return 0;
}
/**********************************************************************************************/

/*-------------------------------MISC device related------------------------------------------*/
static int ltr577_open(struct inode *inode, struct file *file)
{
	file->private_data = ltr577_i2c_client;

	if (!file->private_data)
	{
		APS_ERR("null pointer!!\n");
		return -EINVAL;
	}
	return nonseekable_open(inode, file);
}
/************************************************************/
static int ltr577_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}
/************************************************************/

#ifdef CONFIG_COMPAT
static long ltr577_compat_ioctl(struct file *file, unsigned int cmd,
       unsigned long arg)
{
        long err = 0;
        void __user *arg32 = compat_ptr(arg);
        if (!file->f_op || !file->f_op->unlocked_ioctl || !arg32)
                return -ENOTTY;

        APS_LOG("stk3x1x_compat_ioctl cmd= %x\n", cmd);
        err = file->f_op->unlocked_ioctl(file, cmd, (unsigned long)arg32);

        return err;
}
#endif

static long ltr577_unlocked_ioctl(struct file *file, unsigned int cmd,
       unsigned long arg)       
{
	struct i2c_client *client = (struct i2c_client*)file->private_data;
	struct ltr577_priv *obj = i2c_get_clientdata(client);  
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
			err = ltr577_ps_enable(obj->client, enable);
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
			obj->ps = ltr577_ps_read(obj->client, &obj->ps);
			if (obj->ps < 0)
			{
				goto err_out;
			}
			
			dat = ltr577_get_ps_value(obj, obj->ps);
			if (copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}
			break;

		case ALSPS_GET_PS_RAW_DATA:    
			obj->ps = ltr577_ps_read(obj->client, &obj->ps);
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
			err = ltr577_als_enable(obj->client, enable);
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
			obj->als = ltr577_als_read(obj->client, &obj->als);
			if (obj->als < 0)
			{
				goto err_out;
			}

			dat = ltr577_get_als_value(obj, obj->als);
			if (copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}
			break;

		case ALSPS_GET_ALS_RAW_DATA:    
			obj->als = ltr577_als_read(obj->client, &obj->als);
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
			obj->ps = ltr577_ps_read(obj->client, &obj->ps);
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

			ltr577_ps_set_thres();
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
static struct file_operations ltr577_fops = {
	.owner = THIS_MODULE,
	.open = ltr577_open,
	.release = ltr577_release,
	.unlocked_ioctl = ltr577_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = ltr577_compat_ioctl,
#endif
};

static struct miscdevice ltr577_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "als_ps",
	.fops = &ltr577_fops,
};

/*--------------------------------------------------------------------------------*/
static int ltr577_get_chip_id(struct i2c_client *client , u8 *regdata)
{
	int ret = -1;

	ret = ltr577_master_recv(client, LTR577_PART_ID, regdata, 0x01);
	if (ret < 0) {
		APS_ERR("get part id failed, ret:%d\n", ret);
	}

	return ret;
}

static int ltr577_init_client(void)
{
	int res;
	int init_als_gain;
	u8 buf;

	struct i2c_client *client = ltr577_obj->client;

	struct ltr577_priv *obj = ltr577_obj;

	mdelay(PON_DELAY);

	/* ===============
	* ** IMPORTANT **
	* ===============
	* Other settings like timing and threshold to be set here, if required.
	* Not set and kept as device default for now.
	*/
	buf = PS_PULSES; // 16 pulses
	res = ltr577_master_send(client, LTR577_PS_PULSES, (char *)&buf, 1);	
	if (res<0)
	{
		APS_ERR("ltr577_init_client() PS Pulses error...\n");
		goto EXIT_ERR;
	}

	buf = 0x36;	// 60khz & 100mA
	res = ltr577_master_send(client, LTR577_PS_LED, (char *)&buf, 1);
	if (res<0)
	{
		APS_ERR("ltr577_init_client() PS LED error...\n");
		goto EXIT_ERR;
	}

	buf = 0x5C;	// 11bits & 50ms time 
	res = ltr577_master_send(client, LTR577_PS_MEAS_RATE, (char *)&buf, 1);
	if (res<0)
	{
		APS_ERR("ltr577_init_client() PS time error...\n");
		goto EXIT_ERR;
	}

	/*for interrup work mode support */
	if (0 == obj->hw->polling_mode_ps)
	{
		ltr577_ps_set_thres();

		buf = 0x01;
		res = ltr577_master_send(client, LTR577_INT_CFG, (char *)&buf, 1);		
		if (res < 0)
		{
			goto EXIT_ERR;			
		}

		buf = 0x02;
		res = ltr577_master_send(client, LTR577_INT_PST, (char *)&buf, 1);		
		if (res < 0)
		{
			goto EXIT_ERR;
		}
	}
#ifdef SENSOR_DEFAULT_ENABLED
	res = ltr577_ps_enable(client, 1);
	if (res < 0)
	{
		APS_ERR("enable ps fail: %d\n", res);
		goto EXIT_ERR;
	}
#endif
	// Enable ALS to Full Range at startup
	init_als_gain = ALS_RANGE_9;
	als_gainrange = init_als_gain;//Set global variable
	APS_DBG("ALS sensor gainrange %d!\n", init_als_gain);

	switch (als_gainrange)
	{
	case ALS_RANGE_1:
		buf = MODE_ALS_Range1;
		res = ltr577_master_send(client, LTR577_ALS_GAIN, (char *)&buf, 1);		
		break;

	case ALS_RANGE_3:
		buf = MODE_ALS_Range3;
		res = ltr577_master_send(client, LTR577_ALS_GAIN, (char *)&buf, 1);
		break;

	case ALS_RANGE_6:
		buf = MODE_ALS_Range6;
		res = ltr577_master_send(client, LTR577_ALS_GAIN, (char *)&buf, 1);
		break;

	case ALS_RANGE_9:
		buf = MODE_ALS_Range9;
		res = ltr577_master_send(client, LTR577_ALS_GAIN, (char *)&buf, 1);
		break;

	case ALS_RANGE_18:
		buf = MODE_ALS_Range18;
		res = ltr577_master_send(client, LTR577_ALS_GAIN, (char *)&buf, 1);
		break;

	default:
		buf = MODE_ALS_Range3;
		res = ltr577_master_send(client, LTR577_ALS_GAIN, (char *)&buf, 1);
		break;
	}

	buf = ALS_RESO_MEAS;	// 18 bit & 100ms measurement rate
	res = ltr577_master_send(client, LTR577_ALS_MEAS_RATE, (char *)&buf, 1);
	APS_DBG("ALS sensor resolution & measurement rate: %d!\n", ALS_RESO_MEAS);
#ifdef SENSOR_DEFAULT_ENABLED
	res = ltr577_als_enable(client, 1);
	if (res < 0)
	{
		APS_ERR("enable als fail: %d\n", res);
		goto EXIT_ERR;
	}
#endif
	if ((res = ltr577_setup_eint(client)) != 0)
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
static int ltr577_check_for_esd(void)
{
	int res;
	int init_als_gain;
	u8 buf;
	u8 ps_pulse_data = 0;
	u8 ps_interrupt_data = 0;
	u8 regdata = 0;

	struct i2c_client *client = ltr577_obj->client;
	struct ltr577_priv *obj = ltr577_obj;

	mdelay(PON_DELAY);

	/*check whether sensor is reset or not */

	res = ltr577_master_recv(client, LTR577_PS_PULSES, &ps_pulse_data, 0x01);
	if (res < 0)
	{
		APS_ERR("clear PS interrupt failed, res = %d\n", res);
	}
	APS_DBG(" PS pulse , reg:0x%x\n", ps_pulse_data);

	res = ltr577_master_recv(client, LTR577_INT_CFG, &ps_interrupt_data, 0x01);
	if (res < 0)
	{
		APS_ERR("clear PS interrupt failed, res = %d\n", res);
	}
	APS_DBG("clear PS interrupt, reg:0x%x\n", ps_interrupt_data);

	if((ps_pulse_data == PS_PULSES) && (ps_interrupt_data == 0x01)){
		APS_DBG("ltr578 or ltr577 is not reset \n");
		return 0;
	}

	buf = PS_PULSES; // 16 pulses
	res = ltr577_master_send(client, LTR577_PS_PULSES, (char *)&buf, 1);
	if (res<0)
	{
		APS_ERR("ltr577_init_client() PS Pulses error...\n");
		goto EXIT_ERR;
	}

	buf = 0x36;	// 60khz & 100mA
	res = ltr577_master_send(client, LTR577_PS_LED, (char *)&buf, 1);
	if (res<0)
	{
		APS_ERR("ltr577_init_client() PS LED error...\n");
		goto EXIT_ERR;
	}

	buf = 0x5C;	// 11bits & 50ms time
	res = ltr577_master_send(client, LTR577_PS_MEAS_RATE, (char *)&buf, 1);
	if (res<0)
	{
		APS_ERR("ltr577_init_client() PS time error...\n");
		goto EXIT_ERR;
	}

	/*for interrup work mode support */
	if (0 == obj->hw->polling_mode_ps)
	{
		ltr577_ps_set_thres();

		buf = 0x01;
		res = ltr577_master_send(client, LTR577_INT_CFG, (char *)&buf, 1);
		if (res < 0)
		{
			goto EXIT_ERR;
		}

		buf = 0x02;
		res = ltr577_master_send(client, LTR577_INT_PST, (char *)&buf, 1);
		if (res < 0)
		{
			goto EXIT_ERR;
		}
	}

	// Enable ALS to Full Range at startup
	init_als_gain = ALS_RANGE_9;
	als_gainrange = init_als_gain;//Set global variable
	APS_LOG("ALS sensor gainrange %d!\n", init_als_gain);

	switch (als_gainrange)
	{
	case ALS_RANGE_1:
		buf = MODE_ALS_Range1;
		res = ltr577_master_send(client, LTR577_ALS_GAIN, (char *)&buf, 1);
		break;

	case ALS_RANGE_3:
		buf = MODE_ALS_Range3;
		res = ltr577_master_send(client, LTR577_ALS_GAIN, (char *)&buf, 1);
		break;

	case ALS_RANGE_6:
		buf = MODE_ALS_Range6;
		res = ltr577_master_send(client, LTR577_ALS_GAIN, (char *)&buf, 1);
		break;

	case ALS_RANGE_9:
		buf = MODE_ALS_Range9;
		res = ltr577_master_send(client, LTR577_ALS_GAIN, (char *)&buf, 1);
		break;

	case ALS_RANGE_18:
		buf = MODE_ALS_Range18;
		res = ltr577_master_send(client, LTR577_ALS_GAIN, (char *)&buf, 1);
		break;

	default:
		buf = MODE_ALS_Range3;
		res = ltr577_master_send(client, LTR577_ALS_GAIN, (char *)&buf, 1);
		break;
	}

	buf = ALS_RESO_MEAS;	// 18 bit & 100ms measurement rate
	res = ltr577_master_send(client, LTR577_ALS_MEAS_RATE, (char *)&buf, 1);
	APS_LOG("ALS sensor resolution & measurement rate: %d!\n", ALS_RESO_MEAS);

	if(ps_enabled ==1){
		res = ltr577_master_recv(client, LTR577_MAIN_CTRL, &regdata, 0x01);
		if (res < 0) {
			APS_DBG("i2c error: %d\n", res);
		}
		regdata &= 0xEF;	// Clear reset bit
		regdata |= 0x01;
		res = ltr577_master_send(client, LTR577_MAIN_CTRL, (char *)&regdata, 1);
		if (res < 0)
		{
			APS_ERR("PS: enable ps err: %d\n",res);
			return res;
		}
	}

	if(als_enabled){
		res = ltr577_als_enable(client, 1);
		if (res < 0)
		{
			APS_ERR("PS: enable ALS err: %d\n",res);
			return res;
		}
	}
	return 0;

EXIT_ERR:
	APS_ERR("init esd dev: %d\n", res);
	return 1;
}

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
	APS_DBG("ltr577_obj als enable value = %d\n", en);

	if(!ltr577_obj)
	{
		APS_ERR("ltr577_obj is null!!\n");
		return -1;
	}	

	res = ltr577_als_enable(ltr577_obj->client, en);
	if (res) {
		APS_ERR("als_enable_nodata is failed!!\n");
		return -1;
	}

	mutex_lock(&ltr577_mutex);
	if (en)
		set_bit(CMC_BIT_ALS, &ltr577_obj->enable);
	else
		clear_bit(CMC_BIT_ALS, &ltr577_obj->enable);
	mutex_unlock(&ltr577_mutex);	

#ifdef REPORT_PS_ONCE_WHEN_ALS_ENABLED
	cancel_delayed_work(&ltr577_obj->check_ps_work);
	schedule_delayed_work(&ltr577_obj->check_ps_work, msecs_to_jiffies(300));
#endif	
	
	return 0;
}

static int als_set_delay(u64 ns)
{
	// Do nothing
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

	if(!ltr577_obj)
	{
		APS_ERR("ltr577_obj is null!!\n");
		return -1;
	}

	ltr577_obj->als = ltr577_als_read(ltr577_obj->client, &ltr577_obj->als);
	if (ltr577_obj->als < 0)
		err = -1;
	else {
		*value = ltr577_get_als_value(ltr577_obj, ltr577_obj->als);
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
	APS_DBG("ltr577_obj ps enable value = %d\n", en);

	if(!ltr577_obj)
	{
		APS_ERR("ltr577_obj is null!!\n");
		return -1;
	}
	
	res = ltr577_ps_enable(ltr577_obj->client, en);
	if (res < 0) {
		APS_ERR("ps_enable_nodata is failed!!\n");
		return -1;
	}

	mutex_lock(&ltr577_mutex);
	if (en)
		set_bit(CMC_BIT_PS, &ltr577_obj->enable);
	else
		clear_bit(CMC_BIT_PS, &ltr577_obj->enable);
	mutex_unlock(&ltr577_mutex);
	
	return 0;
}

static int ps_set_delay(u64 ns)
{
	// Do nothing
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

    if(!ltr577_obj)
	{
		APS_ERR("ltr577_obj is null!!\n");
		return -1;
	}
    
	ltr577_obj->ps = ltr577_ps_read(ltr577_obj->client, &ltr577_obj->ps);
	if (ltr577_obj->ps < 0)
		err = -1;
	else {
		*value = ltr577_get_ps_value(ltr577_obj, ltr577_obj->ps);
		if (*value < 0)
			err = -1;
		*status = SENSOR_STATUS_ACCURACY_MEDIUM;
	}
    
	return err;
}
/*-----------------------------------------------------------------------------------*/

/*-----------------------------------i2c operations----------------------------------*/
static int ltr577_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ltr577_priv *obj = NULL;
	struct als_control_path als_ctl={0};
	struct als_data_path als_data={0};
	struct ps_control_path ps_ctl={0};
	struct ps_data_path ps_data={0};
	int err = 0;
	u8 part_id = 0;

	APS_FUN();

	if(CHECK_THIS_DEV_DEBUG_AREADY_EXIT()==0)
        {
            return -EIO;
        }

	/* get customization and power on */
	err = get_alsps_dts_func(client->dev.of_node, hw);
	if (err < 0) {
		APS_ERR("get customization info from dts failed\n");
		return -EFAULT;
	}
	APS_DBG("%s:get hw->i2c_num =%d  \n",__func__,hw->i2c_num);
	
	if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
	{
		err = -ENOMEM;
		goto exit;
	}
	memset(obj, 0, sizeof(*obj));
	ltr577_obj = obj;
	ps_irq_use_old = hw->ps_irq_use_old;
	obj->hw = hw;
	INIT_WORK(&obj->eint_work, ltr577_eint_work);	
#ifdef REPORT_PS_ONCE_WHEN_ALS_ENABLED
	INIT_DELAYED_WORK(&obj->check_ps_work, ltr577_check_ps_work);
#endif
#ifdef DELAYED_PS_CALI
	INIT_DELAYED_WORK(&obj->cali_ps_work, ltr577_cali_ps_work);
#endif

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
	if (!ps_irq_use_old){
		obj->irq_node = client->dev.of_node;
	}

	obj->enable = 0;
	obj->pending_intr = 0;
	obj->ps_cali = 0;
	obj->als_level_num = sizeof(obj->hw->als_level)/sizeof(obj->hw->als_level[0]);
	obj->als_value_num = sizeof(obj->hw->als_value)/sizeof(obj->hw->als_value[0]);
	obj->als_modulus = (400*100)/(16*150);//(1/Gain)*(400/Tine), this value is fix after init ATIME and CONTROL register value
										//(400)/16*2.72 here is amplify *100	
	/*-----------------------------value need to be confirmed-----------------------------------------*/
#if 0	
	BUG_ON(sizeof(obj->als_level) != sizeof(obj->hw->als_level));
	memcpy(obj->als_level, obj->hw->als_level, sizeof(obj->als_level));
	BUG_ON(sizeof(obj->als_value) != sizeof(obj->hw->als_value));
	memcpy(obj->als_value, obj->hw->als_value, sizeof(obj->als_value));
	atomic_set(&obj->i2c_retry, 3);
#endif

	BUG_ON(sizeof(als_level) != sizeof(obj->hw->als_level));
	memcpy(als_level, obj->hw->als_level, sizeof(als_level));
	BUG_ON(sizeof(als_value) != sizeof(obj->hw->als_value));
	memcpy(als_value, obj->hw->als_value, sizeof(als_value));
	atomic_set(&obj->i2c_retry, 3);

#ifdef SENSOR_DEFAULT_ENABLED
	set_bit(CMC_BIT_ALS, &obj->enable);
	set_bit(CMC_BIT_PS, &obj->enable);
#else
	clear_bit(CMC_BIT_ALS, &obj->enable);
	clear_bit(CMC_BIT_PS, &obj->enable);
#endif

	//get part id
	err = ltr577_get_chip_id(client, &part_id);
	if (err < 0)
		goto exit_init_failed;

	if (part_id == 0xB1)
	{
		APS_LOG("The chip is LTR578, pard_id:0x%x.\n", part_id);
		sprintf(&ltr577_prox_vendor_name[0], "LITE-ON-ltr578");
	}
	else
	{
		APS_LOG("The chip is LTR577, pard_id:0x%x.\n", part_id);
		sprintf(&ltr577_prox_vendor_name[0], "LITE-ON-ltr577");
	}

	ltr577_i2c_client = client;
	err = ltr577_init_client();
	if(err)
	{
		goto exit_init_failed;
	}
	APS_DBG("ltr577_init_client() OK!\n");
	
	err = misc_register(&ltr577_device);
	if(err)
	{
		APS_ERR("ltr577_device register failed\n");
		goto exit_misc_device_register_failed;
	}

    als_ctl.is_use_common_factory =false;
	ps_ctl.is_use_common_factory = false;
	
	/*------------------------ltr577 attribute file for debug--------------------------------------*/
	//err = ltr577_create_attr(&(ltr577_init_info.platform_diver_addr->driver));
	err = ltr577_create_attr(&(ltr577_i2c_driver.driver));
	if(err)
	{
		APS_ERR("create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}
	/*------------------------ltr577 attribute file for debug--------------------------------------*/
	
	als_ctl.open_report_data= als_open_report_data;
	als_ctl.enable_nodata = als_enable_nodata;
	als_ctl.set_delay  = als_set_delay;
	als_ctl.batch = als_batch;
	als_ctl.flush = als_flush;
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
	ps_ctl.batch = ps_batch;
	ps_ctl.flush = ps_flush;
	ps_ctl.is_report_input_direct = false;
	ps_ctl.is_support_batch = false;
	ps_ctl.is_polling_mode = obj->hw->polling_mode_ps;

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
#ifdef ontim_debug_info
        REGISTER_AND_INIT_ONTIM_DEBUG_FOR_THIS_DEV();
#endif

	ltr577_init_flag =0;
	APS_LOG("probe successful.\n");
	return 0;

exit_create_attr_failed:
exit_sensor_obj_attach_fail:
exit_misc_device_register_failed:
	misc_deregister(&ltr577_device);
exit_init_failed:
	kfree(obj);
exit:
	ltr577_i2c_client = NULL;           
	APS_ERR("err = %d\n", err);
	ltr577_init_flag =-1;
	return err;
}

static int ltr577_i2c_remove(struct i2c_client *client)
{
	int err;

	err = ltr577_delete_attr(&(ltr577_init_info.platform_diver_addr->driver));
	//err = ltr577_delete_attr(&(ltr577_i2c_driver.driver));
	if(err)
	{
		APS_ERR("ltr577_delete_attr fail: %d\n", err);
	}

	misc_deregister(&ltr577_device);
	//if(err)
	//{
	//	APS_ERR("misc_deregister fail: %d\n", err);    
	//}
		
	ltr577_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));

	return 0;
}

static int ltr577_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	strcpy(info->type, LTR577_DEV_NAME);
	return 0;
}

static int ltr577_i2c_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ltr577_priv *obj = i2c_get_clientdata(client);    
	int err;
	APS_FUN();    

	if(!obj)
	{
		APS_ERR("null pointer!!\n");
		return -EINVAL;
	}
		
	atomic_set(&obj->als_suspend, 1);
	err = ltr577_als_enable(obj->client, 0);
	if(err < 0)
	{
		APS_ERR("disable als: %d\n", err);
		return err;
	}
#ifndef NO_PS_CTRL_WHEN_SUSPEND_RESUME
	atomic_set(&obj->ps_suspend, 1);
	err = ltr577_ps_enable(obj->client, 0);
	if(err < 0)
	{
		APS_ERR("disable ps:  %d\n", err);
		return err;
	}
		
	ltr577_power(obj->hw, 0);
#endif	
	return 0;
}

static int ltr577_i2c_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ltr577_priv *obj = i2c_get_clientdata(client);        
	int err;
	APS_FUN();

	if(!obj)
	{
		APS_ERR("null pointer!!\n");
		return -EINVAL;
	}
#ifndef NO_PS_CTRL_WHEN_SUSPEND_RESUME
	ltr577_power(obj->hw, 1);

	atomic_set(&obj->ps_suspend, 0);
	if (test_bit(CMC_BIT_PS, &obj->enable))
	{
		err = ltr577_ps_enable(obj->client, 1);
		if (err < 0)
		{
			APS_ERR("enable ps fail: %d\n", err);
		}
	}
#endif
	atomic_set(&obj->als_suspend, 0);
	if(test_bit(CMC_BIT_ALS, &obj->enable))
	{
		err = ltr577_als_enable(obj->client, 1);
	    if (err < 0)
		{
			APS_ERR("enable als fail: %d\n", err);        
		}
	}	

	return 0;
}
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static int ltr577_remove(void)
{
	APS_FUN();

	ltr577_power(hw, 0);	
	i2c_del_driver(&ltr577_i2c_driver);
	ltr577_init_flag = -1;

	return 0;
}
/*----------------------------------------------------------------------------*/
static int  ltr577_local_init(void)
{
	ltr577_power(hw, 1);
	
	if(i2c_add_driver(&ltr577_i2c_driver))
	{
		APS_ERR("add driver error\n");
		return -1;
	}

	if(-1 == ltr577_init_flag)
	{
	   return -1;
	}
	
	return 0;
}
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static int __init ltr577_init(void)
{
#if 0
       int err=0;
 //// liuxinyuan add start----------/////////////
	const char *name = "mediatek,alsps_ltr577";
	struct device_node *node =of_find_compatible_node(NULL,NULL,name);
APS_ERR("ltr577_init start!!\n");
	err = get_alsps_dts_func(node, hw);
	if (err<0){
		APS_ERR("%s get alsps dts fail!!\n",__func__);
	}
APS_ERR("%s:get hw->i2c_num =%d  \n",__func__,hw->i2c_num);

	i2c_register_board_info(hw->i2c_num,&ltr577_i2c_board_info,1);

//////////liuixnyuan add end ------////////////////
#endif
	alsps_driver_add(&ltr577_init_info);

	return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit ltr577_exit(void)
{
	APS_FUN();
}
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
module_init(ltr577_init);
module_exit(ltr577_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("Liteon");
MODULE_DESCRIPTION("LTR-577ALSPS Driver");
MODULE_LICENSE("GPL");

