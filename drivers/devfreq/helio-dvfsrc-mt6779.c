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
#if 0
#include <mmdvfs_mgr.h>
#endif
#include <mt-plat/aee.h>

/*#define AUTOK_ENABLE*/
#define TIME_STAMP_SIZE 40
#define TIME_STAMP_SRAM_OFFSET 0x50

static struct reg_config dvfsrc_init_configs[][128] = {
	{
/* MISC */
		{ DVFSRC_TIMEOUT_NEXTREQ,  0x00000023 },
		{ DVFSRC_INT_EN,           0x00000003 },
		{ DVFSRC_QOS_EN,           0x0000407C },
/* QOS   */
		{ DVFSRC_DDR_REQUEST,      0x00004321 },
		{ DVFSRC_DDR_REQUEST3,     0x00000055 },
		{ DVFSRC_DDR_REQUEST5,     0x00321000 },
		{ DVFSRC_DDR_REQUEST7,     0x54000000 },
		{ DVFSRC_DDR_REQUEST8,     0x55000000 },
		{ DVFSRC_DDR_QOS0,         0x00000026 },
		{ DVFSRC_DDR_QOS1,         0x00000033 },
		{ DVFSRC_DDR_QOS2,         0x0000004C },
		{ DVFSRC_DDR_QOS3,         0x00000066 },
		{ DVFSRC_DDR_QOS4,         0x00000077 },
		{ DVFSRC_DDR_QOS5,         0x00000077 },
		{ DVFSRC_DDR_QOS6,         0x00000077 },
/* HRT  */
		{ DVFSRC_HRT_REQ_UNIT,      0x00000110 },
		{ DVSFRC_HRT_REQ_MD_URG,	0x00000000 },
		{ DVFSRC_HRT_REQ_MD_BW_0,	0x00000000 },
		{ DVFSRC_HRT_REQ_MD_BW_1,	0x00000000 },
		{ DVFSRC_HRT_REQ_MD_BW_2,	0x00000000 },
		{ DVFSRC_HRT_REQ_MD_BW_3,	0x00000000 },
		{ DVFSRC_HRT_REQ_MD_BW_4,   0x00000000 },
		{ DVFSRC_HRT_REQ_MD_BW_5,   0x00000000 },
		{ DVFSRC_HRT1_REQ_MD_BW_0,  0x00000000 },
		{ DVFSRC_HRT1_REQ_MD_BW_1,  0x00000000 },
		{ DVFSRC_HRT1_REQ_MD_BW_2,  0x00000000 },
		{ DVFSRC_HRT1_REQ_MD_BW_3,  0x00000000 },
		{ DVFSRC_HRT1_REQ_MD_BW_4,  0x00000000 },
		{ DVFSRC_HRT1_REQ_MD_BW_5,  0x00000000 },
		{ DVFSRC_HRT_HIGH,          0x09600640 },
		{ DVFSRC_HRT_HIGH_1,        0x12C00C80 },
		{ DVFSRC_HRT_HIGH_2,        0x19001900 },
		{ DVFSRC_HRT_HIGH_3,        0x00001900 },
		{ DVFSRC_HRT_LOW,           0x09600640 },
		{ DVFSRC_HRT_LOW_1,         0x12C00C80 },
		{ DVFSRC_HRT_LOW_2,         0x19001900 },
		{ DVFSRC_HRT_LOW_3,         0x00001900 },
		{ DVFSRC_HRT_REQUEST,       0x05554321 },
/* Rising */
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
		{ DVFSRC_BASIC_CONTROL,     0x0180004B },
		{ DVFSRC_CURRENT_FORCE,     0x00000001 },
		{ DVFSRC_BASIC_CONTROL,     0x0180404B },
		{ DVFSRC_BASIC_CONTROL,     0x0180014B },
		{ DVFSRC_CURRENT_FORCE,     0x00000000 },
		{ -1, 0 },
	},
	/* NULL */
	{
		{ -1, 0 },
	},
};

static DEFINE_SPINLOCK(force_req_lock);
static char	force_start_stamp[TIME_STAMP_SIZE];
static char	force_end_stamp[TIME_STAMP_SIZE];
static char	timeout_stamp[TIME_STAMP_SIZE];
static char	level_stamp[TIME_STAMP_SIZE];
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

static void dvfsrc_set_vcore_request(int data, int mask, int shift)
{
	dvfsrc_rmw(DVFSRC_VCORE_REQUEST, data, mask, shift);
}

static void dvfsrc_get_timestamp(char *p)
{
	u64 sys_time;
	u64 kernel_time;
	u64 usec;

	kernel_time = sched_clock_get_cyc(&sys_time);
	if (p) {
		usec = do_div(kernel_time, 1000000000);
		do_div(usec, 1000);
		snprintf(p, TIME_STAMP_SIZE, "%llu.%06llu, 0x%llx",
			kernel_time, usec, sys_time);
	} else {
		dvfsrc_sram_write(TIME_STAMP_SRAM_OFFSET,
			kernel_time);
		dvfsrc_sram_write(TIME_STAMP_SRAM_OFFSET + 0x4,
			kernel_time >> 32);
		dvfsrc_sram_write(TIME_STAMP_SRAM_OFFSET + 0x8,
			sys_time);
		dvfsrc_sram_write(TIME_STAMP_SRAM_OFFSET + 0xC,
			sys_time >> 32);
	}
}


static int is_dvfsrc_forced(void)
{
	return opp_forced;
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
	dvfsrc_write(DVFSRC_ISP_HRT, data >> 4); //16Mbytes unit
}

static u32 dvfsrc_calc_hrt_opp(int data)
{
	if (data < 0x0640)
		return DDR_OPP_5;
	else if (data < 0x0960)
		return DDR_OPP_4;
	else if (data < 0x0C80)
		return DDR_OPP_3;
	else if (data < 0x12C0)
		return DDR_OPP_2;
	else if (data < 0x1900)
		return DDR_OPP_1;
	else
		return DDR_OPP_0;
}

u32 dvfsrc_calc_isp_hrt_opp(int data)
{
	return dvfsrc_calc_hrt_opp((data >> 4) << 4);
}

int commit_data(int type, int data, int check_spmfw)
{
	int ret = 0;
	int level = 16, opp = 16;
	unsigned long flags;

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
			ret = dvfsrc_wait_for_completion(
					get_cur_vcore_opp() <= opp,
					DVFSRC_TIMEOUT);
		}
		spin_unlock_irqrestore(&force_req_lock, flags);
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
		if (is_turbo)
			val = dvfsrc_read(DVFSRC_HRT1_REQ_MD_BW_0 + index * 4);
		else
			val = dvfsrc_read(DVFSRC_HRT_REQ_MD_BW_0 + index * 4);

		val = (val >> shift) & MD_HRT_BW_MASK;
	}
	return val;
}

static u32 vcorefs_get_ddr_qos(void)
{
	unsigned int qos_total_bw = dvfsrc_read(DVFSRC_SW_BW_0) +
			   dvfsrc_read(DVFSRC_SW_BW_1) +
			   dvfsrc_read(DVFSRC_SW_BW_2) +
			   dvfsrc_read(DVFSRC_SW_BW_3) +
			   dvfsrc_read(DVFSRC_SW_BW_4);

	if (qos_total_bw < 0x26)
		return DDR_OPP_5;
	else if (qos_total_bw < 0x33)
		return DDR_OPP_4;
	else if (qos_total_bw < 0x4C)
		return DDR_OPP_3;
	else if (qos_total_bw < 0x66)
		return DDR_OPP_2;
	else if (qos_total_bw < 0x77)
		return DDR_OPP_1;
	else
		return DDR_OPP_0;

}

static u32 vcorefs_get_total_emi_status(void)
{
	u32 val;

	val = dvfsrc_read(DVFSRC_DEBUG_STA_2);

	val = (val >> DEBUG_STA2_EMI_TOTAL_SHIFT) & DEBUG_STA2_EMI_TOTAL_MASK;

	return val;
}

static u32 vcorefs_get_scp_req_status(void)
{
	u32 val;

	val = dvfsrc_read(DVFSRC_DEBUG_STA_2);

	val = (val >> DEBUG_STA2_SCP_SHIFT) & DEBUG_STA2_SCP_MASK;

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

static u32 vcorefs_get_hifi_vcore_status(void)
{
	u32 last, val;

	last = dvfsrc_read(DVFSRC_LAST);

	val = dvfsrc_read(DVFSRC_RECORD_0_4 + RECORD_SHIFT * last);

	val = (val >> RECORD_HIFI_VCORE_SHIFT) & RECORD_HIFI_VCORE_MASK;

	return val;
}

static u32 vcorefs_get_hifi_ddr_status(void)
{
	u32 last, val;

	last = dvfsrc_read(DVFSRC_LAST);

	val = dvfsrc_read(DVFSRC_RECORD_0_4 + RECORD_SHIFT * last);

	val = (val >> RECORD_HIFI_DDR_SHIFT) & RECORD_HIFI_DDR_SHIFT;

	return val;
}


static void dvfsrc_update_md_scenario(bool blank)
{
	/* TODO */
}

static int dvfsrc_fb_notifier_call(struct notifier_block *self,
		unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	int blank;

	if (event != FB_EVENT_BLANK)
		return 0;

	blank = *(int *)evdata->data;

	switch (blank) {
	case FB_BLANK_UNBLANK:
		dvfsrc_update_md_scenario(false);
		break;
	case FB_BLANK_POWERDOWN:
		dvfsrc_update_md_scenario(true);
		break;
	default:
		break;
	}

	return 0;
}

static struct notifier_block dvfsrc_fb_notifier = {
	.notifier_call = dvfsrc_fb_notifier_call,
};


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
#if 0
	struct mmdvfs_prepare_event evt_from_vcore = {
		MMDVFS_EVENT_PREPARE_CALIBRATION_START};

	/* notify MM DVFS for msdc autok start */
	mmdvfs_notify_prepare_action(&evt_from_vcore);
#endif
}

static void finish_autok_task(void)
{
	/* check if dvfs force is released */
	int force = pm_qos_request(PM_QOS_VCORE_DVFS_FORCE_OPP);
#if 0
	struct mmdvfs_prepare_event evt_from_vcore = {
		MMDVFS_EVENT_PREPARE_CALIBRATION_END};

	/* notify MM DVFS for msdc autok finish */
	mmdvfs_notify_prepare_action(&evt_from_vcore);
#endif
	if (force >= 0 && force < 12)
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
	int spmfw_idx = 0; //spm_get_spmfw_idx();
	struct reg_config *config;
	int idx = 0;

	if (spmfw_idx < 0)
		spmfw_idx = ARRAY_SIZE(dvfsrc_init_configs) - 1;

	config = dvfsrc_init_configs[spmfw_idx];

	while (config[idx].offset != -1) {
		dvfsrc_write(config[idx].offset, config[idx].val);
		idx++;
	}
}

static void dvfsrc_level_change_dump(void)
{
	u32 last;
	static u32 log_timer1, log_timer2;

	last = dvfsrc_read(DVFSRC_LAST);
	log_timer1 = dvfsrc_read(DVFSRC_RECORD_0_0 + (RECORD_SHIFT * last));
	log_timer2 = dvfsrc_read(DVFSRC_RECORD_0_1 + (RECORD_SHIFT * last));
	dvfsrc_get_timestamp(level_stamp);
	pr_info("DVFSRC change %08x %08x %d %08x %08x\n",
		dvfsrc_read(DVFSRC_CURRENT_LEVEL),
		dvfsrc_read(DVFSRC_TARGET_LEVEL),
		last,
		log_timer1,
		log_timer2
	);
	pr_info("LEVEL Stamp = %s\n", level_stamp);
}
static irqreturn_t helio_dvfsrc_interrupt(int irq, void *dev_id)
{
	u32 val;

	val = dvfsrc_read(DVFSRC_INT);
	dvfsrc_write(DVFSRC_INT_CLR, val);
	dvfsrc_write(DVFSRC_INT_CLR, 0x0);
	if (val & 0x2) {
		dvfsrc_get_timestamp(timeout_stamp);
		dvfsrc_dump_reg(NULL);
		aee_kernel_warning("DVFSRC", "%s: timeout at spm.", __func__);
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
		dvfsrc_write(DVFSRC_INT_EN, 0x00000007);
	else
		dvfsrc_write(DVFSRC_INT_EN, 0x00000003);

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


static int can_dvfsrc_enable(void)
{
	int enable = 0;

	return enable;
}

static int dvfsrc_resume(struct helio_dvfsrc *dvfsrc)
{
	dvfsrc_get_timestamp(NULL);
	return 0;
}

int helio_dvfsrc_platform_init(struct helio_dvfsrc *dvfsrc)
{
	struct platform_device *pdev = to_platform_device(dvfsrc->dev);
	int ret;

	mtk_rgu_cfg_dvfsrc(1);
	helio_dvfsrc_sram_reg_init();

	if (can_dvfsrc_enable())
		helio_dvfsrc_enable(1);
	else
		helio_dvfsrc_enable(0);

	helio_dvfsrc_config();

	dvfsrc_get_timestamp(NULL);

#ifdef AUTOK_ENABLE
	dvfsrc_autok_manager();
#endif

	fb_register_client(&dvfsrc_fb_notifier);

	dvfsrc->irq = platform_get_irq(pdev, 0);
	ret = request_irq(dvfsrc->irq, helio_dvfsrc_interrupt
		, IRQF_TRIGGER_HIGH, "dvfsrc", dvfsrc);

	sysfs_merge_group(&dvfsrc->dev->kobj, &priv_dvfsrc_attr_group);
	dvfsrc->resume = dvfsrc_resume;

	return 0;
}


void get_opp_info(char *p)
{
#if defined(CONFIG_FPGA_EARLY_PORTING)
	int pmic_val = 0;
#else
	int pmic_val = pmic_get_register_value(PMIC_VCORE_ADDR);
#endif
	int vcore_uv = vcore_pmic_to_uv(pmic_val);
#ifdef CONFIG_MTK_DRAMC
	int ddr_khz = get_dram_data_rate() * 1000;
#endif

	p += sprintf(p, "%-24s: %-8u uv  (PMIC: 0x%x)\n",
			"Vcore", vcore_uv, vcore_uv_to_pmic(vcore_uv));
#ifdef CONFIG_MTK_DRAMC
	p += sprintf(p, "%-24s: %-8u khz\n", "DDR", ddr_khz);
#endif
	p += sprintf(p, "%-24s: %-8u\n", "PTPOD",
		get_devinfo_with_index(63));

}


void get_dvfsrc_reg(char *p)
{
	p += sprintf(p, "%-24s: 0x%08x\n",
			"DVFSRC_BASIC_CONTROL",
			dvfsrc_read(DVFSRC_BASIC_CONTROL));
	p += sprintf(p,
	"%-24s: %08x, %08x, %08x, %08x\n",
			"DVFSRC_SW_REQ 1~4",
			dvfsrc_read(DVFSRC_SW_REQ1),
			dvfsrc_read(DVFSRC_SW_REQ2),
			dvfsrc_read(DVFSRC_SW_REQ3),
			dvfsrc_read(DVFSRC_SW_REQ4));
	p += sprintf(p,
	"%-24s: %08x, %08x, %08x, %08x\n",
			"DVFSRC_SW_REQ 5~8",
			dvfsrc_read(DVFSRC_SW_REQ5),
			dvfsrc_read(DVFSRC_SW_REQ6),
			dvfsrc_read(DVFSRC_SW_REQ7),
			dvfsrc_read(DVFSRC_SW_REQ8));
	p += sprintf(p, "%-24s: %d, %d, %d, %d, %d\n",
			"DVFSRC_SW_BW_0~4",
			dvfsrc_read(DVFSRC_SW_BW_0),
			dvfsrc_read(DVFSRC_SW_BW_1),
			dvfsrc_read(DVFSRC_SW_BW_2),
			dvfsrc_read(DVFSRC_SW_BW_3),
			dvfsrc_read(DVFSRC_SW_BW_4));
	p += sprintf(p, "%-24s: 0x%08x\n",
			"DVFSRC_ISP_HRT",
			dvfsrc_read(DVFSRC_ISP_HRT));
	p += sprintf(p, "%-24s: 0x%08x\n",
			"DVFSRC_MD_HRT_BW",
			dvfsrc_get_md_bw());
	p += sprintf(p, "%-24s: 0x%08x, 0x%08x, 0x%08x\n",
			"DVFSRC_DEBUG_STA 0~2",
			dvfsrc_read(DVFSRC_DEBUG_STA_0),
			dvfsrc_read(DVFSRC_DEBUG_STA_1),
			dvfsrc_read(DVFSRC_DEBUG_STA_2));
	p += sprintf(p, "%-24s: 0x%08x\n",
			"DVFSRC_INT",
			dvfsrc_read(DVFSRC_INT));
	p += sprintf(p, "%-24s: 0x%08x\n",
			"DVFSRC_INT_EN",
			dvfsrc_read(DVFSRC_INT_EN));
	p += sprintf(p, "%-24s: 0x%08x\n",
			"INFO_TOTAL_EMI_REQ",
			vcorefs_get_total_emi_status());
	p += sprintf(p, "%-24s: 0x%08x\n",
			"INFO_DDR_QOS_REQ",
			vcorefs_get_ddr_qos());
	p += sprintf(p, "%-24s: 0x%08x\n",
			"INFO_HRT_BW_REQ",
			vcorefs_get_hrt_bw_status());
	p += sprintf(p, "%-24s: 0x%08x\n",
			"INFO_HIFI_VCORE_REQ",
			vcorefs_get_hifi_vcore_status());
	p += sprintf(p, "%-24s: 0x%08x\n",
			"INFO_HIFI_DDR_REQ",
			vcorefs_get_hifi_ddr_status());
	p += sprintf(p, "%-24s: 0x%08x\n",
			"DVFSRC_CURRENT_LEVEL",
			dvfsrc_read(DVFSRC_CURRENT_LEVEL));
	p += sprintf(p, "%-24s: 0x%08x\n",
			"DVFSRC_TARGET_LEVEL",
			dvfsrc_read(DVFSRC_TARGET_LEVEL));
}

void get_dvfsrc_record(char *p)
{
	char timestamp[40];
	int i, debug_reg;

	dvfsrc_get_timestamp(timestamp);

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

	p += sprintf(p, "%-24s: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",
			"DVFSRC_DDR_QOS 0~3",
			dvfsrc_read(DVFSRC_DDR_QOS0),
			dvfsrc_read(DVFSRC_DDR_QOS1),
			dvfsrc_read(DVFSRC_DDR_QOS2),
			dvfsrc_read(DVFSRC_DDR_QOS3));

	p += sprintf(p, "%-24s: 0x%08x, 0x%08x, 0x%08x\n",
			"DVFSRC_DDR_QOS 4~6",
			dvfsrc_read(DVFSRC_DDR_QOS4),
			dvfsrc_read(DVFSRC_DDR_QOS5),
			dvfsrc_read(DVFSRC_DDR_QOS6));

	p += sprintf(p, "%-24s: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",
			"DVFSRC_DDR_REQUEST 1~3",
			dvfsrc_read(DVFSRC_DDR_REQUEST),
			dvfsrc_read(DVFSRC_DDR_REQUEST2),
			dvfsrc_read(DVFSRC_DDR_REQUEST3),
			dvfsrc_read(DVFSRC_DDR_REQUEST4));

	p += sprintf(p, "%-24s: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",
			"DVFSRC_DDR_REQUEST 4~8",
			dvfsrc_read(DVFSRC_DDR_REQUEST5),
			dvfsrc_read(DVFSRC_DDR_REQUEST6),
			dvfsrc_read(DVFSRC_DDR_REQUEST7),
			dvfsrc_read(DVFSRC_DDR_REQUEST8));

	p += sprintf(p, "%-24s: 0x%08x\n",
			"DVFSRC_VCORE_REQUEST",
			dvfsrc_read(DVFSRC_VCORE_REQUEST));

	p += sprintf(p,
			"%-24s: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",
			"DVFSRC_HRT_HIGH 0~3",
			dvfsrc_read(DVFSRC_HRT_HIGH),
			dvfsrc_read(DVFSRC_HRT_HIGH_1),
			dvfsrc_read(DVFSRC_HRT_HIGH_2),
			dvfsrc_read(DVFSRC_HRT_HIGH_3));

	p += sprintf(p,
			"%-24s: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",
			"DVFSRC_HRT_LOW 0~3",
			dvfsrc_read(DVFSRC_HRT_LOW),
			dvfsrc_read(DVFSRC_HRT_LOW_1),
			dvfsrc_read(DVFSRC_HRT_LOW_2),
			dvfsrc_read(DVFSRC_HRT_LOW_3));

	p += sprintf(p, "%-24s: 0x%08x\n",
			"DVFSRC_HRT_REQUEST",
			dvfsrc_read(DVFSRC_HRT_REQUEST));

	p += sprintf(p, "%-24s: %40s\n",
			"Current Timestamp", timestamp);
	p += sprintf(p, "%-24s: %40s\n",
			"Force Start Timestamp", force_start_stamp);
	p += sprintf(p, "%-24s: %40s\n",
			"Force End Timestamp", force_end_stamp);
	p += sprintf(p, "%-24s: %40s\n",
			"dvfsrc timeout Timestamp", timeout_stamp);
}

/* met profile table */
static unsigned int met_vcorefs_info[INFO_MAX];
static unsigned int met_vcorefs_src[SRC_MAX];

static char *met_info_name[INFO_MAX] = {
	"OPP",
	"FREQ",
	"VCORE",
	"SPM_LEVEL",
};

static char *met_src_name[SRC_MAX] = {
	"MD2SPM",
	"DDR_OPP",
	"DDR_SW_REQ1_SPM",
	"DDR_SW_REQ2_CM",
	"DDR_SW_REQ3_PMQOS",
	"DDR_SW_REQ4_MD",
	"DDR_SW_REQ8_MCUSYS",
	"DDR_QOS_BW",
	"DDR_EMI_TOTAL",
	"DDR_HRT_BW",
	"DDR_HIFI",
	"VCORE_OPP",
	"VCORE_SW_REQ3_PMQOS",
	"VCORE_SCP",
	"VCORE_HIFI",
	"SRC_SCP_REQ",
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
	unsigned int total_bw_status = vcorefs_get_total_emi_status();

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
		__builtin_ffs(total_bw_status);

	met_vcorefs_src[DDR_HRT_BW_IDX] =
		vcorefs_get_hrt_bw_status();

	met_vcorefs_src[DDR_HIFI_IDX] =
		vcorefs_get_hifi_ddr_status();

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

