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

#define CM36686_DEV_NAME     "CM36686"
#define CM36686_DRIVER_NAME     "cm36686_driver"

#define APS_TAG                  "[ALS/PS] "
#define APS_FUN(f)               pr_debug(APS_TAG"%s\n", __func__)
#define APS_ERR(fmt, args...)    pr_err(APS_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#define APS_LOG(fmt, args...)    pr_debug(APS_TAG fmt, ##args)
#define APS_DBG(fmt, args...)    pr_debug(APS_TAG fmt, ##args)

#define I2C_FLAG_WRITE	0
#define I2C_FLAG_READ	1

static int devProbeCnt;
static int devProbeOkCnt;
static int cm36686_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int cm36686_i2c_remove(struct i2c_client *client);
static int cm36686_i2c_detect(struct i2c_client *client, struct i2c_board_info *info);
static int cm36686_i2c_suspend(struct device *dev);
static int cm36686_i2c_resume(struct device *dev);

#ifdef CM36686_PS_EINT_ENABLE
static unsigned long long int_top_time;
struct platform_device *alspsPltFmDev;
#endif
static struct alsps_hw alsps_cust[DEVICE_NUM_MAX];

struct cm36686_priv {
	struct alsps_hw *hw;
	struct i2c_client *client;
	struct work_struct eint_work;

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
	u16 als;
	u16 ps;
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
static const struct i2c_device_id CM36686_i2c_id[] = { {CM36686_DEV_NAME, 0}, {} };
static struct i2c_driver cm36686_i2c_driver = {
	.probe = cm36686_i2c_probe,
	.remove = cm36686_i2c_remove,
	.detect = cm36686_i2c_detect,
	.id_table = CM36686_i2c_id,
	.driver = {
		.name = CM36686_DRIVER_NAME,
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
static struct i2c_client *cm36686_i2c_client[DEVICE_NUM_MAX];
static struct cm36686_priv *cm36686_obj[DEVICE_NUM_MAX];
static int intr_flag = 1;	/* hw default away after enable. */

static int cm36686_local_init(void);
static int cm36686_remove(void);
#ifndef ONLY_USE_IOCTL
static struct alsps_init_info cm36686_init_info = {
	.name = "cm36686",
	.init = cm36686_local_init,
	.uninit = cm36686_remove,
};
#endif
/*----------------------------------------------------------------------------*/
static DEFINE_MUTEX(cm36686_mutex);
static DEFINE_MUTEX(cm36686_i2c_mutex);
/*----------------------------------------------------------------------------*/
enum CMC_BIT {
	CMC_BIT_ALS = 1,
	CMC_BIT_PS = 2,
};
/*-----------------------------CMC for debugging-------------------------------*/
enum CMC_TRC {
	CMC_TRC_ALS_DATA = 0x0001,
	CMC_TRC_PS_DATA = 0x0002,
	CMC_TRC_EINT = 0x0004,
	CMC_TRC_IOCTL = 0x0008,
	CMC_TRC_I2C = 0x0010,
	CMC_TRC_CVT_ALS = 0x0020,
	CMC_TRC_CVT_PS = 0x0040,
	CMC_TRC_CVT_AAL = 0x0080,
	CMC_TRC_DEBUG = 0x8000,
};

static u8 *g_i2c_buff;
static u8 *g_i2c_addr;

static int msg_dma_alloc(void);
static void msg_dma_release(void);

static int msg_dma_alloc(void)
{
	if (g_i2c_buff ==  NULL) {
		g_i2c_buff = kzalloc(CMP_DMA_MAX_TRANSACTION_LEN, GFP_KERNEL);
		if (!g_i2c_buff) {
			APS_ERR("[DMA][Error] Allocate DMA I2C Buffer failed!\n");
			return -1;
		}
	}

	if (g_i2c_addr ==  NULL) {
		g_i2c_addr = kzalloc(CMP_REG_ADDR_LEN, GFP_KERNEL);
		if (!g_i2c_addr) {
			APS_ERR("[DMA]Allocate DMA I2C addr buf failed!\n");
			kfree(g_i2c_buff);
			g_i2c_buff = NULL;
			return -1;
		}
	}
	return 0;
}

static void msg_dma_release(void)
{
	kfree(g_i2c_buff);
	g_i2c_buff = NULL;

	kfree(g_i2c_addr);
	g_i2c_addr = NULL;
}

static s32 i2c_dma_read(struct i2c_client *client, u8 addr, u8 *rxbuf, int len)
{
	int ret;
	s32 retry = 0;
	struct i2c_msg msg[2];

	if (rxbuf == NULL)
		return -1;

	memcpy(g_i2c_addr, &addr, CMP_REG_ADDR_LEN);

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].buf = g_i2c_addr;
	msg[0].len = CMP_REG_ADDR_LEN;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].buf = g_i2c_buff;
	msg[1].len = len;

	for (retry = 0; retry < 5; ++retry) {
		ret = i2c_transfer(client->adapter, &msg[0], 2);
		if (ret < 0)
			continue;
		memcpy(rxbuf, g_i2c_buff, len);
		return 0;
	}
	APS_ERR("Dma I2C Read Error: 0x%4X, %d bytes, err-code: %d\n", addr, len, ret);
	return ret;
}

static s32 i2c_dma_write(struct i2c_client *client, u8 addr, u8 *txbuf, s32 len)
{
	int ret;
	s32 retry = 0;
	struct i2c_msg msg;

	if ((txbuf == NULL) && (len > 0))
		return -1;

	*g_i2c_buff = addr;

	msg.addr = (client->addr);
	msg.flags = 0;
	msg.buf = g_i2c_buff;
	msg.len = CMP_REG_ADDR_LEN + len;

	if (txbuf)
		memcpy(g_i2c_buff + CMP_REG_ADDR_LEN, txbuf, len);
	for (retry = 0; retry < 5; ++retry) {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (ret < 0)
			continue;
		return 0;
	}
	APS_ERR("Dma I2C Write Error: 0x%04X, %d bytes, err-code: %d\n", addr, len, ret);
	return ret;
}

static int cmp_i2c_read(struct i2c_client *client, u8 addr, u8 *rxbuf, int len)
{
	int left = len;
	int readLen = 0;
	u8 *rd_buf = rxbuf;
	int ret = 0;

	while (left > 0) {
		mutex_lock(&cm36686_i2c_mutex);
		readLen = left > CMP_DMA_MAX_RD_SIZE ? CMP_DMA_MAX_RD_SIZE : left;
		ret = i2c_dma_read(client, addr, rd_buf, readLen);
		mutex_unlock(&cm36686_i2c_mutex);
		if (ret < 0) {
			APS_ERR("dma read failed!\n");
			return -1;
		}

		left -= readLen;
		if (left > 0) {
			addr += readLen;
			rd_buf += readLen;
		}
	}
	return 0;
}

static s32 cmp_i2c_write(struct i2c_client *client, u8 addr, u8 *txbuf, int len)
{

	int ret = 0;
	int write_len = 0;
	int left = len;
	u8 *wr_buf = txbuf;
	u8 offset = 0;
	u8 wrAddr = addr;

	while (left > 0) {
		mutex_lock(&cm36686_i2c_mutex);
		write_len = left > CMP_DMA_MAX_WR_SIZE ? CMP_DMA_MAX_WR_SIZE : left;
		ret = i2c_dma_write(client, wrAddr, wr_buf, write_len);
		mutex_unlock(&cm36686_i2c_mutex);
		if (ret < 0) {
			APS_ERR("dma i2c write failed!\n");
			return -1;
		}
		offset += write_len;
		left -= write_len;
		if (left > 0) {
			wrAddr = addr + offset;
			wr_buf = txbuf + offset;
		}
	}
	return 0;
}

/*----------------------------------------------------------------------------*/
static void cm36686_power(struct alsps_hw *hw, unsigned int on)
{
}

/********************************************************************/
int cm36686_enable_ps(struct i2c_client *client, int enable)
{
	struct cm36686_priv *obj = i2c_get_clientdata(client);
	int res;
	u8 databuf[2];

	if (enable == 1) {
		APS_LOG("cm36686_enable_ps enable_ps\n");
		res = cmp_i2c_read(client, CM36686_REG_PS_CONF1_2, databuf, 2);
		if (res < 0) {
			APS_ERR("i2c_master_send function err\n");
			goto ENABLE_PS_EXIT_ERR;
		}
		APS_LOG("PS_CONF1_2 low = %x, high = %x\n",	databuf[0], databuf[1]);

		databuf[0] &= 0xFE;
		res = cmp_i2c_write(client, CM36686_REG_PS_CONF1_2, databuf, 2);
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
		res = cmp_i2c_read(client, CM36686_REG_PS_CONF1_2, databuf, 2);
		if (res < 0) {
			APS_ERR("i2c_master_send function err\n");
			goto ENABLE_PS_EXIT_ERR;
		}

		APS_LOG("PS_CONF1_2 value: low = %x, high = %x\n", databuf[0], databuf[1]);

		databuf[0] |= 0x01;
		res = cmp_i2c_write(client, CM36686_REG_PS_CONF1_2, databuf, 2);
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
	u8 databuf[2];

	if (enable == 1) {
		APS_LOG("enable_als\n");

		res = cmp_i2c_read(client, CM36686_REG_ALS_CONF, databuf, 2);
		if (res < 0) {
			APS_ERR("i2c_master_send function err\n");
			goto ENABLE_ALS_EXIT_ERR;
		}

		APS_LOG("ALS_CONF value: low = %x, high = %x\n", databuf[0], databuf[1]);

		databuf[0] &= 0xFE;
		res = cmp_i2c_write(client, CM36686_REG_ALS_CONF, databuf, 2);
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
		APS_LOG("disable_als\n");
		res = cmp_i2c_write(client, CM36686_REG_ALS_CONF, databuf, 2);
		if (res < 0) {
			APS_ERR("i2c_master_send function err\n");
			goto ENABLE_ALS_EXIT_ERR;
		}

		APS_LOG("ALS_CONF value:low = %x, high = %x\n", databuf[0],	databuf[1]);

		databuf[0] |= 0x01;
		res = cmp_i2c_write(client, CM36686_REG_ALS_CONF, databuf, 2);
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

	if (!test_bit(CMC_BIT_PS, &obj->enable)) {
		res = cm36686_enable_ps(obj->client, 1);
		if (res) {
			APS_ERR("enable als fail: %ld\n", res);
			return -1;
		}
		set_bit(CMC_BIT_PS, &obj->enable);
	}

	res = cmp_i2c_read(client, CM36686_REG_PS_DATA, databuf, 2);
	if (res < 0) {
		APS_ERR("i2c_master_send function err\n");
		goto READ_PS_EXIT_ERR;
	}

	if (atomic_read(&obj->trace) & CMC_TRC_DEBUG)
		APS_LOG("_PS_DATA value: low = %x, high = %x\n", databuf[0], databuf[1]);

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

	if (!test_bit(CMC_BIT_ALS, &obj->enable)) {
		res = cm36686_enable_als(obj->client, 1);
		if (res) {
			APS_ERR("enable als fail: %ld\n", res);
			return -1;
		}
		set_bit(CMC_BIT_ALS, &obj->enable);
	}

	res = cmp_i2c_read(client, CM36686_REG_ALS_DATA, databuf, 2);
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
		if (als < obj->hw->als_level[idx])
			break;
	}

	if (idx >= obj->als_level_num || idx >= obj->als_value_num) {
		if (idx < obj->als_value_num)
			value = obj->hw->als_value[idx - 1];
		else
			value = obj->hw->als_value[obj->als_value_num - 1];
	} else {
		level_high = obj->hw->als_level[idx];
		level_low = (idx > 0) ? obj->hw->als_level[idx - 1] : 0;
		level_diff = level_high - level_low;
		value_high = obj->hw->als_value[idx];
		value_low = (idx > 0) ? obj->hw->als_value[idx - 1] : 0;
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

/*----------------------------------------------------------------------------*/
#ifdef CM36686_PS_EINT_ENABLE
/*----------------------------------interrupt functions--------------------------------*/
static int cm36686_check_intr(struct i2c_client *client)
{
	int res;
	u8 databuf[2];

	databuf[0] = CM36686_REG_PS_DATA;
	res = cmp_i2c_read(client, CM36686_REG_PS_DATA, databuf, 2);
	if (res < 0) {
		APS_ERR("read ps data fail: %d\n", res);
		goto EXIT_ERR;
	}

	APS_LOG("PS_DATA value low = %x, high = %x\n", databuf[0], databuf[1]);

	res = cmp_i2c_read(client, CM36686_REG_INT_FLAG, databuf, 2);
	if (res < 0) {
		APS_ERR("read int flag fail: %d\n", res);
		goto EXIT_ERR;
	}

	APS_LOG("INT_FLAG value: low = %x, high = %x\n", databuf[0], databuf[1]);

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
	int i;

	res = cm36686_check_intr(obj->client);
	if (res) {
		goto EXIT_INTR_ERR;
	} else {
		APS_LOG("cm36686 interrupt value = %d\n", intr_flag);
		res = ps_report_interrupt_data(intr_flag);
	}
	for (i = 0; i < DEVICE_NUM_MAX; i++)
		enable_irq(cm36686_obj[i]->irq);
	return;

EXIT_INTR_ERR:
	for (i = 0; i < DEVICE_NUM_MAX; i++)
		enable_irq(cm36686_obj[i]->irq);
	APS_ERR("cm36686_eint_work err: %d\n", res);
}

/*----------------------------------------------------------------------------*/
static void cm36686_eint_func(int irq)
{
	struct cm36686_priv *obj = NULL;
	int i;

	for (i = 0; i < DEVICE_NUM_MAX; i++) {
		if (irq == cm36686_obj[i]->irq)
		obj = cm36686_obj[i];
		break;
	}

	if (!obj)
		return;
	int_top_time = sched_clock();
	schedule_work(&obj->eint_work);
}

#if defined(CONFIG_OF)
static irqreturn_t cm36686_eint_handler(int irq, void *desc)
{
	int i;

	for (i = 0; i < DEVICE_NUM_MAX; i++)
		disable_irq_nosync(cm36686_obj[i]->irq);
	cm36686_eint_func(irq);

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
	struct device_node *irq_node;
	struct cm36686_priv *obj;

	obj = cm36686_obj[idx];
	irq_node = obj->irq_node;
	alspsPltFmDev = get_alsps_platformdev();
	pinctrl = devm_pinctrl_get(&alspsPltFmDev->dev);
	if (IS_ERR(pinctrl)) {
		ret = PTR_ERR(pinctrl);
		APS_ERR("Cannot find alsps pinctrl!\n");
		return -EINVAL;
	}
	pins_default = pinctrl_lookup_state(pinctrl, "default");
	if (IS_ERR(pins_default)) {
		ret = PTR_ERR(pins_default);
		APS_ERR("Cannot find alsps pinctrl default!\n");
		return -EINVAL;
	}
	pinctrl_select_state(pinctrl, pins_default);

	if (irq_node) {
		ret = of_property_read_u32_array(irq_node, "debounce", ints,
					   ARRAY_SIZE(ints));
		if (ret == 0) {
			gpio_request(ints[0], "p-sensor");
			gpio_set_debounce(ints[0], ints[1]);
			APS_LOG("ints[0] = %d, ints[1] = %d!!\n", ints[0], ints[1]);
		}
		obj->irq = irq_of_parse_and_map(irq_node, 0);
		APS_LOG("irq = %d\n", obj->irq);
		if (!obj->irq) {
			APS_ERR("irq_of_parse_and_map fail!!\n");
			return -EINVAL;
		}
		ret = request_irq(obj->irq, cm36686_eint_handler,
				IRQF_TRIGGER_NONE, "ALS-eint", NULL);
		if (ret) {
			APS_ERR("IRQ LINE NOT AVAILABLE!!\n");
			return -EINVAL;
		}
	} else {
		APS_ERR("null irq node!!\n");
		return -EINVAL;
	}
	return 0;
}
#endif

static int cm36686_open(struct inode *inode, struct file *file)
{
	file->private_data = cm36686_i2c_client[0];

	if (!file->private_data) {
		APS_ERR("null pointer!!\n");
		return -EINVAL;
	}
	return nonseekable_open(inode, file);
}

static int cm36686_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

static int set_psensor_threshold(struct i2c_client *client)
{
	struct cm36686_priv *obj = i2c_get_clientdata(client);
	int res = 0;
	u8 databuf[2];

	APS_LOG("set_psensor_threshold function high: 0x%x, low:0x%x\n",
		atomic_read(&obj->ps_thd_val_high), atomic_read(&obj->ps_thd_val_low));
	databuf[0] = atomic_read(&obj->ps_thd_val_low);
	databuf[1] = atomic_read(&obj->ps_thd_val_low) >> 8;
	res = cmp_i2c_write(client, CM36686_REG_PS_THDL, databuf, 2);
	if (res < 0) {
		APS_ERR("i2c_master_send function err\n");
		return -1;
	}

	databuf[0] = atomic_read(&obj->ps_thd_val_high);
	databuf[1] = atomic_read(&obj->ps_thd_val_high) >> 8;
	res = cmp_i2c_write(client, CM36686_REG_PS_THDH, databuf, 2);
	if (res < 0) {
		APS_ERR("i2c_master_send function err\n");
		return -1;
	}
	return 0;

}

static long cm36686_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	/*struct i2c_client *client = (struct i2c_client *)file->private_data;*/
	struct cm36686_priv *obj = NULL;/*= i2c_get_clientdata(client);*/
	long err = 0;
	int32_t i, j;
	void __user *ptr = (void __user *)arg;
	struct ALSPS_IO_DATA io_data;

	mutex_lock(&cm36686_mutex);
	if (copy_from_user(&io_data, ptr, sizeof(io_data))) {
		err = -EFAULT;
		goto err_out;
	}

	for (i = 0; i < DEVICE_NUM_MAX; i++) {
		if (alsps_cust[i].device_id == io_data.device_id)
			break;
	}

	if ((i == DEVICE_NUM_MAX) || (io_data.device_id >= DEVICE_NUM_MAX)) {
		err = -EFAULT;
		APS_ERR("device_id(%d) ERR!\n", io_data.device_id);
		goto err_out;
	}
	obj = cm36686_obj[io_data.device_id];

	switch (cmd) {
	case ALSPS_SET_PS_MODE:
		if (io_data.data[0]) {/*enable*/
			err = cm36686_enable_ps(obj->client, 1);
			if (err)
				APS_ERR("enable ps fail: %ld\n", err);
			else
				set_bit(CMC_BIT_PS, &obj->enable);
		} else {
			err = cm36686_enable_ps(obj->client, 0);
			if (err)
				APS_ERR("disable ps fail: %ld\n", err);
			else
				clear_bit(CMC_BIT_PS, &obj->enable);
		}
		break;

	case ALSPS_GET_PS_MODE:
		io_data.data[0] = test_bit(CMC_BIT_PS, &obj->enable) ? (1) : (0);
		if (copy_to_user(ptr, &io_data, sizeof(io_data)))
			err = -EFAULT;
		break;

	case ALSPS_GET_PS_DATA:
		err = cm36686_read_ps(obj->client, &obj->ps);
		if (err == 0) {
			io_data.data[0] = cm36686_get_ps_value(obj, obj->ps);
			if (copy_to_user(ptr, &io_data, sizeof(io_data)))
				err = -EFAULT;
		}
		break;

	case ALSPS_GET_PS_RAW_DATA:
		err = cm36686_read_ps(obj->client, &obj->ps);
		if (err == 0) {
			io_data.data[0] = obj->ps;
			if (copy_to_user(ptr, &io_data, sizeof(io_data)))
				err = -EFAULT;
		}
		break;

	case AAL_SET_ALS_MODE:
	case ALSPS_SET_ALS_MODE:
		if (io_data.data[0]) {
			err = cm36686_enable_als(obj->client, 1);
			if (err)
				APS_ERR("enable als fail: %ld\n", err);
			else
				set_bit(CMC_BIT_ALS, &obj->enable);
		} else {
			err = cm36686_enable_als(obj->client, 0);
			if (err)
				APS_ERR("disable als fail: %ld\n", err);
			else
				clear_bit(CMC_BIT_ALS, &obj->enable);
		}
		break;

	case AAL_GET_ALS_MODE:
	case ALSPS_GET_ALS_MODE:
		io_data.data[0] = test_bit(CMC_BIT_ALS, &obj->enable) ? (1) : (0);
		if (copy_to_user(ptr, &io_data, sizeof(io_data)))
			err = -EFAULT;
		break;

	case AAL_GET_ALS_DATA:
	case ALSPS_GET_ALS_DATA:
		err = cm36686_read_als(obj->client, &obj->als);
		if (err == 0) {
			io_data.data[0] = cm36686_get_als_value(obj, obj->als);
			if (copy_to_user(ptr, &io_data, sizeof(io_data)))
				err = -EFAULT;
		}
		break;

	case ALSPS_GET_ALS_RAW_DATA:
		err = cm36686_read_als(obj->client, &obj->als);
		if (err == 0) {
			io_data.data[0] = obj->als;
			if (copy_to_user(ptr, &io_data, sizeof(io_data)))
				err = -EFAULT;
		}
		break;

	case ALSPS_GET_PS_TEST_RESULT:
		err = cm36686_read_ps(obj->client, &obj->ps);
		if (err == 0) {
			if (obj->ps > atomic_read(&obj->ps_thd_val_low))
				io_data.data[0] = 0;
			else
				io_data.data[0] = 1;

			if (copy_to_user(ptr, &io_data, sizeof(io_data)))
				err = -EFAULT;
		}
		break;

	case ALSPS_IOCTL_CLR_CALI:
		if (io_data.data[0] == 0)
			obj->ps_cali = 0;
		break;

	case ALSPS_IOCTL_GET_CALI:
		io_data.data[0] = obj->ps_cali;
		if (copy_to_user(ptr, &io_data, sizeof(io_data)))
			err = -EFAULT;
		break;

	case ALSPS_IOCTL_SET_CALI:
		obj->ps_cali = io_data.data[0];
		break;

	case ALSPS_SET_PS_THRESHOLD:
		APS_LOG("%s set threshold high: 0x%x, low: 0x%x\n", __func__, io_data.data[0],
			io_data.data[1]);
		atomic_set(&obj->ps_thd_val_high, ((int)io_data.data[0] + obj->ps_cali));
		atomic_set(&obj->ps_thd_val_low, ((int)io_data.data[1] + obj->ps_cali));	/* need to confirm */
		set_psensor_threshold(obj->client);
		break;

	case ALSPS_GET_PS_THRESHOLD_HIGH:
		io_data.data[0] = atomic_read(&obj->ps_thd_val_high) - obj->ps_cali;
		APS_LOG("%s get threshold high: 0x%x\n", __func__, io_data.data[0]);
		if (copy_to_user(ptr, &io_data, sizeof(io_data)))
			err = -EFAULT;
		break;

	case ALSPS_GET_PS_THRESHOLD_LOW:
		io_data.data[0] = atomic_read(&obj->ps_thd_val_low) - obj->ps_cali;
		APS_LOG("%s get threshold low: 0x%x\n", __func__, io_data.data[0]);
		if (copy_to_user(ptr, &io_data, sizeof(io_data)))
			err = -EFAULT;
		break;

	case AAL_GET_ALS_INFO:
		io_data.device_id = devProbeOkCnt;
		j = 0;
		for (i = 0; i < DEVICE_NUM_MAX; i++) {
			if (alsps_cust[i].device_id != DEFAULT_DEV_ID) {
				io_data.data[j] = alsps_cust[i].device_id;
				j++;
			}
		}
		if (copy_to_user(ptr, &io_data, sizeof(io_data)))
			err = -EFAULT;
		break;

	default:
		APS_ERR("%s not supported = 0x%04x", __func__, cmd);
		err = -ENOIOCTLCMD;
		break;
	}
err_out:
	mutex_unlock(&cm36686_mutex);
	return err;
}

#ifdef CONFIG_COMPAT
static long compat_cm36686_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	if (!filp->f_op || !filp->f_op->unlocked_ioctl) {
		APS_ERR("compat_ion_ioctl file has no f_op or no f_op->unlocked_ioctl.\n");
		return -ENOTTY;
	}
	return filp->f_op->unlocked_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));
}
#endif
/********************************************************************/
/*------------------------------misc device related operation functions------------------------------------*/
static const struct file_operations cm36686_fops = {
	.owner = THIS_MODULE,
	.open = cm36686_open,
	.release = cm36686_release,
	.unlocked_ioctl = cm36686_unlocked_ioctl,
#if IS_ENABLED(CONFIG_COMPAT)
	.compat_ioctl = compat_cm36686_unlocked_ioctl,
#endif
};

static struct miscdevice cm36686_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "als_ps",
	.fops = &cm36686_fops,
};

/*--------------------------------------------------------------------------------*/
static int cm36686_init_client(struct i2c_client *client)
{
	struct cm36686_priv *obj = i2c_get_clientdata(client);
	u8 databuf[2];
	int res = 0;

	APS_FUN();
	if (obj->hw->polling_mode_als == 1)
		databuf[0] = 0x41; /*0b01000001;*/
	else
		databuf[0] = 0x47; /*0b01000111;*/
	databuf[1] = 0x00; /*0b00000000;*/
	res = cmp_i2c_write(client, CM36686_REG_ALS_CONF, databuf, 2);
	if (res < 0) {
		APS_ERR("i2c_master_send function err\n");
		goto EXIT_ERR;
	}
	APS_LOG("cm36686 ps CM36686_REG_ALS_CONF command!\n");

	databuf[0] = 0x63; /*0b01100011;*/
	if (obj->hw->polling_mode_ps == 1)
		databuf[1] = 0x08; /*0b00001000;*/
	else
		databuf[1] = 0x0B; /*0b00001011;*/
	res = cmp_i2c_write(client, CM36686_REG_PS_CONF1_2, databuf, 2);
	if (res < 0) {
		APS_ERR("i2c_master_send function err\n");
		goto EXIT_ERR;
	}
	APS_LOG("cm36686 ps CM36686_REG_PS_CONF1_2 command!\n");

	databuf[0] = 0x14; /*0b00010100;*/
	databuf[1] = 0x02; /*0b00000010;*/
	res = cmp_i2c_write(client, CM36686_REG_PS_CONF3_MS, databuf, 2);
	if (res < 0) {
		APS_ERR("i2c_master_send function err\n");
		goto EXIT_ERR;
	}
	APS_LOG("cm36686 ps CM36686_REG_PS_CONF3_MS command!\n");

	databuf[0] = 0x00;
	databuf[1] = 0x00;
	res = cmp_i2c_write(client, CM36686_REG_PS_CANC, databuf, 2);
	if (res < 0) {
		APS_ERR("i2c_master_send function err\n");
		goto EXIT_ERR;
	}

	APS_LOG("cm36686 ps CM36686_REG_PS_CANC command!\n");

	if (obj->hw->polling_mode_als == 0) {
		databuf[0] = 0x00;
		databuf[1] = atomic_read(&obj->als_thd_val_high);
		res = cmp_i2c_write(client, CM36686_REG_ALS_THDH, databuf, 2);
			if (res < 0) {
			APS_ERR("i2c_master_send function err\n");
			goto EXIT_ERR;
		}
		databuf[0] = 0x00;
		databuf[1] = atomic_read(&obj->als_thd_val_low);	/* threshold value need to confirm */
		res = cmp_i2c_write(client, CM36686_REG_ALS_THDL, databuf, 2);
		if (res < 0) {
			APS_ERR("i2c_master_send function err\n");
			goto EXIT_ERR;
		}
	}

	if (obj->hw->polling_mode_ps == 0) {
		databuf[0] = atomic_read(&obj->ps_thd_val_low);
		databuf[1] = atomic_read(&obj->ps_thd_val_low) >> 8;
		res = cmp_i2c_write(client, CM36686_REG_PS_THDL, databuf, 2);
		if (res < 0) {
			APS_ERR("i2c_master_send function err\n");
			goto EXIT_ERR;
		}

		databuf[0] = (u8)atomic_read(&obj->ps_thd_val_high);
		databuf[1] = (u8)atomic_read(&obj->ps_thd_val_high) >> 8;
		res = cmp_i2c_write(client, CM36686_REG_PS_THDH, databuf, 2);
		if (res < 0) {
			APS_ERR("i2c_master_send function err\n");
			goto EXIT_ERR;
		}
	}
#ifdef CM36686_PS_EINT_ENABLE
	res = cm36686_setup_eint(client);
	if (res) {
		APS_ERR("setup eint: %d\n", res);
		return res;
	}
#endif
	return CM36686_SUCCESS;

EXIT_ERR:
	APS_ERR("init dev: %d\n", res);
	return res;
}

/*--------------------------------------------------------------------------------*/
#if 0
/* if use  this typ of enable , sensor should report inputEvent(val ,stats, div) to HAL */
static int als_open_report_data(int open)
{
	/* should queuq work to report event if  is_report_input_direct=true */
	return 0;
}

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

/* if use  this typ of enable , sensor should report inputEvent(val ,stats, div) to HAL */
static int ps_open_report_data(int open)
{
	/* should queuq work to report event if  is_report_input_direct=true */
	return 0;
}

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
#endif

/*-----------------------------------i2c operations----------------------------------*/
static int cm36686_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct cm36686_priv *obj;
	struct alsps_hw cust_info;
	int err = 0;
	/*struct als_control_path als_ctl = { 0 };
	 *struct als_data_path als_data = { 0 };
	 *struct ps_control_path ps_ctl = { 0 };
	 *struct ps_data_path ps_data = { 0 };
	 */
	u32 idx;

	devProbeCnt++;
	if (devProbeOkCnt >= DEVICE_NUM_MAX) {
		APS_ERR("ERR! Max support %d devices!\n", DEVICE_NUM_MAX);
		return -1;
	}

	if (get_alsps_dts_func(client->dev.of_node, &cust_info) == NULL) {
		APS_ERR("Get dts info fail!\n");
		return -1;
	}

	idx = cust_info.device_id;

	APS_LOG("cm36686_i2c_probe(%d), name:%s device_id:%d\n",
			devProbeCnt - 1, client->dev.of_node->name, idx);

	if (alsps_cust[idx].device_id != DEFAULT_DEV_ID) {
		APS_ERR("device_id (%d) already registered!\n", idx);
		return -1;
	}

	if (cust_info.device_id >= DEVICE_NUM_MAX) {
		APS_ERR("device_id limit is (%d)!\n", DEVICE_NUM_MAX);
		return -1;
	}

	obj = kzalloc(sizeof(*obj), GFP_KERNEL);
	if (!obj) {
		err = -ENOMEM;
		goto exit;
	}
	memset(obj, 0, sizeof(*obj));
	memcpy(&alsps_cust[idx], &cust_info, sizeof(cust_info));

	cm36686_obj[idx] = obj;
	obj->hw = &alsps_cust[idx];
	cm36686_power(obj->hw, 1);	/*power regulator*/

#ifdef CM36686_PS_EINT_ENABLE
	INIT_WORK(&obj->eint_work, cm36686_eint_work);
#endif
	obj->client = client;
	i2c_set_clientdata(client, obj);
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
	atomic_set(&obj->ps_thd_val_high, obj->hw->ps_threshold_high);
	atomic_set(&obj->ps_thd_val_low, obj->hw->ps_threshold_low);
	atomic_set(&obj->als_thd_val_high, obj->hw->als_threshold_high);
	atomic_set(&obj->als_thd_val_low, obj->hw->als_threshold_low);
	atomic_set(&obj->init_done, 0);
	obj->irq_node = of_find_compatible_node(NULL, NULL, "mediatek, ALS-eint");

	obj->enable = 0;
	obj->pending_intr = 0;
	obj->ps_cali = 0;
	obj->als_level_num = ARRAY_SIZE(obj->hw->als_level);
	obj->als_value_num = ARRAY_SIZE(obj->hw->als_value);
	/*-----------------------------value need to be confirmed-----------------------------------------*/

	WARN_ON(sizeof(obj->als_level) != sizeof(obj->hw->als_level));
	memcpy(obj->als_level, obj->hw->als_level, sizeof(obj->als_level));
	WARN_ON(sizeof(obj->als_value) != sizeof(obj->hw->als_value));
	memcpy(obj->als_value, obj->hw->als_value, sizeof(obj->als_value));
	atomic_set(&obj->i2c_retry, 3);
	clear_bit(CMC_BIT_ALS, &obj->enable);
	clear_bit(CMC_BIT_PS, &obj->enable);

	cm36686_i2c_client[idx] = client;
	if (msg_dma_alloc())
		goto exit;


	err = cm36686_init_client(client);
	if (err)
		goto exit_init_failed;
	APS_LOG("cm36686_init_client() OK!\n");

	if (devProbeOkCnt == 0) {
		err = misc_register(&cm36686_device);
		if (err) {
			APS_ERR("cm36686_device register failed\n");
			goto exit_misc_device_register_failed;
		}
	}

	/*------------------------cm36686 attribute file for debug--------------------------------------*/
/*
#ifdef DEVICE_ATTRIBUTE_ENABLE
	err = cm36686_create_attr(&(client->dev));
#else
	err = cm36686_create_attr(&(cm36686_init_info.platform_diver_addr->driver));
#endif
	if (err) {
		APS_ERR("create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}
*/
#if 0
	als_ctl.is_use_common_factory = false;
	ps_ctl.is_use_common_factory = false;

	als_ctl.open_report_data = als_open_report_data;
	als_ctl.enable_nodata = als_enable_nodata;
	als_ctl.set_delay = als_set_delay;
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
	ps_ctl.is_report_input_direct = false;
	ps_ctl.is_support_batch = false;
	ps_ctl.is_polling_mode = obj->hw->polling_mode_ps;

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

#endif
	devProbeOkCnt++;
	APS_LOG("%s: OK(%d, %d)\n", __func__, devProbeCnt, devProbeOkCnt);
	return 0;

/*exit_create_attr_failed:*/
#if 0
exit_sensor_obj_attach_fail:
#endif
exit_misc_device_register_failed:
	misc_deregister(&cm36686_device);
exit_init_failed:
	kfree(obj);
exit:
	cm36686_i2c_client[idx] = NULL;
	APS_ERR("%s: err = %d\n", __func__, err);
	return err;
}

static int cm36686_i2c_remove(struct i2c_client *client)
{
	int i;

	APS_FUN();
	if (devProbeOkCnt != 0) {
	/*
#ifdef DEVICE_ATTRIBUTE_ENABLE
		err = cm36686_delete_attr(&(client->dev));
#else
		err = cm36686_delete_attr(&(cm36686_init_info.platform_diver_addr->driver));
#endif
		if (err)
			APS_ERR("cm36686_delete_attr fail: %d\n", err);
	*/
		misc_deregister(&cm36686_device);
		msg_dma_release();
		for (i = 0; i < devProbeOkCnt; i++) {
			i2c_unregister_device(cm36686_i2c_client[i]);
			kfree(i2c_get_clientdata(cm36686_i2c_client[i]));
			cm36686_i2c_client[i] = NULL;
		}
		devProbeOkCnt = 0;
	}
	devProbeCnt = 0;
	return 0;
}

static int cm36686_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	APS_LOG("cm36686_i2c_detect %s\n", client->name);
	strncpy(info->type, client->name, sizeof(info->type));
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
	int i;

	for (i = 0; i < DEVICE_NUM_MAX; i++)
		cm36686_power(&alsps_cust[i], 0);

	i2c_del_driver(&cm36686_i2c_driver);
	return 0;
}

/*----------------------------------------------------------------------------*/

static int cm36686_local_init(void)
{
	int i;
	int res;

	APS_FUN();
	devProbeCnt = 0;
	devProbeOkCnt = 0;
	for (i = 0; i < DEVICE_NUM_MAX; i++)
		alsps_cust[i].device_id = DEFAULT_DEV_ID;	/*init device id*/
	res = i2c_add_driver(&cm36686_i2c_driver);
	/*APS_LOG("Total %d devices, %d probe OK!\n", devProbeCnt, devProbeOkCnt);*/
	return res;
}


/*----------------------------------------------------------------------------*/
static int __init cm36686_init(void)
{
	int res;

	APS_FUN();
#ifdef ONLY_USE_IOCTL
	res = cm36686_local_init();
	if (res != 0) {
		APS_ERR("cm36686_init Fail!\n");
		res = -1;
	}
#else
	alsps_driver_add(&cm36686_init_info);
#endif
	return res;
}

/*----------------------------------------------------------------------------*/
static void __exit cm36686_exit(void)
{
	APS_FUN();
#ifdef ONLY_USE_IOCTL
	cm36686_remove();
#endif
}

/*----------------------------------------------------------------------------*/
module_init(cm36686_init);
module_exit(cm36686_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("andrew yang");
MODULE_DESCRIPTION("cm36686 driver");
MODULE_LICENSE("GPL");
