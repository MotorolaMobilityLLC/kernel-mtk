/*
* Copyright (C) 2016 MediaTek Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See http://www.gnu.org/licenses/gpl-2.0.html for more details.
*/

#include <linux/init.h>		/* For init/exit macros */
#include <linux/module.h>	/* For MODULE_ marcros*/
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/power_supply.h>
#include <linux/time.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/proc_fs.h>
#include <linux/platform_device.h>
#include <linux/seq_file.h>
#include <linux/scatterlist.h>
#include <linux/suspend.h>
#include <linux/version.h>
#include <linux/i2c.h>
#include <asm/atomic.h>
#include <mt-plat/mtk_boot.h>

#include <ontim/ontim_dev_dgb.h>
static  char charge_ic_vendor_name[50]="HL7019";
DEV_ATTR_DECLARE(charge_ic)
DEV_ATTR_DEFINE("vendor",charge_ic_vendor_name)
DEV_ATTR_DECLARE_END;
ONTIM_DEBUG_DECLARE_AND_INIT(charge_ic,charge_ic,8);


//Antaiui <AI_BSP_CHG> <hehl> <2021-03-27> add charger ic HL7015A begin
//#define CONFIG_PROJECT_PHY //hhl add
//Antaiui <AI_BSP_CHG> <hehl> <2021-03-27> add charger ic HL7015A end

#define SET_OTG_CURRENT_TO_1A	1

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#endif
#include <mt-plat/upmu_common.h>
#include "hl7019d.h"
//#include "mtk_charger_intf.h"
#include "charger_class.h"

#include <mt-plat/v1/charger_type.h>
#include "mtk_charger.h"
#include <linux/alarmtimer.h>


static int is_otg_mode = 0;

const unsigned int VBAT_CVTH[] = {
	3504000, 3520000, 3536000, 3552000,
	3568000, 3584000, 3600000, 3616000,
	3632000, 3648000, 3664000, 3680000,
	3696000, 3712000, 3728000, 3744000,
	3760000, 3776000, 3792000, 3808000,
	3824000, 3840000, 3856000, 3872000,
	3888000, 3904000, 3920000, 3936000,
	3952000, 3968000, 3984000, 4000000,
	4016000, 4032000, 4048000, 4064000,
	4080000, 4096000, 4112000, 4128000,
	4144000, 4160000, 4176000, 4192000,
	4208000, 4224000, 4240000, 4256000,
	4272000, 4288000, 4304000, 4320000,
	4336000, 4352000, 4368000, 4386000,
	4400000, 4416000, 4432000, 4448000,
	4464000, 4480000, 4496000, 4512000
};

const unsigned int VINDPM_NORMAL_CVTH[] = {
	3880000, 3970000, 4030000, 4120000,
	4200000, 4290000, 4350000, 4440000,
	4520000, 4610000, 4670000, 4760000,
	4840000, 4930000, 4990000, 5080000
};

const unsigned int VINDPM_HIGH_CVTH[] = {
	8320000, 8500000, 8640000, 8820000,
	9010000, 9190000, 9330000, 9510000,
	9690000, 9870000, 10010000, 10190000,
	10380000, 10560000, 10700000, 10880000
};

const unsigned int SYS_MIN_CVTH[] = {
	3000000, 3100000, 3200000, 3300000,
	3400000, 3500000, 3600000, 3700000
};

const unsigned int BOOSTV_CVTH[] = {
	4550000, 4614000, 4678000, 4742000,
	4806000, 4870000, 4934000, 4998000,
	5062000, 5126000, 5190000, 5254000,
	5318000, 5382000, 5446000, 5510000
};

const unsigned int CSTH[] = {
	512000, 576000, 640000, 704000,
	768000, 832000, 896000, 960000,
	1024000, 1088000, 1152000, 1216000,
	1280000, 1344000, 1408000, 1472000,
	1536000, 1600000, 1664000, 1728000,
	1792000, 1856000, 1920000, 1984000,
	2048000, 2112000, 2176000, 2240000,
	2304000, 2368000, 2432000, 2496000, 
	2560000, 2624000, 2688000, 2752000,
	2816000, 2880000, 2944000, 3008000,
	3072000, 3200000, 3264000, 3328000, 
	3392000, 3456000, 3520000, 3584000,
	3648000, 3712000, 3776000, 3840000,
	3904000, 3968000, 4032000, 4096000, 
	4160000, 4224000, 4288000, 4352000,
	4416000, 4480000, 4544000
};

/*hl7019d REG00 IINLIM[5:0]*/
const unsigned int INPUT_CSTH[] = {
	100000, 150000, 500000, 900000,
	1000000, 1500000, 2000000, 3000000
};

const unsigned int IPRE_CSTH[] = {
	128000, 256000, 384000, 512000,
	640000, 768000, 896000, 1024000,
	1152000, 1280000, 1408000, 1536000,
	1664000, 1792000, 1920000, 2048000
};

const unsigned int ITERM_CSTH[] = {
	128000, 256000, 384000, 512000,
	640000, 768000, 896000, 1024000,
	1152000, 1280000, 1408000, 1536000,
	1664000, 1792000, 1920000, 2048000
};


struct hl7019d_info {
	struct charger_device *chg_dev;
	struct power_supply *psy;
	struct charger_properties chg_props;
	struct device *dev;
	struct alarm otg_kthread_gtimer;
	struct workqueue_struct *otg_boost_workq;
	struct work_struct kick_work;
	unsigned int polling_interval;
	bool polling_enabled;

	const char *chg_dev_name;
	const char *eint_name;
	enum charger_type chg_type;
	u8 power_good;
	int irq;
};

static struct hl7019d_info *g_info;
static struct i2c_client *new_client;


//#define hl7019d_SLAVE_ADDR_1 0xD8
//#define hl7019d_SLAVE_ADDR_2 0xD6
//#define HL7019D_CHECK_CHARGING_STATE
#ifdef HL7019D_CHECK_CHARGING_STATE
struct workqueue_struct *charging_check_workqueue;
struct delayed_work charging_check_work;
#define CHARGING_CHECK_WAIT_TIME              500    /* ms */
static int check_time = 0;
//static int dev_detect = 0;
static atomic_t hl7019d_irq_enable_charging = ATOMIC_INIT(0);
//static int is_chager_ic_on = 0;
static int hl7019d_get_charger_type(struct hl7019d_info *info);
static int hl7019d_set_charger_type(struct hl7019d_info *info);
struct hl7019d_info *charging_check_info = NULL;

#define CHARGING_CHECK_TIME	10
#endif




static void enable_boost_polling(bool poll_en);
static void usbotg_boost_kick_work(struct work_struct *work);
static enum alarmtimer_restart usbotg_gtimer_func(struct alarm *alarm,
						 ktime_t now);

unsigned int charging_value_to_parameter_hl7019d(const unsigned int *parameter, const unsigned int array_size,
					const unsigned int val)
{
	if (val < array_size)
		return parameter[val];
	pr_err("[hl7019d] Can't find the parameter\n");
	return parameter[0];
}

unsigned int charging_parameter_to_value_hl7019d(const unsigned int *parameter, const unsigned int array_size,
					const unsigned int val)
{
	unsigned int i;

	pr_err("[hl7019d] array_size = %d\n", array_size);

	for (i = 0; i < array_size; i++) {
		if (val == *(parameter + i))
			return i;
	}

	pr_err("[hl7019d] NO register value match\n");
	/* TODO: ASSERT(0);	// not find the value */
	return 0;
}

static unsigned int bmt_find_closest_level(const unsigned int *pList, unsigned int number,
					 unsigned int level)
{
	unsigned int i;
	unsigned int max_value_in_last_element;

	if (pList[0] < pList[1])
		max_value_in_last_element = 1;
	else
		max_value_in_last_element = 0;

	if (max_value_in_last_element == 1) {
		for (i = (number - 1); i != 0; i--) {	/* max value in the last element */
			if (pList[i] <= level) {
				pr_err("[hl7019d] zzf_%d<=%d, i=%d\n", pList[i], level, i);
				return pList[i];
			}
		}
		pr_err("[hl7019d] Can't find closest level\n");
		return pList[0];
		/* return 000; */
	} else {
		for (i = 0; i < number; i++) {	/* max value in the first element */
			if (pList[i] <= level)
				return pList[i];
		}
		pr_err("[hl7019d] Can't find closest level\n");
		return pList[number - 1];
		/* return 000; */
	}
}

unsigned char hl7019d_reg[HL7019D_REG_NUM] = { 0 };
static DEFINE_MUTEX(hl7019d_i2c_access);
static DEFINE_MUTEX(hl7019d_access_lock);

static int hl7019d_read_byte(u8 reg_addr, u8 *rd_buf, int rd_len)
{
	int ret = 0;
	struct i2c_adapter *adap = new_client->adapter;
	struct i2c_msg msg[2];
	u8 *w_buf = NULL;
	u8 *r_buf = NULL;

	memset(msg, 0, 2 * sizeof(struct i2c_msg));

	w_buf = kzalloc(1, GFP_KERNEL);
	if (w_buf == NULL)
		return -1;
	r_buf = kzalloc(rd_len, GFP_KERNEL);
	if (r_buf == NULL)
		return -1;

	*w_buf = reg_addr;

	msg[0].addr = new_client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = w_buf;

	msg[1].addr = new_client->addr;
	msg[1].flags = 1;
	msg[1].len = rd_len;
	msg[1].buf = r_buf;

	ret = i2c_transfer(adap, msg, 2);

	memcpy(rd_buf, r_buf, rd_len);

	kfree(w_buf);
	kfree(r_buf);
	return ret;
}

int hl7019d_write_byte(unsigned char reg_num, u8 *wr_buf, int wr_len)
{
	int ret = 0;
	struct i2c_adapter *adap = new_client->adapter;
	struct i2c_msg msg;
	u8 *w_buf = NULL;

	memset(&msg, 0, sizeof(struct i2c_msg));

	w_buf = kzalloc(wr_len, GFP_KERNEL);
	if (w_buf == NULL)
		return -1;

	w_buf[0] = reg_num;
	memcpy(w_buf + 1, wr_buf, wr_len);

	msg.addr = new_client->addr;
	msg.flags = 0;
	msg.len = wr_len;
	msg.buf = w_buf;

	ret = i2c_transfer(adap, &msg, 1);

	kfree(w_buf);
	return ret;
}


unsigned int hl7019d_read_interface(unsigned char reg_num, unsigned char *val, unsigned char MASK,
				unsigned char SHIFT)
{
	unsigned char hl7019d_reg = 0;
	unsigned int ret = 0;

	ret = hl7019d_read_byte(reg_num, &hl7019d_reg, 1);
	//pr_err("[hl7019d_read_interface] Reg[%x]=0x%x\n", reg_num, hl7019d_reg);
	hl7019d_reg &= (MASK << SHIFT);
	*val = (hl7019d_reg >> SHIFT);
	//pr_err("[hl7019d_read_interface] val=0x%x\n", *val);

	return ret;
}

unsigned int hl7019d_config_interface(unsigned char reg_num, unsigned char val, unsigned char MASK,
					unsigned char SHIFT)
{
	unsigned char hl7019d_reg = 0;
	unsigned char hl7019d_reg_ori = 0;
	unsigned int ret = 0;

	mutex_lock(&hl7019d_access_lock);
	ret = hl7019d_read_byte(reg_num, &hl7019d_reg, 1);
	hl7019d_reg_ori = hl7019d_reg;
	hl7019d_reg &= ~(MASK << SHIFT);
	hl7019d_reg |= (val << SHIFT);
	//if (reg_num == HL7019D_CON4)
		//hl7019d_reg &= ~(1 << CON4_RESET_SHIFT);

	ret = hl7019d_write_byte(reg_num, &hl7019d_reg, 2);
	mutex_unlock(&hl7019d_access_lock);
	//pr_err("[hl7019d_config_interface] write Reg[%x]=0x%x from 0x%x\n", reg_num,
	//		hl7019d_reg, hl7019d_reg_ori);
	/* Check */
	/* hl7019d_read_byte(reg_num, &hl7019d_reg, 1); */
	/* pr_err("[hl7019d_config_interface] Check Reg[%x]=0x%x\n", reg_num, hl7019d_reg); */

	return ret;
}

/* write one register directly */
unsigned int hl7019d_reg_config_interface(unsigned char reg_num, unsigned char val)
{
	unsigned char hl7019d_reg = val;

	return hl7019d_write_byte(reg_num, &hl7019d_reg, 2);
}

/* CON0 */
void hl7019d_set_en_hiz(unsigned int val)
{
	if(val > 1) {
		pr_err("[%s] parameter error.\n", __func__);
		return;
	}

	hl7019d_config_interface((unsigned char)(HL7019D_CON0),
				(unsigned char)(val),
				(unsigned char)(CON0_EN_HIZ_MASK),
				(unsigned char)(CON0_EN_HIZ_SHIFT)
				);
}

void hl7019d_set_vindpm(unsigned int val)
{
	if(val > 15) {
		pr_err("[%s] parameter error.\n", __func__);
		return;
	}

	hl7019d_config_interface((unsigned char)(HL7019D_CON0),
				(unsigned char)(val),
				(unsigned char)(CON0_VINDPM_MASK),
				(unsigned char)(CON0_VINDPM_SHIFT)
				);
}

void hl7019d_set_iinlim(unsigned int val)
{
	if(val > 7) {
		pr_err("[%s] parameter error.\n", __func__);
		return;
	}

	hl7019d_config_interface((unsigned char)(HL7019D_CON0),
				(unsigned char)(val),
				(unsigned char)(CON0_IINLIM_MASK),
				(unsigned char)(CON0_IINLIM_SHIFT)
				);
}

/* CON1 */
void hl7019d_set_register_reset(unsigned int val)
{
	if(val > 1) {
		pr_err("[%s] parameter error.\n", __func__);
		return;
	}

	hl7019d_config_interface((unsigned char)(HL7019D_CON1),
				(unsigned char)(val),
				(unsigned char)(CON1_REG_RESET_MASK),
				(unsigned char)(CON1_REG_RESET_SHIFT)
				);
}

void hl7019d_set_i2cwatchdog_timer_reset(unsigned int val)
{
	if(val > 1) {
		pr_err("[%s] parameter error.\n", __func__);
		return;
	}

	hl7019d_config_interface((unsigned char)(HL7019D_CON1),
				(unsigned char)(val),
				(unsigned char)(CON1_I2C_WDT_RESET_MASK),
				(unsigned char)(CON1_I2C_WDT_RESET_SHIFT)
				);
}

void hl7019d_set_chg_config(unsigned int val)
{
	if(val > 3) {
		pr_err("[%s] parameter error.\n", __func__);
		return;
	}

	hl7019d_config_interface((unsigned char)(HL7019D_CON1),
				(unsigned char)(val),
				(unsigned char)(CON1_CHG_CONFIG_MASK),
				(unsigned char)(CON1_CHG_CONFIG_SHIFT)
				);
}

void hl7019d_set_sys_min(unsigned int val)
{
	if(val > 7) {
		pr_err("[%s] parameter error.\n", __func__);
		return;
	}

	hl7019d_config_interface((unsigned char)(HL7019D_CON1),
				(unsigned char)(val),
				(unsigned char)(CON1_SYS_MIN_MASK),
				(unsigned char)(CON1_SYS_MIN_SHIFT)
				);
}

void hl7019d_set_boost_lim(unsigned int val)
{
	if(val > 7) {
		pr_err("[%s] parameter error.\n", __func__);
		return;
	}

	hl7019d_config_interface((unsigned char)(HL7019D_CON1),
				(unsigned char)(val),
				(unsigned char)(CON1_BOOST_LIM_MASK),
				(unsigned char)(CON1_BOOST_LIM_SHIFT)
				);
}

/* CON2 */
void hl7019d_set_ichg(unsigned int val)
{
	if(val > 0x27) { //hhl modify
		pr_err("[%s] parameter error.\n", __func__);
		return;
	}

	hl7019d_config_interface((unsigned char)(HL7019D_CON2),
				(unsigned char)(val),
				(unsigned char)(CON2_ICHG_MASK),
				(unsigned char)(CON2_ICHG_SHIFT)
				);
}

void hl7019d_set_bcold(unsigned int val)
{
	if(val > 1) {
		pr_err("[%s] parameter error.\n", __func__);
		return;
	}

	hl7019d_config_interface((unsigned char)(HL7019D_CON2),
				(unsigned char)(val),
				(unsigned char)(CON2_BCLOD_MASK),
				(unsigned char)(CON2_BCLOD_SHIFT)
				);
}

void hl7019d_set_force_20pct(unsigned int val)
{
	if(val > 1) {
		pr_err("[%s] parameter error.\n", __func__);
		return;
	}

	hl7019d_config_interface((unsigned char)(HL7019D_CON2),
				(unsigned char)(val),
				(unsigned char)(CON2_FORCE_20PCT_MASK),
				(unsigned char)(CON2_FORCE_20PCT_SHIFT)
				);
}

/* CON3 */
void hl7019d_set_iprechg(unsigned int val)
{
	if(val > 15) {
		pr_err("[%s] parameter error.\n", __func__);
		return;
	}

	hl7019d_config_interface((unsigned char)(HL7019D_CON3),
				(unsigned char)(val),
				(unsigned char)(CON3_IPRECHG_MASK),
				(unsigned char)(CON3_IPRECHG_SHIFT)
				);
}

void hl7019d_set_iterm(unsigned int val)
{
	if(val > 15) {
		pr_err("[%s] parameter error.\n", __func__);
		return;
	}

	hl7019d_config_interface((unsigned char)(HL7019D_CON3),
				(unsigned char)(val),
				(unsigned char)(CON3_ITERM_MASK),
				(unsigned char)(CON3_ITERM_SHIFT)
				);
}

/* CON4 */
void hl7019d_set_vreg(unsigned int val)
{
	if(val > 0x3f) {
		pr_err("[%s] parameter error.\n", __func__);
		return;
	}

	hl7019d_config_interface((unsigned char)(HL7019D_CON4),
				(unsigned char)(val),
				(unsigned char)(CON4_VREG_MASK),
				(unsigned char)(CON4_VREG_SHIFT)
				);
}

void hl7019d_set_batlowv(unsigned int val)
{
	if(val > 1) {
		pr_err("[%s] parameter error.\n", __func__);
		return;
	}
	
	hl7019d_config_interface((unsigned char)(HL7019D_CON4),
				(unsigned char)(val),
				(unsigned char)(CON4_BATLOWV_MASK),
				(unsigned char)(CON4_BATLOWV_SHIFT)
				);
}

void hl7019d_set_vrechg(unsigned int val)
{
	if(val > 1) {
		pr_err("[%s] parameter error.\n", __func__);
		return;
	}

	hl7019d_config_interface((unsigned char)(HL7019D_CON4),
				(unsigned char)(val),
				(unsigned char)(CON4_VRECHG_MASK),
				(unsigned char)(CON4_VRECHG_SHIFT)
				);
}

/* CON5 */
void hl7019d_set_en_term(unsigned int val)
{
	if(val > 1) {
		pr_err("[%s] parameter error.\n", __func__);
		return;
	}

	hl7019d_config_interface((unsigned char)(HL7019D_CON5),
				(unsigned char)(val),
				(unsigned char)(CON5_EN_TERM_MASK),
				(unsigned char)(CON5_EN_TERM_SHIFT)
				);
}

void hl7019d_set_term_stat(unsigned int val)
{
	if(val > 1) {
		pr_err("[%s] parameter error.\n", __func__);
		return;
	}

	hl7019d_config_interface((unsigned char)(HL7019D_CON5),
				(unsigned char)(val),
				(unsigned char)(CON5_TERM_STAT_MASK),
				(unsigned char)(CON5_TERM_STAT_SHIFT)
				);
}

void hl7019d_set_i2cwatchdog(unsigned int val)
{
	if(val > 3) {
		pr_err("[%s] parameter error.\n", __func__);
		return;
	}

	hl7019d_config_interface((unsigned char)(HL7019D_CON5),
				(unsigned char)(val),
				(unsigned char)(CON5_WATCHDOG_MASK),
				(unsigned char)(CON5_WATCHDOG_SHIFT)
				);
}

void hl7019d_set_en_safty_timer(unsigned int val)
{
	if(val > 1) {
		pr_err("[%s] parameter error.\n", __func__);
		return;
	}

	hl7019d_config_interface((unsigned char)(HL7019D_CON5),
				(unsigned char)(val),
				(unsigned char)(CON5_EN_SAFE_TIMER_MASK),
				(unsigned char)(CON5_EN_SAFE_TIMER_SHIFT)
				);
}

void hl7019d_set_charge_timer(unsigned int val)
{
	if(val > 3) {
		pr_err("[%s] parameter error.\n", __func__);
		return;
	}

	hl7019d_config_interface((unsigned char)(HL7019D_CON5),
				(unsigned char)(val),
				(unsigned char)(CON5_CHG_TIMER_MASK),
				(unsigned char)(CON5_CHG_TIMER_SHIFT)
				);
}

/* CON6 */
void hl7019d_set_boostv(unsigned int val)
{
	if(val > 15) {
		pr_err("[%s] parameter error.\n", __func__);
		return;
	}

	hl7019d_config_interface((unsigned char)(HL7019D_CON6),
				(unsigned char)(val),
				(unsigned char)(CON6_BOOSTV_MASK),
				(unsigned char)(CON6_BOOSTV_SHIFT)
				);
}

void hl7019d_set_bhot(unsigned int val)
{
	if(val > 3) {
		pr_err("[%s] parameter error.\n", __func__);
		return;
	}

	hl7019d_config_interface((unsigned char)(HL7019D_CON6),
				(unsigned char)(val),
				(unsigned char)(CON6_BHOT_MASK),
				(unsigned char)(CON6_BHOT_SHIFT)
				);
}

void hl7019d_set_treg(unsigned int val)
{
	if(val > 3) {
		pr_err("[%s] parameter error.\n", __func__);
		return;
	}

	hl7019d_config_interface((unsigned char)(HL7019D_CON6),
				(unsigned char)(val),
				(unsigned char)(CON6_TREG_MASK),
				(unsigned char)(CON6_TREG_SHIFT)
				);
}

/* CON7 */
void hl7019d_set_dpdm_en(unsigned int val)
{
	if(val > 1) {
		pr_err("[%s] parameter error.\n", __func__);
		return;
	}

	hl7019d_config_interface((unsigned char)(HL7019D_CON7),
				(unsigned char)(val),
				(unsigned char)(CON7_DPDM_EN_MASK),
				(unsigned char)(CON7_DPDM_EN_SHIFT)
				);
}

void hl7019d_set_tmr2x_en(unsigned int val)
{
	if(val > 1) {
		pr_err("[%s] parameter error.\n", __func__);
		return;
	}

	hl7019d_config_interface((unsigned char)(HL7019D_CON7),
				(unsigned char)(val),
				(unsigned char)(CON7_TMR2X_EN_MASK),
				(unsigned char)(CON7_TMR2X_EN_SHIFT)
				);
}

void hl7019d_set_ppfet_disable(unsigned int val)
{
	if(val > 1) {
		pr_err("[%s] parameter error.\n", __func__);
		return;
	}

	hl7019d_config_interface((unsigned char)(HL7019D_CON7),
				(unsigned char)(val),
				(unsigned char)(CON7_PPFET_DISABLE_MASK),
				(unsigned char)(CON7_PPFET_DISABLE_MASK)
				);
}

void hl7019d_set_chrgfault_int_mask(unsigned int val)
{
	if(val > 1) {
		pr_err("[%s] parameter error.\n", __func__);
		return;
	}

	hl7019d_config_interface((unsigned char)(HL7019D_CON7),
				(unsigned char)(val),
				(unsigned char)(CON7_CHRG_FAULT_INT_MASK_MASK),
				(unsigned char)(CON7_CHRG_FAULT_INT_MASK_SHIFT)
				);
}

void hl7019d_set_batfault_int_mask(unsigned int val)
{
	if(val > 1) {
		pr_err("[%s] parameter error.\n", __func__);
		return;
	}

	hl7019d_config_interface((unsigned char)(HL7019D_CON7),
				(unsigned char)(val),
				(unsigned char)(CON7_BAT_FAULT_INT_MASK_MASK),
				(unsigned char)(CON7_BAT_FAULT_INT_MASK_SHIFT)
				);
}

/* CON8 */
static unsigned int hl7019d_get_vin_status(void)
{
	unsigned int ret;
	unsigned char val;

	ret = hl7019d_read_interface((unsigned char)(HL7019D_CON8),
			&val,
			(unsigned char)(CON8_VIN_STAT_MASK),
			(unsigned char)(CON8_VIN_STAT_SHIFT)
			);

	return val;
}

static void hl7019d_vin_status_dump(void)
{
	unsigned int vin_status;

	vin_status = hl7019d_get_vin_status();

	switch(vin_status)
	{
		case 0:
			pr_err("[hl7019d] no input or dpm detection incomplete.\n");
			break;
		case 1:
			pr_err("[hl7019d] USB host inserted.\n");
			break;
		case 2:
			pr_err("[hl7019d] Adapter inserted.\n");
			break;
		case 3:
			pr_err("[hl7019d] OTG device inserted.\n");
			break;
		default:
			pr_err("[hl7019d] wrong vin status.\n");
			break;
	}
}

static unsigned int hl7019d_get_chrg_status(void)
{
	unsigned int ret;
	unsigned char val;

	ret = hl7019d_read_interface((unsigned char)(HL7019D_CON8),
			&val,
			(unsigned char)(CON8_CHRG_STAT_MASK),
			(unsigned char)(CON8_CHRG_STAT_SHIFT)
			);

	return val;
}

static void hl7019d_chrg_status_dump(void)
{
	unsigned int chrg_status;

	chrg_status = hl7019d_get_chrg_status();

	switch(chrg_status)
	{
		case 0:
			pr_err("[hl7019d] not charging.\n");
			break;
		case 1:
			pr_err("[hl7019d] precharging mode.\n");
			break;
		case 2:
			pr_err("[hl7019d] fast charging mode.\n");
			break;
		case 3:
			pr_err("[hl7019d] charge termination done.\n");
			break;
		default:
			pr_err("[hl7019d] wrong charge status.\n");
			break;
	}
}

static unsigned int hl7019d_get_dpm_status(void)
{
	unsigned int ret;
	unsigned char val;

	ret = hl7019d_read_interface((unsigned char)(HL7019D_CON8),
			&val,
			(unsigned char)(CON8_DPM_STAT_MASK),
			(unsigned char)(CON8_DPM_STAT_SHIFT)
			);

	return val;
}

static void hl7019d_dpm_status_dump(void)
{
	unsigned int dpm_status;

	dpm_status = hl7019d_get_dpm_status();

	if(0x0 == dpm_status)
		pr_err("[hl7019d] not in dpm.\n");
	else
		pr_err("[hl7019d] in vindpm or ilimdpm.\n");
}

static unsigned int hl7019d_get_pg_status(void)
{
	unsigned int ret;
	unsigned char val;

	ret = hl7019d_read_interface((unsigned char)(HL7019D_CON8),
			&val,
			(unsigned char)(CON8_PG_STAT_MASK),
			(unsigned char)(CON8_PG_STAT_SHIFT)
			);

	return val;
}

static void hl7019d_pg_status_dump(void)
{
	unsigned int pg_status;

	pg_status = hl7019d_get_pg_status();

	if(0x0 == pg_status)
		pr_err("[hl7019d] power is not good.\n");
	else
		pr_err("[hl7019d] power is good.\n");
}

static unsigned int hl7019d_get_therm_status(void)
{
	unsigned int ret;
	unsigned char val;

	ret = hl7019d_read_interface((unsigned char)(HL7019D_CON8),
			&val,
			(unsigned char)(CON8_THERM_STAT_MASK),
			(unsigned char)(CON8_THERM_STAT_SHIFT)
			);

	return val;
}

static void hl7019d_therm_status_dump(void)
{
	unsigned int therm_status;

	therm_status = hl7019d_get_therm_status();

	if(0x0 == therm_status)
		pr_err("[hl7019d] ic's thermal status is in normal.\n");
	else
		pr_err("[hl7019d] ic is in thermal regulation.\n");
}

static unsigned int hl7019d_get_vsys_status(void)
{
	unsigned int ret;
	unsigned char val;

	ret = hl7019d_read_interface((unsigned char)(HL7019D_CON8),
			&val,
			(unsigned char)(CON8_VSYS_STAT_MASK),
			(unsigned char)(CON8_VSYS_STAT_SHIFT)
			);

	return val;
}

static void hl7019d_vsys_status_dump(void)
{
	unsigned int vsys_status;

	vsys_status = hl7019d_get_vsys_status();

	if(0x0 == vsys_status)
		pr_err("[hl7019d] ic is not in vsysmin regulation(BAT > VSYSMIN).\n");
	else
		pr_err("[hl7019d] ic is in vsysmin regulation(BAT < VSYSMIN).\n");
}

static void hl7019d_charger_system_status(void)
{
	hl7019d_vin_status_dump();
	hl7019d_chrg_status_dump();
	hl7019d_dpm_status_dump();
	hl7019d_pg_status_dump();
	hl7019d_therm_status_dump();
	hl7019d_vsys_status_dump();
}

/* CON9 */
static unsigned int hl7019d_get_watchdog_fault(void)
{
	unsigned int ret;
	unsigned char val;

	ret = hl7019d_read_interface((unsigned char)(HL7019D_CON9),
			&val,
			(unsigned char)(CON9_WATCHDOG_FAULT_MASK),
			(unsigned char)(CON9_WATCHDOG_FAULT_SHIFT)
			);

	return val;
}

static void hl7019d_watchdog_fault_dump(void)
{
	unsigned int wtd_fault;

	wtd_fault = hl7019d_get_watchdog_fault();

	if(0x0 == wtd_fault)
		pr_err("[hl7019d] i2c watchdog is normal.\n");
	else
		pr_err("[hl7019d] i2c watchdog timer is expirate.\n");
}

static unsigned int hl7019d_get_otg_fault(void)
{
	unsigned int ret;
	unsigned char val;

	ret = hl7019d_read_interface((unsigned char)(HL7019D_CON9),
			&val,
			(unsigned char)(CON9_OTG_FAULT_MASK),
			(unsigned char)(CON9_OTG_FAULT_SHIFT)
			);

	return val;
}

static void hl7019d_otg_fault_dump(void)
{
	unsigned int otg_fault;

	otg_fault = hl7019d_get_otg_fault();

	if(0x0 == otg_fault)
		pr_err("[hl7019d] the OTG function is fine.\n");
	else
		pr_err("[hl7019d] the OTG function is error.\n");
}

static unsigned int hl7019d_get_chrg_fault(void)
{
	unsigned int ret;
	unsigned char val;

	ret = hl7019d_read_interface((unsigned char)(HL7019D_CON9),
			&val,
			(unsigned char)(CON9_CHRG_FAULT_MASK),
			(unsigned char)(CON9_CHRG_FAULT_SHIFT)
			);

	return val;
}

static void hl7019d_chrg_fault_dump(void)
{
	unsigned int chrg_fault;

	chrg_fault = hl7019d_get_chrg_fault();
	switch(chrg_fault)
	{
		case 0:
			pr_err("[hl7019d] the ic charging status is normal.\n");
			break;
		case 1:
			pr_err("[hl7019d] the ic's input is fault.\n");
			break;
		case 2:
			pr_err("[hl7019d] the ic's thermal is shutdown.\n");
			break;
		case 3:
			pr_err("[hl7019d] the ic's charge safety timer is expirate.\n");
			break;
		default:
			pr_err("[hl7019d] the ic's charge fault status is unkown.\n");
			break;
	}
}

static unsigned int hl7019d_get_bat_fault(void)
{
	unsigned int ret;
	unsigned char val;

	ret = hl7019d_read_interface((unsigned char)(HL7019D_CON9),
			&val,
			(unsigned char)(CON9_BAT_FAULT_MASK),
			(unsigned char)(CON9_BAT_FAULT_SHIFT)
			);

	return val;
}

static void hl7019d_bat_fault_dump(void)
{
	unsigned int bat_fault;

	bat_fault = hl7019d_get_bat_fault();

	if(0x0 == bat_fault)
		pr_err("[hl7019d] battery is normal.\n");
	else
		pr_err("[hl7019d] battery is in OVP.\n");
}

static unsigned int hl7019d_get_ntc_fault(void)
{
	unsigned int ret;
	unsigned char val;

	ret = hl7019d_read_interface((unsigned char)(HL7019D_CON9),
			&val,
			(unsigned char)(CON9_NTC_FAULT_MASK),
			(unsigned char)(CON9_NTC_FAULT_SHIFT)
			);

	return val;
}

static void hl7019d_ntc_fault_dump(void)
{
	unsigned int ntc_fault;

	ntc_fault = hl7019d_get_ntc_fault();
	switch(ntc_fault)
	{
		case 0:
			pr_err("[hl7019d] the ic's body temperature is normal.\n");
			break;
		case 5:
			pr_err("[hl7019d] the ic's body temperature is cold.\n");
			break;
		case 6:
			pr_err("[hl7019d] the ic's body temperature is hot.\n");
			break;
		default:
			pr_err("[hl7019d] the ic's body temperature is unknow.\n");
			break;
	}
}

static void hl7019d_charger_fault_status(void)
{
	hl7019d_watchdog_fault_dump();
	hl7019d_otg_fault_dump();
	hl7019d_chrg_fault_dump();
	hl7019d_bat_fault_dump();
	hl7019d_ntc_fault_dump();
}

/* CONA */
//vender info register

/* CONB */
void hl7019d_set_tsr(unsigned int val)
{
	if(val > 3) {
		pr_err("[%s] parameter error.\n", __func__);
		return;
	}

	hl7019d_config_interface((unsigned char)(HL7019D_CONB),
				(unsigned char)(val),
				(unsigned char)(CONB_TSR_MASK),
				(unsigned char)(CONB_TSR_SHIFT)
				);
}

void hl7019d_set_tsrp(unsigned int val)
{
	if(val > 1) {
		pr_err("[%s] parameter error.\n", __func__);
		return;
	}

	hl7019d_config_interface((unsigned char)(HL7019D_CONB),
				(unsigned char)(val),
				(unsigned char)(CONB_TRSP_MASK),
				(unsigned char)(CONB_TRSP_SHIFT)
				);
}

void hl7019d_set_dis_connect(unsigned int val)
{
	if(val > 1) {
		pr_err("[%s] parameter error.\n", __func__);
		return;
	}

	hl7019d_config_interface((unsigned char)(HL7019D_CONB),
				(unsigned char)(val),
				(unsigned char)(CONB_DIS_RECONNECT_MASK),
				(unsigned char)(CONB_DIS_RECONNECT_SHIFT)
				);
}

void hl7019d_set_dis_sr_inchg(unsigned int val)
{
	if(val > 1) {
		pr_err("[%s] parameter error.\n", __func__);
		return;
	}

	hl7019d_config_interface((unsigned char)(HL7019D_CONB),
				(unsigned char)(val),
				(unsigned char)(CONB_DIS_SR_INCHG_MASK),
				(unsigned char)(CONB_DIS_SR_INCHG_SHIFT)
				);
}

void hl7019d_set_tship(unsigned int val)
{
	if(val > 7) {
		pr_err("[%s] parameter error.\n", __func__);
		return;
	}

	hl7019d_config_interface((unsigned char)(HL7019D_CONB),
				(unsigned char)(val),
				(unsigned char)(CONB_TSHIP_MASK),
				(unsigned char)(CONB_TSHIP_SHIFT)
				);
}

/* CONC */
void hl7019d_set_bat_comp(unsigned int val)
{
	if(val > 7) {
		pr_err("[%s] parameter error.\n", __func__);
		return;
	}

	hl7019d_config_interface((unsigned char)(HL7019D_CONC),
				(unsigned char)(val),
				(unsigned char)(CONC_BAT_COMP_MASK),
				(unsigned char)(CONC_BAT_COMP_SHIFT)
				);
}

void hl7019d_set_bat_vclamp(unsigned int val)
{
	if(val > 7) {
		pr_err("[%s] parameter error.\n", __func__);
		return;
	}

	hl7019d_config_interface((unsigned char)(HL7019D_CONC),
				(unsigned char)(val),
				(unsigned char)(CONC_BAT_VCLAMP_MASK),
				(unsigned char)(CONC_BAT_VCLAMP_SHIFT)
				);
}

void hl7019d_set_boost_9v_en(unsigned int val)
{
	if(val > 1) {
		pr_err("[%s] parameter error.\n", __func__);
		return;
	}

	hl7019d_config_interface((unsigned char)(HL7019D_CONC),
				(unsigned char)(val),
				(unsigned char)(CONC_BOOST_9V_EN_MASK),
				(unsigned char)(CONC_BOOST_9V_EN_SHIFT)
				);
}

/* COND */
void hl7019d_set_disable_ts(unsigned int val)
{
	if(val > 1) {
		pr_err("[%s] parameter error.\n", __func__);
		return;
	}

	hl7019d_config_interface((unsigned char)(HL7019D_COND),
				(unsigned char)(val),
				(unsigned char)(COND_DISABLE_TS_MASK),
				(unsigned char)(COND_DISABLE_TS_SHIFT)
				);
}

void hl7019d_set_vindpm_offset(unsigned int val)
{
	if(val > 1) {
		pr_err("[%s] parameter error.\n", __func__);
		return;
	}

	hl7019d_config_interface((unsigned char)(HL7019D_COND),
				(unsigned char)(val),
				(unsigned char)(COND_VINDPM_OFFSET_MASK),
				(unsigned char)(COND_VINDPM_OFFSET_SHIFT)
				);
}
/* chager IC fundation functiong */
/*-------------------------------------------------------------------*/

/* charger operation's functions */
static int hl7019d_dump_register(struct charger_device *chg_dev)
{
	unsigned int status = 0;
	int i;
	unsigned char reg_val;

	if(!chg_dev)
		return -EINVAL;

	pr_err("[hl7019d] dump info:\n");
	for(i = 0; i < HL7019D_REG_NUM + 1; i++)
	{
		hl7019d_read_byte(i, &reg_val, 1);
		pr_err("[%d] = 0x%x\n", i, reg_val);
	}

	return status;
}

static int hl7019d_enable_charging(struct charger_device *chg_dev, bool en)
{
	unsigned int status = 0;

	if(!chg_dev)
		return -EINVAL;

	if (is_otg_mode)
	{
		pr_err("[hl7019d]%s: OTG mode,do not setting charging\n", __func__);
		return status;
	}

	if (en) {
		hl7019d_set_chg_config(0x1);
	} else {
		hl7019d_set_chg_config(0x0);
	}

	return status;
}

static int hl7019d_get_current(struct charger_device *chg_dev, u32 *ichg)
{
	int status = 0;
	unsigned int array_size;
	unsigned char reg_value;

	if(!chg_dev)
		return -EINVAL;

	array_size = ARRAY_SIZE(CSTH);
	hl7019d_read_interface(HL7019D_CON2, &reg_value, CON2_ICHG_MASK, CON2_ICHG_SHIFT);	/* charge current */
	*ichg = charging_value_to_parameter_hl7019d(CSTH, array_size, reg_value);

	return status;
}

static int hl7019d_set_current(struct charger_device *chg_dev, u32 current_value)
{
	unsigned int status = 0;
	unsigned int set_chr_current;
	unsigned int array_size;
	unsigned char register_value;

    pr_err("%s()current_value = %d.\n",__func__,current_value);

	if(!chg_dev)
		return -EINVAL;

	if (is_otg_mode)
	{
		pr_err("[hl7019d]%s: OTG mode,do not setting charging\n", __func__);
		return status;
	}

	pr_err("[hl7019d] charge current setting value: %d.\n", current_value);
	if (current_value <= 500000) {
		hl7019d_set_ichg(0x0);
	} else {
		array_size = ARRAY_SIZE(CSTH);
		set_chr_current = bmt_find_closest_level(CSTH, array_size, current_value);
		pr_err("[hl7019d] charge current finally setting value: %d.\n", set_chr_current);
		register_value = charging_parameter_to_value_hl7019d(CSTH, array_size, set_chr_current);
		hl7019d_set_ichg(register_value);
	}

	return status;
}

static int hl7019d_get_cv_voltage(struct charger_device *chg_dev, u32 *cv)
{
	int status = 0;
	unsigned int array_size;
	unsigned char reg_value;

	if(!chg_dev)
		return -EINVAL;

	array_size = ARRAY_SIZE(VBAT_CVTH);
	hl7019d_read_interface(HL7019D_CON4, &reg_value, CON4_VREG_MASK, CON4_VREG_SHIFT);
	*cv = charging_value_to_parameter_hl7019d(VBAT_CVTH, array_size, reg_value);

	return status;
}

static int hl7019d_set_cv_voltage(struct charger_device *chg_dev, u32 cv)
{
	int status = 0;
	unsigned short int array_size;
	unsigned int set_cv_voltage;
	unsigned char  register_value;

	if(!chg_dev)
		return -EINVAL;

	if (is_otg_mode)
	{
		pr_err("[hl7019d]%s: OTG mode,do not setting charging\n", __func__);
		return status;
	}

	pr_err("[hl7019d] charge voltage setting value: %d.\n", cv);
	/*static kal_int16 pre_register_value; */
	array_size = ARRAY_SIZE(VBAT_CVTH);
	/*pre_register_value = -1; */
	set_cv_voltage = bmt_find_closest_level(VBAT_CVTH, array_size, cv);

	register_value =
	charging_parameter_to_value_hl7019d(VBAT_CVTH, array_size, set_cv_voltage);
	pr_err("[hl7019d] charging_set_cv_voltage register_value=0x%x %d %d\n",
	 register_value, cv, set_cv_voltage);
	hl7019d_set_vreg(register_value);

	return status;
}

static int hl7019d_get_input_current(struct charger_device *chg_dev, u32 *aicr)
{
	unsigned int status = 0;
	unsigned int array_size;
	unsigned char register_value;
	
	if(!chg_dev)
		return -EINVAL;

	array_size = ARRAY_SIZE(INPUT_CSTH);
	hl7019d_read_interface(HL7019D_CON0, &register_value, CON0_IINLIM_MASK, CON0_IINLIM_SHIFT);
	*aicr = charging_parameter_to_value_hl7019d(INPUT_CSTH, array_size, register_value);

	return status;
}

static int hl7019d_set_input_current(struct charger_device *chg_dev, u32 current_value)
{
	unsigned int status = 0;
	unsigned int set_chr_current;
	unsigned int array_size;
	unsigned char register_value;
	
    pr_err("%s, input current = %d.\n",__func__,current_value);

	if(!chg_dev)
		return -EINVAL;

	if (is_otg_mode)
	{
		pr_err("[hl7019d]%s: OTG mode,do not setting charging\n", __func__);
		return status;
	}

	pr_err("[hl7019d] charge input current setting value: %d.\n", current_value);
	if (current_value < 100000) {
		register_value = 0x0;
	} else {
		array_size = ARRAY_SIZE(INPUT_CSTH);
		set_chr_current = bmt_find_closest_level(INPUT_CSTH, array_size, current_value);
		pr_err("[hl7019d] charge input current finally setting value: %d.\n", set_chr_current);
		register_value = charging_parameter_to_value_hl7019d(INPUT_CSTH, array_size, set_chr_current);
		pr_err("%s register_value = 0x%x\n", __func__, register_value);
	}

	hl7019d_set_iinlim(register_value);

	return status;
}

static int hl7019d_get_termination_curr(struct charger_device *chg_dev, u32 *term_curr)
{
	unsigned int status = 0;
	unsigned int array_size;
	unsigned char register_value;

	if(!chg_dev)
		return -EINVAL;

	array_size = ARRAY_SIZE(ITERM_CSTH);
	hl7019d_read_interface(HL7019D_CON3, &register_value, CON3_ITERM_MASK, CON3_ITERM_SHIFT);
	*term_curr = charging_parameter_to_value_hl7019d(ITERM_CSTH, array_size, register_value);

	return status;
}

static int hl7019d_set_termination_curr(struct charger_device *chg_dev, u32 term_curr)
{
	unsigned int status = 0;
	unsigned int set_term_current;
	unsigned int array_size;
	unsigned char register_value;

	if(!chg_dev)
		return -EINVAL;

	pr_err("[hl7019d] charge termination current setting value: %d.\n", term_curr);
	if(term_curr < 100000) {
		hl7019d_set_iterm(0x0);
	} else {
		array_size = ARRAY_SIZE(ITERM_CSTH);
		set_term_current = bmt_find_closest_level(ITERM_CSTH, array_size, term_curr);
		pr_err("[hl7019d] charge termination current finally setting value: %d.\n", set_term_current);
		register_value = charging_parameter_to_value_hl7019d(ITERM_CSTH, array_size, set_term_current);
	}

	hl7019d_set_iterm(register_value);

	return status;
}

static int hl7019d_reset_watch_dog_timer(struct charger_device *chg_dev)
{
	unsigned int status = 0;

	if(!chg_dev)
		return -EINVAL;

	hl7019d_set_i2cwatchdog_timer_reset(0x1);
	/* charger status polling */
	hl7019d_charger_system_status();
	hl7019d_charger_fault_status();

	return status;
}

static int hl7019d_do_event(struct charger_device *chg_dev, unsigned int event, unsigned int args)
{
	if (chg_dev == NULL)
		return -EINVAL;

	pr_err("[hl7019d] %s: event = %d\n", __func__, event);

	switch (event) {
	//case EVENT_EOC:
	case EVENT_FULL:
		charger_dev_notify(chg_dev, CHARGER_DEV_NOTIFY_EOC);
		break;
	case EVENT_RECHARGE:
		charger_dev_notify(chg_dev, CHARGER_DEV_NOTIFY_RECHG);
		break;
	default:
		break;
	}

	return 0;
}
//Antaiui <AI_BSP_CHG> <hehl> <2021-05-11> add reset ta begin
static int hl7019d_reset_ta(struct charger_device *chg_dev)
{
	pr_err("%s\n", __func__);
	//hl7019d_set_vindpm(1);
	hl7019d_set_chg_config(0x0);
	msleep(300);
	//hl7019d_set_vindpm(0);
	hl7019d_set_chg_config(0x1);

	return 0;
}
//Antaiui <AI_BSP_CHG> <hehl> <2021-05-11> add reset ta end
static int hl7019d_set_ta_current_pattern(struct charger_device *chg_dev, bool is_increase)
{
	unsigned int status = 0;

	if(!chg_dev)
		return -EINVAL;

	if(true == is_increase) {
		hl7019d_set_iinlim(0x0); /* 100mA */
		msleep(85);

		hl7019d_set_iinlim(0x2); /* 500mA */
		pr_err("[hl7019d] mtk_ta_increase() on 1");
		msleep(85);

		hl7019d_set_iinlim(0x0); /* 100mA */
		pr_err("[hl7019d] mtk_ta_increase() off 1");
		msleep(85);

		hl7019d_set_iinlim(0x2); /* 500mA */
		pr_err("[hl7019d] mtk_ta_increase() on 2");
		msleep(85);

		hl7019d_set_iinlim(0x0); /* 100mA */
		pr_err("[hl7019d] mtk_ta_increase() off 2");
		msleep(85);

		hl7019d_set_iinlim(0x2); /* 500mA */
		pr_err("[hl7019d] mtk_ta_increase() on 3");
		msleep(281);

		hl7019d_set_iinlim(0x0); /* 100mA */
		pr_err("[hl7019d] mtk_ta_increase() off 3");
		msleep(85);

		hl7019d_set_iinlim(0x2); /* 500mA */
		pr_err("[hl7019d] mtk_ta_increase() on 4");
		msleep(281);

		hl7019d_set_iinlim(0x0); /* 100mA */
		pr_err("[hl7019d] mtk_ta_increase() off 4");
		msleep(85);

		hl7019d_set_iinlim(0x2); /* 500mA */
		pr_err("[hl7019d] mtk_ta_increase() on 5");
		msleep(281);

		hl7019d_set_iinlim(0x0); /* 100mA */
		pr_err("[hl7019d] mtk_ta_increase() off 5");
		msleep(85);

		hl7019d_set_iinlim(0x2); /* 500mA */
		pr_err("[hl7019d] mtk_ta_increase() on 6");
		msleep(485);

		hl7019d_set_iinlim(0x0); /* 100mA */
		pr_err("[hl7019d] mtk_ta_increase() off 6");
		msleep(50);

		pr_err("[hl7019d] mtk_ta_increase() end\n");

		hl7019d_set_iinlim(0x2); /* 500mA */
		msleep(200);
	} else {
		hl7019d_set_iinlim(0x0); /* 100mA */
		msleep(85);

		hl7019d_set_iinlim(0x2); /* 500mA */
		pr_err("[hl7019d] mtk_ta_decrease() on 1");
		msleep(281);

		hl7019d_set_iinlim(0x0); /* 100mA */
		pr_err("[hl7019d] mtk_ta_decrease() off 1");
		msleep(85);

		hl7019d_set_iinlim(0x2); /* 500mA */
		pr_err("[hl7019d] mtk_ta_decrease() on 2");
		msleep(281);

		hl7019d_set_iinlim(0x0); /* 100mA */
		pr_err("[hl7019d] mtk_ta_decrease() off 2");
		msleep(85);

		hl7019d_set_iinlim(0x2); /* 500mA */
		pr_err("[hl7019d] mtk_ta_decrease() on 3");
		msleep(281);

		hl7019d_set_iinlim(0x0); /* 100mA */
		pr_err("[hl7019d] mtk_ta_decrease() off 3");
		msleep(85);

		hl7019d_set_iinlim(0x2); /* 500mA */
		pr_err("[hl7019d] mtk_ta_decrease() on 4");
		msleep(85);

		hl7019d_set_iinlim(0x0); /* 100mA */
		pr_err("[hl7019d] mtk_ta_decrease() off 4");
		msleep(85);

		hl7019d_set_iinlim(0x2); /* 500mA */
		pr_err("[hl7019d] mtk_ta_decrease() on 5");
		msleep(85);

		hl7019d_set_iinlim(0x0); /* 100mA */
		pr_err("[hl7019d] mtk_ta_decrease() off 5");
		msleep(85);

		hl7019d_set_iinlim(0x2); /* 500mA */
		pr_err("[hl7019d] mtk_ta_decrease() on 6");
		msleep(485);

		hl7019d_set_iinlim(0x0); /* 100mA */
		pr_err("[hl7019d] mtk_ta_decrease() off 6");
		msleep(50);

		pr_err("[hl7019d] mtk_ta_decrease() end\n");

		hl7019d_set_iinlim(0x2); /* 500mA */
	}

	return status;
}

static int hl7019d_is_powerpath_enabled(struct charger_device *chg_dev, bool *en)
{
	unsigned int status = 0;
	unsigned char register_value = 0;

	if(!chg_dev)
		return -EINVAL;

	hl7019d_read_interface(HL7019D_CON0, &register_value, CON0_VINDPM_MASK, CON0_VINDPM_SHIFT);
	
	if(0xf == register_value)
		*en = false;
	else if (0x7 == register_value)
		*en = true;

	return status;
}

static int hl7019d_enable_powerpath(struct charger_device *chg_dev, bool en)
{
	unsigned int status = 0;

	if(!chg_dev)
		return -EINVAL;
	pr_err("[hl7019d] hl7019d_enable_powerpath: %d.-------------\n", en);

	if(true == en)
		hl7019d_set_vindpm(0x9); // 4.61V
	else
		hl7019d_set_vindpm(0xf); // 5.08V

	return status;
}

static int hl7019d_get_vindpm_voltage(struct charger_device *chg_dev, bool *in_loop)
{
	unsigned int status = 0;
	unsigned char register_value = 0;

	if(!chg_dev)
		return -EINVAL;

	hl7019d_read_interface(HL7019D_CON8, &register_value, CON8_DPM_STAT_MASK, CON8_DPM_STAT_SHIFT);

	if(0 == register_value)
		*in_loop = true;
	else
		*in_loop = false;

	return status;
}

static int hl7019d_set_vindpm_voltage(struct charger_device *chg_dev, u32 vindpm_vol)
{
	unsigned int status = 0;
	unsigned int array_size;
	unsigned int set_vindpm_vol;
	unsigned char register_value;
	unsigned char vindpm_status = 0;

	if(!chg_dev)
		return -EINVAL;
	
	hl7019d_read_interface(HL7019D_COND, &vindpm_status, COND_VINDPM_OFFSET_MASK, COND_VINDPM_OFFSET_SHIFT);

	if(1 == vindpm_status) {
		pr_err("[hl7019d] vindpm voltage setting value: %d.-------------111\n", vindpm_vol);

		if(vindpm_vol < 8300000) {
			pr_err("[hl7019d] enter high vindpm mode.--------111\n");
			hl7019d_set_vindpm(0x0);
		} else {
			array_size = ARRAY_SIZE(VINDPM_HIGH_CVTH);
			set_vindpm_vol = bmt_find_closest_level(VINDPM_HIGH_CVTH, array_size, vindpm_vol);
			pr_err("[hl7019d] vindpm voltage finally setting value: %d.-------------111\n", set_vindpm_vol);
			register_value = charging_parameter_to_value_hl7019d(VINDPM_HIGH_CVTH, array_size, set_vindpm_vol);
			hl7019d_set_vindpm(register_value);
		}	
	} else {
		pr_err("[hl7019d] vindpm voltage setting value: %d.\n", vindpm_vol);

		if(vindpm_vol < 3500000) {
			pr_err("[hl7019d] enter high vindpm mode.\n");
			hl7019d_set_vindpm(0x0);
		} else {
			array_size = ARRAY_SIZE(VINDPM_NORMAL_CVTH);
			set_vindpm_vol = bmt_find_closest_level(VINDPM_NORMAL_CVTH, array_size, vindpm_vol);
			pr_err("[hl7019d] vindpm voltage finally setting value: %d.\n", set_vindpm_vol);
			register_value = charging_parameter_to_value_hl7019d(VINDPM_NORMAL_CVTH, array_size, set_vindpm_vol);
			hl7019d_set_vindpm(register_value);
		}
	}

	return status;
}

static int hl7019d_is_safety_timer_enabled(struct charger_device *chg_dev, bool *en)
{
	unsigned int status = 0; 
	unsigned char register_value = 0;

	if(!chg_dev)
		return -EINVAL;

	hl7019d_read_interface(HL7019D_CON5, &register_value, CON5_CHG_TIMER_MASK, CON5_CHG_TIMER_SHIFT);

	if(1 == register_value)
		*en = true;
	else
		*en = false;

	return status;
}

static int hl7019d_enable_safety_timer(struct charger_device *chg_dev, bool en)
{
	unsigned int status = 0; 

	if(!chg_dev)
		return -EINVAL;

	if(true == en)
		hl7019d_set_en_safty_timer(0x1);
	else
		hl7019d_set_en_safty_timer(0x0);

	return status;
}

static int hl7019d_charger_enable_otg(struct charger_device *chg_dev, bool en)
{
	unsigned int status = 0;

	if(!chg_dev)
		return -EINVAL;
	
	pr_err("[hl7019d] enable otg %d.\n", en);
	
	if(true == en) {
		hl7019d_set_chg_config(0x2);
		//enable_boost_polling(true);
		is_otg_mode = 1;
	}else {
		hl7019d_set_chg_config(0x0);
		//enable_boost_polling(false);
		is_otg_mode = 0;
	}

	return status;
}

static int hl7019d_set_boost_current_limit(struct charger_device *chg_dev, u32 boost_curr)
{
	unsigned int status = 0;

	if(!chg_dev)
		return -EINVAL;

#ifdef SET_OTG_CURRENT_TO_1A
	hl7019d_set_boost_lim(0x0);
#else
	if(boost_curr < 1000000)
		hl7019d_set_boost_lim(0x0);
	else
		hl7019d_set_boost_lim(0x1);
#endif

	return status;
}

static int hl7019d_get_charging_status(struct charger_device *chg_dev, bool *is_done)
{
	unsigned int status = 0;
	unsigned int register_val;

	if(!chg_dev)
		return -EINVAL;

	register_val = hl7019d_get_chrg_status();
	if (0x3 == register_val)
		*is_done = true;
	else
		*is_done = false;

	return status;
}
/*-------------------------------------------------------------------*/

int hl7019d_get_vender_code(void)
{
	int ret = -ENODEV;
	unsigned char register_val;

	hl7019d_read_byte(HL7019D_CONA, &register_val, 1);

	ret = (int) register_val;

	return ret;
}

static int hl7019d_parse_dt(struct hl7019d_info *info, struct device *dev)
{
	struct device_node *np = dev->of_node;

	pr_err("[hl7019d] %s\n", __func__);

	if (!np) {
		pr_err("[hl7019d] %s: no of node\n", __func__);
		return -ENODEV;
	}

	pr_err("[hl7019d] node name: %s.\n", np->name);

	if (of_property_read_string(np, "charger_name", &info->chg_dev_name) < 0) {
		info->chg_dev_name = "primary_chg";
		pr_err("[hl7019d] %s: no charger name\n", __func__);
	}

	if (of_property_read_string(np, "alias_name", &(info->chg_props.alias_name)) < 0) {
		info->chg_props.alias_name = "hl7019d";
		pr_err("[hl7019d] %s: no alias name\n", __func__);
	}

/* Miki <BSP_CHARGER> <lin_zc> <20190428> add for charger mode detect start */
	if (of_property_read_string(np, "eint_name", &info->eint_name) < 0) {
		info->eint_name = "chr_stat";
		pr_err("%s: no eint name\n", __func__);
	}
/* Miki <BSP_CHARGER> <lin_zc> <20190428> add for charger mode detect end */

	return 0;
}

static int hl7019d_charger_ic_init(struct charger_device *chg_dev)
{
	unsigned int status = 0;

	if(!chg_dev)
		return -EINVAL;

	hl7019d_set_en_hiz(0);	 // enable IC
	hl7019d_set_vindpm(0x7);	 //vindpm 4.44V
	hl7019d_set_iinlim(0x2);	 //input current 0.5A

	hl7019d_set_sys_min(0x5);  // sys min 3.5V
	hl7019d_set_boost_lim(0x0); //boost limit 1A

	hl7019d_set_bcold(0x0);		//boost cold threshold
	hl7019d_set_force_20pct(0x0);  //set as charge current as registers value

	hl7019d_set_iprechg(0x03);	//precharge current 512mA
	hl7019d_set_iterm(0x1);		//termination current 256mA

	hl7019d_set_vreg(0x3d);		// VREG 4.48V
	hl7019d_set_batlowv(0x1);	//battery low 3.0V
	hl7019d_set_vrechg(0x0);		//recharge delta 100mV

	hl7019d_set_en_term(0x1);	//enable termination
	hl7019d_set_term_stat(0x1);	//match termination
/* Miki <BSP_CHARGER> <lin_zc> <20190509> modify for charger watchdog start */
	hl7019d_set_i2cwatchdog(0x0); //disable i2c watchdog
/* Miki <BSP_CHARGER> <lin_zc> <20190509> modify for charger watchdog end */
	hl7019d_set_charge_timer(0x2); //safety charge time upto 12h

	hl7019d_set_boostv(0x7);		//boost voltage upto 4.998V
	hl7019d_set_bhot(0x2);		//boost hot threshold 65 centigrade degree
	hl7019d_set_treg(0x3);		//thermal regulation threadhold upto 120 centigrade degree

	hl7019d_set_dpdm_en(0x0);	//disble D+/D- detection
	hl7019d_set_tmr2x_en(0x1);
	hl7019d_set_ppfet_disable(0x0);	//enable Q4
	hl7019d_set_chrgfault_int_mask(0x0);		//disable charge fault intteruppt
	hl7019d_set_batfault_int_mask(0x0);		//disable battery fault intteruppt

/* Miki <BSP_CHARGER> <lin_zc> <20190528> modify for force restart system begin */
	hl7019d_set_tsr(0x3);		//PPON debounce time 10s
	hl7019d_set_tsrp(0x0);		//system reset period 500ms
	hl7019d_set_dis_connect(0x0); //reconnect after system reset
/* Miki <BSP_CHARGER> <lin_zc> <20190528> modify for force restart system end */
	hl7019d_set_dis_sr_inchg(0x0);
	hl7019d_set_tship(0x0);		//shipping mode active delay

	hl7019d_set_bat_comp(0x0);		//disable battery compensation
	hl7019d_set_bat_vclamp(0x0);		//disable clamp
	hl7019d_set_boost_9v_en(0x0);	//diaable boost 9V

	hl7019d_set_disable_ts(0x1);		//disable ts
	hl7019d_set_vindpm_offset(0x0);	//vindpm in normal value

	return status;
}

static void enable_boost_polling(bool poll_en)
{
	struct timespec time, time_now, end_time;
	ktime_t ktime;

	if (g_info) {
		if (poll_en) {
			get_monotonic_boottime(&time_now);
			time.tv_sec = g_info->polling_interval;
			time.tv_nsec = 0;
			timespec_add(time_now, time);
			ktime = ktime_set(end_time.tv_sec, end_time.tv_nsec);
			alarm_start(&g_info->otg_kthread_gtimer, ktime);
			g_info->polling_enabled = true;
		} else {
			g_info->polling_enabled = false;
			alarm_cancel(&g_info->otg_kthread_gtimer);
		}
	}
}

static void usbotg_boost_kick_work(struct work_struct *work)
{
	ktime_t ktime;
	struct timespec time, time_now, end_time;
	struct hl7019d_info *boost_manager =
		container_of(work, struct hl7019d_info, kick_work);

	pr_err("[hl7019d] usbotg_boost_kick_work\n");

	hl7019d_set_i2cwatchdog_timer_reset(0x1);

	if (boost_manager->polling_enabled == true) {
		get_monotonic_boottime(&time_now);
		time.tv_sec = boost_manager->polling_interval;
		time.tv_nsec = 0;
		timespec_add(time_now, time);
		ktime = ktime_set(end_time.tv_sec, end_time.tv_nsec);
		alarm_start(&boost_manager->otg_kthread_gtimer, ktime);
	}
}

static enum alarmtimer_restart usbotg_gtimer_func(struct alarm *alarm,
						 ktime_t now)
{
	struct hl7019d_info *boost_manager =
		container_of(alarm, struct hl7019d_info,
			     otg_kthread_gtimer);

	queue_work(boost_manager->otg_boost_workq,
		   &boost_manager->kick_work);

	return 0;
}

#ifdef HL7019D_CHECK_CHARGING_STATE
void hl7019d_charging_check_switch(int enable)
{

	if (enable) {
		pr_err("[hl7019d]: charging check start!!\n");
		queue_delayed_work(charging_check_workqueue, &charging_check_work,
				msecs_to_jiffies(CHARGING_CHECK_WAIT_TIME));
	} else {
		pr_err("[hl7019d]: charging check stop!!\n");
		cancel_delayed_work_sync(&charging_check_work);
	}
}

static void hl7019d_charging_check_func(struct work_struct *work)
{
	struct hl7019d_info *info = charging_check_info;
	unsigned int pg_stat = 0;
	unsigned int vbus_stat = 0;

	pg_stat = hl7019d_get_pg_status();
	vbus_stat = hl7019d_get_vin_status();

	pr_err("[hl7019d]enter %s,begin charging check\n", __func__);

	if (check_time < CHARGING_CHECK_TIME)	// check vbus&vin x times
	{	
		if ((0 == pg_stat) && (0 ==vbus_stat ))
		{
			pr_err("%s:adapter/usb removed\n", __func__);

			if (1 == atomic_read(&hl7019d_irq_enable_charging))
				atomic_set(&hl7019d_irq_enable_charging, 0);

			info->chg_type = hl7019d_get_charger_type(info);
			if (CHARGER_UNKNOWN != info->chg_type)
			{
				pr_err("[hl7019d]vbus has no voltage,disconnet charger\n");
				info->chg_type = CHARGER_UNKNOWN;
			}

			hl7019d_set_charger_type(info);	// switch charge mode
			info->power_good = pg_stat;	// save vbus state

			check_time = 0;				// reset check_time to default 0
			return;
		} else {
			queue_delayed_work(charging_check_workqueue, &charging_check_work,
					msecs_to_jiffies(CHARGING_CHECK_WAIT_TIME));
			check_time++;
		}
	} else {
		check_time = 0;	// reset check_time to default 0
	}
}

int hl7019d_charging_check_init(void)
{
	charging_check_workqueue = create_singlethread_workqueue("hl7019d_check");
	if (!charging_check_workqueue) {
		pr_err("charging_check_workqueue is NULL, can't run hl7019d charging check function\n");
		return EINVAL;
	}

	INIT_DELAYED_WORK(&charging_check_work, hl7019d_charging_check_func);
	return 0;
}
#endif

#if 0
static int hl7019d_get_charger_type(struct hl7019d_info *info)
{
	unsigned int vbus_stat = 0;
	unsigned int pg_stat = 0;

	enum charger_type CHR_Type_num = CHARGER_UNKNOWN;


	pg_stat = hl7019d_get_pg_status();
	vbus_stat = hl7019d_get_vin_status();
	pr_err("pg_stat: 0x%x, vbus_stat:0x%x\n", pg_stat, vbus_stat);

	switch (vbus_stat) {
	case 0: /* No input */
		CHR_Type_num = CHARGER_UNKNOWN;
		break;
	case 1: /* SDP */
		CHR_Type_num = STANDARD_HOST;
		break;
	case 2: /* DCP */
		CHR_Type_num = STANDARD_CHARGER;
		break;
	default:
		CHR_Type_num = CHARGER_UNKNOWN;
		break;
	}

	return CHR_Type_num;
}


static int hl7019d_set_charger_type(struct hl7019d_info *info)
{
	int ret = 0;
	union power_supply_propval propval;

#if defined(CONFIG_PROJECT_PHY) || defined(CONFIG_PHY_MTK_SSUSB)
	if (info->chg_type == STANDARD_HOST)
		Charger_Detect_Release();
#endif

	if (info->chg_type != CHARGER_UNKNOWN)
		propval.intval = 1;
	else
		propval.intval = 0;
	ret = power_supply_set_property(info->psy,
		POWER_SUPPLY_PROP_ONLINE, &propval);
	if (ret < 0)
		pr_err("%s: inform power supply online failed, ret = %d\n",
			__func__, ret);

	propval.intval = info->chg_type;
	ret = power_supply_set_property(info->psy,
		POWER_SUPPLY_PROP_CHARGE_TYPE, &propval);
	if (ret < 0)
		pr_err("%s: inform power supply type failed, ret = %d\n",
			__func__, ret);

	return ret;
}


void hl7019d_irq_disable_charging(void)
{
	u8 prev_pg = 0;
	u8 now_pg = 0;
	struct hl7019d_info *info = charging_check_info;

	pr_err("enter %s\n", __func__);

	if (!is_chager_ic_on)  // charger IC is not hl7019d
	{
		pr_err("[hl7019d]charger ic is not hl7019d!\n");
		return ;
	}

	if (0 == atomic_read(&hl7019d_irq_enable_charging))
		return;

	prev_pg = info->power_good;
	now_pg = hl7019d_get_pg_status();

	if (now_pg)	// detect vbus still has voltage
	{	
		hl7019d_charging_check_switch(1);	// run thread to double check vbus voltage
		pr_err("[hl7019d]detect pluging out adapter too fast\n");
		return;	// let thread to do it.
	}

	info->power_good = now_pg;

	if (prev_pg && !info->power_good) {
		pr_err("%s:adapter/usb removed\n", __func__);

		if (1 == atomic_read(&hl7019d_irq_enable_charging))
			atomic_set(&hl7019d_irq_enable_charging, 0);

		info->chg_type = hl7019d_get_charger_type(info);
		if (CHARGER_UNKNOWN != info->chg_type)
		{
			pr_err("[hl7019d]vbus has no voltage,force disconnet charger\n");
			info->chg_type = CHARGER_UNKNOWN;
		}
		
		hl7019d_set_charger_type(info);
	}
}

static irqreturn_t hl7019d_irq_handler(int irq, void *data)
{
	u8 prev_pg = 0;
	enum charger_type prev_chg_type;
	struct hl7019d_info *info = (struct hl7019d_info *)data;

	pr_err("enter %s\n", __func__);

	if (1 == atomic_read(&hl7019d_irq_enable_charging))
	{
		pr_err("atomic_read hl7019d_irq_enable_charging 1 %s\n", __func__);
		return IRQ_HANDLED;
	}
	msleep(60);   // avoid plug out by pmic_thread

#if defined(CONFIG_PROJECT_PHY) || defined(CONFIG_PHY_MTK_SSUSB)
	if (0 == atomic_read(&hl7019d_irq_enable_charging))
	{
		Charger_Detect_Init();
		pr_err("%s Charger_Detect_Init\n", __func__);
	}	
#endif
	prev_pg = info->power_good;

	info->power_good = hl7019d_get_pg_status();

	if (!prev_pg && info->power_good)
	{
		pr_err("%s:adapter/usb inserted\n", __func__);
	}

	prev_chg_type = info->chg_type;

	info->chg_type = hl7019d_get_charger_type(info);
	if (prev_chg_type != info->chg_type)
	{
		pr_err("%s hl7019d_set_charger_type\n", __func__);
		hl7019d_set_charger_type(info);
		if ((0 == atomic_read(&hl7019d_irq_enable_charging)) && (info->chg_type != CHARGER_UNKNOWN))
		{
			pr_err("%s hl7019d_irq_enable_charging\n", __func__);
			atomic_set(&hl7019d_irq_enable_charging, 1);
		}
	}

	return IRQ_HANDLED;
}

static int hl7019d_register_irq(struct hl7019d_info *info)
{
	int ret = 0;
	struct device_node *np;

	/* Parse irq number from dts */
	np = of_find_node_by_name(NULL, info->eint_name);
	if (np)
		info->irq = irq_of_parse_and_map(np, 0);
	else {
		pr_err("%s: cannot get node\n", __func__);
		ret = -ENODEV;
		goto err_nodev;
	}
	pr_err("%s: irq = %d\n", __func__, info->irq);

	/* Request threaded IRQ */
	ret = devm_request_threaded_irq(info->dev, info->irq, NULL,
		hl7019d_irq_handler, IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
		info->eint_name, info);
	if (ret < 0) {
		pr_err("%s: request thread irq failed\n", __func__);
		goto err_request_irq;
	}

	enable_irq_wake(info->irq);
	return 0;

err_nodev:
err_request_irq:
	return ret;
}
#endif

static struct charger_ops hl7019d_chg_ops = {

	/* enable charging */
	.enable = hl7019d_enable_charging,
	/* charge current stuff */
	.get_charging_current = hl7019d_get_current,
	.set_charging_current = hl7019d_set_current,
	/* charge voltage stuff */
	.get_constant_voltage = hl7019d_get_cv_voltage,
	.set_constant_voltage = hl7019d_set_cv_voltage,
	/* input charge current stuff */
	.get_input_current = hl7019d_get_input_current,
	.set_input_current = hl7019d_set_input_current,
	/* termination current stuff */
	.get_eoc_current = hl7019d_get_termination_curr,
	.set_eoc_current = hl7019d_set_termination_curr,
	/* ic watch dog */
	.kick_wdt = hl7019d_reset_watch_dog_timer,
	.event = hl7019d_do_event,
	/* pe/pe+ */
	.send_ta_current_pattern = hl7019d_set_ta_current_pattern,
	/* powerpath stuff */
	.is_powerpath_enabled = hl7019d_is_powerpath_enabled,
	.enable_powerpath = hl7019d_enable_powerpath,
	/* vindpm stuff */
	.get_mivr_state = hl7019d_get_vindpm_voltage,
	.set_mivr = hl7019d_set_vindpm_voltage,
	/* safety timer stuff */
	.is_safety_timer_enabled = hl7019d_is_safety_timer_enabled,
	.enable_safety_timer = hl7019d_enable_safety_timer,
	/* OTG */
	.enable_otg = hl7019d_charger_enable_otg,
	.set_boost_current_limit = hl7019d_set_boost_current_limit,
	/* charging over */
	.is_charging_done = hl7019d_get_charging_status,
	/* dump info */
	.dump_registers = hl7019d_dump_register,
//Antaiui <AI_BSP_CHG> <hehl> <2021-05-11> add reset ta begin	
	.reset_ta = hl7019d_reset_ta,
//Antaiui <AI_BSP_CHG> <hehl> <2021-05-11> add reset ta end	
};
#if 0
int is_hl7019d_charger_ic(void)
{
    int i = 1;

    do {
        switch (i) {
            case 1:
                pr_err("try hl7019d_SLAVE_ADDR 0x6B \n");
                new_client->addr = (hl7019d_SLAVE_ADDR_2 >> 1);
                break;
            default:
                pr_err("try hl7019d_SLAVE_ADDR 0x6C \n");
                new_client->addr = (hl7019d_SLAVE_ADDR_1 >> 1);
                break;
        }

        dev_detect = hl7019d_get_vender_code();
		pr_err("Charger IC dev_detect=0x%x \n",dev_detect);
        
        if (0x23 == dev_detect) { //if (0x20 == dev_detect) {
                break;
        }
    }while(i--);

	if (0x23 != dev_detect)//if (0x20 != dev_detect)
	{
		pr_err("Charger IC is not HL7019D\n");
		return 0;
	} else {
		pr_err("Using HL7019D, begin to initial\n");
		return 1;
	}
}
#endif

static int hl7019d_driver_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;
	struct hl7019d_info *info = NULL;
	u8 reg_val = 0;

	pr_err("endy,%s()++\n",__func__);

	//+add by hzb for ontim debug
	if(CHECK_THIS_DEV_DEBUG_AREADY_EXIT()==0)
	{
		return -EIO;
	}
	//-add by hzb for ontim debug

	info = devm_kzalloc(&client->dev, sizeof(struct hl7019d_info), GFP_KERNEL);

	if (!info)
		return -ENOMEM;

	new_client = client;
	info->dev = &client->dev;

	ret = hl7019d_parse_dt(info, &client->dev);
	if (ret < 0)
		return ret;

	reg_val = hl7019d_get_vender_code();
	if (reg_val != 0x20) {
		pr_err("%s: get vendor id failed\n", __func__);
		return -ENODEV;
	}

	/* Register charger device */
	info->chg_dev = charger_device_register(info->chg_dev_name,
		&client->dev, info, &hl7019d_chg_ops, &info->chg_props);

	if (IS_ERR_OR_NULL(info->chg_dev)) {
		pr_err("[hl7019d] %s: register charger device failed\n", __func__);
		ret = PTR_ERR(info->chg_dev);
		return ret;
	}

/*
	info->psy = power_supply_get_by_name("charger");

	if (!info->psy) {
		pr_err("[hl7019d] %s: get power supply failed\n", __func__);
		return -EINVAL;
	}
*/
	ret = hl7019d_charger_ic_init(info->chg_dev);
	if(ret)
	{
		pr_err("[hl7019d] %s: charger ic initiation occurs error.\n", __func__);
		return -ENODEV;
	}

	hl7019d_dump_register(info->chg_dev);

	alarm_init(&info->otg_kthread_gtimer, ALARM_BOOTTIME,
		  usbotg_gtimer_func);

	info->otg_boost_workq = create_singlethread_workqueue("otg_boost_workq");
	INIT_WORK(&info->kick_work, usbotg_boost_kick_work);
	info->polling_interval = 20;
	g_info = info;
	//info->chg_type = CHARGER_UNKNOWN;

//#if defined(CONFIG_PROJECT_PHY) || defined(CONFIG_PHY_MTK_SSUSB)
//	Charger_Detect_Init();
//#endif
	//hl7019d_register_irq(info);
	//hl7019d_set_dpdm_en(0x1);

#ifdef HL7019D_CHECK_CHARGING_STATE
	ret = hl7019d_charging_check_init();
	if (ret)
		pr_err("[hl7019d]can not creat charging check function\n");
#endif

	//charging_check_info = info;

	//+add by hzb for ontim debug
	REGISTER_AND_INIT_ONTIM_DEBUG_FOR_THIS_DEV();
	//-add by hzb for ontim debug

	pr_err("endy,%s()--\n",__func__);

	return 0;
}


static const struct i2c_device_id hl7019d_i2c_id[] = { {"hl7019d", 0}, {} };

#ifdef CONFIG_OF
static const struct of_device_id hl7019d_of_match[] = {
	{.compatible = "mediatek,hl7019d"},
	{},
};
#endif

static struct i2c_driver hl7019d_driver = {
	.driver = {
		.name = "hl7019d",
#ifdef CONFIG_OF
		.of_match_table = hl7019d_of_match,
#endif
		},
	.probe = hl7019d_driver_probe,
	.id_table = hl7019d_i2c_id,
};

static int __init hl7019d_init(void)
{

	if (i2c_add_driver(&hl7019d_driver) != 0)
		pr_err("[hl7019d] Failed to register hl7019d i2c driver.\n");
	else
		pr_err("[hl7019d] Success to register hl7019d i2c driver.\n");

	return 0;
}

static void __exit hl7019d_exit(void)
{
	i2c_del_driver(&hl7019d_driver);
}

module_init(hl7019d_init);
module_exit(hl7019d_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("I2C hl7019d Driver");
MODULE_AUTHOR("hehl@antiui.com>");
