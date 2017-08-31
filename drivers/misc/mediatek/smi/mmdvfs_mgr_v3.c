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

#include <linux/uaccess.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/clk.h>

#include <aee.h>

#include "mtk_gpu_utility.h"
#include "mtk_smi.h"
#include "mtk_vcorefs_manager.h"
#include "mach/mtk_freqhopping.h"
#include "mmdvfs_mgr.h"
#include "mmdvfs_config_util.h"
#include "mmdvfs_internal.h"

#undef pr_fmt
#define pr_fmt(fmt) "[" MMDVFS_LOG_TAG "]" fmt

#define MMDVFS_ENABLE	1

#if defined(MMDVFS_ENABLE) && defined(MMDVFS_USE_FLIPER)
#include <mach/fliper.h>
#endif

/* MMDVFS legacy APIs */

#define MMDVFS_CLK_SWITCH_CB_MAX 16
#define MMDVFS_CLK_SWITCH_CLIENT_MSG_MAX 20

/* Legacy MMCLK change notifier. Keep the API now but */
/* it will be phased out (ISP may using the functionality)*/
static void notify_camsys_clk_change(int ori_mmsys_clk_mode, int update_mmsys_clk_mode);
static int mmsys_clk_change_notify_checked(clk_switch_cb func, int ori_mmsys_clk_mode,
int update_mmsys_clk_mode, char *msg);

/* Keep to adapt the new MMDVFS profile management method */
/* to the legacy IOCTL command quickly */
static int mmdvfs_query(MTK_SMI_BWC_SCEN scenario,
MTK_MMDVFS_CMD *cmd);

/* For legacy mmclk change notification and will be phase out */
static clk_switch_cb quick_mmclk_cbs[MMDVFS_CLK_SWITCH_CB_MAX];

/* Record the current CLK (debug only) */
static int current_mmsys_clk = MMSYS_CLK_MEDIUM;

/* Record the current step */
static int g_mmdvfs_current_step;

/* Record the enabled client */
static unsigned int g_mmdvfs_concurrency;
static MTK_SMI_BWC_MM_INFO *g_mmdvfs_info;
static MTK_MMDVFS_CMD g_mmdvfs_cmd;


typedef struct {
	spinlock_t scen_lock;
	int is_mmdvfs_start;
	int is_vp_high_fps_enable;
	int is_mhl_enable;
	int is_wfd_enable;
	int is_mjc_enable;
	int is_boost_disable;
} mmdvfs_context_struct;

/* mmdvfs_query() return value, remember to sync with user space */
typedef enum {
	MMDVFS_STEP_LOW = 0, MMDVFS_STEP_HIGH,
	MMDVFS_STEP_LOW2LOW, /* LOW */
	MMDVFS_STEP_HIGH2LOW, /* LOW */
	MMDVFS_STEP_LOW2HIGH, /* HIGH */
	MMDVFS_STEP_HIGH2HIGH,
/* HIGH */
} mmdvfs_step_enum;


static mmdvfs_context_struct g_mmdvfs_mgr_cntx;
static mmdvfs_context_struct * const g_mmdvfs_mgr = &g_mmdvfs_mgr_cntx;

static int mmdvfs_get_default_step(void)
{
	return MMDVFS_FINE_STEP_UNREQUEST;
}

static int mmdvfs_get_current_fine_step(void)
{
	return g_mmdvfs_current_step;
}

static int mmdvfs_determine_fine_step(int scenario, int sensor_size, int feature_flag, int sensor_fps,
	int codec_width, int codec_height) {

	int mmdvfs_fine_step = MMDVFS_FINE_STEP_UNREQUEST;

	struct mmdvfs_cam_property cam_setting = {0, 0, 0};
	struct mmdvfs_video_property codec_setting = {0, 0, 0};

	cam_setting.sensor_size = sensor_size;
	cam_setting.feature_flag = feature_flag;
	cam_setting.fps = sensor_fps;

	codec_setting.width = codec_width;
	codec_setting.height = codec_height;

	mmdvfs_fine_step = g_mmdvfs_adaptor->determine_mmdvfs_step(g_mmdvfs_adaptor,
		scenario, &cam_setting, &codec_setting);

	return mmdvfs_fine_step;

}

static int mmdvfs_query(MTK_SMI_BWC_SCEN scenario,
MTK_MMDVFS_CMD *cmd)
{
	int sensor_size = 0;
	int camera_mode = 0;
	int sensor_fps = 0;

		/* use default info */
	if (cmd == NULL) {
		MMDVFSMSG("mmdvfs_query: cmd is NULL, scen=%d\n", scenario);
	} else {
		sensor_size = cmd->sensor_size;
		camera_mode = cmd->camera_mode;
		sensor_fps = cmd->sensor_fps;
	}

	return mmdvfs_determine_fine_step(scenario, sensor_size, camera_mode, sensor_fps,
		g_mmdvfs_info->video_record_size[0], g_mmdvfs_info->video_record_size[1]);
}


int mmdvfs_get_stable_isp_clk(void)
{
	int legacy_mm_step = MMSYS_CLK_LOW;
	int cam_scens = ((1 << SMI_BWC_SCEN_VR) |
		(1 << SMI_BWC_SCEN_VR_SLOW) |
		(1 << SMI_BWC_SCEN_VSS) |
		(1 << SMI_BWC_SCEN_CAM_PV) |
		(1 << SMI_BWC_SCEN_CAM_CP) |
		(1 << SMI_BWC_SCEN_ICFP) |
		(1 << MMDVFS_SCEN_ISP));
	int cam_clk_opp = g_mmdvfs_step_util->get_clients_clk_opp(
		g_mmdvfs_step_util, g_mmdvfs_adaptor,
		cam_scens, MMDVFS_CLK_MUX_TOP_CAM_SEL);

	if (cam_clk_opp != -1)
		legacy_mm_step = g_mmdvfs_step_util->get_legacy_mmclk_step_from_mmclk_opp(
			g_mmdvfs_step_util,	cam_clk_opp);

	if (legacy_mm_step < 0 || legacy_mm_step >= MMDVFS_MMSYS_CLK_COUNT) {
		MMDVFSDEBUG(3, "mmdvfs_get_stable_isp_clk: invalid legacy mmclk return:%d\n",
		legacy_mm_step);
		legacy_mm_step = MMSYS_CLK_LOW;
	}

	return legacy_mm_step;
}

static void mmdvfs_update_cmd(MTK_MMDVFS_CMD *cmd)
{
	if (cmd == NULL)
		return;

	if (cmd->sensor_size)
		g_mmdvfs_cmd.sensor_size = cmd->sensor_size;

	if (cmd->sensor_fps)
		g_mmdvfs_cmd.sensor_fps = cmd->sensor_fps;

	/* MMDVFSMSG("update cm %d\n", cmd->camera_mode); */
	g_mmdvfs_cmd.camera_mode = cmd->camera_mode;
}

/* delay 4 seconds to go LPM to workaround camera ZSD + PIP issue */
static void mmdvfs_cam_work_handler(struct work_struct *work)
{
	/* MMDVFSMSG("CAM handler %d\n", jiffies_to_msecs(jiffies)); */
	mmdvfs_set_fine_step(MMDVFS_CAM_MON_SCEN, MMDVFS_FINE_STEP_UNREQUEST);

}
static DECLARE_DELAYED_WORK(g_mmdvfs_cam_work, mmdvfs_cam_work_handler);

static void mmdvfs_stop_cam_monitor(void)
{
	cancel_delayed_work_sync(&g_mmdvfs_cam_work);
}

static void mmdvfs_start_cam_monitor(int scen, int delay_hz)
{

	if (g_mmdvfs_mgr->is_boost_disable) {
		MMDVFSMSG("MMDVFS boost (cam monitor) is disabled!!\n");
		return;
	}

	mmdvfs_stop_cam_monitor();

	/* Set to MAX clk opp here */
	mmdvfs_set_fine_step(MMDVFS_CAM_MON_SCEN, MMDVFS_FINE_STEP_OPP0);
	schedule_delayed_work(&g_mmdvfs_cam_work, delay_hz * HZ);
}


int mmdvfs_set_corse_step(int scenario, mmdvfs_voltage_enum step)
{
	int fine_step_opp = MMDVFS_FINE_STEP_UNREQUEST;

	if (step == MMDVFS_VOLTAGE_HIGH)
		fine_step_opp = MMDVFS_FINE_STEP_OPP0;
	else
		fine_step_opp = MMDVFS_FINE_STEP_UNREQUEST;

	mmdvfs_set_fine_step(scenario, step);
	return 0;
}

/* The legacy set step function can only support set corse step (HPM/ NON-HPM)*/
int mmdvfs_set_step(MTK_SMI_BWC_SCEN scenario, mmdvfs_voltage_enum step)
{
	return mmdvfs_set_corse_step(scenario, step);
}


int mmdvfs_internal_set_fine_step(MTK_SMI_BWC_SCEN smi_scenario, int mmdvfs_step)
{
	int original_step = 0;
	int final_step = MMDVFS_FINE_STEP_UNREQUEST;
	int legacy_clk = -1;

	if ((smi_scenario >= (MTK_SMI_BWC_SCEN)MMDVFS_SCEN_COUNT) || (smi_scenario
	< SMI_BWC_SCEN_NORMAL)) {
		MMDVFSERR("invalid scenario\n");
		return -1;
	}

	spin_lock(&g_mmdvfs_mgr->scen_lock);
	original_step = g_mmdvfs_current_step;
	final_step = g_mmdvfs_step_util->set_step(g_mmdvfs_step_util, mmdvfs_step, smi_scenario);
	g_mmdvfs_current_step = final_step;
	spin_unlock(&g_mmdvfs_mgr->scen_lock);

	/* Change HW configuration */
	g_mmdvfs_adaptor->apply_hw_configurtion_by_step(g_mmdvfs_adaptor, final_step, original_step);

	notify_camsys_clk_change(original_step, final_step);

	legacy_clk = mmdvfs_get_stable_isp_clk();

	if (g_mmdvfs_rt_debug_disable_mask && ((1 << smi_scenario) & (*g_mmdvfs_rt_debug_disable_mask)))
		MMDVFSDEBUG(3,
		"Set scen:(%d,0x%x) step:(%d,%d,0x%x,0x%x,0x%x,0x%x),C(%d,%d,0x%x),I(%d,%d),CLK:%d\n",
		smi_scenario, g_mmdvfs_concurrency, mmdvfs_step, final_step,
		g_mmdvfs_step_util->mmdvfs_concurrency_of_opps[0],
		g_mmdvfs_step_util->mmdvfs_concurrency_of_opps[1],
		g_mmdvfs_step_util->mmdvfs_concurrency_of_opps[2],
		g_mmdvfs_step_util->mmdvfs_concurrency_of_opps[3],
		g_mmdvfs_cmd.sensor_size, g_mmdvfs_cmd.sensor_fps,
		g_mmdvfs_cmd.camera_mode,
		g_mmdvfs_info->video_record_size[0], g_mmdvfs_info->video_record_size[1], legacy_clk);
	else
		MMDVFSMSG("Set scen:(%d,0x%x) step:(%d,%d,0x%x,0x%x,0x%x,0x%x),C(%d,%d,0x%x),I(%d,%d),CLK:%d\n",
		smi_scenario, g_mmdvfs_concurrency, mmdvfs_step, final_step,
		g_mmdvfs_step_util->mmdvfs_concurrency_of_opps[0],
		g_mmdvfs_step_util->mmdvfs_concurrency_of_opps[1],
		g_mmdvfs_step_util->mmdvfs_concurrency_of_opps[2],
		g_mmdvfs_step_util->mmdvfs_concurrency_of_opps[3],
		g_mmdvfs_cmd.sensor_size, g_mmdvfs_cmd.sensor_fps,
		g_mmdvfs_cmd.camera_mode,
		g_mmdvfs_info->video_record_size[0], g_mmdvfs_info->video_record_size[1], legacy_clk);
	return 0;
}


void mmdvfs_internal_notify_vcore_calibration(struct mmdvfs_prepare_action_event *event)
{
	if (event->event_type == MMDVFS_EVENT_PREPARE_CALIBRATION_START) {
		g_mmdvfs_mgr->is_mmdvfs_start = 0;
		MMDVFSMSG("mmdvfs service is disabled for vcore calibration\n");
	} else if (event->event_type  == MMDVFS_EVENT_PREPARE_CALIBRATION_END) {
		g_mmdvfs_mgr->is_mmdvfs_start = 1;
		MMDVFSMSG("mmdvfs service has been enabled\n");
	} else {
		MMDVFSMSG("mmdvfs_internal_notify_vcore_calibration: unknown status code:%d\n",
		event->event_type);
	}
}

int mmdvfs_set_fine_step_force(MTK_SMI_BWC_SCEN smi_scenario,
	int mmdvfs_step)
{
	int ret = 0;

	/* Update HW runtime option */
	g_mmdvfs_adaptor->enable_vcore = 1;
	g_mmdvfs_adaptor->enable_clk_mux = 1;
	ret = mmdvfs_internal_set_fine_step(smi_scenario, mmdvfs_step);

	/* recover the original setting */
	g_mmdvfs_adaptor->enable_vcore = !is_mmdvfs_disabled();
	g_mmdvfs_adaptor->enable_clk_mux = !is_mmdvfs_freq_mux_disabled();

	return ret;
}

int mmdvfs_set_fine_step(MTK_SMI_BWC_SCEN smi_scenario, int mmdvfs_step)
{
	if (is_mmdvfs_disabled() || g_mmdvfs_mgr->is_mmdvfs_start == 0) {
		MMDVFSMSG("MMDVFS is disable, request denalied; scen:%d, step:%d\n",
		smi_scenario, mmdvfs_step);
		return 0;
	}

	/* Update HW runtime option */
	g_mmdvfs_adaptor->enable_vcore = !is_mmdvfs_disabled();
	g_mmdvfs_adaptor->enable_pll_hopping = !is_mmdvfs_freq_hopping_disabled();
	g_mmdvfs_adaptor->enable_clk_mux = !is_mmdvfs_freq_mux_disabled();

	return mmdvfs_internal_set_fine_step(smi_scenario, mmdvfs_step);

}


static int handle_step_mmmclk_set(MTK_MMDVFS_CMD *cmd)
{
	int mmdvfs_step_request = 0;
	int mmclk_request = 0;

	if (cmd == NULL) {
		MMDVFSMSG("step ioctl cmd can't be NULL\n");
		return -1;
	}

	/* Get step from the command (bit 0-7) */
	mmdvfs_step_request = cmd->step & MMDVFS_IOCTL_CMD_STEP_FIELD_MASK;
	/* Get clk from the command (bit 8-15) */
	mmclk_request = (cmd->step & MMDVFS_IOCTL_CMD_MMCLK_FIELD_MASK) >> MMDVFS_IOCTL_CMD_STEP_FIELD_LEN;

	if (mmdvfs_step_request < MMDVFS_FINE_STEP_UNREQUEST ||
		mmdvfs_step_request >= g_mmdvfs_adaptor->step_num) {
		MMDVFSMSG("invalid step (%d)\n", mmdvfs_step_request);
	} else {
		MMDVFSMSG("Request step=%d, (mmclk %d is ignaored) from ioctl\n", mmdvfs_step_request, mmclk_request);
		mmdvfs_set_fine_step(cmd->scen, mmdvfs_step_request);
	}
	return 0;
}

void mmdvfs_handle_cmd(MTK_MMDVFS_CMD *cmd)
{
	if (is_mmdvfs_disabled()) {
		MMDVFSMSG("MMDVFS is disabled\n");
		return;
	}

	/* MMDVFSMSG("MMDVFS handle cmd %u s %d\n", cmd->type, cmd->scen); */

	switch (cmd->type) {
	case MTK_MMDVFS_CMD_TYPE_SET:
		/* save cmd */
		mmdvfs_update_cmd(cmd);

		if (!(g_mmdvfs_concurrency & (1 << cmd->scen))) {
			/*MMDVFSMSG("invalid set scen %d\n", cmd->scen); */
			cmd->ret = -1;
		} else {
			/* determine the step and apply the HW setting */
			cmd->ret = mmdvfs_set_fine_step(cmd->scen,	mmdvfs_query(cmd->scen, cmd));

		}
		break;

	case MTK_MMDVFS_CMD_TYPE_QUERY: { /* query with some parameters */
		{

			int query_fine_step = mmdvfs_query(cmd->scen, cmd);
			int current_fine_step =	mmdvfs_get_current_fine_step();

			/* Compare the step and return the result */
			if (query_fine_step == current_fine_step) {
				if (query_fine_step == MMDVFS_FINE_STEP_UNREQUEST)
					cmd->ret = (unsigned int)MMDVFS_STEP_LOW2LOW;
				else if (query_fine_step == MMDVFS_FINE_STEP_OPP0)
					cmd->ret = (unsigned int)MMDVFS_STEP_HIGH2HIGH;
			} else if (query_fine_step > current_fine_step) {
				if (current_fine_step == MMDVFS_FINE_STEP_UNREQUEST)
					cmd->ret = (unsigned int)MMDVFS_STEP_LOW2HIGH;
				else
					cmd->ret = (unsigned int)MMDVFS_STEP_HIGH2LOW;
			} else {
				if (query_fine_step == MMDVFS_FINE_STEP_UNREQUEST)
					cmd->ret = (unsigned int)MMDVFS_STEP_HIGH2LOW;
				else
					cmd->ret = (unsigned int)MMDVFS_STEP_LOW2HIGH;
			}
		}
		break;

	case MTK_MMDVFS_CMD_TYPE_GET:
		{
				cmd->ret = 0;
				/* Put step in the command (bit 0-7) */
				cmd->ret = g_mmdvfs_current_step & MMDVFS_IOCTL_CMD_STEP_FIELD_MASK;
				/* Put mmclk in the command (bit 8-15), but not used now */
				/* cmd->ret |= (current_mmsys_clk << MMDVFS_IOCTL_CMD_STEP_FIELD_LEN) */
				/* & MMDVFS_IOCTL_CMD_MMCLK_FIELD_MASK; */

				MMDVFSMSG("Current step query result: %d, 0x%x\n",
				g_mmdvfs_current_step, cmd->ret);
		}
		break;

	case MTK_MMDVFS_CMD_TYPE_CONFIG:
		g_mmdvfs_mgr->is_boost_disable = cmd->boost_disable;
		MMDVFSMSG("Config: is_boost_disable=%d\n", g_mmdvfs_mgr->is_boost_disable);
		break;

	case MTK_MMDVFS_CMD_TYPE_STEP_SET:
		/* Get the target step from step field */
		cmd->ret = handle_step_mmmclk_set(cmd);
		break;
	}

	default:
		MMDVFSMSG("invalid mmdvfs cmd\n");
		dump_stack();
		break;
	}
}

#define MMDVFS_CAMERA_SCENARIOS_MASK	((1<<SMI_BWC_SCEN_VR) | \
					(1<<SMI_BWC_SCEN_VR_SLOW) | \
					(1<<SMI_BWC_SCEN_ICFP) | \
					(1<<SMI_BWC_SCEN_VSS) | \
					(1<<SMI_BWC_SCEN_VENC) | \
					(1<<SMI_BWC_SCEN_CAM_PV) | \
					(1<<SMI_BWC_SCEN_CAM_CP))

void mmdvfs_notify_scenario_exit(MTK_SMI_BWC_SCEN scen)
{
	if (is_mmdvfs_disabled()) {
		MMDVFSMSG("MMDVFS is disabled\n");
		return;
	}

	if (is_force_max_mmsys_clk()) {
		MMDVFSMSG("MMDVFS is always high\n");
		return;
	}

	/* MMDVFSMSG("leave %d\n", scen); */
	if (scen == SMI_BWC_SCEN_WFD)
		g_mmdvfs_mgr->is_wfd_enable = 0;

	if (scen == SMI_BWC_SCEN_VP_HIGH_FPS)
		g_mmdvfs_mgr->is_vp_high_fps_enable = 0;

	/* Boost for ISP related scenario */
	if ((1 << scen & MMDVFS_CAMERA_SCENARIOS_MASK))
		mmdvfs_start_cam_monitor(scen, 8);

	/* If the scenario is defined in disable_auto_control_mask */
	/* mmdvfs_mgr doesn't change the step automatically.       */
	/* The kernel driver of the scenarios will change the step */
	/* by mmdvfs_set_fine_step directly */
	if (!((1 << scen) & g_mmdvfs_adaptor->disable_auto_control_mask))
		mmdvfs_set_fine_step(scen, MMDVFS_FINE_STEP_UNREQUEST);
	/* reset scenario voltage to default when it exits */
	/* Also force the system to leave low low mode */

}


void mmdvfs_notify_scenario_enter(MTK_SMI_BWC_SCEN scen)
{
	int mmdvfs_fine_step = MMDVFS_FINE_STEP_UNREQUEST;


	if (is_mmdvfs_disabled()) {
		MMDVFSMSG("MMDVFS is disabled\n");
		return;
	}

	if (is_force_max_mmsys_clk()) {
		MMDVFSMSG("MMDVFS is always high\n");
		return;
	}

	/* If the scenario is defined in disable_auto_control_mask */
	/* mmdvfs_mgr doesn't change the step automatically.       */
	/* The kernel driver of the scenarios will change the step */
	/* by mmdvfs_set_fine_step directly */

	if (!((1 << scen) & g_mmdvfs_adaptor->disable_auto_control_mask)) {
	mmdvfs_fine_step = mmdvfs_determine_fine_step(scen, g_mmdvfs_cmd.sensor_size,
		g_mmdvfs_cmd.camera_mode, g_mmdvfs_cmd.sensor_fps,
		g_mmdvfs_info->video_record_size[0], g_mmdvfs_info->video_record_size[1]);
	mmdvfs_set_fine_step(scen, mmdvfs_fine_step);
	}

	/* Boost for ISP related scenario */
	if ((1 << scen & MMDVFS_CAMERA_SCENARIOS_MASK))
		mmdvfs_start_cam_monitor(scen, 8);

	/* Record the engine status for debugging */
	if (scen == SMI_BWC_SCEN_WFD)
		g_mmdvfs_mgr->is_wfd_enable = 1;

	if (scen == SMI_BWC_SCEN_VP_HIGH_FPS)
		g_mmdvfs_mgr->is_vp_high_fps_enable = 1;

}



void mmdvfs_init(MTK_SMI_BWC_MM_INFO *info)
{

	spin_lock_init(&g_mmdvfs_mgr->scen_lock);

	/* set current step as the default step */
	g_mmdvfs_current_step = mmdvfs_get_default_step();

	g_mmdvfs_info = info;

	mmdvfs_config_util_init();

}

/* To be implemented */
void mmdvfs_mhl_enable(int enable)
{
	int mmdvfs_fine_step = mmdvfs_determine_fine_step(MMDVFS_SCEN_MHL, 0, 0, 0,	0, 0);

	g_mmdvfs_mgr->is_mhl_enable = enable;

	if (enable)
		mmdvfs_set_fine_step(MMDVFS_SCEN_MHL, mmdvfs_fine_step);
	else
		mmdvfs_set_fine_step(MMDVFS_SCEN_MHL, MMDVFS_FINE_STEP_UNREQUEST);
}

void mmdvfs_mjc_enable(int enable)
{
	g_mmdvfs_mgr->is_mjc_enable = enable;
}

void mmdvfs_notify_scenario_concurrency(unsigned int u4Concurrency)
{
	/*
	 * DO NOT CALL VCORE DVFS API HERE. THIS FUNCTION IS IN SMI SPIN LOCK.
	 */
	g_mmdvfs_concurrency = u4Concurrency;
}

int mmdvfs_is_default_step_need_perf(void)
{
	MMDVFSMSG("mmdvfs_is_default_step_need_perf is not supported in this platform\n");
	return 0;
}

/* switch MM CLK callback from VCORE DVFS driver */
void mmdvfs_mm_clock_switch_notify(int is_before, int is_to_high)
{
	MMDVFSMSG("mmdvfs_mm_clock_switch_notify is not supported in this platform\n");
}

int register_mmclk_switch_cb(clk_switch_cb notify_cb, clk_switch_cb notify_cb_nolock)
{
	MMDVFSMSG("register_mmclk_switch_cb is deplicated\n");
	return 0;
}

/* This desing is only for CLK Mux switch relate flows */
int mmdvfs_notify_mmclk_switch_request(int event)
{
	MMDVFSMSG("mmdvfs_notify_mmclk_switch_request is deplicated: %d\n", event);
	return 0;
}


int mmdvfs_register_mmclk_switch_cb(clk_switch_cb notify_cb, int mmdvfs_client_id)
{
	if (mmdvfs_client_id >= 0 && mmdvfs_client_id < MMDVFS_CLK_SWITCH_CB_MAX) {
		quick_mmclk_cbs[mmdvfs_client_id] = notify_cb;
	} else{
		MMDVFSMSG("clk_switch_cb register failed: id=%d\n", mmdvfs_client_id);
		return 1;
	}
	return 0;
}

static int mmsys_clk_change_notify_checked(clk_switch_cb func, int ori_mmsys_clk_mode,
int update_mmsys_clk_mode, char *msg)
{
	if (func == NULL) {
		MMDVFSMSG("notify_cb_func is NULL, not invoked: %s, (%d,%d)\n", msg, ori_mmsys_clk_mode,
		update_mmsys_clk_mode);
	} else {
		/*
		 * MMDVFSMSG("notify_cb_func: %s, (%d,%d)\n", msg, ori_mmsys_clk_mode,
		 * update_mmsys_clk_mode);
		*/
		func(ori_mmsys_clk_mode, update_mmsys_clk_mode);
		return 1;
	}
	return 0;
}

static void notify_camsys_clk_change(int ori_mmdvfs_step, int update_mmdvfs_step)
{
	int i = 0;
	int ori_cam_clk_mode = 0;
	int update_cam_clk_mode = 0;
	char msg[MMDVFS_CLK_SWITCH_CLIENT_MSG_MAX] = "";

	if (ori_mmdvfs_step == -1)
		ori_mmdvfs_step = g_mmdvfs_adaptor->step_num - 1;
	if (update_mmdvfs_step == -1)
		update_mmdvfs_step = g_mmdvfs_adaptor->step_num - 1;

	if (ori_mmdvfs_step < 0 || ori_mmdvfs_step >= g_mmdvfs_adaptor->step_num
		|| update_mmdvfs_step < 0 || update_mmdvfs_step >= g_mmdvfs_adaptor->step_num) {
		MMDVFSMSG("invalid step change %d --> %d", ori_mmdvfs_step, update_mmdvfs_step);
		return;
	}

	ori_cam_clk_mode = g_mmdvfs_adaptor->get_cam_sys_clk(g_mmdvfs_adaptor, ori_mmdvfs_step);
	update_cam_clk_mode = g_mmdvfs_adaptor->get_cam_sys_clk(g_mmdvfs_adaptor, update_mmdvfs_step);

	for (i = 0; i < MMDVFS_CLK_SWITCH_CB_MAX; i++) {
		snprintf(msg, MMDVFS_CLK_SWITCH_CLIENT_MSG_MAX, "id=%d", i);
		if (quick_mmclk_cbs[i] != NULL)
			mmsys_clk_change_notify_checked(quick_mmclk_cbs[i], ori_cam_clk_mode,
			update_cam_clk_mode, msg);
	}
}

/* Default step configurtion (after boot up) */
static unsigned int delayed_default_step_finished;

void mmdvfs_default_step_set(int default_step)
{
	if (is_force_max_mmsys_clk()) {
		MMDVFSMSG("Forcing max mm clks is enabled\n");
		mmdvfs_set_fine_step_force(MMDVFS_MGR, default_step);
	}

}

static void mmdvfs_default_step_delayed(struct work_struct *work)
{
	if (force_always_on_mm_clks())
		mmdvfs_debug_set_mmdvfs_clks_enabled(1);

	/* Set default high berfore MMDVFS feature is enabled */
	if (is_force_max_mmsys_clk()) {
		mmdvfs_default_step_set(MMDVFS_FINE_STEP_OPP0);
		delayed_default_step_finished = 1;
	}
}

static DECLARE_DELAYED_WORK(g_mmdvfs_set_default_step_delayed,
	mmdvfs_default_step_delayed);

void mmdvfs_default_start_delayed_setting(void)
{
		schedule_delayed_work(&g_mmdvfs_set_default_step_delayed, 60 * HZ);
}

void mmdvfs_default_stop_delayed_setting(void)
{
	/* Set default high berfore MMDVFS feature is enabled */
	if (is_force_max_mmsys_clk() && (delayed_default_step_finished == 0))
		cancel_delayed_work_sync(&g_mmdvfs_set_default_step_delayed);
}

/* Used for MMDVFS before CLK controlling is ready */
static unsigned int mm_clks_enabled;

void mmdvfs_debug_set_mmdvfs_clks_enabled(int clk_enable_request)
{
	int clk_idx = 0;
	int ccf_ret = 0;
	int always_on_mask = force_always_on_mm_clks();

	if (mm_clks_enabled == 0 && clk_enable_request == 0) {
		MMDVFSMSG("mmdvfs_debug_set_mmdvfs_clks_enabled: clk is already disabled\n");
		return;
	}

	if (mm_clks_enabled == 1 && clk_enable_request == 1) {
		MMDVFSMSG("mmdvfs_debug_set_mmdvfs_clks_enabled: clk is already enabled\n");
		return;
	}

	for (clk_idx = 0; clk_idx < g_mmdvfs_adaptor->mmdvfs_clk_hw_maps_num; clk_idx++) {
		/* Get the specific clk descriptor */
		struct mmdvfs_clk_hw_map *clk_hw_map_ptr =
			&(g_mmdvfs_adaptor->mmdvfs_clk_hw_maps[clk_idx]);

		if (clk_hw_map_ptr->config_method
			== MMDVFS_CLK_CONFIG_BY_MUX) {
			if (clk_hw_map_ptr->clk_mux.ccf_handle == NULL) {
				MMDVFSMSG("CCF handle can't be NULL during MMDVFS\n");
				continue;
			}

			/* Check if the clk needs to be always on according the configurtion */
			if (!((1 << clk_idx) & always_on_mask))
				continue;

			if (clk_enable_request == 1) {
				MMDVFSMSG("set_mmdvfs_clks_enabled-clk_prepare_enable: handle = %lx\n",
					((unsigned long)clk_hw_map_ptr->clk_mux.ccf_handle));
				ccf_ret =
					clk_prepare_enable((struct clk *)clk_hw_map_ptr->clk_mux.ccf_handle);
				if (ccf_ret) {
					MMDVFSMSG("Failed to enable clk: %s\n",
						clk_hw_map_ptr->clk_mux.ccf_name);
					continue;
				}
			} else
				MMDVFSMSG("set_mmdvfs_clks_enabled-clk_disable_unprepare: handle = %lx\n",
					((unsigned long)clk_hw_map_ptr->clk_mux.ccf_handle));
				clk_disable_unprepare((struct clk *)clk_hw_map_ptr->clk_mux.ccf_handle);
		}
	}
}


void dump_mmdvfs_info(void)
{
	MMDVFSMSG("MMDVFS dump: CMD(%d,%d,0x%x),INFO VR(%d,%d),CLK: %d\n",
	g_mmdvfs_cmd.sensor_size, g_mmdvfs_cmd.sensor_fps, g_mmdvfs_cmd.camera_mode,
	g_mmdvfs_info->video_record_size[0], g_mmdvfs_info->video_record_size[1],
	current_mmsys_clk);
}

void mmdvfs_unit_test_func(void)
{
	MMDVFSMSG("MMDVFS unit test\n");
	mmdvfs_start_cam_monitor(MMDVFS_SCEN_MHL, 8);

}
