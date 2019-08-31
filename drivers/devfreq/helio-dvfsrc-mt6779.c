/*
 * Copyright (C) 2018 MediaTek Inc.
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

#include <linux/fb.h>
#include <linux/notifier.h>
#include <linux/string.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>

#ifdef CONFIG_MTK_DRAMC
#include <mtk_dramc.h>
#endif
#include <mt-plat/upmu_common.h>
#include <ext_wd_drv.h>
#ifdef CONFIG_MTK_EMI
#include <mt_emi_api.h>
#endif

#include <helio-dvfsrc.h>
#include <helio-dvfsrc-opp.h>
#include <mtk_dvfsrc_reg.h>
//#include <mtk_spm_internal.h>
#include <spm/mtk_vcore_dvfs.h>
#include <mt-plat/mtk_devinfo.h>
#include <linux/mutex.h>

#include <mmdvfs_mgr.h>
#include <mt-plat/aee.h>
#include <mtk_qos_sram.h>

#include <linux/regulator/consumer.h>

#if IS_ENABLED(CONFIG_MMPROFILE)
#include <mmprofile.h>
#include <mmprofile_function.h>
struct dvfsrc_mmp_events_t {
	mmp_event dvfs_event;
	mmp_event level_change;
};
static struct dvfsrc_mmp_events_t dvfsrc_mmp_events;
#endif
static struct regulator *vcore_reg_id;
#define AUTOK_ENABLE
#define TIME_STAMP_SIZE 40
#define DVFSRC_1600_FLOOR

static struct reg_config dvfsrc_init_configs[][128] = {
	{
/* MISC */
		{ DVFSRC_TIMEOUT_NEXTREQ,  0x0000002B },
		{ DVFSRC_INT_EN,           0x00000002 },
		{ DVFSRC_QOS_EN,           0x0000407C },
/* QOS   */
		{ DVFSRC_VCORE_REQUEST4,   0x21110000 },
#ifdef DVFSRC_1600_FLOOR
		{ DVFSRC_DDR_REQUEST,	   0x00004322 },
#else
		{ DVFSRC_DDR_REQUEST,	   0x00004321 },
#endif
		{ DVFSRC_DDR_REQUEST3,     0x00000055 },
#ifdef DVFSRC_1600_FLOOR
		{ DVFSRC_DDR_REQUEST5,	   0x00322000 },
#else
		{ DVFSRC_DDR_REQUEST5,	   0x00321000 },
#endif
		{ DVFSRC_DDR_REQUEST7,     0x54000000 },
		{ DVFSRC_DDR_REQUEST8,     0x55000000 },
		{ DVFSRC_DDR_QOS0,         0x00000019 },
		{ DVFSRC_DDR_QOS1,         0x00000026 },
		{ DVFSRC_DDR_QOS2,         0x00000033 },
		{ DVFSRC_DDR_QOS3,         0x0000004C },
		{ DVFSRC_DDR_QOS4,         0x00000066 },
		{ DVFSRC_DDR_QOS5,         0x00000077 },
		{ DVFSRC_DDR_QOS6,         0x00000077 },
/* HRT  */
		{ DVFSRC_HRT_REQ_UNIT,      0x0000011E },
		{ DVSFRC_HRT_REQ_MD_URG,    0x0000D3D3 },
		{ DVFSRC_HRT_REQ_MD_BW_0,   0x00200802 },
		{ DVFSRC_HRT_REQ_MD_BW_1,   0x00200800 },
		{ DVFSRC_HRT_REQ_MD_BW_2,   0x00200002 },
		{ DVFSRC_HRT_REQ_MD_BW_3,   0x00200802 },
		{ DVFSRC_HRT_REQ_MD_BW_4,   0x00400802 },
		{ DVFSRC_HRT_REQ_MD_BW_5,   0x00601404 },
		{ DVFSRC_HRT_REQ_MD_BW_6,   0x00902008 },
		{ DVFSRC_HRT_REQ_MD_BW_7,   0x00E0380E },
		{ DVFSRC_HRT_REQ_MD_BW_8,   0x00000000 },
		{ DVFSRC_HRT_REQ_MD_BW_9,   0x00000000 },
		{ DVFSRC_HRT_REQ_MD_BW_10,  0x00034C00 },
		{ DVFSRC_HRT1_REQ_MD_BW_0,  0x0360D836 },
		{ DVFSRC_HRT1_REQ_MD_BW_1,  0x0360D800 },
		{ DVFSRC_HRT1_REQ_MD_BW_2,  0x03600036 },
		{ DVFSRC_HRT1_REQ_MD_BW_3,  0x0360D836 },
		{ DVFSRC_HRT1_REQ_MD_BW_4,  0x0360D836 },
		{ DVFSRC_HRT1_REQ_MD_BW_5,  0x0360D836 },
		{ DVFSRC_HRT1_REQ_MD_BW_6,  0x0360D836 },
		{ DVFSRC_HRT1_REQ_MD_BW_7,  0x0360D836 },
		{ DVFSRC_HRT1_REQ_MD_BW_8,  0x00000000 },
		{ DVFSRC_HRT1_REQ_MD_BW_9,  0x00000000 },
		{ DVFSRC_HRT1_REQ_MD_BW_10, 0x00034C00 },
		{ DVFSRC_HRT_HIGH,          0x070804B0 },
		{ DVFSRC_HRT_HIGH_1,        0x11830B80 },
		{ DVFSRC_HRT_HIGH_2,        0x18A618A6 },
		{ DVFSRC_HRT_HIGH_3,        0x000018A6 },
		{ DVFSRC_HRT_LOW,           0x070704AF },
		{ DVFSRC_HRT_LOW_1,         0x11820B7F },
		{ DVFSRC_HRT_LOW_2,         0x18A518A5 },
		{ DVFSRC_HRT_LOW_3,         0x000018A5 },
#ifdef DVFSRC_1600_FLOOR
		{ DVFSRC_HRT_REQUEST,	    0x05554322 },
#else
		{ DVFSRC_HRT_REQUEST,	    0x05554321 },
#endif
/* Rising */
		{ DVFSRC_EMI_MON_DEBOUNCE_TIME,   0x4C4C0AB0 },
		{ DVFSRC_DDR_ADD_REQUEST,   0x05543210 },
		{ DVFSRC_EMI_ADD_REQUEST,   0x03333210 },
/* Level */
		{ DVFSRC_LEVEL_LABEL_0_1,   0x40225032 },
		{ DVFSRC_LEVEL_LABEL_2_3,   0x20223012 },
		{ DVFSRC_LEVEL_LABEL_4_5,   0x40211012 },
		{ DVFSRC_LEVEL_LABEL_6_7,   0x20213011 },
		{ DVFSRC_LEVEL_LABEL_8_9,   0x30101011 },
		{ DVFSRC_LEVEL_LABEL_10_11, 0x10102000 },
		{ DVFSRC_LEVEL_LABEL_12_13, 0x00000000 },
		{ DVFSRC_LEVEL_LABEL_14_15, 0x00000000 },
/* Enable */
		{ DVFSRC_CURRENT_FORCE,     0x00000001 },
		{ DVFSRC_BASIC_CONTROL,     0x3599404B },
		{ DVFSRC_BASIC_CONTROL,     0x3599014B },
		{ DVFSRC_CURRENT_FORCE,     0x00000000 },
		{ -1, 0 },
	},
	/* NULL */
	{
		{ -1, 0 },
	},
};

static DEFINE_SPINLOCK(force_req_lock);
static char	timeout_stamp[TIME_STAMP_SIZE];
static char	force_start_stamp[TIME_STAMP_SIZE];
static char	force_end_stamp[TIME_STAMP_SIZE];
/*static char	level_stamp[TIME_STAMP_SIZE];*/
static char sys_stamp[TIME_STAMP_SIZE];
static char opp_forced;


#define dvfsrc_rmw(offset, val, mask, shift) \
	dvfsrc_write(offset, (dvfsrc_read(offset) & ~(mask << shift)) \
			| (val << shift))

static void dvfsrc_set_sw_req(int data, int mask, int shift)
{
	dvfsrc_rmw(DVFSRC_SW_REQ3, data, mask, shift);
}

static void dvfsrc_set_sw_req2(int data, int mask, int shift)
{
	dvfsrc_rmw(DVFSRC_SW_REQ2, data, mask, shift);
}

static void dvfsrc_set_sw_req6(int data, int mask, int shift)
{
	dvfsrc_rmw(DVFSRC_SW_REQ6, data, mask, shift);
}

static void dvfsrc_set_vcore_request(int data, int mask, int shift)
{
	dvfsrc_rmw(DVFSRC_VCORE_REQUEST, data, mask, shift);
}

static void dvfsrc_get_timestamp(char *p)
{
	u64 sec = local_clock();
	u64 usec = do_div(sec, 1000000000);

	do_div(usec, 1000);
	snprintf(p, TIME_STAMP_SIZE, "%llu.%06llu",
		sec, usec);
}

static void dvfsrc_get_sys_stamp(char *p)
{
	u64 sys_time;
	u64 kernel_time;

	kernel_time = sched_clock_get_cyc(&sys_time);
	qos_sram_write(DVFSRC_TIMESTAMP_OFFSET,
		kernel_time);
	qos_sram_write(DVFSRC_TIMESTAMP_OFFSET + 0x4,
		kernel_time >> 32);
	qos_sram_write(DVFSRC_TIMESTAMP_OFFSET + 0x8,
		sys_time);
	qos_sram_write(DVFSRC_TIMESTAMP_OFFSET + 0xC,
		sys_time >> 32);
	if (p) {
		snprintf(p, TIME_STAMP_SIZE, "0x%llx, 0x%llx",
			kernel_time, sys_time);
	}
}


static int is_dvfsrc_forced(void)
{
	return opp_forced;
}

int is_dvfsrc_opp_fixed(void)
{
	int ret;
	unsigned long flags;

	if (!is_dvfsrc_enabled())
		return 1;

	if (!(dvfsrc_read(DVFSRC_BASIC_CONTROL) & 0x100))
		return 1;

	if (helio_dvfsrc_flag_get() != 0)
		return 1;

	spin_lock_irqsave(&force_req_lock, flags);
	ret = is_dvfsrc_forced();
	spin_unlock_irqrestore(&force_req_lock, flags);

	return ret;
}

static void dvfsrc_set_force_start(int data)
{
	opp_forced = 1;
	dvfsrc_get_timestamp(force_start_stamp);
	dvfsrc_rmw(DVFSRC_BASIC_CONTROL, 0,
				DVFSRC_OUT_EN_MASK, DVFSRC_OUT_EN_SHIFT);
	dvfsrc_rmw(DVFSRC_BASIC_CONTROL, 0,
			FORCE_EN_TAR_MASK, FORCE_EN_TAR_SHIFT);
	dvfsrc_write(DVFSRC_TARGET_FORCE, data);
	dvfsrc_rmw(DVFSRC_BASIC_CONTROL, 1,
			FORCE_EN_TAR_MASK, FORCE_EN_TAR_SHIFT);
	dvfsrc_rmw(DVFSRC_BASIC_CONTROL, 1,
				DVFSRC_OUT_EN_MASK, DVFSRC_OUT_EN_SHIFT);
}

static void dvfsrc_set_force_end(void)
{
	dvfsrc_rmw(DVFSRC_BASIC_CONTROL, 0,
				DVFSRC_OUT_EN_MASK, DVFSRC_OUT_EN_SHIFT);
	dvfsrc_rmw(DVFSRC_BASIC_CONTROL, 0,
			FORCE_EN_TAR_MASK, FORCE_EN_TAR_SHIFT);
	dvfsrc_write(DVFSRC_TARGET_FORCE, 0);
	dvfsrc_write(DVFSRC_CURRENT_FORCE, 0);
	dvfsrc_rmw(DVFSRC_BASIC_CONTROL, 1,
			FORCE_EN_TAR_MASK, FORCE_EN_TAR_SHIFT);
	dvfsrc_rmw(DVFSRC_BASIC_CONTROL, 1,
				DVFSRC_OUT_EN_MASK, DVFSRC_OUT_EN_SHIFT);
}

static void dvfsrc_release_force(void)
{
	dvfsrc_rmw(DVFSRC_BASIC_CONTROL, 0,
				DVFSRC_OUT_EN_MASK, DVFSRC_OUT_EN_SHIFT);
	dvfsrc_rmw(DVFSRC_BASIC_CONTROL, 0,
			FORCE_EN_TAR_MASK, FORCE_EN_TAR_SHIFT);
	dvfsrc_rmw(DVFSRC_BASIC_CONTROL, 0,
			FORCE_EN_CUR_MASK, FORCE_EN_CUR_SHIFT);

	dvfsrc_write(DVFSRC_TARGET_FORCE, 0);
	dvfsrc_write(DVFSRC_CURRENT_FORCE, 0);
	dvfsrc_rmw(DVFSRC_BASIC_CONTROL, 1,
				DVFSRC_OUT_EN_MASK, DVFSRC_OUT_EN_SHIFT);
	dvfsrc_get_timestamp(force_end_stamp);
	opp_forced = 0;
}

static void dvfsrc_set_sw_bw(int type, int data)
{
	switch (type) {
	case PM_QOS_APU_MEMORY_BANDWIDTH:
		dvfsrc_write(DVFSRC_SW_BW_0, data / 100);
		break;
	case PM_QOS_CPU_MEMORY_BANDWIDTH:
		dvfsrc_write(DVFSRC_SW_BW_1, data / 100);
		break;
	case PM_QOS_GPU_MEMORY_BANDWIDTH:
		dvfsrc_write(DVFSRC_SW_BW_2, data / 100);
		break;
	case PM_QOS_MM_MEMORY_BANDWIDTH:
		dvfsrc_write(DVFSRC_SW_BW_3, data / 100);
		break;
	case PM_QOS_OTHER_MEMORY_BANDWIDTH:
		dvfsrc_write(DVFSRC_SW_BW_4, data / 100);
		break;
	default:
		break;
	}
}

static void dvfsrc_set_isp_hrt_bw(int data)
{
	dvfsrc_write(DVFSRC_ISP_HRT, (data + 29) / 30);
}

static u32 dvfsrc_calc_hrt_opp(int data)
{
	if (data < 0x04B0)
		return DDR_OPP_5;
#ifdef DVFSRC_1600_FLOOR
	else if (data < 0x0708)
		return DDR_OPP_3;
#else
	else if (data < 0x0708)
		return DDR_OPP_4;
#endif
	else if (data < 0x0B80)
		return DDR_OPP_3;
	else if (data < 0x1183)
		return DDR_OPP_2;
	else if (data < 0x18A6)
		return DDR_OPP_1;
	else
		return DDR_OPP_0;
}

u32 dvfsrc_calc_isp_hrt_opp(int data)
{
	return dvfsrc_calc_hrt_opp(((data + 29) /  30) * 30);
}

u32 get_dvfs_final_level(void)
{
	return dvfsrc_read(DVFSRC_CURRENT_LEVEL);
}

int commit_data(int type, int data, int check_spmfw)
{
	int ret = 0;
	int level = 16, opp = 16;
	unsigned long flags;
	int opp_uv;
	int vcore_uv = 0;

	if (!is_dvfsrc_enabled())
		return ret;

	if (check_spmfw)
		mtk_spmfw_init(1, 0);

	switch (type) {
	case PM_QOS_APU_MEMORY_BANDWIDTH:
	case PM_QOS_CPU_MEMORY_BANDWIDTH:
	case PM_QOS_GPU_MEMORY_BANDWIDTH:
	case PM_QOS_OTHER_MEMORY_BANDWIDTH:
	case PM_QOS_MM_MEMORY_BANDWIDTH:
		if (data < 0)
			data = 0;
		dvfsrc_set_sw_bw(type, data);
		break;
	case PM_QOS_DDR_OPP:
		spin_lock_irqsave(&force_req_lock, flags);
		if (data >= DDR_OPP_NUM || data < 0)
			data = DDR_OPP_NUM - 1;

		opp = data;
		level = DDR_OPP_NUM - data - 1;

		dvfsrc_set_sw_req(level, DDR_SW_AP_MASK, DDR_SW_AP_SHIFT);

		if (!is_dvfsrc_forced() && check_spmfw) {
			ret = dvfsrc_wait_for_completion(
					get_cur_ddr_opp() <= opp,
					DVFSRC_TIMEOUT);
		}
		spin_unlock_irqrestore(&force_req_lock, flags);
		break;
	case PM_QOS_VCORE_OPP:
		spin_lock_irqsave(&force_req_lock, flags);
		if (data >= VCORE_OPP_NUM || data < 0)
			data = VCORE_OPP_NUM - 1;

		opp = data;
		level = VCORE_OPP_NUM - data - 1;
		dvfsrc_set_sw_req(level, VCORE_SW_AP_MASK, VCORE_SW_AP_SHIFT);

		if (!is_dvfsrc_forced() && check_spmfw) {
			udelay(1);
			dvfsrc_wait_for_completion(
				dvfsrc_read(DVFSRC_TARGET_LEVEL) == 0,
				DVFSRC_TIMEOUT);
			udelay(1);
			ret = dvfsrc_wait_for_completion(
					get_cur_vcore_opp() <= opp,
					DVFSRC_TIMEOUT);
		}
		spin_unlock_irqrestore(&force_req_lock, flags);
		if (!is_dvfsrc_forced() && check_spmfw) {
			if (vcore_reg_id) {
			vcore_uv = regulator_get_voltage(vcore_reg_id);
			opp_uv = get_vcore_uv_table(opp);
				if (vcore_uv < opp_uv) {
					pr_info("DVFS FAIL= %d %d 0x%08x 0x%08x %08x\n",
					vcore_uv, opp_uv,
					dvfsrc_read(DVFSRC_CURRENT_LEVEL),
					dvfsrc_read(DVFSRC_TARGET_LEVEL),
					spm_get_dvfs_level());

					aee_kernel_warning("DVFSRC",
						"VCORE failed.",
						__func__);
				}
			}
		}
		break;
	case PM_QOS_SCP_VCORE_REQUEST:
		spin_lock_irqsave(&force_req_lock, flags);
		if (data >= VCORE_OPP_NUM || data < 0)
			data = 0;

		opp = VCORE_OPP_NUM - data - 1;
		level = data;

		dvfsrc_set_vcore_request(level,
				VCORE_SCP_GEAR_MASK, VCORE_SCP_GEAR_SHIFT);

		if (!is_dvfsrc_forced() && check_spmfw) {
			ret = dvfsrc_wait_for_completion(
					get_cur_vcore_opp() <= opp,
					DVFSRC_TIMEOUT);
		}
		spin_unlock_irqrestore(&force_req_lock, flags);
		break;
	case PM_QOS_POWER_MODEL_DDR_REQUEST:
		if (data >= DDR_OPP_NUM || data < 0)
			data = 0;

		opp = DDR_OPP_NUM - data - 1;
		level = data;

		dvfsrc_set_sw_req2(level,
				DDR_SW_AP_MASK, DDR_SW_AP_SHIFT);
		break;
	case PM_QOS_POWER_MODEL_VCORE_REQUEST:
		if (data >= VCORE_OPP_NUM || data < 0)
			data = 0;

		opp = VCORE_OPP_NUM - data - 1;
		level = data;

		dvfsrc_set_sw_req2(level,
				VCORE_SW_AP_MASK, VCORE_SW_AP_SHIFT);
		break;
	case PM_QOS_VCORE_DVFS_FORCE_OPP:
		spin_lock_irqsave(&force_req_lock, flags);
		if (data >= VCORE_DVFS_OPP_NUM || data < 0)
			data = VCORE_DVFS_OPP_NUM;

		opp = data;
		if (opp == VCORE_DVFS_OPP_NUM) {
			dvfsrc_release_force();
			spin_unlock_irqrestore(&force_req_lock, flags);
			break;
		}

		level = opp;
		dvfsrc_set_force_start(1 << level);
		if (check_spmfw) {
			ret = dvfsrc_wait_for_completion(
					get_cur_vcore_dvfs_opp() == opp,
					DVFSRC_TIMEOUT);
		}
		dvfsrc_set_force_end();
		spin_unlock_irqrestore(&force_req_lock, flags);
		break;
	case PM_QOS_ISP_HRT_BANDWIDTH:
		spin_lock_irqsave(&force_req_lock, flags);
		if (data < 0)
			data = 0;

		dvfsrc_set_isp_hrt_bw(data);
		opp = dvfsrc_calc_isp_hrt_opp(data);
		if (!is_dvfsrc_forced() && check_spmfw) {
			udelay(1);
			dvfsrc_wait_for_completion(
				dvfsrc_read(DVFSRC_TARGET_LEVEL) == 0,
				DVFSRC_TIMEOUT);
			ret = dvfsrc_wait_for_completion(
				get_cur_ddr_opp() <= opp,
				DVFSRC_TIMEOUT);
		}
		spin_unlock_irqrestore(&force_req_lock, flags);
		break;
	default:
		break;
	}

	if (!(dvfsrc_read(DVFSRC_BASIC_CONTROL) & 0x100)) {
		pr_info("DVFSRC OUT Disable\n");
		return ret;
	}

	if (ret < 0) {
		pr_err("%s: type: 0x%x, data: 0x%x, opp: %d, level: %d\n",
				__func__, type, data, opp, level);
		dvfsrc_dump_reg(NULL);
		aee_kernel_warning("DVFSRC", "%s: failed.", __func__);
	}

	return ret;
}

u32 vcorefs_get_md_scenario(void)
{
	return dvfsrc_read(DVFSRC_DEBUG_STA_0);
}
EXPORT_SYMBOL(vcorefs_get_md_scenario);

u32 dvfsrc_get_md_bw(void)
{
	u32 last, is_turbo, is_urgent, md_scen;
	u32 val;

	last = dvfsrc_read(DVFSRC_LAST);

	val = dvfsrc_read(DVFSRC_RECORD_0_6 + RECORD_SHIFT * last);
	is_turbo =
		(val >> MD_TURBO_SWITCH_SHIFT) & MD_TURBO_SWITCH_MASK;

	val = dvfsrc_read(DVFSRC_DEBUG_STA_0);
	is_urgent =
		(val >> MD_EMI_URG_DEBUG_SHIFT) & MD_EMI_URG_DEBUG_MASK;

	md_scen =
		(val >> MD_EMI_VAL_DEBUG_SHIFT) & MD_EMI_VAL_DEBUG_MASK;

	if (is_urgent) {
		val = dvfsrc_read(DVSFRC_HRT_REQ_MD_URG);
		if (is_turbo) {
			val = (val >> MD_HRT_BW_URG1_SHIFT)
				& MD_HRT_BW_URG1_MASK;
		} else {
			val = (val >> MD_HRT_BW_URG_SHIFT)
				& MD_HRT_BW_URG_MASK;
		}
	} else {
		u32 index, shift;

		index = md_scen / 3;
		shift = (md_scen % 3) * 10;

		if (index > 10)
			return 0;

		if (index < 8) {
			if (is_turbo)
				val = dvfsrc_read(DVFSRC_HRT1_REQ_MD_BW_0
					+ index * 4);
			else
				val = dvfsrc_read(DVFSRC_HRT_REQ_MD_BW_0
					+ index * 4);
		} else {
			if (is_turbo)
				val = dvfsrc_read(DVFSRC_HRT1_REQ_MD_BW_8
					+ (index - 8) * 4);
			else
				val = dvfsrc_read(DVFSRC_HRT_REQ_MD_BW_8
					+ (index - 8) * 4);
		}
		val = (val >> shift) & MD_HRT_BW_MASK;
	}
	return val;
}

static u32 vcorefs_get_md_rising_status(void)
{
	u32 last, val;

	last = dvfsrc_read(DVFSRC_LAST);

	val = dvfsrc_read(DVFSRC_RECORD_0_6 + RECORD_SHIFT * last);

	val = (val >> RECORD_MD_DDR_LATENCY_REQ) &
		RECORD_MD_DDR_LATENCY_MASK;

	return val;
}

static u32 vcorefs_get_ddr_qos(void)
{
	unsigned int qos_total_bw = dvfsrc_read(DVFSRC_SW_BW_0) +
			   dvfsrc_read(DVFSRC_SW_BW_1) +
			   dvfsrc_read(DVFSRC_SW_BW_2) +
			   dvfsrc_read(DVFSRC_SW_BW_3) +
			   dvfsrc_read(DVFSRC_SW_BW_4);

	if (qos_total_bw < 0x19)
		return 0;
#ifdef DVFSRC_1600_FLOOR
	else if (qos_total_bw < 0x26)
		return 2;
#else
	else if (qos_total_bw < 0x26)
		return 1;
#endif
	else if (qos_total_bw < 0x33)
		return 2;
	else if (qos_total_bw < 0x4c)
		return 3;
	else if (qos_total_bw < 0x66)
		return 4;
	else
		return 5;

}

static u32 vcorefs_get_total_emi_status(void)
{
	u32 val;

	val = dvfsrc_read(DVFSRC_DEBUG_STA_2);

	val = (val >> DEBUG_STA2_EMI_TOTAL_SHIFT) & DEBUG_STA2_EMI_TOTAL_MASK;

	return val;
}

static int vcorefs_get_emi_mon_gear(void)
{
	unsigned int total_bw_status = vcorefs_get_total_emi_status();
	int i;

	for (i = 4; i >= 0 ; i--) {
		if ((total_bw_status >> i) > 0) {
#ifdef DVFSRC_1600_FLOOR
			if (i == 0)
				return i + 2;
			else
				return i + 1;
#else
			return i + 1;
#endif
		}
	}

	return 0;
}

static u32 vcorefs_get_scp_req_status(void)
{
	u32 val;

	val = dvfsrc_read(DVFSRC_DEBUG_STA_2);

	val = (val >> DEBUG_STA2_SCP_SHIFT) & DEBUG_STA2_SCP_MASK;

	return val;
}

static u32 vcorefs_get_md_emi_latency_status(void)
{
	u32 val;

	val = dvfsrc_read(DVFSRC_DEBUG_STA_2);

	val = (val >> DEBUG_STA2_MD_EMI_LATENCY_SHIFT)
		& DEBUG_STA2_MD_EMI_LATENCY_MASK;

	return val;
}

static u32 vcorefs_get_hrt_bw_status(void)
{
	u32 last, val;

	last = dvfsrc_read(DVFSRC_LAST);

	val = dvfsrc_read(DVFSRC_RECORD_0_6 + RECORD_SHIFT * last);

	val = (val >> RECORD_HRT_BW_REQ_SHIFT) & RECORD_HRT_BW_REQ_MASK;

	return val;
}

static u32 vcorefs_get_hifi_scenario(void)
{
	u32 val;

	val = dvfsrc_read(DVFSRC_DEBUG_STA_2);

	val = (val >> DEBUG_STA2_HIFI_SCENARIO_SHIFT)
		& DEBUG_STA2_HIFI_SCENARIO_MASK;

	return val;
}


static u32 vcorefs_get_hifi_vcore_status(void)
{
	u32 hifi_scen;

	hifi_scen = __builtin_ffs(vcorefs_get_hifi_scenario());

	if (hifi_scen)
		return (dvfsrc_read(DVFSRC_VCORE_REQUEST4) >>
			((hifi_scen - 1) * 4)) & 0xF;
	else
		return 0;
}

static u32 vcorefs_get_hifi_ddr_status(void)
{
	u32 hifi_scen;

	hifi_scen = __builtin_ffs(vcorefs_get_hifi_scenario());

	if (hifi_scen)
		return (dvfsrc_read(DVFSRC_DDR_REQUEST6) >>
			((hifi_scen - 1) * 4)) & 0xF;
	else
		return 0;
}

static u32 vcorefs_get_hifi_rising_status(void)
{
	u32 last, val;

	last = dvfsrc_read(DVFSRC_LAST);

	val = dvfsrc_read(DVFSRC_RECORD_0_6 + RECORD_SHIFT * last);

	val = (val >> RECORD_HIFI_DDR_LATENCY_REQ) &
		RECORD_HIFI_DDR_LATENCY_MASK;

	return val;
}

#ifdef AUTOK_ENABLE
__weak int emmc_autok(void)
{
	pr_info("NOT SUPPORT EMMC AUTOK\n");
	return 0;
}

__weak int sd_autok(void)
{
	pr_info("NOT SUPPORT SD AUTOK\n");
	return 0;
}

__weak int sdio_autok(void)
{
	pr_info("NOT SUPPORT SDIO AUTOK\n");
	return 0;
}


static void begin_autok_task(void)
{
	struct mmdvfs_prepare_event evt_from_vcore = {
		MMDVFS_EVENT_PREPARE_CALIBRATION_START};

	/* notify MM DVFS for msdc autok start */
	mmdvfs_notify_prepare_action(&evt_from_vcore);
}

static void finish_autok_task(void)
{
	/* check if dvfs force is released */
	int force = pm_qos_request(PM_QOS_VCORE_DVFS_FORCE_OPP);

	struct mmdvfs_prepare_event evt_from_vcore = {
		MMDVFS_EVENT_PREPARE_CALIBRATION_END};

	/* notify MM DVFS for msdc autok finish */
	mmdvfs_notify_prepare_action(&evt_from_vcore);

	if (force >= 0 && force < 13)
		pr_info("autok task not release force opp: %d\n", force);
}

static void dvfsrc_autok_manager(void)
{
	int r = 0;

	begin_autok_task();

	r = emmc_autok();
	pr_info("EMMC autok done: %s\n", (r == 0) ? "Yes" : "No");

	r = sd_autok();
	pr_info("SD autok done: %s\n", (r == 0) ? "Yes" : "No");

	r = sdio_autok();
	pr_info("SDIO autok done: %s\n", (r == 0) ? "Yes" : "No");

	finish_autok_task();
}
#endif

static void helio_dvfsrc_config(void)
{
	int spmfw_idx = 0;
	struct reg_config *config;
	int idx = 0;

	config = dvfsrc_init_configs[spmfw_idx];

	while (config[idx].offset != -1) {
		dvfsrc_write(config[idx].offset, config[idx].val);
		idx++;
	}
}

static void dvfsrc_level_change_dump(void)
{
#if 0
	u32 last;
	static u32 log_timer1, log_timer2;

	last = dvfsrc_read(DVFSRC_LAST);
	log_timer1 = dvfsrc_read(DVFSRC_RECORD_0_0 + (RECORD_SHIFT * last));
	log_timer2 = dvfsrc_read(DVFSRC_RECORD_0_1 + (RECORD_SHIFT * last));
	dvfsrc_get_sys_stamp(level_stamp);
	pr_info("DVFSRC change %08x %08x %d %08x %08x\n",
		dvfsrc_read(DVFSRC_CURRENT_LEVEL),
		dvfsrc_read(DVFSRC_TARGET_LEVEL),
		last,
		log_timer1,
		log_timer2
	);
	pr_info("LEVEL Stamp = %s\n", level_stamp);
#else
#if IS_ENABLED(CONFIG_MMPROFILE)
	mmprofile_log_ex(dvfsrc_mmp_events.level_change,
		MMPROFILE_FLAG_PULSE, dvfsrc_read(DVFSRC_CURRENT_LEVEL),
		dvfsrc_read(DVFSRC_TARGET_LEVEL));
#endif
#endif
}
static irqreturn_t helio_dvfsrc_interrupt(int irq, void *dev_id)
{
	u32 val;

	val = dvfsrc_read(DVFSRC_INT);
	dvfsrc_write(DVFSRC_INT_CLR, val);
	dvfsrc_write(DVFSRC_INT_CLR, 0x0);
	if (val & 0x2) {
		dvfsrc_write(DVFSRC_INT_EN,
			dvfsrc_read(DVFSRC_INT_EN) & ~(0x2));
		dvfsrc_get_timestamp(timeout_stamp);
#if 0
		dvfsrc_dump_reg(NULL);
		aee_kernel_warning("DVFSRC", "%s: timeout at spm.", __func__);
#endif
	}

	if (val & 0x4)
		dvfsrc_level_change_dump();

	return IRQ_HANDLED;
}

static ssize_t dvfsrc_dvfsrc_intr_log_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t dvfsrc_dvfsrc_intr_log_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int val = 0;
	struct helio_dvfsrc *dvfsrc;

	dvfsrc = dev_get_drvdata(dev);

	if (kstrtoint(buf, 10, &val))
		return -EINVAL;

	if (val == 1)
		dvfsrc_write(DVFSRC_INT_EN,
			dvfsrc_read(DVFSRC_INT_EN) | (0x4));
	else
		dvfsrc_write(DVFSRC_INT_EN,
			dvfsrc_read(DVFSRC_INT_EN) & ~(0x4));


	return count;
}

static DEVICE_ATTR(dvfsrc_intr_log, 0644,
		dvfsrc_dvfsrc_intr_log_show, dvfsrc_dvfsrc_intr_log_store);

static struct attribute *priv_dvfsrc_attrs[] = {
	&dev_attr_dvfsrc_intr_log.attr,
	NULL,
};

static struct attribute_group priv_dvfsrc_attr_group = {
	.name = "helio-dvfsrc",
	.attrs = priv_dvfsrc_attrs,
};

static int dvfsrc_resume(struct helio_dvfsrc *dvfsrc)
{
	dvfsrc_get_sys_stamp(sys_stamp);
	return 0;
}

int helio_dvfsrc_platform_init(struct helio_dvfsrc *dvfsrc)
{
	struct platform_device *pdev = to_platform_device(dvfsrc->dev);
	struct device_node *np = pdev->dev.of_node;
	u32 dvfs_ddr_floor = 0;
	int ret;

	mtk_rgu_cfg_dvfsrc(1);
#if 0
	dvfsrc->dvfsrc_flag = 0x3;
#endif
	if (of_property_read_u32(np, "dvfs_ddr_floor",
		(u32 *) &dvfs_ddr_floor))
		dvfs_ddr_floor = 0;

	if (dvfs_ddr_floor < 6)
		dvfsrc_set_sw_req6(dvfs_ddr_floor,
				DDR_SW_AP_MASK, DDR_SW_AP_SHIFT);

	helio_dvfsrc_enable(1);

	helio_dvfsrc_config();

	dvfsrc_get_sys_stamp(sys_stamp);

#ifdef AUTOK_ENABLE
	dvfsrc_autok_manager();
#endif

#if IS_ENABLED(CONFIG_MMPROFILE)
	mmprofile_enable(1);
	if (dvfsrc_mmp_events.dvfs_event == 0) {
		dvfsrc_mmp_events.dvfs_event = mmprofile_register_event(
			MMP_ROOT_EVENT, "VCORE_DVFS");
		dvfsrc_mmp_events.level_change =  mmprofile_register_event(
			dvfsrc_mmp_events.dvfs_event, "level_change");
		mmprofile_enable_event_recursive(
			dvfsrc_mmp_events.dvfs_event, 1);
	}
	mmprofile_start(1);
#endif

	dvfsrc->irq = platform_get_irq(pdev, 0);
	ret = request_irq(dvfsrc->irq, helio_dvfsrc_interrupt
		, IRQF_TRIGGER_HIGH, "dvfsrc", dvfsrc);
	if (ret)
		pr_info("dvfsrc interrupt no use\n");

	sysfs_merge_group(&dvfsrc->dev->kobj, &priv_dvfsrc_attr_group);
	dvfsrc->resume = dvfsrc_resume;

	vcore_reg_id = regulator_get(&pdev->dev, "vcore");
	if (!vcore_reg_id)
		pr_info("regulator_get vcore_reg_id failed\n");

	return 0;
}


void get_opp_info(char *p)
{
#if defined(CONFIG_FPGA_EARLY_PORTING)
	int pmic_val = 0;
	int vsram_val = 0;
#else
	int pmic_val = pmic_get_register_value(PMIC_VCORE_ADDR);
	int vsram_val = pmic_get_register_value(PMIC_VSRAM_OTHERS_ADDR);
#endif
	int vcore_uv = vcore_pmic_to_uv(pmic_val);
	int vsram_uv = vsram_pmic_to_uv(vsram_val);
#ifdef CONFIG_MTK_DRAMC
	int ddr_khz = get_dram_data_rate() * 1000;
#endif

	p += sprintf(p, "%-10s: %-8u uv  (PMIC: 0x%x)\n",
			"Vcore", vcore_uv, vcore_uv_to_pmic(vcore_uv));
	p += sprintf(p, "%-10s: %-8u uv  (PMIC: 0x%x)\n",
			"Vsram", vsram_uv, vsram_uv_to_pmic(vsram_uv));

#ifdef CONFIG_MTK_DRAMC
	p += sprintf(p, "%-10s: %-8u khz\n", "DDR", ddr_khz);
#endif
	p += sprintf(p, "%-10s: %08x\n", "PTPOD",
		get_devinfo_with_index(63));
	p += sprintf(p, "%-10s: %08x\n", "CPE",
		dvfsrc_read(DVFSRC_VCORE_MD2SPM0));

}


void get_dvfsrc_reg(char *p)
{
	char timestamp[TIME_STAMP_SIZE];

	dvfsrc_get_timestamp(timestamp);

	p += sprintf(p, "%-20s: 0x%08x\n",
			"BASIC_CONTROL",
			dvfsrc_read(DVFSRC_BASIC_CONTROL));
	p += sprintf(p,
	"%-20s: %08x, %08x, %08x, %08x\n",
			"SW_REQ 1~4",
			dvfsrc_read(DVFSRC_SW_REQ1),
			dvfsrc_read(DVFSRC_SW_REQ2),
			dvfsrc_read(DVFSRC_SW_REQ3),
			dvfsrc_read(DVFSRC_SW_REQ4));
	p += sprintf(p,
	"%-20s: %08x, %08x, %08x, %08x\n",
			"SW_REQ 5~8",
			dvfsrc_read(DVFSRC_SW_REQ5),
			dvfsrc_read(DVFSRC_SW_REQ6),
			dvfsrc_read(DVFSRC_SW_REQ7),
			dvfsrc_read(DVFSRC_SW_REQ8));
	p += sprintf(p, "%-20s: %d, %d, %d, %d, %d\n",
			"SW_BW_0~4",
			dvfsrc_read(DVFSRC_SW_BW_0),
			dvfsrc_read(DVFSRC_SW_BW_1),
			dvfsrc_read(DVFSRC_SW_BW_2),
			dvfsrc_read(DVFSRC_SW_BW_3),
			dvfsrc_read(DVFSRC_SW_BW_4));
	p += sprintf(p, "%-20s: 0x%08x\n",
			"ISP_HRT",
			dvfsrc_read(DVFSRC_ISP_HRT));
	p += sprintf(p, "%-20s: 0x%08x\n",
			"MD_HRT_BW",
			dvfsrc_get_md_bw());
	p += sprintf(p, "%-20s: 0x%08x, 0x%08x, 0x%08x\n",
			"DEBUG_STA",
			dvfsrc_read(DVFSRC_DEBUG_STA_0),
			dvfsrc_read(DVFSRC_DEBUG_STA_1),
			dvfsrc_read(DVFSRC_DEBUG_STA_2));
	p += sprintf(p, "%-20s: 0x%08x\n",
			"DVFSRC_INT",
			dvfsrc_read(DVFSRC_INT));
	p += sprintf(p, "%-20s: 0x%08x\n",
			"DVFSRC_INT_EN",
			dvfsrc_read(DVFSRC_INT_EN));
	p += sprintf(p, "%-20s: 0x%02x\n",
			"INFO_TOTAL_EMI_REQ",
			vcorefs_get_total_emi_status());
	p += sprintf(p, "%-20s: %d\n",
			"INFO_DDR_QOS_REQ",
			vcorefs_get_ddr_qos());
	p += sprintf(p, "%-20s: %d\n",
			"INFO_HRT_BW_REQ",
			vcorefs_get_hrt_bw_status());
	p += sprintf(p, "%-20s: %d\n",
			"INFO_HIFI_VCORE_REQ",
			vcorefs_get_hifi_vcore_status());
	p += sprintf(p, "%-20s: %d\n",
			"INFO_HIFI_DDR_REQ",
			vcorefs_get_hifi_ddr_status());
	p += sprintf(p, "%-20s: %d\n",
			"INFO_HIFI_RISINGREQ",
			vcorefs_get_hifi_rising_status());
	p += sprintf(p, "%-20s: %d\n",
			"INFO_MD_RISING_REQ",
			vcorefs_get_md_rising_status());
	p += sprintf(p, "%-20s: %d , 0x%08x\n",
			"INFO_SCP_VCORE_REQ",
			vcorefs_get_scp_req_status(),
			dvfsrc_read(DVFSRC_VCORE_REQUEST));
	p += sprintf(p, "%-20s: 0x%08x\n",
			"CURRENT_LEVEL",
			dvfsrc_read(DVFSRC_CURRENT_LEVEL));
	p += sprintf(p, "%-20s: 0x%08x\n",
			"TARGET_LEVEL",
			dvfsrc_read(DVFSRC_TARGET_LEVEL));
	p += sprintf(p, "%-20s: %s\n",
			"Current Timestamp", timestamp);
	p += sprintf(p, "%-20s: %s\n",
			"Force Start Time", force_start_stamp);
	p += sprintf(p, "%-20s: %s\n",
			"Force End Time", force_end_stamp);
	p += sprintf(p, "%-20s: %s\n",
			"Timeout Timestamp", timeout_stamp);
	p += sprintf(p, "%-20s: %s\n",
			"Sys Timestamp", sys_stamp);
}

void get_dvfsrc_record(char *p)
{
	int i, debug_reg;

	p += sprintf(p, "%-24s: 0x%08x\n",
			"DVFSRC_LAST",
			dvfsrc_read(DVFSRC_LAST));

	for (i = 0; i < 8; i++) {
		debug_reg = DVFSRC_RECORD_0_0 + (i * RECORD_SHIFT);
		p += sprintf(p, "[%d] %-20s: %08x,%08x,%08x,%08x\n",
			i,
			"DVFSRC_RECORD 0~3",
			dvfsrc_read(debug_reg),
			dvfsrc_read(debug_reg + 0x4),
			dvfsrc_read(debug_reg + 0x8),
			dvfsrc_read(debug_reg + 0xC));
		p += sprintf(p, "[%d] %-20s: %08x,%08x,%08x\n",
			i,
			"DVFSRC_RECORD 4~6",
			dvfsrc_read(debug_reg + 0x10),
			dvfsrc_read(debug_reg + 0x14),
			dvfsrc_read(debug_reg + 0x18));
	}
}

/* met profile table */
static unsigned int met_vcorefs_info[INFO_MAX];
static unsigned int met_vcorefs_src[SRC_MAX];

static char *met_info_name[INFO_MAX] = {
	"OPP",
	"FREQ",
	"VCORE",
	"x__SPM_LEVEL",
};

static char *met_src_name[SRC_MAX] = {
	"MD2SPM",
	"SRC_DDR_OPP",
	"DDR__SW_REQ1_SPM",
	"DDR__SW_REQ2_CM",
	"DDR__SW_REQ3_PMQOS",
	"DDR__SW_REQ4_MD",
	"DDR__SW_REQ8_MCUSYS",
	"DDR__QOS_BW",
	"DDR__EMI_TOTAL",
	"DDR__HRT_BW",
	"DDR__HIFI",
	"DDR__HIFI_LATENCY_IDX",
	"DDR__MD_LATENCY_IDX",
	"SRC_VCORE_OPP",
	"VCORE__SW_REQ3_PMQOS",
	"VCORE__SCP",
	"VCORE__HIFI",
	"SCP_REQ",
	"PMQOS_TATOL",
	"PMQOS_BW0",
	"PMQOS_BW1",
	"PMQOS_BW2",
	"PMQOS_BW3",
	"PMQOS_BW4",
	"TOTAL_EMI_BW",
	"HRT_MD_BW",
	"HRT_DISP_BW",
	"HRT_ISP_BW",
	"MD_SCENARIO",
	"HIFI_SCENARIO_IDX",
	"MD_EMI_LATENCY",
};

/* met profile function */
int vcorefs_get_num_opp(void)
{
	return VCORE_DVFS_OPP_NUM;
}
EXPORT_SYMBOL(vcorefs_get_num_opp);

int vcorefs_get_opp_info_num(void)
{
	return INFO_MAX;
}
EXPORT_SYMBOL(vcorefs_get_opp_info_num);

int vcorefs_get_src_req_num(void)
{
	return SRC_MAX;
}
EXPORT_SYMBOL(vcorefs_get_src_req_num);

char **vcorefs_get_opp_info_name(void)
{
	return met_info_name;
}
EXPORT_SYMBOL(vcorefs_get_opp_info_name);

char **vcorefs_get_src_req_name(void)
{
	return met_src_name;
}
EXPORT_SYMBOL(vcorefs_get_src_req_name);

__weak void pm_qos_trace_dbg_show_request(int pm_qos_class)
{
}

static DEFINE_RATELIMIT_STATE(tracelimit, 5 * HZ, 1);

static void vcorefs_trace_qos(void)
{
	if (__ratelimit(&tracelimit)) {
		pm_qos_trace_dbg_show_request(PM_QOS_DDR_OPP);
		pm_qos_trace_dbg_show_request(PM_QOS_VCORE_OPP);
	}
}

unsigned int *vcorefs_get_opp_info(void)
{
	met_vcorefs_info[INFO_OPP_IDX] = get_cur_vcore_dvfs_opp();
	met_vcorefs_info[INFO_FREQ_IDX] = get_cur_ddr_khz();
	met_vcorefs_info[INFO_VCORE_IDX] = get_cur_vcore_uv();
	met_vcorefs_info[INFO_SPM_LEVEL_IDX] = spm_get_dvfs_level();

	return met_vcorefs_info;
}
EXPORT_SYMBOL(vcorefs_get_opp_info);

static void vcorefs_get_src_ddr_req(void)
{
	unsigned int sw_req;

	met_vcorefs_src[DDR_OPP_IDX] =
		get_cur_ddr_opp();

	sw_req = dvfsrc_read(DVFSRC_SW_REQ1);
	met_vcorefs_src[DDR_SW_REQ1_SPM_IDX] =
		(sw_req >> DDR_SW_AP_SHIFT) & DDR_SW_AP_MASK;

	sw_req = dvfsrc_read(DVFSRC_SW_REQ2);
	met_vcorefs_src[DDR_SW_REQ2_CM_IDX] =
		(sw_req >> DDR_SW_AP_SHIFT) & DDR_SW_AP_MASK;

	sw_req = dvfsrc_read(DVFSRC_SW_REQ3);
	met_vcorefs_src[DDR_SW_REQ3_PMQOS_IDX] =
		(sw_req >> DDR_SW_AP_SHIFT) & DDR_SW_AP_MASK;

	sw_req = dvfsrc_read(DVFSRC_SW_REQ4);
	met_vcorefs_src[DDR_SW_REQ4_MD_IDX] =
		(sw_req >> DDR_SW_AP_SHIFT) & DDR_SW_AP_MASK;

	sw_req = dvfsrc_read(DVFSRC_SW_REQ8);
	met_vcorefs_src[DDR_SW_REQ8_MCUSYS_IDX] =
		(sw_req >> DDR_SW_AP_SHIFT) & DDR_SW_AP_MASK;

	met_vcorefs_src[DDR_QOS_BW_IDX] =
		vcorefs_get_ddr_qos();

	met_vcorefs_src[DDR_EMI_TOTAL_IDX] =
		vcorefs_get_emi_mon_gear();

	met_vcorefs_src[DDR_HRT_BW_IDX] =
		vcorefs_get_hrt_bw_status();

	met_vcorefs_src[DDR_HIFI_IDX] =
		vcorefs_get_hifi_ddr_status();

	met_vcorefs_src[DDR_HIFI_LATENCY_IDX] =
		vcorefs_get_hifi_rising_status();

	met_vcorefs_src[DDR_MD_LATENCY_IDX] =
		vcorefs_get_md_rising_status();
}

static void vcorefs_get_src_vcore_req(void)
{
	u32 sw_req;
	u32 scp_en;

	scp_en = vcorefs_get_scp_req_status();

	met_vcorefs_src[VCORE_OPP_IDX] =
		get_cur_vcore_opp();

	sw_req = dvfsrc_read(DVFSRC_SW_REQ3);
	met_vcorefs_src[VCORE_SW_REQ3_PMQOS_IDX] =
		(sw_req >> VCORE_SW_AP_SHIFT) & VCORE_SW_AP_MASK;

	if (scp_en) {
		sw_req = dvfsrc_read(DVFSRC_VCORE_REQUEST);
		met_vcorefs_src[VCORE_SCP_IDX] =
			(sw_req >> VCORE_SCP_GEAR_SHIFT) & VCORE_SCP_GEAR_MASK;
	} else
		met_vcorefs_src[VCORE_SCP_IDX] = 0;

	met_vcorefs_src[VCORE_HIFI_IDX] =
		vcorefs_get_hifi_vcore_status();
}

static void vcorefs_get_src_misc_info(void)
{
#ifdef CONFIG_MTK_EMI
		unsigned int total_bw_last = (get_emi_bwvl(0) & 0x7F) * 813;
#endif
	u32 qos_bw0, qos_bw1, qos_bw2, qos_bw3, qos_bw4;

	qos_bw0 = dvfsrc_read(DVFSRC_SW_BW_0);
	qos_bw1 = dvfsrc_read(DVFSRC_SW_BW_1);
	qos_bw2 = dvfsrc_read(DVFSRC_SW_BW_2);
	qos_bw3 = dvfsrc_read(DVFSRC_SW_BW_3);
	qos_bw4 = dvfsrc_read(DVFSRC_SW_BW_4);

	met_vcorefs_src[SRC_MD2SPM_IDX] =
		vcorefs_get_md_scenario();

	met_vcorefs_src[SRC_SCP_REQ_IDX] =
		vcorefs_get_scp_req_status();

	met_vcorefs_src[SRC_PMQOS_TATOL_IDX] =
		qos_bw0 + qos_bw1 + qos_bw2 + qos_bw3 + qos_bw4;

	met_vcorefs_src[SRC_PMQOS_BW0_IDX] =
		qos_bw0;

	met_vcorefs_src[SRC_PMQOS_BW1_IDX] =
		qos_bw1;

	met_vcorefs_src[SRC_PMQOS_BW2_IDX] =
		qos_bw2;

	met_vcorefs_src[SRC_PMQOS_BW3_IDX] =
		qos_bw3;

	met_vcorefs_src[SRC_PMQOS_BW4_IDX] =
		qos_bw4;

#ifdef CONFIG_MTK_EMI
	met_vcorefs_src[SRC_TOTAL_EMI_BW_IDX] =
		total_bw_last;
#endif

	met_vcorefs_src[SRC_HRT_MD_BW_IDX] =
		dvfsrc_get_md_bw();

	met_vcorefs_src[SRC_HRT_ISP_BW_IDX] =
		dvfsrc_read(DVFSRC_ISP_HRT);

	met_vcorefs_src[SRC_MD_SCENARIO_IDX] =
		vcorefs_get_md_scenario();

	met_vcorefs_src[SRC_HIFI_SCENARIO_IDX] =
		vcorefs_get_hifi_scenario();

	met_vcorefs_src[SRC_MD_EMI_LATENCY_IDX] =
		vcorefs_get_md_emi_latency_status();

}

unsigned int *vcorefs_get_src_req(void)
{
	vcorefs_get_src_ddr_req();
	vcorefs_get_src_vcore_req();
	vcorefs_get_src_misc_info();

	vcorefs_trace_qos();

	return met_vcorefs_src;
}
EXPORT_SYMBOL(vcorefs_get_src_req);

int get_cur_ddr_ratio(void)
{
	int idx;

	if (!is_dvfsrc_enabled())
		return 0;

	idx = get_cur_vcore_dvfs_opp();

	if (idx >= VCORE_DVFS_OPP_NUM)
		return 0;

	if ((get_ddr_opp(idx) < DDR_OPP_3) || (idx == 10))
		return 8;
	else
		return 4;
}
EXPORT_SYMBOL(get_cur_ddr_ratio);

