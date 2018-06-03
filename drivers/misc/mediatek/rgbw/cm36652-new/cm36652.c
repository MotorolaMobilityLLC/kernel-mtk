/*
 * Author: yucong xiong <yucong.xion@mediatek.com>
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

#include "cust_rgbw.h"
#include "cm36652_rgbw.h"
#include "cm36652.h"
#include "cm36652_cs.h"
#include "rgbw.h"
#ifdef CUSTOM_KERNEL_SENSORHUB
#include <SCP_sensorHub.h>
#endif


/******************************************************************************
 * configuration
*******************************************************************************/
/*----------------------------------------------------------------------------*/

#define CM36652_DEV_NAME	 "cm36652rgbw"
/*----------------------------------------------------------------------------*/
#define RGW_TAG				  "[cm36652rgbw] "
#define RGW_ERR(fmt, args...)	pr_err(RGW_TAG fmt, ##args)
#define RGW_LOG(fmt, args...)	/*pr_debug*/pr_err(RGW_TAG fmt, ##args)
#define RGW_DBG(fmt, args...)	/*pr_debug*/pr_err(RGW_TAG fmt, ##args)


#define I2C_FLAG_WRITE	0
#define I2C_FLAG_READ	1

/*----------------------------------------------------------------------------*/
static int cm36652_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int cm36652_i2c_remove(struct i2c_client *client);
static int cm36652_i2c_detect(struct i2c_client *client, struct i2c_board_info *info);
static int cm36652_i2c_suspend(struct device *dev);
static int cm36652_i2c_resume(struct device *dev);

/*----------------------------------------------------------------------------*/
static const struct i2c_device_id cm36652_i2c_id[] = {{CM36652_DEV_NAME, 0}, {} };
/* static struct i2c_board_info __initdata i2c_cm36652={ I2C_BOARD_INFO(CM36652_DEV_NAME, 0x60)}; */
/* static unsigned long long int_top_time; */
/* Maintain rgbw cust info here */
struct rgbw_hw rgbw_cust;
static struct rgbw_hw *hw = &rgbw_cust;
struct platform_device *rgbwPltFmDev;

/* For rgbw driver get cust info */
struct rgbw_hw *get_cust_rgbw(void)
{
	return &rgbw_cust;
}
/*----------------------------------------------------------------------------*/
struct cm36652_priv {
	struct rgbw_hw  *hw;
	struct i2c_client *client;
#ifdef CUSTOM_KERNEL_SENSORHUB
	struct work_struct init_done_work;
#endif

	/*misc*/
	u16		als_modulus;
	atomic_t	i2c_retry;
	atomic_t	rgbw_suspend;
	atomic_t	rgbw_debounce;	/*debounce time after enabling als*/
	atomic_t	rgbw_deb_on;	/*indicates if the debounce is on*/
	atomic_t	rgbw_deb_end;	/*the jiffies representing the end of debounce*/
	atomic_t	trace;
	atomic_t  init_done;
	struct device_node *irq_node;
	int		irq;

	/*data*/
	u16			als;
	u16			value_g;
	u16			value_b;
	u16			value_w;
	u8			_align;

	/*atomic_t	als_cmd_val;*/	/*the cmd value can't be read, stored in ram*/
	ulong		enable;		/*enable mask*/

};
/*----------------------------------------------------------------------------*/

#ifdef CONFIG_OF
static const struct of_device_id rgbw_of_match[] = {
	{.compatible = "mediatek,rgbw"},
	{},
};
#endif

#ifdef CONFIG_PM_SLEEP
static const struct dev_pm_ops CM36652_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(cm36652_i2c_suspend, cm36652_i2c_resume)
};
#endif

static struct i2c_driver cm36652_i2c_driver = {
	.probe	  = cm36652_i2c_probe,
	.remove	 = cm36652_i2c_remove,
	.detect	 = cm36652_i2c_detect,
	.id_table   = cm36652_i2c_id,
	.driver = {
		.name = CM36652_DEV_NAME,
#ifdef CONFIG_PM_SLEEP
		.pm   = &CM36652_pm_ops,
#endif
#ifdef CONFIG_OF
	.of_match_table = rgbw_of_match,
#endif
	},
};

/*----------------------------------------------------------------------------*/


static struct i2c_client *cm36652_i2c_client;
static struct cm36652_priv *cm36652_obj;
/* static int intr_flag = 1; */ /* hw default away after enable. */

static int rgbw_local_init(void);
static int rgbw_remove(void);
static int rgbw_init_flag =  -1;
static struct rgbw_init_info cm36652rgbw_init_info = {
		.name = "cm36652rgbw",
		.init = rgbw_local_init,
		.uninit = rgbw_remove,

};

static DEFINE_MUTEX(cm36652_mutex);

enum {
	CMC_BIT_ALS	= 1,
	/*CMC_BIT_PS	   = 2,*/
} CMC_RGBW_BIT;
/*-----------------------------CMC for debugging-------------------------------*/
enum {
	CMC_TRC_rgbw_data = 0x0001,
	CMC_TRC_PS_DATA = 0x0002,
	CMC_TRC_EINT	= 0x0004,
	CMC_TRC_IOCTL   = 0x0008,
	CMC_TRC_I2C	 = 0x0010,
	CMC_TRC_CVT_ALS = 0x0020,
	CMC_TRC_CVT_PS  = 0x0040,
	CMC_TRC_CVT_AAL = 0x0080,
	CMC_TRC_DEBUG   = 0x8000,
} CMC_RGBW_TRC;




#ifndef CM36652_ACCESS_BY_ALS_I2C
int CM36652_i2c_master_operate(struct i2c_client *client, char *buf, int count, int i2c_flag)
{
	int res = 0;
#ifndef CONFIG_MTK_I2C_EXTENSION
	struct i2c_msg msg[2];
#endif
	mutex_lock(&cm36652_mutex);
	switch (i2c_flag) {
	case I2C_FLAG_WRITE
#ifdef CONFIG_MTK_I2C_EXTENSION
	client->addr &= I2C_MASK_FLAG;
	res = i2c_master_send(client, buf, count);
	client->addr &= I2C_MASK_FLAG;
#else
	res = i2c_master_send(client, buf, count);
#endif
	break;

	case I2C_FLAG_READ:
#ifdef CONFIG_MTK_I2C_EXTENSION
	client->addr &= I2C_MASK_FLAG;
	client->addr |= I2C_WR_FLAG;
	client->addr |= I2C_RS_FLAG;
	res = i2c_master_send(client, buf, count);
	client->addr &= I2C_MASK_FLAG;
#else
	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = buf;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = count;
	msg[1].buf = buf;
	res = i2c_transfer(client->adapter, msg, ARRAY_SIZE(msg));
#endif
	break;
	default:
	RGW_LOG("CM36652_i2c_master_operate i2c_flag command not support!\n");
	break;
	}
	if (res < 0)
		goto EXIT_ERR;
	mutex_unlock(&cm36652_mutex);
	return res;
EXIT_ERR:
	mutex_unlock(&cm36652_mutex);
	RGW_ERR("CM36652_i2c_master_operate fail\n");
	return res;
}
#endif
/*----------------------------------------------------------------------------*/
static void cm36652_power(struct rgbw_hw *hw, unsigned int on)
{
}
/********************************************************************/
#if 0
int cm36652_enable_ps(struct i2c_client *client, int enable)
{
	struct cm36652_priv *obj = i2c_get_clientdata(client);
	int res;
	u8 databuf[3];

	if (enable == 1) {
		RGW_LOG("cm36652_enable_ps enable_ps\n");
		databuf[0] = CM36652_REG_PS_CONF1_2;
		res = CM36652_WRAP_i2c_master_operate(databuf, 0x201, I2C_FLAG_READ);
		if (res < 0) {
			RGW_ERR("i2c_master_send function err\n");
			goto ENABLE_PS_EXIT_ERR;
		}
		/* RGW_LOG("CM36652_REG_PS_CONF1_2 value value_low = %x, value_high = %x\n", databuf[0], databuf[1]); */
		databuf[2] = databuf[1];
		databuf[1] = databuf[0]&0xFE;
		databuf[0] = CM36652_REG_PS_CONF1_2;
		res = CM36652_WRAP_i2c_master_operate(databuf, 0x3, I2C_FLAG_WRITE);
		if (res < 0) {
			RGW_ERR("i2c_master_send function err\n");
			goto ENABLE_PS_EXIT_ERR;
		}
		atomic_set(&obj->ps_deb_on, 1);
		atomic_set(&obj->ps_deb_end, jiffies+atomic_read(&obj->ps_debounce)/(1000/HZ));
		intr_flag = 1; /* reset hw status to away after enable. */
	} else {
		RGW_LOG("cm36652_enable_ps disable_ps\n");
		databuf[0] = CM36652_REG_PS_CONF1_2;
		res = CM36652_WRAP_i2c_master_operate(databuf, 0x201, I2C_FLAG_READ);
		if (res < 0) {
			RGW_ERR("i2c_master_send function err\n");
			goto ENABLE_PS_EXIT_ERR;
		}
		/* RGW_LOG("CM36652_REG_PS_CONF1_2 value value_low = %x, value_high = %x\n", databuf[0], databuf[1]); */

		databuf[2] = databuf[1];
		databuf[1] = databuf[0]|0x01;
		databuf[0] = CM36652_REG_PS_CONF1_2;

		res = CM36652_WRAP_i2c_master_operate(databuf, 0x3, I2C_FLAG_WRITE);
		if (res < 0) {
			RGW_ERR("i2c_master_send function err\n");
			goto ENABLE_PS_EXIT_ERR;
		}
		atomic_set(&obj->ps_deb_on, 0);
	}

	return 0;
ENABLE_PS_EXIT_ERR:
	return res;
}
#endif
/********************************************************************/
int cm36652_enable_rgbw(struct i2c_client *client, int enable)
{
	struct cm36652_priv *obj = i2c_get_clientdata(client);
	int res;
	u8 databuf[3];

	mutex_lock(&cm36652_cs_mutex);
	if (enable == 1) {
		/* RGW_LOG("cm36652_enable_rgbw enable_als\n"); */
		databuf[0] = CM36652_REG_CS_CONF;
		res = CM36652_WRAP_i2c_master_operate(databuf, 0x201, I2C_FLAG_READ);
		if (res < 0) {
			RGW_ERR("i2c_master_send function err\n");
			goto ENABLE_ALS_EXIT_ERR;
		}
		/* RGW_LOG("CM36652_REG_CS_CONF value value_low = %x, value_high = %x\n", databuf[0], databuf[1]); */

		databuf[2] = databuf[1];
		databuf[1] = databuf[0]&0xFE;
		databuf[0] = CM36652_REG_CS_CONF;
		res = CM36652_WRAP_i2c_master_operate(databuf, 0x3, I2C_FLAG_WRITE);
		if (res < 0) {
			RGW_ERR("i2c_master_send function err\n");
			goto ENABLE_ALS_EXIT_ERR;
		}
		atomic_set(&obj->rgbw_deb_on, 1);
		atomic_set(&obj->rgbw_deb_end, jiffies+atomic_read(&obj->rgbw_debounce)/(1000/HZ));
	} else {
		/* RGW_LOG("cm36652_enable_rgbw disable_als\n"); */
		databuf[0] = CM36652_REG_CS_CONF;
		res = CM36652_WRAP_i2c_master_operate(databuf, 0x201, I2C_FLAG_READ);
		if (res < 0) {
			RGW_ERR("i2c_master_send function err\n");
			goto ENABLE_ALS_EXIT_ERR;
		}
		/* RGW_LOG("CM36652_REG_CS_CONF value value_low = %x, value_high = %x\n", databuf[0], databuf[1]); */

		databuf[2] = databuf[1];
		databuf[1] = databuf[0]|0x01;
		databuf[0] = CM36652_REG_CS_CONF;
		res = CM36652_WRAP_i2c_master_operate(databuf, 0x3, I2C_FLAG_WRITE);
		if (res < 0) {
			RGW_ERR("i2c_master_send function err\n");
			goto ENABLE_ALS_EXIT_ERR;
		}
		atomic_set(&obj->rgbw_deb_on, 0);
	}
	mutex_unlock(&cm36652_cs_mutex);
	return 0;
ENABLE_ALS_EXIT_ERR:
	mutex_unlock(&cm36652_cs_mutex);
	return res;
}
/********************************************************************/
#if 0
long cm36652_read_ps(struct i2c_client *client, u8 *data)
{
	long res;
	u8 databuf[2];
	struct cm36652_priv *obj = i2c_get_clientdata(client);

	databuf[0] = CM36652_REG_PS_DATA;
	res = CM36652_WRAP_i2c_master_operate(databuf, 0x201, I2C_FLAG_READ);
	if (res < 0) {
		RGW_ERR("i2c_master_send function err\n");
		goto READ_PS_EXIT_ERR;
	}
	if (atomic_read(&obj->trace) & CMC_TRC_DEBUG)
		RGW_LOG("CM36652_REG_PS_DATA value value_low = %x, value_high = %x\n", databuf[0], databuf[1]);

	if (databuf[0] < obj->ps_cali)
		*data = 0;
	else
		*data = databuf[0] - obj->ps_cali;
	return 0;
READ_PS_EXIT_ERR:
	return res;
}
#endif
/********************************************************************/
long cm36652_read_rgbw(struct i2c_client *client, u16 *data, u16 *data_1, u16 *data_2, u16 *data_3)
{
	long res;
	u8 databuf[2];
	/* u8 databuf[8]; */
	struct cm36652_priv *obj = i2c_get_clientdata(client);

	memset(&databuf, 0, sizeof(databuf));
#if 0
	/* CS_R */
	databuf[0] = CM36652_REG_CS_R_DATA;
	res = CM36652_WRAP_i2c_master_operate(databuf, 0x801, I2C_FLAG_READ);
	if (res < 0) {
		RGW_ERR("i2c_master_send function err\n");
		goto READ_ALS_EXIT_ERR;
	}

	if (atomic_read(&obj->trace) & CMC_TRC_DEBUG) {
		RGW_LOG("CM36652_REG_CS_R_DATA value: %d\n", ((databuf[1]<<8)|databuf[0]));
		RGW_LOG("CM36652_REG_CS_G_DATA value: %d\n", ((databuf[3]<<8)|databuf[2]));
		RGW_LOG("CM36652_REG_CS_B_DATA value: %d\n", ((databuf[5]<<8)|databuf[4]));
		RGW_LOG("CM36652_REG_White_DATA value: %d\n", ((databuf[7]<<8)|databuf[6]));
	}

	*data = ((databuf[1]<<8)|databuf[0]);
	*data_1 = ((databuf[3]<<8)|databuf[2]);
	*data_2 = ((databuf[5]<<8)|databuf[4]);
	*data_3 = ((databuf[7]<<8)|databuf[6]);
#endif
/*#if 0*/
	/* CS_R */
	databuf[0] = CM36652_REG_CS_R_DATA;
	res = CM36652_WRAP_i2c_master_operate(databuf, 0x201, I2C_FLAG_READ);
	if (res < 0) {
		RGW_ERR("i2c_master_send function err\n");
		goto READ_ALS_EXIT_ERR;
	}

	if (atomic_read(&obj->trace) & CMC_TRC_DEBUG)
		RGW_LOG("CM36652_REG_CS_R_DATA value: %d\n", ((databuf[1]<<8)|databuf[0]));

	*data = ((databuf[1]<<8)|databuf[0]);

	/* CS_G */
	databuf[0] = CM36652_REG_CS_G_DATA;
	res = CM36652_WRAP_i2c_master_operate(databuf, 0x201, I2C_FLAG_READ);
	if (res < 0) {
		RGW_ERR("i2c_master_send function err\n");
		goto READ_ALS_EXIT_ERR;
	}

	if (atomic_read(&obj->trace) & CMC_TRC_DEBUG)
		RGW_LOG("CM36652_REG_CS_G_DATA value: %d\n", ((databuf[1]<<8)|databuf[0]));

	*data_1 = ((databuf[1]<<8)|databuf[0]);

	/* CS_B */
	databuf[0] = CM36652_REG_CS_B_DATA;
	res = CM36652_WRAP_i2c_master_operate(databuf, 0x201, I2C_FLAG_READ);
	if (res < 0) {
		RGW_ERR("i2c_master_send function err\n");
		goto READ_ALS_EXIT_ERR;
	}

	if (atomic_read(&obj->trace) & CMC_TRC_DEBUG)
		RGW_LOG("CM36652_REG_CS_B_DATA value: %d\n", ((databuf[1]<<8)|databuf[0]));

	*data_2 = ((databuf[1]<<8)|databuf[0]);

	/* CS_W */
	databuf[0] = CM36652_REG_White_DATA;
	res = CM36652_WRAP_i2c_master_operate(databuf, 0x201, I2C_FLAG_READ);
	if (res < 0) {
		RGW_ERR("i2c_master_send function err\n");
		goto READ_ALS_EXIT_ERR;
	}

	if (atomic_read(&obj->trace) & CMC_TRC_DEBUG)
		RGW_LOG("CM36652_REG_White_DATA value: %d\n", ((databuf[1]<<8)|databuf[0]));

	*data_3 = ((databuf[1]<<8)|databuf[0]);
/*#endif*/

	return 0;
READ_ALS_EXIT_ERR:
	return res;
}
/********************************************************************/
#if 0
static int cm36652_get_ps_value(struct cm36652_priv *obj, u8 ps)
{
	int val, mask = atomic_read(&obj->ps_mask);
	int invalid = 0;

	val = intr_flag; /* value between high/low threshold should sync. with hw status. */
	if (ps > atomic_read(&obj->ps_thd_val_high))
		val = 0;  /*close*/
	else if (ps < atomic_read(&obj->ps_thd_val_low))
		val = 1;  /*far away*/

	if (atomic_read(&obj->ps_suspend))
		invalid = 1;
	else if (atomic_read(&obj->ps_deb_on) == 1) {
		unsigned long endt = atomic_read(&obj->ps_deb_end);

		if (time_after(jiffies, endt))
			atomic_set(&obj->ps_deb_on, 0);

		if (atomic_read(&obj->ps_deb_on) == 1)
			invalid = 1;
	}

	if (!invalid) {
		if (unlikely(atomic_read(&obj->trace) & CMC_TRC_CVT_PS)) {
			if (mask)
				RGW_DBG("PS:  %05d => %05d [M]\n", ps, val);
			else
				RGW_DBG("PS:  %05d => %05d\n", ps, val);
		}
		if ((test_bit(CMC_BIT_PS, &obj->enable) == 0)) {
			RGW_DBG("PS: not enable and do not report this value\n");
			return -1;
		} else
			return val;

	} else {
		if (unlikely(atomic_read(&obj->trace) & CMC_TRC_CVT_PS))
			RGW_DBG("PS:  %05d => %05d (-1)\n", ps, val);
		return -1;
	}
}
#endif
/********************************************************************/
#if 0
static int cm36652_get_als_value(struct cm36652_priv *obj, u16 als)
{
	int idx = 0;
	int invalid = 0;
	int level_high = 0;
	int level_low = 0;
	int level_diff = 0;
	int value_high = 0;
	int value_low = 0;
	int value_diff = 0;
	int value = 0;

	if ((obj->als_level_num == 0) || (obj->als_value_num == 0)) {
		RGW_ERR("invalid als_level_num = %d, als_value_num = %d\n", obj->als_level_num, obj->als_value_num);
		return -1;
	}

	if (atomic_read(&obj->rgbw_deb_on) == 1)	{
		unsigned long endt = atomic_read(&obj->rgbw_deb_end);

		if (time_after(jiffies, endt))
			atomic_set(&obj->rgbw_deb_on, 0);

		if (atomic_read(&obj->rgbw_deb_on) == 1)
			invalid = 1;
	}

	for (idx = 0; idx < obj->als_level_num; idx++) {
		if (als < obj->hw->als_level[idx])
			break;
	}

	if (idx >= obj->als_level_num || idx >= obj->als_value_num) {
		if (idx < obj->als_value_num)
			value = obj->hw->als_value[idx-1];
		else
			value = obj->hw->als_value[obj->als_value_num-1];
	} else {
		level_high = obj->hw->als_level[idx];
		level_low = (idx > 0) ? obj->hw->als_level[idx-1] : 0;
		level_diff = level_high - level_low;
		value_high = obj->hw->als_value[idx];
		value_low = (idx > 0) ? obj->hw->als_value[idx-1] : 0;
		value_diff = value_high - value_low;

		if ((level_low >= level_high) || (value_low >= value_high))
			value = value_low;
		else
			value = (level_diff * value_low + (als - level_low) * value_diff
				+ ((level_diff + 1) >> 1)) / level_diff;
	}

	if (!invalid) {
		if (atomic_read(&obj->trace) & CMC_TRC_CVT_AAL)
			RGW_DBG("ALS: %d [%d, %d] => %d [%d, %d]\n", als, level_low,
				level_high, value, value_low, value_high);

	} else {
		if (atomic_read(&obj->trace) & CMC_TRC_CVT_ALS)
			RGW_DBG("ALS: %05d => %05d (-1)\n", als, value);

		return -1;
	}
	return value;
}
#endif

/*-------------------------------attribute file for debugging----------------------------------*/

/******************************************************************************
 * Sysfs attributes
*******************************************************************************/
static ssize_t cm36652_show_config(struct device_driver *ddri, char *buf)
{
	ssize_t res;

	if (!cm36652_obj) {
		RGW_ERR("cm36652_obj is null!!\n");
		return 0;
	}

	res = snprintf(buf, PAGE_SIZE, "(%d %d)\n",
	atomic_read(&cm36652_obj->i2c_retry), atomic_read(&cm36652_obj->rgbw_debounce));
	return res;
}
/*----------------------------------------------------------------------------*/
static ssize_t cm36652_store_config(struct device_driver *ddri, const char *buf, size_t count)
{
	int retry, rgbw_deb;

	if (!cm36652_obj) {
		RGW_ERR("cm36652_obj is null!!\n");
		return 0;
	}

	if (sscanf(buf, "%d %d", &retry, &rgbw_deb) == 5) {
		atomic_set(&cm36652_obj->i2c_retry, retry);
		atomic_set(&cm36652_obj->rgbw_debounce, rgbw_deb);
	} else
		RGW_ERR("invalid content: '%s', length = %zu\n", buf, count);
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t cm36652_show_trace(struct device_driver *ddri, char *buf)
{
	ssize_t res;

	if (!cm36652_obj) {
		RGW_ERR("cm36652_obj is null!!\n");
		return 0;
	}

	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&cm36652_obj->trace));
	return res;
}
/*----------------------------------------------------------------------------*/
static ssize_t cm36652_store_trace(struct device_driver *ddri, const char *buf, size_t count)
{
	int trace;

	if (!cm36652_obj) {
		RGW_ERR("cm36652_obj is null!!\n");
		return 0;
	}

	if (sscanf(buf, "0x%x", &trace) == 1)
		atomic_set(&cm36652_obj->trace, trace);
	else
		RGW_ERR("invalid content: '%s', length = %zu\n", buf, count);
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t cm36652_show_als(struct device_driver *ddri, char *buf)
{
	int res;

	if (!cm36652_obj) {
		RGW_ERR("cm36652_obj is null!!\n");
		return 0;
	}
	res = cm36652_read_rgbw(cm36652_obj->client, &cm36652_obj->als,
	&cm36652_obj->value_g, &cm36652_obj->value_b, &cm36652_obj->value_w);
	if (res)
		return snprintf(buf, PAGE_SIZE, "ERROR: %d\n", res);
	else
		return snprintf(buf, PAGE_SIZE, "0x%04X\n", cm36652_obj->als);
}
/*----------------------------------------------------------------------------*/
static ssize_t cm36652_show_reg(struct device_driver *ddri, char *buf)
{
	u8  _bIndex = 0;
	u8 databuf[2] = {0};
	ssize_t  _tLength  = 0;
	int res = 0;

	if (!cm36652_obj) {
		RGW_ERR("cm3623_obj is null!!\n");
		return 0;
	}

	for (_bIndex = 0; _bIndex < 0x0D; _bIndex++) {
		databuf[0] = _bIndex;
#ifdef CM36652_ACCESS_BY_ALS_I2C
		res = CM36652_WRAP_i2c_master_operate(databuf, 0x201, I2C_FLAG_READ);
#else
		res = CM36652_i2c_master_operate(cm36652_obj->client, databuf, 0x201, I2C_FLAG_READ);
#endif
		if (res < 0)
			RGW_ERR("i2c_master_send function err res = %d\n", res);

		_tLength += snprintf((buf + _tLength), (PAGE_SIZE - _tLength),
			"Reg[0x%02X]: 0x%02X\n", _bIndex, databuf[0]);
	}

	return _tLength;

}
/*----------------------------------------------------------------------------*/
static ssize_t cm36652_show_send(struct device_driver *ddri, char *buf)
{
	return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t cm36652_store_send(struct device_driver *ddri, const char *buf, size_t count)
{
	int addr, cmd;
	u8 dat;

	if (!cm36652_obj) {
		RGW_ERR("cm36652_obj is null!!\n");
		return 0;
	} else if (sscanf(buf, "%x %x", &addr, &cmd) != 2) {
		RGW_ERR("invalid format: '%s'\n", buf);
		return 0;
	}

	dat = (u8)cmd;
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t cm36652_show_recv(struct device_driver *ddri, char *buf)
{
	return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t cm36652_store_recv(struct device_driver *ddri, const char *buf, size_t count)
{
	int *addr = NULL;
	int ret;

	if (!cm36652_obj) {
		RGW_ERR("cm36652_obj is null!!\n");
		return 0;
	}
	/*} else if (1 != sscanf(buf, "%x", &addr)) {*/
	ret = kstrtoint(buf, 16, addr);
	if (ret < 0) {
		RGW_ERR("invalid format: '%s'\n", buf);
		return ret;
	}

	return count;
}
/*----------------------------------------------------------------------------*/
#if 0
static ssize_t cm36652_show_status(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;

	if (!cm36652_obj) {
		RGW_ERR("cm36652_obj is null!!\n");
		return 0;
	}

	if (cm36652_obj->hw) {
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: %d, (%d %d)\n",
		cm36652_obj->hw->i2c_num, cm36652_obj->hw->power_id, cm36652_obj->hw->power_vol);
	} else
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: NULL\n");

	len += snprintf(buf+len, PAGE_SIZE-len, "REGS: %02X %02X %02X %02lX %02lX\n",
	atomic_read(&cm36652_obj->als_cmd_val), atomic_read(&cm36652_obj->ps_cmd_val),
	atomic_read(&cm36652_obj->ps_thd_val), cm36652_obj->enable, cm36652_obj->pending_intr);
	len += snprintf(buf+len, PAGE_SIZE-len, "MISC: %d %d\n",
		atomic_read(&cm36652_obj->rgbw_suspend), atomic_read(&cm36652_obj->ps_suspend));

	return len;
}
#endif
#if 0
/*----------------------------------------------------------------------------*/
#define IS_SPACE(CH) (((CH) == ' ') || ((CH) == '\n'))
/*----------------------------------------------------------------------------*/
static int read_int_from_buf(struct cm36652_priv *obj, const char *buf, size_t count, u32 data[], int len)
{
	int idx = 0;
	int ret;
	char *cur = (char *)buf, *end = (char *)(buf+count);

	while (idx < len) {
		while ((cur < end) && IS_SPACE(*cur))
			cur++;

		ret = kstrtoint(cur, 10, &data[idx]);
		if (ret < 0)
			break;

		idx++;
		while ((cur < end) && !IS_SPACE(*cur))
			cur++;

	}
	return idx;
}
#endif
/*---------------------------------------------------------------------------------------*/
static DRIVER_ATTR(als,	 S_IWUSR | S_IRUGO, cm36652_show_als, NULL);
static DRIVER_ATTR(config,  S_IWUSR | S_IRUGO, cm36652_show_config,	cm36652_store_config);
static DRIVER_ATTR(trace,   S_IWUSR | S_IRUGO, cm36652_show_trace,		cm36652_store_trace);
/* static DRIVER_ATTR(status,  S_IWUSR | S_IRUGO, cm36652_show_status, NULL); */
static DRIVER_ATTR(send,	S_IWUSR | S_IRUGO, cm36652_show_send, cm36652_store_send);
static DRIVER_ATTR(recv,	S_IWUSR | S_IRUGO, cm36652_show_recv, cm36652_store_recv);
static DRIVER_ATTR(reg,	 S_IWUSR | S_IRUGO, cm36652_show_reg, NULL);
/*----------------------------------------------------------------------------*/
static struct driver_attribute *cm36652_attr_list[] = {
	&driver_attr_als,
	&driver_attr_trace,		/*trace log*/
	&driver_attr_config,
/*	&driver_attr_status,*/
	&driver_attr_send,
	&driver_attr_recv,
	&driver_attr_reg,
};

/*----------------------------------------------------------------------------*/
static int cm36652_create_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = ARRAY_SIZE(cm36652_attr_list);

	if (driver == NULL)
		return -EINVAL;

	for (idx = 0; idx < num; idx++)	{
		err = driver_create_file(driver, cm36652_attr_list[idx]);
		if (err) {
			RGW_ERR("driver_create_file (%s) = %d\n", cm36652_attr_list[idx]->attr.name, err);
			break;
		}
	}
	return err;
}
/*----------------------------------------------------------------------------*/
static int cm36652_delete_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = ARRAY_SIZE(cm36652_attr_list);

	if (!driver)
		return -EINVAL;

	for (idx = 0; idx < num; idx++)
		driver_remove_file(driver, cm36652_attr_list[idx]);

	return err;
}
/*----------------------------------------------------------------------------*/
#ifdef CUSTOM_KERNEL_SENSORHUB
#if 0
static void cm36652_init_done_work(struct work_struct *work)
{
	struct cm36652_priv *obj = cm36652_obj;
	struct CM36652_CUST_DATA *p_cust_data;
	SCP_SENSOR_HUB_DATA data;
	int max_cust_data_size_per_packet;
	int i;
	uint sizeOfCustData;
	uint len;
	char *p = (char *)obj->hw;

	RGBW_FUN();

	p_cust_data = (struct CM36652_CUST_DATA *)data.set_cust_req.custData;
	sizeOfCustData = sizeof(*(obj->hw));
	max_cust_data_size_per_packet = sizeof(data.set_cust_req.custData) - offsetof(struct CM36652_SET_CUST, data);

	for (i = 0; sizeOfCustData > 0; i++) {
		data.set_cust_req.sensorType = ID_PROXIMITY;
		data.set_cust_req.action = SENSOR_HUB_SET_CUST;
		p_cust_data->setCust.action = CM36652_CUST_ACTION_SET_CUST;
		p_cust_data->setCust.part = i;

		if (sizeOfCustData > max_cust_data_size_per_packet)
			len = max_cust_data_size_per_packet;
		else
			len = sizeOfCustData;

		memcpy(p_cust_data->setCust.data, p, len);
		sizeOfCustData -= len;
		p += len;

		len += offsetof(SCP_SENSOR_HUB_SET_CUST_REQ, custData) + offsetof(struct CM36652_SET_CUST, data);
		SCP_sensorHub_req_send(&data, &len, 1);
	}

	data.set_cust_req.sensorType = ID_PROXIMITY;
	data.set_cust_req.action = SENSOR_HUB_SET_CUST;
	p_cust_data->setEintInfo.action = CM36652_CUST_ACTION_SET_EINT_INFO;
	p_cust_data->setEintInfo.gpio_mode = GPIO_ALS_EINT_PIN_M_EINT;
	p_cust_data->setEintInfo.gpio_pin = GPIO_ALS_EINT_PIN;
	p_cust_data->setEintInfo.eint_num = CUST_EINT_ALS_NUM;
	p_cust_data->setEintInfo.eint_is_deb_en = CUST_EINT_rgbw_debounce_EN;
	p_cust_data->setEintInfo.eint_type = CUST_EINT_ALS_TYPE;
	len = offsetof(SCP_SENSOR_HUB_SET_CUST_REQ, custData) + sizeof(p_cust_data->setEintInfo);
	SCP_sensorHub_req_send(&data, &len, 1);

	mutex_lock(&cm36652_mutex);
	data.activate_req.sensorType = ID_PROXIMITY;
	data.activate_req.action = SENSOR_HUB_ACTIVATE;
	if (test_bit(CMC_BIT_PS, &obj->enable))
		data.activate_req.enable = 1;
	else
		data.activate_req.enable = 0;

	len = sizeof(data.activate_req);
	SCP_sensorHub_req_send(&data, &len, 1);
	data.activate_req.sensorType = ID_LIGHT;
	if (test_bit(CMC_BIT_ALS, &obj->enable))
		data.activate_req.enable = 1;
	else
		data.activate_req.enable = 0;

	len = sizeof(data.activate_req);
	SCP_sensorHub_req_send(&data, &len, 1);
	mutex_unlock(&cm36652_mutex);

	atomic_set(&obj->init_done,  1);
}
#endif
#endif /* #ifdef CUSTOM_KERNEL_SENSORHUB */
/*----------------------------------------------------------------------------*/
#if 0
#ifdef CUSTOM_KERNEL_SENSORHUB
static int cm36652_irq_handler(void *data, uint len)
{
	struct cm36652_priv *obj = cm36652_obj;
	SCP_SENSOR_HUB_DATA_P rsp = (SCP_SENSOR_HUB_DATA_P)data;

	if (!obj)
		return -1;

	RGW_ERR("len = %d, type = %d, action = %d, errCode = %d\n",
		len, rsp->rsp.sensorType, rsp->rsp.action, rsp->rsp.errCode);

	switch (rsp->rsp.action) {
	case SENSOR_HUB_NOTIFY:
		switch (rsp->notify_rsp.event) {
		case SCP_INIT_DONE:
			schedule_work(&obj->init_done_work);
			/* schedule_delayed_work(&obj->init_done_work, HZ); */
			break;
		case SCP_NOTIFY:
			if (rsp->notify_rsp.data[0] == CM36652_NOTIFY_PROXIMITY_CHANGE) {
				intr_flag = rsp->notify_rsp.data[1];
				cm36652_eint_func();
			} else
				RGW_ERR("Unknown notify");
			break;
		default:
			RGW_ERR("Error sensor hub notify");
			break;
		}
		break;
	default:
		RGW_ERR("Error sensor hub action");
		break;
	}

	return 0;
}
#endif /* #ifdef CUSTOM_KERNEL_SENSORHUB */
#endif
/*-------------------------------MISC device related------------------------------------------*/



/************************************************************/
static int cm36652_open(struct inode *inode, struct file *file)
{
	file->private_data = cm36652_i2c_client;

	if (!file->private_data) {
		RGW_ERR("null pointer!!\n");
		return -EINVAL;
	}
	return nonseekable_open(inode, file);
}
/************************************************************/
static int cm36652_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}
/************************************************************/
static long cm36652_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct i2c_client *client = (struct i2c_client *)file->private_data;
	struct cm36652_priv *obj = i2c_get_clientdata(client);
	long err = 0;
	void __user *ptr = (void __user *) arg;
	int dat;
	uint32_t enable;
/*	int ps_cali;*/
/*	int threshold[2];*/
#ifdef CUSTOM_KERNEL_SENSORHUB
#if 0
	SCP_SENSOR_HUB_DATA data;
	struct CM36652_CUST_DATA *pCustData;
	int len;

	data.set_cust_req.sensorType = ID_PROXIMITY;
	data.set_cust_req.action = SENSOR_HUB_SET_CUST;
	pCustData = (struct CM36652_CUST_DATA *)(&data.set_cust_req.custData);
#endif
#endif /* #ifdef CUSTOM_KERNEL_SENSORHUB */

	switch (cmd) {
	case ALSPS_SET_ALS_MODE:
		if (copy_from_user(&enable, ptr, sizeof(enable))) {
			err = -EFAULT;
			goto err_out;
		}
		if (enable) {
			err = cm36652_enable_rgbw(obj->client, 1);
			if (err) {
				RGW_ERR("enable als fail: %ld\n", err);
				goto err_out;
			}
			set_bit(CMC_BIT_ALS, &obj->enable);
		} else {
			err = cm36652_enable_rgbw(obj->client, 0);
			if (err) {
				RGW_ERR("disable als fail: %ld\n", err);
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
#if 0
	case ALSPS_GET_RGBW_DATA:
		err = cm36652_read_rgbw(obj->client, &obj->als);
		if (err)
			goto err_out;

		dat = cm36652_get_als_value(obj, obj->als);
		if (copy_to_user(ptr, &dat, sizeof(dat))) {
			err = -EFAULT;
			goto err_out;
		}
		break;
#endif
	case ALSPS_GET_ALS_RAW_DATA:
		err = cm36652_read_rgbw(obj->client, &obj->als,
			&obj->value_g, &obj->value_b, &obj->value_w);
		if (err)
			goto err_out;

		/* Note: Need add dat for G,B,W */
		dat = obj->als;
		if (copy_to_user(ptr, &dat, sizeof(dat))) {
			err = -EFAULT;
			goto err_out;
		}
		break;

/*----------------------------------for factory mode test---------------------------------------*/
#if 0
	case ALSPS_IOCTL_CLR_CALI:
		if (copy_from_user(&dat, ptr, sizeof(dat))) {
			err = -EFAULT;
			goto err_out;
		}
		if (dat == 0)
			obj->ps_cali = 0;

#ifdef CUSTOM_KERNEL_SENSORHUB
		pCustData->clearCali.action = CM36652_CUST_ACTION_CLR_CALI;
		len = offsetof(SCP_SENSOR_HUB_SET_CUST_REQ, custData) + sizeof(pCustData->clearCali);
		err = SCP_sensorHub_req_send(&data, &len, 1);
#endif
		break;

	case ALSPS_IOCTL_GET_CALI:
		ps_cali = obj->ps_cali;
		if (copy_to_user(ptr, &ps_cali, sizeof(ps_cali))) {
			err = -EFAULT;
			goto err_out;
		}
		break;

	case ALSPS_IOCTL_SET_CALI:
		if (copy_from_user(&ps_cali, ptr, sizeof(ps_cali))) {
			err = -EFAULT;
			goto err_out;
		}
		obj->ps_cali = ps_cali;

#ifdef CUSTOM_KERNEL_SENSORHUB
		pCustData->setCali.action = CM36652_CUST_ACTION_SET_CALI;
		pCustData->setCali.cali = ps_cali;
		len = offsetof(SCP_SENSOR_HUB_SET_CUST_REQ, custData) + sizeof(pCustData->setCali);
		err = SCP_sensorHub_req_send(&data, &len, 1);
#endif
		break;

	case ALSPS_SET_PS_THRESHOLD:
		if (copy_from_user(threshold, ptr, sizeof(threshold))) {
			err = -EFAULT;
			goto err_out;
		}
		RGW_ERR("%s set threshold high: 0x%x, low: 0x%x\n", __func__, threshold[0], threshold[1]);
		atomic_set(&obj->ps_thd_val_high,  (threshold[0]+obj->ps_cali));
		atomic_set(&obj->ps_thd_val_low,  (threshold[1]+obj->ps_cali));/* need to confirm */
		break;

	case ALSPS_GET_PS_THRESHOLD_HIGH:
		threshold[0] = atomic_read(&obj->ps_thd_val_high) - obj->ps_cali;
		RGW_ERR("%s get threshold high: 0x%x\n", __func__, threshold[0]);
		if (copy_to_user(ptr, &threshold[0], sizeof(threshold[0]))) {
			err = -EFAULT;
			goto err_out;
		}
		break;

	case ALSPS_GET_PS_THRESHOLD_LOW:
		threshold[0] = atomic_read(&obj->ps_thd_val_low) - obj->ps_cali;
		RGW_ERR("%s get threshold low: 0x%x\n", __func__, threshold[0]);
		if (copy_to_user(ptr, &threshold[0], sizeof(threshold[0]))) {
			err = -EFAULT;
			goto err_out;
		}
		break;
#endif
	default:
		RGW_ERR("%s not supported = 0x%04x", __func__, cmd);
		err = -ENOIOCTLCMD;
		break;
	}

err_out:
	return err;
}
#ifdef CONFIG_COMPAT
static long cm36652_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long err = 0;

	/* void __user *arg32 = compat_ptr(arg); */

	if (!file->f_op || !file->f_op->unlocked_ioctl)
		return -ENOTTY;

	switch (cmd) {
#if 0
	case COMPAT_ALSPS_SET_ALS_MODE:
		err = file->f_op->unlocked_ioctl(file, ALSPS_SET_ALS_MODE, (unsigned long)arg32);
		break;
	case COMPAT_ALSPS_GET_ALS_MODE:
		err = file->f_op->unlocked_ioctl(file, ALSPS_GET_ALS_MODE, (unsigned long)arg32);
		break;
#if 0
	case COMPAT_ALSPS_GET_RGBW_DATA:
		err = file->f_op->unlocked_ioctl(file, ALSPS_GET_RGBW_DATA, (unsigned long)arg32);
		break;
#endif
	case COMPAT_ALSPS_GET_ALS_RAW_DATA:
		err = file->f_op->unlocked_ioctl(file, ALSPS_GET_ALS_RAW_DATA, (unsigned long)arg32);
		break;
#if 0
	case COMPAT_ALSPS_IOCTL_CLR_CALI:
		err = file->f_op->unlocked_ioctl(file, ALSPS_IOCTL_CLR_CALI, (unsigned long)arg32);
		break;
	case COMPAT_ALSPS_IOCTL_GET_CALI:
		err = file->f_op->unlocked_ioctl(file, ALSPS_IOCTL_GET_CALI, (unsigned long)arg32);
		break;
	case COMPAT_ALSPS_IOCTL_SET_CALI:
		err = file->f_op->unlocked_ioctl(file, ALSPS_IOCTL_SET_CALI, (unsigned long)arg32);
		break;
	case COMPAT_ALSPS_SET_PS_THRESHOLD:
		err = file->f_op->unlocked_ioctl(file, ALSPS_SET_PS_THRESHOLD, (unsigned long)arg32);
		break;
	case COMPAT_ALSPS_GET_PS_THRESHOLD_HIGH:
		err = file->f_op->unlocked_ioctl(file, ALSPS_GET_PS_THRESHOLD_HIGH, (unsigned long)arg32);
		break;
	case COMPAT_ALSPS_GET_PS_THRESHOLD_LOW:
		err = file->f_op->unlocked_ioctl(file, ALSPS_GET_PS_THRESHOLD_LOW, (unsigned long)arg32);
		break;
#endif
#endif
	default:
		RGW_ERR("%s not supported = 0x%04x", __func__, cmd);
		err = -ENOIOCTLCMD;
		break;
	}

	return err;
}
#endif
/*------------------------------misc device related operation functions------------------------------------*/
static const struct file_operations cm36652_fops = {
	.owner = THIS_MODULE,
	.open = cm36652_open,
	.release = cm36652_release,
	.unlocked_ioctl = cm36652_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = cm36652_compat_ioctl,
#endif
};

static struct miscdevice cm36652_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "rgb_w",
	.fops = &cm36652_fops,
};


/*--------------------------------------------------------------------------------*/
static int cm36652_init_client(struct i2c_client *client)
{
	/* struct cm36652_priv *obj = i2c_get_clientdata(client); */
	u8 databuf[3];
	int res = 0;

	databuf[0] = CM36652_REG_CS_CONF;
/* Need Modify if Consider Interrupt Mode */
/*	if (1 == obj->hw->polling_mode)*/
		databuf[1] = 0x11;
/*	else*/
/*		databuf[1] = 0x17;*/
	databuf[2] = 0x0;
	res = CM36652_WRAP_i2c_master_operate(databuf, 0x3, I2C_FLAG_WRITE);
	if (res <= 0) {
		RGW_ERR("i2c_master_send function err\n");
		goto EXIT_ERR;
	}
	/* RGW_LOG("cm36652 ps CM36652_REG_CS_CONF command!\n"); */


	return CM36652_SUCCESS;

EXIT_ERR:
	RGW_ERR("init dev: %d\n", res);
	return res;
}
/*--------------------------------------------------------------------------------*/

/* if use  this typ of enable , Gsensor should report inputEvent(x, y, z ,stats, div) to HAL */
static int rgbw_open_report_data(int open)
{
	/* should queuq work to report event if  is_report_input_direct=true */
	return 0;
}

/* if use  this typ of enable , Gsensor only enabled but not report inputEvent to HAL */

static int rgbw_enable_nodata(int en)
{
	int res = 0;
#ifdef CUSTOM_KERNEL_SENSORHUB
	SCP_SENSOR_HUB_DATA req;
	int len;
#endif /* #ifdef CUSTOM_KERNEL_SENSORHUB */

	/* RGW_LOG("cm36652_obj als enable value = %d\n", en); */

#ifdef CUSTOM_KERNEL_SENSORHUB
#if 0
	if (atomic_read(&cm36652_obj->init_done)) {
		req.activate_req.sensorType = ID_LIGHT;
		req.activate_req.action = SENSOR_HUB_ACTIVATE;
		req.activate_req.enable = en;
		len = sizeof(req.activate_req);
		res = SCP_sensorHub_req_send(&req, &len, 1);
	} else
		RGW_ERR("sensor hub has not been ready!!\n");

	mutex_lock(&cm36652_mutex);
	if (en)
		set_bit(CMC_BIT_ALS, &cm36652_obj->enable);
	else
		clear_bit(CMC_BIT_ALS, &cm36652_obj->enable);
	mutex_unlock(&cm36652_mutex);
#endif
#else /* #ifdef CUSTOM_KERNEL_SENSORHUB */
	mutex_lock(&cm36652_mutex);
	if (en)
		set_bit(CMC_BIT_ALS, &cm36652_obj->enable);
	else
		clear_bit(CMC_BIT_ALS, &cm36652_obj->enable);
	mutex_unlock(&cm36652_mutex);
	mutex_lock(&cm36652_cs_mutex);
	if (en)	/* mean RGB want to en */
		set_bit(CMC_BIT_RGB, &cs_enable);
	else
		clear_bit(CMC_BIT_RGB, &cs_enable);
	mutex_unlock(&cm36652_cs_mutex);
	if (!cm36652_obj) {
		RGW_ERR("cm36652_obj is null!!\n");
		return -1;
	}
	if (en) {
		if (test_bit(CMC_BIT_ALS, &cs_enable) == 0)
			res = cm36652_enable_rgbw(cm36652_obj->client, en);
	} else {
		if (test_bit(CMC_BIT_ALS, &cs_enable) == 0)
			res = cm36652_enable_rgbw(cm36652_obj->client, en);
	}
#endif /* #ifdef CUSTOM_KERNEL_SENSORHUB */
	if (res) {
		RGW_ERR("rgbw_enable_nodata is failed!!\n");
		return -1;
	}
	return 0;
}

static int rgbw_set_delay(u64 ns)
{
	return 0;
}

static int rgbw_get_data(int *value, int *value_g, int *value_b, int *value_w, int *status)
{
	int err = 0;
#ifdef CUSTOM_KERNEL_SENSORHUB
#if 0
	SCP_SENSOR_HUB_DATA req;
	int len;
#endif
#else
	struct cm36652_priv *obj = NULL;
#endif /* #ifdef CUSTOM_KERNEL_SENSORHUB */

#ifdef CUSTOM_KERNEL_SENSORHUB
#if 0
	if (atomic_read(&cm36652_obj->init_done)) {
		req.get_data_req.sensorType = ID_LIGHT;
		req.get_data_req.action = SENSOR_HUB_GET_DATA;
		len = sizeof(req.get_data_req);
		err = SCP_sensorHub_req_send(&req, &len, 1);
	if (err)
		RGW_ERR("SCP_sensorHub_req_send fail!\n");
	else {
		*value = req.get_data_rsp.int16_Data[0];
		*status = SENSOR_STATUS_ACCURACY_MEDIUM;
	}

	if (atomic_read(&cm36652_obj->trace) & CMC_TRC_PS_DATA)
		RGW_LOG("value = %d\n", *value);
	else {
		RGW_ERR("sensor hub hat not been ready!!\n");
		err = -1;
	}
#endif
#else /* #ifdef CUSTOM_KERNEL_SENSORHUB */
	if (!cm36652_obj) {
		RGW_ERR("cm36652_obj is null!!\n");
		return -1;
	}
	obj = cm36652_obj;
	err = cm36652_read_rgbw(obj->client, &obj->als,
		&obj->value_g, &obj->value_b, &obj->value_w);
	if (err)
		err = -1;
	else {
		*value = obj->als/*cm36652_get_als_value(obj, obj->als)*/;
		*value_g = obj->value_g;
		*value_b = obj->value_b;
		*value_w = obj->value_w;
/*		if (*value < 0)
*			err = -1;
*/
		*status = SENSOR_STATUS_ACCURACY_MEDIUM;
	}
#endif /* #ifdef CUSTOM_KERNEL_SENSORHUB */

	return err;
}
#if 0
/* if use  this typ of enable , Gsensor should report inputEvent(x, y, z ,stats, div) to HAL */
static int ps_open_report_data(int open)
{
	/* should queuq work to report event if  is_report_input_direct=true */
	return 0;
}

/* if use  this typ of enable , Gsensor only enabled but not report inputEvent to HAL */

static int ps_enable_nodata(int en)
{
	int res = 0;
#ifdef CUSTOM_KERNEL_SENSORHUB
	SCP_SENSOR_HUB_DATA req;
	int len;
#endif /* #ifdef CUSTOM_KERNEL_SENSORHUB */

	/* RGW_LOG("cm36652_obj als enable value = %d\n", en); */

#ifdef CUSTOM_KERNEL_SENSORHUB
	if (atomic_read(&cm36652_obj->init_done)) {
		req.activate_req.sensorType = ID_PROXIMITY;
		req.activate_req.action = SENSOR_HUB_ACTIVATE;
		req.activate_req.enable = en;
		len = sizeof(req.activate_req);
		res = SCP_sensorHub_req_send(&req, &len, 1);
	} else
		RGW_ERR("sensor hub has not been ready!!\n");

	mutex_lock(&cm36652_mutex);
	if (en)
		set_bit(CMC_BIT_PS, &cm36652_obj->enable);
	else
		clear_bit(CMC_BIT_PS, &cm36652_obj->enable);
	mutex_unlock(&cm36652_mutex);
#else /* #ifdef CUSTOM_KERNEL_SENSORHUB */
	mutex_lock(&cm36652_mutex);
	if (en)
		set_bit(CMC_BIT_PS, &cm36652_obj->enable);

	else
		clear_bit(CMC_BIT_PS, &cm36652_obj->enable);

	mutex_unlock(&cm36652_mutex);
	if (!cm36652_obj) {
		RGW_ERR("cm36652_obj is null!!\n");
		return -1;
	}
	res = cm36652_enable_ps(cm36652_obj->client, en);
#endif /* #ifdef CUSTOM_KERNEL_SENSORHUB */

	if (res) {
		RGW_ERR("ps_enable_nodata is failed!!\n");
		return -1;
	}
	return 0;

}

static int ps_set_delay(u64 ns)
{
	return 0;
}

static int ps_get_data(int *value, int *status)
{
	int err = 0;
#ifdef CUSTOM_KERNEL_SENSORHUB
	SCP_SENSOR_HUB_DATA req;
	int len;
#endif /* #ifdef CUSTOM_KERNEL_SENSORHUB */

#ifdef CUSTOM_KERNEL_SENSORHUB
	if (atomic_read(&cm36652_obj->init_done)) {
		req.get_data_req.sensorType = ID_PROXIMITY;
		req.get_data_req.action = SENSOR_HUB_GET_DATA;
		len = sizeof(req.get_data_req);
		err = SCP_sensorHub_req_send(&req, &len, 1);
	if (err) {
		RGW_ERR("SCP_sensorHub_req_send fail!\n");
		*value = -1;
		err = -1;
	} else {
		*value = req.get_data_rsp.int16_Data[0];
		*status = SENSOR_STATUS_ACCURACY_MEDIUM;
	}

	if (atomic_read(&cm36652_obj->trace) & CMC_TRC_PS_DATA)
		RGW_LOG("value = %d\n", *value)
	else {
		RGW_ERR("sensor hub has not been ready!!\n");
		err = -1;
	}
#else /* #ifdef CUSTOM_KERNEL_SENSORHUB */
	if (!cm36652_obj) {
		RGW_ERR("cm36652_obj is null!!\n");
		return -1;
	}

	err = cm36652_read_ps(cm36652_obj->client, &cm36652_obj->ps);
	if (err)
		err = -1;
	else {
		*value = cm36652_get_ps_value(cm36652_obj, cm36652_obj->ps);
		if (*value < 0)
			err = -1;
		*status = SENSOR_STATUS_ACCURACY_MEDIUM;
	}
#endif /* #ifdef CUSTOM_KERNEL_SENSORHUB */

	return err;
}
#endif

/*-----------------------------------i2c operations----------------------------------*/
static int cm36652_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct cm36652_priv *obj;
	struct rgbw_control_path rgbw_ctl = {0};
	struct rgbw_data_path rgbw_data = {0};
	int err = 0;

	RGW_LOG("cm36652_i2c_probe\n");

	obj = kzalloc(sizeof(*obj), GFP_KERNEL);
	if (!obj) {
		err = -ENOMEM;
		goto exit;
	}

	memset(obj, 0, sizeof(*obj));
	cm36652_obj = obj;
	obj->hw = hw;
#ifdef CUSTOM_KERNEL_SENSORHUB
	INIT_WORK(&obj->init_done_work, cm36652_init_done_work);
#endif /* #ifdef CUSTOM_KERNEL_SENSORHUB */

	obj->client = client;
	i2c_set_clientdata(client, obj);

	/*-----------------------------value need to be confirmed-----------------------------------------*/
	atomic_set(&obj->rgbw_debounce, 200);
	atomic_set(&obj->rgbw_deb_on, 0);
	atomic_set(&obj->rgbw_deb_end, 0);
	atomic_set(&obj->rgbw_suspend, 0);
#if 0
	atomic_set(&obj->als_cmd_val, 0xDF);
	atomic_set(&obj->als_thd_val_high,  obj->hw->als_threshold_high);
	atomic_set(&obj->als_thd_val_low,  obj->hw->als_threshold_low);
	atomic_set(&obj->init_done,  0);
	obj->irq_node = of_find_compatible_node(NULL, NULL, "mediatek, als-eint");

	obj->enable = 0;
	obj->pending_intr = 0;
	obj->als_level_num = ARRAY_SIZE(obj->hw->als_level);
	obj->als_value_num = ARRAY_SIZE(obj->hw->als_value);
#endif
	/*-----------------------------value need to be confirmed-----------------------------------------*/

	atomic_set(&obj->i2c_retry, 3);
	clear_bit(CMC_BIT_ALS, &obj->enable);
	/*clear_bit(CMC_BIT_PS, &obj->enable);*/

	cm36652_i2c_client = client;

	err = cm36652_init_client(client);
	if (err)
		goto exit_init_failed;
	RGW_LOG("cm36652_init_client() OK!\n");

	err = misc_register(&cm36652_device);
	if (err) {
		RGW_ERR("cm36652_device register failed\n");
		goto exit_misc_device_register_failed;
	}
	rgbw_ctl.is_use_common_factory = false;
	RGW_LOG("cm36652_device misc_register OK!\n");

	/*------------------------cm36652 attribute file for debug--------------------------------------*/
	err = cm36652_create_attr(&(cm36652rgbw_init_info.platform_diver_addr->driver));
	if (err) {
		RGW_ERR("create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}
	/*------------------------cm36652 attribute file for debug--------------------------------------*/
	rgbw_ctl.open_report_data = rgbw_open_report_data;
	rgbw_ctl.enable_nodata = rgbw_enable_nodata;
	rgbw_ctl.set_delay  = rgbw_set_delay;
	rgbw_ctl.is_report_input_direct = false;
#ifdef CUSTOM_KERNEL_SENSORHUB
	rgbw_ctl.is_support_batch = obj->hw->is_batch_supported;
#else
	rgbw_ctl.is_support_batch = false;
#endif

	err = rgbw_register_control_path(&rgbw_ctl);
	if (err) {
		RGW_ERR("register fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}

	rgbw_data.get_data = rgbw_get_data;
	rgbw_data.vender_div = 100;
	err = rgbw_register_data_path(&rgbw_data);
	if (err) {
		RGW_ERR("tregister fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}


	/*err = batch_register_support_info(ID_LIGHT, rgbw_ctl.is_support_batch, 1, 0);
	*if (err)
	*	RGW_ERR("register light batch support err = %d\n", err);
	*/

	rgbw_init_flag = 0;
	RGW_LOG("%s: OK\n", __func__);
	return 0;

exit_create_attr_failed:
exit_sensor_obj_attach_fail:
exit_misc_device_register_failed:
		misc_deregister(&cm36652_device);
exit_init_failed:
		kfree(obj);
exit:
	cm36652_i2c_client = NULL;
	RGW_ERR("%s: err = %d\n", __func__, err);
	rgbw_init_flag =  -1;
	return err;
}

static int cm36652_i2c_remove(struct i2c_client *client)
{
	int err;

	err = cm36652_delete_attr(&(cm36652rgbw_init_info.platform_diver_addr->driver));
	if (err)
		RGW_ERR("cm36652_delete_attr fail: %d\n", err);

	misc_deregister(&cm36652_device);
	RGW_ERR("misc_deregister\n");

	cm36652_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));
	return 0;

}

static int cm36652_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	strncpy(info->type, CM36652_DEV_NAME, strlen(CM36652_DEV_NAME));
	return 0;

}

static int cm36652_i2c_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cm36652_priv *obj = i2c_get_clientdata(client);
	int err;

	/* RGW_LOG("cm36652_i2c_suspend\n"); */

	if (!obj) {
		RGW_ERR("null pointer!!\n");
		return 0;
	}

	atomic_set(&obj->rgbw_suspend, 1);
	err = cm36652_enable_rgbw(obj->client, 0);
	if (err)
		RGW_ERR("disable als fail: %d\n", err);

	return 0;
}

static int cm36652_i2c_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cm36652_priv *obj = i2c_get_clientdata(client);
	int err;
	struct hwm_sensor_data sensor_data;

	memset(&sensor_data, 0, sizeof(sensor_data));
	/* RGW_LOG("cm36652_i2c_resume\n"); */
	if (!obj) {
		RGW_ERR("null pointer!!\n");
		return 0;
	}

	atomic_set(&obj->rgbw_suspend, 0);
	if (test_bit(CMC_BIT_ALS, &obj->enable)) {
		err = cm36652_enable_rgbw(obj->client, 1);
		if (err)
			RGW_ERR("enable als fail: %d\n", err);
	}
	return 0;
}

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static int rgbw_remove(void)
{
	cm36652_power(hw, 0);/* ***************** */

	i2c_del_driver(&cm36652_i2c_driver);
	return 0;
}
/*----------------------------------------------------------------------------*/

static int rgbw_local_init(void)
{
	/* printk("fwq loccal init+++\n"); */

	cm36652_power(hw, 1);
	if (i2c_add_driver(&cm36652_i2c_driver)) {
		RGW_ERR("add driver error\n");
		return -1;
	}
	if (-1 == rgbw_init_flag)
		return -1;

	return 0;
}


/*----------------------------------------------------------------------------*/
static int __init cm36652_init(void)
{
	const char *name = "mediatek,cm36652rgbw";

	hw = get_rgbw_dts_func(name, hw);
	if (!hw)
		RGW_ERR("get dts info fail\n");
	rgbw_driver_add(&cm36652rgbw_init_info);
	return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit cm36652_exit(void)
{
	RGBW_FUN();
}
/*----------------------------------------------------------------------------*/
module_init(cm36652_init);
module_exit(cm36652_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("yucong xiong");
MODULE_DESCRIPTION("cm36652 driver");
MODULE_LICENSE("GPL");

