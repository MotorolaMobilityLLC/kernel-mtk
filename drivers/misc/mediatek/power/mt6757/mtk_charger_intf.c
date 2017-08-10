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

/* Include from MTK */
#include <mt-plat/charging.h>
#include <mt-plat/upmu_common.h>
#include <mt-plat/mt_boot.h>
#include <mt-plat/mt_gpio.h>
#include <mt-plat/battery_meter.h>
#include <mach/upmu_hw.h>
#include <mach/mt_sleep.h>
#include <mach/mt_charging.h>
#include <mach/mt_pmic.h>

#include <linux/delay.h>
#include <linux/reboot.h>

#include "mtk_charger_intf.h"
#include "mtk_bif_intf.h"


/* Necessary functions for integrating with MTK */
/* All of them are copied from original source code of MTK */

#define GETARRAYNUM(array) (sizeof(array)/sizeof(array[0]))

#if defined(MTK_WIRELESS_CHARGER_SUPPORT)
#define WIRELESS_CHARGER_EXIST_STATE 0

#if defined(GPIO_PWR_AVAIL_WLC)
/*K.S.?*/
unsigned int wireless_charger_gpio_number = GPIO_PWR_AVAIL_WLC;
#else
unsigned int wireless_charger_gpio_number = 0;
#endif

#endif /* MTK_WIRELESS_CHARGER_SUPPORT */

#if defined(CONFIG_POWER_EXT) || defined(CONFIG_MTK_FPGA)
#else
static CHARGER_TYPE g_charger_type = CHARGER_UNKNOWN;
#endif

kal_bool charging_type_det_done = KAL_TRUE;

const u32 VCDT_HV_VTH[] = {
	BATTERY_VOLT_04_200000_V, BATTERY_VOLT_04_250000_V, BATTERY_VOLT_04_300000_V,
	    BATTERY_VOLT_04_350000_V,
	BATTERY_VOLT_04_400000_V, BATTERY_VOLT_04_450000_V, BATTERY_VOLT_04_500000_V,
	    BATTERY_VOLT_04_550000_V,
	BATTERY_VOLT_04_600000_V, BATTERY_VOLT_06_000000_V, BATTERY_VOLT_06_500000_V,
	    BATTERY_VOLT_07_000000_V,
	BATTERY_VOLT_07_500000_V, BATTERY_VOLT_08_500000_V, BATTERY_VOLT_09_500000_V,
	    BATTERY_VOLT_10_500000_V
};

#ifdef CONFIG_MTK_DUAL_INPUT_CHARGER_SUPPORT
#ifndef CUST_GPIO_VIN_SEL
#define CUST_GPIO_VIN_SEL 18
#endif
DISO_IRQ_Data DISO_IRQ;
int g_diso_state = 0;
int vin_sel_gpio_number = (CUST_GPIO_VIN_SEL | 0x80000000);
static char *DISO_state_s[8] = {
	"IDLE",
	"OTG_ONLY",
	"USB_ONLY",
	"USB_WITH_OTG",
	"DC_ONLY",
	"DC_WITH_OTG",
	"DC_WITH_USB",
	"DC_USB_OTG",
};
#endif /*CONFIG_MTK_DUAL_INPUT_CHARGER_SUPPORT */

static unsigned int charging_error;

u32 charging_value_to_parameter(const u32 *parameter, const u32 array_size,
				       const u32 val)
{
	if (val < array_size)
		return parameter[val];

	battery_log(BAT_LOG_CRTI, "Can't find the parameter \r\n");
	return parameter[0];
}

u32 charging_parameter_to_value(const u32 *parameter, const u32 array_size,
				       const u32 val)
{
	u32 i;

	battery_log(BAT_LOG_FULL, "array_size = %d \r\n", array_size);

	for (i = 0; i < array_size; i++) {
		if (val == *(parameter + i))
			return i;
	}

	battery_log(BAT_LOG_CRTI, "NO register value match \r\n");
	/* TODO: ASSERT(0);    // not find the value */
	return 0;
}

static u32 bmt_find_closest_level(const u32 *pList, u32 number,
					 u32 level)
{
	u32 i;
	u32 max_value_in_last_element;

	if (pList[0] < pList[1])
		max_value_in_last_element = KAL_TRUE;
	else
		max_value_in_last_element = KAL_FALSE;

	if (max_value_in_last_element == KAL_TRUE) {
		for (i = (number - 1); i != 0; i--) {	/* max value in the last element */
			if (pList[i] <= level) {
				battery_log(2, "zzf_%d<=%d     i=%d\n", pList[i], level, i);
				return pList[i];
			}
		}

		battery_log(BAT_LOG_CRTI, "Can't find closest level \r\n");
		return pList[0];
		/* return CHARGE_CURRENT_0_00_MA; */
	} else {
		for (i = 0; i < number; i++) {	/* max value in the first element */
			if (pList[i] <= level)
				return pList[i];
		}

		battery_log(BAT_LOG_CRTI, "Can't find closest level \r\n");
		return pList[number - 1];
		/* return CHARGE_CURRENT_0_00_MA; */
	}
}

#if defined(CONFIG_POWER_EXT) || defined(CONFIG_MTK_FPGA)
#else
static unsigned int is_chr_det(void)
{
	unsigned int val = 0;

	val = pmic_get_register_value(MT6351_PMIC_RGS_CHRDET);
	battery_log(BAT_LOG_CRTI, "[is_chr_det] %d\n", val);

	return val;
}
#endif

/* The following functions are for chr_control_interface */

static int charging_hw_init(void *data)
{
	int ret = 0;

#if defined(MTK_WIRELESS_CHARGER_SUPPORT)
	if (wireless_charger_gpio_number != 0) {
#ifdef CONFIG_MTK_LEGACY
		mt_set_gpio_mode(wireless_charger_gpio_number, 0);	/* 0:GPIO mode */
		mt_set_gpio_dir(wireless_charger_gpio_number, 0);	/* 0: input, 1: output */
#else
/*K.S. way here*/
#endif
	}
#endif

#ifdef CONFIG_MTK_DUAL_INPUT_CHARGER_SUPPORT
#ifdef CONFIG_MTK_LEGACY
	mt_set_gpio_mode(vin_sel_gpio_number, 0);	/* 0:GPIO mode */
	mt_set_gpio_dir(vin_sel_gpio_number, 0);	/* 0: input, 1: output */
#else
/*K.S. way here*/
#endif
#endif

	return ret;
}

static int charging_dump_register(void *data)
{
	int ret = 0;

	ret = rt_charger_dump_register();

	return ret;
}

static int charging_enable(void *data)
{
	int ret = 0;
	u8 enable = 0;

	enable = *((u8 *)data);
	ret = rt_charger_enable_charging(enable);
	if (!enable && charging_error)
		battery_log(BAT_LOG_CRTI, "[charging_enable] under test mode: disable charging\n");

	return ret;
}

static int charging_set_cv_voltage(void *data)
{
	int ret = 0;
	u32 voreg = 0;

	/* MTK's voltage unit : uV */
	/* Our voltage unit : mV */
	voreg = *((u32 *)data);
	voreg /= 1000;

	ret = rt_charger_set_battery_voreg(voreg);
	if (ret < 0)
		return ret;

	ret = rt_charger_get_battery_voreg(&voreg);
	if (ret < 0)
		return ret;

	return ret;
}

static int charging_get_current(void *data)
{
	int ret = 0;
	u32 ichg = 0;

	ret = rt_charger_get_ichg(&ichg);

	/* MTK's current unit : 10uA */
	/* Our current unit : mA */
	ichg *= 100;
	*((u32 *)data) = ichg;

	return ret;
}

static int charging_set_current(void *data)
{
	int ret = 0;
	u32 ichg = 0;

	/* MTK's current unit : 10uA */
	/* Our current unit : mA */
	ichg = *((u32 *)data);
	ichg /= 100;

	ret = rt_charger_set_ichg(ichg);

	return ret;
}

static int charging_set_input_current(void *data)
{
	int ret = 0;
	u32 aicr = 0;

	/* MTK's current unit : 10uA */
	/* Our current unit : mA */
	aicr = *((u32 *)data);
	aicr /= 100;

	ret = rt_charger_set_aicr(aicr);

	return ret;
}

static int charging_get_charging_status(void *data)
{
	int ret = 0;
	enum rt_charging_status chg_stat = RT_CHG_STATUS_READY;

	ret = rt_charger_get_charging_status(&chg_stat);

	/* Return is charging done or not */
	switch (chg_stat) {
	case RT_CHG_STATUS_READY:
	case RT_CHG_STATUS_PROGRESS:
	case RT_CHG_STATUS_FAULT:
		*((u8 *)data) = 0;
		break;
	case RT_CHG_STATUS_DONE:
		*((u8 *)data) = 1;
		break;
	default:
		*((u8 *)data) = 0;
		break;
	}

	return ret;
}

static int charging_reset_watch_dog_timer(void *data)
{
	int ret = 0;

	ret = rt_charger_reset_wchdog_timer();

	return ret;
}

static int charging_set_hv_threshold(void *data)
{
	int ret = 0;
	u32 set_hv_voltage;
	u32 array_size;
	u16 register_value;
	u32 voltage = *((u32 *)data);

	array_size = GETARRAYNUM(VCDT_HV_VTH);
	set_hv_voltage = bmt_find_closest_level(VCDT_HV_VTH, array_size, voltage);
	register_value = charging_parameter_to_value(VCDT_HV_VTH, array_size,
		set_hv_voltage);
	pmic_set_register_value(MT6351_PMIC_RG_VCDT_HV_VTH, register_value);

	return ret;
}

static int charging_get_hv_status(void *data)
{
	int ret = 0;

	*(kal_bool *)(data) = pmic_get_register_value(MT6351_PMIC_RGS_VCDT_HV_DET);

	return ret;
}

static int charging_get_battery_status(void *data)
{
	int ret = 0;

#if defined(CONFIG_POWER_EXT) || defined(CONFIG_MTK_FPGA)
	*(kal_bool *)(data) = 0;
	battery_log(BAT_LOG_CRTI, "no battery for evb\n");
#else
	u32 val = 0;

	val = pmic_get_register_value(MT6351_PMIC_BATON_TDET_EN);
	battery_log(BAT_LOG_FULL, "%s BATON_TDET_EN = %d\n", __func__, val);
	if (val) {
		pmic_set_register_value(MT6351_PMIC_BATON_TDET_EN, 1);
		pmic_set_register_value(MT6351_PMIC_RG_BATON_EN, 1);
		*(kal_bool *)(data) =
			pmic_get_register_value(MT6351_PMIC_RGS_BATON_UNDET);
	} else {
		*(kal_bool *)(data) = KAL_FALSE;
	}
#endif

	return ret;
}

static int charging_get_charger_det_status(void *data)
{
	int ret = 0;

#if defined(CONFIG_MTK_FPGA)
	*(kal_bool *)(data) = 1;
	battery_log(BAT_LOG_CRTI, "chr exist for fpga\n");
#else
	*(kal_bool *)(data) = pmic_get_register_value(MT6351_PMIC_RGS_CHRDET);
#endif

	return ret;
}

static int charging_get_charger_type(void *data)
{
	int ret = 0;

#if defined(CONFIG_POWER_EXT) || defined(CONFIG_MTK_FPGA)
	*(CHARGER_TYPE *) (data) = STANDARD_HOST;
#else
#if defined(MTK_WIRELESS_CHARGER_SUPPORT)
	int wireless_state = 0;

	if (wireless_charger_gpio_number != 0) {
#ifdef CONFIG_MTK_LEGACY
		wireless_state = mt_get_gpio_in(wireless_charger_gpio_number);
#else
		/* K.S. way here*/
#endif
		if (wireless_state == WIRELESS_CHARGER_EXIST_STATE) {
			*(CHARGER_TYPE *) (data) = WIRELESS_CHARGER;
			battery_log(BAT_LOG_CRTI, "WIRELESS_CHARGER!\n");
			return ret;
		}
	} else {
		battery_log(BAT_LOG_CRTI, "wireless_charger_gpio_number=%d\n",
			wireless_charger_gpio_number);
	}

	if (g_charger_type != CHARGER_UNKNOWN &&
		g_charger_type != WIRELESS_CHARGER) {
		*(CHARGER_TYPE *) (data) = g_charger_type;
		battery_log(BAT_LOG_CRTI, "return %d!\n", g_charger_type);
		return ret;
	}
#endif /* MTK_WIRELESS_CHARGER_SUPPORT */

	if (is_chr_det() == 0) {
		g_charger_type = CHARGER_UNKNOWN;
		*(CHARGER_TYPE *) (data) = CHARGER_UNKNOWN;
		battery_log(BAT_LOG_CRTI, "%s: return CHARGER_UNKNOWN\n", __func__);
		return ret;
	}

	charging_type_det_done = KAL_FALSE;
	*(CHARGER_TYPE *) (data) = hw_charging_get_charger_type();
	charging_type_det_done = KAL_TRUE;
	g_charger_type = *(CHARGER_TYPE *) (data);
#endif

	return ret;
}

static int charging_get_is_pcm_timer_trigger(void *data)
{
	int ret = 0;

#if defined(CONFIG_POWER_EXT) || defined(CONFIG_MTK_FPGA)
	*(kal_bool *)(data) = KAL_FALSE;
#else
	if (slp_get_wake_reason() == WR_PCM_TIMER)
		*(kal_bool *)(data) = KAL_TRUE;
	else
		*(kal_bool *)(data) = KAL_FALSE;

	battery_log(BAT_LOG_CRTI, "slp_get_wake_reason=%d\n",
		slp_get_wake_reason());
#endif

	return ret;
}

static int charging_set_platform_reset(void *data)
{
	int ret = 0;

#if defined(CONFIG_POWER_EXT) || defined(CONFIG_MTK_FPGA)
#else
	battery_log(BAT_LOG_CRTI, "%s\n", __func__);
	kernel_restart("battery service reboot system");
#endif

	return ret;
}

static int charging_get_platform_boot_mode(void *data)
{
	int ret = 0;

#if defined(CONFIG_POWER_EXT) || defined(CONFIG_MTK_FPGA)
#else
	*((u32 *)data) = get_boot_mode();
	battery_log(BAT_LOG_CRTI, "get_boot_mode=%d\n", get_boot_mode());
#endif

	return ret;
}

static int charging_set_power_off(void *data)
{
	int ret = 0;

#if defined(CONFIG_POWER_EXT) || defined(CONFIG_MTK_FPGA)
#else
	/* Added dump_stack to see who the caller is */
	dump_stack();
	battery_log(BAT_LOG_CRTI, "%s\n", __func__);
	kernel_power_off();
#endif

	return ret;
}

static int charging_get_power_source(void *data)
{
	int ret = 0;

#if 0
/* #if defined(MTK_POWER_EXT_DETECT) */
	if (MT_BOARD_PHONE == mt_get_board_type())
		*(kal_bool *) data = KAL_FALSE;
	else
		*(kal_bool *) data = KAL_TRUE;
#else
	*(kal_bool *)data = KAL_FALSE;
#endif

	return ret;
}

static int charging_get_csdac_full_flag(void *data)
{
	return -ENOTSUPP;
}

static int charging_set_ta_current_pattern(void *data)
{
	int ret = 0;
	u8 is_pump_up = *((u8 *)data);

	ret = rt_charger_set_ta_current_pattern(is_pump_up);
	if (ret < 0)
		return ret;

	ret = rt_charger_enable_pumpX(1);

	return ret;
}

static int charging_set_hiz_swchr(void *data)
{
	int ret = 0;
	u32 mivr = 0;
	u32 en = *((u32 *)data);

	if (en == 1)
		mivr = 15300;
	else
		mivr = 4500;

	ret = rt_charger_set_mivr(mivr);

	return ret;
}

static int charging_set_error_state(void *data)
{
	int ret = 0;

	charging_error = *(u32 *)(data);
	charging_set_hiz_swchr(&charging_error);

	return ret;
}

static int charging_diso_init(void *data)
{
	int ret = 0;

#if defined(MTK_DUAL_INPUT_CHARGER_SUPPORT)
	struct device_node *node;
	DISO_ChargerStruct *pDISO_data = (DISO_ChargerStruct *) data;

	int ret_irq;
	/* Initialization DISO Struct */
	pDISO_data->diso_state.cur_otg_state = DISO_OFFLINE;
	pDISO_data->diso_state.cur_vusb_state = DISO_OFFLINE;
	pDISO_data->diso_state.cur_vdc_state = DISO_OFFLINE;

	pDISO_data->diso_state.pre_otg_state = DISO_OFFLINE;
	pDISO_data->diso_state.pre_vusb_state = DISO_OFFLINE;
	pDISO_data->diso_state.pre_vdc_state = DISO_OFFLINE;

	pDISO_data->chr_get_diso_state = KAL_FALSE;

	pDISO_data->hv_voltage = VBUS_MAX_VOLTAGE;

	/* Initial AuxADC IRQ */
	DISO_IRQ.vdc_measure_channel.number = AP_AUXADC_DISO_VDC_CHANNEL;
	DISO_IRQ.vusb_measure_channel.number = AP_AUXADC_DISO_VUSB_CHANNEL;
	DISO_IRQ.vdc_measure_channel.period = AUXADC_CHANNEL_DELAY_PERIOD;
	DISO_IRQ.vusb_measure_channel.period = AUXADC_CHANNEL_DELAY_PERIOD;
	DISO_IRQ.vdc_measure_channel.debounce = AUXADC_CHANNEL_DEBOUNCE;
	DISO_IRQ.vusb_measure_channel.debounce = AUXADC_CHANNEL_DEBOUNCE;

	/* use default threshold voltage, if use high voltage,maybe refine */
	DISO_IRQ.vusb_measure_channel.falling_threshold = VBUS_MIN_VOLTAGE / 1000;
	DISO_IRQ.vdc_measure_channel.falling_threshold = VDC_MIN_VOLTAGE / 1000;
	DISO_IRQ.vusb_measure_channel.rising_threshold = VBUS_MIN_VOLTAGE / 1000;
	DISO_IRQ.vdc_measure_channel.rising_threshold = VDC_MIN_VOLTAGE / 1000;

	node = of_find_compatible_node(NULL, NULL, "mediatek,AUXADC");
	if (!node) {
		battery_log(BAT_LOG_CRTI, "[diso_adc]: of_find_compatible_node failed!!\n");
	} else {
		pDISO_data->irq_line_number = irq_of_parse_and_map(node, 0);
		battery_log(BAT_LOG_FULL, "[diso_adc]: IRQ Number: 0x%x\n",
			    pDISO_data->irq_line_number);
	}

	mt_irq_set_sens(pDISO_data->irq_line_number, MT_EDGE_SENSITIVE);
	mt_irq_set_polarity(pDISO_data->irq_line_number, MT_POLARITY_LOW);

	ret_irq = request_threaded_irq(pDISO_data->irq_line_number, diso_auxadc_irq_handler,
				   pDISO_data->irq_callback_func, IRQF_ONESHOT, "DISO_ADC_IRQ",
				   NULL);

	if (ret_irq) {
		battery_log(BAT_LOG_CRTI, "[diso_adc]: request_irq failed.\n");
	} else {
		set_vdc_auxadc_irq(DISO_IRQ_DISABLE, 0);
		set_vusb_auxadc_irq(DISO_IRQ_DISABLE, 0);
		battery_log(BAT_LOG_FULL, "[diso_adc]: diso_init success.\n");
	}

#if defined(MTK_DISCRETE_SWITCH) && defined(MTK_DSC_USE_EINT)
	battery_log(BAT_LOG_CRTI, "[diso_eint]vdc eint irq registitation\n");
	mt_eint_set_hw_debounce(CUST_EINT_VDC_NUM, CUST_EINT_VDC_DEBOUNCE_CN);
	mt_eint_registration(CUST_EINT_VDC_NUM, CUST_EINTF_TRIGGER_LOW, vdc_eint_handler, 0);
	mt_eint_mask(CUST_EINT_VDC_NUM);
#endif

#endif /* MTK_DUAL_INPUT_CHARGER_SUPPORT */

	return ret;
}

static int charging_get_diso_state(void *data)
{
	int ret = 0;

#if defined(MTK_DUAL_INPUT_CHARGER_SUPPORT)
	int diso_state = 0x0;
	DISO_ChargerStruct *pDISO_data = (DISO_ChargerStruct *) data;

	_get_diso_interrupt_state();
	diso_state = g_diso_state;
	battery_log(BAT_LOG_FULL, "[do_chrdet_int_task] current diso state is %s!\n",
		    DISO_state_s[diso_state]);
	if (((diso_state >> 1) & 0x3) != 0x0) {
		switch (diso_state) {
		case USB_ONLY:
			set_vdc_auxadc_irq(DISO_IRQ_DISABLE, 0);
			set_vusb_auxadc_irq(DISO_IRQ_DISABLE, 0);
#ifdef MTK_DISCRETE_SWITCH
#ifdef MTK_DSC_USE_EINT
			mt_eint_unmask(CUST_EINT_VDC_NUM);
#else
			set_vdc_auxadc_irq(DISO_IRQ_ENABLE, 1);
#endif
#endif
			pDISO_data->diso_state.cur_vusb_state = DISO_ONLINE;
			pDISO_data->diso_state.cur_vdc_state = DISO_OFFLINE;
			pDISO_data->diso_state.cur_otg_state = DISO_OFFLINE;
			break;
		case DC_ONLY:
			set_vdc_auxadc_irq(DISO_IRQ_DISABLE, 0);
			set_vusb_auxadc_irq(DISO_IRQ_DISABLE, 0);
			set_vusb_auxadc_irq(DISO_IRQ_ENABLE, DISO_IRQ_RISING);
			pDISO_data->diso_state.cur_vusb_state = DISO_OFFLINE;
			pDISO_data->diso_state.cur_vdc_state = DISO_ONLINE;
			pDISO_data->diso_state.cur_otg_state = DISO_OFFLINE;
			break;
		case DC_WITH_USB:
			set_vdc_auxadc_irq(DISO_IRQ_DISABLE, 0);
			set_vusb_auxadc_irq(DISO_IRQ_DISABLE, 0);
			set_vusb_auxadc_irq(DISO_IRQ_ENABLE, DISO_IRQ_FALLING);
			pDISO_data->diso_state.cur_vusb_state = DISO_ONLINE;
			pDISO_data->diso_state.cur_vdc_state = DISO_ONLINE;
			pDISO_data->diso_state.cur_otg_state = DISO_OFFLINE;
			break;
		case DC_WITH_OTG:
			set_vdc_auxadc_irq(DISO_IRQ_DISABLE, 0);
			set_vusb_auxadc_irq(DISO_IRQ_DISABLE, 0);
			pDISO_data->diso_state.cur_vusb_state = DISO_OFFLINE;
			pDISO_data->diso_state.cur_vdc_state = DISO_ONLINE;
			pDISO_data->diso_state.cur_otg_state = DISO_ONLINE;
			break;
		default:	/* OTG only also can trigger vcdt IRQ */
			pDISO_data->diso_state.cur_vusb_state = DISO_OFFLINE;
			pDISO_data->diso_state.cur_vdc_state = DISO_OFFLINE;
			pDISO_data->diso_state.cur_otg_state = DISO_ONLINE;
			battery_log(BAT_LOG_FULL, " switch load vcdt irq triggerd by OTG Boost!\n");
			break;	/* OTG plugin no need battery sync action */
		}
	}

	if (DISO_ONLINE == pDISO_data->diso_state.cur_vdc_state)
		pDISO_data->hv_voltage = VDC_MAX_VOLTAGE;
	else
		pDISO_data->hv_voltage = VBUS_MAX_VOLTAGE;
#endif

	return ret;

}

static int charging_set_vindpm(void *data)
{
	/* MTK's mivr setting (It's actually register val from TI's BQ25896 */
	/* SWITCH_CHR_VINDPM_5V = 0x13 = 4.5V */
	/* SWITCH_CHR_VINDPM_7V = 0x25 = 6.3V */
	/* SWITCH_CHR_VINDPM_9V = 0x37 = 8.1V */
	/* SWITCH_CHR_VINDPM_12V = 0x54 = 11V */
	/* Our unit of votage is mV */

	int ret = 0;
#ifdef CONFIF_MTK_DYNAMIC_BAT_CV_SUPPORT
	u32 mivr = 0;
	u8 reg_vindpm = *((u8 *)data);

	switch (reg_vindpm) {
	case SWITCH_CHR_VINDPM_5V:
		mivr = 4500;
		break;
	case SWITCH_CHR_VINDPM_7V:
		mivr = 6300;
		break;
	case SWITCH_CHR_VINDPM_9V:
		mivr = 8100;
		break;
	case SWITCH_CHR_VINDPM_12V:
		mivr = 11000;
		break;
	default:
		mivr = 4500;
		break;
	}

	ret = rt_charger_set_mivr(mivr);
#endif

	return ret;
}

static int charging_set_vbus_ovp_en(void *data)
{
	int ret = 0;
	u32 enable = *((u32 *)data);

	pmic_set_register_value(MT6351_PMIC_RG_VCDT_HV_EN, enable);
	pr_info("%s: enable pmic ovp = %d\n", __func__, enable);

	return ret;
}

static int charging_get_bif_vbat(void *data)
{
	int ret = 0;
	u32 vbat = 0;

	ret = mtk_bif_get_vbat(&vbat);
	if (ret >= 0)
		*((u32 *)data) = vbat;

	return ret;
}

static int charging_set_chrind_ck_pdn(void *data)
{
	int ret = 0;
	u32 pwr_dn;

	pwr_dn = *((u32 *)data);
	pmic_set_register_value(PMIC_RG_DRV_CHRIND_CK_PDN, pwr_dn);

	return ret;
}


static int charging_sw_init(void *data)
{
	int ret = 0;

	/* BIF init */
	mtk_bif_init();

	/* Richtek charger init */
	ret = rt_charger_sw_init();

	return ret;
}

static int charging_enable_safetytimer(void *data)
{
	int ret = 0;
	u8 timer_enable = *((u8 *)data);

	ret = rt_charger_enable_timer(timer_enable);

	return ret;
}


static int charging_get_bif_tbat(void *data)
{
	int ret = 0, tbat = 0;

	ret = mtk_bif_get_tbat(&tbat);
	if (ret >= 0)
		*((int *)data) = tbat;

	return ret;
}

static int charging_set_ta20_reset(void *data)
{
	int ret = -ENOTSUPP;
	return ret;
}

static int charging_set_ta20_current_pattern(void *data)
{
	int ret = -ENOTSUPP;
	return ret;
}

static int charging_set_dp(void *data)
{
	int ret = 0;

#ifndef CONFIG_POWER_EXT
	u32 enable;

	enable = *((u32 *)data);
	hw_charging_enable_dp_voltage(enable);
#endif

	return ret;
}

static int charging_get_charger_temperature(void *data)
{
	int ret = 0;
	int min_temp = 0, max_temp = 0;

	ret = rt_charger_get_temperature(&min_temp, &max_temp);
	if (ret >= 0) {
		((int *)data)[0] = min_temp;
		((int *)data)[1] = max_temp;
	}

	return ret;
}

static int charging_set_boost_current_limit(void *data)
{
	int ret = 0;
	u32 current_limit = 0;

	current_limit = *((u32 *)data);

	ret = rt_charger_set_boost_current_limit(current_limit);

	return ret;
}

static int charging_enable_otg(void *data)
{
	int ret = 0;
	u8 enable = *((u8 *)data);

	ret = rt_charger_enable_otg(enable);

	return ret;
}

static int charging_enable_power_path(void *data)
{
	int ret = 0;
	u8 enable = *((u8 *)data);

	ret = rt_charger_enable_power_path(enable);

	return ret;
}

static int(*const charging_func[CHARGING_CMD_NUMBER]) (void *data) = {
	charging_hw_init, charging_dump_register, charging_enable,
	charging_set_cv_voltage, charging_get_current, charging_set_current,
	charging_set_input_current, charging_get_charging_status,
	charging_reset_watch_dog_timer, charging_set_hv_threshold,
	charging_get_hv_status, charging_get_battery_status,
	charging_get_charger_det_status, charging_get_charger_type,
	charging_get_is_pcm_timer_trigger, charging_set_platform_reset,
	charging_get_platform_boot_mode, charging_set_power_off,
	charging_get_power_source, charging_get_csdac_full_flag,
	charging_set_ta_current_pattern, charging_set_error_state,
	charging_diso_init, charging_get_diso_state, charging_set_vindpm,
	charging_set_vbus_ovp_en, charging_get_bif_vbat, charging_set_chrind_ck_pdn,
	charging_sw_init, charging_enable_safetytimer, charging_set_hiz_swchr,
	charging_get_bif_tbat, charging_set_ta20_reset, charging_set_ta20_current_pattern,
	charging_set_dp, charging_get_charger_temperature, charging_set_boost_current_limit,
	charging_enable_otg, charging_enable_power_path,
};


signed int chr_control_interface(CHARGING_CTRL_CMD cmd, void *data)
{
	signed int ret;

	if (cmd < CHARGING_CMD_NUMBER)
		ret = charging_func[cmd](data);
	else
		ret = -ENOTSUPP;

	return ret;
}
