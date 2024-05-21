#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/of_irq.h>
#include <linux/of_device.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <linux/debugfs.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/of_regulator.h>
#include <linux/power_supply.h>
#include <linux/pm_runtime.h>
#include <linux/sched/clock.h>
#include <hl7139_charger.h>
#if defined (CONFIG_OF)
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#endif
#include <linux/reboot.h>
#ifdef CONFIG_RT_REGMAP
#include <mt-plat/rt-regmap.h>
#endif /* CONFIG_RT_REGMAP */

#define BITS(_end, _start)          ((BIT(_end) - BIT(_start)) + BIT(_end))
#define MIN(a, b)                   ((a < b) ? (a):(b))

static bool bpcp = HL7139_DEV_MODE_BP_MODE;
static int hl7139_fsw_set(struct hl7139_charger *chg, int hz);
static void hl7139_reg_reset(struct hl7139_charger *chg);

static enum power_supply_usb_type hl7139_usb_type[] = {
	POWER_SUPPLY_USB_TYPE_UNKNOWN,
	POWER_SUPPLY_USB_TYPE_SDP,
	POWER_SUPPLY_USB_TYPE_DCP,
	POWER_SUPPLY_USB_TYPE_CDP,
};

static int hl7139_read_reg(struct hl7139_charger *chg, int reg, void *value)
{
	int ret = 0;
	mutex_lock(&chg->i2c_lock);
	ret = regmap_read(chg->regmap, reg, value);
	mutex_unlock(&chg->i2c_lock);

	if (ret < 0)
		pr_info("%s: reg(0x%x), ret(%d)\n", __func__, reg, ret);
	return ret;
}

static int hl7139_bulk_read_reg(struct hl7139_charger *chg, int reg, void *val, int count)
{
	int ret = 0;

	mutex_lock(&chg->i2c_lock);
	ret = regmap_bulk_read(chg->regmap, reg, val, count); 
	mutex_unlock(&chg->i2c_lock);

	if (ret < 0)
		pr_info("%s: reg(0x%x), ret(%d)\n", __func__, reg, ret);
	return ret;
}

static int hl7139_write_reg(struct hl7139_charger *chg, int reg, u8 value)
{
	int ret = 0;
	mutex_lock(&chg->i2c_lock);
	ret = regmap_write(chg->regmap, reg, value); 
	mutex_unlock(&chg->i2c_lock);

	if (ret < 0)
		pr_info("%s: reg(0x%x), ret(%d)\n", __func__, reg, ret);
	return ret;
}

static int hl7139_update_reg(struct hl7139_charger *chg, int reg, u8 mask, u8 val)
{
	int ret = 0;
	unsigned int temp;

	ret = hl7139_read_reg(chg, reg, &temp);
	if (ret < 0)
		pr_err("failed to read reg 0x%x, ret= %d\n", reg, ret);
	else {
		temp &= ~mask;
		temp |= val & mask;
		ret = hl7139_write_reg(chg, reg, temp);
		if(ret < 0){
			pr_err("failed to write reg0x%x, ret= %d\n", reg, ret);
		}
	}
	return ret;
}

static int hl7139_parse_dt(struct device *dev, struct hl7139_platform_data *pdata)
{
	struct device_node *np = dev->of_node;
	int rc = 0;
	int ret;
	//        u32 temp;

	if (!np)
		return -EINVAL;

	pdata->bat_ovp_disable = of_property_read_bool(np,
			"hl,hl7139,bat-ovp-disable");
	pdata->bat_ocp_disable = of_property_read_bool(np,
			"hl,hl7139,bat-ocp-disable");
	pdata->vin_ovp_disable = of_property_read_bool(np,
			"hl,hl7139,vin-ovp-disable");
	pdata->iin_ocp_disable = of_property_read_bool(np,
			"hl,hl7139,iin-ocp-disable");
	pdata->vout_ovp_disable = of_property_read_bool(np,
			"hl,hl7139,vout-ovp-disable");

	ret = of_property_read_u32(np, "hl,hl7139,bat-ovp-threshold",
			&pdata->bat_ovp_th);
	if (ret) {
		pr_err("failed to read bat-ovp-threshold\n");
		return ret;
	}
	ret = of_property_read_u32(np, "hl,hl7139,bat-ocp-threshold",
			&pdata->bat_ocp_th);
	if (ret) {
		pr_err("failed to read bat-ocp-threshold\n");
		return ret;
	}
	ret = of_property_read_u32(np, "hl,hl7139,vin-ovp-threshold",
			&pdata->ac_ovp_th);
	if (ret) {
		pr_err("failed to read ac-ovp-threshold\n");
		return ret;
	}
	ret = of_property_read_u32(np, "hl,hl7139,bus-ovp-threshold",
			&pdata->bus_ovp_th);
	if (ret) {
		pr_err("failed to read bus-ovp-threshold\n");
		return ret;
	}

	pdata->irq_gpio = of_get_named_gpio(np, "hl7139,irq-gpio", 0);
	//        LOG_DBG("irq-gpio:: %d \n", pdata->irq_gpio);
	pr_info("[%s]:: irq-gpio:: %d \n",__func__,  pdata->irq_gpio);

	rc = of_property_read_u32(np, "hl7139,sw-frequency", &pdata->fsw_cfg);
	if (rc) {
		pr_info("%s: failed to get switching frequency from dtsi", __func__ );
		pdata->fsw_cfg = FSW_CFG_500KHZ;
	}

	rc = of_property_read_u32(np, "hl7139,vbat-reg", &pdata->vbat_reg);
	if (rc) {
		pr_info("%s: failed to get vbat-reg from dtsi", __func__ );
		pdata->vbat_reg = HL7139_VBAT_REG_DFT;
	}

	rc = of_property_read_u32(np, "hl7139,input-current-limit", &pdata->iin_cfg);
	if (rc) {
		pr_info("%s: failed to get input-current-limit from dtsi", __func__ );
		pdata->iin_cfg = HL7139_CP_IIN_CFG_DFT;
	}

	rc = of_property_read_u32(np, "hl7139,topoff-current", &pdata->iin_topoff);
	if (rc) {
		pr_info("%s: failed to get topoff current from dtsi", __func__ );
		pdata->iin_topoff = HL7139_IIN_DONE_DFT;
	}

	rc = of_property_read_u32(np, "hl7139,wd-tmr", &pdata->wd_tmr);
	if (rc) {
		pr_info("%s: failed to get watchdog timer from dtsi", __func__ );
		pdata->wd_tmr = WD_TMR_5S;
	}

	/*
	 *     pdata->ts_prot_en = of_property_read_bool(np, "hl7139,ts-prot-en");
	 *     pdata->all_auto_recovery = of_property_read_bool(np, "hl7139,all-auto-recovery");
	 *     pdata->wd_dis = of_property_read_bool(np, "hl7139,wdt-disable");
	 */

	pdata->ts_prot_en = false;
	pdata->all_auto_recovery = false;
	pdata->wd_dis = true;//false;

	return 0;
}

static int hl7139_enable_charge(struct hl7139_charger *chg, bool enable)
{
	int ret;
	u8 val;

	if (enable)
		val = HL7139_CHG_EN_ENABLE;
	else
		val = HL7139_CHG_EN_DISABLE;

	val <<= HL7139_CHG_EN_SHIFT;

	ret = hl7139_update_reg(chg, HL7139_CTRL0, HL7139_CHG_EN_MASK, val);
	return ret;
}

static int hl7139_check_charge_enabled(struct hl7139_charger *chg, bool *enabled)
{
	int ret;
	u8 val;

	ret = hl7139_read_reg(chg, HL7139_CTRL0, &val);
	pr_info(">>>reg [0x12] = 0x%02x\n", val);
	if (!ret)
		*enabled = !!(val & HL7139_CHG_EN_MASK);
	return ret;
}

static int hl7139_enable_wdt(struct hl7139_charger *chg, bool enable)
{
	int ret;
	u8 val;

	if (enable)
		val = HL7139_WD_DIS_WATDOG_ENABLE;
	else
		val = HL7139_WD_DIS_WATDOG_DISABLE;

	val <<= HL7139_WD_DIS_SHIFT;

	ret = hl7139_update_reg(chg, HL7139_CTRL2, HL7139_WD_DIS_MASK, val);
	return ret;
}

static int hl7139_dump_reg(struct hl7139_charger *chg)
{
	int idx;
	u8 data;
	int ret;

	for (idx = 0; idx < 0x18; idx++) {
		ret = hl7139_read_reg(chg, idx, &data);
		pr_info("%s reg 0x%2x value is 0x%2x\n", __func__, idx, data);
	}

	for (idx = 0x40; idx < 0x4f; idx++) {
		ret = hl7139_read_reg(chg, idx, &data);
		pr_info("%s reg 0x%2x value is 0x%2x\n", __func__, idx, data);
	}

	return 0;
}

static int hl7139_set_wdt(struct hl7139_charger *chg, int ms)
{
	int ret;
	u8 val;

	if (ms == 200)
		val = HL7139_WD_TMR_200MS;
	else if (ms == 500)
		val = HL7139_WD_TMR_500MS;
	else if (ms == 1000)
		val = HL7139_WD_TMR_1000MS;
	else if (ms == 2000)
		val = HL7139_WD_TMR_2000MS;
	else if (ms == 5000)
		val = HL7139_WD_TMR_5000MS;
	else if (ms == 10000)
		val = HL7139_WD_TMR_10000MS;
	else if (ms == 20000)
		val = HL7139_WD_TMR_20000MS;
	else if (ms == 40000)
		val = HL7139_WD_TMR_40000MS;
	else
		return -EINVAL;
	val <<= HL7139_WD_TMR_SHIFT;

	ret = hl7139_update_reg(chg, HL7139_CTRL2,
			HL7139_WD_TMR_MASK, val);
	return ret;
}


static int hl7139_enable_batovp(struct hl7139_charger *chg, bool enable)
{
	int ret;
	u8 val;

	if (enable)
		val = HL7139_BAT_OVP_ENABLE;
	else
		val = HL7139_BAT_OVP_DISABLE;

	val <<= HL7139_BAT_OVP_SHIFT;

	ret = hl7139_update_reg(chg, HL7139_VBAT_REG, HL7139_BAT_OVP_MASK, val);
	return ret;
}

static int hl7139_set_batovp_deb(struct hl7139_charger *chg, int deb)
{
	int ret;
	u8 val;

	if (deb >= 2000)
		val = HL7139_BAT_OVP_DEB_2MS;
	else 
		val = HL7139_BAT_OVP_DEB_80US;

	val <<= HL7139_BAT_OVP_DEB_SHIFT;

	ret = hl7139_update_reg(chg, HL7139_VBAT_REG, HL7139_BAT_OVP_DEB_MASK, val);
	return ret;
}

static int hl7139_set_batovp_th(struct hl7139_charger *chg, int threshold)
{
	int ret;
	u8 val;

	if (threshold < HL7139_BAT_OVP_BASE)
		threshold = HL7139_BAT_OVP_BASE;

	val = (threshold - HL7139_BAT_OVP_BASE) / HL7139_BAT_OVP_LSB;

	val <<= HL7139_BAT_OVP_SET_SHIFT;

	ret = hl7139_update_reg(chg, HL7139_VBAT_REG, HL7139_BAT_OVP_SET_MASK, val);
	return ret;
}

static int hl7139_enable_batocp(struct hl7139_charger *chg, bool enable)
{
	int ret;
	u8 val;

	if (enable)
		val = HL7139_BAT_OCP_ENABLE;
	else
		val = HL7139_BAT_OCP_DISABLE;

	val <<= HL7139_BAT_OCP_SHIFT;

	ret = hl7139_update_reg(chg, HL7139_IBAT_REG, HL7139_BAT_OCP_MASK, val);
	return ret;
}

static int hl7139_set_batocp_deb(struct hl7139_charger *chg, int deb)
{
	int ret;
	u8 val;

	if (deb >= 200) 
		val = HL7139_BAT_OCP_DEB_200US;
	else 
		val = HL7139_BAT_OCP_DEB_80US;

	val <<= HL7139_BAT_OCP_DEB_SHIFT;

	ret = hl7139_update_reg(chg, HL7139_IBAT_REG, HL7139_BAT_OCP_DEB_MASK, val);
	return ret;
}

static int hl7139_set_batocp_th(struct hl7139_charger *chg, int threshold)
{
	int ret;
	u8 val;

	if (threshold < HL7139_BAT_OCP_ALM_BASE)
		threshold = HL7139_BAT_OCP_ALM_BASE;

	val = (threshold - HL7139_BAT_OCP_ALM_BASE) / HL7139_BAT_OCP_ALM_LSB;

	val <<= HL7139_BAT_OCP_ALM_SHIFT;

	ret = hl7139_update_reg(chg, HL7139_IBAT_REG, HL7139_BAT_OCP_ALM_MASK, val);
	return ret;
}

static int hl7139_set_ext_nfet_use(struct hl7139_charger *chg, bool enable)
{
	int ret;
	u8 val;

	if (enable)
		val = HL7139_EXT_NFET_USE_USED;
	else
		val = HL7139_EXT_NFET_USE_NOT_USED;

	val <<= HL7139_VBUS_OVP_TH_SHIFT;

	ret = hl7139_update_reg(chg, HL7139_VBUS_OVP_REG, HL7139_EXT_NFET_USE_MASK, val);
	return ret;
}

static int hl7139_set_vbus_pd_en(struct hl7139_charger *chg, bool enable)
{
	int ret;
	u8 val;

	if (enable)
		val = HL7139_VBUS_PD_EN_ENABLE;
	else
		val = HL7139_VBUS_PD_EN_DISABLE;

	val <<= HL7139_VBUS_PD_EN_SHIFT;

	ret = hl7139_update_reg(chg, HL7139_VBUS_OVP_REG, HL7139_VBUS_PD_EN_MASK, val);
	return ret;
}

static int hl7139_set_vgs_sel(struct hl7139_charger *chg, int vol)
{
	int ret;
	u8 val;

	if (vol <= 7000)
		val = HL7139_VGS_SEL_VOL_7V;
	else if (vol == 7500)
		val = HL7139_VGS_SEL_VOL_75V;
	else if (vol == 8000)
		val = HL7139_VGS_SEL_VOL_8V;
	else if (vol >= 8500)
		val = HL7139_VGS_SEL_VOL_85V;
	else
		return -EINVAL;
	val <<= HL7139_VGS_SEL_SHIFT;

	ret = hl7139_update_reg(chg, HL7139_VBUS_OVP_REG, HL7139_VGS_SEL_MASK, val);
	return ret;
}

static int hl7139_set_busovp_th(struct hl7139_charger *chg, int threshold)
{
	int ret;
	u8 val;

	if (threshold < HL7139_VBUS_OVP_TH_BASE)
		threshold = HL7139_VBUS_OVP_TH_BASE;

	val = (threshold - HL7139_VBUS_OVP_TH_BASE) / HL7139_VBUS_OVP_TH_LSB;

	val <<= HL7139_VBUS_OVP_TH_SHIFT;

	ret = hl7139_update_reg(chg, HL7139_VBUS_OVP_REG, HL7139_VBUS_OVP_TH_MASK, val);
	return ret;
}

static int hl7139_set_vin_pd_cfg(struct hl7139_charger *chg, bool mode)
{
	int ret;
	u8 val;

	if (mode)
		val = HL7139_PD_CFG_ENABLE;
	else
		val = HL7139_PD_CFG_AUTO;

	val <<= HL7139_PD_CFG_SHIFT;

	ret = hl7139_update_reg(chg, HL7139_VIN_OVP_REG, HL7139_PD_CFG_MASK, val);
	return ret;
}

static int hl7139_set_vin_ovp_dis(struct hl7139_charger *chg, bool enable)
{
	int ret;
	u8 val;

	if (!enable)
		val = HL7139_OVP_DIS_ENABLE;
	else
		val = HL7139_OVP_DIS_DISABLE;

	val <<= HL7139_OVP_DIS_SHIFT;

	ret = hl7139_update_reg(chg, HL7139_VIN_OVP_REG, HL7139_OVP_DIS_MASK, enable);
	return ret;
}

static int hl7139_set_vin_ovp_th(struct hl7139_charger *chg, int threshold)
{
	int ret;
	u8 val;

	if (bpcp == HL7139_DEV_MODE_BP_MODE) {
		if (threshold <= 5100)
			threshold = HL7139_OVP_VAL_BP_MIN;
		else if (threshold >= 5850)
			threshold = HL7139_OVP_VAL_BP_MAX;
		else
			return -EINVAL;
		val = (threshold - HL7139_OVP_VAL_BP_MIN) / HL7139_OVP_VAL_BP_LSB;
	} else {
		if (threshold <= 10200)
			threshold = HL7139_OVP_VAL_CP_MIN;
		else if (threshold >= 11700)
			threshold = HL7139_OVP_VAL_CP_MAX;
		else
			return -EINVAL;
		val = (threshold - HL7139_OVP_VAL_CP_MIN) / HL7139_OVP_VAL_CP_LSB;
	}

	pr_info("%s: val 0x%x threshold %d\n", __func__, val, threshold);
	val <<= HL7139_OVP_VAL_SHIFT;

	ret = hl7139_update_reg(chg, HL7139_VIN_OVP_REG, HL7139_OVP_VAL_MASK, val);
	return ret;
}

static int hl7139_set_iin_ocp_dis(struct hl7139_charger *chg, bool enable)
{
	int ret;
	u8 val;

	if (enable)
		val = HL7139_IIN_OCP_DIS_ENABLE;
	else
		val = HL7139_IIN_OCP_DIS_DISABLE;

	val <<= HL7139_IIN_OCP_DIS_SHIFT;

	ret = hl7139_update_reg(chg, HL7139_IIN_REG, HL7139_IIN_OCP_DIS_MASK, val);
	return ret;
}

static int hl7139_set_iin_reg_th(struct hl7139_charger *chg, int threshold)
{
	int ret;
	u8 val;

	if (bpcp == HL7139_DEV_MODE_BP_MODE) {
		if (threshold <= 1000)
			threshold = HL7139_IIN_REG_TH_BP_MIN;
		else if (threshold >= 3500)
			threshold = HL7139_IIN_REG_TH_BP_MAX;
		else
			return -EINVAL;
		val = (threshold - HL7139_IIN_REG_TH_BP_MIN) / HL7139_IIN_REG_TH_BP_LSB;
	} else {
		if (threshold <= 2000)
			threshold = HL7139_IIN_REG_TH_CP_MIN;
		else if (threshold >= 7000)
			threshold = HL7139_IIN_REG_TH_CP_MAX;
		else
			return -EINVAL;
		val = (threshold - HL7139_IIN_REG_TH_CP_MIN) / HL7139_IIN_REG_TH_CP_LSB;
	}

	pr_info("%s: val 0x%x threshold %d\n", __func__, val, threshold);
	val <<= HL7139_IIN_REG_TH_SHIFT;

	ret = hl7139_update_reg(chg, HL7139_IIN_REG, HL7139_IIN_REG_TH_MASK, val);
	return ret;
}

static int hl7139_set_tiin_oc_deb(struct hl7139_charger *chg, int tdeb)
{
	int ret;
	u8 val;

	if (tdeb)
		val = HL7139_IIN_OC_DEB_200US;
	else
		val = HL7139_IIN_OC_DEB_80US;

	val <<= HL7139_IIN_OC_DEB_SHIFT;

	ret = hl7139_update_reg(chg, HL7139_IIN_OC_REG, HL7139_IIN_OC_DEB_MASK, val);
	return ret;

}

static int hl7139_set_iin_ocp_th(struct hl7139_charger *chg, int threshold)
{
	int ret;
	u8 val;

	if (bpcp == HL7139_DEV_MODE_BP_MODE) {
		if (threshold <= 150)
			val = HL7139_IIN_OCP_TH_BP_150MA;
		else if (threshold == 200)
			val = HL7139_IIN_OCP_TH_BP_200MA;
		else if (threshold == 250)
			val = HL7139_IIN_OCP_TH_BP_250MA;
		else if (threshold >= 300)
			val = HL7139_IIN_OCP_TH_BP_300MA;
		else
			return -EINVAL;
	} else {
		if (threshold <= 300)
			val = HL7139_IIN_OCP_TH_CP_300MA;
		else if (threshold == 400)
			val = HL7139_IIN_OCP_TH_CP_400MA;
		else if (threshold == 500)
			val = HL7139_IIN_OCP_TH_CP_500MA;
		else if (threshold >= 600)
			val = HL7139_IIN_OCP_TH_CP_600MA;
		else
			return -EINVAL;
	}

	pr_info("%s: val 0x%x\n", __func__, val);
	val <<= HL7139_IIN_OCP_TH_SHIFT;

	ret = hl7139_update_reg(chg, HL7139_IIN_OC_REG, HL7139_IIN_OCP_TH_MASK, val);
	return ret;

}

static int hl7139_set_tdie_reg_dis(struct hl7139_charger *chg, bool enable)
{
	int ret;
	u8 val;

	if (enable)
		val = HL7139_TDIE_REG_DIS_ENABLE;
	else
		val = HL7139_TDIE_REG_DIS_DISABLE;

	val <<= HL7139_TDIE_REG_DIS_SHIFT;

	ret = hl7139_update_reg(chg, HL7139_REG_CTRL0, HL7139_TDIE_REG_DIS_MASK, val);
	return ret;
}

static int hl7139_set_iin_reg_dis(struct hl7139_charger *chg, bool enable)
{
	int ret;
	u8 val;

	if (enable)
		val = HL7139_IIN_REG_DIS_ENABLE;
	else
		val = HL7139_IIN_REG_DIS_DISABLE;

	val <<= HL7139_IIN_REG_DIS_SHIFT;
	pr_info("%s: val 0x%x\n", __func__, val);

	ret = hl7139_update_reg(chg, HL7139_REG_CTRL0, HL7139_IIN_REG_DIS_MASK, val);
	return ret;
}

static int hl7139_set_tdie_reg_th(struct hl7139_charger *chg, int threshold)
{
	int ret;
	u8 val;

	if (threshold <= 80)
		val = HL7139_TDIE_REG_TH_80;
	else if (threshold == 90)
		val = HL7139_TDIE_REG_TH_90;
	else if (threshold == 100)
		val = HL7139_TDIE_REG_TH_100;
	else if (threshold >= 110)
		val = HL7139_TDIE_REG_TH_110;
	else
		return -EINVAL;

	val <<= HL7139_TDIE_REG_TH_SHIFT;

	ret = hl7139_update_reg(chg, HL7139_REG_CTRL0, HL7139_TDIE_REG_TH_MASK, val);
	return ret;
}

static int hl7139_set_vbat_reg_dis(struct hl7139_charger *chg, bool enable)
{
	int ret;
	u8 val;

	if (enable)
		val = HL7139_VBAT_REG_DIS_ENABLE;
	else
		val = HL7139_VBAT_REG_DIS_DISABLE;

	val <<= HL7139_VBAT_REG_DIS_SHIFT;

	hl7139_read_reg(chg, HL7139_REG_CTRL1, &val);
	pr_err("%s:reg val = %d\n", __func__, val);
	ret = hl7139_update_reg(chg, HL7139_REG_CTRL1, HL7139_VBAT_REG_DIS_MASK, val);
	hl7139_read_reg(chg, HL7139_REG_CTRL1, &val);
	pr_err("%s:reg val = %d\n", __func__, val);
	return ret;
}

static int hl7139_set_ibat_reg_dis(struct hl7139_charger *chg, bool enable)
{
	int ret;
	u8 val;

	if (enable)
		val = HL7139_IBAT_REG_DIS_ENABLE;
	else
		val = HL7139_IBAT_REG_DIS_DISABLE;

	val <<= HL7139_IBAT_REG_DIS_SHIFT;
	hl7139_read_reg(chg, HL7139_REG_CTRL1, &val);
	pr_err("%s:reg val = %d\n", __func__, val);
	ret = hl7139_update_reg(chg, HL7139_REG_CTRL1, HL7139_IBAT_REG_DIS_MASK, val);
	hl7139_read_reg(chg, HL7139_REG_CTRL1, &val);
	pr_err("%s:reg val = %d\n", __func__, val);
	return ret;
}

static int hl7139_set_vbat_ovp_th(struct hl7139_charger *chg, int threshold)
{
	int ret;
	u8 val;

	if (threshold <= 80)
		val = HL7139_VBAT_OVP_TH_80;
	else if (threshold == 90)
		val = HL7139_VBAT_OVP_TH_90;
	else if (threshold == 100)
		val = HL7139_VBAT_OVP_TH_100;
	else if (threshold >= 110)
		val = HL7139_VBAT_OVP_TH_110;
	else
		return -EINVAL;

	val <<= HL7139_VBAT_OVP_TH_SHIFT;

	ret = hl7139_update_reg(chg, HL7139_REG_CTRL1, HL7139_VBAT_OVP_TH_MASK, val);
	return ret;
}

static int hl7139_set_ibat_ovp_th(struct hl7139_charger *chg, int threshold)
{
	int ret;
	u8 val;

	if (threshold <= 200)
		val = HL7139_IBAT_OCP_TH_200;
	else if (threshold == 300)
		val = HL7139_IBAT_OCP_TH_300;
	else if (threshold == 400)
		val = HL7139_IBAT_OCP_TH_400;
	else if (threshold >= 500)
		val = HL7139_IBAT_OCP_TH_500;
	else
		return -EINVAL;

	val <<= HL7139_IBAT_OCP_TH_SHIFT;

	ret = hl7139_update_reg(chg, HL7139_REG_CTRL1, HL7139_IBAT_OCP_TH_MASK, val);
	return ret;
}

static int hl7139_set_dev_mode(struct hl7139_charger *chg, bool mode)
{
	int ret;
	u8 val;

	if (mode)
		val = HL7139_DEV_MODE_BP_MODE;
	else
		val = HL7139_DEV_MODE_CP_MODE;

	bpcp = val;

	val <<= HL7139_DEV_MODE_SHIFT;

	pr_info("%s: val 0x%x\n", __func__, val);
	ret = hl7139_update_reg(chg, HL7139_CTRL3, HL7139_DEV_MODE_MASK, val);
	return ret;
}

static int hl7139_get_dev_mode(struct hl7139_charger *chg, int *result)
{
	int ret;
	u8 val;

	ret = hl7139_read_reg(chg, HL7139_CTRL3, &val);
	if(ret < 0) {
		return ret;
	}
	pr_info("%s: val 0x%x\n", __func__, val);

	*result = (val >> HL7139_DEV_MODE_SHIFT) & HL7139_DEV_MODE_MASK;
	pr_info("%s: result 0x%x\n", __func__, *result);

	return ret;
}

static int hl7139_set_dpdm_cfg(struct hl7139_charger *chg, bool mode)
{
	int ret;
	u8 val;

	if (mode)
		val = HL7139_DPDM_CFG_SYNC_TS;
	else
		val = HL7139_DPDM_CFG_DPDM;

	pr_info("%s: val 0x%x\n", __func__, val);
	val <<= HL7139_DPDM_CFG_SHIFT;

	ret = hl7139_update_reg(chg, HL7139_CTRL3, HL7139_DPDM_CFG_MASK, val);
	return ret;
}

static int hl7139_set_gpp_cfg(struct hl7139_charger *chg, int mode)
{
	int ret;
	u8 val;

	val = (u8)mode;
	val <<= HL7139_DPDM_CFG_SHIFT;

	ret = hl7139_update_reg(chg, HL7139_CTRL3, HL7139_DPDM_CFG_MASK, val);
	return ret;
}

static int hl7139_check_vbus_error_status(struct hl7139_charger *chg, int *result)
{
	int ret;
	u8 reg;
	int tol = 0;

	ret = hl7139_read_reg(chg, HL7139_STATUS_A, &reg);
	tol |= (reg << 16);
	reg = 0;

	ret = hl7139_read_reg(chg, HL7139_STATUS_B, &reg);
	tol |= (reg << 8);
	reg = 0;

	ret = hl7139_read_reg(chg, HL7139_STATUS_C, &reg);  
	tol |= reg;
	*result = tol;
	pr_info("%s: reg STATUS_A/B/C 0x%x tol = 0x%x\n", __func__, reg, tol);
	return ret;
}

static int hl7139_device_init(struct hl7139_charger *chg)
{
	int ret = 0;
	unsigned int value = 0;
	int i;


	pr_err("Start!!\n");

	ret = hl7139_read_reg(chg, REG_DEVICE_ID, &value);      
	if (ret < 0){
		/*Workaround for Raspberry PI*/ 
		ret = hl7139_read_reg(chg, REG_DEVICE_ID, &value);  
		if (ret < 0){
			dev_err(chg->dev, "Read DEVICE_ID failed , val[0x%x\n", value);
			return -EINVAL;
		}
	}

	hl7139_reg_reset(chg);
	pr_info("%s:after reset reg\n", __func__);
	/* REG0x12, FSW setting */
//	value = chg->pdata->fsw_cfg << 3;
//	ret = hl7139_update_reg(chg, REG_CTRL_0, BITS_FSW_SET, value);
	ret = hl7139_fsw_set(chg, chg->pdata->fsw_cfg);
	if (ret < 0)
		goto I2C_FAIL;   

	/* REG0x13 CTRL setting */
	value = chg->pdata->ts_prot_en ? 0x10 : 0x00;
	value = chg->pdata->all_auto_recovery ? (value | 0x07) : (value | 0x00);
	ret = hl7139_write_reg(chg, HL7139_CTRL1, value);
	if (ret < 0)
		goto I2C_FAIL;

	/* REG0x14, WDT setting */ 
	value = chg->pdata->wd_dis ? (0x8|chg->pdata->wd_tmr) : (0x00|chg->pdata->wd_tmr);
	ret = hl7139_write_reg(chg, HL7139_CTRL2, value);
	if (ret < 0)
		goto I2C_FAIL;

	/* REg0x40, ADC_CTRL, 4 samples, auto-mode,adc_read_en bit is 1 */  
	ret = hl7139_write_reg(chg, HL7139_ADC_CTRL0, 0x05);
	if (ret < 0)
		goto I2C_FAIL;

	/* Set CP MODE for default setting  */
	ret = hl7139_set_dev_mode(chg, CP_MODE);
	if (ret < 0)
		return ret;
	/* Set iin to default iin */
	ret = hl7139_set_iin_reg_th(chg, chg->pdata->iin_cfg);
	if (ret < 0)
		return ret;

	/* set vbat regulation voltage to default */
	ret = hl7139_set_batovp_th(chg, chg->pdata->vbat_reg);
	if (ret < 0)
		return ret;

	chg->pdata->vbat_reg_max = chg->pdata->vbat_reg;

#if 1
	for (i=8; i<0x17; i++){
		hl7139_read_reg(chg, i, &value);
		pr_info("dbg--reg[0x%2x], val[0x%2x]\n", i, value);
	}
	hl7139_read_reg(chg, HL7139_ADC_CTRL0, &value);
	pr_info("dbg--reg[0x%2x], val[0x%2x]\n", HL7139_ADC_CTRL0, value);
#endif    

	return ret;

I2C_FAIL:
	pr_err("[%s]:: Failed to update i2c\r\n", __func__);
	return ret;
}

static int hl7139_adc_man_copy(struct hl7139_charger *chg , bool mode)
{
	u8 val;
	int ret;

	if (mode) 
		val = HL7139_ADC_REG_COPY_MM;
	else
		val = HL7139_ADC_REG_COPY_FR;

	val <<= HL7139_ADC_REG_COPY_SHIFT;
	ret = hl7139_update_reg(chg, HL7139_ADC_CTRL0, HL7139_ADC_REG_COPY_MASK, val);

	return ret;
}
static int hl7139_adc_reg_copy(struct hl7139_charger *chg, bool enable)
{
	u8 val;
	int ret;

	if (enable)
		val = HL7139_ADC_MAN_COPY_CP;
	else
		val = HL7139_ADC_MAN_COPY_NO_CP;

	val <<= HL7139_ADC_MAN_COPY_SHIFT;
	ret = hl7139_update_reg(chg, HL7139_ADC_CTRL0, HL7139_ADC_MAN_COPY_MASK, val);

	return ret;
}
static int hl7139_adc_mode_config(struct hl7139_charger *chg, int mode)
{
	u8 val;
	int ret;

	val = (u8)mode;
	val <<= HL7139_ADC_MODE_CFG_SHIFT;
	ret = hl7139_update_reg(chg, HL7139_ADC_CTRL0, HL7139_ADC_MODE_CFG_MASK, val);

	return ret;
}
static int hl7139_adc_avg_time(struct hl7139_charger *chg, int mode)
{
	u8 val;
	int ret;

	val = (u8)mode;
	val <<= HL7139_ADC_AVG_TIME_SHIFT;
	ret = hl7139_update_reg(chg, HL7139_ADC_CTRL0, HL7139_ADC_AVG_TIME_MASK, val);

	return ret;
}
static int hl7139_adc_read_enable(struct hl7139_charger *chg, bool enable)
{
	u8 val;
	int ret;

	if (enable)
		val = HL7139_ADC_READ_EN_ENABLE;
	else
		val = HL7139_ADC_READ_EN_DISABLE;

	val <<= HL7139_ADC_READ_EN_SHIFT;
    ret = hl7139_update_reg(chg, HL7139_ADC_CTRL0, HL7139_ADC_READ_EN_MASK, val);
	return ret;
}

static int hl7139_adc_vin_disable(struct hl7139_charger *chg, bool enable)
{
	u8 val;
	int ret;

	if (enable)
		val = HL7139_VIN_ADC_DIS_ENABLE;
	else
		val = HL7139_VIN_ADC_DIS_DISABLE;

	val <<= HL7139_VIN_ADC_DIS_SHIFT;
	ret = hl7139_update_reg(chg, HL7139_ADC_CTRL1, HL7139_VIN_ADC_DIS_MASK, val);

	return ret;
}
static int hl7139_adc_iin_disable(struct hl7139_charger *chg, bool enable)
{
	u8 val;
	int ret;

	if (enable)
		val = HL7139_IIN_ADC_DIS_ENABLE;
	else
		val = HL7139_IIN_ADC_DIS_DISABLE;

	val <<= HL7139_IIN_ADC_DIS_SHIFT;
	ret = hl7139_update_reg(chg, HL7139_ADC_CTRL1, HL7139_IIN_ADC_DIS_MASK, val);

	return ret;
}
static int hl7139_adc_vbat_disable(struct hl7139_charger *chg, bool enable)
{
	u8 val;
	int ret;

	if (enable)
		val = HL7139_VBAT_ADC_DIS_ENABLE;
	else
		val = HL7139_VBAT_ADC_DIS_DISABLE;

	val <<= HL7139_VBAT_ADC_DIS_SHIFT;
	ret = hl7139_update_reg(chg, HL7139_ADC_CTRL1, HL7139_VBAT_ADC_DIS_MASK, val);

	return ret;
}

static int hl7139_adc_ibat_disable(struct hl7139_charger *chg, bool enable)
{
	u8 val;
	int ret;

	if (enable)
		val = HL7139_IBAT_ADC_DIS_ENABLE;
	else
		val = HL7139_IBAT_ADC_DIS_DISABLE;

	val <<= HL7139_IBAT_ADC_DIS_SHIFT;
	ret = hl7139_update_reg(chg, HL7139_ADC_CTRL1, HL7139_IBAT_ADC_DIS_MASK, val);

	return ret;
}
static int hl7139_adc_ts_disable(struct hl7139_charger *chg, bool enable)
{
	u8 val;
	int ret;

	if (enable)
		val = HL7139_TS_ADC_DIS_ENABLE;
	else
		val = HL7139_TS_ADC_DIS_DISABLE;

	val <<= HL7139_TS_ADC_DIS_SHIFT;
	ret = hl7139_update_reg(chg, HL7139_ADC_CTRL1, HL7139_TS_ADC_DIS_MASK, val);

	return ret;
}
static int hl7139_adc_tdie_disable(struct hl7139_charger *chg, bool enable)
{
	u8 val;
	int ret;

	if (enable)
		val = HL7139_TDIE_ADC_DIS_ENABLE;
	else
		val = HL7139_TDIE_ADC_DIS_DISABLE;

	val <<= HL7139_TDIE_ADC_DIS_SHIFT;
	ret = hl7139_update_reg(chg, HL7139_ADC_CTRL1, HL7139_TDIE_ADC_DIS_MASK, val);

	return ret;
}
static int hl7139_adc_vout_disable(struct hl7139_charger *chg, bool enable)
{
	u8 val;
	int ret;

	if (enable)
		val = HL7139_VOUT_ADC_DIS_ENABLE;
	else
		val = HL7139_VOUT_ADC_DIS_DISABLE;

	val <<= HL7139_VOUT_ADC_DIS_SHIFT;
	ret = hl7139_update_reg(chg, HL7139_ADC_CTRL1, HL7139_VOUT_ADC_DIS_MASK, val);

	return ret;
}
static int hl7139_track_ov_disable(struct hl7139_charger *chg, bool enable)
{
	u8 val;
	int ret;

	if (enable)
		val = HL7139_TRACK_OV_DIS_ENABLE;
	else
		val = HL7139_TRACK_OV_DIS_DISABLE;

	val <<= HL7139_TRACK_OV_DIS_SHIFT;
	ret = hl7139_update_reg(chg, HL7139_TRACK_OV_UV, HL7139_TRACK_OV_DIS_MASK, val);

	return ret;
}
static int hl7139_track_uv_disable(struct hl7139_charger *chg, bool enable)
{
	u8 val;
	int ret;

	if (enable)
		val = HL7139_TRACK_UV_DIS_ENABLE;
	else
		val = HL7139_TRACK_UV_DIS_DISABLE;

	val <<= HL7139_TRACK_UV_DIS_SHIFT;
	ret = hl7139_update_reg(chg, HL7139_TRACK_OV_UV, HL7139_TRACK_UV_DIS_MASK, val);

	return ret;
}
static int hl7139_track_ov(struct hl7139_charger *chg, int mv)
{
	int ret;
	u8 val;

	if (mv <= 200)
		val = HL7139_TRACK_OV_200;
	else if (mv == 300)
		val = HL7139_TRACK_OV_300;
	else if (mv == 400)
		val = HL7139_TRACK_OV_400;
	else if (mv == 500)
		val = HL7139_TRACK_OV_500;
	else if (mv == 600)
		val = HL7139_TRACK_OV_600;
	else if (mv == 700)
		val = HL7139_TRACK_OV_700;
	else if (mv == 800)
		val = HL7139_TRACK_OV_800;
	else if (mv >= 900)
		val = HL7139_TRACK_OV_900;
	else
		return -EINVAL;

	val <<= HL7139_TRACK_OV_SHIFT;
	ret = hl7139_update_reg(chg, HL7139_TRACK_OV_UV, HL7139_TRACK_OV_MASK, val);

	return ret;
}
static int hl7139_track_uv(struct hl7139_charger *chg, int mv)
{
	int ret;
	u8 val;

	if (mv <= 50)
		val = HL7139_TRACK_UV_50;
	else if (mv == 100)
		val = HL7139_TRACK_UV_100;
	else if (mv == 150)
		val = HL7139_TRACK_UV_150;
	else if (mv == 200)
		val = HL7139_TRACK_UV_200;
	else if (mv == 250)
		val = HL7139_TRACK_UV_250;
	else if (mv == 300)
		val = HL7139_TRACK_UV_300;
	else if (mv == 350)
		val = HL7139_TRACK_UV_350;
	else if (mv >= 400)
		val = HL7139_TRACK_UV_400;
	else    
		return -EINVAL;

	val <<= HL7139_TRACK_UV_SHIFT;
	ret = hl7139_update_reg(chg, HL7139_TRACK_OV_UV, HL7139_TRACK_UV_MASK, val);

	return ret;
}
static int hl7139_set_sft_reset(struct hl7139_charger *chg, bool reset)
{
	u8 val;
	int ret;

	if (reset)
		val = HL7139_SFT_RST_SET_RESET;
	else
		val = HL7139_SFT_RST_NO_RESET;

	val <<= HL7139_SFT_RST_SHIFT;
	ret = hl7139_update_reg(chg, HL7139_CTRL2, HL7139_SFT_RST_MASK, val);

	return ret;
}

static int hl7139_watchdog_time_enable(struct hl7139_charger *chg, bool enable)
{
	u8 val;
	int ret;

	if (enable)
		val = HL7139_WD_DIS_WATDOG_ENABLE;
	else
		val = HL7139_WD_DIS_WATDOG_DISABLE;

	val <<= HL7139_WD_DIS_SHIFT;
	ret = hl7139_update_reg(chg, HL7139_CTRL2, HL7139_WD_DIS_MASK, val);

	return ret;
}

static int hl7139_set_watchdog_timer(struct hl7139_charger *chg, int time)
{
	int ret;
	u8 val;

	if (time <= 200)
		val = HL7139_WD_TMR_200MS;
	else if (time == 500)
		val = HL7139_WD_TMR_500MS;
	else if (time == 1000)
		val = HL7139_WD_TMR_1000MS;
	else if (time == 2000)
		val = HL7139_WD_TMR_2000MS;
	else if (time == 5000)
		val = HL7139_WD_TMR_5000MS;
	else if (time == 10000)
		val = HL7139_WD_TMR_10000MS;
	else if (time == 20000)
		val = HL7139_WD_TMR_20000MS;
	else if (time == 40000)
		val = HL7139_WD_TMR_40000MS;
	else
		return -EINVAL;

	val <<= HL7139_WD_TMR_SHIFT;
	ret = hl7139_update_reg(chg, HL7139_CTRL2, HL7139_WD_TMR_MASK, val);

	return ret;
}

static int hl7139_ucp_deb_select(struct hl7139_charger *chg, u8 mode)
{
	u8 val;
	int ret;

	if (mode <= 10)
		val = HL7139_UCP_DEB_SEL_10;
	else if (mode == 100)
		val = HL7139_UCP_DEB_SEL_100;
	else if (mode == 500 )
		val = HL7139_UCP_DEB_SEL_500;
	else if (mode >= 1000)
		val = HL7139_UCP_DEB_SEL_1000;
	else
		return -EINVAL;

	val <<= HL7139_UCP_DEB_SEL_SHIFT;
	ret = hl7139_update_reg(chg, HL7139_CTRL1, HL7139_UCP_DEB_SEL_MASK, val);

	return ret;
}
static int hl7139_vout_ovp_dis(struct hl7139_charger *chg, bool enable)
{
	u8 val;
	int ret;

	if (enable)
		val = HL7139_VOUT_OVP_DIS_ENABLE;
	else
		val = HL7139_VOUT_OVP_DIS_DISABLE;

	val <<= HL7139_VOUT_OVP_DIS_SHIFT;
	ret = hl7139_update_reg(chg, HL7139_CTRL1, HL7139_VOUT_OVP_DIS_MASK, val);

	return ret;
}

static int hl7139_ts_port_enable(struct hl7139_charger *chg, bool enable)
{
	u8 val;
	int ret;

	if (enable)
		val = HL7139_TS_PORT_EN_ENABLE;
	else
		val = HL7139_TS_PORT_EN_DISABLE;

	val <<= HL7139_TS_PORT_EN_SHIFT;
	ret = hl7139_update_reg(chg, HL7139_CTRL1, HL7139_TS_PORT_EN_MASK, val);

	return ret;
}

static int hl7139_auto_v_rec_enable(struct hl7139_charger *chg, bool enable)
{
	u8 val;
	int ret;

	if (enable)
		val = HL7139_AUTO_V_REC_EN_ENABLE;
	else
		val = HL7139_AUTO_V_REC_EN_DISABLE;

	val <<= HL7139_AUTO_V_REC_EN_SHIFT;
	ret = hl7139_update_reg(chg, HL7139_CTRL1, HL7139_AUTO_V_REC_EN_MASK, val);

	return ret;
}
static int hl7139_auto_i_rec_enable(struct hl7139_charger *chg, bool enable)
{
	u8 val;
	int ret;

	if (enable)
		val = HL7139_AUTO_I_REC_EN_ENABLE;
	else
		val = HL7139_AUTO_I_REC_EN_DISABLE;

	val <<= HL7139_AUTO_I_REC_EN_SHIFT;
	ret = hl7139_update_reg(chg, HL7139_CTRL1, HL7139_AUTO_I_REC_EN_MASK, val);

	return ret;
}
static int hl7139_auto_ucp_rec_enable(struct hl7139_charger *chg, bool enable)
{
	u8 val;
	int ret;

	if (enable)
		val = HL7139_AUTO_UCP_REC_EN_ENABLE;
	else
		val = HL7139_AUTO_UCP_REC_EN_DISABLE;

	val <<= HL7139_AUTO_UCP_REC_EN_SHIFT;
	ret = hl7139_update_reg(chg, HL7139_CTRL1, HL7139_AUTO_UCP_REC_EN_MASK, val);

	return ret;
}

static int hl7139_fsw_set(struct hl7139_charger *chg, int hz)
{
	u8 val;
	int ret;

	val = (hz - HL7139_FSW_SET_BASE) / HL7139_FSW_SET_LB;

	val <<= HL7139_FSW_SET_SHIFT;
	ret = hl7139_update_reg(chg, HL7139_CTRL0, HL7139_FSW_SET_MASK, val);

	return ret;
}

static int hl7139_unplug_detect_enable(struct hl7139_charger *chg, bool enable)
{
	u8 val;
	int ret;

	if (enable)
		val = HL7139_UNPLUG_DET_ENABLE;
	else
		val = HL7139_UNPLUG_DET_DISABLE;

	val <<= HL7139_AUTO_UCP_REC_EN_SHIFT;
	ret = hl7139_update_reg(chg, HL7139_CTRL0, HL7139_UNPLUG_DET_MASK, val);

	return ret;
}

static int hl7139_iin_ucp_th(struct hl7139_charger *chg, int threshold)
{
	u8 val;
	int ret;

	if (bpcp == HL7139_DEV_MODE_BP_MODE) {
		if (threshold <= 100)
			val = HL7139_IIN_UCP_TH_100;
		else if (threshold == 150)
			val = HL7139_IIN_UCP_TH_150;
		else if(threshold == 200)
			val = HL7139_IIN_UCP_TH_200;
		else if (threshold >= 250)
			val = HL7139_IIN_UCP_TH_250;
		else 
			return -EINVAL;
	} else {
		if (threshold <= 200)
			val = HL7139_IIN_UCP_TH_200_BP;
		else if (threshold == 300)
			val = HL7139_IIN_UCP_TH_300_BP;
		else if(threshold == 400)
			val = HL7139_IIN_UCP_TH_400_BP;
		else if (threshold >= 500)
			val = HL7139_IIN_UCP_TH_500_BP;
		else 
			return -EINVAL;
	}

	val <<= HL7139_IIN_UCP_TH_SHIFT;
	ret = hl7139_update_reg(chg, HL7139_CTRL0, HL7139_IIN_UCP_TH_MASK, val);

	return ret;
}

static int hl7139_get_adc_data(struct hl7139_charger *chg, int channel,  int *result)
{
	int ret;
	u8 r_val_high;
	u8 r_val_low;
	u16 t;
	u16 val;

	if(channel >= ADC_MAX_NUM) return 0;

	ret = hl7139_read_reg(chg, AL7139_GET_ADC_BASE_REG + (channel << 1), &r_val_low);
	ret = hl7139_read_reg(chg, AL7139_GET_ADC_BASE_REG + (channel << 1) + 1, &r_val_high);

	if (ret < 0)
		return ret;
	t = r_val_high & 0xFF;
	t <<= 8;
	t |= (r_val_low >> 8) & 0xFF;
	val = t;

	switch (channel)
	{
		case ADC_VIN:
			val = val * AL7139_VBUS_ADC_LSB;
			break;
		case ADC_IIN:
			val = val * AL7139_IBUS_ADC_LSB;
			break;
		case ADC_VBAT:
			val = val * AL7139_VBAT_ADC_LSB;
			break;
		case ADC_IBAT:
			val = val * AL7139_IBAT_ADC_LSB;
			break;
		case ADC_TS:
			val = val * AL7139_VTS_ADC_LSB;
			break;
		case ADC_VOUT:
			val = val * AL7139_VOUT_ADC_LSB;
			break;
		case ADC_TDIE:
			val = val * AL7139_TDIE_ADC_LSB;
			break;
		default:
			pr_info("Not find adc channel : %d\n", channel);
			break;
	}

	*result = val;

	return 0;
}

static int hl7139_init_protection(struct hl7139_charger *chg)
{
	int ret;

	ret = hl7139_enable_batovp(chg, !chg->pdata->bat_ovp_disable);
	pr_info("%s bat ovp %s\n", chg->pdata->bat_ovp_disable ? "enable" : "disable", !ret ? "successfullly" : "failed");

	ret = hl7139_enable_batocp(chg, !chg->pdata->bat_ocp_disable);
	pr_info("%s bat ocp %s\n", chg->pdata->bat_ocp_disable ? "enable" : "disable", !ret ? "successfullly" : "failed");

	ret = hl7139_set_vin_ovp_dis(chg, !chg->pdata->vin_ovp_disable);
	pr_info("%s vin ovp %s\n", chg->pdata->vin_ovp_disable ? "enable" : "disable", !ret ? "successfullly" : "failed");

	ret = hl7139_set_iin_ocp_dis(chg, !chg->pdata->iin_ocp_disable);
	pr_info("%s iin ocp %s\n", chg->pdata->iin_ocp_disable ? "enable" : "disable", !ret ? "successfullly" : "failed");

	ret = hl7139_vout_ovp_dis(chg, !chg->pdata->vout_ovp_disable);
	pr_info("%s vout ovp %s\n", chg->pdata->vout_ovp_disable ? "enable" : "disable", !ret ? "successfullly" : "failed");




	ret = hl7139_set_batovp_th(chg, chg->pdata->bat_ovp_th);
	pr_info("set bat ovp th %d %s\n", chg->pdata->bat_ovp_th, !ret ? "successfully" : "failed");

	ret = hl7139_set_batocp_th(chg, chg->pdata->bat_ocp_th);
	pr_info("set bat ocp threshold %d %s\n", chg->pdata->bat_ocp_th, !ret ? "successfully" : "failed");

	ret = hl7139_set_vin_ovp_th(chg, chg->pdata->ac_ovp_th);
	pr_info("set vin ovp threshold %d %s\n", chg->pdata->ac_ovp_th, !ret ? "successfully" : "failed");

	ret = hl7139_set_busovp_th(chg, chg->pdata->bus_ovp_th);
	pr_info("set bus ovp threshold %d %s\n", chg->pdata->bus_ovp_th, !ret ? "successfully" : "failed");


	return 0;
}

static void hl7139_reg_reset(struct hl7139_charger *chg)
{	
	hl7139_write_reg (chg, HL7139_VBAT_REG, 0x23);
	hl7139_write_reg (chg, HL7139_IBAT_REG, 0x2E);
	hl7139_write_reg (chg, HL7139_VBUS_OVP_REG, 0x88);
	hl7139_write_reg (chg, HL7139_VIN_OVP_REG, 0x03);
	hl7139_write_reg (chg, HL7139_IIN_REG, 0x28);
	hl7139_write_reg (chg, HL7139_IIN_OC_REG, 0x00);
	hl7139_write_reg (chg, HL7139_REG_CTRL0, 0x2C);
	hl7139_write_reg (chg, HL7139_REG_CTRL1, 0x5C);
	hl7139_write_reg (chg, HL7139_CTRL0, 0x25);
	hl7139_write_reg (chg, HL7139_CTRL1, 0x00);
	hl7139_write_reg (chg, HL7139_CTRL2, 0x08);
	hl7139_write_reg (chg, HL7139_CTRL3, 0x00);
	hl7139_write_reg (chg, HL7139_TRACK_OV_UV, 0x3C);
	hl7139_write_reg (chg, HL7139_ADC_CTRL0, 0x00);
	hl7139_write_reg (chg, HL7139_ADC_CTRL1, 0x00);
}

static int hl7139_init_regulation(struct hl7139_charger *chg)
{

	hl7139_set_vbat_ovp_th(chg, 80);
	hl7139_set_ibat_ovp_th(chg, 300);


	hl7139_set_vbat_reg_dis(chg, false);
	hl7139_set_ibat_reg_dis(chg, false);
	hl7139_set_iin_reg_dis(chg, false);

	return 0;
}

static int hl7139_set_adc_scan(struct hl7139_charger *chg, int channel, bool enable)
{
	u8 mask;
	u8 shift;
	u8 val;

	if (channel > ADC_MAX_NUM)
		return -EINVAL;

	shift = HL7139_ADC_DIS_SHIFT - channel;
	mask = HL7139_ADC_DIS_MASK << shift;

	if (true)
		val = HL7139_ADC_CHANNEL_ENABLE << shift;
	else
		val = HL7139_ADC_CHANNEL_DISABLE << shift;

	return hl7139_update_reg(chg, HL7139_ADC_CTRL1, mask, val);
}

static int hl7139_init_adc(struct hl7139_charger *chg)
{

	hl7139_set_adc_scan(chg, ADC_VIN, true);
	hl7139_set_adc_scan(chg, ADC_IIN, true);
	hl7139_set_adc_scan(chg, ADC_VBAT, true);
	hl7139_set_adc_scan(chg, ADC_IBAT, true);
	hl7139_set_adc_scan(chg, ADC_TS, true);
	hl7139_set_adc_scan(chg, ADC_TDIE, true);
	hl7139_set_adc_scan(chg, ADC_VOUT, true);

	hl7139_adc_read_enable(chg, true);

	return 0;
}

static int hl7139_init_device(struct hl7139_charger *chg)
{
        pr_err("%s:enter\n", __func__);
	hl7139_init_protection(chg);
        pr_err("%s:after init protect\n", __func__);
	hl7139_init_regulation(chg);
        pr_err("%s:after init regulat\n", __func__);
	hl7139_init_adc(chg);
        pr_err("%s:after init adc\n", __func__);
	hl7139_set_wdt(chg, 0);
        pr_err("%s:after set wdt\n", __func__);

	return 0;
}

static int hl7139_set_present(struct hl7139_charger *chg, bool present)
{
	chg->usb_present = present;

	if (present)
		hl7139_init_device(chg);
	return 0;
}
#if 0
static int hl7139_read_adc(struct hl7139_charger *chg)
{
	u8 r_data[2]; 
	u16 raw_adc;
	int ret = 0;

	//LOG_DBG(chg, "Start!!\n");
	/* Set AD_MAN_COPY BIT to read manually */
	ret = hl7139_update_reg(chg, REG_ADC_CTRL_0, BIT_ADC_MAN_COPY, BIT_ADC_MAN_COPY);
	if (ret < 0){
		pr_err("Can't update ADC_MAN_COPY BIT\n");
		goto Err;
	}

	/* VIN ADC */
	ret = hl7139_bulk_read_reg(chg, REG_ADC_VIN_0, r_data, 2);
	if (ret < 0) {
		pr_err("Can't read VIN ADC\n");
		goto Err;
	}
	raw_adc = ((r_data[0] << 4) | (r_data[1] & BITS_ADC_VIN_LSB));
	chg->adc_vin = raw_adc * HL7139_VIN_ADC_STEP;
	//LOG_DBG(chg, "[VIN-ADC] rValue:[0x%x][0x%x], raw_adc:[0x%x]\n", r_data[0], r_data[1], raw_adc);

	/* IIN ADC */
	ret = hl7139_bulk_read_reg(chg, REG_ADC_IIN_0, r_data, 2);
	if (ret < 0) {
		pr_err("Can't read IIN ADC\n");
		goto Err;
	}
	raw_adc = ((r_data[0] << 4) | (r_data[1] & BITS_ADC_IIN_LSB));
	chg->adc_iin = (chg->dev_mode == CP_MODE) ? (raw_adc * HL7139_CP_IIN_ADC_STEP) : (raw_adc * HL7139_BP_IIN_ADC_STEP);
	//LOG_DBG(chg, "[IIN-ADC] rValue:[0x%x][0x%x], raw_adc:[0x%x]\n", r_data[0], r_data[1], raw_adc);

	/* Set AD_MAN_COPY BIT to read manually */
	ret = hl7139_update_reg(chg, REG_ADC_CTRL_0, BIT_ADC_MAN_COPY, BIT_ADC_MAN_COPY);
	if (ret < 0){
		pr_err("Can't update ADC_MAN_COPY BIT\n");
		goto Err;
	}
	/* VBAT ADC */
	ret = hl7139_bulk_read_reg(chg, REG_ADC_VBAT_0, r_data, 2);
	if (ret < 0) {
		pr_err("Can't read VBAT_ADC\n");
		goto Err;
	}
	raw_adc = ((r_data[0] << 4) | (r_data[1] & BITS_ADC_VBAT_LSB));
	chg->adc_vbat = raw_adc * HL7139_VBAT_ADC_STEP;
	//LOG_DBG(chg, "[VBAT-ADC] rValue:[0x%x][0x%x], raw_adc:[0x%x]\n", r_data[0], r_data[1], raw_adc);

	/* IBAT ADC */
	ret = hl7139_bulk_read_reg(chg, REG_ADC_IBAT_0, r_data, 2);
	if (ret < 0) {
		pr_err("Can't read IBAT ADC\n");
		goto Err;
	}
	raw_adc = ((r_data[0] << 4) | (r_data[1] & BITS_ADC_IBAT_LSB));
	chg->adc_ibat = raw_adc * HL7139_IBAT_ADC_STEP;
	//LOG_DBG(chg, "[IBAT-ADC] rValue:[0x%x][0x%x], raw_adc:[0x%x]\n", r_data[0], r_data[1], raw_adc);

	/* VTS ADC  */
	ret = hl7139_bulk_read_reg(chg, REG_ADC_VTS_0, r_data, 2);
	if (ret < 0) {
		pr_err("Can't read VTS ADC\n");
		goto Err;
	}
	raw_adc = ((r_data[0] << 4) | (r_data[1] & BITS_ADC_VTS_LSB));
	chg->adc_vts = raw_adc * HL7139_VTS_ADC_STEP;
	//LOG_DBG(chg, "[VTS-ADC] rValue:[0x%x][0x%x], raw_adc:[0x%x]\n", r_data[0], r_data[1], raw_adc);

	/* VOUT ADC */
	ret = hl7139_bulk_read_reg(chg, REG_ADC_VOUT_0, r_data, 2);
	if (ret < 0) {
		pr_err("Can't read VOUT ADC\n");
		goto Err;
	}
	raw_adc = ((r_data[0] << 4) | (r_data[1] & BITS_ADC_VOUT_LSB));
	chg->adc_vout = raw_adc * HL7139_VOUT_ADC_STEP;
	//LOG_DBG(chg, "[VOUT-ADC] rValue:[0x%x][0x%x], raw_adc:[0x%x]\n", r_data[0], r_data[1], raw_adc);

	/* TDIE ADC */
	ret = hl7139_bulk_read_reg(chg, REG_ADC_TDIE_0, r_data, 2);
	if (ret < 0) {
		pr_err("Can't read TDIE ADC\n");
		goto Err;
	}
	raw_adc = ((r_data[0] << 4) | (r_data[1] & BITS_ADC_TDIE_LSB));
	chg->adc_tdie = raw_adc * HL7139_TDIE_ADC_STEP;
	//LOG_DBG(chg, "[TDIE-ADC] rValue:[0x%x][0x%x], raw_adc:[0x%x]\n", r_data[0], r_data[1], raw_adc);


	pr_err("VIN:[%d], IIN:[%d], VBAT:[%d]\n", chg->adc_vin, chg->adc_iin, chg->adc_vbat);
	pr_err("IBAT:[%d], VTS:[%d], VOUT:[%d], TDIE:[%d]\n", 
			chg->adc_ibat, chg->adc_vts, chg->adc_vout, chg->adc_tdie);
	return ret;  

Err:
	pr_err("Failed to read ADC!!\n");
	return ret;
}
#endif
static irqreturn_t hl7139_interrupt_handler(int irg, void *data)
{
	struct hl7139_charger *chg = data;

	pr_err("START!, irq_handled=%d, irq_none=%d\n", IRQ_HANDLED, IRQ_NONE);

//	disable_irq_nosync(chg->pdata->irq_gpio);
	return IRQ_HANDLED;
}


static int hl7139_irq_init(struct hl7139_charger *chg, struct i2c_client *client)
{
	struct hl7139_platform_data *pdata = chg->pdata;
	int ret, irq;

	//pr_info("start!\n");

	irq = gpio_to_irq(pdata->irq_gpio);
	ret = gpio_request_one(pdata->irq_gpio, GPIOF_IN, client->name);
	if (ret < 0) 
		goto fail;

	ret = request_threaded_irq(irq, NULL, hl7139_interrupt_handler,
			IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
			client->name, chg);
	if (ret < 0)
		goto fail_irq;

	client->irq = irq;   

	disable_irq(irq);
	return 0;

fail_irq:
	pr_err("request_threaded_irq failed :: %d\n", ret);
	gpio_free(pdata->irq_gpio);
fail:
	pr_err("gpio_request failed :: ret->%d\n", ret);
	client->irq = 0;
	return ret;
}

static int hl7139_charger_is_writeable(struct power_supply *psy,
		enum power_supply_property prop)
{
	int ret;

	switch (prop) {
		case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		case POWER_SUPPLY_PROP_PRESENT:
		case POWER_SUPPLY_PROP_SC_CHARGE_MODE:
		case POWER_SUPPLY_PROP_SC_DIRECT_CHARGE:
			ret = 1;
			break;
		default:
			ret = 0;
			break;
	}
	return ret;
}
static int hl7139_psy_get_property(struct power_supply *psy, enum power_supply_property psp, 
		union power_supply_propval *val)
{
	struct hl7139_charger *chg = power_supply_get_drvdata(psy);
	int ret = 0;
	u8 reg_val;
	u8 r_data[2];
	u16 raw_adc;
	int result;
	bool charger_enable;

	switch(psp) {
		case POWER_SUPPLY_PROP_CHARGING_ENABLED:
			ret = hl7139_check_charge_enabled(chg, &charger_enable);
			if (!ret) {
				chg->charger_enable = charger_enable;
				val->intval = charger_enable;
			}
			pr_info("get property POWER_SUPPLY_PROP_CHARGING_ENABLED: %s\n",
					val->intval ? "enable" : "disable");
			break;
		case POWER_SUPPLY_PROP_STATUS:
			val->intval = 0;
			pr_info("%s:prop status\n", __func__);
			break;
		case POWER_SUPPLY_PROP_PRESENT:
			val->intval = chg->usb_present;        
			pr_info("%s:prop usb present %d\n", __func__, val->intval);
			break;
		case POWER_SUPPLY_PROP_SC_BATTERY_PRESENT:
			val->intval = 1;        
			pr_info("%s:prop bat present %d\n", __func__, val->intval);
			break;
		case POWER_SUPPLY_PROP_SC_VBUS_PRESENT:
			ret = hl7139_read_reg(chg, HL7139_STATUS_A, &reg_val);
			if(!ret){
				chg->vbus_present = !!(reg_val & BIT_VBUS_UV_STS);	
				val->intval = chg->vbus_present;
			}
			pr_info("%s:prop vbus present %d\n", __func__, val->intval);
			break;
		case POWER_SUPPLY_PROP_SC_BATTERY_VOLTAGE:
			ret = hl7139_bulk_read_reg(chg, REG_ADC_VBAT_0, r_data, 2);
			if (ret < 0) {
				pr_err("Can't read VBAT ADC\n");
			} else {
				raw_adc = ((r_data[0] << 4) | (r_data[1] & BITS_ADC_VBAT_LSB));
				chg->adc_vbat = raw_adc * AL7139_VBAT_ADC_LSB * 1000;
				val->intval = chg->adc_vbat;
			}
			pr_info("%s:prop bat voltage %d\n", __func__, val->intval);
			break;
		case POWER_SUPPLY_PROP_SC_BATTERY_CURRENT:
			ret = hl7139_bulk_read_reg(chg, REG_ADC_IBAT_0, r_data, 2);
			if (ret < 0) {
				pr_err("Can't read IBAT ADC\n");
			} else {	
				raw_adc = ((r_data[0] << 4) | (r_data[1] & BITS_ADC_IBAT_LSB));
				chg->adc_ibat = raw_adc * AL7139_IBAT_ADC_LSB * 1000;
				val->intval = chg->adc_ibat;
			}
			pr_info("%s:prop bat current %d\n", __func__, val->intval);
			break;
		case POWER_SUPPLY_PROP_SC_BUS_VOLTAGE:
			ret = hl7139_bulk_read_reg(chg, REG_ADC_VIN_0, r_data, 2);
			if (ret < 0) {
				pr_err("Can't read VIN ADC\n");
			} else {
				raw_adc = ((r_data[0] << 4) | (r_data[1] & BITS_ADC_VIN_LSB));
				chg->adc_vin = raw_adc * AL7139_VBUS_ADC_LSB * 1000;
				val->intval = chg->adc_vin;
			}
			pr_info("%s:prop bus voltage %d\n", __func__, val->intval);
			break;
		case POWER_SUPPLY_PROP_SC_BUS_CURRENT:
			ret = hl7139_bulk_read_reg(chg, REG_ADC_IIN_0, r_data, 2);
			if (ret < 0) {
				pr_err("Can't read IIN ADC\n");
			} else {
				raw_adc = ((r_data[0] << 4) | (r_data[1] & BITS_ADC_IIN_LSB));
				chg->adc_iin = (chg->dev_mode == CP_MODE) ? (raw_adc * HL7139_CP_IIN_ADC_STEP) : (raw_adc * HL7139_BP_IIN_ADC_STEP);
				val->intval = chg->adc_iin;
			}
			pr_info("%s:prop ibus cur %d\n", __func__, val->intval);
			break;
		case POWER_SUPPLY_PROP_SC_DIE_TEMPERATURE:
			ret = hl7139_bulk_read_reg(chg, REG_ADC_TDIE_0, r_data, 2);
			if (ret < 0) {
				pr_err("Can't read TDIE ADC\n");
			} else {
				raw_adc = ((r_data[0] << 4) | (r_data[1] & BITS_ADC_TDIE_LSB));
				chg->adc_tdie = raw_adc * AL7139_TDIE_ADC_LSB;
				val->intval = chg->adc_tdie;
			}
			pr_info("%s:prop die temp %d\n", __func__, val->intval);
			break;
		case POWER_SUPPLY_PROP_SC_VBUS_ERROR_STATUS:
			ret = hl7139_check_vbus_error_status(chg, &result);
			if (!ret) {
				chg->vbus_error = result;
				val->intval = chg->vbus_error;
			}
			pr_info("%s:prop vbus err %d\n", __func__, val->intval);
			break;
		case POWER_SUPPLY_PROP_SC_CHARGE_MODE:
			ret = hl7139_get_dev_mode(chg, &result);
			if (!ret) {
				chg->dev_mode = result;
				val->intval = chg->dev_mode;
			}
			pr_info("%s:prop charger mode %d\n", __func__, val->intval);
			break;
		case POWER_SUPPLY_PROP_SC_DIRECT_CHARGE:
			val->intval = chg->direct_charge;
			pr_info("%s:prop dir charger %d\n", __func__, val->intval);
			break;
		default:
			return -EINVAL;
	}

	return 0;
}

static int hl7139_psy_set_property(struct power_supply *psy, enum power_supply_property psp,
		const union power_supply_propval *val)
{
	struct hl7139_charger *chg = power_supply_get_drvdata(psy);

	pr_err("%s:set prop==[%d], val==[%d]\n", psp, val->intval);

	switch(psp) {
		case POWER_SUPPLY_PROP_CHARGING_ENABLED:
			hl7139_enable_charge(chg, val->intval);
			hl7139_check_charge_enabled(chg, &chg->charger_enable);
			pr_info("POWER_SUPPLY_PROP_CHARGING_ENABLED: %s\n",
					chg->charger_enable ? "enable" : "disable");
			break;
		case POWER_SUPPLY_PROP_PRESENT:
			hl7139_set_present(chg, val->intval);
			break;
		case POWER_SUPPLY_PROP_SC_CHARGE_MODE:
			hl7139_set_dev_mode(chg, val->intval);
			break;
		case POWER_SUPPLY_PROP_SC_DIRECT_CHARGE:
			chg->direct_charge = val->intval;
			break; 

		default:
			return -EINVAL;
	}

	return 0;
}

static int hl7139_regmap_init(struct hl7139_charger *chg)
{
	int ret;
	//LOG_DBG(chg,  "Start!!\n");
	if (!i2c_check_functionality(chg->client->adapter, I2C_FUNC_I2C)) {
		dev_err(chg->dev, "%s: check_functionality failed.", __func__);
		return -ENODEV;
	}

	chg->regmap = devm_regmap_init_i2c(chg->client, &hl7139_regmap_config);
	if(IS_ERR(chg->regmap)) {
		ret = PTR_ERR(chg->regmap);
		dev_err(chg->dev, "regmap init failed, err == %d\n", ret);
		return PTR_ERR(chg->regmap);
	}
	i2c_set_clientdata(chg->client, chg);

	return 0;
}

static int read_reg(void *data, u64 *val)
{
	struct hl7139_charger *chg = data;
	int rc;
	unsigned int temp;

	pr_err("Start!\n");

	rc = hl7139_read_reg(chg, chg->debug_address, &temp);
	if (rc) {
		pr_err("Couldn't read reg 0x%x rc = %d\n",
				chg->debug_address, rc);
		return -EAGAIN;
	}

	pr_err("address = [0x%x], value = [0x%x]\n", chg->debug_address, temp);
	*val = temp;
	return 0;
}

static int write_reg(void *data, u64 val)
{
	struct hl7139_charger *chg = data;
	int rc;
	u8 temp;

	pr_err("Start! val == [%x]\n", (u8)val);
	temp = (u8) val;

	rc = hl7139_write_reg(chg, chg->debug_address, temp);
	if (rc) {
		pr_err("Couldn't write 0x%02x to 0x%02x rc= %d\n",
				temp, chg->debug_address, rc);
		return -EAGAIN;
	}
	return 0;
}

static int hl7139_get_devmode(void *data, u64 *val)
{
	struct hl7139_charger *chg = data;

	pr_err("Charging mode[%d]\n", chg->dev_mode);
	*val = chg->dev_mode; 
	return 0;
}

static int hl7139_set_devmode(void *data, u64 val)
{
	struct hl7139_charger *chg = data;
	int rc = 0;

	if (val > 3)
		val = 3;

	pr_err("val == [%x]\n", val);

	if ( chg->dev_mode != val) {
		if (chg->online) {
			pr_err("Can't change the charging mode! during charging[%d]!!\n", chg->dev_mode);
			return -EAGAIN;
		} else {
			rc = hl7139_set_dev_mode(chg, val);       
		}
	} else {
		pr_info("Don't need to update the charging mode!\n");
	}

	return rc;
}


DEFINE_SIMPLE_ATTRIBUTE(register_debug_ops, read_reg, write_reg, "0x%02llx\n");
DEFINE_SIMPLE_ATTRIBUTE(hl7139_dcmode_ops, hl7139_get_devmode, hl7139_set_devmode, "0x%02llx\n");


static int hl7139_create_debugfs_entries(struct hl7139_charger *chg)
{
	struct dentry *ent;
	int rc = 0;

	chg->debug_root = debugfs_create_dir("hl7139-debugfs", NULL);
	if (!chg->debug_root) {
		dev_err(chg->dev, "Couldn't create debug dir\n");
		rc = -ENOENT;
	} else {
		ent = debugfs_create_x32("address", S_IFREG | S_IWUSR | S_IRUGO,
				chg->debug_root, &(chg->debug_address));
		if (!ent) {
			dev_err(chg->dev, "Couldn't create address debug file\n");
			rc = -ENOENT;
		}

		ent = debugfs_create_file("data", S_IFREG | S_IWUSR | S_IRUGO,
				chg->debug_root, chg, &register_debug_ops);
		if (!ent) {
			dev_err(chg->dev, "Couldn't create data debug file\n");
			rc = -ENOENT;
		}

		ent = debugfs_create_file("chgmode", S_IFREG | S_IWUSR | S_IRUGO,
				chg->debug_root, chg, &hl7139_dcmode_ops);
		if (!ent) {
			dev_err(chg->dev, "Couldn't create chgmode debug file\n");
			rc = -ENOENT;
		}
	}
	return rc;
}

static enum power_supply_property hl7139_psy_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_CHARGING_ENABLED,
	POWER_SUPPLY_PROP_SC_BATTERY_PRESENT,
	POWER_SUPPLY_PROP_SC_VBUS_PRESENT,
	POWER_SUPPLY_PROP_SC_BATTERY_VOLTAGE,
	POWER_SUPPLY_PROP_SC_BATTERY_CURRENT,
	POWER_SUPPLY_PROP_SC_BUS_VOLTAGE,
	POWER_SUPPLY_PROP_SC_BUS_CURRENT,
	POWER_SUPPLY_PROP_SC_DIE_TEMPERATURE,
	POWER_SUPPLY_PROP_SC_VBUS_ERROR_STATUS,
	POWER_SUPPLY_PROP_SC_CHARGE_MODE,
	POWER_SUPPLY_PROP_SC_DIRECT_CHARGE,
};

/*
   static const struct power_supply_desc hl7139_psy_desc = {
   .name = "hl7139-standalone",
   .type = POWER_SUPPLY_TYPE_MAINS,
   .get_property = hl7139_psy_get_property,
   .set_property = hl7139_psy_set_property,
   .properties = hl7139_psy_props,
   .property_is_writeable = hl7139_charger_is_writeable;
   .num_properties = ARRAY_SIZE(hl7139_psy_props),

   };
   */

static int hl7139_psy_register(struct hl7139_charger *chg)
{
	chg->psy_cfg.drv_data = chg;
	chg->psy_cfg.of_node = chg->dev->of_node;

	chg->psy_desc.name = "sc854x-standalone",
	chg->psy_desc.type = POWER_SUPPLY_TYPE_MAINS,
	chg->psy_desc.get_property = hl7139_psy_get_property,
	chg->psy_desc.set_property = hl7139_psy_set_property,
	chg->psy_desc.properties = hl7139_psy_props,
	chg->psy_desc.property_is_writeable = hl7139_charger_is_writeable;
	chg->psy_desc.num_properties = ARRAY_SIZE(hl7139_psy_props),

	chg->fc2_psy = devm_power_supply_register(chg->dev,
				&chg->psy_desc, &chg->psy_cfg);
	if (IS_ERR(chg->fc2_psy)) {
		pr_err("failed to register fc2_psy\n");
		return PTR_ERR(chg->fc2_psy);
	}

	pr_info("%s power supply register successfully\n", chg->psy_desc.name);

	return 0;
}

static int hl7139_read_device_id(struct i2c_client *client)
{
	u8 dev_id = 0;
	int ret = 0;

	ret = i2c_smbus_read_byte_data(client, REG_DEVICE_ID);
	if (ret < 0) {
		pr_err("i2c read fail: can't read from reg\n");
		ret =  -1;
	} else {
		dev_id = (u8) (ret & BITS_DEV_ID);
		if (dev_id != DEVICE_ID) {
			ret = -1;
		}
	}

	return ret;
}

static int hl7139_suspend_notifier(struct notifier_block *nb,
                unsigned long event,
                void *dummy)
{
	struct hl7139_charger *sc = container_of(nb, struct hl7139_charger, pm_nb);

	switch (event) {
	case PM_SUSPEND_PREPARE:
		pr_err("SC854X PM_SUSPEND \n");
		usbqc_suspend_notifier(true);
		sc->hl7139_suspend_flag = 1;
		return NOTIFY_OK;

	case PM_POST_SUSPEND:
		pr_err("SC854X PM_RESUME \n");
		usbqc_suspend_notifier(false);
		sc->hl7139_suspend_flag = 0;
		return NOTIFY_OK;

	default:
		return NOTIFY_DONE;
	}
}

static int hl7139_charger_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct hl7139_platform_data *pdata;
	struct hl7139_charger *charger;
	//struct power_supply_config psy_cfg = {};
	int ret;

	//LOG_DBG(chg, charger, "Start!\n");
	pr_info("[%s]:: STart!\n", __func__);

	ret = hl7139_read_device_id(client);
	if (ret < 0) {
		pr_err("hl7139 chip not found\n");
		return -EINVAL;
	}

	charger = devm_kzalloc(&client->dev, sizeof(*charger), GFP_KERNEL);
	if (!charger){
		dev_err(&client->dev, "failed to allocate chip memory\n");
		return -ENOMEM;
	}
	if (client->dev.of_node){
		pdata = devm_kzalloc(&client->dev, sizeof(struct hl7139_platform_data), GFP_KERNEL);
		if(!pdata){
			dev_err(&client->dev, "Failed to allocate pdata memory\n");
			return -ENOMEM;
		}

		ret = hl7139_parse_dt(&client->dev, pdata);
		if(ret < 0){
			dev_err(&client->dev, "Failed to get device of node\n");
			return -ENOMEM;
		}

		client->dev.platform_data = pdata;
	} else {
		pdata = client->dev.platform_data;
	}
	charger->dev = &client->dev;
	charger->pdata = pdata;
	charger->client = client;
	charger->dc_state = DC_STATE_NOT_CHARGING;
		
	ret = hl7139_regmap_init(charger);
	if (ret < 0) {
		dev_err(&client->dev, "Failed to initialize i2c,[%s]\n", HL7139_I2C_NAME);
		return ret;
	}
	mutex_init(&charger->i2c_lock);
	hl7139_device_init(charger);
	hl7139_init_device(charger);
	hl7139_dump_reg(charger);
	charger->pm_nb.notifier_call = hl7139_suspend_notifier;
	register_pm_notifier(&charger->pm_nb);
	hl7139_psy_register(charger);
	/*
	   psy_cfg.supplied_to     = hl7139_supplied_to;
	   psy_cfg.num_supplicants = ARRAY_SIZE(hl7139_supplied_to);
	   psy_cfg.drv_data        = charger;
	   psy_cfg.of_node = client->dev.of_node;

	   charger->psy_chg = power_supply_register(&client->dev, &hl7139_psy_desc, &psy_cfg); */

	if (IS_ERR(charger->psy_chg)) {
		dev_err(&client->dev, "failed to register power supply\n");
		return PTR_ERR(charger->psy_chg);
	}

	if (pdata->irq_gpio >= 0){
		ret = hl7139_irq_init(charger, client);
		if (ret < 0){
			dev_warn(&client->dev, "failed to initialize IRQ :: %d\n", ret);
			goto FAIL_IRQ;
		}
	}

	ret = hl7139_create_debugfs_entries(charger);
	if (ret < 0){
		goto FAIL_DEBUGFS;
	}

	return 0;

FAIL_DEBUGFS:
	free_irq(charger->pdata->irq_gpio, NULL);
FAIL_IRQ:
	power_supply_unregister(charger->psy_chg);
	//wakeup_source_trash(&charger->wake_lock);
	devm_kfree(&client->dev, charger);
	devm_kfree(&client->dev, pdata);
	mutex_destroy(&charger->i2c_lock);
	return ret;
}

static int hl7139_charger_remove(struct i2c_client *client)
{
	struct hl7139_charger *chg = i2c_get_clientdata(client);

	pr_err("START!!");

	if (client->irq){
		free_irq(client->irq, chg);
		gpio_free(chg->pdata->irq_gpio);
	}

	//wakeup_source_trash(&chg->wake_lock);

	if(chg->psy_chg)
		power_supply_unregister(chg->psy_chg);
	return 0;
}

static void hl7139_charger_shutdown(struct i2c_client *client)
{
	struct hl7139_charger *chg = i2c_get_clientdata(client);
	pr_err("START!!");
	hl7139_enable_charge(chg, false);
	if (client->irq){
		free_irq(client->irq, chg);
		gpio_free(chg->pdata->irq_gpio);
	}

	//wakeup_source_trash(&chg->wake_lock);

	if(chg->psy_chg)
		power_supply_unregister(chg->psy_chg);
}

#if defined (CONFIG_PM)
static int hl7139_charger_resume(struct device *dev)
{
	//struct hl7139_charger *charger = dev_get_drvdata(dev);
	return 0;
}

static int hl7139_charger_suspend(struct device *dev)
{
	//struct hl7139_charger *chg = 
	dev_get_drvdata(dev);
	pr_err("Start!!\r\n");

	return 0;
}
#else
#define hl7139_charger_suspend   NULL
#define hl7139_charger_resume    NULL
#endif

static const struct i2c_device_id hl7139_id_table[] = {
	{HL7139_I2C_NAME, 0},
	{ },
};


static struct of_device_id hl7139_match_table[] = {
	{ .compatible = "halo,hl7139-standalone", },
	{},
};


const struct dev_pm_ops hl7139_pm_ops = {
	.suspend = hl7139_charger_suspend,
	.resume  = hl7139_charger_resume,
};

static struct i2c_driver hl7139_driver = {
	.driver = {
		.name           = HL7139_I2C_NAME,
		.of_match_table     = hl7139_match_table,
#if defined(CONFIG_PM)
		.pm                 = &hl7139_pm_ops,
#endif
	},
	.probe      = hl7139_charger_probe,
	.remove     = hl7139_charger_remove,
	.shutdown   = hl7139_charger_shutdown,
	.id_table   = hl7139_id_table,
};

static int __init hl7139_charger_init(void)
{
	int err;

	err = i2c_add_driver(&hl7139_driver);
	if (err) {
		pr_err("hl7139_charger driver failed (errno = %d\n", err);
	} else {
		pr_err("Successfully added driver %s\n", hl7139_driver.driver.name);
	}

	return err;
}


static void __exit hl7139_charger_exit(void)
{
	i2c_del_driver(&hl7139_driver);
}

module_init(hl7139_charger_init);
module_exit(hl7139_charger_exit);


