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
	/* __u32 reg_val = 0; */

	__u32 efuse_val0, efuse_val1;

	mtktspmic_info("[mtktspmic_read_efuse] start\n");

	/* 1. enable RD trig bit */
	pmic_config_interface(0x0608, 0x1, 0x1, 0);
	mtktspmic_info("PMIC_Debug %d\n", __LINE__);

	/* 2. read thermal efuse */
	/* 0x063A    160  175 */
	/* 0x063C    176  191 */
	/* Thermal data from 161 to 184 */
	pmic_read_interface(0x063A, &efuse_val0, 0xFFFF, 0);
	pmic_read_interface(0x063C, &efuse_val1, 0xFFFF, 0);
	mtktspmic_info("PMIC_Debug %d\n", __LINE__);

	/* 3. polling RD busy bit */
	/*      do {
	   pmic_read_interface(0x60C, &reg_val, 0x1, 0);
	   mtktspmic_dbg("[mtktspmic_read_efuse] polling Reg[0x60C][0]=0x%x\n", reg_val);
	   } while(reg_val == 1);
	   mtktspmic_info("PMIC_Debug %d\n",__LINE__);
	 */
	mtktspmic_info("[mtktspmic_read_efuse] Reg(0x63a) = 0x%x, Reg(0x63c) = 0x%x\n", efuse_val0,
		       efuse_val1);

	/* 4. parse thermal parameters */
	g_adc_cali_en = (efuse_val0 >> 1) & 0x0001;
	g_degc_cali = (efuse_val0 >> 2) & 0x003f;
	g_o_vts = ((efuse_val1 & 0x001F) << 8) + ((efuse_val0 >> 8) & 0x00FF);
	g_o_slope_sign = (efuse_val1 >> 5) & 0x0001;
	g_o_slope = (((efuse_val1 >> 11) & 0x0007) << 3) + ((efuse_val1 >> 6) & 0x007);
	g_id = (efuse_val1 >> 14) & 0x0001;

	mtktspmic_info("[mtktspmic_read_efuse] done\n");
}

void mtktspmic_cali_prepare(void)
{
	mtktspmic_read_efuse();

	mtktspmic_info("PMIC_Debug %d\n", __LINE__);
	if (g_id == 0)
		g_o_slope = 0;

	if (g_adc_cali_en == 0) {	/* no calibration */
		g_o_vts = 3698;
		g_degc_cali = 50;
		g_o_slope = 0;
		g_o_slope_sign = 0;
	}

	mtktspmic_info("g_o_vts = 0x%x\n", g_o_vts);
	mtktspmic_info("g_degc_cali = 0x%x\n", g_degc_cali);
	mtktspmic_info("g_adc_cali_en = 0x%x\n", g_adc_cali_en);
	mtktspmic_info("g_o_slope = 0x%x\n", g_o_slope);
	mtktspmic_info("g_o_slope_sign = 0x%x\n", g_o_slope_sign);
	mtktspmic_info("g_id = 0x%x\n", g_id);

}

void mtktspmic_cali_prepare2(void)
{
	__s32 vbe_t;

	g_slope1 = (100 * 1000);	/* 1000 is for 0.001 degree */

	if (g_o_slope_sign == 0)
		g_slope2 = -(171 + g_o_slope);
	else
		g_slope2 = -(171 - g_o_slope);

	vbe_t = (-1) * (((g_o_vts + 9102) * 1800) / 32768) * 1000;

	if (g_o_slope_sign == 0)
		g_intercept = (vbe_t * 100) / (-(171 + g_o_slope));	/* 0.001 degree */
	else
		g_intercept = (vbe_t * 100) / (-(171 - g_o_slope));	/* 0.001 degree */

	g_intercept = g_intercept + (g_degc_cali * (1000 / 2));	/* 1000 is for 0.1 degree */

	mtktspmic_info("[Thermal calibration] SLOPE1=%d SLOPE2=%d INTERCEPT=%d, Vbe = %d\n",
		       g_slope1, g_slope2, g_intercept, vbe_t);
}

int mtktspmic_get_hw_temp(void)
{
	int temp = 0, temp1 = 0;

	mutex_lock(&TSPMIC_lock);

	/* TODO: check this */
	temp = PMIC_IMM_GetOneChannelValue(3, y_pmic_repeat_times, 2);
	temp1 = pmic_raw_to_temp(temp);

	mtktspmic_dprintk("[mtktspmic_get_hw_temp] Raw=%d, T=%d\n", temp, temp1);

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
