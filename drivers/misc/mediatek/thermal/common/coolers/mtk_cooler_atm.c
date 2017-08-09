#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/thermal.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include "mt-plat/mtk_thermal_monitor.h"
#include "mach/mt_typedefs.h"
#include "mach/mt_thermal.h"
#include "mt_gpufreq.h"
#include <mach/mt_clkmgr.h>
#include <mt_spm.h>
#include <mt_ptp.h>
#include <mach/wd_api.h>
#include <linux/slab.h>
#include <linux/seq_file.h>
#include <tscpu_settings.h>
#include <mt-plat/aee.h>
#include <linux/uidgid.h>

#if defined(CONFIG_ARCH_MT6755)
#include "mach/mt_ppm_api.h"
#else
#include "mt_cpufreq.h"
#endif
/*=============================================================
 *Local variable definition
 *=============================================================*/
static kuid_t uid = KUIDT_INIT(0);
static kgid_t gid = KGIDT_INIT(1000);
unsigned int adaptive_cpu_power_limit = 0x7FFFFFFF;
unsigned int adaptive_gpu_power_limit = 0x7FFFFFFF;
static unsigned int prv_adp_cpu_pwr_lim;

#if CPT_ADAPTIVE_AP_COOLER
static int TARGET_TJ = 65000;
static int cpu_target_tj = 65000;
static int cpu_target_offset = 10000;
static int TARGET_TJ_HIGH = 66000;
static int TARGET_TJ_LOW = 64000;
static int PACKAGE_THETA_JA_RISE = 10;
static int PACKAGE_THETA_JA_FALL = 10;
static int MINIMUM_CPU_POWER = 500;
static int MAXIMUM_CPU_POWER = 1240;
static int MINIMUM_GPU_POWER = 676;
static int MAXIMUM_GPU_POWER = 676;
static int MINIMUM_TOTAL_POWER = 500 + 676;
static int MAXIMUM_TOTAL_POWER = 1240 + 676;
static int FIRST_STEP_TOTAL_POWER_BUDGET = 1750;

/* 1. MINIMUM_BUDGET_CHANGE = 0 ==> thermal equilibrium maybe at higher than TARGET_TJ_HIGH */
/* 2. Set MINIMUM_BUDGET_CHANGE > 0 if to keep Tj at TARGET_TJ */
static int MINIMUM_BUDGET_CHANGE = 50;
static int g_total_power;
#endif

#if CPT_ADAPTIVE_AP_COOLER
static struct thermal_cooling_device *cl_dev_adp_cpu[MAX_CPT_ADAPTIVE_COOLERS] = { NULL };
static unsigned int cl_dev_adp_cpu_state[MAX_CPT_ADAPTIVE_COOLERS] = { 0 };
int TARGET_TJS[MAX_CPT_ADAPTIVE_COOLERS] = { 85000, 0 };

static unsigned int cl_dev_adp_cpu_state_active;
#endif				/* end of CPT_ADAPTIVE_AP_COOLER */

#if CPT_ADAPTIVE_AP_COOLER
char *adaptive_cooler_name = "cpu_adaptive_";
static int FIRST_STEP_TOTAL_POWER_BUDGETS[MAX_CPT_ADAPTIVE_COOLERS] = { 3300, 0 };
static int PACKAGE_THETA_JA_RISES[MAX_CPT_ADAPTIVE_COOLERS] = { 35, 0 };
static int PACKAGE_THETA_JA_FALLS[MAX_CPT_ADAPTIVE_COOLERS] = { 25, 0 };
static int MINIMUM_BUDGET_CHANGES[MAX_CPT_ADAPTIVE_COOLERS] = { 50, 0 };
static int MINIMUM_CPU_POWERS[MAX_CPT_ADAPTIVE_COOLERS] = { 1200, 0 };
static int MAXIMUM_CPU_POWERS[MAX_CPT_ADAPTIVE_COOLERS] = { 4400, 0 };
static int MINIMUM_GPU_POWERS[MAX_CPT_ADAPTIVE_COOLERS] = { 350, 0 };
static int MAXIMUM_GPU_POWERS[MAX_CPT_ADAPTIVE_COOLERS] = { 960, 0 };

static int active_adp_cooler;

static int GPU_L_H_TRIP = 80, GPU_L_L_TRIP = 40;

/* +ASC+ */
/*0: default:  ATM v1 */
/*1:	 FTL ATM v2 */
/*2:	 CPU_GPU_Weight ATM v2 */
static int tscpu_atm = 1;
static int tt_ratio_high_rise = 1;
static int tt_ratio_high_fall = 1;
static int tt_ratio_low_rise = 1;
static int tt_ratio_low_fall = 1;
static int tp_ratio_high_rise = 1;
static int tp_ratio_high_fall;
static int tp_ratio_low_rise;
static int tp_ratio_low_fall;
/* static int cpu_loading = 0; */
/* -ASC- */

#if THERMAL_HEADROOM
static int p_Tpcb_correlation;
static int Tpcb_trip_point;
static int thp_max_cpu_power;
static int thp_p_tj_correlation;
static int thp_threshold_tj;
#endif

#if CONTINUOUS_TM
static int ctm_on;		/* 1: on, 0: off */
static int MAX_TARGET_TJ = -1;
static int STEADY_TARGET_TJ = -1;
static int TRIP_TPCB = -1;
static int STEADY_TARGET_TPCB = -1;
static int MAX_EXIT_TJ = -1;
static int STEADY_EXIT_TJ = -1;
static int COEF_AE = -1;
static int COEF_BE = -1;
static int COEF_AX = -1;
static int COEF_BX = -1;
/* static int current_TTJ = -1; */
static int current_ETJ = -1;
#endif
#endif				/* end of CPT_ADAPTIVE_AP_COOLER */
/*=============================================================
 *Local function prototype
 *=============================================================*/
static void set_adaptive_gpu_power_limit(unsigned int limit);
/*=============================================================
 *Weak functions
 *=============================================================*/
#if defined(CONFIG_ARCH_MT6755)
void __attribute__ ((weak))
mt_ppm_cpu_thermal_protect(unsigned int limited_power)
{
	pr_err("E_WF: %s doesn't exist\n", __func__);
}
#else
void __attribute__ ((weak))
mt_cpufreq_thermal_protect(unsigned int limited_power)
{
	pr_err("E_WF: %s doesn't exist\n", __func__);
}
#endif

bool __attribute__((weak))
mtk_get_gpu_loading(unsigned int *pLoading)
{
	pr_err("E_WF: %s doesn't exist\n", __func__);
	return 0;
}
unsigned int  __attribute__((weak))
mt_gpufreq_get_min_power(void)
{
	pr_err("E_WF: %s doesn't exist\n", __func__);
	return 0;
}
unsigned int  __attribute__((weak))
mt_gpufreq_get_max_power(void)
{
	pr_err("E_WF: %s doesn't exist\n", __func__);
	return 0;
}
/*=============================================================*/

/*
   static int step0_mask[11] = {1,1,1,1,1,1,1,1,1,1,1};
   static int step1_mask[11] = {0,1,1,1,1,1,1,1,1,1,1};
   static int step2_mask[11] = {0,0,1,1,1,1,1,1,1,1,1};
   static int step3_mask[11] = {0,0,0,1,1,1,1,1,1,1,1};
   static int step4_mask[11] = {0,0,0,0,1,1,1,1,1,1,1};
   static int step5_mask[11] = {0,0,0,0,0,1,1,1,1,1,1};
   static int step6_mask[11] = {0,0,0,0,0,0,1,1,1,1,1};
   static int step7_mask[11] = {0,0,0,0,0,0,0,1,1,1,1};
   static int step8_mask[11] = {0,0,0,0,0,0,0,0,1,1,1};
   static int step9_mask[11] = {0,0,0,0,0,0,0,0,0,1,1};
   static int step10_mask[11]= {0,0,0,0,0,0,0,0,0,0,1};
 */

static void set_adaptive_cpu_power_limit(unsigned int limit)
{
	unsigned int final_limit;

	prv_adp_cpu_pwr_lim = adaptive_cpu_power_limit;
	adaptive_cpu_power_limit = (limit != 0) ? limit : 0x7FFFFFFF;
	final_limit = MIN(adaptive_cpu_power_limit, static_cpu_power_limit);

	if (prv_adp_cpu_pwr_lim != adaptive_cpu_power_limit) {
		tscpu_printk("set_adaptive_cpu_power_limit %d, T=",
			     (final_limit != 0x7FFFFFFF) ? final_limit : 0);
		tscpu_print_all_temperature(0);

	#if defined(CONFIG_ARCH_MT6755)
		mt_ppm_cpu_thermal_protect((final_limit != 0x7FFFFFFF) ? final_limit : 0);
	#else
		mt_cpufreq_thermal_protect((final_limit != 0x7FFFFFFF) ? final_limit : 0);
	#endif
	}
}

static void set_adaptive_gpu_power_limit(unsigned int limit)
{
	unsigned int final_limit;

	adaptive_gpu_power_limit = (limit != 0) ? limit : 0x7FFFFFFF;
	final_limit = MIN(adaptive_gpu_power_limit, static_gpu_power_limit);
	tscpu_printk("set_adaptive_gpu_power_limit %d\n",
		     (final_limit != 0x7FFFFFFF) ? final_limit : 0);
	mt_gpufreq_thermal_protect((final_limit != 0x7FFFFFFF) ? final_limit : 0);
}

#if CPT_ADAPTIVE_AP_COOLER
int is_cpu_power_unlimit(void)
{
	return (g_total_power == 0 || g_total_power >= MAXIMUM_TOTAL_POWER) ? 1 : 0;
}
EXPORT_SYMBOL(is_cpu_power_unlimit);

int is_cpu_power_min(void)
{
	return (g_total_power <= MINIMUM_TOTAL_POWER) ? 1 : 0;
}
EXPORT_SYMBOL(is_cpu_power_min);

int get_cpu_target_tj(void)
{
	return cpu_target_tj;
}
EXPORT_SYMBOL(get_cpu_target_tj);

int get_cpu_target_offset(void)
{
	return cpu_target_offset;
}
EXPORT_SYMBOL(get_cpu_target_offset);

/*add for DLPT*/
int tscpu_get_min_cpu_pwr(void)
{
	return MINIMUM_CPU_POWER;
}
EXPORT_SYMBOL(tscpu_get_min_cpu_pwr);

int tscpu_get_min_gpu_pwr(void)
{
	return MINIMUM_GPU_POWER;
}
EXPORT_SYMBOL(tscpu_get_min_gpu_pwr);

static int P_adaptive(int total_power, unsigned int gpu_loading)
{
	/*
	Here the gpu_power is the gpu power limit for the next interval,
	not exactly what gpu power selected by GPU DVFS
	But the ground rule is real gpu power should always under gpu_power for the same time interval
	*/
	static int cpu_power = 0, gpu_power;
	static int last_cpu_power = 0, last_gpu_power;

	last_cpu_power = cpu_power;
	last_gpu_power = gpu_power;
	g_total_power = total_power;

	if (total_power == 0) {
		cpu_power = gpu_power = 0;
#if THERMAL_HEADROOM
		if (thp_max_cpu_power != 0)
			set_adaptive_cpu_power_limit((unsigned int)
						     MAX(thp_max_cpu_power, MINIMUM_CPU_POWER));
		else
			set_adaptive_cpu_power_limit(0);
#else
		set_adaptive_cpu_power_limit(0);
#endif
		set_adaptive_gpu_power_limit(0);
		return 0;
	}

	if (total_power <= MINIMUM_TOTAL_POWER) {
		cpu_power = MINIMUM_CPU_POWER;
		gpu_power = MINIMUM_GPU_POWER;
	} else if (total_power >= MAXIMUM_TOTAL_POWER) {
		cpu_power = MAXIMUM_CPU_POWER;
		gpu_power = MAXIMUM_GPU_POWER;
	} else {
		int max_allowed_gpu_power =
		    MIN((total_power - MINIMUM_CPU_POWER), MAXIMUM_GPU_POWER);
		int highest_possible_gpu_power = -1;
		/* int highest_possible_gpu_power_idx = 0; */
		int i = 0;

		unsigned int cur_gpu_freq = mt_gpufreq_get_cur_freq();
		/* int cur_idx = 0; */
		unsigned int cur_gpu_power = 0;
		unsigned int next_lower_gpu_power = 0;

		/* get GPU highest possible power and index and current power and index and next lower power */
		for (; i < Num_of_GPU_OPP; i++) {
			if ((mtk_gpu_power[i].gpufreq_power <= max_allowed_gpu_power) &&
			    (-1 == highest_possible_gpu_power)) {
				highest_possible_gpu_power = mtk_gpu_power[i].gpufreq_power;
				/* highest_possible_gpu_power_idx = i; */
			}

			if (mtk_gpu_power[i].gpufreq_khz == cur_gpu_freq) {
				next_lower_gpu_power = cur_gpu_power =
				    mtk_gpu_power[i].gpufreq_power;
				/* cur_idx = i; */

				if ((i != Num_of_GPU_OPP - 1)
				    && (mtk_gpu_power[i + 1].gpufreq_power >= MINIMUM_GPU_POWER)) {
					next_lower_gpu_power = mtk_gpu_power[i + 1].gpufreq_power;
				}
			}
		}

		/* decide GPU power limit by loading */
		if (gpu_loading > GPU_L_H_TRIP) {
			gpu_power = highest_possible_gpu_power;
		} else if (gpu_loading <= GPU_L_L_TRIP) {
			gpu_power = MIN(next_lower_gpu_power, highest_possible_gpu_power);
			gpu_power = MAX(gpu_power, MINIMUM_GPU_POWER);
		} else {
			gpu_power = MIN(highest_possible_gpu_power, cur_gpu_power);
		}

		cpu_power = MIN((total_power - gpu_power), MAXIMUM_CPU_POWER);
	}

	if (cpu_power != last_cpu_power)
		set_adaptive_cpu_power_limit(cpu_power);

	if (gpu_power != last_gpu_power)
		set_adaptive_gpu_power_limit(gpu_power);

	tscpu_dprintk("%s cpu %d, gpu %d\n", __func__, cpu_power, gpu_power);

	return 0;
}

static int _adaptive_power(long prev_temp, long curr_temp, unsigned int gpu_loading)
{
	static int triggered = 0, total_power;
	int delta_power = 0;

	if (cl_dev_adp_cpu_state_active == 1) {
		tscpu_dprintk("%s %d %d %d %d %d %d %d %d\n", __func__,
			      FIRST_STEP_TOTAL_POWER_BUDGET, PACKAGE_THETA_JA_RISE,
			      PACKAGE_THETA_JA_FALL, MINIMUM_BUDGET_CHANGE, MINIMUM_CPU_POWER,
			      MAXIMUM_CPU_POWER, MINIMUM_GPU_POWER, MAXIMUM_GPU_POWER);

		/* Check if it is triggered */
		if (!triggered) {
			if (curr_temp < TARGET_TJ)
				return 0;

			triggered = 1;
			switch (tscpu_atm) {
			case 1:	/* FTL ATM v2 */
			case 2:	/* CPU_GPU_Weight ATM v2 */
#if MTKTSCPU_FAST_POLLING
				total_power =
					FIRST_STEP_TOTAL_POWER_BUDGET -
					((curr_temp - TARGET_TJ) * tt_ratio_high_rise +
					(curr_temp - prev_temp) * tp_ratio_high_rise) /
					(PACKAGE_THETA_JA_RISE * tscpu_cur_fp_factor);
#else
				total_power =
					FIRST_STEP_TOTAL_POWER_BUDGET -
					((curr_temp - TARGET_TJ) * tt_ratio_high_rise +
					(curr_temp - prev_temp) * tp_ratio_high_rise) /
					PACKAGE_THETA_JA_RISE;
#endif
				break;
			case 0:
			default:	/* ATM v1 */
				total_power = FIRST_STEP_TOTAL_POWER_BUDGET;
			}
			tscpu_dprintk("%s Tp %ld, Tc %ld, Pt %d\n", __func__, prev_temp, curr_temp, total_power);
			return P_adaptive(total_power, gpu_loading);
		}

		/* Adjust total power budget if necessary */
		switch (tscpu_atm) {
		case 1:	/* FTL ATM v2 */
		case 2:	/* CPU_GPU_Weight ATM v2 */
			if ((curr_temp >= TARGET_TJ_HIGH) && (curr_temp > prev_temp)) {
#if MTKTSCPU_FAST_POLLING
				total_power -=
				    MAX(((curr_temp - TARGET_TJ) * tt_ratio_high_rise +
					 (curr_temp -
					  prev_temp) * tp_ratio_high_rise) /
					(PACKAGE_THETA_JA_RISE * tscpu_cur_fp_factor),
					MINIMUM_BUDGET_CHANGE);
#else
				total_power -=
				    MAX(((curr_temp - TARGET_TJ) * tt_ratio_high_rise +
					 (curr_temp -
					  prev_temp) * tp_ratio_high_rise) / PACKAGE_THETA_JA_RISE,
					MINIMUM_BUDGET_CHANGE);
#endif
			} else if ((curr_temp >= TARGET_TJ_HIGH) && (curr_temp <= prev_temp)) {
#if MTKTSCPU_FAST_POLLING
				total_power -=
				    MAX(((curr_temp - TARGET_TJ) * tt_ratio_high_fall -
					 (prev_temp -
					  curr_temp) * tp_ratio_high_fall) /
					(PACKAGE_THETA_JA_FALL * tscpu_cur_fp_factor),
					MINIMUM_BUDGET_CHANGE);
#else
				total_power -=
				    MAX(((curr_temp - TARGET_TJ) * tt_ratio_high_fall -
					 (prev_temp -
					  curr_temp) * tp_ratio_high_fall) / PACKAGE_THETA_JA_FALL,
					MINIMUM_BUDGET_CHANGE);
#endif
			} else if ((curr_temp <= TARGET_TJ_LOW) && (curr_temp > prev_temp)) {
#if MTKTSCPU_FAST_POLLING
				total_power +=
				    MAX(((TARGET_TJ - curr_temp) * tt_ratio_low_rise -
					 (curr_temp -
					  prev_temp) * tp_ratio_low_rise) / (PACKAGE_THETA_JA_RISE *
									     tscpu_cur_fp_factor),
					MINIMUM_BUDGET_CHANGE);
#else
				total_power +=
				    MAX(((TARGET_TJ - curr_temp) * tt_ratio_low_rise -
					 (curr_temp -
					  prev_temp) * tp_ratio_low_rise) / PACKAGE_THETA_JA_RISE,
					MINIMUM_BUDGET_CHANGE);
#endif
			} else if ((curr_temp <= TARGET_TJ_LOW) && (curr_temp <= prev_temp)) {
#if MTKTSCPU_FAST_POLLING
				total_power +=
				    MAX(((TARGET_TJ - curr_temp) * tt_ratio_low_fall +
					 (prev_temp -
					  curr_temp) * tp_ratio_low_fall) / (PACKAGE_THETA_JA_FALL *
									     tscpu_cur_fp_factor),
					MINIMUM_BUDGET_CHANGE);
#else
				total_power +=
				    MAX(((TARGET_TJ - curr_temp) * tt_ratio_low_fall +
					 (prev_temp -
					  curr_temp) * tp_ratio_low_fall) / PACKAGE_THETA_JA_FALL,
					MINIMUM_BUDGET_CHANGE);
#endif
			}

			total_power =
			    (total_power > MINIMUM_TOTAL_POWER) ? total_power : MINIMUM_TOTAL_POWER;
			total_power =
			    (total_power < MAXIMUM_TOTAL_POWER) ? total_power : MAXIMUM_TOTAL_POWER;
			break;

		case 0:
		default:	/* ATM v1 */
			if ((curr_temp > TARGET_TJ_HIGH) && (curr_temp >= prev_temp)) {
#if MTKTSCPU_FAST_POLLING
				delta_power =
				    (curr_temp -
				     prev_temp) / (PACKAGE_THETA_JA_RISE * tscpu_cur_fp_factor);
#else
				delta_power = (curr_temp - prev_temp) / PACKAGE_THETA_JA_RISE;
#endif
				if (prev_temp > TARGET_TJ_HIGH) {
					delta_power =
					    (delta_power >
					     MINIMUM_BUDGET_CHANGE) ? delta_power :
					    MINIMUM_BUDGET_CHANGE;
				}
				total_power -= delta_power;
				total_power =
				    (total_power >
				     MINIMUM_TOTAL_POWER) ? total_power : MINIMUM_TOTAL_POWER;
			}

			if ((curr_temp < TARGET_TJ_LOW) && (curr_temp <= prev_temp)) {
#if MTKTSCPU_FAST_POLLING
				delta_power =
				    (prev_temp -
				     curr_temp) / (PACKAGE_THETA_JA_FALL * tscpu_cur_fp_factor);
#else
				delta_power = (prev_temp - curr_temp) / PACKAGE_THETA_JA_FALL;
#endif
				if (prev_temp < TARGET_TJ_LOW) {
					delta_power =
					    (delta_power >
					     MINIMUM_BUDGET_CHANGE) ? delta_power :
					    MINIMUM_BUDGET_CHANGE;
				}
				total_power += delta_power;
				total_power =
				    (total_power <
				     MAXIMUM_TOTAL_POWER) ? total_power : MAXIMUM_TOTAL_POWER;
			}
			break;
		}

		tscpu_dprintk("%s Tp %ld, Tc %ld, Pt %d\n", __func__, prev_temp, curr_temp,
			      total_power);
		return P_adaptive(total_power, gpu_loading);
	}
#if CONTINUOUS_TM
	else if ((cl_dev_adp_cpu_state_active == 1) && (ctm_on == 1) && (curr_temp < current_ETJ)) {
		tscpu_printk("CTM exit curr_temp %d cetj %d\n", TARGET_TJ, current_ETJ);
		/* even cooler not exit, when CTM is on and current Tj < current_ETJ, leave ATM */
		if (triggered) {
			triggered = 0;
			tscpu_dprintk("%s Tp %ld, Tc %ld, Pt %d\n", __func__, prev_temp, curr_temp,
				      total_power);
			return P_adaptive(0, 0);
		}
#if THERMAL_HEADROOM
		else {
			if (thp_max_cpu_power != 0)
				set_adaptive_cpu_power_limit((unsigned int)
							     MAX(thp_max_cpu_power,
								 MINIMUM_CPU_POWER));
			else
				set_adaptive_cpu_power_limit(0);
		}
#endif
	}
#endif
	else {
		if (triggered) {
			triggered = 0;
			tscpu_dprintk("%s Tp %ld, Tc %ld, Pt %d\n", __func__, prev_temp, curr_temp,
				      total_power);
			return P_adaptive(0, 0);
		}
#if THERMAL_HEADROOM
		else {
			if (thp_max_cpu_power != 0)
				set_adaptive_cpu_power_limit((unsigned int)
							     MAX(thp_max_cpu_power,
								 MINIMUM_CPU_POWER));
			else
				set_adaptive_cpu_power_limit(0);
		}
#endif
	}

	return 0;
}

static int decide_ttj(void)
{
	int i = 0;
	int active_cooler_id = -1;
	int ret = 117000;	/* highest allowable TJ */
	int temp_cl_dev_adp_cpu_state_active = 0;

	for (; i < MAX_CPT_ADAPTIVE_COOLERS; i++) {
		if (cl_dev_adp_cpu_state[i]) {
			ret = MIN(ret, TARGET_TJS[i]);
			temp_cl_dev_adp_cpu_state_active = 1;

			if (ret == TARGET_TJS[i])
				active_cooler_id = i;
		}
	}
	cl_dev_adp_cpu_state_active = temp_cl_dev_adp_cpu_state_active;
	TARGET_TJ = ret;
#if CONTINUOUS_TM
	if (ctm_on == 1) {
		int curr_tpcb = mtk_thermal_get_temp(MTK_THERMAL_SENSOR_AP);

		TARGET_TJ =
		    MIN(MAX_TARGET_TJ,
			MAX(STEADY_TARGET_TJ, (COEF_AE - COEF_BE * curr_tpcb / 1000)));
		current_ETJ =
		    MIN(MAX_EXIT_TJ, MAX(STEADY_EXIT_TJ, (COEF_AX - COEF_BX * curr_tpcb / 1000)));
		/* tscpu_printk("cttj %d cetj %d tpcb %d\n", TARGET_TJ, current_ETJ, bts_cur_temp); */
	}
#endif
	cpu_target_tj = TARGET_TJ;
#if CONTINUOUS_TM
	cpu_target_offset = TARGET_TJ - current_ETJ;
#endif

	TARGET_TJ_HIGH = TARGET_TJ + 1000;
	TARGET_TJ_LOW = TARGET_TJ - 1000;

	if (0 <= active_cooler_id && MAX_CPT_ADAPTIVE_COOLERS > active_cooler_id) {
		PACKAGE_THETA_JA_RISE = PACKAGE_THETA_JA_RISES[active_cooler_id];
		PACKAGE_THETA_JA_FALL = PACKAGE_THETA_JA_FALLS[active_cooler_id];
		MINIMUM_CPU_POWER = MINIMUM_CPU_POWERS[active_cooler_id];
		MAXIMUM_CPU_POWER = MAXIMUM_CPU_POWERS[active_cooler_id];

#if DYNAMIC_GET_GPU_POWER
		{
			MAXIMUM_GPU_POWER = (int)mt_gpufreq_get_max_power();
			MINIMUM_GPU_POWER = (int)mt_gpufreq_get_min_power();
			tscpu_printk("decide_ttj: MAXIMUM_GPU_POWER=%d,MINIMUM_GPU_POWER=%d\n",
			       MAXIMUM_GPU_POWER, MINIMUM_GPU_POWER);
		}
#else
		MINIMUM_GPU_POWER = MINIMUM_GPU_POWERS[active_cooler_id];
		MAXIMUM_GPU_POWER = MAXIMUM_GPU_POWERS[active_cooler_id];
#endif
		MINIMUM_TOTAL_POWER = MINIMUM_CPU_POWER + MINIMUM_GPU_POWER;
		MAXIMUM_TOTAL_POWER = MAXIMUM_CPU_POWER + MAXIMUM_GPU_POWER;
		FIRST_STEP_TOTAL_POWER_BUDGET = FIRST_STEP_TOTAL_POWER_BUDGETS[active_cooler_id];
		MINIMUM_BUDGET_CHANGE = MINIMUM_BUDGET_CHANGES[active_cooler_id];
	} else {
		MINIMUM_CPU_POWER = MINIMUM_CPU_POWERS[0];
		MAXIMUM_CPU_POWER = MAXIMUM_CPU_POWERS[0];
	}

#if THERMAL_HEADROOM
	MAXIMUM_CPU_POWER -= p_Tpcb_correlation * MAX((bts_cur_temp - Tpcb_trip_point), 0) / 1000;

	/* tscpu_printk("max_cpu_pwr %d %d\n", bts_cur_temp, MAXIMUM_CPU_POWER); */
	/* TODO: use 0 as current P */
	thp_max_cpu_power = (thp_threshold_tj - tscpu_read_curr_temp) * thp_p_tj_correlation / 1000 + 0;

	if (thp_max_cpu_power != 0)
		MAXIMUM_CPU_POWER = MIN(MAXIMUM_CPU_POWER, thp_max_cpu_power);

	MAXIMUM_CPU_POWER = MAX(MAXIMUM_CPU_POWER, MINIMUM_CPU_POWER);

	/* tscpu_printk("thp max_cpu_pwr %d %d\n", thp_max_cpu_power, MAXIMUM_CPU_POWER); */
#endif

	return ret;
}
#endif

#if CPT_ADAPTIVE_AP_COOLER
static int adp_cpu_get_max_state(struct thermal_cooling_device *cdev, unsigned long *state)
{
	/* tscpu_dprintk("adp_cpu_get_max_state\n"); */
	*state = 1;
	return 0;
}

static int adp_cpu_get_cur_state(struct thermal_cooling_device *cdev, unsigned long *state)
{
	/* tscpu_dprintk("adp_cpu_get_cur_state\n"); */
	*state = cl_dev_adp_cpu_state[(cdev->type[13] - '0')];
	/* *state = cl_dev_adp_cpu_state; */
	return 0;
}

static int adp_cpu_set_cur_state(struct thermal_cooling_device *cdev, unsigned long state)
{
	int ttj = 117000;

	cl_dev_adp_cpu_state[(cdev->type[13] - '0')] = state;

	ttj = decide_ttj();	/* TODO: no exit point can be obtained in mtk_ts_cpu.c */

	/* tscpu_dprintk("adp_cpu_set_cur_state[%d] =%d, ttj=%d\n", (cdev->type[13] - '0'), state, ttj); */

	if (active_adp_cooler == (int)(cdev->type[13] - '0')) {
		/* = (NULL == mtk_thermal_get_gpu_loading_fp) ? 0 : mtk_thermal_get_gpu_loading_fp(); */
		unsigned int gpu_loading;

		if (!mtk_get_gpu_loading(&gpu_loading))
			gpu_loading = 0;
		_adaptive_power(tscpu_g_prev_temp, tscpu_g_curr_temp, (unsigned int)gpu_loading);
		/* _adaptive_power(tscpu_g_prev_temp, tscpu_g_curr_temp, (unsigned int) 0); */
	}
	return 0;
}
#endif

#if CPT_ADAPTIVE_AP_COOLER
static struct thermal_cooling_device_ops mtktscpu_cooler_adp_cpu_ops = {
	.get_max_state = adp_cpu_get_max_state,
	.get_cur_state = adp_cpu_get_cur_state,
	.set_cur_state = adp_cpu_set_cur_state,
};
#endif

#if CPT_ADAPTIVE_AP_COOLER
static int tscpu_read_atm_setting(struct seq_file *m, void *v)
{
	int i;

	for (i = 0; i < MAX_CPT_ADAPTIVE_COOLERS; i++) {
		seq_printf(m, "%s%02d\n", adaptive_cooler_name, i);
		seq_printf(m, " first_step = %d\n", FIRST_STEP_TOTAL_POWER_BUDGETS[i]);
		seq_printf(m, " theta rise = %d\n", PACKAGE_THETA_JA_RISES[i]);
		seq_printf(m, " theta fall = %d\n", PACKAGE_THETA_JA_FALLS[i]);
		seq_printf(m, " min_budget_change = %d\n", MINIMUM_BUDGET_CHANGES[i]);
		seq_printf(m, " m cpu = %d\n", MINIMUM_CPU_POWERS[i]);
		seq_printf(m, " M cpu = %d\n", MAXIMUM_CPU_POWERS[i]);
		seq_printf(m, " m gpu = %d\n", MINIMUM_GPU_POWERS[i]);
		seq_printf(m, " M gpu = %d\n", MAXIMUM_GPU_POWERS[i]);
	}


	return 0;
}

static ssize_t tscpu_write_atm_setting(struct file *file, const char __user *buffer, size_t count,
				       loff_t *data)
{
	char desc[128];
	/* char arg_name[32] = {0}; */
	/* int arg_val = 0; */
	int len = 0;

	int i_id = -1, i_first_step = -1, i_theta_r = -1, i_theta_f = -1, i_budget_change =
	    -1, i_min_cpu_pwr = -1, i_max_cpu_pwr = -1, i_min_gpu_pwr = -1, i_max_gpu_pwr = -1;


	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (9 <= sscanf(desc, "%d %d %d %d %d %d %d %d %d", &i_id, &i_first_step, &i_theta_r, &i_theta_f,
		   &i_budget_change, &i_min_cpu_pwr, &i_max_cpu_pwr, &i_min_gpu_pwr,
		   &i_max_gpu_pwr)) {
		tscpu_printk("tscpu_write_atm_setting input %d %d %d %d %d %d %d %d %d\n", i_id,
			     i_first_step, i_theta_r, i_theta_f, i_budget_change, i_min_cpu_pwr,
			     i_max_cpu_pwr, i_min_gpu_pwr, i_max_gpu_pwr);

		if (i_id >= 0 && i_id < MAX_CPT_ADAPTIVE_COOLERS) {
			if (i_first_step > 0)
				FIRST_STEP_TOTAL_POWER_BUDGETS[i_id] = i_first_step;
			else
				#ifdef CONFIG_MTK_AEE_FEATURE
				aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_DEFAULT,
					"tscpu_write_atm_setting", "Wrong thermal policy");
				#endif

			if (i_theta_r > 0)
				PACKAGE_THETA_JA_RISES[i_id] = i_theta_r;
			else
				#ifdef CONFIG_MTK_AEE_FEATURE
				aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_DEFAULT,
					"tscpu_write_atm_setting", "Wrong thermal policy");
				#endif

			if (i_theta_f > 0)
				PACKAGE_THETA_JA_FALLS[i_id] = i_theta_f;
			else
				#ifdef CONFIG_MTK_AEE_FEATURE
				aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_DEFAULT,
					"tscpu_write_atm_setting", "Wrong thermal policy");
				#endif

			if (i_budget_change > 0)
				MINIMUM_BUDGET_CHANGES[i_id] = i_budget_change;
			else
				#ifdef CONFIG_MTK_AEE_FEATURE
				aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_DEFAULT,
					"tscpu_write_atm_setting", "Wrong thermal policy");
				#endif

			if (i_min_cpu_pwr > 0)
				MINIMUM_CPU_POWERS[i_id] = i_min_cpu_pwr;
			else
				#ifdef CONFIG_MTK_AEE_FEATURE
				aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_DEFAULT,
					"tscpu_write_atm_setting", "Wrong thermal policy");
				#endif

			if (i_max_cpu_pwr > 0)
				MAXIMUM_CPU_POWERS[i_id] = i_max_cpu_pwr;
			else
				#ifdef CONFIG_MTK_AEE_FEATURE
				aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_DEFAULT,
					"tscpu_write_atm_setting", "Wrong thermal policy");
				#endif

			if (i_min_gpu_pwr > 0)
				MINIMUM_GPU_POWERS[i_id] = i_min_gpu_pwr;
			else
				#ifdef CONFIG_MTK_AEE_FEATURE
				aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_DEFAULT,
					"tscpu_write_atm_setting", "Wrong thermal policy");
				#endif

			if (i_max_gpu_pwr > 0)
				MAXIMUM_GPU_POWERS[i_id] = i_max_gpu_pwr;
			else
				#ifdef CONFIG_MTK_AEE_FEATURE
				aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_DEFAULT,
					"tscpu_write_atm_setting", "Wrong thermal policy");
				#endif


			active_adp_cooler = i_id;

			tscpu_printk("tscpu_write_dtm_setting applied %d %d %d %d %d %d %d %d %d\n",
				     i_id, FIRST_STEP_TOTAL_POWER_BUDGETS[i_id],
				     PACKAGE_THETA_JA_RISES[i_id], PACKAGE_THETA_JA_FALLS[i_id],
				     MINIMUM_BUDGET_CHANGES[i_id], MINIMUM_CPU_POWERS[i_id],
				     MAXIMUM_CPU_POWERS[i_id], MINIMUM_GPU_POWERS[i_id],
				     MAXIMUM_GPU_POWERS[i_id]);
		} else {
			#ifdef CONFIG_MTK_AEE_FEATURE
			aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_DEFAULT,
				"tscpu_write_atm_setting", "Wrong thermal policy");
			#endif
		}

		/* MINIMUM_TOTAL_POWER = MINIMUM_CPU_POWER + MINIMUM_GPU_POWER; */
		/* MAXIMUM_TOTAL_POWER = MAXIMUM_CPU_POWER + MAXIMUM_GPU_POWER; */

		return count;
	}
	tscpu_dprintk("tscpu_write_dtm_setting bad argument\n");
	return -EINVAL;
}

static int tscpu_read_gpu_threshold(struct seq_file *m, void *v)
{
	seq_printf(m, "H %d L %d\n", GPU_L_H_TRIP, GPU_L_L_TRIP);
	return 0;
}

static ssize_t tscpu_write_gpu_threshold(struct file *file, const char __user *buffer,
					 size_t count, loff_t *data)
{
	char desc[128];
	int len = 0;

	int gpu_h = -1, gpu_l = -1;


	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (2 <= sscanf(desc, "%d %d", &gpu_h, &gpu_l)) {
		tscpu_printk("tscpu_write_gpu_threshold input %d %d\n", gpu_h, gpu_l);

		if ((gpu_h > 0) && (gpu_l > 0) && (gpu_h > gpu_l)) {
			GPU_L_H_TRIP = gpu_h;
			GPU_L_L_TRIP = gpu_l;

			tscpu_printk("tscpu_write_gpu_threshold applied %d %d\n", GPU_L_H_TRIP,
				     GPU_L_L_TRIP);
		} else {
			tscpu_dprintk("tscpu_write_gpu_threshold out of range\n");
		}

		return count;
	}
	tscpu_dprintk("tscpu_write_gpu_threshold bad argument\n");
	return -EINVAL;
}

/* +ASC+ */
static int tscpu_read_atm(struct seq_file *m, void *v)
{

	seq_printf(m, "[tscpu_read_atm] ver = %d\n", tscpu_atm);
	seq_printf(m, "tt_ratio_high_rise = %d\n", tt_ratio_high_rise);
	seq_printf(m, "tt_ratio_high_fall = %d\n", tt_ratio_high_fall);
	seq_printf(m, "tt_ratio_low_rise = %d\n", tt_ratio_low_rise);
	seq_printf(m, "tt_ratio_low_fall = %d\n", tt_ratio_low_fall);
	seq_printf(m, "tp_ratio_high_rise = %d\n", tp_ratio_high_rise);
	seq_printf(m, "tp_ratio_high_fall = %d\n", tp_ratio_high_fall);
	seq_printf(m, "tp_ratio_low_rise = %d\n", tp_ratio_low_rise);
	seq_printf(m, "tp_ratio_low_fall = %d\n", tp_ratio_low_fall);

	return 0;
}

/* -ASC- */

/* +ASC+ */
static ssize_t tscpu_write_atm(struct file *file, const char __user *buffer, size_t count,
			       loff_t *data)
{
	char desc[128];
	int atm_ver;
	int tmp_tt_ratio_high_rise;
	int tmp_tt_ratio_high_fall;
	int tmp_tt_ratio_low_rise;
	int tmp_tt_ratio_low_fall;
	int tmp_tp_ratio_high_rise;
	int tmp_tp_ratio_high_fall;
	int tmp_tp_ratio_low_rise;
	int tmp_tp_ratio_low_fall;
	int len = 0;


	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (sscanf(desc, "%d %d %d %d %d %d %d %d %d ",
		   &atm_ver, &tmp_tt_ratio_high_rise, &tmp_tt_ratio_high_fall,
		   &tmp_tt_ratio_low_rise, &tmp_tt_ratio_low_fall, &tmp_tp_ratio_high_rise,
		   &tmp_tp_ratio_high_fall, &tmp_tp_ratio_low_rise, &tmp_tp_ratio_low_fall) == 9)
		/* if (5 <= sscanf(desc, "%d %d %d %d %d", &log_switch, &hot, &normal, &low, &lv_offset)) */
	{
		tscpu_atm = atm_ver;
		tt_ratio_high_rise = tmp_tt_ratio_high_rise;
		tt_ratio_high_fall = tmp_tt_ratio_high_fall;
		tt_ratio_low_rise = tmp_tt_ratio_low_rise;
		tt_ratio_low_fall = tmp_tt_ratio_low_fall;
		tp_ratio_high_rise = tmp_tp_ratio_high_rise;
		tp_ratio_high_fall = tmp_tp_ratio_high_fall;
		tp_ratio_low_rise = tmp_tp_ratio_low_rise;
		tp_ratio_low_fall = tmp_tp_ratio_low_fall;
		return count;
	}
	tscpu_printk("tscpu_write_atm bad argument\n");
	return -EINVAL;

}

/* -ASC- */
#if THERMAL_HEADROOM
static int tscpu_read_thp(struct seq_file *m, void *v)
{
	seq_printf(m, "Tpcb pt coef %d\n", p_Tpcb_correlation);
	seq_printf(m, "Tpcb threshold %d\n", Tpcb_trip_point);
	seq_printf(m, "Tj pt coef %d\n", thp_p_tj_correlation);
	seq_printf(m, "thp tj threshold %d\n", thp_threshold_tj);

	return 0;
}


static ssize_t tscpu_write_thp(struct file *file, const char __user *buffer, size_t count,
			       loff_t *data)
{
	char desc[128];
	int len = 0;
	int tpcb_coef = -1, tpcb_trip = -1, thp_coef = -1, thp_threshold = -1;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (4 <= sscanf(desc, "%d %d %d %d", &tpcb_coef, &tpcb_trip, &thp_coef, &thp_threshold)) {
		tscpu_printk("%s input %d %d\n", __func__, tpcb_coef, tpcb_trip, thp_coef,
			     thp_threshold);

		p_Tpcb_correlation = tpcb_coef;
		Tpcb_trip_point = tpcb_trip;
		thp_p_tj_correlation = thp_coef;
		thp_threshold_tj = thp_threshold;

		return count;
	}
	tscpu_dprintk("%s bad argument\n", __func__);
	return -EINVAL;
}
#endif

#if CONTINUOUS_TM
static int tscpu_read_ctm(struct seq_file *m, void *v)
{
	seq_printf(m, "ctm %d\n", ctm_on);
	seq_printf(m, "Target Tj 0 %d\n", MAX_TARGET_TJ);
	seq_printf(m, "Target Tj 2 %d\n", STEADY_TARGET_TJ);
	seq_printf(m, "Tpcb 1 %d\n", TRIP_TPCB);
	seq_printf(m, "Tpcb 2 %d\n", STEADY_TARGET_TPCB);
	seq_printf(m, "Exit Tj 0 %d\n", MAX_EXIT_TJ);
	seq_printf(m, "Exit Tj 2 %d\n", STEADY_EXIT_TJ);
	seq_printf(m, "Enter_a %d\n", COEF_AE);
	seq_printf(m, "Enter_b %d\n", COEF_BE);
	seq_printf(m, "Exit_a %d\n", COEF_AX);
	seq_printf(m, "Exit_b %d\n", COEF_BX);
	return 0;
}

static ssize_t tscpu_write_ctm(struct file *file, const char __user *buffer, size_t count,
			       loff_t *data)
{
	char desc[256];
	int len = 0;
	int t_ctm_on = -1, t_MAX_TARGET_TJ = -1, t_STEADY_TARGET_TJ = -1, t_TRIP_TPCB =
	    -1, t_STEADY_TARGET_TPCB = -1, t_MAX_EXIT_TJ = -1, t_STEADY_EXIT_TJ = -1, t_COEF_AE =
	    -1, t_COEF_BE = -1, t_COEF_AX = -1, t_COEF_BX = -1;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (11 <= sscanf(desc, "%d %d %d %d %d %d %d %d %d %d %d", &t_ctm_on, &t_MAX_TARGET_TJ,
		   &t_STEADY_TARGET_TJ, &t_TRIP_TPCB, &t_STEADY_TARGET_TPCB, &t_MAX_EXIT_TJ,
		   &t_STEADY_EXIT_TJ, &t_COEF_AE, &t_COEF_BE, &t_COEF_AX, &t_COEF_BX)) {
		tscpu_printk("%s input %d %d %d %d %d %d %d %d %d %d %d\n", __func__, t_ctm_on,
			     t_MAX_TARGET_TJ, t_STEADY_TARGET_TJ, t_TRIP_TPCB, t_STEADY_TARGET_TPCB,
			     t_MAX_EXIT_TJ, t_STEADY_EXIT_TJ, t_COEF_AE, t_COEF_BE, t_COEF_AX,
			     t_COEF_BX);

		if (t_ctm_on < 0 || t_ctm_on > 1)
			#ifdef CONFIG_MTK_AEE_FEATURE
			aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_DEFAULT,
				"tscpu_write_ctm", "Wrong thermal policy");
			#endif

		if (t_MAX_TARGET_TJ < -20000 || t_MAX_TARGET_TJ > 200000)
			#ifdef CONFIG_MTK_AEE_FEATURE
			aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_DEFAULT,
				"tscpu_write_ctm", "Wrong thermal policy");
			#endif

		if (t_STEADY_TARGET_TJ < -20000 || t_STEADY_TARGET_TJ > 200000)
			#ifdef CONFIG_MTK_AEE_FEATURE
			aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_DEFAULT,
				"tscpu_write_ctm", "Wrong thermal policy");
			#endif

		if (t_TRIP_TPCB < -20000 || t_TRIP_TPCB > 200000)
			#ifdef CONFIG_MTK_AEE_FEATURE
			aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_DEFAULT,
				"tscpu_write_ctm", "Wrong thermal policy");
			#endif

		if (t_STEADY_TARGET_TPCB < -20000 || t_STEADY_TARGET_TPCB > 200000)
			#ifdef CONFIG_MTK_AEE_FEATURE
			aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_DEFAULT,
				"tscpu_write_ctm", "Wrong thermal policy");
			#endif

		if (t_MAX_EXIT_TJ < -20000 || t_MAX_EXIT_TJ > 200000)
			#ifdef CONFIG_MTK_AEE_FEATURE
			aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_DEFAULT,
				"tscpu_write_ctm", "Wrong thermal policy");
			#endif

		if (t_STEADY_EXIT_TJ < -20000 || t_STEADY_EXIT_TJ > 200000)
			#ifdef CONFIG_MTK_AEE_FEATURE
			aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_DEFAULT,
				"tscpu_write_ctm", "Wrong thermal policy");
			#endif

		if (t_COEF_AE < 0 || t_COEF_BE < 0 || t_COEF_AX < 0 || t_COEF_BX < 0)
			#ifdef CONFIG_MTK_AEE_FEATURE
			aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_DEFAULT,
				"tscpu_write_ctm", "Wrong thermal policy");
			#endif

		/* no parameter checking here */
		ctm_on = t_ctm_on;	/* 1: on, 0: off */

		MAX_TARGET_TJ = t_MAX_TARGET_TJ;
		STEADY_TARGET_TJ = t_STEADY_TARGET_TJ;
		TRIP_TPCB = t_TRIP_TPCB;
		STEADY_TARGET_TPCB = t_STEADY_TARGET_TPCB;
		MAX_EXIT_TJ = t_MAX_EXIT_TJ;
		STEADY_EXIT_TJ = t_STEADY_EXIT_TJ;

		COEF_AE = t_COEF_AE;
		COEF_BE = t_COEF_BE;
		COEF_AX = t_COEF_AX;
		COEF_BX = t_COEF_BX;

		return count;
	}

	tscpu_dprintk("%s bad argument\n", __func__);
	return -EINVAL;
}
#endif
#endif

#if CPT_ADAPTIVE_AP_COOLER
static int tscpu_atm_setting_open(struct inode *inode, struct file *file)
{
	return single_open(file, tscpu_read_atm_setting, NULL);
}

static const struct file_operations mtktscpu_atm_setting_fops = {
	.owner = THIS_MODULE,
	.open = tscpu_atm_setting_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = tscpu_write_atm_setting,
	.release = single_release,
};


static int tscpu_gpu_threshold_open(struct inode *inode, struct file *file)
{
	return single_open(file, tscpu_read_gpu_threshold, NULL);
}

static const struct file_operations mtktscpu_gpu_threshold_fops = {
	.owner = THIS_MODULE,
	.open = tscpu_gpu_threshold_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = tscpu_write_gpu_threshold,
	.release = single_release,
};

/* +ASC+ */
static int tscpu_open_atm(struct inode *inode, struct file *file)
{
	return single_open(file, tscpu_read_atm, NULL);
}

static const struct file_operations mtktscpu_atm_fops = {
	.owner = THIS_MODULE,
	.open = tscpu_open_atm,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = tscpu_write_atm,
	.release = single_release,
};

/* -ASC- */

#if THERMAL_HEADROOM
static int tscpu_thp_open(struct inode *inode, struct file *file)
{
	return single_open(file, tscpu_read_thp, NULL);
}

static const struct file_operations mtktscpu_thp_fops = {
	.owner = THIS_MODULE,
	.open = tscpu_thp_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = tscpu_write_thp,
	.release = single_release,
};
#endif

#if CONTINUOUS_TM
static int tscpu_ctm_open(struct inode *inode, struct file *file)
{
	return single_open(file, tscpu_read_ctm, NULL);
}

static const struct file_operations mtktscpu_ctm_fops = {
	.owner = THIS_MODULE,
	.open = tscpu_ctm_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = tscpu_write_ctm,
	.release = single_release,
};
#endif				/* CONTINUOUS_TM */
#endif				/* CPT_ADAPTIVE_AP_COOLER */

static void tscpu_cooler_create_fs(void)
{

	struct proc_dir_entry *entry = NULL;
	struct proc_dir_entry *mtktscpu_dir = NULL;

	mtktscpu_dir = mtk_thermal_get_proc_drv_therm_dir_entry();
	if (!mtktscpu_dir) {
		tscpu_printk("[%s]: mkdir /proc/driver/thermal failed\n", __func__);
	} else {
#if CPT_ADAPTIVE_AP_COOLER
		entry =
		    proc_create("clatm_setting", S_IRUGO | S_IWUSR | S_IWGRP, mtktscpu_dir,
				&mtktscpu_atm_setting_fops);
		if (entry)
			proc_set_user(entry, uid, gid);

		entry =
		    proc_create("clatm_gpu_threshold", S_IRUGO | S_IWUSR | S_IWGRP, mtktscpu_dir,
				&mtktscpu_gpu_threshold_fops);
		if (entry)
			proc_set_user(entry, uid, gid);
#endif				/* #if CPT_ADAPTIVE_AP_COOLER */


		/* +ASC+ */
		entry = proc_create("clatm", S_IRUGO | S_IWUSR, mtktscpu_dir, &mtktscpu_atm_fops);
		if (entry)
			proc_set_user(entry, uid, gid);
		/* -ASC- */

#if THERMAL_HEADROOM
		entry = proc_create("clthp", S_IRUGO | S_IWUSR, mtktscpu_dir, &mtktscpu_thp_fops);
		if (entry)
			proc_set_user(entry, uid, gid);
#endif

#if CONTINUOUS_TM
		entry = proc_create("clctm", S_IRUGO | S_IWUSR, mtktscpu_dir, &mtktscpu_ctm_fops);
		if (entry)
			proc_set_user(entry, uid, gid);
#endif
	}


}

static int __init mtk_cooler_atm_init(void)
{
	int err = 0;

	tscpu_dprintk("mtk_cooler_atm_init: Start\n");
#if CPT_ADAPTIVE_AP_COOLER
	cl_dev_adp_cpu[0] = mtk_thermal_cooling_device_register("cpu_adaptive_0", NULL,
								&mtktscpu_cooler_adp_cpu_ops);

	cl_dev_adp_cpu[1] = mtk_thermal_cooling_device_register("cpu_adaptive_1", NULL,
								&mtktscpu_cooler_adp_cpu_ops);

	cl_dev_adp_cpu[2] = mtk_thermal_cooling_device_register("cpu_adaptive_2", NULL,
								&mtktscpu_cooler_adp_cpu_ops);
#endif
	if (err) {
		tscpu_printk("tscpu_register_DVFS_hotplug_cooler fail\n");
		return err;
	}
	tscpu_cooler_create_fs();
	tscpu_dprintk("mtk_cooler_atm_init: End\n");
	return 0;
}

static void __exit mtk_cooler_atm_exit(void)
{
#if CPT_ADAPTIVE_AP_COOLER
	if (cl_dev_adp_cpu[0]) {
		mtk_thermal_cooling_device_unregister(cl_dev_adp_cpu[0]);
		cl_dev_adp_cpu[0] = NULL;
	}

	if (cl_dev_adp_cpu[1]) {
		mtk_thermal_cooling_device_unregister(cl_dev_adp_cpu[1]);
		cl_dev_adp_cpu[1] = NULL;
	}

	if (cl_dev_adp_cpu[2]) {
		mtk_thermal_cooling_device_unregister(cl_dev_adp_cpu[2]);
		cl_dev_adp_cpu[2] = NULL;
	}
#endif
}
module_init(mtk_cooler_atm_init);
module_exit(mtk_cooler_atm_exit);
