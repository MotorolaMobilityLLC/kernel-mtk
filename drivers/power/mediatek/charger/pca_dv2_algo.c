/*
 * Copyright (C) 2019 MediaTek Inc.
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/of.h>
#include <linux/notifier.h>
#include <linux/alarmtimer.h>
#include <tcpm.h>
#include <mt-plat/prop_chgalgo_class.h>

#define PCA_DV2_ALGO_VERSION	"1.0.0_G"
#define MS_TO_NS(msec) ((msec) * 1000 * 1000)
#define MIN(A, B) (((A) < (B)) ? (A) : (B))

enum dv2_ta_type {
	DV2_TATYP_RFC = 0,
	DV2_TATYP_PPS,
	DV2_TATYP_MAX,
};

enum dv2_algo_state {
	DV2_ALGO_INIT = 0,
	DV2_ALGO_MEASURE_R,
	DV2_ALGO_SS_SWCHG,
	DV2_ALGO_SS_DVCHG,
	DV2_ALGO_CC_CV,
	DV2_ALGO_STOP,
};

enum dv2_tta_level {
	DV2_TTA_NORMAL = 0,
	DV2_TTA_LEVEL1,
	DV2_TTA_LEVEL2,
	DV2_TTA_LEVEL3,
	DV2_TTA_MAX, /* max ta tempearture */
	DV2_TTA_NUM,
};

enum dv2_tbat_level {
	DV2_TBAT_MIN = 0, /* min bat temperature */
	DV2_TBAT_NORMAL,
	DV2_TBAT_LEVEL1,
	DV2_TBAT_LEVEL2,
	DV2_TBAT_LEVEL3,
	DV2_TBAT_MAX, /* max bat temperature */
	DV2_TBAT_NUM,
};

enum dv2_tdvchg_level {
	DV2_TDVCHG_NORMAL = 0,
	DV2_TDVCHG_MAX,
	DV2_TDVCHG_NUM,
};

enum dv2_rcable_level {
	DV2_RCABLE_NORMAL = 0,
	DV2_RCABLE_BAD1,
	DV2_RCABLE_BAD2,
	DV2_RCABLE_BAD3,
	DV2_RCABLE_NUM,
};

/* Setting from dtsi */
struct dv2_algo_desc {
	u32 polling_interval;		/* polling interval */
	u32 vbus_upper_bound;		/* vbus upper bound */
	u32 vbat_upper_bound;		/* vbat upper bound */
	u32 start_soc_min;		/* algo start bat low bound */
	u32 idvchg_term;		/* terminated current */
	u32 idvchg_step;		/* input current step */
	u32 ita_level[DV2_RCABLE_NUM];	/* input current */
	u32 rcable_level[DV2_RCABLE_NUM];	/* cable impedance level */
	u32 idvchg_ss_init;		/* SS state init input current */
	u32 idvchg_ss_step;		/* SS state input current step */
	u32 idvchg_ss_step1;		/* SS state input current step2 */
	u32 idvchg_ss_step2;		/* SS state input current step3 */
	u32 idvchg_ss_step1_vbat;	/* vbat threshold for ic_ss_step2 */
	u32 idvchg_ss_step2_vbat;	/* vbat threshold for ic_ss_step3 */
	u32 ta_blanking;		/* wait TA stable */
	u32 swchg_aicr;			/* CC state swchg input current */
	u32 swchg_ichg;			/* CC state swchg charging current */
	u32 swchg_aicr_ss_init;		/* SWCHG_SS state init input current */
	u32 swchg_aicr_ss_step;		/* SWCHG_SS state input current step */
	u32 chg_time_max;		/* max charging time */
	int ta_temp_level[DV2_TTA_NUM];    /* TA temperature level */
	int ta_temp_curlmt[DV2_TTA_NUM];   /* TA temperature current limit */
	int bat_temp_level[DV2_TBAT_NUM];  /* BAT temperature level */
	int bat_temp_curlmt[DV2_TBAT_NUM]; /* BAT temperature current limit */
	int dvchg_temp_level[DV2_TDVCHG_NUM];  /* DVCHG temp level */
	int dvchg_temp_curlmt[DV2_TDVCHG_NUM]; /* DVCHG temp current limit */
	u32 ta_temp_recover_area;
	u32 bat_temp_recover_area;
	u32 fod_current;		/* FOD current threshold */
	u32 r_bat_min;			/* min r_bat */
	u32 r_sw_min;			/* min r_sw */
	u32 ta_cap_vmin;		/* min ta voltage capability */
	u32 ta_cap_vmax;		/* max ta voltage capability */
	u32 ta_cap_imin;		/* min ta current capability */
	bool need_bif;			/* is bif needed */
};

/* Algorithm related information */
struct dv2_algo_data {
	struct prop_chgalgo_device *pca_ta_pool[DV2_TATYP_MAX];
	struct prop_chgalgo_device *pca_ta;
	struct prop_chgalgo_device *pca_swchg;
	struct prop_chgalgo_device *pca_dvchg;

	/* Thread & Timer */
	struct alarm timer;
	struct task_struct *task;
	struct mutex lock;

	/* TCPC */
	struct notifier_block tcp_nb;
	struct mutex tcp_lock;
	u32 tcp_evt;

	/* Algorithm */
	bool check_once;
	bool inited;
	bool run_once;
	bool is_swchg_en;
	bool is_dvchg_en;
	bool is_bif_exist;
	u32 vta_boundary_min;
	u32 vta_boundary_max;
	u32 ita_boundary_max;
	u32 vta_setting;
	u32 ita_setting;
	u32 vta_measure;
	u32 ita_measure;
	u32 tta_measure;
	u32 ichg_setting;
	u32 aicr_setting;
	u32 aicr_lmt;
	u32 idvchg_cc;
	u32 vbusovp;
	u32 ibusocp;
	u32 vbatovp;
	int vbus_cali;
	u32 r_sw;
	u32 r_cable;
	u32 r_bat;
	u32 r_total;
	u32 ita_lmt;
	u32 ita_lmt_by_r;
	u32 cv_lower_bound;
	u32 cv_readd_ita_cnt;
	u32 vbat_swchg_off;
	bool is_vbat_over_cv;
	int tbat;
	int tdvchg;
	struct timespec stime;
	enum dv2_algo_state state;
	enum dv2_tbat_level tbat_level;
	enum dv2_tta_level tta_level;
	enum dv2_tdvchg_level tdvchg_level;
};

struct dv2_algo_info {
	struct device *dev;
	struct tcpc_device *tcpc;
	struct prop_chgalgo_device *pca;
	struct dv2_algo_desc *desc;
	struct dv2_algo_data *data;
};

/* if there's no property in dts, these values will by applied */
static struct dv2_algo_desc algo_desc_defval = {
	.polling_interval = 500,
	.vbus_upper_bound = 10000,
	.vbat_upper_bound = 4350,
	.start_soc_min = 5,
	.idvchg_term = 1000,
	.idvchg_step = 50,
	.ita_level = {2000, 1800, 1600, 1400},
	.rcable_level = {125, 150, 188, 250},
	.idvchg_ss_init = 750,
	.idvchg_ss_step = 250,
	.idvchg_ss_step1 = 100,
	.idvchg_ss_step2 = 50,
	.idvchg_ss_step1_vbat = 4000,
	.idvchg_ss_step2_vbat = 4200,
	.ta_blanking = 300,
	.swchg_ichg = 0,
	.swchg_aicr = 0,
	.swchg_aicr_ss_init = 0,
	.swchg_aicr_ss_step = 0,
	.chg_time_max = 5400,
	.ta_temp_level = {25, 50, 60, 70, 80},
	.ta_temp_curlmt = {-1, 1800, 1600, 1400, 0},
	.ta_temp_recover_area = 3,
	.bat_temp_level = {0, 25, 40, 45, 50, 55},
	.bat_temp_curlmt = {0, -1, 1800, 1600, 1400, 0},
	.bat_temp_recover_area = 3,
	.dvchg_temp_level = {25, 70},
	.dvchg_temp_curlmt = {-1, 0},
	.fod_current = 200,
	.r_bat_min = 40,
	.r_sw_min = 20,
	.ta_cap_vmin = 6800,
	.ta_cap_vmax = 11000,
	.ta_cap_imin = 1000,
	.need_bif = false,
};

static const char *__dv2_ta_name[DV2_TATYP_MAX] = {
	"pca_ta_pps",
	"pca_ta_rfc",
};

/*
 * name of pca device used to get corresponding adc
 * For each channel, it can be
 * "pca_chg_swchg" or "pca_chg_dvchg"
 */
static const char *__dv2_adc_pcadev_name[PCA_ADCCHAN_MAX] = {
	"pca_chg_swchg",
	"pca_chg_swchg",
	"pca_chg_swchg",
	"pca_chg_swchg",
	"pca_chg_swchg",
	"pca_chg_swchg",
	"pca_chg_swchg",
	"pca_chg_swchg",
	"pca_chg_swchg",
};

static struct prop_chgalgo_device *__dv2_adc_pcadev[PCA_ADCCHAN_MAX];

/*
 * Get ADC value
 * It uses pca device according to the name specified in __dv2_adc_pcadev_name
 */
static int __dv2_get_adc(struct dv2_algo_info *info,
			 enum prop_chgalgo_adc_channel chan, int *min, int *max)
{
	struct prop_chgalgo_device *pca  = __dv2_adc_pcadev[chan];

	if (!pca)
		return -EINVAL;
	return prop_chgalgo_get_adc(pca, chan, min, max);
}

/*
 * Calculate VBUS for divider charger
 */
#define DVCHG_CONVERT_RATIO	(206)
static inline u32 __dv2_vbat2vta(u32 vbat)
{
	return vbat * DVCHG_CONVERT_RATIO / 100;
}

/*
 * Calculate calibrated output voltage of TA by measured resistence
 * Firstly, calculate voltage outputing from divider charger
 * Secondly, calculate voltage outputing from TA
 *
 * @ita: expected output current of TA
 * @vta: calibrated output voltage of TA
 */
static inline int __dv2_get_cali_vta(struct dv2_algo_info *info, u32 ita,
				     u32 *vta)
{
	int ret, vbat;
	struct dv2_algo_data *data = info->data;
	struct dv2_algo_desc *desc = info->desc;
	u32 r_cali, ibat;

	ret = __dv2_get_adc(info, data->is_bif_exist ?
			    PCA_ADCCHAN_BIFVBAT : PCA_ADCCHAN_VBAT, &vbat,
			    &vbat);
	if (ret < 0) {
		PCA_ERR("get vbat fail\n");
		return ret;
	}
	r_cali = data->is_bif_exist ? (data->r_sw + data->r_bat) : data->r_sw;
	ibat = data->is_swchg_en ? (ita - data->aicr_setting) : ita;
	*vta = (vbat + (ibat * 2 * r_cali) / 1000);
	*vta = __dv2_vbat2vta(*vta);
	*vta += (data->vbus_cali + (ita * data->r_cable / 1000));
	if (*vta >= desc->vbus_upper_bound)
		*vta = desc->vbus_upper_bound;
	return 0;
}

/*
 * Set output capability of TA and update setting in data
 *
 * @vta: output voltage of TA, mV
 * @ita: output current of TA, mA
 */
#define CC_MODE_VTA_COMP	(100)
static inline int __dv2_set_ta_cap(struct dv2_algo_info *info, u32 vta,
				   u32 ita)
{
	int ret;
	struct dv2_algo_data *data = info->data;
	struct dv2_algo_desc *desc = info->desc;
	struct prop_chgalgo_ta_status status;

	ret = prop_chgalgo_set_ta_cap(data->pca_ta, vta, ita);
	if (ret < 0)
		return ret;
	if (data->vta_setting != vta || data->ita_setting != ita) {
		while (data->is_dvchg_en) {
			msleep(desc->ta_blanking);
			ret = prop_chgalgo_get_ta_status(data->pca_ta, &status);
			if (ret < 0)
				return ret;
			if (status.cc_mode)
				break;
			PCA_ERR("Not in cc mode\n");
			if (vta >= desc->vbus_upper_bound)
				return -EINVAL;
			data->vbus_cali += CC_MODE_VTA_COMP;
			vta += CC_MODE_VTA_COMP;
			vta = MIN(vta, desc->vbus_upper_bound);
			ret = prop_chgalgo_set_ta_cap(data->pca_ta, vta, ita);
			if (ret < 0)
				return ret;
		}
		data->vta_setting = vta;
		data->ita_setting = ita;
	}
	PCA_INFO("vta = %d, ita = %d\n", vta, ita);
	return 0;
}

/*
 * Get output current and voltage measured by TA
 * and updates measured data
 */
static inline int __dv2_get_ta_cap(struct dv2_algo_info *info)
{
	struct dv2_algo_data *data = info->data;

	return prop_chgalgo_get_ta_measure_cap(data->pca_ta, &data->vta_measure,
					       &data->ita_measure);
}

/*
 * Select current limit according to severial status
 * If switching charger is charging, add AICR setting to ita
 * For now, the following features are taken into consider
 * 1. Resistence
 * 2. Battery's temperature
 * 3. TA's temperature
 */
static inline int __dv2_get_ita_lmt(struct dv2_algo_info *info)
{
	struct dv2_algo_data *data = info->data;
	struct dv2_algo_desc *desc = info->desc;
	int ita = data->ita_lmt;

	PCA_INFO("tta,tdvchg,tbat(level,curlmt) = (%d,%d),(%d,%d),(%d,%d)\n",
		 data->tta_level, desc->ta_temp_curlmt[data->tta_level],
		 data->tdvchg_level,
		 desc->dvchg_temp_curlmt[data->tdvchg_level], data->tbat_level,
		 desc->bat_temp_curlmt[data->tbat_level]);
	if (desc->ta_temp_curlmt[data->tta_level] != -1)
		ita = MIN(ita, desc->ta_temp_curlmt[data->tta_level]);
	if (desc->dvchg_temp_curlmt[data->tdvchg_level] != -1)
		ita = MIN(ita, desc->dvchg_temp_curlmt[data->tdvchg_level]);
	if (desc->bat_temp_curlmt[data->tbat_level] != -1)
		ita = MIN(ita, desc->bat_temp_curlmt[data->tbat_level]);
	return ita;
}

#define DV2_VBUSOVP_RATIO	(110)
#define DV2_IBUSOCP_RATIO	(140)
#define DV2_VBATOVP_RATIO	(110)
#define DV2_IBATOCP_RATIO	(110)
#define DV2_ITAOCP_RATIO	(110)

/* Calculate VBUSOV level */
static u32 __dv2_get_dvchg_vbusovp(struct dv2_algo_info *info, u32 ita)
{
	struct dv2_algo_data *data = info->data;
	struct dv2_algo_desc *desc = info->desc;
	u32 vbusov, ibat;

	ibat = 2 * (data->is_swchg_en ? (ita - data->aicr_setting) : ita);
	vbusov = desc->vbat_upper_bound + ibat * data->r_sw / 1000;
	return DV2_VBUSOVP_RATIO * __dv2_vbat2vta(vbusov) / 100;
}

/* Calculate IBUSOC level */
static u32 __dv2_get_dvchg_ibusocp(struct dv2_algo_info *info, u32 ita)
{
	struct dv2_algo_data *data = info->data;
	u32 ibus;

	ibus = data->is_swchg_en ? (ita - data->aicr_setting) : ita;
	return DV2_IBUSOCP_RATIO * ibus / 100;
}

/* Calculate VBATOV level */
static u32 __dv2_get_vbatovp(struct dv2_algo_info *info, u32 ita)
{
	struct dv2_algo_data *data = info->data;
	struct dv2_algo_desc *desc = info->desc;
	u32 vbatovp, ibat;

	ibat = 2 * (data->is_swchg_en ?
		(ita - data->aicr_setting) + data->ichg_setting : ita);
	vbatovp = desc->vbat_upper_bound + ibat * data->r_bat / 1000;
	return DV2_VBATOVP_RATIO * vbatovp / 100;
}

/* Calculate IBATOC level */
static u32 __dv2_get_ibatocp(struct dv2_algo_info *info, u32 ita)
{
	struct dv2_algo_data *data = info->data;
	u32 ibat;

	ibat = data->is_swchg_en ?
		(2 * (ita - data->aicr_setting) + data->ichg_setting) : 2 * ita;
	return DV2_IBATOCP_RATIO * ibat / 100;
}

static u32 __dv2_get_itaocp(struct dv2_algo_info *info)
{
	struct dv2_algo_data *data = info->data;

	return DV2_ITAOCP_RATIO * data->ita_setting / 100;
}

/*
 * Check TA's status
 * Get status from TA and check temperature, OCP, OTP, and OVP, etc...
 *
 * return true if TA is normal and false if it is abnormal
 */
static bool __dv2_check_ta_status(struct dv2_algo_info *info)
{
	int ret;
	struct prop_chgalgo_ta_status status;
	struct dv2_algo_data *data = info->data;

	ret = prop_chgalgo_get_ta_status(data->pca_ta, &status);
	if (ret < 0) {
		PCA_ERR("get ta status fail(%d)\n", ret);
		return false;
	}

	PCA_INFO("temp = (%d,%d), (OVP,OCP,OTP) = (%d,%d,%d)\n",
		 status.temp1, status.temp_level, status.ovp, status.ocp,
		 status.otp);
	if (status.ocp || status.otp || status.ovp)
		return false;
	return true;
}

/*
 * Check VBUS voltage of divider charger
 * return false if VBUS is over voltage otherwise return true
 */
static bool __dv2_check_dvchg_vbusovp(struct dv2_algo_info *info)
{
	int ret, vbus;
	struct dv2_algo_data *data = info->data;
	u32 vbusovp;

	ret = __dv2_get_adc(info, PCA_ADCCHAN_VBUS, &vbus, &vbus);
	if (ret < 0) {
		PCA_ERR("get vbus fail\n");
		return false;
	}
	vbusovp = __dv2_get_dvchg_vbusovp(info, data->ita_setting);
	PCA_INFO("vbus = %d, vbusovp = %d\n", vbus, vbusovp);
	if (vbus >= vbusovp) {
		PCA_ERR("vbus(%d) >= vbusovp(%d)\n", vbus, vbusovp);
		return false;
	}
	return true;
}

static bool __dv2_check_dvchg_ibusocp(struct dv2_algo_info *info)
{
	int ret, ibus;
	struct dv2_algo_data *data = info->data;
	u32 ibusocp;

	ret = __dv2_get_adc(info, PCA_ADCCHAN_IBUS, &ibus, &ibus);
	if (ret < 0) {
		PCA_ERR("get ibus fail(%d)\n", ret);
		return false;
	}
	ibusocp = __dv2_get_dvchg_ibusocp(info, data->ita_setting);
	PCA_INFO("ibus = %d, ibusocp = %d\n", ibus, ibusocp);
	if (ibus >= ibusocp) {
		PCA_ERR("ibus(%d) >= ibusocp(%d)\n", ibus, data->ibusocp);
		return false;
	}
	return true;
}

static bool __dv2_check_ta_ibusocp(struct dv2_algo_info *info)
{
	int ret;
	struct dv2_algo_data *data = info->data;
	u32 itaocp;

	ret = __dv2_get_ta_cap(info);
	if (ret < 0) {
		PCA_ERR("get ta cap fail(%d)\n", ret);
		return false;
	}
	itaocp = __dv2_get_itaocp(info);
	PCA_INFO("ita = %d, itaocp = %d\n", data->ita_measure, itaocp);
	if (data->ita_measure >= itaocp) {
		PCA_ERR("ita(%d) >= itaocp(%d)\n", data->ita_measure, itaocp);
		return false;
	}
	return true;
}

static bool __dv2_check_vbatovp(struct dv2_algo_info *info)
{
	int ret, vbat, ibat;
	struct dv2_algo_data *data = info->data;
	u32 vbatovp;

	ret = __dv2_get_adc(info, data->is_bif_exist ?
			    PCA_ADCCHAN_BIFVBAT : PCA_ADCCHAN_VBAT, &vbat,
			    &vbat);
	if (ret < 0) {
		PCA_ERR("get vbat fail(%d)\n", ret);
		return false;
	}
	if (data->is_bif_exist) {
		ret = __dv2_get_adc(info, PCA_ADCCHAN_IBAT, &ibat, &ibat);
		if (ret < 0) {
			PCA_ERR("get ibat fail(%d)\n", ret);
			return false;
		}
		vbat += ibat * data->r_bat / 1000;
	}

	vbatovp = __dv2_get_vbatovp(info, data->ita_setting);
	PCA_INFO("vbat = %d vbatovp = %d\n", vbat, vbatovp);
	if (vbat >= vbatovp) {
		PCA_ERR("vbat(%d) >= vbatovp(%d)\n", vbat, vbatovp);
		return false;
	}
	return true;
}

static bool __dv2_check_ibatocp(struct dv2_algo_info *info)
{
	int ret, ibat;
	struct dv2_algo_data *data = info->data;
	u32 ibatocp;

	ret = __dv2_get_adc(info, PCA_ADCCHAN_IBAT, &ibat, &ibat);
	if (ret < 0)
		return false;

	ibatocp = __dv2_get_ibatocp(info, data->ita_setting);
	PCA_INFO("ibat = %d, ibatocp = %d\n", ibat, ibatocp);
	if (ibat >= ibatocp) {
		PCA_ERR("ibat(%d) >= ibatocp(%d)\n", ibat, ibatocp);
		return false;
	}
	return true;
}

/*
 * Check and adjust battery's temperature level
 * return false if battery's temperature is over maximum or under minimum
 * otherwise return true
 */
static bool __dv2_check_tbat_level(struct dv2_algo_info *info)
{
	int ret;
	struct dv2_algo_data *data = info->data;
	struct dv2_algo_desc *desc = info->desc;

	ret = __dv2_get_adc(info, PCA_ADCCHAN_TBAT, &data->tbat, &data->tbat);
	if (ret < 0) {
		PCA_ERR("get tbat fail(%d)\n", ret);
		return false;
	}
	if (data->tbat >= desc->bat_temp_level[DV2_TBAT_MAX]) {
		PCA_ERR("tbat(%d) is over max(%d)\n", data->tbat,
			desc->bat_temp_level[DV2_TBAT_MAX]);
		return false;
	}
	if (data->tbat <= desc->bat_temp_level[DV2_TBAT_MIN]) {
		PCA_ERR("tbat(%d) is under min(%d)\n", data->tbat,
			desc->bat_temp_level[DV2_TBAT_MIN]);
		return false;
	}
	switch (data->tbat_level) {
	case DV2_TBAT_NORMAL:
		if (data->tbat >= desc->bat_temp_level[DV2_TBAT_LEVEL1])
			data->tbat_level = DV2_TBAT_LEVEL1;
		break;
	case DV2_TBAT_LEVEL1:
		if (data->tbat <= (desc->bat_temp_level[DV2_TBAT_LEVEL1] -
				   desc->bat_temp_recover_area))
			data->tbat_level = DV2_TBAT_NORMAL;
		else if (data->tbat >= desc->bat_temp_level[DV2_TBAT_LEVEL2])
			data->tbat_level = DV2_TBAT_LEVEL2;
		break;
	case DV2_TBAT_LEVEL2:
		if (data->tbat <= (desc->bat_temp_level[DV2_TBAT_LEVEL2] -
				   desc->bat_temp_recover_area))
			data->tbat_level = DV2_TBAT_LEVEL1;
		else if (data->tbat >= desc->bat_temp_level[DV2_TBAT_LEVEL3])
			data->tbat_level = DV2_TBAT_LEVEL3;
		break;
	case DV2_TBAT_LEVEL3:
		if (data->tbat <= (desc->bat_temp_level[DV2_TBAT_LEVEL3] -
				   desc->bat_temp_recover_area))
			data->tbat_level = DV2_TBAT_LEVEL2;
		break;
	default:
		PCA_ERR("NO SUCH STATE\n");
		return false;
	}
	PCA_INFO("tbat = (%d,%d)\n", data->tbat, data->tbat_level);
	return true;
}

/*
 * Check and adjust TA's temperature level
 * return false if TA's temperature is over maximum
 * otherwise return true
 */
static bool __dv2_check_tta_level(struct dv2_algo_info *info)
{
	int ret;
	struct dv2_algo_data *data = info->data;
	struct dv2_algo_desc *desc = info->desc;

	ret = prop_chgalgo_get_ta_temperature(data->pca_ta, &data->tta_measure);
	if (ret < 0) {
		PCA_ERR("get tta fail(%d)\n", ret);
		return false;
	}
	if (data->tta_measure >= desc->ta_temp_level[DV2_TTA_MAX]) {
		PCA_ERR("tta(%d) is over max(%d)\n", data->tta_measure,
			desc->ta_temp_level[DV2_TTA_MAX]);
		return false;
	}
	switch (data->tta_level) {
	case DV2_TTA_NORMAL:
		if (data->tta_measure >= desc->ta_temp_level[DV2_TTA_LEVEL1])
			data->tta_level = DV2_TTA_LEVEL1;
		break;
	case DV2_TTA_LEVEL1:
		if (data->tta_measure <= (desc->ta_temp_level[DV2_TTA_LEVEL1] -
					  desc->ta_temp_recover_area))
			data->tta_level = DV2_TTA_NORMAL;
		else if (data->tta_measure >=
			 desc->ta_temp_level[DV2_TTA_LEVEL2])
			data->tta_level = DV2_TTA_LEVEL2;
		break;
	case DV2_TTA_LEVEL2:
		if (data->tta_measure <= (desc->ta_temp_level[DV2_TTA_LEVEL2] -
					  desc->ta_temp_recover_area))
			data->tta_level = DV2_TTA_LEVEL1;
		else if (data->tta_measure >=
			 desc->ta_temp_level[DV2_TTA_LEVEL3])
			data->tta_level = DV2_TTA_LEVEL3;
		break;
	case DV2_TTA_LEVEL3:
		if (data->tta_measure <= (desc->ta_temp_level[DV2_TTA_LEVEL3] -
					  desc->ta_temp_recover_area))
			data->tta_level = DV2_TTA_LEVEL2;
		break;
	default:
		PCA_ERR("NO SUCH STATE\n");
		return false;
	}
	PCA_INFO("tta = (%d,%d)\n", data->tta_measure, data->tta_level);
	return true;
}

/*
 * Check and adjust charger's temperature level
 * return false if charger's temperature is over maximum
 * otherwise return true
 */
static bool __dv2_check_tdvchg_level(struct dv2_algo_info *info)
{
	int ret;
	struct dv2_algo_data *data = info->data;
	struct dv2_algo_desc *desc = info->desc;

	ret = __dv2_get_adc(info, PCA_ADCCHAN_TCHG, &data->tdvchg,
			    &data->tdvchg);
	if (ret < 0)
		return false;
	if (data->tdvchg >= desc->dvchg_temp_level[DV2_TDVCHG_MAX]) {
		PCA_ERR("tdvchg is over max(%d)\n",
			desc->dvchg_temp_level[DV2_TDVCHG_MAX]);
		return false;
	}
	PCA_INFO("tdvchg = (%d,%d)\n", data->tdvchg, data->tdvchg_level);
	return true;
}

static int __dv2_set_dvchg_protection(struct dv2_algo_info *info)
{
	int ret;
	struct dv2_algo_data *data = info->data;
	struct dv2_algo_desc *desc = info->desc;
	u32 vbusovp, ibusocp, vbatovp, ibatocp;
	u32 ita = desc->ita_level[DV2_RCABLE_NORMAL];

	/* VBUSOVP */
	vbusovp = __dv2_get_dvchg_vbusovp(info, ita);
	ret = prop_chgalgo_set_vbusovp(data->pca_dvchg, vbusovp);
	if (ret < 0) {
		PCA_ERR("set vbusovp fail(%d)\n", ret);
		return ret;
	}
	/* IBUSOCP */
	ibusocp = __dv2_get_dvchg_ibusocp(info, ita);
	ret = prop_chgalgo_set_ibusocp(data->pca_dvchg, ibusocp);
	if (ret < 0) {
		PCA_ERR("set ibusocp fail(%d)\n", ret);
		return ret;
	}
	/* VBATOVP */
	vbatovp = __dv2_get_vbatovp(info, ita);
	ret = prop_chgalgo_set_vbatovp(data->pca_dvchg, vbatovp);
	if (ret < 0) {
		PCA_ERR("set vbatovp fail(%d)\n", ret);
		return ret;
	}
	/* IBATOCP */
	ibatocp = __dv2_get_ibatocp(info, ita);
	ret = prop_chgalgo_set_ibatocp(data->pca_dvchg, ibatocp);
	if (ret < 0) {
		PCA_ERR("set ibatocp fail(%d)\n", ret);
		return ret;
	}
	data->vbusovp = vbusovp;
	data->ibusocp = ibusocp;
	data->vbatovp = vbatovp;
	PCA_INFO("vbusovp,ibusocp,vbatovp,ibatocp = (%d,%d,%d,%d)\n",
		 vbusovp, ibusocp, vbatovp, ibatocp);
	return 0;
}

/*
 * Set protection parameters and enable divider charger
 *
 * @en: enable/disable
 */
static int __dv2_enable_dvchg_charging(struct dv2_algo_info *info, bool en)
{
	int ret;
	struct dv2_algo_data *data = info->data;

	PCA_INFO("en = %d\n", en);

	if (en) {
		ret = prop_chgalgo_enable_hz(data->pca_swchg, true);
		if (ret < 0) {
			PCA_ERR("set swchg hz fail(%d)\n", ret);
			return ret;
		}

		ret = __dv2_set_dvchg_protection(info);
		if (ret < 0) {
			PCA_ERR("set protection fail(%d)\n", ret);
			return ret;
		}
	}

	ret = prop_chgalgo_enable_charging(data->pca_dvchg, en);
	if (ret < 0) {
		PCA_ERR("en chg fail(%d)\n", ret);
		return ret;
	}
	data->is_dvchg_en = en;

	if (!en) {
		ret = prop_chgalgo_enable_hz(data->pca_swchg, false);
		if (ret < 0) {
			PCA_ERR("disable swchg hz fail(%d)\n", ret);
			return ret;
		}
	}
	return 0;
}

/*
 * Enable charging of switching charger
 * For divide by two algorithm, according to swchg_ichg to decide enable or not
 *
 * @en: enable/disable
 */
static int __dv2_enable_swchg_charging(struct dv2_algo_info *info, bool en)
{
	int ret;
	struct dv2_algo_data *data = info->data;

	PCA_INFO("en = %d\n", en);
	if (en) {
		ret = prop_chgalgo_enable_charging(data->pca_swchg, true);
		if (ret < 0) {
			PCA_ERR("en swchg fail(%d)\n", ret);
			return ret;
		}
		ret = prop_chgalgo_enable_hz(data->pca_swchg, false);
		if (ret < 0) {
			PCA_ERR("disable hz fail(%d)\n", ret);
			return ret;
		}
	} else {
		ret = prop_chgalgo_enable_hz(data->pca_swchg, true);
		if (ret < 0) {
			PCA_ERR("disable hz fail(%d)\n", ret);
			return ret;
		}
		ret = prop_chgalgo_enable_charging(data->pca_swchg, false);
		if (ret < 0) {
			PCA_ERR("en swchg fail(%d)\n", ret);
			return ret;
		}
	}
	data->is_swchg_en = en;
	return 0;
}

/*
 * Set AICR & ICHG of switching charger
 *
 * @aicr: setting of AICR
 * @ichg: setting of ICHG
 */
static int __dv2_set_swchg_cap(struct dv2_algo_info *info, u32 aicr, u32 ichg)
{
	int ret;
	struct dv2_algo_data *data = info->data;

	ret = prop_chgalgo_set_aicr(data->pca_swchg, aicr);
	if (ret < 0) {
		PCA_ERR("set aicr fail\n");
		return ret;
	}
	data->aicr_setting = aicr;
	ret = prop_chgalgo_set_ichg(data->pca_swchg, ichg);
	if (ret < 0) {
		PCA_ERR("set_ichg fail\n");
		return ret;
	}
	data->ichg_setting = ichg;
	PCA_INFO("AICR = %d, ICHG = %d\n", aicr, ichg);
	return 0;
}

/*
 * Enable TA by algo
 *
 * @en: enable/disable
 * @mV: requested output voltage
 * @mA: requested output current
 */
static int __dv2_enable_ta_charging(struct dv2_algo_info *info, bool en, int mV,
				    int mA)
{
	int ret;
	struct dv2_algo_data *data = info->data;
	struct dv2_algo_desc *desc = info->desc;
	u32 wdt = desc->polling_interval * 3;

	PCA_INFO("en = %d\n", en);
	if (en) {
		ret = prop_chgalgo_set_ta_wdt(data->pca_ta, wdt);
		if (ret < 0)
			return ret;
		ret = prop_chgalgo_enable_ta_wdt(data->pca_ta, true);
		if (ret < 0)
			return ret;
	}
	ret = prop_chgalgo_enable_ta_charging(data->pca_ta, en, mV, mA);
	if (ret < 0)
		return ret;
	if (!en)
		ret = prop_chgalgo_enable_ta_wdt(data->pca_ta, false);
	return ret;
}

/*
 * Stop dv2 charging and reset parameter
 *
 * @reset_ta: set output voltage/current of TA to 5V/3A and disable
 *            direct charge
 * @hrst: send hardreset to port partner
 */
static int __dv2_end(struct dv2_algo_info *info, bool reset_ta, bool hrst)
{
	struct dv2_algo_data *data = info->data;

	if (data->state == DV2_ALGO_STOP) {
		PCA_INFO("already stop\n");
		return 0;
	}

	PCA_INFO("reset ta(%d), hardreset(%d)\n", reset_ta, hrst);

	data->vta_measure = 0;
	data->ita_measure = 0;
	data->tta_measure = 0;
	data->state = DV2_ALGO_STOP;

	__dv2_enable_dvchg_charging(info, false);
	if (reset_ta) {
		__dv2_set_ta_cap(info, 5000, 3000);
		__dv2_enable_ta_charging(info, false, 5000, 3000);
		if (hrst)
			prop_chgalgo_send_ta_hardreset(data->pca_ta);
	}
	return 0;
}

#define DV2_SWCHG_OFF_THRESHOLD	(100) /* mV */
static inline void __dv2_init_algo_data(struct dv2_algo_info *info)
{
	struct dv2_algo_data *data = info->data;
	struct dv2_algo_desc *desc = info->desc;

	data->ita_lmt =
		(desc->ita_level[DV2_RCABLE_NORMAL] < data->ita_boundary_max) ?
		desc->ita_level[DV2_RCABLE_NORMAL] : data->ita_boundary_max;
	data->ita_lmt_by_r = data->ita_lmt;
	data->idvchg_cc = desc->ita_level[DV2_RCABLE_NORMAL] - desc->swchg_aicr;
	data->is_swchg_en = false;
	data->aicr_setting = 0;
	data->ichg_setting = 0;
	data->is_vbat_over_cv = false;
	data->cv_lower_bound = desc->vbat_upper_bound - 15;
	data->cv_readd_ita_cnt = 0;
	data->r_bat = desc->r_bat_min;
	data->r_sw = desc->r_sw_min;
	data->r_cable = desc->rcable_level[DV2_RCABLE_NORMAL];
	data->tbat_level = DV2_TBAT_NORMAL;
	data->tta_level = DV2_TTA_NORMAL;
	data->tdvchg_level = DV2_TDVCHG_NORMAL;
	data->run_once = true;
	data->state = DV2_ALGO_INIT;
	data->vbat_swchg_off = desc->vbat_upper_bound - DV2_SWCHG_OFF_THRESHOLD;
	mutex_lock(&data->tcp_lock);
	data->tcp_evt = 0;
	mutex_unlock(&data->tcp_lock);
	get_monotonic_boottime(&data->stime);
}

/*
 * Start dv2 algo timer and run algo
 * It cannot start algo again if algo has been started once before
 * Run once flag will be reset after TA is plugged out
 */
static inline int __dv2_start(struct dv2_algo_info *info)
{
	int ret;
	struct dv2_algo_data *data = info->data;
	struct dv2_algo_desc *desc = info->desc;
	ktime_t ktime = ktime_set(0, MS_TO_NS(desc->polling_interval));

	PCA_INFO("++\n");

	if (data->run_once) {
		PCA_ERR("already run dv2 algo once\n");
		return -EINVAL;
	}

	/* disable charger */
	ret = prop_chgalgo_enable_charging(data->pca_swchg, false);
	if (ret < 0) {
		PCA_ERR("disable charger fail\n");
		return ret;
	}
	msleep(500); /* wait for battery to recovery */

	__dv2_init_algo_data(info);
	alarm_start_relative(&data->timer, ktime);
	return 0;
}

/*
 * DV2 algorithm initial state
 * It does foreign object detection
 */
static int __dv2_algo_init(struct dv2_algo_info *info)
{
	int ret, i, vbus, ita_avg = 0, vta_avg = 0, vbus_avg = 0;
	const int avg_times = 10;
	bool hrst = false;
	u32 vta;
	struct dv2_algo_data *data = info->data;
	struct dv2_algo_desc *desc = info->desc;

	PCA_INFO("++\n");

	/* Change charging policy first */
	ret = __dv2_enable_ta_charging(info, true, 5000, 3000);
	if (ret < 0) {
		PCA_ERR("enable ta direct charge fail\n");
		hrst = true;
		goto err;
	}

	/* Foreign Object Detection (FOD) */
	ret = prop_chgalgo_enable_power_path(data->pca_swchg, false);
	if (ret < 0) {
		PCA_ERR("disable power path fail\n");
		goto err;
	}
	msleep(20); /* Wait current stable */

	for (i = 0; i < avg_times; i++) {
		ret = __dv2_get_ta_cap(info);
		if (ret < 0) {
			PCA_ERR("get ta current cap fail\n");
			hrst = true;
			goto err_en_pp;
		}
		ret = __dv2_get_adc(info, PCA_ADCCHAN_VBUS, &vbus, &vbus);
		if (ret < 0) {
			PCA_ERR("get vbus fail\n");
			goto err_en_pp;
		}
		ita_avg += data->ita_measure;
		vta_avg += data->vta_measure;
		vbus_avg += vbus;
	}
	ita_avg /= avg_times;
	vta_avg /= avg_times;
	vbus_avg /= avg_times;

	ret = prop_chgalgo_enable_power_path(data->pca_swchg, true);
	if (ret < 0) {
		PCA_ERR("en power path fail\n");
		goto err_en_pp;
	}

	/* vbus calibration: voltage difference between TA & device */
	data->vbus_cali = vta_avg - vbus_avg;
	if (abs(data->vbus_cali) > 100) {
		PCA_ERR("vbus cali error(%d)\n", data->vbus_cali);
		goto err;
	}
	PCA_INFO("avg(ita,vta,vbus):(%d, %d, %d), vbus cali:%d\n", ita_avg,
		 vta_avg, vbus_avg, data->vbus_cali);

	if (ita_avg > desc->fod_current) {
		PCA_ERR("foreign object detected\n");
		goto err;
	}

	__dv2_get_cali_vta(info, desc->idvchg_ss_init, &vta);
	ret = __dv2_set_ta_cap(info, vta, desc->idvchg_ss_init);
	if (ret < 0) {
		PCA_ERR("set ta cap by algo fail(%d)\n", ret);
		hrst = true;
		goto err;
	}
	ret = __dv2_enable_dvchg_charging(info, true);
	if (ret < 0) {
		PCA_ERR("en dvchg fail\n");
		goto err;
	}
	data->state = DV2_ALGO_MEASURE_R;
	return 0;

err_en_pp:
	prop_chgalgo_enable_power_path(data->pca_swchg, true);
err:
	return __dv2_end(info, true, hrst);
}

struct meas_r_info {
	u32 vbus;
	u32 vbat;
	u32 bifvbat;
	u32 ibat;
	u32 ita;
	u32 vta;
	u32 vout;
};

static int __dv2_algo_get_r_info(struct dv2_algo_info *info, u32 ita,
				 struct meas_r_info *r_info)
{
	int ret;
	u32 vta;
	struct dv2_algo_data *data = info->data;

	memset(r_info, 0, sizeof(struct meas_r_info));
	__dv2_get_cali_vta(info, ita, &vta);
	ret = __dv2_set_ta_cap(info, vta, ita);
	if (ret < 0) {
		PCA_ERR("set ta cap fail(%d)\n", ret);
		return ret;
	}

	ret = __dv2_get_adc(info, PCA_ADCCHAN_VBUS, &r_info->vbus,
			    &r_info->vbus);
	if (ret < 0) {
		PCA_ERR("get vbus fail\n");
		return ret;
	}
	ret = __dv2_get_adc(info, PCA_ADCCHAN_VBAT, &r_info->vbat,
			    &r_info->vbat);
	if (ret < 0) {
		PCA_ERR("get vbat fail\n");
		return ret;
	}
	if (data->is_bif_exist) {
		ret = __dv2_get_adc(info, PCA_ADCCHAN_BIFVBAT, &r_info->bifvbat,
				    &r_info->bifvbat);
		if (ret < 0) {
			PCA_ERR("get bif vbat fail\n");
			return ret;
		}
	}
	ret = __dv2_get_adc(info, PCA_ADCCHAN_IBAT, &r_info->ibat,
			    &r_info->ibat);
	if (ret < 0) {
		PCA_ERR("get ibat fail\n");
		return ret;
	}
#if 0 /* TODO: RT9759 ADC */
	ret = __dv2_get_adc(info, PCA_ADCCHAN_VOUT, &r_info->vout,
			    &r_info->vout);
	if (ret < 0) {
		PCA_ERR("get vout fail\n");
		return ret;
	}
#endif

	ret = __dv2_get_ta_cap(info);
	if (ret < 0) {
		PCA_ERR("get ta cap fail(%d)\n", ret);
		return ret;
	}
	r_info->vta = data->vta_measure;
	r_info->ita = data->ita_measure;
	return 0;
}

static int __dv2_algo_measure_r(struct dv2_algo_info *info)
{
	int ret;
	bool hrst = false;
	struct dv2_algo_data *data = info->data;
	struct dv2_algo_desc *desc = info->desc;
	struct meas_r_info r_info[2];
	int r_cable_ref[2] = {0};
	u32 ita_lmt;

	PCA_INFO("++\n");

	/* measure 1 */
	ret = __dv2_algo_get_r_info(info, desc->idvchg_ss_init, &r_info[0]);
	if (ret < 0) {
		PCA_ERR("meas 1 get r info fail(%d)\n", ret);
		hrst = true;
		goto err;
	}

	/* BIF VBAT >= bat_upper_bound */
	if (data->is_bif_exist && r_info[0].bifvbat != 0 &&
	    r_info[0].bifvbat >= desc->vbat_upper_bound) {
		PCA_INFO("mea1 vbif(%d) >= CV(%d), ita:%d, vta:%d\n",
			 r_info[0].bifvbat, desc->vbat_upper_bound,
			 r_info[0].ita, r_info[0].vta);
		goto err;
	}

	/* measure 2 */
	ret = __dv2_algo_get_r_info(info, desc->idvchg_ss_init + 250, &r_info[1]);
	if (ret < 0) {
		PCA_ERR("meas 2 get r info fail(%d)\n", ret);
		hrst = true;
		goto err;
	}

	/* BIF VBAT >= bat_upper_bound */
	if (data->is_bif_exist && r_info[1].bifvbat != 0 &&
	    r_info[1].bifvbat >= desc->vbat_upper_bound) {
		PCA_INFO("mea2 vbif(%d) >= CV(%d), ita:%d, vta:%d\n",
			 r_info[1].bifvbat, desc->vbat_upper_bound,
			 r_info[1].ita, r_info[1].vta);
		goto err;
	}

	/* TA does not support different current level */
	if (r_info[0].ita == r_info[1].ita) {
		PCA_ERR("ita1(%d) = ita2(%d)\n", r_info[0].ita, r_info[1].ita);
		hrst = true;
		goto err;
	}

	/* use bifvbat to calculate r_bat */
	if (data->is_bif_exist && r_info[0].bifvbat != 0 &&
	    r_info[1].bifvbat != 0)
		data->r_bat = abs((r_info[1].vbat - r_info[1].bifvbat) -
				   (r_info[0].vbat - r_info[0].bifvbat)) *
				   1000 / abs(r_info[1].ibat - r_info[0].ibat);
	if (data->r_bat < desc->r_bat_min)
		data->r_bat = desc->r_bat_min;

#if 0 /* TODO: RT9759 ADC */
	data->r_sw = abs((r_info[1].vout - r_info[1].vbat) -
			 (r_info[0].vout - r_info[0].vbat)) * 1000 /
			 abs(r_info[1].ibat - r_info[0].ibat);
	if (data->r_sw < desc->r_sw_min)
		data->r_sw = desc->r_sw_min;
#endif

	/* Use absolute instead of relative calculation */
	data->r_cable = abs(r_info[1].vta - data->vbus_cali - r_info[1].vbus)
			    * 1000 / abs(r_info[1].ita);

	/*
	 * Following two resistences are for reference
	 * (r_cable_ref[0] & r_cable_ref[1])
	 */
	r_cable_ref[0] = abs(r_info[0].vta - data->vbus_cali - r_info[0].vbus)
			     * 1000 / abs(r_info[0].ita);

	/* Relative calculation might have larger variation */
	r_cable_ref[1] = abs((r_info[1].vta - r_info[0].vta) -
			     (r_info[1].vbus - r_info[0].vbus)) *
			     1000 / abs(r_info[1].ita - r_info[0].ita);

	data->r_total = data->r_bat + data->r_sw + data->r_cable;

	PCA_INFO("ita:%d %d, vta:%d %d, vbus:%d %d, vout %d %d\n",
		 r_info[0].ita, r_info[1].ita, r_info[0].vta, r_info[1].vta,
		 r_info[0].vbus, r_info[1].vbus, r_info[0].vout,
		 r_info[1].vout);

	PCA_INFO("vbat:%d %d, bifvbat:%d %d, ibat:%d %d\n",
		 r_info[0].vbat, r_info[1].vbat, r_info[0].bifvbat,
		 r_info[1].bifvbat, r_info[0].ibat, r_info[1].ibat);

	PCA_INFO("r_sw:%d, r_bat:%d, r_cable:%d,%d,%d, r_total:%d\n",
		 data->r_sw, data->r_bat, data->r_cable, r_cable_ref[0],
		 r_cable_ref[1], data->r_total);

	if (data->r_cable < desc->rcable_level[DV2_RCABLE_NORMAL])
		data->ita_lmt_by_r = desc->ita_level[DV2_RCABLE_NORMAL];
	else if (data->r_cable < desc->rcable_level[DV2_RCABLE_BAD1])
		data->ita_lmt_by_r = desc->ita_level[DV2_RCABLE_BAD1];
	else if (data->r_cable < desc->rcable_level[DV2_RCABLE_BAD2])
		data->ita_lmt_by_r = desc->ita_level[DV2_RCABLE_BAD2];
	else if (data->r_cable < desc->rcable_level[DV2_RCABLE_BAD3])
		data->ita_lmt_by_r = desc->ita_level[DV2_RCABLE_BAD3];
	else {
		PCA_ERR("r_cable(%d) too worse\n", data->r_cable);
		hrst = false;
		goto err;
	}
	PCA_INFO("ita limited by r = %d\n", data->ita_lmt_by_r);
	data->ita_lmt = MIN(data->ita_lmt_by_r, data->ita_lmt);

	ita_lmt = __dv2_get_ita_lmt(info);
	if (ita_lmt < desc->idvchg_term) {
		PCA_ERR("ita_lmt(%d) < dvchg_term(%d)\n", ita_lmt,
			desc->idvchg_term);
		goto err;
	}

	data->state = DV2_ALGO_SS_DVCHG;
	return 0;

err:
	return __dv2_end(info, true, hrst);
}

static int __dv2_algo_ss_dvchg(struct dv2_algo_info *info)
{
	int ret, vbat;
	bool hrst = true;
	u32 ita, vta, ita_lmt, idvchg_lmt;
	struct dv2_algo_data *data = info->data;
	struct dv2_algo_desc *desc = info->desc;

	PCA_INFO("++\n");

	ret = __dv2_get_ta_cap(info);
	if (ret < 0) {
		PCA_ERR("get ta measure cap fail(%d)\n", ret);
		goto err;
	}
	ret = __dv2_get_adc(info, data->is_bif_exist ?
			    PCA_ADCCHAN_BIFVBAT : PCA_ADCCHAN_VBAT, &vbat,
			    &vbat);
	if (ret < 0) {
		PCA_ERR("get vbat fail\n");
		goto err;
	}
	PCA_INFO("vbat = %d, vta,ita(set,meas) = (%d,%d,%d,%d)\n", vbat,
		 data->vta_setting, data->vta_measure, data->ita_setting,
		 data->ita_measure);

	if (vbat >= desc->vbat_upper_bound) {
		if (data->ita_measure < desc->idvchg_term) {
			PCA_INFO("dv2 end(vbat = %d, ita = %d, iterm = %d)\n",
				 vbat, data->ita_measure, desc->idvchg_term);
			hrst = false;
			goto err;
		}
		/* set new vchg & ichg */
		ita = data->ita_setting - desc->idvchg_ss_step;
		ret = __dv2_get_cali_vta(info, ita, &vta);
		if (ret < 0) {
			PCA_ERR("get cali vta fail(%d)\n", ret);
			goto err;
		}
		ret = __dv2_set_ta_cap(info, vta, ita);
		if (ret < 0) {
			PCA_ERR("set ta cap fail(%d)\n", ret);
			goto err;
		}
		data->state = DV2_ALGO_CC_CV;
		goto out;
	}

	ita_lmt = __dv2_get_ita_lmt(info);
	idvchg_lmt = MIN(ita_lmt, data->idvchg_cc);
	if (data->ita_measure >= idvchg_lmt ||
	    data->ita_setting >= idvchg_lmt) {
		if (vbat < data->vbat_swchg_off && desc->swchg_aicr > 0 &&
		    desc->swchg_ichg > 0 &&
		    (ita_lmt > data->idvchg_cc + desc->swchg_aicr_ss_init)) {
			ret = __dv2_set_swchg_cap(info,
						  desc->swchg_aicr_ss_init,
						  desc->swchg_ichg);
			if (ret < 0)
				goto err;
			ret = __dv2_enable_swchg_charging(info, true);
			if (ret < 0) {
				PCA_ERR("en swchg fail(%d)\n", ret);
				goto err;
			}
			ita = data->ita_setting + desc->swchg_aicr_ss_init;
			ret = __dv2_get_cali_vta(info, ita, &vta);
			if (ret < 0)
				goto err;
			ret = __dv2_set_ta_cap(info, vta, ita);
			if (ret < 0)
				goto err;
			data->aicr_lmt = ita_lmt - data->idvchg_cc;
			data->state = DV2_ALGO_SS_SWCHG;
			goto out;
		}
		data->state = DV2_ALGO_CC_CV;
		goto out;
	}

	if (vbat < desc->idvchg_ss_step1_vbat)
		ita = data->ita_setting + desc->idvchg_ss_step;
	else if (vbat < desc->idvchg_ss_step2_vbat)
		ita = data->ita_setting + desc->idvchg_ss_step1;
	else
		ita = data->ita_setting + desc->idvchg_ss_step2;
	if (ita >= idvchg_lmt)
		ita = idvchg_lmt;

	ret = __dv2_get_cali_vta(info, ita, &vta);
	if (ret < 0) {
		PCA_ERR("get cali vta fail(%d)\n", ret);
		goto err;
	}
	ret = __dv2_set_ta_cap(info, vta, ita);
	if (ret < 0) {
		PCA_ERR("set ta cap fail(%d)\n", ret);
		goto err;
	}
out:
	PCA_INFO("vta,ita(new setting):(%d,%d)\n", data->vta_setting,
		 data->ita_setting);
	return 0;
err:
	return __dv2_end(info, true, hrst);
}

static int __dv2_algo_ss_swchg(struct dv2_algo_info *info)
{
	int ret;
	struct dv2_algo_desc *desc = info->desc;
	struct dv2_algo_data *data = info->data;
	bool hrst = false;
	u32 vta, ita, aicr, ibus, vbat;

	PCA_INFO("++\n");

	ret = __dv2_get_adc(info, data->is_bif_exist ?
			    PCA_ADCCHAN_BIFVBAT : PCA_ADCCHAN_VBAT, &vbat,
			    &vbat);
	if (ret < 0) {
		PCA_ERR("get vbat fail\n");
		goto err;
	}
	ret = __dv2_get_ta_cap(info);
	if (ret < 0)
		goto err;
	ret = prop_chgalgo_get_adc(data->pca_swchg, PCA_ADCCHAN_IBUS, &ibus,
				   &ibus);
	if (ret < 0)
		goto err;
	PCA_INFO("vta,ita(set,meas) = (%d,%d,%d,%d), vbat,ibus = (%d,%d)\n",
		 data->vta_setting, data->vta_measure, data->ita_setting,
		 data->ita_measure, vbat, ibus);
	aicr = data->aicr_setting + desc->swchg_aicr_ss_step;
	if (aicr >= data->aicr_lmt)
		aicr = data->aicr_lmt;
	ita = data->ita_setting + (aicr - data->aicr_setting);
	ret = __dv2_set_swchg_cap(info, aicr, desc->swchg_ichg);
	if (ret < 0)
		goto err;
	ret = __dv2_get_cali_vta(info, ita, &vta);
	if (ret < 0)
		goto err;
	ret = __dv2_set_ta_cap(info, vta, ita);
	if (ret < 0) {
		hrst = true;
		goto err;
	}
	ret = __dv2_get_adc(info, data->is_bif_exist ?
			    PCA_ADCCHAN_BIFVBAT : PCA_ADCCHAN_VBAT, &vbat,
			    &vbat);
	if (ret < 0) {
		PCA_ERR("get vbat fail\n");
		goto err;
	}
	if (vbat >= data->vbat_swchg_off) {
		aicr = data->aicr_setting - desc->swchg_aicr_ss_step;
		if (aicr > desc->swchg_aicr_ss_init)
			ita = data->ita_setting - desc->swchg_aicr_ss_step;
		else
			ita = data->ita_setting - data->aicr_setting;
		ret = __dv2_get_cali_vta(info, ita, &vta);
		if (ret < 0)
			goto err;
		ret = __dv2_set_ta_cap(info, vta, ita);
		if (ret < 0) {
			hrst = true;
			goto err;
		}
		if (aicr > desc->swchg_aicr_ss_init)
			ret = __dv2_set_swchg_cap(info, aicr, desc->swchg_ichg);
		else
			ret = __dv2_enable_swchg_charging(info, false);
		if (ret < 0)
			goto err;
		data->state = DV2_ALGO_CC_CV;
	} else if (aicr >= data->aicr_lmt)
		data->state = DV2_ALGO_CC_CV;
	PCA_INFO("vta,ita(new setting):(%d,%d), aicr = %d\n", data->vta_setting,
		 data->ita_setting, data->aicr_setting);
	return 0;
err:
	return __dv2_end(info, true, hrst);
}

static int __dv2_algo_cc_cv(struct dv2_algo_info *info)
{
	int ret, vbat = 0;
	bool hrst = true;
	struct dv2_algo_data *data = info->data;
	struct dv2_algo_desc *desc = info->desc;
	u32 vta, ita = data->ita_setting, ita_lmt, aicr;

	PCA_INFO("++\n");

	ret = __dv2_get_adc(info, data->is_bif_exist ?
			    PCA_ADCCHAN_BIFVBAT : PCA_ADCCHAN_VBAT, &vbat,
			    &vbat);
	if (ret < 0)
		PCA_ERR("get vbat fail\n");

	if (vbat >= data->vbat_swchg_off && data->is_swchg_en) {
		aicr = data->aicr_setting - desc->swchg_aicr_ss_step;
		if (aicr > desc->swchg_aicr_ss_init)
			ita = data->ita_setting - desc->swchg_aicr_ss_step;
		else
			ita = data->ita_setting - data->aicr_setting;
		ret = __dv2_get_cali_vta(info, ita, &vta);
		if (ret < 0)
			goto err;
		ret = __dv2_set_ta_cap(info, vta, ita);
		if (ret < 0) {
			hrst = true;
			goto err;
		}
		if (aicr > desc->swchg_aicr_ss_init)
			ret = __dv2_set_swchg_cap(info, aicr, desc->swchg_ichg);
		else
			ret = __dv2_enable_swchg_charging(info, false);
		if (ret < 0)
			goto err;
	}

	ret = __dv2_get_ta_cap(info);
	if (ret < 0) {
		PCA_ERR("get ta cap fail(%d)\n", ret);
		goto err;
	}
	PCA_INFO("vbat:%d, vta,ita(set,meas) = (%d,%d,%d,%d), r:%d\n",
		 vbat, data->vta_setting, data->vta_measure, data->ita_setting,
		 data->ita_measure, data->r_total);

	if (data->ita_measure <= desc->idvchg_term) {
		PCA_INFO("finish, ita = %dmA\n", data->ita_measure);
		hrst = false;
		return __dv2_end(info, true, hrst);
	}

	if (data->is_vbat_over_cv) {
		data->cv_lower_bound = vbat - 5;
		data->cv_readd_ita_cnt = 0;
		data->is_vbat_over_cv = false;
	}

	if (vbat >= desc->vbat_upper_bound) {
		ita = data->ita_setting - desc->idvchg_step;
		data->is_vbat_over_cv = true;
	} else if (vbat <= data->cv_lower_bound) {
		if (data->cv_readd_ita_cnt > 5)
			ita = data->ita_setting + desc->idvchg_step;
		else
			data->cv_readd_ita_cnt++;
	}

	ita_lmt = __dv2_get_ita_lmt(info);
	ita_lmt = MIN(ita_lmt, data->is_swchg_en ?
		      (data->idvchg_cc + data->aicr_setting) : data->idvchg_cc);
	if (ita > ita_lmt) {
		ita = ita_lmt;
		if (ita < desc->idvchg_term) {
			PCA_INFO("ita_lmt(%d) < idvchg_term(%d)\n", ita,
				 desc->idvchg_term);
			hrst = false;
			goto err;
		}
	}

	ret = __dv2_get_cali_vta(info, ita, &vta);
	if (ret < 0) {
		PCA_ERR("get cali vta fail(%d)\n", ret);
		goto err;
	}
	ret = __dv2_set_ta_cap(info, vta, ita);
	if (ret < 0) {
		PCA_ERR("set_ta_cap fail(%d)\n", ret);
		goto err;
	}

	PCA_INFO("vta,ita(new setting):(%d,%d)\n", data->vta_setting,
		 data->ita_setting);
	return 0;
err:
	return __dv2_end(info, true, hrst);
}

static bool __dv2_algo_safety_check(struct dv2_algo_info *info)
{
	bool hrst = false;

	PCA_INFO("++\n");

	if (!__dv2_check_ta_status(info)) {
		PCA_ERR("check TA status fail\n");
		hrst = true;
		goto err;
	}

	if (!__dv2_check_dvchg_vbusovp(info)) {
		PCA_ERR("check vbusov fail\n");
		goto err;
	}

	if (!__dv2_check_dvchg_ibusocp(info)) {
		PCA_ERR("check chg ibusoc fail\n");
		goto err;
	}

	if (!__dv2_check_ta_ibusocp(info)) {
		PCA_ERR("check ta ibusoc fail\n");
		hrst = true;
		goto err;
	}

	if (!__dv2_check_vbatovp(info)) {
		PCA_ERR("check vbatov fail\n");
		goto err;
	}

	if (!__dv2_check_ibatocp(info)) {
		PCA_ERR("check vbatov fail\n");
		goto err;
	}

	if (!__dv2_check_tbat_level(info)) {
		PCA_ERR("check tbat level fail\n");
		goto err;
	}

	if (!__dv2_check_tta_level(info)) {
		PCA_ERR("check tta level fail\n");
		hrst = true;
		goto err;
	}

	if (!__dv2_check_tdvchg_level(info)) {
		PCA_ERR("check tchg level fail\n");
		goto err;
	}
	return true;

err:
	__dv2_end(info, true, hrst);
	return false;
}

static bool __is_ta_rdy(struct dv2_algo_info *info)
{
	int ret, i;
	struct dv2_algo_desc *desc = info->desc;
	struct dv2_algo_data *data = info->data;
	struct prop_chgalgo_ta_auth_data auth_data = {
		.cap_vmin = desc->ta_cap_vmin,
		.cap_vmax = desc->ta_cap_vmax,
		.cap_imin = desc->ta_cap_imin,
	};

	for (i = 0; i < DV2_TATYP_MAX; i++) {
		if (!data->pca_ta_pool[i])
			continue;
		ret = prop_chgalgo_authenticate_ta(data->pca_ta_pool[i],
						   &auth_data);
		if (ret >= 0) {
			data->vta_boundary_min = auth_data.ta_vmin;
			data->vta_boundary_max = auth_data.ta_vmax;
			data->ita_boundary_max = auth_data.ta_imax;
			data->pca_ta = data->pca_ta_pool[i];
			PCA_INFO("using %s\n", __dv2_ta_name[i]);
			return true;
		}
		PCA_ERR("%s authentication fail(%d)\n", __dv2_ta_name[i], ret);
	}
	return false;
}

static enum alarmtimer_restart
__dv2_algo_timer_cb(struct alarm *alarm, ktime_t now)
{
	struct dv2_algo_data *data =
		container_of(alarm, struct dv2_algo_data, timer);

	PCA_DBG("++\n");
	if (!wake_up_process(data->task))
		PCA_ERR("wakeup thread fail\n");
	return ALARMTIMER_NORESTART;
}

/*
 * Check charging time of dv2 algorithm
 * return false if timeout otherwise return true
 */
static bool __dv2_algo_check_charging_time(struct dv2_algo_info *info)
{
	struct dv2_algo_data *data = info->data;
	struct dv2_algo_desc *desc = info->desc;
	struct timespec etime, dtime;

	get_monotonic_boottime(&etime);
	dtime = timespec_sub(etime, data->stime);
	if (dtime.tv_sec >= desc->chg_time_max) {
		PCA_ERR("dv2 algo timeout(%d, %d)\n", (int)dtime.tv_sec,
			desc->chg_time_max);
		__dv2_end(info, true, false);
		return false;
	}
	return true;
}

static int __dv2_tcp_notify_hardreset_cb(struct dv2_algo_info *info)
{
	struct dv2_algo_data *data = info->data;

	PCA_INFO("++\n");
	data->check_once = false;
	data->run_once = false;
	return __dv2_end(info, false, false);
}

static int __dv2_tcp_notify_detach_cb(struct dv2_algo_info *info)
{
	struct dv2_algo_data *data = info->data;

	PCA_INFO("++\n");
	data->check_once = false;
	data->run_once = false;
	return __dv2_end(info, true, false);
}

enum dv2_tcp_notify_evt {
	DV2_TCP_NOTIFY_HARDRESET = 0,
	DV2_TCP_NOTIFY_DETACH,
	DV2_TCP_NOTIFY_MAX,
};

static int
(*__dv2_tcp_notify_cb[DV2_TCP_NOTIFY_MAX])(struct dv2_algo_info *info) = {
	[DV2_TCP_NOTIFY_HARDRESET] = __dv2_tcp_notify_hardreset_cb,
	[DV2_TCP_NOTIFY_DETACH] = __dv2_tcp_notify_detach_cb,
};

static int __dv2_handle_tcp_evt(struct dv2_algo_info *info)
{
	int i;
	struct dv2_algo_data *data = info->data;

	mutex_lock(&data->tcp_lock);
	for (i = 0; i < DV2_TCP_NOTIFY_MAX; i++) {
		if ((data->tcp_evt & BIT(i)) && __dv2_tcp_notify_cb[i])
			__dv2_tcp_notify_cb[i](info);
	}
	data->tcp_evt = 0;
	mutex_unlock(&data->tcp_lock);
	return 0;
}

static int __dv2_algo_threadfn(void *param)
{
	struct dv2_algo_info *info = param;
	struct dv2_algo_data *data = info->data;
	struct dv2_algo_desc *desc = info->desc;
	ktime_t ktime = ktime_set(0, MS_TO_NS(desc->polling_interval));

	while (!kthread_should_stop()) {
		pm_stay_awake(info->dev);
		mutex_lock(&data->lock);
		__dv2_algo_check_charging_time(info);
		switch (data->state) {
		case DV2_ALGO_INIT:
			__dv2_algo_init(info);
			break;
		case DV2_ALGO_MEASURE_R:
			__dv2_algo_measure_r(info);
			break;
		case DV2_ALGO_SS_SWCHG:
			__dv2_algo_ss_swchg(info);
			break;
		case DV2_ALGO_SS_DVCHG:
			__dv2_algo_ss_dvchg(info);
			break;
		case DV2_ALGO_CC_CV:
			__dv2_algo_cc_cv(info);
			break;
		case DV2_ALGO_STOP:
			PCA_INFO("PPS ALGO STOP\n");
			break;
		default:
			PCA_ERR("NO SUCH STATE\n");
			break;
		}

		__dv2_handle_tcp_evt(info);
		if (data->state != DV2_ALGO_STOP) {
			__dv2_algo_safety_check(info);
			alarm_start_relative(&data->timer, ktime);
		}
		mutex_unlock(&data->lock);
		pm_relax(info->dev);
		set_current_state(TASK_INTERRUPTIBLE);
		schedule();
	}
	return 0;
}

static inline int __dv2_plugout_reset(struct dv2_algo_info *info)
{
	struct dv2_algo_data *data = info->data;

	PCA_INFO("++\n");

	/* reset pps data */
	data->check_once = false;
	data->run_once = false;
	return __dv2_end(info, true, false);
}

static int __dv2_tcp_notifier_call(struct notifier_block *nb,
				   unsigned long event, void *notify)
{
	struct tcp_notify *noti = notify;
	struct prop_chgalgo_device *pca_dv2 =
		prop_chgalgo_dev_get_by_name("pca_algo_dv2");
	struct dv2_algo_info *info = NULL;
	struct dv2_algo_data *data = NULL;

	if (!pca_dv2)
		return NOTIFY_OK;
	info = prop_chgalgo_get_drvdata(pca_dv2);
	if (!info)
		return NOTIFY_OK;
	data = info->data;

	mutex_lock(&data->tcp_lock);
	switch (event) {
	case TCP_NOTIFY_PD_STATE:
		switch (noti->pd_state.connected) {
		case PD_CONNECT_NONE:
			PCA_INFO("detached\n");
			data->tcp_evt |= BIT(DV2_TCP_NOTIFY_DETACH);
			break;
		case PD_CONNECT_HARD_RESET:
			PCA_INFO("hardreset received\n");
			data->tcp_evt |= BIT(DV2_TCP_NOTIFY_HARDRESET);
			break;
		default:
			break;
		}
	default:
		break;
	}
	mutex_unlock(&data->tcp_lock);
	return NOTIFY_OK;
}

/* =================================================================== */
/* DV2 Algo OPS                                                        */
/* =================================================================== */

static int dv2_init_algo(struct prop_chgalgo_device *pca)
{
	int ret = 0, i;
	struct dv2_algo_info *info = prop_chgalgo_get_drvdata(pca);
	struct dv2_algo_data *data = info->data;
	bool has_ta = false;

	PCA_INFO("++\n");

	mutex_lock(&data->lock);
	if (data->inited) {
		PCA_INFO("already inited\n");
		goto out;
	}

	/* get ta & chg pps */
	for (i = 0; i < DV2_TATYP_MAX; i++) {
		data->pca_ta_pool[i] = prop_chgalgo_dev_get_by_name(
			__dv2_ta_name[i]);
		if (!data->pca_ta_pool[i])
			PCA_ERR("no %s\n", __dv2_ta_name[i]);
		else
			has_ta = true;
	}
	if (!has_ta) {
		ret = -ENODEV;
		goto out;
	}

	data->pca_swchg = prop_chgalgo_dev_get_by_name("pca_chg_swchg");
	if (!data->pca_swchg) {
		PCA_ERR("get pca_swchg fail\n");
		ret = -ENODEV;
		goto out;
	}
	data->pca_dvchg = prop_chgalgo_dev_get_by_name("pca_chg_dvchg");
	if (!data->pca_dvchg) {
		PCA_ERR("get pca_dvchg fail\n");
		ret = -ENODEV;
		goto out;
	}

	/* set adc chg pca */
	for (i = 0; i < PCA_ADCCHAN_MAX; i++) {
		if (strcmp(__dv2_adc_pcadev_name[i], "pca_chg_swchg") == 0) {
			__dv2_adc_pcadev[i] = data->pca_swchg;
			continue;
		}
		if (strcmp(__dv2_adc_pcadev_name[i], "pca_chg_dvchg") == 0) {
			__dv2_adc_pcadev[i] = data->pca_dvchg;
			continue;
		}
	}

	/* register tcp notifier callback */
	data->tcp_nb.notifier_call = __dv2_tcp_notifier_call;
	ret = register_tcp_dev_notifier(info->tcpc, &data->tcp_nb,
					TCP_NOTIFY_TYPE_USB);
	if (ret < 0) {
		PCA_ERR("register tcpc notifier fail\n");
		goto out;
	}

	data->inited = true;
	PCA_INFO("successfully\n");
out:
	mutex_unlock(&data->lock);
	return ret;
}

static bool dv2_is_algo_ready(struct prop_chgalgo_device *pca)
{
	int ret;
	bool rdy = true;
	struct dv2_algo_info *info = prop_chgalgo_get_drvdata(pca);
	struct dv2_algo_data *data = info->data;
	struct dv2_algo_desc *desc = info->desc;
	u32 soc;

	mutex_lock(&data->lock);
	PCA_INFO("++\n");

	if (!data->inited) {
		PCA_ERR("not init yet\n");
		rdy = false;
		goto out;
	}

	if (data->check_once) {
		PCA_ERR("already check once\n");
		rdy = false;
		goto out;
	}

	data->is_bif_exist = prop_chgalgo_is_bif_exist(data->pca_swchg);
	if (desc->need_bif && !data->is_bif_exist) {
		PCA_ERR("no bif\n");
		goto out;
	}

	ret = prop_chgalgo_get_soc(data->pca_swchg, &soc);
	if (ret >= 0) {
		PCA_INFO("soc(%d)\n", soc);
		if (soc < desc->start_soc_min) {
			PCA_INFO("soc(%d) < start soc(%d)\n", soc,
				 desc->start_soc_min);
			rdy = false;
			goto out;
		}
	}

	if (!__is_ta_rdy(info)) {
		rdy = false;
		goto out;
	}
	data->check_once = true;
out:
	mutex_unlock(&data->lock);
	return rdy;
}

static int dv2_start_algo(struct prop_chgalgo_device *pca)
{
	int ret;
	struct dv2_algo_info *info = prop_chgalgo_get_drvdata(pca);
	struct dv2_algo_data *data = info->data;

	mutex_lock(&data->lock);
	PCA_INFO("++\n");
	ret = __dv2_start(info);
	if (ret < 0)
		PCA_ERR("start dv2 algo fail\n");
	mutex_unlock(&data->lock);
	return ret;
}

static bool dv2_is_algo_running(struct prop_chgalgo_device *pca)
{
	struct dv2_algo_info *info = prop_chgalgo_get_drvdata(pca);
	struct dv2_algo_data *data = info->data;
	bool running = false;

	mutex_lock(&data->lock);
	if (!data->inited)
		goto out;

	running = (data->state == DV2_ALGO_STOP) ? false : true;
	PCA_INFO("running = %d\n", running);
out:
	mutex_unlock(&data->lock);
	return running;
}

static int dv2_plugout_reset(struct prop_chgalgo_device *pca)
{
	int ret = 0;
	struct dv2_algo_info *info = prop_chgalgo_get_drvdata(pca);
	struct dv2_algo_data *data = info->data;

	/* no need to reset before inited */
	mutex_lock(&data->lock);
	if (!data->inited)
		goto out;

	PCA_INFO("++\n");
	ret = __dv2_plugout_reset(info);
out:
	mutex_unlock(&data->lock);
	return ret;
}

static int dv2_stop_algo(struct prop_chgalgo_device *pca)
{
	int ret = 0;
	struct dv2_algo_info *info = prop_chgalgo_get_drvdata(pca);
	struct dv2_algo_data *data = info->data;

	mutex_lock(&data->lock);
	if (!data->inited)
		goto out;
	ret = __dv2_end(info, true, false);
out:
	mutex_unlock(&data->lock);
	return ret;
}

static struct prop_chgalgo_algo_ops pca_dv2_ops = {
	.init_algo = dv2_init_algo,
	.is_algo_ready = dv2_is_algo_ready,
	.start_algo = dv2_start_algo,
	.is_algo_running = dv2_is_algo_running,
	.plugout_reset = dv2_plugout_reset,
	.stop_algo = dv2_stop_algo,
};

static struct prop_chgalgo_desc pca_dv2_desc = {
	.name = "pca_algo_dv2",
	.type = PCA_DEVTYPE_ALGO,
};

#define DV2_DT_VALPROP_ARR(name, sz) \
	{#name, offsetof(struct dv2_algo_desc, name), sz}

#define DV2_DT_VALPROP(name) \
	DV2_DT_VALPROP_ARR(name, 1)

struct dv2_dtprop {
	const char *name;
	size_t offset;
	size_t sz;
};

static inline void dv2_parse_dt_u32(struct device_node *np, void *desc,
				    const struct dv2_dtprop *props,
				    int prop_cnt)
{
	int i;

	for (i = 0; i < prop_cnt; i++) {
		if (unlikely(!props[i].name))
			continue;
		of_property_read_u32(np, props[i].name, desc + props[i].offset);
	}
}

static inline void dv2_parse_dt_u32_arr(struct device_node *np, void *desc,
					   const struct dv2_dtprop *props,
					   int prop_cnt)
{
	int i;

	for (i = 0; i < prop_cnt; i++) {
		if (unlikely(!props[i].name))
			continue;
		of_property_read_u32_array(np, props[i].name,
					   desc + props[i].offset, props[i].sz);
	}
}

static inline int __of_property_read_s32_array(const struct device_node *np,
					       const char *propname,
					       s32 *out_values, size_t sz)
{
	return of_property_read_u32_array(np, propname, (u32 *)out_values, sz);
}

static inline void dv2_parse_dt_s32_arr(struct device_node *np, void *desc,
					   const struct dv2_dtprop *props,
					   int prop_cnt)
{
	int i;

	for (i = 0; i < prop_cnt; i++) {
		if (unlikely(!props[i].name))
			continue;
		__of_property_read_s32_array(np, props[i].name,
					     desc + props[i].offset,
					     props[i].sz);
	}
}

static const struct dv2_dtprop dv2_dtprops_u32[] = {
	DV2_DT_VALPROP(polling_interval),
	DV2_DT_VALPROP(vbus_upper_bound),
	DV2_DT_VALPROP(vbat_upper_bound),
	DV2_DT_VALPROP(start_soc_min),
	DV2_DT_VALPROP(idvchg_term),
	DV2_DT_VALPROP(idvchg_step),
	DV2_DT_VALPROP(idvchg_ss_init),
	DV2_DT_VALPROP(idvchg_ss_step),
	DV2_DT_VALPROP(idvchg_ss_step1),
	DV2_DT_VALPROP(idvchg_ss_step2),
	DV2_DT_VALPROP(idvchg_ss_step1_vbat),
	DV2_DT_VALPROP(idvchg_ss_step2_vbat),
	DV2_DT_VALPROP(ta_blanking),
	DV2_DT_VALPROP(swchg_aicr),
	DV2_DT_VALPROP(swchg_ichg),
	DV2_DT_VALPROP(swchg_aicr_ss_init),
	DV2_DT_VALPROP(swchg_aicr_ss_step),
	DV2_DT_VALPROP(chg_time_max),
	DV2_DT_VALPROP(ta_temp_recover_area),
	DV2_DT_VALPROP(bat_temp_recover_area),
	DV2_DT_VALPROP(fod_current),
	DV2_DT_VALPROP(r_bat_min),
	DV2_DT_VALPROP(r_sw_min),
	DV2_DT_VALPROP(ta_cap_vmin),
	DV2_DT_VALPROP(ta_cap_vmax),
	DV2_DT_VALPROP(ta_cap_imin),
};

static const struct dv2_dtprop dv2_dtprops_u32_array[] = {
	DV2_DT_VALPROP_ARR(ita_level, DV2_RCABLE_NUM),
	DV2_DT_VALPROP_ARR(rcable_level, DV2_RCABLE_NUM),
};

static const struct dv2_dtprop dv2_dtprops_s32_array[] = {
	DV2_DT_VALPROP_ARR(ta_temp_level, DV2_TTA_NUM),
	DV2_DT_VALPROP_ARR(ta_temp_curlmt, DV2_TTA_NUM),
	DV2_DT_VALPROP_ARR(bat_temp_level, DV2_TBAT_NUM),
	DV2_DT_VALPROP_ARR(bat_temp_curlmt, DV2_TBAT_NUM),
	DV2_DT_VALPROP_ARR(dvchg_temp_level, DV2_TDVCHG_NUM),
	DV2_DT_VALPROP_ARR(dvchg_temp_curlmt, DV2_TDVCHG_NUM),
};

static const char * const __dv2_dev_node_name[] = {
	"mtk_pe50",
	"rt_dv2",
};

static int dv2_parse_dt(struct dv2_algo_info *info)
{
	int i;
	struct dv2_algo_desc *desc;
	struct device_node *np;

	desc = devm_kzalloc(info->dev, sizeof(*desc), GFP_KERNEL);
	if (!desc)
		return -ENOMEM;
	info->desc = desc;
	memcpy(desc, &algo_desc_defval, sizeof(*desc));
	for (i = 0; i < ARRAY_SIZE(__dv2_dev_node_name); i++) {
		np = of_find_node_by_name(NULL, __dv2_dev_node_name[i]);
		if (np) {
			PCA_ERR("find node %s\n", __dv2_dev_node_name[i]);
			break;
		}
	}
	if (i == ARRAY_SIZE(__dv2_dev_node_name)) {
		PCA_ERR("no device node found\n");
		return -EINVAL;
	}

	desc->need_bif = of_property_read_bool(np, "need_bif");
	dv2_parse_dt_u32(np, (void *)desc, dv2_dtprops_u32,
			 ARRAY_SIZE(dv2_dtprops_u32));
	dv2_parse_dt_u32_arr(np, (void *)desc, dv2_dtprops_u32_array,
			     ARRAY_SIZE(dv2_dtprops_u32_array));
	dv2_parse_dt_s32_arr(np, (void *)desc, dv2_dtprops_s32_array,
			     ARRAY_SIZE(dv2_dtprops_s32_array));
	if (desc->swchg_aicr == 0 || desc->swchg_ichg == 0) {
		desc->swchg_aicr = 0;
		desc->swchg_ichg = 0;
	}
	return 0;
}

static int dv2_algo_probe(struct platform_device *pdev)
{
	int ret;
	struct dv2_algo_info *info;
	struct dv2_algo_data *data;

	dev_info(&pdev->dev, "%s(%s)\n", __func__, PCA_DV2_ALGO_VERSION);

	info = devm_kzalloc(&pdev->dev, sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;
	data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;
	info->data = data;
	info->dev = &pdev->dev;
	platform_set_drvdata(pdev, info);

	ret = dv2_parse_dt(info);
	if (ret < 0) {
		dev_info(info->dev, "%s parse dt fail(%d)\n", __func__, ret);
		return ret;
	}

	info->tcpc = tcpc_dev_get_by_name("type_c_port0");
	if (!info->tcpc) {
		dev_info(info->dev, "%s get tcpc dev fail\n", __func__);
		return -ENODEV;
	}

	info->pca = prop_chgalgo_device_register(info->dev, &pca_dv2_desc,
						 NULL, NULL, &pca_dv2_ops,
						 info);
	if (IS_ERR_OR_NULL(info->pca)) {
		dev_info(info->dev, "%s reg dv2 algo fail(%d)\n", __func__,
			 ret);
		return PTR_ERR(info->pca);
	}

	/* init algo thread & timer */
	mutex_init(&data->tcp_lock);
	mutex_init(&data->lock);
	data->state = DV2_ALGO_STOP;
	alarm_init(&data->timer, ALARM_REALTIME, __dv2_algo_timer_cb);
	data->task = kthread_create(__dv2_algo_threadfn, info, "dv2_algo_task");
	if (IS_ERR(data->task)) {
		ret = PTR_ERR(data->task);
		dev_info(info->dev, "%s run task fail(%d)\n", __func__, ret);
		goto err;
	}
	device_init_wakeup(info->dev, true);
	dev_info(info->dev, "%s successfully\n", __func__);
	return 0;

err:
	mutex_destroy(&data->lock);
	mutex_destroy(&data->tcp_lock);
	prop_chgalgo_device_unregister(info->pca);
	return ret;
}

static int dv2_algo_remove(struct platform_device *pdev)
{
	struct dv2_algo_info *info = platform_get_drvdata(pdev);
	struct dv2_algo_data *data;

	if (info) {
		data = info->data;
		kthread_stop(data->task);
		mutex_destroy(&data->lock);
		mutex_destroy(&data->tcp_lock);
		prop_chgalgo_device_unregister(info->pca);
	}

	return 0;
}

static int __maybe_unused dv2_algo_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct dv2_algo_info *info = platform_get_drvdata(pdev);
	struct dv2_algo_data *data = info->data;

	dev_info(dev, "%s\n", __func__);
	mutex_lock(&data->lock);
	return 0;
}

static int __maybe_unused dv2_algo_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct dv2_algo_info *info = platform_get_drvdata(pdev);
	struct dv2_algo_data *data = info->data;

	dev_info(dev, "%s\n", __func__);
	mutex_unlock(&data->lock);
	return 0;
}

static SIMPLE_DEV_PM_OPS(dv2_algo_pm_ops, dv2_algo_suspend, dv2_algo_resume);

static struct platform_device dv2_algo_platdev = {
	.name = "pca_dv2_algo",
	.id = -1,
};

static struct platform_driver dv2_algo_platdrv = {
	.probe = dv2_algo_probe,
	.remove = dv2_algo_remove,
	.driver = {
		.name = "pca_dv2_algo",
		.owner = THIS_MODULE,
		.pm = &dv2_algo_pm_ops,
	},
};

static int __init dv2_algo_init(void)
{
	platform_device_register(&dv2_algo_platdev);
	return platform_driver_register(&dv2_algo_platdrv);
}
device_initcall_sync(dv2_algo_init);

static void __exit dv2_algo_exit(void)
{
	platform_driver_unregister(&dv2_algo_platdrv);
	platform_device_unregister(&dv2_algo_platdev);
}
module_exit(dv2_algo_exit);

MODULE_DESCRIPTION("Divide By Two Algorithm For PCA");
MODULE_AUTHOR("ShuFanLee <shufan_lee@richtek.com>");
MODULE_VERSION(PCA_DV2_ALGO_VERSION);
MODULE_LICENSE("GPL");
