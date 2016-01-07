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
#include <linux/spinlock.h>
#include <mt-plat/sync_write.h>
#include "mt-plat/mtk_thermal_monitor.h"
#include <linux/seq_file.h>
#include <linux/slab.h>
#include "mtk_thermal_typedefs.h"
#include "mach/mt_thermal.h"
#include "mt_gpufreq.h"

#if defined(CONFIG_MTK_CLKMGR)
#include <mach/mt_clkmgr.h>
#else
#include <linux/clk.h>
#endif

#include <mt_spm.h>
#include <mt_ptp.h>
#include <mach/wd_api.h>
#include <mtk_gpu_utility.h>
#include <linux/time.h>

#include <mach/mt_clkmgr.h>
#define __MT_MTK_TS_CPU_C__
#include <tscpu_settings.h>

/* 1: turn on RT kthread for thermal protection in this sw module; 0: turn off */
#if MTK_TS_CPU_RT
#include <linux/sched.h>
#include <linux/kthread.h>
#endif

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif
#define __MT_MTK_TS_CPU_C__

#include <mt-plat/mt_devinfo.h>
/*=============================================================
 *Local variable definition
 *=============================================================*/
/*
Bank0 : CPU     (TS_MCU2)
Bank1 : SOC+GPU (TS_MCU1)
Bank2 : LTE     (TS_MCU3)
*/

int tscpu_bank_ts[THERMAL_BANK_NUM][ENUM_MAX];
static int tscpu_bank_ts_r[THERMAL_BANK_NUM][ENUM_MAX];

/* chip dependent */
bank_t tscpu_g_bank[THERMAL_BANK_NUM] = {
	[0] = {
	       .ts = {TS_FILL(MCU2)},
	       .ts_number = 1},
	[1] = {
	       .ts = {TS_FILL(MCU1)},
	       .ts_number = 1},
	[2] = {
	       .ts = {TS_FILL(MCU3)},
	       .ts_number = 1},
};

#ifdef CONFIG_OF
const struct of_device_id mt_thermal_of_match[2] = {
	{.compatible = "mediatek,mt6735-therm_ctrl",},
	{},
};
#endif

int tscpu_debug_log = 0;

#if MTK_TS_CPU_RT
static struct task_struct *ktp_thread_handle;
#endif

static S32 g_adc_ge_t;
static S32 g_adc_oe_t;
static S32 g_o_vtsmcu1;
static S32 g_o_vtsmcu2;
static S32 g_o_vtsmcu3;
static S32 g_degc_cali;
static S32 g_adc_cali_en_t;
static S32 g_o_slope;
static S32 g_o_slope_sign;
static S32 g_id;

static S32 g_ge = 1;
static S32 g_oe = 1;
static S32 g_gain = 1;

static S32 g_x_roomt[THERMAL_SENSOR_NUM] = { 0 };

static U32 calefuse1;
static U32 calefuse2;
static U32 calefuse3;
/**
 * If curr_temp >= tscpu_polling_trip_temp1, use interval
 * else if cur_temp >= tscpu_polling_trip_temp2 && curr_temp < tscpu_polling_trip_temp1,
 * use interval*tscpu_polling_factor1
 * else, use interval*tscpu_polling_factor2
 */

/* chip dependent */
int tscpu_polling_trip_temp1 = 30000;
int tscpu_polling_trip_temp2 = 20000;
int tscpu_polling_factor1 = 1;
int tscpu_polling_factor2 = 2;
#if MTKTSCPU_FAST_POLLING
/* Combined fast_polling_trip_temp and fast_polling_factor,
it means polling_delay will be 1/5 of original interval
after mtktscpu reports > 65C w/o exit point */
int fast_polling_trip_temp = 70000;
int fast_polling_factor = 2;
int tscpu_cur_fp_factor = 1;
int tscpu_next_fp_factor = 1;
#endif
/*=============================================================
 * Local function declartation
 *=============================================================*/
static S32 temperature_to_raw_room(U32 ret);
static void set_tc_trigger_hw_protect(int temperature, int temperature2);
/*=============================================================
 *Weak functions
 *=============================================================*/
void __attribute__ ((weak))
mt_ptp_lock(unsigned long *flags)
{
	pr_err("E_WF: %s doesn't exist\n", __func__);
}

void __attribute__ ((weak))
mt_ptp_unlock(unsigned long *flags)
{
	pr_err("E_WF: %s doesn't exist\n", __func__);
}

/*=============================================================*/
/* chip dependent */
int tscpu_thermal_clock_on(void)
{
	int ret = -1;

	tscpu_printk("tscpu_thermal_clock_on\n");

#if defined(CONFIG_MTK_CLKMGR)
	ret = enable_clock(MT_CG_PERI_THERM, "THERMAL");
#else
	tscpu_printk("CCF_thermal_clock_on\n");
	ret = clk_prepare_enable(therm_main);
	if (ret)
		tscpu_printk("Cannot enable thermal clock.\n");
#endif
	return ret;
}

/* chip dependent */
int tscpu_thermal_clock_off(void)
{
	int ret = -1;

	tscpu_dprintk("tscpu_thermal_clock_off\n");

#if defined(CONFIG_MTK_CLKMGR)
	ret = disable_clock(MT_CG_PERI_THERM, "THERMAL");
#else
	tscpu_printk("CCF_thermal_clock_off\n");
	clk_disable_unprepare(therm_main);
#endif

	return ret;
}

#if 0
static void get_thermal_all_register(void)
{
	tscpu_dprintk("get_thermal_all_register\n");

	tscpu_dprintk("TEMPMSR1			  = 0x%8x\n", DRV_Reg32(TEMPMSR1));
	tscpu_dprintk("TEMPMSR2            = 0x%8x\n", DRV_Reg32(TEMPMSR2));

	tscpu_dprintk("TEMPMONCTL0	  = 0x%8x\n", DRV_Reg32(TEMPMONCTL0));
	tscpu_dprintk("TEMPMONCTL1	  = 0x%8x\n", DRV_Reg32(TEMPMONCTL1));
	tscpu_dprintk("TEMPMONCTL2	  = 0x%8x\n", DRV_Reg32(TEMPMONCTL2));
	tscpu_dprintk("TEMPMONINT	  = 0x%8x\n", DRV_Reg32(TEMPMONINT));
	tscpu_dprintk("TEMPMONINTSTS	  = 0x%8x\n", DRV_Reg32(TEMPMONINTSTS));
	tscpu_dprintk("TEMPMONIDET0	  = 0x%8x\n", DRV_Reg32(TEMPMONIDET0));

	tscpu_dprintk("TEMPMONIDET1	  = 0x%8x\n", DRV_Reg32(TEMPMONIDET1));
	tscpu_dprintk("TEMPMONIDET2	  = 0x%8x\n", DRV_Reg32(TEMPMONIDET2));
	tscpu_dprintk("TEMPH2NTHRE	  = 0x%8x\n", DRV_Reg32(TEMPH2NTHRE));
	tscpu_dprintk("TEMPHTHRE	  = 0x%8x\n", DRV_Reg32(TEMPHTHRE));
	tscpu_dprintk("TEMPCTHRE	  = 0x%8x\n", DRV_Reg32(TEMPCTHRE));
	tscpu_dprintk("TEMPOFFSETH	  = 0x%8x\n", DRV_Reg32(TEMPOFFSETH));

	tscpu_dprintk("TEMPOFFSETL	  = 0x%8x\n", DRV_Reg32(TEMPOFFSETL));
	tscpu_dprintk("TEMPMSRCTL0	  = 0x%8x\n", DRV_Reg32(TEMPMSRCTL0));
	tscpu_dprintk("TEMPMSRCTL1	  = 0x%8x\n", DRV_Reg32(TEMPMSRCTL1));
	tscpu_dprintk("TEMPAHBPOLL	  = 0x%8x\n", DRV_Reg32(TEMPAHBPOLL));
	tscpu_dprintk("TEMPAHBTO	  = 0x%8x\n", DRV_Reg32(TEMPAHBTO));
	tscpu_dprintk("TEMPADCPNP0	  = 0x%8x\n", DRV_Reg32(TEMPADCPNP0));

	tscpu_dprintk("TEMPADCPNP1	  = 0x%8x\n", DRV_Reg32(TEMPADCPNP1));
	tscpu_dprintk("TEMPADCPNP2	  = 0x%8x\n", DRV_Reg32(TEMPADCPNP2));
	tscpu_dprintk("TEMPADCMUX	  = 0x%8x\n", DRV_Reg32(TEMPADCMUX));
	tscpu_dprintk("TEMPADCEXT	  = 0x%8x\n", DRV_Reg32(TEMPADCEXT));
	tscpu_dprintk("TEMPADCEXT1	  = 0x%8x\n", DRV_Reg32(TEMPADCEXT1));
	tscpu_dprintk("TEMPADCEN	  = 0x%8x\n", DRV_Reg32(TEMPADCEN));


	tscpu_dprintk("TEMPPNPMUXADDR      = 0x%8x\n", DRV_Reg32(TEMPPNPMUXADDR));
	tscpu_dprintk("TEMPADCMUXADDR      = 0x%8x\n", DRV_Reg32(TEMPADCMUXADDR));
	tscpu_dprintk("TEMPADCEXTADDR      = 0x%8x\n", DRV_Reg32(TEMPADCEXTADDR));
	tscpu_dprintk("TEMPADCEXT1ADDR     = 0x%8x\n", DRV_Reg32(TEMPADCEXT1ADDR));
	tscpu_dprintk("TEMPADCENADDR       = 0x%8x\n", DRV_Reg32(TEMPADCENADDR));
	tscpu_dprintk("TEMPADCVALIDADDR    = 0x%8x\n", DRV_Reg32(TEMPADCVALIDADDR));

	tscpu_dprintk("TEMPADCVOLTADDR     = 0x%8x\n", DRV_Reg32(TEMPADCVOLTADDR));
	tscpu_dprintk("TEMPRDCTRL          = 0x%8x\n", DRV_Reg32(TEMPRDCTRL));
	tscpu_dprintk("TEMPADCVALIDMASK    = 0x%8x\n", DRV_Reg32(TEMPADCVALIDMASK));
	tscpu_dprintk("TEMPADCVOLTAGESHIFT = 0x%8x\n", DRV_Reg32(TEMPADCVOLTAGESHIFT));
	tscpu_dprintk("TEMPADCWRITECTRL    = 0x%8x\n", DRV_Reg32(TEMPADCWRITECTRL));
	tscpu_dprintk("TEMPMSR0            = 0x%8x\n", DRV_Reg32(TEMPMSR0));


	tscpu_dprintk("TEMPIMMD0           = 0x%8x\n", DRV_Reg32(TEMPIMMD0));
	tscpu_dprintk("TEMPIMMD1           = 0x%8x\n", DRV_Reg32(TEMPIMMD1));
	tscpu_dprintk("TEMPIMMD2           = 0x%8x\n", DRV_Reg32(TEMPIMMD2));
	tscpu_dprintk("TEMPPROTCTL         = 0x%8x\n", DRV_Reg32(TEMPPROTCTL));

	tscpu_dprintk("TEMPPROTTA          = 0x%8x\n", DRV_Reg32(TEMPPROTTA));
	tscpu_dprintk("TEMPPROTTB		  = 0x%8x\n", DRV_Reg32(TEMPPROTTB));
	tscpu_dprintk("TEMPPROTTC		  = 0x%8x\n", DRV_Reg32(TEMPPROTTC));
	tscpu_dprintk("TEMPSPARE0		  = 0x%8x\n", DRV_Reg32(TEMPSPARE0));
	tscpu_dprintk("TEMPSPARE1		  = 0x%8x\n", DRV_Reg32(TEMPSPARE1));
	tscpu_dprintk("TEMPSPARE2		  = 0x%8x\n", DRV_Reg32(TEMPSPARE2));
	tscpu_dprintk("TEMPSPARE3		  = 0x%8x\n", DRV_Reg32(TEMPSPARE3));
	/* tscpu_dprintk("0x11001040               = 0x%8x\n", DRV_Reg32(0xF1001040)); */

}
#endif

/* TODO: FIXME */
void get_thermal_slope_intercept(struct TS_PTPOD *ts_info, thermal_bank_name ts_bank)
{
	unsigned int temp0, temp1, temp2;
	struct TS_PTPOD ts_ptpod;
	S32 x_roomt;

	tscpu_dprintk("get_thermal_slope_intercept\n");

/*
Bank0 : CPU     (TS_MCU2)
Bank1 : SOC+GPU (TS_MCU1)
Bank2 : LTE     (TS_MCU3)
*/
	/*
	   If there are two or more sensors in a bank, choose the sensor calibration value of
	   the dominant sensor. You can observe it in the thermal doc provided by Thermal DE.
	   For example,
	   Bank 1 is for SOC + GPU. Observe all scenarios related to GPU tests to
	   determine which sensor is the highest temperature in all tests.
	   Then, It is the dominant sensor.
	   (Confirmed by Thermal DE Alfred Tsai)
	 */
/* chip dependent */
	switch (ts_bank) {
	case THERMAL_BANK0:	/* CPU (TS_MCU2) */
		x_roomt = g_x_roomt[1];
		break;
	case THERMAL_BANK1:	/* GPU (TS_MCU1) */
		x_roomt = g_x_roomt[0];
		break;
	case THERMAL_BANK2:	/* LTE (TS_MCU3) */
		x_roomt = g_x_roomt[2];
		break;
	default:		/* choose high temp */
		x_roomt = g_x_roomt[1];
		break;
	}

	/*
	   The equations in this function are confirmed by Thermal DE Alfred Tsai.
	   Don't have to change until using next generation thermal sensors.
	 */

	temp0 = (10000 * 100000 / g_gain) * 15 / 18;

	if (g_o_slope_sign == 0)
		temp1 = temp0 / (165 + g_o_slope);
	else
		temp1 = temp0 / (165 - g_o_slope);

	ts_ptpod.ts_MTS = temp1;

	temp0 = (g_degc_cali * 10 / 2);
	temp1 = ((10000 * 100000 / 4096 / g_gain) * g_oe + x_roomt * 10) * 15 / 18;

	if (g_o_slope_sign == 0)
		temp2 = temp1 * 10 / (165 + g_o_slope);
	else
		temp2 = temp1 * 10 / (165 - g_o_slope);

	ts_ptpod.ts_BTS = (temp0 + temp2 - 250) * 4 / 10;


	ts_info->ts_MTS = ts_ptpod.ts_MTS;
	ts_info->ts_BTS = ts_ptpod.ts_BTS;
	tscpu_printk("ts_MTS=%d, ts_BTS=%d\n", ts_ptpod.ts_MTS, ts_ptpod.ts_BTS);
}
EXPORT_SYMBOL(get_thermal_slope_intercept);

/* chip dependent */
void mtkts_dump_cali_info(void)
{
	tscpu_printk("[cal] g_adc_ge_t      = 0x%x\n", g_adc_ge_t);
	tscpu_printk("[cal] g_adc_oe_t      = 0x%x\n", g_adc_oe_t);
	tscpu_printk("[cal] g_degc_cali     = 0x%x\n", g_degc_cali);
	tscpu_printk("[cal] g_adc_cali_en_t = 0x%x\n", g_adc_cali_en_t);
	tscpu_printk("[cal] g_o_slope       = 0x%x\n", g_o_slope);
	tscpu_printk("[cal] g_o_slope_sign  = 0x%x\n", g_o_slope_sign);
	tscpu_printk("[cal] g_id            = 0x%x\n", g_id);

	tscpu_printk("[cal] g_o_vtsmcu2     = 0x%x\n", g_o_vtsmcu2);
	tscpu_printk("[cal] g_o_vtsmcu3     = 0x%x\n", g_o_vtsmcu3);

}


void tscpu_thermal_cal_prepare(void)
{
	U32 temp0 = 0, temp1 = 0, temp2 = 0;

	temp0 = get_devinfo_with_index(ADDRESS_INDEX_0);
	temp1 = get_devinfo_with_index(ADDRESS_INDEX_1);
	temp2 = get_devinfo_with_index(ADDRESS_INDEX_2);


	pr_debug("[calibration] temp0=0x%x, temp1=0x%x, temp2=0x%x\n", temp0, temp1, temp2);

	/* chip dependent */
	g_adc_ge_t = ((temp0 & 0xFFC00000) >> 22);	/* ADC_GE_T    [9:0] *(0x102061A0)[31:22] */
	g_adc_oe_t = ((temp0 & 0x003FF000) >> 12);	/* ADC_OE_T    [9:0] *(0x102061A0)[21:12] */

	g_o_vtsmcu1 = (temp1 & 0x03FE0000) >> 17;	/* O_VTSMCU1    (9b) *(0x1020619C)[25:17] */
	g_o_vtsmcu2 = (temp1 & 0x0001FF00) >> 8;	/* O_VTSMCU2    (9b) *(0x1020619C)[16:8] */
	g_o_vtsmcu3 = (temp0 & 0x000001FF);	/* O_VTSMCU3    (9b) *(0x10206184)[8:0] */

	g_degc_cali = (temp1 & 0x0000007E) >> 1;	/* DEGC_cali    (6b) *(0x1020619C)[6:1] */
	g_adc_cali_en_t = (temp1 & 0x00000001);	/* ADC_CALI_EN_T(1b) *(0x1020619C)[0] */
	g_o_slope_sign = (temp1 & 0x00000080) >> 7;	/* O_SLOPE_SIGN (1b) *(0x1020619C)[7] */
	g_o_slope = (temp1 & 0xFC000000) >> 26;	/* O_SLOPE      (6b) *(0x1020619C)[31:26] */

	g_id = (temp0 & 0x00000200) >> 9;	/* ID           (1b) *(0x102061A0)[9] */

	/*
	   Check ID bit
	   If ID=0 (TSMC sample)    , ignore O_SLOPE EFuse value and set O_SLOPE=0.
	   If ID=1 (non-TSMC sample), read O_SLOPE EFuse value for following calculation.
	 */
	if (g_id == 0)
		g_o_slope = 0;

	/* g_adc_cali_en_t=0;//test only */
	if (g_adc_cali_en_t == 1) {
		/* thermal_enable = true; */
	} else {
		tscpu_printk("This sample is not Thermal calibrated\n");
		g_adc_ge_t = 512;
		g_adc_oe_t = 512;
		g_degc_cali = 40;
		g_o_slope = 0;
		g_o_slope_sign = 0;
		g_o_vtsmcu1 = 260;
		g_o_vtsmcu2 = 260;
		g_o_vtsmcu3 = 260;

	}

	mtkts_dump_cali_info();
}

void tscpu_thermal_cal_prepare_2(U32 ret)
{
	S32 format_1 = 0, format_2 = 0, format_3 = 0;

	/* tscpu_printk("tscpu_thermal_cal_prepare_2\n"); */

	g_ge = ((g_adc_ge_t - 512) * 10000) / 4096;	/* ge * 10000 */
	g_oe = (g_adc_oe_t - 512);

	g_gain = (10000 + g_ge);

	format_1 = (g_o_vtsmcu1 + 3350 - g_oe);
	format_2 = (g_o_vtsmcu2 + 3350 - g_oe);
	format_3 = (g_o_vtsmcu3 + 3350 - g_oe);

	g_x_roomt[0] = (((format_1 * 10000) / 4096) * 10000) / g_gain;	/* g_x_roomt1 * 10000 */
	g_x_roomt[1] = (((format_2 * 10000) / 4096) * 10000) / g_gain;	/* g_x_roomt2 * 10000 */
	g_x_roomt[2] = (((format_3 * 10000) / 4096) * 10000) / g_gain;	/* g_x_roomt3 * 10000 */
	/*
	   tscpu_printk("[cal] g_ge         = 0x%x\n",g_ge);
	   tscpu_printk("[cal] g_gain       = 0x%x\n",g_gain);

	   tscpu_printk("[cal] g_x_roomt1   = 0x%x\n",g_x_roomt[0]);
	   tscpu_printk("[cal] g_x_roomt2   = 0x%x\n",g_x_roomt[1]);
	   tscpu_printk("[cal] g_x_roomt3   = 0x%x\n",g_x_roomt[2]);
	 */
}

#if THERMAL_CONTROLLER_HW_TP
static S32 temperature_to_raw_room(U32 ret)
{
	/* Ycurr = [(Tcurr - DEGC_cali/2)*(165+O_slope)*(18/15)*(1/10000)+X_roomtabb]*Gain*4096 + OE */

	S32 t_curr = ret;
	S32 format_1 = 0;
	S32 format_2 = 0;
	S32 format_3[THERMAL_SENSOR_NUM] = { 0 };
	S32 format_4[THERMAL_SENSOR_NUM] = { 0 };
	S32 i, index = 0, temp = 0;

	/* tscpu_dprintk("temperature_to_raw_room\n"); */

	if (g_o_slope_sign == 0) {	/* O_SLOPE is Positive. */
		format_1 = t_curr - (g_degc_cali * 1000 / 2);
		format_2 = format_1 * (165 + g_o_slope) * 18 / 15;
		format_2 = format_2 - 2 * format_2;

		for (i = 0; i < THERMAL_SENSOR_NUM; i++) {
			format_3[i] = format_2 / 1000 + g_x_roomt[i] * 10;
			format_4[i] = (format_3[i] * 4096 / 10000 * g_gain) / 100000 + g_oe;
		}

	} else {		/* O_SLOPE is Negative. */
		format_1 = t_curr - (g_degc_cali * 1000 / 2);
		format_2 = format_1 * (165 - g_o_slope) * 18 / 15;
		format_2 = format_2 - 2 * format_2;

		for (i = 0; i < THERMAL_SENSOR_NUM; i++) {
			format_3[i] = format_2 / 1000 + g_x_roomt[i] * 10;
			format_4[i] = (format_3[i] * 4096 / 10000 * g_gain) / 100000 + g_oe;
		}

		/* tscpu_dprintk("temperature_to_raw_abb format_1=%d, format_2=%d, format_3=%d, format_4=%d\n",
			format_1, format_2, format_3, format_4); */
	}


	temp = 0;
	for (i = 0; i < THERMAL_SENSOR_NUM; i++) {
		if (temp < format_4[i]) {
			temp = format_4[i];
			index = i;
		}
	}

	/* tscpu_dprintk("[Temperature_to_raw_roomt] temperature=%d, raw[%d]=%d", ret, index, format_4[index]); */
	return format_4[index];

}
#endif

static S32 raw_to_temperature_roomt(U32 ret, thermal_sensor_name ts_name)
{
	S32 t_current = 0;
	S32 y_curr = ret;
	S32 format_1 = 0;
	S32 format_2 = 0;
	S32 format_3 = 0;
	S32 format_4 = 0;
	S32 xtoomt = 0;


	xtoomt = g_x_roomt[ts_name];

	/* tscpu_dprintk("raw_to_temperature_room,ts_num=%d,xtoomt=%d\n",ts_name,xtoomt); */

	if (ret == 0)
		return 0;

	format_1 = ((g_degc_cali * 10) >> 1);
	format_2 = (y_curr - g_oe);

	format_3 = (((((format_2) * 10000) >> 12) * 10000) / g_gain) - xtoomt;
	format_3 = format_3 * 15 / 18;


	if (g_o_slope_sign == 0)
		format_4 = ((format_3 * 100) / (165 + g_o_slope));	/* uint = 0.1 deg */
	else
		format_4 = ((format_3 * 100) / (165 - g_o_slope));	/* uint = 0.1 deg */

	format_4 = format_4 - (format_4 << 1);


	t_current = format_1 + format_4;	/* uint = 0.1 deg */

	/* tscpu_dprintk("raw_to_temperature_room,t_current=%d\n",t_current); */
	return t_current;
}

/*
Bank0 : CPU     (TS_MCU2)
Bank1 : SOC+GPU (TS_MCU1)
Bank2 : LTE     (TS_MCU3)
*/
/* chip dependent */
int get_immediate_cpu_wrap(void)
{
	int curr_temp;

	curr_temp = tscpu_bank_ts[THERMAL_BANK0][MCU2];
	tscpu_dprintk("get_immediate_cpu_wrap curr_temp=%d\n", curr_temp);

	return curr_temp;
}

int get_immediate_gpu_wrap(void)
{
	int curr_temp;

	curr_temp = tscpu_bank_ts[THERMAL_BANK1][MCU1];
	tscpu_dprintk("get_immediate_gpu_wrap curr_temp=%d\n", curr_temp);

	return curr_temp;
}

int get_immediate_lte_wrap(void)
{
	int curr_temp;

	curr_temp = tscpu_bank_ts[THERMAL_BANK2][MCU3];
	tscpu_dprintk("get_immediate_soc_wrap curr_temp=%d\n", curr_temp);

	return curr_temp;
}

/*
Bank0 : CPU     (TS_MCU2)
Bank1 : SOC+GPU (TS_MCU1)
Bank2 : LTE     (TS_MCU3)
*/
/* chip dependent */
int get_immediate_ts1_wrap(void)
{
	int curr_temp;

	/* curr_temp = GPU_TS_MCU1_T; */
	curr_temp = tscpu_bank_ts[THERMAL_BANK1][MCU1];
	tscpu_dprintk("get_immediate_ts1_wrap curr_temp=%d\n", curr_temp);

	return curr_temp;
}

int get_immediate_ts2_wrap(void)
{
	int curr_temp;

	/* curr_temp = CPU_TS_MCU2_T; */
	curr_temp = tscpu_bank_ts[THERMAL_BANK0][MCU2];
	tscpu_dprintk("get_immediate_ts2_wrap curr_temp=%d\n", curr_temp);

	return curr_temp;
}

int get_immediate_ts3_wrap(void)
{
	int curr_temp;

	/* curr_temp = LTE_TS_MCU3_T; */
	curr_temp = tscpu_bank_ts[THERMAL_BANK2][MCU3];
	tscpu_dprintk("get_immediate_ts3_wrap curr_temp=%d\n", curr_temp);

	return curr_temp;
}

static void thermal_interrupt_handler(int bank)
{
	U32 ret = 0;
	unsigned long flags;

	mt_ptp_lock(&flags);

	tscpu_switch_bank(bank);

	ret = DRV_Reg32(TEMPMONINTSTS);
	/* pr_debug("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n"); */
	tscpu_printk("thermal_interrupt_handler,bank=0x%08x,ret=0x%08x\n", bank, ret);
	/* pr_debug("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n"); */

	/* ret2 = DRV_Reg32(THERMINTST); */
	/* pr_debug("thermal_interrupt_handler : THERMINTST = 0x%x\n", ret2); */


	/* for SPM reset debug */
	/* dump_spm_reg(); */

	/* tscpu_printk("thermal_isr: [Interrupt trigger]: status = 0x%x\n", ret); */
	if (ret & THERMAL_MON_CINTSTS0)
		tscpu_printk("thermal_isr: thermal sensor point 0 - cold interrupt trigger\n");

	if (ret & THERMAL_MON_HINTSTS0)
		tscpu_printk("<<<thermal_isr>>>: thermal sensor point 0 - hot interrupt trigger\n");

	if (ret & THERMAL_MON_HINTSTS1)
		tscpu_printk("<<<thermal_isr>>>: thermal sensor point 1 - hot interrupt trigger\n");

	if (ret & THERMAL_MON_HINTSTS2)
		tscpu_printk("<<<thermal_isr>>>: thermal sensor point 2 - hot interrupt trigger\n");

	if (ret & THERMAL_tri_SPM_State0)
		tscpu_printk("thermal_isr: Thermal state0 to trigger SPM state0\n");

	if (ret & THERMAL_tri_SPM_State1) {
		/* tscpu_printk("thermal_isr: Thermal state1 to trigger SPM state1\n"); */
#if MTK_TS_CPU_RT
		/* tscpu_printk("THERMAL_tri_SPM_State1, T=%d,%d,%d\n", CPU_TS_MCU2_T, GPU_TS_MCU1_T, LTE_TS_MCU3_T); */
		wake_up_process(ktp_thread_handle);
#endif
	}
	if (ret & THERMAL_tri_SPM_State2)
		tscpu_printk("thermal_isr: Thermal state2 to trigger SPM state2\n");

	mt_ptp_unlock(&flags);

}

irqreturn_t tscpu_thermal_all_bank_interrupt_handler(int irq, void *dev_id)
{
	U32 ret = 0, i, mask = 1;


	ret = DRV_Reg32(THERMINTST);
	ret = ret & 0xF;

	pr_debug("thermal_interrupt_handler : THERMINTST = 0x%x\n", ret);

	for (i = 0; i < TS_LEN_ARRAY(tscpu_g_bank); i++) {
		mask = 1 << i;
		if ((ret & mask) == 0)
			thermal_interrupt_handler(i);
	}

	return IRQ_HANDLED;
}

static void thermal_reset_and_initial(void)
{

	/* pr_debug( "thermal_reset_and_initial\n"); */

	/* tscpu_thermal_clock_on(); */

	/* Calculating period unit in Module clock x 256, and the Module clock */
	/* will be changed to 26M when Infrasys enters Sleep mode. */
	/* THERMAL_WRAP_WR32(0x000003FF, TEMPMONCTL1);    // counting unit is 1023 * 15.15ns ~ 15.5us */


	/* bus clock 66M counting unit is 4*15.15ns* 256 = 15513.6 ms=15.5us */
	/* THERMAL_WRAP_WR32(0x00000004, TEMPMONCTL1);*/
	/* bus clock 66M counting unit is 12*15.15ns* 256 = 46.540us */
	THERMAL_WRAP_WR32(0x0000000C, TEMPMONCTL1);
	/* bus clock 66M counting unit is 4*15.15ns* 256 = 15513.6 ms=15.5us */
	/* THERMAL_WRAP_WR32(0x000001FF, TEMPMONCTL1);*/

#if THERMAL_CONTROLLER_HW_FILTER == 2
	THERMAL_WRAP_WR32(0x07E007E0, TEMPMONCTL2);	/* both filt and sen interval is 2016*15.5us = 31.25ms */
	THERMAL_WRAP_WR32(0x001F7972, TEMPAHBPOLL);	/* poll is set to 31.25ms */
	THERMAL_WRAP_WR32(0x00000049, TEMPMSRCTL0);	/* temperature sampling control, 2 out of 4 samples */
#elif THERMAL_CONTROLLER_HW_FILTER == 4
	THERMAL_WRAP_WR32(0x050A050A, TEMPMONCTL2);	/* both filt and sen interval is 20ms */
	THERMAL_WRAP_WR32(0x001424C4, TEMPAHBPOLL);	/* poll is set to 20ms */
	THERMAL_WRAP_WR32(0x000000DB, TEMPMSRCTL0);	/* temperature sampling control, 4 out of 6 samples */
#elif THERMAL_CONTROLLER_HW_FILTER == 8
	THERMAL_WRAP_WR32(0x03390339, TEMPMONCTL2);	/* both filt and sen interval is 12.5ms */
	THERMAL_WRAP_WR32(0x000C96FA, TEMPAHBPOLL);	/* poll is set to 12.5ms */
	THERMAL_WRAP_WR32(0x00000124, TEMPMSRCTL0);	/* temperature sampling control, 8 out of 10 samples */
#elif THERMAL_CONTROLLER_HW_FILTER == 16
	THERMAL_WRAP_WR32(0x01C001C0, TEMPMONCTL2);	/* both filt and sen interval is 6.94ms */
	THERMAL_WRAP_WR32(0x0006FE8B, TEMPAHBPOLL);	/* poll is set to 458379*15.15= 6.94ms */
	THERMAL_WRAP_WR32(0x0000016D, TEMPMSRCTL0);	/* temperature sampling control, 16 out of 18 samples */
#else				/* default 1 */
	/* filt interval is 1 * 46.540us = 46.54us, sen interval is 429 * 46.540us = 19.96ms */
	THERMAL_WRAP_WR32(0x000101AD, TEMPMONCTL2);
	/* filt interval is 1 * 46.540us = 46.54us, sen interval is 858 * 46.540us = 39.93ms */
	/* THERMAL_WRAP_WR32(0x0001035A, TEMPMONCTL2);*/
	/* filt interval is 1 * 46.540us = 46.54us, sen interval is 1287 * 46.540us = 59.89 ms */
	/* THERMAL_WRAP_WR32(0x00010507, TEMPMONCTL2);*/
	/* THERMAL_WRAP_WR32(0x00000001, TEMPAHBPOLL);  // poll is set to 1 * 46.540us = 46.540us */
	THERMAL_WRAP_WR32(0x00000300, TEMPAHBPOLL);	/* poll is set to 10u */
	THERMAL_WRAP_WR32(0x00000000, TEMPMSRCTL0);	/* temperature sampling control, 1 sample */
#endif

	THERMAL_WRAP_WR32(0xFFFFFFFF, TEMPAHBTO);	/* exceed this polling time, IRQ would be inserted */

	THERMAL_WRAP_WR32(0x00000000, TEMPMONIDET0);	/* times for interrupt occurrance */
	THERMAL_WRAP_WR32(0x00000000, TEMPMONIDET1);	/* times for interrupt occurrance */

	/* this value will be stored to TEMPPNPMUXADDR (TEMPSPARE0) automatically by hw */
	THERMAL_WRAP_WR32(0x800, TEMPADCMUX);
	THERMAL_WRAP_WR32((UINT32) AUXADC_CON1_CLR_P, TEMPADCMUXADDR);	/* AHB address for auxadc mux selection */
	/* THERMAL_WRAP_WR32(0x1100100C, TEMPADCMUXADDR);// AHB address for auxadc mux selection */

	THERMAL_WRAP_WR32(0x800, TEMPADCEN);	/* AHB value for auxadc enable */
	/* AHB address for auxadc enable (channel 0 immediate mode selected) */
	THERMAL_WRAP_WR32((UINT32) AUXADC_CON1_SET_P, TEMPADCENADDR);
	/* THERMAL_WRAP_WR32(0x11001008, TEMPADCENADDR);*/
	/* AHB address for auxadc enable (channel 0 immediate mode selected)
	this value will be stored to TEMPADCENADDR automatically by hw */


	THERMAL_WRAP_WR32((UINT32) AUXADC_DAT11_P, TEMPADCVALIDADDR);	/* AHB address for auxadc valid bit */
	THERMAL_WRAP_WR32((UINT32) AUXADC_DAT11_P, TEMPADCVOLTADDR);	/* AHB address for auxadc voltage output */
	/* THERMAL_WRAP_WR32(0x11001040, TEMPADCVALIDADDR); // AHB address for auxadc valid bit */
	/* THERMAL_WRAP_WR32(0x11001040, TEMPADCVOLTADDR);  // AHB address for auxadc voltage output */


	THERMAL_WRAP_WR32(0x0, TEMPRDCTRL);	/* read valid & voltage are at the same register */
	/* indicate where the valid bit is (the 12th bit is valid bit and 1 is valid) */
	THERMAL_WRAP_WR32(0x0000002C, TEMPADCVALIDMASK);
	THERMAL_WRAP_WR32(0x0, TEMPADCVOLTAGESHIFT);	/* do not need to shift */
	/* THERMAL_WRAP_WR32(0x2, TEMPADCWRITECTRL);                     // enable auxadc mux write transaction */

}

/* Refactor */
static int thermal_config_Bank(void)
{
	int i, j = 0;
	int ret = -1;
	UINT32 temp = 0;

	/* AuxADC Initialization,ref MT6592_AUXADC.doc // TODO: check this line */
	temp = DRV_Reg32(AUXADC_CON0_V);	/* Auto set enable for CH11 */
	temp &= 0xFFFFF7FF;	/* 0: Not AUTOSET mode */
	THERMAL_WRAP_WR32(temp, AUXADC_CON0_V);	/* disable auxadc channel 11 synchronous mode */
	THERMAL_WRAP_WR32(0x800, AUXADC_CON1_CLR_V);	/* disable auxadc channel 11 immediate mode */


	for (i = 0; i < TS_LEN_ARRAY(tscpu_g_bank); i++) {
		ret = tscpu_switch_bank(i);
		thermal_reset_and_initial();

		for (j = 0; j < tscpu_g_bank[i].ts_number; j++) {
			tscpu_dprintk("%s i %d, j %d\n", __func__, i, j);
			tscpu_thermal_tempADCPNP(tscpu_thermal_ADCValueOfMcu
						 (tscpu_g_bank[i].ts[j].type), j);
		}

		THERMAL_WRAP_WR32(TS_CONFIGURE_P, TEMPPNPMUXADDR);
		THERMAL_WRAP_WR32(0x3, TEMPADCWRITECTRL);
	}

	THERMAL_WRAP_WR32(0x800, AUXADC_CON1_SET_V);

	for (i = 0; i < TS_LEN_ARRAY(tscpu_g_bank); i++) {
		ret = tscpu_switch_bank(i);
		tscpu_thermal_enable_all_periodoc_sensing_point(i);
	}

	return ret;
}


/**
 *  temperature2 to set the middle threshold for interrupting CPU. -275000 to disable it.
 */
static void set_tc_trigger_hw_protect(int temperature, int temperature2)
{
	int temp = 0;
	int raw_high, raw_middle, raw_low;


	/* temperature2=80000;  test only */
	tscpu_dprintk("set_tc_trigger_hw_protect t1=%d t2=%d\n", temperature, temperature2);


	/* temperature to trigger SPM state2 */
	raw_high = temperature_to_raw_room(temperature);
	if (temperature2 > -275000)
		raw_middle = temperature_to_raw_room(temperature2);
	raw_low = temperature_to_raw_room(5000);


	temp = DRV_Reg32(TEMPMONINT);
	/* tscpu_printk("set_tc_trigger_hw_protect 1 TEMPMONINT:temp=0x%x\n",temp); */
	/* THERMAL_WRAP_WR32(temp & 0x1FFFFFFF, TEMPMONINT);     // disable trigger SPM interrupt */
	THERMAL_WRAP_WR32(temp & 0x00000000, TEMPMONINT);	/* disable trigger SPM interrupt */


	/* THERMAL_WRAP_WR32(0x60000, TEMPPROTCTL);// set hot to wakeup event control */
	/* THERMAL_WRAP_WR32(0x20000, TEMPPROTCTL);// set hot to wakeup event control */
	THERMAL_WRAP_WR32(0x20000, TEMPPROTCTL);	/* set hot to wakeup event control */

	THERMAL_WRAP_WR32(raw_low, TEMPPROTTA);
	if (temperature2 > -275000)
		THERMAL_WRAP_WR32(raw_middle, TEMPPROTTB);	/* register will remain unchanged if -275000... */


	THERMAL_WRAP_WR32(raw_high, TEMPPROTTC);	/* set hot to HOT wakeup event */


	/*trigger cold ,normal and hot interrupt */
	/* remove for temp       THERMAL_WRAP_WR32(temp | 0xE0000000, TEMPMONINT);
	 * enable trigger SPM interrupt */
	/*Only trigger hot interrupt */
	if (temperature2 > -275000)
		THERMAL_WRAP_WR32(temp | 0xC0000000, TEMPMONINT);	/* enable trigger middle & Hot SPM interrupt */
	else
		THERMAL_WRAP_WR32(temp | 0x80000000, TEMPMONINT);	/* enable trigger Hot SPM interrupt */
}


static int read_tc_raw_and_temp(volatile u32 *tempmsr_name, thermal_sensor_name ts_name,
				int *ts_raw)
{
	int temp = 0, raw = 0;

	raw = (tempmsr_name != 0) ? (DRV_Reg32((tempmsr_name)) & 0x0fff) : 0;
	temp = (tempmsr_name != 0) ? raw_to_temperature_roomt(raw, ts_name) : 0;

	*ts_raw = raw;
	tscpu_dprintk("read_tc_raw_temp,ts_raw=%d,temp=%d\n", *ts_raw, temp * 100);

	return temp * 100;
}


void tscpu_thermal_read_bank_temp(thermal_bank_name bank, ts_e type, int order)
{

	tscpu_dprintk("%s bank %d type %d order %d\n", __func__, bank, type, order);

	switch (order) {
	case 0:
		tscpu_bank_ts[bank][type] =
		    read_tc_raw_and_temp((volatile u32 *)TEMPMSR0, type,
					 &tscpu_bank_ts_r[bank][type]);
		tscpu_dprintk("%s order %d bank %d type %d tscpu_bank_ts %d tscpu_bank_ts_r %d\n",
			      __func__, order, bank, type, tscpu_bank_ts[bank][type],
			      tscpu_bank_ts_r[bank][type]);
		break;
	case 1:
		tscpu_bank_ts[bank][type] =
		    read_tc_raw_and_temp((volatile u32 *)TEMPMSR1, type,
					 &tscpu_bank_ts_r[bank][type]);
		tscpu_dprintk("%s order %d bank %d type %d tscpu_bank_ts %d tscpu_bank_ts_r %d\n",
			      __func__, order, bank, type, tscpu_bank_ts[bank][type],
			      tscpu_bank_ts_r[bank][type]);
		break;
	case 2:
		tscpu_bank_ts[bank][type] =
		    read_tc_raw_and_temp((volatile u32 *)TEMPMSR2, type,
					 &tscpu_bank_ts_r[bank][type]);
		tscpu_dprintk("%s order %d bank %d type %d tscpu_bank_ts %d tscpu_bank_ts_r %d\n",
			      __func__, order, bank, type, tscpu_bank_ts[bank][type],
			      tscpu_bank_ts_r[bank][type]);
		break;
	case 3:
		tscpu_bank_ts[bank][type] =
		    read_tc_raw_and_temp((volatile u32 *)TEMPMSR3, type,
					 &tscpu_bank_ts_r[bank][type]);
		tscpu_dprintk("%s order %d bank %d type %d tscpu_bank_ts %d tscpu_bank_ts_r %d\n",
			      __func__, order, bank, type, tscpu_bank_ts[bank][type],
			      tscpu_bank_ts_r[bank][type]);
		break;
	default:
		tscpu_bank_ts[bank][type] =
		    read_tc_raw_and_temp((volatile u32 *)TEMPMSR0, type,
					 &tscpu_bank_ts_r[bank][type]);
		tscpu_dprintk("%s order %d bank %d type %d tscpu_bank_ts %d tscpu_bank_ts_r %d\n",
			      __func__, order, bank, type, tscpu_bank_ts[bank][type],
			      tscpu_bank_ts_r[bank][type]);
		break;
	}
}

int tscpu_thermal_fast_init(void)
{
	UINT32 temp = 0;
	UINT32 cunt = 0;
	/* UINT32 temp1 = 0,temp2 = 0,temp3 = 0,count=0; */

	/* tscpu_printk( "tscpu_thermal_fast_init\n"); */


	temp = 0xDA1;
	DRV_WriteReg32(PTPSPARE2, (0x00001000 + temp));	/* write temp to spare register */

	DRV_WriteReg32(TEMPMONCTL1, 1);	/* counting unit is 320 * 31.25us = 10ms */
	DRV_WriteReg32(TEMPMONCTL2, 1);	/* sensing interval is 200 * 10ms = 2000ms */
	DRV_WriteReg32(TEMPAHBPOLL, 1);	/* polling interval to check if temperature sense is ready */

	DRV_WriteReg32(TEMPAHBTO, 0x000000FF);	/* exceed this polling time, IRQ would be inserted */
	DRV_WriteReg32(TEMPMONIDET0, 0x00000000);	/* times for interrupt occurrance */
	DRV_WriteReg32(TEMPMONIDET1, 0x00000000);	/* times for interrupt occurrance */

	DRV_WriteReg32(TEMPMSRCTL0, 0x0000000);	/* temperature measurement sampling control */
	/* this value will be stored to TEMPPNPMUXADDR (TEMPSPARE0) automatically by hw */
	DRV_WriteReg32(TEMPADCPNP0, 0x1);
	DRV_WriteReg32(TEMPADCPNP1, 0x2);
	DRV_WriteReg32(TEMPADCPNP2, 0x3);
	DRV_WriteReg32(TEMPADCPNP3, 0x4);

#if 0
	DRV_WriteReg32(TEMPPNPMUXADDR, 0x1100B420);	/* AHB address for pnp sensor mux selection */
	DRV_WriteReg32(TEMPADCMUXADDR, 0x1100B420);	/* AHB address for auxadc mux selection */
	DRV_WriteReg32(TEMPADCENADDR, 0x1100B424);	/* AHB address for auxadc enable */
	DRV_WriteReg32(TEMPADCVALIDADDR, 0x1100B428);	/* AHB address for auxadc valid bit */
	DRV_WriteReg32(TEMPADCVOLTADDR, 0x1100B428);	/* AHB address for auxadc voltage output */

#else
	DRV_WriteReg32(TEMPPNPMUXADDR, (UINT32) PTPSPARE0_P);	/* AHB address for pnp sensor mux selection */
	DRV_WriteReg32(TEMPADCMUXADDR, (UINT32) PTPSPARE0_P);	/* AHB address for auxadc mux selection */
	DRV_WriteReg32(TEMPADCENADDR, (UINT32) PTPSPARE1_P);	/* AHB address for auxadc enable */
	DRV_WriteReg32(TEMPADCVALIDADDR, (UINT32) PTPSPARE2_P);	/* AHB address for auxadc valid bit */
	DRV_WriteReg32(TEMPADCVOLTADDR, (UINT32) PTPSPARE2_P);	/* AHB address for auxadc voltage output */
#endif
	DRV_WriteReg32(TEMPRDCTRL, 0x0);	/* read valid & voltage are at the same register */
	/* indicate where the valid bit is (the 12th bit is valid bit and 1 is valid) */
	DRV_WriteReg32(TEMPADCVALIDMASK, 0x0000002C);
	DRV_WriteReg32(TEMPADCVOLTAGESHIFT, 0x0);	/* do not need to shift */
	DRV_WriteReg32(TEMPADCWRITECTRL, 0x3);	/* enable auxadc mux & pnp write transaction */

	/* enable all interrupt except filter sense and immediate sense interrupt */
	DRV_WriteReg32(TEMPMONINT, 0x00000000);


	DRV_WriteReg32(TEMPMONCTL0, 0x0000000F);	/* enable all sensing point (sensing point 2 is unused) */


	cunt = 0;
	temp = DRV_Reg32(TEMPMSR0) & 0x0fff;
	while (temp != 0xDA1 && cunt < 20) {
		cunt++;
		/* pr_debug("[Power/CPU_Thermal]0 temp=%d,cunt=%d\n",temp,cunt); */
		temp = DRV_Reg32(TEMPMSR0) & 0x0fff;
	}

	cunt = 0;
	temp = DRV_Reg32(TEMPMSR1) & 0x0fff;
	while (temp != 0xDA1 && cunt < 20) {
		cunt++;
		/* pr_debug("[Power/CPU_Thermal]1 temp=%d,cunt=%d\n",temp,cunt); */
		temp = DRV_Reg32(TEMPMSR1) & 0x0fff;
	}

	cunt = 0;
	temp = DRV_Reg32(TEMPMSR2) & 0x0fff;
	while (temp != 0xDA1 && cunt < 20) {
		cunt++;
		/* pr_debug("[Power/CPU_Thermal]2 temp=%d,cunt=%d\n",temp,cunt); */
		temp = DRV_Reg32(TEMPMSR2) & 0x0fff;
	}

	cunt = 0;
	temp = DRV_Reg32(TEMPMSR3) & 0x0fff;
	while (temp != 0xDA1 && cunt < 20) {
		cunt++;
		/* pr_debug("[Power/CPU_Thermal]3 temp=%d,cunt=%d\n",temp,cunt); */
		temp = DRV_Reg32(TEMPMSR3) & 0x0fff;
	}




	return 0;
}

int tscpu_switch_bank(thermal_bank_name bank)
{
	/* tscpu_dprintk( "tscpu_switch_bank =bank=%d\n",bank); */

	switch (bank) {
	case THERMAL_BANK0:	/* CPU (TSMCU2) */
		thermal_clrl(PTPCORESEL , 0xF);	/* bank0 */
		break;
	case THERMAL_BANK1:	/* GPU (TSMCU1) */
		thermal_clrl(PTPCORESEL, 0xF);
		thermal_setl(PTPCORESEL, 0x1);	/* bank1 */
		break;
	case THERMAL_BANK2:	/* LTE (TSMCU3) */
		thermal_clrl(PTPCORESEL, 0xF);
		thermal_setl(PTPCORESEL, 0x2);	/* bank2 */
		break;
	default:
		thermal_clrl(PTPCORESEL, 0xF);	/* bank0 */
		break;
	}
	return 0;
}

void tscpu_thermal_initial_all_bank(void)
{
	unsigned long flags;

	mt_ptp_lock(&flags);

	thermal_config_Bank();

	mt_ptp_unlock(&flags);
}

void tscpu_config_all_tc_hw_protect(int temperature, int temperature2)
{
	int i = 0;
	int wd_api_ret;
	unsigned long flags;
	struct wd_api *wd_api;

	tscpu_dprintk("tscpu_config_all_tc_hw_protect,temperature=%d,temperature2=%d,\n",
		      temperature, temperature2);

#if THERMAL_PERFORMANCE_PROFILE
	struct timeval begin, end;
	unsigned long val;

	do_gettimeofday(&begin);
#endif
	/*spend 860~1463 us */
	/*Thermal need to config to direct reset mode
	   this API provide by Weiqi Fu(RGU SW owner). */

	wd_api_ret = get_wd_api(&wd_api);
	if (wd_api_ret >= 0) {
		wd_api->wd_thermal_direct_mode_config(WD_REQ_DIS, WD_REQ_RST_MODE);	/* reset mode */
	} else {
		tscpu_printk("%d FAILED TO GET WD API\n", __LINE__);
		BUG();
	}

#if THERMAL_PERFORMANCE_PROFILE
	do_gettimeofday(&end);

	/* Get milliseconds */
	pr_debug("resume time spent, sec : %lu , usec : %lu\n", (end.tv_sec - begin.tv_sec),
	       (end.tv_usec - begin.tv_usec));
#endif

	mt_ptp_lock(&flags);

	for (i = 0; i < TS_LEN_ARRAY(tscpu_g_bank); i++) {
		tscpu_switch_bank(i);

		set_tc_trigger_hw_protect(temperature, temperature2);	/* Move thermal HW protection ahead... */
	}

	mt_ptp_unlock(&flags);


	/*Thermal need to config to direct reset mode
	   this API provide by Weiqi Fu(RGU SW owner). */
	wd_api->wd_thermal_direct_mode_config(WD_REQ_EN, WD_REQ_RST_MODE);	/* reset mode */

}

void tscpu_reset_thermal(void)
{
	int temp = 0;
	/* reset thremal ctrl */
	temp = DRV_Reg32(INFRA_GLOBALCON_RST_0_SET);
	temp |= 0x00000001;	/* 1: Enables thermal control software reset */
	THERMAL_WRAP_WR32(temp, INFRA_GLOBALCON_RST_0_SET);

	/* un reset */
	temp = DRV_Reg32(INFRA_GLOBALCON_RST_0_CLR);
	temp |= 0x00000001;	/* 1: Enable reset Disables thermal control software reset */
	THERMAL_WRAP_WR32(temp, INFRA_GLOBALCON_RST_0_CLR);
}

int tscpu_read_temperature_info(struct seq_file *m, void *v)
{


	seq_printf(m, "current temp:%d\n", tscpu_read_curr_temp);
	seq_printf(m, "calefuse1:0x%x\n", calefuse1);
	seq_printf(m, "calefuse2:0x%x\n", calefuse2);
	seq_printf(m, "calefuse3:0x%x\n", calefuse3);
	seq_printf(m, "g_adc_ge_t:%d\n", g_adc_ge_t);
	seq_printf(m, "g_adc_oe_t:%d\n", g_adc_oe_t);
	seq_printf(m, "g_degc_cali:%d\n", g_degc_cali);
	seq_printf(m, "g_adc_cali_en_t:%d\n", g_adc_cali_en_t);
	seq_printf(m, "g_o_slope:%d\n", g_o_slope);
	seq_printf(m, "g_o_slope_sign:%d\n", g_o_slope_sign);
	seq_printf(m, "g_id:%d\n", g_id);
	seq_printf(m, "g_o_vtsmcu1:%d\n", g_o_vtsmcu1);
	seq_printf(m, "g_o_vtsmcu2:%d\n", g_o_vtsmcu2);
	seq_printf(m, "g_o_vtsmcu3:%d\n", g_o_vtsmcu3);

	return 0;
}

int tscpu_get_curr_temp(void)
{
	tscpu_update_tempinfo();
	return tscpu_max_temperature();
}

#ifdef CONFIG_OF
int get_io_reg_base(void)
{
	struct device_node *node = NULL;

	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6735-therm_ctrl");
	BUG_ON(node == 0);
	if (node) {
		/* Setup IO addresses */
		thermal_base = of_iomap(node, 0);
		/* pr_debug("[THERM_CTRL] thermal_base=0x%p\n",thermal_base); */
	}

	/*get thermal irq num */
	thermal_irq_number = irq_of_parse_and_map(node, 0);
	pr_debug("[THERM_CTRL] thermal_irq_number=%d\n", thermal_irq_number);
	if (!thermal_irq_number) {
		pr_debug("[THERM_CTRL] get irqnr failed=%d\n", thermal_irq_number);
		return 0;
	}

	of_property_read_u32(node, "reg", &thermal_phy_base);
	/* pr_debug("[THERM_CTRL] thermal_base thermal_phy_base=0x%x\n",thermal_phy_base); */

	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6735-auxadc");
	BUG_ON(node == 0);
	if (node) {
		/* Setup IO addresses */
		auxadc_ts_base = of_iomap(node, 0);
		/* pr_debug("[THERM_CTRL] auxadc_ts_base=0x%p\n",auxadc_ts_base); */
	}
	of_property_read_u32(node, "reg", &auxadc_ts_phy_base);
	/* pr_debug("[THERM_CTRL] auxadc_ts_phy_base=0x%x\n",auxadc_ts_phy_base); */

	node = of_find_compatible_node(NULL, NULL, "mediatek,INFRACFG_AO");
	BUG_ON(node == 0);
	if (node) {
		/* Setup IO addresses */
		infracfg_ao_base = of_iomap(node, 0);
		/* pr_debug("[THERM_CTRL] infracfg_ao_base=0x%p\n",infracfg_ao_base); */
	}


	node = of_find_compatible_node(NULL, NULL, "mediatek,APMIXED");
	BUG_ON(node == 0);
	if (node) {
		/* Setup IO addresses */
		apmixed_base = of_iomap(node, 0);
		/* pr_debug("[THERM_CTRL] apmixed_base=0x%p\n",apmixed_base); */
	}
	of_property_read_u32(node, "reg", &apmixed_phy_base);
	/* pr_debug("[THERM_CTRL] apmixed_phy_base=0x%x\n",apmixed_phy_base); */

	return 1;
}
#endif
