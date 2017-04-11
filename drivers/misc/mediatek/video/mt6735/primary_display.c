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
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/module.h>
#include <linux/wait.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/ktime.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/switch.h>

#include "disp_drv_platform.h"
#include "ion_drv.h"
#include "mtk_ion.h"

#include "m4u.h"

#include "mt_idle.h"
#include "mt_spm_idle.h"
#include "mt_spm.h" /* for sodi reg addr define */
#define mt_eint_set_hw_debounce(eint_num, ms) (void)0

#include <mt-plat/mt_gpio.h>
#ifdef CONFIG_MTK_CLKMGR
#include <mach/mt_clkmgr.h>
#endif

#include "mtk_sync.h"
#include "mtkfb.h"
#include "mtkfb_fence.h"
#include "disp_session.h"
#include "disp_lcm.h"
#include "disp_utils.h"
#include "disp_recorder.h"
#include "fbconfig_kdebug.h"
#include "primary_display.h"
#include "disp_helper.h"
#include "disp_debug.h"
#include "ddp_hal.h"
#include "ddp_dump.h"
#include "ddp_path.h"
#include "ddp_drv.h"
#include "ddp_ovl.h"
#include "ddp_rdma.h"
#include "ddp_manager.h"
#include "ddp_mmp.h"
#include "ddp_reg.h"
#include "ddp_irq.h"

#ifndef MTK_FB_CMDQ_DISABLE
#include "cmdq_def.h"
#include "cmdq_record.h"
#include "cmdq_reg.h"
#include "cmdq_core.h"
#endif

#include "disp_assert_layer.h"
#include "ddp_dsi.h"
#include "mtk_disp_mgr.h"
#include "ddp_wdma.h"
#include "ddp_wdma_ex.h"
#include "mt_clk_id.h"
#include "mmdvfs_mgr.h"
#include "mt_smi.h"
#include <mach/mt_freqhopping.h>

//add this file for device info --sunsiyuan@wind-mobi.com add 20161129 begin
#ifdef CONFIG_WIND_DEVICE_INFO
		extern char *g_lcm_name;
#endif
//add this file for device info --sunsiyuan@wind-mobi.com add 20161129 end
typedef void (*fence_release_callback) (unsigned int data);
unsigned int is_hwc_enabled = 0;
static int is_hwc_update_frame;
unsigned int gEnableLowPowerFeature = 0;
/* int _trigger_display_interface(int blocking, void *callback, unsigned int userdata); */

int primary_display_use_cmdq = CMDQ_DISABLE;
int primary_display_use_m4u = 1;
#if defined(MTK_OVL_DECOUPLE_SUPPORT)
DISP_PRIMARY_PATH_MODE primary_display_mode = DECOUPLE_MODE;
#else
DISP_PRIMARY_PATH_MODE primary_display_mode = DIRECT_LINK_MODE;
#endif

//add by longcheer yufangfang for esd recover backlight
#ifdef CONFIG_LCT_ESD_CHECK_MULTI_REG
#ifdef CONFIG_LCT_LCM_GPIO_UTIL
#include <lcm_drv.h>
extern void lcm_led_en_settting(unsigned int value);
#endif
#endif
static unsigned long dim_layer_mva;
/* wdma dump thread */
static unsigned int primary_dump_wdma;
static struct task_struct *primary_display_wdma_out;
static unsigned long dc_vAddr[DISP_INTERNAL_BUFFER_COUNT];
static disp_internal_buffer_info *decouple_buffer_info[DISP_INTERNAL_BUFFER_COUNT];
static disp_internal_buffer_info *freeze_buffer_info;
static RDMA_CONFIG_STRUCT decouple_rdma_config;
static WDMA_CONFIG_STRUCT decouple_wdma_config;
static disp_mem_output_config mem_config;

static unsigned int primary_session_id = MAKE_DISP_SESSION(DISP_SESSION_PRIMARY, 0);
unsigned int ext_session_id = MAKE_DISP_SESSION(DISP_SESSION_MEMORY, 2);

/* primary display uses itself's abs macro */
#ifdef abs
#undef  abs
#define abs(a) (((a) < 0) ? -(a) : (a))
#endif

#define FRM_UPDATE_SEQ_CACHE_NUM (DISP_INTERNAL_BUFFER_COUNT+1)
static disp_frm_seq_info frm_update_sequence[FRM_UPDATE_SEQ_CACHE_NUM];
static unsigned int frm_update_cnt;
/* DDP_SCENARIO_ENUM ddp_scenario = DDP_SCENARIO_SUB_RDMA1_DISP; */
#ifdef DISP_SWITCH_DST_MODE
int primary_display_def_dst_mode = 0;
int primary_display_cur_dst_mode = 0;
#endif
#ifdef CONFIG_OF
/* extern unsigned int islcmconnected; */
#endif
#define ALIGN_TO(x, n)	\
	(((x) + ((n) - 1)) & ~((n) - 1))
int primary_trigger_cnt = 0;
#define PRIMARY_DISPLAY_TRIGGER_CNT (1)
unsigned int gEnterSodiAfterEOF = 0;

unsigned int WDMA0_FRAME_START_FLAG = 0;
unsigned int NEW_BUF_IDX = 0;
unsigned int ALL_LAYER_DISABLE_STEP = 0;
unsigned long long last_primary_trigger_time = 0xffffffffffffffff;

unsigned int isIdlePowerOff = 0;
unsigned int isDSIOff = 0;
unsigned int gPresentFenceIndex = 0;
unsigned int gTriggerDispMode = 0; /* 0: normal, 1: lcd only, 2: none of lcd and lcm */
disp_ddp_path_config last_primary_config;

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
static struct switch_dev disp_switch_data;
#endif

static int g_is_inited;

#ifdef CONFIG_LCT_ESD_CHECK_MULTI_REG
unsigned int esd_recovery_level = 0;	//add by zhudaolong at 20170411
#endif

void enqueue_buffer(display_primary_path_context *ctx, struct list_head *head,
		    disp_internal_buffer_info *buf)
{
	if (ctx && head && buf)
		list_add_tail(&buf->list, head);
		/* DISPMSG("enqueue_buffer, head=0x%08x, buf=0x%08x, mva=0x%08x\n", head, buf, buf->mva); */

}

void reset_buffer(display_primary_path_context *ctx, disp_internal_buffer_info *buf)
{
	if (ctx && buf)
		list_del_init(&buf->list);
		/* DISPMSG("reset_buffer, buf=0x%08x, mva=0x%08x\n", buf, buf->mva); */

}

disp_internal_buffer_info *dequeue_buffer(display_primary_path_context *ctx,
					  struct list_head *head)
{
	disp_internal_buffer_info *temp = NULL;

	if (ctx && head) {
		if (!list_empty(head)) {
			temp = list_entry(head->prev, disp_internal_buffer_info, list);
			/* DISPMSG("dequeue_buffer, head=0x%08x, buf=0x%08x, mva=0x%08x\n", head, temp, temp->mva); */
			list_del_init(&temp->list);

			if (list_empty(head))
				; /* DISPMSG("after dequeue_buffer, head:0x%08x is empty\n", head); */
		} else {
			DISPMSG("list is empty, alloc new buffer\n");
			return NULL;
		}
	}

	return temp;
}

disp_internal_buffer_info *find_buffer_by_mva(display_primary_path_context *ctx,
					      struct list_head *head, uint32_t mva)
{
	disp_internal_buffer_info *buf = NULL;
	disp_internal_buffer_info *temp = NULL;

	if (ctx && head && mva) {
		if (!list_empty(head)) {
			list_for_each_entry(temp, head, list) {
				/* DISPMSG("find buffer: temp=0x%08x, mva=0x%08x\n", temp, temp->mva); */
				if (mva == temp->mva) {
					buf = temp;
					break;
				}
			}
		}
		/* DISPMSG("find buffer by mva, head=0x%08x, buf=0x%08x\n", head, buf); */
	}

	return buf;
}

#define pgc	_get_context()

static display_primary_path_context *_get_context(void)
{
	static int is_context_inited;
	static display_primary_path_context g_context;

	if (!is_context_inited) {
		memset((void *)&g_context, 0, sizeof(display_primary_path_context));
		is_context_inited = 1;
	}

	return &g_context;
}

static inline int _is_mirror_mode(DISP_MODE mode)
{
	if (mode == DISP_SESSION_DIRECT_LINK_MIRROR_MODE || mode == DISP_SESSION_DECOUPLE_MIRROR_MODE)
		return 1;
	else
		return 0;
}

static inline int _is_decouple_mode(DISP_MODE mode)
{
	if (mode == DISP_SESSION_DECOUPLE_MODE || mode == DISP_SESSION_DECOUPLE_MIRROR_MODE)
		return 1;
	else
		return 0;
}

struct mutex esd_mode_switch_lock;
static void _primary_path_esd_check_lock(void)
{
	mutex_lock(&esd_mode_switch_lock);
}

static void _primary_path_esd_check_unlock(void)
{
	mutex_unlock(&esd_mode_switch_lock);
}

display_primary_path_context *primary_display_path_lock(const char *caller)
{
	dprec_logger_start(DPREC_LOGGER_PRIMARY_MUTEX, 0, 0);
	disp_sw_mutex_lock(&(pgc->lock));
	pgc->mutex_locker = (char *)caller;
	return pgc;
}

void primary_display_path_unlock(const char *caller)
{
	pgc->mutex_locker = NULL;
	disp_sw_mutex_unlock(&(pgc->lock));
	dprec_logger_done(DPREC_LOGGER_PRIMARY_MUTEX, 0, 0);
}

static void _primary_path_cmd_lock(void)
{
	mutex_lock(&(pgc->cmd_lock));
}

static void _primary_path_cmd_unlock(void)
{
	mutex_unlock(&(pgc->cmd_lock));
}

static void _primary_path_lock(const char *caller)
{
	dprec_logger_start(DPREC_LOGGER_PRIMARY_MUTEX, 0, 0);
	disp_sw_mutex_lock(&(pgc->lock));
	pgc->mutex_locker = (char *)caller;
}

static void _primary_path_unlock(const char *caller)
{
	pgc->mutex_locker = NULL;
	disp_sw_mutex_unlock(&(pgc->lock));
	dprec_logger_done(DPREC_LOGGER_PRIMARY_MUTEX, 0, 0);
}

static DECLARE_WAIT_QUEUE_HEAD(display_state_wait_queue);

static DISP_POWER_STATE primary_get_state(void)
{
	return pgc->state;
}
static DISP_POWER_STATE primary_set_state(DISP_POWER_STATE new_state)
{
	DISP_POWER_STATE old_state = pgc->state;

	pgc->state = new_state;
	DISPMSG("%s %d to %d\n", __func__, old_state, new_state);
	wake_up(&display_state_wait_queue);
	return old_state;
}

/* use MAX_SCHEDULE_TIMEOUT to wait for ever
 * NOTES: primary_path_lock should NOT be held when call this func !!!!!!!!
 */
#define __primary_display_wait_state(condition, timeout)                       \
({                                                                     \
	wait_event_timeout(display_state_wait_queue, condition, timeout);\
})

long primary_display_wait_state(DISP_POWER_STATE state, long timeout)
{
	long ret;

	ret = __primary_display_wait_state(primary_get_state() == state, timeout);
	return ret;
}

long primary_display_wait_not_state(DISP_POWER_STATE state, long timeout)
{
	long ret;

	ret = __primary_display_wait_state(primary_get_state() != state, timeout);
	return ret;
}

static void _primary_path_vsync_lock(void)
{
	mutex_lock(&(pgc->vsync_lock));
}

static void _primary_path_vsync_unlock(void)
{
	mutex_unlock(&(pgc->vsync_lock));
}

static void _cmdq_flush_config_handle_mira(void *handle, int blocking);

#ifdef MTK_DISP_IDLE_LP
static atomic_t isDdp_Idle = ATOMIC_INIT(0);
static atomic_t idle_detect_flag = ATOMIC_INIT(0);
static struct mutex idle_lock;
static struct task_struct *primary_display_idle_detect_task;
#define DISP_DSI_REG_VFP 0x28
static DECLARE_WAIT_QUEUE_HEAD(idle_detect_wq);

static int _disp_primary_path_idle_clock_on(unsigned int level)
{
	dpmgr_path_idle_on(pgc->dpmgr_handle, NULL, level);
	return 0;
}

static int _disp_primary_path_idle_clock_off(unsigned int level)
{
	dpmgr_path_idle_off(pgc->dpmgr_handle, NULL, level);
	return 0;
}
#ifdef CONFIG_SINGLE_PANEL_OUTPUT
static void primary_suspend_release_present_fence(void)
{
	int fence_increment = 0;
	disp_sync_info *layer_info = NULL;

	/* if session not created, do not release present fence*/
	if (pgc->session_id == 0) {
		DISPDBG("_get_sync_info fail in present_fence_release thread\n");
		return;
	}
	layer_info = _get_sync_info(pgc->session_id, disp_sync_get_present_timeline_id());

	if (layer_info == NULL) {
		DISPERR("_get_sync_info fail in present_fence_release thread\n");
		return;
	}
	_primary_path_lock(__func__);
	fence_increment = gPresentFenceIndex-layer_info->timeline->value;
	if (fence_increment > 0) {
		timeline_inc(layer_info->timeline, fence_increment);
		MMProfileLogEx(ddp_mmp_get_events()->present_fence_release,
			MMProfileFlagPulse, gPresentFenceIndex, fence_increment);
	}
	_primary_path_unlock(__func__);
	DISPPR_FENCE("RPF/%d/%d\n", gPresentFenceIndex, gPresentFenceIndex-layer_info->timeline->value);
}
#endif

static DEFINE_SPINLOCK(gLockTopClockOff);
static int _disp_primary_path_dsi_clock_on(unsigned int level)
{
	if (!primary_display_is_video_mode()) {
		unsigned long flags;

#ifndef CONFIG_MTK_CLKMGR
		ddp_clk_prepare(DISP_MTCMOS_CLK);
#endif
		spin_lock_irqsave(&gLockTopClockOff, flags);
		isDSIOff = 0;
		dpmgr_path_dsi_on(pgc->dpmgr_handle, NULL, level);
		spin_unlock_irqrestore(&gLockTopClockOff, flags);
	}

	return 0;
}

static int _disp_primary_path_dsi_clock_off(unsigned int level)
{
	if (!primary_display_is_video_mode()) {
		unsigned long flags;

		spin_lock_irqsave(&gLockTopClockOff, flags);
		dpmgr_path_dsi_off(pgc->dpmgr_handle, NULL, level);
		isDSIOff = 1;
		spin_unlock_irqrestore(&gLockTopClockOff, flags);
#ifndef CONFIG_MTK_CLKMGR
		ddp_clk_unprepare(DISP_MTCMOS_CLK);
#endif
	}

	return 0;
}

int _disp_primary_path_set_vfp(int enter)
{
	int ret = 0;

	if (primary_display_is_video_mode()) {
		LCM_PARAMS *lcm_param = disp_lcm_get_params(pgc->plcm);
		cmdqRecHandle cmdq_handle_vfp = NULL;

		ret = cmdqRecCreate(CMDQ_SCENARIO_PRIMARY_DISP, &cmdq_handle_vfp);
		if (ret != 0) {
			DISPMSG("fail to create primary cmdq handle for set vfp\n");
			return -1;
		}
		DISPMSG("primary set vfp, handle=%p\n", cmdq_handle_vfp);
		cmdqRecReset(cmdq_handle_vfp);
		_cmdq_insert_wait_frame_done_token_mira(cmdq_handle_vfp);
		if (enter)
			dpmgr_path_ioctl(pgc->dpmgr_handle, cmdq_handle_vfp, DDP_DSI_VFP_LP,
					 (unsigned long *)&(lcm_param->dsi.vertical_vfp_lp));
		else
			dpmgr_path_ioctl(pgc->dpmgr_handle, cmdq_handle_vfp, DDP_DSI_VFP_LP,
					 (unsigned long *)&(lcm_param->dsi.vertical_frontporch));

		MMProfileLogEx(ddp_mmp_get_events()->dal_clean, MMProfileFlagPulse, 0, enter);

		_cmdq_flush_config_handle_mira(cmdq_handle_vfp, 1);
		DISPMSG("[VFP]cmdq_handle_vfp ret=%d\n", ret);
		cmdqRecDestroy(cmdq_handle_vfp);
		cmdq_handle_vfp = NULL;
	} else {
		DISPMSG("CMD mode don't set vfp for lows\n");
	}

	return ret;
}

int primary_display_save_power_for_idle(int enter, unsigned int need_primary_lock)
{
	static unsigned int isLowPowerMode;

	if (is_hwc_enabled == 0)
		return 0;

	if (need_primary_lock)	/* if outer api has add primary lock, do not have to lock again */
		_primary_path_lock(__func__);

	if (primary_get_state() == DISP_SLEPT) {
		DISPMSG("suspend mode can not enable low power.\n");
		goto end;
	}
	if (enter == 1 && isLowPowerMode == 1) {
		DISPMSG("already in low power mode.\n");
		goto end;
	}
	if (enter == 0 && isLowPowerMode == 0) {
		DISPMSG("already not in low power mode.\n");
		goto end;
	}
	isLowPowerMode = enter;

	if (pgc->plcm == NULL) {
		DISPERR("lcm handle is null\n");
		goto end;
	}

	DISPMSG("low power in, enter=%d.\n", enter);

	if (disp_low_power_enlarge_blanking == 1) {
#if 0
		if (pgc->plcm->params) {
			vfp_for_low_power = pgc->plcm->params->dsi.vertical_frontporch_for_low_power;
			vfp_original = pgc->plcm->params->dsi.vertical_frontporch;
		}

		if (enter)
			DISP_REG_SET(pgc->cmdq_handle_config, dispsys_reg[DISP_REG_DSI0] + DISP_DSI_REG_VFP,
				     vfp_for_low_power);
		else
			DISP_REG_SET(pgc->cmdq_handle_config, dispsys_reg[DISP_REG_DSI0] + DISP_DSI_REG_VFP,
				     vfp_original);
#else
		if (primary_display_is_video_mode() == 1)
			_disp_primary_path_set_vfp(enter);

#endif
	}

	if (1 == disp_low_power_disable_ddp_clock) {/* open */
		if (primary_display_is_video_mode() == 0 /* && gEnableLowPowerFeature==1 */) {
			static unsigned int disp_low_power_disable_ddp_clock_cnt;

			DISPDBG("MM clock, disp_low_power_disable_ddp_clock enter %d.\n",
				disp_low_power_disable_ddp_clock_cnt++);
			if (1 == enter)	{ /* only for command mode */
				if (isIdlePowerOff == 0) {
					/* extern void clk_stat_bug(void); */
					unsigned long flags;

					DISPMSG("off MM clock start.\n");
					spin_lock_irqsave(&gLockTopClockOff, flags);
					_disp_primary_path_idle_clock_off(0); /* parameter represent level */
					isIdlePowerOff = 1;
					spin_unlock_irqrestore(&gLockTopClockOff, flags);
#ifndef CONFIG_MTK_CLKMGR
					ddp_clk_unprepare(DISP_MTCMOS_CLK);
#endif
					/* DISPMSG("off MM clock end.\n"); */
					DISPMSG("***start dump regs! clk_stat_check\n");
#ifdef CONFIG_MTK_CLKMGR
					clk_stat_check(SYS_DIS);
#endif
					/*DISPMSG("---- start dump regs! clk_stat_bug.\n"); */
					/* clk_stat_bug(); */
				}
			} else {
				if (isIdlePowerOff == 1) {
					unsigned long flags;

					DISPMSG("on MM clock start.\n");
#ifndef CONFIG_MTK_CLKMGR
					ddp_clk_prepare(DISP_MTCMOS_CLK);
#endif
					spin_lock_irqsave(&gLockTopClockOff, flags);
					isIdlePowerOff = 0;
					_disp_primary_path_idle_clock_on(0); /* parameter represent level */
					spin_unlock_irqrestore(&gLockTopClockOff, flags);
					DISPMSG("on MM clock end.\n");
				}

			}
		}
	}

	if (disp_low_power_disable_fence_thread == 1)
		; /* already implemented. */

	/* no need idle lock, cause primary lock will be used inside switch_mode */
	if (disp_low_power_remove_ovl == 1) {
		if (primary_display_is_video_mode() == 1) {/* only for video mode */
			/* DISPMSG("[LP] OVL pgc->session_mode=%d, enter=%d\n", pgc->session_mode, enter); */
			if (enter) {
				if (pgc->session_mode == DISP_SESSION_DIRECT_LINK_MODE) {
					DISPDBG("[LP]remove ovl.\n");
					primary_display_switch_mode_nolock(DISP_SESSION_DECOUPLE_MODE,
									   pgc->session_id, 1);
				}
				/* DISPMSG("disp_low_power_remove_ovl 2\n"); */
			} else {
				if (pgc->session_mode == DISP_SESSION_DECOUPLE_MODE) {
					DISPDBG("[LP]add ovl.\n");
					primary_display_switch_mode_nolock(DISP_SESSION_DIRECT_LINK_MODE,
									   pgc->session_id, 1);
				}
				/* DISPMSG("disp_low_power_remove_ovl 4\n"); */
			}
		}
	}

	if (is_mmdvfs_supported() && mmdvfs_get_mmdvfs_profile() == MMDVFS_PROFILE_D1_PLUS) {
		DISPMSG("MMDVFS enter:%d\n", enter);
		if (enter)
			mmdvfs_set_step(MMDVFS_SCEN_DISP, MMDVFS_VOLTAGE_LOW); /* Vote to LPM mode */
		else if (primary_display_get_width() > 800)
			mmdvfs_set_step(MMDVFS_SCEN_DISP, MMDVFS_VOLTAGE_HIGH); /* Enter HPM mode */
	}

end:
	if (enter)
		atomic_set(&isDdp_Idle, 1);

	if (need_primary_lock)
		_primary_path_unlock(__func__);

	return 0;
}

#if defined(CONFIG_MTK_GMO_RAM_OPTIMIZE)
static int release_idle_lp_dc_buffer(unsigned int need_primary_lock);
static int allocate_idle_lp_dc_buffer(void);
#endif

void _disp_primary_path_exit_idle(const char *caller, unsigned int need_primary_lock)
{
	/* _disp_primary_idle_lock(); */
	if (atomic_read(&isDdp_Idle) == 1) {
		DISPMSG("[ddp_idle_on]_disp_primary_path_exit_idle (%s) &&&\n", caller);
		primary_display_save_power_for_idle(0, need_primary_lock);
#if defined(CONFIG_MTK_GMO_RAM_OPTIMIZE)
		if (primary_display_is_video_mode())
			release_idle_lp_dc_buffer(need_primary_lock);
#endif
		atomic_set(&isDdp_Idle, 0);
		atomic_set(&idle_detect_flag, 1);
		wake_up(&idle_detect_wq);
	}
	/* _disp_primary_idle_unlock(); */
}

/* used by aal/pwm to exit idle before config register(for "screen idle top clock off" feature) */

void disp_exit_idle_ex(const char *caller)
{
	if (pgc->plcm) {
		if (primary_display_is_video_mode() == 0) {
			disp_update_trigger_time();
			_disp_primary_path_exit_idle(caller, 0);
		}
	}
}

static int primary_display_is_secure_path(DISP_SESSION_TYPE session_type)
{
	int i;
	disp_session_input_config *session_input;

	session_input = &cached_session_input[session_type - 1];
	for (i = 0; i < session_input->config_layer_num; i++) {
		if (session_input->config[i].layer_enable &&
		    (session_input->config[i].security == DISP_SECURE_BUFFER))
			return 1;
	}
	return 0;
}

static int _disp_primary_path_idle_detect_thread(void *data)
{
	int ret = 0, idle_time;
	static int enter_cnt;
	static int end_cnt;
	static int stop_cnt;

#if defined(CONFIG_MTK_GMO_RAM_OPTIMIZE)
	idle_time = 2000;
#else
	idle_time = 50;
#endif

	while (1) {
		msleep(idle_time); /* 0.5s trigger once */
		/* DISPMSG("[ddp_idle]_disp_primary_path_idle_detect start 1\n"); */

		if (gSkipIdleDetect || atomic_read(&isDdp_Idle) == 1 ||
			is_hwc_update_frame == 0) /* skip is already in idle mode */
			continue;

		_primary_path_lock(__func__);
		if (primary_get_state() != DISP_ALIVE || primary_display_is_secure_path(DISP_SESSION_PRIMARY)) {
			MMProfileLogEx(ddp_mmp_get_events()->esd_check_t, MMProfileFlagPulse, 1, 0);
			/* DISPMSG("[ddp_idle]primary display path is slept?? -- skip ddp_idle\n"); */
#if !defined(CONFIG_MTK_GMO_RAM_OPTIMIZE)
			idle_time = 500;
#endif
			_primary_path_unlock(__func__);
			continue;
		}
		if (pgc->session_mode != DISP_SESSION_DIRECT_LINK_MODE) {
			DISPMSG("[ddp_idle]primary display path is decouple mode. -- skip ddp_idle\n");
			_primary_path_unlock(__func__);
			continue;
		}

		_primary_path_unlock(__func__);

#if !defined(CONFIG_MTK_GMO_RAM_OPTIMIZE)
		idle_time = 50;
#endif

		/* _disp_primary_idle_lock(); */
		_primary_path_esd_check_lock();
		_primary_path_lock(__func__);
		if (((sched_clock() - last_primary_trigger_time) / 1000) > idle_time * 1000) {
#if defined(CONFIG_MTK_GMO_RAM_OPTIMIZE)
			/* Dynamically allocate decouple buffer. */
			if (primary_display_is_video_mode()) {
				ret = allocate_idle_lp_dc_buffer();
				if (ret < 0) {
					DISPMSG("[ddp_idle]allocate dc buffer fail\n");
					_primary_path_unlock(__func__);
					_primary_path_esd_check_unlock();
					continue;
				}
			}
#endif
			enter_cnt++;
			pr_debug("[LP] - enter: %d, flag:%d,%d\n", enter_cnt,
				atomic_read(&isDdp_Idle), atomic_read(&idle_detect_flag));
			primary_display_save_power_for_idle(1, 0);
			_primary_path_unlock(__func__);
			_primary_path_esd_check_unlock();
		} else {
			/* _disp_primary_idle_unlock(); */
			_primary_path_unlock(__func__);
			_primary_path_esd_check_unlock();
			continue;
		}
		/* _disp_primary_idle_unlock(); */

		ret = wait_event_interruptible(idle_detect_wq, (atomic_read(&idle_detect_flag) != 0));
		atomic_set(&idle_detect_flag, 0);
		/* printk("[ddp_idle]ret=%d\n", ret); */
		if (kthread_should_stop()) {
			stop_cnt++;
			pr_debug("[LP] stop: %d, flag:%d,%d\n", stop_cnt,
				 atomic_read(&isDdp_Idle), atomic_read(&idle_detect_flag));
			break;
		}

		end_cnt++;
		pr_debug("[LP] end: %d, flag:%d,%d\n", end_cnt, atomic_read(&isDdp_Idle),
			 atomic_read(&idle_detect_flag));
		if (enter_cnt != end_cnt) {
			pr_err("[LP][ASSERT]%d, %d, %d\n", enter_cnt, end_cnt, stop_cnt);
			ASSERT(0);
		}
	}

	return ret;
}

int primary_display_cmdq_set_reg(unsigned int addr, unsigned int val)
{
	int ret = 0;
	cmdqRecHandle handle = NULL;

	ret = cmdqRecCreate(CMDQ_SCENARIO_PRIMARY_DISP, &handle);
	cmdqRecReset(handle);
	_cmdq_insert_wait_frame_done_token_mira(handle);

	cmdqRecWrite(handle, addr & 0x1fffffff, val, ~0);
	cmdqRecFlushAsync(handle);

	cmdqRecDestroy(handle);

	return 0;
}
#endif

unsigned int dvfs_test = 0xFF;
static atomic_t gMmsysClk = ATOMIC_INIT(0xFF);

static void set_mmsys_clk(unsigned int clk)
{
	if (atomic_read(&gMmsysClk) != clk)
		atomic_set(&gMmsysClk, clk);
}

static unsigned int get_mmsys_clk(void)
{
	return atomic_read(&gMmsysClk);
}

static int _switch_mmsys_clk(int mmsys_clk_old, int mmsys_clk_new)
{
	int ret = -1;
	cmdqRecHandle handle;

	DISPMSG("[LP]: %s\n", __func__);
	if (mmsys_clk_new == get_mmsys_clk())
		return ret;

	if (pgc->state == DISP_SLEPT) {
		DISP_REG_MASK(NULL, DISP_REG_CONFIG_C13, 0x07000000, 0x07000000);	/* clear */
		DISP_REG_MASK(NULL, DISP_REG_CONFIG_C12, 0x06000000, 0x07000000);	/* set syspll2_d2 */

		DISPMSG("[DISP] DVFS mode=%d, profile=%d, state=0x%x\n", mmsys_clk_new,
			mmdvfs_get_mmdvfs_profile(), pgc->state);
		switch (mmsys_clk_new) {
		case MMSYS_CLK_LOW:
			DISP_REG_MASK(NULL, DISP_REG_VENCPLL_CON1, 0x830E0000, 0xFFFFFFFF);	/* update */
			break;

		case MMSYS_CLK_MEDIUM:
			/* by frequency hopping */
			goto cpu_d;

		case MMSYS_CLK_HIGH:
			if (mmdvfs_get_mmdvfs_profile() == MMDVFS_PROFILE_D2_P_PLUS)
				DISP_REG_MASK(NULL, DISP_REG_VENCPLL_CON1, 0x82110000, 0xFFFFFFFF);	/* update */
			else if (mmdvfs_get_mmdvfs_profile() == MMDVFS_PROFILE_D2_M_PLUS)
				DISP_REG_MASK(NULL, DISP_REG_VENCPLL_CON1, 0x820F0000, 0xFFFFFFFF);	/* update */
			break;

		default:
			DISPERR("[DISP] DVFS mode=%d\n", mmsys_clk_new);
			goto cpu_d;
		}

		udelay(40);

		DISP_REG_MASK(NULL, DISP_REG_CONFIG_C13, 0x07000000, 0x07000000);	/* clear */
		DISP_REG_MASK(NULL, DISP_REG_CONFIG_C12, 0x01000000, 0x07000000);	/* set vencpll_ck */

cpu_d:
		/* set rdma golden setting parameters */
		set_mmsys_clk(mmsys_clk_new);

		return get_mmsys_clk();
	}

	/* 1.create and reset cmdq */
	cmdqRecCreate(CMDQ_SCENARIO_PRIMARY_DISP, &handle);

	cmdqRecReset(handle);

	/* wait eof */
	_cmdq_insert_wait_frame_done_token_mira(handle);

	cmdqRecWrite(handle, 0x10210048, 0x07000000, 0x07000000);	/* clear */
	cmdqRecWrite(handle, 0x10210044, 0x06000000, 0x07000000);	/* set syspll2_d2 */

	DISPMSG("[DISP] DVFS mode=%d, profile=%d, state=0x%x\n", mmsys_clk_new,
		mmdvfs_get_mmdvfs_profile(), pgc->state);
	switch (mmsys_clk_new) {
	case MMSYS_CLK_LOW:
		cmdqRecWrite(handle, 0x10209254, 0x830E0000, 0xFFFFFFFF);	/* update */
		break;

	case MMSYS_CLK_MEDIUM:
		/* by frequency hopping */
		goto cmdq_d;

	case MMSYS_CLK_HIGH:
		if (mmdvfs_get_mmdvfs_profile() == MMDVFS_PROFILE_D2_P_PLUS)
			cmdqRecWrite(handle, 0x10209254, 0x82110000, 0xFFFFFFFF);	/* update */
		else if (mmdvfs_get_mmdvfs_profile() == MMDVFS_PROFILE_D2_M_PLUS)
			cmdqRecWrite(handle, 0x10209254, 0x820F0000, 0xFFFFFFFF);	/* update */
		break;

	default:
		DISPERR("[DISP] DVFS mode=%d\n", mmsys_clk_new);
		goto cmdq_d;
	}

	/* wait rdma0_sof: only used for video mode & trigger loop need wait and clear rdma0 sof */
	cmdqRecWaitNoClear(handle, CMDQ_EVENT_DISP_RDMA0_SOF);

	cmdqRecWrite(handle, 0x10210048, 0x07000000, 0x07000000);	/* clear */
	cmdqRecWrite(handle, 0x10210044, 0x01000000, 0x07000000);	/* set vencpll_ck */

	cmdqRecFlush(handle);

cmdq_d:
	cmdqRecDestroy(handle);

	/* set rdma golden setting parameters */
	set_mmsys_clk(mmsys_clk_new);

	return get_mmsys_clk();

}

int primary_display_switch_mmsys_clk(int mmsys_clk_old, int mmsys_clk_new)
{
	int ret = -1;

	/* need lock */
	DISPMSG("[LP] %s\n", __func__);

	primary_display_manual_lock();
	ret = _switch_mmsys_clk(mmsys_clk_old, mmsys_clk_new);
	primary_display_manual_unlock();

	return ret;
}

int primary_display_set_secondary_display(int add, DISP_SESSION_TYPE type)
{
	if (add) {
#ifdef MTK_DISP_IDLE_LP
		gSkipIdleDetect = 1;
		_disp_primary_path_exit_idle(__func__, 0);
#endif
	} else {

#ifdef MTK_DISP_IDLE_LP
		gSkipIdleDetect = 0;
#endif
	}

	return 0;
}

static void update_frm_seq_info(unsigned int addr, unsigned int addr_offset, unsigned int seq,
				DISP_FRM_SEQ_STATE state)
{
	int i = 0;

	if (FRM_CONFIG == state) {
		frm_update_sequence[frm_update_cnt].state = state;
		frm_update_sequence[frm_update_cnt].mva = addr;
		frm_update_sequence[frm_update_cnt].max_offset = addr_offset;
		if (seq > 0)
			frm_update_sequence[frm_update_cnt].seq = seq;
		MMProfileLogEx(ddp_mmp_get_events()->primary_seq_config, MMProfileFlagPulse, addr, seq);

	} else if (FRM_TRIGGER == state) {
		frm_update_sequence[frm_update_cnt].state = FRM_TRIGGER;
		MMProfileLogEx(ddp_mmp_get_events()->primary_seq_trigger, MMProfileFlagPulse,
			       frm_update_cnt, frm_update_sequence[frm_update_cnt].seq);

		dprec_logger_frame_seq_begin(pgc->session_id, frm_update_sequence[frm_update_cnt].seq);

		frm_update_cnt++;
		frm_update_cnt %= FRM_UPDATE_SEQ_CACHE_NUM;

	} else if (FRM_START == state) {
		for (i = 0; i < FRM_UPDATE_SEQ_CACHE_NUM; i++) {
			if ((abs(addr - frm_update_sequence[i].mva) <= frm_update_sequence[i].max_offset) &&
			    (frm_update_sequence[i].state == FRM_TRIGGER)) {
				MMProfileLogEx(ddp_mmp_get_events()->primary_seq_rdma_irq, MMProfileFlagPulse,
					       frm_update_sequence[i].mva | seq, frm_update_sequence[i].state);
				frm_update_sequence[i].state = FRM_START;
				/* break; */
			}
		}
	} else if (FRM_END == state) {
		for (i = 0; i < FRM_UPDATE_SEQ_CACHE_NUM; i++) {
			if (FRM_START == frm_update_sequence[i].state) {
				frm_update_sequence[i].state = FRM_END;
				dprec_logger_frame_seq_end(pgc->session_id, frm_update_sequence[i].seq);
				MMProfileLogEx(ddp_mmp_get_events()->primary_seq_release, MMProfileFlagPulse,
					       frm_update_sequence[i].mva, frm_update_sequence[i].seq);
			}
		}
	}
}

static int _config_wdma_output(WDMA_CONFIG_STRUCT *wdma_config, disp_path_handle disp_handle,
			       cmdqRecHandle cmdq_handle)
{
	disp_ddp_path_config *pconfig = dpmgr_path_get_last_config(disp_handle);

	pconfig->wdma_config = *wdma_config;
	pconfig->wdma_dirty = 1;
	dpmgr_path_config(disp_handle, pconfig, cmdq_handle);
	return 0;
}

static int _config_rdma_input_data(RDMA_CONFIG_STRUCT *rdma_config, disp_path_handle disp_handle,
				   cmdqRecHandle cmdq_handle)
{
	disp_ddp_path_config *pconfig = dpmgr_path_get_last_config(disp_handle);

	pconfig->rdma_config = *rdma_config;
	pconfig->rdma_dirty = 1;
	dpmgr_path_config(disp_handle, pconfig, cmdq_handle);
	return 0;
}

#if 0
/**
 * Defined but not used, avoid build warning. (2015.1.30 Rynn Wu)
 */
static unsigned int get_frm_seq_by_addr(unsigned int addr, DISP_FRM_SEQ_STATE state)
{
	int i = 0;

	for (i = 0; i < FRM_UPDATE_SEQ_CACHE_NUM; i++) {
		if (addr == frm_update_sequence[i].mva)
			return frm_update_sequence[i].seq;
	}
	return 0;
}
#endif

#if 0
/**
 * Defined but not used, avoid build warning. (2015.1.30 Rynn Wu)
 */
static unsigned int release_started_frm_seq(unsigned int addr, DISP_FRM_SEQ_STATE FRM_END)
{
	int i = 0;

	for (i = 0; i < FRM_UPDATE_SEQ_CACHE_NUM; i++) {
		if (FRM_END == frm_update_sequence[i].state)
			return frm_update_sequence[i].seq;
	}
	return 0;
}
#endif


#ifdef DISP_SWITCH_DST_MODE
unsigned long long last_primary_trigger_time;
bool is_switched_dst_mode = false;
static struct task_struct *primary_display_switch_dst_mode_task;

static void _primary_path_switch_dst_lock(void)
{
	mutex_lock(&(pgc->switch_dst_lock));
}

static void _primary_path_switch_dst_unlock(void)
{
	mutex_unlock(&(pgc->switch_dst_lock));
}

static int _disp_primary_path_switch_dst_mode_thread(void *data)
{
	int ret = 0;

	while (1) {
		msleep(1000);

		if (((sched_clock() - last_primary_trigger_time) / 1000) > 500000) { /* 500ms not trigger disp */
			primary_display_switch_dst_mode(0); /* switch to cmd mode */
			is_switched_dst_mode = true;
		}
		if (kthread_should_stop())
			break;
	}
	return 0;
}
#endif

unsigned long long idlemgr_last_kick_time = 0xffffffffffffffff;
bool is_switched_dst_mode = false;

/*----------------------------------------------------------------
    defined but not used because some functions are not used either.
  ----------------------------------------------------------------
static wait_queue_head_t  idlemgr_wq; // For display idle manager
static atomic_t idlemgr_wakeup = ATOMIC_INIT(0); // For display idle manager
----------------------------------------------------------------*/
struct task_struct *primary_display_idlemgr_task = NULL;

#if 0
/**
 * Defined but not used, avoid build warning. (2015.1.30 Rynn Wu)
 */
static void _idlemgr_wait_for_wakeup(void)
{
	int ret = 0;

	atomic_set(&idlemgr_wakeup, 0);
	ret = wait_event_interruptible(idlemgr_wq, atomic_read(&idlemgr_wakeup));
	if (ret < 0)
		DISPERR("display idle manager thread waked up by signal\n");
}
#endif

#if 0
/**
 * Defined but not used, avoid build warning. (2015.1.30 Rynn Wu)
 */
static void _idlemgr_wakeup(void)
{
	atomic_set(&idlemgr_wakeup, 1);
	wake_up_interruptible(&idlemgr_wq);
}
#endif

#if 0
/**
 * Defined but not used, avoid build warning. (2015.1.30 Rynn Wu)
 */
static void _idlemgr_update_last_kick_time(void)
{
	idlemgr_last_kick_time = sched_clock();
}
#endif

#if 0
void primary_display_idlemgr_enter_idle(void)
{
	MMProfileLogEx(ddp_mmp_get_events()->idlemgr_enter_idle, MMProfileFlagPulse, 0, 0);

	if (!primary_display_is_video_mode()) {
		spm_enable_sodi(1);
		spm_sodi_mempll_pwr_mode(0);
		MMProfileLogEx(ddp_mmp_get_events()->sodi_enable, MMProfileFlagPulse, 0, 0);
	}
}

void primary_display_idlemgr_leave_idle(void)
{
	MMProfileLogEx(ddp_mmp_get_events()->idlemgr_leave_idle, MMProfileFlagPulse, 0, 0);

	if (!primary_display_is_video_mode()) {
		spm_enable_sodi(0);
		MMProfileLogEx(ddp_mmp_get_events()->sodi_disable, MMProfileFlagPulse, 0, 0);
	}
}

static int _primary_path_idlemgr_monitor_thread(void *data)
{
	int ret = 0;

	init_waitqueue_head(&idlemgr_wq);

	while (1) {
		msleep(100);

		if (((sched_clock() - idlemgr_last_kick_time) / 1000) > 500000) {/* 500ms not trigger disp */
			primary_display_idlemgr_enter_idle();	/* switch to cmd mode */
			_idlemgr_wait_for_wakeup();
		}

		if (kthread_should_stop())
			break;
	}

	return 0;
}
#else
void primary_display_idlemgr_kick(char *source)
{
}

void primary_display_idlemgr_enter_idle(void)
{

}

void primary_display_idlemgr_leave_idle(void)
{

}

#if 0
/**
 * Defined but not used, avoid build warning. (2015.1.30 Rynn Wu)
 */
static int _primary_path_idlemgr_monitor_thread(void *data)
{
	return 0;
}
#endif
#endif

/* extern int disp_od_is_enabled(void); */
int primary_display_get_debug_state(char *stringbuf, int buf_len)
{
	int len = 0;
	LCM_PARAMS *lcm_param = disp_lcm_get_params(pgc->plcm);
	LCM_DRIVER *lcm_drv = pgc->plcm->drv;

	len += scnprintf(stringbuf + len, buf_len - len,
			"|--------------------------------------------------------------------------------------|\n");
	len += scnprintf(stringbuf + len, buf_len - len,
			"|********Primary Display Path General Information********\n");
	len += scnprintf(stringbuf + len, buf_len - len, "|Primary Display is %s\n",
			dpmgr_path_is_idle(pgc->dpmgr_handle) ? "idle" : "busy");

	if (mutex_trylock(&(pgc->lock))) {
		mutex_unlock(&(pgc->lock));
		len += scnprintf(stringbuf + len, buf_len - len,
				"|primary path global mutex is free\n");
	} else {
		len += scnprintf(stringbuf + len, buf_len - len,
				"|primary path global mutex is hold by [%s]\n", pgc->mutex_locker);
	}

	if (lcm_param && lcm_drv)
		len += scnprintf(stringbuf + len, buf_len - len,
				"|LCM Driver=[%s]\tResolution=%dx%d,Interface:%s\n", lcm_drv->name,
				lcm_param->width, lcm_param->height,
				(lcm_param->type == LCM_TYPE_DSI) ? "DSI" : "Other");

	/* no OD */
	/* len += scnprintf(stringbuf+len, buf_len - len, "|OD is %s\n", disp_od_is_enabled()?"enabled":"disabled"); */
	len += scnprintf(stringbuf + len, buf_len - len, "|session_mode is 0x%08x\n", pgc->session_mode);

	len += scnprintf(stringbuf + len, buf_len - len,
			"|State=%s\tlcm_fps=%d\tmax_layer=%d\tmode:%d\tvsync_drop=%d\n",
			pgc->state == DISP_ALIVE ? "Alive" : "Sleep", pgc->lcm_fps, pgc->max_layer,
			pgc->mode, pgc->vsync_drop);
	len += scnprintf(stringbuf + len, buf_len - len,
			"|cmdq_handle_config=%p\tcmdq_handle_trigger=%p\tdpmgr_handle=%p\tovl2mem_path_handle=%p\n",
			pgc->cmdq_handle_config, pgc->cmdq_handle_trigger, pgc->dpmgr_handle,
			pgc->ovl2mem_path_handle);
	len += scnprintf(stringbuf + len, buf_len - len, "|Current display driver status=%s + %s\n",
			primary_display_is_video_mode() ? "video mode" : "cmd mode",
			primary_display_cmdq_enabled() ? "CMDQ Enabled" : "CMDQ Disabled");

	return len;
}

static char _dump_buffer_string[512];
static void _dump_internal_buffer_into(display_primary_path_context *ctx, char *dump_reason)
{
	int n = 0;
	int len = 0;
	disp_internal_buffer_info *buf = NULL;
	struct list_head *head = NULL;
	int i = 0;

	len = sizeof(_dump_buffer_string);
	n += scnprintf(_dump_buffer_string + n, len - n, "dump for %s\n",
		       dump_reason ? dump_reason : "unknown");

	if (ctx) {
		mutex_lock(&(pgc->dc_lock));

		head = &(pgc->dc_free_list);
		i = 0;
		n += scnprintf(_dump_buffer_string + n, len - n, "free list: ");
		if (!list_empty(head)) {
			list_for_each_entry(buf, head, list)
				n += scnprintf(_dump_buffer_string + n, len - n, "\t0x%08x", buf->mva);
		}

		n += scnprintf(_dump_buffer_string + n, len - n, "\n");

#if 0
		head = &(pgc->dc_writing_list);
		i = 0;
		n += scnprintf(_dump_buffer_string + n, len - n, "writing list: ");
		if (!list_empty(head)) {
			list_for_each_entry(buf, head, list)
				n += scnprintf(_dump_buffer_string + n, len - n, "\t0x%08x", buf->mva);
		}
		n += scnprintf(_dump_buffer_string + n, len - n, "\n");
#endif

		head = &(pgc->dc_reading_list);
		i = 0;
		n += scnprintf(_dump_buffer_string + n, len - n, "reading list: ");
		if (!list_empty(head)) {
			list_for_each_entry(buf, head, list)
				n += scnprintf(_dump_buffer_string + n, len - n, "\t0x%08x", buf->mva);
		}
		n += scnprintf(_dump_buffer_string + n, len - n, "\n");

		DISPMSG("%s", _dump_buffer_string);
		mutex_unlock(&(pgc->dc_lock));
	}
}

int32_t decouple_rdma_worker_callback(unsigned long data)
{
	disp_internal_buffer_info *reading_buf = NULL;
	disp_internal_buffer_info *temp = NULL;
	disp_internal_buffer_info *n = NULL;
	unsigned int current_reading_addr = 0;

	mutex_lock(&(pgc->dc_lock));
	current_reading_addr = ddp_ovl_get_cur_addr(1, 0);
	DISPMSG("rdma applied, 0x%lx, 0x%08x\n", data, current_reading_addr);

	reading_buf = find_buffer_by_mva(pgc, &pgc->dc_reading_list, data);
	if (!reading_buf) {
		/* DISPERR("fatal error, we can't find related buffer info with callback data:0x%08x\n", data); */
		mutex_unlock(&(pgc->dc_lock));
		return 0;
	}

	list_for_each_entry_safe(temp, n, &(pgc->dc_reading_list), list) {
		if (temp && temp->mva != current_reading_addr) {
			DISPMSG("temp=0x%p, temp->mva=0x%08x, temp->list=0x%lx\n", temp, temp->mva,
				(unsigned long)(&temp->list));
			reset_buffer(pgc, temp);
			enqueue_buffer(pgc, &(pgc->dc_free_list), temp);
		}
	}

	mutex_unlock(&(pgc->dc_lock));

	_dump_internal_buffer_into(pgc, "interface applied");

	return 0;
}

#if 0
/**
 * Defined but not used, avoid build warning. (2015.1.30 Rynn Wu)
 */
static int _disp_primary_path_decouple_worker_thread(void *data)
{
	int ret = 0;
	primary_disp_input_config input;
	disp_internal_buffer_info *writing_buf = NULL;
	disp_internal_buffer_info *reading_buf = NULL;
	uint32_t current_writing_mva = 0;
	disp_ddp_path_config *pconfig = NULL;
	struct sched_param param = { .sched_priority = 90 };

	sched_setscheduler(current, SCHED_RR, &param);


	while (1) {
		/* shuold wait FRAME_START here, but MUTEX1's sof always issued with MUTEX0's sof,
		 * root cause still unkonwn. so use FRAME_COMPLETE instead
		 */
		dpmgr_wait_event(pgc->ovl2mem_path_handle, DISP_PATH_EVENT_FRAME_COMPLETE);
		pconfig = dpmgr_path_get_last_config(pgc->ovl2mem_path_handle);
		current_writing_mva = pconfig->wdma_config.dstAddress;


		_dump_internal_buffer_into(pgc, "ovl2mem done");
		disp_sw_mutex_lock(&(pgc->dc_lock));
		writing_buf = find_buffer_by_mva(pgc, &pgc->dc_writing_list, current_writing_mva);
		if (!writing_buf) {
			DISPERR("fatal error, we can't find related buffer info with current_writing_mva:0x%08x\n",
				current_writing_mva);
			disp_sw_mutex_unlock(&(pgc->dc_lock));
			continue;
		} else {
			reset_buffer(pgc, writing_buf);
			enqueue_buffer(pgc, &(pgc->dc_reading_list), writing_buf);

			reading_buf = dequeue_buffer(pgc, &(pgc->dc_free_list));
			if (!reading_buf) {
				DISPERR("dc_free_list is empty!!\n");

				disp_sw_mutex_unlock(&(pgc->dc_lock));
				continue;
			} else {
				enqueue_buffer(pgc, &(pgc->dc_writing_list), reading_buf);
			}
			disp_sw_mutex_unlock(&(pgc->dc_lock));
		}

		_dump_internal_buffer_into(pgc, "trigger interface");

		pconfig->wdma_config.dstAddress		= reading_buf->mva;
		pconfig->wdma_config.srcHeight		= primary_display_get_height();
		pconfig->wdma_config.srcWidth		= primary_display_get_width();
		pconfig->wdma_config.clipX		= 0;
		pconfig->wdma_config.clipY		= 0;
		pconfig->wdma_config.clipHeight		= primary_display_get_height();
		pconfig->wdma_config.clipWidth		= primary_display_get_width();
		pconfig->wdma_config.outputFormat	= eRGB888;
		pconfig->wdma_config.useSpecifiedAlpha	= 1;
		pconfig->wdma_config.alpha		= 0xFF;
		pconfig->wdma_config.dstPitch		= primary_display_get_width() *
								DP_COLOR_BITS_PER_PIXEL(eRGB888) / 8;
		pconfig->wdma_dirty			= 1;

		ret = dpmgr_path_config(pgc->ovl2mem_path_handle, pconfig, CMDQ_DISABLE);

		memset((void *)&input, 0, sizeof(primary_disp_input_config));

		input.addr = (unsigned int) writing_buf->mva;
		input.src_x = 0;
		input.src_y = 0;
		input.src_w = primary_display_get_width();
		input.src_h = primary_display_get_height();
		input.dst_x = 0;
		input.dst_y = 0;
		input.dst_w = primary_display_get_width();
		input.dst_h = primary_display_get_height();
		input.fmt = eRGB888;
		input.alpha = 0xFF;

		input.src_pitch = primary_display_get_width() * 3;
		input.isDirty = 1;
		MMProfileLogEx(ddp_mmp_get_events()->interface_trigger, MMProfileFlagPulse,
			       input.addr, pconfig->wdma_config.dstAddress);
		ret = primary_display_config_interface_input(&input);
		ret = _trigger_display_interface(FALSE, decouple_rdma_worker_callback, ddp_ovl_get_cur_addr(1, 0));

		if (kthread_should_stop())
			break;
	}
	return 0;
}
#endif

static DISP_MODULE_ENUM _get_dst_module_by_lcm(disp_lcm_handle *plcm)
{
	if (plcm == NULL) {
		DISPERR("plcm is null\n");
		return DISP_MODULE_UNKNOWN;
	}

	if (plcm->params->type == LCM_TYPE_DSI) {
		if (plcm->lcm_if_id == LCM_INTERFACE_DSI0)
			return DISP_MODULE_DSI0;
		else if (plcm->lcm_if_id == LCM_INTERFACE_DSI1)
			return DISP_MODULE_DSI1;
		else if (plcm->lcm_if_id == LCM_INTERFACE_DSI_DUAL)
			return DISP_MODULE_DSIDUAL;
		else
			return DISP_MODULE_DSI0;
	} else if (plcm->params->type == LCM_TYPE_DPI) {
		return DISP_MODULE_DPI;
	}

	DISPERR("can't find primary path dst module\n");
	return DISP_MODULE_UNKNOWN;
}

#define AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA

/**
 * trigger operation:  VDO+CMDQ  CMD+CMDQ VDO+CPU  CMD+CPU
 * 1.wait idle:           N         N       Y        Y
 * 2.lcm update:          N         Y       N        Y
 * 3.path start:	idle->Y      Y    idle->Y    Y
 * 4.path trigger:     idle->Y      Y    idle->Y     Y
 * 5.mutex enable:        N         N    idle->Y     Y
 * 6.set cmdq dirty:      N         Y       N        N
 * 7.flush cmdq:          Y         Y       N        N
 */

/**
 * trigger operation:  VDO+CMDQ  CMD+CMDQ VDO+CPU  CMD+CPU
 * 1.wait idle:	          N         N        Y        Y
 */
int _should_wait_path_idle(void)
{
	if (primary_display_cmdq_enabled()) {
		if (primary_display_is_video_mode())
			return 0;
		else
			return 0;
	} else {
		if (primary_display_is_video_mode())
			return dpmgr_path_is_busy(pgc->dpmgr_handle);
		else
			return dpmgr_path_is_busy(pgc->dpmgr_handle);
	}
}

/**
 * trigger operation:  VDO+CMDQ  CMD+CMDQ VDO+CPU  CMD+CPU
 * 2.lcm update:          N         Y       N        Y
 */
int _should_update_lcm(void)
{
	if (primary_display_cmdq_enabled()) {
		if (primary_display_is_video_mode())
			return 0;

		/* TODO: lcm_update can't use cmdq now */
		return 0;
	}

	if (primary_display_is_video_mode())
		return 0;
	else
		return 1;
}

/**
 * trigger operation:  VDO+CMDQ  CMD+CMDQ VDO+CPU  CMD+CPU
 * 3.path start:	idle->Y      Y    idle->Y     Y
 */
int _should_start_path(void)
{
	if (primary_display_cmdq_enabled()) {
		if (primary_display_is_video_mode())
			return 0;
			/* return dpmgr_path_is_idle(pgc->dpmgr_handle); */
		else
			return 0;
	} else {
		if (primary_display_is_video_mode())
			return dpmgr_path_is_idle(pgc->dpmgr_handle);
		else
			return 1;
	}
}

/**
 * trigger operation:  VDO+CMDQ  CMD+CMDQ VDO+CPU  CMD+CPU
 * 4.path trigger:     idle->Y      Y    idle->Y     Y
 * 5.mutex enable:        N         N    idle->Y     Y
 */
int _should_trigger_path(void)
{
	/* this is not a perfect design, we can't decide path trigger(ovl/rdma/dsi..) separately with mutex enable */
	/* but it's lucky because path trigger and mutex enable is the same w/o cmdq, and it's correct w/ CMDQ(Y+N). */
	if (primary_display_cmdq_enabled()) {
		if (primary_display_is_video_mode())
			return 0;
			/* return dpmgr_path_is_idle(pgc->dpmgr_handle); */
		else
			return 0;
	} else {
		if (primary_display_is_video_mode())
			return dpmgr_path_is_idle(pgc->dpmgr_handle);
		else
			return 1;
	}
}

/**
 * trigger operation:  VDO+CMDQ  CMD+CMDQ VDO+CPU  CMD+CPU
 * 6.set cmdq dirty:	    N         Y       N        N
 */
int _should_set_cmdq_dirty(void)
{
	if (primary_display_cmdq_enabled()) {
		if (primary_display_is_video_mode())
			return 0;
		else
			return 1;
	} else {
		if (primary_display_is_video_mode())
			return 0;
		else
			return 0;
	}
}

/**
 * trigger operation:  VDO+CMDQ  CMD+CMDQ VDO+CPU  CMD+CPU
 * 7.flush cmdq:          Y         Y       N        N
 */
int _should_flush_cmdq_config_handle(void)
{
	if (primary_display_cmdq_enabled()) {
		if (primary_display_is_video_mode())
			return 1;
		else
			return 1;
	} else {
		if (primary_display_is_video_mode())
			return 0;
		else
			return 0;
	}
}

int _should_reset_cmdq_config_handle(void)
{
	if (primary_display_cmdq_enabled()) {
		if (primary_display_is_video_mode())
			return 1;
		else
			return 1;
	} else {
		if (primary_display_is_video_mode())
			return 0;
		else
			return 0;
	}
}

/**
 * trigger operation:  VDO+CMDQ  CMD+CMDQ VDO+CPU  CMD+CPU
 * 7.flush cmdq:          Y         Y       N        N
 */
int _should_insert_wait_frame_done_token(void)
{
	if (primary_display_cmdq_enabled()) {
		if (primary_display_is_video_mode())
			return 1;
		else
			return 1;
	} else {
		if (primary_display_is_video_mode())
			return 0;
		else
			return 0;
	}
}

int _should_trigger_interface(void)
{
	if (pgc->mode == DECOUPLE_MODE)
		return 0;
	else
		return 1;
}

int _should_config_ovl_input(void)
{
	/* should extend this when display path dynamic switch is ready */
	if (pgc->mode == SINGLE_LAYER_MODE || pgc->mode == DEBUG_RDMA1_DSI0_MODE)
		return 0;
	else
		return 1;

}

int _should_config_ovl_to_memory(display_primary_path_context *ctx)
{
	if (ctx == NULL) {
		DISPAEE("Context is NULL!\n");
		return 0;
	}

	if (ctx->mode == DECOUPLE_MODE)
		return 1;
	else
		return 0;
}

#define OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO
static unsigned long get_current_time_us(void)
{
	struct timeval t;

	do_gettimeofday(&t);
	return (t.tv_sec & 0xFFF) * 1000000 + t.tv_usec;
}

static struct hrtimer cmd_mode_update_timer;
static int is_fake_timer_inited;
static enum hrtimer_restart _DISP_CmdModeTimer_handler(struct hrtimer *timer)
{
	dpmgr_signal_event(pgc->dpmgr_handle, DISP_PATH_EVENT_IF_VSYNC);
#if 0
	if ((get_current_time_us() - pgc->last_vsync_tick) > 16666) {
		dpmgr_signal_event(pgc->dpmgr_handle, DISP_PATH_EVENT_IF_VSYNC);
		pgc->last_vsync_tick = get_current_time_us();
	}
#endif
	hrtimer_forward_now(timer, ns_to_ktime(16666666));
	return HRTIMER_RESTART;
}

int _init_vsync_fake_monitor(int fps)
{
	if (is_fake_timer_inited)
		return 0;

	is_fake_timer_inited = 1;

	if (fps == 0)
		fps = 6000;

	hrtimer_init(&cmd_mode_update_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	cmd_mode_update_timer.function = _DISP_CmdModeTimer_handler;
	hrtimer_start(&cmd_mode_update_timer, ns_to_ktime(16666666), HRTIMER_MODE_REL);

	DISPMSG("fake timer, init\n");
	return 0;
}

#define DISP_REG_SODI_PA 0x10006b0c
/* extern unsigned int gEnableSODIControl; */
/* extern unsigned int gPrefetchControl; */
void disp_set_sodi(unsigned int enable, void *cmdq_handle)
{
#ifndef CONFIG_FPGA_EARLY_PORTING
#ifdef FORCE_SODI_BY_SW
	if (gEnableSODIControl == 1) {
		if (cmdq_handle != NULL) {
			if (enable == 1)
				cmdqRecWrite(cmdq_handle, DISP_REG_SODI_PA, 0, 1);
			else
				cmdqRecWrite(cmdq_handle, DISP_REG_SODI_PA, 1, 1);
		} else {
			if (enable == 1)
				DISP_REG_SET(0, SPM_PCM_SRC_REQ,
					     DISP_REG_GET(SPM_PCM_SRC_REQ) & (~0x1));
			else
				DISP_REG_SET(0, SPM_PCM_SRC_REQ,
					     DISP_REG_GET(SPM_PCM_SRC_REQ) | 0x1);
		}
	}
#endif
#endif
}

/* extern unsigned int gEnableSWTrigger; */
/* extern unsigned int gEnableMutexRisingEdge; */
/* extern unsigned int gDisableSODIForTriggerLoop; */
static void _cmdq_build_trigger_loop(void)
{
#ifndef MTK_FB_CMDQ_DISABLE
	int ret = 0;
	int i = 0;

	if (pgc->cmdq_handle_trigger == NULL) {
		cmdqRecCreate(CMDQ_SCENARIO_TRIGGER_LOOP, &(pgc->cmdq_handle_trigger));
		DISPMSG("primary path trigger thread cmd handle=%p\n", pgc->cmdq_handle_trigger);
	}
	cmdqRecReset(pgc->cmdq_handle_trigger);

	if (primary_display_is_video_mode()) {
		/* wait and clear stream_done, HW will assert mutex enable automatically in frame done reset. */
		/* todo: should let dpmanager to decide wait which mutex's eof. */

		ret = cmdqRecWait(pgc->cmdq_handle_trigger, CMDQ_EVENT_DISP_RDMA0_EOF);
		ret = cmdqRecWait(pgc->cmdq_handle_trigger,
				  dpmgr_path_get_mutex(pgc->dpmgr_handle) +
				  CMDQ_EVENT_MUTEX0_STREAM_EOF);

		/* wait and clear rdma0_sof for vfp change */
		cmdqRecClearEventToken(pgc->cmdq_handle_trigger, CMDQ_EVENT_DISP_RDMA0_SOF);

		if (gEnableSWTrigger == 1)
			DISP_REG_SET(pgc->cmdq_handle_trigger, DISP_REG_CONFIG_MUTEX_EN(DISP_OVL_SEPARATE_MUTEX_ID), 1);

		/* for some module(like COLOR) to read hw register to GPR after frame done */
		dpmgr_path_build_cmdq(pgc->dpmgr_handle, pgc->cmdq_handle_trigger,
				      CMDQ_AFTER_STREAM_EOF);
	} else {
		ret = cmdqRecWaitNoClear(pgc->cmdq_handle_trigger, CMDQ_SYNC_TOKEN_CABC_EOF);
		/* DSI command mode doesn't have mutex_stream_eof, need use CMDQ token instead */
		ret = cmdqRecWait(pgc->cmdq_handle_trigger, CMDQ_SYNC_TOKEN_CONFIG_DIRTY);

		if (_need_wait_esd_eof())
			/* Wait esd config thread done. */
			ret = cmdqRecWaitNoClear(pgc->cmdq_handle_trigger, CMDQ_SYNC_TOKEN_ESD_EOF);

		/* ret = cmdqRecWait(pgc->cmdq_handle_trigger, CMDQ_EVENT_MDP_DSI0_TE_SOF); */
		/* for operations before frame transfer, such as waiting for DSI TE */
		if (islcmconnected)
			dpmgr_path_build_cmdq(pgc->dpmgr_handle, pgc->cmdq_handle_trigger, CMDQ_BEFORE_STREAM_SOF);

		/* pull DSI clk lane back to HS */
		DSI_manual_enter_HS(pgc->cmdq_handle_trigger);

		/* cleat frame done token, now the config thread will not allowed to config registers. */
		/* remember that config thread's priority is higher than trigger thread,
		 * so all the config queued before will be applied then STREAM_EOF token be cleared
		 */
		/* this is what CMDQ did as "Merge" */
		ret = cmdqRecClearEventToken(pgc->cmdq_handle_trigger, CMDQ_SYNC_TOKEN_STREAM_EOF);

		ret = cmdqRecClearEventToken(pgc->cmdq_handle_trigger, CMDQ_SYNC_TOKEN_CONFIG_DIRTY);

		/* enable mutex, only cmd mode need this */
		/* this is what CMDQ did as "Trigger" */
		/* clear rdma EOF token before wait */
		ret = cmdqRecClearEventToken(pgc->cmdq_handle_trigger, CMDQ_EVENT_DISP_RDMA0_EOF);

#ifdef FORCE_SODI_BY_SW
		/* TODO: use separate api for sodi control flow later */
		/* Enable EMI Force On(Equal to SODI Disable) */
		cmdqRecWrite(pgc->cmdq_handle_trigger, 0x10006b0c, 1, 1);
#endif

#ifndef FORCE_SODI_CG_MODE
		/* Enable SPM CG Mode(Force 30+ times to ensure write success, need find root cause and fix later) */
		cmdqRecWrite(pgc->cmdq_handle_trigger, 0x10006b04, 0x80, 0x80);
		/* Polling EMI Status to ensure EMI is enabled */
		cmdqRecPoll(pgc->cmdq_handle_trigger, 0x100063b4, 0, 0x00200000);
#endif

#ifdef FORCE_SODI_BY_SW
		/* Clear EMI Force on, Let SPM control EMI state now */
		cmdqRecWrite(pgc->cmdq_handle_trigger, 0x10006b0c, 0, 1);
#endif
		dpmgr_path_trigger(pgc->dpmgr_handle, pgc->cmdq_handle_trigger, CMDQ_ENABLE);
		/* ret = cmdqRecWrite(pgc->cmdq_handle_trigger,
				      (unsigned int)(DISP_REG_CONFIG_MUTEX_EN(0))&0x1fffffff, 1, ~0);
		 */

		/* backup DSI state register to slot */
		if (gEnableDSIStateCheck == 1) {
			for (i = 0; i < 10; i++) {
				cmdqRecBackupRegisterToSlot(pgc->cmdq_handle_trigger, pgc->dsi_state_info, i,
							    DISP_REG_DSI_STATE);
				cmdqRecBackupRegisterToSlot(pgc->cmdq_handle_trigger, pgc->rdma_state_info, i * 5 + 0,
							    DISP_REG_RDMA_INT_STATUS);
				cmdqRecBackupRegisterToSlot(pgc->cmdq_handle_trigger, pgc->rdma_state_info, i * 5 + 1,
							    DISP_REG_RDMA_IN_P_CNT);
				cmdqRecBackupRegisterToSlot(pgc->cmdq_handle_trigger, pgc->rdma_state_info, i * 5 + 2,
							    DISP_REG_RDMA_IN_LINE_CNT);
				cmdqRecBackupRegisterToSlot(pgc->cmdq_handle_trigger, pgc->rdma_state_info, i * 5 + 3,
							    DISP_REG_RDMA_OUT_P_CNT);
				cmdqRecBackupRegisterToSlot(pgc->cmdq_handle_trigger, pgc->rdma_state_info, i * 5 + 4,
							    DISP_REG_RDMA_OUT_LINE_CNT);
			}
		}
		/* SODI is disabled in config thread, so mutex enable/dsi start/CPU wait
		 * TE will not be blocked by SODI
		 */
		/* should enable SODI here, */
		if (gDisableSODIForTriggerLoop == 1)
			disp_set_sodi(1, pgc->cmdq_handle_trigger);

		/* waiting for frame done, because we can't use mutex stream eof here,
		 * so need to let dpmanager help to decide which event to wait
		 */
		/* most time we wait rdmax frame done event. */
		ret = cmdqRecWait(pgc->cmdq_handle_trigger, CMDQ_EVENT_DISP_RDMA0_EOF);

		dpmgr_path_build_cmdq(pgc->dpmgr_handle, pgc->cmdq_handle_trigger, CMDQ_WAIT_STREAM_EOF_EVENT);

		/* dsi is not idle rightly after rdma frame done,
		 * so we need to polling about 1us for dsi returns to idle
		 */
		/* do not polling dsi idle directly which will decrease CMDQ performance */
		dpmgr_path_build_cmdq(pgc->dpmgr_handle, pgc->cmdq_handle_trigger, CMDQ_CHECK_IDLE_AFTER_STREAM_EOF);

		/* for some module(like COLOR) to read hw register to GPR after frame done */
		dpmgr_path_build_cmdq(pgc->dpmgr_handle, pgc->cmdq_handle_trigger, CMDQ_AFTER_STREAM_EOF);

#ifndef FORCE_SODI_CG_MODE
		/* Enable EMI Power Down Mode */
		cmdqRecWrite(pgc->cmdq_handle_trigger, 0x10006b04, 0, 0x80);
#endif

		/* pull DSI clk lane to LP */
		DSI_sw_clk_trail_cmdq(0, pgc->cmdq_handle_trigger);
		/* polling DSI idle */
		/* ret = cmdqRecPoll(pgc->cmdq_handle_trigger, 0x1401b00c, 0, 0x80000000); */
		/* polling wdma frame done */
		/* ret = cmdqRecPoll(pgc->cmdq_handle_trigger, 0x140060A0, 1, 0x1); */

		/* now frame done, config thread is allowed to config register now */
		ret = cmdqRecSetEventToken(pgc->cmdq_handle_trigger, CMDQ_SYNC_TOKEN_STREAM_EOF);

		/* RUN forever!!!! */
		BUG_ON(ret < 0);
	}

	/* dump trigger loop instructions to check whether dpmgr_path_build_cmdq works correctly */
	pr_warn("[DISP]primary display BUILD cmdq trigger loop finished\n");

	return;
#endif
}

void disp_spm_enter_cg_mode(void)
{
	MMProfileLogEx(ddp_mmp_get_events()->cg_mode, MMProfileFlagPulse, 0, 0);
}

void disp_spm_enter_power_down_mode(void)
{
	MMProfileLogEx(ddp_mmp_get_events()->power_down_mode, MMProfileFlagPulse, 0, 0);
}

#if 0
/**
 * Defined but not used, avoid build warning. (2015.1.30 Rynn Wu)
 */
static void _cmdq_build_monitor_loop(void)
{
	int ret = 0;
	cmdqRecHandle g_cmdq_handle_monitor;

	cmdqRecCreate(CMDQ_SCENARIO_DISP_SCREEN_CAPTURE, &(g_cmdq_handle_monitor));
	DISPMSG("primary path monitor thread cmd handle=%p\n", g_cmdq_handle_monitor);

	cmdqRecReset(g_cmdq_handle_monitor);

	/* wait and clear stream_done, HW will assert mutex enable automatically in frame done reset. */
	/* todo: should let dpmanager to decide wait which mutex's eof. */
	ret = cmdqRecWait(g_cmdq_handle_monitor, CMDQ_EVENT_DISP_RDMA0_UNDERRUN);

	cmdqRecReadToDataRegister(g_cmdq_handle_monitor, 0x10006b0c, CMDQ_DATA_REG_2D_SHARPNESS_1_DST);
	cmdqRecWriteFromDataRegister(g_cmdq_handle_monitor, CMDQ_DATA_REG_2D_SHARPNESS_1_DST, 0x1401b280);

	cmdqRecReadToDataRegister(g_cmdq_handle_monitor, 0x10006b08, CMDQ_DATA_REG_2D_SHARPNESS_1_DST);
	cmdqRecWriteFromDataRegister(g_cmdq_handle_monitor, CMDQ_DATA_REG_2D_SHARPNESS_1_DST, 0x1401b284);

	cmdqRecReadToDataRegister(g_cmdq_handle_monitor, 0x10006b04, CMDQ_DATA_REG_2D_SHARPNESS_1_DST);
	cmdqRecWriteFromDataRegister(g_cmdq_handle_monitor, CMDQ_DATA_REG_2D_SHARPNESS_1_DST, 0x1401b288);

	cmdqRecReadToDataRegister(g_cmdq_handle_monitor, 0x1401b16c, CMDQ_DATA_REG_2D_SHARPNESS_1_DST);
	cmdqRecWriteFromDataRegister(g_cmdq_handle_monitor, CMDQ_DATA_REG_2D_SHARPNESS_1_DST, 0x1401b28C);

	ret = cmdqRecStartLoop(g_cmdq_handle_monitor);
}
#endif

static void _cmdq_start_trigger_loop(void)
{
#ifndef MTK_FB_CMDQ_DISABLE
	int ret = 0;

	cmdqRecDumpCommand(pgc->cmdq_handle_trigger);
	/* this should be called only once because trigger loop will nevet stop */
	ret = cmdqRecStartLoop(pgc->cmdq_handle_trigger);
	if (!primary_display_is_video_mode()) {
		/* Need set esd check eof synctoken to let trigger loop go. */
		if (_need_wait_esd_eof())
			cmdqCoreSetEvent(CMDQ_SYNC_TOKEN_ESD_EOF);

		/* need to set STREAM_EOF for the first time, otherwise we will stuck in dead loop */
		cmdqCoreSetEvent(CMDQ_SYNC_TOKEN_STREAM_EOF);
		cmdqCoreSetEvent(CMDQ_SYNC_TOKEN_CABC_EOF);
		dprec_event_op(DPREC_EVENT_CMDQ_SET_EVENT_ALLOW);
	} else {
#if 0
		if (dpmgr_path_is_idle(pgc->dpmgr_handle))
			cmdqCoreSetEvent(CMDQ_EVENT_MUTEX0_STREAM_EOF);

#endif
	}

	if (_is_decouple_mode(pgc->session_mode))
		cmdqCoreSetEvent(CMDQ_EVENT_DISP_WDMA0_EOF);

	pr_warn("[DISP]primary display START cmdq trigger loop finished\n");
#endif
}

static void _cmdq_stop_trigger_loop(void)
{
#ifndef MTK_FB_CMDQ_DISABLE
	int ret = 0;

	/* this should be called only once because trigger loop will nevet stop */
	ret = cmdqRecStopLoop(pgc->cmdq_handle_trigger);

	DISPMSG("primary display STOP cmdq trigger loop finished\n");
#endif
}


static void _cmdq_set_config_handle_dirty(void)
{
#ifndef MTK_FB_CMDQ_DISABLE
	if (!primary_display_is_video_mode()) {
		dprec_logger_start(DPREC_LOGGER_PRIMARY_CMDQ_SET_DIRTY, 0, 0);
		/* only command mode need to set dirty */
		cmdqRecSetEventToken(pgc->cmdq_handle_config, CMDQ_SYNC_TOKEN_CONFIG_DIRTY);
		dprec_event_op(DPREC_EVENT_CMDQ_SET_DIRTY);
		dprec_logger_done(DPREC_LOGGER_PRIMARY_CMDQ_SET_DIRTY, 0, 0);
	}
#endif
}

static void _cmdq_set_config_handle_dirty_mira(void *handle)
{
#ifndef MTK_FB_CMDQ_DISABLE
	if (!primary_display_is_video_mode()) {
		dprec_logger_start(DPREC_LOGGER_PRIMARY_CMDQ_SET_DIRTY, 0, 0);
		/* only command mode need to set dirty */
		cmdqRecSetEventToken(handle, CMDQ_SYNC_TOKEN_CONFIG_DIRTY);
		dprec_event_op(DPREC_EVENT_CMDQ_SET_DIRTY);
		dprec_logger_done(DPREC_LOGGER_PRIMARY_CMDQ_SET_DIRTY, 0, 0);
	}
#endif
}

static void _cmdq_reset_config_handle(void)
{
#ifndef MTK_FB_CMDQ_DISABLE
	cmdqRecReset(pgc->cmdq_handle_config);
	dprec_event_op(DPREC_EVENT_CMDQ_RESET);
#endif
}

static void _cmdq_flush_config_handle(int blocking, void *callback, unsigned int userdata)
{
#ifndef MTK_FB_CMDQ_DISABLE
	dprec_logger_start(DPREC_LOGGER_PRIMARY_CMDQ_FLUSH, blocking,
			   (unsigned int)(unsigned long)callback);
	if (blocking) {
		/* DISPERR("Should not use blocking cmdq flush,
		 * may block primary display path for 1 frame period\n");
		 */
		cmdqRecFlush(pgc->cmdq_handle_config);
	} else {
		if (callback)
			cmdqRecFlushAsyncCallback(pgc->cmdq_handle_config, callback, userdata);
		else
			cmdqRecFlushAsync(pgc->cmdq_handle_config);
	}

	dprec_event_op(DPREC_EVENT_CMDQ_FLUSH);
	dprec_logger_done(DPREC_LOGGER_PRIMARY_CMDQ_FLUSH, userdata, 0);
#endif
}

static void _cmdq_flush_config_handle_mira(void *handle, int blocking)
{
#ifndef MTK_FB_CMDQ_DISABLE
	dprec_logger_start(DPREC_LOGGER_PRIMARY_CMDQ_FLUSH, 0, 0);
	if (blocking)
		cmdqRecFlush(handle);
	else
		cmdqRecFlushAsync(handle);

	dprec_event_op(DPREC_EVENT_CMDQ_FLUSH);
	dprec_logger_done(DPREC_LOGGER_PRIMARY_CMDQ_FLUSH, 0, 0);
#endif
}

static void _cmdq_insert_wait_frame_done_token(void)
{
#ifndef MTK_FB_CMDQ_DISABLE
	if (primary_display_is_video_mode())
		cmdqRecWaitNoClear(pgc->cmdq_handle_config, CMDQ_EVENT_MUTEX0_STREAM_EOF);
	else
		cmdqRecWaitNoClear(pgc->cmdq_handle_config, CMDQ_SYNC_TOKEN_STREAM_EOF);

	dprec_event_op(DPREC_EVENT_CMDQ_WAIT_STREAM_EOF);
#endif
}

void _cmdq_insert_wait_frame_done_token_mira(void *handle)
{
#ifndef MTK_FB_CMDQ_DISABLE
	if (primary_display_is_video_mode())
		cmdqRecWaitNoClear(handle, CMDQ_EVENT_MUTEX0_STREAM_EOF);
	else
		cmdqRecWaitNoClear(handle, CMDQ_SYNC_TOKEN_STREAM_EOF);

	dprec_event_op(DPREC_EVENT_CMDQ_WAIT_STREAM_EOF);
#endif
}

static void directlink_path_add_memory(WDMA_CONFIG_STRUCT *p_wdma)
{
	int ret = 0;
	int secure_path_on = primary_display_is_secure_path(DISP_SESSION_PRIMARY);
	cmdqRecHandle cmdq_handle = NULL;
	cmdqRecHandle cmdq_wait_handle = NULL;
	disp_ddp_path_config *pconfig = NULL;

	/*create config thread*/
	ret = cmdqRecCreate(CMDQ_SCENARIO_PRIMARY_DISP, &cmdq_handle);
	if (ret != 0) {
		DISPMSG("dl_to_dc capture:Fail to create cmdq handle\n");
		ret = -1;
		goto out;
	}
	cmdqRecReset(cmdq_handle);

	/*create wait thread*/
	ret = cmdqRecCreate(CMDQ_SCENARIO_DISP_SCREEN_CAPTURE, &cmdq_wait_handle);
	if (ret != 0) {
		DISPMSG("dl_to_dc capture:Fail to create cmdq wait handle\n");
		ret = -1;
		goto out;
	}
	cmdqRecReset(cmdq_wait_handle);

	/*configure  config thread*/
	if (!secure_path_on)
		cmdqRecEnablePrefetch(cmdq_handle);
	_cmdq_insert_wait_frame_done_token_mira(cmdq_handle);

	dpmgr_path_add_memout(pgc->dpmgr_handle, ENGINE_OVL0, cmdq_handle);

	pconfig = dpmgr_path_get_last_config(pgc->dpmgr_handle);
	pconfig->wdma_config = *p_wdma;
	pconfig->wdma_dirty  = 1;
	ret = dpmgr_path_config(pgc->dpmgr_handle, pconfig, cmdq_handle);

	_cmdq_set_config_handle_dirty_mira(cmdq_handle);

	dpmgr_wdma_path_force_power_on();
	if (!secure_path_on)
		cmdqRecDisablePrefetch(cmdq_handle);
	_cmdq_flush_config_handle_mira(cmdq_handle, 0);
	DISPMSG("dl_to_dc capture:Flush add memout mva(0x%lx)\n", p_wdma->dstAddress);

	/*wait wdma0 sof*/
	cmdqRecWait(cmdq_wait_handle, CMDQ_EVENT_DISP_WDMA0_SOF);
	cmdqRecFlush(cmdq_wait_handle);
	DISPMSG("dl_to_dc capture:Flush wait wdma sof\n");
#if 0
	cmdqRecReset(cmdq_handle);
	_cmdq_insert_wait_frame_done_token_mira(cmdq_handle);

	dpmgr_path_remove_memout(pgc->dpmgr_handle, cmdq_handle);
	_cmdq_set_config_handle_dirty_mira(cmdq_handle);
	/* flush remove memory to cmdq */
	_cmdq_flush_config_handle_mira(cmdq_handle, 0);
	DISPMSG("dl_to_dc capture: Flush remove memout\n");

	dpmgr_path_memout_clock(pgc->dpmgr_handle, 0);
#endif
out:
	cmdqRecDestroy(cmdq_handle);
	cmdqRecDestroy(cmdq_wait_handle);
}


static unsigned int _get_switch_dc_buffer(void)
{
	if (pgc->freeze_buf)
		return pgc->freeze_buf;
#ifdef CONFIG_MTK_GMO_RAM_OPTIMIZE
	return pgc->dc_buf[0];
#else
	if (primary_display_is_secure_path(DISP_SESSION_PRIMARY))
		return pgc->session_buf[pgc->session_buf_id];
	else
		return pgc->dc_buf[pgc->dc_buf_id];
#endif
}

static int _DL_switch_to_DC_fast(void)
{
	int ret = 0;
	int secure_path_on = primary_display_is_secure_path(DISP_SESSION_PRIMARY);

	RDMA_CONFIG_STRUCT rdma_config = decouple_rdma_config;
	WDMA_CONFIG_STRUCT wdma_config = decouple_wdma_config;

	disp_ddp_path_config *data_config_dl = NULL;
	disp_ddp_path_config *data_config_dc = NULL;
	unsigned int mva;

	mva = _get_switch_dc_buffer();
	wdma_config.dstAddress = mva;
	if (secure_path_on)
		wdma_config.security = DISP_SECURE_BUFFER;
	else
		wdma_config.security = DISP_NORMAL_BUFFER;

	/* disable SODI by CPU to prevent underflow */
#if defined(MTK_FB_SODI_SUPPORT) && !defined(CONFIG_FPGA_EARLY_PORTING)
	spm_enable_sodi(0);
#endif

	/* 1.save a temp frame to intermediate buffer */
	directlink_path_add_memory(&wdma_config);

	/* 2.reset primary handle */
	_cmdq_reset_config_handle();
	if (!secure_path_on)
		cmdqRecEnablePrefetch(pgc->cmdq_handle_config);
	_cmdq_insert_wait_frame_done_token();

	/* 3.modify interface path handle to new scenario(rdma->dsi) */
#ifdef CONFIG_FOR_SOURCE_PQ
	dpmgr_modify_path(pgc->dpmgr_handle, DDP_SCENARIO_PRIMARY_RDMA0_DISP, pgc->cmdq_handle_config,
			  primary_display_is_video_mode() ? DDP_VIDEO_MODE : DDP_CMD_MODE);
#else
	dpmgr_modify_path(pgc->dpmgr_handle, DDP_SCENARIO_PRIMARY_RDMA0_COLOR0_DISP, pgc->cmdq_handle_config,
			  primary_display_is_video_mode() ? DDP_VIDEO_MODE : DDP_CMD_MODE);
#endif
	/* 4.config rdma from directlink mode to memory mode */
	rdma_config.address = mva;
	rdma_config.security = DISP_NORMAL_BUFFER;
	rdma_config.pitch = primary_display_get_width() * DP_COLOR_BITS_PER_PIXEL(rdma_config.inputFormat) >> 3;
	if (secure_path_on) {
		rdma_config.address = pgc->session_buf[pgc->session_buf_id];
		rdma_config.security = DISP_SECURE_BUFFER;
	}

	data_config_dl = dpmgr_path_get_last_config(pgc->dpmgr_handle);

	data_config_dl->rdma_config = rdma_config;
	data_config_dl->rdma_dirty = 1;
	ret = dpmgr_path_config(pgc->dpmgr_handle, data_config_dl, pgc->cmdq_handle_config);

	/* 5. backup rdma address to slots */
	cmdqRecBackupUpdateSlot(pgc->cmdq_handle_config, pgc->rdma_buff_info, 0, mva);

	/* 6 .flush to cmdq */
	_cmdq_set_config_handle_dirty();
	if (!secure_path_on)
		cmdqRecDisablePrefetch(pgc->cmdq_handle_config);
	_cmdq_flush_config_handle(1, NULL, 0);

	/* ddp_mmp_rdma_layer(&rdma_config, 0,  20, 20); */

	/* 7.reset  cmdq */
	_cmdq_reset_config_handle();
	_cmdq_insert_wait_frame_done_token();

	/* 9. create ovl2mem path handle */
	cmdqRecReset(pgc->cmdq_handle_ovl1to2_config);
#ifdef CONFIG_FOR_SOURCE_PQ
	pgc->ovl2mem_path_handle = dpmgr_create_path(DDP_SCENARIO_PRIMARY_DITHER_MEMOUT,
						     pgc->cmdq_handle_ovl1to2_config);
#else
	pgc->ovl2mem_path_handle = dpmgr_create_path(DDP_SCENARIO_PRIMARY_OVL_MEMOUT,
						     pgc->cmdq_handle_ovl1to2_config);
#endif
	if (pgc->ovl2mem_path_handle) {
		DISPMSG("dpmgr create ovl memout path SUCCESS(%p)\n", pgc->ovl2mem_path_handle);
	} else {
		DISPMSG("dpmgr create path FAIL\n");
		return -1;
	}

	dpmgr_path_set_video_mode(pgc->ovl2mem_path_handle, 0);
#ifndef CONFIG_MTK_CLKMGR
	ddp_clk_prepare(DISP_MTCMOS_CLK);
#endif
	dpmgr_path_init(pgc->ovl2mem_path_handle, CMDQ_ENABLE);

	data_config_dc = dpmgr_path_get_last_config(pgc->ovl2mem_path_handle);

	data_config_dc->dst_w = rdma_config.width;
	data_config_dc->dst_h = rdma_config.height;
	data_config_dc->dst_dirty = 1;

	/* move ovl config info from dl to dc */
	memcpy(data_config_dc->ovl_config, data_config_dl->ovl_config, sizeof(data_config_dl->ovl_config));

	ret = dpmgr_path_config(pgc->ovl2mem_path_handle, data_config_dc, pgc->cmdq_handle_ovl1to2_config);
	ret = dpmgr_path_start(pgc->ovl2mem_path_handle, CMDQ_ENABLE);

	/* use blocking flush to make sure all config is done. */

	/* cmdqRecDumpCommand(pgc->cmdq_handle_ovl1to2_config); */
	cmdqRecClearEventToken(pgc->cmdq_handle_ovl1to2_config, CMDQ_EVENT_DISP_WDMA0_EOF);
	_cmdq_flush_config_handle_mira(pgc->cmdq_handle_ovl1to2_config, 1);
	cmdqRecReset(pgc->cmdq_handle_ovl1to2_config);

	/* 11..enable event for new path */
/* dpmgr_enable_event(pgc->ovl2mem_path_handle, DISP_PATH_EVENT_FRAME_COMPLETE); */
/* dpmgr_map_event_to_irq(pgc->ovl2mem_path_handle, DISP_PATH_EVENT_FRAME_START, DDP_IRQ_WDMA0_FRAME_COMPLETE); */
/* dpmgr_enable_event(pgc->ovl2mem_path_handle, DISP_PATH_EVENT_FRAME_START); */

	if (primary_display_is_video_mode())
		dpmgr_map_event_to_irq(pgc->dpmgr_handle, DISP_PATH_EVENT_IF_VSYNC, DDP_IRQ_RDMA0_DONE);

	dpmgr_enable_event(pgc->dpmgr_handle, DISP_PATH_EVENT_IF_VSYNC);
	/* dpmgr_enable_event(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_DONE); */
	dpmgr_enable_event(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_START);

	/* enable SODI after switch */
#if defined(MTK_FB_SODI_SUPPORT) && !defined(CONFIG_FPGA_EARLY_PORTING)
	spm_enable_sodi(1);
#endif

	return ret;
}


static int _DC_switch_to_DL_fast(void)
{
	int ret = 0;
	int layer = 0;
	int secure_path_on = primary_display_is_secure_path(DISP_SESSION_PRIMARY);
	disp_ddp_path_config *data_config_dl = NULL;
	disp_ddp_path_config *data_config_dc = NULL;
	OVL_CONFIG_STRUCT ovl_config[OVL_LAYER_NUM];

	/* 1. disable SODI */
#if defined(MTK_FB_SODI_SUPPORT) && !defined(CONFIG_FPGA_EARLY_PORTING)
	spm_enable_sodi(0);
#endif

	/* 2.enable ovl/wdma clock */

	/* 3.destroy ovl->mem path. */
#if defined(OVL_TIME_SHARING)
	data_config_dc = &last_primary_config;
#else
	data_config_dc = dpmgr_path_get_last_config(pgc->ovl2mem_path_handle);
#endif
	/*save ovl info */;
	memcpy(ovl_config, data_config_dc->ovl_config, sizeof(ovl_config));

	dpmgr_path_deinit(pgc->ovl2mem_path_handle,
			  (int)(unsigned long)pgc->cmdq_handle_ovl1to2_config);
#ifndef CONFIG_MTK_CLKMGR
	ddp_clk_unprepare(DISP_MTCMOS_CLK);
#endif
	dpmgr_destroy_path(pgc->ovl2mem_path_handle, pgc->cmdq_handle_ovl1to2_config);
	pgc->ovl2mem_path_handle = NULL;

	/*clear sof token for next dl to dc */
	cmdqRecClearEventToken(pgc->cmdq_handle_ovl1to2_config, CMDQ_EVENT_DISP_WDMA0_SOF);

	_cmdq_flush_config_handle_mira(pgc->cmdq_handle_ovl1to2_config, 1);
	cmdqRecReset(pgc->cmdq_handle_ovl1to2_config);

	/* release output buffer */
	layer = disp_sync_get_output_timeline_id();
	mtkfb_release_layer_fence(primary_session_id, layer);


	/* 4.modify interface path handle to new scenario(rdma->dsi) */
	_cmdq_reset_config_handle();
	if (!secure_path_on)
		cmdqRecEnablePrefetch(pgc->cmdq_handle_config);
	_cmdq_insert_wait_frame_done_token();

	dpmgr_modify_path(pgc->dpmgr_handle, DDP_SCENARIO_PRIMARY_DISP, pgc->cmdq_handle_config,
			  primary_display_is_video_mode() ? DDP_VIDEO_MODE : DDP_CMD_MODE);

	/* 5.config rdma from memory mode to directlink mode */
	data_config_dl = dpmgr_path_get_last_config(pgc->dpmgr_handle);
	data_config_dl->rdma_config = decouple_rdma_config;
	data_config_dl->rdma_config.address = 0;
	data_config_dl->rdma_config.pitch = 0;
	data_config_dl->rdma_config.security = DISP_NORMAL_BUFFER;
	data_config_dl->rdma_dirty = 1;
	memcpy(data_config_dl->ovl_config, ovl_config, sizeof(ovl_config));
	data_config_dl->ovl_dirty = 1;
	ret = dpmgr_path_config(pgc->dpmgr_handle, data_config_dl, pgc->cmdq_handle_config);

	/* use blocking flush to make sure all config is done, then stop/start trigger loop */

	cmdqRecBackupUpdateSlot(pgc->cmdq_handle_config, pgc->rdma_buff_info, 0, 0);

	/* cmdqRecDumpCommand(pgc->cmdq_handle_config); */
	_cmdq_set_config_handle_dirty();
	if (!secure_path_on)
		cmdqRecDisablePrefetch(pgc->cmdq_handle_config);

	_cmdq_flush_config_handle(1, NULL, 0);

	/* release output buffer */
	layer = disp_sync_get_output_interface_timeline_id();
	mtkfb_release_layer_fence(primary_session_id, layer);

	_cmdq_reset_config_handle();
	_cmdq_insert_wait_frame_done_token();

	/* 9.enable event for new path */
	if (primary_display_is_video_mode())
		dpmgr_map_event_to_irq(pgc->dpmgr_handle, DISP_PATH_EVENT_IF_VSYNC, DDP_IRQ_RDMA0_DONE);

	dpmgr_enable_event(pgc->dpmgr_handle, DISP_PATH_EVENT_IF_VSYNC);
	dpmgr_enable_event(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_DONE);
	dpmgr_enable_event(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_START);

	/* 1. enable SODI */
#if defined(MTK_FB_SODI_SUPPORT) && !defined(CONFIG_FPGA_EARLY_PORTING)
	spm_enable_sodi(1);
#endif
	return ret;
}

const char *session_mode_spy(unsigned int mode)
{
	switch (mode) {
	case DISP_SESSION_DIRECT_LINK_MODE:
		return "DIRECT_LINK";
	case DISP_SESSION_DIRECT_LINK_MIRROR_MODE:
		return "DIRECT_LINK_MIRROR";
	case DISP_SESSION_DECOUPLE_MODE:
		return "DECOUPLE";
	case DISP_SESSION_DECOUPLE_MIRROR_MODE:
		return "DECOUPLE_MIRROR";
	default:
		return "UNKNOWN";
	}
}

static int config_display_m4u_port(void)
{
	int ret = 0;
	M4U_PORT_STRUCT sPort;

	sPort.ePortID = M4U_PORT_DISP_OVL0;
	sPort.Virtuality = primary_display_use_m4u;
	sPort.Security = 0;
	sPort.Distance = 1;
	sPort.Direction = 0;
	ret = m4u_config_port(&sPort);
	if (ret == 0) {
		DISPMSG("config M4U Port %s to %s SUCCESS\n",
			  ddp_get_module_name(DISP_MODULE_OVL0),
			  primary_display_use_m4u ? "virtual" : "physical");
	} else {
		DISPMSG("config M4U Port %s to %s FAIL(ret=%d)\n",
			  ddp_get_module_name(DISP_MODULE_OVL0),
			  primary_display_use_m4u ? "virtual" : "physical", ret);
		return -1;
	}
#ifdef OVL_CASCADE_SUPPORT
	sPort.ePortID = M4U_PORT_DISP_OVL1;
	ret = m4u_config_port(&sPort);
	if (ret) {
		DISPMSG("config M4U Port %s to %s FAIL(ret=%d)\n",
			  ddp_get_module_name(DISP_MODULE_OVL1),
			  primary_display_use_m4u ? "virtual" : "physical", ret);
		return -1;
	}
#endif
	sPort.ePortID = M4U_PORT_DISP_RDMA0;
	ret = m4u_config_port(&sPort);
	if (ret) {
		DISPMSG("config M4U Port %s to %s FAIL(ret=%d)\n",
			  ddp_get_module_name(DISP_MODULE_RDMA0),
			  primary_display_use_m4u ? "virtual" : "physical", ret);
		return -1;
	}
	sPort.ePortID = M4U_PORT_DISP_WDMA0;
	ret = m4u_config_port(&sPort);
	if (ret) {
		DISPMSG("config M4U Port %s to %s FAIL(ret=%d)\n",
			  ddp_get_module_name(DISP_MODULE_WDMA0),
			  primary_display_use_m4u ? "virtual" : "physical", ret);
		return -1;
	}
	return ret;
}

static disp_internal_buffer_info *allocat_decouple_buffer(int size)
{
	void *buffer_va = NULL;
	unsigned int buffer_mva = 0;
	unsigned int mva_size = 0;

	struct ion_client *client = NULL;
	struct ion_handle *handle = NULL;
	disp_internal_buffer_info *buf_info = NULL;

	struct ion_mm_data mm_data;

	memset((void *)&mm_data, 0, sizeof(struct ion_mm_data));

	client = ion_client_create(g_ion_device, "disp_decouple");

	buf_info = kzalloc(sizeof(disp_internal_buffer_info), GFP_KERNEL);
	if (buf_info) {
		handle = ion_alloc(client, size, 0, ION_HEAP_MULTIMEDIA_MASK, 0);
		if (IS_ERR(handle)) {
			DISPERR("Fatal Error, ion_alloc for size %d failed\n", size);
			ion_free(client, handle);
			ion_client_destroy(client);
			kfree(buf_info);
			return NULL;
		}

		buffer_va = ion_map_kernel(client, handle);
		if (buffer_va == NULL) {
			DISPERR("ion_map_kernrl failed\n");
			ion_free(client, handle);
			ion_client_destroy(client);
			kfree(buf_info);
			return NULL;
		}
		mm_data.config_buffer_param.kernel_handle = handle;
		mm_data.mm_cmd = ION_MM_CONFIG_BUFFER;
		if (ion_kernel_ioctl(client, ION_CMD_MULTIMEDIA, (unsigned long)&mm_data) < 0) {
			DISPERR("ion_test_drv: Config buffer failed.\n");
			ion_free(client, handle);
			ion_client_destroy(client);
			kfree(buf_info);
			return NULL;
		}

		ion_phys(client, handle, (ion_phys_addr_t *) &buffer_mva, (size_t *) &mva_size);
		if (buffer_mva == 0) {
			DISPERR("Fatal Error, get mva failed\n");
			ion_free(client, handle);
			ion_client_destroy(client);
			kfree(buf_info);
			return NULL;
		}
		buf_info->handle = handle;
		buf_info->mva = buffer_mva;
		buf_info->size = mva_size;
		buf_info->va = buffer_va;
		buf_info->client = client;
	} else {
		DISPERR("Fatal error, kzalloc internal buffer info failed!!\n");
		kfree(buf_info);
		return NULL;
	}

	return buf_info;
}

static int init_decouple_buffers(void)
{
	int i = 0;
	int height = primary_display_get_height();
	int width = primary_display_get_width();
	int bpp = primary_display_get_dc_bpp();

	int buffer_size = width * height * bpp / 8;

	for (i = 0; i < DISP_INTERNAL_BUFFER_COUNT; i++) {
		decouple_buffer_info[i] = allocat_decouple_buffer(buffer_size);
		if (decouple_buffer_info[i] != NULL) {
			pgc->dc_buf[i] = decouple_buffer_info[i]->mva;
			dc_vAddr[i] = (unsigned long)decouple_buffer_info[i]->va;

		}
	}

	/*initialize rdma config */
	decouple_rdma_config.height = height;
	decouple_rdma_config.width = width;
	decouple_rdma_config.idx = 0;
	decouple_rdma_config.inputFormat = eRGB888;
	decouple_rdma_config.pitch = width * DP_COLOR_BITS_PER_PIXEL(eRGB888) / 8;
	decouple_rdma_config.security = DISP_NORMAL_BUFFER;

	/*initialize wdma config */
	decouple_wdma_config.srcHeight = height;
	decouple_wdma_config.srcWidth = width;
	decouple_wdma_config.clipX = 0;
	decouple_wdma_config.clipY = 0;
	decouple_wdma_config.clipHeight = height;
	decouple_wdma_config.clipWidth = width;
	decouple_wdma_config.outputFormat = eRGB888;
	decouple_wdma_config.useSpecifiedAlpha = 1;
	decouple_wdma_config.alpha = 0xFF;
	decouple_wdma_config.dstPitch = width * DP_COLOR_BITS_PER_PIXEL(eRGB888) / 8;
	decouple_wdma_config.security = DISP_NORMAL_BUFFER;

	return 0;
}

#if defined(CONFIG_MTK_GMO_RAM_OPTIMIZE)


static int allocate_idle_lp_dc_buffer(void)
{
	int height = primary_display_get_height();
	int width = primary_display_get_width();
	int bpp = primary_display_get_dc_bpp();
	int buffer_size =  width * height * bpp / 8;

	decouple_buffer_info[0] = allocat_decouple_buffer(buffer_size);
	if (decouple_buffer_info[0] != NULL) {
		pgc->dc_buf[0] = decouple_buffer_info[0]->mva;
		dc_vAddr[0] = (unsigned long)decouple_buffer_info[0]->va;
	} else {
		return -1;
	}

	/*initialize rdma config*/
	decouple_rdma_config.height = height;
	decouple_rdma_config.width = width;
	decouple_rdma_config.idx = 0;
	decouple_rdma_config.inputFormat = eRGB888;
	decouple_rdma_config.pitch = width * DP_COLOR_BITS_PER_PIXEL(eRGB888) / 8;
	decouple_rdma_config.security = DISP_NORMAL_BUFFER;

	/*initialize wdma config*/
	decouple_wdma_config.srcHeight = height;
	decouple_wdma_config.srcWidth = width;
	decouple_wdma_config.clipX = 0;
	decouple_wdma_config.clipY = 0;
	decouple_wdma_config.clipHeight = height;
	decouple_wdma_config.clipWidth = width;
	decouple_wdma_config.outputFormat = eRGB888;
	decouple_wdma_config.useSpecifiedAlpha = 1;
	decouple_wdma_config.alpha = 0xFF;
	decouple_wdma_config.dstPitch = width * DP_COLOR_BITS_PER_PIXEL(eRGB888) / 8;
	decouple_wdma_config.security = DISP_NORMAL_BUFFER;

	return 0;
}

static int release_idle_lp_dc_buffer(unsigned int need_primary_lock)
{
	if (need_primary_lock)
		_primary_path_lock(__func__);

	if (decouple_buffer_info[0]) {
		ion_free(decouple_buffer_info[0]->client, decouple_buffer_info[0]->handle);
		ion_client_destroy(decouple_buffer_info[0]->client);
		kfree(decouple_buffer_info[0]);
		decouple_buffer_info[0] = NULL;
	}

	if (need_primary_lock)
		_primary_path_unlock(__func__);
	return 0;
}
#endif

static int __build_path_direct_link(void)
{
	int ret = 0;

	DISP_MODULE_ENUM dst_module = 0;
/*	DISPFUNC();*/
	pr_warn("[DISP]%s\n", __func__);
	pgc->mode = DIRECT_LINK_MODE;

	pgc->dpmgr_handle = dpmgr_create_path(DDP_SCENARIO_PRIMARY_DISP, pgc->cmdq_handle_config);
	if (pgc->dpmgr_handle) {
		pr_warn("[DISP]dpmgr create path SUCCESS(0x%p)\n", pgc->dpmgr_handle);
	} else {
		DISPERR("dpmgr create path FAIL\n");
		return -1;
	}

	dst_module = _get_dst_module_by_lcm(pgc->plcm);
	dpmgr_path_set_dst_module(pgc->dpmgr_handle, dst_module);
	DISPMSG("dpmgr set dst module FINISHED(%s)\n", ddp_get_module_name(dst_module));
#ifndef MTKFB_NO_M4U
	config_display_m4u_port();
#if !defined(CONFIG_MTK_GMO_RAM_OPTIMIZE)
	init_decouple_buffers();
#endif
#endif
	dpmgr_set_lcm_utils(pgc->dpmgr_handle, pgc->plcm->drv);

	dpmgr_enable_event(pgc->dpmgr_handle, DISP_PATH_EVENT_IF_VSYNC);
	dpmgr_enable_event(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_DONE);
	dpmgr_enable_event(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_START);

	/* set video mode must before path_init */
	dpmgr_path_set_video_mode(pgc->dpmgr_handle, primary_display_is_video_mode());
#ifndef CONFIG_MTK_CLKMGR
	ddp_clk_prepare(DISP_MTCMOS_CLK);
#endif
#ifndef MTK_FB_CMDQ_DISABLE
	dpmgr_path_init(pgc->dpmgr_handle, CMDQ_ENABLE);
/* cmdqRecFlush(pgc->cmdq_handle_config); */
#else
	dpmgr_path_init(pgc->dpmgr_handle, CMDQ_DISABLE);
#endif

	return ret;
}

static void copy_lklogo_to_dc_buf(void)
{
	unsigned int line, k, src_argb8888;
	unsigned int *s;
	unsigned char *d;

	for (line = 0; line < primary_display_get_height(); line++) {
		d = (char *)(dc_vAddr[0] + line * primary_display_get_width() * DP_COLOR_BITS_PER_PIXEL(eRGB888) / 8);
		s = (int *)(pgc->framebuffer_va + line * ALIGN_TO(primary_display_get_width(),
								  MTK_FB_ALIGNMENT) * primary_display_get_dc_bpp() / 8);
		for (k = 0; k < primary_display_get_width(); k++) {
			src_argb8888 = *s++;
			*d++ = ((src_argb8888 & 0xFF));
			*d++ = ((src_argb8888 & 0xFF00) >> 8);
			*d++ = ((src_argb8888 & 0xFF0000) >> 16);
		}
	}
}

static int _build_path_rdma_to_dsi(void)
{
	DISP_MODULE_ENUM dst_module = 0;
	uint32_t writing_mva = 0;

#ifdef CONFIG_FOR_SOURCE_PQ
	pgc->dpmgr_handle =
	    dpmgr_create_path(DDP_SCENARIO_PRIMARY_RDMA0_DISP, pgc->cmdq_handle_config);
#else
	pgc->dpmgr_handle =
	    dpmgr_create_path(DDP_SCENARIO_PRIMARY_RDMA0_COLOR0_DISP, pgc->cmdq_handle_config);
#endif

	if (pgc->dpmgr_handle) {
		DISPMSG("dpmgr create interface path SUCCESS\n");
	} else {
		DISPMSG("dpmgr create path FAIL\n");
		return -1;
	}

	/* VSYNC event will be provided to hwc for system vsync hw source
	 * FRAME_DONE will be used in esd/suspend/resume for path status check
	 */

	dpmgr_enable_event(pgc->dpmgr_handle, DISP_PATH_EVENT_IF_VSYNC);
	dpmgr_enable_event(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_DONE);
	dpmgr_enable_event(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_START);

	dpmgr_set_lcm_utils(pgc->dpmgr_handle, pgc->plcm->drv);
	dst_module = _get_dst_module_by_lcm(pgc->plcm);
	dpmgr_path_set_dst_module(pgc->dpmgr_handle, dst_module);

	dpmgr_path_set_video_mode(pgc->dpmgr_handle, primary_display_is_video_mode());

	if (primary_display_is_video_mode())
		_cmdq_insert_wait_frame_done_token();
#ifndef CONFIG_MTK_CLKMGR
	ddp_clk_prepare(DISP_MTCMOS_CLK);
#endif
	dpmgr_path_init(pgc->dpmgr_handle, CMDQ_ENABLE);

	writing_mva = pgc->dc_buf[pgc->dc_buf_id];
	DISPMSG("writing_mva = 0x%08x\n", writing_mva);
	pgc->dc_buf_id++;
	pgc->dc_buf_id %= DISP_INTERNAL_BUFFER_COUNT;

	/*initialize rdma config */
	decouple_rdma_config.address = (unsigned int)writing_mva;
	decouple_rdma_config.width = primary_display_get_width();
	decouple_rdma_config.height = primary_display_get_height();
	decouple_rdma_config.idx = 0;
	decouple_rdma_config.inputFormat = eRGB888;
	decouple_rdma_config.pitch = primary_display_get_width() * DP_COLOR_BITS_PER_PIXEL(eRGB888) / 8;

	_config_rdma_input_data(&decouple_rdma_config, pgc->dpmgr_handle, pgc->cmdq_handle_config);
	cmdqRecFlush(pgc->cmdq_handle_config);
	DISPMSG("dpmgr set dst module FINISHED(%s)\n", ddp_get_module_name(dst_module));

	return 0;
}

static int _build_path_ovl_to_wdma(void)
{
	int ret = 0;
	uint32_t writing_mva = 0;

#ifdef CONFIG_FOR_SOURCE_PQ
	pgc->ovl2mem_path_handle = dpmgr_create_path(DDP_SCENARIO_PRIMARY_DITHER_MEMOUT,
						     pgc->cmdq_handle_ovl1to2_config);
#else
	pgc->ovl2mem_path_handle = dpmgr_create_path(DDP_SCENARIO_PRIMARY_OVL_MEMOUT,
						     pgc->cmdq_handle_ovl1to2_config);
#endif

	/*
	 * FRAME_START will be used in decouple-mirror mode, for post-path fence release(rdma->dsi)
	 */
	dpmgr_enable_event(pgc->ovl2mem_path_handle, DISP_PATH_EVENT_FRAME_COMPLETE);
	dpmgr_enable_event(pgc->ovl2mem_path_handle, DISP_PATH_EVENT_FRAME_START);

	if (pgc->ovl2mem_path_handle) {
		DISPMSG("dpmgr create ovl memout path SUCCESS\n");
	} else {
		DISPMSG("dpmgr create path FAIL\n");
		return -1;
	}
	dpmgr_path_set_video_mode(pgc->ovl2mem_path_handle, 0);
	dpmgr_path_init(pgc->ovl2mem_path_handle, CMDQ_DISABLE);

	writing_mva = pgc->dc_buf[pgc->dc_buf_id];
	/*initialize wdma config */
	decouple_wdma_config.dstAddress = writing_mva;
	decouple_wdma_config.srcHeight = primary_display_get_height();
	decouple_wdma_config.srcWidth = primary_display_get_width();
	decouple_wdma_config.clipX = 0;
	decouple_wdma_config.clipY = 0;
	decouple_wdma_config.clipHeight = primary_display_get_height();
	decouple_wdma_config.clipWidth = primary_display_get_width();
	decouple_wdma_config.outputFormat = eRGB888;
	decouple_wdma_config.useSpecifiedAlpha = 1;
	decouple_wdma_config.alpha = 0xFF;
	decouple_wdma_config.dstPitch = primary_display_get_width() * DP_COLOR_BITS_PER_PIXEL(eRGB888) / 8;

	dpmgr_path_reset(pgc->ovl2mem_path_handle, CMDQ_DISABLE);
	_config_wdma_output(&decouple_wdma_config, pgc->ovl2mem_path_handle, NULL);
	ret = dpmgr_path_start(pgc->ovl2mem_path_handle, CMDQ_DISABLE);

	DISPMSG("build rdma to dsi path finished\n");
	return ret;
}

/* #define CONFIG_USE_CMDQ */
static int __build_path_decouple(void)
{
	int ret = 0;

	/* DISPFUNC(); */
	pr_warn("[DISP]%s\n", __func__);
	pgc->mode = DECOUPLE_MODE;

	/* 1. Allocate decouple buffers and copy the LK logo to dc buffer. */
	init_decouple_buffers();
	copy_lklogo_to_dc_buf();
	mutex_init(&(pgc->dc_lock));

	/*
	 * 2. Enable m4u of RDMA0
	 * The OVL engine would be used in VDO mode.
	 * Before turn on all the m4u of display modules,
	 * the data path need to change to rdma0->dsi.
	 */
	{
		M4U_PORT_STRUCT sPort;

		sPort.ePortID = M4U_PORT_DISP_RDMA0;
		sPort.Virtuality = primary_display_use_m4u;
		sPort.Security = 0;
		sPort.Distance = 1;
		sPort.Direction = 0;
		ret = m4u_config_port(&sPort);
		if (ret == 0) {
			DISPMSG("config M4U Port %s to %s SUCCESS\n",
				  ddp_get_module_name(DISP_MODULE_OVL0),
				  primary_display_use_m4u ? "virtual" : "physical");
		} else {
			DISPMSG("config M4U Port %s to %s FAIL(ret=%d)\n",
				  ddp_get_module_name(DISP_MODULE_OVL0),
				  primary_display_use_m4u ? "virtual" : "physical", ret);
			return -1;
		}
	}

	/* 3. Build and switch display data path from ovl->dsi to rdma->dsi. */
	ret = _build_path_rdma_to_dsi();
	if (ret) {
		pr_debug("build path rdma to dsi fail\n");
		return ret;
	}

	/*
	 * 4. Enable all display engine m4u.
	 * Since the display data path has been changed to rdma->dsi,
	 * all the display engine m4u can be enabled safely.
	 */
	config_display_m4u_port();

	/* 5. Build/Connect ovl->wdma display data path. */
	ret = _build_path_ovl_to_wdma();
	if (ret) {
		pr_debug("build path ovl to wdma fail\n");
		return ret;
	}

	DISPMSG("build decouple path finished\n");
	return ret;
}

static int __build_path_single_layer(void)
{
	return 0;		/* avoid build warning. */
}

static int __build_path_debug_rdma1_dsi0(void)
{
	int ret = 0;

#if defined(MTK_FB_RDMA1_SUPPORT)
	DISP_MODULE_ENUM dst_module = 0;

	pgc->mode = DEBUG_RDMA1_DSI0_MODE;

	pgc->dpmgr_handle = dpmgr_create_path(DDP_SCENARIO_SUB_RDMA1_DISP, pgc->cmdq_handle_config);
	if (pgc->dpmgr_handle) {
		DISPMSG("dpmgr create path SUCCESS(0x%p)\n", pgc->dpmgr_handle);
	} else {
		DISPMSG("dpmgr create path FAIL\n");
		return -1;
	}

	dst_module = _get_dst_module_by_lcm(pgc->plcm);
	dpmgr_path_set_dst_module(pgc->dpmgr_handle, dst_module);
	DISPMSG("dpmgr set dst module FINISHED(%s)\n", ddp_get_module_name(dst_module));
#ifndef MTKFB_NO_M4U
	{
#ifdef MTK_FB_RDMA1_SUPPORT
		M4U_PORT_STRUCT sPort;

		sPort.ePortID = M4U_PORT_DISP_RDMA1;
		sPort.Virtuality = primary_display_use_m4u;
		sPort.Security = 0;
		sPort.Distance = 1;
		sPort.Direction = 0;
		ret = m4u_config_port(&sPort);
		if (ret == 0) {
			DISPMSG("config M4U Port %s to %s SUCCESS\n",
				  ddp_get_module_name(DISP_MODULE_RDMA1),
				  primary_display_use_m4u ? "virtual" : "physical");
		} else {
			DISPMSG("config M4U Port %s to %s FAIL(ret=%d)\n",
				  ddp_get_module_name(DISP_MODULE_RDMA1),
				  primary_display_use_m4u ? "virtual" : "physical", ret);
			return -1;
		}
#endif
	}
#endif
	dpmgr_set_lcm_utils(pgc->dpmgr_handle, pgc->plcm->drv);

	dpmgr_enable_event(pgc->dpmgr_handle, DISP_PATH_EVENT_IF_VSYNC);
	dpmgr_enable_event(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_DONE);
#endif

	return ret;
}


int disp_fmt_to_hw_fmt(DISP_FORMAT src_fmt, unsigned int *hw_fmt,
		       unsigned int *Bpp, unsigned int *bpp)
{
	switch (src_fmt) {
	case DISP_FORMAT_YUV422:
		*hw_fmt = eYUY2;
		*Bpp = 2;
		*bpp = 16;
		break;

	case DISP_FORMAT_RGB565:
		*hw_fmt = eRGB565;
		*Bpp = 2;
		*bpp = 16;
		break;

	case DISP_FORMAT_RGB888:
		*hw_fmt = eRGB888;
		*Bpp = 3;
		*bpp = 24;
		break;

	case DISP_FORMAT_BGR888:
		*hw_fmt = eBGR888;
		*Bpp = 3;
		*bpp = 24;
		break;

	case DISP_FORMAT_ARGB8888:
		*hw_fmt = eARGB8888;
		*Bpp = 4;
		*bpp = 32;
		break;

	case DISP_FORMAT_ABGR8888:
		*hw_fmt = eABGR8888;
		*Bpp = 4;
		*bpp = 32;
		break;

	case DISP_FORMAT_RGBA8888:
		*hw_fmt = eRGBA8888;
		*Bpp = 4;
		*bpp = 32;
		break;

	case DISP_FORMAT_BGRA8888:
		/* *hw_fmt = eABGR8888; */
		*hw_fmt = eBGRA8888;
		*Bpp = 4;
		*bpp = 32;
		break;

	case DISP_FORMAT_XRGB8888:
		*hw_fmt = eARGB8888;
		*Bpp = 4;
		*bpp = 32;
		break;

	case DISP_FORMAT_XBGR8888:
		*hw_fmt = eABGR8888;
		*Bpp = 4;
		*bpp = 32;
		break;

	case DISP_FORMAT_RGBX8888:
		*hw_fmt = eRGBA8888;
		*Bpp = 4;
		*bpp = 32;
		break;

	case DISP_FORMAT_BGRX8888:
		*hw_fmt = eBGRA8888;
		*Bpp = 4;
		*bpp = 32;
		break;

	case DISP_FORMAT_UYVY:
		*hw_fmt = eUYVY;
		*Bpp = 2;
		*bpp = 16;
		break;

	case DISP_FORMAT_YV12:
		*hw_fmt = eYV12;
		*Bpp = 1;
		*bpp = 8;
		break;
	default:
		DISPERR("Invalid color format: 0x%x\n", src_fmt);
		return -1;
	}

	return 0;
}

static int _convert_disp_input_to_ovl(OVL_CONFIG_STRUCT *dst, disp_input_config *src)
{
	int ret;
	unsigned int Bpp = 0;
	unsigned int bpp = 0;

	if (!src || !dst) {
		DISPAEE("%s src(0x%p) or dst(0x%p) is null\n",
			       __func__, src, dst);
		return -1;
	}

	dst->layer = src->layer_id;
	dst->isDirty = 1;
	dst->buff_idx = src->next_buff_idx;
	dst->layer_en = src->layer_enable;

	/* if layer is disable, we just needs config above params. */
	if (!src->layer_enable)
		return 0;

	ret = disp_fmt_to_hw_fmt(src->src_fmt, (unsigned int *)(&(dst->fmt)), (unsigned int *)(&Bpp),
				 (unsigned int *)(&bpp));

	dst->addr = (unsigned long)src->src_phy_addr;
	dst->vaddr = (unsigned long)src->src_base_addr;
	dst->src_x = src->src_offset_x;
	dst->src_y = src->src_offset_y;
	dst->src_w = src->src_width;
	dst->src_h = src->src_height;
	dst->src_pitch = src->src_pitch * Bpp;
	dst->dst_x = src->tgt_offset_x;
	dst->dst_y = src->tgt_offset_y;

	/* dst W/H should <= src W/H */
	if (src->buffer_source != DISP_BUFFER_ALPHA) { /* dim layer do not care for src_width */
		dst->dst_w = min(src->src_width, src->tgt_width);
		dst->dst_h = min(src->src_height, src->tgt_height);
	} else {
		dst->dst_w = src->tgt_width;
		dst->dst_h = src->tgt_height;
	}

	dst->keyEn = src->src_use_color_key;
	dst->key = src->src_color_key;

	dst->aen = src->alpha_enable;
	dst->alpha = src->alpha;
	dst->sur_aen = src->sur_aen;
	dst->src_alpha = src->src_alpha;
	dst->dst_alpha = src->dst_alpha;

#ifdef DISP_DISABLE_X_CHANNEL_ALPHA
	if (DISP_FORMAT_ARGB8888 == src->src_fmt || DISP_FORMAT_ABGR8888 == src->src_fmt ||
	    DISP_FORMAT_RGBA8888 == src->src_fmt || DISP_FORMAT_BGRA8888 == src->src_fmt ||
	    src->buffer_source == DISP_BUFFER_ALPHA) {
		;/* nothing */
	} else {
		dst->aen = FALSE;
		dst->sur_aen = FALSE;
	}
#endif

	dst->identity = src->identity;
	dst->connected_type = src->connected_type;
	dst->security = src->security;
	dst->yuv_range = src->yuv_range;

	if (src->buffer_source == DISP_BUFFER_ALPHA) {
		dst->source = OVL_LAYER_SOURCE_RESERVED; /* dim layer, constant alpha */
	} else if (src->buffer_source == DISP_BUFFER_ION || src->buffer_source == DISP_BUFFER_MVA) {
		dst->source = OVL_LAYER_SOURCE_MEM; /* from memory */
	} else {
		DISPERR("unknown source = %d", src->buffer_source);
		dst->source = OVL_LAYER_SOURCE_MEM;
	}

	return ret;
}

static int _convert_disp_input_to_rdma(RDMA_CONFIG_STRUCT *dst, disp_input_config *src)
{
	int ret;
	unsigned int Bpp = 0;
	unsigned int bpp = 0;

	if (!src || !dst) {
		DISPAEE("%s src(0x%p) or dst(0x%p) is null\n",
			       __func__, src, dst);
		return -1;
	}

	ret = disp_fmt_to_hw_fmt(src->src_fmt, &(dst->inputFormat), &Bpp, &bpp);
	dst->address = (unsigned long)src->src_phy_addr;
	dst->width = src->src_width;
	dst->height = src->src_height;
	dst->pitch = src->src_pitch * Bpp;

	return ret;
}

/* Video mode SODI CMDQ flow */
/* extern unsigned int gDumpConfigCMD; */
/* extern unsigned int gEnableOVLStatusCheck; */
#define DISP_REG_CMDQ_TOKEN_ID    0x10212060
#define DISP_REG_CMDQ_TOKEN_VALUE 0x10212064
#define DISP_MUTEX0_STREAM_EOF_ID 55

int _trigger_display_interface(int blocking, void *callback, unsigned int userdata)
{
	static unsigned int cnt;

	/* 4. enable SODI after config */
	if (primary_display_is_video_mode() == 1)
		disp_set_sodi(1, pgc->cmdq_handle_config);

#ifdef DISP_ENABLE_SODI_FOR_VIDEO_MODE
	if (gPrefetchControl == 1 && cnt >= 20)
		cmdqRecDisablePrefetch(pgc->cmdq_handle_config);
#endif
	if (_should_wait_path_idle())
		dpmgr_wait_event_timeout(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_DONE, HZ * 1);

	if (_should_update_lcm())
		disp_lcm_update(pgc->plcm, 0, 0, pgc->plcm->params->width,
				pgc->plcm->params->height, 0);

	if (_should_start_path())
		dpmgr_path_start(pgc->dpmgr_handle, primary_display_cmdq_enabled());

	if (_should_trigger_path())
		/* trigger_loop_handle is used only for build trigger loop,
		 * which should always be NULL for config thread
		 */
		dpmgr_path_trigger(pgc->dpmgr_handle, NULL, primary_display_cmdq_enabled());

#ifndef MTK_FB_CMDQ_DISABLE
	if (_should_set_cmdq_dirty()) {
		_cmdq_set_config_handle_dirty();

		/* disable SODI after set dirty */
		if (primary_display_is_video_mode() == 0 && gDisableSODIForTriggerLoop == 1)
			disp_set_sodi(0, pgc->cmdq_handle_config);
	}
	/* 1. disable SODI by CPU before flush CMDQ by CPU */
	if (primary_display_is_video_mode() == 1)
		disp_set_sodi(0, 0);

	if (gDumpConfigCMD == 1) {
		DISPMSG("primary_display_config, dump before flush:\n");
		cmdqRecDumpCommand(pgc->cmdq_handle_config);
	}
	/* insert update ovl status slot command */
	if (primary_display_is_video_mode() == 1) {
		cmdqRecBackupRegisterToSlot(pgc->cmdq_handle_config, pgc->ovl_status_info,
			0, DISP_REG_OVL0_STATE_PA);
		cmdqRecBackupRegisterToSlot(pgc->cmdq_handle_config, pgc->ovl_status_info,
			1, DISP_REG_OVL0_STATUS_PA);
	}

	if (_should_flush_cmdq_config_handle())
		_cmdq_flush_config_handle(blocking, callback, userdata);

	if (_should_reset_cmdq_config_handle()) {
		_cmdq_reset_config_handle();
#ifdef DISP_ENABLE_SODI_FOR_VIDEO_MODE
		/* do not have to enable/disable prefetch at power on stage. */
		if (gPrefetchControl == 1 && cnt >= 20)
			cmdqRecEnablePrefetch(pgc->cmdq_handle_config);
#endif
	}
	/* TODO: _is_decouple_mode() shuold be protected by mutex!!!!!!!!when dynamic switch decouple/directlink */
	if (_should_insert_wait_frame_done_token() && (!_is_decouple_mode(pgc->session_mode))) {
		/* 2. enable SODI by CMDQ before wait */
		if (primary_display_is_video_mode() == 1)
			disp_set_sodi(1, pgc->cmdq_handle_config);

		if (primary_display_is_video_mode() == 1)
			cmdqRecWaitNoClear(pgc->cmdq_handle_config, CMDQ_EVENT_DISP_RDMA0_EOF);
		else
			_cmdq_insert_wait_frame_done_token();

		/* 3. disable AODI by CMDQ before config */
		if (primary_display_is_video_mode() == 1)
			disp_set_sodi(0, pgc->cmdq_handle_config);
	}

	if (cnt < 20)
		cnt++;
	else
		gEnableLowPowerFeature = 1;
#endif

	return 0;
}

int _trigger_ovl_to_memory(disp_path_handle disp_handle, cmdqRecHandle cmdq_handle,
			   fence_release_callback callback, unsigned int data, int blocking)
{
	unsigned int rdma_pitch_sec;

	dpmgr_wdma_path_force_power_on();
	dpmgr_path_trigger(disp_handle, cmdq_handle, CMDQ_ENABLE);
	cmdqRecWaitNoClear(cmdq_handle, CMDQ_EVENT_DISP_WDMA0_EOF);

	cmdqRecBackupUpdateSlot(cmdq_handle, pgc->rdma_buff_info, 0, mem_config.addr);
	/* rdma pitch only use bit[15..0], we use bit[31:30] to store secure information */
	rdma_pitch_sec = mem_config.pitch | (mem_config.security << 30);
	cmdqRecBackupUpdateSlot(cmdq_handle, pgc->rdma_buff_info, 1, rdma_pitch_sec);
	cmdqRecBackupUpdateSlot(cmdq_handle, pgc->rdma_buff_info, 2, mem_config.fmt);

	if (blocking)
		cmdqRecFlush(cmdq_handle);
	else
		cmdqRecFlushAsyncCallback(cmdq_handle, (CmdqAsyncFlushCB) callback, data);
	cmdqRecReset(cmdq_handle);
	cmdqRecWait(cmdq_handle, CMDQ_EVENT_DISP_WDMA0_EOF);
	MMProfileLogEx(ddp_mmp_get_events()->ovl_trigger, MMProfileFlagPulse, 0, data);
	return 0;
}

int _trigger_ovl_to_memory_mirror(disp_path_handle disp_handle, cmdqRecHandle cmdq_handle,
				  fence_release_callback callback, unsigned int data)
{
	int layer = 0;
	unsigned int rdma_pitch_sec;

	dpmgr_wdma_path_force_power_on();
	dpmgr_path_trigger(disp_handle, cmdq_handle, CMDQ_ENABLE);

	cmdqRecWaitNoClear(cmdq_handle, CMDQ_EVENT_DISP_WDMA0_EOF);

	layer = disp_sync_get_output_timeline_id();
	cmdqRecBackupUpdateSlot(cmdq_handle, pgc->cur_config_fence, layer, mem_config.buff_idx);

	layer = disp_sync_get_output_interface_timeline_id();
	cmdqRecBackupUpdateSlot(cmdq_handle, pgc->cur_config_fence, layer, mem_config.interface_idx);

	cmdqRecBackupUpdateSlot(cmdq_handle, pgc->rdma_buff_info, 0, (unsigned int)mem_config.addr);

	/*rdma pitch only use bit[15..0], we use bit[31:30] to store secure information */
	rdma_pitch_sec = mem_config.pitch | (mem_config.security << 30);
	cmdqRecBackupUpdateSlot(cmdq_handle, pgc->rdma_buff_info, 1, rdma_pitch_sec);

	cmdqRecBackupUpdateSlot(cmdq_handle, pgc->rdma_buff_info, 2, mem_config.fmt);

	cmdqRecFlushAsyncCallback(cmdq_handle, (CmdqAsyncFlushCB) callback, data);
	cmdqRecReset(cmdq_handle);
	cmdqRecWait(cmdq_handle, CMDQ_EVENT_DISP_WDMA0_EOF);
	MMProfileLogEx(ddp_mmp_get_events()->ovl_trigger, MMProfileFlagPulse, 0, data);

	return 0;
}

#define EEEEEEEEEEEEEEE
/******************************************************************************/
/* ESD CHECK / RECOVERY ---- BEGIN                                            */
/******************************************************************************/
#ifdef MTK_FB_ESD_ENABLE
static struct task_struct *primary_display_esd_check_task;
#define IS_ESD_ENABLE true
#else
#define IS_ESD_ENABLE false
#endif

#ifdef MTK_FB_PULLCLK_ENABLE
static struct task_struct *primary_display_pullclk_task;
#endif


static wait_queue_head_t esd_check_task_wq; /* For Esd Check Task */
static atomic_t esd_check_task_wakeup = ATOMIC_INIT(0);	/* For Esd Check Task */
static wait_queue_head_t esd_ext_te_wq;	/* For Vdo Mode EXT TE Check */
static atomic_t esd_ext_te_event = ATOMIC_INIT(0); /* For Vdo Mode EXT TE Check */
static atomic_t esd_check_bycmdq = ATOMIC_INIT(0);

static inline bool _is_enable_esd_check(void)
{
	return IS_ESD_ENABLE;
}

struct task_struct *primary_display_frame_update_task = NULL;
wait_queue_head_t primary_display_frame_update_wq;
atomic_t primary_display_frame_update_event = ATOMIC_INIT(0);

struct task_struct *decouple_fence_release_task = NULL;
wait_queue_head_t decouple_fence_release_wq;
atomic_t decouple_fence_release_event = ATOMIC_INIT(0);

static int eint_flag;	/* For DCT Setting */

unsigned int _need_do_esd_check(void)
{
	int ret = 0;
#ifdef CONFIG_OF
	if ((pgc->plcm->params->dsi.esd_check_enable == 1) && (islcmconnected == 1))
		ret = 1;
#else
	if (pgc->plcm->params->dsi.esd_check_enable == 1)
		ret = 1;
#endif
	return ret;
}


unsigned int _need_register_eint(void)
{
	int ret = 1;

	/* 1.need do esd check */
	/* 2.dsi vdo mode */
	/* 3.customization_esd_check_enable = 0 */
	if (_need_do_esd_check() == 0)
		ret = 0;
	else if (primary_display_is_video_mode() == 0)
		ret = 0;
	else if (pgc->plcm->params->dsi.customization_esd_check_enable == 1)
		ret = 0;

	return ret;
}

unsigned int _need_wait_esd_eof(void)
{
	int ret = 1;

	/* 1.need do esd check */
	/* 2.customization_esd_check_enable = 1 */
	/* 3.dsi cmd mode */
	if (_need_do_esd_check() == 0)
		ret = 0;
	else if (pgc->plcm->params->dsi.customization_esd_check_enable == 0)
		ret = 0;
	else if (primary_display_is_video_mode())
		ret = 0;

	return ret;
}

/* For Cmd Mode Read LCM Check */
/* Config cmdq_handle_config_esd */
int _esd_check_config_handle_cmd(void)
{
	int ret = 0;		/* 0:success */

	/* 1.reset */
	cmdqRecReset(pgc->cmdq_handle_config_esd);

	/* 2.write first instruction */
	/* cmd mode: wait CMDQ_SYNC_TOKEN_STREAM_EOF(wait trigger thread done) */
	cmdqRecWaitNoClear(pgc->cmdq_handle_config_esd, CMDQ_SYNC_TOKEN_STREAM_EOF);

	/* 3.clear CMDQ_SYNC_TOKEN_ESD_EOF(trigger thread need wait this sync token) */
	cmdqRecClearEventToken(pgc->cmdq_handle_config_esd, CMDQ_SYNC_TOKEN_ESD_EOF);

	/* 4.write instruction(read from lcm) */
	dpmgr_path_build_cmdq(pgc->dpmgr_handle, pgc->cmdq_handle_config_esd, CMDQ_ESD_CHECK_READ);

	/* 5.set CMDQ_SYNC_TOKE_ESD_EOF(trigger thread can work now) */
	cmdqRecSetEventToken(pgc->cmdq_handle_config_esd, CMDQ_SYNC_TOKEN_ESD_EOF);

	/* 6.flush instruction */
	dprec_logger_start(DPREC_LOGGER_ESD_CMDQ, 0, 0);
	ret = cmdqRecFlush(pgc->cmdq_handle_config_esd);
	dprec_logger_done(DPREC_LOGGER_ESD_CMDQ, 0, 0);

	DISPMSG("[ESD]_esd_check_config_handle_cmd ret=%d\n", ret);

	if (ret)
		ret = 1;
	return ret;
}

void primary_display_esd_cust_bycmdq(int enable)
{
	atomic_set(&esd_check_bycmdq, enable);
}

int primary_display_esd_cust_get(void)
{
	return atomic_read(&esd_check_bycmdq);
}

/* For Vdo Mode Read LCM Check */
/* Config cmdq_handle_config_esd */
/* extern unsigned int gESDEnableSODI; */
/* extern unsigned int gDumpESDCMD; */
int _esd_check_config_handle_vdo(void)
{
	int ret = 0;		/* 0:success , 1:fail */

	primary_display_esd_cust_bycmdq(1);

	_primary_path_lock(__func__);

#if defined(MTK_FB_SODI_SUPPORT) && !defined(CONFIG_FPGA_EARLY_PORTING)
	spm_enable_sodi(0);
#endif

	/* 1.reset */
	cmdqRecReset(pgc->cmdq_handle_config_esd);

	/* wait stream eof first */
	cmdqRecWait(pgc->cmdq_handle_config_esd, CMDQ_EVENT_DISP_RDMA0_EOF);
	cmdqRecWait(pgc->cmdq_handle_config_esd, CMDQ_EVENT_MUTEX0_STREAM_EOF);

#ifdef DISP_DUMP_EVENT_STATUS
	DISP_REG_SET_PA(pgc->cmdq_handle_config_esd, DISP_REG_CMDQ_TOKEN_ID, DISP_MUTEX0_STREAM_EOF_ID);
	cmdqRecBackupRegisterToSlot(pgc->cmdq_handle_config_esd, pgc->event_status, 1, DISP_REG_CMDQ_TOKEN_VALUE);
#endif


	/* 2.stop dsi vdo mode */
	dpmgr_path_build_cmdq(pgc->dpmgr_handle, pgc->cmdq_handle_config_esd, CMDQ_STOP_VDO_MODE);

	/* 3.write instruction(read from lcm) */
	dpmgr_path_build_cmdq(pgc->dpmgr_handle, pgc->cmdq_handle_config_esd, CMDQ_ESD_CHECK_READ);

	/* pull DSI clock lane */
	DSI_sw_clk_trail_cmdq(0, pgc->cmdq_handle_config_esd);
	DSI_manual_enter_HS(pgc->cmdq_handle_config_esd);


	/* start dsi vdo mode */
	dpmgr_path_build_cmdq(pgc->dpmgr_handle, pgc->cmdq_handle_config_esd, CMDQ_START_VDO_MODE);

	/* 5. trigger path */
	cmdqRecClearEventToken(pgc->cmdq_handle_config_esd, CMDQ_EVENT_MUTEX0_STREAM_EOF);

	if (gEnableSWTrigger == 1)
		DISP_REG_SET(pgc->cmdq_handle_config_esd, DISP_REG_CONFIG_MUTEX_EN(DISP_OVL_SEPARATE_MUTEX_ID), 1);

	dpmgr_path_trigger(pgc->dpmgr_handle, pgc->cmdq_handle_config_esd, CMDQ_ENABLE);

	/* 6.flush instruction */
	dprec_logger_start(DPREC_LOGGER_ESD_CMDQ, 0, 0);

	if (gDumpESDCMD == 1) {
		DISPMSG("esd dump before flush:\n");
		cmdqRecDumpCommand(pgc->cmdq_handle_config_esd);
	}

	ret = cmdqRecFlush(pgc->cmdq_handle_config_esd);

#if defined(MTK_FB_SODI_SUPPORT) && !defined(CONFIG_FPGA_EARLY_PORTING)
	spm_enable_sodi(1);
#endif
	_primary_path_unlock(__func__);

#ifdef DISP_DUMP_EVENT_STATUS
	{
		unsigned int i = 0;
		unsigned int event_status[6];

		DISPMSG("dump esd status: ");
		for (i = 0; i < 6; i++)
			cmdqBackupReadSlot(pgc->event_status, i, &event_status[i]);

		DISPMSG("%d, %d, %d, %d, %d, %d\n",
			event_status[0], event_status[1], event_status[2],
			event_status[3], event_status[4], event_status[5]);
	}
#endif

	dprec_logger_done(DPREC_LOGGER_ESD_CMDQ, 0, 0);

	DISPMSG("[ESD]_esd_check_config_handle_vdo ret=%d\n", ret);

	if (ret)
		ret = 1;
	primary_display_esd_cust_bycmdq(0);
	return ret;
}

/* For Vdo Mode EXT TE Check */
static irqreturn_t _esd_check_ext_te_irq_handler(int irq, void *data)
{
	MMProfileLogEx(ddp_mmp_get_events()->esd_vdo_eint, MMProfileFlagPulse, 0, 0);
	atomic_set(&esd_ext_te_event, 1);
	wake_up_interruptible(&esd_ext_te_wq);
	return IRQ_HANDLED;
}

int primary_display_switch_esd_mode(int mode)
{
	int ret = 0;
#ifdef GPIO_DSI_TE_PIN
	int gpio_mode = 0;
#endif

	DISPFUNC();

	if (pgc->plcm->params->dsi.customization_esd_check_enable != 0)
		return -1;	/* avoid build warning. */

	DISPMSG("switch esd mode to %d\n", mode);

#ifdef GPIO_DSI_TE_PIN
#ifndef CONFIG_FPGA_EARLY_PORTING
	gpio_mode = mt_get_gpio_mode(GPIO_DSI_TE_PIN);
	/* DISPMSG("[ESD]gpio_mode=%d\n", gpio_mode); */
#endif
#endif
	if (mode == 1) {
#ifdef GPIO_DSI_TE_PIN
		/*switch to vdo mode */
		if (gpio_mode == GPIO_DSI_TE_PIN_M_DSI_TE) {
#endif
			/* if(_need_register_eint()) */
			{
				/* DISPMSG("[ESD]switch video mode\n"); */
				struct device_node *node = NULL;
				int irq;
				u32 ints[2] = { 0, 0 };
#ifdef GPIO_DSI_TE_PIN
#ifndef CONFIG_FPGA_EARLY_PORTING
				/* 1.set GPIO107 eint mode */
				mt_set_gpio_mode(GPIO_DSI_TE_PIN, GPIO_DSI_TE_PIN_M_GPIO);
#endif
#endif
				/* 2.register eint */
				node = of_find_compatible_node(NULL, NULL, "mediatek, DSI_TE_1-eint");
				if (node) {
					/* DISPMSG("node 0x%x\n", node); */
					of_property_read_u32_array(node, "debounce", ints, ARRAY_SIZE(ints));
					/* mt_gpio_set_debounce(ints[0], ints[1]); */
					mt_eint_set_hw_debounce(ints[0], ints[1]);

					irq = irq_of_parse_and_map(node, 0);
					if (request_irq(irq, _esd_check_ext_te_irq_handler, IRQF_TRIGGER_RISING,
							"DSI_TE_1-eint", NULL))
						DISPERR("[ESD]EINT IRQ LINE NOT AVAILABLE!!\n");
				} else {
					DISPERR("[ESD][%s] can't find DSI_TE_1 eint compatible node\n", __func__);
				}
			}
#ifdef GPIO_DSI_TE_PIN
		}
#endif
	} else if (mode == 0) {
#ifdef GPIO_DSI_TE_PIN
		/* switch to cmd mode */
		if (gpio_mode == GPIO_DSI_TE_PIN_M_GPIO) {
#endif
			struct device_node *node = NULL;
			int irq;
			/* DISPMSG("[ESD]switch cmd mode\n"); */

			/* unregister eint */
			node = of_find_compatible_node(NULL, NULL, "mediatek, DSI_TE_1-eint");
			/* DISPMSG("node 0x%x\n", node); */
			if (node) {
				irq = irq_of_parse_and_map(node, 0);
				free_irq(irq, NULL);
			} else {
				DISPERR("[ESD][%s] can't find DSI_TE_1 eint compatible node\n",
					__func__);
			}
#ifdef GPIO_DSI_TE_PIN
#ifndef CONFIG_FPGA_EARLY_PORTING
			/* set GPIO107 DSI TE mode */
			mt_set_gpio_mode(GPIO_DSI_TE_PIN, GPIO_DSI_TE_PIN_M_DSI_TE);
#endif
#endif
#ifdef GPIO_DSI_TE_PIN
		}
#endif
	}
	/* DISPMSG("primary_display_switch_esd_mode end\n"); */
	return ret;
}

/* ESD CHECK FUNCTION */
/* return 1: esd check fail */
/* return 0: esd check pass */
int primary_display_esd_check(void)
{
	int ret = 0;

	_primary_path_esd_check_lock();
	dprec_logger_start(DPREC_LOGGER_ESD_CHECK, 0, 0);
	MMProfileLogEx(ddp_mmp_get_events()->esd_check_t, MMProfileFlagStart, 0, 0);
	DISPMSG("[ESD]ESD check begin\n");
	_primary_path_lock(__func__);
	if (pgc->state == DISP_SLEPT) {
		MMProfileLogEx(ddp_mmp_get_events()->esd_check_t, MMProfileFlagPulse, 1, 0);
		DISPMSG("[ESD]primary display path is slept?? -- skip esd check\n");
		_primary_path_unlock(__func__);
		/* goto done; */
		DISPMSG("[ESD]ESD check end\n");
		MMProfileLogEx(ddp_mmp_get_events()->esd_check_t, MMProfileFlagEnd, 0, ret);
		dprec_logger_done(DPREC_LOGGER_ESD_CHECK, 0, 0);
		_primary_path_esd_check_unlock();
		return ret;
	}
	_primary_path_unlock(__func__);
#ifdef MTK_DISP_IDLE_LP
	_disp_primary_path_dsi_clock_on(0);
#endif
	/* / Esd Check : EXT TE */
	if (pgc->plcm->params->dsi.customization_esd_check_enable == 0) {
		MMProfileLogEx(ddp_mmp_get_events()->esd_extte, MMProfileFlagStart, 0, 0);
		if (primary_display_is_video_mode()) {
			primary_display_switch_esd_mode(1);

			/* use cmdq to pull DSI clk lane */
			if (primary_display_cmdq_enabled()) {
				_primary_path_lock(__func__);

				/* 0.create esd check cmdq */
				cmdqRecCreate(CMDQ_SCENARIO_DISP_ESD_CHECK,
					      &(pgc->cmdq_handle_config_esd));
				_primary_path_unlock(__func__);

				primary_display_esd_cust_bycmdq(1);

				/* 1.reset */
				cmdqRecReset(pgc->cmdq_handle_config_esd);

				/* wait stream eof first */
				ret =
				    cmdqRecWait(pgc->cmdq_handle_config_esd,
						CMDQ_EVENT_DISP_RDMA0_EOF);

				cmdqRecWait(pgc->cmdq_handle_config_esd,
					    CMDQ_EVENT_MUTEX0_STREAM_EOF);

				_primary_path_lock(__func__);
				/* 2.stop dsi vdo mode */
				dpmgr_path_build_cmdq(pgc->dpmgr_handle,
						      pgc->cmdq_handle_config_esd,
						      CMDQ_STOP_VDO_MODE);

				/* 3.pull DSI clock lane */
				DSI_sw_clk_trail_cmdq(0, pgc->cmdq_handle_config_esd);
				DSI_manual_enter_HS(pgc->cmdq_handle_config_esd);

				/* 4.start dsi vdo mode */
				dpmgr_path_build_cmdq(pgc->dpmgr_handle,
						      pgc->cmdq_handle_config_esd,
						      CMDQ_START_VDO_MODE);


				/* 5. trigger path */
				cmdqRecClearEventToken(pgc->cmdq_handle_config_esd,
						       CMDQ_EVENT_MUTEX0_STREAM_EOF);

				if (gEnableSWTrigger == 1)
					DISP_REG_SET(pgc->cmdq_handle_config_esd,
						     DISP_REG_CONFIG_MUTEX_EN
						     (DISP_OVL_SEPARATE_MUTEX_ID), 1);

				dpmgr_path_trigger(pgc->dpmgr_handle, pgc->cmdq_handle_config_esd,
						   CMDQ_ENABLE);

				_primary_path_unlock(__func__);
				cmdqRecFlush(pgc->cmdq_handle_config_esd);

				primary_display_esd_cust_bycmdq(0);

				cmdqRecDestroy(pgc->cmdq_handle_config_esd);
				pgc->cmdq_handle_config_esd = NULL;
			}

			if (_need_register_eint()) {
				MMProfileLogEx(ddp_mmp_get_events()->esd_extte, MMProfileFlagPulse, 1, 1);

				if (wait_event_interruptible_timeout
				    (esd_ext_te_wq, atomic_read(&esd_ext_te_event), HZ / 2) > 0) {
					ret = 0; /* esd check pass */
				} else {
					ret = 1; /* esd check fail */
					DISPMSG("esd check fail release fence fake\n");
					primary_display_release_fence_fake();
				}
				atomic_set(&esd_ext_te_event, 0);
			}
			primary_display_switch_esd_mode(0);
		} else {
			MMProfileLogEx(ddp_mmp_get_events()->esd_extte, MMProfileFlagPulse, 0, 1);
			if (dpmgr_wait_event_timeout(pgc->dpmgr_handle, DISP_PATH_EVENT_IF_VSYNC, HZ / 2) > 0) {
				ret = 0; /* esd check pass */
			} else {
				ret = 1; /* esd check fail */
				DISPMSG("esd check fail release fence fake\n");
				primary_display_release_fence_fake();
			}
		}
		MMProfileLogEx(ddp_mmp_get_events()->esd_extte, MMProfileFlagEnd, 0, ret);
		goto done;
	}
	/* / Esd Check : Read from lcm */
	MMProfileLogEx(ddp_mmp_get_events()->esd_rdlcm, MMProfileFlagStart, 0, primary_display_cmdq_enabled());
	if (primary_display_cmdq_enabled()) {
		_primary_path_lock(__func__);
		MMProfileLogEx(ddp_mmp_get_events()->esd_rdlcm, MMProfileFlagPulse, 0, 1);
		/* 0.create esd check cmdq */
		cmdqRecCreate(CMDQ_SCENARIO_DISP_ESD_CHECK, &(pgc->cmdq_handle_config_esd));
		dpmgr_path_build_cmdq(pgc->dpmgr_handle, pgc->cmdq_handle_config_esd, CMDQ_ESD_ALLC_SLOT);
		MMProfileLogEx(ddp_mmp_get_events()->esd_rdlcm, MMProfileFlagPulse, 0, 2);
		DISPMSG("[ESD]ESD config thread=%p\n", pgc->cmdq_handle_config_esd);
		_primary_path_unlock(__func__);

		/* 1.use cmdq to read from lcm */
		if (primary_display_is_video_mode())
			ret = _esd_check_config_handle_vdo();
		else
			ret = _esd_check_config_handle_cmd();

		MMProfileLogEx(ddp_mmp_get_events()->esd_rdlcm, MMProfileFlagPulse, primary_display_is_video_mode(), 3);
		if (ret == 1) {/* cmdq fail */
			/* Need set esd check eof synctoken to let trigger loop go. */
			if (_need_wait_esd_eof())
				cmdqCoreSetEvent(CMDQ_SYNC_TOKEN_ESD_EOF);

			/* do dsi reset */
			dpmgr_path_build_cmdq(pgc->dpmgr_handle, pgc->cmdq_handle_config_esd, CMDQ_DSI_RESET);
			DISPMSG("esd check fail release fence fake\n");
			primary_display_release_fence_fake();
			goto destroy_cmdq;
		}

		DISPMSG("[ESD]ESD config thread done~\n");

		/* 2.check data(*cpu check now) */
		ret = dpmgr_path_build_cmdq(pgc->dpmgr_handle, pgc->cmdq_handle_config_esd, CMDQ_ESD_CHECK_CMP);
		MMProfileLogEx(ddp_mmp_get_events()->esd_rdlcm, MMProfileFlagPulse, 0, 4);
		if (ret) {
			ret = 1;	/* esd check fail */
			DISPMSG("esd check fail release fence fake\n");
			primary_display_release_fence_fake();
		}

destroy_cmdq:
		dpmgr_path_build_cmdq(pgc->dpmgr_handle, pgc->cmdq_handle_config_esd,
				      CMDQ_ESD_FREE_SLOT);
		/* 3.destroy esd config thread */
		cmdqRecDestroy(pgc->cmdq_handle_config_esd);
		pgc->cmdq_handle_config_esd = NULL;
	} else { /* by cpu */
		/* 0: lock path */
		/* 1: stop path */
		/* 2: do esd check (!!!) */
		/* 3: start path */
		/* 4: unlock path */

		MMProfileLogEx(ddp_mmp_get_events()->esd_rdlcm, MMProfileFlagPulse, 0, 1);
		_primary_path_lock(__func__);

		/* 1: stop path */
		DISPMSG("[ESD]display cmdq trigger loop stop[begin]\n");
		_cmdq_stop_trigger_loop();
		DISPMSG("[ESD]display cmdq trigger loop stop[end]\n");

		if (dpmgr_path_is_busy(pgc->dpmgr_handle)) {
			DISPMSG("[ESD]primary display path is busy\n");
			ret = dpmgr_wait_event_timeout(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_DONE, HZ * 1);
			DISPMSG("[ESD]wait frame done ret:%d\n", ret);
		}

		DISPMSG("[ESD]stop dpmgr path[begin]\n");
		dpmgr_path_stop(pgc->dpmgr_handle, CMDQ_DISABLE);
		DISPMSG("[ESD]stop dpmgr path[end]\n");

		if (dpmgr_path_is_busy(pgc->dpmgr_handle)) {
			DISPMSG("[ESD]primary display path is busy after stop\n");
			dpmgr_wait_event_timeout(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_DONE, HZ * 1);
			DISPMSG("[ESD]wait frame done ret:%d\n", ret);
		}

		DISPMSG("[ESD]reset display path[begin]\n");
		dpmgr_path_reset(pgc->dpmgr_handle, CMDQ_DISABLE);
		DISPMSG("[ESD]reset display path[end]\n");

		/* 2: do esd check (!!!) */
		MMProfileLogEx(ddp_mmp_get_events()->esd_rdlcm, MMProfileFlagPulse, 0, 2);

		if (primary_display_is_video_mode())
			/* ret = 0; */
			ret = disp_lcm_esd_check(pgc->plcm);
		else
			ret = disp_lcm_esd_check(pgc->plcm);


		/* 3: start path */
		MMProfileLogEx(ddp_mmp_get_events()->esd_rdlcm, MMProfileFlagPulse, primary_display_is_video_mode(), 3);

		DISPMSG("[ESD]start dpmgr path[begin]\n");
		dpmgr_path_start(pgc->dpmgr_handle, CMDQ_DISABLE);
		DISPMSG("[ESD]start dpmgr path[end]\n");

		DISPMSG("[ESD]start cmdq trigger loop[begin]\n");
		_cmdq_start_trigger_loop();
		DISPMSG("[ESD]start cmdq trigger loop[end]\n");

		_primary_path_unlock(__func__);
	}
	MMProfileLogEx(ddp_mmp_get_events()->esd_rdlcm, MMProfileFlagEnd, 0, ret);

done:
#ifdef MTK_DISP_IDLE_LP
	_disp_primary_path_dsi_clock_off(0);
#endif
	DISPMSG("[ESD]ESD check end\n");
	MMProfileLogEx(ddp_mmp_get_events()->esd_check_t, MMProfileFlagEnd, 0, ret);
	dprec_logger_done(DPREC_LOGGER_ESD_CHECK, 0, 0);
	_primary_path_esd_check_unlock();
	return ret;

}

#ifdef MTK_FB_ESD_ENABLE
static int primary_display_esd_check_worker_kthread(void *data)
{
	int ret = 0;
	int i = 0;
	int esd_try_cnt = 5;	/* 20; */
	int count = 0;
	struct sched_param param = {.sched_priority = 87 }; /* RTPM_PRIO_FB_THREAD */

	sched_setscheduler(current, SCHED_RR, &param);
	dpmgr_enable_event(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_DONE);
	dpmgr_enable_event(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_START);

	while (1) {
#if 0
		dpmgr_wait_event(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_START);
		ret = dpmgr_wait_event_timeout(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_DONE, HZ);
		if (ret <= 0) {
			DISPERR("wait frame done timeout, reset whole path now\n");
			primary_display_diagnose();
			dprec_logger_trigger(DPREC_LOGGER_ESD_RECOVERY);
			dpmgr_path_reset(pgc->dpmgr_handle, CMDQ_DISABLE);
		}
#else
		if (count == 0) {
			count++;
			msleep(3000);
		}
	
		msleep(2000); /* esd check every 2s */
		ret = wait_event_interruptible(esd_check_task_wq, atomic_read(&esd_check_task_wakeup));
		if (ret < 0) {
			DISPMSG("[ESD]esd check thread waked up accidently\n");
			continue;
		}
#ifdef DISP_SWITCH_DST_MODE
		_primary_path_switch_dst_lock();
#endif
		_primary_path_cmd_lock();
#if 0
		{
			/* let's do a mutex holder check here */
			unsigned long long period = 0;

			period = dprec_logger_get_current_hold_period(DPREC_LOGGER_PRIMARY_MUTEX);
			if (period > 2000 * 1000 * 1000) {
				DISPERR("primary display mutex is hold by %s for %dns\n",
					pgc->mutex_locker, period);
			}
		}
#endif
		ret = primary_display_esd_check();

		if (ret == 1) {
			DISPMSG("[ESD]esd check fail, will do esd recovery %d\n", ret);
			i = esd_try_cnt;
			while (i--) {
				DISPMSG("[ESD]esd recovery try:%d\n", i);
				primary_display_esd_recovery();
				msleep(50);
				ret = primary_display_esd_check();

				if (ret == 0) {
					DISPMSG("[ESD]esd recovery success\n");
					break;

				DISPMSG("[ESD]after esd recovery, esd check still fail\n");
				if (i == 0) {
					DISPMSG("[ESD]after esd recovery %d times, esd check still fail,\n",
						  esd_try_cnt);
					DISPMSG("disable esd check\n");
					primary_display_esd_check_enable(0);
					primary_display_esd_recovery();
#ifdef CONFIG_LCT_ESD_CHECK_MULTI_REG
					msleep(50);
#endif
				}
			}
		}
	}
		_primary_path_cmd_unlock();
#ifdef DISP_SWITCH_DST_MODE
		_primary_path_switch_dst_unlock();
#endif
#endif

		if (kthread_should_stop())
			break;
	}
	return 0;
}
#endif /* MTK_FB_ESD_ENABLE */

/* ESD RECOVERY */
int primary_display_esd_recovery(void)
{
	DISP_STATUS ret = DISP_STATUS_OK;
	LCM_PARAMS *lcm_param = NULL;

	DISPFUNC();
	dprec_logger_start(DPREC_LOGGER_ESD_RECOVERY, 0, 0);
	MMProfileLogEx(ddp_mmp_get_events()->esd_recovery_t, MMProfileFlagStart, 0, 0);
	DISPMSG("[ESD]ESD recovery begin\n");

	_primary_path_lock(__func__);

	MMProfileLogEx(ddp_mmp_get_events()->esd_recovery_t, MMProfileFlagPulse,
		       primary_display_is_video_mode(), 1);


	lcm_param = disp_lcm_get_params(pgc->plcm);
	if (pgc->state == DISP_SLEPT) {
		DISPMSG("[ESD]esd recovery but primary display path is slept??\n");
		goto done;
	}

	MMProfileLogEx(ddp_mmp_get_events()->esd_recovery_t, MMProfileFlagPulse, 0, 2);

	if (dpmgr_path_is_busy(pgc->dpmgr_handle)) {
		DISPMSG("[ESD]primary display path is busy\n");
		ret = dpmgr_wait_event_timeout(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_DONE, HZ * 1);
		DISPMSG("[ESD]wait frame done ret:%d\n", ret);
	}

	MMProfileLogEx(ddp_mmp_get_events()->esd_recovery_t, MMProfileFlagPulse, 0, 3);

	DISPMSG("[ESD]stop dpmgr path[begin]\n");
	dpmgr_path_stop(pgc->dpmgr_handle, CMDQ_DISABLE);
	DISPMSG("[ESD]stop dpmgr path[end]\n");

	MMProfileLogEx(ddp_mmp_get_events()->esd_recovery_t, MMProfileFlagPulse, 0, 4);

	if (dpmgr_path_is_busy(pgc->dpmgr_handle)) {
		DISPMSG("[ESD]primary display path is busy after stop\n");
		dpmgr_wait_event_timeout(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_DONE, HZ * 1);
		DISPMSG("[ESD]wait frame done ret:%d\n", ret);
	}

	MMProfileLogEx(ddp_mmp_get_events()->esd_recovery_t, MMProfileFlagPulse, 0, 5);

	DISPMSG("[ESD]display cmdq trigger loop stop[begin]\n");
	_cmdq_stop_trigger_loop();
	DISPMSG("[ESD]display cmdq trigger loop stop[end]\n");

	MMProfileLogEx(ddp_mmp_get_events()->esd_recovery_t, MMProfileFlagPulse, 0, 6);

	DISPMSG("[ESD]reset display path[begin]\n");
	dpmgr_path_reset(pgc->dpmgr_handle, CMDQ_DISABLE);
	DISPMSG("[ESD]reset display path[end]\n");

	DISPMSG("[POWER]lcm suspend[begin]\n");
	disp_lcm_suspend(pgc->plcm);
	DISPMSG("[POWER]lcm suspend[end]\n");

	MMProfileLogEx(ddp_mmp_get_events()->esd_recovery_t, MMProfileFlagPulse, 0, 7);

	DISPMSG("[ESD]dsi power reset[begin]\n");
	dpmgr_path_dsi_power_off(pgc->dpmgr_handle, CMDQ_DISABLE);
	dpmgr_path_dsi_power_on(pgc->dpmgr_handle, CMDQ_DISABLE);
	if (!primary_display_is_video_mode())
		dpmgr_path_ioctl(pgc->dpmgr_handle, NULL, DDP_DSI_ENABLE_TE, NULL);
	DISPMSG("[ESD]dsi power reset[end]\n");

	MMProfileLogEx(ddp_mmp_get_events()->esd_recovery_t, MMProfileFlagPulse, 0, 8);

	DISPMSG("[ESD]lcm force init[begin]\n");
	disp_lcm_init(pgc->plcm, 1);
#ifdef CONFIG_LCT_ESD_CHECK_MULTI_REG
#ifdef CONFIG_LCT_LCM_GPIO_UTIL
	lcm_led_en_settting(1);
	mdelay(5);
#endif
#endif
	DISPMSG("[ESD]lcm force init[end]\n");

	MMProfileLogEx(ddp_mmp_get_events()->esd_recovery_t, MMProfileFlagPulse, 0, 9);

	DISPMSG("[ESD]start dpmgr path[begin]\n");
	dpmgr_path_start(pgc->dpmgr_handle, CMDQ_DISABLE);
	DISPMSG("[ESD]start dpmgr path[end]\n");
	if (dpmgr_path_is_busy(pgc->dpmgr_handle)) {
		DISPERR("[ESD]Fatal error, we didn't trigger display path but it's already busy\n");
		ret = -1;
		/* goto done; */
	}

	MMProfileLogEx(ddp_mmp_get_events()->esd_recovery_t, MMProfileFlagPulse, 0, 10);

	if (primary_display_is_video_mode()) {
		/* for video mode, we need to force trigger here */
		/* for cmd mode, just set DPREC_EVENT_CMDQ_SET_EVENT_ALLOW when trigger loop start */
		dpmgr_path_trigger(pgc->dpmgr_handle, NULL, CMDQ_DISABLE);
	}

	MMProfileLogEx(ddp_mmp_get_events()->esd_recovery_t, MMProfileFlagPulse, 0, 11);

	DISPMSG("[ESD]start cmdq trigger loop[begin]\n");
	_cmdq_start_trigger_loop();
	DISPMSG("[ESD]start cmdq trigger loop[end]\n");

	if (!primary_display_is_video_mode())
		dpmgr_signal_event(pgc->dpmgr_handle, DISP_PATH_EVENT_TRIGGER);

	MMProfileLogEx(ddp_mmp_get_events()->esd_recovery_t, MMProfileFlagPulse, 0, 12);

done:
	_primary_path_unlock(__func__);

#ifdef CONFIG_LCT_ESD_CHECK_MULTI_REG
	//add by zhudaolong at 20170411
	DISPERR("[ESD]lcm force backlight set to %d, line=%d\n", esd_recovery_level, __LINE__);
     	mdelay(10);
	if(esd_recovery_level == 0)
	{
		esd_recovery_level = 128;
	}
 	disp_lcm_set_backlight(pgc->plcm, esd_recovery_level);
#endif

	DISPMSG("[ESD]ESD recovery end\n");
	MMProfileLogEx(ddp_mmp_get_events()->esd_recovery_t, MMProfileFlagEnd, 0, 0);
	dprec_logger_done(DPREC_LOGGER_ESD_RECOVERY, 0, 0);
	return ret;
}

void primary_display_esd_check_enable(int enable)
{
	/* check if enable ESD check mechanism first */
	if (_is_enable_esd_check() != true) {
		DISPMSG("[ESD]Checking if ESD enable but ESD check mechanism is not enabled.\n");
		return;
	}

	if (_need_do_esd_check()) {
		if (_need_register_eint() && eint_flag != 2) {
			DISPMSG("[ESD]Please check DCT setting about GPIO107/EINT107\n");
			return;
		}

		if (enable) {
			pr_warn("[DISP][ESD]esd check thread wakeup\n");
			atomic_set(&esd_check_task_wakeup, 1);
			wake_up_interruptible(&esd_check_task_wq);
		} else {
			DISPMSG("[ESD]esd check thread stop\n");
			atomic_set(&esd_check_task_wakeup, 0);
		}
	}
}

/******************************************************************************/
/* ESD CHECK / RECOVERY ---- End                                              */
/******************************************************************************/
#define EEEEEEEEEEEEEEEEEEEEEEEEEE

#ifdef MTK_FB_PULLCLK_ENABLE
static int primary_display_vdo_pullclk_worker_kthread(void *data)
{
	int ret = 0;
	int count = 0;
	struct sched_param param = {.sched_priority = 87 }; /* RTPM_PRIO_FB_THREAD */

	sched_setscheduler(current, SCHED_RR, &param);


	while (1) {
		if (count == 0) {
			count++;
			msleep(3000);
		}
		msleep(2000);
#ifdef DISP_SWITCH_DST_MODE
		_primary_path_switch_dst_lock();
#endif
		_primary_path_cmd_lock();


		_primary_path_esd_check_lock();
		_primary_path_lock(__func__);
		if (pgc->state == DISP_SLEPT) {
			_primary_path_unlock(__func__);
			_primary_path_esd_check_unlock();
			continue;
		}
		_primary_path_unlock(__func__);

#ifdef MTK_DISP_IDLE_LP
		_disp_primary_path_dsi_clock_on(0);
#endif

		_primary_path_lock(__func__);
		cmdqRecCreate(CMDQ_SCENARIO_DISP_ESD_CHECK, &(pgc->cmdq_handle_config_esd));
		dpmgr_path_build_cmdq(pgc->dpmgr_handle, pgc->cmdq_handle_config_esd, CMDQ_ESD_ALLC_SLOT);
		_primary_path_unlock(__func__);
#ifdef DISP_DUMP_EVENT_STATUS
		DISP_REG_SET_PA(pgc->cmdq_handle_config_esd, DISP_REG_CMDQ_TOKEN_ID, DISP_MUTEX0_STREAM_EOF_ID);
		cmdqRecBackupRegisterToSlot(pgc->cmdq_handle_config_esd, pgc->event_status, 1
						, DISP_REG_CMDQ_TOKEN_VALUE);
#endif

		primary_display_esd_cust_bycmdq(1);

		_primary_path_lock(__func__);

#if defined(MTK_FB_SODI_SUPPORT) && !defined(CONFIG_FPGA_EARLY_PORTING)
		spm_enable_sodi(0);
#endif

		/* reset*/
		cmdqRecReset(pgc->cmdq_handle_config_esd);

		/* wait stream eof first*/
		cmdqRecWait(pgc->cmdq_handle_config_esd, CMDQ_EVENT_DISP_RDMA0_EOF);
		cmdqRecWait(pgc->cmdq_handle_config_esd, CMDQ_EVENT_MUTEX0_STREAM_EOF);



		/*stop dsi vdo mode*/
		dpmgr_path_build_cmdq(pgc->dpmgr_handle, pgc->cmdq_handle_config_esd, CMDQ_STOP_VDO_MODE);


		/* pull DSI clock lane */
		DSI_sw_clk_trail_cmdq(0, pgc->cmdq_handle_config_esd);
		DSI_manual_enter_HS(pgc->cmdq_handle_config_esd);

		/* start dsi vdo mode*/
		dpmgr_path_build_cmdq(pgc->dpmgr_handle, pgc->cmdq_handle_config_esd, CMDQ_START_VDO_MODE);

		/* trigger path*/
		cmdqRecClearEventToken(pgc->cmdq_handle_config_esd, CMDQ_EVENT_MUTEX0_STREAM_EOF);

		if (gEnableSWTrigger == 1)
			DISP_REG_SET(pgc->cmdq_handle_config_esd
					, DISP_REG_CONFIG_MUTEX_EN(DISP_OVL_SEPARATE_MUTEX_ID), 1);

		dpmgr_path_trigger(pgc->dpmgr_handle, pgc->cmdq_handle_config_esd, CMDQ_ENABLE);
		if (gDumpESDCMD == 1) {
			DISPMSG("esd dump before flush:\n");
			cmdqRecDumpCommand(pgc->cmdq_handle_config_esd);
		}

		ret = cmdqRecFlush(pgc->cmdq_handle_config_esd);

#if defined(MTK_FB_SODI_SUPPORT) && !defined(CONFIG_FPGA_EARLY_PORTING)
		spm_enable_sodi(1);
#endif
		_primary_path_unlock(__func__);
		primary_display_esd_cust_bycmdq(0);

		_primary_path_lock(__func__);
		dpmgr_path_build_cmdq(pgc->dpmgr_handle, pgc->cmdq_handle_config_esd, CMDQ_ESD_FREE_SLOT);
		cmdqRecDestroy(pgc->cmdq_handle_config_esd);
		pgc->cmdq_handle_config_esd = NULL;
		_primary_path_unlock(__func__);

#ifdef MTK_DISP_IDLE_LP
		_disp_primary_path_dsi_clock_off(0);
#endif

		_primary_path_esd_check_unlock();

		_primary_path_cmd_unlock();
#ifdef DISP_SWITCH_DST_MODE
		_primary_path_switch_dst_unlock();
#endif

		if (kthread_should_stop())
			break;

	}
	return 0;
}
#endif

static struct task_struct *primary_path_aal_task;

void disp_update_trigger_time(void)
{
	last_primary_trigger_time = sched_clock();
}

/* extern unsigned int gResetOVLInAALTrigger; */
static int _disp_primary_path_check_trigger(void *data)
{

	cmdqRecHandle handle = NULL;
#ifndef MTK_FB_CMDQ_DISABLE
	int ret = 0;

	ret = cmdqRecCreate(CMDQ_SCENARIO_PRIMARY_DISP, &handle);
#endif

	dpmgr_enable_event(pgc->dpmgr_handle, DISP_PATH_EVENT_TRIGGER);
	while (1) {
		dpmgr_wait_event(pgc->dpmgr_handle, DISP_PATH_EVENT_TRIGGER);
		MMProfileLogEx(ddp_mmp_get_events()->primary_display_aalod_trigger,
			       MMProfileFlagPulse, 0, 0);

		_primary_path_lock(__func__);
		if (primary_get_state() == DISP_ALIVE) {
#ifdef MTK_DISP_IDLE_LP
			last_primary_trigger_time = sched_clock();
			_disp_primary_path_exit_idle(__func__, 0);
#endif
			cmdqRecReset(handle);

#ifndef MTK_FB_CMDQ_DISABLE
			if (primary_display_is_video_mode())
				cmdqRecWaitNoClear(handle, CMDQ_EVENT_DISP_RDMA0_EOF);
			else
				cmdqRecWaitNoClear(handle, CMDQ_SYNC_TOKEN_STREAM_EOF);
#endif
			if (gResetOVLInAALTrigger == 1) {
				ovl_reset_by_cmdq(handle, DISP_MODULE_OVL0);
				if (ovl_get_status() != DDP_OVL1_STATUS_SUB)
					ovl_reset_by_cmdq(handle, DISP_MODULE_OVL1);
			}

			_cmdq_set_config_handle_dirty_mira(handle);

			/* disable SODI after set dirty */
			if (primary_display_is_video_mode() == 0 && gDisableSODIForTriggerLoop == 1)
				disp_set_sodi(0, handle);

			_cmdq_flush_config_handle_mira(handle, 0);
		}
		_primary_path_unlock(__func__);
		if (kthread_should_stop())
			break;
	}

	cmdqRecDestroy(handle);

	return 0;
}

#define OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO

/* need remove */
unsigned int cmdqDdpClockOn(uint64_t engineFlag)
{
	/* DISP_LOG_I("cmdqDdpClockOff\n"); */
	return 0;
}

unsigned int cmdqDdpClockOff(uint64_t engineFlag)
{
	/* DISP_LOG_I("cmdqDdpClockOff\n"); */
	return 0;
}

unsigned int cmdqDdpDumpInfo(uint64_t engineFlag, char *pOutBuf, unsigned int bufSize)
{
	DISPERR("cmdq timeout:%llu\n", engineFlag);
	primary_display_diagnose();
	/* DISP_LOG_I("cmdqDdpDumpInfo\n"); */

	if (primary_display_is_decouple_mode()) {
		ddp_dump_analysis(DISP_MODULE_OVL0);
#if defined(OVL_CASCADE_SUPPORT)
		ddp_dump_analysis(DISP_MODULE_OVL1);
#endif
	}
	ddp_dump_analysis(DISP_MODULE_WDMA0);

	/* try to set event by CPU to avoid blocking auto test such as Monkey/MTBF */
	cmdqCoreSetEvent(CMDQ_SYNC_TOKEN_STREAM_EOF);
	cmdqCoreSetEvent(CMDQ_EVENT_DISP_RDMA0_EOF);

	return 0;
}

unsigned int cmdqDdpResetEng(uint64_t engineFlag)
{
	/* DISP_LOG_I("cmdqDdpResetEng\n"); */
	return 0;
}


/* TODO: these 2 functions should be splited into another file */
unsigned int display_path_idle_cnt = 0;
#if defined(MTK_FB_SODI_SUPPORT)
static void _RDMA0_INTERNAL_IRQ_Handler(DISP_MODULE_ENUM module, unsigned int param)
{
	if (!_is_decouple_mode(pgc->session_mode) && param & 0x2) {
		/* RDMA Start */
		display_path_idle_cnt++;
#if !defined(CONFIG_FPGA_EARLY_PORTING) && !defined(FORCE_SODI_CG_MODE)
		spm_sodi_mempll_pwr_mode(1);
#endif
		MMProfileLogEx(ddp_mmp_get_events()->sodi_disable, MMProfileFlagPulse, 0, 0);
	}
	if (param & 0x4) {
		/* RDMA Done */
		display_path_idle_cnt--;
		if (display_path_idle_cnt == 0) {
#if !defined(CONFIG_FPGA_EARLY_PORTING) && !defined(FORCE_SODI_CG_MODE)
			spm_sodi_mempll_pwr_mode(0);
#endif
			MMProfileLogEx(ddp_mmp_get_events()->sodi_enable, MMProfileFlagPulse, 0, 0);
		}
	}
}

static void _WDMA0_INTERNAL_IRQ_Handler(DISP_MODULE_ENUM module, unsigned int param)
{
	if (param & 0x1) {
		/* WDMA Done */
		dpmgr_wdma_path_force_power_off();
		if (!primary_display_is_video_mode()) {
			display_path_idle_cnt--;
			if (display_path_idle_cnt == 0) {
#if !defined(CONFIG_FPGA_EARLY_PORTING) && !defined(FORCE_SODI_CG_MODE)
				spm_sodi_mempll_pwr_mode(0);
#endif
			}
		}

	}
}

static void _MUTEX_INTERNAL_IRQ_Handler(DISP_MODULE_ENUM module, unsigned int param)
{
	if (param & 0x1) { /* RDMA-->DSI SOF */
		display_path_idle_cnt++;
#if !defined(CONFIG_FPGA_EARLY_PORTING) && !defined(FORCE_SODI_CG_MODE)
		spm_sodi_mempll_pwr_mode(1);
#endif
	}
	if (param & 0x2) { /* OVL->WDMA SOF */
		display_path_idle_cnt++;
#if !defined(CONFIG_FPGA_EARLY_PORTING) && !defined(FORCE_SODI_CG_MODE)
		spm_sodi_mempll_pwr_mode(1);
#endif
	}
}
#endif

void primary_display_sodi_rule_init(void)
{
#if defined(MTK_FB_SODI_DEFEATURE)
	if (primary_display_is_video_mode() == 1) {
#ifndef CONFIG_FPGA_EARLY_PORTING
		defeature_soidle_by_display();
#endif
		DISPERR("SODI defeatured!\n");
		return;
	}
#endif
#if defined(MTK_FB_SODI_SUPPORT)
	/* if ((primary_display_mode == DECOUPLE_MODE) && primary_display_is_video_mode()) */
	if (gEnableSODIControl == 0 && primary_display_is_video_mode() == 1) {
#ifndef CONFIG_FPGA_EARLY_PORTING
		spm_enable_sodi(0);
#endif
		DISPMSG("SODI disabled!\n");
		return;
	}
#ifndef CONFIG_FPGA_EARLY_PORTING
	spm_enable_sodi(1);
#endif

	if (primary_display_is_video_mode()) {
		/* if switch to video mode, should de-register callback */
		disp_unregister_module_irq_callback(DISP_MODULE_RDMA0, _RDMA0_INTERNAL_IRQ_Handler);
#ifdef WDMA_PATH_CLOCK_DYNAMIC_SWITCH
		disp_register_module_irq_callback(DISP_MODULE_WDMA0, _WDMA0_INTERNAL_IRQ_Handler);
#endif
		/* spm_sodi_mempll_pwr_mode(0); */
	} else {
		disp_register_module_irq_callback(DISP_MODULE_RDMA0, _RDMA0_INTERNAL_IRQ_Handler);
#ifdef WDMA_PATH_CLOCK_DYNAMIC_SWITCH
		disp_register_module_irq_callback(DISP_MODULE_WDMA0, _WDMA0_INTERNAL_IRQ_Handler);
#endif
		if (_is_decouple_mode(pgc->session_mode)) {
			disp_register_module_irq_callback(DISP_MODULE_MUTEX, _MUTEX_INTERNAL_IRQ_Handler);
			disp_register_module_irq_callback(DISP_MODULE_WDMA0, _WDMA0_INTERNAL_IRQ_Handler);
		}
	}
#endif
}


/* extern int dfo_query(const char *s, unsigned long *v); */

int primary_display_change_lcm_resolution(unsigned int width, unsigned int height)
{
	if (pgc->plcm) {
		DISPMSG("LCM Resolution will be changed, original: %dx%d, now: %dx%d\n",
			pgc->plcm->params->width, pgc->plcm->params->height, width, height);
		/* align with 4 is the minimal check, to ensure we can boot up into kernel,
		 * and could modify dfo setting again using meta tool
		 */
		/* otherwise we will have a panic in lk(root cause unknown). */
		if (width > pgc->plcm->params->width || height > pgc->plcm->params->height ||
		    width == 0 || height == 0 || width % 4 || height % 2) {
			DISPERR("Invalid resolution: %dx%d\n", width, height);
			return -1;
		}

		if (primary_display_is_video_mode()) {
			DISPERR("Warning!!!Video Mode can't support multiple resolution!\n");
			return -1;
		}

		pgc->plcm->params->width = width;
		pgc->plcm->params->height = height;

		return 0;
	} else {
		return -1;
	}
}

static void primary_display_frame_update_irq_callback(DISP_MODULE_ENUM module, unsigned int param)
{
	/* if (pgc->session_mode == DISP_SESSION_DIRECT_LINK_MIRROR_MODE) */
	/*    return; */

	if (module == DISP_MODULE_RDMA0) {
		if (param & 0x20) { /* rdma0 frame start */
			if (pgc->session_id > 0)
				update_frm_seq_info(ddp_ovl_get_cur_addr(1, 0), 0, 1, FRM_START);
		}

		if (param & 0x4) { /* rdma0 frame done */
			atomic_set(&primary_display_frame_update_event, 1);
			wake_up_interruptible(&primary_display_frame_update_wq);
		}
	}

	if ((module == DISP_MODULE_OVL0) && (_is_decouple_mode(pgc->session_mode) == 0)) {
		if (param & 0x2) { /* ov0 frame done */
			atomic_set(&primary_display_frame_update_event, 1);
			wake_up_interruptible(&primary_display_frame_update_wq);
		}
	}
#if defined(CONFIG_TRUSTONIC_TEE_SUPPORT) && defined(CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT)
	/* In TEE, we have to protect WDMA registers, so we can't enable WDMA interrupt */
	/* here we use ovl frame done interrupt instead */
	if ((module == DISP_MODULE_OVL0) && (_is_decouple_mode(pgc->session_mode) == 1)) {
		if (param & 0x2) {/* OVL0 frame done */
			atomic_set(&decouple_fence_release_event, 1);
			wake_up_interruptible(&decouple_fence_release_wq);
		}
	}
#else
	if ((module == DISP_MODULE_WDMA0) && (_is_decouple_mode(pgc->session_mode) == 1)) {
		if (param & 0x1) {/* wdma0 frame done */
			atomic_set(&decouple_fence_release_event, 1);
			wake_up_interruptible(&decouple_fence_release_wq);
		}
	}
#endif

}

static int primary_display_frame_update_kthread(void *data)
{
	struct sched_param param = {.sched_priority = 94 }; /* RTPM_PRIO_SCRN_UPDATE */

	sched_setscheduler(current, SCHED_RR, &param);

	for (;;) {
		wait_event_interruptible(primary_display_frame_update_wq,
					 atomic_read(&primary_display_frame_update_event));
		atomic_set(&primary_display_frame_update_event, 0);

		if (pgc->session_id > 0)
			update_frm_seq_info(0, 0, 0, FRM_END);

		if (kthread_should_stop())
			break;
	}

	return 0;
}

/* defined but not used */
/* static struct task_struct *if_fence_release_worker_task = NULL; */

#if 0
/**
 * Defined but not used, avoid build warning. (2015.1.30 Rynn Wu)
 */
static int _if_fence_release_worker_thread(void *data)
{
	unsigned int session_id = MAKE_DISP_SESSION(DISP_SESSION_PRIMARY, 0);
	struct sched_param param = { .sched_priority = RTPM_PRIO_SCRN_UPDATE };

	sched_setscheduler(current, SCHED_RR, &param);

	dpmgr_enable_event(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_START);

	while (1) {
		dpmgr_wait_event(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_START);
		if (_is_mirror_mode(pgc->session_mode)) {
			int fence_idx, subtractor, layer;

			layer = disp_sync_get_output_interface_timeline_id();

			cmdqBackupReadSlot(pgc->cur_config_fence, layer, &fence_idx);
			cmdqBackupReadSlot(pgc->subtractor_when_free, layer, &subtractor);
			mtkfb_release_fence(session_id, layer, fence_idx-1);
		}
		if (kthread_should_stop())
			break;
	}

	return 0;
}
#endif

/* defined but not used. */
/* static struct task_struct *ovl2mem_fence_release_worker_task = NULL; */

#if 0
/**
 * Defined but not used, avoid build warning. (2015.1.30 Rynn Wu)
 */
static int _ovl2mem_fence_release_worker_thread(void *data)
{
	unsigned int session_id = MAKE_DISP_SESSION(DISP_SESSION_PRIMARY, 0);
	struct sched_param param = { .sched_priority = RTPM_PRIO_SCRN_UPDATE };

	sched_setscheduler(current, SCHED_RR, &param);

	dpmgr_enable_event(pgc->ovl2mem_path_handle, DISP_PATH_EVENT_FRAME_COMPLETE);

	while (1) {
		/* it's not good to use FRAME_COMPLETE here, because when CPU read wdma addr,
		 * maybe it's already changed by next request,
		 * but luckly currently we will wait rdma frame done after wdma done(in CMDQ), so it's safe now
		 */
		dpmgr_wait_event(pgc->ovl2mem_path_handle, DISP_PATH_EVENT_FRAME_COMPLETE);
		if (_is_mirror_mode(pgc->session_mode)) {
			int fence_idx, subtractor, layer;

			layer = disp_sync_get_output_timeline_id();

			cmdqBackupReadSlot(pgc->cur_config_fence, layer, &fence_idx);
			cmdqBackupReadSlot(pgc->subtractor_when_free, layer, &subtractor);
			mtkfb_release_fence(session_id, layer, fence_idx);
		}

		if (kthread_should_stop())
			break;
	}

	return 0;
}
#endif

/* defined but not used. */
/* static struct task_struct *fence_release_worker_task = NULL; */

/* extern unsigned int ddp_ovl_get_cur_addr(bool rdma_mode, int layerid); */

static void _wdma_fence_release_callback(uint32_t userdata)
{
	int fence_idx, layer;

	layer = disp_sync_get_output_timeline_id();

	cmdqBackupReadSlot(pgc->cur_config_fence, layer, &fence_idx);
	mtkfb_release_fence(primary_session_id, layer, fence_idx);
	MMProfileLogEx(ddp_mmp_get_events()->primary_wdma_fence_release, MMProfileFlagPulse, layer,
		       fence_idx);
}

static int _Interface_fence_release_callback(uint32_t userdata)
{
	int layer = disp_sync_get_output_interface_timeline_id();

	if (userdata > 0) {
		mtkfb_release_fence(primary_session_id, layer, userdata);
		MMProfileLogEx(ddp_mmp_get_events()->primary_wdma_fence_release, MMProfileFlagPulse,
			       layer, userdata);
	}

	return 0;
}

static int _ovl_ext_fence_release_callback(uint32_t userdata)
{
	int i = 0;
	int ret = 0;
	int fence_idx, layer;

	MMProfileLogEx(ddp_mmp_get_events()->session_release, MMProfileFlagStart, 1, userdata);
#ifndef MTK_FB_CMDQ_DISABLE
	for (i = 0; i < PRIMARY_DISPLAY_SESSION_LAYER_COUNT; i++) {
		int fence_idx  = 0;
		int subtractor = 0;

		if (i == primary_display_get_option("ASSERT_LAYER") && is_DAL_Enabled()) {
			mtkfb_release_layer_fence(ext_session_id, i);
		} else {
			cmdqBackupReadSlot(pgc->cur_mem_config_fence, i, &fence_idx);
			cmdqBackupReadSlot(pgc->mem_subtractor_when_free, i, &subtractor);
			mtkfb_release_fence(ext_session_id, i, fence_idx);
		}
		MMProfileLogEx(ddp_mmp_get_events()->primary_ovl_fence_release, MMProfileFlagPulse, i, fence_idx);
	}
#endif

	if (userdata == DISP_SESSION_MEMORY) {
#ifndef MTK_FB_CMDQ_DISABLE
		layer = disp_sync_get_output_timeline_id();
		cmdqBackupReadSlot(pgc->cur_mem_config_fence, layer, &fence_idx);
		mtkfb_release_fence(ext_session_id, layer, fence_idx);
#endif
	}

	return ret;
}

static int _ovl_fence_release_callback(uint32_t userdata)
{
	int i = 0;
	unsigned int addr = 0;
	int ret = 0;
	unsigned int dsi_state[10];
	unsigned int rdma_state[50];

	MMProfileLogEx(ddp_mmp_get_events()->session_release, MMProfileFlagStart, 1, userdata);


	/* releaes OVL1 when primary setting */
	if (ovl_get_status() == DDP_OVL1_STATUS_PRIMARY_RELEASED)
		dpmgr_set_ovl1_status(DDP_OVL1_STATUS_SUB);
	else if (ovl_get_status() == DDP_OVL1_STATUS_PRIMARY_DISABLE)
		dpmgr_set_ovl1_status(DDP_OVL1_STATUS_IDLE);

#ifndef MTK_FB_CMDQ_DISABLE
	for (i = 0; i < PRIMARY_DISPLAY_SESSION_LAYER_COUNT; i++) {
		int fence_idx = 0;
		int subtractor = 0;

		if (i == primary_display_get_option("ASSERT_LAYER") && is_DAL_Enabled()) {
			mtkfb_release_layer_fence(primary_session_id, i);
		} else {
			cmdqBackupReadSlot(pgc->cur_config_fence, i, &fence_idx);
			cmdqBackupReadSlot(pgc->subtractor_when_free, i, &subtractor);
			mtkfb_release_fence(primary_session_id, i, fence_idx - subtractor);
		}
		MMProfileLogEx(ddp_mmp_get_events()->primary_ovl_fence_release, MMProfileFlagPulse,
			       i, fence_idx - subtractor);
	}

	if (primary_display_is_video_mode() == 1) {
		unsigned int ovl_status[2];

		cmdqBackupReadSlot(pgc->ovl_status_info, 0, &ovl_status[0]);
		cmdqBackupReadSlot(pgc->ovl_status_info, 1, &ovl_status[1]);
		DISPDBG("ovl fsm state:0x%x\n", ovl_status[0]);
		if ((ovl_status[1] & 0x1) != OVL_STATUS_IDLE)
			DISPERR("disp ovl status error, 0x%x, 0x%x\n", ovl_status[0],
				ovl_status[1]);
	}
#endif

	/* backup DSI state register to slot */
	if (gEnableDSIStateCheck == 1) {
		for (i = 0; i < 10; i++) {
			cmdqBackupReadSlot(pgc->dsi_state_info, i, &dsi_state[i]);

			if ((dsi_state[i] & 0x2) == 0x2) {
				DISPERR("disp DSI_STATE error, %d, 0x%x\n", i, dsi_state[i]);
				cmdqBackupReadSlot(pgc->rdma_state_info, i * 5 + 0,
						   &rdma_state[i * 5 + 0]);
				cmdqBackupReadSlot(pgc->rdma_state_info, i * 5 + 1,
						   &rdma_state[i * 5 + 1]);
				cmdqBackupReadSlot(pgc->rdma_state_info, i * 5 + 2,
						   &rdma_state[i * 5 + 2]);
				cmdqBackupReadSlot(pgc->rdma_state_info, i * 5 + 3,
						   &rdma_state[i * 5 + 3]);
				cmdqBackupReadSlot(pgc->rdma_state_info, i * 5 + 4,
						   &rdma_state[i * 5 + 4]);
				DISPERR("disp RDMA_STATE:%d, irq=0x%x, in/out=(%d,%d,%d,%d)\n",
					i, rdma_state[i * 5 + 0], rdma_state[i * 5 + 1],
					rdma_state[i * 5 + 2], rdma_state[i * 5 + 3], rdma_state[i * 5 + 4]);
			} else {
				DISPERR("disp DSI_STATE pass, %d, 0x%x\n", i, dsi_state[i]);
			}
		}
	}

	addr = ddp_ovl_get_cur_addr(!_should_config_ovl_input(), 0);
	if ((_is_decouple_mode(pgc->session_mode) == 0))
		update_frm_seq_info(addr, 0, 2, FRM_START);

#ifndef MTK_FB_CMDQ_DISABLE
	if (userdata == 5) {
		int fence_idx, subtractor, layer;

		layer = disp_sync_get_output_timeline_id();

		cmdqBackupReadSlot(pgc->cur_config_fence, layer, &fence_idx);
		cmdqBackupReadSlot(pgc->subtractor_when_free, layer, &subtractor);
		mtkfb_release_fence(primary_session_id, layer, fence_idx);
	}

	/* async callback, need to check if it is still decouple */
	if (_is_decouple_mode(pgc->session_mode) && !_is_mirror_mode(pgc->session_mode)
	    && (userdata == DISP_SESSION_DECOUPLE_MODE || userdata == 5)) {
		static cmdqRecHandle cmdq_handle;
		unsigned int rdma_pitch_sec, rdma_fmt;

		if (primary_get_state() != DISP_ALIVE)
			return 0;

		if (cmdq_handle == NULL)
			ret = cmdqRecCreate(CMDQ_SCENARIO_PRIMARY_DISP, &cmdq_handle);
		if (ret == 0) {
			cmdqBackupReadSlot(pgc->rdma_buff_info, 1, &(rdma_pitch_sec));
			rdma_pitch_sec = rdma_pitch_sec >> 30;
			if (rdma_pitch_sec == DISP_NORMAL_BUFFER)
				_primary_path_lock(__func__);
			cmdqRecReset(cmdq_handle);
			_cmdq_insert_wait_frame_done_token_mira(cmdq_handle);
			cmdqBackupReadSlot(pgc->rdma_buff_info, 0, &addr);
			decouple_rdma_config.address = addr;
			/*rdma pitch only use bit[15..0], we use bit[31:30] to store secure information */
			decouple_rdma_config.security = rdma_pitch_sec;
			_config_rdma_input_data(&decouple_rdma_config, pgc->dpmgr_handle,
						cmdq_handle);
			_cmdq_set_config_handle_dirty_mira(cmdq_handle);
			cmdqRecFlushAsyncCallback(cmdq_handle, NULL, 0);

			cmdqBackupReadSlot(pgc->rdma_buff_info, 2, &(rdma_fmt));
			decouple_rdma_config.inputFormat = rdma_fmt;

			if (rdma_pitch_sec == DISP_NORMAL_BUFFER)
				_primary_path_unlock(__func__);
			MMProfileLogEx(ddp_mmp_get_events()->primary_rdma_config,
				       MMProfileFlagPulse, 0, decouple_rdma_config.address);
			/* cmdqRecDestroy(cmdq_handle); */
		} else {
			/* ret = -1; */
			DISPERR("fail to create cmdq\n");
		}
	}
#endif

	return ret;
}

static int decouple_fence_release_kthread(void *data)
{
	int interface_fence = 0;
	int layer = 0;
	int ret = 0;

	struct sched_param param = {.sched_priority = 94 }; /* RTPM_PRIO_SCRN_UPDATE */

	sched_setscheduler(current, SCHED_RR, &param);


	for (;;) {
		wait_event_interruptible(decouple_fence_release_wq,
					 atomic_read(&decouple_fence_release_event));
		atomic_set(&decouple_fence_release_event, 0);

		/* async callback, need to check if it is still decouple */
		_primary_path_lock(__func__);
		if (pgc->session_mode == DISP_SESSION_DECOUPLE_MIRROR_MODE
		    && pgc->state != DISP_SLEPT) {
			static cmdqRecHandle cmdq_handle;
			unsigned int rdma_pitch_sec, rdma_fmt;

			if (cmdq_handle == NULL)
				ret = cmdqRecCreate(CMDQ_SCENARIO_PRIMARY_DISP, &cmdq_handle);
			if (ret == 0) {
				RDMA_CONFIG_STRUCT tmpConfig = decouple_rdma_config;

				cmdqRecReset(cmdq_handle);
				_cmdq_insert_wait_frame_done_token_mira(cmdq_handle);
				cmdqBackupReadSlot(pgc->rdma_buff_info, 0,
						   (uint32_t *)&(tmpConfig.address));

				/*rdma pitch only use bit[15..0], we use bit[31:30] to store secure information */
				cmdqBackupReadSlot(pgc->rdma_buff_info, 1, &(rdma_pitch_sec));
				tmpConfig.pitch = rdma_pitch_sec & ~(3 << 30);
				tmpConfig.security = rdma_pitch_sec >> 30;

				cmdqBackupReadSlot(pgc->rdma_buff_info, 2, &(rdma_fmt));
				tmpConfig.inputFormat = rdma_fmt;

				_config_rdma_input_data(&tmpConfig, pgc->dpmgr_handle, cmdq_handle);

				layer = disp_sync_get_output_timeline_id();
				cmdqBackupReadSlot(pgc->cur_config_fence, layer, &interface_fence);
				_cmdq_set_config_handle_dirty_mira(cmdq_handle);
				cmdqRecFlushAsyncCallback(cmdq_handle,
							  (CmdqAsyncFlushCB)_Interface_fence_release_callback,
							  interface_fence > 1 ? interface_fence - 1 : 0);
				MMProfileLogEx(ddp_mmp_get_events()->primary_rdma_config,
					       MMProfileFlagPulse, interface_fence,
					       decouple_rdma_config.address);

				/* dump rdma input if enabled */
				dprec_mmp_dump_rdma_layer(&tmpConfig, 0);

				/* cmdqRecDestroy(cmdq_handle); */
			} else {
				DISPERR("fail to create cmdq\n");
			}
		}
		_primary_path_unlock(__func__);

		if (kthread_should_stop())
			break;
	}

	return 0;
}

static int _olv_wdma_fence_release_callback(uint32_t userdata)
{
	_ovl_fence_release_callback(userdata);
	_wdma_fence_release_callback(userdata);
	return 0;
}

#if 0
/**
 * Defined but not used, avoid build warning. (2015.1.30 Rynn Wu)
 */
static int _fence_release_worker_thread(void *data)
{
	int i = 0;
	unsigned int addr = 0;
	unsigned int session_id = MAKE_DISP_SESSION(DISP_SESSION_PRIMARY, 0);

	struct sched_param param = { .sched_priority = RTPM_PRIO_SCRN_UPDATE };

	sched_setscheduler(current, SCHED_RR, &param);

	if (!_is_decouple_mode(pgc->session_mode))
		dpmgr_enable_event(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_START);
	else
		dpmgr_enable_event(pgc->ovl2mem_path_handle, DISP_PATH_EVENT_FRAME_START);

	while (1) {
		if (!_is_decouple_mode(pgc->session_mode))
			dpmgr_wait_event(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_START);
		else
			dpmgr_wait_event(pgc->ovl2mem_path_handle, DISP_PATH_EVENT_FRAME_START);

	if (is_hwc_enabled == 0)
		continue;

		/* releaes OVL1 when primary setting */
		if (ovl_get_status() == DDP_OVL1_STATUS_PRIMARY_RELEASED) {
			ovl_set_status(DDP_OVL1_STATUS_SUB);
			wake_up_interruptible(&ovl1_wait_queue);
		} else if (ovl_get_status() == DDP_OVL1_STATUS_PRIMARY_DISABLE) {
			ovl_set_status(DDP_OVL1_STATUS_IDLE);
			wake_up_interruptible(&ovl1_wait_queue);
		}

		for (i = 0; i < PRIMARY_DISPLAY_SESSION_LAYER_COUNT; i++) {
			int fence_idx, subtractor;

			if (i == primary_display_get_option("ASSERT_LAYER") && is_DAL_Enabled()) {
				mtkfb_release_layer_fence(session_id, i);
			} else {
				cmdqBackupReadSlot(pgc->cur_config_fence, i, &fence_idx);
				cmdqBackupReadSlot(pgc->subtractor_when_free, i, &subtractor);
				mtkfb_release_fence(session_id, i, fence_idx-subtractor);
			}
		}

		addr = ddp_ovl_get_cur_addr(!_should_config_ovl_input(), 0);
		if ((_is_decouple_mode(pgc->session_mode) == 0))
			update_frm_seq_info(addr, 0, 2, FRM_START);

		MMProfileLogEx(ddp_mmp_get_events()->session_release, MMProfileFlagEnd, 0, 0);

		if (kthread_should_stop())
			break;
	}

	return 0;
}
#endif

//add by LCT yufangfang for dev_info
#ifdef CONFIG_LCT_DEVINFO_SUPPORT
//#define SLT_DEVINFO_LCM_DEBUG
#include  <dev_info.h>
#include <linux/timer.h>
static int devinfo_first=0;
struct devinfo_struct s_DEVINFO_lcm[10];   //suppose 10 max lcm device 

extern LCM_DRIVER* lcm_driver_list[];
extern unsigned int lcm_count;

static void devinfo_lcm_reg(void)
{
	int i=0;
	for(i = 0;i < lcm_count;i++)
	{
#ifdef SLT_DEVINFO_LCM_DEBUG
		printk("[DEVINFO LCM]registe LCM device!num:<%d> type:<%s> module:<%s> vendor<%s> ic<%s> version<%s> info<%s> used<%s>\n",i,
				s_DEVINFO_lcm[i].device_type,s_DEVINFO_lcm[i].device_module,s_DEVINFO_lcm[i].device_vendor,s_DEVINFO_lcm[i].device_ic,
				s_DEVINFO_lcm[i].device_version,s_DEVINFO_lcm[i].device_info,s_DEVINFO_lcm[i].device_used);
#endif
		DEVINFO_CHECK_DECLARE(s_DEVINFO_lcm[i].device_type,s_DEVINFO_lcm[i].device_module,s_DEVINFO_lcm[i].device_vendor,
				s_DEVINFO_lcm[i].device_ic,s_DEVINFO_lcm[i].device_version,s_DEVINFO_lcm[i].device_info,s_DEVINFO_lcm[i].device_used);

	}
		//return 0;
}


void DISP_DEVINFO_LCM_get(const char* lcm_name)
{
	LCM_DRIVER *slt_lcm = NULL;
	int i;
	LCM_PARAMS slt_s_lcm_params= {0};
	LCM_PARAMS *slt_lcm_params= &slt_s_lcm_params;
	for(i = 0;i < lcm_count;i++)
	{
		slt_lcm_params = &slt_s_lcm_params;
		slt_lcm = lcm_driver_list[i];
		memset((void*)slt_lcm_params, 0, sizeof(LCM_PARAMS));
		slt_lcm->get_params(slt_lcm_params);

		s_DEVINFO_lcm[i].device_info=kmalloc(64,GFP_KERNEL);

			s_DEVINFO_lcm[i].device_type="LCM";
			s_DEVINFO_lcm[i].device_module=slt_lcm_params->module;
			s_DEVINFO_lcm[i].device_vendor=slt_lcm_params->vendor;
			s_DEVINFO_lcm[i].device_ic=slt_lcm_params->ic;
			s_DEVINFO_lcm[i].device_info=slt_lcm_params->info;
			//s_DEVINFO_lcm[i].device_comments=slt_lcm_params->comments;
			s_DEVINFO_lcm[i].device_version=DEVINFO_NULL;


			if(!strcmp(lcm_name,slt_lcm->name))
				s_DEVINFO_lcm[i].device_used=DEVINFO_USED;
			else
				s_DEVINFO_lcm[i].device_used=DEVINFO_UNUSED;

//			sprintf(s_DEVINFO_lcm[i].device_info,"%d * %d",slt_lcm_params->width,slt_lcm_params->height);
#ifdef SLT_DEVINFO_LCM_DEBUG
		printk("[DEVINFO LCM]Num:[%d] type:[%s] module:[%s] vendor:[%s] ic:[%s] info:[%s] used:[%s] \n",i,s_DEVINFO_lcm[i].device_type,
			s_DEVINFO_lcm[i].device_module,
			s_DEVINFO_lcm[i].device_vendor,
			s_DEVINFO_lcm[i].device_ic,
			s_DEVINFO_lcm[i].device_info,
			s_DEVINFO_lcm[i].device_used);
#endif
	}
	//return 0;
}	
#endif

static struct task_struct *present_fence_release_worker_task;

static int _present_fence_release_worker_thread(void *data)
{
	int ret = 0;
	disp_sync_info *layer_info = NULL;
	struct sched_param param = {.sched_priority = 87 }; /* RTPM_PRIO_FB_THREAD */
#ifdef CONFIG_LCT_DEVINFO_SUPPORT
	if(devinfo_first==0)
	{
		devinfo_first=1;
		devinfo_lcm_reg();
	}
#endif

	sched_setscheduler(current, SCHED_RR, &param);

	dpmgr_enable_event(pgc->dpmgr_handle, DISP_PATH_EVENT_IF_VSYNC);

	while (1) {
		ret = dpmgr_wait_event(pgc->dpmgr_handle, DISP_PATH_EVENT_IF_VSYNC);

		/* release present fence in vsync callback */
		{
			/* extern disp_sync_info *_get_sync_info(unsigned int session_id,
							      unsigned int timeline_id);
			 */
			int fence_increment = 0;

			/* if session not created, do not release present fence */
			if (pgc->session_id == 0) {
				MMProfileLogEx(ddp_mmp_get_events()->present_fence_release,
					       MMProfileFlagPulse, -1, 0x4a4a4a4a);
				/* DISPDBG("_get_sync_info fail in present_fence_release thread\n"); */
				continue;
			}

			layer_info = _get_sync_info(pgc->session_id, disp_sync_get_present_timeline_id());
			if (layer_info == NULL) {
				MMProfileLogEx(ddp_mmp_get_events()->present_fence_release,
					       MMProfileFlagPulse, -1, 0x5a5a5a5a);
				/* DISPERR("_get_sync_info fail in present_fence_release thread\n"); */
				continue;
			}

			_primary_path_lock(__func__);
			fence_increment = gPresentFenceIndex - layer_info->timeline->value;
			if (fence_increment > 0) {
				timeline_inc(layer_info->timeline, fence_increment);
				MMProfileLogEx(ddp_mmp_get_events()->present_fence_release,
					       MMProfileFlagPulse, gPresentFenceIndex,
					       fence_increment);
			}
			_primary_path_unlock(__func__);
			/* DISPPR_FENCE("RPF/%d/%d\n",
					gPresentFenceIndex, gPresentFenceIndex-layer_info->timeline->value);
			 */
		}
	}

	return 0;
}

int primary_display_capture_framebuffer_wdma(void *data)
{
	int ret = 0;
	cmdqRecHandle cmdq_handle = NULL;
	disp_ddp_path_config *pconfig = NULL;
	m4u_client_t *m4uClient = NULL;
	unsigned int w_xres = primary_display_get_width();
	unsigned int h_yres = primary_display_get_height();
	unsigned int pixel_byte = primary_display_get_dc_bpp() / 8; /* bpp is either 32 or 16, can not be other value */
	int buffer_size = h_yres * w_xres * pixel_byte;
	int buf_idx = 0;
	unsigned int mva[2] = { 0, 0 };
	void *va[2] = { NULL, NULL };
	unsigned int format = eRGBA8888;


	struct sched_param param = {.sched_priority = 87 }; /* RTPM_PRIO_FB_THREAD */

	sched_setscheduler(current, SCHED_RR, &param);

	va[0] = vmalloc(buffer_size);
	if (va[0] == NULL) {
		DISPMSG("wdma dump:Fail to alloc vmalloc 0\n");
		ret = -1;
		goto out;
	}
	va[1] = vmalloc(buffer_size);
	if (va[1] == NULL) {
		DISPMSG("wdma dump:Fail to alloc vmalloc 1\n");
		ret = -1;
		goto out;
	}
	m4uClient = m4u_create_client();
	if (m4uClient == NULL) {
		DISPMSG("wdma dump:Fail to alloc  m4uClient=0x%p\n", m4uClient);
		ret = -1;
		goto out;
	}

	ret = m4u_alloc_mva(m4uClient, M4U_PORT_DISP_WDMA0, (unsigned long)va[0], NULL, buffer_size,
			    M4U_PROT_READ | M4U_PROT_WRITE, 0, (unsigned int *)&mva[0]);
	if (ret != 0) {
		DISPMSG("wdma dump::Fail to allocate mva 0\n");
		ret = -1;
		goto out;
	}

	ret = m4u_alloc_mva(m4uClient, M4U_PORT_DISP_WDMA0, (unsigned long)va[1], NULL, buffer_size,
			    M4U_PROT_READ | M4U_PROT_WRITE, 0, (unsigned int *)&mva[1]);
	if (ret != 0) {
		DISPMSG("wdma dump::Fail to allocate mva 1\n");
		ret = -1;
		goto out;
	}
	if (primary_display_cmdq_enabled()) {
		/*create config thread */
		ret = cmdqRecCreate(CMDQ_SCENARIO_PRIMARY_DISP, &cmdq_handle);
		if (ret != 0) {
			DISPMSG("wdma dump:Fail to create primary cmdq handle for capture\n");
			ret = -1;
			goto out;
		}
		dpmgr_path_memout_clock(pgc->dpmgr_handle, 1);
		dpmgr_enable_event(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_START);
		cmdqRecReset(cmdq_handle);
		_cmdq_insert_wait_frame_done_token_mira(cmdq_handle);
		_primary_path_lock(__func__);
		dpmgr_path_add_memout(pgc->dpmgr_handle, ENGINE_OVL0, cmdq_handle);
		_primary_path_unlock(__func__);
		while (primary_dump_wdma) {

			ret = m4u_cache_sync(m4uClient, M4U_PORT_DISP_WDMA0, (unsigned long)va[buf_idx],
					     buffer_size, mva[buf_idx], M4U_CACHE_FLUSH_BY_RANGE);
			_primary_path_lock(__func__);
			pconfig = dpmgr_path_get_last_config(pgc->dpmgr_handle);
			pconfig->wdma_dirty = 1;
			pconfig->wdma_config.dstAddress = mva[buf_idx];
			pconfig->wdma_config.srcHeight = h_yres;
			pconfig->wdma_config.srcWidth = w_xres;
			pconfig->wdma_config.clipX = 0;
			pconfig->wdma_config.clipY = 0;
			pconfig->wdma_config.clipHeight = h_yres;
			pconfig->wdma_config.clipWidth = w_xres;
			pconfig->wdma_config.outputFormat = format;
			pconfig->wdma_config.useSpecifiedAlpha = 1;
			pconfig->wdma_config.alpha = 0xFF;
			pconfig->wdma_config.dstPitch = w_xres * DP_COLOR_BITS_PER_PIXEL(format) / 8;

			ret = dpmgr_path_config(pgc->dpmgr_handle, pconfig, cmdq_handle);
			pconfig->wdma_dirty = 0;
			_primary_path_unlock(__func__);
			_cmdq_set_config_handle_dirty_mira(cmdq_handle);
			_cmdq_flush_config_handle_mira(cmdq_handle, 0);
			ret = dpmgr_wait_event(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_START);

			/* pconfig->wdma_config.dstAddress = va[buf_idx++]; */
			/* DISPMSG("capture wdma\n"); */
			dprec_mmp_dump_wdma_layer(&pconfig->wdma_config, 0);
			buf_idx = buf_idx % 2;
			cmdqRecReset(cmdq_handle);
			_cmdq_insert_wait_frame_done_token_mira(cmdq_handle);
		}
		_primary_path_lock(__func__);
		dpmgr_path_remove_memout(pgc->dpmgr_handle, cmdq_handle);
		_primary_path_unlock(__func__);

		_cmdq_set_config_handle_dirty_mira(cmdq_handle);
		/* flush remove memory to cmdq */
		_cmdq_flush_config_handle_mira(cmdq_handle, 1);
		DISPMSG("wdma dump: Flush remove memout\n");

		dpmgr_path_memout_clock(pgc->dpmgr_handle, 0);

	}

out:
	cmdqRecDestroy(cmdq_handle);
	if (mva[0] > 0)
		m4u_dealloc_mva(m4uClient, M4U_PORT_DISP_WDMA0, mva[0]);

	if (mva[1] > 0)
		m4u_dealloc_mva(m4uClient, M4U_PORT_DISP_WDMA0, mva[1]);

	if (va[0] != NULL)
		vfree(va[0]);
	if (va[1] != NULL)
		vfree(va[1]);

	if (m4uClient != 0)
		m4u_destroy_client(m4uClient);
	DISPMSG("wdma dump:end\n");

	return ret;
}


int primary_display_switch_wdma_dump(int on)
{
	if (on && (!primary_dump_wdma)) {
		primary_dump_wdma = 1;
		primary_display_wdma_out = kthread_create(primary_display_capture_framebuffer_wdma, NULL,
							  "display_wdma_out");
		wake_up_process(primary_display_wdma_out);
	} else {
		primary_dump_wdma = 0;
	}
	return 0;
}

#define xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

int primary_display_set_frame_buffer_address(unsigned long va, unsigned long mva)
{

	/* DISPMSG("framebuffer va %lu, mva %lu\n", va, mva); */
	pgc->framebuffer_va = va;
	pgc->framebuffer_mva = mva;
	/*
	    int frame_buffer_size = ALIGN_TO(DISP_GetScreenWidth(), MTK_FB_ALIGNMENT) *
			      ALIGN_TO(DISP_GetScreenHeight(), MTK_FB_ALIGNMENT) * 4;
	    unsigned long dim_layer_va = va + 2*frame_buffer_size;
	    dim_layer_mva = mva + 2*frame_buffer_size;
	    memset_io(dim_layer_va, 0, frame_buffer_size);
	*/
	return 0;
}

unsigned long primary_display_get_frame_buffer_mva_address(void)
{
	return pgc->framebuffer_mva;
}

unsigned long primary_display_get_frame_buffer_va_address(void)
{
	return pgc->framebuffer_va;
}

int primary_display_get_session_mode(void)
{
	return pgc->session_mode;
}

int is_dim_layer(unsigned int long mva)
{
	if (mva == dim_layer_mva)
		return 1;
	return 0;
}

void *primary_get_dpmgr_handle(void)
{
	return pgc->dpmgr_handle;
}

unsigned long get_dim_layer_mva_addr(void)
{
	if (dim_layer_mva == 0) {
		int frame_buffer_size = ALIGN_TO(DISP_GetScreenWidth(),
						 MTK_FB_ALIGNMENT) * DISP_GetScreenHeight() * 4;

		dim_layer_mva = pgc->framebuffer_mva + 2 * frame_buffer_size;
		DISPMSG("init dim layer mva %lu, size %d", dim_layer_mva, frame_buffer_size);
	}
	return dim_layer_mva;
}

#ifndef MTK_FB_CMDQ_DISABLE
static int init_cmdq_slots(cmdqBackupSlotHandle *pSlot, int count, int init_val)
{
	int i;

	cmdqBackupAllocateSlot(pSlot, count);

	for (i = 0; i < count; i++)
		cmdqBackupWriteSlot(*pSlot, i, init_val);

	return 0;
}
#endif



int primary_display_init(char *lcm_name, unsigned int lcm_fps, int is_lcm_inited)
{
	DISP_STATUS ret = DISP_STATUS_OK;

#ifndef MTK_FB_DFO_DISABLE
	unsigned int lcm_fake_width = 0;
	unsigned int lcm_fake_height = 0;
#endif

	LCM_PARAMS *lcm_param = NULL;
	disp_ddp_path_config *data_config = NULL;

	pr_warn("[DISP]primary_display_init begin\n");

	dprec_init();
	dpmgr_init();

#ifndef MTK_FB_CMDQ_DISABLE
	init_cmdq_slots(&(pgc->cur_config_fence), DISP_SESSION_TIMELINE_COUNT, 0);
	init_cmdq_slots(&(pgc->subtractor_when_free), DISP_SESSION_TIMELINE_COUNT, 0);
	init_cmdq_slots(&(pgc->cur_mem_config_fence), DISP_SESSION_TIMELINE_COUNT, 0);
	init_cmdq_slots(&(pgc->mem_subtractor_when_free), DISP_SESSION_TIMELINE_COUNT, 0);
	init_cmdq_slots(&(pgc->rdma_buff_info), 3, 0);
	init_cmdq_slots(&(pgc->ovl_status_info), 2, 0);
	init_cmdq_slots(&(pgc->dsi_state_info), 10, 0);
	init_cmdq_slots(&(pgc->rdma_state_info), 50, 0);
#endif

#ifdef DISP_DUMP_EVENT_STATUS
	init_cmdq_slots(&(pgc->event_status), 6, 0);
#endif

	mutex_init(&(pgc->capture_lock));
	mutex_init(&(pgc->lock));
	mutex_init(&(pgc->cmd_lock));
	mutex_init(&(pgc->vsync_lock));
	mutex_init(&esd_mode_switch_lock);
#ifdef MTK_DISP_IDLE_LP
	mutex_init(&idle_lock);
#endif
#ifdef DISP_SWITCH_DST_MODE
	mutex_init(&(pgc->switch_dst_lock));
#endif
	_primary_path_lock(__func__);

//add by LCT yufangfang for lcm dev_info
#ifdef CONFIG_LCT_DEVINFO_SUPPORT
	DISP_DEVINFO_LCM_get(lcm_name);
#endif

	pgc->plcm = disp_lcm_probe(lcm_name, LCM_INTERFACE_NOTDEFINED, is_lcm_inited);

	if (pgc->plcm == NULL) {
		DISPERR("disp_lcm_probe returns null\n");
		ret = DISP_STATUS_ERROR;
		goto done;
	}
	//add this file for device info --sunsiyuan@wind-mobi.com add 20161129 begin
	#ifdef CONFIG_WIND_DEVICE_INFO
	g_lcm_name = (char *)pgc->plcm->drv->name;
	#endif
	//add this file for device info --sunsiyuan@wind-mobi.com add 20161129 end

#ifndef MTK_FB_DFO_DISABLE
	if ((0 == dfo_query("LCM_FAKE_WIDTH", &lcm_fake_width))
	    && (0 == dfo_query("LCM_FAKE_HEIGHT", &lcm_fake_height))) {
		pr_debug("[DFO] LCM_FAKE_WIDTH=%d, LCM_FAKE_HEIGHT=%d\n", lcm_fake_width,
			 lcm_fake_height);
		if (lcm_fake_width && lcm_fake_height) {
			if (DISP_STATUS_OK != primary_display_change_lcm_resolution(lcm_fake_width, lcm_fake_height))
				DISPMSG("[DISP\DFO]WARNING!!! Change LCM Resolution FAILED!!!\n");
		}
	}
#endif
	lcm_param = disp_lcm_get_params(pgc->plcm);

	if (lcm_param == NULL) {
		DISPERR("get lcm params FAILED\n");
		ret = DISP_STATUS_ERROR;
		goto done;
	}
#ifndef MTK_FB_CMDQ_DISABLE
	ret = cmdqCoreRegisterCB(CMDQ_GROUP_DISP, (CmdqClockOnCB)cmdqDdpClockOn, (CmdqDumpInfoCB)cmdqDdpDumpInfo,
				 (CmdqResetEngCB)cmdqDdpResetEng, (CmdqClockOffCB)cmdqDdpClockOff);
	if (ret) {
		DISPERR("cmdqCoreRegisterCB failed, ret=%d\n", ret);
		ret = DISP_STATUS_ERROR;
		goto done;
	}

	ret = cmdqRecCreate(CMDQ_SCENARIO_PRIMARY_DISP, &(pgc->cmdq_handle_config));
	if (ret) {
		DISPMSG("cmdqRecCreate FAIL, ret=%d\n", ret);
		ret = DISP_STATUS_ERROR;
		goto done;
	} else {
		DISPMSG("cmdqRecCreate SUCCESS, g_cmdq_handle=0x%p\n", pgc->cmdq_handle_config);
	}
	/*create ovl2mem path cmdq handle */
	ret = cmdqRecCreate(CMDQ_SCENARIO_DISP_COLOR, &(pgc->cmdq_handle_ovl1to2_config));
	if (ret != 0) {
		DISPERR("cmdqRecCreate FAIL, ret=%d\n", ret);
		return -1;
	}
	primary_display_use_cmdq = CMDQ_ENABLE;
#else
	primary_display_use_cmdq = CMDQ_DISABLE;
#endif

	/* debug for bus hang issue (need to remove) */
	ddp_dump_analysis(DISP_MODULE_CONFIG);
	if (primary_display_mode == DIRECT_LINK_MODE) {
		__build_path_direct_link();
		pgc->session_mode = DISP_SESSION_DIRECT_LINK_MODE;
		DISPMSG("primary display is DIRECT LINK MODE\n");
	} else if (primary_display_mode == DECOUPLE_MODE) {
		__build_path_decouple();
		pgc->session_mode = DISP_SESSION_DECOUPLE_MODE;

		DISPMSG("primary display is DECOUPLE MODE\n");
	} else if (primary_display_mode == SINGLE_LAYER_MODE) {
		__build_path_single_layer();

		DISPMSG("primary display is SINGLE LAYER MODE\n");
	} else if (primary_display_mode == DEBUG_RDMA1_DSI0_MODE) {
		__build_path_debug_rdma1_dsi0();

		DISPMSG("primary display is DEBUG RDMA1 DSI0 MODE\n");
	} else {
		DISPMSG("primary display mode is WRONG\n");
	}
#ifndef MTK_FB_CMDQ_DISABLE
	primary_display_use_cmdq = CMDQ_ENABLE;
#else
	primary_display_use_cmdq = CMDQ_DISABLE;
#endif

	/* dpmgr_path_init(pgc->dpmgr_handle, CMDQ_DISABLE); */
	dpmgr_path_set_video_mode(pgc->dpmgr_handle, primary_display_is_video_mode());
	/* dpmgr_path_init(pgc->dpmgr_handle, CMDQ_DISABLE); */

	/* use fake timer to generate vsync signal for cmd mode w/o LCM(originally using LCM TE Signal as VSYNC) */
	/* so we don't need to modify display driver's behavior. */
	if (disp_helper_get_option
	    (DISP_HELPER_OPTION_NO_LCM_FOR_LOW_POWER_MEASUREMENT)) {
		/* only for low power measurement */
		DISPMSG("WARNING!!!!!! FORCE NO LCM MODE!!!\n");
		islcmconnected = 0;

		/* no need to change video mode vsync behavior */
		if (!primary_display_is_video_mode()) {
			_init_vsync_fake_monitor(lcm_fps);

			dpmgr_map_event_to_irq(pgc->dpmgr_handle, DISP_PATH_EVENT_IF_VSYNC,
					       DDP_IRQ_UNKNOWN);
		}
	}

#ifdef CONFIG_FPGA_EARLY_PORTING
	dpmgr_path_reset(pgc->dpmgr_handle, CMDQ_DISABLE);
#endif

	if (primary_display_use_cmdq == CMDQ_ENABLE) {
		_cmdq_build_trigger_loop();
		_cmdq_start_trigger_loop();
		_cmdq_reset_config_handle();
		_cmdq_insert_wait_frame_done_token();
	}


	data_config = dpmgr_path_get_last_config(pgc->dpmgr_handle);

#ifdef OVL_CASCADE_SUPPORT
	if (ovl_get_status() == DDP_OVL1_STATUS_IDLE || ovl_get_status() == DDP_OVL1_STATUS_PRIMARY) {
		if (primary_display_mode == DECOUPLE_MODE)
			dpmgr_path_enable_cascade(pgc->ovl2mem_path_handle,
						  pgc->cmdq_handle_config);
		else
			dpmgr_path_enable_cascade(pgc->dpmgr_handle, pgc->cmdq_handle_config);
	}
#endif
	memcpy(&(data_config->dispif_config), lcm_param, sizeof(LCM_PARAMS));

	data_config->dst_w = lcm_param->width;
	data_config->dst_h = lcm_param->height;
	if (lcm_param->type == LCM_TYPE_DSI) {
		if (lcm_param->dsi.data_format.format == LCM_DSI_FORMAT_RGB888)
			data_config->lcm_bpp = 24;
		else if (lcm_param->dsi.data_format.format == LCM_DSI_FORMAT_RGB565)
			data_config->lcm_bpp = 16;
		else if (lcm_param->dsi.data_format.format == LCM_DSI_FORMAT_RGB666)
			data_config->lcm_bpp = 18;
	} else if (lcm_param->type == LCM_TYPE_DPI) {
		if (lcm_param->dpi.format == LCM_DPI_FORMAT_RGB888)
			data_config->lcm_bpp = 24;
		else if (lcm_param->dpi.format == LCM_DPI_FORMAT_RGB565)
			data_config->lcm_bpp = 16;
		if (lcm_param->dpi.format == LCM_DPI_FORMAT_RGB666)
			data_config->lcm_bpp = 18;
	}

	data_config->fps = lcm_fps;
	data_config->dst_dirty = 1;
#ifdef CONFIG_FPGA_EARLY_PORTING
	data_config->ovl_dirty = 1;
#endif

	rdma_set_target_line(DISP_MODULE_RDMA0, primary_display_get_height() * 1 / 2,
			     pgc->cmdq_handle_config);
	rdma_set_target_line(DISP_MODULE_RDMA0, primary_display_get_height() * 1 / 2, NULL);

	if (primary_display_use_cmdq == CMDQ_ENABLE) {
		ret = dpmgr_path_config(pgc->dpmgr_handle, data_config, pgc->cmdq_handle_config);

		/* should we set dirty here???????? */
		_cmdq_flush_config_handle(0, NULL, 0);

		_cmdq_reset_config_handle();
		_cmdq_insert_wait_frame_done_token();
	} else {
		ret = dpmgr_path_config(pgc->dpmgr_handle, data_config, NULL);
	}

	{
#ifdef MTK_NO_DISP_IN_LK
		ret = disp_lcm_init(pgc->plcm, 1);
#else
		ret = disp_lcm_init(pgc->plcm, 0);
#endif
	}

#ifndef MTK_NO_DISP_IN_LK
	if (_is_decouple_mode(pgc->session_mode))
#endif
		/* this should remove? for video mode when LK has start path */
		dpmgr_path_start(pgc->dpmgr_handle, CMDQ_DISABLE);
#ifdef MTK_FB_PULLCLK_ENABLE
	primary_display_pullclk_task = kthread_create(primary_display_vdo_pullclk_worker_kthread,
							NULL, "display_vdo_pullclk");
	wake_up_process(primary_display_pullclk_task);
#endif


#ifdef MTK_FB_ESD_ENABLE
	primary_display_esd_check_task = kthread_create(primary_display_esd_check_worker_kthread,
							NULL, "display_esd_check");
	init_waitqueue_head(&esd_ext_te_wq);
	init_waitqueue_head(&esd_check_task_wq);
	if (_need_do_esd_check())
		wake_up_process(primary_display_esd_check_task);
#if 0
	if (_need_do_esd_check())
		primary_display_esd_check_enable(1);
#endif

	if (_need_register_eint()) {
		struct device_node *node = NULL;
		int irq;
		u32 ints[2] = { 0, 0 };
#ifdef GPIO_DSI_TE_PIN
#ifndef CONFIG_FPGA_EARLY_PORTING
		/* 1.set GPIO107 eint mode */
		mt_set_gpio_mode(GPIO_DSI_TE_PIN, GPIO_DSI_TE_PIN_M_GPIO);
#endif
		eint_flag++;
#endif

#ifndef CONFIG_MTK_LEGACY
		eint_flag++;
#endif
		/* 2.register eint */
		node = of_find_compatible_node(NULL, NULL, "mediatek, DSI_TE_1-eint");
		if (node) {
			of_property_read_u32_array(node, "debounce", ints, ARRAY_SIZE(ints));
			/* FIXME: find definatition of  mt_gpio_set_debounce */
			/* mt_gpio_set_debounce(ints[0], ints[1]); */
			mt_eint_set_hw_debounce(ints[0], ints[1]);
			irq = irq_of_parse_and_map(node, 0);
			if (request_irq(irq, _esd_check_ext_te_irq_handler, IRQF_TRIGGER_RISING, "DSI_TE_1-eint", NULL))
				DISPMSG("[ESD]EINT IRQ LINE NOT AVAILABLE!!\n");
			else
				eint_flag++;

		} else {
			DISPMSG("[ESD][%s] can't find DSI_TE_1 eint compatible node\n", __func__);
		}
	}


	if (_need_do_esd_check())
		primary_display_esd_check_enable(1);

#endif
#ifdef DISP_SWITCH_DST_MODE
	primary_display_switch_dst_mode_task = kthread_create(_disp_primary_path_switch_dst_mode_thread, NULL,
							      "display_switch_dst_mode");
	wake_up_process(primary_display_switch_dst_mode_task);
#endif

	primary_path_aal_task = kthread_create(_disp_primary_path_check_trigger, NULL, "display_check_aal");
	wake_up_process(primary_path_aal_task);
#if 0 /* Zaikuo: disable it when HWC not enable to reduce error log */
	fence_release_worker_task =
	    kthread_create(_fence_release_worker_thread, NULL, "fence_worker");
	wake_up_process(fence_release_worker_task);
	if (_is_decouple_mode(pgc->session_mode)) {
		if_fence_release_worker_task =
		    kthread_create(_if_fence_release_worker_thread, NULL, "if_fence_worker");
		wake_up_process(if_fence_release_worker_task);

		ovl2mem_fence_release_worker_task =
		    kthread_create(_ovl2mem_fence_release_worker_thread, NULL,
				   "ovl2mem_fence_worker");
		wake_up_process(ovl2mem_fence_release_worker_task);
	}
#endif

	present_fence_release_worker_task = kthread_create(_present_fence_release_worker_thread, NULL,
							   "present_fence_worker");
	wake_up_process(present_fence_release_worker_task);

	if (primary_display_frame_update_task == NULL) {
		init_waitqueue_head(&primary_display_frame_update_wq);
		disp_register_module_irq_callback(DISP_MODULE_RDMA0, primary_display_frame_update_irq_callback);
		disp_register_module_irq_callback(DISP_MODULE_OVL0, primary_display_frame_update_irq_callback);
		disp_register_module_irq_callback(DISP_MODULE_WDMA0, primary_display_frame_update_irq_callback);
		primary_display_frame_update_task = kthread_create(primary_display_frame_update_kthread, NULL,
								   "frame_update_worker");
		wake_up_process(primary_display_frame_update_task);
		init_waitqueue_head(&decouple_fence_release_wq);
		decouple_fence_release_task = kthread_create(decouple_fence_release_kthread, NULL,
							     "decouple_fence_release_worker");
		wake_up_process(decouple_fence_release_task);
	}
	/* primary_display_use_cmdq = CMDQ_ENABLE; */

	/* this will be set to always enable cmdq later */
	if (primary_display_is_video_mode()) {
#ifdef DISP_SWITCH_DST_MODE
		primary_display_cur_dst_mode = 1;	/* video mode */
		primary_display_def_dst_mode = 1;	/* default mode is video mode */
#endif
		dpmgr_map_event_to_irq(pgc->dpmgr_handle, DISP_PATH_EVENT_IF_VSYNC, DDP_IRQ_RDMA0_DONE);
	} else {

	}

#ifndef MTKFB_NO_M4U
	{
		M4U_PORT_STRUCT sPort;

		sPort.ePortID = M4U_PORT_DISP_WDMA0;
		sPort.Virtuality = primary_display_use_m4u;
		sPort.Security = 0;
		sPort.Distance = 1;
		sPort.Direction = 0;
		ret = m4u_config_port(&sPort);
		if (ret != 0) {
			DISPMSG("config M4U Port %s to %s FAIL(ret=%d)\n",
				  ddp_get_module_name(DISP_MODULE_WDMA0),
				  primary_display_use_m4u ? "virtual" : "physical", ret);
			return -1;
		}
	}
#endif
	pgc->lcm_fps = lcm_fps;
	if (lcm_fps > 6000)	/* FIXME: if fps bigger than 60, support 8 layer? */
		pgc->max_layer = OVL_LAYER_NUM;
	else
		pgc->max_layer = OVL_LAYER_NUM;

	pgc->state = DISP_ALIVE;

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
	disp_switch_data.name = "disp";
	disp_switch_data.index = 0;
	disp_switch_data.state = DISP_ALIVE;
	ret = switch_dev_register(&disp_switch_data);
#endif

	primary_display_sodi_rule_init();

#ifdef MTK_DISP_IDLE_LP
	init_waitqueue_head(&idle_detect_wq);
	primary_display_idle_detect_task = kthread_create(_disp_primary_path_idle_detect_thread, NULL,
							  "display_idle_detect");
	wake_up_process(primary_display_idle_detect_task);
#endif

	pgc->dc_type = DISP_OUTPUT_DECOUPLE;

	if ((mmdvfs_get_mmdvfs_profile() == MMDVFS_PROFILE_D2_P_PLUS) ||
	    (mmdvfs_get_mmdvfs_profile() == MMDVFS_PROFILE_D2_M_PLUS))
		register_mmclk_switch_cb(primary_display_switch_mmsys_clk, _switch_mmsys_clk);
done:

	/* disable OVL TF in video mode, cause cmdq/sodi log is not ready, avoid too much TF issue */
	{
		/* extern unsigned int gDisableOVLTF; */

		if (gDisableOVLTF == 1 && primary_display_is_video_mode() == 1) {
			/* extern void disp_m4u_tf_disable(void); */
			disp_m4u_tf_disable();
		}
	}

#ifndef WDMA_PATH_CLOCK_DYNAMIC_SWITCH
#ifndef CONFIG_FPGA_EARLY_PORTING
	enable_soidle_by_bit(MT_CG_DISP0_DISP_WDMA0);
#endif
#endif
	g_is_inited = 1;
	_primary_path_unlock(__func__);
	/* DISPMSG("primary_display_init end\n"); */

	return ret;
}

int primary_display_deinit(void)
{
	_primary_path_lock(__func__);

	_cmdq_stop_trigger_loop();
	dpmgr_path_deinit(pgc->dpmgr_handle, CMDQ_DISABLE);
#ifndef CONFIG_MTK_CLKMGR
	ddp_clk_unprepare(DISP_MTCMOS_CLK);
#endif
	_primary_path_unlock(__func__);
	return 0;
}

/* register rdma done event */
int primary_display_wait_for_idle(void)
{
	DISP_STATUS ret = DISP_STATUS_OK;

	DISPFUNC();

	_primary_path_lock(__func__);
	/* Nothing to do ??? */
	_primary_path_unlock(__func__);
	return ret;
}

int primary_display_wait_for_dump(void)
{
	return 0; /* avoid build warning. */
}

int primary_display_release_fence_fake(void)
{
	unsigned int layer_en = 0;
	unsigned int addr = 0;
	unsigned int fence_idx = -1;
	unsigned int session_id = MAKE_DISP_SESSION(DISP_SESSION_PRIMARY, 0);
	int i = 0;

	DISPFUNC();

	for (i = 0; i < PRIMARY_DISPLAY_SESSION_LAYER_COUNT; i++) {
		if (i == primary_display_get_option("ASSERT_LAYER") && is_DAL_Enabled()) {
			mtkfb_release_layer_fence(session_id, 3);
		} else {
			disp_sync_get_cached_layer_info(session_id, i, &layer_en,
							(unsigned long *)&addr, &fence_idx);
			if (fence_idx < 0) {
				if (fence_idx == -1) {
					DISPPR_ERROR("find fence idx for layer %d,addr 0x%08x fail,\n", i, 0);
					DISPPR_ERROR("unregistered addr%d\n", fence_idx);
				} else if (fence_idx == -2) {
					;
				} else {
					DISPPR_ERROR("find fence idx for layer %d,addr 0x%08x fail,reason unknown%d\n",
						     i, 0, fence_idx);
				}
			} else {
				if (layer_en)
					mtkfb_release_fence(session_id, i, fence_idx - 1);
				else
					mtkfb_release_fence(session_id, i, fence_idx);
			}
		}
	}
	return 0;		/* avoid build warning */
}

int primary_display_wait_for_vsync(void *config)
{
	disp_session_vsync_config *c = (disp_session_vsync_config *) config;
	int ret = 0;
	unsigned long long ts = 0ULL;
	/* kick idle manager here to ensure sodi is disabled when screen update begin(not 100% ensure) */
	primary_display_idlemgr_kick((char *)__func__);

	_primary_path_vsync_lock();
	if (pgc->state == DISP_SLEPT) {
		DISPMSG("VSYNC DISP_SLEPT Return\n");
		_primary_path_vsync_unlock();
		return 0;
	}
#ifdef MTK_DISP_IDLE_LP
	_disp_primary_path_dsi_clock_on(0);
#endif
	if (!islcmconnected) {
		DISPMSG("lcm not connect, use fake vsync\n");
		/* msleep(16); */
		usleep_range(16000, 17000);
		goto done;
	}
	ret = dpmgr_wait_event_ts(pgc->dpmgr_handle, DISP_PATH_EVENT_IF_VSYNC, &ts);
	if (pgc->vsync_drop)
		ret = dpmgr_wait_event_ts(pgc->dpmgr_handle, DISP_PATH_EVENT_IF_VSYNC, &ts);

	if (ret != 0)
		DISPMSG("vsync signaled by unknown signal ret=%d\n", ret);

	c->vsync_ts = ts;
	c->vsync_cnt++;

done:
#ifdef MTK_DISP_IDLE_LP
	_disp_primary_path_dsi_clock_off(0);
#endif
	_primary_path_vsync_unlock();

	return 0;
}

unsigned int primary_display_get_ticket(void)
{
	return dprec_get_vsync_count();
}
#ifdef CONFIG_SINGLE_PANEL_OUTPUT
int primary_suspend_release_ovl_fence(disp_session_input_config *session_input)
{
	unsigned int session = (unsigned int)((DISP_SESSION_PRIMARY)<<16 | (0));
	unsigned int i = 0;
	unsigned int layer = 0;

	for (i = 0; i < session_input->config_layer_num; i++) {
		unsigned int cur_fence;
		disp_input_config *input_cfg = &session_input->config[i];

		layer = input_cfg->layer_id;
		cur_fence = input_cfg->next_buff_idx;
		if (cur_fence != -1)
			mtkfb_release_fence(session, i, cur_fence);
	}
	return 0;
}

int primary_suspend_clr_ovl_config(void)
{
	disp_ddp_path_config *data_config = NULL;
	disp_path_handle handle = NULL;
	OVL_CONFIG_STRUCT  ovl_config[OVL_LAYER_NUM];
	int i = 0;

	DISPFUNC();
	if (0 == g_suspend_flag)
		return -1;

	if (_is_decouple_mode(pgc->session_mode) == 0)
		handle = pgc->dpmgr_handle;
	else
		handle = pgc->ovl2mem_path_handle;

	data_config = dpmgr_path_get_last_config(handle);
	memset((void *)&(data_config->ovl_config), 0, sizeof(ovl_config));
	last_primary_config = *data_config;

	for (i = 0; i < OVL_LAYER_NUM; i++)
		ovl_layer_switch(DISP_MODULE_OVL0, i, 0, NULL);
	DISP_REG_SET(NULL, DISP_REG_OVL_SRC_CON, 0x0);

	return 0;
}
#endif

int primary_suspend_release_fence(void)
{
	unsigned int session = (unsigned int)((DISP_SESSION_PRIMARY) << 16 | (0));
	unsigned int i = 0;

	for (i = 0; i < HW_OVERLAY_COUNT; i++) {
		DISPMSG("mtkfb_release_layer_fence  session=0x%x,layerid=%d\n", session, i);
		mtkfb_release_layer_fence(session, i);
	}
	return 0;
}

static void primary_display_suspend_wait_tui(void)
{
	while (primary_get_state() == DISP_BLANK) {
		_primary_path_vsync_unlock();
		_primary_path_unlock(__func__);
		_primary_path_esd_check_unlock();
		disp_sw_mutex_unlock(&(pgc->capture_lock));
#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
		switch_set_state(&disp_switch_data, DISP_SLEPT);
#endif
		primary_display_wait_state(DISP_ALIVE, MAX_SCHEDULE_TIMEOUT);

		disp_sw_mutex_lock(&(pgc->capture_lock));
		_primary_path_esd_check_lock();
		_primary_path_lock(__func__);
		_primary_path_vsync_lock();
		DISPMSG("primary_display_suspend wait tui done stat=%d\n", primary_get_state());
	}
}

int primary_display_suspend(void)
{
	DISP_STATUS ret = DISP_STATUS_OK;

	DISPMSG("primary_display_suspend begin\n");
	MMProfileLogEx(ddp_mmp_get_events()->primary_suspend, MMProfileFlagStart, 0, 0);
#ifdef DISP_SWITCH_DST_MODE
	primary_display_switch_dst_mode(primary_display_def_dst_mode);
#endif
	disp_sw_mutex_lock(&(pgc->capture_lock));
	_primary_path_esd_check_lock();
	_primary_path_lock(__func__);
	_primary_path_vsync_lock();

	DISPMSG("wait tui finish[begin]\n");
	primary_display_suspend_wait_tui();
	DISPMSG("wait tui finish[end]\n");

	DISPMSG("leave freeze mode[begin]\n");
	display_freeze_mode(0, 0);
	DISPMSG("leave freeze mode[end]\n");

	if (pgc->state == DISP_SLEPT) {
		DISPMSG("primary display path is already sleep, skip\n");
		goto done;
	}
#ifdef MTK_DISP_IDLE_LP
	_disp_primary_path_exit_idle(__func__, 0);
#endif

	MMProfileLogEx(ddp_mmp_get_events()->primary_suspend, MMProfileFlagPulse, 0, 1);
	/* msleep(16); */ /* wait last frame done */
	usleep_range(16000, 17000);
	if (dpmgr_path_is_busy(pgc->dpmgr_handle)) {
		int event_ret = 0;

		MMProfileLogEx(ddp_mmp_get_events()->primary_suspend, MMProfileFlagPulse, 1, 2);
		event_ret = dpmgr_wait_event_timeout(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_DONE, HZ * 1);
		MMProfileLogEx(ddp_mmp_get_events()->primary_suspend, MMProfileFlagPulse, 2, 2);
		DISPMSG("[POWER]primary display path is busy now, wait frame done, event_ret=%d\n", event_ret);
		if (event_ret <= 0) {
			DISPERR("wait frame done in suspend timeout\n");
			MMProfileLogEx(ddp_mmp_get_events()->primary_suspend, MMProfileFlagPulse, 3, 2);
			primary_display_diagnose();
			ret = -1;
		}
	}
	/* for decouple mode */
	if (_is_decouple_mode(pgc->session_mode)) {
		if (dpmgr_path_is_busy(pgc->ovl2mem_path_handle))
			dpmgr_wait_event_timeout(pgc->ovl2mem_path_handle, DISP_PATH_EVENT_FRAME_COMPLETE, HZ);

		/* xuecheng, BAD WROKAROUND for decouple mode */
		msleep(30);
	}

	MMProfileLogEx(ddp_mmp_get_events()->primary_suspend, MMProfileFlagPulse, 0, 2);
	DISPMSG("[POWER]display cmdq trigger loop stop[begin]\n");
	_cmdq_stop_trigger_loop();
	DISPMSG("[POWER]display cmdq trigger loop stop[end]\n");
	MMProfileLogEx(ddp_mmp_get_events()->primary_suspend, MMProfileFlagPulse, 0, 3);

#ifdef CONFIG_LCM_SEND_CMD_IN_VIDEO
	DISPMSG("[POWER]lcm suspend[begin]\n");
	disp_lcm_suspend(pgc->plcm);
	DISPMSG("[POWER]lcm suspend[end]\n");
#endif

	DISPMSG("[POWER]primary display path stop[begin]\n");
	dpmgr_path_stop(pgc->dpmgr_handle, CMDQ_DISABLE);
	DISPMSG("[POWER]primary display path stop[end]\n");
	MMProfileLogEx(ddp_mmp_get_events()->primary_suspend, MMProfileFlagPulse, 0, 4);

#ifdef	CONFIG_SINGLE_PANEL_OUTPUT
	primary_suspend_clr_ovl_config();
#endif

	if (dpmgr_path_is_busy(pgc->dpmgr_handle)) {
		MMProfileLogEx(ddp_mmp_get_events()->primary_suspend, MMProfileFlagPulse, 1, 4);
		DISPERR("[POWER]stop display path failed, still busy\n");
		dpmgr_path_reset(pgc->dpmgr_handle, CMDQ_DISABLE);
		ret = -1;
		/* even path is busy(stop fail), we still need to continue power off other module/devices */
		/* goto done; */
	}
	MMProfileLogEx(ddp_mmp_get_events()->primary_suspend, MMProfileFlagPulse, 0, 5);

	/* remove OVL1 from display if there is more than one session, */
	/* because the other session may use OVL1 in suspend mode */
#if 0
	{
		/* extern int disp_get_session_number(void); */
		if (ovl_get_status() != DDP_OVL1_STATUS_SUB && disp_get_session_number() > 1) {
			unsigned int dp_handle;
			unsigned int cmdq_handle;

			DISPMSG("disable cascade before suspend!\n");
			dpmgr_path_get_handle(&dp_handle, &cmdq_handle);
			dpmgr_path_disable_cascade(dp_handle, CMDQ_DISABLE);
			if (ovl_get_status() == DDP_OVL1_STATUS_SUB_REQUESTING) {
				ovl_set_status(DDP_OVL1_STATUS_SUB);
				wake_up_interruptible(&ovl1_wait_queue);
			} else {
				ovl_set_status(DDP_OVL1_STATUS_IDLE);
			}
			_cmdq_build_trigger_loop();
		}
	}
#else
	DISPMSG("disable cascade before suspend!\n");
	if (_is_decouple_mode(pgc->session_mode) == 0)
		dpmgr_path_disable_cascade(pgc->dpmgr_handle, CMDQ_DISABLE);
	else
		dpmgr_path_disable_cascade(pgc->ovl2mem_path_handle, CMDQ_DISABLE);

	if (ovl_get_status() == DDP_OVL1_STATUS_SUB_REQUESTING)
		dpmgr_set_ovl1_status(DDP_OVL1_STATUS_SUB);
	else if (ovl_get_status() != DDP_OVL1_STATUS_SUB)
		dpmgr_set_ovl1_status(DDP_OVL1_STATUS_IDLE);
#endif

#ifndef CONFIG_LCM_SEND_CMD_IN_VIDEO
	DISPMSG("[POWER]lcm suspend[begin]\n");
	disp_lcm_suspend(pgc->plcm);
	DISPMSG("[POWER]lcm suspend[end]\n");
#endif
	MMProfileLogEx(ddp_mmp_get_events()->primary_suspend, MMProfileFlagPulse, 0, 6);
	DISPMSG("[POWER]primary display path Release Fence[begin]\n");
	primary_suspend_release_fence();
	DISPMSG("[POWER]primary display path Release Fence[end]\n");
	MMProfileLogEx(ddp_mmp_get_events()->primary_suspend, MMProfileFlagPulse, 0, 7);

	DISPMSG("[POWER]dpmanager path power off[begin]\n");
	dpmgr_path_power_off(pgc->dpmgr_handle, CMDQ_DISABLE);
	DISPMSG("[POWER]dpmanager path power off[end]\n");

	if (_is_decouple_mode(pgc->session_mode)) {
		dpmgr_path_power_off(pgc->ovl2mem_path_handle, CMDQ_DISABLE);
	} else if (is_mmdvfs_supported() && mmdvfs_get_mmdvfs_profile() == MMDVFS_PROFILE_D1_PLUS) {
		DISPMSG("set MMDVFS low\n");
		mmdvfs_set_step(MMDVFS_SCEN_DISP, MMDVFS_VOLTAGE_LOW); /* Vote to LPM mode */
	}

#ifndef CONFIG_MTK_CLKMGR
	ddp_clk_unprepare(DISP_MTCMOS_CLK);
#endif
	MMProfileLogEx(ddp_mmp_get_events()->primary_suspend, MMProfileFlagPulse, 0, 8);
	display_path_idle_cnt = 0;

	pgc->state = DISP_SLEPT;
done:
	_primary_path_vsync_unlock();
	_primary_path_unlock(__func__);
	_primary_path_esd_check_unlock();
	disp_sw_mutex_unlock(&(pgc->capture_lock));
#ifndef DISP_NO_AEE
	aee_kernel_wdt_kick_Powkey_api("mtkfb_early_suspend", WDT_SETBY_Display);
#endif
	primary_trigger_cnt = 0;
	MMProfileLogEx(ddp_mmp_get_events()->primary_suspend, MMProfileFlagEnd, 0, 0);
	DISPMSG("primary_display_suspend end\n");
	return ret;
}

int primary_display_get_lcm_index(void)
{
    //liujinzhou@wind-mobi.com modify at 20170311 begin
	int index = 0;
	#ifndef CONFIG_WIND_DEVICE_INFO
	DISPFUNC();
	if (pgc->plcm == NULL) {
		DISPERR("lcm handle is null\n");
		return 0;
	}
	index = pgc->plcm->index;
	DISPMSG("lcm index = %d\n", index);
	#else
	g_lcm_name = (char *)pgc->plcm->drv->name;
	
     if(!strcmp(g_lcm_name,"hx8394f_hd720_dsi_vdo_boe"))
        {
         index = 1;
	}
        else 
         index = 0;
	printk("liujinzhou--%d\n",index);
	#endif
	return index;
    //liujinzhou@wind-mobi.com modify at 20170311 end
}

int primary_display_resume(void)
{
	DISP_STATUS ret = DISP_STATUS_OK;

	DISPFUNC();
	MMProfileLogEx(ddp_mmp_get_events()->primary_resume, MMProfileFlagStart, 0, 0);

	_primary_path_lock(__func__);
	if (pgc->state != DISP_SLEPT) {
		DISPMSG("primary display path is not in suspend(state:%d), skip\n", pgc->state);
		goto done;
	}
	MMProfileLogEx(ddp_mmp_get_events()->primary_resume, MMProfileFlagPulse, 0, 1);

#ifdef CONFIG_MTK_CLKMGR

#ifdef CONFIG_SINGLE_PANEL_OUTPUT
	dpmgr_reset_module_handle(pgc->dpmgr_handle);
#endif
	/* For CCF, we move the power on control to noirq_restore in mtkfb */
	DISPMSG("dpmanager path power on[begin]\n");
	dpmgr_path_power_on(pgc->dpmgr_handle, CMDQ_DISABLE);
	DISPMSG("dpmanager path power on[end]\n");

	if (_is_decouple_mode(pgc->session_mode) && !pgc->force_on_wdma_path)
		dpmgr_path_power_on(pgc->ovl2mem_path_handle, CMDQ_DISABLE);

	MMProfileLogEx(ddp_mmp_get_events()->primary_resume, MMProfileFlagPulse, 0, 2);
#endif

#ifndef CONFIG_MTK_CLKMGR
	ddp_clk_prepare(DISP_MTCMOS_CLK);
#endif

	if (is_ipoh_bootup) {
		DISPMSG("[primary display path] leave primary_display_resume -- IPOH\n");
		DISPMSG("ESD check start[begin]\n");
		primary_display_esd_check_enable(1);
		DISPMSG("ESD check start[end]\n");
		is_ipoh_bootup = false;
		DISPMSG("[POWER]start cmdq[begin]--IPOH\n");
		_cmdq_start_trigger_loop();
		DISPMSG("[POWER]start cmdq[end]--IPOH\n");
		pgc->state = DISP_ALIVE;
		goto done;
	}

#ifndef CONFIG_MTK_CLKMGR

#ifdef CONFIG_SINGLE_PANEL_OUTPUT
	dpmgr_reset_module_handle(pgc->dpmgr_handle);
#endif

	DISPMSG("dpmanager path power on[begin]\n");
	dpmgr_path_power_on(pgc->dpmgr_handle, CMDQ_DISABLE);
	DISPMSG("dpmanager path power on[end]\n");

	if (_is_decouple_mode(pgc->session_mode) && !pgc->force_on_wdma_path)
		dpmgr_path_power_on(pgc->ovl2mem_path_handle, CMDQ_DISABLE);

	MMProfileLogEx(ddp_mmp_get_events()->primary_resume, MMProfileFlagPulse, 0, 2);
#endif

	if (primary_display_is_video_mode()) {
		DISPMSG("mutex0 clear[begin]\n");
		ddp_mutex_clear(0, NULL);
		DISPMSG("mutex0 clear[end]\n");
	}
	DISPMSG("[POWER]dpmanager re-init[begin]\n");

	{
		LCM_PARAMS *lcm_param = NULL;
		disp_ddp_path_config *data_config = NULL;

		dpmgr_path_connect(pgc->dpmgr_handle, CMDQ_DISABLE);
		if (_is_decouple_mode(pgc->session_mode))
			dpmgr_path_connect(pgc->ovl2mem_path_handle, CMDQ_DISABLE);

		MMProfileLogEx(ddp_mmp_get_events()->primary_resume, MMProfileFlagPulse, 1, 2);
		lcm_param = disp_lcm_get_params(pgc->plcm);

		data_config = dpmgr_path_get_last_config(pgc->dpmgr_handle);

		data_config->dst_w = lcm_param->width;
		data_config->dst_h = lcm_param->height;
		if (lcm_param->type == LCM_TYPE_DSI) {
			if (lcm_param->dsi.data_format.format == LCM_DSI_FORMAT_RGB888)
				data_config->lcm_bpp = 24;
			else if (lcm_param->dsi.data_format.format == LCM_DSI_FORMAT_RGB565)
				data_config->lcm_bpp = 16;
			else if (lcm_param->dsi.data_format.format == LCM_DSI_FORMAT_RGB666)
				data_config->lcm_bpp = 18;
		} else if (lcm_param->type == LCM_TYPE_DPI) {
			if (lcm_param->dpi.format == LCM_DPI_FORMAT_RGB888)
				data_config->lcm_bpp = 24;
			else if (lcm_param->dpi.format == LCM_DPI_FORMAT_RGB565)
				data_config->lcm_bpp = 16;
			if (lcm_param->dpi.format == LCM_DPI_FORMAT_RGB666)
				data_config->lcm_bpp = 18;
		}

		data_config->fps = pgc->lcm_fps;
		data_config->dst_dirty = 1;

		ret = dpmgr_path_config(pgc->dpmgr_handle, data_config, NULL);
		MMProfileLogEx(ddp_mmp_get_events()->primary_resume, MMProfileFlagPulse, 2, 2);

		if (_is_decouple_mode(pgc->session_mode)) {
			data_config = dpmgr_path_get_last_config(pgc->ovl2mem_path_handle);

			data_config->fps = pgc->lcm_fps;
			data_config->dst_dirty = 1;
			data_config->wdma_dirty = 1;
			ret = dpmgr_path_config(pgc->ovl2mem_path_handle, data_config, NULL);
			MMProfileLogEx(ddp_mmp_get_events()->primary_resume, MMProfileFlagPulse, 2,
				       2);
		}
		data_config->dst_dirty = 0;
	}
	DISPMSG("[POWER]dpmanager re-init[end]\n");
	MMProfileLogEx(ddp_mmp_get_events()->primary_resume, MMProfileFlagPulse, 0, 3);

	DISPMSG("[POWER]lcm resume[begin]\n");
	disp_lcm_resume(pgc->plcm);
	DISPMSG("[POWER]lcm resume[end]\n");

	MMProfileLogEx(ddp_mmp_get_events()->primary_resume, MMProfileFlagPulse, 0, 4);
	if (dpmgr_path_is_busy(pgc->dpmgr_handle)) {
		MMProfileLogEx(ddp_mmp_get_events()->primary_resume, MMProfileFlagPulse, 1, 4);
		DISPERR("[POWER]Fatal error, we didn't start display path but it's already busy\n");
		ret = -1;
		/* goto done; */
	}

	MMProfileLogEx(ddp_mmp_get_events()->primary_resume, MMProfileFlagPulse, 0, 5);
	DISPMSG("[POWER]dpmgr path start[begin]\n");
	dpmgr_path_start(pgc->dpmgr_handle, CMDQ_DISABLE);
	if (_is_decouple_mode(pgc->session_mode))
		dpmgr_path_start(pgc->ovl2mem_path_handle, CMDQ_DISABLE);

	DISPMSG("[POWER]dpmgr path start[end]\n");

	MMProfileLogEx(ddp_mmp_get_events()->primary_resume, MMProfileFlagPulse, 0, 6);
	if (dpmgr_path_is_busy(pgc->dpmgr_handle)) {
		MMProfileLogEx(ddp_mmp_get_events()->primary_resume, MMProfileFlagPulse, 1, 6);
		DISPERR
		    ("[POWER]Fatal error, we didn't trigger display path but it's already busy\n");
		ret = -1;
		/* goto done; */
	}

	MMProfileLogEx(ddp_mmp_get_events()->primary_resume, MMProfileFlagPulse, 0, 7);
	if (primary_display_is_video_mode()) {
		MMProfileLogEx(ddp_mmp_get_events()->primary_resume, MMProfileFlagPulse, 1, 7);
		/* for video mode, we need to force trigger here */
		/* for cmd mode, just set DPREC_EVENT_CMDQ_SET_EVENT_ALLOW when trigger loop start */
		dpmgr_path_trigger(pgc->dpmgr_handle, NULL, CMDQ_DISABLE);

		/* insert a wait token to make sure first config after resume will config to HW when HW idle */
		_cmdq_insert_wait_frame_done_token();
	}
	MMProfileLogEx(ddp_mmp_get_events()->primary_resume, MMProfileFlagPulse, 0, 8);

	DISPMSG("[POWER]start cmdq[begin]\n");
	_cmdq_start_trigger_loop();
	DISPMSG("[POWER]start cmdq[end]\n");
	MMProfileLogEx(ddp_mmp_get_events()->primary_resume, MMProfileFlagPulse, 0, 9);

	if (!primary_display_is_video_mode()) {
		/*refresh black picture of ovl bg */
		DISPMSG("[POWER]triggger cmdq[begin]\n");
		_trigger_display_interface(0, NULL, 0);
		DISPMSG("[POWER]triggger cmdq[end]\n");
	}

	DISPMSG("[POWER]wait frame done[begin]\n");
	dpmgr_wait_event_timeout(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_DONE, HZ / 10);
	DISPMSG("[POWER]wait frame done[end]\n");

	/* reinit fake timer for debug, we can enable option then press powerkey to enable thsi feature. */
	/* use fake timer to generate vsync signal for cmd mode w/o LCM(originally using LCM TE Signal as VSYNC) */
	/* so we don't need to modify display driver's behavior. */
	if (disp_helper_get_option
	    (DISP_HELPER_OPTION_NO_LCM_FOR_LOW_POWER_MEASUREMENT)) {
		/* only for low power measurement */
		DISPMSG("WARNING!!!!!! FORCE NO LCM MODE!!!\n");
		islcmconnected = 0;

		/* no need to change video mode vsync behavior */
		if (!primary_display_is_video_mode()) {
			_init_vsync_fake_monitor(pgc->lcm_fps);

			dpmgr_map_event_to_irq(pgc->dpmgr_handle, DISP_PATH_EVENT_IF_VSYNC,
					       DDP_IRQ_UNKNOWN);
		}
	}

	if (is_mmdvfs_supported() && mmdvfs_get_mmdvfs_profile() == MMDVFS_PROFILE_D1_PLUS &&
		primary_display_get_width() > 800) {
		DISPMSG("set MMDVFS high\n");
		mmdvfs_set_step(MMDVFS_SCEN_DISP, MMDVFS_VOLTAGE_HIGH); /* Enter HPM mode */
	}

#if 0
	DISPMSG("[POWER]wakeup aal/od trigger process[begin]\n");
	wake_up_process(primary_path_aal_task);
	DISPMSG("[POWER]wakeup aal/od trigger process[end]\n");
#endif
	pgc->state = DISP_ALIVE;
	pgc->force_on_wdma_path = 0;
#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
	switch_set_state(&disp_switch_data, DISP_ALIVE);
#endif

done:
	_primary_path_unlock(__func__);

	/* primary_display_diagnose(); */
#ifndef DISP_NO_AEE
	aee_kernel_wdt_kick_Powkey_api("mtkfb_late_resume", WDT_SETBY_Display);
#endif
	MMProfileLogEx(ddp_mmp_get_events()->primary_resume, MMProfileFlagEnd, 0, 0);

	ddp_dump_analysis(DISP_MODULE_OVL0);
#if defined(OVL_CASCADE_SUPPORT)
	ddp_dump_analysis(DISP_MODULE_OVL1);
#endif
	ddp_dump_analysis(DISP_MODULE_RDMA0);

	return 0;
}

int primary_display_resume_ovl2mem(void)
{
	DISP_STATUS ret = DISP_STATUS_OK;

	DISPFUNC();

#ifdef CONFIG_MTK_CLKMGR
	if (_is_decouple_mode(pgc->session_mode))
		dpmgr_path_power_on(pgc->ovl2mem_path_handle, CMDQ_DISABLE);
#endif

#ifndef CONFIG_MTK_CLKMGR
	ddp_clk_prepare(DISP_MTCMOS_CLK);
#endif

#ifndef CONFIG_MTK_CLKMGR
	if (_is_decouple_mode(pgc->session_mode))
		dpmgr_path_power_on(pgc->ovl2mem_path_handle, CMDQ_DISABLE);
#endif

	DISPMSG("[POWER]dpmanager re-init[begin]\n");
	{
		LCM_PARAMS *lcm_param = NULL;
		disp_ddp_path_config *data_config = NULL;

		if (_is_decouple_mode(pgc->session_mode))
			dpmgr_path_connect(pgc->ovl2mem_path_handle, CMDQ_DISABLE);

		lcm_param = disp_lcm_get_params(pgc->plcm);

		data_config = dpmgr_path_get_last_config(pgc->dpmgr_handle);

		data_config->dst_w = lcm_param->width;
		data_config->dst_h = lcm_param->height;
		if (lcm_param->type == LCM_TYPE_DSI) {
			if (lcm_param->dsi.data_format.format == LCM_DSI_FORMAT_RGB888)
				data_config->lcm_bpp = 24;
			else if (lcm_param->dsi.data_format.format == LCM_DSI_FORMAT_RGB565)
				data_config->lcm_bpp = 16;
			else if (lcm_param->dsi.data_format.format == LCM_DSI_FORMAT_RGB666)
				data_config->lcm_bpp = 18;
		} else if (lcm_param->type == LCM_TYPE_DPI) {
			if (lcm_param->dpi.format == LCM_DPI_FORMAT_RGB888)
				data_config->lcm_bpp = 24;
			else if (lcm_param->dpi.format == LCM_DPI_FORMAT_RGB565)
				data_config->lcm_bpp = 16;
			if (lcm_param->dpi.format == LCM_DPI_FORMAT_RGB666)
				data_config->lcm_bpp = 18;
		}

		data_config->fps = pgc->lcm_fps;
		data_config->dst_dirty = 1;

		if (_is_decouple_mode(pgc->session_mode)) {
			data_config = dpmgr_path_get_last_config(pgc->ovl2mem_path_handle);

			data_config->fps = pgc->lcm_fps;
			data_config->dst_dirty = 1;
			data_config->wdma_dirty = 1;
			ret = dpmgr_path_config(pgc->ovl2mem_path_handle, data_config, NULL);
		}
	}

	DISPMSG("[POWER]ovl2mem path start[begin]\n");
	if (_is_decouple_mode(pgc->session_mode))
		dpmgr_path_start(pgc->ovl2mem_path_handle, CMDQ_DISABLE);
	DISPMSG("[POWER]ovl2mem path start[end]\n");

	DISPMSG("[POWER]start cmdq[begin]\n");
	_cmdq_start_trigger_loop();
	DISPMSG("[POWER]start cmdq[end]\n");

	if (is_mmdvfs_supported() && mmdvfs_get_mmdvfs_profile() == MMDVFS_PROFILE_D1_PLUS &&
		primary_display_get_width() > 800) {
		DISPMSG("set MMDVFS high\n");
		mmdvfs_set_step(MMDVFS_SCEN_DISP, MMDVFS_VOLTAGE_HIGH); /* Enter HPM mode */
	}

	/* primary_display_diagnose(); */
#ifndef DISP_NO_AEE
	aee_kernel_wdt_kick_Powkey_api("mtkfb_late_resume", WDT_SETBY_Display);
#endif
	MMProfileLogEx(ddp_mmp_get_events()->primary_resume, MMProfileFlagEnd, 0, 0);

	ddp_dump_analysis(DISP_MODULE_OVL0);
	pgc->force_on_wdma_path = 1;
	return 0;
}

int primary_display_ipoh_restore(void)
{
	DISPMSG("primary_display_ipoh_restore In\n");
	DISPMSG("ESD check stop[begin]\n");
	primary_display_esd_check_enable(0);
	DISPMSG("ESD check stop[end]\n");
	if (NULL != pgc->cmdq_handle_trigger) {
		struct TaskStruct *pTask = pgc->cmdq_handle_trigger->pRunningTask;

		if (NULL != pTask) {
			DISPMSG("[Primary_display]display cmdq trigger loop stop[begin]\n");
			_cmdq_stop_trigger_loop();
			DISPMSG("[Primary_display]display cmdq trigger loop stop[end]\n");
		}
	}
	DISPMSG("primary_display_ipoh_restore Out\n");
	return 0;
}
//tuwenzan@wind-mobi.com add for init cabc ctrl backlight at 20170105 begin
#ifdef CONFIG_WIND_CABC_MODE_SUPPORT
static unsigned int cabc_mode_value = 0;
int primary_recognition_cabc_mode(void)
{
	return cabc_mode_value;
}
EXPORT_SYMBOL_GPL(primary_recognition_cabc_mode)

int _set_cabc_by_cmdq(unsigned int enbale)
{
	int ret = 0;
	cmdqRecHandle cmdq_handle_cabc = NULL;

	ret = cmdqRecCreate(CMDQ_SCENARIO_PRIMARY_DISP, &cmdq_handle_cabc);
	DISPDBG("primary cabc, handle=%p\n", cmdq_handle_cabc);
	if (ret != 0) 
	{
		DISPERR("fail to create primary cmdq handle for cabc\n");
		return -1;
	}

	if (primary_display_is_video_mode()) 
	{
		cmdqRecReset(cmdq_handle_cabc);
		disp_lcm_set_cabc(pgc->plcm, cmdq_handle_cabc, enbale);
		_cmdq_flush_config_handle_mira(cmdq_handle_cabc, 1);
		//DISPCHECK("[BL]_set_cabc_by_cmdq ret=%d\n", ret);
	} 
	else 
	{
		cmdqRecReset(cmdq_handle_cabc);
		cmdqRecWait(cmdq_handle_cabc, CMDQ_SYNC_TOKEN_CABC_EOF);
		//_cmdq_handle_clear_dirty(cmdq_handle_cabc);
		_cmdq_insert_wait_frame_done_token_mira(cmdq_handle_cabc);
		disp_lcm_set_cabc(pgc->plcm, cmdq_handle_cabc, enbale);
		cmdqRecSetEventToken(cmdq_handle_cabc, CMDQ_SYNC_TOKEN_CONFIG_DIRTY);
		cmdqRecSetEventToken(cmdq_handle_cabc, CMDQ_SYNC_TOKEN_CABC_EOF);
		_cmdq_flush_config_handle_mira(cmdq_handle_cabc, 1);
		//DISPCHECK("[BL]_set_cabc_by_cmdq ret=%d\n", ret);
	}
	cmdqRecDestroy(cmdq_handle_cabc);
	cmdq_handle_cabc = NULL;
	return ret;
}

int primary_display_setcabc(unsigned int enbale)
{
	//int ret = 0;
	static unsigned int last_enable;
	cabc_mode_value = enbale;
	if (last_enable == enbale)
	return 0;

	//_primary_path_switch_dst_lock();
	_primary_path_lock(__func__);

	if (pgc->state == DISP_SLEPT) 
	{
		DISPERR("Sleep State set cabc invald\n");
	}
	else 
	{
		//primary_display_idlemgr_kick(__func__, 0);
		primary_display_idlemgr_kick((char *)__func__);
		if (primary_display_cmdq_enabled()) 
			{
				if (primary_display_is_video_mode()) 
					{
						disp_lcm_set_cabc(pgc->plcm, NULL, enbale);
					} 
					else 
					{
						_set_cabc_by_cmdq(enbale);
					}
			}
		last_enable = enbale;
	}

	_primary_path_unlock(__func__);
	//_primary_path_switch_dst_unlock();

	return 0;
}
EXPORT_SYMBOL_GPL(primary_display_setcabc)

#endif
//add by lct yufangfang for cabc mode setting
#ifdef CONFIG_LCT_CABC_MODE_SUPPORT
static unsigned int cabc_mode_value = 0;
int primary_recognition_cabc_mode(void)
{
	return cabc_mode_value;
}
EXPORT_SYMBOL_GPL(primary_recognition_cabc_mode)

int _set_cabc_by_cmdq(unsigned int enbale)
{
	int ret = 0;
	cmdqRecHandle cmdq_handle_cabc = NULL;

	ret = cmdqRecCreate(CMDQ_SCENARIO_PRIMARY_DISP, &cmdq_handle_cabc);
	DISPDBG("primary cabc, handle=%p\n", cmdq_handle_cabc);
	if (ret != 0) 
	{
		DISPERR("fail to create primary cmdq handle for cabc\n");
		return -1;
	}

	if (primary_display_is_video_mode()) 
	{
		cmdqRecReset(cmdq_handle_cabc);
		disp_lcm_set_cabc(pgc->plcm, cmdq_handle_cabc, enbale);
		_cmdq_flush_config_handle_mira(cmdq_handle_cabc, 1);
		//DISPCHECK("[BL]_set_cabc_by_cmdq ret=%d\n", ret);
	} 
	else 
	{
		cmdqRecReset(cmdq_handle_cabc);
		cmdqRecWait(cmdq_handle_cabc, CMDQ_SYNC_TOKEN_CABC_EOF);
		//_cmdq_handle_clear_dirty(cmdq_handle_cabc);
		_cmdq_insert_wait_frame_done_token_mira(cmdq_handle_cabc);
		disp_lcm_set_cabc(pgc->plcm, cmdq_handle_cabc, enbale);
		cmdqRecSetEventToken(cmdq_handle_cabc, CMDQ_SYNC_TOKEN_CONFIG_DIRTY);
		cmdqRecSetEventToken(cmdq_handle_cabc, CMDQ_SYNC_TOKEN_CABC_EOF);
		_cmdq_flush_config_handle_mira(cmdq_handle_cabc, 1);
		//DISPCHECK("[BL]_set_cabc_by_cmdq ret=%d\n", ret);
	}
	cmdqRecDestroy(cmdq_handle_cabc);
	cmdq_handle_cabc = NULL;

	return ret;
}

int primary_display_setcabc(unsigned int enbale)
{
	//int ret = 0;
	static unsigned int last_enable;
	cabc_mode_value = enbale;
	if (last_enable == enbale)
	return 0;

	//_primary_path_switch_dst_lock();
	_primary_path_lock(__func__);

	if (pgc->state == DISP_SLEPT) 
	{
		DISPERR("Sleep State set cabc invald\n");
	}
	else 
	{
		//primary_display_idlemgr_kick(__func__, 0);
		primary_display_idlemgr_kick((char *)__func__);
		if (primary_display_cmdq_enabled()) 
			{
				if (primary_display_is_video_mode()) 
					{
						disp_lcm_set_cabc(pgc->plcm, NULL, enbale);
					} 
					else 
					{
						_set_cabc_by_cmdq(enbale);
					}
			}
		last_enable = enbale;
	}

	_primary_path_unlock(__func__);
	//_primary_path_switch_dst_unlock();

	return 0;
}
EXPORT_SYMBOL_GPL(primary_display_setcabc)

#endif


int primary_display_ipoh_recover(void)
{
	DISPMSG("%s In\n", __func__);
	_cmdq_start_trigger_loop();
	DISPMSG("%s Out\n", __func__);
	return 0;
}

int primary_display_start(void)
{
	DISP_STATUS ret = DISP_STATUS_OK;

	DISPFUNC();

	_primary_path_lock(__func__);
	dpmgr_path_start(pgc->dpmgr_handle, CMDQ_DISABLE);

	if (dpmgr_path_is_busy(pgc->dpmgr_handle)) {
		DISPMSG("Fatal error, we didn't trigger display path but it's already busy\n");
		ret = -1;
		goto done;
	}

done:
	_primary_path_unlock(__func__);
	return ret;
}

int primary_display_stop(void)
{
	DISP_STATUS ret = DISP_STATUS_OK;

	DISPFUNC();
	_primary_path_lock(__func__);

	if (dpmgr_path_is_busy(pgc->dpmgr_handle))
		dpmgr_wait_event_timeout(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_DONE, HZ * 1);


	dpmgr_path_stop(pgc->dpmgr_handle, CMDQ_DISABLE);

	if (dpmgr_path_is_busy(pgc->dpmgr_handle)) {
		DISPMSG("stop display path failed, still busy\n");
		ret = -1;
		goto done;
	}

done:
	_primary_path_unlock(__func__);
	return ret;
}

static int primary_display_remove_output(void *callback, unsigned int userdata)
{
	int ret = 0;
	static cmdqRecHandle cmdq_handle;
	static cmdqRecHandle cmdq_wait_handle;

	/*create config thread */
	if (cmdq_handle == NULL)
		ret = cmdqRecCreate(CMDQ_SCENARIO_PRIMARY_DISP, &cmdq_handle);

	if (ret == 0) {
		/* capture thread wait wdma sof */
		ret = cmdqRecCreate(CMDQ_SCENARIO_DISP_MIRROR_MODE, &cmdq_wait_handle);
		if (ret == 0) {
			cmdqRecReset(cmdq_wait_handle);
			cmdqRecWait(cmdq_wait_handle, CMDQ_EVENT_DISP_WDMA0_SOF);
			cmdqRecFlush(cmdq_wait_handle);
			/* cmdqRecDestroy(cmdq_wait_handle); */
		} else {
			DISPERR("fail to create  wait handle\n");
		}
		cmdqRecReset(cmdq_handle);
		_cmdq_insert_wait_frame_done_token_mira(cmdq_handle);

		/* update output fence */
		cmdqRecBackupUpdateSlot(cmdq_handle, pgc->cur_config_fence,
					disp_sync_get_output_timeline_id(), mem_config.buff_idx);

		dpmgr_path_remove_memout(pgc->dpmgr_handle, cmdq_handle);

		cmdqRecClearEventToken(cmdq_handle, CMDQ_EVENT_DISP_WDMA0_SOF);
		_cmdq_set_config_handle_dirty_mira(cmdq_handle);
		cmdqRecFlushAsyncCallback(cmdq_handle, callback, 0);
		pgc->need_trigger_ovl1to2 = 0;
		/* cmdqRecDestroy(cmdq_handle); */
	} else {
		ret = -1;
		DISPERR("fail to remove memout out\n");
	}
	return ret;
}

static bool is_multipass_trigger;

static int primary_display_config_input_multiple_nolock(disp_session_input_config *session_input);
static int primary_display_config_output_nolock(disp_mem_output_config *output, unsigned int session_id);


static int primary_display_merge_session_cmd(disp_session_config *config)
{
	disp_session_input_config *session_input;
	disp_mem_output_config *primary_output;
	unsigned int output_type = config->dc_type;

	if (DISP_OUTPUT_UNKNOWN == output_type)
		output_type = DISP_OUTPUT_DECOUPLE;

	pgc->dc_type = output_type;

#ifdef CONFIG_ALL_IN_TRIGGER_STAGE
	mutex_lock(&session_config_mutex);
	session_input = &cached_session_input[config->type - 1];
	cached_session_input[config->type - 1] = captured_session_input[config->type - 1];
	cached_session_input[config->type - 1].session_id = config->session_id;

	if (output_type == DISP_OUTPUT_MEMORY || is_multipass_trigger)
		cached_session_output[config->type - 1] = captured_session_output[config->type - 1];


	mutex_unlock(&session_config_mutex);

	session_input->config_layer_num = OVL_LAYER_NUM;
	if (isAEEEnabled) {
		int aee_layer = primary_display_get_option("ASSERT_LAYER");

		/*
		   For multipass mode, the AEE layer can only be enabled at final pass when AEE enabled.
		   Otherwise the AEE layer would be composed several time.
		 */
		session_input->setter = SESSION_USER_AEE;
		if (config->type == DISP_SESSION_PRIMARY) {
			if (output_type == DISP_OUTPUT_MEMORY) {
				session_input->config[aee_layer].layer_enable = 0;
				DISPMSG("set the aee layer:%d en to 0\n",
					session_input->config[aee_layer].layer_id);
			} else {
				session_input->config[aee_layer].layer_enable = 1;
			}
		}
	}

	primary_display_config_input_multiple_nolock(session_input);
	if (output_type == DISP_OUTPUT_MEMORY || pgc->session_mode == DISP_SESSION_DECOUPLE_MIRROR_MODE ||
	    config->type == DISP_SESSION_MEMORY) {
		mutex_lock(&session_config_mutex);
		cached_session_output[config->type - 1] = captured_session_output[config->type - 1];
		mutex_unlock(&session_config_mutex);
		primary_output = &cached_session_output[config->type - 1];
		primary_display_config_output_nolock(primary_output, config->session_id);
	}
#endif
	return 0;
}

void primary_display_update_present_fence(unsigned int fence_idx)
{
	gPresentFenceIndex = fence_idx;
#ifdef CONFIG_SINGLE_PANEL_OUTPUT
	if (pgc->state == DISP_SLEPT)
		primary_suspend_release_present_fence();
#endif
}

static int config_wdma_output(disp_path_handle disp_handle, cmdqRecHandle cmdq_handle,
			disp_mem_output_config *output, int is_multipass);

static int _primary_display_trigger(int blocking, void *callback, unsigned int userdata, int lock)
{
	int ret = 0;

	if (lock)
		_primary_path_lock(__func__);
	last_primary_trigger_time = sched_clock();
#ifdef DISP_SWITCH_DST_MODE
	if (is_switched_dst_mode) {
		primary_display_switch_dst_mode(1); /* swith to vdo mode if trigger disp */
		is_switched_dst_mode = false;
	}
#endif
	if (gTriggerDispMode > 0) {
		primary_display_release_fence_fake();
		_primary_path_unlock(__func__);
		return ret;
	}

	primary_trigger_cnt++;

	if (pgc->state == DISP_SLEPT) {
		DISPMSG("%s, skip because primary dipslay is sleep\n", __func__);
		goto done;
	}

	if (blocking)
		DISPMSG("%s, change blocking to non blocking trigger\n", __func__);

#ifdef MTK_DISP_IDLE_LP
	_disp_primary_path_exit_idle(__func__, 0);
#endif
	dprec_logger_start(DPREC_LOGGER_PRIMARY_TRIGGER, pgc->session_mode, pgc->dc_type);

	if (pgc->session_mode == DISP_SESSION_DIRECT_LINK_MODE) {
		_trigger_display_interface(blocking, _ovl_fence_release_callback,
					   DISP_SESSION_DIRECT_LINK_MODE);
	} else if (pgc->session_mode == DISP_SESSION_DIRECT_LINK_MIRROR_MODE) {
		_trigger_display_interface(0, _ovl_fence_release_callback,
					   DISP_SESSION_DIRECT_LINK_MIRROR_MODE);
		if (pgc->need_trigger_ovl1to2 == 0) {
			DISPPR_ERROR("There is no output config when directlink mirror!!\n");
		} else {
			primary_display_remove_output(_wdma_fence_release_callback,
						      DISP_SESSION_DIRECT_LINK_MIRROR_MODE);
			pgc->need_trigger_ovl1to2 = 0;
		}
	} else if (pgc->session_mode == DISP_SESSION_DECOUPLE_MIRROR_MODE) {
		if (pgc->need_trigger_dcMirror_out == 0) {
			if (cached_session_output[DISP_SESSION_PRIMARY - 1].security == DISP_SECURE_BUFFER)
				config_wdma_output(pgc->ovl2mem_path_handle, pgc->cmdq_handle_ovl1to2_config,
					&cached_session_output[DISP_SESSION_PRIMARY - 1], 0);
			DISPPR_ERROR("There is no output config when decouple mirror!!\n");
		}
		is_output_buffer_set = 0;
		pgc->need_trigger_dcMirror_out = 0;
		_trigger_ovl_to_memory_mirror(pgc->ovl2mem_path_handle,
					      pgc->cmdq_handle_ovl1to2_config,
					      (fence_release_callback)
					      _olv_wdma_fence_release_callback,
					      DISP_SESSION_DECOUPLE_MIRROR_MODE);
	} else if (pgc->session_mode == DISP_SESSION_DECOUPLE_MODE) {
		if (pgc->dc_type == DISP_OUTPUT_DECOUPLE) {
			uint32_t writing_mva = 0;

			if (primary_display_is_secure_path(DISP_SESSION_PRIMARY)) {
				pgc->session_buf_id++;
				pgc->session_buf_id %= DISP_INTERNAL_BUFFER_COUNT;
				writing_mva = pgc->session_buf[pgc->session_buf_id];
				mem_config.security = DISP_SECURE_BUFFER;
				decouple_wdma_config.security = DISP_SECURE_BUFFER;
			} else {
				pgc->dc_buf_id++;
				pgc->dc_buf_id %= DISP_INTERNAL_BUFFER_COUNT;
				writing_mva = pgc->dc_buf[pgc->dc_buf_id];
				mem_config.security = DISP_NORMAL_BUFFER;
				decouple_wdma_config.security = DISP_NORMAL_BUFFER;
			}

			decouple_wdma_config.dstAddress = writing_mva;
			mem_config.addr = writing_mva;
			mem_config.fmt = decouple_wdma_config.outputFormat;
			mem_config.pitch = decouple_wdma_config.dstPitch;
			_config_wdma_output(&decouple_wdma_config, pgc->ovl2mem_path_handle,
					    pgc->cmdq_handle_ovl1to2_config);
			MMProfileLogEx(ddp_mmp_get_events()->primary_wdma_config,
				       MMProfileFlagPulse, pgc->dc_buf_id, writing_mva);
			DISPMSG("%s, config wdma address 0x%lx\n", __func__,
				decouple_wdma_config.dstAddress);
			/*
			 * At the last round of multipass, the session mode would be configured as DECOUPLE MODE.
			 * Need to set data as DISP_SESSION_DECOUPLE_MULTIPASS_MODE to _ovl_fence_release_callback.
			 * Then the multipass output buffer fence would be release in _ovl_fence_release_callback.
			 */
			if (is_multipass_trigger) {
				_trigger_ovl_to_memory(pgc->ovl2mem_path_handle, pgc->cmdq_handle_ovl1to2_config,
						       (fence_release_callback)_ovl_fence_release_callback, 5, 0);
				is_multipass_trigger = 0;
			} else {
				if (primary_display_is_secure_path(DISP_SESSION_PRIMARY)) {
					_trigger_ovl_to_memory(pgc->ovl2mem_path_handle,
							       pgc->cmdq_handle_ovl1to2_config,
							       (fence_release_callback)_ovl_fence_release_callback,
							       DISP_SESSION_DECOUPLE_MODE, 1);
					_ovl_fence_release_callback(DISP_SESSION_DECOUPLE_MODE);
				} else {
					_trigger_ovl_to_memory(pgc->ovl2mem_path_handle,
							       pgc->cmdq_handle_ovl1to2_config,
						       (fence_release_callback)_ovl_fence_release_callback,
						       DISP_SESSION_DECOUPLE_MODE, 0);
				}
			}
		} else if (pgc->dc_type == DISP_OUTPUT_MEMORY) {
			is_multipass_trigger = 1;
			_trigger_ovl_to_memory(pgc->ovl2mem_path_handle, pgc->cmdq_handle_ovl1to2_config, NULL, 0, 0);
		}
	}

	dprec_logger_done(DPREC_LOGGER_PRIMARY_TRIGGER, 0, 0);

done:
	if (lock)
		_primary_path_unlock(__func__);
	/* FIXME: find aee_kernel_Powerkey_is_press definitation */
#ifndef DISP_NO_AEE
	if ((primary_trigger_cnt > PRIMARY_DISPLAY_TRIGGER_CNT) && aee_kernel_Powerkey_is_press()) {
		aee_kernel_wdt_kick_Powkey_api("primary_display_trigger", WDT_SETBY_Display);
		primary_trigger_cnt = 0;
	}
#endif

	if (pgc->session_id > 0)
		update_frm_seq_info(0, 0, 0, FRM_TRIGGER);
	return ret;
}

int primary_display_trigger(int blocking, void *callback, unsigned int userdata)
{
	return _primary_display_trigger(blocking, callback, userdata, 1);
}

int primary_display_trigger_nolock(int blocking, void *callback, unsigned int userdata)
{
	return _primary_display_trigger(blocking, callback, userdata, 0);
}


int primary_display_memory_trigger_nolock(int blocking, void *callback, unsigned int userdata)
{
	int ret = 0;

	if (pgc->state == DISP_SLEPT) {
		DISPMSG("%s, primary dipslay is sleep\n", __func__);
		if (pgc->force_on_wdma_path == 0)
			primary_display_resume_ovl2mem();
	}

	if (pgc->session_mode == DISP_SESSION_DECOUPLE_MODE || pgc->session_mode == DISP_SESSION_DECOUPLE_MIRROR_MODE) {
		if (primary_display_is_secure_path(DISP_SESSION_MEMORY)) {
			_trigger_ovl_to_memory(pgc->ovl2mem_path_handle, pgc->cmdq_handle_ovl1to2_config,
					       (fence_release_callback)_ovl_ext_fence_release_callback,
					       DISP_SESSION_MEMORY, 1);
			_ovl_ext_fence_release_callback(DISP_SESSION_MEMORY);
		} else {
			_trigger_ovl_to_memory(pgc->ovl2mem_path_handle, pgc->cmdq_handle_ovl1to2_config,
					       (fence_release_callback)_ovl_ext_fence_release_callback,
					       DISP_SESSION_MEMORY, 0);
		}
	} else {
		DISPMSG("Not support memory trigger using session_mode:%d\n", pgc->session_mode);
	}

	return ret;
}

void primary_display_trigger_and_merge(disp_session_config *config, int session_id)
{
	_primary_path_lock(__func__);
	primary_display_merge_session_cmd(config);

	if (DISP_SESSION_TYPE(session_id) == DISP_SESSION_PRIMARY)
		primary_display_trigger_nolock(0, NULL, 0);
	else if (DISP_SESSION_TYPE(session_id) == DISP_SESSION_MEMORY)
		primary_display_memory_trigger_nolock(0, NULL, 0);
	else
		DISPERR("Invalid session trigger\n");
	_primary_path_unlock(__func__);
}

static int primary_display_ovl2mem_callback(unsigned int userdata)
{
	disp_ddp_path_config *data_config = NULL;
	unsigned int session_id = 0;
	int fence_idx = userdata;

	WDMA0_FRAME_START_FLAG = 0;

	session_id = MAKE_DISP_SESSION(DISP_SESSION_PRIMARY, 0);
	data_config = dpmgr_path_get_last_config(pgc->dpmgr_handle);

	if (data_config) {
		WDMA_CONFIG_STRUCT wdma_layer;

		wdma_layer.dstAddress = mtkfb_query_buf_mva(session_id, 4, fence_idx);
		wdma_layer.outputFormat = data_config->wdma_config.outputFormat;
		wdma_layer.srcWidth = primary_display_get_width();
		wdma_layer.srcHeight = primary_display_get_height();
		wdma_layer.dstPitch = data_config->wdma_config.dstPitch;
		dprec_mmp_dump_wdma_layer(&wdma_layer, 0);
	}

	if (fence_idx > 0)
		mtkfb_release_fence(session_id, EXTERNAL_DISPLAY_SESSION_LAYER_COUNT, fence_idx);

#ifdef OVL_CASCADE_SUPPORT
	if (fence_idx < NEW_BUF_IDX && ALL_LAYER_DISABLE_STEP == 1) {
		DISPMSG("primary and memout does not match!!\n");
		cmdqCoreSetEvent(CMDQ_EVENT_DISP_WDMA0_SOF);
		if (_should_set_cmdq_dirty())
			_cmdq_set_config_handle_dirty_mira(pgc->cmdq_handle_ovl1to2_config);

		if ((fence_idx + 1) == NEW_BUF_IDX)
			ALL_LAYER_DISABLE_STEP = 0;
	}
#endif
	DISPMSG("mem_out release fence idx:0x%x\n", fence_idx);

	return 0;
}

/* extern unsigned int gDumpMemoutCmdq; */
int primary_display_mem_out_trigger(int blocking, void *callback, unsigned int userdata)
{
	int ret = 0;

	/* DISPFUNC(); */
	if (pgc->state == DISP_SLEPT || !_is_mirror_mode(pgc->session_mode)) {
		DISPMSG("mem out trigger is already slept or is not mirror mode(%d)\n",
			pgc->session_mode);
		return 0;
	}
	/* /dprec_logger_start(DPREC_LOGGER_PRIMARY_TRIGGER, 0, 0); */

	/* if(blocking) */
	_primary_path_lock(__func__);

	if (pgc->need_trigger_ovl1to2 == 0)
		goto done;


	NEW_BUF_IDX = userdata;
	ALL_LAYER_DISABLE_STEP = 0;

	if (_should_wait_path_idle())
		dpmgr_wait_event_timeout(pgc->ovl2mem_path_handle, DISP_PATH_EVENT_FRAME_DONE,
					 HZ * 1);

	if (_should_trigger_path())
		;/* /dpmgr_path_trigger(pgc->dpmgr_handle, NULL, primary_display_cmdq_enabled()); */

	if (_should_set_cmdq_dirty())
		_cmdq_set_config_handle_dirty_mira(pgc->cmdq_handle_ovl1to2_config);

	if (gDumpMemoutCmdq == 1) {
		DISPMSG("primary_display_mem_out_trigger, dump before flush 1:\n");
		cmdqRecDumpCommand(pgc->cmdq_handle_ovl1to2_config);
	}

	if (_should_flush_cmdq_config_handle())
		_cmdq_flush_config_handle_mira(pgc->cmdq_handle_ovl1to2_config, 0);

	if (_should_reset_cmdq_config_handle())
		cmdqRecReset(pgc->cmdq_handle_ovl1to2_config);

	cmdqRecWait(pgc->cmdq_handle_ovl1to2_config, CMDQ_EVENT_DISP_WDMA0_SOF);
	WDMA0_FRAME_START_FLAG = 1;
	_cmdq_insert_wait_frame_done_token_mira(pgc->cmdq_handle_ovl1to2_config);
	dpmgr_path_remove_memout(pgc->ovl2mem_path_handle, pgc->cmdq_handle_ovl1to2_config);

	if (gDumpMemoutCmdq == 1) {
		DISPMSG("primary_display_mem_out_trigger, dump before flush 2:\n");
		cmdqRecDumpCommand(pgc->cmdq_handle_ovl1to2_config);
	}

	if (_should_flush_cmdq_config_handle())
		/* _cmdq_flush_config_handle_mira(pgc->cmdq_handle_ovl1to2_config, 0); */
		cmdqRecFlushAsyncCallback(pgc->cmdq_handle_ovl1to2_config,
					  (CmdqAsyncFlushCB)primary_display_ovl2mem_callback,
					  userdata);

	if (_should_reset_cmdq_config_handle())
		cmdqRecReset(pgc->cmdq_handle_ovl1to2_config);

	/* _cmdq_insert_wait_frame_done_token_mira(pgc->cmdq_handle_ovl1to2_config); */

done:

	pgc->need_trigger_ovl1to2 = 0;

	_primary_path_unlock(__func__);

	/* dprec_logger_done(DPREC_LOGGER_PRIMARY_TRIGGER, 0, 0); */

	return ret;
}


static int config_wdma_output(disp_path_handle disp_handle,
			      cmdqRecHandle cmdq_handle,
			      disp_mem_output_config *output, int is_multipass)
{
	disp_ddp_path_config *pconfig = NULL;

	ASSERT(output != NULL);
	pconfig = dpmgr_path_get_last_config(disp_handle);
	pconfig->wdma_config.dstAddress = output->addr;
	pconfig->wdma_config.srcHeight = output->h;
	pconfig->wdma_config.srcWidth = output->w;
	pconfig->wdma_config.clipX = output->x;
	pconfig->wdma_config.clipY = output->y;
	pconfig->wdma_config.clipHeight = output->h;
	pconfig->wdma_config.clipWidth = output->w;
	pconfig->wdma_config.outputFormat = output->fmt;
	pconfig->wdma_config.alpha = 0xFF;
	pconfig->wdma_config.dstPitch = output->pitch;
	pconfig->wdma_config.security = output->security;
	pconfig->wdma_dirty = 1;
	pconfig->dst_dirty = 1;
	pconfig->dst_h = output->h;
	pconfig->dst_w = output->w;

	if (is_multipass)
		pconfig->wdma_config.useSpecifiedAlpha = 0;
	else
		pconfig->wdma_config.useSpecifiedAlpha = 1;

	return dpmgr_path_config(disp_handle, pconfig, cmdq_handle);
}

#ifdef CONFIG_SINGLE_PANEL_OUTPUT
int primary_suspend_outputbuf_fence_release(void)
{
	int layer = 0;

	layer = disp_sync_get_output_timeline_id();
	mtkfb_release_layer_fence(primary_session_id, layer);
	/* release rdma buffer */
	layer = disp_sync_get_output_interface_timeline_id();
	mtkfb_release_layer_fence(primary_session_id, layer);
	return 0;
}
#endif

int _primary_display_config_output(disp_mem_output_config *output, unsigned int session_id, int lock)
{
	int ret = 0;
	disp_path_handle disp_handle;
	cmdqRecHandle cmdq_handle = NULL;

	DISPFUNC();
	if (lock)
		_primary_path_lock(__func__);

	if (pgc->state == DISP_SLEPT && DISP_SESSION_TYPE(session_id) != DISP_SESSION_MEMORY) {
		DISPMSG("mem out is already slept or mode wrong(%d)\n", pgc->session_mode);
#ifdef CONFIG_SINGLE_PANEL_OUTPUT
		primary_suspend_outputbuf_fence_release();
#endif
		goto done;
	}
#if !defined(OVL_MULTIPASS_SUPPORT) && !defined(OVL_TIME_SHARING)
	if (!_is_mirror_mode(pgc->session_mode)) {
		DISPERR("should not config output if not mirror mode!!\n");
		goto done;
	}
#endif

	if (_is_decouple_mode(pgc->session_mode)) {
		disp_handle = pgc->ovl2mem_path_handle;
		cmdq_handle = pgc->cmdq_handle_ovl1to2_config;
	} else {
		disp_handle = pgc->dpmgr_handle;
		cmdq_handle = pgc->cmdq_handle_config;
	}

	if (_is_decouple_mode(pgc->session_mode)) {
		if (_is_mirror_mode(pgc->session_mode) && is_output_buffer_set)
			pgc->need_trigger_dcMirror_out = 1;

	} else {
		/* direct link mirror mode should add memout first */
		dpmgr_path_add_memout(pgc->dpmgr_handle, ENGINE_OVL0, cmdq_handle);
		pgc->need_trigger_ovl1to2 = 1;
	}

	if (_is_decouple_mode(pgc->session_mode) && !_is_mirror_mode(pgc->session_mode))
		/* Multipass Trigger */
		ret = config_wdma_output(disp_handle, cmdq_handle, output, 1);
	else
		ret = config_wdma_output(disp_handle, cmdq_handle, output, 0);

	if ((pgc->session_id > 0) && _is_decouple_mode(pgc->session_mode))
		update_frm_seq_info(output->addr, 0, mtkfb_query_frm_seq_by_addr(pgc->session_id, 0, 0), FRM_CONFIG);

	if (DISP_SESSION_TYPE(session_id) == DISP_SESSION_PRIMARY) {
		mem_config = *output;
	} else {
		int layer;

		layer = disp_sync_get_output_timeline_id();
		cmdqRecBackupUpdateSlot(cmdq_handle, pgc->cur_mem_config_fence, layer, output->buff_idx);
		layer = disp_sync_get_output_interface_timeline_id();
		cmdqRecBackupUpdateSlot(cmdq_handle, pgc->cur_mem_config_fence, layer, output->interface_idx);
	}

	MMProfileLogEx(ddp_mmp_get_events()->primary_wdma_config, MMProfileFlagPulse,
		       output->buff_idx, (unsigned int)output->addr);

done:
	if (lock)
		_primary_path_unlock(__func__);

	/* dprec_logger_done(DPREC_LOGGER_PRIMARY_CONFIG, output->src_x, output->src_y); */
	return ret;

}

int primary_display_config_output(disp_mem_output_config *output, unsigned int session_id)
{
	return _primary_display_config_output(output, session_id, 1);
}

static int primary_display_config_output_nolock(disp_mem_output_config *output, unsigned int session_id)
{
	return _primary_display_config_output(output, session_id, 0);
}

#if 0
/**
 * Defined but not used, avoid build warning. (2015.1.30 Rynn Wu)
 */
static int _config_interface_input(primary_disp_input_config *input)
{
	int ret = 0;
	void *cmdq_handle = NULL;
	disp_ddp_path_config *data_config;

	/* all dirty should be cleared in dpmgr_path_get_last_config() */
	data_config = dpmgr_path_get_last_config(pgc->dpmgr_handle);

	dprec_logger_start(DPREC_LOGGER_PRIMARY_CONFIG, input->layer|(input->layer_en<<16), input->addr);

	ret = _convert_disp_input_to_rdma(&(data_config->rdma_config), (disp_input_config *)input);
	data_config->rdma_dirty = 1;

	if (_should_wait_path_idle()) {
		if (dpmgr_path_is_busy(pgc->dpmgr_handle))
			dpmgr_wait_event_timeout(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_DONE, HZ*1);
	}

	if (primary_display_cmdq_enabled())
		cmdq_handle = pgc->cmdq_handle_config;

	ret = dpmgr_path_config(pgc->dpmgr_handle, data_config, cmdq_handle);


	dprec_logger_done(DPREC_LOGGER_PRIMARY_CONFIG, input->src_x, input->src_y);

	return ret;
}
#endif

static void update_debug_fps_meter(disp_ddp_path_config *data_config)
{
	int i, dst_id = 0;

	for (i = 0; i < HW_OVERLAY_COUNT; i++) {
		if (data_config->ovl_config[i].layer_en && data_config->ovl_config[i].dst_x == 0 &&
		    data_config->ovl_config[i].dst_y == 0)
			dst_id = i;
	}
	_debug_fps_meter(data_config->ovl_config[dst_id].addr,
			 data_config->ovl_config[dst_id].vaddr,
			 data_config->ovl_config[dst_id].dst_w,
			 data_config->ovl_config[dst_id].dst_h,
			 data_config->ovl_config[dst_id].src_pitch, 0x00000000, dst_id,
			 data_config->ovl_config[dst_id].buff_idx);
}


static int _config_ovl_input(disp_session_input_config *session_input,
			     disp_path_handle disp_handle, cmdqRecHandle cmdq_handle)
{
	int ret = 0, i = 0, layer = 0;
	disp_ddp_path_config *data_config = NULL;
	int max_layer_id_configed = 0;
#ifndef MTK_FB_CMDQ_DISABLE
	int force_disable_ovl1 = 0;
#endif
	cmdqBackupSlotHandle *p_cur_config_fence;
	cmdqBackupSlotHandle *p_subtractor_when_free;
	LCM_PARAMS *lcm_param = NULL;

	/* create new data_config for ovl input */
	data_config = dpmgr_path_get_last_config(disp_handle);
	if (DISP_SESSION_TYPE(session_input->session_id) == DISP_SESSION_MEMORY) {
		p_cur_config_fence = &(pgc->cur_mem_config_fence);
		p_subtractor_when_free = &(pgc->mem_subtractor_when_free);
		data_config->is_memory = true;
	} else {
		lcm_param = disp_lcm_get_params(pgc->plcm);
		p_cur_config_fence = &(pgc->cur_config_fence);
		p_subtractor_when_free = &(pgc->subtractor_when_free);
		data_config->dst_h = lcm_param->height;
		data_config->dst_w = lcm_param->width;
		data_config->is_memory = false;
	}

	for (i = 0; i < session_input->config_layer_num; i++) {
		disp_input_config *input_cfg = &session_input->config[i];
		OVL_CONFIG_STRUCT *ovl_cfg;

		layer = input_cfg->layer_id;

		/*security issue*/
		if (layer >= OVL_LAYER_NUM)
			continue;

		ovl_cfg = &(data_config->ovl_config[layer]);
		if (session_input->setter != SESSION_USER_AEE) {
			if (isAEEEnabled && layer == primary_display_get_option("ASSERT_LAYER")) {
				DISPMSG("skip AEE layer %d\n", layer);
				continue;
			}
		} else {
			DISPMSG("set AEE layer %d\n", layer);
		}
		_convert_disp_input_to_ovl(ovl_cfg, input_cfg);

		if (ovl_cfg->layer_en)
			_debug_pattern(ovl_cfg->addr, ovl_cfg->vaddr, ovl_cfg->dst_w,
				       ovl_cfg->dst_h, ovl_cfg->src_pitch, 0x00000000,
				       ovl_cfg->layer, ovl_cfg->buff_idx);

		dprec_logger_start(DPREC_LOGGER_PRIMARY_CONFIG, ovl_cfg->layer | (ovl_cfg->layer_en << 16),
				   ovl_cfg->addr);
		dprec_logger_done(DPREC_LOGGER_PRIMARY_CONFIG, ovl_cfg->src_x, ovl_cfg->src_y);

		dprec_mmp_dump_ovl_layer(ovl_cfg, layer, 1);

		if ((ovl_cfg->layer == 0) && (!_is_decouple_mode(pgc->session_mode)))
			update_frm_seq_info(ovl_cfg->addr, ovl_cfg->src_x * 4 + ovl_cfg->src_y * ovl_cfg->src_pitch,
					    mtkfb_query_frm_seq_by_addr(pgc->session_id, 0, 0), FRM_CONFIG);

		if (max_layer_id_configed < layer)
			max_layer_id_configed = layer;

		data_config->ovl_dirty = 1;
#if defined(CONFIG_FOR_SOURCE_PQ)
		data_config->dst_dirty = 1;
#endif

#if defined(OVL_TIME_SHARING)
		data_config->roi_dirty = 1;
#endif
	}

#ifdef OVL_CASCADE_SUPPORT
	if (ovl_get_status() == DDP_OVL1_STATUS_SUB_REQUESTING) {
		/* disable ovl layer 4~8 to free ovl1 */
		if (max_layer_id_configed < OVL_LAYER_NUM_PER_OVL - is_DAL_Enabled()) {
			for (i = OVL_LAYER_NUM_PER_OVL; i < OVL_LAYER_NUM; i++)
				data_config->ovl_config[i].layer_en = 0;

			force_disable_ovl1 = 1;
			DISPMSG("cascade: HWC set %d layers, force disable OVL1 layers\n",
				max_layer_id_configed);
		} else {
			DISPMSG("cascade: try to split ovl1 fail: HWC set %d layers\n",
				max_layer_id_configed);
		}
	}
#endif

	if (_should_wait_path_idle())
		dpmgr_wait_event_timeout(disp_handle, DISP_PATH_EVENT_FRAME_DONE, HZ * 1);

	/* should we use cmdq_hand_config? need to check */
	ret = dpmgr_path_config(disp_handle, data_config, cmdq_handle);

#ifndef MTK_FB_CMDQ_DISABLE
	/* write fence_id/enable to DRAM using cmdq,
	 * it will be used when release fence (put these after config registers done)
	 */
	for (i = 0; i < session_input->config_layer_num; i++) {
		unsigned int last_fence, cur_fence;
		disp_input_config *input_cfg = &session_input->config[i];

		layer = input_cfg->layer_id;

		cmdqBackupReadSlot(*p_cur_config_fence, layer, &last_fence);
		cur_fence = input_cfg->next_buff_idx;

		if (cur_fence != -1 && cur_fence > last_fence)
			cmdqRecBackupUpdateSlot(cmdq_handle, *p_cur_config_fence, layer, cur_fence);

		/* for dim_layer/disable_layer/no_fence_layer, just release all fences configured */
		/* for other layers, release current_fence-1 */
		if (input_cfg->buffer_source == DISP_BUFFER_ALPHA || input_cfg->layer_enable == 0 ||
		    cur_fence == -1 || DISP_SESSION_TYPE(session_input->session_id) == DISP_SESSION_MEMORY)
			cmdqRecBackupUpdateSlot(cmdq_handle, *p_subtractor_when_free, layer, 0);
		else
			cmdqRecBackupUpdateSlot(cmdq_handle, *p_subtractor_when_free, layer, 1);
	}

	if (force_disable_ovl1) {
		for (layer = OVL_LAYER_NUM_PER_OVL; layer < OVL_LAYER_NUM; layer++)
			/* will release all fences */
			cmdqRecBackupUpdateSlot(cmdq_handle, *p_subtractor_when_free, layer, 0);

	}
#endif

	update_debug_fps_meter(data_config);
	if (DISP_SESSION_TYPE(session_input->session_id) == DISP_SESSION_PRIMARY) {
		last_primary_config = *data_config;
		is_hwc_update_frame = 1;
	}

	return ret;
}

#if 0
/**
 * Defined but not used, avoid build warning. (2015.1.30 Rynn Wu)
 */
static int _config_ovl_output(disp_mem_output_config *output)
{
	int ret = 0;
	disp_ddp_path_config *data_config = NULL;
	disp_path_handle *handle = NULL;

	data_config = dpmgr_path_get_last_config(pgc->ovl2mem_path_handle);

	data_config->wdma_dirty = 1;

	if (_should_wait_path_idle())
		dpmgr_wait_event_timeout(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_DONE, HZ*1);

	handle = pgc->ovl2mem_path_handle;

	ret = dpmgr_path_config(pgc->ovl2mem_path_handle, data_config, pgc->cmdq_handle_config);

	return ret;
}
#endif

static int _config_rdma_input(disp_session_input_config *session_input, disp_path_handle *handle)
{
	int ret;
	disp_ddp_path_config *data_config = NULL;

	/* all dirty should be cleared in dpmgr_path_get_last_config() */
	data_config = dpmgr_path_get_last_config(handle);
	data_config->dst_dirty = 0;
	data_config->ovl_dirty = 0;
	data_config->rdma_dirty = 0;
	data_config->wdma_dirty = 0;

	ret =
	    _convert_disp_input_to_rdma(&(data_config->rdma_config),
					(disp_input_config *) session_input);
	data_config->rdma_dirty = 1;

	if (_should_wait_path_idle())
		dpmgr_wait_event_timeout(handle, DISP_PATH_EVENT_FRAME_DONE, HZ * 1);

	ret =
	    dpmgr_path_config(handle, data_config,
			      primary_display_cmdq_enabled() ? pgc->cmdq_handle_config : NULL);
	return ret;
}

static int _primary_display_config_input_multiple(disp_session_input_config *session_input, int lock)
{
	int ret = 0;
	disp_path_handle disp_handle;
	cmdqRecHandle cmdq_handle;

	if (gTriggerDispMode > 0)
		return 0;

	if (lock)
		_primary_path_lock(__func__);

	if (primary_get_state() == DISP_SLEPT && DISP_SESSION_TYPE(session_input->session_id) != DISP_SESSION_MEMORY) {
		DISPMSG("%s, skip because primary dipslay is sleep\n", __func__);
#ifdef CONFIG_SINGLE_PANEL_OUTPUT
		primary_suspend_release_ovl_fence(session_input);
#endif
		goto done;
	}
#ifdef MTK_DISP_IDLE_LP
	/* call this in trigger is enough, do not have to call this in config */
	_disp_primary_path_exit_idle(__func__, 0);
#endif

	if (_is_decouple_mode(pgc->session_mode)) {
		disp_handle = pgc->ovl2mem_path_handle;
		cmdq_handle = pgc->cmdq_handle_ovl1to2_config;
	} else {
		disp_handle = pgc->dpmgr_handle;
		cmdq_handle = pgc->cmdq_handle_config;
	}

	if (_should_config_ovl_input())
		_config_ovl_input(session_input, disp_handle, cmdq_handle);
	else
		_config_rdma_input(session_input, disp_handle);

done:
	if (lock)
		_primary_path_unlock(__func__);
	return ret;
}

int primary_display_config_input_multiple(disp_session_input_config *session_input)
{
	return _primary_display_config_input_multiple(session_input, 1);
}

static int primary_display_config_input_multiple_nolock(disp_session_input_config *session_input)
{
	return _primary_display_config_input_multiple(session_input, 0);
}

int primary_display_config_interface_input(primary_disp_input_config *input)
{
	int ret = 0;
	void *cmdq_handle = NULL;
	disp_ddp_path_config *data_config;

	/* all dirty should be cleared in dpmgr_path_get_last_config() */
	data_config = dpmgr_path_get_last_config(pgc->dpmgr_handle);

	dprec_logger_start(DPREC_LOGGER_PRIMARY_CONFIG, input->layer | (input->layer_en << 16),
			   input->addr);

	_primary_path_lock(__func__);

	if (pgc->state == DISP_SLEPT) {
		DISPMSG("%s, skip because primary dipslay is sleep\n", __func__);
		goto done;
	}

	ret = _convert_disp_input_to_rdma(&(data_config->rdma_config), (disp_input_config *) input);
	data_config->rdma_dirty = 1;

	if (_should_wait_path_idle()) {
		if (dpmgr_path_is_busy(pgc->dpmgr_handle))
			dpmgr_wait_event_timeout(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_DONE,
						 HZ * 1);
	}

	if (primary_display_cmdq_enabled())
		cmdq_handle = pgc->cmdq_handle_config;

	ret = dpmgr_path_config(pgc->dpmgr_handle, data_config, cmdq_handle);

	/* this is used for decouple mode, to indicate whether we need to trigger ovl */
	pgc->need_trigger_overlay = 1;

	dprec_logger_done(DPREC_LOGGER_PRIMARY_CONFIG, input->src_x, input->src_y);

done:
	_primary_path_unlock(__func__);

	return ret;
}

static int Panel_Master_primary_display_config_dsi(const char *name, uint32_t config_value)
{
	int ret = 0;
	disp_ddp_path_config *data_config;
	/* all dirty should be cleared in dpmgr_path_get_last_config() */
	data_config = dpmgr_path_get_last_config(pgc->dpmgr_handle);
	/* modify below for config dsi */
	if (!strcmp(name, "PM_CLK")) {
		pr_debug("Pmaster_config_dsi: PM_CLK:%d\n", config_value);
		data_config->dispif_config.dsi.PLL_CLOCK = config_value;
	} else if (!strcmp(name, "PM_SSC")) {
		data_config->dispif_config.dsi.ssc_range = config_value;
	}
	pr_debug("Pmaster_config_dsi: will Run path_config()\n");
	ret = dpmgr_path_config(pgc->dpmgr_handle, data_config, NULL);

	return ret;
}

int primary_display_user_cmd(unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	cmdqRecHandle handle = NULL;
	int cmdqsize = 0;

	MMProfileLogEx(ddp_mmp_get_events()->primary_display_cmd, MMProfileFlagStart,
		       (unsigned long)handle, 0);

#ifndef MTK_FB_CMDQ_DISABLE
#if 0 /* CONFIG_FOR_SOURCE_PQ */
	if (primary_display_is_decouple_mode()) {
		ret = cmdqRecCreate(CMDQ_SCENARIO_DISP_COLOR, &handle);
		cmdqRecReset(handle);
		cmdqRecWait(handle, CMDQ_EVENT_DISP_WDMA0_EOF);
	} else {
		ret = cmdqRecCreate(CMDQ_SCENARIO_PRIMARY_DISP, &handle);
		cmdqRecReset(handle);
		_cmdq_insert_wait_frame_done_token_mira(handle);
	}
#else
	ret = cmdqRecCreate(CMDQ_SCENARIO_PRIMARY_DISP, &handle);
	cmdqRecReset(handle);

	if (primary_display_is_video_mode())
		cmdqRecWaitNoClear(handle, CMDQ_EVENT_DISP_RDMA0_EOF);
	else
		cmdqRecWaitNoClear(handle, CMDQ_SYNC_TOKEN_STREAM_EOF);
#endif
	cmdqsize = cmdqRecGetInstructionCount(handle);

#ifdef MTK_DISP_IDLE_LP
	/* will write register in dpmgr_path_user_cmd, need to exit idle */
	last_primary_trigger_time = sched_clock();
	_disp_primary_path_exit_idle(__func__, 1);
#endif

#ifdef CONFIG_FOR_SOURCE_PQ
	if (primary_display_is_decouple_mode())
		ret = dpmgr_path_user_cmd(pgc->ovl2mem_path_handle, cmd, arg, handle);
	else
		ret = dpmgr_path_user_cmd(pgc->dpmgr_handle, cmd, arg, handle);

#else
	ret = dpmgr_path_user_cmd(pgc->dpmgr_handle, cmd, arg, handle);
#endif

	if (handle) {
		if (cmdqRecGetInstructionCount(handle) > cmdqsize) {
			_primary_path_lock(__func__);
			if (pgc->state == DISP_ALIVE) {
#if 0				/* CONFIG_FOR_SOURCE_PQ */
				if (primary_display_is_decouple_mode()) {
					dpmgr_path_trigger(pgc->ovl2mem_path_handle, handle,
							   CMDQ_ENABLE);
					cmdqRecFlushAsync(handle);
				} else {
					_cmdq_set_config_handle_dirty_mira(handle);
					/* use non-blocking flush here to avoid primary path is locked for too long */
					_cmdq_flush_config_handle_mira(handle, 0);
				}
#else
				_cmdq_set_config_handle_dirty_mira(handle);
				/* use non-blocking flush here to avoid primary path is locked for too long */
				_cmdq_flush_config_handle_mira(handle, 0);
#endif
			}
			_primary_path_unlock(__func__);
		}

		cmdqRecDestroy(handle);
	}
#endif

	MMProfileLogEx(ddp_mmp_get_events()->primary_display_cmd, MMProfileFlagEnd,
		       (unsigned long)handle, cmdqsize);
	return ret;
}


int init_ext_decouple_buffers(void)
{
	int ret = 0;
#if defined(CONFIG_MTK_GMO_RAM_OPTIMIZE)
	_primary_path_lock(__func__);

	if (pgc->state == DISP_SLEPT) {
		ret = -1;
	} else {
		if (decouple_buffer_info[0] == NULL)
			ret = allocate_idle_lp_dc_buffer();
	}

	_primary_path_unlock(__func__);
#endif
	return ret;
}
int deinit_ext_decouple_buffers(void)
{
	int ret = 0;
#if defined(CONFIG_MTK_GMO_RAM_OPTIMIZE)
	_primary_path_lock(__func__);
	ret = release_idle_lp_dc_buffer(0);
	_primary_path_unlock(__func__);
#endif
	return ret;
}



int primary_display_switch_mode(int sess_mode, unsigned int session, int force)
{
	DISPDBG("primary_display_switch_mode sess_mode %d, session 0x%x\n", sess_mode, session);

	_primary_path_lock(__func__);

	MMProfileLogEx(ddp_mmp_get_events()->primary_switch_mode, MMProfileFlagStart,
		       pgc->session_mode, sess_mode);

	if (primary_get_state() == DISP_FREEZE || primary_get_state() == DISP_BLANK)
		sess_mode = DISP_SESSION_DECOUPLE_MODE;

	if (pgc->session_mode == sess_mode)
		goto done;

	if (pgc->state == DISP_SLEPT) {
		DISPMSG("primary display switch from %s to %s in suspend state!!!\n",
			session_mode_spy(pgc->session_mode), session_mode_spy(sess_mode));

#ifdef OVL_TIME_SHARING
		/*
		Allow system to switch mode between Deouple and Decouple Mirror when time-sharing support.
		Since system may use extension path to output data even when the primay path has suspended.
		*/
		if (pgc->session_mode == DISP_SESSION_DECOUPLE_MODE
			&& sess_mode == DISP_SESSION_DECOUPLE_MIRROR_MODE) {
			pgc->session_mode = sess_mode;
			DISPMSG("primary display is %s mode now\n", session_mode_spy(pgc->session_mode));
		} else if (pgc->session_mode == DISP_SESSION_DECOUPLE_MIRROR_MODE
			&& sess_mode == DISP_SESSION_DECOUPLE_MODE) {
			pgc->session_mode = sess_mode;
			DISPMSG("primary display is %s mode now\n", session_mode_spy(pgc->session_mode));
		}
#endif
		goto done;
	}
	DISPMSG("primary display will switch from %s to %s\n", session_mode_spy(pgc->session_mode),
		session_mode_spy(sess_mode));

	if (pgc->session_mode == DISP_SESSION_DIRECT_LINK_MODE
	    && sess_mode == DISP_SESSION_DECOUPLE_MODE) {
		/* dl to dc */
		_DL_switch_to_DC_fast();
		pgc->session_mode = sess_mode;
		DISPMSG("primary display is %s mode now\n", session_mode_spy(pgc->session_mode));
		MMProfileLogEx(ddp_mmp_get_events()->primary_switch_mode, MMProfileFlagPulse,
			       pgc->session_mode, sess_mode);
		/* primary_display_diagnose(); */
	} else if (pgc->session_mode == DISP_SESSION_DECOUPLE_MODE
		   && sess_mode == DISP_SESSION_DIRECT_LINK_MODE) {
		/* dc to dl */
		_DC_switch_to_DL_fast();
		pgc->session_mode = sess_mode;
		DISPMSG("primary display is %s mode now\n", session_mode_spy(pgc->session_mode));
		MMProfileLogEx(ddp_mmp_get_events()->primary_switch_mode, MMProfileFlagPulse,
			       pgc->session_mode, sess_mode);
		/* primary_display_diagnose(); */
	} else if (pgc->session_mode == DISP_SESSION_DIRECT_LINK_MODE
		   && sess_mode == DISP_SESSION_DIRECT_LINK_MIRROR_MODE) {
		/* dl to dl mirror */
		/* cmdqRecCreate(CMDQ_SCENARIO_DISP_SCREEN_CAPTURE, &pgc->cmdq_handle_dl_mirror); */
		/* cmdqRecReset(pgc->cmdq_handle_dl_mirror); */
		pgc->session_mode = sess_mode;
		DISPMSG("primary display is %s mode now\n", session_mode_spy(pgc->session_mode));
		MMProfileLogEx(ddp_mmp_get_events()->primary_switch_mode, MMProfileFlagPulse,
			       pgc->session_mode, sess_mode);
	} else if (pgc->session_mode == DISP_SESSION_DIRECT_LINK_MIRROR_MODE
		   && sess_mode == DISP_SESSION_DIRECT_LINK_MODE) {
		/*dl mirror to dl */
		/* cmdqRecDestroy(pgc->cmdq_handle_dl_mirror); */
		/* pgc->cmdq_handle_dl_mirror = NULL; */
		pgc->session_mode = sess_mode;
		DISPMSG("primary display is %s mode now\n", session_mode_spy(pgc->session_mode));
		MMProfileLogEx(ddp_mmp_get_events()->primary_switch_mode, MMProfileFlagPulse,
			       pgc->session_mode, sess_mode);
	} else if (pgc->session_mode == DISP_SESSION_DIRECT_LINK_MODE
		   && sess_mode == DISP_SESSION_DECOUPLE_MIRROR_MODE) {
		/* dl to dc mirror  mirror */
#ifdef CONFIG_MTK_GMO_RAM_OPTIMIZE
		if (decouple_buffer_info[0] == NULL)
			allocate_idle_lp_dc_buffer();
#endif
		_DL_switch_to_DC_fast();
		pgc->session_mode = sess_mode;
		DISPMSG("primary display is %s mode now\n", session_mode_spy(pgc->session_mode));
		MMProfileLogEx(ddp_mmp_get_events()->primary_switch_mode, MMProfileFlagPulse,
			       pgc->session_mode, sess_mode);
		/* primary_display_diagnose(); */
	} else if (pgc->session_mode == DISP_SESSION_DECOUPLE_MIRROR_MODE
		   && sess_mode == DISP_SESSION_DIRECT_LINK_MODE) {
		/*dc mirror  to dl */
		_DC_switch_to_DL_fast();
		pgc->session_mode = sess_mode;
#ifdef CONFIG_MTK_GMO_RAM_OPTIMIZE
		release_idle_lp_dc_buffer(0);
#endif
		DISPMSG("primary display is %s mode now\n", session_mode_spy(pgc->session_mode));
		MMProfileLogEx(ddp_mmp_get_events()->primary_switch_mode, MMProfileFlagPulse,
			       pgc->session_mode, sess_mode);
		/* primary_display_diagnose(); */
	} else if (pgc->session_mode == DISP_SESSION_DECOUPLE_MODE
		   && sess_mode == DISP_SESSION_DECOUPLE_MIRROR_MODE) {
		/*dc to dc mirror */
		pgc->session_mode = sess_mode;
		DISPMSG("primary display is %s mode now\n", session_mode_spy(pgc->session_mode));
		MMProfileLogEx(ddp_mmp_get_events()->primary_switch_mode, MMProfileFlagPulse,
			       pgc->session_mode, sess_mode);
	} else if (pgc->session_mode == DISP_SESSION_DECOUPLE_MIRROR_MODE
		   && sess_mode == DISP_SESSION_DECOUPLE_MODE) {
		/*dc mirror to dc */
		pgc->session_mode = sess_mode;
		DISPMSG("primary display is %s mode now\n", session_mode_spy(pgc->session_mode));
		MMProfileLogEx(ddp_mmp_get_events()->primary_switch_mode, MMProfileFlagPulse,
			       pgc->session_mode, sess_mode);
	} else {
		DISPERR("invalid mode switch from %s to %s\n", session_mode_spy(pgc->session_mode),
			session_mode_spy(sess_mode));
	}
done:
	_primary_path_unlock(__func__);
	pgc->session_id = session;

	MMProfileLogEx(ddp_mmp_get_events()->primary_switch_mode, MMProfileFlagEnd,
		       pgc->session_mode, sess_mode);

	return 0;		/* avoid build warning. */
}

#ifdef MTK_DISP_IDLE_LP
int primary_display_switch_mode_nolock(int sess_mode, unsigned int session, int force)
{
	DISPMSG("primary_display_switch_mode sess_mode %d, session 0x%x\n", sess_mode, session);

	/* if(!force && _is_decouple_mode(pgc->session_mode)) */
	/* return 0; */

	/* _primary_path_lock(__func__); */

	MMProfileLogEx(ddp_mmp_get_events()->primary_switch_mode, MMProfileFlagStart,
		       pgc->session_mode, sess_mode);

	if (pgc->session_mode == sess_mode)
		goto done;

	DISPMSG("primary display will switch from %s to %s\n", session_mode_spy(pgc->session_mode),
		session_mode_spy(sess_mode));

	if (pgc->session_mode == DISP_SESSION_DIRECT_LINK_MODE && sess_mode == DISP_SESSION_DECOUPLE_MODE) {
		/* signal frame start event to switch logic in fence_release_worker_thread */
		/* dpmgr_signal_event(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_START); */
		_DL_switch_to_DC_fast();
		pgc->session_mode = sess_mode;
		DISPMSG("primary display is %s mode now\n", session_mode_spy(pgc->session_mode));
		MMProfileLogEx(ddp_mmp_get_events()->primary_switch_mode, MMProfileFlagPulse,
			       pgc->session_mode, sess_mode);
		/* primary_display_diagnose(); */
	} else if (pgc->session_mode == DISP_SESSION_DECOUPLE_MODE && sess_mode == DISP_SESSION_DIRECT_LINK_MODE) {
		/* signal frame start event to switch logic in fence_release_worker_thread */
		/* dpmgr_signal_event(pgc->ovl2mem_path_handle, DISP_PATH_EVENT_FRAME_START); */
		_DC_switch_to_DL_fast();
		pgc->session_mode = sess_mode;
		DISPMSG("* primary display is %s mode now\n", session_mode_spy(pgc->session_mode));
		MMProfileLogEx(ddp_mmp_get_events()->primary_switch_mode, MMProfileFlagPulse,
			       pgc->session_mode, sess_mode);
		/* primary_display_diagnose(); */
	} else if (pgc->session_mode == DISP_SESSION_DIRECT_LINK_MODE &&
		   sess_mode == DISP_SESSION_DIRECT_LINK_MIRROR_MODE) {
		/*need delay switch to output */
		pgc->session_mode = sess_mode;
	} else if (pgc->session_mode == DISP_SESSION_DIRECT_LINK_MIRROR_MODE
		   && sess_mode == DISP_SESSION_DIRECT_LINK_MODE) {
		/* xxx */
		pgc->session_mode = sess_mode;
	} else if (pgc->session_mode == DISP_SESSION_DIRECT_LINK_MODE &&
		   sess_mode == DISP_SESSION_DECOUPLE_MIRROR_MODE) {
		/*need delay switch to output */
		_DL_switch_to_DC_fast();
		pgc->session_mode = sess_mode;
		DISPMSG("primary display is %s mode now\n", session_mode_spy(pgc->session_mode));
		MMProfileLogEx(ddp_mmp_get_events()->primary_switch_mode, MMProfileFlagPulse,
			       pgc->session_mode, sess_mode);
		/* primary_display_diagnose(); */
		/* pgc->session_delay_mode = sess_mode; */
	} else if (pgc->session_mode == DISP_SESSION_DECOUPLE_MIRROR_MODE &&
		   sess_mode == DISP_SESSION_DIRECT_LINK_MODE) {
		_DC_switch_to_DL_fast();
		pgc->session_mode = sess_mode;
		DISPMSG("primary display is %s mode now\n", session_mode_spy(pgc->session_mode));
		MMProfileLogEx(ddp_mmp_get_events()->primary_switch_mode, MMProfileFlagPulse,
			       pgc->session_mode, sess_mode);
		/* primary_display_diagnose(); */
	} else {
		DISPERR("invalid mode switch from %s to %s\n", session_mode_spy(pgc->session_mode),
			session_mode_spy(sess_mode));
	}
done:
	/* _primary_path_unlock(__func__); */
	pgc->session_id = session;

	MMProfileLogEx(ddp_mmp_get_events()->primary_switch_mode, MMProfileFlagEnd,
		       pgc->session_mode, sess_mode);

	return 0;		/* avoid build warning. */
}
#endif

int primary_display_is_alive(void)
{
	unsigned int temp = 0;
	/* DISPFUNC(); */
	_primary_path_lock(__func__);

	if (pgc->state == DISP_ALIVE)
		temp = 1;

	_primary_path_unlock(__func__);

	return temp;
}
int primary_display_is_sleepd_nolock(void)
{
	unsigned int temp = 0;
	/* DISPFUNC(); */

	if (pgc->state == DISP_SLEPT)
		temp = 1;

	return temp;
}

int primary_display_is_sleepd(void)
{
	unsigned int temp = 0;
	/* DISPFUNC(); */
	_primary_path_lock(__func__);

	if (pgc->state == DISP_SLEPT)
		temp = 1;

	_primary_path_unlock(__func__);

	return temp;
}



int primary_display_get_width(void)
{
	if (pgc->plcm == NULL) {
		DISPERR("lcm handle is null\n");
		return 0;
	}

	if (pgc->plcm->params)
		return pgc->plcm->params->width;

	DISPERR("lcm_params is null!\n");
	return 0;
}

int primary_display_get_height(void)
{
	if (pgc->plcm == NULL) {
		DISPERR("lcm handle is null\n");
		return 0;
	}

	if (pgc->plcm->params)
		return pgc->plcm->params->height;

	DISPERR("lcm_params is null!\n");
	return 0;
}


int primary_display_get_original_width(void)
{
	if (pgc->plcm == NULL) {
		DISPERR("lcm handle is null\n");
		return 0;
	}

	if (pgc->plcm->params)
		return pgc->plcm->lcm_original_width;

	DISPERR("lcm_params is null!\n");
	return 0;
}

int primary_display_get_original_height(void)
{
	if (pgc->plcm == NULL) {
		DISPERR("lcm handle is null\n");
		return 0;
	}

	if (pgc->plcm->params)
		return pgc->plcm->lcm_original_height;

	DISPERR("lcm_params is null!\n");
	return 0;
}

int primary_display_get_bpp(void)
{
	return 32;
}

int primary_display_get_dc_bpp(void)
{
	return 24;
}

void primary_display_set_max_layer(int maxlayer)
{
	pgc->max_layer = maxlayer;
}

int primary_display_get_info(void *info)
{
#if 1
	/* DISPFUNC(); */
	disp_session_info *dispif_info = (disp_session_info *) info;

	LCM_PARAMS *lcm_param = disp_lcm_get_params(pgc->plcm);

	if (lcm_param == NULL) {
		DISPMSG("lcm_param is null\n");
		return -1;
	}

	memset((void *)dispif_info, 0, sizeof(disp_session_info));

#ifdef OVL_CASCADE_SUPPORT
	if (is_DAL_Enabled() && pgc->max_layer == OVL_LAYER_NUM) {
		/* OVL1 is used by mem session */
		if (ovl_get_status() == DDP_OVL1_STATUS_PRIMARY || ovl_get_status() == DDP_OVL1_STATUS_IDLE)
			dispif_info->maxLayerNum = pgc->max_layer - 1;
		else
			dispif_info->maxLayerNum = pgc->max_layer - 1 - 4;
	} else {
		/* OVL1 is used by mem session */
		if (ovl_get_status() == DDP_OVL1_STATUS_PRIMARY || ovl_get_status() == DDP_OVL1_STATUS_IDLE)
			dispif_info->maxLayerNum = pgc->max_layer;
		else
			dispif_info->maxLayerNum = pgc->max_layer - 4;
	}
#else
	if (is_DAL_Enabled() && pgc->max_layer == OVL_LAYER_NUM)
		dispif_info->maxLayerNum = pgc->max_layer - 1;
	else
		dispif_info->maxLayerNum = pgc->max_layer;
#endif
	/* DISPDBG("available layer num=%d\n", dispif_info->maxLayerNum); */

	switch (lcm_param->type) {
	case LCM_TYPE_DBI:
	{
		dispif_info->displayType = DISP_IF_TYPE_DBI;
		dispif_info->displayMode = DISP_IF_MODE_COMMAND;
		dispif_info->isHwVsyncAvailable = 1;
		/* DISPMSG("DISP Info: DBI, CMD Mode, HW Vsync enable\n"); */
		break;
	}
	case LCM_TYPE_DPI:
	{
		dispif_info->displayType = DISP_IF_TYPE_DPI;
		dispif_info->displayMode = DISP_IF_MODE_VIDEO;
		dispif_info->isHwVsyncAvailable = 1;
		/* DISPMSG("DISP Info: DPI, VDO Mode, HW Vsync enable\n"); */
		break;
	}
	case LCM_TYPE_DSI:
	{
		dispif_info->displayType = DISP_IF_TYPE_DSI0;
		if (lcm_param->dsi.mode == CMD_MODE) {
			dispif_info->displayMode = DISP_IF_MODE_COMMAND;
			dispif_info->isHwVsyncAvailable = 1;
			/* DISPMSG("DISP Info: DSI, CMD Mode, HW Vsync enable\n"); */
		} else {
			dispif_info->displayMode = DISP_IF_MODE_VIDEO;
			dispif_info->isHwVsyncAvailable = 1;
			/* DISPMSG("DISP Info: DSI, VDO Mode, HW Vsync enable\n"); */
		}

		break;
	}
	default:
		break;
	}


	dispif_info->displayFormat = DISP_IF_FORMAT_RGB888;

	dispif_info->displayWidth = primary_display_get_width();
	dispif_info->displayHeight = primary_display_get_height();

	dispif_info->physicalWidth = DISP_GetActiveWidth();
	dispif_info->physicalHeight = DISP_GetActiveHeight();


	dispif_info->vsyncFPS = pgc->lcm_fps;
	dispif_info->isConnected = 1;

#ifdef ROME_TODO
#error
	{
		LCM_PARAMS lcm_params_temp;

		memset((void *)&lcm_params_temp, 0, sizeof(lcm_params_temp));
		if (lcm_drv) {
			lcm_drv->get_params(&lcm_params_temp);
			dispif_info->lcmOriginalWidth = lcm_params_temp.width;
			dispif_info->lcmOriginalHeight = lcm_params_temp.height;
			DISPMSG("DISP Info: LCM Panel Original Resolution(For DFO Only): %d x %d\n",
				dispif_info->lcmOriginalWidth, dispif_info->lcmOriginalHeight);
		} else {
			DISPMSG("DISP Info: Fatal Error!!, lcm_drv is null\n");
		}
	}
#endif

#endif
	return 0;		/* avoid build warning. */
}

int primary_display_get_pages(void)
{
	return 3;
}


int primary_display_is_video_mode(void)
{
	/* TODO: we should store the video/cmd mode in runtime, because we will support cmd/vdo dynamic switch */
	return disp_lcm_is_video_mode(pgc->plcm);
}

int primary_display_is_decouple_mode(void)
{
	return _is_decouple_mode(pgc->session_mode);
}

int primary_display_is_mirror_mode(void)
{
	return _is_mirror_mode(pgc->session_mode);
}

int primary_display_is_ovl1to2_handle(cmdqRecHandle *handle)
{
	if (handle && (handle == (cmdqRecHandle *)pgc->cmdq_handle_ovl1to2_config))
		return 1;
	else
		return 0;
}

int primary_display_diagnose(void)
{
	int ret = 0;

	if (pgc->dpmgr_handle != NULL)
		dpmgr_check_status(pgc->dpmgr_handle);

	if (_is_decouple_mode(pgc->session_mode) && (pgc->ovl2mem_path_handle != NULL))
		dpmgr_check_status(pgc->ovl2mem_path_handle);
	primary_display_check_path(NULL, 0);

	return ret;
}

CMDQ_SWITCH primary_display_cmdq_enabled(void)
{
	return primary_display_use_cmdq;
}

int primary_display_switch_cmdq_cpu(CMDQ_SWITCH use_cmdq)
{
	_primary_path_lock(__func__);

	primary_display_use_cmdq = use_cmdq;
	DISPMSG("display driver use %s to config register now\n",
		  (use_cmdq == CMDQ_ENABLE) ? "CMDQ" : "CPU");

	_primary_path_unlock(__func__);
	return primary_display_use_cmdq;
}

int primary_display_manual_lock(void)
{
	_primary_path_lock(__func__);
	return 0;		/* avoid build warning */
}

int primary_display_manual_unlock(void)
{
	_primary_path_unlock(__func__);
	return 0;
}

void primary_display_reset(void)
{
	dpmgr_path_reset(pgc->dpmgr_handle, CMDQ_DISABLE);
}

unsigned int primary_display_get_fps(void)
{
	unsigned int fps = 0;

	_primary_path_lock(__func__);
	fps = pgc->lcm_fps;
	_primary_path_unlock(__func__);

	return fps;
}

int primary_display_force_set_vsync_fps(unsigned int fps)
{
	int ret = 0;

	DISPMSG("force set fps to %d\n", fps);
	_primary_path_lock(__func__);

	if (fps == pgc->lcm_fps) {
		pgc->vsync_drop = 0;
		ret = 0;
	} else if (fps == 30) {
		pgc->vsync_drop = 1;
		ret = 0;
	} else {
		ret = -1;
	}

	_primary_path_unlock(__func__);

	return ret;
}

int primary_display_insert_session_buf(disp_session_buf_info *session_buf_info)
{
	int i = 0;
	unsigned int *session_buf;

	session_buf = pgc->session_buf;
	pgc->session_buf_id = 0;
	for (i = 0; i < DISP_INTERNAL_BUFFER_COUNT; i++) {
		session_buf[i] = session_buf_info->buf_hnd[i];
		DISPMSG("%s buf_hnd[%d] = 0x%x\n", __func__, i, session_buf_info->buf_hnd[i]);
	}
	return 0;
}

int primary_display_enable_path_cg(int enable)
{
	int ret = 0;
#ifdef ENABLE_CLK_MGR
	DISPMSG("%s primary display's path cg\n", enable ? "enable" : "disable");
	_primary_path_lock(__func__);

	if (enable) {
#ifdef CONFIG_MTK_CLKMGR
		ret += disable_clock(MT_CG_DISP1_DSI_ENGINE, "DSI0");
		ret += disable_clock(MT_CG_DISP1_DSI_DIGITAL, "DSI0");
		ret += disable_clock(MT_CG_DISP0_DISP_OVL0, "DDP");
		ret += disable_clock(MT_CG_DISP0_DISP_COLOR, "DDP");
		ret += disable_clock(MT_CG_DISP0_DISP_CCORR, "DDP");
		ret += disable_clock(MT_CG_DISP0_DISP_AAL, "DDP");
		ret += disable_clock(MT_CG_DISP0_DISP_GAMMA, "DDP");
		ret += disable_clock(MT_CG_DISP0_DISP_DITHER, "DDP");
		ret += disable_clock(MT_CG_DISP0_DISP_RDMA0, "DDP");

		ret += disable_clock(MT_CG_PERI_DISP_PWM, "PWM");

		/* ret += disable_clock(MT_CG_DISP0_MUTEX_32K     , "Debug"); */
		ret += disable_clock(MT_CG_DISP0_SMI_LARB0, "Debug");
		ret += disable_clock(MT_CG_DISP0_SMI_COMMON, "Debug");

		/* ret += disable_clock(MT_CG_DISP0_MUTEX_32K   , "Debug2"); */
		ret += disable_clock(MT_CG_DISP0_SMI_LARB0, "Debug2");
		ret += disable_clock(MT_CG_DISP0_SMI_COMMON, "Debug2");
#else
		ddp_clk_disable(DISP1_DSI_ENGINE);
		ddp_clk_disable(DISP1_DSI_DIGITAL);
		ddp_clk_disable(DISP0_DISP_OVL0);
		ddp_clk_disable(DISP0_DISP_COLOR);
		ddp_clk_disable(DISP0_DISP_CCORR);
		ddp_clk_disable(DISP0_DISP_AAL);
		ddp_clk_disable(DISP0_DISP_GAMMA);
		ddp_clk_disable(DISP0_DISP_DITHER);
		ddp_clk_disable(DISP0_DISP_RDMA0);

		ddp_clk_disable(DISP_PWM);

		/*ddp_clk_disable(DISP0_MUTEX_32K); */
		ddp_clk_disable(DISP0_SMI_LARB0);
		ddp_clk_disable(DISP0_SMI_COMMON);

		/*ddp_clk_disable(DISP0_MUTEX_32K); */
		ddp_clk_disable(DISP0_SMI_LARB0);
		ddp_clk_disable(DISP0_SMI_COMMON);
		ddp_clk_disable(DISP_MTCMOS_CLK);
		ddp_clk_unprepare(DISP_MTCMOS_CLK);
#endif

	} else {
#ifdef CONFIG_MTK_CLKMGR
		ret += enable_clock(MT_CG_DISP1_DSI_ENGINE, "DSI0");
		ret += enable_clock(MT_CG_DISP1_DSI_DIGITAL, "DSI0");
		ret += enable_clock(MT_CG_DISP0_DISP_OVL0, "DDP");
		ret += enable_clock(MT_CG_DISP0_DISP_COLOR, "DDP");
		ret += enable_clock(MT_CG_DISP0_DISP_CCORR, "DDP");
		ret += enable_clock(MT_CG_DISP0_DISP_AAL, "DDP");
		ret += enable_clock(MT_CG_DISP0_DISP_GAMMA, "DDP");
		ret += enable_clock(MT_CG_DISP0_DISP_DITHER, "DDP");
		ret += enable_clock(MT_CG_DISP0_DISP_RDMA0, "DDP");

		ret += enable_clock(MT_CG_PERI_DISP_PWM, "PWM");

		/* ret += enable_clock(MT_CG_DISP0_MUTEX_32K      , "Debug"); */
		ret += enable_clock(MT_CG_DISP0_SMI_LARB0, "Debug");
		ret += enable_clock(MT_CG_DISP0_SMI_COMMON, "Debug");

		/* ret += enable_clock(MT_CG_DISP0_MUTEX_32K   , "Debug2"); */
		ret += enable_clock(MT_CG_DISP0_SMI_LARB0, "Debug2");
		ret += enable_clock(MT_CG_DISP0_SMI_COMMON, "Debug2");
#else
		ddp_clk_prepare(DISP_MTCMOS_CLK);
		ret += ddp_clk_enable(DISP_MTCMOS_CLK);
		ret += ddp_clk_enable(DISP1_DSI_ENGINE);
		ret += ddp_clk_enable(DISP1_DSI_DIGITAL);
		ret += ddp_clk_enable(DISP0_DISP_OVL0);
		ret += ddp_clk_enable(DISP0_DISP_COLOR);
		ret += ddp_clk_enable(DISP0_DISP_CCORR);
		ret += ddp_clk_enable(DISP0_DISP_AAL);
		ret += ddp_clk_enable(DISP0_DISP_GAMMA);
		ret += ddp_clk_enable(DISP0_DISP_DITHER);
		ret += ddp_clk_enable(DISP0_DISP_RDMA0);

		ret += ddp_clk_enable(DISP_PWM);

		/*ret += ddp_clk_enable(DISP0_MUTEX_32K); */
		ret += ddp_clk_enable(DISP0_SMI_LARB0);
		ret += ddp_clk_enable(DISP0_SMI_COMMON);

		/*ret += ddp_clk_enable(DISP0_MUTEX_32K); */
		ret += ddp_clk_enable(DISP0_SMI_LARB0);
		ret += ddp_clk_enable(DISP0_SMI_COMMON);
#endif
	}

	_primary_path_unlock(__func__);
#endif

	return ret;
}

int _set_backlight_by_cmdq(unsigned int level)
{
	int ret = 0;
	cmdqRecHandle cmdq_handle_backlight = NULL;

	MMProfileLogEx(ddp_mmp_get_events()->primary_set_bl, MMProfileFlagPulse, 1, 1);
	ret = cmdqRecCreate(CMDQ_SCENARIO_PRIMARY_DISP, &cmdq_handle_backlight);
	DISPMSG("primary backlight, handle=%p\n", cmdq_handle_backlight);
	if (ret != 0) {
		DISPMSG("fail to create primary cmdq handle for backlight\n");
		return -1;
	}

	if (primary_display_is_video_mode()) {
		MMProfileLogEx(ddp_mmp_get_events()->primary_set_bl, MMProfileFlagPulse, 1, 2);
		cmdqRecReset(cmdq_handle_backlight);
		dpmgr_path_ioctl(pgc->dpmgr_handle, cmdq_handle_backlight, DDP_BACK_LIGHT,
				 (unsigned long *)&level);
		_cmdq_flush_config_handle_mira(cmdq_handle_backlight, 1);
		DISPMSG("[BL]_set_backlight_by_cmdq ret=%d\n", ret);
	} else {
		MMProfileLogEx(ddp_mmp_get_events()->primary_set_bl, MMProfileFlagPulse, 1, 3);
		cmdqRecReset(cmdq_handle_backlight);
		_cmdq_insert_wait_frame_done_token_mira(cmdq_handle_backlight);
		cmdqRecClearEventToken(cmdq_handle_backlight, CMDQ_SYNC_TOKEN_CABC_EOF);
		dpmgr_path_ioctl(pgc->dpmgr_handle, cmdq_handle_backlight, DDP_BACK_LIGHT,
				 (unsigned long *)&level);
		cmdqRecSetEventToken(cmdq_handle_backlight, CMDQ_SYNC_TOKEN_CABC_EOF);
		MMProfileLogEx(ddp_mmp_get_events()->primary_set_bl, MMProfileFlagPulse, 1, 4);
		_cmdq_flush_config_handle_mira(cmdq_handle_backlight, 1);
		MMProfileLogEx(ddp_mmp_get_events()->primary_set_bl, MMProfileFlagPulse, 1, 6);
		DISPMSG("[BL]_set_backlight_by_cmdq ret=%d\n", ret);
	}
	cmdqRecDestroy(cmdq_handle_backlight);
	cmdq_handle_backlight = NULL;
	MMProfileLogEx(ddp_mmp_get_events()->primary_set_bl, MMProfileFlagPulse, 1, 5);
	return ret;
}

int _set_backlight_by_cpu(unsigned int level)
{
	int ret = 0;

	MMProfileLogEx(ddp_mmp_get_events()->primary_set_bl, MMProfileFlagPulse, 0, 1);
	if (primary_display_is_video_mode()) {
		disp_lcm_set_backlight(pgc->plcm, level);
	} else {
		DISPMSG("[BL]display cmdq trigger loop stop[begin]\n");
		_cmdq_stop_trigger_loop();
		DISPMSG("[BL]display cmdq trigger loop stop[end]\n");

		if (dpmgr_path_is_busy(pgc->dpmgr_handle)) {
			DISPMSG("[BL]primary display path is busy\n");
			ret = dpmgr_wait_event_timeout(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_DONE, HZ * 1);
			DISPMSG("[BL]wait frame done ret:%d\n", ret);
		}

		DISPMSG("[BL]stop dpmgr path[begin]\n");
		dpmgr_path_stop(pgc->dpmgr_handle, CMDQ_DISABLE);
		DISPMSG("[BL]stop dpmgr path[end]\n");
		if (dpmgr_path_is_busy(pgc->dpmgr_handle)) {
			DISPMSG("[BL]primary display path is busy after stop\n");
			dpmgr_wait_event_timeout(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_DONE,
						 HZ * 1);
			DISPMSG("[BL]wait frame done ret:%d\n", ret);
		}
		DISPMSG("[BL]reset display path[begin]\n");
		dpmgr_path_reset(pgc->dpmgr_handle, CMDQ_DISABLE);
		DISPMSG("[BL]reset display path[end]\n");

		MMProfileLogEx(ddp_mmp_get_events()->primary_set_bl, MMProfileFlagPulse, 0, 2);

		disp_lcm_set_backlight(pgc->plcm, level);

		MMProfileLogEx(ddp_mmp_get_events()->primary_set_bl, MMProfileFlagPulse, 0, 3);

		DISPMSG("[BL]start dpmgr path[begin]\n");
		dpmgr_path_start(pgc->dpmgr_handle, CMDQ_DISABLE);
		DISPMSG("[BL]start dpmgr path[end]\n");

		DISPMSG("[BL]start cmdq trigger loop[begin]\n");
		_cmdq_start_trigger_loop();
		DISPMSG("[BL]start cmdq trigger loop[end]\n");
	}
	MMProfileLogEx(ddp_mmp_get_events()->primary_set_bl, MMProfileFlagPulse, 0, 7);
	return ret;
}


int primary_display_setbacklight(unsigned int level)
{
	int ret = 0;
	static unsigned int last_level;

	DISPFUNC();

	if (last_level == level)
		return 0;

	MMProfileLogEx(ddp_mmp_get_events()->primary_set_bl, MMProfileFlagStart, 0, 0);
#ifdef DISP_SWITCH_DST_MODE
	_primary_path_switch_dst_lock();
#endif
	_primary_path_cmd_lock();
	_primary_path_lock(__func__);
	if (pgc->state == DISP_SLEPT) {
		DISPMSG("Sleep State set backlight invald\n");
	} else {
		disp_update_trigger_time();
		if (primary_display_cmdq_enabled()) {
			if (primary_display_is_video_mode()) {
				MMProfileLogEx(ddp_mmp_get_events()->primary_set_bl,
					       MMProfileFlagPulse, 0, 7);
				disp_lcm_set_backlight(pgc->plcm, level);

				#ifdef CONFIG_LCT_ESD_CHECK_MULTI_REG
				esd_recovery_level = level;	//add by zhudaolong at 20170411
				#endif

			} else {
#ifdef MTK_DISP_IDLE_LP
				/* CMD mode need to exit top clock off idle mode */
				_disp_primary_path_exit_idle("primary_display_setbacklight", 0);
#endif
				_set_backlight_by_cmdq(level);
			}
		} else {
#ifdef MTK_DISP_IDLE_LP
			if (primary_display_is_video_mode() == 0)
				_disp_primary_path_exit_idle("primary_display_setbacklight", 0);
#endif
			_set_backlight_by_cpu(level);
		}
		last_level = level;
	}
#ifndef CONFIG_FPGA_EARLY_PORTING
#ifdef GPIO_LCM_LED_EN
	if (0 == level)
		mt_set_gpio_out(GPIO_LCM_LED_EN, GPIO_OUT_ZERO);
	else
		mt_set_gpio_out(GPIO_LCM_LED_EN, GPIO_OUT_ONE);
#endif
#endif
#if 0	/* check writed success? for test after CABC */
	{
		/* extern uint32_t DSI_dcs_read_lcm_reg_v2(DISP_MODULE_ENUM module, cmdqRecHandle cmdq,
						      uint8_t cmd, uint8_t *buffer, uint8_t buffer_size);
		 */
		uint8_t buffer[2];

		if (primary_display_is_video_mode())
			dpmgr_path_ioctl(pgc->dpmgr_handle, NULL, DDP_STOP_VIDEO_MODE, NULL);

		DSI_dcs_read_lcm_reg_v2(DISP_MODULE_DSI0, NULL, 0x51, buffer, 1);
		pr_debug("[CABC check result 0x51 = 0x%x,0x%x]\n", buffer[0], buffer[1]);
		dpmgr_path_start(pgc->dpmgr_handle, CMDQ_DISABLE);
		if (primary_display_is_video_mode()) {
			/* for video mode, we need to force trigger here */
			/* for cmd mode, just set DPREC_EVENT_CMDQ_SET_EVENT_ALLOW when trigger loop start */
			dpmgr_path_trigger(pgc->dpmgr_handle, NULL, CMDQ_DISABLE);
		}
	}
#endif
	_primary_path_unlock(__func__);
	_primary_path_cmd_unlock();
#ifdef DISP_SWITCH_DST_MODE
	_primary_path_switch_dst_unlock();
#endif
	MMProfileLogEx(ddp_mmp_get_events()->primary_set_bl, MMProfileFlagEnd, 0, 0);
	return ret;
}

//add by LCT yufangfang for hbm
#ifdef CONFIG_LCT_HBM_SUPPORT
static unsigned int hbm_backlight_level = 0;
int primary_recognition_hbm_level(void)
{
	return hbm_backlight_level;
}
EXPORT_SYMBOL_GPL(primary_recognition_hbm_level)

int primary_display_setbacklight_hbm(unsigned int level)
{
	int ret = 0;
	static unsigned int last_level;

	DISPFUNC();

	if (last_level == level)
		return 0;
	if(level==255)
	hbm_backlight_level = 1;
	else
	hbm_backlight_level = 0;
	MMProfileLogEx(ddp_mmp_get_events()->primary_set_bl, MMProfileFlagStart, 0, 0);
#ifdef DISP_SWITCH_DST_MODE
	_primary_path_switch_dst_lock();
#endif
	_primary_path_cmd_lock();
	_primary_path_lock(__func__);
	if (pgc->state == DISP_SLEPT) {
		DISPMSG("Sleep State set backlight invald\n");
	} else {
		disp_update_trigger_time();
		if (primary_display_cmdq_enabled()) {
			if (primary_display_is_video_mode()) {
				MMProfileLogEx(ddp_mmp_get_events()->primary_set_bl,
					       MMProfileFlagPulse, 0, 7);
				disp_lcm_set_backlight_hbm(pgc->plcm, level);
				printk("yufangfang hbm level = %d\n",level);
			} else {
#ifdef MTK_DISP_IDLE_LP
				/* CMD mode need to exit top clock off idle mode */
				_disp_primary_path_exit_idle("primary_display_setbacklight", 0);
#endif
				_set_backlight_by_cmdq(level);
			}
		} else {
#ifdef MTK_DISP_IDLE_LP
			if (primary_display_is_video_mode() == 0)
				_disp_primary_path_exit_idle("primary_display_setbacklight", 0);
#endif
			_set_backlight_by_cpu(level);
		}
		last_level = level;
	}
#ifndef CONFIG_FPGA_EARLY_PORTING
#ifdef GPIO_LCM_LED_EN
	if (0 == level)
		mt_set_gpio_out(GPIO_LCM_LED_EN, GPIO_OUT_ZERO);
	else
		mt_set_gpio_out(GPIO_LCM_LED_EN, GPIO_OUT_ONE);
#endif
#endif
#if 0	/* check writed success? for test after CABC */
	{
		/* extern uint32_t DSI_dcs_read_lcm_reg_v2(DISP_MODULE_ENUM module, cmdqRecHandle cmdq,
						      uint8_t cmd, uint8_t *buffer, uint8_t buffer_size);
		 */
		uint8_t buffer[2];

		if (primary_display_is_video_mode())
			dpmgr_path_ioctl(pgc->dpmgr_handle, NULL, DDP_STOP_VIDEO_MODE, NULL);

		DSI_dcs_read_lcm_reg_v2(DISP_MODULE_DSI0, NULL, 0x51, buffer, 1);
		pr_debug("[CABC check result 0x51 = 0x%x,0x%x]\n", buffer[0], buffer[1]);
		dpmgr_path_start(pgc->dpmgr_handle, CMDQ_DISABLE);
		if (primary_display_is_video_mode()) {
			/* for video mode, we need to force trigger here */
			/* for cmd mode, just set DPREC_EVENT_CMDQ_SET_EVENT_ALLOW when trigger loop start */
			dpmgr_path_trigger(pgc->dpmgr_handle, NULL, CMDQ_DISABLE);
		}
	}
#endif
	_primary_path_unlock(__func__);
	_primary_path_cmd_unlock();
#ifdef DISP_SWITCH_DST_MODE
	_primary_path_switch_dst_unlock();
#endif
	MMProfileLogEx(ddp_mmp_get_events()->primary_set_bl, MMProfileFlagEnd, 0, 0);
	return ret;
}
EXPORT_SYMBOL_GPL(primary_display_setbacklight_hbm)
#endif
int primary_display_set_cmd(int *lcm_cmd, unsigned int cmd_num)
{
	int ret = 0;

	DISPFUNC();

	_primary_path_cmd_lock();
	_primary_path_lock(__func__);
	if (pgc->state == DISP_SLEPT) {
		DISPMSG("Sleep State set cmd invald\n");
	} else {
		if (primary_display_cmdq_enabled()) {	/* cmdq */
			int ret = 0;
			cmdqRecHandle cmdq_handle_cmd = NULL;

			ret = cmdqRecCreate(CMDQ_SCENARIO_PRIMARY_DISP, &cmdq_handle_cmd);
			DISPMSG("primary set cmd, handle=%p\n", cmdq_handle_cmd);
			if (ret != 0) {
				DISPMSG("fail to create primary cmdq handle for cmd\n");
				return -1;
			}
			if (primary_display_is_video_mode()) {
				/* video mode */
				cmdqRecReset(cmdq_handle_cmd);
				disp_lcm_set_cmd(pgc->plcm, (void *)cmdq_handle_cmd, lcm_cmd,
						 cmd_num);
				_cmdq_flush_config_handle_mira(cmdq_handle_cmd, 1);
				DISPMSG("[Display]_set_cmd_by_cmdq ret=%d\n", ret);

			} else {
				/* cmd mode */
				cmdqRecReset(cmdq_handle_cmd);
				_cmdq_insert_wait_frame_done_token_mira(cmdq_handle_cmd);
				cmdqRecClearEventToken(cmdq_handle_cmd, CMDQ_SYNC_TOKEN_CABC_EOF);
				disp_lcm_set_cmd(pgc->plcm, (void *)cmdq_handle_cmd, lcm_cmd,
						 cmd_num);
				cmdqRecSetEventToken(cmdq_handle_cmd, CMDQ_SYNC_TOKEN_CABC_EOF);
				_cmdq_flush_config_handle_mira(cmdq_handle_cmd, 1);
				DISPMSG("[Display]_set_cmd_by_cmdq ret=%d\n", ret);
			}
			cmdqRecDestroy(cmdq_handle_cmd);
			cmdq_handle_cmd = NULL;
		}
	}
	_primary_path_unlock(__func__);
	_primary_path_cmd_unlock();
	return ret;

}

#define LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL

/***********************/
/*****Legacy DISP API*****/
/***********************/
uint32_t DISP_GetScreenWidth(void)
{
	return primary_display_get_width();
}
EXPORT_SYMBOL(DISP_GetScreenWidth);

uint32_t DISP_GetScreenHeight(void)
{
	return primary_display_get_height();
}
EXPORT_SYMBOL(DISP_GetScreenHeight);

uint32_t DISP_GetActiveHeight(void)
{
	if (pgc->plcm == NULL) {
		DISPERR("lcm handle is null\n");
		return 0;
	}

	if (pgc->plcm->params)
		return pgc->plcm->params->physical_height;

	DISPERR("lcm_params is null!\n");
	return 0;
}


uint32_t DISP_GetActiveWidth(void)
{
	if (pgc->plcm == NULL) {
		DISPERR("lcm handle is null\n");
		return 0;
	}

	if (pgc->plcm->params)
		return pgc->plcm->params->physical_width;

	DISPERR("lcm_params is null!\n");
	return 0;
}

LCM_PARAMS *DISP_GetLcmPara(void)
{
	if (pgc->plcm == NULL) {
		DISPERR("lcm handle is null\n");
		return NULL;
	}

	if (pgc->plcm->params)
		return pgc->plcm->params;
	else
		return NULL;
}

LCM_DRIVER *DISP_GetLcmDrv(void)
{

	if (pgc->plcm == NULL) {
		DISPERR("lcm handle is null\n");
		return NULL;
	}

	if (pgc->plcm->drv)
		return pgc->plcm->drv;
	else
		return NULL;
}

int primary_display_capture_framebuffer_decouple(unsigned long pbuf, unsigned int format)
{
	unsigned int i = 0;
	int ret = 0;
	disp_ddp_path_config *pconfig = NULL;
	m4u_client_t *m4uClient = NULL;
	unsigned int mva = 0;
	unsigned long va = 0;
	unsigned int mapped_size = 0;
	unsigned int w_xres = primary_display_get_width();
	unsigned int h_yres = primary_display_get_height();
	unsigned int pixel_byte = primary_display_get_dc_bpp() / 8;/* bpp is either 32 or 16, can not be other value */
	unsigned int pitch = 0;
	int buffer_size = h_yres * w_xres * pixel_byte;

	m4uClient = m4u_create_client();
	if (m4uClient == NULL) {
		DISPMSG("primary capture:Fail to alloc  m4uClient=0x%p\n", m4uClient);
		ret = -1;
		goto out;
	}

/* mva = pgc->dc_buf[pgc->dc_buf_id]; */
	pconfig = dpmgr_path_get_last_config(pgc->dpmgr_handle);

	if (_is_decouple_mode(pgc->session_mode) && !_is_mirror_mode(pgc->session_mode)) {
		w_xres = decouple_wdma_config.srcWidth;
		h_yres = decouple_wdma_config.srcHeight;
		pitch = decouple_wdma_config.dstPitch;
		mva = decouple_wdma_config.dstAddress;
	} else {
		w_xres = pconfig->wdma_config.srcWidth;
		h_yres = pconfig->wdma_config.srcHeight;
		pitch = pconfig->wdma_config.dstPitch;
		mva = pconfig->wdma_config.dstAddress;
	}
	buffer_size = h_yres * pitch;
	ASSERT((pitch / 4) >= w_xres);
/* dpmgr_get_input_address(pgc->dpmgr_handle,&mva); */
	ret = m4u_mva_map_kernel(mva, buffer_size, &va, &mapped_size);
	if (!va || ret < 0) {
		DISPERR("%s m4u_mva_map_kernel failed, mva:0x%08x, ret:%d\n",
			__func__, mva, ret);
		goto out;
	}

	DISPMSG("map 0x%08x with %d bytes to 0x%08lx with %d bytes\n", mva, buffer_size, va,
		mapped_size);

	ret =
	    m4u_cache_sync(m4uClient, M4U_PORT_DISP_WDMA0, va, buffer_size, mva,
			   M4U_CACHE_FLUSH_ALL);
#if 1
	{
		unsigned int j = 0;
		unsigned char tem_va[4];
		unsigned long src_va = va;

		for (i = 0; i < h_yres; i++) {
			for (j = 0; j < w_xres; j++) {
				memcpy((void *)tem_va, (void *)src_va, 4);
				*(unsigned long *)(pbuf + (i * w_xres + j) * 4) = 0xFF000000 | (tem_va[0] << 16) |
										(tem_va[1] << 8) | (tem_va[2]);
				src_va += 4;
			}
			src_va += (pitch - w_xres * 4);
		}
	}
#else
	memcpy(pbuf, va, mapped_size);
#endif
out:
	if (mapped_size)
		m4u_mva_unmap_kernel(mva, mapped_size, va);

	if (m4uClient != NULL)
		m4u_destroy_client(m4uClient);

	DISPMSG("primary capture: end\n");

	return ret;
}

int primary_display_capture_framebuffer_ovl(unsigned long pbuf, unsigned int fb_format)
{
	int ret = 0;
	cmdqRecHandle cmdq_handle = NULL;
	cmdqRecHandle cmdq_wait_handle = NULL;
	disp_ddp_path_config *pconfig = NULL;
	m4u_client_t *m4uClient = NULL;
	unsigned int mva = 0;
	unsigned int w_xres = primary_display_get_width();
	unsigned int h_yres = primary_display_get_height();
	unsigned int pixel_byte = primary_display_get_dc_bpp() / 8; /* bpp is either 32 or 16, can not be other value */
	int buffer_size = h_yres * w_xres * pixel_byte;
	int format;

	DISPMSG("primary capture: begin\n");

	switch (fb_format) {
	case MTK_FB_FORMAT_RGB888:
		format = eRGB888;
		break;
	case MTK_FB_FORMAT_BGR888:
		format = eBGR888;
		break;
	case MTK_FB_FORMAT_ARGB8888:
		format = eARGB8888;
		break;
	case MTK_FB_FORMAT_RGB565:
		format = eRGB565;
		break;
	case MTK_FB_FORMAT_UYVY:
		format = eYUV_420_2P_UYVY;
		break;
	case MTK_FB_FORMAT_BGRA8888:
		format = eBGRA8888;
		break;
	case MTK_FB_FORMAT_ABGR8888:
	default:
		format = eABGR8888;
		break;
	}

	disp_sw_mutex_lock(&(pgc->capture_lock));
	_primary_path_lock(__func__);

	if (pgc->state == DISP_SLEPT) {
		memset_io((void *)pbuf, 0, buffer_size);
		DISPMSG("primary capture:fill black for sleep\n");
		goto out;
	}

	if (!primary_display_cmdq_enabled()) {
		memset_io((void *)pbuf, 0, buffer_size);
		DISPMSG("primary capture:fill black to cmdq disable\n");
		goto out;
	}

	if (_is_decouple_mode(pgc->session_mode) || _is_mirror_mode(pgc->session_mode)) {
		primary_display_capture_framebuffer_decouple(pbuf, format);
		memset_io((void *)pbuf, 0, buffer_size);
		DISPMSG("primary capture: fill black for decouple & mirror mode End\n");
		goto out;
	}

	m4uClient = m4u_create_client();
	if (m4uClient == NULL) {
		DISPMSG("primary capture:Fail to alloc  m4uClient=0x%p\n", m4uClient);
		ret = -1;
		goto out;
	}

	ret = m4u_alloc_mva(m4uClient, M4U_PORT_DISP_WDMA0, pbuf, NULL, buffer_size,
			    M4U_PROT_READ | M4U_PROT_WRITE, 0, (unsigned int *)&mva);
	if (ret != 0) {
		DISPMSG("primary capture:Fail to allocate mva\n");
		ret = -1;
		goto out;
	}

	ret = m4u_cache_sync(m4uClient, M4U_PORT_DISP_WDMA0, pbuf, buffer_size, mva,
			     M4U_CACHE_FLUSH_BY_RANGE);
	if (ret != 0) {
		DISPMSG("primary capture:Fail to cach sync\n");
		ret = -1;
		goto out;
	}

	if (primary_display_cmdq_enabled()) {
		/* create config thread */
		ret = cmdqRecCreate(CMDQ_SCENARIO_PRIMARY_DISP, &cmdq_handle);
		if (ret != 0) {
			DISPMSG("primary capture:Fail to create primary cmdq handle for capture\n");
			ret = -1;
			goto out;
		}
		cmdqRecReset(cmdq_handle);

		/* create wait thread */
		ret = cmdqRecCreate(CMDQ_SCENARIO_DISP_SCREEN_CAPTURE, &cmdq_wait_handle);
		if (ret != 0) {
			DISPMSG("primary capture:Fail to create primary cmdq wait handle for capture\n");
			ret = -1;
			goto out;
		}
		cmdqRecReset(cmdq_wait_handle);

		/* configure config thread */
		_cmdq_insert_wait_frame_done_token_mira(cmdq_handle);
		dpmgr_path_memout_clock(pgc->dpmgr_handle, 1);

		pconfig = dpmgr_path_get_last_config(pgc->dpmgr_handle);
		pconfig->wdma_dirty = 1;
		pconfig->wdma_config.dstAddress = mva;
		pconfig->wdma_config.srcHeight = h_yres;
		pconfig->wdma_config.srcWidth = w_xres;
		pconfig->wdma_config.clipX = 0;
		pconfig->wdma_config.clipY = 0;
		pconfig->wdma_config.clipHeight = h_yres;
		pconfig->wdma_config.clipWidth = w_xres;
		pconfig->wdma_config.outputFormat = format;
		pconfig->wdma_config.useSpecifiedAlpha = 1;
		pconfig->wdma_config.alpha = 0xFF;
		pconfig->wdma_config.dstPitch = w_xres * DP_COLOR_BITS_PER_PIXEL(format) / 8;
		dpmgr_path_add_memout(pgc->dpmgr_handle, ENGINE_OVL0, cmdq_handle);

#if 0		/* do not have to call enable_cascade here, dpmgr_path_add_memout() will add ovl1 if necessary */
		if (ovl_get_status() == DDP_OVL1_STATUS_IDLE || ovl_get_status() == DDP_OVL1_STATUS_PRIMARY) {
			DISPMSG("dpmgr_path_enable_cascade called!\n");
			dpmgr_path_enable_cascade(pgc->dpmgr_handle, cmdq_handle);

			if (ovl_get_status() != DDP_OVL1_STATUS_PRIMARY)
				pconfig->ovl_dirty = 1;
		}
#endif

		ret = dpmgr_path_config(pgc->dpmgr_handle, pconfig, cmdq_handle);
		pconfig->wdma_dirty = 0;
		_cmdq_set_config_handle_dirty_mira(cmdq_handle);
		_cmdq_flush_config_handle_mira(cmdq_handle, 0);
		DISPMSG("primary capture:Flush add memout mva(0x%x)\n", mva);

		/* primary_display_diagnose(); */
		/* wait wdma0 sof */
		cmdqRecWait(cmdq_wait_handle, CMDQ_EVENT_DISP_WDMA0_SOF);
		cmdqRecFlush(cmdq_wait_handle);
		DISPMSG("primary capture:Flush wait wdma sof\n");

		cmdqRecReset(cmdq_handle);
		_cmdq_insert_wait_frame_done_token_mira(cmdq_handle);
		dpmgr_path_remove_memout(pgc->dpmgr_handle, cmdq_handle);
		cmdqRecClearEventToken(cmdq_handle, CMDQ_EVENT_DISP_WDMA0_SOF);
		_cmdq_set_config_handle_dirty_mira(cmdq_handle);
		/* flush remove memory to cmdq */
		_cmdq_flush_config_handle_mira(cmdq_handle, 1);
		DISPMSG("primary capture: Flush remove memout\n");

		dpmgr_path_memout_clock(pgc->dpmgr_handle, 0);
	}

out:

	cmdqRecDestroy(cmdq_handle);
	cmdqRecDestroy(cmdq_wait_handle);
	if (mva > 0)
		m4u_dealloc_mva(m4uClient, M4U_PORT_DISP_WDMA0, mva);

	if (m4uClient != 0)
		m4u_destroy_client(m4uClient);

	_primary_path_unlock(__func__);
	disp_sw_mutex_unlock(&(pgc->capture_lock));
	DISPMSG("primary capture: end\n");

	return ret;
}

int primary_display_capture_framebuffer(unsigned long pbuf)
{
#if 1
	unsigned int fb_layer_id = primary_display_get_option("FB_LAYER");
	unsigned int w_xres = primary_display_get_width();
	unsigned int h_yres = primary_display_get_height();
	/* bpp is either 32 or 16, can not be other value */
	unsigned int pixel_bpp = primary_display_get_dc_bpp() / 8;
	unsigned int w_fb = ALIGN_TO(w_xres, MTK_FB_ALIGNMENT);
	unsigned int fbsize = w_fb * h_yres * pixel_bpp; /* frame buffer size */
	unsigned long fbaddress = dpmgr_path_get_last_config(pgc->dpmgr_handle)->ovl_config[fb_layer_id].addr;
	unsigned long fbv = 0;
	unsigned int i = 0;
	unsigned long ttt = 0;

	DISPMSG("w_res=%d, h_yres=%d, pixel_bpp=%d, w_fb=%d, fbsize=%d, fbaddress=0x%lx\n",
		w_xres, h_yres, pixel_bpp, w_fb, fbsize, fbaddress);

	fbv = (unsigned long)ioremap(fbaddress, fbsize);
	DISPMSG("w_xres = %d, h_yres = %d, w_fb = %d, pixel_bpp = %d, fbsize = %d, fbaddress = 0x%lx\n",
		w_xres, h_yres, w_fb, pixel_bpp, fbsize, fbaddress);
	if (!fbv) {
		DISPMSG("[FB Driver], Unable to allocate memory for frame buffer: address=0x%lx, size=0x%08x\n",
			fbaddress, fbsize);
		return -1;
	}

	ttt = get_current_time_us();
	for (i = 0; i < h_yres; i++) {
		/* DISPMSG("i=%d, dst=0x%08x, src=%08x\n",
				i, (pbuf + i * w_xres * pixel_bpp), (fbv + i * w_fb * pixel_bpp));
		 */
		memcpy((void *)(pbuf + i * w_xres * pixel_bpp),
		       (void *)(fbv + i * w_fb * pixel_bpp), w_xres * pixel_bpp);
	}
	DISPMSG("capture framebuffer cost %ld us\n", get_current_time_us() - ttt);
	iounmap((void *)fbv);
#endif
	return -1;
}

uint32_t DISP_GetPanelBPP(void)
{
#if 0
	PANEL_COLOR_FORMAT fmt;

	disp_drv_init_context();

	if (disp_if_drv->get_panel_color_format == NULL)
		return DISP_STATUS_NOT_IMPLEMENTED;

	fmt = disp_if_drv->get_panel_color_format();
	switch (fmt) {
	case PANEL_COLOR_FORMAT_RGB332:
		return 8;
	case PANEL_COLOR_FORMAT_RGB444:
		return 12;
	case PANEL_COLOR_FORMAT_RGB565:
		return 16;
	case PANEL_COLOR_FORMAT_RGB666:
		return 18;
	case PANEL_COLOR_FORMAT_RGB888:
		return 24;
	default:
		return 0;
	}
#else
	return 0;		/* avoid build warning */
#endif
}

static uint32_t disp_fb_bpp = 32;
static uint32_t disp_fb_pages = 3;

uint32_t DISP_GetScreenBpp(void)
{
	return disp_fb_bpp;
}

uint32_t DISP_GetPages(void)
{
	return disp_fb_pages;
}

uint32_t DISP_GetFBRamSize(void)
{
	return ALIGN_TO(DISP_GetScreenWidth(), MTK_FB_ALIGNMENT) * DISP_GetScreenHeight() *
			((DISP_GetScreenBpp() + 7) >> 3) * DISP_GetPages();
}


uint32_t DISP_GetVRamSize(void)
{
#if 0
	/* Use a local static variable to cache the calculated vram size */
	/*  */
	static uint32_t vramSize;

	if (0 == vramSize) {
		disp_drv_init_context();

		/* /get framebuffer size */
		vramSize = DISP_GetFBRamSize();

		/* /get DXI working buffer size */
		vramSize += disp_if_drv->get_working_buffer_size();

		/* get assertion layer buffer size */
		vramSize += DAL_GetLayerSize();

		/* Align vramSize to 1MB */
		/*  */
		vramSize = ALIGN_TO_POW_OF_2(vramSize, 0x100000);

		DISP_LOG("DISP_GetVRamSize: %u bytes\n", vramSize);
	}

	return vramSize;
#else
	return 0; /* avoid build warning */
#endif


}

uint32_t DISP_GetVRamSizeBoot(char *cmdline)
{
#ifdef CONFIG_OF
	/* extern unsigned int vramsize; */

	/* extern int _parse_tag_videolfb(void); */
	_parse_tag_videolfb();
	if (vramsize == 0)
		vramsize = 0x3000000;
	/* not necessary */
	/* DISPMSG("[DT]display vram size = 0x%08x|%d\n", vramsize, vramsize); */
	return vramsize;
#else
	int ret = 0;
	char *p = NULL;
	uint32_t vramSize = 0;

	DISPMSG("%s, cmdline=%s\n", __func__, cmdline);
	p = strstr(cmdline, "vram=");
	if (p == NULL) {
		vramSize = 0x3000000;
		DISPERR("[FB driver]can not get vram size from lk\n");
	} else {
		p += 5;
		ret = kstrtol(p, 10, &vramSize);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);
		if (0 == vramSize)
			vramSize = 0x3000000;
	}

	DISPMSG("display vram size = 0x%08x|%d\n", vramSize, vramSize);
	return vramSize;
#endif
}


struct sg_table table;

int disp_hal_allocate_framebuffer(phys_addr_t pa_start, phys_addr_t pa_end, unsigned long *va,
				  unsigned long *mva)
{
#ifndef MTKFB_NO_M4U
	int ret = 0;
#endif

	*va = (unsigned long)ioremap_nocache(pa_start, pa_end - pa_start + 1);
	/* printk("disphal_allocate_fb, pa=%p, va=0x%lx\n", &pa_start, *va); */

	/* if (_get_init_setting("M4U")) */
	/* xuecheng, m4u not enabled now */
#ifndef MTKFB_NO_M4U
	if (1) {
		m4u_client_t *client;

		struct sg_table *sg_table = &table;

		sg_alloc_table(sg_table, 1, GFP_KERNEL);

		sg_dma_address(sg_table->sgl) = pa_start;
		sg_dma_len(sg_table->sgl) = (pa_end - pa_start + 1);
		client = m4u_create_client();
		if (IS_ERR_OR_NULL(client))
			DISPMSG("create client fail!\n");

		*mva = pa_start & 0xffffffffULL;
		ret = m4u_alloc_mva(client, M4U_PORT_DISP_OVL0, 0, sg_table, (pa_end - pa_start + 1),
				    M4U_PROT_READ | M4U_PROT_WRITE, M4U_FLAGS_FIX_MVA,
				    (unsigned int *)mva);
		/* m4u_alloc_mva(M4U_PORT_DISP_OVL0, pa_start, (pa_end - pa_start + 1), 0, 0, mva); */
		if (ret)
			DISPMSG("m4u_alloc_mva returns fail: %d\n", ret);

		/* printk("[DISPHAL] FB MVA is 0x%lx PA is %p\n", *mva, &pa_start);*/

	} else
#endif
	{
		*mva = pa_start & 0xffffffffULL;
	}

	return 0;
}

int primary_display_remap_irq_event_map(void)
{
	return 0; /* avoid build warning. */
}

unsigned int primary_display_get_option(const char *option)
{
	if (!strcmp(option, "FB_LAYER"))
		return 0;
	if (!strcmp(option, "ASSERT_LAYER")) {
#ifdef OVL_CASCADE_SUPPORT
		if (ovl_get_status() == DDP_OVL1_STATUS_PRIMARY
		    || ovl_get_status() == DDP_OVL1_STATUS_IDLE)
			return OVL_LAYER_NUM - 1;
		else
			return OVL_LAYER_NUM - 4 - 1;
#else
		return OVL_LAYER_NUM - 1;
#endif
	}
	if (!strcmp(option, "M4U_ENABLE"))
		return 1;

	ASSERT(0);
	return 0; /* avoid build warning */
}

int primary_display_get_debug_info(char *buf)
{
	/* resolution */
	/* cmd/video mode */
	/* display path */
	/* dsi data rate/lane number/state */
	/* primary path trigger count */
	/* frame done count */
	/* suspend/resume count */
	/* current fps 10s/5s/1s */
	/* error count and message */
	/* current state of each module on the path */
	return 0; /* avoid build warning */
}

#include "ddp_reg.h"

#define IS_READY(x)	((x)?"READY\t":"Not READY")
#define IS_VALID(x)	((x)?"VALID\t":"Not VALID")
#define READY_BIT0(x) ((DISP_REG_GET(DISPSYS_CONFIG_BASE + 0x8a0) & (1 << x)))
#define VALID_BIT0(x) ((DISP_REG_GET(DISPSYS_CONFIG_BASE + 0x8a4) & (1 << x)))
#define READY_BIT1(x) ((DISP_REG_GET(DISPSYS_CONFIG_BASE + 0x8a8) & (1 << x)))
#define VALID_BIT1(x) ((DISP_REG_GET(DISPSYS_CONFIG_BASE + 0x8ac) & (1 << x)))

int primary_display_check_path(char *stringbuf, int buf_len)
{

	return 0; /* status will print in config dump. */

#if 0
	int len = 0;

	DISPMSG("primary_display_check_path() check signal status:\n");
	if (stringbuf) {
		len +=
		    scnprintf(stringbuf + len, buf_len - len,
			      "|--------------------------------------------------------------------------------------|\n");
		len +=
		    scnprintf(stringbuf + len, buf_len - len,
			      "READY0=0x%08x, READY1=0x%08x, VALID0=0x%08x, VALID1=0x%08x\n",
			      DISP_REG_GET(DISPSYS_CONFIG_BASE + 0x8a0),
			      DISP_REG_GET(DISPSYS_CONFIG_BASE + 0x8a4),
			      DISP_REG_GET(DISPSYS_CONFIG_BASE + 0x8a8),
			      DISP_REG_GET(DISPSYS_CONFIG_BASE + 0x8ac));
		len +=
		    scnprintf(stringbuf + len, buf_len - len, "OVL0\t\t\t%s\t%s\n",
			      IS_READY(READY_BIT0(DDP_SIGNAL_OVL0__OVL0_MOUT)),
			      IS_VALID(READY_BIT0(DDP_SIGNAL_OVL0__OVL0_MOUT)));
		len +=
		    scnprintf(stringbuf + len, buf_len - len, "OVL0_MOUT:\t\t%s\t%s\n",
			      IS_READY(READY_BIT1(DDP_SIGNAL_OVL0_MOUT0__COLOR_SIN1)),
			      IS_VALID(READY_BIT1(DDP_SIGNAL_OVL0_MOUT0__COLOR_SIN1)));
		len +=
		    scnprintf(stringbuf + len, buf_len - len, "COLOR0_SEL:\t\t%s\t%s\n",
			      IS_READY(READY_BIT0(DDP_SIGNAL_COLOR_SEL__COLOR)),
			      IS_VALID(READY_BIT0(DDP_SIGNAL_COLOR_SEL__COLOR)));
		len +=
		    scnprintf(stringbuf + len, buf_len - len, "COLOR0:\t\t\t%s\t%s\n",
			      IS_READY(READY_BIT0(DDP_SIGNAL_COLOR__CCORR)),
			      IS_VALID(READY_BIT0(DDP_SIGNAL_COLOR__CCORR)));
		len +=
		    scnprintf(stringbuf + len, buf_len - len, "CCORR:\t\t%s\t%s\n",
			      IS_READY(READY_BIT0(DDP_SIGNAL_CCORR__AAL)),
			      IS_VALID(READY_BIT0(DDP_SIGNAL_CCORR__AAL)));
		len +=
		    scnprintf(stringbuf + len, buf_len - len, "AAL0:\t\t\t%s\t%s\n",
			      IS_READY(READY_BIT0(DDP_SIGNAL_AAL__GAMMA)),
			      IS_VALID(READY_BIT0(DDP_SIGNAL_AAL__GAMMA)));
		len +=
		    scnprintf(stringbuf + len, buf_len - len, "GAMMA:\t\t\t%s\t%s\n",
			      IS_READY(READY_BIT1(DDP_SIGNAL_GAMMA__DITHER)),
			      IS_VALID(READY_BIT1(DDP_SIGNAL_GAMMA__DITHER)));
		len +=
		    scnprintf(stringbuf + len, buf_len - len, "DITHER:\t\t\t%s\t%s\n",
			      IS_READY(READY_BIT1(DDP_SIGNAL_DITHER__DITHER_MOUT)),
			      IS_VALID(READY_BIT1(DDP_SIGNAL_DITHER__DITHER_MOUT)));
		len +=
		    scnprintf(stringbuf + len, buf_len - len, "DITHER_MOUT:\t\t%s\t%s\n",
			      IS_READY(READY_BIT1(DDP_SIGNAL_DITHER_MOUT0__RDMA0)),
			      IS_VALID(READY_BIT1(DDP_SIGNAL_DITHER_MOUT0__RDMA0)));
		len +=
		    scnprintf(stringbuf + len, buf_len - len, "RDMA0:\t\t\t%s\t%s\n",
			      IS_READY(READY_BIT1(DDP_SIGNAL_RDMA0__RDMA0_SOUT)),
			      IS_VALID(READY_BIT1(DDP_SIGNAL_RDMA0__RDMA0_SOUT)));
		len +=
		    scnprintf(stringbuf + len, buf_len - len, "RDMA0_SOUT:\t\t%s\t%s\n",
			      IS_READY(READY_BIT1(DDP_SIGNAL_RDMA0_SOUT0__UFOE_SIN0)),
			      IS_VALID(READY_BIT1(DDP_SIGNAL_RDMA0_SOUT0__UFOE_SIN0)));
		len +=
		    scnprintf(stringbuf + len, buf_len - len, "UFOE_SEL:\t\t%s\t%s\n",
			      IS_READY(READY_BIT0(DDP_SIGNAL_UFOE_SEL__UFOE)),
			      IS_VALID(READY_BIT0(DDP_SIGNAL_UFOE_SEL__UFOE)));
		len +=
		    scnprintf(stringbuf + len, buf_len - len, "UFOE:\t\t\t%s\t%s\n",
			      IS_READY(READY_BIT0(DDP_SIGNAL_UFOE__UFOE_MOUT)),
			      IS_VALID(READY_BIT0(DDP_SIGNAL_UFOE__UFOE_MOUT)));
		len +=
		    scnprintf(stringbuf + len, buf_len - len, "UFOE_MOUT:\t\t%s\t%s\n",
			      IS_READY(READY_BIT0(DDP_SIGNAL_UFOE_MOUT0__DSI0_SIN0)),
			      IS_VALID(READY_BIT0(DDP_SIGNAL_UFOE_MOUT0__DSI0_SIN0)));
		len +=
		    scnprintf(stringbuf + len, buf_len - len, "DSI0_SEL:\t\t%s\t%s\n",
			      IS_READY(READY_BIT1(DDP_SIGNAL_DIS0_SEL__DSI0)),
			      IS_VALID(READY_BIT1(DDP_SIGNAL_DIS0_SEL__DSI0)));
	} else {
		DISPMSG
		    ("|--------------------------------------------------------------------------------------|\n");
		DISPMSG("READY0=0x%08x, READY1=0x%08x, VALID0=0x%08x, VALID1=0x%08x\n",
			DISP_REG_GET(DISPSYS_CONFIG_BASE + 0x8a0),
			DISP_REG_GET(DISPSYS_CONFIG_BASE + 0x8a4),
			DISP_REG_GET(DISPSYS_CONFIG_BASE + 0x8a8),
			DISP_REG_GET(DISPSYS_CONFIG_BASE + 0x8ac));
		DISPMSG("OVL0\t\t\t%s\t%s\n", IS_READY(READY_BIT0(DDP_SIGNAL_OVL0__OVL0_MOUT)),
			IS_VALID(READY_BIT0(DDP_SIGNAL_OVL0__OVL0_MOUT)));
		DISPMSG("OVL0_MOUT:\t\t%s\t%s\n",
			IS_READY(READY_BIT1(DDP_SIGNAL_OVL0_MOUT0__COLOR_SIN1)),
			IS_VALID(READY_BIT1(DDP_SIGNAL_OVL0_MOUT0__COLOR_SIN1)));
		DISPMSG("COLOR0_SEL:\t\t%s\t%s\n",
			IS_READY(READY_BIT0(DDP_SIGNAL_COLOR_SEL__COLOR)),
			IS_VALID(READY_BIT0(DDP_SIGNAL_COLOR_SEL__COLOR)));
		DISPMSG("COLOR0:\t\t\t%s\t%s\n", IS_READY(READY_BIT0(DDP_SIGNAL_COLOR__CCORR)),
			IS_VALID(READY_BIT0(DDP_SIGNAL_COLOR__CCORR)));
		DISPMSG("CCORR:\t\t%s\t%s\n", IS_READY(READY_BIT0(DDP_SIGNAL_CCORR__AAL)),
			IS_VALID(READY_BIT0(DDP_SIGNAL_CCORR__AAL)));
		DISPMSG("AAL0:\t\t\t%s\t%s\n", IS_READY(READY_BIT0(DDP_SIGNAL_AAL__GAMMA)),
			IS_VALID(READY_BIT0(DDP_SIGNAL_AAL__GAMMA)));
		DISPMSG("GAMMA:\t\t\t%s\t%s\n", IS_READY(READY_BIT1(DDP_SIGNAL_GAMMA__DITHER)),
			IS_VALID(READY_BIT1(DDP_SIGNAL_GAMMA__DITHER)));
		DISPMSG("DITHER:\t\t\t%s\t%s\n",
			IS_READY(READY_BIT1(DDP_SIGNAL_DITHER__DITHER_MOUT)),
			IS_VALID(READY_BIT1(DDP_SIGNAL_DITHER__DITHER_MOUT)));
		DISPMSG("DITHER_MOUT:\t\t%s\t%s\n",
			IS_READY(READY_BIT1(DDP_SIGNAL_DITHER_MOUT0__RDMA0)),
			IS_VALID(READY_BIT1(DDP_SIGNAL_DITHER_MOUT0__RDMA0)));
		DISPMSG("RDMA0:\t\t\t%s\t%s\n", IS_READY(READY_BIT1(DDP_SIGNAL_RDMA0__RDMA0_SOUT)),
			IS_VALID(READY_BIT1(DDP_SIGNAL_RDMA0__RDMA0_SOUT)));
		DISPMSG("RDMA0_SOUT:\t\t%s\t%s\n",
			IS_READY(READY_BIT1(DDP_SIGNAL_RDMA0_SOUT0__UFOE_SIN0)),
			IS_VALID(READY_BIT1(DDP_SIGNAL_RDMA0_SOUT0__UFOE_SIN0)));
		DISPMSG("UFOE_SEL:\t\t%s\t%s\n", IS_READY(READY_BIT0(DDP_SIGNAL_UFOE_SEL__UFOE)),
			IS_VALID(READY_BIT0(DDP_SIGNAL_UFOE_SEL__UFOE)));
		DISPMSG("UFOE:\t\t\t%s\t%s\n", IS_READY(READY_BIT0(DDP_SIGNAL_UFOE__UFOE_MOUT)),
			IS_VALID(READY_BIT0(DDP_SIGNAL_UFOE__UFOE_MOUT)));
		DISPMSG("UFOE_MOUT:\t\t%s\t%s\n",
			IS_READY(READY_BIT0(DDP_SIGNAL_UFOE_MOUT0__DSI0_SIN0)),
			IS_VALID(READY_BIT0(DDP_SIGNAL_UFOE_MOUT0__DSI0_SIN0)));
		DISPMSG("DSI0_SEL:\t\t%s\t%s\n", IS_READY(READY_BIT1(DDP_SIGNAL_DIS0_SEL__DSI0)),
			IS_VALID(READY_BIT1(DDP_SIGNAL_DIS0_SEL__DSI0)));
	}

	return len;
#endif
}

int primary_display_lcm_ATA(void)
{
	DISP_STATUS ret = DISP_STATUS_OK;

	DISPFUNC();
	_primary_path_lock(__func__);
	if (pgc->state == 0) {
		DISPMSG("ATA_LCM, primary display path is already sleep, skip\n");
		goto done;
	}

	DISPMSG("[ATA_LCM]primary display path stop[begin]\n");
	if (primary_display_is_video_mode())
		dpmgr_path_ioctl(pgc->dpmgr_handle, NULL, DDP_STOP_VIDEO_MODE, NULL);

	DISPMSG("[ATA_LCM]primary display path stop[end]\n");
	ret = disp_lcm_ATA(pgc->plcm);
	dpmgr_path_start(pgc->dpmgr_handle, CMDQ_DISABLE);
	/* for video mode, we need to force trigger here
	 * for cmd mode, just set DPREC_EVENT_CMDQ_SET_EVENT_ALLOW when trigger loop start
	 */
	if (primary_display_is_video_mode())
		dpmgr_path_trigger(pgc->dpmgr_handle, NULL, CMDQ_DISABLE);

done:
	_primary_path_unlock(__func__);
	return ret;
}

int fbconfig_get_esd_check_test(uint32_t dsi_id, uint32_t cmd, uint8_t *buffer, uint32_t num)
{
	int ret = 0;

	/* extern int fbconfig_get_esd_check(DSI_INDEX dsi_id, uint32_t cmd, uint8_t *buffer, uint32_t num); */

	_primary_path_lock(__func__);

	if (pgc->state == DISP_SLEPT) {
		DISPMSG("[ESD]primary display path is slept?? -- skip esd check\n");
		_primary_path_unlock(__func__);
		goto done;
	}
	/* primary_display_esd_check_enable(0); */
	/* / 1: stop path */
	_cmdq_stop_trigger_loop();
	if (dpmgr_path_is_busy(pgc->dpmgr_handle))
		DISPMSG("[ESD]wait frame done ret:%d\n", ret);

	dpmgr_path_stop(pgc->dpmgr_handle, CMDQ_DISABLE);
	DISPMSG("[ESD]stop dpmgr path[end]\n");
	if (dpmgr_path_is_busy(pgc->dpmgr_handle))
		DISPMSG("[ESD]wait frame done ret:%d\n", ret);

	/* dpmgr_path_reset(pgc->dpmgr_handle, CMDQ_DISABLE); */
	ret = fbconfig_get_esd_check(dsi_id, cmd, buffer, num);
	dpmgr_path_start(pgc->dpmgr_handle, CMDQ_DISABLE);
	DISPMSG("[ESD]start dpmgr path[end]\n");
	if (primary_display_is_video_mode()) {
		/* for video mode, we need to force trigger here */
		/* for cmd mode, just set DPREC_EVENT_CMDQ_SET_EVENT_ALLOW when trigger loop start */
		dpmgr_path_trigger(pgc->dpmgr_handle, NULL, CMDQ_DISABLE);
	}
	_cmdq_start_trigger_loop();
	DISPMSG("[ESD]start cmdq trigger loop[end]\n");
	_primary_path_unlock(__func__);

done:
	return ret;
}

int Panel_Master_dsi_config_entry(const char *name, void *config_value)
{
	int ret = 0;
	int force_trigger_path = 0;
	uint32_t *config_dsi = (uint32_t *)config_value;
	LCM_PARAMS *lcm_param = NULL;
	LCM_DRIVER *pLcm_drv = NULL;
	int esd_check_backup = 0;

	DISPFUNC();

	pLcm_drv = DISP_GetLcmDrv();
	esd_check_backup = atomic_read(&esd_check_task_wakeup);
	if (!strcmp(name, "DRIVER_IC_RESET") || !strcmp(name, "PM_DDIC_CONFIG")) {
		primary_display_esd_check_enable(0);
		msleep(2500);
	}
	_primary_path_lock(__func__);

	lcm_param = disp_lcm_get_params(pgc->plcm);
	if (pgc->state == DISP_SLEPT) {
		DISPERR("[Pmaster]Panel_Master: primary display path is slept??\n");
		goto done;
	}
	/* / Esd Check : Read from lcm */
	/* / the following code is to */
	/* / 0: lock path */
	/* / 1: stop path */
	/* / 2: do esd check (!!!) */
	/* / 3: start path */
	/* / 4: unlock path */
	/* / 1: stop path */
	_cmdq_stop_trigger_loop();

	if (dpmgr_path_is_busy(pgc->dpmgr_handle))
		DISPMSG("[ESD]wait frame done ret:%d\n", ret);


	dpmgr_path_stop(pgc->dpmgr_handle, CMDQ_DISABLE);
	DISPMSG("[ESD]stop dpmgr path[end]\n");

	if (dpmgr_path_is_busy(pgc->dpmgr_handle))
		DISPMSG("[ESD]wait frame done ret:%d\n", ret);

	dpmgr_path_reset(pgc->dpmgr_handle, CMDQ_DISABLE);
	if ((!strcmp(name, "PM_CLK")) || (!strcmp(name, "PM_SSC")))
		Panel_Master_primary_display_config_dsi(name, *config_dsi);
	else if (!strcmp(name, "PM_DDIC_CONFIG")) {
		Panel_Master_DDIC_config();
		force_trigger_path = 1;
	} else if (!strcmp(name, "DRIVER_IC_RESET")) {
		if (pLcm_drv && pLcm_drv->init_power)
			pLcm_drv->init_power();
		if (pLcm_drv)
			pLcm_drv->init();
		else
			ret = -1;
		force_trigger_path = 1;
	}
	dpmgr_path_start(pgc->dpmgr_handle, CMDQ_DISABLE);
	if (primary_display_is_video_mode()) {
		/* for video mode, we need to force trigger here */
		/* for cmd mode, just set DPREC_EVENT_CMDQ_SET_EVENT_ALLOW when trigger loop start */
		dpmgr_path_trigger(pgc->dpmgr_handle, NULL, CMDQ_DISABLE);
		force_trigger_path = 0;
	}
	_cmdq_start_trigger_loop();
	DISPMSG("[Pmaster]start cmdq trigger loop\n");
done:
	_primary_path_unlock(__func__);

	if (force_trigger_path) {/* command mode only */
		primary_display_trigger(0, NULL, 0);
		DISPMSG("[Pmaster]force trigger display path\r\n");
	}
	atomic_set(&esd_check_task_wakeup, esd_check_backup);

	return ret;
}

/*
mode: 0, switch to cmd mode; 1, switch to vdo mode
*/
int primary_display_switch_dst_mode(int mode)
{
	DISP_STATUS ret = DISP_STATUS_ERROR;
#ifdef DISP_SWITCH_DST_MODE
	void *lcm_cmd = NULL;

	DISPFUNC();
	_primary_path_switch_dst_lock();
	disp_sw_mutex_lock(&(pgc->capture_lock));
	if (pgc->plcm->params->type != LCM_TYPE_DSI) {
		pr_debug("[primary_display_switch_dst_mode] Error, only support DSI IF\n");
		goto done;
	}
	if (pgc->state == DISP_SLEPT) {
		DISPMSG
		    ("[primary_display_switch_dst_mode], primary display path is already sleep, skip\n");
		goto done;
	}

	if (mode == primary_display_cur_dst_mode) {
		DISPMSG("[primary_display_switch_dst_mode]not need switch, cur_mode:%d, switch_mode:%d\n",
			  primary_display_cur_dst_mode, mode);
		goto done;
	}
	/* DISPMSG("[primary_display_switch_mode]need switch, cur_mode:%d, switch_mode:%d\n",
		     primary_display_cur_dst_mode, mode);
	 */
	lcm_cmd = disp_lcm_switch_mode(pgc->plcm, mode);
	if (lcm_cmd == NULL) {
		DISPMSG("[primary_display_switch_dst_mode]get lcm cmd fail %d, %d\n",
			  primary_display_cur_dst_mode, mode);
		goto done;
	} else {
		int temp_mode = 0;

		if (0 != dpmgr_path_ioctl(pgc->dpmgr_handle, pgc->cmdq_handle_config,
					  DDP_SWITCH_LCM_MODE, lcm_cmd)) {
			pr_debug("switch lcm mode fail, return directly\n");
			goto done;
		}
		_primary_path_lock(__func__);
		temp_mode = (int)(pgc->plcm->params->dsi.mode);
		pgc->plcm->params->dsi.mode = pgc->plcm->params->dsi.switch_mode;
		pgc->plcm->params->dsi.switch_mode = temp_mode;
		dpmgr_path_set_video_mode(pgc->dpmgr_handle, primary_display_is_video_mode());
		if (0 != dpmgr_path_ioctl(pgc->dpmgr_handle, pgc->cmdq_handle_config,
					  DDP_SWITCH_DSI_MODE, lcm_cmd)) {
			pr_debug("switch dsi mode fail, return directly\n");
			_primary_path_unlock(__func__);
			goto done;
		}
	}
	primary_display_sodi_rule_init();
	_cmdq_stop_trigger_loop();
	_cmdq_build_trigger_loop();
	_cmdq_start_trigger_loop();
	_cmdq_reset_config_handle(); /* must do this */
	_cmdq_insert_wait_frame_done_token();

	primary_display_cur_dst_mode = mode;

	if (primary_display_is_video_mode())
		dpmgr_map_event_to_irq(pgc->dpmgr_handle, DISP_PATH_EVENT_IF_VSYNC,
				       DDP_IRQ_RDMA0_DONE);
	else
		dpmgr_map_event_to_irq(pgc->dpmgr_handle, DISP_PATH_EVENT_IF_VSYNC,
				       DDP_IRQ_DSI0_EXT_TE);

	_primary_path_unlock(__func__);
	ret = DISP_STATUS_OK;
done:
	/* dprec_handle_option(0x0); */
	disp_sw_mutex_unlock(&(pgc->capture_lock));
	_primary_path_switch_dst_unlock();
#else
	pr_debug("[ERROR: primary_display_switch_dst_mode]this function not enable in disp driver\n");
#endif
	return ret;
}

/* warm reset ovl0 and ovl1 by CMDQ */
void primary_display_reset_ovl_by_cmdq(unsigned int force)
{
	cmdqRecWaitNoClear(pgc->cmdq_handle_config, CMDQ_EVENT_MUTEX0_STREAM_EOF);

	if (force == 0) { /* warm reset ovl */
		DISPMSG("warm reset ovl\n");
		/* reset ovl0 */
		DISP_REG_SET(pgc->cmdq_handle_config, DISP_REG_OVL_RST, 0x1);
		DISP_REG_SET(pgc->cmdq_handle_config, DISP_REG_OVL_RST, 0x0);
		cmdqRecPoll(pgc->cmdq_handle_config, disp_addr_convert(DISP_REG_OVL_STA), 0, 0x1);

		/* reset ovl1 */
		if (ovl_get_status() == DDP_OVL1_STATUS_PRIMARY) {
			DISP_REG_SET(pgc->cmdq_handle_config,
				     DISP_REG_OVL_RST + DISP_OVL_INDEX_OFFSET, 0x1);
			DISP_REG_SET(pgc->cmdq_handle_config,
				     DISP_REG_OVL_RST + DISP_OVL_INDEX_OFFSET, 0x0);
			cmdqRecPoll(pgc->cmdq_handle_config,
				    disp_addr_convert(DISP_REG_OVL_STA + DISP_OVL_INDEX_OFFSET), 0, 0x1);
		}
	} else if (force == 1) { /* force reset ovl */
		DISPMSG("force reset ovl\n");
		/* reset ovl0 */
		DISP_REG_SET_FIELD(pgc->cmdq_handle_config, RST_FLD_FORCE_RST, DISP_REG_OVL_RST, 0x1);
		DISP_REG_SET_FIELD(pgc->cmdq_handle_config, RST_FLD_FORCE_RST, DISP_REG_OVL_RST, 0x0);
		cmdqRecPoll(pgc->cmdq_handle_config, disp_addr_convert(DISP_REG_OVL_STA), 0, 0x1);

		/* reset ovl1 */
		if (ovl_get_status() == DDP_OVL1_STATUS_PRIMARY) {
			DISP_REG_SET_FIELD(pgc->cmdq_handle_config, RST_FLD_FORCE_RST,
					   DISP_REG_OVL_RST + DISP_OVL_INDEX_OFFSET, 0x1);
			DISP_REG_SET_FIELD(pgc->cmdq_handle_config, RST_FLD_FORCE_RST,
					   DISP_REG_OVL_RST + DISP_OVL_INDEX_OFFSET, 0x0);
			cmdqRecPoll(pgc->cmdq_handle_config,
				    disp_addr_convert(DISP_REG_OVL_STA + DISP_OVL_INDEX_OFFSET), 0,
				    0x1);
		}
	}
}


/* extern void DSI_ForceConfig(int forceconfig); */
/* extern int DSI_set_roi(int x, int y); */
/* extern int DSI_check_roi(void); */
/* extern atomic_t ESDCheck_byCPU; */

static int width_array[] = { 2560, 1440, 1920, 1280, 1200, 800, 960, 640 };
static int heigh_array[] = { 1440, 2560, 1200, 800, 1920, 1280, 640, 960 };
static int array_id[] = { 6, 2, 7, 4, 3, 0, 5, 1 };
LCM_PARAMS *lcm_param2 = NULL;
disp_ddp_path_config data_config2;

int primary_display_te_test(void)
{
	int ret = 0;
	int try_cnt = 3;
	int time_interval = 0;
	int time_interval_max = 0;
	long long time_te = 0;
	long long time_framedone = 0;

	pr_debug("display_test te begin\n");
	if (primary_display_is_video_mode()) {
		pr_debug("Video Mode No TE\n");
		return ret;
	}

	while (try_cnt >= 0) {
		try_cnt--;
		ret = dpmgr_wait_event_timeout(pgc->dpmgr_handle, DISP_PATH_EVENT_IF_VSYNC, HZ * 1);
		time_te = sched_clock();

		dpmgr_path_trigger(pgc->dpmgr_handle, NULL, CMDQ_DISABLE);
		ret = dpmgr_wait_event_timeout(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_DONE, HZ * 1);
		time_framedone = sched_clock();
		time_interval = (int)(time_framedone - time_te);
		time_interval = time_interval / 1000;
		if (time_interval > time_interval_max)
			time_interval_max = time_interval;

	}
	if (time_interval_max > 20000)
		ret = 0;
	else
		ret = -1;

	if (ret >= 0)
		pr_debug("[display_test_result]==>Force On TE Open!(%d)\n", time_interval_max);
	else
		pr_debug("[display_test_result]==>Force On TE Closed!(%d)\n", time_interval_max);

	pr_debug("display_test te  end\n");
	return ret;

}

#if 0
int primary_display_fps_test(void)
{
	int ret = 0;
	unsigned int w_backup = 0;
	unsigned int h_backup = 0;
	LCM_DSI_MODE_CON dsi_mode_backup = primary_display_is_video_mode();

	memset((void *)&data_config2, 0, sizeof(data_config2));
	lcm_param2 = NULL;
	memcpy((void *)&data_config2, (void *)dpmgr_path_get_last_config(pgc->dpmgr_handle),
	       sizeof(disp_ddp_path_config));
	w_backup = data_config2.dst_w;
	h_backup = data_config2.dst_h;
	DISPMSG("[display_test]w_backup %d h_backup %d dsi_mode_backup %d\n", w_backup, h_backup,
		  dsi_mode_backup);

	/* for dsi config */
	DSI_ForceConfig(1);

	DISPMSG("[display_test]FPS config[begin]\n");

	lcm_param2 = disp_lcm_get_params(pgc->plcm);
	lcm_param2->dsi.mode = SYNC_PULSE_VDO_MODE;
	lcm_param2->dsi.vertical_active_line = 1280;
	lcm_param2->dsi.horizontal_active_pixel = 360;

	data_config2.dst_w = 360;
	data_config2.dst_h = 1280;
	data_config2.dispif_config.dsi.vertical_active_line = 1280;
	data_config2.dispif_config.dsi.horizontal_active_pixel = 360;
	data_config2.dispif_config.dsi.mode = SYNC_PULSE_VDO_MODE;
	data_config2.dst_dirty = 1;

	dpmgr_path_set_video_mode(pgc->dpmgr_handle, primary_display_is_video_mode());
	DISPMSG("[display_test]==>FPS set vdo mode done, is_vdo_mode:%d\n",
		  primary_display_is_video_mode());
	dpmgr_path_connect(pgc->dpmgr_handle, CMDQ_DISABLE);
	dpmgr_path_config(pgc->dpmgr_handle, &data_config2, CMDQ_DISABLE);
	data_config2.dst_dirty = 0;
	DISPMSG("[display_test]FPS config[end]\n");

	DISPMSG("[display_test]Start dpmgr path[begin]\n");
	dpmgr_path_start(pgc->dpmgr_handle, CMDQ_DISABLE);
	if (dpmgr_path_is_busy(pgc->dpmgr_handle))
		DISPERR("[display_test]==>Fatal error, we didn't trigger display path but it's already busy\n");

	DISPMSG("[display_test]Start dpmgr path[end]\n");

	DISPMSG("[display_test]Trigger dpmgr path[begin]\n");
	dpmgr_path_trigger(pgc->dpmgr_handle, NULL, CMDQ_DISABLE);
	DISPMSG("[display_test]Trigger dpmgr path[end]\n");

	/* check fps bonding: rdma frame end interval < 12ms */
	disp_record_rdma_end_interval(1);

	/* loop 50 times to get max rdma end interval */
	int i = 50;

	while (i--)
		dpmgr_wait_event_timeout(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_DONE, HZ * 1);

	unsigned long long min_interval = disp_get_rdma_min_interval();
	unsigned long long max_interval = disp_get_rdma_max_interval();

	disp_record_rdma_end_interval(0);

	DISPMSG("[display_test]Check RDMA frame end interval:%lld[end]\n", min_interval);
	if (min_interval < 12 * 1000000) {
		DISPMSG("[display_test_result]=>0.No limit\n");
	} else {
		DISPMSG("[display_test_result]=>1.limit max_interval %lld\n", max_interval);
		if (max_interval < 13 * 1000000)
			DISPMSG("[display_test_result]=>2.naughty enable\n");
	}

	DISPMSG("[display_test]Stop dpmgr path[begin]\n");
	dpmgr_path_stop(pgc->dpmgr_handle, CMDQ_DISABLE);
	DISPMSG("[display_test]Stop dpmgr path[end]\n");

	DISPMSG("[display_test]Restore path config[begin]\n");
	lcm_param2 = disp_lcm_get_params(pgc->plcm);
	lcm_param2->dsi.mode = dsi_mode_backup;
	lcm_param2->dsi.vertical_active_line = h_backup;
	lcm_param2->dsi.horizontal_active_pixel = w_backup;
	data_config2.dispif_config.dsi.vertical_active_line = h_backup;
	data_config2.dispif_config.dsi.horizontal_active_pixel = w_backup;
	data_config2.dispif_config.dsi.mode = dsi_mode_backup;
	data_config2.dst_w = w_backup;
	data_config2.dst_h = h_backup;
	data_config2.dst_dirty = 1;

	dpmgr_path_set_video_mode(pgc->dpmgr_handle, primary_display_is_video_mode());
	DISPMSG("[display_test]==>Restore mode done, is_vdo_mode:%d\n",
		  primary_display_is_video_mode());
	DISPMSG("[display_test]==>Restore resolution done, w=%d, h=%d\n", w_backup, h_backup);
	dpmgr_path_connect(pgc->dpmgr_handle, CMDQ_DISABLE);
	dpmgr_path_config(pgc->dpmgr_handle, &data_config2, CMDQ_DISABLE);
	data_config2.dst_dirty = 0;
	DSI_ForceConfig(0);
	DISPMSG("[display_test]Restore path config[end]\n");
	return ret;
}
#endif

int primary_display_roi_test(int x, int y)
{
	int ret = 0;

	pr_debug("display_test roi begin\n");
	pr_debug("display_test roi set roi %d, %d\n", x, y);
	DSI_set_roi(x, y);
	msleep(50);
	dpmgr_path_trigger(pgc->dpmgr_handle, NULL, CMDQ_DISABLE);
	msleep(50);
	pr_debug("display_test DSI_check_roi\n");
	ret = DSI_check_roi();
	msleep(20);
	if (ret == 0)
		pr_debug("[display_test_result]==>DSI_ROI limit!\n");
	else
		pr_debug("[display_test_result]==>DSI_ROI Normal!\n");

	pr_debug("display_test set roi %d, %d\n", 0, 0);

	DSI_set_roi(0, 0);

	msleep(20);
	DISPMSG("display_test end\n");
	return ret;
}

int primary_display_resolution_test(void)
{
	int ret = 0;
	int i = 0;
	unsigned int w_backup = 0;
	unsigned int h_backup = 0;
	int dst_width = 0;
	int dst_heigh = 0;
	LCM_DSI_MODE_CON dsi_mode_backup = primary_display_is_video_mode();

	memset((void *)&data_config2, 0, sizeof(data_config2));
	lcm_param2 = NULL;
	memcpy((void *)&data_config2, (void *)dpmgr_path_get_last_config(pgc->dpmgr_handle),
	       sizeof(disp_ddp_path_config));
	w_backup = data_config2.dst_w;
	h_backup = data_config2.dst_h;
	DISPMSG("[display_test resolution]w_backup %d h_backup %d dsi_mode_backup %d\n",
		  w_backup, h_backup, dsi_mode_backup);
	/* for dsi config */
	DSI_ForceConfig(1);
	for (i = 0; i < sizeof(width_array) / sizeof(int); i++) {
		dst_width = width_array[i];
		dst_heigh = heigh_array[i];
		DISPMSG("[display_test resolution] width %d, heigh %d\n", dst_width, dst_heigh);
		lcm_param2 = disp_lcm_get_params(pgc->plcm);
		lcm_param2->dsi.mode = CMD_MODE;
		lcm_param2->dsi.horizontal_active_pixel = dst_width;
		lcm_param2->dsi.vertical_active_line = dst_heigh;

		data_config2.dispif_config.dsi.mode = CMD_MODE;
		data_config2.dispif_config.dsi.horizontal_active_pixel = dst_width;
		data_config2.dispif_config.dsi.vertical_active_line = dst_heigh;


		data_config2.dst_w = dst_width;
		data_config2.dst_h = dst_heigh;

		data_config2.ovl_config[0].layer = 0;
		data_config2.ovl_config[0].layer_en = 0;
		data_config2.ovl_config[1].layer = 1;
		data_config2.ovl_config[1].layer_en = 0;
		data_config2.ovl_config[2].layer = 2;
		data_config2.ovl_config[2].layer_en = 0;
		data_config2.ovl_config[3].layer = 3;
		data_config2.ovl_config[3].layer_en = 0;

		data_config2.dst_dirty = 1;
		data_config2.ovl_dirty = 1;

		dpmgr_path_set_video_mode(pgc->dpmgr_handle, primary_display_is_video_mode());

		dpmgr_path_config(pgc->dpmgr_handle, &data_config2, CMDQ_DISABLE);
		data_config2.dst_dirty = 0;
		data_config2.ovl_dirty = 0;

		dpmgr_path_start(pgc->dpmgr_handle, CMDQ_DISABLE);

		if (dpmgr_path_is_busy(pgc->dpmgr_handle))
			DISPERR("[display_test]==>Fatal error, we didn't trigger display path but it's already busy\n");

		dpmgr_path_trigger(pgc->dpmgr_handle, NULL, CMDQ_DISABLE);

		ret = dpmgr_wait_event_timeout(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_DONE, HZ * 1);
		if (ret > 0) {
			if (!dpmgr_path_is_busy(pgc->dpmgr_handle)) {
				if (i == 0)
					DISPMSG("[display_test resolution] display_result 0x%x unlimited!\n",
						  array_id[i]);
				else if (i == 1)
					DISPMSG("[display_test resolution] display_result 0x%x unlimited (W<H)\n",
						  array_id[i]);
				else
					DISPMSG("[display_test resolution] display_result 0x%x(%d x %d)\n",
						  array_id[i], dst_width, dst_heigh);

				break;
			}
		}
		dpmgr_path_reset(pgc->dpmgr_handle, CMDQ_DISABLE);
	}
	dpmgr_path_stop(pgc->dpmgr_handle, CMDQ_DISABLE);
	lcm_param2 = disp_lcm_get_params(pgc->plcm);
	lcm_param2->dsi.mode = dsi_mode_backup;
	lcm_param2->dsi.vertical_active_line = h_backup;
	lcm_param2->dsi.horizontal_active_pixel = w_backup;
	data_config2.dispif_config.dsi.vertical_active_line = h_backup;
	data_config2.dispif_config.dsi.horizontal_active_pixel = w_backup;
	data_config2.dispif_config.dsi.mode = dsi_mode_backup;
	data_config2.dst_w = w_backup;
	data_config2.dst_h = h_backup;
	data_config2.dst_dirty = 1;
	dpmgr_path_set_video_mode(pgc->dpmgr_handle, primary_display_is_video_mode());
	dpmgr_path_connect(pgc->dpmgr_handle, CMDQ_DISABLE);
	dpmgr_path_config(pgc->dpmgr_handle, &data_config2, CMDQ_DISABLE);
	data_config2.dst_dirty = 0;
	DSI_ForceConfig(0);

	return ret;
}

int primary_display_check_test(void)
{
	int ret = 0;
	int esd_backup = 0;

	DISPMSG("[display_test]Display test[Start]\n");
	_primary_path_lock(__func__);
	/* disable esd check */
	if (atomic_read(&esd_check_task_wakeup)) {
		esd_backup = 1;
		primary_display_esd_check_enable(0);
		msleep(2000);
		DISPMSG("[display_test]Disable esd check end\n");
	}
	/* if suspend => return */
	if (pgc->state == DISP_SLEPT) {
		DISPMSG("[display_test_result]======================================\n");
		DISPMSG("[display_test_result]==>Test Fail : primary display path is slept\n");
		DISPMSG("[display_test_result]======================================\n");
		goto done;
	}
	/* stop trigger loop */
	DISPMSG("[display_test]Stop trigger loop[begin]\n");
	_cmdq_stop_trigger_loop();
	atomic_set(&ESDCheck_byCPU, 1);
	if (dpmgr_path_is_busy(pgc->dpmgr_handle)) {
		DISPMSG("[display_test]==>primary display path is busy\n");
		ret = dpmgr_wait_event_timeout(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_DONE, HZ * 1);
		if (ret <= 0)
			dpmgr_path_reset(pgc->dpmgr_handle, CMDQ_DISABLE);

		DISPMSG("[display_test]==>wait frame done ret:%d\n", ret);
	}
	DISPMSG("[display_test]Stop trigger loop[end]\n");
	/* test rdma res after reset */
	/* primary_display_rdma_res_test(); */

	/* test force te */
	primary_display_te_test();

	/* test roi */
	primary_display_roi_test(30, 30);

	/* test resolution test */
	primary_display_resolution_test();

	/* mutex fps */
	/* primary_display_fps_test(); */

	DISPMSG("[display_test]start dpmgr path[begin]\n");
	dpmgr_path_start(pgc->dpmgr_handle, CMDQ_DISABLE);
	if (dpmgr_path_is_busy(pgc->dpmgr_handle))
		DISPERR("[display_test]==>Fatal error, we didn't trigger display path but it's already busy\n");

	DISPMSG("[display_test]start dpmgr path[end]\n");

	DISPMSG("[display_test]Start trigger loop[begin]\n");
	_cmdq_start_trigger_loop();
	DISPMSG("[display_test]Start trigger loop[end]\n");
	atomic_set(&ESDCheck_byCPU, 0);

done:

	/* restore esd */
	if (esd_backup == 1) {
		primary_display_esd_check_enable(1);
		DISPMSG("[display_test]Restore esd check\n");
	}
	/* unlock path */
	_primary_path_unlock(__func__);
	DISPMSG("[display_test]Display test[End]\n");
	return ret;
}

/*
* Now the normal display vsync is DDP_IRQ_RDMA0_DONE in vdo mode, but when enter TUI,
* we must protect the rdma0, then, should
* switch it to the DDP_IRQ_DSI0_FRAME_DONE.
*/
int display_vsync_switch_to_dsi(unsigned int flg)
{
	if (!primary_display_is_video_mode())
		return 0;

	if (!flg) {
		dpmgr_map_event_to_irq(pgc->dpmgr_handle, DISP_PATH_EVENT_IF_VSYNC,
					DDP_IRQ_RDMA0_DONE);
		dsi_enable_irq(DISP_MODULE_DSI0, NULL, 0);

	} else {
		dpmgr_map_event_to_irq(pgc->dpmgr_handle, DISP_PATH_EVENT_IF_VSYNC,
					DDP_IRQ_DSI0_FRAME_DONE);
		dsi_enable_irq(DISP_MODULE_DSI0, NULL, 1);
	}

	return 0;
}



static DISP_POWER_STATE power_stat_backup;
static int session_mode_backup;

int display_enter_tui(void)
{
	msleep(500);
	DISPMSG("TDDP: %s\n", __func__);

	MMProfileLogEx(ddp_mmp_get_events()->tui, MMProfileFlagStart, 0, 0);

	_primary_path_lock(__func__);

	if (primary_get_state() != DISP_ALIVE) {
		DISPERR("Can't enter tui: current_stat=%d is not alive\n", primary_get_state());
		goto err0;
	}

	power_stat_backup = primary_set_state(DISP_BLANK);

	if (primary_display_is_mirror_mode()) {
		DISPERR("Can't enter tui: current_mode=%s\n", session_mode_spy(pgc->session_mode));
		goto err1;
	}

#ifdef MTK_DISP_IDLE_LP
	_disp_primary_path_exit_idle(__func__, 0);
#endif
	session_mode_backup = pgc->session_mode;
	primary_display_switch_mode_nolock(DISP_SESSION_DECOUPLE_MODE, pgc->session_id, 0);
	display_vsync_switch_to_dsi(1);

	MMProfileLogEx(ddp_mmp_get_events()->tui, MMProfileFlagPulse, 0, 1);
	_primary_path_unlock(__func__);
	return 0;

err1:
	primary_set_state(power_stat_backup);

err0:
	MMProfileLogEx(ddp_mmp_get_events()->tui, MMProfileFlagEnd, 0, 0);
	display_vsync_switch_to_dsi(0);
	_primary_path_unlock(__func__);

	return -1;
}

int display_exit_tui(void)
{
	MMProfileLogEx(ddp_mmp_get_events()->tui, MMProfileFlagPulse, 1, 1);

	_primary_path_lock(__func__);

	primary_set_state(power_stat_backup);

	/* trigger rdma to display last normal buffer
	_decouple_update_rdma_config_nolock();*/
	/* workaround: wait until this frame triggered to lcm */
	msleep(32);
	primary_display_switch_mode_nolock(session_mode_backup, pgc->session_id, 0);

	_primary_path_unlock(__func__);

	MMProfileLogEx(ddp_mmp_get_events()->tui, MMProfileFlagEnd, 0, 0);
	DISPMSG("TDDP: %s\n", __func__);

	return 0;
}

int primary_display_get_init_status(void)
{
	return g_is_inited;
}

static int allocate_freeze_buffer(void)
{
	int height = primary_display_get_height();
	int width = primary_display_get_width();
	int bpp = primary_display_get_dc_bpp();
	int buffer_size =  width * height * bpp / 8;

	freeze_buffer_info = allocat_decouple_buffer(buffer_size);
	if (freeze_buffer_info != NULL)
		pgc->freeze_buf = freeze_buffer_info->mva;
	else
		return -1;

	return 0;
}

static int release_freeze_buffer(unsigned int need_primary_lock)
{
	if (need_primary_lock)
		_primary_path_lock(__func__);

	if (freeze_buffer_info) {
		ion_free(freeze_buffer_info->client, freeze_buffer_info->handle);
		ion_client_destroy(freeze_buffer_info->client);
		kfree(freeze_buffer_info);
		freeze_buffer_info = NULL;
		pgc->freeze_buf = 0;
	}

	if (need_primary_lock)
		_primary_path_unlock(__func__);
	return 0;
}

int display_freeze_mode(int enable, int need_lock)
{
	DISPMSG("%s, enable:%d\n", __func__, enable);

	MMProfileLogEx(ddp_mmp_get_events()->tui, MMProfileFlagStart, 0, 0);

	if (need_lock)
		_primary_path_lock(__func__);
	if (enable) {

		if (primary_get_state() == DISP_FREEZE || primary_get_state() == DISP_SLEPT) {
			DISPERR("Can't enter freeze mode: current_stat=%d is not alive\n", primary_get_state());
			goto end;
		}

		allocate_freeze_buffer();
		power_stat_backup = primary_set_state(DISP_FREEZE);
		if (primary_display_is_mirror_mode()) {
			DISPERR("Can't enter freeze mode: current_mode=%s\n", session_mode_spy(pgc->session_mode));
			goto err1;
		}
#ifdef MTK_DISP_IDLE_LP
		_disp_primary_path_exit_idle(__func__, 0);
#endif
		session_mode_backup = pgc->session_mode;
		primary_display_switch_mode_nolock(DISP_SESSION_DECOUPLE_MODE, pgc->session_id, 0);

	} else {
		if (primary_get_state() != DISP_FREEZE)
			goto end;
		primary_set_state(power_stat_backup);
		primary_display_switch_mode_nolock(session_mode_backup, pgc->session_id, 0);
		release_freeze_buffer(0);
	}
	if (need_lock)
		_primary_path_unlock(__func__);
	return 0;

err1:
	primary_set_state(power_stat_backup);

end:
	if (need_lock)
		_primary_path_unlock(__func__);

	return -1;
}

#if defined(OVL_TIME_SHARING)
int primary_display_disable_ovl2mem(void)
{
	DISPMSG("%s\n", __func__);

	_primary_path_lock(__func__);

	if (_is_decouple_mode(pgc->session_mode) &&
		pgc->state == DISP_SLEPT &&
		pgc->force_on_wdma_path == 1) {

		/* msleep(16); */ /* wait last frame done */
		usleep_range(16000, 17000);
		if (dpmgr_path_is_busy(pgc->ovl2mem_path_handle))
			dpmgr_wait_event_timeout(pgc->ovl2mem_path_handle, DISP_PATH_EVENT_FRAME_COMPLETE, HZ);

		DISPMSG("[POWER]stop cmdq[begin]\n");
		_cmdq_stop_trigger_loop();
		DISPMSG("[POWER]stop cmdq[end]\n");

		dpmgr_path_power_off(pgc->ovl2mem_path_handle, CMDQ_DISABLE);

#ifndef CONFIG_MTK_CLKMGR
		ddp_clk_unprepare(DISP_MTCMOS_CLK);
#endif
		DISPMSG("disable ovl power\n");
		pgc->force_on_wdma_path = 0;
	}

	_primary_path_unlock(__func__);

	return 1;
}
#endif
