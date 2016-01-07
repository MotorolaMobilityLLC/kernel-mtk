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
static kal_int32 g_o_vts;
static kal_int32 g_degc_cali;
static kal_int32 g_adc_cali_en;
static kal_int32 g_o_slope;
static kal_int32 g_o_slope_sign;
static kal_int32 g_id;
static kal_int32 g_slope1 = 1;
static kal_int32 g_slope2 = 1;
static kal_int32 g_intercept;

static DEFINE_MUTEX(TSPMIC_lock);
static int pre_temp1 = 0, PMIC_counter;
/*=============================================================*/

static kal_int32 pmic_raw_to_temp(kal_uint32 ret)
{
	kal_int32 y_curr = ret;
	kal_int32 t_current;

	t_current = g_intercept + ((g_slope1 * y_curr) / (g_slope2));
	mtktspmic_dprintk("[pmic_raw_to_temp] t_current=%d\n", t_current);
	return t_current;
}

static void mtktspmic_read_efuse(void)
{
	U32 efusevalue[3];

	mtktspmic_info("[mtktspmic_read_efuse] start\n");
	/*
	   0x8  640     655
	   0x9  656     671
	   0xa  672     687
	   Thermal data from 653 to 680
	 */
	efusevalue[0] = pmic_Read_Efuse_HPOffset(0x8);
	efusevalue[1] = pmic_Read_Efuse_HPOffset(0x9);
	efusevalue[2] = pmic_Read_Efuse_HPOffset(0xa);
	mtktspmic_info("[mtktspmic_read_efuse]6328_efuse:\n"
		       "efusevalue[0]=0x%x\n"
		       "efusevalue[1]=0x%x\n"
		       "efusevalue[2]=0x%x\n\n", efusevalue[0], efusevalue[1], efusevalue[2]);

	g_adc_cali_en = (efusevalue[0] >> 13) & 0x1;
	g_degc_cali = ((efusevalue[0] >> 14) & 0x3) + ((efusevalue[1] & 0xF) << 2);
	g_o_vts = ((efusevalue[1] >> 4) & 0x0FFF) + (((efusevalue[2]) & 0x1) << 11);
	g_o_slope_sign = (efusevalue[2] >> 1) & 0x1;
	g_o_slope = (efusevalue[2] >> 2) & 0x3F;
	g_id = (efusevalue[2] >> 8) & 0x1;

	/* Note: O_SLOPE is signed integer. */
	/* O_SLOPE_SIGN=1 ' it is Negative. */
	/* O_SLOPE_SIGN=0 ' it is Positive. */
	mtktspmic_info("[mtktspmic_read_efuse]6328_efuse: g_o_vts        = %x\n", g_o_vts);
	mtktspmic_info("[mtktspmic_read_efuse]6328_efuse: g_degc_cali    = %x\n", g_degc_cali);
	mtktspmic_info("[mtktspmic_read_efuse]6328_efuse: g_adc_cali_en  = %x\n", g_adc_cali_en);
	mtktspmic_info("[mtktspmic_read_efuse]6328_efuse: g_o_slope      = %x\n", g_o_slope);
	mtktspmic_info("[mtktspmic_read_efuse]6328_efuse: g_o_slope_sign = %x\n", g_o_slope_sign);
	mtktspmic_info("[mtktspmic_read_efuse]6328_efuse: g_id           = %x\n", g_id);

	mtktspmic_info("[mtktspmic_read_efuse] end\n");
}

void mtktspmic_cali_prepare(void)
{
	mtktspmic_read_efuse();

	if (g_id == 0)
		g_o_slope = 0;

	/* g_adc_cali_en=0;//FIX ME */

	if (g_adc_cali_en == 0) {	/* no calibration */
		g_o_vts = 1600;
		g_degc_cali = 50;
		g_o_slope = 0;
		g_o_slope_sign = 0;
	}

	mtktspmic_info("g_o_vts = 0x%x\n", g_o_vts);
	mtktspmic_info("g_degc_cali    = 0x%x\n", g_degc_cali);
	mtktspmic_info("g_adc_cali_en  = 0x%x\n", g_adc_cali_en);
	mtktspmic_info("g_o_slope      = 0x%x\n", g_o_slope);
	mtktspmic_info("g_o_slope_sign = 0x%x\n", g_o_slope_sign);
	mtktspmic_info("g_id           = 0x%x\n", g_id);

}

void mtktspmic_cali_prepare2(void)
{

	kal_int32 vbe_t;

	g_slope1 = (100 * 1000);	/* 1000 is for 0.001 degree */

	if (g_o_slope_sign == 0)
		g_slope2 = -(171 + g_o_slope);
	else
		g_slope2 = -(171 - g_o_slope);

	vbe_t = (-1) * ((((g_o_vts) * 1800)) / 4096) * 1000;

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

	/* AUX_TSENSE_AP is for MT6735 */
	temp = PMIC_IMM_GetOneChannelValue(MT6328_AUX_CH4, y_pmic_repeat_times, 2);
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
