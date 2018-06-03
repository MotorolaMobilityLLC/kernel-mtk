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
#include <linux/time.h>

#include <asm/div64.h>

#include <mt-plat/upmu_common.h>

#include <mach/mtk_battery_property.h>
#include <mach/mtk_pmic.h>
#include <mt-plat/mtk_battery.h>
#include <mt-plat/upmu_common.h>
#include <mt-plat/mtk_rtc_hal_common.h>
#include <mt-plat/mtk_rtc.h>
#include "include/pmic_throttling_dlpt.h"
#ifdef CONFIG_MTK_PMIC_CHIP_MT6336
#include "mt6336.h"
#endif

#ifdef CONFIG_MTK_AUXADC_INTF
#include <mt-plat/mtk_auxadc_intf.h>
#endif

/*============================================================ //
 *define
 *============================================================ //
 */
#define STATUS_OK    0
#define STATUS_UNSUPPORTED    -1
#ifndef CONFIG_MTK_AUXADC_INTF
#define VOLTAGE_FULL_RANGE    1800
#endif
#define ADC_PRECISE           32768	/* 12 bits */

#define UNIT_SOC				10000 /* 0.01 %*/
#define UNIT_OHM				10000 /* 0.1 mOhm to Ohm*/

/* #define UNIT_FGCURRENT    (158122) */	/* 158.122 uA */

#define HAL_AVG_TIMES	60
#define HAL_AVG_DELAY	1
/*============================================================ //
 *global variable
 *============================================================ //
 */

#if !defined(CONFIG_POWER_EXT)
static signed int g_hw_ocv_tune_value;
static bool g_fg_is_charger_exist;
#endif

static signed int ptim_vol;
static signed int ptim_curr;
static signed int g_meta_input_cali_current;

static bool g_fg_is_charging;

static int nag_zcv_mv;
static int nag_c_dltv_mv;
static int tmp_int_lt;
static int tmp_int_ht;
static int fg_bat_int2_ht_en_flag;
static int fg_bat_int2_lt_en_flag;

static int rtc_invalid;
static int is_bat_plugout;
static int bat_plug_out_time;

static struct mutex fg_wlock;


#define SWCHR_POWER_PATH

struct daemon_data dm_data;
struct hw_info_data fg_hw_info;

enum {
	FROM_SW_OCV = 1,
	FROM_6335_PLUG_IN,
	FROM_6335_PON_ON,
	FROM_6336_CHR_IN
};

/* ============================================================ */
/* function prototype */
/* ============================================================ */
static signed int read_ptim_bat_vol(void *data);
static signed int read_ptim_R_curr(void *data);
static signed int read_ptim_rac_val(void *data);
static signed int fgauge_read_current(void *data);
static signed int fgauge_get_time(void *data);
static signed int fgauge_get_soff_time(void *data);
static void read_fg_hw_info_current_1(void);
static signed int fg_set_iavg_intr(void *data);
static void fgauge_read_RTC_boot_status(void);


/* ============================================================ */
/* extern variable */
/* ============================================================ */


/* ============================================================ */
/* extern function */
/* ============================================================ */

/* ============================================================ */
#if 0
void fg_dump_register(void)
{
	unsigned int i = 0;

	bm_err("dump fg register 0x40c=0x%x\n", upmu_get_reg_value(0x40c));
	for (i = 0x2000; i <= 0x207a; i = i + 10) {
		bm_err
		("Reg[0x%x]=0x%x Reg[0x%x]=0x%x Reg[0x%x]=0x%x Reg[0x%x]=0x%x Reg[0x%x]=0x%x\n",
			i, upmu_get_reg_value(i), i + 1, upmu_get_reg_value(i + 1), i + 2,
			upmu_get_reg_value(i + 2), i + 3, upmu_get_reg_value(i + 3), i + 4,
			upmu_get_reg_value(i + 4));

		bm_err
		("Reg[0x%x]=0x%x Reg[0x%x]=0x%x Reg[0x%x]=0x%x Reg[0x%x]=0x%x Reg[0x%x]=0x%x\n",
			i + 5, upmu_get_reg_value(i + 5), i + 6, upmu_get_reg_value(i + 6), i + 7,
			upmu_get_reg_value(i + 7), i + 8, upmu_get_reg_value(i + 8), i + 9,
			upmu_get_reg_value(i + 9));
	}

}
#endif

signed int MV_to_REG_12_value(signed int _reg)
{
	int ret = (_reg * 4096) / (VOLTAGE_FULL_RANGE * 10 * R_VAL_TEMP_3);

	bm_trace("[MV_to_REG_12_value] %d => %d\n", _reg, ret);
	return ret;
}

signed int MV_to_REG_12_temp_value(signed int _reg)
{
	int ret = (_reg * 4096) / (VOLTAGE_FULL_RANGE * 10 * R_VAL_TEMP_2);

	bm_trace("[MV_to_REG_12_temp_value] %d => %d\n", _reg, ret);
	return ret;
}

signed int REG_to_MV_value(signed int _reg)
{
	int ret = (_reg * VOLTAGE_FULL_RANGE * 10 * R_VAL_TEMP_3) / ADC_PRECISE;

	bm_trace("[REG_to_MV_value] %d => %d\n", _reg, ret);
	return ret;
}

signed int MV_to_REG_value(signed int _mv)
{
	int ret = (_mv * ADC_PRECISE) / (VOLTAGE_FULL_RANGE * 10 * R_VAL_TEMP_3);

	bm_trace("[MV_to_REG_value] %d => %d\n", _mv, ret);
	return ret;
}

/*============================================================//*/

#if defined(CONFIG_POWER_EXT)
 /**/
#else

static unsigned int fg_get_data_ready_status(void)
{
	unsigned int ret = 0;
	unsigned int temp_val = 0;

	ret = pmic_read_interface(PMIC_FG_LATCHDATA_ST_ADDR, &temp_val, 0xFFFF, 0x0);
/*
	bm_info("[fg_get_data_ready_status] Reg[0x%x]=0x%x\r\n", PMIC_FG_LATCHDATA_ST_ADDR,
		 temp_val);
*/
	temp_val =
	(temp_val & (PMIC_FG_LATCHDATA_ST_MASK << PMIC_FG_LATCHDATA_ST_SHIFT))
	>> PMIC_FG_LATCHDATA_ST_SHIFT;

	return temp_val;
}
#endif

#if 0
void preloader_init(void)
{
	int fg_reset_status = pmic_get_register_value(PMIC_FG_RSTB_STATUS);
	signed int efuse_cal;
	int fg_curr_time;
	int shutdown_pmic_time;
	int do_init_fgadc_reset;
	int ret;
	int hw_id, sw_id;

	/* eFuse */
	efuse_cal = pmic_get_register_value(PMIC_RG_FGADC_GAINERROR_CAL);
	pmic_set_register_value(PMIC_FG_GAIN, efuse_cal);

	hw_id = upmu_get_reg_value(0x0200);
	sw_id = upmu_get_reg_value(0x0202);


	ret = fgauge_get_time(&fg_curr_time);

	ret = fgauge_get_soff_time(&shutdown_pmic_time);

	if (fg_reset_status == 0)
		do_init_fgadc_reset = 1;
	else if ((shutdown_pmic_time != 0) && (fg_curr_time - shutdown_pmic_time > PMIC_SHUTDOWN_TIME * 60))
		do_init_fgadc_reset = 1;
	else
		do_init_fgadc_reset = 0;

	if (do_init_fgadc_reset == 1) {
		pmic_set_register_value(PMIC_FG_SOFF_SLP_EN, 1);

		/* reset CON1 */
		ret = pmic_config_interface(MT6335_FGADC_CON1, 0x1F04, 0xFFFF, 0x0);
		mdelay(1);
		ret = pmic_config_interface(MT6335_FGADC_CON1, 0x0000, 0x1F04, 0x0);

	}

	/* Elbrus E1 */
	if (hw_id == 0x3510) {
		pmic_set_register_value(PMIC_RG_BATON_CHRDET_SW, 1);
		pmic_set_register_value(PMIC_RG_BATON_CHRDET_TEST_MODE, 1);

		bm_err("0x01E0 = 0x%x  0x01DE = 0x%x before\n", upmu_get_reg_value(0x15E0), upmu_get_reg_value(0x15DE));
		upmu_set_reg_value(0x15E0, 0x1);
		upmu_set_reg_value(0x15DE, 0x1);
		mdelay(10);
		bm_err("0x01E0 = 0x%x  0x01DE = 0x%x after\n", upmu_get_reg_value(0x15E0), upmu_get_reg_value(0x15DE));

	}

	bm_err(
		"[preloader_init] fg_reset_status %d do_init_fgadc_reset %d fg_curr_time %d shutdown_pmic_time %d hw_id 0x%x sw_id 0x%x\n",
			fg_reset_status, do_init_fgadc_reset, fg_curr_time, shutdown_pmic_time, hw_id, sw_id);
}
#endif

static signed int fgauge_initialization(void *data)
{
#if defined(CONFIG_POWER_EXT)
	/* */
#else
#if 1
	/*preloader_init();*/
	fgauge_read_RTC_boot_status();
#else
	int fg_reset_status;
	int fg_curr_time;
	int shutdown_pmic_time;
	int do_init_fgadc_reset;
	int ret;

	fg_reset_status = pmic_get_register_value(PMIC_FG_RSTB_STATUS);
	ret = fgauge_get_time(&fg_curr_time);
	ret = fgauge_get_soff_time(&shutdown_pmic_time);

	if (fg_reset_status == 0)
		do_init_fgadc_reset = 1;
	else if ((shutdown_pmic_time != 0) && (fg_curr_time - shutdown_pmic_time > PMIC_SHUTDOWN_TIME * 60))
		do_init_fgadc_reset = 1;
	else
		do_init_fgadc_reset = 0;

	bm_err(
		"[preloader_init] fg_reset_status %d do_init_fgadc_reset %d fg_curr_time %d shutdown_pmic_time %d\n",
			fg_reset_status, do_init_fgadc_reset, fg_curr_time, shutdown_pmic_time);
#endif
#endif
	return STATUS_OK;
}

static signed int fgauge_read_current(void *data)
{
#if defined(CONFIG_POWER_EXT)
	*(signed int *) (data) = 0;
#else
	unsigned short uvalue16 = 0;
	signed int dvalue = 0;
	int m = 0;
	long long Temp_Value = 0;
	unsigned int ret = 0;

	if (Bat_EC_ctrl.debug_fg_curr_en == 1) {
		*(signed int *) (data) = Bat_EC_ctrl.debug_fg_curr_value;
		return STATUS_OK;
	}

	/* HW Init
	 *(1)    i2c_write (0x60, 0xC8, 0x01); // Enable VA2
	 *(2)    i2c_write (0x61, 0x15, 0x00); // Enable FGADC clock for digital
	 *(3)    i2c_write (0x61, 0x69, 0x28); // Set current mode, auto-calibration mode and 32KHz clock source
	 *(4)    i2c_write (0x61, 0x69, 0x29); // Enable FGADC
	 */

	/* Read HW Raw Data
	 *(1)    Set READ command
	 */
	ret = pmic_config_interface(MT6335_FGADC_CON1, 0x0001, 0x000F, 0x0);
	/*(2)     Keep i2c read when status = 1 (0x06) */
	m = 0;
		while (fg_get_data_ready_status() == 0) {
			m++;
			if (m > 1000) {
				bm_err(
				"[fgauge_read_current] fg_get_data_ready_status timeout 1 !\r\n");
				break;
			}
		}
	/*
	 *(3)    Read FG_CURRENT_OUT[15:08]
	 *(4)    Read FG_CURRENT_OUT[07:00]
	 */
	uvalue16 = pmic_get_register_value(PMIC_FG_CURRENT_OUT);	/*mt6325_upmu_get_fg_current_out(); */
	bm_trace("[fgauge_read_current] : FG_CURRENT = %x\r\n", uvalue16);
	/*
	 *(5)    (Read other data)
	 *(6)    Clear status to 0
	 */
	ret = pmic_config_interface(MT6335_FGADC_CON1, 0x0008, 0x000F, 0x0);
	/*
	 *(7)    Keep i2c read when status = 0 (0x08)
	 * while ( fg_get_sw_clear_status() != 0 )
	 */
	m = 0;
		while (fg_get_data_ready_status() != 0) {
			m++;
			if (m > 1000) {
				bm_err(
					"[fgauge_read_current] fg_get_data_ready_status timeout 2 !\r\n");
				break;
			}
		}
	/*(8)    Recover original settings */
	ret = pmic_config_interface(MT6335_FGADC_CON1, 0x0000, 0x000F, 0x0);

	/*calculate the real world data    */
	dvalue = (unsigned int) uvalue16;
		if (dvalue == 0) {
			Temp_Value = (long long) dvalue;
			g_fg_is_charging = false;
		} else if (dvalue > 32767) {
			/* > 0x8000 */
			Temp_Value = (long long) (dvalue - 65535);
			Temp_Value = Temp_Value - (Temp_Value * 2);
			g_fg_is_charging = false;
		} else {
			Temp_Value = (long long) dvalue;
			g_fg_is_charging = true;
		}

	Temp_Value = Temp_Value * UNIT_FGCURRENT;
	do_div(Temp_Value, 100000);
	dvalue = (unsigned int) Temp_Value;

	if (g_fg_is_charging == true)
		bm_trace("[fgauge_read_current] current(charging) = %d mA\r\n",
			 dvalue);
	else
		bm_trace("[fgauge_read_current] current(discharging) = %d mA\r\n",
			 dvalue);

		/* Auto adjust value */
		if (fg_cust_data.r_fg_value != 100) {
			bm_trace(
			"[fgauge_read_current] Auto adjust value due to the Rfg is %d\n Ori current=%d, ",
			fg_cust_data.r_fg_value, dvalue);

			dvalue = (dvalue * 100) / fg_cust_data.r_fg_value;

			bm_trace("[fgauge_read_current] new current=%d\n", dvalue);
		}

		bm_trace("[fgauge_read_current] ori current=%d\n", dvalue);

		dvalue = ((dvalue * fg_cust_data.car_tune_value) / 1000);

		bm_debug("[fgauge_read_current] final current=%d (ratio=%d)\n",
			 dvalue, fg_cust_data.car_tune_value);

		*(signed int *) (data) = dvalue;
#endif

		return STATUS_OK;
	}



signed int fgauge_read_IM_current(void *data)
{
#if defined(CONFIG_POWER_EXT)
		*(signed int *) (data) = 0;
#else
		unsigned short uvalue16 = 0;
		signed int dvalue = 0;
		/*int m = 0;*/
		long long Temp_Value = 0;
		/*unsigned int ret = 0;*/

		uvalue16 = pmic_get_register_value(PMIC_FG_R_CURR);
		bm_trace("[fgauge_read_IM_current] : FG_CURRENT = %x\r\n",
			 uvalue16);

		/*calculate the real world data    */
		dvalue = (unsigned int) uvalue16;
		if (dvalue == 0) {
			Temp_Value = (long long) dvalue;
			g_fg_is_charging = false;
		} else if (dvalue > 32767) {
			/* > 0x8000 */
			Temp_Value = (long long) (dvalue - 65535);
			Temp_Value = Temp_Value - (Temp_Value * 2);
			g_fg_is_charging = false;
		} else {
			Temp_Value = (long long) dvalue;
			g_fg_is_charging = true;
		}

		Temp_Value = Temp_Value * UNIT_FGCURRENT;
		do_div(Temp_Value, 100000);
		dvalue = (unsigned int) Temp_Value;

		if (g_fg_is_charging == true)
			bm_trace(
			"[fgauge_read_IM_current] current(charging) = %d mA\r\n",
			dvalue);
		else
			bm_trace(
			"[fgauge_read_IM_current] current(discharging) = %d mA\r\n",
			dvalue);

		/* Auto adjust value */
		if (fg_cust_data.r_fg_value != 100) {
			bm_trace(
			"[fgauge_read_IM_current] Auto adjust value due to the Rfg is %d\n Ori current=%d, ",
			fg_cust_data.r_fg_value, dvalue);

			dvalue = (dvalue * 100) / fg_cust_data.r_fg_value;

			bm_trace("[fgauge_read_IM_current] new current=%d\n",
			dvalue);
		}

		bm_trace("[fgauge_read_IM_current] ori current=%d\n", dvalue);

		dvalue = ((dvalue * fg_cust_data.car_tune_value) / 1000);

		bm_debug("[fgauge_read_IM_current] final current=%d (ratio=%d)\n",
			 dvalue, fg_cust_data.car_tune_value);

		*(signed int *) (data) = dvalue;
#endif

		return STATUS_OK;
}


static signed int fgauge_read_current_sign(void *data)
{
	*(bool *) (data) = g_fg_is_charging;

	return STATUS_OK;
}

static signed int fg_get_current_iavg_valid(void *data)
{
	int iavg_valid = pmic_get_register_value(PMIC_FG_IAVG_VLD);

	*(signed int *) (data) = iavg_valid;
	bm_debug("[fg_get_current_iavg_valid] iavg_valid %d\n", iavg_valid);

	return STATUS_OK;
}

static signed int fg_get_current_iavg(void *data)
{
#if defined(CONFIG_POWER_EXT)
		*(signed int *) (data) = 0;
		return STATUS_OK;
#else
	long long fg_iavg_reg = 0;
	long long fg_iavg_reg_tmp = 0;
	long long fg_iavg_ma = 0;
	int fg_iavg_reg_27_16 = 0;
	int fg_iavg_reg_15_00 = 0;
	int sign_bit = 0;
	int is_bat_charging;
	int ret, m;

	/* Set Read Latchdata */
	ret = pmic_config_interface(MT6335_FGADC_CON1, 0x0001, 0x000F, 0x0);
	m = 0;
		while (fg_get_data_ready_status() == 0) {
			m++;
			if (m > 1000) {
				bm_err(
				"[fg_get_current_iavg] fg_get_data_ready_status timeout 1 !\r\n");
				break;
			}
		}

	if (pmic_get_register_value(PMIC_FG_IAVG_VLD) == 1) {
		fg_iavg_reg_27_16 = pmic_get_register_value(PMIC_FG_IAVG_27_16);
		fg_iavg_reg_15_00 = pmic_get_register_value(PMIC_FG_IAVG_15_00);
		fg_iavg_reg = (fg_iavg_reg_27_16 << 16) + fg_iavg_reg_15_00;
		sign_bit = (fg_iavg_reg_27_16 & 0x800) >> 11;

		if (sign_bit) {
			fg_iavg_reg_tmp = fg_iavg_reg;
			/*fg_iavg_reg = fg_iavg_reg_tmp - 0xfffffff - 1;*/
			fg_iavg_reg = 0xfffffff - fg_iavg_reg_tmp + 1;
		}

		if (sign_bit == 1)
			is_bat_charging = 0;	/* discharge */
		else
			is_bat_charging = 1;	/* charge */

		fg_iavg_ma = fg_iavg_reg * UNIT_FG_IAVG * fg_cust_data.car_tune_value;
		bm_trace("[fg_get_current_iavg] fg_iavg_ma %lld fg_iavg_reg %lld fg_iavg_reg_tmp %lld\n",
			fg_iavg_ma, fg_iavg_reg, fg_iavg_reg_tmp);

		do_div(fg_iavg_ma, 1000000);
		bm_trace("[fg_get_current_iavg] fg_iavg_ma %lld\n", fg_iavg_ma);

		do_div(fg_iavg_ma, fg_cust_data.r_fg_value);
		bm_trace("[fg_get_current_iavg] fg_iavg_ma %lld\n", fg_iavg_ma);


		if (sign_bit == 1)
			fg_iavg_ma = 0 - fg_iavg_ma;

		bm_trace("[fg_get_current_iavg] fg_iavg_ma %lld fg_iavg_reg %lld r_fg_value %d 27_16 0x%x 15_00 0x%x\n",
			fg_iavg_ma, fg_iavg_reg, fg_cust_data.r_fg_value, fg_iavg_reg_27_16, fg_iavg_reg_15_00);
		fg_hw_info.current_avg = fg_iavg_ma;
		fg_hw_info.current_avg_sign = sign_bit;
		bm_trace("[fg_get_current_iavg] PMIC_FG_IAVG_VLD == 1\n");
	} else {
		read_fg_hw_info_current_1();
		fg_hw_info.current_avg = fg_hw_info.current_1;
		if (fg_hw_info.current_1 < 0)
			fg_hw_info.current_avg_sign = 1;
		bm_debug("[fg_get_current_iavg] PMIC_FG_IAVG_VLD != 1, avg %d, current_1 %d\n",
			fg_hw_info.current_avg, fg_hw_info.current_1);
	}

	/* recover read */
	ret = pmic_config_interface(MT6335_FGADC_CON1, 0x0008, 0x000F, 0x0);
	m = 0;
		while (fg_get_data_ready_status() != 0) {
			m++;
			if (m > 1000) {
				bm_err(
					"[fg_get_current_iavg] fg_get_data_ready_status timeout 2 !\r\n");
				break;
			}
		}
	ret = pmic_config_interface(MT6335_FGADC_CON1, 0x0000, 0x000F, 0x0);

	*(signed int *)(data) = fg_hw_info.current_avg;

	bm_debug("[fg_get_current_iavg] %d\n", *(signed int *) (data));

	return STATUS_OK;
#endif
}

signed int fgauge_set_columb_interrupt_internal2(void *data, int is_ht)
{
#if defined(CONFIG_POWER_EXT)
	return STATUS_OK;
#else
	signed int upperbound_31_16 = 0;
	signed int upperbound_15_14 = 0;
	signed int lowbound_31_16 = 0;
	signed int lowbound_15_14 = 0;
	signed int thr = *(unsigned int *) (data);

	if (is_ht == 1)
		bm_trace("fgauge_set_columb_interrupt_internal2 ht_thr %d\n", thr);
	if (is_ht == 0)
		bm_trace("fgauge_set_columb_interrupt_internal2 lt_thr %d\n", thr);


	/* gap to register-base */
	thr = thr * CAR_TO_REG_FACTOR / 10;

	if (fg_cust_data.r_fg_value != 100)
		thr = (thr * fg_cust_data.r_fg_value) / 100;

	thr = ((thr * 1000) / fg_cust_data.car_tune_value);

	if (is_ht) {
		upperbound_31_16 = (thr & 0xffff0000) >> 16;
		upperbound_15_14 = (thr & 0xffff) >> 14;

		bm_trace("[fgauge_set_columb_interrupt_internal2] upper_thr 0x%x 31_16 0x%x 15_14 0x%x\n",
			thr, upperbound_31_16, upperbound_15_14);

		pmic_enable_interrupt(FG_BAT0_INT_H_NO, 0, "GM30");
		pmic_set_register_value(PMIC_FG_BAT0_HTH_15_14, upperbound_15_14);
		pmic_set_register_value(PMIC_FG_BAT0_HTH_31_16, upperbound_31_16);
		mdelay(1);

		if (fg_bat_int2_ht_en_flag == true)
			pmic_enable_interrupt(FG_BAT0_INT_H_NO, 1, "GM30");

		bm_trace(
			"[fgauge_set_columb_interrupt_internal2] high:[0xcb0]=0x%x 0x%x\r\n",
			pmic_get_register_value(PMIC_FG_BAT0_HTH_15_14),
			pmic_get_register_value(PMIC_FG_BAT0_HTH_31_16));

	} else {
		lowbound_31_16 = (thr & 0xffff0000) >> 16;
		lowbound_15_14 = (thr & 0xffff) >> 14;

		bm_trace("[fgauge_set_columb_interrupt_internal2] low_thr 0x%x 31_16 0x%x 15_14 0x%x\n",
			thr, lowbound_31_16, lowbound_15_14);


		pmic_enable_interrupt(FG_BAT0_INT_L_NO, 0, "GM30");
		pmic_set_register_value(PMIC_FG_BAT0_LTH_15_14, lowbound_15_14);
		pmic_set_register_value(PMIC_FG_BAT0_LTH_31_16, lowbound_31_16);
		mdelay(1);

		if (fg_bat_int2_lt_en_flag == true)
			pmic_enable_interrupt(FG_BAT0_INT_L_NO, 1, "GM30");

		bm_trace(
			"[fgauge_set_columb_interrupt_internal2] low:[0xcae]=0x%x 0x%x\r\n",
			pmic_get_register_value(PMIC_FG_BAT0_LTH_15_14),
			pmic_get_register_value(PMIC_FG_BAT0_LTH_31_16));
	}

	return STATUS_OK;
#endif

}

static signed int fgauge_set_columb_interrupt2_ht(void *data)
{
	return fgauge_set_columb_interrupt_internal2(data, 1);

}

static signed int fgauge_set_columb_interrupt2_lt(void *data)
{
	return fgauge_set_columb_interrupt_internal2(data, 0);
}

static signed int fgauge_set_columb_interrupt2_ht_en(void *data)
{
	int en = *(unsigned int *) (data);
#if 0
	if (en == true)
		pmic_set_register_value(PMIC_RG_INT_EN_FG_BAT0_H, 1);
	else
		pmic_set_register_value(PMIC_RG_INT_EN_FG_BAT0_H, 0);
#else
	if (en == true) {
		fg_bat_int2_ht_en_flag = true;
		pmic_enable_interrupt(FG_BAT0_INT_H_NO, 1, "GM30");
	} else {
		fg_bat_int2_ht_en_flag = false;
		pmic_enable_interrupt(FG_BAT0_INT_H_NO, 0, "GM30");
	}
#endif

	return STATUS_OK;
}

static signed int fgauge_set_columb_interrupt2_lt_en(void *data)
{
		int en = *(unsigned int *) (data);
#if 0
		if (en == true)
			pmic_set_register_value(PMIC_RG_INT_EN_FG_BAT0_L, 1);
		else
			pmic_set_register_value(PMIC_RG_INT_EN_FG_BAT0_L, 0);
#else
		if (en == true) {
			fg_bat_int2_lt_en_flag = true;
			pmic_enable_interrupt(FG_BAT0_INT_L_NO, 1, "GM30");
		} else {
			fg_bat_int2_lt_en_flag = false;
			pmic_enable_interrupt(FG_BAT0_INT_L_NO, 0, "GM30");
		}
#endif

		return STATUS_OK;

}

signed int fgauge_set_columb_interrupt_internal1(void *data, int reset)
{
#if defined(CONFIG_POWER_EXT)
		return STATUS_OK;
#else

	unsigned int uvalue32_CAR = 0;
	unsigned int uvalue32_CAR_MSB = 0;
	signed int upperbound = 0, lowbound = 0;
	signed int upperbound_31_16 = 0, upperbound_15_14 = 0;
	signed int lowbound_31_16 = 0, lowbound_15_14 = 0;


	signed short m;
	unsigned int car = *(unsigned int *) (data);
	unsigned int ret = 0;
	signed int value32_CAR;

	bm_trace("fgauge_set_columb_interrupt_internal1 car=%d\n", car);


	if (car == 0) {
		pmic_set_register_value(PMIC_RG_INT_EN_FG_BAT1_H, 0);
		pmic_set_register_value(PMIC_RG_INT_EN_FG_BAT1_L, 0);
		bm_trace(
			"[fgauge_set_columb_interrupt_internal1] low:[0xcae]=0x%x 0x%x  high:[0xcb0]=0x%x 0x%x now:0x%x 0x%x %d %d \r\n",
			 pmic_get_register_value(PMIC_FG_BAT1_LTH_15_14),
			 pmic_get_register_value(PMIC_FG_BAT1_LTH_31_16),
			 pmic_get_register_value(PMIC_FG_BAT1_HTH_15_14),
			 pmic_get_register_value(PMIC_FG_BAT1_HTH_31_16),
			 pmic_get_register_value(PMIC_FG_CAR_15_00),
			 pmic_get_register_value(PMIC_FG_CAR_31_16),
			 pmic_get_register_value(PMIC_RG_INT_EN_FG_BAT1_L),
			 pmic_get_register_value(PMIC_RG_INT_EN_FG_BAT1_H));
		return STATUS_OK;
	}

	if (car == 0xfffff) {
		pmic_set_register_value(PMIC_RG_INT_EN_FG_BAT1_H, 1);
		pmic_set_register_value(PMIC_RG_INT_EN_FG_BAT1_L, 1);
			bm_trace(
			 "[fgauge_set_columb_interrupt_internal1] low:[0xcae]=0x%x 0x%x  high:[0xcb0]=0x%x 0x%x now:0x%x 0x%x %d %d \r\n",
			 pmic_get_register_value(PMIC_FG_BAT1_LTH_15_14),
			 pmic_get_register_value(PMIC_FG_BAT1_LTH_31_16),
			 pmic_get_register_value(PMIC_FG_BAT1_HTH_15_14),
			 pmic_get_register_value(PMIC_FG_BAT1_HTH_31_16),
			 pmic_get_register_value(PMIC_FG_CAR_15_00),
			 pmic_get_register_value(PMIC_FG_CAR_31_16),
			 pmic_get_register_value(PMIC_RG_INT_EN_FG_BAT1_L),
			 pmic_get_register_value(PMIC_RG_INT_EN_FG_BAT1_H));
		return STATUS_OK;
	}

/*
 * HW Init
 *(1)    i2c_write (0x60, 0xC8, 0x01); // Enable VA2
 *(2)    i2c_write (0x61, 0x15, 0x00); // Enable FGADC clock for digital
 *(3)    i2c_write (0x61, 0x69, 0x28); // Set current mode, auto-calibration mode and 32KHz clock source
 *(4)    i2c_write (0x61, 0x69, 0x29); // Enable FGADC
 *
 *Read HW Raw Data
 *(1)    Set READ command
*/
	if (reset == 0) {
		ret = pmic_config_interface(MT6335_FGADC_CON1, 0x0001, 0x1F0F, 0x0);
	} else {
		ret = pmic_config_interface(MT6335_FGADC_CON1, 0x1F05, 0xFF0F, 0x0);
		bm_err("[fgauge_set_columb_interrupt_internal1] reset fgadc 0x1F05\n");
	}

	/*(2)    Keep i2c read when status = 1 (0x06) */
	m = 0;
	while (fg_get_data_ready_status() == 0) {
		m++;
		if (m > 1000) {
			bm_err(
				 "[fgauge_set_columb_interrupt_internal1] fg_get_data_ready_status timeout 1 !");
			break;
		}
	}
/*
 *(3)    Read FG_CURRENT_OUT[28:14]
 *(4)    Read FG_CURRENT_OUT[31]
*/
	value32_CAR = (pmic_get_register_value(PMIC_FG_CAR_15_00));
	value32_CAR |= ((pmic_get_register_value(PMIC_FG_CAR_31_16)) & 0xffff) << 16;

	uvalue32_CAR_MSB = (pmic_get_register_value(PMIC_FG_CAR_31_16) & 0x8000) >> 15;

	bm_trace(
		"[fgauge_set_columb_interrupt_internal1] FG_CAR = 0x%x   uvalue32_CAR_MSB:0x%x 0x%x 0x%x\r\n",
		uvalue32_CAR, uvalue32_CAR_MSB, (pmic_get_register_value(PMIC_FG_CAR_15_00)),
		(pmic_get_register_value(PMIC_FG_CAR_31_16)));

	/* gap to register-base */
	car = car * CAR_TO_REG_FACTOR / 10;

	if (fg_cust_data.r_fg_value != 100)
		car = (car * fg_cust_data.r_fg_value) / 100;

	car = ((car * 1000) / fg_cust_data.car_tune_value);

	upperbound = value32_CAR;
	lowbound = value32_CAR;

	bm_trace("[fgauge_set_columb_interrupt_internal1] upper = 0x%x:%d  low=0x%x:%d  diff_car=0x%x:%d\r\n",
		 upperbound, upperbound, lowbound, lowbound, car, car);

	upperbound = upperbound + car;
	lowbound = lowbound - car;

	upperbound_31_16 = (upperbound & 0xffff0000) >> 16;
	lowbound_31_16 = (lowbound & 0xffff0000) >> 16;

	upperbound_15_14 = (upperbound & 0xffff) >> 14;
	lowbound_15_14 = (lowbound & 0xffff) >> 14;

	bm_trace("[fgauge_set_columb_interrupt_internal1]final upper = 0x%x:%d  low=0x%x:%d  car=0x%x:%d\r\n",
		 upperbound, upperbound, lowbound, lowbound, car, car);

	bm_trace("[fgauge_set_columb_interrupt_internal1]final upper 0x%x 0x%x 0x%x low 0x%x 0x%x 0x%x car=0x%x\n",
		 upperbound, upperbound_31_16, upperbound_15_14, lowbound, lowbound_31_16, lowbound_15_14, car);

	pmic_enable_interrupt(FG_BAT1_INT_H_NO, 0, "GM30");
	pmic_enable_interrupt(FG_BAT1_INT_L_NO, 0, "GM30");
	pmic_set_register_value(PMIC_FG_BAT1_LTH_15_14, lowbound_15_14);
	pmic_set_register_value(PMIC_FG_BAT1_LTH_31_16, lowbound_31_16);
	pmic_set_register_value(PMIC_FG_BAT1_HTH_15_14, upperbound_15_14);
	pmic_set_register_value(PMIC_FG_BAT1_HTH_31_16, upperbound_31_16);
	mdelay(1);

	pmic_enable_interrupt(FG_BAT1_INT_H_NO, 1, "GM30");
	pmic_enable_interrupt(FG_BAT1_INT_L_NO, 1, "GM30");

	bm_trace(
		"[fgauge_set_columb_interrupt_internal1] low:[0xcae]=0x%x 0x%x   high:[0xcb0]=0x%x 0x%x\r\n",
		pmic_get_register_value(PMIC_FG_BAT1_LTH_15_14),
		pmic_get_register_value(PMIC_FG_BAT1_LTH_31_16),
		pmic_get_register_value(PMIC_FG_BAT1_HTH_15_14),
		pmic_get_register_value(PMIC_FG_BAT1_HTH_31_16));

	return STATUS_OK;
#endif

}


static signed int fgauge_set_columb_interrupt1(void *data)
{
	return fgauge_set_columb_interrupt_internal1(data, 0);
}

static signed int fgauge_read_columb_internal(void *data, int reset, int precise)
{
#if defined(CONFIG_POWER_EXT)
	*(signed int *) (data) = 0;
#else
	unsigned int uvalue32_CAR = 0;
	unsigned int uvalue32_CAR_MSB = 0;
	unsigned int temp_CAR_15_0 = 0;
	unsigned int temp_CAR_31_16 = 0;
	signed int dvalue_CAR = 0;
	int m = 0;
	long long Temp_Value = 0;
	unsigned int ret = 0;

/*
 * HW Init
 *(1)    i2c_write (0x60, 0xC8, 0x01); // Enable VA2
 *(2)    i2c_write (0x61, 0x15, 0x00); // Enable FGADC clock for digital
 *(3)    i2c_write (0x61, 0x69, 0x28); // Set current mode, auto-calibration mode and 32KHz clock source
 *(4)    i2c_write (0x61, 0x69, 0x29); // Enable FGADC
 *
 * Read HW Raw Data
 *(1)    Set READ command
*/

	/*fg_dump_register();*/

	if (reset == 0)
		ret = pmic_config_interface(MT6335_FGADC_CON1, 0x0001, 0x1F05, 0x0);
	else {
		ret = pmic_config_interface(MT6335_FGADC_CON1, 0x0705, 0x1F05, 0x0); /*weiching0803*/
		bm_err("[fgauge_read_columb_internal] reset fgadc 0x0705\n");
	}

	/*(2)    Keep i2c read when status = 1 (0x06) */
	m = 0;
	while (fg_get_data_ready_status() == 0) {
		m++;
		if (m > 1000) {
			bm_err(
				 "[fgauge_read_columb_internal] fg_get_data_ready_status timeout 1 !\r\n");
			break;
		}
	}
/*
 *(3)    Read FG_CURRENT_OUT[28:14]
 *(4)    Read FG_CURRENT_OUT[31]
*/

#if 1
	temp_CAR_15_0 = pmic_get_register_value(PMIC_FG_CAR_15_00);
	temp_CAR_31_16 = pmic_get_register_value(PMIC_FG_CAR_31_16);

	uvalue32_CAR = temp_CAR_15_0 >> 11;
	uvalue32_CAR |= ((temp_CAR_31_16) & 0x7FFF) << 5;

	uvalue32_CAR_MSB = (temp_CAR_31_16 & 0x8000) >> 15;

	bm_trace("[fgauge_read_columb_internal] temp_CAR_15_0 = 0x%x  temp_CAR_31_16 = 0x%x\n",
		 temp_CAR_15_0, temp_CAR_31_16);

	bm_trace("[fgauge_read_columb_internal] FG_CAR = 0x%x\r\n",
		 uvalue32_CAR);
	bm_trace(
		 "[fgauge_read_columb_internal] uvalue32_CAR_MSB = 0x%x\r\n",
		 uvalue32_CAR_MSB);

#else
	uvalue32_CAR = (pmic_get_register_value(PMIC_FG_CAR_15_00)) >> 11;
	uvalue32_CAR |= ((pmic_get_register_value(PMIC_FG_CAR_31_16)) & 0x0FFF) << 5;

	uvalue32_CAR_MSB = (pmic_get_register_value(PMIC_FG_CAR_31_16) & 0x8000) >> 15;

	bm_notice("[fgauge_read_columb_internal] FG_CAR = 0x%x\r\n",
		 uvalue32_CAR);
	bm_notice(
		 "[fgauge_read_columb_internal] uvalue32_CAR_MSB = 0x%x\r\n",
		 uvalue32_CAR_MSB);
#endif
/*
 *(5)    (Read other data)
 *(6)    Clear status to 0
*/
	ret = pmic_config_interface(MT6335_FGADC_CON1, 0x0008, 0x000F, 0x0);
/*
 *(7)    Keep i2c read when status = 0 (0x08)
 * while ( fg_get_sw_clear_status() != 0 )
*/
	m = 0;
	while (fg_get_data_ready_status() != 0) {
		m++;
		if (m > 1000) {
			bm_err(
				 "[fgauge_read_columb_internal] fg_get_data_ready_status timeout 2 !\r\n");
			break;
		}
	}
	/*(8)    Recover original settings */
	ret = pmic_config_interface(MT6335_FGADC_CON1, 0x0000, 0x000F, 0x0);

/*calculate the real world data    */
	dvalue_CAR = (signed int) uvalue32_CAR;

	if (uvalue32_CAR == 0) {
		Temp_Value = 0;
	} else if (uvalue32_CAR == 0xfffff) {
		Temp_Value = 0;
	} else if (uvalue32_CAR_MSB == 0x1) {
		/* dis-charging */
		Temp_Value = (long long) (dvalue_CAR - 0xfffff);	/* keep negative value */
		Temp_Value = Temp_Value - (Temp_Value * 2);
	} else {
		/*charging */
		Temp_Value = (long long) dvalue_CAR;
	}

#if 0
	Temp_Value = (((Temp_Value * 35986) / 10) + (5)) / 10;	/*[28:14]'s LSB=359.86 uAh */
#else
	/* 0.1 mAh */
	Temp_Value = Temp_Value * UNIT_FGCAR / 1000;
	do_div(Temp_Value, 10);
	Temp_Value = Temp_Value + 5;
	do_div(Temp_Value, 10);
#endif

#if 0
	dvalue_CAR = Temp_Value / 1000;	/*mAh */
#else
	if (uvalue32_CAR_MSB == 0x1)
		dvalue_CAR = (signed int) (Temp_Value - (Temp_Value * 2));	/* keep negative value */
	else
		dvalue_CAR = (signed int) Temp_Value;
#endif

	bm_trace("[fgauge_read_columb_internal] dvalue_CAR = %d\r\n",
		 dvalue_CAR);

	/*#if (OSR_SELECT_7 == 1) */
/*Auto adjust value*/
	if (fg_cust_data.r_fg_value != 100) {
		bm_trace(
			 "[fgauge_read_columb_internal] Auto adjust value deu to the Rfg is %d\n Ori CAR=%d, ",
			 fg_cust_data.r_fg_value, dvalue_CAR);

		dvalue_CAR = (dvalue_CAR * 100) / fg_cust_data.r_fg_value;

		bm_trace("[fgauge_read_columb_internal] new CAR=%d\n",
			 dvalue_CAR);
	}

	dvalue_CAR = ((dvalue_CAR * fg_cust_data.car_tune_value) / 1000);

	bm_debug("[fgauge_read_columb_internal] CAR=%d r_fg_value=%d car_tune_value=%d\n",
				 dvalue_CAR, fg_cust_data.r_fg_value, fg_cust_data.car_tune_value);

	*(signed int *) (data) = dvalue_CAR;
#endif

	return STATUS_OK;
}

signed int fgauge_get_time(void *data)
{
#if defined(CONFIG_POWER_EXT)
	*(signed int *) (data) = 0;
	return STATUS_OK;
#else
	unsigned int time_29_16, time_15_00, ret_time;
	int m = 0;
	unsigned int ret = 0;
	unsigned long time = 0;

	ret = pmic_config_interface(MT6335_FGADC_CON1, 0x0001, 0x1F05, 0x0);
	/*(2)    Keep i2c read when status = 1 (0x06) */
	m = 0;
	while (fg_get_data_ready_status() == 0) {
		m++;
		if (m > 1000) {
			bm_err(
				 "[fgauge_get_time] fg_get_data_ready_status timeout 1 !\r\n");
			break;
		}
	}

	time_15_00 = pmic_get_register_value(PMIC_FG_NTER_15_00);
	time_29_16 = pmic_get_register_value(PMIC_FG_NTER_29_16);

	time = time_15_00;
	time |= time_29_16 << 16;
	time = time * UNIT_TIME / 100;
	ret_time = time;

	bm_trace(
		 "[fgauge_get_time] low:0x%x high:0x%x rtime:0x%lx 0x%x!\r\n",
		 time_15_00, time_29_16, time, ret_time);


	ret = pmic_config_interface(MT6335_FGADC_CON1, 0x0008, 0x000F, 0x0);

	m = 0;
	while (fg_get_data_ready_status() != 0) {
		m++;
		if (m > 1000) {
			bm_err(
				 "[fgauge_get_time] fg_get_data_ready_status timeout 2 !\r\n");
			break;
		}
	}
	/*(8)    Recover original settings */
	ret = pmic_config_interface(MT6335_FGADC_CON1, 0x0000, 0x000F, 0x0);

	*(unsigned int *) (data) = ret_time;

	return STATUS_OK;
#endif
}

signed int fgauge_get_soff_time(void *data)
{
#if defined(CONFIG_POWER_EXT)
	*(signed int *) (data) = 0;
	return STATUS_OK;
#else
	unsigned int soff_time_29_16, soff_time_15_00, ret_time;
	int m = 0;
	unsigned int ret = 0;
	unsigned long time = 0;

	ret = pmic_config_interface(MT6335_FGADC_CON1, 0x0001, 0x1F05, 0x0);
	/*(2)    Keep i2c read when status = 1 (0x06) */
	m = 0;
	while (fg_get_data_ready_status() == 0) {
		m++;
		if (m > 1000) {
			bm_err(
				 "[fgauge_get_time] fg_get_data_ready_status timeout 1 !\r\n");
			break;
		}
	}

	soff_time_15_00 = pmic_get_register_value(PMIC_FG_SOFF_TIME_15_00);
	soff_time_29_16 = pmic_get_register_value(PMIC_FG_SOFF_TIME_29_16);

	time = soff_time_15_00;
	time |= soff_time_29_16 << 16;
	time = time * UNIT_TIME / 100;
	ret_time = time;

	bm_trace(
		 "[fgauge_get_soff_time] low:0x%x high:0x%x rtime:0x%lx 0x%x!\r\n",
		 soff_time_15_00, soff_time_29_16, time, ret_time);


	ret = pmic_config_interface(MT6335_FGADC_CON1, 0x0008, 0x000F, 0x0);

	m = 0;
	while (fg_get_data_ready_status() != 0) {
		m++;
		if (m > 1000) {
			bm_err(
				 "[fgauge_get_soff_time] fg_get_data_ready_status timeout 2 !\r\n");
			break;
		}
	}
	/*(8)    Recover original settings */
	ret = pmic_config_interface(MT6335_FGADC_CON1, 0x0000, 0x000F, 0x0);

	*(unsigned int *) (data) = ret_time;

	return STATUS_OK;
#endif
}

signed int fgauge_set_time_interrupt(void *data)
{
#if defined(CONFIG_POWER_EXT)
	*(signed int *) (data) = 0;
	return STATUS_OK;
#else
	unsigned int time_29_16, time_15_00;
	int m = 0;
	unsigned int ret = 0;
	unsigned long time = 0, time2;
	unsigned long now;
	unsigned int offsetTime = *(unsigned int *) (data);

	if (offsetTime == 0) {
		pmic_enable_interrupt(FG_TIME_NO, 0, "GM30");
		return STATUS_OK;
	}

	do {
	ret = pmic_config_interface(MT6335_FGADC_CON1, 0x0001, 0x1F05, 0x0);
	/*(2)    Keep i2c read when status = 1 (0x06) */
	m = 0;
	while (fg_get_data_ready_status() == 0) {
		m++;
		if (m > 1000) {
			bm_err(
				 "[fgauge_set_time_interrupt] fg_get_data_ready_status timeout 1 !\r\n");
			break;
		}
	}

	time_15_00 = pmic_get_register_value(PMIC_FG_NTER_15_00);
	time_29_16 = pmic_get_register_value(PMIC_FG_NTER_29_16);

	time = time_15_00;
	time |= time_29_16 << 16;
		now = time;
	time = time + offsetTime * 100 / UNIT_TIME;

	bm_debug(
			 "[fgauge_set_time_interrupt] now:%ld time:%ld\r\n",
			 now/2, time/2);
	ret = pmic_config_interface(MT6335_FGADC_CON1, 0x0008, 0x000F, 0x0);

	m = 0;
	while (fg_get_data_ready_status() != 0) {
		m++;
		if (m > 1000) {
			bm_err(
				 "[fgauge_set_time_interrupt] fg_get_data_ready_status timeout 2 !\r\n");
			break;
		}
	}
	/*(8)    Recover original settings */
	ret = pmic_config_interface(MT6335_FGADC_CON1, 0x0000, 0x000F, 0x0);

	pmic_enable_interrupt(FG_TIME_NO, 0, "GM30");
	pmic_set_register_value(PMIC_FG_TIME_HTH_15_00, (time & 0xffff));
	pmic_set_register_value(PMIC_FG_TIME_HTH_29_16, ((time & 0x3fff0000) >> 16));
	pmic_enable_interrupt(FG_TIME_NO, 1, "GM30");


		/*read again to confirm */
		ret = pmic_config_interface(MT6335_FGADC_CON1, 0x0001, 0x1F05, 0x0);
		/*(2)    Keep i2c read when status = 1 (0x06) */
		m = 0;
		while (fg_get_data_ready_status() == 0) {
			m++;
			if (m > 1000) {
				bm_err(
					 "[fgauge_set_time_interrupt] fg_get_data_ready_status timeout 1 !\r\n");
				break;
			}
		}

		time_15_00 = pmic_get_register_value(PMIC_FG_NTER_15_00);
		time_29_16 = pmic_get_register_value(PMIC_FG_NTER_29_16);
		time2 = time_15_00;
		time2 |= time_29_16 << 16;

		bm_debug(
			 "[fgauge_set_time_interrupt] now:%ld time:%ld\r\n",
			 time2/2, time/2);
		ret = pmic_config_interface(MT6335_FGADC_CON1, 0x0008, 0x000F, 0x0);

		m = 0;
		while (fg_get_data_ready_status() != 0) {
			m++;
			if (m > 1000) {
				bm_err(
					 "[fgauge_set_time_interrupt] fg_get_data_ready_status timeout 2 !\r\n");
				break;
			}
		}
		/*(8)    Recover original settings */
		ret = pmic_config_interface(MT6335_FGADC_CON1, 0x0000, 0x000F, 0x0);

		bm_trace(
			 "[fgauge_set_time_interrupt] low:0x%x high:0x%x time:%ld %ld\r\n",
			 pmic_get_register_value(PMIC_FG_TIME_HTH_15_00),
			 pmic_get_register_value(PMIC_FG_TIME_HTH_29_16), time, time2);
	} while (time2 >= time);
	return STATUS_OK;
#endif

}


static signed int fgauge_read_columb_accurate(void *data)
{

#if defined(SOC_BY_3RD_FG)
	*(kal_int32 *)(data) = bq27531_get_remaincap();
	return STATUS_OK;

#else
	return fgauge_read_columb_internal(data, 0, 1);
#endif
}


static signed int fgauge_hw_reset(void *data)
{
#if defined(CONFIG_POWER_EXT)
	 /**/
#else
	volatile unsigned int val_car = 1;
	unsigned int val_car_temp = 1;
	unsigned int ret = 0;

	bm_trace("[fgauge_hw_reset] : Start \r\n");

	while (val_car != 0x0) {
		ret = pmic_config_interface(MT6335_FGADC_CON1, 0x0600, 0x1F00, 0x0);
		bm_err("[fgauge_hw_reset] reset fgadc 0x0600\n");

		fgauge_read_columb_internal(&val_car_temp, 0, 0);
		val_car = val_car_temp;
	}

	bm_trace("[fgauge_hw_reset] : End \r\n");
#endif

	return STATUS_OK;
}

void Intr_Number_to_Name(char *intr_name, int intr_no)
{
	switch (intr_no) {
	case FG_INTR_0:
		sprintf(intr_name, "FG_INTR_INIT");
		break;

	case FG_INTR_TIMER_UPDATE:
		sprintf(intr_name, "FG_INTR_TIMER_UPDATE");
		break;

	case FG_INTR_BAT_CYCLE:
		sprintf(intr_name, "FG_INTR_BAT_CYCLE");
		break;

	case FG_INTR_CHARGER_OUT:
		sprintf(intr_name, "FG_INTR_CHARGER_OUT");
		break;

	case FG_INTR_CHARGER_IN:
		sprintf(intr_name, "FG_INTR_CHARGER_IN");
		break;

	case FG_INTR_FG_TIME:
		sprintf(intr_name, "FG_INTR_FG_TIME");
		break;

	case FG_INTR_BAT_INT1_HT:
		sprintf(intr_name, "FG_INTR_COULOMB_HT");
		break;

	case FG_INTR_BAT_INT1_LT:
		sprintf(intr_name, "FG_INTR_COULOMB_LT");
		break;

	case FG_INTR_BAT_INT2_HT:
		sprintf(intr_name, "FG_INTR_UISOC_HT");
		break;

	case FG_INTR_BAT_INT2_LT:
		sprintf(intr_name, "FG_INTR_UISOC_LT");
		break;

	case FG_INTR_BAT_TMP_HT:
		sprintf(intr_name, "FG_INTR_BAT_TEMP_HT");
		break;

	case FG_INTR_BAT_TMP_LT:
		sprintf(intr_name, "FG_INTR_BAT_TEMP_LT");
		break;

	case FG_INTR_BAT_TIME_INT:
		sprintf(intr_name, "FG_INTR_BAT_TIME_INT");		/* fixme : don't know what it is */
		break;

	case FG_INTR_NAG_C_DLTV:
		sprintf(intr_name, "FG_INTR_NAFG_VOLTAGE");
		break;

	case FG_INTR_FG_ZCV:
		sprintf(intr_name, "FG_INTR_FG_ZCV");
		break;

	case FG_INTR_SHUTDOWN:
		sprintf(intr_name, "FG_INTR_SHUTDOWN");
		break;

	case FG_INTR_RESET_NVRAM:
		sprintf(intr_name, "FG_INTR_RESET_NVRAM");
		break;

	case FG_INTR_BAT_PLUGOUT:
		sprintf(intr_name, "FG_INTR_BAT_PLUGOUT");
		break;

	case FG_INTR_IAVG:
		sprintf(intr_name, "FG_INTR_IAVG");
		break;

	case FG_INTR_VBAT2_L:
		sprintf(intr_name, "FG_INTR_VBAT2_L");
		break;

	case FG_INTR_VBAT2_H:
		sprintf(intr_name, "FG_INTR_VBAT2_H");
		break;

	case FG_INTR_CHR_FULL:
		sprintf(intr_name, "FG_INTR_CHR_FULL");
		break;

	case FG_INTR_DLPT_SD:
		sprintf(intr_name, "FG_INTR_DLPT_SD");
		break;

	case FG_INTR_BAT_TMP_C_HT:
		sprintf(intr_name, "FG_INTR_BAT_TMP_C_HT");
		break;

	case FG_INTR_BAT_TMP_C_LT:
		sprintf(intr_name, "FG_INTR_BAT_TMP_C_LT");
		break;


	default:
		sprintf(intr_name, "FG_INTR_UNKNOWN");
		bm_err("[Intr_Number_to_Name] unknown intr %d\n", intr_no);
		break;
	}
}

void read_fg_hw_info_time(void)
{
	unsigned int time_29_16, time_15_00;
	unsigned long time = 0;

	time_15_00 = pmic_get_register_value(PMIC_FG_NTER_15_00);
	time_29_16 = pmic_get_register_value(PMIC_FG_NTER_29_16);

	time = time_15_00;
	time |= time_29_16 << 16;
	time = time * UNIT_TIME / 100;
	fg_hw_info.time = time;
}

void read_fg_hw_info_ncar(void)
{
	unsigned int uvalue32_NCAR = 0;
	unsigned int uvalue32_NCAR_MSB = 0;
	unsigned int temp_NCAR_15_0 = 0;
	unsigned int temp_NCAR_31_16 = 0;
	signed int dvalue_NCAR = 0;
	long long Temp_Value = 0;

	temp_NCAR_15_0 = pmic_get_register_value(PMIC_FG_NCAR_15_00);
	temp_NCAR_31_16 = pmic_get_register_value(PMIC_FG_NCAR_31_16);

	uvalue32_NCAR = temp_NCAR_15_0 >> 11;
	uvalue32_NCAR |= ((temp_NCAR_31_16) & 0x7FFF) << 5;

	uvalue32_NCAR_MSB = (temp_NCAR_31_16 & 0x8000) >> 15;

	/*calculate the real world data    */
	dvalue_NCAR = (signed int) uvalue32_NCAR;

	if (uvalue32_NCAR == 0) {
		Temp_Value = 0;
	} else if (uvalue32_NCAR == 0xfffff) {
		Temp_Value = 0;
	} else if (uvalue32_NCAR_MSB == 0x1) {
		/* dis-charging */
		Temp_Value = (long long) (dvalue_NCAR - 0xfffff);	/* keep negative value */
		Temp_Value = Temp_Value - (Temp_Value * 2);
	} else {
		/*charging */
		Temp_Value = (long long) dvalue_NCAR;
	}

	/* 0.1 mAh */
	Temp_Value = Temp_Value * UNIT_FGCAR / 1000;
	do_div(Temp_Value, 10);
	Temp_Value = Temp_Value + 5;
	do_div(Temp_Value, 10);

	if (uvalue32_NCAR_MSB == 0x1)
		dvalue_NCAR = (signed int) (Temp_Value - (Temp_Value * 2));	/* keep negative value */
	else
		dvalue_NCAR = (signed int) Temp_Value;

	/*Auto adjust value*/
	if (fg_cust_data.r_fg_value != 100)
		dvalue_NCAR = (dvalue_NCAR * 100) / fg_cust_data.r_fg_value;

	fg_hw_info.ncar = ((dvalue_NCAR * fg_cust_data.car_tune_value) / 1000);

}


void read_fg_hw_info_car(void)
{
	unsigned int uvalue32_CAR = 0;
	unsigned int uvalue32_CAR_MSB = 0;
	unsigned int temp_CAR_15_0 = 0;
	unsigned int temp_CAR_31_16 = 0;
	signed int dvalue_CAR = 0;
	long long Temp_Value = 0;

	temp_CAR_15_0 = pmic_get_register_value(PMIC_FG_CAR_15_00);
	temp_CAR_31_16 = pmic_get_register_value(PMIC_FG_CAR_31_16);

	uvalue32_CAR = temp_CAR_15_0 >> 11;
	uvalue32_CAR |= ((temp_CAR_31_16) & 0x7FFF) << 5;

	uvalue32_CAR_MSB = (temp_CAR_31_16 & 0x8000) >> 15;

	/*calculate the real world data    */
	dvalue_CAR = (signed int) uvalue32_CAR;

	if (uvalue32_CAR == 0) {
		Temp_Value = 0;
	} else if (uvalue32_CAR == 0xfffff) {
		Temp_Value = 0;
	} else if (uvalue32_CAR_MSB == 0x1) {
		/* dis-charging */
		Temp_Value = (long long) (dvalue_CAR - 0xfffff);	/* keep negative value */
		Temp_Value = Temp_Value - (Temp_Value * 2);
	} else {
		/*charging */
		Temp_Value = (long long) dvalue_CAR;
	}

	/* 0.1 mAh */
	Temp_Value = Temp_Value * UNIT_FGCAR / 1000;
	do_div(Temp_Value, 10);
	Temp_Value = Temp_Value + 5;
	do_div(Temp_Value, 10);

	if (uvalue32_CAR_MSB == 0x1)
		dvalue_CAR = (signed int) (Temp_Value - (Temp_Value * 2));	/* keep negative value */
	else
		dvalue_CAR = (signed int) Temp_Value;

	/*Auto adjust value*/
	if (fg_cust_data.r_fg_value != 100)
		dvalue_CAR = (dvalue_CAR * 100) / fg_cust_data.r_fg_value;

	fg_hw_info.car = ((dvalue_CAR * fg_cust_data.car_tune_value) / 1000);

}

void read_fg_hw_info_Iavg(int *is_iavg_valid)
{
	long long fg_iavg_reg = 0;
	long long fg_iavg_reg_tmp = 0;
	long long fg_iavg_ma = 0;
	int fg_iavg_reg_27_16 = 0;
	int fg_iavg_reg_15_00 = 0;
	int sign_bit = 0;
	int is_bat_charging;
	int valid_bit;

	valid_bit = pmic_get_register_value(PMIC_FG_IAVG_VLD);
	*is_iavg_valid = valid_bit;

	if (valid_bit == 1) {
		fg_iavg_reg_27_16 = pmic_get_register_value(PMIC_FG_IAVG_27_16);
		fg_iavg_reg_15_00 = pmic_get_register_value(PMIC_FG_IAVG_15_00);
		fg_iavg_reg = (fg_iavg_reg_27_16 << 16) + fg_iavg_reg_15_00;
		sign_bit = (fg_iavg_reg_27_16 & 0x800) >> 11;

		if (sign_bit) {
			fg_iavg_reg_tmp = fg_iavg_reg;
			fg_iavg_reg = 0xfffffff - fg_iavg_reg_tmp + 1;
		}

		if (sign_bit)
			is_bat_charging = 0;	/* discharge */
		else
			is_bat_charging = 1;	/* charge */

		fg_iavg_ma = fg_iavg_reg * UNIT_FG_IAVG * fg_cust_data.car_tune_value;
		do_div(fg_iavg_ma, 1000000);
		do_div(fg_iavg_ma, fg_cust_data.r_fg_value);

		if (sign_bit == 1)
			fg_iavg_ma = 0 - fg_iavg_ma;

		fg_hw_info.current_avg = fg_iavg_ma;
		fg_hw_info.current_avg_sign = sign_bit;
	} else {
		read_fg_hw_info_current_1();
		fg_hw_info.current_avg = fg_hw_info.current_1;
		is_bat_charging = 0;	/* discharge */
	}

	bm_debug(
		"[read_fg_hw_info_Iavg] fg_iavg_reg 0x%llx fg_iavg_reg_tmp 0x%llx 27_16 0x%x 15_00 0x%x\n",
			fg_iavg_reg, fg_iavg_reg_tmp, fg_iavg_reg_27_16, fg_iavg_reg_15_00);
	bm_debug(
		"[read_fg_hw_info_Iavg] is_bat_charging %d fg_iavg_ma 0x%llx\n",
			is_bat_charging, fg_iavg_ma);


}

void read_fg_hw_info_current_2(void)
{
	long long fg_current_2_reg;
	signed int dvalue;
	long long Temp_Value;
	int sign_bit = 0;

	fg_current_2_reg = pmic_get_register_value(PMIC_FG_CIC2);

	/*calculate the real world data    */
	dvalue = (unsigned int) fg_current_2_reg;
	if (dvalue == 0) {
		Temp_Value = (long long) dvalue;
		sign_bit = 0;
	} else if (dvalue > 32767) {
		/* > 0x8000 */
		Temp_Value = (long long) (dvalue - 65535);
		Temp_Value = Temp_Value - (Temp_Value * 2);
		sign_bit = 1;
	} else {
		Temp_Value = (long long) dvalue;
		sign_bit = 0;
	}

	Temp_Value = Temp_Value * UNIT_FGCURRENT;
	do_div(Temp_Value, 100000);
	dvalue = (unsigned int) Temp_Value;

	if (fg_cust_data.r_fg_value != 100)
		dvalue = (dvalue * 100) / fg_cust_data.r_fg_value;

	if (sign_bit == 1)
		dvalue = dvalue - (dvalue * 2);

	fg_hw_info.current_2 = ((dvalue * fg_cust_data.car_tune_value) / 1000);

}

void read_fg_hw_info_current_1(void)
{
	long long fg_current_1_reg;
	signed int dvalue;
	long long Temp_Value;
	int sign_bit = 0;

	fg_current_1_reg = pmic_get_register_value(PMIC_FG_CURRENT_OUT);

	/*calculate the real world data    */
	dvalue = (unsigned int) fg_current_1_reg;
	if (dvalue == 0) {
		Temp_Value = (long long) dvalue;
		sign_bit = 0;
	} else if (dvalue > 32767) {
		/* > 0x8000 */
		Temp_Value = (long long) (dvalue - 65535);
		Temp_Value = Temp_Value - (Temp_Value * 2);
		sign_bit = 1;
	} else {
		Temp_Value = (long long) dvalue;
		sign_bit = 0;
	}

	Temp_Value = Temp_Value * UNIT_FGCURRENT;
	do_div(Temp_Value, 100000);
	dvalue = (unsigned int) Temp_Value;

	if (fg_cust_data.r_fg_value != 100)
		dvalue = (dvalue * 100) / fg_cust_data.r_fg_value;

	if (sign_bit == 1)
		dvalue = dvalue - (dvalue * 2);

	fg_hw_info.current_1 = ((dvalue * fg_cust_data.car_tune_value) / 1000);

}

static signed int read_fg_hw_info(void *data)
{
#if defined(CONFIG_POWER_EXT)
	*(signed int *) (data) = 0;
	return STATUS_OK;
#else
	int ret, m;
	int intr_no;
	char intr_name[32];
	int is_iavg_valid;
	int iavg_th;

	intr_no = *(signed int *) (data);
	Intr_Number_to_Name(intr_name, intr_no);

	/* Set Read Latchdata */
	ret = pmic_config_interface(MT6335_FGADC_CON1, 0x0001, 0x000F, 0x0);
	m = 0;
		while (fg_get_data_ready_status() == 0) {
			m++;
			if (m > 1000) {
				bm_err(
				"[read_fg_hw_info] fg_get_data_ready_status timeout 1 !\r\n");
				break;
			}
		}

	/* Current_1 */
	read_fg_hw_info_current_1();

	/* Current_2 */
	read_fg_hw_info_current_2();

	/* Iavg */
	read_fg_hw_info_Iavg(&is_iavg_valid);
	if ((is_iavg_valid == 1) && (FG_status.iavg_intr_flag == 0)) {
		bm_trace("[read_fg_hw_info] set first fg_set_iavg_intr %d %d\n",
			is_iavg_valid, FG_status.iavg_intr_flag);
		FG_status.iavg_intr_flag = 1;
		iavg_th = fg_cust_data.diff_iavg_th;
		ret = fg_set_iavg_intr(&iavg_th);
	} else if (is_iavg_valid == 0) {
		FG_status.iavg_intr_flag = 0;
		pmic_set_register_value(PMIC_RG_INT_EN_FG_IAVG_H, 0);
		pmic_set_register_value(PMIC_RG_INT_EN_FG_IAVG_L, 0);
		pmic_enable_interrupt(FG_IAVG_H_NO, 0, "GM30");
		pmic_enable_interrupt(FG_IAVG_L_NO, 0, "GM30");
		bm_trace("[read_fg_hw_info] doublecheck first fg_set_iavg_intr %d %d\n",
			is_iavg_valid, FG_status.iavg_intr_flag);
	}
	bm_trace("[read_fg_hw_info] thirdcheck first fg_set_iavg_intr %d %d\n",
		is_iavg_valid, FG_status.iavg_intr_flag);

	/* Car */
	read_fg_hw_info_car();

	/* Ncar */
	read_fg_hw_info_ncar();

	/* Time */
	read_fg_hw_info_time();


	/* recover read */
	ret = pmic_config_interface(MT6335_FGADC_CON1, 0x0008, 0x000F, 0x0);
	m = 0;
		while (fg_get_data_ready_status() != 0) {
			m++;
			if (m > 1000) {
				bm_err(
					"[read_fg_hw_info] fg_get_data_ready_status timeout 2 !\r\n");
				break;
			}
		}
	ret = pmic_config_interface(MT6335_FGADC_CON1, 0x0000, 0x000F, 0x0);


	*(signed int *) (data) = 0;
	bm_err("[FGADC_intr_end][%s][read_fg_hw_info] curr_1 %d curr_2 %d Iavg %d sign %d car %d ncar %d time %d\n",
		intr_name, fg_hw_info.current_1, fg_hw_info.current_2,
		fg_hw_info.current_avg, fg_hw_info.current_avg_sign,
		fg_hw_info.car, fg_hw_info.ncar, fg_hw_info.time);
	return STATUS_OK;
#endif
}

static signed int read_nafg_vbat(void *data)
{
#if defined(CONFIG_POWER_EXT)
	*(signed int *) (data) = 4202;
	return STATUS_OK;
#else
	int nag_vbat_reg, nag_vbat_mv;

	nag_vbat_reg = pmic_get_register_value(PMIC_AUXADC_ADC_OUT_NAG);
	nag_vbat_mv = REG_to_MV_value(nag_vbat_reg);

	*(signed int *) (data) = nag_vbat_mv;

	bm_err("[FGADC_intr_end][read_nafg_vbat] nag_vbat_reg 0x%x nag_vbat_mv %d\n",
		nag_vbat_reg, nag_vbat_mv);

	return STATUS_OK;
#endif
}

static signed int read_adc_v_bat_sense(void *data)
{
#if defined(CONFIG_POWER_EXT)
	*(signed int *) (data) = 4201;
#else
#if defined(SWCHR_POWER_PATH)
	*(signed int *) (data) =
#ifndef CONFIG_MTK_AUXADC_INTF
		PMIC_IMM_GetOneChannelValue(PMIC_AUX_ISENSE_AP, *(signed int *) (data), 1);
#else
		pmic_get_auxadc_value(AUXADC_LIST_BATADC);
#endif
#else
	*(signed int *) (data) =
		PMIC_IMM_GetOneChannelValue(PMIC_AUX_BATSNS_AP, *(signed int *) (data), 1);
#endif
#endif

	return STATUS_OK;
}


#if 0
static signed int read_adc_v_i_sense(void *data)
{
#if defined(CONFIG_POWER_EXT)
	*(signed int *) (data) = 4202;
#else
#if defined(SWCHR_POWER_PATH)
	*(signed int *) (data) =
#ifndef CONFIG_MTK_AUXADC_INTF
	PMIC_IMM_GetOneChannelValue(PMIC_AUX_BATSNS_AP, *(signed int *) (data), 1);
#else
	0; /* MT6335 doesn't have ISENSE AUXADC */
#endif
#else
	*(signed int *) (data) =
	PMIC_IMM_GetOneChannelValue(PMIC_AUX_ISENSE_AP, *(signed int *) (data), 1);
#endif
#endif

	return STATUS_OK;
}
#endif

static signed int read_adc_v_bat_temp(void *data)
{
#if defined(CONFIG_POWER_EXT)
	*(signed int *) (data) = 0;
#else
#if defined(MTK_PCB_TBAT_FEATURE)
	/* no HW support */
#else
	bm_trace(
	 "[read_adc_v_bat_temp] return PMIC_IMM_GetOneChannelValue(4,times,1);\n");
	*(signed int *) (data) =
#ifndef CONFIG_MTK_AUXADC_INTF
	PMIC_IMM_GetOneChannelValue(PMIC_AUX_BATON_AP, *(signed int *) (data), 1);
#else
	pmic_get_auxadc_value(AUXADC_LIST_BATTEMP_35);
#endif
#endif
#endif

	return STATUS_OK;
}

static signed int read_adc_v_charger(void *data)
{
#if defined(CONFIG_POWER_EXT)
	*(signed int *) (data) = 5001;
#else
	signed int val;

#ifndef CONFIG_MTK_AUXADC_INTF
	val = PMIC_IMM_GetOneChannelValue(PMIC_AUX_VCDT_AP, *(signed int *) (data), 1);
#else
	val = pmic_get_auxadc_value(AUXADC_LIST_VCDT);
#endif
	val =
		(((fg_cust_data.r_charger_1 +
		fg_cust_data.r_charger_2) * 100 * val) /
		fg_cust_data.r_charger_2) / 100;
	*(signed int *) (data) = val;
#endif

	return STATUS_OK;
}

static signed int fg_set_fg_quse(void *data)
{
	dm_data.quse = *(unsigned int *) (data);

	return STATUS_OK;
}

static signed int fg_set_fg_resistence_bat(void *data)
{
	dm_data.nafg_resistance_bat = *(unsigned int *) (data);

	return STATUS_OK;
}

static signed int fg_set_fg_dc_ratio(void *data)
{
	dm_data.dc_ratio = *(unsigned int *) (data);

	return STATUS_OK;
}

static signed int fg_is_charger_exist(void *data)
{
	int is_charger_exist;

	is_charger_exist = pmic_get_register_value(PMIC_RGS_CHRDET);
	*(unsigned int *) (data) = is_charger_exist;

	bm_trace("[fg_is_charger_exist] %d\n",
			*(signed int *) (data));

	return STATUS_OK;
}


static signed int fg_is_battery_exist(void *data)
{
#if defined(CONFIG_POWER_EXT)
	*(signed int *) (data) = 1;
	return STATUS_OK;
#else
	int temp;
	int is_bat_exist;
	int hw_id = upmu_get_reg_value(0x0200);

	temp = pmic_get_register_value(PMIC_RGS_BATON_UNDET);

	if (temp == 0)
		is_bat_exist = 1;
	else
		is_bat_exist = 0;

	*(unsigned int *)(data) = is_bat_exist;

	bm_debug("[fg_is_battery_exist] PMIC_RGS_BATON_UNDET = %d\n", *(unsigned int *)(data));

	if (is_bat_exist == 0) {
		if (hw_id == 0x3510) {
			pmic_set_register_value(PMIC_AUXADC_ADC_RDY_WAKEUP_CLR, 1);
			mdelay(1);
			pmic_set_register_value(PMIC_AUXADC_ADC_RDY_WAKEUP_CLR, 0);
		} else {
			pmic_set_register_value(PMIC_AUXADC_ADC_RDY_PWRON_CLR, 1);
			mdelay(1);
			pmic_set_register_value(PMIC_AUXADC_ADC_RDY_PWRON_CLR, 0);
		}
	}
	return STATUS_OK;
#endif
}


static signed int fg_is_bat_charging(void *data)
{
#if defined(CONFIG_POWER_EXT)
	*(signed int *) (data) = 0;
	return STATUS_OK;
#else
	int is_bat_charging;
	int sign_bit;
	int ret, m;
	int fg_iavg_reg_27_16;

	/* Set Read Latchdata */
	ret = pmic_config_interface(MT6335_FGADC_CON1, 0x0001, 0x000F, 0x0);
	m = 0;
		while (fg_get_data_ready_status() == 0) {
			m++;
			if (m > 1000) {
				bm_err(
				"[read_fg_hw_info] fg_get_data_ready_status timeout 1 !\r\n");
				break;
			}
		}

	if (pmic_get_register_value(PMIC_FG_IAVG_VLD) == 1) {
		fg_iavg_reg_27_16 = pmic_get_register_value(PMIC_FG_IAVG_27_16);
		sign_bit = (fg_iavg_reg_27_16 & 0x800) >> 11;
		if (sign_bit == 1)
			is_bat_charging = 0;	/* discharge */
		else
			is_bat_charging = 1;	/* charging */
	} else {
		is_bat_charging = 0;		/* discharge */
	}

	/* recover read */
	ret = pmic_config_interface(MT6335_FGADC_CON1, 0x0008, 0x000F, 0x0);
	m = 0;
		while (fg_get_data_ready_status() != 0) {
			m++;
			if (m > 1000) {
				bm_err(
					"[read_fg_hw_info] fg_get_data_ready_status timeout 2 !\r\n");
				break;
			}
		}
	ret = pmic_config_interface(MT6335_FGADC_CON1, 0x0000, 0x000F, 0x0);

	*(signed int *)(data) = is_bat_charging;

	bm_debug("[fg_is_bat_charging] %d\n", *(signed int *) (data));

	return STATUS_OK;
#endif
}

static signed int fg_set_nafg_en(void *data)
{
	int nafg_en;

	nafg_en = *(signed int *) (data);
	pmic_set_register_value(PMIC_RG_INT_EN_NAG_C_DLTV, nafg_en);
	pmic_set_register_value(PMIC_AUXADC_NAG_IRQ_EN, nafg_en);
	pmic_set_register_value(PMIC_AUXADC_NAG_EN, nafg_en);

	return STATUS_OK;
}

static signed int fg_get_nafg_cnt(void *data)
{
	signed int NAG_C_DLTV_CNT;
	signed int NAG_C_DLTV_CNT_H;

	/*AUXADC_NAG_4*/
	NAG_C_DLTV_CNT = pmic_get_register_value(PMIC_AUXADC_NAG_CNT_15_0);

	/*AUXADC_NAG_5*/
	NAG_C_DLTV_CNT_H = pmic_get_register_value(PMIC_AUXADC_NAG_CNT_25_16);

	*(signed int *) (data) = (NAG_C_DLTV_CNT & 0xffff) + ((NAG_C_DLTV_CNT_H & 0x3ff) << 16);


	bm_debug("[fg_bat_nafg][fgauge_get_nafg_cnt] %d [25_16 %d 15_0 %d]\n",
			*(signed int *) (data), NAG_C_DLTV_CNT_H, NAG_C_DLTV_CNT);

	return STATUS_OK;
}

static signed int fg_get_nafg_dltv(void *data)
{
	signed int NAG_DLTV_reg_value;
	signed int NAG_DLTV_mV_value;

	/*AUXADC_NAG_6*/
	NAG_DLTV_reg_value = pmic_get_register_value(PMIC_AUXADC_NAG_DLTV);

	NAG_DLTV_mV_value = REG_to_MV_value(NAG_DLTV_reg_value);
	*(signed int *) (data) = NAG_DLTV_mV_value;

	bm_debug("[fg_bat_nafg][fgauge_get_nafg_dltv] mV:Reg [%d:%d]\n", NAG_DLTV_mV_value, NAG_DLTV_reg_value);

	return STATUS_OK;
}

static signed int fg_get_nafg_c_dltv(void *data)
{
	signed int NAG_C_DLTV_value;
	signed int NAG_C_DLTV_value_H;
	signed int NAG_C_DLTV_reg_value;
	signed int NAG_C_DLTV_mV_value;


	/*AUXADC_NAG_7*/
	NAG_C_DLTV_value = pmic_get_register_value(PMIC_AUXADC_NAG_C_DLTV_15_0);

	/*AUXADC_NAG_8*/
	NAG_C_DLTV_value_H = pmic_get_register_value(PMIC_AUXADC_NAG_C_DLTV_26_16);

	if (NAG_C_DLTV_value_H == 0)
		NAG_C_DLTV_reg_value = (NAG_C_DLTV_value & 0xffff);
	else
		NAG_C_DLTV_reg_value = (NAG_C_DLTV_value & 0xffff) +
			(((NAG_C_DLTV_value_H | 0xf800) & 0xffff) << 16);

	NAG_C_DLTV_mV_value = REG_to_MV_value(NAG_C_DLTV_reg_value);
	*(signed int *) (data) = NAG_C_DLTV_mV_value;

	bm_debug("[fg_bat_nafg][fgauge_get_nafg_c_dltv] mV:Reg [%d:%d] [26_16 %d 15_00 %d]\n",
			NAG_C_DLTV_mV_value, NAG_C_DLTV_reg_value, NAG_C_DLTV_value_H, NAG_C_DLTV_value);

	return STATUS_OK;

}

void fg_set_nafg_intr_internal(int _prd, int _zcv_mv, int _thr_mv)
{
	int NAG_C_DLTV_Threashold_26_16;
	int NAG_C_DLTV_Threashold_15_0;
	int _zcv_reg = MV_to_REG_value(_zcv_mv);
	int _thr_reg = MV_to_REG_value(_thr_mv);

	NAG_C_DLTV_Threashold_26_16 = (_thr_reg & 0xffff0000) >> 16;
	NAG_C_DLTV_Threashold_15_0 = (_thr_reg & 0x0000ffff);

	pmic_set_register_value(PMIC_RG_INT_EN_NAG_C_DLTV, 1);

	pmic_set_register_value(PMIC_AUXADC_NAG_ZCV, _zcv_reg);

	pmic_set_register_value(PMIC_AUXADC_NAG_C_DLTV_TH_26_16, NAG_C_DLTV_Threashold_26_16);
	pmic_set_register_value(PMIC_AUXADC_NAG_C_DLTV_TH_15_0, NAG_C_DLTV_Threashold_15_0);

	pmic_set_register_value(PMIC_AUXADC_NAG_PRD, _prd);

	pmic_set_register_value(PMIC_AUXADC_NAG_VBAT1_SEL, 0);	/* use Batsns */

	pmic_set_register_value(PMIC_AUXADC_NAG_IRQ_EN, 1);

	pmic_set_register_value(PMIC_AUXADC_NAG_EN, 1);

	bm_debug("[fg_bat_nafg][fgauge_set_nafg_interrupt_internal] time[%d] zcv[%d:%d] thr[%d:%d] 26_16[0x%x] 15_00[0x%x]\n",
		_prd, _zcv_mv, _zcv_reg, _thr_mv, _thr_reg, NAG_C_DLTV_Threashold_26_16, NAG_C_DLTV_Threashold_15_0);

}

static signed int fg_set_nag_zcv(void *data)
{
	nag_zcv_mv = *(unsigned int *) (data);	/* 0.1 mv*/

	return STATUS_OK;
}

static signed int fg_set_nag_c_dltv(void *data)
{
	nag_c_dltv_mv = *(unsigned int *) (data);	/* 0.1 mv*/

	fg_set_nafg_intr_internal(fg_cust_data.nafg_time_setting, nag_zcv_mv, nag_c_dltv_mv);

	return STATUS_OK;
}

static signed int fg_set_fg_bat_tmp_en(void *data)
{
	int en = *(unsigned int *) (data);

	if (en == 0) {
		pmic_set_register_value(PMIC_RG_INT_EN_BAT_TEMP_L, 0);
		pmic_set_register_value(PMIC_RG_INT_EN_BAT_TEMP_H, 0);
		pmic_set_register_value(PMIC_AUXADC_BAT_TEMP_IRQ_EN_MAX, 0);
		pmic_set_register_value(PMIC_AUXADC_BAT_TEMP_IRQ_EN_MIN, 0);
		pmic_set_register_value(PMIC_AUXADC_BAT_TEMP_EN_MAX, 0);
		pmic_set_register_value(PMIC_AUXADC_BAT_TEMP_EN_MIN, 0);
	}

	return STATUS_OK;
}

void fg_set_fg_bat_tmp_int_internal(int tmp_int_lt, int tmp_int_ht)
{
	pmic_set_register_value(PMIC_RG_INT_EN_BAT_TEMP_L, 1);
	pmic_set_register_value(PMIC_RG_INT_EN_BAT_TEMP_H, 1);
	pmic_set_register_value(PMIC_AUXADC_BAT_TEMP_VOLT_MAX, tmp_int_lt);	/* MAX is high temp */
	pmic_set_register_value(PMIC_AUXADC_BAT_TEMP_VOLT_MIN, tmp_int_ht);	/* MAX is low temp */
	pmic_set_register_value(PMIC_AUXADC_BAT_TEMP_DET_PRD_15_0, 3333);	/* unit: ms, 3.33 seconds */
	pmic_set_register_value(PMIC_AUXADC_BAT_TEMP_DET_PRD_19_16, 0);
	pmic_set_register_value(PMIC_AUXADC_BAT_TEMP_DEBT_MAX, 3);			/* debounce 3 => 10s refresh */
	pmic_set_register_value(PMIC_AUXADC_BAT_TEMP_DEBT_MIN, 3);
	pmic_set_register_value(PMIC_AUXADC_BAT_TEMP_IRQ_EN_MAX, 1);
	pmic_set_register_value(PMIC_AUXADC_BAT_TEMP_IRQ_EN_MIN, 1);
	pmic_set_register_value(PMIC_AUXADC_BAT_TEMP_EN_MAX, 1);
	pmic_set_register_value(PMIC_AUXADC_BAT_TEMP_EN_MIN, 1);
}

static signed int fg_set_fg_bat_tmp_int_lt(void *data)
{
	int _tmp_int_value;

	_tmp_int_value = *(unsigned int *) (data);
	tmp_int_lt = MV_to_REG_12_temp_value(_tmp_int_value);

	bm_debug("[FG_TEMP_INT][fg_set_fg_bat_tmp_int_lt] mv:%d reg:%d\n",
		_tmp_int_value, tmp_int_lt);

	return STATUS_OK;
}

static signed int fg_set_fg_bat_tmp_int_ht(void *data)
{
	int _tmp_int_value;

	_tmp_int_value = *(unsigned int *) (data);
	tmp_int_ht = MV_to_REG_12_temp_value(_tmp_int_value);

	bm_debug("[FG_TEMP_INT][fg_set_fg_bat_tmp_int_ht] mv:%d reg:%d\n",
		_tmp_int_value, tmp_int_ht);

	fg_set_fg_bat_tmp_int_internal(tmp_int_lt, tmp_int_ht);

	return STATUS_OK;
}

static signed int fg_set_iavg_intr(void *data)
{
	int iavg_gap = *(unsigned int *) (data);
	int iavg;
	long long iavg_ht, iavg_lt;
	int ret;
	int sign_bit_ht, sign_bit_lt;
	long long fg_iavg_reg_ht, fg_iavg_reg_lt;
	int fg_iavg_lth_28_16, fg_iavg_lth_15_00;
	int fg_iavg_hth_28_16, fg_iavg_hth_15_00;

/* fg_iavg_ma = fg_iavg_reg * UNIT_FG_IAVG * fg_cust_data.car_tune_value */
/* fg_iavg_ma = fg_iavg_ma / 1000 / 1000 / fg_cust_data.r_fg_value; */

	ret = fg_get_current_iavg(&iavg);

	iavg_ht = abs(iavg) + iavg_gap;
	iavg_lt = abs(iavg) - iavg_gap;
	if (iavg_lt <= 0)
		iavg_lt = 0;

	fg_iavg_reg_ht = iavg_ht * 1000 * 1000 * fg_cust_data.r_fg_value;
	if (fg_iavg_reg_ht < 0) {
		sign_bit_ht = 1;
		fg_iavg_reg_ht = 0x1fffffff - fg_iavg_reg_ht + 1;
	} else
		sign_bit_ht = 0;

	do_div(fg_iavg_reg_ht, UNIT_FG_IAVG);
	do_div(fg_iavg_reg_ht, fg_cust_data.car_tune_value);
	if (sign_bit_ht == 1)
		fg_iavg_reg_ht = fg_iavg_reg_ht - (fg_iavg_reg_ht * 2);

	fg_iavg_reg_lt = iavg_lt * 1000 * 1000 * fg_cust_data.r_fg_value;
	if (fg_iavg_reg_lt < 0) {
		sign_bit_lt = 1;
		fg_iavg_reg_lt = 0x1fffffff - fg_iavg_reg_lt + 1;
	} else
		sign_bit_lt = 0;

	do_div(fg_iavg_reg_lt, UNIT_FG_IAVG);
	do_div(fg_iavg_reg_lt, fg_cust_data.car_tune_value);
	if (sign_bit_lt == 1)
		fg_iavg_reg_lt = fg_iavg_reg_lt - (fg_iavg_reg_lt * 2);



	fg_iavg_lth_28_16 = (fg_iavg_reg_lt & 0x1fff0000) >> 16;
	fg_iavg_lth_15_00 = fg_iavg_reg_lt & 0xffff;
	fg_iavg_hth_28_16 = (fg_iavg_reg_ht & 0x1fff0000) >> 16;
	fg_iavg_hth_15_00 = fg_iavg_reg_ht & 0xffff;

	pmic_enable_interrupt(FG_IAVG_H_NO, 0, "GM30");
	pmic_enable_interrupt(FG_IAVG_L_NO, 0, "GM30");

	pmic_set_register_value(PMIC_FG_IAVG_LTH_28_16, fg_iavg_lth_28_16);
	pmic_set_register_value(PMIC_FG_IAVG_LTH_15_00, fg_iavg_lth_15_00);
	pmic_set_register_value(PMIC_FG_IAVG_HTH_28_16, fg_iavg_hth_28_16);
	pmic_set_register_value(PMIC_FG_IAVG_HTH_15_00, fg_iavg_hth_15_00);

	pmic_enable_interrupt(FG_IAVG_H_NO, 1, "GM30");
	if (iavg_lt > 0)
		pmic_enable_interrupt(FG_IAVG_L_NO, 1, "GM30");
	else
		pmic_enable_interrupt(FG_IAVG_L_NO, 0, "GM30");

	bm_debug("[FG_IAVG_INT][fg_set_iavg_intr] iavg %d iavg_gap %d iavg_ht %lld iavg_lt %lld fg_iavg_reg_ht %lld fg_iavg_reg_lt %lld\n",
			iavg, iavg_gap, iavg_ht, iavg_lt, fg_iavg_reg_ht, fg_iavg_reg_lt);
	bm_debug("[FG_IAVG_INT][fg_set_iavg_intr] lt_28_16 0x%x lt_15_00 0x%x ht_28_16 0x%x ht_15_00 0x%x\n",
			fg_iavg_lth_28_16, fg_iavg_lth_15_00, fg_iavg_hth_28_16, fg_iavg_hth_15_00);

	pmic_set_register_value(PMIC_RG_INT_EN_FG_IAVG_H, 1);
	if (iavg_lt > 0)
		pmic_set_register_value(PMIC_RG_INT_EN_FG_IAVG_L, 1);
	else
		pmic_set_register_value(PMIC_RG_INT_EN_FG_IAVG_L, 0);

	return STATUS_OK;
}

void fg_set_zcv_intr_internal(int fg_zcv_det_time, int fg_zcv_car_th)
{
	int fg_zcv_car_thr_h_reg, fg_zcv_car_thr_l_reg;
	long long fg_zcv_car_th_reg = fg_zcv_car_th;


	/*fg_zcv_car_th_reg = (fg_zcv_car_th * 100 * 3600 * 1000) / UNIT_FGCAR_ZCV;*/
	fg_zcv_car_th_reg = (fg_zcv_car_th_reg * 100 * 3600 * 1000);
	do_div(fg_zcv_car_th_reg, UNIT_FGCAR_ZCV);

	if (fg_cust_data.r_fg_value != 100)
		fg_zcv_car_th_reg = (fg_zcv_car_th_reg * fg_cust_data.r_fg_value) / 100;

	fg_zcv_car_th_reg = ((fg_zcv_car_th_reg * 1000) / fg_cust_data.car_tune_value);

	fg_zcv_car_thr_h_reg = (fg_zcv_car_th_reg & 0xffff0000) >> 16;
	fg_zcv_car_thr_l_reg = fg_zcv_car_th_reg & 0x0000ffff;

	pmic_set_register_value(PMIC_FG_ZCV_DET_IV, fg_zcv_det_time);
	pmic_set_register_value(PMIC_FG_ZCV_CAR_TH_15_00, fg_zcv_car_thr_l_reg);
	pmic_set_register_value(PMIC_FG_ZCV_CAR_TH_30_16, fg_zcv_car_thr_h_reg);

	bm_debug("[FG_ZCV_INT][fg_set_zcv_intr_internal] det_time %d mv %d reg %lld 30_16 0x%x 15_00 0x%x\n",
			fg_zcv_det_time, fg_zcv_car_th, fg_zcv_car_th_reg, fg_zcv_car_thr_h_reg, fg_zcv_car_thr_l_reg);
}

static signed int fg_set_zcv_intr_en(void *data)
{
	int en = *(unsigned int *) (data);

	pmic_set_register_value(PMIC_FG_ZCV_DET_EN, en);
	pmic_set_register_value(PMIC_RG_INT_EN_FG_ZCV, en);
	bm_debug("[FG_ZCV_INT][fg_set_zcv_intr_en] En %d\n", en);

	return STATUS_OK;
}

static signed int fg_set_zcv_intr(void *data)
{
	int fg_zcv_det_time = fg_cust_data.zcv_suspend_time;
	int fg_zcv_car_th = *(unsigned int *) (data);
	int fg_zcv_en = 1;

	fg_set_zcv_intr_internal(fg_zcv_det_time, fg_zcv_car_th);
	fg_set_zcv_intr_en(&fg_zcv_en);

	return STATUS_OK;
}

static signed int fg_reset_soff_time(void *data)
{
	int ret;

	ret = pmic_config_interface(MT6335_FGADC_CON1, 0x1000, 0x1000, 0x0);
	mdelay(1);
	ret = pmic_config_interface(MT6335_FGADC_CON1, 0x0000, 0x1000, 0x0);

	return STATUS_OK;
}

static signed int fg_reset_ncar(void *data)
{
	pmic_set_register_value(PMIC_FG_N_CHARGE_RST, 1);
	udelay(200);
	pmic_set_register_value(PMIC_FG_N_CHARGE_RST, 0);

	return STATUS_OK;
}

static signed int fg_set_bat_plugout_intr_en(void *data)
{
	pmic_enable_interrupt(FG_BAT_PLUGOUT_NO, 1, "GM30");
	return STATUS_OK;
}

static signed int fg_set_cycle_interrupt(void *data)
{
	long long car = *(unsigned int *) (data);
	long long carReg;

	pmic_enable_interrupt(FG_N_CHARGE_L_NO, 0, "GM30");

	car = car * CAR_TO_REG_FACTOR;
	if (fg_cust_data.r_fg_value != 100) {
		car = (car * fg_cust_data.r_fg_value);
		do_div(car, 100);
	}

	car = car * 1000;
	do_div(car, fg_cust_data.car_tune_value);

	carReg = car + 5;
	do_div(carReg, 10);
	carReg = 0 - carReg;

	pmic_set_register_value(PMIC_FG_N_CHARGE_LTH_15_14, (carReg & 0xffff)>>14);
	pmic_set_register_value(PMIC_FG_N_CHARGE_LTH_31_16, (carReg & 0xffff0000) >> 16);

	bm_debug("car:%d carR:%lld r:%lld current:low:0x%x high:0x%x target:low:0x%x high:0x%x",
		*(unsigned int *) (data), car, carReg,
		pmic_get_register_value(PMIC_FG_NCAR_15_00),
		pmic_get_register_value(PMIC_FG_NCAR_31_16),
		pmic_get_register_value(PMIC_FG_N_CHARGE_LTH_15_14),
		pmic_get_register_value(PMIC_FG_N_CHARGE_LTH_31_16));

	pmic_enable_interrupt(FG_N_CHARGE_L_NO, 1, "GM30");


	return STATUS_OK;
}

int read_hw_ocv_6335_plug_in(void)
{
#if defined(CONFIG_POWER_EXT)
	return 37000;
	bm_debug("[read_hw_ocv_6335_plug_in] TBD\n");
#else
	signed int adc_rdy = 0;
	signed int adc_result_reg = 0;
	signed int adc_result = 0;

#if defined(SWCHR_POWER_PATH)
	adc_rdy = pmic_get_register_value(PMIC_AUXADC_ADC_RDY_BAT_PLUGIN1);
	adc_result_reg = pmic_get_register_value(PMIC_AUXADC_ADC_OUT_BAT_PLUGIN1);
	adc_result = REG_to_MV_value(adc_result_reg);
	bm_debug("[oam] read_hw_ocv_6335_plug_in (swchr) : adc_result_reg=%d, adc_result=%d, start_sel=%d, rdy=%d\n",
		 adc_result_reg, adc_result, pmic_get_register_value(PMIC_RG_STRUP_AUXADC_START_SEL), adc_rdy);
#else
	adc_rdy = pmic_get_register_value(PMIC_AUXADC_ADC_RDY_BAT_PLUGIN1);
	adc_result_reg = pmic_get_register_value(PMIC_AUXADC_ADC_OUT_BAT_PLUGIN1);
	adc_result = REG_to_MV_value(adc_result_reg);
	bm_debug("[oam] read_hw_ocv_6335_plug_in (pchr) : adc_result_reg=%d, adc_result=%d, start_sel=%d, rdy=%d\n",
		 adc_result_reg, adc_result, pmic_get_register_value(PMIC_RG_STRUP_AUXADC_START_SEL), adc_rdy);
#endif

	if (adc_rdy == 1) {
		pmic_set_register_value(PMIC_AUXADC_ADC_RDY_BAT_PLUGIN_CLR, 1);
		mdelay(1);
		pmic_set_register_value(PMIC_AUXADC_ADC_RDY_BAT_PLUGIN_CLR, 0);
	}

	adc_result += g_hw_ocv_tune_value;
	return adc_result;
#endif
}


int read_hw_ocv_6335_power_on(void)
{
#if defined(CONFIG_POWER_EXT)
	return 37000;
	bm_debug("[read_hw_ocv_6335_power_on] TBD\n");
#else
	signed int adc_rdy = 0;
	signed int adc_result_reg = 0;
	signed int adc_result = 0;
	int hw_id = upmu_get_reg_value(0x0200);

#if defined(SWCHR_POWER_PATH)
	if (hw_id == 0x3510) {
		adc_rdy = pmic_get_register_value(PMIC_AUXADC_ADC_RDY_WAKEUP_PCHR);
		adc_result_reg = pmic_get_register_value(PMIC_AUXADC_ADC_OUT_WAKEUP_PCHR);
	} else {
		adc_rdy = pmic_get_register_value(PMIC_AUXADC_ADC_RDY_PWRON_PCHR);
		adc_result_reg = pmic_get_register_value(PMIC_AUXADC_ADC_OUT_PWRON_PCHR);
	}

	adc_result = REG_to_MV_value(adc_result_reg);
	bm_debug("[oam] read_hw_ocv_6335_power_on (swchr) : adc_result_reg=%d, adc_result=%d, start_sel=%d, rdy=%d\n",
		 adc_result_reg, adc_result, pmic_get_register_value(PMIC_RG_STRUP_AUXADC_START_SEL), adc_rdy);

	if (adc_rdy == 1) {
		if (hw_id == 0x3510) {
			pmic_set_register_value(PMIC_AUXADC_ADC_RDY_WAKEUP_CLR, 1);
			mdelay(1);
			pmic_set_register_value(PMIC_AUXADC_ADC_RDY_WAKEUP_CLR, 0);
		} else {
			pmic_set_register_value(PMIC_AUXADC_ADC_RDY_PWRON_CLR, 1);
			mdelay(1);
			pmic_set_register_value(PMIC_AUXADC_ADC_RDY_PWRON_CLR, 0);
		}
	}
#else
	adc_result_rdy = pmic_get_register_value(PMIC_AUXADC_ADC_RDY_WAKEUP_PCHR);
	adc_result_reg = pmic_get_register_value(PMIC_AUXADC_ADC_OUT_WAKEUP_PCHR);
	adc_result = REG_to_MV_value(adc_result_reg);
	bm_debug("[oam] read_hw_ocv_6335_power_on (pchr) : adc_result_reg=%d, adc_result=%d, start_sel=%d, rdy=%d\n",
		 adc_result_reg, adc_result, pmic_get_register_value(PMIC_RG_STRUP_AUXADC_START_SEL), adc_rdy);

	if (adc_rdy == 1) {
		pmic_set_register_value(PMIC_AUXADC_ADC_RDY_WAKEUP_CLR, 1);
		mdelay(1);
		pmic_set_register_value(PMIC_AUXADC_ADC_RDY_WAKEUP_CLR, 0);
	}
#endif
	adc_result += g_hw_ocv_tune_value;
	return adc_result;
#endif
}

#ifdef CONFIG_MTK_PMIC_CHIP_MT6336
int read_hw_ocv_6336_charger_in(void)
{
#if defined(CONFIG_POWER_EXT)
		return 37000;
#else
		int zcv_36_low, zcv_36_high, zcv_36_rdy;
		int zcv_chrgo_1_lo, zcv_chrgo_1_hi, zcv_chrgo_1_rdy;
		int zcv_fgadc1_lo, zcv_fgadc1_hi, zcv_fgadc1_rdy;
		int hw_ocv_36_reg1 = 0;
		int hw_ocv_36_reg2 = 0;
		int hw_ocv_36_reg3 = 0;
		int hw_ocv_36_1 = 0;
		int hw_ocv_36_2 = 0;
		int hw_ocv_36_3 = 0;

		zcv_36_rdy = mt6336_get_flag_register_value(MT6336_AUXADC_ADC_RDY_WAKEUP1);
		zcv_chrgo_1_rdy = mt6336_get_flag_register_value(MT6336_AUXADC_ADC_RDY_CHRGO1);
		zcv_fgadc1_rdy = mt6336_get_flag_register_value(MT6336_AUXADC_ADC_RDY_FGADC1);

		if (1) {
			zcv_36_low = mt6336_get_flag_register_value(MT6336_AUXADC_ADC_OUT_WAKEUP1_L);
			zcv_36_high = mt6336_get_flag_register_value(MT6336_AUXADC_ADC_OUT_WAKEUP1_H);
			hw_ocv_36_reg1 = (zcv_36_high << 8) + zcv_36_low;
			hw_ocv_36_1 = REG_to_MV_value(hw_ocv_36_reg1);
			bm_err("[read_hw_ocv_6336_charger_in] zcv_36_rdy %d hw_ocv_36_1 %d [0x%x:0x%x]\n",
				zcv_36_rdy, hw_ocv_36_1, zcv_36_low, zcv_36_high);

			mt6336_set_flag_register_value(MT6336_AUXADC_ADC_RDY_WAKEUP_CLR, 1);
			mdelay(1);
			mt6336_set_flag_register_value(MT6336_AUXADC_ADC_RDY_WAKEUP_CLR, 0);
		}

		if (1) {
			zcv_chrgo_1_lo = mt6336_get_flag_register_value(MT6336_AUXADC_ADC_OUT_CHRGO1_L);
			zcv_chrgo_1_hi = mt6336_get_flag_register_value(MT6336_AUXADC_ADC_OUT_CHRGO1_H);
			hw_ocv_36_reg2 = (zcv_chrgo_1_hi << 8) + zcv_chrgo_1_lo;
			hw_ocv_36_2 = REG_to_MV_value(hw_ocv_36_reg2);
			bm_err("[read_hw_ocv_6336_charger_in] zcv_chrgo_1_rdy %d hw_ocv_36_2 %d [0x%x:0x%x]\n",
				zcv_chrgo_1_rdy, hw_ocv_36_2, zcv_chrgo_1_lo, zcv_chrgo_1_hi);
			mt6336_set_flag_register_value(MT6336_AUXADC_ADC_RDY_CHRGO_CLR, 1);
			mdelay(1);
			mt6336_set_flag_register_value(MT6336_AUXADC_ADC_RDY_CHRGO_CLR, 0);
		}

		if (1) {
			zcv_fgadc1_lo = mt6336_get_flag_register_value(MT6336_AUXADC_ADC_OUT_FGADC1_L);
			zcv_fgadc1_hi = mt6336_get_flag_register_value(MT6336_AUXADC_ADC_OUT_FGADC1_H);
			hw_ocv_36_reg3 = (zcv_fgadc1_hi << 8) + zcv_fgadc1_lo;
			hw_ocv_36_3 = REG_to_MV_value(hw_ocv_36_reg3);
			bm_err("[read_hw_ocv_6336_charger_in] FGADC1 %d hw_ocv_36_3 %d [0x%x:0x%x]\n",
				zcv_fgadc1_rdy, hw_ocv_36_3, zcv_fgadc1_lo, zcv_fgadc1_hi);
			mt6336_set_flag_register_value(MT6336_AUXADC_ADC_RDY_FGADC_CLR, 1);
			mdelay(1);
			mt6336_set_flag_register_value(MT6336_AUXADC_ADC_RDY_FGADC_CLR, 0);
		}

		return hw_ocv_36_2;
#endif
}

#else
int read_hw_ocv_6336_charger_in(void)
{
#if defined(CONFIG_POWER_EXT)
	return 37000;
#else
	int hw_ocv_35 = read_hw_ocv_6335_power_on();

	return hw_ocv_35;
#endif
}
#endif

int read_hw_ocv_6335_power_on_rdy(void)
{
#if defined(CONFIG_POWER_EXT)
	return 1;
	bm_debug("[read_hw_ocv_6335_power_on_rdy] POWER_EXT\n");
#else
	signed int pon_rdy = 0;
	int hw_id = upmu_get_reg_value(0x0200);

	if (hw_id == 0x3510)
		pon_rdy = pmic_get_register_value(PMIC_AUXADC_ADC_RDY_WAKEUP_PCHR);
	else
		pon_rdy = pmic_get_register_value(PMIC_AUXADC_ADC_RDY_PWRON_PCHR);

	bm_err("[read_hw_ocv_6335_power_on_rdy] 0x%x pon_rdy %d\n", hw_id, pon_rdy);

	return pon_rdy;
#endif
}

static signed int read_hw_ocv(void *data)
{
#if defined(CONFIG_POWER_EXT)
	*(signed int *) (data) = 37000;
#else
	int _hw_ocv, _sw_ocv;
	int _hw_ocv_src;
	int _prev_hw_ocv, _prev_hw_ocv_src;
	int _hw_ocv_rdy;
	int _flag_unreliable;
	int ret;
	int _hw_ocv_35_pon;
	int _hw_ocv_35_plugin;
	int _hw_ocv_36_chgin;
	int _hw_ocv_35_pon_rdy;

	_hw_ocv_35_pon_rdy = read_hw_ocv_6335_power_on_rdy();
	_hw_ocv_35_pon = read_hw_ocv_6335_power_on();
	_hw_ocv_35_plugin = read_hw_ocv_6335_plug_in();
	_hw_ocv_36_chgin = read_hw_ocv_6336_charger_in();

	ret = fg_is_charger_exist(&g_fg_is_charger_exist);
	_hw_ocv = _hw_ocv_35_pon;
	_sw_ocv = get_sw_ocv();
	_hw_ocv_src = FROM_6335_PON_ON;
	_prev_hw_ocv = _hw_ocv;
	_prev_hw_ocv_src = FROM_6335_PON_ON;
	_flag_unreliable = 0;

	if (g_fg_is_charger_exist) {
		_hw_ocv_rdy = _hw_ocv_35_pon_rdy;
		if (_hw_ocv_rdy == 1) {
			if (MTK_CHR_EXIST == 1) {
				_hw_ocv = _hw_ocv_36_chgin;
				_hw_ocv_src = FROM_6336_CHR_IN;
			} else {
				_hw_ocv = _hw_ocv_35_pon;
				_hw_ocv_src = FROM_6335_PON_ON;
			}

			if (abs(_hw_ocv - _sw_ocv) > EXT_HWOCV_SWOCV) {
				_prev_hw_ocv = _hw_ocv;
				_prev_hw_ocv_src = _hw_ocv_src;
				_hw_ocv = _sw_ocv;
				_hw_ocv_src = FROM_SW_OCV;
				set_hw_ocv_unreliable(true);
				_flag_unreliable = 1;
			}
		} else {
			/* fixme: swocv is workaround */
			/*_hw_ocv = _hw_ocv_35_plugin;*/
			/*_hw_ocv_src = FROM_6335_PLUG_IN;*/
			_hw_ocv = _sw_ocv;
			_hw_ocv_src = FROM_SW_OCV;
			if (MTK_CHR_EXIST != 1) {
				if (abs(_hw_ocv - _sw_ocv) > EXT_HWOCV_SWOCV) {
					_prev_hw_ocv = _hw_ocv;
					_prev_hw_ocv_src = _hw_ocv_src;
					_hw_ocv = _sw_ocv;
					_hw_ocv_src = FROM_SW_OCV;
					set_hw_ocv_unreliable(true);
					_flag_unreliable = 1;
				}
			}
		}
	} else {
		if (_hw_ocv_35_pon_rdy == 0) {
			_hw_ocv = _sw_ocv;
			_hw_ocv_src = FROM_SW_OCV;
		}
	}

	*(signed int *) (data) = _hw_ocv;

	bm_debug("[read_hw_ocv] g_fg_is_charger_exist %d MTK_CHR_EXIST %d\n",
		g_fg_is_charger_exist, MTK_CHR_EXIST);
	bm_debug("[read_hw_ocv] _hw_ocv %d _sw_ocv %d EXT_HWOCV_SWOCV %d\n",
		_prev_hw_ocv, _sw_ocv, EXT_HWOCV_SWOCV);
	bm_debug("[read_hw_ocv] _hw_ocv %d _hw_ocv_src %d _prev_hw_ocv %d _prev_hw_ocv_src %d _flag_unreliable %d\n",
		_hw_ocv, _hw_ocv_src, _prev_hw_ocv, _prev_hw_ocv_src, _flag_unreliable);
	bm_debug("[read_hw_ocv] _hw_ocv_35_pon_rdy %d _hw_ocv_35_pon %d _hw_ocv_35_plugin %d _hw_ocv_36_chgin %d _sw_ocv %d\n",
		_hw_ocv_35_pon_rdy, _hw_ocv_35_pon, _hw_ocv_35_plugin, _hw_ocv_36_chgin, _sw_ocv);

#endif
	return STATUS_OK;
}

static signed int dump_register_fgadc(void *data)
{
	return STATUS_OK;
}

static signed int fgauge_do_ptim(void *data)
{
	int is_ptim_lock;
	int ret;

	is_ptim_lock = *(int *)data;
	ret = do_ptim_ex(is_ptim_lock, &ptim_vol, &ptim_curr);

	if ((g_fg_is_charging == 0) && (ptim_curr >= 0))
		ptim_curr = 0 - ptim_curr;

	return ret;
}

static signed int read_ptim_bat_vol(void *data)
{
	if (Bat_EC_ctrl.debug_ptim_v_en == 1)
		*(signed int *) (data) = Bat_EC_ctrl.debug_ptim_v_value;
	else
		*(signed int *) (data) = ptim_vol;

	return STATUS_OK;
}

static signed int read_ptim_R_curr(void *data)
{
	if (Bat_EC_ctrl.debug_ptim_r_en == 1)
		*(signed int *) (data) = Bat_EC_ctrl.debug_ptim_r_value;
	else
		*(signed int *) (data) = ptim_curr;

	return STATUS_OK;
}

static signed int read_ptim_rac_val(void *data)
{
	if (Bat_EC_ctrl.debug_rac_en == 1)
		*(signed int *) (data) = Bat_EC_ctrl.debug_rac_value;
	else
		*(signed int *) (data) = get_rac();

	return STATUS_OK;
}

static signed int fg_set_is_fg_initialized(void *data)
{
	int fg_reset_status = *(signed int *) (data);

	pmic_set_register_value(PMIC_FG_RSTB_STATUS, fg_reset_status);

	return STATUS_OK;
}

static signed int fg_get_is_fg_initialized(void *data)
{
	int fg_reset_status = pmic_get_register_value(PMIC_FG_RSTB_STATUS);

	*(signed int *) (data) = fg_reset_status;

	return STATUS_OK;
}

static signed int fg_get_rtc_ui_soc(void *data)
{
	int spare3_reg = get_rtc_spare_fg_value();
	int rtc_ui_soc;

	rtc_ui_soc = (spare3_reg & 0x7f);

	*(signed int *) (data) = rtc_ui_soc;
	bm_notice("[fg_get_rtc_ui_soc] rtc_ui_soc %d spare3_reg 0x%x\n", rtc_ui_soc, spare3_reg);

	return STATUS_OK;
}

static signed int fg_set_rtc_ui_soc(void *data)
{
	int spare3_reg = get_rtc_spare_fg_value();
	int rtc_ui_soc = *(signed int *) (data);
	int spare3_reg_valid;
	int new_spare3_reg;

	spare3_reg_valid = (spare3_reg & 0x80);
	new_spare3_reg = spare3_reg_valid + rtc_ui_soc;

	/* set spare3 0x7f */
	set_rtc_spare_fg_value(new_spare3_reg);

	bm_notice("[fg_set_rtc_ui_soc] rtc_ui_soc %d spare3_reg 0x%x new_spare3_reg 0x%x\n",
		rtc_ui_soc, spare3_reg, new_spare3_reg);

	return STATUS_OK;
}

static void fgauge_read_RTC_boot_status(void)
{
	int hw_id = upmu_get_reg_value(0x0200);
	int spare0_reg, spare0_reg_b13;
	int spare3_reg;
	int spare3_reg_valid;

	spare0_reg = hal_rtc_get_spare_register(RTC_FG_INIT);
	spare3_reg = get_rtc_spare_fg_value();
	spare3_reg_valid = (spare3_reg & 0x80) >> 7;

	if (spare3_reg_valid == 0)
		rtc_invalid = 1;
	else
		rtc_invalid = 0;

	if (rtc_invalid == 0) {
		spare0_reg_b13 = (spare0_reg & 0x20) >> 5;
		if ((hw_id & 0xff00) == 0x3500)
			is_bat_plugout = spare0_reg_b13;
		else
			is_bat_plugout = ~spare0_reg_b13;

		bat_plug_out_time = spare0_reg & 0x1f;	/*[12:8], 5 bits*/
	} else {
		is_bat_plugout = 1;
		bat_plug_out_time = 31;	/*[12:8], 5 bits*/
	}

	bm_err("[fgauge_read_RTC_boot_status] rtc_invalid %d is_bat_plugout %d bat_plug_out_time %d spare3 0x%x spare0 0x%x\n",
			rtc_invalid, is_bat_plugout, bat_plug_out_time,
			spare3_reg, spare0_reg);
}

static signed int fg_set_fg_reset_rtc_status(void *data)
{
	int hw_id = upmu_get_reg_value(0x0200);
	int temp_value;
	int spare0_reg, after_rst_spare0_reg;
	int spare3_reg, after_rst_spare3_reg;

	fgauge_read_RTC_boot_status();

	/* read spare0 */
	spare0_reg = hal_rtc_get_spare_register(RTC_FG_INIT);

	/* raise 15b to reset */
#if 0
	temp_value = spare0_reg | (1<<7);
	hal_rtc_set_spare_register(RTC_FG_INIT, temp_value);
	mdelay(1);
	temp_value &= 0x7f;
	hal_rtc_set_spare_register(RTC_FG_INIT, temp_value);
#else
	if ((hw_id & 0xff00) == 0x3500) {
		temp_value = 0x80;
		hal_rtc_set_spare_register(RTC_FG_INIT, temp_value);
		mdelay(1);
		temp_value = 0x00;
		hal_rtc_set_spare_register(RTC_FG_INIT, temp_value);
	} else {
		temp_value = 0x80;
		hal_rtc_set_spare_register(RTC_FG_INIT, temp_value);
		mdelay(1);
		temp_value = 0x20;
		hal_rtc_set_spare_register(RTC_FG_INIT, temp_value);
	}
#endif
	/* read spare0 again */
	after_rst_spare0_reg = hal_rtc_get_spare_register(RTC_FG_INIT);


	/* read spare3 */
	spare3_reg = get_rtc_spare_fg_value();

	/* set spare3 0x7f */
	set_rtc_spare_fg_value(spare3_reg | 0x80);

	/* read spare3 again */
	after_rst_spare3_reg = get_rtc_spare_fg_value();

	bm_err("[fgauge_read_RTC_boot_status] spare0 0x%x 0x%x, spare3 0x%x 0x%x\n",
		spare0_reg, after_rst_spare0_reg, spare3_reg, after_rst_spare3_reg);

	return STATUS_OK;

}

static signed int read_boot_battery_plug_out_status(void *data)
{
	*(unsigned int *) (data) = is_bat_plugout;
	bm_err("[read_boot_battery_plug_out_status] rtc_invalid %d is_bat_plugout %d bat_plug_out_time %d\n",
			rtc_invalid, is_bat_plugout, bat_plug_out_time);

	return STATUS_OK;
}

static signed int read_bat_plug_out_time(void *data) /* weiching fix */
{
	*(unsigned int *) (data) = bat_plug_out_time;

	bm_err("[read_bat_plug_out_time] rtc_invalid %d is_bat_plugout %d bat_plug_out_time %d\n",
			rtc_invalid, is_bat_plugout, bat_plug_out_time);

	return STATUS_OK;
}

static signed int fgauge_get_zcv_curr(void *data)
{
#if defined(CONFIG_POWER_EXT)
	return 4001;
	bm_debug("[fgauge_get_zcv_curr]\n");
#else
	unsigned short uvalue16 = 0;
	signed int dvalue = 0;
	long long Temp_Value = 0;

	uvalue16 = pmic_get_register_value(PMIC_FG_ZCV_CURR);
	dvalue = (unsigned int) uvalue16;
		if (dvalue == 0) {
			Temp_Value = (long long) dvalue;
		} else if (dvalue > 32767) {
			/* > 0x8000 */
			Temp_Value = (long long) (dvalue - 65535);
			Temp_Value = Temp_Value - (Temp_Value * 2);
		} else {
			Temp_Value = (long long) dvalue;
		}

	Temp_Value = Temp_Value * UNIT_FGCURRENT;
	do_div(Temp_Value, 100000);
	dvalue = (unsigned int) Temp_Value;

	/* Auto adjust value */
	if (fg_cust_data.r_fg_value != 100) {
		bm_trace(
		"[fgauge_read_current] Auto adjust value due to the Rfg is %d\n Ori current=%d, ",
		fg_cust_data.r_fg_value, dvalue);

		dvalue = (dvalue * 100) / fg_cust_data.r_fg_value;

		bm_trace("[fgauge_read_current] new current=%d\n", dvalue);
	}

	bm_trace("[fgauge_read_current] ori current=%d\n", dvalue);

	dvalue = ((dvalue * fg_cust_data.car_tune_value) / 1000);

	bm_debug("[fgauge_read_current] final current=%d (ratio=%d)\n",
		 dvalue, fg_cust_data.car_tune_value);

	*(signed int *) (data) = dvalue;

	return STATUS_OK;
#endif
}

static signed int fgauge_get_zcv(void *data)
{
#if defined(CONFIG_POWER_EXT)
		return 4001;
		bm_debug("[fgauge_get_zcv]\n");
#else
		signed int adc_result_reg = 0;
		signed int adc_result = 0;

#if defined(SWCHR_POWER_PATH)
		adc_result_reg = pmic_get_register_value(PMIC_AUXADC_ADC_OUT_FGADC1);
		adc_result = REG_to_MV_value(adc_result_reg);
		bm_debug("[oam] fgauge_get_zcv BATSNS (swchr) : adc_result_reg=%d, adc_result=%d\n",
			 adc_result_reg, adc_result);
#else
		adc_result_reg = pmic_get_register_value(PMIC_AUXADC_ADC_OUT_FGADC1);
		adc_result = REG_to_MV_value(adc_result_reg);
		bm_debug("[oam] fgauge_get_zcv BATSNS  (pchr) : adc_result_reg=%d, adc_result=%d\n",
			 adc_result_reg, adc_result);
#endif
		adc_result += g_hw_ocv_tune_value;
		*(signed int *) (data) = adc_result;
		return STATUS_OK;
#endif
}

static signed int fg_set_vbat2_l_en(void *data)
{
	int en = *(signed int *) (data);

	pmic_set_register_value(PMIC_RG_INT_EN_BAT2_L, en);
	pmic_set_register_value(PMIC_AUXADC_LBAT2_IRQ_EN_MIN, en);
	pmic_set_register_value(PMIC_AUXADC_LBAT2_EN_MIN, en);

	return STATUS_OK;
}

static signed int fg_set_vbat2_h_en(void *data)
{
	int en = *(signed int *) (data);

	pmic_set_register_value(PMIC_RG_INT_EN_BAT2_H, en);
	pmic_set_register_value(PMIC_AUXADC_LBAT2_IRQ_EN_MAX, en);
	pmic_set_register_value(PMIC_AUXADC_LBAT2_EN_MAX, en);

	return STATUS_OK;

}

static signed int fg_set_vbat2_l_th(void *data)
{
	int vbat2_l_th_mv =  *(signed int *) (data);
	int vbat2_l_th_reg = MV_to_REG_12_value(vbat2_l_th_mv);
	int vbat2_det_time_15_0 = ((1000 * fg_cust_data.vbat2_det_time) & 0xffff);
	int vbat2_det_time_19_16 = ((1000 * fg_cust_data.vbat2_det_time) & 0xffff0000) >> 16;
	int vbat2_det_counter = fg_cust_data.vbat2_det_counter;

	pmic_set_register_value(PMIC_AUXADC_LBAT2_VOLT_MIN, vbat2_l_th_reg);
	pmic_set_register_value(PMIC_AUXADC_LBAT2_DET_PRD_15_0, vbat2_det_time_15_0);
	pmic_set_register_value(PMIC_AUXADC_LBAT2_DET_PRD_19_16, vbat2_det_time_19_16);
	pmic_set_register_value(PMIC_AUXADC_LBAT2_DEBT_MIN, vbat2_det_counter);

	bm_debug("[fg_set_vbat2_l_th] set [0x%x 0x%x 0x%x 0x%x] get [0x%x 0x%x 0x%x 0x%x]\n",
		vbat2_l_th_reg, vbat2_det_time_15_0, vbat2_det_time_19_16, vbat2_det_counter,
		pmic_get_register_value(PMIC_AUXADC_LBAT2_VOLT_MIN),
		pmic_get_register_value(PMIC_AUXADC_LBAT2_DET_PRD_15_0),
		pmic_get_register_value(PMIC_AUXADC_LBAT2_DET_PRD_19_16),
		pmic_get_register_value(PMIC_AUXADC_LBAT2_DEBT_MIN));

	return STATUS_OK;
}

static signed int fg_set_vbat2_h_th(void *data)
{
	int vbat2_h_th_mv =  *(signed int *) (data);
	int vbat2_h_th_reg = MV_to_REG_12_value(vbat2_h_th_mv);
	int vbat2_det_time_15_0 = ((1000 * fg_cust_data.vbat2_det_time) & 0xffff);
	int vbat2_det_time_19_16 = ((1000 * fg_cust_data.vbat2_det_time) & 0xffff0000) >> 16;
	int vbat2_det_counter = fg_cust_data.vbat2_det_counter;

	pmic_set_register_value(PMIC_AUXADC_LBAT2_VOLT_MAX, vbat2_h_th_reg);
	pmic_set_register_value(PMIC_AUXADC_LBAT2_DET_PRD_15_0, vbat2_det_time_15_0);
	pmic_set_register_value(PMIC_AUXADC_LBAT2_DET_PRD_19_16, vbat2_det_time_19_16);
	pmic_set_register_value(PMIC_AUXADC_LBAT2_DEBT_MAX, vbat2_det_counter);

	bm_debug("[fg_set_vbat2_h_th] set [0x%x 0x%x 0x%x 0x%x] get [0x%x 0x%x 0x%x 0x%x]\n",
		vbat2_h_th_reg, vbat2_det_time_15_0, vbat2_det_time_19_16, vbat2_det_counter,
		pmic_get_register_value(PMIC_AUXADC_LBAT2_VOLT_MAX),
		pmic_get_register_value(PMIC_AUXADC_LBAT2_DET_PRD_15_0),
		pmic_get_register_value(PMIC_AUXADC_LBAT2_DET_PRD_19_16),
		pmic_get_register_value(PMIC_AUXADC_LBAT2_DEBT_MAX));

	return STATUS_OK;
}

static signed int fg_get_hw_ver(void *data)
{
	int hw_id = upmu_get_reg_value(0x0200);

	*(signed int *) (data) = hw_id;
	return STATUS_OK;
}

static signed int fgauge_set_meta_cali_current(void *data)
{
	g_meta_input_cali_current = *(signed int *)(data);

	return STATUS_OK;
}

#if defined(CONFIG_POWER_EXT)
 /**/
#else
static signed int fgauge_get_AUXADC_current_rawdata(unsigned short *uvalue16)
{
	int m;
	int ret;
	/* (1)    Set READ command */
	ret = pmic_config_interface(MT6335_FGADC_CON1, 0x0001, 0x000F, 0x0);

	/*(2)     Keep i2c read when status = 1 (0x06) */
	m = 0;
	while (fg_get_data_ready_status() == 0) {
		m++;
		if (m > 1000) {
			bm_err(
			"[fgauge_get_AUXADC_current_rawdata] fg_get_data_ready_status timeout 1 !\r\n");
			break;
		}
	}

	/* (3)    Read FG_CURRENT_OUT[15:08] */
	/* (4)    Read FG_CURRENT_OUT[07:00] */
	*uvalue16 = pmic_get_register_value(PMIC_FG_CURRENT_OUT);

	/* (5)    (Read other data) */
	/* (6)    Clear status to 0 */
	ret = pmic_config_interface(MT6335_FGADC_CON1, 0x0008, 0x000F, 0x0);

	/* (7)    Keep i2c read when status = 0 (0x08) */
	m = 0;
	while (fg_get_data_ready_status() != 0) {
		m++;
		if (m > 1000) {
			bm_err(
			 "[fgauge_get_AUXADC_current_rawdata] fg_get_data_ready_status timeout 2 !\r\n");
			break;
		}
	}

	/*(8)    Recover original settings */
	ret = pmic_config_interface(MT6335_FGADC_CON1, 0x0000, 0x000F, 0x0);

	return ret;
}
#endif

static signed int fgauge_meta_cali_car_tune_value(void *data)
{
#if defined(CONFIG_POWER_EXT)
	return STATUS_OK;
#else
	int cali_car_tune;
	long long sum_all = 0;
	long long temp_sum = 0;
	int	avg_cnt = 0;
	int i;
	unsigned short uvalue16;
	unsigned int uvalue32;
	signed int dvalue = 0;
	long long Temp_Value1 = 0;
	long long Temp_Value2 = 0;
	long long current_from_ADC = 0;

	if (g_meta_input_cali_current != 0) {
		for (i = 0; i < CALI_CAR_TUNE_AVG_NUM; i++) {
			if (!fgauge_get_AUXADC_current_rawdata(&uvalue16)) {
				uvalue32 = (unsigned int) uvalue16;
				if (uvalue32 <= 0x8000) {
					Temp_Value1 = (long long)uvalue32;
					bm_err("[111]uvalue16 %d uvalue32 %d Temp_Value1 %lld\n",
						uvalue16, uvalue32, Temp_Value1);
				} else if (uvalue32 > 0x8000) {
					/*Temp_Value1 = (long long) (uvalue32 - 65535);*/
					/*Temp_Value1 = 0 - Temp_Value1;*/
					Temp_Value1 = (long long) (65535 - uvalue32);
					bm_err("[222]uvalue16 %d uvalue32 %d Temp_Value1 %lld\n",
						uvalue16, uvalue32, Temp_Value1);
				}
				sum_all += Temp_Value1;
				avg_cnt++;
				/*****************/
				bm_err("[333]uvalue16 %d uvalue32 %d Temp_Value1 %lld sum_all %lld\n",
						uvalue16, uvalue32, Temp_Value1, sum_all);
				/*****************/
			}
			mdelay(30);
		}
		/*calculate the real world data    */
		/*current_from_ADC = sum_all / avg_cnt;*/
		temp_sum = sum_all;
		bm_err("[444]sum_all %lld temp_sum %lld avg_cnt %d current_from_ADC %lld\n",
			sum_all, temp_sum, avg_cnt, current_from_ADC);

		do_div(temp_sum, avg_cnt);
		current_from_ADC = temp_sum;

		bm_err("[555]sum_all %lld temp_sum %lld avg_cnt %d current_from_ADC %lld\n",
			sum_all, temp_sum, avg_cnt, current_from_ADC);

		Temp_Value2 = current_from_ADC * UNIT_FGCURRENT;

		bm_err("[555]Temp_Value2 %lld current_from_ADC %lld UNIT_FGCURRENT %d\n",
			Temp_Value2, current_from_ADC, UNIT_FGCURRENT);

		do_div(Temp_Value2, 1000000);

		bm_err("[666]Temp_Value2 %lld current_from_ADC %lld UNIT_FGCURRENT %d\n",
			Temp_Value2, current_from_ADC, UNIT_FGCURRENT);

		dvalue = (unsigned int) Temp_Value2;

		/* Auto adjust value */
		if (fg_cust_data.r_fg_value != 100)
			dvalue = (dvalue * 100) / fg_cust_data.r_fg_value;

		bm_err("[666]dvalue %d fg_cust_data.r_fg_value %d\n", dvalue, fg_cust_data.r_fg_value);

		cali_car_tune = g_meta_input_cali_current * 1000 / dvalue;	/* 1000 base, so multiple by 1000*/

		bm_err("[777]dvalue %d fg_cust_data.r_fg_value %d cali_car_tune %d\n",
			dvalue, fg_cust_data.r_fg_value, cali_car_tune);
		*(signed int *) (data) = cali_car_tune;

		bm_err(
			"[fgauge_meta_cali_car_tune_value][%d] meta:%d, adc:%lld, UNI_FGCUR:%d, r_fg_value:%d\n",
			cali_car_tune, g_meta_input_cali_current, current_from_ADC,
			UNIT_FGCURRENT, fg_cust_data.r_fg_value);

		return STATUS_OK;
	}

	return STATUS_UNSUPPORTED;
#endif
}


static signed int(*bm_func[BATTERY_METER_CMD_NUMBER]) (void *data);

signed int bm_ctrl_cmd(BATTERY_METER_CTRL_CMD cmd, void *data)
{
	signed int status = STATUS_UNSUPPORTED;
	static signed int init = -1;

	if (init == -1) {
		init = 0;
		mutex_init(&fg_wlock);
		bm_func[BATTERY_METER_CMD_HW_FG_INIT] = fgauge_initialization;
		bm_func[BATTERY_METER_CMD_GET_HW_FG_CURRENT] = fgauge_read_current;
		bm_func[BATTERY_METER_CMD_GET_HW_FG_CURRENT_SIGN] = fgauge_read_current_sign;
		bm_func[BATTERY_METER_CMD_GET_FG_CURRENT_IAVG] = fg_get_current_iavg;
		bm_func[BATTERY_METER_CMD_GET_FG_HW_CAR] = fgauge_read_columb_accurate;
		bm_func[BATTERY_METER_CMD_HW_RESET] = fgauge_hw_reset;
		bm_func[BATTERY_METER_CMD_GET_ADC_V_BAT_SENSE] = read_adc_v_bat_sense;
#if 0
		bm_func[BATTERY_METER_CMD_GET_ADC_V_I_SENSE] = read_adc_v_i_sense;
#endif
		bm_func[BATTERY_METER_CMD_GET_ADC_V_BAT_TEMP] = read_adc_v_bat_temp;
		bm_func[BATTERY_METER_CMD_GET_ADC_V_CHARGER] = read_adc_v_charger;
		bm_func[BATTERY_METER_CMD_GET_HW_OCV] = read_hw_ocv;
		bm_func[BATTERY_METER_CMD_DUMP_REGISTER] = dump_register_fgadc;
		bm_func[BATTERY_METER_CMD_SET_COLUMB_INTERRUPT1] = fgauge_set_columb_interrupt1;
		bm_func[BATTERY_METER_CMD_SET_COLUMB_INTERRUPT2_HT] = fgauge_set_columb_interrupt2_ht;
		bm_func[BATTERY_METER_CMD_SET_COLUMB_INTERRUPT2_LT] = fgauge_set_columb_interrupt2_lt;
		bm_func[BATTERY_METER_CMD_SET_COLUMB_INTERRUPT2_HT_EN] = fgauge_set_columb_interrupt2_ht_en;
		bm_func[BATTERY_METER_CMD_SET_COLUMB_INTERRUPT2_LT_EN] = fgauge_set_columb_interrupt2_lt_en;
		bm_func[BATTERY_METER_CMD_GET_BOOT_BATTERY_PLUG_STATUS] = read_boot_battery_plug_out_status;
		bm_func[BATTERY_METER_CMD_DO_PTIM] = fgauge_do_ptim;
		bm_func[BATTERY_METER_CMD_GET_PTIM_BAT_VOL] = read_ptim_bat_vol;
		bm_func[BATTERY_METER_CMD_GET_PTIM_R_CURR] = read_ptim_R_curr;
		bm_func[BATTERY_METER_CMD_GET_PTIM_RAC_VAL] = read_ptim_rac_val;
		bm_func[BATTERY_METER_CMD_GET_IS_FG_INITIALIZED] = fg_get_is_fg_initialized;
		bm_func[BATTERY_METER_CMD_SET_IS_FG_INITIALIZED] = fg_set_is_fg_initialized;
		bm_func[BATTERY_METER_CMD_GET_SHUTDOWN_DURATION_TIME] = fgauge_get_soff_time;
		bm_func[BATTERY_METER_CMD_GET_BAT_PLUG_OUT_TIME] = read_bat_plug_out_time;
		bm_func[BATTERY_METER_CMD_GET_ZCV] = fgauge_get_zcv;
		bm_func[BATTERY_METER_CMD_GET_ZCV_CURR] = fgauge_get_zcv_curr;
		bm_func[BATTERY_METER_CMD_SET_CYCLE_INTERRUPT] = fg_set_cycle_interrupt;
		bm_func[BATTERY_METER_CMD_SOFF_RESET] = fg_reset_soff_time;
		bm_func[BATTERY_METER_CMD_NCAR_RESET] = fg_reset_ncar;
		bm_func[BATTERY_METER_CMD_SET_NAG_ZCV] = fg_set_nag_zcv;
		bm_func[BATTERY_METER_CMD_SET_NAG_C_DLTV] = fg_set_nag_c_dltv;
		bm_func[BATTERY_METER_CMD_SET_ZCV_INTR] = fg_set_zcv_intr;
		bm_func[BATTERY_METER_CMD_SET_ZCV_INTR_EN] = fg_set_zcv_intr_en;
		bm_func[BATTERY_METER_CMD_SET_SW_CAR_NAFG_EN] = fg_set_nafg_en;
		bm_func[BATTERY_METER_CMD_GET_SW_CAR_NAFG_CNT] = fg_get_nafg_cnt;
		bm_func[BATTERY_METER_CMD_GET_SW_CAR_NAFG_DLTV] = fg_get_nafg_dltv;
		bm_func[BATTERY_METER_CMD_GET_SW_CAR_NAFG_C_DLTV] = fg_get_nafg_c_dltv;
		bm_func[BATTERY_METER_CMD_SET_FG_QUSE] = fg_set_fg_quse;
		bm_func[BATTERY_METER_CMD_SET_FG_RESISTENCE] = fg_set_fg_resistence_bat;
		bm_func[BATTERY_METER_CMD_SET_FG_DC_RATIO] = fg_set_fg_dc_ratio;
		bm_func[BATTERY_METER_CMD_IS_CHARGER_EXIST] = fg_is_charger_exist;
		bm_func[BATTERY_METER_CMD_SET_FG_BAT_TMP_INT_LT] = fg_set_fg_bat_tmp_int_lt;
		bm_func[BATTERY_METER_CMD_SET_FG_BAT_TMP_INT_HT] = fg_set_fg_bat_tmp_int_ht;
		bm_func[BATTERY_METER_CMD_SET_FG_BAT_TMP_EN] = fg_set_fg_bat_tmp_en;
		bm_func[BATTERY_METER_CMD_GET_TIME] = fgauge_get_time;
		bm_func[BATTERY_METER_CMD_SET_TIME_INTERRUPT] = fgauge_set_time_interrupt;
		bm_func[BATTERY_METER_CMD_GET_NAFG_VBAT] = read_nafg_vbat;
		bm_func[BATTERY_METER_CMD_GET_FG_HW_INFO] = read_fg_hw_info;
		bm_func[BATTERY_METER_CMD_GET_IS_BAT_EXIST] = fg_is_battery_exist;
		bm_func[BATTERY_METER_CMD_GET_IS_BAT_CHARGING] = fg_is_bat_charging;
		bm_func[BATTERY_METER_CMD_GET_FG_CURRENT_IAVG] = fg_get_current_iavg;
		bm_func[BATTERY_METER_CMD_SET_BAT_PLUGOUT_INTR_EN] = fg_set_bat_plugout_intr_en;
		bm_func[BATTERY_METER_CMD_SET_FG_RESET_RTC_STATUS] = fg_set_fg_reset_rtc_status;
		bm_func[BATTERY_METER_CMD_SET_SET_IAVG_INTR] = fg_set_iavg_intr;
		bm_func[BATTERY_METER_CMD_ENABLE_FG_VBAT_L_INT] = fg_set_vbat2_l_en;
		bm_func[BATTERY_METER_CMD_ENABLE_FG_VBAT_H_INT] = fg_set_vbat2_h_en;
		bm_func[BATTERY_METER_CMD_SET_FG_VBAT_L_TH] = fg_set_vbat2_l_th;
		bm_func[BATTERY_METER_CMD_SET_FG_VBAT_H_TH] = fg_set_vbat2_h_th;
		bm_func[BATTERY_METER_CMD_HW_VERSION] = fg_get_hw_ver;
		bm_func[BATTERY_METER_CMD_SET_META_CALI_CURRENT] = fgauge_set_meta_cali_current;
		bm_func[BATTERY_METER_CMD_META_CALI_CAR_TUNE_VALUE] = fgauge_meta_cali_car_tune_value;
		bm_func[BATTERY_METER_CMD_GET_FG_CURRENT_IAVG_VALID] = fg_get_current_iavg_valid;
		bm_func[BATTERY_METER_CMD_GET_RTC_UI_SOC] = fg_get_rtc_ui_soc;
		bm_func[BATTERY_METER_CMD_SET_RTC_UI_SOC] = fg_set_rtc_ui_soc;
	}

	if (cmd < BATTERY_METER_CMD_NUMBER) {
		if (bm_func[cmd] != NULL) {
			mutex_lock(&fg_wlock);
			status = bm_func[cmd] (data);
			mutex_unlock(&fg_wlock);
		} else
			status = STATUS_UNSUPPORTED;
	} else
		status = STATUS_UNSUPPORTED;

	return status;
}

