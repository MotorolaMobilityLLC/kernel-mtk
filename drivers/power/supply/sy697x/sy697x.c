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
#include <mt-plat/v1/charger_class.h>
#include "mtk_charger_intf.h"
#include <mt-plat/v1/mtk_battery.h>
#include <mt-plat/v1/mtk_charger.h>
#include "sy697x.h"
#include <mt-plat/upmu_common.h>
/**********************************************************
 *
 *   [I2C Slave Setting]
 *
 *********************************************************/
#define SY697X_REG_NUM    (0x14)
#define SY697X_BOOST_LIM_NUM  7
 /* SY697X REG02 BOOST_LIM[7:7], uA */

#define CHARGER_EVENT_USE
extern int g_charger_status;
extern bool g_qc3_type_set;

static const unsigned int BOOST_CURRENT_LIMIT[] = {
	500000, 750000, 1200000, 1400000, 1650000, 
	1875000, 2150000, 2450000 
};

static const unsigned int IPRECHG_CURRENT_STABLE[] = {
	5000, 10000, 15000, 20000, 30000, 40000, 50000, 60000,
	80000, 100000, 120000, 140000, 160000, 180000, 200000, 240000
};

static const unsigned int ITERM_CURRENT_STABLE[] = {
	5000, 10000, 15000, 20000, 30000, 40000, 50000, 60000,
	80000, 100000, 120000, 140000, 160000, 180000, 200000, 240000
};

static enum power_supply_usb_type sy697x_usb_type[] = {
	POWER_SUPPLY_USB_TYPE_UNKNOWN,
	POWER_SUPPLY_USB_TYPE_SDP,
	POWER_SUPPLY_USB_TYPE_DCP,
	POWER_SUPPLY_USB_TYPE_CDP,
	POWER_SUPPLY_USB_TYPE_ACA,
};

/**********************************************************
 *
 *   [Global Variable]
 *
 *********************************************************/
static struct power_supply_desc sy697x_power_supply_desc;
static struct charger_device *s_chg_dev_otg;

/**********************************************************
 *
 *   [I2C Function For Read/Write sgm4154x]
 *
 *********************************************************/
static int __sy697x_read_byte(struct sy697x_device *sy, u8 reg, u8 *data)
{
    s32 ret;

    ret = i2c_smbus_read_byte_data(sy->client, reg);
    if (ret < 0) {
        pr_err("i2c read fail: can't read from reg 0x%02X\n", reg);
        return ret;
    }

    *data = (u8) ret;

    return 0;
}

static int __sy697x_write_byte(struct sy697x_device *sy, int reg, u8 val)
{
    s32 ret;

    ret = i2c_smbus_write_byte_data(sy->client, reg, val);
    if (ret < 0) {
        pr_err("i2c write fail: can't write 0x%02X to reg 0x%02X: %d\n",
               val, reg, ret);
        return ret;
    }
    return 0;
}

static int sy697x_read_reg(struct sy697x_device *sy, u8 reg, u8 *data)
{
	int ret;

	mutex_lock(&sy->i2c_rw_lock);
	ret = __sy697x_read_byte(sy, reg, data);
	mutex_unlock(&sy->i2c_rw_lock);

	return ret;
}
#if 0
static int sy697x_write_reg(struct sy697x_device *sy, u8 reg, u8 val)
{
	int ret;

	mutex_lock(&sy->i2c_rw_lock);
	ret = __sy697x_write_byte(sy, reg, val);
	mutex_unlock(&sy->i2c_rw_lock);

	if (ret)
		pr_err("Failed: reg=%02X, ret=%d\n", reg, ret);

	return ret;
}
#endif
static int sy697x_update_bits(struct sy697x_device *sy, u8 reg,
					u8 mask, u8 val)
{
	int ret;
	u8 tmp;

	mutex_lock(&sy->i2c_rw_lock);
	ret = __sy697x_read_byte(sy, reg, &tmp);
	if (ret) {
		pr_err("Failed: reg=%02X, ret=%d\n", reg, ret);
		goto out;
	}

	tmp &= ~mask;
	tmp |= val & mask;

	ret = __sy697x_write_byte(sy, reg, tmp);
	if (ret)
		pr_err("Failed: reg=%02X, ret=%d\n", reg, ret);

out:
	mutex_unlock(&sy->i2c_rw_lock);
	return ret;
}

/**********************************************************
 *
 *   [Internal Function]
 *
 *********************************************************/


 static int sy697x_set_watchdog_timer(struct sy697x_device *sy, int time)
{
	int ret;
	u8 reg_val;

	if (time == 0)
		reg_val = SY697X_WDT_TIMER_DISABLE;
	else if (time == 40)
		reg_val = SY697X_WDT_TIMER_40S;
	else if (time == 80)
		reg_val = SY697X_WDT_TIMER_80S;
	else
		reg_val = SY697X_WDT_TIMER_160S;	

	ret = sy697x_update_bits(sy, SY697X_CHRG_CTRL_7,
				SY697X_WDT_TIMER_MASK, reg_val);

	return ret;
}

 #if 0
 static int sgm4154x_get_term_curr(struct sgm4154x_device *sgm)
{
	int ret;
	u8 reg_val;
	int curr;
	int offset = SGM4154x_TERMCHRG_I_MIN_uA;

	ret = sgm4154x_read_reg(sgm, SGM4154x_CHRG_CTRL_3, &reg_val);
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

	ret = sgm4154x_read_reg(sgm, SGM4154x_CHRG_CTRL_3, &reg_val);
	if (ret)
		return ret;

	reg_val = (reg_val&SGM4154x_PRECHRG_CUR_MASK)>>4;
	curr = reg_val * SGM4154x_PRECHRG_CURRENT_STEP_uA + offset;
	return curr;
}

static int sgm4154x_get_ichg_curr(struct sgm4154x_device *sgm)
{
	int ret;
	u8 ichg;
    unsigned int curr;
	
	ret = sgm4154x_read_reg(sgm, SGM4154x_CHRG_CTRL_2, &ichg);
	if (ret)
		return ret;	

	ichg &= SGM4154x_ICHRG_I_MASK;
#if (defined(__SGM41513_CHIP_ID__) || defined(__SGM41513A_CHIP_ID__) || defined(__SGM41513D_CHIP_ID__))	
	if (ichg <= 0x8)
		curr = ichg * 5000;
	else if (ichg <= 0xF)
		curr = 40000 + (ichg - 0x8) * 10000;
	else if (ichg <= 0x17)
		curr = 110000 + (ichg - 0xF) * 20000;
	else if (ichg <= 0x20)
		curr = 270000 + (ichg - 0x17) * 30000;
	else if (ichg <= 0x30)
		curr = 540000 + (ichg - 0x20) * 60000;
	else if (ichg <= 0x3C)
		curr = 1500000 + (ichg - 0x30) * 120000;
	else
		curr = 3000000;
#else
	curr = ichg * SGM4154x_ICHRG_I_STEP_uA;
#endif	
	return curr;
}
#endif

static int sy697x_set_term_curr(struct sy697x_device *sy, int uA)
{
	u8 reg_val;
	
	if (uA < SY697X_TERMCHRG_I_MIN_uA)
		uA = SY697X_TERMCHRG_I_MIN_uA;
	else if (uA > SY697X_TERMCHRG_I_MAX_uA)
		uA = SY697X_TERMCHRG_I_MAX_uA;
	
	reg_val = (uA - 1) / SY697X_TERMCHRG_CURRENT_STEP_uA;// -1 in order to seam protection
	return sy697x_update_bits(sy, SY697X_CHRG_CTRL_5,
				  SY697X_TERMCHRG_CUR_MASK, reg_val);
}

static int sy697x_set_prechrg_curr(struct sy697x_device *sy, int uA)
{
	u8 reg_val;
	
	if (uA < SY697X_PRECHRG_I_MIN_uA)
		uA = SY697X_PRECHRG_I_MIN_uA;
	else if (uA > SY697X_PRECHRG_I_MAX_uA)
		uA = SY697X_PRECHRG_I_MAX_uA;

	reg_val = (uA - SY697X_PRECHRG_I_MIN_uA) / SY697X_PRECHRG_CURRENT_STEP_uA;
	reg_val = reg_val << 4;
	return sy697x_update_bits(sy, SY697X_CHRG_CTRL_5,
				  SY697X_PRECHRG_CUR_MASK, reg_val);
}

static int sy697x_set_ichrg_curr(struct charger_device *chg_dev, unsigned int uA)
{
	int ret;
	u8 reg_val;
	struct sy697x_device *sy = charger_get_data(chg_dev);
	pr_info("%s: set curr %d uA\n", __func__, uA);

	if (uA < SY697X_ICHRG_I_MIN_uA)
		uA = SY697X_ICHRG_I_MIN_uA;
	else if ( uA > sy->init_data.max_ichg)
		uA = sy->init_data.max_ichg;

	reg_val = uA / SY697X_ICHRG_I_STEP_uA;
	ret = sy697x_update_bits(sy, SY697X_CHRG_CTRL_4,
				  SY697X_ICHRG_I_MASK, reg_val);	
	return ret;
}

static int sy697x_get_ichrg_curr(struct charger_device *chg_dev,unsigned int *uA)
{
    int ret;
    u8 reg_val;
    struct sy697x_device *sy = charger_get_data(chg_dev);

    ret = sy697x_read_reg(sy, SY697X_CHRG_CTRL_4, &reg_val);
    if (ret)
        return ret;
    reg_val &= SY697X_ICHRG_I_MASK;

    *uA = reg_val * SY697X_ICHRG_I_STEP_uA;
    pr_info("%s: get curr %d uA\n", __func__, *uA);
    return 0;
}

static int sy697x_set_chrg_volt(struct charger_device *chg_dev, unsigned int chrg_volt)
{
	int ret;
	u8 reg_val;
	struct sy697x_device *sy = charger_get_data(chg_dev);
	
	if (chrg_volt < SY697X_VREG_V_MIN_uV)
		chrg_volt = SY697X_VREG_V_MIN_uV;
	else if (chrg_volt > sy->init_data.max_vreg)
		chrg_volt = sy->init_data.max_vreg;
	
	
	reg_val = (chrg_volt-SY697X_VREG_V_MIN_uV) / SY697X_VREG_V_STEP_uV;
	reg_val = reg_val<<2;
	ret = sy697x_update_bits(sy, SY697X_CHRG_CTRL_6,
				  SY697X_VREG_V_MASK, reg_val);

	return ret;
}

static int sy697x_get_chrg_volt(struct charger_device *chg_dev,unsigned int *volt)
{
	int ret;
	u8 vreg_val;
	struct sy697x_device *sy = charger_get_data(chg_dev);
	
	ret = sy697x_read_reg(sy, SY697X_CHRG_CTRL_6, &vreg_val);
	if (ret)
		return ret;	

	vreg_val = (vreg_val & SY697X_VREG_V_MASK)>>2;

	*volt = vreg_val*SY697X_VREG_V_STEP_uV + SY697X_VREG_V_MIN_uV;	

	return 0;
}
#if 0
static int sgm4154x_get_vindpm_offset_os(struct sgm4154x_device *sgm)
{
	int ret;
	u8 reg_val;

	ret = sgm4154x_read_reg(sgm, SGM4154x_CHRG_CTRL_f, &reg_val);
	if (ret)
		return ret;	

	reg_val = reg_val & SGM4154x_VINDPM_OS_MASK;	

	return reg_val;
}
#endif
static int sy697x_set_vindpm_offset_os(struct sy697x_device *sy,u8 offset_os)
{
	int ret;	
	
	ret = sy697x_update_bits(sy, SY697X_CHRG_CTRL_d,
				  SY697X_VINDPM_OS_MASK, offset_os);
	
	if (ret){
		pr_err("%s fail\n",__func__);
		return ret;
	}
	
	return ret;
}
static int sy697x_set_input_volt_lim(struct charger_device *chg_dev, unsigned int vindpm)
{
	int ret;
	//unsigned int offset;
	u8 reg_val;
	u8 os_val;
	struct sy697x_device *sy = charger_get_data(chg_dev);
	
	if (vindpm < SY697X_VINDPM_V_MIN_uV ||
	    vindpm > SY697X_VINDPM_V_MAX_uV)
 		return -EINVAL;	
	
	os_val = 0x80;
	if (vindpm < SY697X_VINDPM_V_MIN_uV) {
		reg_val = 0x0d;
	} else{
		reg_val = (vindpm - SY697X_VINDPM_V_MIN_uV) / SY697X_VINDPM_STEP_uV + 0x0d;
	}		
	
	pr_err("%s vindpm =%d,reg_val =%d\n", __func__, vindpm, reg_val);
	sy697x_set_vindpm_offset_os(sy,os_val);

	ret = sy697x_update_bits(sy, SY697X_CHRG_CTRL_d,
				  SY697X_VINDPM_V_MASK, reg_val); 

	return ret;
}
#if 0
static int sgm4154x_get_input_volt_lim(struct sgm4154x_device *sgm)
{
	int ret;
	int offset;
	u8 vlim;
	int temp;

	ret = sgm4154x_read_reg(sgm, SGM4154x_CHRG_CTRL_6, &vlim);
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

static int sy697x_set_input_curr_lim(struct charger_device *chg_dev, unsigned int iindpm)
{
	int ret;
	u8 reg_val;
	struct sy697x_device *sy = charger_get_data(chg_dev);
	
	if (iindpm < SY697X_IINDPM_I_MIN_uA ||
			iindpm > SY697X_IINDPM_I_MAX_uA)
		return -EINVAL;	

	reg_val = (iindpm-SY697X_IINDPM_I_MIN_uA) / SY697X_IINDPM_STEP_uA;
	ret = sy697x_update_bits(sy, SY697X_CHRG_CTRL_0, 0x40, 0x00);

    ret = sy697x_update_bits(sy, SY697X_CHRG_CTRL_0,
				  SY697X_IINDPM_I_MASK, reg_val);
	return ret;
}

static int sy697x_get_input_curr_lim(struct charger_device *chg_dev,unsigned int *ilim)
{
	int ret;	
	u8 reg_val;
	struct sy697x_device *sy = charger_get_data(chg_dev);
	
	ret = sy697x_read_reg(sy, SY697X_CHRG_CTRL_13, &reg_val);
	if (ret)
		return ret;	
	if (SY697X_IINDPM_I_MASK == (reg_val & SY697X_IINDPM_I_MASK))
		*ilim =  SY697X_IINDPM_I_MAX_uA;
	else
		*ilim = (reg_val & SY697X_IINDPM_I_MASK)*SY697X_IINDPM_STEP_uA + SY697X_IINDPM_I_MIN_uA;

	return 0;
}

static int sy697x_get_state(struct sy697x_device *sy,
			     struct sy697x_state *state)
{
	u8 chrg_stat;
	u8 fault;
	u8 chrg_param_0,chrg_param_1,chrg_param_2;
	int ret;

	msleep(1); /*for  BC12*/
	ret = sy697x_read_reg(sy, SY697X_CHRG_CTRL_b, &chrg_stat);
	if (ret){
		ret = sy697x_read_reg(sy, SY697X_CHRG_CTRL_b, &chrg_stat);
		if (ret){
			pr_err("%s chrg_type =%d,chrg_stat =%d online = %d\n", __func__, sy->state.chrg_type, sy->state.chrg_stat, sy->state.online);
			pr_err("%s read SY697X_CHRG_STAT fail\n",__func__);
			return ret;
		}
	}
	state->chrg_type = chrg_stat & SY697X_VBUS_STAT_MASK;
	state->chrg_stat = chrg_stat & SY697X_CHG_STAT_MASK;
	state->online = !!(chrg_stat & SY697X_PG_STAT);
	state->therm_stat = !!(chrg_stat & SY697X_THERM_STAT);
	state->vsys_stat = !!(chrg_stat & SY697X_VSYS_STAT);
	
	pr_err("%s chrg_type =%d,chrg_stat =%d online = %d\n",__func__,state->chrg_type,state->chrg_stat,state->online);
	

	ret = sy697x_read_reg(sy, SY697X_CHRG_CTRL_c, &fault);
	if (ret){
		pr_err("%s read SY697X_CHRG_CTRL_c fail\n",__func__);
		return ret;
	}
	state->chrg_fault = fault;	
	state->ntc_fault = fault & SY697X_TEMP_MASK;
	state->health = state->ntc_fault;
	ret = sy697x_read_reg(sy, SY697X_CHRG_CTRL_0, &chrg_param_0);
	if (ret){
		pr_err("%s read SY697X_CHRG_CTRL_0 fail\n",__func__);
		return ret;
	}
	state->hiz_en = !!(chrg_param_0 & SY697X_HIZ_EN);
	
	ret = sy697x_read_reg(sy, SY697X_CHRG_CTRL_7, &chrg_param_1);
	if (ret){
		pr_err("%s read SY697X_CHRG_CTRL_7 fail\n",__func__);
		return ret;
	}
	state->term_en = !!(chrg_param_1 & SY697X_TERM_EN);
	
	ret = sy697x_read_reg(sy, SY697X_CHRG_CTRL_11, &chrg_param_2);
	if (ret){
		pr_err("%s read SY697X_CHRG_CTRL_11 fail\n",__func__);
		return ret;
	}

	state->vbus_gd = !!(chrg_param_2 & SY697X_VBUS_GOOD);
	if (state->chrg_stat == 0 || state->chrg_stat == SY697X_TERM_CHRG) {
		state->charge_enabled = false;
	} else {
		state->charge_enabled = true;
	}

	return 0;
}

static int sy697x_set_hiz(struct charger_device *chg_dev, bool hiz_en)
{
    u8 reg_val;
    struct sy697x_device *sy = charger_get_data(chg_dev);

    dev_notice(sy->dev, "%s:%d", __func__, hiz_en);
    reg_val = hiz_en ? SY697X_HIZ_EN : 0;

    return sy697x_update_bits(sy, SY697X_CHRG_CTRL_0,SY697X_HIZ_MASK, reg_val);
}

static int sy697x_enable_powerpath(struct charger_device *chg_dev, bool en)
{
    struct sy697x_device *sy = charger_get_data(chg_dev);

    dev_notice(sy->dev, "%s en = %d\n", __func__, en);

    return sy697x_set_hiz(chg_dev, !en);
}

static int sy697x_enable_charger(struct sy697x_device *sy)
{
    int ret;
    
    ret = sy697x_update_bits(sy, SY697X_CHRG_CTRL_3, SY697X_CHRG_EN,
                     SY697X_CHRG_EN);
	
    return ret;
}

static int sy697x_disable_charger(struct sy697x_device *sy)
{
    int ret;
    
    ret = sy697x_update_bits(sy, SY697X_CHRG_CTRL_3, SY697X_CHRG_EN,
                     0);
    return ret;
}

static int sy697x_charging_switch(struct charger_device *chg_dev,bool enable)
{
	int ret;
	struct sy697x_device *sy = charger_get_data(chg_dev);
	
	if (enable)
		ret = sy697x_enable_charger(sy);
	else
		ret = sy697x_disable_charger(sy);
	return ret;
}

static int sy697x_enable_apsd_bc12(struct sy697x_device *sy)
{
	int ret;

	ret = sy697x_update_bits(sy, SY697X_CHRG_CTRL_2, SY697X_FORCE_DPDM_EN,
		SY697X_FORCE_DPDM_EN);
	return ret;
}

static int sy697x_get_apsd_status(struct sy697x_device *sy, u8 *state)
{
	u8 val;
	int ret;

	ret = sy697x_read_reg(sy, SY697X_CHRG_CTRL_2, &val);
	if (ret) {
		return ret;
	}
	pr_info("%s reg2 = %d\n",__func__, val);
	*state = (val & SY697X_CHG_DPDM_MASK) >> SY697X_CHG_DPDM_SHIFT;

	return ret;
}

static int sy697x_set_recharge_volt(struct sy697x_device *sy, int mV)
{
	u8 reg_val;

	if (mV > SY697X_VRECHARGE_MAX_mV) {
		mV = SY697X_VRECHARGE_MAX_mV;
	}

	reg_val = (mV - SY697X_VRECHRG_OFFSET_mV) / SY697X_VRECHRG_STEP_mV;

	return sy697x_update_bits(sy, SY697X_CHRG_CTRL_6,
		SY697X_VRECHARGE, reg_val);
}

static int sy697x_set_wdt_rst(struct sy697x_device *sy, bool is_rst)
{
	u8 val;

	if (is_rst) {
		val = SY697X_WDT_RST_MASK;
	} else {
		val = 0;
	}

	return sy697x_update_bits(sy, SY697X_CHRG_CTRL_3,
		SY697X_WDT_RST_MASK, val);
}

static int sy697x_enable_chg_type_det(struct charger_device *chg_dev, bool en)
{
    struct sy697x_device *sy = charger_get_data(chg_dev);

    dev_notice(sy->dev, "%s en = %d\n", __func__, en);

    if(en) {
        schedule_delayed_work(&sy->apsd_work, 10);
        pr_err("apsd work error");
    }
    return 0;
}


/**********************************************************
 *
 *   [Internal Function]
 *
 *********************************************************/
static int sy697x_dump_register(struct charger_device *chg_dev)
{

	unsigned char i = 0;
	unsigned int ret = 0;
	unsigned char sy697x_reg[SY697X_REG_NUM+1] = { 0 };
	struct sy697x_device *sy = charger_get_data(chg_dev);
		
	for (i = 0; i < SY697X_REG_NUM+1; i++) {
		ret = sy697x_read_reg(sy,i, &sy697x_reg[i]);
		if (ret) {
			pr_info("[sy697x] i2c transfor error\n");
			return 1;
		}
		pr_info("%s,[0x%x]=0x%x ",__func__, i, sy697x_reg[i]);
	}
	
	return 0;
}


/**********************************************************
 *
 *   [Internal Function]
 *
 *********************************************************/
static int sy697x_hw_chipid_detect(struct sy697x_device *sy)
{
	int ret = 0;
	u8 val = 0;
	ret = sy697x_read_reg(sy,SY697X_CHRG_CTRL_14,&val);
	if (ret < 0)
	{
		pr_info("[%s] read SY697X_CHRG_CTRL_14 fail\n", __func__);
		return -1;
	}		
	val = val & SY697X_PN_MASK;
	pr_info("[%s] Reg[0x14]=0x%x\n", __func__,val);

	return 0;
}

static int sy697x_reset_watch_dog_timer(struct charger_device
		*chg_dev)
{
	int ret;
	struct sy697x_device *sy = charger_get_data(chg_dev);

	pr_info("charging_reset_watch_dog_timer\n");

	ret = sy697x_set_wdt_rst(sy,0x1);	/* RST watchdog */	

	return ret;
}


static int sy697x_get_charging_status(struct charger_device *chg_dev,
				       bool *is_done)
{
	//struct sgm4154x_state state;
	struct sy697x_device *sy = charger_get_data(chg_dev);
	//sgm4154x_get_state(sgm, &state);

	if (sy->state.chrg_stat == SY697X_TERM_CHRG)
		*is_done = true;
	else
		*is_done = false;

	return 0;
}

static int sy697x_set_en_timer(struct sy697x_device *sy)
{
	int ret;	

	ret = sy697x_update_bits(sy, SY697X_CHRG_CTRL_7,
				SY697X_SAFETY_TIMER_EN, SY697X_SAFETY_TIMER_EN);

	return ret;
}

static int sy697x_set_disable_timer(struct sy697x_device *sy)
{
	int ret;	

	ret = sy697x_update_bits(sy, SY697X_CHRG_CTRL_7,
				SY697X_SAFETY_TIMER_EN, 0);

	return ret;
}

static int sy697x_enable_safetytimer(struct charger_device *chg_dev,bool en)
{
	struct sy697x_device *sy = charger_get_data(chg_dev);
	int ret = 0;

	if (en)
		ret = sy697x_set_en_timer(sy);
	else
		ret = sy697x_set_disable_timer(sy);
	return ret;
}

static int sy697x_get_is_safetytimer_enable(struct charger_device
		*chg_dev,bool *en)
{
	int ret = 0;
	u8 val = 0;
	
	struct sy697x_device *sy = charger_get_data(chg_dev);
	
	ret = sy697x_read_reg(sy,SY697X_CHRG_CTRL_7,&val);
	if (ret < 0)
	{
		pr_info("[%s] read SY697X_CHRG_CTRL_7 fail\n", __func__);
		return ret;
	}
	*en = !!(val & SY697X_SAFETY_TIMER_EN);
	return 0;
}

static int sy697x_en_pe_current_partern(struct charger_device
		*chg_dev,bool is_up)
{
	int ret = 0;	
	
	struct sy697x_device *sy = charger_get_data(chg_dev);
	
	ret = sy697x_update_bits(sy, SY697X_CHRG_CTRL_4,
				SY697X_EN_PUMPX, SY697X_EN_PUMPX);
	if (ret < 0)
	{
		pr_info("[%s] read SY697X_CHRG_CTRL_4 fail\n", __func__);
		return ret;
	}
	if (is_up)
		ret = sy697x_update_bits(sy, SY697X_CHRG_CTRL_9,
				SY697X_PUMPX_UP, SY697X_PUMPX_UP);
	else
		ret = sy697x_update_bits(sy, SY697X_CHRG_CTRL_9,
				SY697X_PUMPX_DN, SY697X_PUMPX_DN);
	return ret;
}

static int sy697x_set_boost_current_limit(struct charger_device *chg_dev, u32 uA)
{
    int ret = 0;
    u8 index;
    struct sy697x_device *sy = charger_get_data(chg_dev);

    for(index = SY697X_BOOST_LIM_NUM - 1;index >= 0; index--) {
        if(uA >= BOOST_CURRENT_LIMIT[index]) {
            ret = sy697x_update_bits(sy, SY697X_CHRG_CTRL_a, SY697X_BOOST_CUR_MASK,
                            index);
            break;
        }
    }
    return ret;
}


static enum power_supply_property sy697x_power_supply_props[] = {
	POWER_SUPPLY_PROP_MANUFACTURER,
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_TYPE,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_USB_TYPE,
	POWER_SUPPLY_PROP_CHARGING_ENABLED,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_VOLTAGE_MAX,
	POWER_SUPPLY_PROP_CURRENT_MAX
};

static int sy697x_property_is_writeable(struct power_supply *psy,
					 enum power_supply_property prop)
{
	switch (prop) {
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE:
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
	case POWER_SUPPLY_PROP_PRECHARGE_CURRENT:
	case POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT:
	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		return true;
	default:
		return false;
	}
}
static int sy697x_charger_set_property(struct power_supply *psy,
		enum power_supply_property prop,
		const union power_supply_propval *val)
{
	//struct sgm4154x_device *sgm = power_supply_get_drvdata(psy);
	int ret = 0;

	switch (prop) {
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		ret = sy697x_set_input_curr_lim(s_chg_dev_otg, val->intval);
		break;
	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		sy697x_charging_switch(s_chg_dev_otg,val->intval);
		break;
/*	case POWER_SUPPLY_PROP_INPUT_VOLTAGE_LIMIT:
		ret = sgm4154x_set_input_volt_lim(s_chg_dev_otg, val->intval);
		break;*/
	default:
		return -EINVAL;
	}

	return ret;
}

static int sy697x_charger_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct sy697x_device *sy = power_supply_get_drvdata(psy);
	struct sy697x_state state;
	int ret = 0;

	mutex_lock(&sy->lock);
	//ret = sgm4154x_get_state(sgm, &state);
	state = sy->state;
	mutex_unlock(&sy->lock);
	if (ret)
		return ret;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		if (!state.chrg_type || (state.chrg_type == SY697X_OTG_MODE))
			val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
		else if (!state.chrg_stat)
			val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
		else if (state.chrg_stat == SY697X_TERM_CHRG)
			val->intval = POWER_SUPPLY_STATUS_FULL;
		else
			val->intval = POWER_SUPPLY_STATUS_CHARGING;
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		switch (state.chrg_stat) {		
		case SY697X_PRECHRG:
			val->intval = POWER_SUPPLY_CHARGE_TYPE_TRICKLE;
			break;
		case SY697X_FAST_CHRG:
			val->intval = POWER_SUPPLY_CHARGE_TYPE_FAST;
			break;		
		case SY697X_TERM_CHRG:
			val->intval = POWER_SUPPLY_CHARGE_TYPE_TRICKLE;
			break;
		case SY697X_NOT_CHRGING:
			val->intval = POWER_SUPPLY_CHARGE_TYPE_NONE;
			break;
		default:
			val->intval = POWER_SUPPLY_CHARGE_TYPE_UNKNOWN;
		}
		break;

	case POWER_SUPPLY_PROP_USB_TYPE:
		val->intval = sy->psy_usb_type;
		break;

	case POWER_SUPPLY_PROP_MANUFACTURER:
		val->strval = SY697X_MANUFACTURER;
		break;

	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = sy->sy697x_chg_props.alias_name;
		break;

	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = state.online;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = state.vbus_gd;
		break;
	case POWER_SUPPLY_PROP_TYPE:
		val->intval = sy697x_power_supply_desc.type;
		break;	

	case POWER_SUPPLY_PROP_HEALTH:
		if (state.chrg_fault & 0xF8)
			val->intval = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
		else
			val->intval = POWER_SUPPLY_HEALTH_GOOD;

		switch (state.health) {
		case SY697X_TEMP_HOT:
			val->intval = POWER_SUPPLY_HEALTH_OVERHEAT;
			break;
		case SY697X_TEMP_WARM:
			val->intval = POWER_SUPPLY_HEALTH_GOOD;
			break;
		case SY697X_TEMP_COOL:
			val->intval = POWER_SUPPLY_HEALTH_GOOD;
			break;
		case SY697X_TEMP_COLD:
			val->intval = POWER_SUPPLY_HEALTH_COLD;
			break;
		}
		break;

	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = battery_get_vbus();
		break;

	case POWER_SUPPLY_PROP_CURRENT_NOW:
		//val->intval = state.ibus_adc;
		break;

/*	case POWER_SUPPLY_PROP_INPUT_VOLTAGE_LIMIT:
		ret = sgm4154x_get_input_volt_lim(sgm);
		if (ret < 0)
			return ret;

		val->intval = ret;
		break;*/

	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		break;

	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		val->intval = state.charge_enabled;
		break;

	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		val->intval = battery_get_vbus() * 1000; /* uv */
		break;

	case POWER_SUPPLY_PROP_CURRENT_MAX:
		switch (sy->psy_usb_type) {
		case POWER_SUPPLY_USB_TYPE_SDP: /* 500mA */
			val->intval = 500000;
			break;
		case POWER_SUPPLY_USB_TYPE_CDP: /* 1500mA */
		case POWER_SUPPLY_USB_TYPE_ACA:
			val->intval = 1500000;
			break;
		case POWER_SUPPLY_USB_TYPE_DCP: /* 2000mA */
			val->intval = 2000000;
			break;
		default :
			break;
		}
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

static int sy697x_get_hvdcp_type(struct charger_device *chg_dev, bool *state)
{
	struct sy697x_device *sy = charger_get_data(chg_dev);

	if ((g_qc3_type_set == true) && (sy->advanced ==  CHARGERING_33W)) {
		*state = true;
	} else {
		*state = false;
	}

	return 0;
}

static void chg_type_apsd_work_func(struct work_struct *work)
{
	int ret;
	int i = 0;
	u8 state;
	struct sy697x_device * sy = NULL;
	struct delayed_work *apsd_work = NULL;

	pr_info("%s enter",__func__);

	apsd_work = container_of(work, struct delayed_work, work);
	if (apsd_work == NULL) {
		pr_err("Cann't get charge_monitor_work\n");
		return;
	}
	sy = container_of(apsd_work, struct sy697x_device, apsd_work);
	if (sy == NULL) {
		pr_err("Cann't get sy \n");
		return;
	}

	/*Set cur is 500ma before do BC1.2 & QC*/
	charger_manager_set_qc_input_current_limit(sy->chg_consumer, 0, 500000);
	charger_manager_set_qc_charging_current_limit(sy->chg_consumer, 0, 500000);
	sy697x_set_input_curr_lim(s_chg_dev_otg, 500000);
	sy697x_set_ichrg_curr(s_chg_dev_otg, 500000);

	msleep(1000);
	Charger_Detect_Init();
	ret = sy697x_enable_apsd_bc12(sy);
	/*BC12 input detection is completed at most 1s*/
	for (i = 0; i < 10; i++) {
		msleep(100);
		ret = sy697x_get_apsd_status(sy,&state);
		if (!ret) {
			if (state) {
				pr_err("input detection not completed error ret = %d\n", state);
			} else {
				pr_err("input detection completed ret = %d\n", state);
				break;
	    		}
		} else {
			pr_err("get apsd status fail ret = %d\n", ret);
		}
	}
	Charger_Detect_Release();
	/* set curr default after BC1.2*/
	charger_manager_set_qc_input_current_limit(sy->chg_consumer, 0, -1);
	charger_manager_set_qc_charging_current_limit(sy->chg_consumer, 0, -1);

	return;
}

static void charger_monitor_work_func(struct work_struct *work)
{
	int ret = 0;
	struct sy697x_device * sy = NULL;
	struct delayed_work *charge_monitor_work = NULL;
	//static u8 last_chg_method = 0;
	struct sy697x_state state;

	pr_err("%s --Start---\n",__func__);
	charge_monitor_work = container_of(work, struct delayed_work, work);
	if(charge_monitor_work == NULL) {
		pr_err("Cann't get charge_monitor_work\n");
		return ;
	}
	sy = container_of(charge_monitor_work, struct sy697x_device, charge_monitor_work);
	if(sy == NULL) {
		pr_err("Cann't get sy \n");
		return ;
	}

	ret = sy697x_get_state(sy, &state);
	if (ret) {
		pr_err("%s: get reg state error\n", __func__);
		goto OUT;
	}

	mutex_lock(&sy->lock);
	sy->state = state;
	mutex_unlock(&sy->lock);

	if (!state.chrg_type || (state.chrg_type == SY697X_OTG_MODE))
		g_charger_status = POWER_SUPPLY_STATUS_DISCHARGING;
	else if (!state.chrg_stat)
		g_charger_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
	else if (state.chrg_stat == SY697X_TERM_CHRG)
		g_charger_status = POWER_SUPPLY_STATUS_FULL;
	else
		g_charger_status = POWER_SUPPLY_STATUS_CHARGING;

	if(!sy->state.vbus_gd) {
		dev_err(sy->dev, "Vbus not present, disable charge\n");
		sy697x_disable_charger(sy);
		goto OUT;
	}
	if(!state.online)
	{
		dev_err(sy->dev, "Vbus not online\n");
		goto OUT;
	}
	sy697x_dump_register(sy->chg_dev);

	pr_err("%s--End--\n",__func__);
OUT:
	//sy697x_dump_register(sy->chg_dev);
	schedule_delayed_work(&sy->charge_monitor_work, 10*HZ);
}

extern void wt6670f_get_charger_type_func_work(struct work_struct *work);
extern int wt6670f_reset_charger_type(void);
static void charger_detect_work_func(struct work_struct *work)
{
	static bool type_dect_again = 1;
	struct delayed_work *charge_detect_delayed_work = NULL;
	struct sy697x_device * sy = NULL;
	//static int charge_type_old = 0;
	int curr_in_limit = 0;
	unsigned int non_stand_current_limit = 0;
	struct power_supply *chrgdet_psy;
	enum charger_type chrg_type = CHARGER_UNKNOWN;
	union power_supply_propval propval = {0};
	static bool fullStatus = false;
	struct sy697x_state state;
	int ret;

	pr_err("%s --do charger Detect--\n",__func__);
	charge_detect_delayed_work = container_of(work, struct delayed_work, work);
	if(charge_detect_delayed_work == NULL) {
		pr_err("Cann't get charge_detect_delayed_work\n");
		return ;
	}

	chrgdet_psy = power_supply_get_by_name("charger");
	if (!chrgdet_psy) {
		pr_notice("[%s]: get power supply failed\n", __func__);
		return;
	}

	sy = container_of(charge_detect_delayed_work, struct sy697x_device, charge_detect_delayed_work);
	if(sy == NULL) {
		pr_err("Cann't get sy697x_device\n");
		return ;
	}

	if (!sy->charger_wakelock->active)
		__pm_stay_awake(sy->charger_wakelock);

	ret = sy697x_get_state(sy, &state);
	mutex_lock(&sy->lock);
	sy->state = state;
	mutex_unlock(&sy->lock);

	if(!sy->state.vbus_gd) {
		dev_err(sy->dev, "Vbus not present, disable charge & Relax wakelock\n");
		sy697x_disable_charger(sy);
		fullStatus = false;
		g_qc3_type_set = false;
		wt6670f_reset_charger_type();
		__pm_relax(sy->charger_wakelock);
		type_dect_again = 1;
		//Charger_Detect_Init();
		goto err;
	}
	if(!state.online)
	{
		dev_err(sy->dev, "vbus is present, but not online, doing poor source detecting\n");
		dev_err(sy->dev, "Relax wakelock\n");
		wt6670f_reset_charger_type();
		if (sy->charger_wakelock->active)
			__pm_relax(sy->charger_wakelock);
		goto err;
	}

	if (state.online && state.chrg_stat == SY697X_TERM_CHRG) {
		dev_err(sy->dev, "charging terminated\n");
	}

	switch(sy->state.chrg_type) {
		case SY697X_USB_SDP:
			sy697x_power_supply_desc.type = POWER_SUPPLY_TYPE_USB;
			sy->psy_usb_type = POWER_SUPPLY_USB_TYPE_SDP;
			chrg_type = STANDARD_HOST;
			pr_err("SY697X charger type: SDP\n");
			curr_in_limit = 500000;
			break;

		case SY697X_USB_CDP:
			sy697x_power_supply_desc.type = POWER_SUPPLY_TYPE_USB_CDP;
			sy->psy_usb_type = POWER_SUPPLY_USB_TYPE_CDP;
			chrg_type = CHARGING_HOST;
			pr_err("SY697X charger type: CDP\n");
			curr_in_limit = 1500000;
			break;

		case SY697X_USB_DCP:
			sy697x_power_supply_desc.type = POWER_SUPPLY_TYPE_USB_DCP;
			sy->psy_usb_type = POWER_SUPPLY_USB_TYPE_DCP;
			chrg_type = STANDARD_CHARGER;
			pr_err("SY697X charger type: DCP\n");

			/* in terminated status forbiden QC charge */
			if (state.online && (state.chrg_stat == SY697X_TERM_CHRG) && !fullStatus) {
				fullStatus = true;
			} else if (fullStatus) {
				goto err;
			}
			if (sy->advanced == CHARGERING_33W) {
				schedule_delayed_work(&sy->psy_work, 0);
			}
			curr_in_limit = 2000000;
			break;

		case SY697X_NON_STANDARD:
			sy697x_power_supply_desc.type = POWER_SUPPLY_TYPE_USB_ACA;
			sy->psy_usb_type = POWER_SUPPLY_USB_TYPE_ACA;
			chrg_type = NONSTANDARD_CHARGER;
			pr_err("SY697X charger type: NON_STANDARD\n");
			curr_in_limit = 1000000;
			ret = sy697x_get_input_curr_lim(sy->chg_dev, &non_stand_current_limit);
			if (!ret) {
				curr_in_limit = non_stand_current_limit;
				switch(curr_in_limit) {
				case SY697x_APPLE_1A:
					chrg_type = APPLE_1_0A_CHARGER;
					break;
				case SY697x_APPLE_2A:
				case SY697x_APPLE_2P1A:
				case SY697x_APPLE_2P4A:
					chrg_type = APPLE_2_1A_CHARGER;
					break;
				default:
					chrg_type = APPLE_1_0A_CHARGER;
					break;
				}
			}
			pr_err("SY697x charger type: NON_STANDARD_ADAPTER, curr_in_limit = %d \n", curr_in_limit);
			break;

		case SY697X_UNKNOWN:
			sy697x_power_supply_desc.type = POWER_SUPPLY_TYPE_UNKNOWN;
			sy->psy_usb_type = POWER_SUPPLY_USB_TYPE_UNKNOWN;
			chrg_type = NONSTANDARD_CHARGER;
			pr_err("SY697X charger type: UNKNOWN\n");
			curr_in_limit = 1000000;
			if (type_dect_again) {
				type_dect_again = 0;
				schedule_delayed_work(&sy->apsd_work, 1000);
				pr_err("[%s]: chrg_type = unknown, try again \n", __func__);
				goto err;
			}
			break;

		default:
			pr_err("SY697X charger type: default\n");
			//curr_in_limit = 500000;
			//break;
			return;
	}

	ret = power_supply_get_property(chrgdet_psy,
			POWER_SUPPLY_PROP_CHARGE_TYPE, &propval);

	if(ret < 0) {
		pr_info("[%s]: get charge type failed, ret = %d\n", __func__, ret);
		return ;
	}

	/* at DCP do not release */
	if (sy->state.chrg_type != SY697X_USB_DCP) {
		Charger_Detect_Release();
	}

	//set charge parameters
	dev_err(sy->dev, "Update:curr_in_limit = %d\n", curr_in_limit);
	sy697x_set_input_curr_lim(sy->chg_dev, curr_in_limit);
	//enable charge
	sy697x_enable_charger(sy);
	sy697x_dump_register(sy->chg_dev);

err:
	//release wakelock
	pr_err("%s --Notify charged--\n",__func__);
	propval.intval = chrg_type;
	pr_info("%s: set mt_charger type %d\n", __func__, chrg_type);
	ret = power_supply_set_property(chrgdet_psy,
			POWER_SUPPLY_PROP_CHARGE_TYPE, &propval);

	power_supply_changed(sy->charger);
	//dev_err(sy->dev, "Relax wakelock\n");
	//__pm_relax(sy->charger_wakelock);
	return;
}

static irqreturn_t sy697x_irq_handler_thread(int irq, void *private)
{
	struct sy697x_device *sy = private;
	int ret = 0;
	u8 state = 0;
	//lock wakelock
	pr_err("%s entry\n",__func__);

	ret = sy697x_get_apsd_status(sy,&state);
	if (!ret) {
		if (!state) {
			schedule_delayed_work(&sy->charge_detect_delayed_work,1);
		} else {
			pr_err("input detection not completed error state = %d\n", state);
		}
	} else {
		pr_err("get apsd status fail ret = %d\n", ret);
	}
	//power_supply_changed(sgm->charger);

	return IRQ_HANDLED;
}
static char *sy697x_charger_supplied_to[] = {
	"battery",
};

static struct power_supply_desc sy697x_power_supply_desc = {
	.name = "sgm4154x-charger",
	.type = POWER_SUPPLY_TYPE_USB,
	.usb_types = sy697x_usb_type,
	.num_usb_types = ARRAY_SIZE(sy697x_usb_type),
	.properties = sy697x_power_supply_props,
	.num_properties = ARRAY_SIZE(sy697x_power_supply_props),
	.get_property = sy697x_charger_get_property,
	.set_property = sy697x_charger_set_property,
	.property_is_writeable = sy697x_property_is_writeable,
};

static int sy697x_power_supply_init(struct sy697x_device *sy,
							struct device *dev)
{
	struct power_supply_config psy_cfg = { .drv_data = sy,
						.of_node = dev->of_node, };

	psy_cfg.supplied_to = sy697x_charger_supplied_to;
	psy_cfg.num_supplicants = ARRAY_SIZE(sy697x_charger_supplied_to);

	sy->charger = devm_power_supply_register(sy->dev,
						 &sy697x_power_supply_desc,
						 &psy_cfg);
	if (IS_ERR(sy->charger))
		return -EINVAL;
	
	return 0;
}

static int sy697x_hw_init(struct sy697x_device *sy)
{
	int ret = 0;	
	struct power_supply_battery_info bat_info = { };	

	bat_info.constant_charge_current_max_ua =
			SY697X_ICHRG_I_DEF_uA;

	bat_info.constant_charge_voltage_max_uv =
			SY697X_VREG_V_DEF_uV;

	bat_info.precharge_current_ua =
			SY697X_PRECHRG_I_DEF_uA;

	bat_info.charge_term_current_ua =
			SY697X_TERMCHRG_I_DEF_uA;

	sy->init_data.max_ichg =
			SY697X_ICHRG_I_MAX_uA;

	sy->init_data.max_vreg =
			SY697X_VREG_V_MAX_uV;
			
	sy697x_set_watchdog_timer(sy,0);

	ret = sy697x_set_ichrg_curr(s_chg_dev_otg,
				bat_info.constant_charge_current_max_ua);
	if (ret)
		goto err_out;

	ret = sy697x_set_prechrg_curr(sy, bat_info.precharge_current_ua);
	if (ret)
		goto err_out;

	ret = sy697x_set_chrg_volt(s_chg_dev_otg,
				bat_info.constant_charge_voltage_max_uv);
	if (ret)
		goto err_out;

	ret = sy697x_set_term_curr(sy, bat_info.charge_term_current_ua);
	if (ret)
		goto err_out;

	ret = sy697x_set_input_volt_lim(s_chg_dev_otg, sy->init_data.vlim);
	if (ret)
		goto err_out;
	/* write bit3 close sy6970 hvdcp en cortol */
	ret = sy697x_update_bits(sy, SY697X_CHRG_CTRL_2, 0x18, 0x0);
	if (ret)
		goto err_out;

	ret = sy697x_set_input_curr_lim(s_chg_dev_otg, sy->init_data.ilim);
	if (ret)
		goto err_out;

	ret = sy697x_set_boost_current_limit(s_chg_dev_otg, 1200000);// defualt boost current limit
	if (ret)
		goto err_out;
	/* set boost voltage is 5.44V (0XE0) when durk OVP */
	ret = sy697x_update_bits(sy, SY697X_CHRG_CTRL_a, SY697X_BOOST_VOL_MASK, 0xe0);
	if (ret)
		goto err_out;

	ret = sy697x_set_recharge_volt(sy, 200);//100~200mv
	if (ret)
		goto err_out;

	dev_notice(sy->dev, "ichrg_curr:%d prechrg_curr:%d chrg_vol:%d"
		" term_curr:%d input_curr_lim:%d",
		bat_info.constant_charge_current_max_ua,
		bat_info.precharge_current_ua,
		bat_info.constant_charge_voltage_max_uv,
		bat_info.charge_term_current_ua,
		sy->init_data.ilim);

	/*VINDPM*/
//	ret = sy697x_update_bits(sgm, SY697X_CHRG_CTRL_a,
//				SY697X_VINDPM_V_MASK, 0x3);

	/*ts error-- temp disable jeita */
//ret = __sy697x_write_byte(sgm, SGM4154x_CHRG_CTRL_d, 0x0);

	return 0;

err_out:
	return ret;

}

static int sy697x_parse_dt(struct sy697x_device *sy)
{
	int ret;	
	int irq_gpio = 0, irqn = 0;	
	int chg_en_gpio = 0;

	ret = device_property_read_string(sy->dev, "alias_name",
					&sy->sy697x_chg_props.alias_name);
	if (ret < 0) {
		pr_info("%s: no alias name\n", __func__);
		sy->sy697x_chg_props.alias_name = "sy6970";
	}

	ret = device_property_read_u32(sy->dev,
				       "input-voltage-limit-microvolt",
				       &sy->init_data.vlim);
	if (ret)
		sy->init_data.vlim = SY697X_VINDPM_DEF_uV;

	if (sy->init_data.vlim > SY697X_VINDPM_V_MAX_uV ||
	    sy->init_data.vlim < SY697X_VINDPM_V_MIN_uV)
		return -EINVAL;

	ret = device_property_read_u32(sy->dev,
				       "input-current-limit-microamp",
				       &sy->init_data.ilim);
	if (ret)
		sy->init_data.ilim = SY697X_IINDPM_DEF_uA;

	if (sy->init_data.ilim > SY697X_IINDPM_I_MAX_uA ||
	    sy->init_data.ilim < SY697X_IINDPM_I_MIN_uA)
		return -EINVAL;

        ret = device_property_read_u32(sy->dev, "sy,is-advanced",
                &sy->advanced);
        if (ret) {
                pr_err("no type of machine %d\n",&sy->advanced);
        }

	irq_gpio = of_get_named_gpio(sy->dev->of_node, "sy,irq-gpio", 0);
	if (!gpio_is_valid(irq_gpio))
	{
		dev_err(sy->dev, "%s: %d gpio get failed\n", __func__, irq_gpio);
		return -EINVAL;
	}
	ret = gpio_request(irq_gpio, "sy697x irq pin");
	if (ret) {
		dev_err(sy->dev, "%s: %d gpio request failed\n", __func__, irq_gpio);
		return ret;
	}
	gpio_direction_input(irq_gpio);
	irqn = gpio_to_irq(irq_gpio);
	if (irqn < 0) {
		dev_err(sy->dev, "%s:%d gpio_to_irq failed\n", __func__, irqn);
		return irqn;
	}
	sy->client->irq = irqn;
	
	chg_en_gpio = of_get_named_gpio(sy->dev->of_node, "sy,chg-en-gpio", 0);
	if (!gpio_is_valid(chg_en_gpio))
	{
		dev_err(sy->dev, "%s: %d gpio get failed\n", __func__, chg_en_gpio);
		return -EINVAL;
	}
	ret = gpio_request(chg_en_gpio, "sy chg en pin");
	if (ret) {
		dev_err(sy->dev, "%s: %d gpio request failed\n", __func__, chg_en_gpio);
		return ret;
	}
	gpio_direction_output(chg_en_gpio,0);//default enable charge
	return 0;
}

static int sy697x_enable_vbus(struct regulator_dev *rdev)
{	
	int ret = 0;
	struct sy697x_device *sy = charger_get_data(s_chg_dev_otg);

	pr_err("%s enable_vbus\n", __func__);
	
	Charger_Detect_Release();
	sy697x_set_hiz(sy->chg_dev, false);
	ret = sy697x_update_bits(sy, SY697X_CHRG_CTRL_3, SY697X_OTG_EN,
                     SY697X_OTG_EN);
	return ret;
}

static int sy697x_disable_vbus(struct regulator_dev *rdev)
{
	int ret = 0;
	struct sy697x_device *sy = charger_get_data(s_chg_dev_otg);
	pr_err("%s disable_vbus\n", __func__);

	ret = sy697x_update_bits(sy, SY697X_CHRG_CTRL_3, SY697X_OTG_EN,
                     0);

	return ret;
}

static int sy697x_is_enabled_vbus(struct regulator_dev *rdev)
{
	u8 temp = 0;
	int ret = 0;
	struct sy697x_device *sy = charger_get_data(s_chg_dev_otg);
	pr_err("%s -----\n", __func__);

	ret = sy697x_read_reg(sy, SY697X_CHRG_CTRL_3, &temp);
	return (temp & SY697X_OTG_EN)? 1 : 0;
}

static int sy697x_enable_otg(struct charger_device *chg_dev, bool en)
{
	int ret = 0;

	pr_info("%s en = %d\n", __func__, en);
	if (en) {
		ret = sy697x_enable_vbus(NULL);
	} else {
		ret = sy697x_disable_vbus(NULL);
	}
	return ret;
}

int sy697x_set_ieoc(struct charger_device *chg_dev, u32 uA)
{
    int ret = 0;
    struct sy697x_device *sy = charger_get_data(s_chg_dev_otg);

    pr_err("%s sy697x_set_ieoc %d\n", __func__, uA);

    ret = sy697x_set_term_curr(sy, uA);

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
	ret = sgm4154x_update_bits(sgm, SGM4154x_CHRG_CTRL_6,
				  SGM4154x_BOOSTV, reg_val);

	return ret;
}

static int sgm4154x_boost_get_voltage_sel(struct regulator_dev *rdev)
{
	int ret;
	u8 vlim;
	struct sgm4154x_device *sgm = charger_get_data(s_chg_dev_otg);

	ret = sgm4154x_read_reg(sgm, SGM4154x_CHRG_CTRL_6, &vlim);
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
		ret = sgm4154x_update_bits(sgm, SGM4154x_CHRG_CTRL_2, SGM4154x_BOOST_LIM,
                     0);
	}

	else if (uA == BOOST_CURRENT_LIMIT[1]){
		ret = sgm4154x_update_bits(sgm, SGM4154x_CHRG_CTRL_2, SGM4154x_BOOST_LIM,
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

	ret = sgm4154x_read_reg(sgm, SGM4154x_CHRG_CTRL_2, &vlim);
	if (ret)
		return ret;
	if (!!(vlim & SGM4154x_BOOST_LIM))
		curr = 2000000;
	else
		curr = 1200000;
	return curr ;
}
#endif

static struct regulator_ops sy697x_vbus_ops = {
	.enable = sy697x_enable_vbus,
	.disable = sy697x_disable_vbus,
	.is_enabled = sy697x_is_enabled_vbus,
/*	.list_voltage = regulator_list_voltage_linear,
	.set_voltage_sel = sgm4154x_set_boost_voltage_limit,
	.get_voltage_sel = sgm4154x_boost_get_voltage_sel,
	.set_current_limit = sgm4154x_set_boost_current_limit,
	.get_current_limit = sgm4154x_boost_get_current_limit,*/

};

static const struct regulator_desc sy697x_otg_rdesc = {
	.of_match = "sy697x,otg-vbus",
	.name = "usb-otg-vbus",
	.ops = &sy697x_vbus_ops,
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

static int sy697x_vbus_regulator_register(struct sy697x_device *sy)
{
	struct regulator_config config = {};
	int ret = 0;
	/* otg regulator */
	config.dev = sy->dev;
	config.driver_data = sy;
	sy->otg_rdev = devm_regulator_register(sy->dev,
						&sy697x_otg_rdesc, &config);
	if (IS_ERR(sy->otg_rdev)) {
		ret = PTR_ERR(sy->otg_rdev);
		pr_info("%s: register otg regulator failed (%d)\n", __func__, ret);
	}
    pr_info("%s: register otg regulator success (%d)\n", __func__, ret);
	sy->otg_rdev->constraints->valid_ops_mask |= REGULATOR_CHANGE_STATUS;
	return ret;
}

static int sy697x_do_event(struct charger_device *chg_dev, u32 event, u32 args)
{
    if (chg_dev == NULL)
        return -EINVAL;
    pr_info("%s: event = %d\n", __func__, event);
#ifdef CHARGER_EVENT_USE
    switch (event) {
        case EVENT_EOC: 
            charger_dev_notify(chg_dev, CHARGER_DEV_NOTIFY_EOC);
            break;
        case EVENT_RECHARGE:
            charger_dev_notify(chg_dev, CHARGER_DEV_NOTIFY_RECHG);
            break;
        default:
            break;
    }
#endif
    return 0;
}

static int sy697x_enable_termination(struct charger_device *chg_dev, bool en)
{
    int ret;
    u8 val;
    struct sy697x_device *sy = dev_get_drvdata(&chg_dev->dev);
    if (en) {
        val = SY6970X_TERM_ENABLE << SY6970X_EN_TERM_SHIFT;
    } else {
        val = SY6970X_TERM_DISABLE << SY6970X_EN_TERM_SHIFT;
    }
    ret = sy697x_update_bits(sy, SY697X_CHRG_CTRL_7,
                    SY6970X_EN_TERM_MASK, val);
    return ret;
}

static struct charger_ops sy697x_chg_ops = {
	.enable_hz = sy697x_set_hiz,
	/* Normal charging */
	.dump_registers = sy697x_dump_register,
	.enable = sy697x_charging_switch,
	.get_charging_current = sy697x_get_ichrg_curr,
	.set_charging_current = sy697x_set_ichrg_curr,
	.get_input_current = sy697x_get_input_curr_lim,
	.set_input_current = sy697x_set_input_curr_lim,
	.get_constant_voltage = sy697x_get_chrg_volt,
	.set_constant_voltage = sy697x_set_chrg_volt,
	.kick_wdt = sy697x_reset_watch_dog_timer,
	.set_mivr = sy697x_set_input_volt_lim,
	.is_charging_done = sy697x_get_charging_status,
	.enable_termination = sy697x_enable_termination,
	.set_eoc_current = sy697x_set_ieoc,
	.get_charger_type_hvdcp = sy697x_get_hvdcp_type,
	/* Safety timer */
	.enable_safety_timer = sy697x_enable_safetytimer,
	.is_safety_timer_enabled = sy697x_get_is_safetytimer_enable,

	/* Power path */
	.enable_powerpath = sy697x_enable_powerpath,
	/*.is_powerpath_enabled = sgm4154x_get_is_power_path_enable, */

	/* Charger type detection */
	.enable_chg_type_det = sy697x_enable_chg_type_det,

	/* OTG */
	.enable_otg = sy697x_enable_otg,	
	.set_boost_current_limit = sy697x_set_boost_current_limit,
	.event = sy697x_do_event,
	
	/* PE+/PE+20 */
	.send_ta_current_pattern = sy697x_en_pe_current_partern,
	/*.set_pe20_efficiency_table = NULL,*/
	/*.send_ta20_current_pattern = NULL,*/
	/*.set_ta20_reset = NULL,*/
	/*.enable_cable_drop_comp = NULL,*/
};

static int sy697x_driver_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	int ret = 0;
	struct device *dev = &client->dev;
	struct sy697x_device *sy;

    	char *name = NULL;
	
	pr_info("[%s]\n", __func__);

	sy = devm_kzalloc(dev, sizeof(*sy), GFP_KERNEL);
	if (!sy)
		return -ENOMEM;

	sy->client = client;
	sy->dev = dev;	
	
	mutex_init(&sy->lock);
	mutex_init(&sy->i2c_rw_lock);
	
	i2c_set_clientdata(client, sy);
	
    ret = sy697x_hw_chipid_detect(sy);
    if (ret < 0) {
        pr_info("[%s] device not found !!!\n", __func__);
        return ret;
    }

	ret = sy697x_parse_dt(sy);
	if (ret)
		return ret;
	
	name = devm_kasprintf(sy->dev, GFP_KERNEL, "%s","sy697x suspend wakelock");
	sy->charger_wakelock =	wakeup_source_register(NULL,name);
	
	/* Register charger device */
	sy->chg_dev = charger_device_register("primary_chg",
						&client->dev, sy,
						&sy697x_chg_ops,
						&sy->sy697x_chg_props);
	if (IS_ERR_OR_NULL(sy->chg_dev)) {
		pr_info("%s: register charger device  failed\n", __func__);
		ret = PTR_ERR(sy->chg_dev);
		return ret;
	}    

	/* otg regulator */
	s_chg_dev_otg=sy->chg_dev;
	
	INIT_DELAYED_WORK(&sy->charge_detect_delayed_work, charger_detect_work_func);
	INIT_DELAYED_WORK(&sy->charge_monitor_work, charger_monitor_work_func);
	INIT_DELAYED_WORK(&sy->apsd_work, chg_type_apsd_work_func);
	INIT_DELAYED_WORK(&sy->psy_work, wt6670f_get_charger_type_func_work);

	if (client->irq) {
		ret = devm_request_threaded_irq(dev, client->irq, NULL,
						sy697x_irq_handler_thread,
						IRQF_TRIGGER_FALLING |
						IRQF_ONESHOT,
						dev_name(&client->dev), sy);
		if (ret)
			return ret;
		enable_irq_wake(client->irq);
	}	
	
	ret = sy697x_power_supply_init(sy, dev);
	if (ret) {
		pr_err("Failed to register power supply\n");
		return ret;
	}

	/* Not RESET */
	/* ret =  __sy697x_write_byte(sy, SY697X_CHRG_CTRL_14, 0x80); */

	ret = sy697x_hw_init(sy);
	if (ret) {
		dev_err(dev, "Cannot initialize the chip.\n");
		return ret;
	}


	//OTG setting
	//sgm4154x_set_otg_voltage(s_chg_dev_otg, 5000000); //5V
	//sgm4154x_set_otg_current(s_chg_dev_otg, 1200000); //1.2A
	sy->chg_consumer = charger_manager_get_by_name(sy->dev, "primary_chg");
	if(!sy->chg_consumer) {
		pr_err("%s: get chg_consumer failed\n", __func__);
	}

	ret = sy697x_vbus_regulator_register(sy);
	if (ret < 0) {
		dev_err(dev, "failed to init regulator\n");
		return ret;
	}

	schedule_delayed_work(&sy->charge_monitor_work,100);

	return ret;
}

static int sy697x_charger_remove(struct i2c_client *client)
{
	struct sy697x_device *sy = i2c_get_clientdata(client);

	cancel_delayed_work_sync(&sy->charge_monitor_work);

	regulator_unregister(sy->otg_rdev);

	power_supply_unregister(sy->charger);

	mutex_destroy(&sy->lock);
	mutex_destroy(&sy->i2c_rw_lock);
	return 0;
}

static void sy697x_charger_shutdown(struct i2c_client *client)
{
	int ret = 0;
	struct sy697x_device *sy = i2c_get_clientdata(client);

	pr_info("sy697x_charger_shutdown\n");

	ret =  __sy697x_write_byte(sy, SY697X_CHRG_CTRL_14, 0x80);
	if (ret < 0) {
		pr_err("sy697x register reset fail\n");
	}

	ret = sy697x_update_bits(sy, SY697X_CHRG_CTRL_2, 0x08, 0x0);
	if (ret) {
		pr_err("Failed to disable hvdcp\n");
	}

	ret = sy697x_disable_charger(sy);
	if (ret) {
        	pr_err("Failed to disable charger, ret = %d\n", ret);
    	}
}

static const struct i2c_device_id sy697x_i2c_ids[] = {
	{ "sy6970", 0 },
	{},
};
MODULE_DEVICE_TABLE(i2c, sy697x_i2c_ids);

static const struct of_device_id sy697x_of_match[] = {
	{ .compatible = "sy,sy697x", },
	{ },
};
MODULE_DEVICE_TABLE(of, sy697x_of_match);


static struct i2c_driver sy697x_driver = {
	.driver = {
		.name = "sy697x_charger",
		.of_match_table = sy697x_of_match,		
	},
	.probe = sy697x_driver_probe,
	.remove = sy697x_charger_remove,
	.shutdown = sy697x_charger_shutdown,
	.id_table = sy697x_i2c_ids,
};
module_i2c_driver(sy697x_driver);

MODULE_AUTHOR(" chan <chan@silergy.com>");
MODULE_DESCRIPTION("sy697x charger driver");
MODULE_LICENSE("GPL v2");

