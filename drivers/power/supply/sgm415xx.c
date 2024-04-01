// SPDX-License-Identifier: GPL-2.0

/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/types.h>
#include <linux/init.h>		/* For init/exit macros */
#include <linux/module.h>	/* For MODULE_ marcros  */
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#endif
//#include <mt-plat/mtk_boot.h>
//#include <mt-plat/upmu_common.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/power_supply.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>
#include <linux/regulator/machine.h>
#include "sgm415xx.h"
#include "wt6670f.h"
#include "charger_class.h"
#include "mtk_charger.h"
#include "tcpm.h"
/**********************************************************
 *
 *   [I2C Slave Setting]
 *
 *********************************************************/

extern void Charger_Detect_Init(void);
extern void Charger_Detect_Release(void);
#define SGM4154x_REG_NUM    (0xF)
#ifdef CONFIG_MOTO_CHG_WT6670F_SUPPORT
extern int wt6670f_set_voltage(u16 voltage);
extern int wt6670f_en_hvdcp(void);
extern void wt6670f_reset_chg_type(void);
extern int wt6670f_force_qc2_5V(void);
extern int wt6670f_force_qc3_5V(void);
extern int wt6670f_start_detection(void);
extern int wt6670f_get_protocol(void);
extern int wt6670f_get_charger_type(void);
extern bool wt6670f_is_charger_ready(void);
//extern void bq2597x_set_psy(void);
extern int wt6670f_do_reset(void);
extern bool qc3p_z350_init_ok;
bool m_chg_ready = false;
extern int m_chg_type;
extern int is_already_probe_ok;
bool wt6670f_is_detect = false;
EXPORT_SYMBOL_GPL(wt6670f_is_detect);
extern int g_qc3p_id;
#endif
static bool charger_irq_normal_flag = false;
unsigned int boost_current_limit[2];
unsigned int SGM4154x_IINDPM_I_MAX_uA;
/* SGM4154x REG06 BOOST_LIM[5:4], uV */
unsigned int BOOST_VOLT_LIMIT[] = {
	4850000, 5000000, 5150000, 5300000		
};
 /* SGM4154x REG02 BOOST_LIM[7:7], uA */
unsigned int BOOST_CURRENT_LIMIT[2][2] = {
	{1200000, 2000000},
	{500000, 1200000}
};

static const unsigned int IPRECHG_CURRENT_STABLE[] = {
	5000, 10000, 15000, 20000, 30000, 40000, 50000, 60000,
	80000, 100000, 120000, 140000, 160000, 180000, 200000, 240000
};

static const unsigned int ITERM_CURRENT_STABLE[] = {
	5000, 10000, 15000, 20000, 30000, 40000, 50000, 60000,
	80000, 100000, 120000, 140000, 160000, 180000, 200000, 240000
};

enum SGM4154x_VREG_FT {
	VREG_FT_DISABLE,
	VREG_FT_UP_8mV,
	VREG_FT_DN_8mV,
	VREG_FT_DN_16mV,
};

static enum power_supply_usb_type sgm4154x_usb_type[] = {
	POWER_SUPPLY_USB_TYPE_UNKNOWN,
	POWER_SUPPLY_USB_TYPE_SDP,
	POWER_SUPPLY_USB_TYPE_DCP,
	POWER_SUPPLY_USB_TYPE_CDP,	
};

struct charger_properties sgm4154x_chg_props = {
	.alias_name = SGM41542_NAME,
};

/**********************************************************
 *
 *   [Global Variable]
 *
 *********************************************************/
static struct power_supply_desc sgm4154x_power_supply_desc;
static struct charger_device *s_chg_dev_otg;

/**********************************************************
 *
 *   [I2C Function For Read/Write sgm4154x]
 *
 *********************************************************/
static int __sgm4154x_read_byte(struct sgm4154x_device *sgm, u8 reg, u8 *data)
{
    s32 ret;

    ret = i2c_smbus_read_byte_data(sgm->client, reg);
    if (ret < 0) {
        pr_err("i2c read fail: can't read from reg 0x%02X\n", reg);
        return ret;
    }

    *data = (u8) ret;

    return 0;
}

static int __sgm4154x_write_byte(struct sgm4154x_device *sgm, int reg, u8 val)
{
    s32 ret;

    ret = i2c_smbus_write_byte_data(sgm->client, reg, val);
    if (ret < 0) {
        pr_err("i2c write fail: can't write 0x%02X to reg 0x%02X: %d\n",
               val, reg, ret);
        return ret;
    }
    return 0;
}

static int sgm4154x_read_reg(struct sgm4154x_device *sgm, u8 reg, u8 *data)
{
	int ret;

	mutex_lock(&sgm->i2c_rw_lock);
	ret = __sgm4154x_read_byte(sgm, reg, data);
	mutex_unlock(&sgm->i2c_rw_lock);

	return ret;
}
#if 0
static int sgm4154x_write_reg(struct sgm4154x_device *sgm, u8 reg, u8 val)
{
	int ret;

	mutex_lock(&sgm->i2c_rw_lock);
	ret = __sgm4154x_write_byte(sgm, reg, val);
	mutex_unlock(&sgm->i2c_rw_lock);

	if (ret)
		pr_err("Failed: reg=%02X, ret=%d\n", reg, ret);

	return ret;
}
#endif
static int sgm4154x_update_bits(struct sgm4154x_device *sgm, u8 reg,
					u8 mask, u8 val)
{
	int ret;
	u8 tmp;

	mutex_lock(&sgm->i2c_rw_lock);
	ret = __sgm4154x_read_byte(sgm, reg, &tmp);
	if (ret) {
		pr_err("Failed: reg=%02X, ret=%d\n", reg, ret);
		goto out;
	}

	tmp &= ~mask;
	tmp |= val & mask;

	ret = __sgm4154x_write_byte(sgm, reg, tmp);
	if (ret)
		pr_err("Failed: reg=%02X, ret=%d\n", reg, ret);

out:
	mutex_unlock(&sgm->i2c_rw_lock);
	return ret;
}

#define WAIT_I2C_COUNT 50
#define WAIT_I2C_TIME 10
int mmi_sgm4154x_update_bits(struct sgm4154x_device *sgm, u8 reg,
					u8 mask, u8 val)
{
	int retry_count = 0;

	while (sgm->sgm4154x_suspend_flag && retry_count < WAIT_I2C_COUNT) {
		retry_count ++;
		dev_err(sgm->dev, "wait system resume when I2C write, count %d\n", retry_count);
		msleep(WAIT_I2C_TIME);
	}

	if (retry_count >= WAIT_I2C_COUNT)
		return -EBUSY;

	return sgm4154x_update_bits(sgm, reg, mask, val);
}

int mmi_sgm4154x_read_reg(struct sgm4154x_device *sgm, u8 reg, u8 *data)
{
	int retry_count = 0;

	while (sgm->sgm4154x_suspend_flag && retry_count < WAIT_I2C_COUNT) {
		retry_count ++;
		dev_err(sgm->dev, "wait system resume when I2C read, count %d\n", retry_count);
		msleep(WAIT_I2C_TIME);
	}

	if (retry_count >= WAIT_I2C_COUNT)
		return -EBUSY;

	return sgm4154x_read_reg(sgm, reg, data);
}

/**********************************************************
 *
 *   [Internal Function]
 *
 *********************************************************/

 static int sgm4154x_set_watchdog_timer(struct sgm4154x_device *sgm, int time)
{
	int ret;
	u8 reg_val;

	if (time == 0)
		reg_val = SGM4154x_WDT_TIMER_DISABLE;
	else if (time == 40)
		reg_val = SGM4154x_WDT_TIMER_40S;
	else if (time == 80)
		reg_val = SGM4154x_WDT_TIMER_80S;
	else
		reg_val = SGM4154x_WDT_TIMER_160S;

	ret = mmi_sgm4154x_update_bits(sgm, SGM4154x_CHRG_CTRL_5,
				SGM4154x_WDT_TIMER_MASK, reg_val);

	return ret;
}

 #if 0
 static int sgm4154x_get_term_curr(struct sgm4154x_device *sgm)
{
	int ret;
	u8 reg_val;
	int curr;
	int offset = SGM4154x_TERMCHRG_I_MIN_uA;
	
	ret = mmi_sgm4154x_read_reg(sgm, SGM4154x_CHRG_CTRL_3, &reg_val);
	if (ret)
		return ret;

	reg_val &= SGM4154x_TERMCHRG_CUR_MASK;
	curr = reg_val * SGM4154x_TERMCHRG_CURRENT_STEP_uA + offset;
	return curr;
}

static int sgm4154x_get_prechrg_curr(struct sgm4154x_device *sgm)
{
	int ret;
	u8 reg_val;
	int curr;
	int offset = SGM4154x_PRECHRG_I_MIN_uA;

	ret = mmi_sgm4154x_read_reg(sgm, SGM4154x_CHRG_CTRL_3, &reg_val);
	if (ret)
		return ret;

	reg_val = (reg_val&SGM4154x_PRECHRG_CUR_MASK)>>4;
	curr = reg_val * SGM4154x_PRECHRG_CURRENT_STEP_uA + offset;
	return curr;
}
#endif
static int sgm4154x_get_ichg_curr(struct charger_device *chg_dev, u32 *curr)
{
	int ret;
	u8 ichg;
	struct sgm4154x_device *sgm = charger_get_data(chg_dev);

	ret = mmi_sgm4154x_read_reg(sgm, SGM4154x_CHRG_CTRL_2, &ichg);
	if (ret)
		return ret;

	ichg &= SGM4154x_ICHRG_I_MASK;

	if(sgm->dev_id == SGM41513_ID || sgm->dev_id == SGM41513A_ID ||  sgm->dev_id == SGM41513D_ID) {
		if (ichg <= 0x8)
			*curr = ichg * 5000;
		else if (ichg <= 0xF)
			*curr = 40000 + (ichg - 0x8) * 10000;
		else if (ichg <= 0x17)
			*curr = 110000 + (ichg - 0xF) * 20000;
		else if (ichg <= 0x20)
			*curr = 270000 + (ichg - 0x17) * 30000;
		else if (ichg <= 0x30)
			*curr = 540000 + (ichg - 0x20) * 60000;
		else if (ichg <= 0x3C)
			*curr = 1500000 + (ichg - 0x30) * 120000;
		else
			*curr = 3000000;
	} else {
		*curr = ichg * SGM4154x_ICHRG_I_STEP_uA;
	}
	return ret;
}


static int sgm4154x_set_term_curr(struct sgm4154x_device *sgm, int uA)
{
	u8 reg_val;
	
	if(sgm->dev_id == SGM41513_ID || sgm->dev_id == SGM41513A_ID ||  sgm->dev_id == SGM41513D_ID) {
		for(reg_val = 1; reg_val < 16 && uA >= ITERM_CURRENT_STABLE[reg_val]; reg_val++)
			;
		reg_val--;
	} else {
		if (uA < SGM4154x_TERMCHRG_I_MIN_uA)
			uA = SGM4154x_TERMCHRG_I_MIN_uA;
		else if (uA > SGM4154x_TERMCHRG_I_MAX_uA)
			uA = SGM4154x_TERMCHRG_I_MAX_uA;

		reg_val = (uA - SGM4154x_TERMCHRG_I_MIN_uA) / SGM4154x_TERMCHRG_CURRENT_STEP_uA;
	}

	return mmi_sgm4154x_update_bits(sgm, SGM4154x_CHRG_CTRL_3,
				  SGM4154x_TERMCHRG_CUR_MASK, reg_val);
}

static int sgm4154x_set_prechrg_curr(struct sgm4154x_device *sgm, int uA)
{
	u8 reg_val;

	if(sgm->dev_id == SGM41513_ID || sgm->dev_id == SGM41513A_ID ||  sgm->dev_id == SGM41513D_ID) {
		for(reg_val = 1; reg_val < 16 && uA >= IPRECHG_CURRENT_STABLE[reg_val]; reg_val++)
			;
		reg_val--;
	} else {
		if (uA < SGM4154x_PRECHRG_I_MIN_uA)
			uA = SGM4154x_PRECHRG_I_MIN_uA;
		else if (uA > SGM4154x_PRECHRG_I_MAX_uA)
			uA = SGM4154x_PRECHRG_I_MAX_uA;

		reg_val = (uA - SGM4154x_PRECHRG_I_MIN_uA) / SGM4154x_PRECHRG_CURRENT_STEP_uA;
	}
	reg_val = reg_val << 4;
	return mmi_sgm4154x_update_bits(sgm, SGM4154x_CHRG_CTRL_3,
				  SGM4154x_PRECHRG_CUR_MASK, reg_val);
}

static int sgm4154x_set_ichrg_curr(struct charger_device *chg_dev, unsigned int uA)
{
	int ret;
	u8 reg_val;
	struct sgm4154x_device *sgm = charger_get_data(chg_dev);

	if (uA < SGM4154x_ICHRG_I_MIN_uA)
		uA = SGM4154x_ICHRG_I_MAX_uA;
	else if ( uA > sgm->init_data.max_ichg)
		uA = sgm->init_data.max_ichg;

	pr_info("%s set charging curr = %d\n", __func__, uA);
	if(sgm->dev_id == SGM41513_ID || sgm->dev_id == SGM41513A_ID ||  sgm->dev_id == SGM41513D_ID) {
		if (uA <= 40000)
			reg_val = uA / 5000;
		else if (uA <= 110000)
			reg_val = 0x08 + (uA -40000) / 10000;
		else if (uA <= 270000)
			reg_val = 0x0F + (uA -110000) / 20000;
		else if (uA <= 540000)
			reg_val = 0x17 + (uA -270000) / 30000;
		else if (uA <= 1500000)
			reg_val = 0x20 + (uA -540000) / 60000;
		else if (uA <= 2940000)
			reg_val = 0x30 + (uA -1500000) / 120000;
		else
			reg_val = 0x3d;
	} else {
		reg_val = uA / SGM4154x_ICHRG_I_STEP_uA;
	}

	ret = mmi_sgm4154x_update_bits(sgm, SGM4154x_CHRG_CTRL_2,
				  SGM4154x_ICHRG_I_MASK, reg_val);

	return ret;
}
/*
static int sgm4154x_set_chrg_volt(struct charger_device *chg_dev, unsigned int chrg_volt)
{
	int ret;
	u8 reg_val;
	struct sgm4154x_device *sgm = charger_get_data(chg_dev);
	pr_info("%s set VREG = %d\n", __func__, chrg_volt);
	if (chrg_volt < SGM4154x_VREG_V_MIN_uV)
		chrg_volt = SGM4154x_VREG_V_MIN_uV;
	else if (chrg_volt > sgm->init_data.max_vreg)
		chrg_volt = sgm->init_data.max_vreg;


	reg_val = (chrg_volt-SGM4154x_VREG_V_MIN_uV) / SGM4154x_VREG_V_STEP_uV;
	reg_val = reg_val << 3;
	ret = mmi_sgm4154x_update_bits(sgm, SGM4154x_CHRG_CTRL_4,
				  SGM4154x_VREG_V_MASK, reg_val);

	return ret;
}
*/
// fine tuning termination voltage,to Improve accuracy
static int sgm4154x_vreg_fine_tuning(struct sgm4154x_device *sgm, enum SGM4154x_VREG_FT ft)
{
	int ret;
	int reg_val;

	switch(ft) {
	case VREG_FT_DISABLE:
		reg_val = 0;
		break;

	case VREG_FT_UP_8mV:
		reg_val = SGM4154x_VREG_FT_UP_8mV;
		break;

	case VREG_FT_DN_8mV:
		reg_val = SGM4154x_VREG_FT_DN_8mV;
		break;

	case VREG_FT_DN_16mV:
		reg_val = SGM4154x_VREG_FT_DN_16mV;
		break;

	default:
		reg_val = 0;
		break;
	}
	ret = mmi_sgm4154x_update_bits(sgm, SGM4154x_CHRG_CTRL_f,
				  SGM4154x_VREG_FT_MASK, reg_val);
	pr_info("%s reg_val:%d\n",__func__,reg_val);

	return ret;
}

static int sgm4154x_set_chrg_volt(struct charger_device *chg_dev, unsigned int chrg_volt)
{
	int ret;
	int reg_val;
	enum SGM4154x_VREG_FT ft = VREG_FT_DISABLE;
	struct sgm4154x_device *sgm = charger_get_data(chg_dev);

	if (chrg_volt < SGM4154x_VREG_V_MIN_uV)
		chrg_volt = SGM4154x_VREG_V_MIN_uV;
	else if (chrg_volt > sgm->init_data.max_vreg)
		chrg_volt = sgm->init_data.max_vreg;
	pr_info("%s chrg_volt = %d\n",__func__, chrg_volt);

	reg_val = (chrg_volt-SGM4154x_VREG_V_MIN_uV) / SGM4154x_VREG_V_STEP_uV;

	switch(chrg_volt) {
	case 4512000:
	case 4480000:
	case 4450000:
		reg_val++;
		ft = VREG_FT_DN_16mV;
		break;
	case 4200000:
		reg_val++;
		ft = VREG_FT_DN_8mV;
		break;
	case 4504000:
		ft = VREG_FT_UP_8mV;
		break;
	default:
		break;
	}

	ret = sgm4154x_vreg_fine_tuning(sgm, ft);
	if (ret) {
		pr_err("%s can't set vreg fine tunning ret=%d\n", __func__, ret);
		return ret;
	}

	reg_val = reg_val<<3;
	ret = mmi_sgm4154x_update_bits(sgm, SGM4154x_CHRG_CTRL_4,
				  SGM4154x_VREG_V_MASK, reg_val);

	return ret;
}

static int sgm4154x_get_chrg_volt(struct charger_device *chg_dev,unsigned int *volt)
{
	int ret;
	u8 vreg_val;
	struct sgm4154x_device *sgm = charger_get_data(chg_dev);

	ret = mmi_sgm4154x_read_reg(sgm, SGM4154x_CHRG_CTRL_4, &vreg_val);
	if (ret)
		return ret;

	vreg_val = (vreg_val & SGM4154x_VREG_V_MASK) >> 3;

	if (15 == vreg_val)
		*volt = 4352000; //default
	else if (vreg_val < 25)
		*volt = vreg_val*SGM4154x_VREG_V_STEP_uV + SGM4154x_VREG_V_MIN_uV;
	pr_info("%s get VREG = %d\n", __func__, *volt);

	return 0;
}

static int sgm4154x_get_min_ichg(struct charger_device *chg_dev, u32 *curr)
{
	*curr = 60 * 1000;

	return 0;
}

void sgm4154x_rerun_apsd(struct sgm4154x_device * sgm)
{
	int rc;

	dev_info(sgm->dev, "re-running APSD\n");

	rc = mmi_sgm4154x_update_bits(sgm, SGM4154x_CHRG_CTRL_7, SGM4154x_IINDET_EN_MASK,
                     SGM4154x_IINDET_EN);
	if (rc < 0)
		dev_err(sgm->dev, "Couldn't re-run APSD rc=%d\n", rc);

	return;
}

static bool sgm4154x_is_rerun_apsd_done(struct sgm4154x_device * sgm)
{
        int rc = 0;
        u8 val = 0;
        bool result = false;
	rc = mmi_sgm4154x_read_reg(sgm, SGM4154x_CHRG_CTRL_7, &val);
        if (rc)
                return false;

        result = !(val & SGM4154x_IINDET_EN_MASK);
        pr_info("%s:rerun apsd %s", __func__, result ? "done" : "not complete");
        return result;
}

#if 0
static int sgm4154x_get_vindpm_offset_os(struct sgm4154x_device *sgm)
{
	int ret;
	u8 reg_val;

	ret = mmi_sgm4154x_read_reg(sgm, SGM4154x_CHRG_CTRL_f, &reg_val);
	if (ret)
		return ret;

	reg_val = reg_val & SGM4154x_VINDPM_OS_MASK;

	return reg_val;
}
#endif
static int sgm4154x_set_vindpm_offset_os(struct sgm4154x_device *sgm,u8 offset_os)
{
	int ret;

	ret = mmi_sgm4154x_update_bits(sgm, SGM4154x_CHRG_CTRL_f,
				  SGM4154x_VINDPM_OS_MASK, offset_os);

	if (ret){
		pr_err("%s fail\n",__func__);
		return ret;
	}

	return ret;
}
static int sgm4154x_set_input_volt_lim(struct charger_device *chg_dev, unsigned int vindpm)
{
	int ret;
	unsigned int offset;
	u8 reg_val;
	u8 os_val;
	struct sgm4154x_device *sgm = charger_get_data(chg_dev);
	
	if (vindpm < SGM4154x_VINDPM_V_MIN_uV ||
	    vindpm > SGM4154x_VINDPM_V_MAX_uV)
 		return -EINVAL;	
	
	if (vindpm < 5900000){
		os_val = 0;
		offset = 3900000;
	}		
	else if (vindpm >= 5900000 && vindpm < 7500000){
		os_val = 1;
		offset = 5900000; //uv
	}		
	else if (vindpm >= 7500000 && vindpm < 10500000){
		os_val = 2;
		offset = 7500000; //uv
	}
	else{
		os_val = 3;
		offset = 10500000; //uv
	}

	sgm4154x_set_vindpm_offset_os(sgm,os_val);
	reg_val = (vindpm - offset) / SGM4154x_VINDPM_STEP_uV;

	ret = mmi_sgm4154x_update_bits(sgm, SGM4154x_CHRG_CTRL_6,
				  SGM4154x_VINDPM_V_MASK, reg_val);

	return ret;
}
#if 0
static int sgm4154x_get_input_volt_lim(struct sgm4154x_device *sgm)
{
	int ret;
	int offset;
	u8 vlim;
	int temp;

	ret = mmi_sgm4154x_read_reg(sgm, SGM4154x_CHRG_CTRL_6, &vlim);
	if (ret)
		return ret;

	temp = sgm4154x_get_vindpm_offset_os(sgm);
	if (0 == temp)
		offset = 3900000; //uv
	else if (1 == temp)
		offset = 5900000;
	else if (2 == temp)
		offset = 7500000;
	else if (3 == temp)
		offset = 10500000;
	
	temp = offset + (vlim & 0x0F) * SGM4154x_VINDPM_STEP_uV;
	return temp;
}
#endif

static int sgm4154x_set_input_curr_lim(struct charger_device *chg_dev, unsigned int iindpm)
{
	int ret;
	u8 reg_val;
	struct sgm4154x_device *sgm = charger_get_data(chg_dev);

	pr_info("%s set input curr = %d\n", __func__, iindpm);

	if (iindpm < SGM4154x_IINDPM_I_MIN_uA)
		reg_val = 0;
	else if (iindpm >= SGM4154x_IINDPM_I_MAX_uA)
		reg_val = 0x1F;
	if(sgm->dev_id == SGM41513_ID || sgm->dev_id == SGM41513A_ID ||  sgm->dev_id == SGM41513D_ID) {
		reg_val = (iindpm-SGM4154x_IINDPM_I_MIN_uA) / SGM4154x_IINDPM_STEP_uA;
	} else {
		if (iindpm >= SGM4154x_IINDPM_I_MIN_uA && iindpm <= 3100000)//default
			reg_val = (iindpm-SGM4154x_IINDPM_I_MIN_uA) / SGM4154x_IINDPM_STEP_uA;
		else if (iindpm > 3100000 && iindpm < SGM4154x_IINDPM_I_MAX_uA)
			reg_val = 0x1E;
		else
			reg_val = SGM4154x_IINDPM_I_MASK;
	}
	ret = mmi_sgm4154x_update_bits(sgm, SGM4154x_CHRG_CTRL_0,
				  SGM4154x_IINDPM_I_MASK, reg_val);
	return ret;
}

static int sgm4154x_get_input_curr_lim(struct charger_device *chg_dev,unsigned int *ilim)
{
	int ret;
	u8 reg_val;
	struct sgm4154x_device *sgm = charger_get_data(chg_dev);

	ret = mmi_sgm4154x_read_reg(sgm, SGM4154x_CHRG_CTRL_0, &reg_val);
	if (ret)
		return ret;
	if (SGM4154x_IINDPM_I_MASK == (reg_val & SGM4154x_IINDPM_I_MASK))
		*ilim =  SGM4154x_IINDPM_I_MAX_uA;
	else
		*ilim = (reg_val & SGM4154x_IINDPM_I_MASK)*SGM4154x_IINDPM_STEP_uA + SGM4154x_IINDPM_I_MIN_uA;

	return 0;
}

// Must enter 3.0 mode to call ,otherwise cannot step correctly.
static int sgm4154x_qc30_step_up_vbus(struct sgm4154x_device *sgm)
{
	int ret,i;
	int dp_val;

	for(i=0;i<2;i++){
		/*  dm 3.3v to dm 0.6v  step up 200mV when IC is QC3.0 mode*/
		dp_val = SGM4154x_DP_VSEL_MASK;
		ret = mmi_sgm4154x_update_bits(sgm, SGM4154x_CHRG_CTRL_d,
					  SGM4154x_DP_VSEL_MASK, dp_val); //dp 3.3v
		if (ret)
			return ret;

		udelay(2500);
		dp_val = 0x2<<3;
		ret = mmi_sgm4154x_update_bits(sgm, SGM4154x_CHRG_CTRL_d,
					  SGM4154x_DP_VSEL_MASK, dp_val); //dp 0.6v
		if (ret)
			return ret;

		udelay(2500);
	}
	return ret;
}
// Must enter 3.0 mode to call ,otherwise cannot step correctly.
static int sgm4154x_qc30_step_down_vbus(struct sgm4154x_device *sgm)
{
	int ret,i;
	int dm_val;
	for(i=0;i<2;i++){
		/* dp 0.6v and dm 0.6v step down 200mV when IC is QC3.0 mode*/
		dm_val = 0x2<<1;
		ret = mmi_sgm4154x_update_bits(sgm, SGM4154x_CHRG_CTRL_d,
					  SGM4154x_DM_VSEL_MASK, dm_val); //dm 0.6V
		if (ret)
			return ret;

		udelay(2500);
		dm_val = SGM4154x_DM_VSEL_MASK;
		ret = mmi_sgm4154x_update_bits(sgm, SGM4154x_CHRG_CTRL_d,
					  SGM4154x_DM_VSEL_MASK, dm_val); //dm 3.3v
		udelay(2500);
	}
	return ret;
}
static int sgm4154x_set_dp_dm(struct sgm4154x_device *sgm, int val)
{
	int rc = 0;

	switch(val) {
	case SGM_POWER_SUPPLY_DP_DM_DP_PULSE:
		sgm->pulse_cnt++;
		rc = sgm4154x_qc30_step_up_vbus(sgm);
		if (rc) {
			pr_err("Couldn't increase pulse count rc=%d\n",rc);
			sgm->pulse_cnt--;
		}
		pr_info("DP_DM_DP_PULSE rc=%d cnt=%d\n",rc, sgm->pulse_cnt);
		break;
	case SGM_POWER_SUPPLY_DP_DM_DM_PULSE:
		rc = sgm4154x_qc30_step_down_vbus(sgm);
		if (!rc && sgm->pulse_cnt)
			sgm->pulse_cnt--;
		pr_err("DP_DM_DM_PULSE rc=%d cnt=%d\n",rc, sgm->pulse_cnt);
		break;
	default:
		break;
	}

	return rc;
}
#define HVDCP_POWER_MIN			15000
#define HVDCP_VOLTAGE_BASIC		5000
#define HVDCP_VOLTAGE_NOM		(HVDCP_VOLTAGE_BASIC - 200)
#define HVDCP_VOLTAGE_MAX		(HVDCP_VOLTAGE_BASIC + 200)
#define HVDCP_VOLTAGE_MIN		4000
#define HVDCP_PULSE_COUNT_MAX 	((HVDCP_VOLTAGE_BASIC - 5000) / 200 + 1)
int sgm_config_qc_charger(struct charger_device *chg_dev)
{
	int rc = 0;
	int vbus_mv;
	struct sgm4154x_device *sgm = charger_get_data(chg_dev);

#ifdef CONFIG_MOTO_CHG_WT6670F_SUPPORT
	if(is_already_probe_ok == 1){
		vbus_mv = wt6670f_get_vbus_voltage();
	}else{
		vbus_mv = -1;
		pr_err("wt6670 get vbus failed\n");
	}
#else
	vbus_mv = 5000;
#endif
	if(vbus_mv < 4000 || vbus_mv > 6000){
		pr_err("vbus is not for qc3.o\n");
		return -1;
	}

	pr_info("pulse_cnt=%d, vbus_mv=%d\n", sgm->pulse_cnt, vbus_mv);
	if (vbus_mv < HVDCP_VOLTAGE_NOM && sgm->pulse_cnt < HVDCP_PULSE_COUNT_MAX)
		rc=sgm4154x_set_dp_dm(sgm, SGM_POWER_SUPPLY_DP_DM_DP_PULSE);
	else if (vbus_mv > HVDCP_VOLTAGE_MAX && sgm->pulse_cnt > 0 )
		rc=sgm4154x_set_dp_dm(sgm, SGM_POWER_SUPPLY_DP_DM_DM_PULSE);
	else {
		pr_info("QC3.0 output configure completed\n");
		//rc = 0;
	}
	msleep(100);
	return rc;
}
EXPORT_SYMBOL_GPL(sgm_config_qc_charger);

#ifdef CONFIG_MOTO_CHG_WT6670F_SUPPORT
void wt6670f_get_charger_type_func_work(struct work_struct *work)
{
	struct delayed_work *psy_dwork = NULL;
	struct sgm4154x_device *sgm;
	bool early_notified = false;
	bool should_notify = false;
	bool need_retry = false;
	int early_chg_type = 0;
	int wait_count = 0;
	int count = 0;
        u8 chrg_stat;
        struct sgm4154x_state state;
        int ret;

        union power_supply_propval propval = {.intval = 0};
     	psy_dwork = container_of(work, struct delayed_work, work);
	if(psy_dwork == NULL) {
		pr_err("Cann't get charge_monitor_work\n");
		return ;
	}
	sgm = container_of(psy_dwork, struct sgm4154x_device, psy_dwork);
	if(sgm == NULL) {
		pr_err("Cann't get sgm4154x_device\n");
		return ;
	}

        mutex_lock(&sgm->lock);
        state = sgm->state;
        mutex_unlock(&sgm->lock);

	wt6670f_is_detect = true;
	sgm4154x_set_input_curr_lim(sgm->chg_dev, 500000);
	Charger_Detect_Init();
	if(0 == g_qc3p_id){//wt6670f
		do{
			m_chg_ready = false;
			m_chg_type = 0;
			early_notified = false;
			should_notify = false;
			need_retry = false;
			early_chg_type = 0;
			wt6670f_start_detection();
			while((!m_chg_ready)&&(count<100)){
				msleep(30);
				count++;
				m_chg_ready = wt6670f_is_charger_ready();

				if(!early_notified){
				      early_chg_type = wt6670f_get_protocol();
				}
	
				if(early_chg_type == 0x08 || early_chg_type == 0x09){
					pr_err("[%s] WT6670F early type is QC3+: %d, skip detecting\n",__func__, early_chg_type);
					break;
				}
               			pr_err("wt6670f waiting early type: 0x%x, detect ready: 0x%x, count: %d\n", early_chg_type, m_chg_ready, count);

       			}
      			m_chg_type = wt6670f_get_protocol();

			if(m_chg_type == 0x7 && !need_retry){
				need_retry = true;
			} else {
				need_retry = false;
			}

        		pr_err("[%s] WT6670F charge type is  0x%x\n",__func__, m_chg_type);
		}while(need_retry);
	}//wt6670f
	if(1 == g_qc3p_id){//z350
		if(qc3p_z350_init_ok) {
			qc3p_z350_init_ok =false;
			wt6670f_do_reset();
		}
		m_chg_type = 0;
		wait_count = 0;
		early_chg_type = 0;
		while((!m_chg_type)&&(wait_count<30)){
			msleep(30);
			wait_count++;
			pr_err("z350 early waiting dcp type:%x,%d\n",m_chg_type,wait_count);
		}
		m_chg_type = wt6670f_get_protocol();
		if((m_chg_type != 0x02)&&(m_chg_type != 0x03))
		{
			if (!sgm->psy)
				sgm->psy = power_supply_get_by_name("charger");

			if(sgm->psy){
				propval.intval = SGM4154x_USB_DCP;//STANDARD_CHARGER;
				power_supply_set_property(sgm->psy,POWER_SUPPLY_PROP_CHARGE_TYPE,&propval);
			}
			wait_count = 0;
			while((!m_chg_type)&&(wait_count<30)){
				msleep(100);
				wait_count++;
				pr_err("z350 waiting dcp type:%x,%d\n",m_chg_type,wait_count);
			}
			m_chg_type = wt6670f_get_protocol();
		}
		if(m_chg_type == 0x04){
			pr_err("z350==0x04 retry type");
			msleep(2000);
			m_chg_type = wt6670f_get_protocol();
			pr_err("z350==0x04 retry type:%x,%d\n",m_chg_type,wait_count);
		}
		if(m_chg_type == 0x10){
			wt6670f_en_hvdcp();

			wait_count = 0;
			while((m_chg_type != 0xff)&&(wait_count<30)){
				msleep(100);
				wait_count++;
				early_chg_type = wt6670f_get_protocol();
				if(early_chg_type == 0x06 || early_chg_type == 0x09){
					pr_err("[%s] z350 early type is QC3+/QC3: %d, skip detecting\n",__func__, early_chg_type);
					break;
				}
			}
			m_chg_type = wt6670f_get_protocol();
		}
        	pr_err("[%s] z350 charge type is  0x%x\n",__func__, m_chg_type);
	}
	wt6670f_is_detect = false;
	Charger_Detect_Release();
	if(m_chg_type == 0x05){
		wt6670f_force_qc2_5V();
		pr_err("Force set qc2 5V");
		msleep(100);
	}else if(m_chg_type == 0x06){
		wt6670f_force_qc3_5V();
		msleep(100);
		sgm->pulse_cnt = 0;
		pr_err("Force set qc3 5V");
	}
	sgm4154x_set_input_curr_lim(sgm->chg_dev, 2000000);

        ret = mmi_sgm4154x_read_reg(sgm, SGM4154x_CHRG_STAT, &chrg_stat);
        if (ret){
                pr_err("%s read SGM4154x_CHRG_STAT faild\n",__func__);
        }
        state.online = !!(chrg_stat & SGM4154x_PG_STAT);
        if (state.online){
	        power_supply_changed(sgm->charger);
                pr_err("%s new online=%d,socket online done\n",__func__, state.online);
        }
}
#endif
static int sgm4154x_get_state(struct sgm4154x_device *sgm,
			     struct sgm4154x_state *state)
{
	u8 chrg_stat;
	u8 fault;
	u8 chrg_param_0,chrg_param_1,chrg_param_2;
	int ret;

	msleep(1); /*for  BC12*/
	ret = mmi_sgm4154x_read_reg(sgm, SGM4154x_CHRG_STAT, &chrg_stat);
	if (ret){
		ret = mmi_sgm4154x_read_reg(sgm, SGM4154x_CHRG_STAT, &chrg_stat);
		if (ret){
			pr_err("%s read SGM4154x_CHRG_STAT fail\n",__func__);
			return ret;
		}
	}

	state->chrg_type = chrg_stat & SGM4154x_VBUS_STAT_MASK;
	state->chrg_stat = chrg_stat & SGM4154x_CHG_STAT_MASK;
	state->online = !!(chrg_stat & SGM4154x_PG_STAT);
	state->therm_stat = !!(chrg_stat & SGM4154x_THERM_STAT);
	state->vsys_stat = !!(chrg_stat & SGM4154x_VSYS_STAT);

	pr_err("%s chrg_type =%d,chrg_stat =%d online = %d\n",__func__,state->chrg_type,state->chrg_stat,state->online);


	ret = mmi_sgm4154x_read_reg(sgm, SGM4154x_CHRG_FAULT, &fault);
	if (ret){
		pr_err("%s read SGM4154x_CHRG_FAULT fail\n",__func__);
		return ret;
	}
	state->chrg_fault = fault;
	state->ntc_fault = fault & SGM4154x_TEMP_MASK;
	state->health = state->ntc_fault;
	ret = mmi_sgm4154x_read_reg(sgm, SGM4154x_CHRG_CTRL_0, &chrg_param_0);
	if (ret){
		pr_err("%s read SGM4154x_CHRG_CTRL_0 fail\n",__func__);
		return ret;
	}
	state->hiz_en = !!(chrg_param_0 & SGM4154x_HIZ_EN);

	ret = mmi_sgm4154x_read_reg(sgm, SGM4154x_CHRG_CTRL_5, &chrg_param_1);
	if (ret){
		pr_err("%s read SGM4154x_CHRG_CTRL_5 fail\n",__func__);
		return ret;
	}
	state->term_en = !!(chrg_param_1 & SGM4154x_TERM_EN);

	ret = mmi_sgm4154x_read_reg(sgm, SGM4154x_CHRG_CTRL_a, &chrg_param_2);
	if (ret){
		pr_err("%s read SGM4154x_CHRG_CTRL_a fail\n",__func__);
		return ret;
	}

	state->vbus_gd = !!(chrg_param_2 & SGM4154x_VBUS_GOOD);

	return 0;
}

static int sgm4154x_set_hiz_en(struct charger_device *chg_dev, bool hiz_en)
{
	u8 reg_val;
	struct sgm4154x_device *sgm = charger_get_data(chg_dev);

	dev_notice(sgm->dev, "%s:%d", __func__, hiz_en);
	reg_val = hiz_en ? SGM4154x_HIZ_EN : 0;

	return mmi_sgm4154x_update_bits(sgm, SGM4154x_CHRG_CTRL_0,
				  SGM4154x_HIZ_EN, reg_val);
}

static int sgm4154x_enable_power_path(struct charger_device *chg_dev, bool enable)
{
	int ret = 0;

	ret = sgm4154x_set_hiz_en(chg_dev, !enable);

	pr_err("charger %s enable_vbus %s\n", enable ? "enable" : "disable",
	       !ret ? "successfully" : "failed");

	return ret;
}
#if 0
static int sgm4154x_get_vbus(struct charger_device *chg_dev, u32 *vbus)
{
	struct sgm4154x_device *sgm = charger_get_data(chg_dev);

	*vbus = (u32)wt6670f_get_vbus_voltage();
//	*vbus = 5000;
	pr_err("VBUS = %d\n", *vbus);
	sgm->usb_voltage = *vbus;

	return 0;
}
#endif
static int sgm4154x_enable_charger(struct sgm4154x_device *sgm)
{
    int ret;

    ret = mmi_sgm4154x_update_bits(sgm, SGM4154x_CHRG_CTRL_1, SGM4154x_CHRG_EN,
                     SGM4154x_CHRG_EN);

    return ret;
}

static int sgm4154x_disable_charger(struct sgm4154x_device *sgm)
{
    int ret;

    ret = mmi_sgm4154x_update_bits(sgm, SGM4154x_CHRG_CTRL_1, SGM4154x_CHRG_EN,
                     0);
    return ret;
}

static int sgm4154x_charging_switch(struct charger_device *chg_dev,bool enable)
{
	int ret;
	u8 val;
	struct sgm4154x_device *sgm = charger_get_data(chg_dev);

	if (enable){
#ifdef CONFIG_MOTO_CHG_WT6670F_SUPPORT
		sgm->charging_enabled = true;
#endif
		ret = sgm4154x_enable_charger(sgm);
	}else{
#ifdef CONFIG_MOTO_CHG_WT6670F_SUPPORT
		sgm->charging_enabled = false;
#endif
		ret = sgm4154x_disable_charger(sgm);
#ifdef CONFIG_MOTO_CHG_WT6670F_SUPPORT
		if(m_chg_type != 0 && is_already_probe_ok != 0){
			wt6670f_reset_chg_type();
			m_chg_type = 0;
			sgm->pulse_cnt = 0;
		}
#endif
	}

	pr_err("%s charger %s\n", enable ? "enable" : "disable",
	       !ret ? "successfully" : "failed");
	ret = mmi_sgm4154x_read_reg(sgm, SGM4154x_CHRG_CTRL_1, &val);

	if (!ret) {
		val = !!(val & SGM4154x_CHRG_EN);
		if (sgm->charge_enabled != val) {
			sgm->charge_enabled = val;
			power_supply_changed(sgm->charger);
		}
	}
	return ret;
}

static int sgm4154x_plug_in(struct charger_device *chg_dev)
{

	int ret;

	ret = sgm4154x_charging_switch(chg_dev, true);

	if (ret)
		pr_err("Failed to enable charging:%d\n", ret);

	return ret;
}

static int sgm4154x_plug_out(struct charger_device *chg_dev)
{
	int ret;

	ret = sgm4154x_charging_switch(chg_dev, false);

	if (ret)
		pr_err("Failed to disable charging:%d\n", ret);


	return ret;
}

static int sgm4154x_enable_termination(struct charger_device *chg_dev, bool enable)
{
	struct sgm4154x_device *sgm = charger_get_data(chg_dev);
       int ret = 0;

       ret = mmi_sgm4154x_update_bits(sgm, SGM4154x_CHRG_CTRL_5, SGM4154x_EN_TERM_MASK,
                     enable  ? SGM4154x_EN_TERM_ENABLE : SGM4154x_EN_TERM_DISABLE );

	pr_info("%s, %s enable term %s\n", __func__,
		enable ? "enable" : "disable",
		ret ? "failed" : "success");

       return ret;
}

int sgm4154x_extern_enable_termination(bool enable)
{
       int ret = 0;

       ret = sgm4154x_enable_termination(s_chg_dev_otg, enable);

       return ret;
}
EXPORT_SYMBOL_GPL(sgm4154x_extern_enable_termination);

static int sgm4154x_is_charging_enable(struct charger_device *chg_dev, bool *en)
{
	struct sgm4154x_device *sgm = charger_get_data(chg_dev);

	*en = sgm->charge_enabled;

	return 0;
}

static int sgm4154x_set_recharge_volt(struct sgm4154x_device *sgm, int mV)
{
	u8 reg_val;
	
	reg_val = (mV - SGM4154x_VRECHRG_OFFSET_mV) / SGM4154x_VRECHRG_STEP_mV;

	return mmi_sgm4154x_update_bits(sgm, SGM4154x_CHRG_CTRL_4,
				  SGM4154x_VRECHARGE, reg_val);
}

static int sgm4154x_set_wdt_rst(struct sgm4154x_device *sgm, bool is_rst)
{
	u8 val;
	
	if (is_rst)
		val = SGM4154x_WDT_RST_MASK;
	else
		val = 0;
	return mmi_sgm4154x_update_bits(sgm, SGM4154x_CHRG_CTRL_1,
				  SGM4154x_WDT_RST_MASK, val);	
}

/**********************************************************
 *
 *   [Internal Function]
 *
 *********************************************************/
static int sgm4154x_dump_register(struct charger_device *chg_dev)
{

	unsigned char i = 0;
	unsigned int ret = 0;
	unsigned char sgm4154x_reg[SGM4154x_REG_NUM+1] = { 0 };
	struct sgm4154x_device *sgm = charger_get_data(chg_dev);
		
	for (i = 0; i < SGM4154x_REG_NUM+1; i++) {
		ret = mmi_sgm4154x_read_reg(sgm,i, &sgm4154x_reg[i]);
		if (ret) {
			pr_info("[sgm4154x] i2c transfor error\n");
			return 1;
		}
		pr_info("%s,[0x%x]=0x%x ",__func__, i, sgm4154x_reg[i]);
	}
	
	return 0;
}


/**********************************************************
 *
 *   [Internal Function]
 *
 *********************************************************/
static int sgm4154x_hw_chipid_detect(struct sgm4154x_device *sgm)
{
	int ret = 0;
	u8 val = 0;
	ret = mmi_sgm4154x_read_reg(sgm,SGM4154x_CHRG_CTRL_b,&val);
	if (ret < 0)
	{
		pr_info("[%s] read SGM4154x_CHRG_CTRL_b fail\n", __func__);
		return ret;
	}		
	val = val & SGM4154x_PN_MASK;
	pr_info("[%s] Reg[0x0B]=0x%x\n", __func__,val);
	
	return val;
}

static int sgm4154x_reset_watch_dog_timer(struct charger_device
		*chg_dev)
{
	int ret;
	struct sgm4154x_device *sgm = charger_get_data(chg_dev);

	pr_info("charging_reset_watch_dog_timer\n");

	ret = sgm4154x_set_wdt_rst(sgm,0x1);	/* RST watchdog */	

	return ret;
}


static int sgm4154x_get_charging_status_done(struct charger_device *chg_dev,
				       bool *is_done)
{
	//struct sgm4154x_state state;
	struct sgm4154x_device *sgm = charger_get_data(chg_dev);
	//sgm4154x_get_state(sgm, &state);

	if (sgm->state.chrg_stat == SGM4154x_TERM_CHRG)
		*is_done = true;
	else
		*is_done = false;

	return 0;
}

static int sgm4154x_set_en_timer(struct sgm4154x_device *sgm)
{
	int ret;	

	ret = mmi_sgm4154x_update_bits(sgm, SGM4154x_CHRG_CTRL_5,
				SGM4154x_SAFETY_TIMER_EN, SGM4154x_SAFETY_TIMER_EN);

	return ret;
}

static int sgm4154x_set_disable_timer(struct sgm4154x_device *sgm)
{
	int ret;	

	ret = mmi_sgm4154x_update_bits(sgm, SGM4154x_CHRG_CTRL_5,
				SGM4154x_SAFETY_TIMER_EN, 0);

	return ret;
}

static int sgm4154x_enable_safetytimer(struct charger_device *chg_dev,bool en)
{
	struct sgm4154x_device *sgm = charger_get_data(chg_dev);
	int ret = 0;

	if (en)
		ret = sgm4154x_set_en_timer(sgm);
	else
		ret = sgm4154x_set_disable_timer(sgm);
	return ret;
}

static int sgm4154x_get_is_safetytimer_enable(struct charger_device
		*chg_dev,bool *en)
{
	int ret = 0;
	u8 val = 0;
	
	struct sgm4154x_device *sgm = charger_get_data(chg_dev);
	
	ret = mmi_sgm4154x_read_reg(sgm,SGM4154x_CHRG_CTRL_5,&val);
	if (ret < 0)
	{
		pr_info("[%s] read SGM4154x_CHRG_CTRL_5 fail\n", __func__);
		return ret;
	}
	*en = !!(val & SGM4154x_SAFETY_TIMER_EN);
	return 0;
}

static int sgm4154x_en_pe_current_partern(struct charger_device
		*chg_dev,bool is_up)
{
	int ret = 0;	
	
	struct sgm4154x_device *sgm = charger_get_data(chg_dev);
	
	if(sgm->dev_id == SGM41542_ID || sgm->dev_id == SGM41543D_ID || sgm->dev_id == SGM41516D_ID) {
		ret = mmi_sgm4154x_update_bits(sgm, SGM4154x_CHRG_CTRL_d,
					SGM4154x_EN_PUMPX, SGM4154x_EN_PUMPX);
		if (ret < 0)
		{
			pr_info("[%s] read SGM4154x_CHRG_CTRL_d fail\n", __func__);
			return ret;
		}
		if (is_up)
			ret = mmi_sgm4154x_update_bits(sgm, SGM4154x_CHRG_CTRL_d,
					SGM4154x_PUMPX_UP, SGM4154x_PUMPX_UP);
		else
			ret = mmi_sgm4154x_update_bits(sgm, SGM4154x_CHRG_CTRL_d,
					SGM4154x_PUMPX_DN, SGM4154x_PUMPX_DN);
	}
	return ret;
}

static u8 sgm4154x_get_charging_status(struct sgm4154x_device *sgm)
{
	int ret = 0;
	u8 chrg_stat;

	ret = mmi_sgm4154x_read_reg(sgm, SGM4154x_CHRG_STAT, &chrg_stat);
	if (ret) {
		pr_err("%s read SGM4154x_CHRG_STAT fail\n",__func__);
		return ret;
	}

	chrg_stat &= SGM4154x_CHG_STAT_MASK;

	return chrg_stat;
}

static enum power_supply_property sgm4154x_power_supply_props[] = {
	POWER_SUPPLY_PROP_MANUFACTURER,
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_TYPE,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CURRENT_MAX,
	POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_USB_TYPE,
	POWER_SUPPLY_PROP_PRESENT
};

static int sgm4154x_property_is_writeable(struct power_supply *psy,
					 enum power_supply_property prop)
{
	switch (prop) {
	case POWER_SUPPLY_PROP_CURRENT_MAX:
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE:
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
	case POWER_SUPPLY_PROP_PRECHARGE_CURRENT:
	case POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT:
		return true;
	default:
		return false;
	}
}
static int sgm4154x_charger_set_property(struct power_supply *psy,
		enum power_supply_property prop,
		const union power_supply_propval *val)
{
	//struct sgm4154x_device *sgm = power_supply_get_drvdata(psy);
	int ret = -EINVAL;

	switch (prop) {
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		ret = sgm4154x_set_input_curr_lim(s_chg_dev_otg, val->intval);
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		ret = sgm4154x_set_ichrg_curr(s_chg_dev_otg, val->intval);
		break;
/*	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		sgm4154x_charging_switch(s_chg_dev_otg,val->intval);		
		break;
	case POWER_SUPPLY_PROP_INPUT_VOLTAGE_LIMIT:
		ret = sgm4154x_set_input_volt_lim(s_chg_dev_otg, val->intval);
		break;*/
	default:
		return -EINVAL;
	}

	return ret;
}

static int sgm4154x_charger_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct sgm4154x_device *sgm = power_supply_get_drvdata(psy);
	struct sgm4154x_state state = sgm->state;
	u8 chrg_status = 0;
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		chrg_status = sgm4154x_get_charging_status(sgm);
		if (!state.chrg_type || (state.chrg_type == SGM4154x_OTG_MODE))
			val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
		else if (!chrg_status)
			val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
		else if (chrg_status == SGM4154x_TERM_CHRG)
			val->intval = POWER_SUPPLY_STATUS_FULL;
		else
			val->intval = POWER_SUPPLY_STATUS_CHARGING;

		if (sgm->mmi_charging_full == true)
			val->intval = POWER_SUPPLY_STATUS_FULL;
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		chrg_status = sgm4154x_get_charging_status(sgm);
		switch (chrg_status) {
		case SGM4154x_PRECHRG:
			val->intval = POWER_SUPPLY_CHARGE_TYPE_TRICKLE;
			break;
		case SGM4154x_FAST_CHRG:
			val->intval = POWER_SUPPLY_CHARGE_TYPE_FAST;
			break;
		case SGM4154x_TERM_CHRG:
			val->intval = POWER_SUPPLY_CHARGE_TYPE_TRICKLE;
			break;
		case SGM4154x_NOT_CHRGING:
			val->intval = POWER_SUPPLY_CHARGE_TYPE_NONE;
			break;
		default:
			val->intval = POWER_SUPPLY_CHARGE_TYPE_UNKNOWN;
		}
		break;

	case POWER_SUPPLY_PROP_USB_TYPE:
		val->intval = sgm->psy_usb_type;
		break;

	case POWER_SUPPLY_PROP_MANUFACTURER:
		val->strval = SGM4154x_MANUFACTURER;
		break;

	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = sgm4154x_chg_props.alias_name;
		break;

	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = state.online;
		if (!state.online)
			sgm->mmi_charging_full = false;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = state.vbus_gd;
		break;
	case POWER_SUPPLY_PROP_TYPE:
		val->intval = sgm4154x_power_supply_desc.type;
		break;	

	case POWER_SUPPLY_PROP_HEALTH:
		if (state.chrg_fault & 0xF8)
			val->intval = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
		else
			val->intval = POWER_SUPPLY_HEALTH_GOOD;

		switch (state.health) {
		case SGM4154x_TEMP_HOT:
			val->intval = POWER_SUPPLY_HEALTH_OVERHEAT;
			break;
		case SGM4154x_TEMP_WARM:
			val->intval = POWER_SUPPLY_HEALTH_GOOD;
			break;
		case SGM4154x_TEMP_COOL:
			val->intval = POWER_SUPPLY_HEALTH_GOOD;
			break;
		case SGM4154x_TEMP_COLD:
			val->intval = POWER_SUPPLY_HEALTH_COLD;
			break;
		}
		break;

	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
#ifdef CONFIG_MOTO_CHG_WT6670F_SUPPORT
		if(is_already_probe_ok == 1){
			val->intval = wt6670f_get_vbus_voltage();
			if(val->intval > 13000){
				wt6670f_do_reset();
				val->intval = wt6670f_get_vbus_voltage();
			}
			if(val->intval > 13000){
				val->intval = 5000;
			}
		}else{
			val->intval = 5000;
			pr_err("wt6670 probe error,force vbus 5000");
		}
#else
		val->intval = 5000;
#endif
		break;

	case POWER_SUPPLY_PROP_CURRENT_NOW:
		//val->intval = state.ibus_adc;
		break;

       case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE:
               sgm4154x_get_chrg_volt(sgm->chg_dev,&val->intval);
               break;

/*	case POWER_SUPPLY_PROP_INPUT_VOLTAGE_LIMIT:
		ret = sgm4154x_get_input_volt_lim(sgm);
		if (ret < 0)
			return ret;

		val->intval = ret;
		break;*/

	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:		
         sgm4154x_get_input_curr_lim(s_chg_dev_otg,&val->intval);
		break;

	case POWER_SUPPLY_PROP_CURRENT_MAX:
		if (sgm->psy_usb_type == POWER_SUPPLY_USB_TYPE_SDP)
			val->intval = 500000;
		else if (sgm->psy_usb_type == POWER_SUPPLY_USB_TYPE_CDP)
			val->intval = 1500000;
		else if (sgm->psy_usb_type == POWER_SUPPLY_USB_TYPE_DCP)
			val->intval = sgm->init_data.max_ichg;
		break;
	default:
		return -EINVAL;
	}

	return ret;
}

#if 0
static bool sgm4154x_state_changed(struct sgm4154x_device *sgm,
				  struct sgm4154x_state *new_state)
{
	struct sgm4154x_state old_state;

	mutex_lock(&sgm->lock);
	old_state = sgm->state;
	mutex_unlock(&sgm->lock);

	return (old_state.chrg_type != new_state->chrg_type ||
		old_state.chrg_stat != new_state->chrg_stat     ||		
		old_state.online != new_state->online		    ||
		old_state.therm_stat != new_state->therm_stat	||		
		old_state.vsys_stat != new_state->vsys_stat 	||
		old_state.chrg_fault != new_state->chrg_fault	
		);
}
#endif

static void sgm4154x_rerun_apsd_work_func(struct work_struct *work)
{
        struct sgm4154x_device * sgm = NULL;
	struct sgm4154x_state state;
        int ret = 0;
        bool apsd_done = false;
        int check_count = 0;

        pr_err("%s --Start---\n",__func__);

        sgm = container_of(work, struct sgm4154x_device, rerun_apsd_work);
        if(sgm == NULL) {
                pr_err("Cann't get sgm4154x_device\n");
                return;
        }

        ret = sgm4154x_get_state(sgm, &state);
        mutex_lock(&sgm->lock);
        sgm->state = state;
        mutex_unlock(&sgm->lock);

	if(!sgm->state.vbus_gd) {
		dev_err(sgm->dev, "Vbus not present, disable charge\n");
                return;
	}

        sgm->typec_apsd_rerun_done = true;

        sgm4154x_rerun_apsd(sgm);

        while(check_count < 10) {
                apsd_done = sgm4154x_is_rerun_apsd_done(sgm);
                if (apsd_done && check_count > 2) {
                        break;
                }

                msleep(100);
                check_count ++;
        }

	schedule_delayed_work(&sgm->charge_detect_delayed_work, 100);
}

static void typec_in_work_func(struct work_struct *work)
{
    struct sgm4154x_device *sgm = NULL;
    struct delayed_work *typec_in_work = NULL;

    typec_in_work = (struct delayed_work *)container_of(work, struct delayed_work, work);
    if(typec_in_work == NULL) {
        pr_err("Cann't get typec in work\n");
        return;
    }

    sgm = (struct sgm4154x_device *)container_of(typec_in_work, struct sgm4154x_device, typec_in_work);
    if(sgm == NULL) {
        pr_err("Cann't get sgm\n");
	return;
    }

    if(charger_irq_normal_flag == false) {
        schedule_delayed_work(&sgm->charge_detect_delayed_work, msecs_to_jiffies(10));
        pr_info("%s USB Plug in charger\n", __func__);
    }
    pr_err("%s:typec in is running\n", __func__);
}

static void typec_out_work_func(struct work_struct *work)
{
    struct sgm4154x_device *sgm = NULL;
    struct delayed_work *typec_out_work = NULL;

    typec_out_work = (struct delayed_work *)container_of(work, struct delayed_work, work);
    if(typec_out_work == NULL) {
        pr_err("Cann't get typec in work\n");
        return;
    }

    sgm = (struct sgm4154x_device *)container_of(typec_out_work, struct sgm4154x_device, typec_out_work);
    if(sgm == NULL) {
        pr_err("Cann't get sgm\n");
        return;
    }

    if(charger_irq_normal_flag == true) {
        schedule_delayed_work(&sgm->charge_detect_delayed_work, msecs_to_jiffies(10));
	pr_info("%s USB Plug out charger\n", __func__);
    }
    pr_err("%s:typec in is running\n", __func__);
}

static int pd_tcp_notifier_call(struct notifier_block *pnb, unsigned long event, void *data)
{
    struct sgm4154x_device *sgm  = NULL;
    struct tcp_notify *noti = data;
    pr_info("%s USB Plug single to charger\n", __func__);

    sgm = (struct sgm4154x_device *)container_of(pnb, struct sgm4154x_device, pd_nb);
    if(sgm == NULL) {
        pr_err("%s, fail to get sgm\n", __func__);
        return NOTIFY_BAD;
    }

    switch(event) {
        case TCP_NOTIFY_TYPEC_STATE:
		if(noti->typec_state.old_state == TYPEC_UNATTACHED &&
				(noti->typec_state.new_state == TYPEC_ATTACHED_SNK)) {
		    schedule_delayed_work(&sgm->typec_in_work, msecs_to_jiffies(500));
		} else if(noti->typec_state.old_state == TYPEC_ATTACHED_SNK &&
				(noti->typec_state.new_state == TYPEC_UNATTACHED)) {
                    schedule_delayed_work(&sgm->typec_out_work, msecs_to_jiffies(500));
		}
	break;
    }
    return NOTIFY_OK;
}

static void charger_monitor_work_func(struct work_struct *work)
{
	int ret = 0;
	struct sgm4154x_device * sgm = NULL;
	struct delayed_work *charge_monitor_work = NULL;
	//static u8 last_chg_method = 0;
	struct sgm4154x_state state;

	pr_err("%s --Start---\n",__func__);
	charge_monitor_work = container_of(work, struct delayed_work, work);
	if(charge_monitor_work == NULL) {
		pr_err("Cann't get charge_monitor_work\n");
		return ;
	}
	sgm = container_of(charge_monitor_work, struct sgm4154x_device, charge_monitor_work);
	if(sgm == NULL) {
		pr_err("Cann't get sgm \n");
		return ;
	}

	if((sgm->typec_support) && (sgm->tcpc_dev == NULL)) {
		sgm->tcpc_dev = tcpc_dev_get_by_name("type_c_port0");
		if(sgm->tcpc_dev) {
			sgm->pd_nb.notifier_call = pd_tcp_notifier_call;
        		ret = register_tcp_dev_notifier(sgm->tcpc_dev, &sgm->pd_nb, TCP_NOTIFY_TYPE_ALL);
        		if(ret < 0) {
				sgm->tcpc_dev = NULL;
            			pr_err("%d: sgm4154x register usb fail\n", __LINE__);
			}
                } else {
			pr_err("%d: sgm4154x tcpc get typec_c_port0 fail\n", __LINE__);
		}
	}

	ret = sgm4154x_get_state(sgm, &state);
	if(ret)
	{
		pr_err("%s--get_state--\n",__func__);
		goto OUT;
	}
	mutex_lock(&sgm->lock);
	sgm->state = state;
	mutex_unlock(&sgm->lock);

	if(!sgm->state.vbus_gd) {
		dev_err(sgm->dev, "Vbus not present, disable charge\n");

#ifdef CONFIG_MOTO_CHG_WT6670F_SUPPORT
		if(m_chg_type != 0 && is_already_probe_ok != 0){
			wt6670f_reset_chg_type();
			m_chg_type = 0;
			sgm->pulse_cnt = 0;
		}
#endif
		goto OUT;
	}
	if(!state.online)
	{
		dev_err(sgm->dev, "Vbus not online\n");
		goto OUT;
	}
	sgm4154x_dump_register(sgm->chg_dev);
	pr_err("%s--End--\n",__func__);
OUT:
	return;
	//schedule_delayed_work(&sgm->charge_monitor_work, 10*HZ);
}

static void charger_detect_work_func(struct work_struct *work)
{
	struct delayed_work *charge_detect_delayed_work = NULL;
	struct sgm4154x_device * sgm = NULL;
	//static int charge_type_old = 0;
	int curr_in_limit = 0;	
	struct sgm4154x_state state;	
	int ret;

	pr_err("%s --do charger Detect--\n",__func__);
	charge_detect_delayed_work = container_of(work, struct delayed_work, work);
	if(charge_detect_delayed_work == NULL) {
		pr_err("Cann't get charge_detect_delayed_work\n");
		return ;
	}
	sgm = container_of(charge_detect_delayed_work, struct sgm4154x_device, charge_detect_delayed_work);
	if(sgm == NULL) {
		pr_err("Cann't get sgm4154x_device\n");
		return ;
	}

	if (!sgm->charger_wakelock->active)
		__pm_stay_awake(sgm->charger_wakelock);

	ret = sgm4154x_get_state(sgm, &state);
	if (ret)
	{
		pr_err("%s--get_state--\n",__func__);
		goto err;
	}
	mutex_lock(&sgm->lock);
	sgm->state = state;
	charger_irq_normal_flag = true;
	mutex_unlock(&sgm->lock);	
	
	if(!sgm->state.vbus_gd) {
		dev_err(sgm->dev, "Vbus not present, disable charge\n");
		sgm4154x_disable_charger(sgm);
		sgm->mmi_qc3p_rerun_done = false;
		sgm->psy_usb_type = POWER_SUPPLY_USB_TYPE_UNKNOWN;
		charger_irq_normal_flag = false;
#ifdef CONFIG_MOTO_CHG_WT6670F_SUPPORT
		if(m_chg_type != 0 && is_already_probe_ok != 0){
			wt6670f_reset_chg_type();
			m_chg_type = 0;
			sgm->pulse_cnt = 0;
		}
#endif
		goto err;
	}
	if(!state.online)
	{
		dev_err(sgm->dev, "Vbus not online\n");		
		goto err;
	}

	if((sgm->psy_usb_type == POWER_SUPPLY_USB_TYPE_DCP) && (sgm->state.chrg_type == SGM4154x_USB_DCP))
	{
		dev_err(sgm->dev, "Has already detected the dcp type, goto end.\n");
		goto err;
	}
	if(sgm->dev_id == SGM41542_ID || sgm->dev_id == SGM41543D_ID || sgm->dev_id == SGM41516D_ID) {
		switch(sgm->state.chrg_type) {
			case SGM4154x_USB_SDP:
				sgm4154x_power_supply_desc.type = POWER_SUPPLY_TYPE_USB;
				sgm->psy_usb_type = POWER_SUPPLY_USB_TYPE_SDP;
				pr_err("SGM4154x charger type: SDP\n");
				curr_in_limit = 500000;
				break;

			case SGM4154x_USB_CDP:
				sgm4154x_power_supply_desc.type = POWER_SUPPLY_TYPE_USB_CDP;
				sgm->psy_usb_type = POWER_SUPPLY_USB_TYPE_CDP;
				pr_err("SGM4154x charger type: CDP\n");
				curr_in_limit = 1500000;
				break;

			case SGM4154x_USB_DCP:
				sgm4154x_power_supply_desc.type = POWER_SUPPLY_TYPE_USB_DCP;
				sgm->psy_usb_type = POWER_SUPPLY_USB_TYPE_DCP;
				pr_err("SGM4154x charger type: DCP\n");
				curr_in_limit = 2000000;
#ifdef CONFIG_MOTO_CHG_WT6670F_SUPPORT
				schedule_delayed_work(&sgm->psy_dwork, 0);
				curr_in_limit = 500000;
#endif
				break;
			case SGM4154x_NON_STANDARD:
			case SGM4154x_UNKNOWN:
				sgm4154x_power_supply_desc.type = POWER_SUPPLY_TYPE_USB;
				sgm->psy_usb_type = POWER_SUPPLY_USB_TYPE_SDP;
				pr_err("SGM4154x charger type: UNKNOWN\n");
				if (!sgm->mmi_qc3p_rerun_done) {
					pr_err("SGM4154x charger type: UNKNOWN and rerun apsd once.\n");
					sgm->mmi_qc3p_rerun_done = true;
					schedule_work(&sgm->rerun_apsd_work);
				}
				curr_in_limit = 500000;
				break;

			default:
				pr_err("SGM4154x charger type: default\n");
				//curr_in_limit = 500000;
				//break;
				return;
		}

		//set charge parameters
		dev_err(sgm->dev, "Update: curr_in_limit = %d\n", curr_in_limit);
		sgm4154x_set_input_curr_lim(sgm->chg_dev, curr_in_limit);
	}
	//enable charge
	sgm4154x_enable_charger(sgm);

err:
	//release wakelock
	pr_err("%s --Notify charged--\n",__func__);
	power_supply_changed(sgm->charger);
	dev_err(sgm->dev, "Relax wakelock\n");
	__pm_relax(sgm->charger_wakelock);
	return;
}

static irqreturn_t sgm4154x_irq_handler_thread(int irq, void *private)
{
	struct sgm4154x_device *sgm = private;

	//lock wakelock
	pr_err("%s entry\n",__func__);
    
	schedule_delayed_work(&sgm->charge_detect_delayed_work, 100);
	//power_supply_changed(sgm->charger);
	
	return IRQ_HANDLED;
}
static char *sgm4154x_charger_supplied_to[] = {
	"battery",
	"mtk-master-charger",
};

static struct power_supply_desc sgm4154x_power_supply_desc = {
	.name = "primary_chg",
	.type = POWER_SUPPLY_TYPE_USB,
	.usb_types = sgm4154x_usb_type,
	.num_usb_types = ARRAY_SIZE(sgm4154x_usb_type),
	.properties = sgm4154x_power_supply_props,
	.num_properties = ARRAY_SIZE(sgm4154x_power_supply_props),
	.get_property = sgm4154x_charger_get_property,
	.set_property = sgm4154x_charger_set_property,
	.property_is_writeable = sgm4154x_property_is_writeable,
};

static int sgm4154x_power_supply_init(struct sgm4154x_device *sgm,
							struct device *dev)
{
	struct power_supply_config psy_cfg = { .drv_data = sgm,
						.of_node = dev->of_node, };

	psy_cfg.supplied_to = sgm4154x_charger_supplied_to;
	psy_cfg.num_supplicants = ARRAY_SIZE(sgm4154x_charger_supplied_to);

	sgm->charger = devm_power_supply_register(sgm->dev,
						 &sgm4154x_power_supply_desc,
						 &psy_cfg);
	if (IS_ERR(sgm->charger))
		return -EINVAL;
	
	return 0;
}

static int sgm4154x_hw_init(struct sgm4154x_device *sgm)
{
	int ret = 0;	
	struct power_supply_battery_info bat_info = { };	

	if(sgm->dev_id == SGM41513_ID || sgm->dev_id == SGM41513D_ID) {
		bat_info.constant_charge_current_max_ua =
				SGM41513_ICHRG_I_DEF_uA;
		sgm->init_data.max_ichg =
				SGM41513_ICHRG_I_MAX_uA;
	} else {
		bat_info.constant_charge_current_max_ua =
				SGM4154x_ICHRG_I_DEF_uA;
		sgm->init_data.max_ichg =
				SGM4154x_ICHRG_I_MAX_uA;
	}
	bat_info.constant_charge_voltage_max_uv =
			SGM4154x_VREG_V_DEF_uV;

	bat_info.precharge_current_ua =
			SGM4154x_PRECHRG_I_DEF_uA;

	bat_info.charge_term_current_ua =
			SGM4154x_TERMCHRG_I_DEF_uA;

	sgm->init_data.max_vreg =
			SGM4154x_VREG_V_MAX_uV;
			
	sgm4154x_set_watchdog_timer(sgm,0);

	ret = sgm4154x_set_ichrg_curr(s_chg_dev_otg,
				bat_info.constant_charge_current_max_ua);
	if (ret)
		goto err_out;

	ret = sgm4154x_set_prechrg_curr(sgm, bat_info.precharge_current_ua);
	if (ret)
		goto err_out;

	ret = sgm4154x_set_chrg_volt(s_chg_dev_otg,
				bat_info.constant_charge_voltage_max_uv);
	if (ret)
		goto err_out;

	ret = sgm4154x_set_term_curr(sgm, bat_info.charge_term_current_ua);
	if (ret)
		goto err_out;

	/*ret = sgm4154x_set_input_volt_lim(sgm, sgm->init_data.vlim);
	if (ret)
		goto err_out;*/

	ret = sgm4154x_set_input_curr_lim(s_chg_dev_otg, sgm->init_data.ilim);
	if (ret)
		goto err_out;
	#if 0
	ret = sgm4154x_set_vac_ovp(sgm);//14V
	if (ret)
		goto err_out;	
	#endif
	ret = sgm4154x_set_recharge_volt(sgm, 200);//100~200mv
	if (ret)
		goto err_out;
	
	dev_notice(sgm->dev, "ichrg_curr:%d prechrg_curr:%d chrg_vol:%d"
		" term_curr:%d input_curr_lim:%d",
		bat_info.constant_charge_current_max_ua,
		bat_info.precharge_current_ua,
		bat_info.constant_charge_voltage_max_uv,
		bat_info.charge_term_current_ua,
		sgm->init_data.ilim);

	/*VINDPM*/
	ret = mmi_sgm4154x_update_bits(sgm, SGM4154x_CHRG_CTRL_a,
				SGM4154x_VINDPM_V_MASK, 0x3);

	/*ts error-- temp disable jeita */
	ret = __sgm4154x_write_byte(sgm, SGM4154x_CHRG_CTRL_d, 0x0);

	/*sys_min set to 3.4V*/
	ret = mmi_sgm4154x_update_bits(sgm, SGM4154x_CHRG_CTRL_1,
				SGM4154x_SYS_MIN_MASK, SGM4154x_SYS_MIN_3400MV);
	return 0;

err_out:
	return ret;

}

static int sgm4154x_parse_dt(struct sgm4154x_device *sgm)
{
	int ret;	

	ret = device_property_read_u32(sgm->dev,
				       "input-voltage-limit-microvolt",
				       &sgm->init_data.vlim);
	if (ret)
		sgm->init_data.vlim = SGM4154x_VINDPM_DEF_uV;

	if (sgm->init_data.vlim > SGM4154x_VINDPM_V_MAX_uV ||
	    sgm->init_data.vlim < SGM4154x_VINDPM_V_MIN_uV)
		return -EINVAL;

	ret = device_property_read_u32(sgm->dev,
				       "input-current-limit-microamp",
				       &sgm->init_data.ilim);
	if (ret)
		sgm->init_data.ilim = SGM4154x_IINDPM_DEF_uA;

	sgm->typec_support = device_property_read_bool(sgm->dev,
				       "typec-support");

	if (sgm->init_data.ilim > SGM4154x_IINDPM_I_MAX_uA ||
	    sgm->init_data.ilim < SGM4154x_IINDPM_I_MIN_uA)
		return -EINVAL;

	return 0;
}

static int sgm4154x_enable_vbus(struct regulator_dev *rdev)
{	
	int ret = 0;
	struct sgm4154x_device *sgm = charger_get_data(s_chg_dev_otg);

	pr_err("%s enable_vbus\n", __func__);
	
	ret = mmi_sgm4154x_update_bits(sgm, SGM4154x_CHRG_CTRL_1, SGM4154x_OTG_EN,
                     SGM4154x_OTG_EN);
	return ret;
}

static int sgm4154x_disable_vbus(struct regulator_dev *rdev)
{
	int ret = 0;
	struct sgm4154x_device *sgm = charger_get_data(s_chg_dev_otg);
	pr_err("%s disable_vbus\n", __func__);

	ret = mmi_sgm4154x_update_bits(sgm, SGM4154x_CHRG_CTRL_1, SGM4154x_OTG_EN,
                     0);

	return ret;
}

static int sgm4154x_is_enabled_vbus(struct regulator_dev *rdev)
{
	u8 temp = 0;
	int ret = 0;
	struct sgm4154x_device *sgm = charger_get_data(s_chg_dev_otg);
	pr_err("%s -----\n", __func__);

	ret = mmi_sgm4154x_read_reg(sgm, SGM4154x_CHRG_CTRL_1, &temp);
	return (temp&SGM4154x_OTG_EN)? 1 : 0;
}

static int sgm4154x_enable_otg(struct charger_device *chg_dev, bool en)
{
	int ret = 0;

	pr_info("%s en = %d\n", __func__, en);
	if (en) {
		ret = sgm4154x_set_hiz_en(chg_dev, !en);
		ret = sgm4154x_enable_vbus(NULL);
	} else {
		ret = sgm4154x_disable_vbus(NULL);
	}
	return ret;
}

#if 0
static int sgm4154x_set_boost_voltage_limit(struct regulator_dev *rdev, unsigned int uV)
{	
	int ret = 0;
	char reg_val = -1;
	int i = 0;
	struct sgm4154x_device *sgm = charger_get_data(s_chg_dev_otg);
	
	while(i<4){
		if (uV == BOOST_VOLT_LIMIT[i]){
			reg_val = i;
			break;
		}
		i++;
	}
	if (reg_val < 0)
		return reg_val;
	reg_val = reg_val << 4;
	ret = mmi_sgm4154x_update_bits(sgm, SGM4154x_CHRG_CTRL_6,
				  SGM4154x_BOOSTV, reg_val);

	return ret;
}

static int sgm4154x_boost_get_voltage_sel(struct regulator_dev *rdev)
{
	int ret;
	u8 vlim;
	struct sgm4154x_device *sgm = charger_get_data(s_chg_dev_otg);

	ret = mmi_sgm4154x_read_reg(sgm, SGM4154x_CHRG_CTRL_6, &vlim);
	if (ret)
		return ret;

	return (vlim & SGM4154x_BOOSTV);
}

static int sgm4154x_set_boost_current_limit(struct regulator_dev *rdev,
										int min_uA, int max_uA)
{
	int ret = 0;
	int uA = 1200000;
	struct sgm4154x_device *sgm = charger_get_data(s_chg_dev_otg);

	if (min_uA < uA && max_uA > uA)
		uA = 1200000;

	if (uA == BOOST_CURRENT_LIMIT[0]){
		ret = mmi_sgm4154x_update_bits(sgm, SGM4154x_CHRG_CTRL_2, SGM4154x_BOOST_LIM,
                     0);
	}

	else if (uA == BOOST_CURRENT_LIMIT[1]){
		ret = mmi_sgm4154x_update_bits(sgm, SGM4154x_CHRG_CTRL_2, SGM4154x_BOOST_LIM,
                     BIT(7));
	}
	return ret;
}

static int sgm4154x_boost_get_current_limit(struct regulator_dev *rdev)
{
	int ret;
	u8 vlim;
	u32 curr;

	struct sgm4154x_device *sgm = charger_get_data(s_chg_dev_otg);

	ret = mmi_sgm4154x_read_reg(sgm, SGM4154x_CHRG_CTRL_2, &vlim);
	if (ret)
		return ret;
	if (!!(vlim & SGM4154x_BOOST_LIM))
		curr = 2000000;
	else
		curr = 1200000;
	return curr ;
}

#else
static int sgm4154x_set_boost_current_limit(struct charger_device *chg_dev, u32 uA)
{	
	int ret = 0;
	struct sgm4154x_device *sgm = charger_get_data(chg_dev);
	
	if (uA == boost_current_limit[0]){
		ret = mmi_sgm4154x_update_bits(sgm, SGM4154x_CHRG_CTRL_2, SGM4154x_BOOST_LIM,
                     0); 
	}
		
	else if (uA == boost_current_limit[1]){
		ret = mmi_sgm4154x_update_bits(sgm, SGM4154x_CHRG_CTRL_2, SGM4154x_BOOST_LIM,
                     BIT(7)); 
	}
	return ret;
}
#endif

/* ============================================================ */
/* sysfs */
/* ============================================================ */
static int charging_enable_get(struct sgm4154x_device *sgm,
	struct sgm_sysfs_field_info *attr,
	int *val)
{
	*val = sgm->charge_enabled;
	pr_err("%s:sgm charging_enable_get:%d\n", __func__, *val);
	return 0;
}

static ssize_t sgm_sysfs_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct power_supply *psy;
	struct sgm4154x_device *sgm;
	struct sgm_sysfs_field_info *sgm_attr;
	int val;
	ssize_t ret;

	ret = kstrtos32(buf, 0, &val);
	if (ret < 0)
		return ret;

	psy = dev_get_drvdata(dev);
	sgm = (struct sgm4154x_device *)power_supply_get_drvdata(psy);

	sgm_attr = container_of(attr,
		struct sgm_sysfs_field_info, attr);
	if (sgm_attr->set != NULL)
		sgm_attr->set(sgm, sgm_attr, val);

	return count;
}

static ssize_t sgm_sysfs_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct power_supply *psy;
	struct sgm4154x_device *sgm;
	struct sgm_sysfs_field_info *sgm_attr;
	int val = 0;
	ssize_t count;

	psy = dev_get_drvdata(dev);
	sgm = (struct sgm4154x_device *)power_supply_get_drvdata(psy);

	sgm_attr = container_of(attr,
		struct sgm_sysfs_field_info, attr);
	if (sgm_attr->get != NULL)
		sgm_attr->get(sgm, sgm_attr, &val);

	count = scnprintf(buf, PAGE_SIZE, "%d\n", val);
	return count;
}

static struct sgm_sysfs_field_info sgm_sysfs_field_tbl[] = {
	SGM_SYSFS_FIELD_RO(charging_enable, SGM_PROP_CHARGING_ENABLED),
};

int sgm_get_property(enum sgm_property bp,int *val)
{
	struct sgm4154x_device *sgm;
	struct power_supply *psy;

	psy = power_supply_get_by_name("sgm4154x-charger");
	if (psy == NULL)
		return -ENODEV;

	sgm = (struct sgm4154x_device *)power_supply_get_drvdata(psy);
	if (sgm_sysfs_field_tbl[bp].prop == bp)
		sgm_sysfs_field_tbl[bp].get(sgm,
			&sgm_sysfs_field_tbl[bp], val);
	else {
		pr_err("%s bp:%d idx error\n", __func__, bp);
		return -ENOTSUPP;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(sgm_get_property);

int sgm_get_int_property(enum sgm_property bp)
{
	int val;

	sgm_get_property(bp, &val);
	return val;
}
EXPORT_SYMBOL_GPL(sgm_get_int_property);

static struct attribute *
	sgm_sysfs_attrs[ARRAY_SIZE(sgm_sysfs_field_tbl) + 1];

static const struct attribute_group sgm_sysfs_attr_group = {
	.attrs = sgm_sysfs_attrs,
};

static void sgm_sysfs_init_attrs(void)
{
	int i, limit = ARRAY_SIZE(sgm_sysfs_field_tbl);

	for (i = 0; i < limit; i++)
		sgm_sysfs_attrs[i] = &sgm_sysfs_field_tbl[i].attr.attr;

	sgm_sysfs_attrs[limit] = NULL; /* Has additional entry for this */
}

static int sgm_sysfs_create_group(struct power_supply *psy)
{
	sgm_sysfs_init_attrs();

	return sysfs_create_group(&psy->dev.kobj,
			&sgm_sysfs_attr_group);
}




static struct regulator_ops sgm4154x_vbus_ops = {
	.enable = sgm4154x_enable_vbus,
	.disable = sgm4154x_disable_vbus,
	.is_enabled = sgm4154x_is_enabled_vbus,
/*	.list_voltage = regulator_list_voltage_linear,
	.set_voltage_sel = sgm4154x_set_boost_voltage_limit,
	.get_voltage_sel = sgm4154x_boost_get_voltage_sel,
	.set_current_limit = sgm4154x_set_boost_current_limit,
	.get_current_limit = sgm4154x_boost_get_current_limit,*/

};

static const struct regulator_desc sgm4154x_otg_rdesc = {
	.of_match = "sgm4154x,otg-vbus",
	//.of_match = "usb-otg-vbus",
	.name = "usb-otg-vbus",
	.ops = &sgm4154x_vbus_ops,
	.owner = THIS_MODULE,
	.type = REGULATOR_VOLTAGE,
	.fixed_uV = 5150000,
	.n_voltages = 1,
//	.min_uV = 4850000,
//	.uV_step = 150000, /* 150mV per step */
//	.n_voltages = 4, /* 4850mV to 5300mV */
/*	.vsel_reg = SGM4154x_CHRG_CTRL_6,
	.vsel_mask = SGM4154x_BOOSTV,
	.enable_reg = SGM4154x_CHRG_CTRL_1,
	.enable_mask = SGM4154x_OTG_EN,
	.csel_reg = SGM4154x_CHRG_CTRL_2,
	.csel_mask = SGM4154x_BOOST_LIM,*/
};

static int sgm4154x_vbus_regulator_register(struct sgm4154x_device *sgm)
{
	struct regulator_config config = {};
	int ret = 0;
	/* otg regulator */
	config.dev = sgm->dev;
	config.driver_data = sgm;
	sgm->otg_rdev = devm_regulator_register(sgm->dev,
						&sgm4154x_otg_rdesc, &config);
	if (IS_ERR(sgm->otg_rdev)) {
		ret = PTR_ERR(sgm->otg_rdev);
		pr_info("%s: register otg regulator failed (%d)\n", __func__, ret);
	}
	sgm->otg_rdev->constraints->valid_ops_mask |= REGULATOR_CHANGE_STATUS;
	return ret;
}

static int sgm4154x_do_event(struct charger_device *chg_dev, u32 event, u32 args)
{
	struct sgm4154x_device *sgm = charger_get_data(chg_dev);

	if (!sgm->charger) {
		dev_notice(sgm->dev, "%s: cannot get charger\n", __func__);
		return -ENODEV;
	}

	pr_err("%s:event:%d\n", __func__, event);
	switch (event) {
	case EVENT_FULL:
		sgm->mmi_charging_full = true;
		break;
	case EVENT_RECHARGE:
	case EVENT_DISCHARGE:
		sgm->mmi_charging_full = false;
		break;
	default:
		break;
	}

	power_supply_changed(sgm->charger);

	return 0;
}

static struct charger_ops sgm4154x_chg_ops = {
	/* Normal charging */
	.plug_in = sgm4154x_plug_in,
	.plug_out = sgm4154x_plug_out,
	/* dump registers */
	.dump_registers = sgm4154x_dump_register,
	.enable = sgm4154x_charging_switch,
	.is_enabled = sgm4154x_is_charging_enable,
	.get_charging_current = sgm4154x_get_ichg_curr,
	.set_charging_current = sgm4154x_set_ichrg_curr,
	.get_input_current = sgm4154x_get_input_curr_lim,
	.set_input_current = sgm4154x_set_input_curr_lim,
	.get_constant_voltage = sgm4154x_get_chrg_volt,
	.set_constant_voltage = sgm4154x_set_chrg_volt,
	.kick_wdt = sgm4154x_reset_watch_dog_timer,
	.set_mivr = sgm4154x_set_input_volt_lim,
	.is_charging_done = sgm4154x_get_charging_status_done,
	.get_min_charging_current = sgm4154x_get_min_ichg,
	/* Safety timer */
	.enable_safety_timer = sgm4154x_enable_safetytimer,
	.is_safety_timer_enabled = sgm4154x_get_is_safetytimer_enable,
	/* Get vbus voltage*/
	/*.get_vbus_adc = sgm4154x_get_vbus,*/
	/* Power path */
	.enable_powerpath = sgm4154x_enable_power_path,
	/*.is_powerpath_enabled = sgm4154x_get_is_power_path_enable, */

	/* Hz mode */
	.enable_hz = sgm4154x_set_hiz_en,
	/* OTG */
	.enable_otg = sgm4154x_enable_otg,	
	.set_boost_current_limit = sgm4154x_set_boost_current_limit,
	.event = sgm4154x_do_event,

	/* PE+/PE+20 */
	.send_ta_current_pattern = sgm4154x_en_pe_current_partern,
	/*.set_pe20_efficiency_table = NULL,*/
	/*.send_ta20_current_pattern = NULL,*/
	/*.set_ta20_reset = NULL,*/
	/*.enable_cable_drop_comp = NULL,*/
};

static int sgm4154x_suspend_notifier(struct notifier_block *nb,
				unsigned long event,void *dummy)
{
    struct sgm4154x_device *sgm = container_of(nb, struct sgm4154x_device, pm_nb);

    switch (event) {

    case PM_SUSPEND_PREPARE:
        pr_err("sgm4154x PM_SUSPEND \n");

        sgm->sgm4154x_suspend_flag = 1;

        return NOTIFY_OK;

    case PM_POST_SUSPEND:
        pr_err("sgm4154x PM_RESUME \n");

        sgm->sgm4154x_suspend_flag = 0;

        return NOTIFY_OK;

    default:
        return NOTIFY_DONE;
    }
}

static int sgm4154x_driver_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	int ret = 0;
	int irq_gpio = 0, irqn = 0;
	struct device *dev = &client->dev;
	struct sgm4154x_device *sgm;
        char *name = NULL;

	pr_info("[%s]\n", __func__);

	sgm = devm_kzalloc(dev, sizeof(*sgm), GFP_KERNEL);
	if (!sgm)
		return -ENOMEM;

	sgm->client = client;
	sgm->dev = dev;	
	
	mutex_init(&sgm->lock);
	mutex_init(&sgm->i2c_rw_lock);

	i2c_set_clientdata(client, sgm);

	ret = sgm4154x_hw_chipid_detect(sgm);
	if (ret != SGM41542_PN_ID && ret != SGM41543D_PN_ID) {
		ret = sgm4154x_hw_chipid_detect(sgm);
		pr_info("[%s] device not found !!!\n", __func__);
		return ret;
	} else {
		if(ret == SGM41542_PN_ID) {
			sgm->dev_id  = SGM41542_ID;
		} else if(ret == SGM41543D_PN_ID) {
			sgm->dev_id = SGM41543D_ID;
		}
	}

	if(sgm->dev_id == SGM41513_ID || sgm->dev_id == SGM41513A_ID || sgm->dev_id == SGM41513D_ID)
		SGM4154x_IINDPM_I_MAX_uA = SGM41513_IINDPM_I_MAX_uA;
        else
		SGM4154x_IINDPM_I_MAX_uA = SGM415xx_IINDPM_I_MAX_uA;

        ret = sgm4154x_parse_dt(sgm);
        if (ret) {
                pr_info("[%s] device parse dtbo fail!!!\n", __func__);
                return ret;
        }
	
	if(sgm->dev_id == SGM41542_ID || sgm->dev_id == SGM41541_ID || sgm->dev_id == SGM41543_ID
		|| sgm->dev_id == SGM41543D_ID) {
		boost_current_limit[0] = BOOST_CURRENT_LIMIT[0][0];
		boost_current_limit[1] = BOOST_CURRENT_LIMIT[0][1];
	} else {
		boost_current_limit[0] = BOOST_CURRENT_LIMIT[1][0];
		boost_current_limit[1] = BOOST_CURRENT_LIMIT[1][1];
	}

	name = devm_kasprintf(sgm->dev, GFP_KERNEL, "%s","sgm4154x suspend wakelock");
	sgm->charger_wakelock =	wakeup_source_register(NULL,name);
	
	/* Register charger device */
	sgm->chg_dev = charger_device_register("primary_chg",
						&client->dev, sgm,
						&sgm4154x_chg_ops,
						&sgm4154x_chg_props);
	if (IS_ERR_OR_NULL(sgm->chg_dev)) {
		pr_info("%s: register charger device  failed\n", __func__);
		ret = PTR_ERR(sgm->chg_dev);
		return ret;
	}

	/* otg regulator */
	s_chg_dev_otg=sgm->chg_dev;

	ret = sgm4154x_hw_init(sgm);
	if (ret) {
		dev_err(dev, "Cannot initialize the chip.\n");
		return ret;
	}

	irq_gpio = of_get_named_gpio(sgm->dev->of_node, "sgm,irq-gpio", 0);
	if (!gpio_is_valid(irq_gpio))
	{
		dev_err(sgm->dev, "%s: %d gpio get failed\n", __func__, irq_gpio);
		return -EINVAL;
	}
	ret = gpio_request(irq_gpio, "sgm4154x irq pin");
	if (ret) {
		dev_err(sgm->dev, "%s: %d gpio request failed\n", __func__, irq_gpio);
		return ret;
	}
	gpio_direction_input(irq_gpio);
	irqn = gpio_to_irq(irq_gpio);
	if (irqn < 0) {
		dev_err(sgm->dev, "%s:%d gpio_to_irq failed\n", __func__, irqn);
		return irqn;
	}
	sgm->client->irq = irqn;

	INIT_DELAYED_WORK(&sgm->charge_detect_delayed_work, charger_detect_work_func);
	INIT_DELAYED_WORK(&sgm->charge_monitor_work, charger_monitor_work_func);
	INIT_WORK(&sgm->rerun_apsd_work, sgm4154x_rerun_apsd_work_func);

	if(sgm->typec_support) {
		INIT_DELAYED_WORK(&sgm->typec_in_work, typec_in_work_func);
		INIT_DELAYED_WORK(&sgm->typec_out_work, typec_out_work_func);
	}

	//rerun apsd and trigger charger detect when boot with charger
	schedule_work(&sgm->rerun_apsd_work);

	sgm->pm_nb.notifier_call = sgm4154x_suspend_notifier;
	ret = register_pm_notifier(&sgm->pm_nb);
	if (ret) {
		pr_err("register pm failed\n", __func__);
	}

#ifdef CONFIG_MOTO_CHG_WT6670F_SUPPORT
    INIT_DELAYED_WORK(&sgm->psy_dwork, wt6670f_get_charger_type_func_work);
#endif
    if (client->irq) {
		ret = devm_request_threaded_irq(dev, client->irq, NULL,
						sgm4154x_irq_handler_thread,
						IRQF_TRIGGER_FALLING |
						IRQF_ONESHOT,
						dev_name(&client->dev), sgm);
		if (ret)
			return ret;
		enable_irq_wake(client->irq);
	}

	ret = sgm4154x_power_supply_init(sgm, dev);
	if (ret) {
		pr_err("Failed to register power supply\n");
		return ret;
	}
	sgm_sysfs_create_group(sgm->charger);

	/*RESET*/
	//ret =  __sgm4154x_write_byte(sgm, SGM4154x_CHRG_CTRL_b, 0x80);

	//OTG setting
	//sgm4154x_set_otg_voltage(s_chg_dev_otg, 5000000); //5V
	//sgm4154x_set_otg_current(s_chg_dev_otg, 1200000); //1.2A

	ret = sgm4154x_vbus_regulator_register(sgm);
	if (ret < 0) {
		dev_err(dev, "failed to init regulator\n");
		return ret;
	}
	schedule_delayed_work(&sgm->charge_monitor_work,100);

	return ret;
}

static int sgm4154x_charger_remove(struct i2c_client *client)
{
    struct sgm4154x_device *sgm = i2c_get_clientdata(client);

    cancel_delayed_work_sync(&sgm->charge_monitor_work);

#ifdef CONFIG_MOTO_CHG_WT6670F_SUPPORT
    cancel_delayed_work_sync(&sgm->psy_dwork);
#endif
    regulator_unregister(sgm->otg_rdev);

    power_supply_unregister(sgm->charger);

	mutex_destroy(&sgm->lock);
    mutex_destroy(&sgm->i2c_rw_lock);

    return 0;
}

static void sgm4154x_charger_shutdown(struct i2c_client *client)
{
    int ret = 0;

	struct sgm4154x_device *sgm = i2c_get_clientdata(client);
    ret = sgm4154x_disable_charger(sgm);
    if (ret) {
        pr_err("Failed to disable charger, ret = %d\n", ret);
    }

    ret = sgm4154x_disable_vbus(NULL);
    if (ret < 0) {
        pr_err("Failed to disable vbus, ret = %d\n", ret);
    }
    pr_info("sgm4154x_charger_shutdown\n");
}

static const struct i2c_device_id sgm4154x_i2c_ids[] = {
	{ "sgm41541", 0 },
	{ "sgm41542", 1 },
	{ "sgm41543", 2 },
	{ "sgm41543D", 3 },
	{ "sgm41513", 4 },
	{ "sgm41513A", 5 },
	{ "sgm41513D", 6 },
	{ "sgm41516", 7 },
	{ "sgm41516D", 8 },
	{},
};
MODULE_DEVICE_TABLE(i2c, sgm4154x_i2c_ids);

static const struct of_device_id sgm4154x_of_match[] = {
	{ .compatible = "sgm,sgm41541", },
	{ .compatible = "sgm,sgm41542", },
	{ .compatible = "sgm,sgm41543", },
	{ .compatible = "sgm,sgm41543D", },
	{ .compatible = "sgm,sgm41513", },
	{ .compatible = "sgm,sgm41513A", },
	{ .compatible = "sgm,sgm41513D", },
	{ .compatible = "sgm,sgm41516", },
	{ .compatible = "sgm,sgm41516D", },
	{ },
};
MODULE_DEVICE_TABLE(of, sgm4154x_of_match);


static struct i2c_driver sgm4154x_driver = {
	.driver = {
		.name = "sgm4154x_charger",
		.of_match_table = sgm4154x_of_match,
	},
	.probe = sgm4154x_driver_probe,
	.remove = sgm4154x_charger_remove,
	.shutdown = sgm4154x_charger_shutdown,
	.id_table = sgm4154x_i2c_ids,
};
module_i2c_driver(sgm4154x_driver);

MODULE_AUTHOR(" qhq <Allen_qin@sg-micro.com>");
MODULE_DESCRIPTION("sgm4154x charger driver");
MODULE_LICENSE("GPL v2");
