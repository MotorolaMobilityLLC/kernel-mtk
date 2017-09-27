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
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <linux/atomic.h>
#include <linux/io.h>
#include "cust_alsps.h"
#include "cm36686.h"
#include <linux/sched.h>
#include <alsps.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
/******************************************************************************
 * configuration
*******************************************************************************/
/*----------------------------------------------------------------------------*/

#define CM36686_DEV_NAME     "cm36686"
/*----------------------------------------------------------------------------*/
#define APS_TAG                  "[ALS/PS] "
#define APS_FUN(f)               pr_err(APS_TAG"%s\n", __func__)
#define APS_ERR(fmt, args...)    pr_err(APS_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#define APS_LOG(fmt, args...)    pr_err(APS_TAG fmt, ##args)
#define APS_DBG(fmt, args...)    pr_err(APS_TAG fmt, ##args)

#define I2C_FLAG_WRITE	0
#define I2C_FLAG_READ	1

/******************************************************************************
 * extern functions
*******************************************************************************/
static int cm36686_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int cm36686_i2c_remove(struct i2c_client *client);
static int cm36686_i2c_detect(struct i2c_client *client, struct i2c_board_info *info);
static int cm36686_i2c_suspend(struct device *dev);
static int cm36686_i2c_resume(struct device *dev);

/*----------------------------------------------------------------------------*/
static const struct i2c_device_id cm36686_i2c_id[] = { {CM36686_DEV_NAME, 0}, {} };

static unsigned long long int_top_time;

/*----------------------------------------------------------------------------*/
struct cm36686_priv {
	struct alsps_hw hw;
	struct i2c_client *client;
	struct work_struct eint_work;

	/*misc */
	u16 als_modulus;
	atomic_t i2c_retry;
	atomic_t als_suspend;
	atomic_t als_debounce;	/*debounce time after enabling als */
	atomic_t als_deb_on;	/*indicates if the debounce is on */
#ifdef CONFIG_64BIT
	atomic64_t als_deb_end;	/*the jiffies representing the end of debounce */
#else
	atomic_t als_deb_end;	/*the jiffies representing the end of debounce */
#endif
	atomic_t ps_mask;	/*mask ps: always return far away */
	atomic_t ps_debounce;	/*debounce time after enabling ps */
	atomic_t ps_deb_on;	/*indicates if the debounce is on */
#ifdef CONFIG_64BIT
	atomic64_t ps_deb_end;	/*the jiffies representing the end of debounce */
#else
	atomic_t ps_deb_end;	/*the jiffies representing the end of debounce */
#endif
	atomic_t ps_suspend;
	atomic_t trace;
	atomic_t init_done;
	struct device_node *irq_node;
	int irq;

	/*data */
	u16 als;
	u16 ps;
	/* u8                   _align; */
	u16 als_level_num;
	u16 als_value_num;
	u32 als_level[C_CUST_ALS_LEVEL - 1];
	u32 als_value[C_CUST_ALS_LEVEL];
	int ps_cali;

	atomic_t als_cmd_val;	/*the cmd value can't be read, stored in ram */
	atomic_t ps_cmd_val;	/*the cmd value can't be read, stored in ram */
	atomic_t ps_thd_val_high;	/*the cmd value can't be read, stored in ram */
	atomic_t ps_thd_val_low;	/*the cmd value can't be read, stored in ram */
	atomic_t als_thd_val_high;	/*the cmd value can't be read, stored in ram */
	atomic_t als_thd_val_low;	/*the cmd value can't be read, stored in ram */
	atomic_t ps_thd_val;
	ulong enable;		/*enable mask */
	ulong pending_intr;	/*pending interrupt */
};
/*----------------------------------------------------------------------------*/

#ifdef CONFIG_OF
static const struct of_device_id alsps_of_match[] = {
	{.compatible = "mediatek,alsps"},
	{},
};
#endif

#ifdef CONFIG_PM_SLEEP
static const struct dev_pm_ops CM36686_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(cm36686_i2c_suspend, cm36686_i2c_resume)
};
#endif

static struct i2c_driver cm36686_i2c_driver = {
	.probe = cm36686_i2c_probe,
	.remove = cm36686_i2c_remove,
	.detect = cm36686_i2c_detect,
	.id_table = cm36686_i2c_id,
	.driver = {
		   .name = CM36686_DEV_NAME,
#ifdef CONFIG_PM_SLEEP
		.pm   = &CM36686_pm_ops,
#endif
#ifdef CONFIG_OF
		   .of_match_table = alsps_of_match,
#endif
		   },
};

/*----------------------------------------------------------------------------*/
struct PS_CALI_DATA_STRUCT {
	int close;
	int far_away;
	int valid;
};

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static struct i2c_client *cm36686_i2c_client;
static struct cm36686_priv *cm36686_obj;
static int intr_flag = 1;	/* hw default away after enable. */

static int cm36686_local_init(void);
static int cm36686_remove(void);
static int cm36686_init_flag = -1;	/* 0<==>OK -1 <==> fail */
static struct alsps_init_info cm36686_init_info = {
	.name = "cm36686",
	.init = cm36686_local_init,
	.uninit = cm36686_remove,
};

/*----------------------------------------------------------------------------*/
static DEFINE_MUTEX(cm36686_mutex);
/*----------------------------------------------------------------------------*/
typedef enum {
	CMC_BIT_ALS = 1,
	CMC_BIT_PS = 2,
} CMC_BIT;
/*-----------------------------CMC for debugging-------------------------------*/
typedef enum {
	CMC_TRC_ALS_DATA = 0x0001,
	CMC_TRC_PS_DATA = 0x0002,
	CMC_TRC_EINT = 0x0004,
	CMC_TRC_IOCTL = 0x0008,
	CMC_TRC_I2C = 0x0010,
	CMC_TRC_CVT_ALS = 0x0020,
	CMC_TRC_CVT_PS = 0x0040,
	CMC_TRC_CVT_AAL = 0x0080,
	CMC_TRC_DEBUG = 0x8000,
} CMC_TRC;
/*-----------------------------------------------------------------------------*/

int CM36686_i2c_master_operate(struct i2c_client *client, const char *buf, int count, int i2c_flag)
{
	int res = 0;
#ifndef CONFIG_MTK_I2C_EXTENSION
	struct i2c_msg msg[2];
#endif

	mutex_lock(&cm36686_mutex);
	switch (i2c_flag) {
	case I2C_FLAG_WRITE:
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
		res = i2c_master_send(client, buf, (count << 8) | 1);
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
		APS_LOG("CM36686_i2c_master_operate i2c_flag command not support!\n");
		break;
	}

	if (res < 0)
		goto EXIT_ERR;

	mutex_unlock(&cm36686_mutex);
	return res;
EXIT_ERR:
	mutex_unlock(&cm36686_mutex);
	APS_ERR("CM36686_i2c_master_operate fail\n");
	return res;
}

/********************************************************************/
int cm36686_enable_ps(struct i2c_client *client, int enable)
{
	struct cm36686_priv *obj = i2c_get_clientdata(client);
	int res;
	u8 databuf[3];

	if (enable == 1) {
		APS_LOG("cm36686_enable_ps enable_ps\n");
		databuf[0] = CM36686_REG_PS_CONF1_2;
		res = CM36686_i2c_master_operate(client, databuf, 2, I2C_FLAG_READ);
		if (res < 0) {
			APS_ERR("i2c_master_send function err\n");
			goto ENABLE_PS_EXIT_ERR;
		}
		APS_LOG("CM36686_REG_PS_CONF1_2 value value_low = %x, value_high = %x\n",
			databuf[0], databuf[1]);
		databuf[2] = databuf[1];
		databuf[1] = databuf[0] & 0xFE;

		databuf[0] = CM36686_REG_PS_CONF1_2;
		res = CM36686_i2c_master_operate(client, databuf, 0x3, I2C_FLAG_WRITE);
		if (res < 0) {
			APS_ERR("i2c_master_send function err\n");
			goto ENABLE_PS_EXIT_ERR;
		}
		atomic_set(&obj->ps_deb_on, 1);
#ifdef CONFIG_64BIT
		atomic64_set(&obj->ps_deb_end,
			     jiffies + atomic_read(&obj->ps_debounce) / (1000 / HZ));
#else
		atomic_set(&obj->ps_deb_end,
			   jiffies + atomic_read(&obj->ps_debounce) / (1000 / HZ));
#endif
		intr_flag = 1;	/* reset hw status to away after enable. */
	} else {
		APS_LOG("cm36686_enable_ps disable_ps\n");
		databuf[0] = CM36686_REG_PS_CONF1_2;
		res = CM36686_i2c_master_operate(client, databuf, 2, I2C_FLAG_READ);
		if (res < 0) {
			APS_ERR("i2c_master_send function err\n");
			goto ENABLE_PS_EXIT_ERR;
		}

		APS_LOG("CM36686_REG_PS_CONF1_2 value value_low = %x, value_high = %x\n",
			databuf[0], databuf[1]);

		databuf[2] = databuf[1];
		databuf[1] = databuf[0] | 0x01;
		databuf[0] = CM36686_REG_PS_CONF1_2;

		res = CM36686_i2c_master_operate(client, databuf, 0x3, I2C_FLAG_WRITE);
		if (res < 0) {
			APS_ERR("i2c_master_send function err\n");
			goto ENABLE_PS_EXIT_ERR;
		}
		atomic_set(&obj->ps_deb_on, 0);

	}

	return 0;
ENABLE_PS_EXIT_ERR:
	return res;
}

/********************************************************************/
int cm36686_enable_als(struct i2c_client *client, int enable)
{
	struct cm36686_priv *obj = i2c_get_clientdata(client);
	int res;
	u8 databuf[3];

	if (enable == 1) {
		APS_LOG("cm36686_enable_als enable_als\n");
		databuf[0] = CM36686_REG_ALS_CONF;
		res = CM36686_i2c_master_operate(client, databuf, 2, I2C_FLAG_READ);
		if (res < 0) {
			APS_ERR("i2c_master_send function err\n");
			goto ENABLE_ALS_EXIT_ERR;
		}

		APS_LOG("CM36686_REG_ALS_CONF value value_low = %x, value_high = %x\n", databuf[0],
			databuf[1]);

		databuf[2] = databuf[1];
		databuf[1] = databuf[0] & 0xFE;
		databuf[0] = CM36686_REG_ALS_CONF;

		res = CM36686_i2c_master_operate(client, databuf, 0x3, I2C_FLAG_WRITE);
		if (res < 0) {
			APS_ERR("i2c_master_send function err\n");
			goto ENABLE_ALS_EXIT_ERR;
		}
		atomic_set(&obj->als_deb_on, 1);
#ifdef CONFIG_64BIT
		atomic64_set(&obj->als_deb_end,
			     jiffies + atomic_read(&obj->als_debounce) / (1000 / HZ));
#else
		atomic_set(&obj->als_deb_end,
			   jiffies + atomic_read(&obj->als_debounce) / (1000 / HZ));
#endif

	} else {
		APS_LOG("cm36686_enable_als disable_als\n");
		databuf[0] = CM36686_REG_ALS_CONF;
		res = CM36686_i2c_master_operate(client, databuf, 2, I2C_FLAG_READ);
		if (res < 0) {
			APS_ERR("i2c_master_send function err\n");
			goto ENABLE_ALS_EXIT_ERR;
		}

		APS_LOG("CM36686_REG_ALS_CONF value value_low = %x, value_high = %x\n", databuf[0],
			databuf[1]);

		databuf[2] = databuf[1];
		databuf[1] = databuf[0] | 0x01;
		databuf[0] = CM36686_REG_ALS_CONF;

		res = CM36686_i2c_master_operate(client, databuf, 0x3, I2C_FLAG_WRITE);
		if (res < 0) {
			APS_ERR("i2c_master_send function err\n");
			goto ENABLE_ALS_EXIT_ERR;
		}
		atomic_set(&obj->als_deb_on, 0);
	}
	return 0;
ENABLE_ALS_EXIT_ERR:
	return res;
}

/********************************************************************/
long cm36686_read_ps(struct i2c_client *client, u16 *data)
{
	long res;
	u8 databuf[2];
	struct cm36686_priv *obj = i2c_get_clientdata(client);

	databuf[0] = CM36686_REG_PS_DATA;
	res = CM36686_i2c_master_operate(client, databuf, 2, I2C_FLAG_READ);
	if (res < 0) {
		APS_ERR("i2c_master_send function err\n");
		goto READ_PS_EXIT_ERR;
	}

	if (atomic_read(&obj->trace) & CMC_TRC_DEBUG) {
		APS_LOG("CM36686_REG_PS_DATA value value_low = %x, value_high = %x\n", databuf[0],
			databuf[1]);
	}

	*data = ((databuf[1] << 8) | databuf[0]);

	if (*data < obj->ps_cali)
		*data = 0;
	else
		*data = *data - obj->ps_cali;
	return 0;
READ_PS_EXIT_ERR:
	return res;
}

/********************************************************************/
long cm36686_read_als(struct i2c_client *client, u16 *data)
{
	long res;
	u8 databuf[2];
	struct cm36686_priv *obj = i2c_get_clientdata(client);

	databuf[0] = CM36686_REG_ALS_DATA;
	res = CM36686_i2c_master_operate(client, databuf, 2, I2C_FLAG_READ);
	if (res < 0) {
		APS_ERR("i2c_master_send function err\n");
		goto READ_ALS_EXIT_ERR;
	}

	if (atomic_read(&obj->trace) & CMC_TRC_DEBUG)
		APS_LOG("CM36686_REG_ALS_DATA value: %d\n", ((databuf[1] << 8) | databuf[0]));

	*data = ((databuf[1] << 8) | databuf[0]);

	return 0;
READ_ALS_EXIT_ERR:
	return res;
}

/********************************************************************/
static int cm36686_get_ps_value(struct cm36686_priv *obj, u16 ps)
{
	int val, mask = atomic_read(&obj->ps_mask);
	int invalid = 0;

	val = intr_flag;	/* value between high/low threshold should sync. with hw status. */

	if (ps > atomic_read(&obj->ps_thd_val_high))
		val = 0;	/*close */
	else if (ps < atomic_read(&obj->ps_thd_val_low))
		val = 1;	/*far away */

	if (atomic_read(&obj->ps_suspend)) {
		invalid = 1;
	} else if (atomic_read(&obj->ps_deb_on) == 1) {
#ifdef CONFIG_64BIT
		unsigned long endt = atomic64_read(&obj->ps_deb_end);
#else
		unsigned long endt = atomic_read(&obj->ps_deb_end);
#endif
		if (time_after(jiffies, endt))
			atomic_set(&obj->ps_deb_on, 0);

		if (atomic_read(&obj->ps_deb_on) == 1)
			invalid = 1;
	}

	if (!invalid) {
		if (unlikely(atomic_read(&obj->trace) & CMC_TRC_CVT_PS)) {
			if (mask)
				APS_DBG("PS:  %05d => %05d [M]\n", ps, val);
			else
				APS_DBG("PS:  %05d => %05d\n", ps, val);
		}
		if (test_bit(CMC_BIT_PS, &obj->enable) == 0) {
			/* if ps is disable do not report value */
			APS_DBG("PS: not enable and do not report this value\n");
			return -1;
		} else {
			return val;
		}

	} else {
		if (unlikely(atomic_read(&obj->trace) & CMC_TRC_CVT_PS))
			APS_DBG("PS:  %05d => %05d (-1)\n", ps, val);
		return -1;
	}
}

/********************************************************************/
static int cm36686_get_als_value(struct cm36686_priv *obj, u16 als)
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
		APS_ERR("invalid als_level_num = %d, als_value_num = %d\n", obj->als_level_num,
			obj->als_value_num);
		return -1;
	}

	if (atomic_read(&obj->als_deb_on) == 1) {
#ifdef CONFIG_64BIT
		unsigned long endt = (unsigned long)atomic64_read(&obj->als_deb_end);
#else
		unsigned long endt = (unsigned long)atomic_read(&obj->als_deb_end);
#endif
		if (time_after(jiffies, endt))
			atomic_set(&obj->als_deb_on, 0);

		if (atomic_read(&obj->als_deb_on) == 1)
			invalid = 1;
	}

	for (idx = 0; idx < obj->als_level_num; idx++) {
		if (als < obj->hw.als_level[idx])
			break;
	}

	if (idx >= obj->als_level_num || idx >= obj->als_value_num) {
		if (idx < obj->als_value_num)
			value = obj->hw.als_value[idx - 1];
		else
			value = obj->hw.als_value[obj->als_value_num - 1];
	} else {
		level_high = obj->hw.als_level[idx];
		level_low = (idx > 0) ? obj->hw.als_level[idx - 1] : 0;
		level_diff = level_high - level_low;
		value_high = obj->hw.als_value[idx];
		value_low = (idx > 0) ? obj->hw.als_value[idx - 1] : 0;
		value_diff = value_high - value_low;

		if ((level_low >= level_high) || (value_low >= value_high))
			value = value_low;
		else
			value =
			    (level_diff * value_low + (als - level_low) * value_diff +
			     ((level_diff + 1) >> 1)) / level_diff;
	}

	if (!invalid) {
		if (atomic_read(&obj->trace) & CMC_TRC_CVT_AAL)
			APS_DBG("ALS: %d [%d, %d] => %d [%d, %d]\n", als, level_low, level_high,
				value, value_low, value_high);
		return value;

	} else {
		if (atomic_read(&obj->trace) & CMC_TRC_CVT_ALS)
			APS_DBG("ALS: %05d => %05d (-1)\n", als, value);

		return -1;
	}

}


/*-------------------------------attribute file for debugging----------------------------------*/

/******************************************************************************
 * Sysfs attributes
*******************************************************************************/
static ssize_t cm36686_show_config(struct device_driver *ddri, char *buf)
{
	ssize_t res;

	if (!cm36686_obj) {
		APS_ERR("cm36686_obj is null!!\n");
		return 0;
	}

	res = snprintf(buf, PAGE_SIZE, "(%d %d %d %d %d)\n",
		       atomic_read(&cm36686_obj->i2c_retry),
		       atomic_read(&cm36686_obj->als_debounce), atomic_read(&cm36686_obj->ps_mask),
		       atomic_read(&cm36686_obj->ps_thd_val),
		       atomic_read(&cm36686_obj->ps_debounce));
	return res;
}

/*----------------------------------------------------------------------------*/
static ssize_t cm36686_store_config(struct device_driver *ddri, const char *buf, size_t count)
{
	int retry, als_deb, ps_deb, mask, thres;

	if (!cm36686_obj) {
		APS_ERR("cm36686_obj is null!!\n");
		return 0;
	}

	if (sscanf(buf, "%d %d %d %d %d", &retry, &als_deb, &mask, &thres, &ps_deb) == 5) {
		atomic_set(&cm36686_obj->i2c_retry, retry);
		atomic_set(&cm36686_obj->als_debounce, als_deb);
		atomic_set(&cm36686_obj->ps_mask, mask);
		atomic_set(&cm36686_obj->ps_thd_val, thres);
		atomic_set(&cm36686_obj->ps_debounce, ps_deb);
	} else {
		APS_ERR("invalid content: '%s', length = %zu\n", buf, count);
	}
	return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t cm36686_show_trace(struct device_driver *ddri, char *buf)
{
	ssize_t res;

	if (!cm36686_obj) {
		APS_ERR("cm36686_obj is null!!\n");
		return 0;
	}

	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&cm36686_obj->trace));
	return res;
}

/*----------------------------------------------------------------------------*/
static ssize_t cm36686_store_trace(struct device_driver *ddri, const char *buf, size_t count)
{
	int trace;

	if (!cm36686_obj) {
		APS_ERR("cm36686_obj is null!!\n");
		return 0;
	}

	if (sscanf(buf, "0x%x", &trace) == 1)
		atomic_set(&cm36686_obj->trace, trace);
	else
		APS_ERR("invalid content: '%s', length = %zu\n", buf, count);

	return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t cm36686_show_als(struct device_driver *ddri, char *buf)
{
	int res;

	if (!cm36686_obj) {
		APS_ERR("cm36686_obj is null!!\n");
		return 0;
	}
	res = cm36686_read_als(cm36686_obj->client, &cm36686_obj->als);
	if (res)
		return snprintf(buf, PAGE_SIZE, "ERROR: %d\n", res);
	else
		return snprintf(buf, PAGE_SIZE, "0x%04X\n", cm36686_obj->als);
}

/*----------------------------------------------------------------------------*/
static ssize_t cm36686_show_ps(struct device_driver *ddri, char *buf)
{
	ssize_t res;

	if (!cm36686_obj) {
		APS_ERR("cm3623_obj is null!!\n");
		return 0;
	}

	res = cm36686_read_ps(cm36686_obj->client, &cm36686_obj->ps);
	if (res)
		return snprintf(buf, PAGE_SIZE, "ERROR: %d\n", (int)res);
	else
		return snprintf(buf, PAGE_SIZE, "0x%04X\n", cm36686_obj->ps);
}

/*----------------------------------------------------------------------------*/
static ssize_t cm36686_show_reg(struct device_driver *ddri, char *buf)
{
	u8 _bIndex = 0;
	u8 databuf[2] = { 0 };
	ssize_t _tLength = 0;
	int res;

	if (!cm36686_obj) {
		APS_ERR("cm3623_obj is null!!\n");
		return 0;
	}

	for (_bIndex = 0; _bIndex < 0x0D; _bIndex++) {
		databuf[0] = _bIndex;
		res = CM36686_i2c_master_operate(cm36686_obj->client, databuf, 2, I2C_FLAG_READ);
		if (res < 0)
			APS_ERR("i2c_master_send function err res = %d\n", res);

		_tLength +=
		    snprintf((buf + _tLength), (PAGE_SIZE - _tLength), "Reg[0x%02X]: 0x%02X\n",
			     _bIndex, databuf[0]);
	}

	return _tLength;

}

/*----------------------------------------------------------------------------*/
static ssize_t cm36686_show_send(struct device_driver *ddri, char *buf)
{
	return 0;
}

/*----------------------------------------------------------------------------*/
static ssize_t cm36686_store_send(struct device_driver *ddri, const char *buf, size_t count)
{
	int addr, cmd;
	u8 dat;

	if (!cm36686_obj) {
		APS_ERR("cm36686_obj is null!!\n");
		return 0;
	} else if (sscanf(buf, "%x %x", &addr, &cmd) != 2) {
		APS_ERR("invalid format: '%s'\n", buf);
		return 0;
	}

	dat = (u8) cmd;

	return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t cm36686_show_recv(struct device_driver *ddri, char *buf)
{
	return 0;
}

/*----------------------------------------------------------------------------*/
static ssize_t cm36686_store_recv(struct device_driver *ddri, const char *buf, size_t count)
{
	int addr;
	int ret;

	if (!cm36686_obj) {
		APS_ERR("cm36686_obj is null!!\n");
		return 0;
	}
	ret = kstrtoint(buf, 16, &addr);
	if (ret < 0) {
		APS_ERR("invalid format: '%s'\n", buf);
		return 0;
	}

	return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t cm36686_show_status(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;

	if (!cm36686_obj) {
		APS_ERR("cm36686_obj is null!!\n");
		return 0;
	}

	len += snprintf(buf + len, PAGE_SIZE - len, "CUST: %d, (%d %d)\n",
			cm36686_obj->hw.i2c_num, cm36686_obj->hw.power_id,
			cm36686_obj->hw.power_vol);

	len += snprintf(buf + len, PAGE_SIZE - len, "REGS: %02X %02X %02X %02lX %02lX\n",
			atomic_read(&cm36686_obj->als_cmd_val),
			atomic_read(&cm36686_obj->ps_cmd_val),
			atomic_read(&cm36686_obj->ps_thd_val), cm36686_obj->enable,
			cm36686_obj->pending_intr);

	len +=
	    snprintf(buf + len, PAGE_SIZE - len, "MISC: %d %d\n",
		     atomic_read(&cm36686_obj->als_suspend), atomic_read(&cm36686_obj->ps_suspend));

	return len;
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
#define IS_SPACE(CH) (((CH) == ' ') || ((CH) == '\n'))
/*----------------------------------------------------------------------------*/
static int read_int_from_buf(struct cm36686_priv *obj, const char *buf, size_t count, u32 data[],
			     int len)
{
	int idx = 0;
	int ret;
	char *cur = (char *)buf, *end = (char *)(buf + count);

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

/*----------------------------------------------------------------------------*/
static ssize_t cm36686_show_alslv(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	int idx;

	if (!cm36686_obj) {
		APS_ERR("cm36686_obj is null!!\n");
		return 0;
	}

	for (idx = 0; idx < cm36686_obj->als_level_num; idx++)
		len += snprintf(buf + len, PAGE_SIZE - len, "%d ", cm36686_obj->hw.als_level[idx]);
	len += snprintf(buf + len, PAGE_SIZE - len, "\n");
	return len;
}

/*----------------------------------------------------------------------------*/
static ssize_t cm36686_store_alslv(struct device_driver *ddri, const char *buf, size_t count)
{
	if (!cm36686_obj) {
		APS_ERR("cm36686_obj is null!!\n");
		return 0;
	} else if (!strcmp(buf, "def")) {
		memcpy(cm36686_obj->als_level, cm36686_obj->hw.als_level,
		       sizeof(cm36686_obj->als_level));
	} else if (cm36686_obj->als_level_num !=
		   read_int_from_buf(cm36686_obj, buf, count, cm36686_obj->hw.als_level,
				     cm36686_obj->als_level_num)) {
		APS_ERR("invalid format: '%s'\n", buf);
	}
	return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t cm36686_show_alsval(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	int idx;

	if (!cm36686_obj) {
		APS_ERR("cm36686_obj is null!!\n");
		return 0;
	}

	for (idx = 0; idx < cm36686_obj->als_value_num; idx++)
		len += snprintf(buf + len, PAGE_SIZE - len, "%d ", cm36686_obj->hw.als_value[idx]);
	len += snprintf(buf + len, PAGE_SIZE - len, "\n");
	return len;
}

/*----------------------------------------------------------------------------*/
static ssize_t cm36686_store_alsval(struct device_driver *ddri, const char *buf, size_t count)
{
	if (!cm36686_obj) {
		APS_ERR("cm36686_obj is null!!\n");
		return 0;
	} else if (!strcmp(buf, "def")) {
		memcpy(cm36686_obj->als_value, cm36686_obj->hw.als_value,
		       sizeof(cm36686_obj->als_value));
	} else if (cm36686_obj->als_value_num !=
		   read_int_from_buf(cm36686_obj, buf, count, cm36686_obj->hw.als_value,
				     cm36686_obj->als_value_num)) {
		APS_ERR("invalid format: '%s'\n", buf);
	}
	return count;
}

/*---------------------------------------------------------------------------------------*/
static DRIVER_ATTR(als, S_IWUSR | S_IRUGO, cm36686_show_als, NULL);
static DRIVER_ATTR(ps, S_IWUSR | S_IRUGO, cm36686_show_ps, NULL);
static DRIVER_ATTR(config, S_IWUSR | S_IRUGO, cm36686_show_config, cm36686_store_config);
static DRIVER_ATTR(alslv, S_IWUSR | S_IRUGO, cm36686_show_alslv, cm36686_store_alslv);
static DRIVER_ATTR(alsval, S_IWUSR | S_IRUGO, cm36686_show_alsval, cm36686_store_alsval);
static DRIVER_ATTR(trace, S_IWUSR | S_IRUGO, cm36686_show_trace, cm36686_store_trace);
static DRIVER_ATTR(status, S_IWUSR | S_IRUGO, cm36686_show_status, NULL);
static DRIVER_ATTR(send, S_IWUSR | S_IRUGO, cm36686_show_send, cm36686_store_send);
static DRIVER_ATTR(recv, S_IWUSR | S_IRUGO, cm36686_show_recv, cm36686_store_recv);
static DRIVER_ATTR(reg, S_IWUSR | S_IRUGO, cm36686_show_reg, NULL);
/*----------------------------------------------------------------------------*/
static struct driver_attribute *cm36686_attr_list[] = {
	&driver_attr_als,
	&driver_attr_ps,
	&driver_attr_trace,	/*trace log */
	&driver_attr_config,
	&driver_attr_alslv,
	&driver_attr_alsval,
	&driver_attr_status,
	&driver_attr_send,
	&driver_attr_recv,
	&driver_attr_reg,
};

/*----------------------------------------------------------------------------*/
static int cm36686_create_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(ARRAY_SIZE(cm36686_attr_list));

	if (driver == NULL)
		return -EINVAL;

	for (idx = 0; idx < num; idx++) {
		err = driver_create_file(driver, cm36686_attr_list[idx]);
		if (err) {
			APS_ERR("driver_create_file (%s) = %d\n", cm36686_attr_list[idx]->attr.name,
				err);
			break;
		}
	}
	return err;
}

/*----------------------------------------------------------------------------*/
static int cm36686_delete_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(ARRAY_SIZE(cm36686_attr_list));

	if (!driver)
		return -EINVAL;

	for (idx = 0; idx < num; idx++)
		driver_remove_file(driver, cm36686_attr_list[idx]);

	return err;
}

/*----------------------------------------------------------------------------*/

/*----------------------------------interrupt functions--------------------------------*/
static int cm36686_check_intr(struct i2c_client *client)
{
	int res;
	u8 databuf[2];

	databuf[0] = CM36686_REG_PS_DATA;
	res = CM36686_i2c_master_operate(client, databuf, 2, I2C_FLAG_READ);
	if (res < 0) {
		APS_ERR("i2c_master_send function err res = %d\n", res);
		goto EXIT_ERR;
	}

	APS_LOG("CM36686_REG_PS_DATA value value_low = %x, value_high = %x\n", databuf[0],
		databuf[1]);

	databuf[0] = CM36686_REG_INT_FLAG;
	res = CM36686_i2c_master_operate(client, databuf, 2, I2C_FLAG_READ);
	if (res < 0) {
		APS_ERR("i2c_master_send function err res = %d\n", res);
		goto EXIT_ERR;
	}

	APS_LOG("CM36686_REG_INT_FLAG value value_low = %x, value_high = %x\n", databuf[0],
		databuf[1]);

	if (databuf[1] & 0x02) {
		intr_flag = 0;	/* for close */
	} else if (databuf[1] & 0x01) {
		intr_flag = 1;	/* for away */
	} else {
		res = -1;
		APS_ERR("cm36686_check_intr fail databuf[1]&0x01: %d\n", res);
		goto EXIT_ERR;
	}

	return 0;
EXIT_ERR:
	APS_ERR("cm36686_check_intr dev: %d\n", res);
	return res;
}

/*----------------------------------------------------------------------------*/
static void cm36686_eint_work(struct work_struct *work)
{
	struct cm36686_priv *obj =
	    (struct cm36686_priv *)container_of(work, struct cm36686_priv, eint_work);
	int res = 0;

	APS_LOG("cm36686 int top half time = %lld\n", int_top_time);

	res = cm36686_check_intr(obj->client);
	if (res != 0) {
		goto EXIT_INTR_ERR;
	} else {
		APS_LOG("cm36686 interrupt value = %d\n", intr_flag);
		res = ps_report_interrupt_data(intr_flag);
	}
	enable_irq(obj->irq);
	return;
EXIT_INTR_ERR:
	enable_irq(obj->irq);
	APS_ERR("cm36686_eint_work err: %d\n", res);
}

/*----------------------------------------------------------------------------*/
static void cm36686_eint_func(void)
{
	struct cm36686_priv *obj = cm36686_obj;

	if (!obj)
		return;
	int_top_time = sched_clock();
	schedule_work(&obj->eint_work);
}

#if defined(CONFIG_OF)
static irqreturn_t cm36686_eint_handler(int irq, void *desc)
{
	cm36686_eint_func();
	disable_irq_nosync(cm36686_obj->irq);

	return IRQ_HANDLED;
}
#endif
/*----------------------------------------------------------------------------*/
int cm36686_setup_eint(struct i2c_client *client)
{
#if defined(CONFIG_OF)
	u32 ints[2] = { 0, 0 };
#endif

	int ret;
	struct pinctrl *pinctrl;
	struct pinctrl_state *pins_default;
	struct pinctrl_state *pins_cfg;

	pinctrl = devm_pinctrl_get(&client->dev);
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

	} else
		pinctrl_select_state(pinctrl, pins_cfg);

	if (cm36686_obj->irq_node) {
		of_property_read_u32_array(cm36686_obj->irq_node, "debounce", ints,
					   ARRAY_SIZE(ints));
		gpio_request(ints[0], "p-sensor");
		gpio_set_debounce(ints[0], ints[1]);
		APS_LOG("ints[0] = %d, ints[1] = %d!!\n", ints[0], ints[1]);

		cm36686_obj->irq = irq_of_parse_and_map(cm36686_obj->irq_node, 0);
		APS_LOG("cm36686_obj->irq = %d\n", cm36686_obj->irq);
		if (!cm36686_obj->irq) {
			APS_ERR("irq_of_parse_and_map fail!!\n");
			return -EINVAL;
		}

		if (request_irq
		    (cm36686_obj->irq, cm36686_eint_handler, IRQF_TRIGGER_NONE, "ALS-eint", NULL)) {
			APS_ERR("IRQ LINE NOT AVAILABLE!!\n");
			return -EINVAL;
		}

		enable_irq_wake(cm36686_obj->irq);
		enable_irq(cm36686_obj->irq);
	} else {
		APS_ERR("null irq node!!\n");
		return -EINVAL;
	}

	return 0;
}

/*-------------------------------MISC device related------------------------------------------*/

/************************************************************/
static int set_psensor_threshold(struct i2c_client *client)
{
	struct cm36686_priv *obj = i2c_get_clientdata(client);
	int res = 0;
	u8 databuf[3];

	APS_ERR("set_psensor_threshold function high: 0x%x, low:0x%x\n",
		atomic_read(&obj->ps_thd_val_high), atomic_read(&obj->ps_thd_val_low));
	databuf[0] = CM36686_REG_PS_THDL;
	databuf[1] = atomic_read(&obj->ps_thd_val_low);
	databuf[2] = atomic_read(&obj->ps_thd_val_low) >> 8;
	res = CM36686_i2c_master_operate(client, databuf, 3, I2C_FLAG_WRITE);
	if (res <= 0) {
		APS_ERR("i2c_master_send function err\n");
		return -1;
	}

	databuf[0] = CM36686_REG_PS_THDH;
	databuf[1] = atomic_read(&obj->ps_thd_val_high);
	databuf[2] = atomic_read(&obj->ps_thd_val_high) >> 8;
	res = CM36686_i2c_master_operate(client, databuf, 3, I2C_FLAG_WRITE);
	if (res <= 0) {
		APS_ERR("i2c_master_send function err\n");
		return -1;
	}
	return 0;

}

/*--------------------------------------------------------------------------------*/
static int cm36686_init_client(struct i2c_client *client)
{
	struct cm36686_priv *obj = i2c_get_clientdata(client);
	u8 databuf[3];
	int res = 0;

	APS_FUN();
	databuf[0] = CM36686_REG_ALS_CONF;
	if (obj->hw.polling_mode_als == 1)
		databuf[1] = 0x41; /*0b01000001;*/
	else
		databuf[1] = 0x47; /*0b01000111;*/
	databuf[2] = 0x00; /*0b00000000;*/
	res = CM36686_i2c_master_operate(client, databuf, 0x3, I2C_FLAG_WRITE);
	if (res <= 0) {
		APS_ERR("i2c_master_send function err\n");
		goto EXIT_ERR;
	}
	APS_LOG("cm36686 ps CM36686_REG_ALS_CONF command!\n");

	databuf[0] = CM36686_REG_PS_CONF1_2;
	databuf[1] = 0x63; /*0b01100011;*/
	if (obj->hw.polling_mode_ps == 1)
		databuf[2] = 0x08; /*0b00001000;*/
	else
		databuf[2] = 0x0B; /*0b00001011;*/
	res = CM36686_i2c_master_operate(client, databuf, 0x3, I2C_FLAG_WRITE);
	if (res <= 0) {
		APS_ERR("i2c_master_send function err\n");
		goto EXIT_ERR;
	}
	APS_LOG("cm36686 ps CM36686_REG_PS_CONF1_2 command!\n");

	databuf[0] = CM36686_REG_PS_CONF3_MS;
	databuf[1] = 0x14; /*0b00010100;*/
	databuf[2] = 0x02; /*0b00000010;*/
	res = CM36686_i2c_master_operate(client, databuf, 0x3, I2C_FLAG_WRITE);
	if (res <= 0) {
		APS_ERR("i2c_master_send function err\n");
		goto EXIT_ERR;
	}
	APS_LOG("cm36686 ps CM36686_REG_PS_CONF3_MS command!\n");

	databuf[0] = CM36686_REG_PS_CANC;	/* value need to confirm */
	databuf[1] = 0x00;
	databuf[2] = 0x00;
	res = CM36686_i2c_master_operate(client, databuf, 0x3, I2C_FLAG_WRITE);
	if (res <= 0) {
		APS_ERR("i2c_master_send function err\n");
		goto EXIT_ERR;
	}

	APS_LOG("cm36686 ps CM36686_REG_PS_CANC command!\n");

	if (obj->hw.polling_mode_als == 0) {
		databuf[0] = CM36686_REG_ALS_THDH;
		databuf[1] = 0x00;
		databuf[2] = atomic_read(&obj->als_thd_val_high);
		res = CM36686_i2c_master_operate(client, databuf, 0x3, I2C_FLAG_WRITE);
		if (res <= 0) {
			APS_ERR("i2c_master_send function err\n");
			goto EXIT_ERR;
		}
		databuf[0] = CM36686_REG_ALS_THDL;
		databuf[1] = 0x00;
		databuf[2] = atomic_read(&obj->als_thd_val_low);	/* threshold value need to confirm */
		res = CM36686_i2c_master_operate(client, databuf, 0x3, I2C_FLAG_WRITE);
		if (res <= 0) {
			APS_ERR("i2c_master_send function err\n");
			goto EXIT_ERR;
		}
	}

	if (obj->hw.polling_mode_ps == 0) {
		databuf[0] = CM36686_REG_PS_THDL;
		databuf[1] = atomic_read(&obj->ps_thd_val_low);
		databuf[2] = atomic_read(&obj->ps_thd_val_low) >> 8;
		res = CM36686_i2c_master_operate(client, databuf, 3, I2C_FLAG_WRITE);
		if (res <= 0) {
			APS_ERR("i2c_master_send function err\n");
			goto EXIT_ERR;
		}

		databuf[0] = CM36686_REG_PS_THDH;
		databuf[1] = atomic_read(&obj->ps_thd_val_high);
		databuf[2] = atomic_read(&obj->ps_thd_val_high) >> 8;
		res = CM36686_i2c_master_operate(client, databuf, 3, I2C_FLAG_WRITE);
		if (res <= 0) {
			APS_ERR("i2c_master_send function err\n");
			goto EXIT_ERR;
		}
	}

	res = cm36686_setup_eint(client);
	if (res != 0) {
		APS_ERR("setup eint: %d\n", res);
		return res;
	}

	return CM36686_SUCCESS;

EXIT_ERR:
	APS_ERR("init dev: %d\n", res);
	return res;
}

/*--------------------------------------------------------------------------------*/

/* if use  this typ of enable , Gsensor should report inputEvent(x, y, z ,stats, div) to HAL */
static int als_open_report_data(int open)
{
	/* should queuq work to report event if  is_report_input_direct=true */
	return 0;
}

/* if use  this typ of enable , Gsensor only enabled but not report inputEvent to HAL */

static int als_enable_nodata(int en)
{
	int res = 0;

	APS_LOG("cm36686_obj als enable value = %d\n", en);

	mutex_lock(&cm36686_mutex);
	if (en)
		set_bit(CMC_BIT_ALS, &cm36686_obj->enable);
	else
		clear_bit(CMC_BIT_ALS, &cm36686_obj->enable);
	mutex_unlock(&cm36686_mutex);

	if (!cm36686_obj) {
		APS_ERR("cm36686_obj is null!!\n");
		return -1;
	}

	res = cm36686_enable_als(cm36686_obj->client, en);
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
	struct cm36686_priv *obj = NULL;

	if (!cm36686_obj) {
		APS_ERR("cm36686_obj is null!!\n");
		return -1;
	}
	obj = cm36686_obj;
	err = cm36686_read_als(obj->client, &obj->als);
	if (err) {
		err = -1;
	} else {
		*value = cm36686_get_als_value(obj, obj->als);
		if (*value < 0)
			err = -1;
		*status = SENSOR_STATUS_ACCURACY_MEDIUM;
	}

	return err;
}

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

	APS_LOG("cm36686_obj als enable value = %d\n", en);

	mutex_lock(&cm36686_mutex);
	if (en)
		set_bit(CMC_BIT_PS, &cm36686_obj->enable);

	else
		clear_bit(CMC_BIT_PS, &cm36686_obj->enable);

	mutex_unlock(&cm36686_mutex);
	if (!cm36686_obj) {
		APS_ERR("cm36686_obj is null!!\n");
		return -1;
	}
	res = cm36686_enable_ps(cm36686_obj->client, en);

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

	if (!cm36686_obj) {
		APS_ERR("cm36686_obj is null!!\n");
		return -1;
	}

	err = cm36686_read_ps(cm36686_obj->client, &cm36686_obj->ps);
	if (err) {
		err = -1;
	} else {
		*value = cm36686_get_ps_value(cm36686_obj, cm36686_obj->ps);
		if (*value < 0)
			err = -1;
		*status = SENSOR_STATUS_ACCURACY_MEDIUM;
	}

	return err;
}

static int cm36686_als_factory_enable_sensor(bool enable_disable, int64_t sample_periods_ms)
{
	int err = 0;

	err = als_enable_nodata(enable_disable ? 1 : 0);
	if (err) {
		APS_ERR("%s:%s failed\n", __func__, enable_disable ? "enable" : "disable");
		return -1;
	}
	err = als_batch(0, sample_periods_ms * 1000000, 0);
	if (err) {
		APS_ERR("%s set_batch failed\n", __func__);
		return -1;
	}
	return 0;
}
static int cm36686_als_factory_get_data(int32_t *data)
{
	int status;

	return als_get_data(data, &status);
}
static int cm36686_als_factory_get_raw_data(int32_t *data)
{
	int err = 0;
	struct cm36686_priv *obj = cm36686_obj;

	if (!obj) {
		APS_ERR("obj is null!!\n");
		return -1;
	}

	err = cm36686_read_als(obj->client, &obj->als);
	if (err) {
		APS_ERR("%s failed\n", __func__);
		return -1;
	}
	*data = cm36686_obj->als;

	return 0;
}
static int cm36686_als_factory_enable_calibration(void)
{
	return 0;
}
static int cm36686_als_factory_clear_cali(void)
{
	return 0;
}
static int cm36686_als_factory_set_cali(int32_t offset)
{
	return 0;
}
static int cm36686_als_factory_get_cali(int32_t *offset)
{
	return 0;
}
static int cm36686_ps_factory_enable_sensor(bool enable_disable, int64_t sample_periods_ms)
{
	int err = 0;

	err = ps_enable_nodata(enable_disable ? 1 : 0);
	if (err) {
		APS_ERR("%s:%s failed\n", __func__, enable_disable ? "enable" : "disable");
		return -1;
	}
	err = ps_batch(0, sample_periods_ms * 1000000, 0);
	if (err) {
		APS_ERR("%s set_batch failed\n", __func__);
		return -1;
	}
	return err;
}
static int cm36686_ps_factory_get_data(int32_t *data)
{
	int err = 0, status = 0;

	err = ps_get_data(data, &status);
	if (err < 0)
		return -1;
	return 0;
}
static int cm36686_ps_factory_get_raw_data(int32_t *data)
{
	int err = 0;
	struct cm36686_priv *obj = cm36686_obj;

	err = cm36686_read_ps(obj->client, &obj->ps);
	if (err) {
		APS_ERR("%s failed\n", __func__);
		return -1;
	}
	*data = cm36686_obj->ps;
	return 0;
}
static int cm36686_ps_factory_enable_calibration(void)
{
	return 0;
}
static int cm36686_ps_factory_clear_cali(void)
{
	struct cm36686_priv *obj = cm36686_obj;

	obj->ps_cali = 0;
	return 0;
}
static int cm36686_ps_factory_set_cali(int32_t offset)
{
	struct cm36686_priv *obj = cm36686_obj;

	obj->ps_cali = offset;
	return 0;
}
static int cm36686_ps_factory_get_cali(int32_t *offset)
{
	struct cm36686_priv *obj = cm36686_obj;

	*offset = obj->ps_cali;
	return 0;
}
static int cm36686_ps_factory_set_threashold(int32_t threshold[2])
{
	int err = 0;
	struct cm36686_priv *obj = cm36686_obj;

	APS_ERR("%s set threshold high: 0x%x, low: 0x%x\n", __func__, threshold[0], threshold[1]);
	atomic_set(&obj->ps_thd_val_high, (threshold[0] + obj->ps_cali));
	atomic_set(&obj->ps_thd_val_low, (threshold[1] + obj->ps_cali));
	err = set_psensor_threshold(obj->client);

	if (err < 0) {
		APS_ERR("set_psensor_threshold fail\n");
		return -1;
	}
	return 0;
}
static int cm36686_ps_factory_get_threashold(int32_t threshold[2])
{
	struct cm36686_priv *obj = cm36686_obj;

	threshold[0] = atomic_read(&obj->ps_thd_val_high) - obj->ps_cali;
	threshold[1] = atomic_read(&obj->ps_thd_val_low) - obj->ps_cali;
	return 0;
}

static struct alsps_factory_fops cm36686_factory_fops = {
	.als_enable_sensor = cm36686_als_factory_enable_sensor,
	.als_get_data = cm36686_als_factory_get_data,
	.als_get_raw_data = cm36686_als_factory_get_raw_data,
	.als_enable_calibration = cm36686_als_factory_enable_calibration,
	.als_clear_cali = cm36686_als_factory_clear_cali,
	.als_set_cali = cm36686_als_factory_set_cali,
	.als_get_cali = cm36686_als_factory_get_cali,

	.ps_enable_sensor = cm36686_ps_factory_enable_sensor,
	.ps_get_data = cm36686_ps_factory_get_data,
	.ps_get_raw_data = cm36686_ps_factory_get_raw_data,
	.ps_enable_calibration = cm36686_ps_factory_enable_calibration,
	.ps_clear_cali = cm36686_ps_factory_clear_cali,
	.ps_set_cali = cm36686_ps_factory_set_cali,
	.ps_get_cali = cm36686_ps_factory_get_cali,
	.ps_set_threashold = cm36686_ps_factory_set_threashold,
	.ps_get_threashold = cm36686_ps_factory_get_threashold,
};

static struct alsps_factory_public cm36686_factory_device = {
	.gain = 1,
	.sensitivity = 1,
	.fops = &cm36686_factory_fops,
};
/*-----------------------------------i2c operations----------------------------------*/
static int cm36686_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct cm36686_priv *obj = NULL;

	int err = 0;
	struct als_control_path als_ctl = { 0 };
	struct als_data_path als_data = { 0 };
	struct ps_control_path ps_ctl = { 0 };
	struct ps_data_path ps_data = { 0 };

	APS_FUN();
	/* get customization and power on */
	obj = kzalloc(sizeof(*obj), GFP_KERNEL);
	if (!obj) {
		err = -ENOMEM;
		goto exit;
	}
	err = get_alsps_dts_func(client->dev.of_node, &obj->hw);
	if (err < 0) {
		APS_ERR("get customization info from dts failed\n");
		goto exit_init_failed;
	}

	cm36686_obj = obj;

	INIT_WORK(&obj->eint_work, cm36686_eint_work);

	obj->client = client;
	i2c_set_clientdata(client, obj);

	/*-----------------------------value need to be confirmed-----------------------------------------*/
	atomic_set(&obj->als_debounce, 200);
	atomic_set(&obj->als_deb_on, 0);
#ifdef CONFIG_64BIT
	atomic64_set(&obj->als_deb_end, 0);
#else
	atomic_set(&obj->als_deb_end, 0);
#endif
	atomic_set(&obj->ps_debounce, 200);
	atomic_set(&obj->ps_deb_on, 0);
#ifdef CONFIG_64BIT
	atomic64_set(&obj->ps_deb_end, 0);
#else
	atomic_set(&obj->ps_deb_end, 0);
#endif
	atomic_set(&obj->ps_mask, 0);
	atomic_set(&obj->als_suspend, 0);
	atomic_set(&obj->als_cmd_val, 0xDF);
	atomic_set(&obj->ps_cmd_val, 0xC1);
	atomic_set(&obj->ps_thd_val_high, obj->hw.ps_threshold_high);
	atomic_set(&obj->ps_thd_val_low, obj->hw.ps_threshold_low);
	atomic_set(&obj->als_thd_val_high, obj->hw.als_threshold_high);
	atomic_set(&obj->als_thd_val_low, obj->hw.als_threshold_low);
	atomic_set(&obj->init_done, 0);
	obj->irq_node = client->dev.of_node;

	obj->enable = 0;
	obj->pending_intr = 0;
	obj->ps_cali = 0;
	obj->als_level_num = ARRAY_SIZE(obj->hw.als_level);
	obj->als_value_num = ARRAY_SIZE(obj->hw.als_value);
	/*-----------------------------value need to be confirmed-----------------------------------------*/

	BUG_ON(sizeof(obj->als_level) != sizeof(obj->hw.als_level));
	memcpy(obj->als_level, obj->hw.als_level, sizeof(obj->als_level));
	BUG_ON(sizeof(obj->als_value) != sizeof(obj->hw.als_value));
	memcpy(obj->als_value, obj->hw.als_value, sizeof(obj->als_value));
	atomic_set(&obj->i2c_retry, 3);
	clear_bit(CMC_BIT_ALS, &obj->enable);
	clear_bit(CMC_BIT_PS, &obj->enable);

	cm36686_i2c_client = client;

	err = cm36686_init_client(client);
	if (err)
		goto exit_init_failed;
	APS_LOG("cm36686_init_client() OK!\n");

	err = alsps_factory_device_register(&cm36686_factory_device);
	if (err) {
		APS_ERR("cm36686_device register failed\n");
		goto exit_misc_device_register_failed;
	}
	als_ctl.is_use_common_factory = false;
	ps_ctl.is_use_common_factory = false;
	APS_LOG("cm36686_device misc_register OK!\n");

	/*------------------------cm36686 attribute file for debug--------------------------------------*/
	err = cm36686_create_attr(&(cm36686_init_info.platform_diver_addr->driver));
	if (err) {
		APS_ERR("create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}
	/*------------------------cm36686 attribute file for debug--------------------------------------*/
	als_ctl.open_report_data = als_open_report_data;
	als_ctl.enable_nodata = als_enable_nodata;
	als_ctl.set_delay = als_set_delay;
	als_ctl.batch = als_batch;
	als_ctl.flush = als_flush;
	als_ctl.is_report_input_direct = false;
	als_ctl.is_support_batch = false;

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
	ps_ctl.set_delay = ps_set_delay;
	ps_ctl.batch = ps_batch;
	ps_ctl.flush = ps_flush;
	ps_ctl.is_report_input_direct = true;
	ps_ctl.is_support_batch = false;
	ps_ctl.is_polling_mode = obj->hw.polling_mode_ps;

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

	cm36686_init_flag = 0;
	APS_LOG("%s: OK\n", __func__);
	return 0;

exit_create_attr_failed:
exit_sensor_obj_attach_fail:
exit_misc_device_register_failed:
exit_init_failed:
	kfree(obj);
exit:
	obj = NULL;
	cm36686_i2c_client = NULL;
	cm36686_obj = NULL;
	APS_ERR("%s: err = %d\n", __func__, err);
	cm36686_init_flag = -1;
	return err;
}

static int cm36686_i2c_remove(struct i2c_client *client)
{
	int err;

	/*------------------------cm36686 attribute file for debug--------------------------------------*/
	err = cm36686_delete_attr(&(cm36686_init_info.platform_diver_addr->driver));
	if (err)
		APS_ERR("cm36686_delete_attr fail: %d\n", err);
	/*----------------------------------------------------------------------------------------*/

	alsps_factory_device_deregister(&cm36686_factory_device);

	cm36686_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));
	return 0;

}

static int cm36686_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	strncpy(info->type, CM36686_DEV_NAME, sizeof(info->type));
	return 0;

}

static int cm36686_i2c_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cm36686_priv *obj = i2c_get_clientdata(client);
	int err;

	APS_FUN();

	if (!obj) {
		APS_ERR("null pointer!!\n");
		return 0;
	}

	atomic_set(&obj->als_suspend, 1);
	err = cm36686_enable_als(obj->client, 0);
	if (err)
		APS_ERR("disable als fail: %d\n", err);
	return 0;
}

static int cm36686_i2c_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cm36686_priv *obj = i2c_get_clientdata(client);
	int err;
	struct hwm_sensor_data sensor_data;

	memset(&sensor_data, 0, sizeof(sensor_data));
	APS_FUN();
	if (!obj) {
		APS_ERR("null pointer!!\n");
		return 0;
	}

	atomic_set(&obj->als_suspend, 0);
	if (test_bit(CMC_BIT_ALS, &obj->enable)) {
		err = cm36686_enable_als(obj->client, 1);
		if (err)
			APS_ERR("enable als fail: %d\n", err);
	}
	return 0;
}

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static int cm36686_remove(void)
{
	i2c_del_driver(&cm36686_i2c_driver);
	return 0;
}

/*----------------------------------------------------------------------------*/

static int cm36686_local_init(void)
{
	if (i2c_add_driver(&cm36686_i2c_driver)) {
		APS_ERR("add driver error\n");
		return -1;
	}
	if (-1 == cm36686_init_flag)
		return -1;

	return 0;
}


/*----------------------------------------------------------------------------*/
static int __init cm36686_init(void)
{
	alsps_driver_add(&cm36686_init_info);
	return 0;
}

/*----------------------------------------------------------------------------*/
static void __exit cm36686_exit(void)
{
	APS_FUN();
}

/*----------------------------------------------------------------------------*/
module_init(cm36686_init);
module_exit(cm36686_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("andrew yang");
MODULE_DESCRIPTION("cm36686 driver");
MODULE_LICENSE("GPL");
