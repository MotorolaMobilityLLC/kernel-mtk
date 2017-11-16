/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/delay.h>
#include <asm/div64.h>
#include <linux/delay.h>

#include <mt-plat/upmu_common.h>

#include <mt-plat/battery_meter_hal.h>
#include <mach/mt_battery_meter.h>
#include <mach/mt_pmic.h>
#include <mt-plat/battery_meter.h>


/*============================================================ */
/*define*/
/*============================================================ */
#define STATUS_OK    0
#define STATUS_UNSUPPORTED    -1
#define VOLTAGE_FULL_RANGE     1800
#define ADC_PRECISE           32768	/* 15 bits */

/*============================================================ */
/*global variable*/
/*============================================================ */
signed int chip_diff_trim_value_4_0 = 0;
signed int chip_diff_trim_value = 0;	/* unit = 0.1 */

signed int g_hw_ocv_tune_value = 8;

kal_bool g_fg_is_charging = 0;

/*============================================================ */
/*function prototype*/
/*============================================================*/

void get_hw_chip_diff_trim_value(void)
{
#if defined(CONFIG_POWER_EXT)
#else
	signed int reg_val_1 = 0;
	signed int reg_val_2 = 0;

#if 1
	reg_val_1 = upmu_get_reg_value(0x01C4);
	reg_val_1 = (reg_val_1 & 0xE000) >> 13;

	reg_val_2 = upmu_get_reg_value(0x01C6);
	reg_val_2 = (reg_val_2 & 0x0003);

	chip_diff_trim_value_4_0 = reg_val_1 | (reg_val_2 << 3);

	bm_print(BM_LOG_FULL,
		 "[Chip_Trim] Reg[0x%x]=0x%x, Reg[0x%x]=0x%x, chip_diff_trim_value_4_0=%d\n",
		 0x01C4, upmu_get_reg_value(0x01C4), 0x01C6, upmu_get_reg_value(0x01C6),
		 chip_diff_trim_value_4_0);

#else
	bm_print(BM_LOG_FULL, "[Chip_Trim] need check reg number\n");
#endif

	switch (chip_diff_trim_value_4_0) {
	case 0:
		chip_diff_trim_value = 1000;
		bm_print(BM_LOG_FULL, "[Chip_Trim] chip_diff_trim_value = 1000\n");
		break;
	case 1:
		chip_diff_trim_value = 1005;
		break;
	case 2:
		chip_diff_trim_value = 1010;
		break;
	case 3:
		chip_diff_trim_value = 1015;
		break;
	case 4:
		chip_diff_trim_value = 1020;
		break;
	case 5:
		chip_diff_trim_value = 1025;
		break;
	case 6:
		chip_diff_trim_value = 1030;
		break;
	case 7:
		chip_diff_trim_value = 1035;
		break;
	case 8:
		chip_diff_trim_value = 1040;
		break;
	case 9:
		chip_diff_trim_value = 1045;
		break;
	case 10:
		chip_diff_trim_value = 1050;
		break;
	case 11:
		chip_diff_trim_value = 1055;
		break;
	case 12:
		chip_diff_trim_value = 1060;
		break;
	case 13:
		chip_diff_trim_value = 1065;
		break;
	case 14:
		chip_diff_trim_value = 1070;
		break;
	case 15:
		chip_diff_trim_value = 1075;
		break;
	case 31:
		chip_diff_trim_value = 995;
		break;
	case 30:
		chip_diff_trim_value = 990;
		break;
	case 29:
		chip_diff_trim_value = 985;
		break;
	case 28:
		chip_diff_trim_value = 980;
		break;
	case 27:
		chip_diff_trim_value = 975;
		break;
	case 26:
		chip_diff_trim_value = 970;
		break;
	case 25:
		chip_diff_trim_value = 965;
		break;
	case 24:
		chip_diff_trim_value = 960;
		break;
	case 23:
		chip_diff_trim_value = 955;
		break;
	case 22:
		chip_diff_trim_value = 950;
		break;
	case 21:
		chip_diff_trim_value = 945;
		break;
	case 20:
		chip_diff_trim_value = 940;
		break;
	case 19:
		chip_diff_trim_value = 935;
		break;
	case 18:
		chip_diff_trim_value = 930;
		break;
	case 17:
		chip_diff_trim_value = 925;
		break;
	default:
		bm_print(BM_LOG_FULL, "[Chip_Trim] Invalid value(%d)\n", chip_diff_trim_value_4_0);
		break;
	}

	bm_print(BM_LOG_FULL, "[Chip_Trim] %d,%d\n",
		 chip_diff_trim_value_4_0, chip_diff_trim_value);
#endif
}

signed int use_chip_trim_value(signed int not_trim_val)
{
#if defined(CONFIG_POWER_EXT)
	return not_trim_val;
#else
	signed int ret_val = 0;

	ret_val = ((not_trim_val * chip_diff_trim_value) / 1000);

	bm_print(BM_LOG_FULL, "[use_chip_trim_value] %d -> %d\n", not_trim_val, ret_val);

	return ret_val;
#endif
}


int get_hw_ocv(void)
{
#if defined(CONFIG_POWER_EXT)
	return 4001;
#else
	signed int adc_result_reg = 0;
	signed int adc_result = 0;
	signed int r_val_temp = 4;

#if defined(SWCHR_POWER_PATH)
	adc_result_reg = pmic_get_register_value(PMIC_RG_ADC_OUT_WAKEUP_SWCHR);
	adc_result = (adc_result_reg * r_val_temp * VOLTAGE_FULL_RANGE) / ADC_PRECISE;
	bm_err("[oam] get_hw_ocv (swchr) : adc_result_reg=%d, adc_result=%d, start_sel=%d\n",
		 adc_result_reg, adc_result, pmic_get_register_value(PMIC_STRUP_AUXADC_START_SEL));
#else
	adc_result_reg = pmic_get_register_value(PMIC_RG_ADC_OUT_WAKEUP_PCHR);
	adc_result = (adc_result_reg * r_val_temp * VOLTAGE_FULL_RANGE) / ADC_PRECISE;
	bm_err("[oam] get_hw_ocv (pchr) : adc_result_reg=%d, adc_result=%d, start_sel=%d\n",
		 adc_result_reg, adc_result, pmic_get_register_value(PMIC_STRUP_AUXADC_START_SEL));
#endif

	adc_result += g_hw_ocv_tune_value;
	return adc_result;
#endif
}


/*============================================================*/

static signed int fgauge_initialization(void *data)
{
	return STATUS_OK;
}

static signed int fgauge_read_current(void *data)
{
	return STATUS_OK;
}

static signed int fgauge_read_current_sign(void *data)
{
	return STATUS_OK;
}

static signed int fgauge_read_columb(void *data)
{
	return STATUS_OK;
}


static signed int fgauge_hw_reset(void *data)
{
	return STATUS_OK;
}


static signed int read_adc_v_bat_sense(void *data)
{
#if defined(CONFIG_POWER_EXT)
	*(signed int *)(data) = 4201;
#else
	*(signed int *)(data) =
	    PMIC_IMM_GetOneChannelValue(MT6350_AUX_BATSNS, *(signed int *)(data), 1);
#endif

	return STATUS_OK;
}



static signed int read_adc_v_i_sense(void *data)
{
#if defined(CONFIG_POWER_EXT)
	*(signed int *)(data) = 4202;
#else
	*(signed int *)(data) =
	    PMIC_IMM_GetOneChannelValue(MT6350_AUX_ISENSE, *(signed int *)(data), 1);
#endif

	return STATUS_OK;
}

static signed int read_adc_v_bat_temp(void *data)
{
#if defined(CONFIG_POWER_EXT)
	*(signed int *)(data) = 0;
#else
#if defined(MTK_PCB_TBAT_FEATURE)

	int ret = 0, data[4], i, ret_value = 0, ret_temp = 0;
	int Channel = 1;

	if (IMM_IsAdcInitReady() == 0) {
		bm_print(BM_LOG_CRTI, "[get_tbat_volt] AUXADC is not ready");
		return 0;
	}

	i = times;
	while (i--) {
		ret_value = IMM_GetOneChannelValue(Channel, data, &ret_temp);
		ret += ret_temp;
		bm_print(BM_LOG_FULL, "[get_tbat_volt] ret_temp=%d\n", ret_temp);
	}

	ret = ret * 1500 / 4096;
	ret = ret / times;
	bm_print(BM_LOG_CRTI, "[get_tbat_volt] Battery output mV = %d\n", ret);

	*(signed int *)(data) = ret;

#else
	bm_print(BM_LOG_FULL,
		 "[read_adc_v_charger] return PMIC_IMM_GetOneChannelValue(4,times,1);\n");
	*(signed int *)(data) =
	    PMIC_IMM_GetOneChannelValue(MT6350_AUX_BATON1, *(signed int *)(data), 1);
#endif
#endif

	return STATUS_OK;
}

static signed int read_adc_v_charger(void *data)
{
#if defined(CONFIG_POWER_EXT)
	*(signed int *)(data) = 5001;
#else
	signed int val;

	val = PMIC_IMM_GetOneChannelValue(MT6350_AUX_VCDT, *(signed int *)(data), 1);
	val = (((R_CHARGER_1 + R_CHARGER_2) * 100 * val) / R_CHARGER_2) / 100;

	*(signed int *)(data) = val;
#endif

	return STATUS_OK;
}

static signed int read_hw_ocv(void *data)
{
#if defined(CONFIG_POWER_EXT)
	*(signed int *)(data) = 3999;
#else
	*(signed int *)(data) = get_hw_ocv();
#endif

	return STATUS_OK;
}

static signed int dump_register_fgadc(void *data)
{
	return STATUS_OK;
}

static signed int read_battery_plug_out_status(void *data)
{
	*(signed int *)(data) = get_battery_plug_out_status();

	return STATUS_OK;
}

static signed int fgauge_set_columb_interrupt(void *data)
{
	return STATUS_OK;
}




#if defined(CONFIG_MTK_HAFG_20)
signed char is_lbat_int = KAL_TRUE;
void fg_bat_l_int_handler(void)
{
	bm_print(BM_LOG_CRTI, "[fg_bat_l_int_handler]\n");

	pmic_set_register_value(PMIC_RG_LBAT_IRQ_EN_MIN, 0);
	pmic_set_register_value(PMIC_RG_LBAT_EN_MIN, 0);
	pmic_set_register_value(PMIC_RG_INT_EN_BAT_L, 0);
	is_lbat_int = KAL_TRUE;

}


#endif
static signed int fgauge_set_low_battery_interrupt(void *data)
{
#if defined(CONFIG_MTK_HAFG_20)
	unsigned int voltage = *(unsigned int *)(data);
	unsigned int rawdata;
	static int eint_init;

	if (eint_init == 0) {
		pmic_register_interrupt_callback(2, fg_bat_l_int_handler);
		eint_init = 1;
	}

	is_lbat_int = KAL_FALSE;

	pmic_turn_on_clock(KAL_TRUE);

	rawdata = voltage * 4096 / 1800 / 4;

	bm_print(BM_LOG_CRTI, "[fgauge_set_low_battery_interrupt] Battery voltage %d %d\n", rawdata,
		 voltage);

	/* 0:set interrupt */
	pmic_set_register_value(PMIC_RG_INT_EN_BAT_L, 1);

	/* 1.setup min voltage threshold 3.4v */
	pmic_set_register_value(PMIC_RG_LBAT_VOLT_MIN, rawdata);

	/* 2.setup detection period */
	pmic_set_register_value(PMIC_RG_LBAT_DET_PRD_19_16, 0x0);
	pmic_set_register_value(PMIC_RG_LBAT_DET_PRD_15_0, 0x12f);

	/* 3.setup max/min debounce time */
	pmic_set_register_value(PMIC_RG_LBAT_DEBT_MIN, 0x5a);	/*15mins */

	/* 4.turn on IRQ */
	pmic_set_register_value(PMIC_RG_LBAT_IRQ_EN_MIN, 0x1);

	/* 5. turn on LowBattery Detection */
	pmic_set_register_value(PMIC_RG_LBAT_EN_MIN, 1);
#endif

	return STATUS_OK;
}

static signed int fgauge_get_low_battery_interrupt_status(void *data)
{
#if defined(CONFIG_MTK_HAFG_20)
	*(signed int *)(data) = is_lbat_int;
#else
	*(signed int *)(data) = KAL_FALSE;
#endif

	return STATUS_OK;
}


void trigger_hw_ocv(void)
{
	pmic_set_register_value(PMIC_STRUP_AUXADC_START_SEL, 0x1);
	/* udelay(50); */
	pmic_set_register_value(PMIC_STRUP_AUXADC_START_SW, 0x0);
	/* udelay(50); */
	pmic_set_register_value(PMIC_STRUP_AUXADC_START_SW, 0x1);

	while (pmic_get_register_value(PMIC_RG_ADC_RDY_WAKEUP_PCHR) == 0) {
		bm_print(BM_LOG_FULL, "[trigger_hw_ocv] delay\n");
		udelay(100);
	}
}


static signed int get_refresh_hw_ocv(void *data)
{
#if defined(CONFIG_MTK_HAFG_20)

	signed int hwocv1, hwocv2;

	pmic_turn_on_clock(1);
	udelay(30);
	hwocv1 = get_hw_ocv();
	trigger_hw_ocv();
	hwocv2 = get_hw_ocv();
	bm_print(BM_LOG_CRTI, "[bat_get_zcv]%d %d\n", hwocv1, hwocv2);
	pmic_set_register_value(PMIC_STRUP_AUXADC_START_SEL, 0x0);
	pmic_turn_on_clock(0);

	*(signed int *)(data) = hwocv2;

#else
	*(signed int *)(data) = 0;
#endif
	return STATUS_OK;
}


static signed int (*bm_func[BATTERY_METER_CMD_NUMBER]) (void *data);

signed int bm_ctrl_cmd(BATTERY_METER_CTRL_CMD cmd, void *data)
{
	signed int status = STATUS_UNSUPPORTED;
	static signed int init = -1;

	if (init == -1) {
		init = 0;
		bm_func[BATTERY_METER_CMD_HW_FG_INIT] = fgauge_initialization;
		bm_func[BATTERY_METER_CMD_GET_HW_FG_CURRENT] = fgauge_read_current;
		bm_func[BATTERY_METER_CMD_GET_HW_FG_CURRENT_SIGN] = fgauge_read_current_sign;
		bm_func[BATTERY_METER_CMD_GET_HW_FG_CAR] = fgauge_read_columb;
		bm_func[BATTERY_METER_CMD_HW_RESET] = fgauge_hw_reset;
		bm_func[BATTERY_METER_CMD_GET_ADC_V_BAT_SENSE] = read_adc_v_bat_sense;
		bm_func[BATTERY_METER_CMD_GET_ADC_V_I_SENSE] = read_adc_v_i_sense;
		bm_func[BATTERY_METER_CMD_GET_ADC_V_BAT_TEMP] = read_adc_v_bat_temp;
		bm_func[BATTERY_METER_CMD_GET_ADC_V_CHARGER] = read_adc_v_charger;
		bm_func[BATTERY_METER_CMD_GET_HW_OCV] = read_hw_ocv;
		bm_func[BATTERY_METER_CMD_DUMP_REGISTER] = dump_register_fgadc;
		bm_func[BATTERY_METER_CMD_SET_COLUMB_INTERRUPT] = fgauge_set_columb_interrupt;
		bm_func[BATTERY_METER_CMD_GET_BATTERY_PLUG_STATUS] = read_battery_plug_out_status;
		bm_func[BATTERY_METER_CMD_SET_LOW_BAT_INTERRUPT] = fgauge_set_low_battery_interrupt;
		bm_func[BATTERY_METER_CMD_GET_LOW_BAT_INTERRUPT_STATUS] =
		    fgauge_get_low_battery_interrupt_status;
		bm_func[BATTERY_METER_CMD_GET_REFRESH_HW_OCV] = get_refresh_hw_ocv;

	}

	if (cmd < BATTERY_METER_CMD_NUMBER) {
		if (bm_func[cmd] == NULL) {
			bm_print(BM_LOG_FULL, "[bm_ctrl_cmd] NULL cmd=%d\n", cmd);
			return STATUS_UNSUPPORTED;
		}
		status = bm_func[cmd] (data);
	} else
		return STATUS_UNSUPPORTED;

	return status;
}
