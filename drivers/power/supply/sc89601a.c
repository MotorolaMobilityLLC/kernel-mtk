/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 Southchip Semiconductor Technology(Shanghai) Co., Ltd.
 */
#define pr_fmt(fmt)	"[sc89601a]:%s: " fmt, __func__

#include <linux/init.h>	/* For init/exit macros */
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

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#endif
#include <mt-plat/upmu_common.h>
//#include "mtk_charger_intf.h"
#include "charger_class.h"

#include <mt-plat/v1/charger_type.h>
#include "mtk_charger.h"
#include <linux/alarmtimer.h>

//#include <platform/mtk_charger_intf.h>
#include "sc89601a.h"

static  char charge_ic_vendor_name[50]="SC89601A";
DEV_ATTR_DECLARE(charge_ic)
DEV_ATTR_DEFINE("vendor",charge_ic_vendor_name)
DEV_ATTR_DECLARE_END;
ONTIM_DEBUG_DECLARE_AND_INIT(charge_ic,charge_ic,8);

#if 1
#undef pr_debug
#define pr_debug pr_err
#undef pr_info
#define pr_info pr_err
#undef dev_dbg
#define dev_dbg dev_err
#else
#undef pr_info
#define pr_info pr_debug
#endif

enum sc89601a_part_no {
	SC89601A = 0x05,
};

enum {
    BOOSTI_500 = 500,
    BOOSTI_1200 = 1200,
};

struct chg_para{
	int vlim;
	int ilim;

	int vreg;
	int ichg;
};

struct sc89601a_platform_data {
	int iprechg;
	int iterm;
	int statctrl;
	int vac_ovp;

	int boostv;
	int boosti;

	struct chg_para usb;
};

enum sc89601a_charge_state {
	CHARGE_STATE_IDLE = SC89601A_CHRG_STAT_IDLE,
	CHARGE_STATE_PRECHG = SC89601A_CHRG_STAT_PRECHG,
	CHARGE_STATE_FASTCHG = SC89601A_CHRG_STAT_FASTCHG,
	CHARGE_STATE_CHGDONE = SC89601A_CHRG_STAT_CHGDONE,
};


struct sc89601a {
	struct device *dev;
	struct i2c_client *client;

	enum sc89601a_part_no part_no;
	int revision;
	int status;
	struct mutex i2c_rw_lock;

	bool charge_enabled;/* Register bit status */

	struct sc89601a_platform_data* platform_data;
	struct charger_device *chg_dev;
};

static const struct charger_properties sc89601a_chg_props = {
	.alias_name = "sc89601a",
};

static int __sc89601a_read_reg(struct sc89601a* sc, u8 reg, u8 *data)
{
	s32 ret;

	ret = i2c_smbus_read_byte_data(sc->client, reg);
	if (ret < 0) {
		pr_err("i2c read fail: can't read from reg 0x%02X\n", reg);
		return ret;
	}
	*data = (u8)ret;

	return 0;
}

static int __sc89601a_write_reg(struct sc89601a* sc, int reg, u8 val)
{
	s32 ret;
	ret = i2c_smbus_write_byte_data(sc->client, reg, val);
	if (ret < 0) {
		pr_err("i2c write fail: can't write 0x%02X to reg 0x%02X: %d\n",
				val, reg, ret);
		return ret;
	}
	return 0;
}

static int sc89601a_read_byte(struct sc89601a *sc, u8 *data, u8 reg)
{
	int ret;

	mutex_lock(&sc->i2c_rw_lock);
	ret = __sc89601a_read_reg(sc, reg, data);
	mutex_unlock(&sc->i2c_rw_lock);

	return ret;
}

static int sc89601a_write_byte(struct sc89601a *sc, u8 reg, u8 data)
{
	int ret;

	mutex_lock(&sc->i2c_rw_lock);
	ret = __sc89601a_write_reg(sc, reg, data);
	mutex_unlock(&sc->i2c_rw_lock);

	if (ret) {
		pr_err("Failed: reg=%02X, ret=%d\n", reg, ret);
	}

	return ret;
}

static int sc89601a_update_bits(struct sc89601a *sc, u8 reg,
				u8 mask, u8 data)
{
	int ret;
	u8 tmp;

	mutex_lock(&sc->i2c_rw_lock);
	ret = __sc89601a_read_reg(sc, reg, &tmp);
	if (ret) {
		pr_err("Failed: reg=%02X, ret=%d\n", reg, ret);
		goto out;
	}
	tmp &= ~mask;
	tmp |= data & mask;

	ret = __sc89601a_write_reg(sc, reg, tmp);
	if (ret) {
		pr_err("Failed: reg=%02X, ret=%d\n", reg, ret);
	}
out:
	mutex_unlock(&sc->i2c_rw_lock);
	return ret;
}

static int sc89601a_enable_otg(struct sc89601a *sc)
{
	u8 val = SC89601A_OTG_ENABLE << SC89601A_OTG_CONFIG_SHIFT;
	pr_info("sc89601a_enable_otg enter\n");
	return sc89601a_update_bits(sc, SC89601A_REG_03,
				SC89601A_OTG_CONFIG_MASK, val);

}

static int sc89601a_disable_otg(struct sc89601a *sc)
{
	u8 val = SC89601A_OTG_DISABLE << SC89601A_OTG_CONFIG_SHIFT;
	pr_info("sc89601a_disable_otg enter\n");
	return sc89601a_update_bits(sc, SC89601A_REG_03,
				   SC89601A_OTG_CONFIG_MASK, val);

}

static int sc89601a_enable_charger(struct sc89601a *sc)
{
	int ret;
	u8 val = SC89601A_CHG_ENABLE << SC89601A_CHG_CONFIG_SHIFT;

	ret = sc89601a_update_bits(sc, SC89601A_REG_03, SC89601A_CHG_CONFIG_MASK, val);

	return ret;
}

static int sc89601a_disable_charger(struct sc89601a *sc)
{
	int ret;
	u8 val = SC89601A_CHG_DISABLE << SC89601A_CHG_CONFIG_SHIFT;

	ret = sc89601a_update_bits(sc, SC89601A_REG_03, SC89601A_CHG_CONFIG_MASK, val);
	return ret;
}

int sc89601a_set_chargecurrent(struct sc89601a *sc, int curr)
{
	u8 ichg;

	if (curr < SC89601A_ICHG_BASE)
		curr = SC89601A_ICHG_BASE;

	ichg = (curr - SC89601A_ICHG_BASE)/SC89601A_ICHG_LSB;
	return sc89601a_update_bits(sc, SC89601A_REG_04, SC89601A_ICHG_MASK,
				ichg << SC89601A_ICHG_SHIFT);

}

int sc89601a_set_term_current(struct sc89601a *sc, int curr)
{
	u8 iterm;

	if (curr < SC89601A_ITERM_BASE)
		curr = SC89601A_ITERM_BASE;

	iterm = (curr - SC89601A_ITERM_BASE) / SC89601A_ITERM_LSB;

	return sc89601a_update_bits(sc, SC89601A_REG_05, SC89601A_ITERM_MASK,
				iterm << SC89601A_ITERM_SHIFT);
}
EXPORT_SYMBOL_GPL(sc89601a_set_term_current);


int sc89601a_set_prechg_current(struct sc89601a *sc, int curr)
{
	u8 iprechg;

	if (curr < SC89601A_IPRECHG_BASE)
		curr = SC89601A_IPRECHG_BASE;

	iprechg = (curr - SC89601A_IPRECHG_BASE) / SC89601A_IPRECHG_LSB;

	return sc89601a_update_bits(sc, SC89601A_REG_05, SC89601A_IPRECHG_MASK,
				iprechg << SC89601A_IPRECHG_SHIFT);
}
EXPORT_SYMBOL_GPL(sc89601a_set_prechg_current);

int sc89601a_set_chargevolt(struct sc89601a *sc, int volt)
{
	u8 val;

	if (volt < SC89601A_VREG_BASE)
		volt = SC89601A_VREG_BASE;

	val = (volt - SC89601A_VREG_BASE)/SC89601A_VREG_LSB;
	return sc89601a_update_bits(sc, SC89601A_REG_06, SC89601A_VREG_MASK,
				val << SC89601A_VREG_SHIFT);
}


int sc89601a_set_input_volt_limit(struct sc89601a *sc, int volt)
{
	u8 val;

	if (volt < SC89601A_VINDPM_BASE)
		volt = SC89601A_VINDPM_BASE;

	val = (volt - SC89601A_VINDPM_BASE) / SC89601A_VINDPM_LSB;
	return sc89601a_update_bits(sc, SC89601A_REG_0D, SC89601A_VINDPM_MASK,
				val << SC89601A_VINDPM_SHIFT);
}

int sc89601a_set_input_current_limit(struct sc89601a *sc, int curr)
{
	u8 val;

	if (curr < SC89601A_IINLIM_BASE)
		curr = SC89601A_IINLIM_BASE;

	val = (curr - SC89601A_IINLIM_BASE) / SC89601A_IINLIM_LSB;
	return sc89601a_update_bits(sc, SC89601A_REG_00, SC89601A_IINLIM_MASK,
				val << SC89601A_IINLIM_SHIFT);
}


int sc89601a_set_watchdog_timer(struct sc89601a *sc, u8 timeout)
{
	u8 temp;

	temp = (u8)(((timeout - SC89601A_WDT_BASE) / SC89601A_WDT_LSB) << SC89601A_WDT_SHIFT);

	return sc89601a_update_bits(sc, SC89601A_REG_07, SC89601A_WDT_MASK, temp); 
}
EXPORT_SYMBOL_GPL(sc89601a_set_watchdog_timer);

int sc89601a_disable_watchdog_timer(struct sc89601a *sc)
{
	u8 val = SC89601A_WDT_DISABLE << SC89601A_WDT_SHIFT;

	return sc89601a_update_bits(sc, SC89601A_REG_07, SC89601A_WDT_MASK, val);
}
EXPORT_SYMBOL_GPL(sc89601a_disable_watchdog_timer);

int sc89601a_reset_watchdog_timer(struct sc89601a *sc)
{
	u8 val = SC89601A_WDT_RESET << SC89601A_WDT_RESET_SHIFT;

	return sc89601a_update_bits(sc, SC89601A_REG_03, SC89601A_WDT_RESET_MASK, val);
}
EXPORT_SYMBOL_GPL(sc89601a_reset_watchdog_timer);

int sc89601a_reset_chip(struct sc89601a *sc)
{
	int ret;
	u8 val = SC89601A_RESET << SC89601A_RESET_SHIFT;

	ret = sc89601a_update_bits(sc, SC89601A_REG_14, SC89601A_RESET_MASK, val);
	return ret;
}
EXPORT_SYMBOL_GPL(sc89601a_reset_chip);

int sc89601a_enter_hiz_mode(struct sc89601a *sc)
{
	u8 val = SC89601A_HIZ_ENABLE << SC89601A_ENHIZ_SHIFT;

	return sc89601a_update_bits(sc, SC89601A_REG_00, SC89601A_ENHIZ_MASK, val);
}
EXPORT_SYMBOL_GPL(sc89601a_enter_hiz_mode);

int sc89601a_exit_hiz_mode(struct sc89601a *sc)
{
	u8 val = SC89601A_HIZ_DISABLE << SC89601A_ENHIZ_SHIFT;

	return sc89601a_update_bits(sc, SC89601A_REG_00, SC89601A_ENHIZ_MASK, val);
}
EXPORT_SYMBOL_GPL(sc89601a_exit_hiz_mode);

int sc89601a_set_ico_enable(struct sc89601a *sc, bool enable)
{
	if (enable) {
		return sc89601a_update_bits(sc, SC89601A_REG_02, 0x10, 0x10);
	} else {
		return sc89601a_update_bits(sc, SC89601A_REG_02, 0x10, 0x00);
	}
}

static int sc89601a_set_vindpm_os(struct sc89601a *sc, int mv)
{
	if (mv == 400) {
		return sc89601a_update_bits(sc, SC89601A_REG_01, 0x01, 0x00);
	} else {
		return sc89601a_update_bits(sc, SC89601A_REG_01, 0x01, 0x01);
	}
}

int sc89601a_get_hiz_mode(struct sc89601a *sc, u8 *state)
{
	u8 val;
	int ret;

	ret = sc89601a_read_byte(sc, &val, SC89601A_REG_00);
	if (ret)
		return ret;
	*state = (val & SC89601A_ENHIZ_MASK) >> SC89601A_ENHIZ_SHIFT;

	return 0;
}
EXPORT_SYMBOL_GPL(sc89601a_get_hiz_mode);


static int sc89601a_enable_term(struct sc89601a* sc, bool enable)
{
	u8 val;
	int ret;

	if (enable)
		val = SC89601A_TERM_ENABLE << SC89601A_EN_TERM_SHIFT;
	else
		val = SC89601A_TERM_DISABLE << SC89601A_EN_TERM_SHIFT;

	ret = sc89601a_update_bits(sc, SC89601A_REG_07, SC89601A_EN_TERM_MASK, val);

	return ret;
}
EXPORT_SYMBOL_GPL(sc89601a_enable_term);

int sc89601a_set_boost_current(struct sc89601a *sc, int curr)
{
	u8 val;

	val = SC89601A_BOOST_LIM_1200MA;
	if (curr == BOOSTI_500)
		val = SC89601A_BOOST_LIM_500MA;

	return sc89601a_update_bits(sc, SC89601A_REG_0A, SC89601A_BOOST_LIM_MASK, 
				val << SC89601A_BOOST_LIM_SHIFT);
}

void sc89601a_set_sys_min(struct sc89601a *sc, unsigned int val)
{
	sc89601a_update_bits(sc, (unsigned char)(SC89601A_REG_03),
				(unsigned char)(SC89601A_SYS_MINV_MASK),
				(unsigned char)(val << SC89601A_SYS_MINV_SHIFT)
				);
}

void sc89601a_set_batlowv(struct sc89601a *sc, unsigned int val)
{
	sc89601a_update_bits(sc, (unsigned char)(SC89601A_REG_06),
				(unsigned char)(SC89601A_BATLOWV_MASK),
				(unsigned char)(val << SC89601A_VREG_SHIFT)
				);
}

void sc89601a_set_vrechg(struct sc89601a *sc, unsigned int val)
{
	sc89601a_update_bits(sc, (unsigned char)(SC89601A_REG_06),
				(unsigned char)(SC89601A_VRECHG_MASK),
				(unsigned char)(val << SC89601A_VRECHG_SHIFT)
				);
}

void sc89601a_set_charge_timer(struct sc89601a *sc, unsigned int val)
{
	sc89601a_update_bits(sc, (unsigned char)(SC89601A_REG_07),
				(unsigned char)(SC89601A_CHG_TIMER_MASK),
				(unsigned char)(val << SC89601A_CHG_TIMER_SHIFT)
				);
}

int sc89601a_set_boost_voltage(struct sc89601a *sc, int volt)
{
	u8 val;

	if (volt < SC89601A_BOOSTV_BASE) {
		volt = SC89601A_BOOSTV_BASE;
	}

	val = (volt - SC89601A_BOOSTV_BASE) / SC89601A_BOOSTV_LSB;

	return sc89601a_update_bits(sc, SC89601A_REG_0A, SC89601A_BOOSTV_MASK, 
				val << SC89601A_BOOSTV_SHIFT);
}
EXPORT_SYMBOL_GPL(sc89601a_set_boost_voltage);

static int sc89601a_set_acovp_threshold(struct sc89601a *sc, int volt)
{
	//defualt value is 14v
	return 0;
}
EXPORT_SYMBOL_GPL(sc89601a_set_acovp_threshold);

static int sc89601a_set_stat_ctrl(struct sc89601a *sc, int ctrl)
{
	return 0;
}


static int sc89601a_enable_batfet(struct sc89601a *sc)
{
	const u8 val = SC89601A_BATFET_ON << SC89601A_BATFET_DIS_SHIFT;

	return sc89601a_update_bits(sc, SC89601A_REG_09, SC89601A_BATFET_DIS_MASK,
				val);
}
EXPORT_SYMBOL_GPL(sc89601a_enable_batfet);


static int sc89601a_disable_batfet(struct sc89601a *sc)
{
	const u8 val = SC89601A_BATFET_OFF << SC89601A_BATFET_DIS_SHIFT;

	return sc89601a_update_bits(sc, SC89601A_REG_09, SC89601A_BATFET_DIS_MASK,
				val);
}
EXPORT_SYMBOL_GPL(sc89601a_disable_batfet);

static int sc89601a_set_batfet_delay(struct sc89601a *sc, uint8_t delay)
{
	u8 val;

	if (delay == 0)
		val = SC89601A_BATFET_DLY_0S;
	else
		val = SC89601A_BATFET_DLY_10S;

	val <<= SC89601A_BATFET_DLY_SHIFT;

	return sc89601a_update_bits(sc, SC89601A_REG_09, SC89601A_BATFET_DLY_MASK,
								val);
}
EXPORT_SYMBOL_GPL(sc89601a_set_batfet_delay);

static int sc89601a_enable_safety_timer(struct sc89601a *sc)
{
	const u8 val = SC89601A_CHG_TIMER_ENABLE << SC89601A_EN_TIMER_SHIFT;

	return sc89601a_update_bits(sc, SC89601A_REG_07, SC89601A_EN_TIMER_MASK,
				val);
}
EXPORT_SYMBOL_GPL(sc89601a_enable_safety_timer);


static int sc89601a_disable_safety_timer(struct sc89601a *sc)
{
	const u8 val = SC89601A_CHG_TIMER_DISABLE << SC89601A_EN_TIMER_SHIFT;

	return sc89601a_update_bits(sc, SC89601A_REG_07, SC89601A_EN_TIMER_MASK,
				val);
}
EXPORT_SYMBOL_GPL(sc89601a_disable_safety_timer);

static struct sc89601a_platform_data* sc89601a_parse_dt(struct device *dev,
							struct sc89601a * sc)
{
	int ret;
	struct device_node *np = dev->of_node;
	struct sc89601a_platform_data* pdata;

	pdata = devm_kzalloc(dev, sizeof(struct sc89601a_platform_data),
						GFP_KERNEL);
	if (!pdata) {
		pr_err("Out of memory\n");
		return NULL;
	}
#if 0
#if 0
	ret = of_property_read_u32(np, "mediatek,sc89601a_charger,chip-enable-gpio", &sc->gpio_ce);
	if(ret) {
		pr_err("Failed to read node of mediatek,sc89601a_charger,chip-enable-gpio\n");
	}
#endif

	ret = of_property_read_u32(np,"mediatek,sc89601a_charger,usb-vlim",&pdata->usb.vlim);
	if(ret) {
		pr_err("Failed to read node of mediatek,sc89601a_charger,usb-vlim\n");
	}

	ret = of_property_read_u32(np,"mediatek,sc89601a_charger,usb-ilim",&pdata->usb.ilim);
	if(ret) {
		pr_err("Failed to read node of mediatek,sc89601a_charger,usb-ilim\n");
	}

	ret = of_property_read_u32(np,"mediatek,sc89601a_charger,usb-vreg",&pdata->usb.vreg);
	if(ret) {
		pr_err("Failed to read node of mediatek,sc89601a_charger,usb-vreg\n");
	}

	ret = of_property_read_u32(np,"mediatek,sc89601a_charger,usb-ichg",&pdata->usb.ichg);
	if(ret) {
		pr_err("Failed to read node of mediatek,sc89601a_charger,usb-ichg\n");
	}

	ret = of_property_read_u32(np,"mediatek,sc89601a_charger,stat-pin-ctrl",&pdata->statctrl);
	if(ret) {
		pr_err("Failed to read node of mediatek,sc89601a_charger,stat-pin-ctrl\n");
	}

	ret = of_property_read_u32(np,"mediatek,sc89601a_charger,precharge-current",&pdata->iprechg);
	if(ret) {
		pr_err("Failed to read node of mediatek,sc89601a_charger,precharge-current\n");
	}

	ret = of_property_read_u32(np,"mediatek,sc89601a_charger,termination-current",&pdata->iterm);
	if(ret) {
		pr_err("Failed to read node of mediatek,sc89601a_charger,termination-current\n");
	}

	ret = of_property_read_u32(np,"mediatek,sc89601a_charger,boost-voltage",&pdata->boostv);
	if(ret) {
		pr_err("Failed to read node of mediatek,sc89601a_charger,boost-voltage\n");
	}

	ret = of_property_read_u32(np,"mediatek,sc89601a_charger,boost-current",&pdata->boosti);
	if(ret) {
		pr_err("Failed to read node of mediatek,sc89601a_charger,boost-current\n");
	}

	ret = of_property_read_u32(np,"mediatek,sc89601a_charger,vac-ovp-threshold",&pdata->vac_ovp);
	if(ret) {
		pr_err("Failed to read node of mediatek,sc89601a_charger,vac-ovp-threshold\n");
	}
#else
	pdata->usb.vlim=4600;
	pdata->usb.ilim = 500;
	pdata->usb.vreg = 4400;
	pdata->usb.ichg = 500;
	pdata->statctrl = 3;
	pdata->iprechg = 540; //预充电540mA
	pdata->iterm = 210;
	pdata->boostv = 5150;
	pdata->boosti = 1200; //boost limit 1.2A
	pdata->vac_ovp = 6500;
#endif

	return pdata;
}

static int sc89601a_init_device(struct sc89601a *sc)
{
	int ret;

	sc89601a_disable_watchdog_timer(sc);

	sc89601a_set_ico_enable(sc, false);
	sc89601a_disable_safety_timer(sc);
	sc89601a_exit_hiz_mode(sc);
	sc89601a_set_input_volt_limit(sc, 4440);	//vindpm 4.44V
	sc89601a_set_input_current_limit(sc, 2000); //input current 2A
	sc89601a_set_sys_min(sc, 0x5);  // sys min 3.5V
	sc89601a_set_batlowv(sc, 0x1);	//battery low 3.0V
	sc89601a_set_vrechg(sc, 0);	    //recharge delta 100mV
	sc89601a_set_chargevolt(sc, 4480);	// VREG 4.4V
	sc89601a_enable_term(sc, SC89601A_TERM_ENABLE);	//enable termination
	sc89601a_set_charge_timer(sc, SC89601A_CHG_TIMER_12HOURS); //safety charge time upto 12h
	sc89601a_set_vindpm_os(sc, 400);

	ret = sc89601a_set_stat_ctrl(sc, sc->platform_data->statctrl);
	if (ret)
		pr_err("Failed to set stat pin control mode, ret = %d\n",ret);

	ret = sc89601a_set_prechg_current(sc, sc->platform_data->iprechg);
	if (ret)
		pr_err("Failed to set prechg current, ret = %d\n",ret);

	ret = sc89601a_set_term_current(sc, sc->platform_data->iterm);
	if (ret)
		pr_err("Failed to set termination current, ret = %d\n",ret);

	ret = sc89601a_set_boost_voltage(sc, sc->platform_data->boostv);
	if (ret)
		pr_err("Failed to set boost voltage, ret = %d\n",ret);

	ret = sc89601a_set_boost_current(sc, sc->platform_data->boosti);
	if (ret)
		pr_err("Failed to set boost current, ret = %d\n",ret);

	ret = sc89601a_set_acovp_threshold(sc, sc->platform_data->vac_ovp);
	if (ret)
		pr_err("Failed to set acovp threshold, ret = %d\n",ret);

	return 0;
}

static int sc89601a_detect_device(struct sc89601a* sc)
{
	int ret;
	u8 data;

	ret = sc89601a_read_byte(sc, &data, SC89601A_REG_14);
	if(ret == 0){
		sc->part_no = (data & SC89601A_PN_MASK) >> SC89601A_PN_SHIFT;
		sc->revision = (data & SC89601A_DEV_REV_MASK) >> SC89601A_DEV_REV_SHIFT;
	}

	pr_info("[%s] part_no=%d revision=%d\n", __func__, sc->part_no, sc->revision);

	return ret;
}

static void sc89601a_dump_regs(struct sc89601a *sc)
{
	int addr;
	u8 val;
	int ret;

	for (addr = 0x0; addr <= 0x14; addr++) {
		ret = sc89601a_read_byte(sc, &val, addr);
		if (ret == 0)
			pr_err("Reg[%.2x] = 0x%.2x\n", addr, val);
	}
}

static ssize_t sc89601a_show_registers(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sc89601a *sc = dev_get_drvdata(dev);
	u8 addr;
	u8 val;
	u8 tmpbuf[200];
	int len;
	int idx = 0;
	int ret ;

	idx = snprintf(buf, PAGE_SIZE, "%s:\n", "sc89601a Reg");
	for (addr = 0x0; addr <= 0x14; addr++) {
		ret = sc89601a_read_byte(sc, &val, addr);
		if (ret == 0) {
			len = snprintf(tmpbuf, PAGE_SIZE - idx,"Reg[%.2x] = 0x%.2x\n", addr, val);
			memcpy(&buf[idx], tmpbuf, len);
			idx += len;
		}
	}

	return idx;
}

static ssize_t sc89601a_store_registers(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct sc89601a *sc = dev_get_drvdata(dev);
	int ret;
	unsigned int reg;
	unsigned int val;

	ret = sscanf(buf, "%x %x", &reg, &val);
	if (ret == 2 && reg <= 0x14) {
		sc89601a_write_byte(sc, (unsigned char)reg, (unsigned char)val);
	}

	return count;
}

static DEVICE_ATTR(registers, S_IRUGO | S_IWUSR, sc89601a_show_registers, sc89601a_store_registers);

static struct attribute *sc89601a_attributes[] = {
	&dev_attr_registers.attr,
	NULL,
};

static const struct attribute_group sc89601a_attr_group = {
	.attrs = sc89601a_attributes,
};

static int sc89601a_set_hizmode(struct charger_device *chg_dev, bool enable)
{

	struct sc89601a *sc = dev_get_drvdata(&chg_dev->dev);
	int ret = 0;
	u8 tmp;

	if (enable)
	{
		ret = sc89601a_enter_hiz_mode(sc);
	}
	else
	{
		ret = sc89601a_exit_hiz_mode(sc);
	}

	ret = sc89601a_read_byte(sc, &tmp, SC89601A_REG_00);

	pr_err("set hiz mode i2c  read reg 0x%02X is 0x%02X  ret :%d\n",SC89601A_REG_00,tmp,ret);

	pr_err("%s set hizmode %s\n", enable ? "enable" : "disable",
				  !ret ? "successfully" : "failed");

	return ret;
}

static int sc89601a_charging(struct charger_device *chg_dev, bool enable)
{

	struct sc89601a *sc = dev_get_drvdata(&chg_dev->dev);
	int ret = 0,rc = 0;
	u8 val;

	if (enable)
	{
		ret = sc89601a_enable_charger(sc);
	}
	else
	{
		ret = sc89601a_disable_charger(sc);
	}
	pr_err("%s charger %s\n", enable ? "enable" : "disable",
				  !ret ? "successfully" : "failed");

	ret = sc89601a_read_byte(sc, &val, SC89601A_REG_03);
	if (!ret && !rc )
		sc->charge_enabled = !!(val & SC89601A_CHG_CONFIG_MASK);

	return ret;
}

static int sc89601a_plug_in(struct charger_device *chg_dev)
{

	int ret;

	pr_info("[%s] enter!", __func__);
	ret = sc89601a_charging(chg_dev, true);

	if (!ret)
		pr_err("Failed to enable charging:%d\n", ret);

	return ret;
}

static int sc89601a_plug_out(struct charger_device *chg_dev)
{
	int ret;

	pr_info("[%s] enter!", __func__);
	ret = sc89601a_charging(chg_dev, false);

	if (!ret)
		pr_err("Failed to disable charging:%d\n", ret);

	return ret;
}

static int sc89601a_dump_register(struct charger_device *chg_dev)
{
	struct sc89601a *sc = dev_get_drvdata(&chg_dev->dev);

	sc89601a_dump_regs(sc);

	return 0;
}

static int sc89601a_is_charging_enable(struct charger_device *chg_dev, bool *en)
{
	struct sc89601a *sc = dev_get_drvdata(&chg_dev->dev);

	*en = sc->charge_enabled;

	return 0;
}

static int sc89601a_is_charging_done(struct charger_device *chg_dev, bool *done)
{
	struct sc89601a *sc = dev_get_drvdata(&chg_dev->dev);
	int ret;
	u8 val;

	ret = sc89601a_read_byte(sc, &val, SC89601A_REG_0B);
	if (!ret) {
		val = val & SC89601A_CHRG_STAT_MASK;
		val = val >> SC89601A_CHRG_STAT_SHIFT;
		*done = (val == SC89601A_CHRG_STAT_CHGDONE);
	}

	return ret;
}

static int sc89601a_set_ichg(struct charger_device *chg_dev, u32 curr)
{
	struct sc89601a *sc = dev_get_drvdata(&chg_dev->dev);
	u8 reg_val;
	int ichg_bef;
	int ret;

	ret = sc89601a_read_byte(sc, &reg_val, SC89601A_REG_04);
	if (!ret) {
		ichg_bef = (reg_val & SC89601A_ICHG_MASK) >> SC89601A_ICHG_SHIFT;
		ichg_bef = ichg_bef * SC89601A_ICHG_LSB + SC89601A_ICHG_BASE;
		if ((ichg_bef <= curr / 1000) &&
			(ichg_bef + SC89601A_ICHG_LSB > curr / 1000)) {
			pr_info("[%s] current has set!\n", __func__, curr);
			return ret;
		}
	}

	pr_info("[%s] curr=%d, ichg_bef = %d\n", __func__, curr, ichg_bef);

	return sc89601a_set_chargecurrent(sc, curr/1000);
}

static int sc89601a_get_ichg(struct charger_device *chg_dev, u32 *curr)
{
	struct sc89601a *sc = dev_get_drvdata(&chg_dev->dev);
	u8 reg_val;
	int ichg;
	int ret;

	ret = sc89601a_read_byte(sc, &reg_val, SC89601A_REG_04);
	if (!ret) {
		ichg = (reg_val & SC89601A_ICHG_MASK) >> SC89601A_ICHG_SHIFT;
		ichg = ichg * SC89601A_ICHG_LSB + SC89601A_ICHG_BASE;
		*curr = ichg * 1000;
	}
	pr_info("[%s] curr=%d\n", __func__, *curr);

	return ret;
}

static int sc89601a_get_min_ichg(struct charger_device *chg_dev, u32 *curr)
{

	*curr = 60 * 1000;

	return 0;
}

static int sc89601a_set_vchg(struct charger_device *chg_dev, u32 volt)
{

	struct sc89601a *sc = dev_get_drvdata(&chg_dev->dev);

	return sc89601a_set_chargevolt(sc, volt/1000);
}

static int sc89601a_get_vchg(struct charger_device *chg_dev, u32 *volt)
{
	struct sc89601a *sc = dev_get_drvdata(&chg_dev->dev);
	u8 reg_val;
	int vchg;
	int ret;

	ret = sc89601a_read_byte(sc, &reg_val, SC89601A_REG_06);
	if (!ret) {
		vchg = (reg_val & SC89601A_VREG_MASK) >> SC89601A_VREG_SHIFT;
		vchg = vchg * SC89601A_VREG_LSB + SC89601A_VREG_BASE;
		*volt = vchg * 1000;
	}

	return ret;
}

static int sc89601a_set_ivl(struct charger_device *chg_dev, u32 volt)
{

	struct sc89601a *sc = dev_get_drvdata(&chg_dev->dev);

	return sc89601a_set_input_volt_limit(sc, volt/1000);

}

static int sc89601a_set_icl(struct charger_device *chg_dev, u32 curr)
{

	struct sc89601a *sc = dev_get_drvdata(&chg_dev->dev);

	return sc89601a_set_input_current_limit(sc, curr/1000);
}

static int sc89601a_get_icl(struct charger_device *chg_dev, u32 *curr)
{
	struct sc89601a *sc = dev_get_drvdata(&chg_dev->dev);
	u8 reg_val;
	int icl;
	int ret;

	ret = sc89601a_read_byte(sc, &reg_val, SC89601A_REG_00);
	if (!ret) {
		icl = (reg_val & SC89601A_IINLIM_MASK) >> SC89601A_IINLIM_SHIFT;
		icl = icl * SC89601A_IINLIM_LSB + SC89601A_IINLIM_BASE;
		*curr = icl * 1000;
	}

	return ret;

}

static int sc89601a_kick_wdt(struct charger_device *chg_dev)
{
	struct sc89601a *sc = dev_get_drvdata(&chg_dev->dev);

	return sc89601a_reset_watchdog_timer(sc);
}

static int sc89601a_set_otg(struct charger_device *chg_dev, bool en)
{
	int ret;
	struct sc89601a *sc = dev_get_drvdata(&chg_dev->dev);

	if (en)
	{
		ret = sc89601a_disable_charger(sc);
		ret = sc89601a_enable_otg(sc);
	}
	else
	{
		ret = sc89601a_disable_otg(sc);
		ret = sc89601a_enable_charger(sc);
	}
	return ret;
}

static int sc89601a_set_safety_timer(struct charger_device *chg_dev, bool en)
{
	struct sc89601a *sc = dev_get_drvdata(&chg_dev->dev);
	int ret;

	if (en)
		ret = sc89601a_enable_safety_timer(sc);
	else
		ret = sc89601a_disable_safety_timer(sc);

	return ret;
}

static int sc89601a_is_safety_timer_enabled(struct charger_device *chg_dev, bool *en)
{
	struct sc89601a *sc = dev_get_drvdata(&chg_dev->dev);
	int ret;
	u8 reg_val;

	ret = sc89601a_read_byte(sc, &reg_val, SC89601A_REG_07);

	if (!ret)
		*en = !!(reg_val & SC89601A_EN_TIMER_MASK);

	return ret;
}


static int sc89601a_set_boost_ilmt(struct charger_device *chg_dev, u32 curr)
{
	struct sc89601a *sc = dev_get_drvdata(&chg_dev->dev);
	int ret;

	ret = sc89601a_set_boost_current(sc, curr/1000);

	return ret;
}

static int sc89601a_do_event(struct charger_device *chg_dev, u32 event,
			    u32 args)
{
	if (chg_dev == NULL)
		return -EINVAL;

	pr_info("%s: event = %d\n", __func__, event);
	switch (event) {
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

static int sc89601a_get_input_current_limit(struct sc89601a *sc_chg, int *curr)
{
	u8 val = 0;
	int ret;

	ret = sc89601a_read_byte(sc_chg, &val, SC89601A_REG_00);
	if (ret < 0) {
		dev_err(sc_chg->dev, "Failed to read register 0x00:%d\n", ret);
		return ret;
	}

	val = (val & SC89601A_IINLIM_MASK) >> SC89601A_IINLIM_SHIFT;

	*curr = (val * SC89601A_IINLIM_LSB) + SC89601A_IINLIM_BASE;

	dev_dbg(sc_chg->dev, "get slave charge: input current: %d\n", *curr);

	return ret;
}

static int sc89601a_pumpx_up(struct sc89601a *sc)
{
	int curr;

	sc89601a_get_input_current_limit(sc, &curr);
	pr_err("[sc89601a] sc89601a_pumpx_up   curr=%d\n",curr);
	sc89601a_set_input_current_limit(sc, 100);
	msleep(100);
	sc89601a_set_input_current_limit(sc, 500);
	msleep(100);
	sc89601a_set_input_current_limit(sc, 100);
	msleep(100);
	sc89601a_set_input_current_limit(sc, 500);
	msleep(100);
	sc89601a_set_input_current_limit(sc, 100);
	msleep(100);
	sc89601a_set_input_current_limit(sc, 500);
	msleep(300);
	sc89601a_set_input_current_limit(sc, 100);
	msleep(100);
	sc89601a_set_input_current_limit(sc, 500);
	msleep(300);
	sc89601a_set_input_current_limit(sc, 100);
	msleep(100);
	sc89601a_set_input_current_limit(sc, 500);
	msleep(300);
	sc89601a_set_input_current_limit(sc, 100);
	msleep(100);
	sc89601a_set_input_current_limit(sc, 500);
	msleep(500);
	sc89601a_set_input_current_limit(sc, 100);
	msleep(100);
	return sc89601a_set_input_current_limit(sc, curr);
}

static int sc89601a_pumpx_dn(struct sc89601a *sc)
{
	int curr;

	sc89601a_get_input_current_limit(sc, &curr);

	sc89601a_set_input_current_limit(sc, 100);
	msleep(100);
	sc89601a_set_input_current_limit(sc, 500);
	msleep(300);
	sc89601a_set_input_current_limit(sc, 100);
	msleep(100);
	sc89601a_set_input_current_limit(sc, 500);
	msleep(300);
	sc89601a_set_input_current_limit(sc, 100);
	msleep(100);
	sc89601a_set_input_current_limit(sc, 500);
	msleep(300);
	sc89601a_set_input_current_limit(sc, 100);
	msleep(100);
	sc89601a_set_input_current_limit(sc, 500);
	msleep(100);
	sc89601a_set_input_current_limit(sc, 100);
	msleep(100);
	sc89601a_set_input_current_limit(sc, 500);
	msleep(100);
	sc89601a_set_input_current_limit(sc, 100);
	msleep(100);
	sc89601a_set_input_current_limit(sc, 500);
	msleep(500);
	sc89601a_set_input_current_limit(sc, 100);
	msleep(100);
	return sc89601a_set_input_current_limit(sc, curr);
}


static int sc89601a_set_ta_current_pattern(struct charger_device *chg_dev, bool is_increase)
{
	unsigned int status = 0;

	if(!chg_dev)
		return -EINVAL;

	struct sc89601a *sc = dev_get_drvdata(&chg_dev->dev);
	pr_err("[sc89601a] sc89601a_set_ta_current_pattern  is_increase=%d",is_increase);
	if(true == is_increase) {
		sc89601a_pumpx_up(sc);
	}else{
		sc89601a_pumpx_dn(sc);
	}

	return status;
}

static int sc89601a_set_ta_reset(struct charger_device *chg_dev)
{
	struct sc89601a *sc = dev_get_drvdata(&chg_dev->dev);

	sc89601a_set_input_current_limit(sc, 100);
	msleep(300);
	sc89601a_set_input_current_limit(sc, 500);
	return 0;
}
static struct charger_ops sc89601a_chg_ops = {
	/* Normal charging */
	//.hiz_mode = sc89601a_set_hizmode,
	.plug_in = sc89601a_plug_in,
	.plug_out = sc89601a_plug_out,
	.dump_registers = sc89601a_dump_register,
	.enable = sc89601a_charging,
	.is_enabled = sc89601a_is_charging_enable,
	.get_charging_current = sc89601a_get_ichg,
	.set_charging_current = sc89601a_set_ichg,
	.get_input_current = sc89601a_get_icl,
	.set_input_current = sc89601a_set_icl,
	.get_constant_voltage = sc89601a_get_vchg,
	.set_constant_voltage = sc89601a_set_vchg,
	.kick_wdt = sc89601a_kick_wdt,
	.set_mivr = sc89601a_set_ivl,
	.is_charging_done = sc89601a_is_charging_done,
	.get_min_charging_current = sc89601a_get_min_ichg,

	/* Safety timer */
	.enable_safety_timer = sc89601a_set_safety_timer,
	.is_safety_timer_enabled = sc89601a_is_safety_timer_enabled,

	/* Power path */
	.enable_powerpath = NULL,
	.is_powerpath_enabled = NULL,

	/* OTG */
	.enable_otg = sc89601a_set_otg,
	.set_boost_current_limit = sc89601a_set_boost_ilmt,
	.enable_discharge = NULL,
	.event = sc89601a_do_event,

	/* PE+/PE+20 */
	.send_ta_current_pattern = sc89601a_set_ta_current_pattern,
	.set_pe20_efficiency_table = NULL,
	.send_ta20_current_pattern = NULL,
	//.set_ta20_reset = NULL,
	.reset_ta = sc89601a_set_ta_reset,
	.enable_cable_drop_comp = NULL,

	/* ADC */
	.get_tchg_adc = NULL,
};

static int sc89601a_charger_probe(struct i2c_client *client,
					const struct i2c_device_id *id)
{
	struct sc89601a *sc;

	int ret;

	if(CHECK_THIS_DEV_DEBUG_AREADY_EXIT()==0)
	{
		return -EIO;
	}

	sc = devm_kzalloc(&client->dev, sizeof(struct sc89601a), GFP_KERNEL);
	if (!sc) {
		pr_err("[sc89601a] Out of memory\n");
		return -ENOMEM;
	}
	client->addr = 0x6b;
	sc->dev = &client->dev;
	sc->client = client;

	i2c_set_clientdata(client, sc);

	mutex_init(&sc->i2c_rw_lock);

	ret = sc89601a_detect_device(sc);
	if(ret) {
		pr_err("[sc89601a] No sc89601a device found!\n");
		return -ENODEV;
	}

	if (client->dev.of_node)
		sc->platform_data = sc89601a_parse_dt(&client->dev, sc);
	else
		sc->platform_data = client->dev.platform_data;

	if (!sc->platform_data) {
		pr_err("[sc89601a] No platform data provided.\n");
		return -EINVAL;
	}

	sc->chg_dev = charger_device_register("primary_chg",
			&client->dev, sc,
			&sc89601a_chg_ops,
			&sc89601a_chg_props);
	if (IS_ERR_OR_NULL(sc->chg_dev)) {
		ret = PTR_ERR(sc->chg_dev);
		goto err_0;
	}

	ret = sc89601a_init_device(sc);
	if (ret) {
		pr_err("[sc89601a] Failed to init device\n");
		return ret;
	}

	ret = sysfs_create_group(&sc->dev->kobj, &sc89601a_attr_group);
	if (ret) {
		dev_err(sc->dev, "[sc89601a] failed to register sysfs. err: %d\n", ret);
	}

	sc89601a_dump_regs(sc);

	REGISTER_AND_INIT_ONTIM_DEBUG_FOR_THIS_DEV();

	pr_err("sc89601a probe successfully, Part Num:%d, Revision:%d\n!",
				sc->part_no, sc->revision);

	return 0;
err_0:
	return ret;
}

static int sc89601a_charger_remove(struct i2c_client *client)
{
	struct sc89601a *sc = i2c_get_clientdata(client);

	mutex_destroy(&sc->i2c_rw_lock);

	sysfs_remove_group(&sc->dev->kobj, &sc89601a_attr_group);

	return 0;
}

static struct of_device_id sc89601a_charger_match_table[] = {
	{.compatible = "mediatek,sc89601a_charger",},
	{},
};
//MODULE_DEVICE_TABLE(of,sc89601a_charger_match_table);

static const struct i2c_device_id sc89601a_charger_id[] = {
	{ "sc89601a-charger", SC89601A },
	{},
};
//MODULE_DEVICE_TABLE(i2c, sc89601a_charger_id);

static struct i2c_driver sc89601a_charger_driver = {
	.driver 	= {
		.name 	= "sc89601a-charger",
		.owner 	= THIS_MODULE,
		.of_match_table = sc89601a_charger_match_table,
	},
	.id_table	= sc89601a_charger_id,

	.probe		= sc89601a_charger_probe,
	.remove		= sc89601a_charger_remove,

};

//module_i2c_driver(sc89601a_charger_driver);

static int __init sc89601a_init(void)
{
	if (i2c_add_driver(&sc89601a_charger_driver) != 0)
		pr_err("[sc89601a] Failed to register sc89601a i2c driver.\n");
	else
		pr_err("[sc89601a] Success to register sc89601a i2c driver.\n");

	return 0;
}

static void __exit sc89601a_exit(void)
{
	i2c_del_driver(&sc89601a_charger_driver);
}

module_init(sc89601a_init);
module_exit(sc89601a_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SC sc89601a Charger Driver");
MODULE_AUTHOR("hehl@antiui.com>");
