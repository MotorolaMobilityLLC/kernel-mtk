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

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/dmi.h>
#include <linux/acpi.h>
#include <linux/thermal.h>
#include <linux/platform_device.h>
#include <mt-plat/aee.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include "mt-plat/mtk_thermal_monitor.h"
#include "mtk_thermal_typedefs.h"
#include "mach/mt_thermal.h"
#include <mt-plat/upmu_common.h>
#include <tspmic_settings.h>
/*=============================================================
 *Local variable definition
 *=============================================================*/
int mtktspmic_debug_log = 0;
/* Cali */
static __s32 g_o_vts;
static __s32 g_degc_cali;
static __s32 g_adc_cali_en;
static __s32 g_o_slope;
static __s32 g_o_slope_sign;
static __s32 g_id;
static __s32 g_slope1 = 1;
static __s32 g_slope2 = 1;
static __s32 g_intercept;

static DEFINE_MUTEX(TSPMIC_lock);
static int pre_temp1 = 0, PMIC_counter;
/*=============================================================*/

static __s32 pmic_raw_to_temp(__u32 ret)
{
	__s32 y_curr = ret;
	__s32 t_current;
	t_current = g_intercept + ((g_slope1 * y_curr) / (g_slope2));
	mtktspmic_dprintk("[pmic_raw_to_temp] t_current=%d\n", t_current);
	return t_current;
}

static void mtktspmic_read_efuse(void)
{
	__u32 efusevalue[3];

	mtktspmic_info("[pmic_debug]  start\n");
	/*
	   0x0  512     527
	   0x1  528     543
	   Thermal data from 512 to 539
	 */
	efusevalue[0] = pmic_Read_Efuse_HPOffset(0x0);
	efusevalue[1] = pmic_Read_Efuse_HPOffset(0x1);
	mtktspmic_info("[pmic_debug] 6351_efuse:\n"
		       "efusevalue[0]=0x%x\n"
		       "efusevalue[1]=0x%x\n\n", efusevalue[0], efusevalue[1]);

	g_adc_cali_en = (efusevalue[0] & _BIT_(0));
	g_degc_cali = ((efusevalue[0] & _BITMASK_(6:1)) >> 1);
	g_o_vts = ((efusevalue[1] & _BITMASK_(3:0)) << 9) + ((efusevalue[0] & _BITMASK_(15:7)) >> 7);
	g_o_slope_sign = ((efusevalue[1] & _BIT_(4)) >> 4);
	g_o_slope = ((efusevalue[1] & _BITMASK_(10:5)) >> 5);
	g_id = ((efusevalue[1] & _BIT_(11)) >> 11);

	/* Note: O_SLOPE is signed integer. */
	/* O_SLOPE_SIGN=1 ' it is Negative. */
	/* O_SLOPE_SIGN=0 ' it is Positive. */
	mtktspmic_dprintk("[pmic_debug] 6351_efuse: g_o_vts        = %d\n", g_o_vts);
	mtktspmic_dprintk("[pmic_debug] 6351_efuse: g_degc_cali    = %d\n", g_degc_cali);
	mtktspmic_dprintk("[pmic_debug] 6351_efuse: g_adc_cali_en  = %d\n", g_adc_cali_en);
	mtktspmic_dprintk("[pmic_debug] 6351_efuse: g_o_slope      = %d\n", g_o_slope);
	mtktspmic_dprintk("[pmic_debug] 6351_efuse: g_o_slope_sign = %d\n", g_o_slope_sign);
	mtktspmic_dprintk("[pmic_debug] 6351_efuse: g_id           = %d\n", g_id);

	mtktspmic_info("[pmic_debug]  end\n");
}

void mtktspmic_cali_prepare(void)
{
	mtktspmic_read_efuse();

	if (g_id == 0)
		g_o_slope = 0;

	/* g_adc_cali_en=0;//FIX ME */

	if (g_adc_cali_en == 0) {	/* no calibration */
		mtktspmic_info("[pmic_debug]  It isn't calibration values\n");
		g_o_vts = 1600;
		g_degc_cali = 50;
		g_o_slope = 0;
		g_o_slope_sign = 0;
	}

	/*SW workaround patch for mt6755 E2*/
	if (g_degc_cali < 38 || g_degc_cali > 60)
		g_degc_cali = 53;

	mtktspmic_info("[pmic_debug] g_o_vts = 0x%d\n", g_o_vts);
	mtktspmic_info("[pmic_debug] g_degc_cali    = 0x%d\n", g_degc_cali);
	mtktspmic_info("[pmic_debug] g_adc_cali_en  = 0x%d\n", g_adc_cali_en);
	mtktspmic_info("[pmic_debug] g_o_slope      = 0x%d\n", g_o_slope);
	mtktspmic_info("[pmic_debug] g_o_slope_sign = 0x%d\n", g_o_slope_sign);
	mtktspmic_info("[pmic_debug] g_id           = 0x%d\n", g_id);

}

void mtktspmic_cali_prepare2(void)
{

	__s32 vbe_t;
	g_slope1 = (100 * 1000 * 10);	/* 1000 is for 0.001 degree */

	if (g_o_slope_sign == 0)
		g_slope2 = -(1598 + g_o_slope);
	else
		g_slope2 = -(1598 - g_o_slope);

	vbe_t = (-1) * ((((g_o_vts) * 1800)) / 4096) * 1000;

	if (g_o_slope_sign == 0)
		g_intercept = (vbe_t * 1000) / (-(1598 + g_o_slope * 10));	/*0.001 degree */
	else
		g_intercept = (vbe_t * 1000) / (-(1598 - g_o_slope * 10));	/*0.001 degree */

	g_intercept = g_intercept + (g_degc_cali * (1000 / 2));	/* 1000 is for 0.1 degree */

	mtktspmic_info("[Thermal calibration] SLOPE1=%d SLOPE2=%d INTERCEPT=%d, Vbe = %d\n",
		       g_slope1, g_slope2, g_intercept, vbe_t);
}

int mtktspmic_get_hw_temp(void)
{
	int temp = 0, temp1 = 0;

	mutex_lock(&TSPMIC_lock);

	temp = PMIC_IMM_GetOneChannelValue(PMIC_AUX_CH4, y_pmic_repeat_times, 2);

	temp1 = pmic_raw_to_temp(temp);

	mtktspmic_dprintk("[pmic_debug] Raw=%d, T=%d\n", temp, temp1);

	if ((temp1 > 100000) || (temp1 < -30000))
		mtktspmic_info("[mtktspmic_get_hw_temp] raw=%d, PMIC T=%d", temp, temp1);

	if ((temp1 > 150000) || (temp1 < -50000)) {
		mtktspmic_info("[mtktspmic_get_hw_temp] temp(%d) too high, drop this data!\n",
			       temp1);
		temp1 = pre_temp1;
	} else if ((PMIC_counter != 0)
		   && (((pre_temp1 - temp1) > 30000) || ((temp1 - pre_temp1) > 30000))) {
		mtktspmic_info("[mtktspmic_get_hw_temp] temp diff too large, drop this data\n");
		temp1 = pre_temp1;
	} else {
		/* update previous temp */
		pre_temp1 = temp1;
		mtktspmic_dprintk("[mtktspmic_get_hw_temp] pre_temp1=%d\n", pre_temp1);

		if (PMIC_counter == 0)
			PMIC_counter++;
	}

	mutex_unlock(&TSPMIC_lock);

	return temp1;
}
