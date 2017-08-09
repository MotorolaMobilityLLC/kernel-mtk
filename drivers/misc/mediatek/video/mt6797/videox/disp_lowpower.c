#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/module.h>
#include <linux/wait.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/rtpm_prio.h>
#include <linux/types.h>
#include <linux/ktime.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/slab.h>
#include "ion_drv.h"
#include "mtk_ion.h"
#include "mt_idle.h"
/* #include "mt_spm_reg.h" */ /* FIXME: tmp comment */
/* #include "pcm_def.h" */ /* FIXME: tmp comment */
#include "mt_spm_idle.h"
#include "mt-plat/mt_smi.h"
#include "m4u.h"
#include "m4u_port.h"

#if 1
#include "disp_drv_platform.h"
#include "debug.h"
#include "disp_drv_log.h"
#include "disp_lcm.h"
#include "disp_utils.h"
#include "disp_session.h"
#include "primary_display.h"
#include "disp_helper.h"
#include "cmdq_def.h"
#include "cmdq_record.h"
#include "cmdq_reg.h"
#include "cmdq_core.h"
#include "ddp_manager.h"
#include "disp_lcm.h"
#include "ddp_clkmgr.h"
#include "mt_smi.h"
/* #include "mmdvfs_mgr.h" */
#include "disp_drv_log.h"
#include "disp_lowpower.h"

#define MMSYS_CLK_LOW (0)
#define MMSYS_CLK_HIGH (1)
#define MMSYS_CLK_MEDIUM (2)

#define idlemgr_pgc		_get_idlemgr_context()
#define golden_setting_pgc	_get_golden_setting_context()

#define NO_SPM /* FIXME: tmp define for bring up */

/* Local API */
/*********************************************************************************************************************/
static int _primary_path_idlemgr_monitor_thread(void *data);

static disp_idlemgr_context *_get_idlemgr_context(void)
{
	static int is_inited;
	static disp_idlemgr_context g_idlemgr_context;

	if (!is_inited) {
		init_waitqueue_head(&(g_idlemgr_context.idlemgr_wait_queue));
		g_idlemgr_context.session_mode_before_enter_idle = DISP_INVALID_SESSION_MODE;
		g_idlemgr_context.is_primary_idle = 0;
		g_idlemgr_context.enterulps = 0;
		g_idlemgr_context.idlemgr_last_kick_time = ~(0ULL);
		g_idlemgr_context.primary_display_idlemgr_task
			= kthread_create(_primary_path_idlemgr_monitor_thread, NULL, "disp_idlemgr");

		/* wakeup process when idlemgr init */
		/* wake_up_process(g_idlemgr_context.primary_display_idlemgr_task); */

		is_inited = 1;
	}

	return &g_idlemgr_context;
}

int primary_display_idlemgr_init(void)
{
	wake_up_process(idlemgr_pgc->primary_display_idlemgr_task);
	return 0;
}

static golden_setting_context *_get_golden_setting_context(void)
{
	unsigned int width	= 0;
	unsigned int height	= 0;
	static int is_inited;
	static golden_setting_context g_golden_setting_context;

	if (!is_inited) {

		/* default setting */
		g_golden_setting_context.is_dc = 0;
		g_golden_setting_context.is_display_idle = 0;
		g_golden_setting_context.is_wrot_sram = 0;
		g_golden_setting_context.mmsys_clk = MMSYS_CLK_MEDIUM; /* 286: defalut ; 182: low ; 364: high */

		width = primary_get_lcm()->params->width;
		height = primary_get_lcm()->params->height;
		if (width == 1080 && height == 1920)
			g_golden_setting_context.hrt_magicnum = 2;
		else if (width == 720 && height == 1280)
			g_golden_setting_context.hrt_magicnum = 4;

		/* set hrtnum max */
		g_golden_setting_context.hrt_num = g_golden_setting_context.hrt_magicnum;

		/* fifo mode : 0/1/2 */
		if (g_golden_setting_context.is_display_idle)
			g_golden_setting_context.fifo_mode = 0;
		else if (g_golden_setting_context.hrt_num > g_golden_setting_context.hrt_magicnum)
			g_golden_setting_context.fifo_mode = 2;
		else
			g_golden_setting_context.fifo_mode = 1;

		is_inited = 1;
	}

	return &g_golden_setting_context;
}


static void set_is_display_idle(unsigned int is_displayidle)
{

	if (golden_setting_pgc->is_display_idle != is_displayidle) {
		golden_setting_pgc->is_display_idle = is_displayidle;

		if (is_displayidle)
			golden_setting_pgc->fifo_mode = 0;
		else if (golden_setting_pgc->hrt_num <= golden_setting_pgc->hrt_magicnum)
			golden_setting_pgc->fifo_mode = 1;
		else
			golden_setting_pgc->fifo_mode = 2;
		/* wait for mmsys mgr ready */
		#if 0
		/* notify to mmsys mgr */
		if (disp_helper_get_option(DISP_OPT_DYNAMIC_SWITCH_MMSYSCLK))
			if (is_displayidle)
				mmdvfs_notify_mmclk_switch_request(MMDVFS_EVENT_UI_IDLE_ENTER);
			else
				mmdvfs_notify_mmclk_switch_request(MMDVFS_EVENT_UI_IDLE_EXIT);
		#endif
	}

}


#if 0 /* defined but not used */
static unsigned int get_hrtnum(void)
{
	return golden_setting_pgc->hrt_num;
}
#endif


static void set_share_sram(unsigned int is_share_sram)
{
	if (golden_setting_pgc->is_wrot_sram != is_share_sram)
		golden_setting_pgc->is_wrot_sram = is_share_sram;
}

#if 0 /* defined but not used */
static unsigned int use_wrot_sram(void)
{
	return golden_setting_pgc->is_wrot_sram;
}

static void set_mmsys_clk(unsigned int clk)
{
	if (golden_setting_pgc->mmsys_clk != clk)
		golden_setting_pgc->mmsys_clk = clk;
}

static unsigned int get_mmsys_clk(void)
{
	return golden_setting_pgc->mmsys_clk;
}
#endif

static int primary_display_set_idle_stat(int is_idle)
{
	int old_stat = idlemgr_pgc->is_primary_idle;

	idlemgr_pgc->is_primary_idle = is_idle;
	return old_stat;
}

int primary_display_dsi_vfp_change(int state)
{
	int ret = 0;
	cmdqRecHandle handle = NULL;

	cmdqRecCreate(CMDQ_SCENARIO_DISP_SCREEN_CAPTURE, &handle);
	cmdqRecReset(handle);
	_cmdq_insert_wait_frame_done_token_mira(handle);
	if (state == 1)
		dpmgr_path_ioctl(primary_get_dpmgr_handle(), handle,
			DDP_DSI_PORCH_CHANGE,
			&primary_get_lcm()->params->dsi.vertical_frontporch_for_low_power);
	else if (state == 0)
		dpmgr_path_ioctl(primary_get_dpmgr_handle(), handle,
			DDP_DSI_PORCH_CHANGE,
			&primary_get_lcm()->params->dsi.vertical_frontporch);
	cmdqRecFlushAsync(handle);
	cmdqRecDestroy(handle);
	return ret;
}

void _idle_set_golden_setting(void)
{
	cmdqRecHandle handle;
	disp_ddp_path_config *pconfig = dpmgr_path_get_last_config(primary_get_dpmgr_handle());

	/* no need lock */
	/* 1.create and reset cmdq */
	cmdqRecCreate(CMDQ_SCENARIO_PRIMARY_DISP, &handle);

	cmdqRecReset(handle);

	/* 2.wait sof */
	cmdqRecWaitNoClear(handle, CMDQ_EVENT_MUTEX0_STREAM_EOF);


	/* 3.golden setting */
	dpmgr_path_ioctl(primary_get_dpmgr_handle(), handle, DDP_RDMA_GOLDEN_SETTING, pconfig);

	/* 4.flush */
	cmdqRecFlushAsync(handle);
	cmdqRecDestroy(handle);
}

/* Share wrot sram for vdo mode increase enter sodi ratio */
void _acquire_wrot_resource_nolock(CMDQ_EVENT_ENUM resourceEvent)
{
	cmdqRecHandle handle;

	int32_t acquireResult;
	disp_ddp_path_config *pconfig = dpmgr_path_get_last_config(primary_get_dpmgr_handle());

	DISPMSG("[disp_lowpower]%s\n", __func__);

	/* 1.create and reset cmdq */
	cmdqRecCreate(CMDQ_SCENARIO_PRIMARY_DISP, &handle);

	cmdqRecReset(handle);

	/* 2. wait sof */
	_cmdq_insert_wait_frame_done_token_mira(handle);

	/* 3.try to share wrot sram */
	acquireResult = cmdqRecWriteForResource(handle, resourceEvent,
		0x1400A000+0xb0, 1, ~0);
	if (acquireResult < 0) {
		/* acquire resource fail */
		DISPCHECK("acquire resource fail\n");

		cmdqRecDestroy(handle);
		return;

	} else {
		/* acquire resource success */
		/* cmdqRecClearEventToken(handle, resourceEvent); //???cmdq do it */

		/* set rdma golden setting parameters*/
		set_share_sram(1);

		/* add instr for modification rdma fifo regs */
		/* dpmgr_handle can cover both dc & dl */
		if (disp_helper_get_option(DISP_OPT_DYNAMIC_RDMA_GOLDEN_SETTING))
			dpmgr_path_ioctl(primary_get_dpmgr_handle(), handle, DDP_RDMA_GOLDEN_SETTING, pconfig);

	}

	cmdqRecFlushAsync(handle);
	cmdqRecDestroy(handle);
}

static int32_t _acquire_wrot_resource(CMDQ_EVENT_ENUM resourceEvent)
{
	/* need lock */
	primary_display_manual_lock();
	_acquire_wrot_resource_nolock(resourceEvent);
	primary_display_manual_unlock();

	return 0;
}


void _release_wrot_resource_nolock(CMDQ_EVENT_ENUM resourceEvent)
{
	cmdqRecHandle handle;
	disp_ddp_path_config *pconfig = dpmgr_path_get_last_config(primary_get_dpmgr_handle());

	DISPMSG("[disp_lowpower]%s\n", __func__);

	/* 1.create and reset cmdq */
	cmdqRecCreate(CMDQ_SCENARIO_PRIMARY_DISP, &handle);

	cmdqRecReset(handle);

	/* 2.wait sof */
	cmdqRecWaitNoClear(handle, CMDQ_EVENT_MUTEX0_STREAM_EOF);

	/* 3.release share sram */
	cmdqRecWrite(handle, 0x1400A000+0xb0, 0, ~0); /* why need ??? */
	cmdqRecReleaseResource(handle, resourceEvent);

	/* set rdma golden setting parameters*/
	set_share_sram(0);

	/* 4.add instr for modification rdma fifo regs */
	/* rdma: dpmgr_handle can cover both dc & dl */
	if (disp_helper_get_option(DISP_OPT_DYNAMIC_RDMA_GOLDEN_SETTING))
		dpmgr_path_ioctl(primary_get_dpmgr_handle(), handle, DDP_RDMA_GOLDEN_SETTING, pconfig);

	cmdqRecFlushAsync(handle);
	cmdqRecDestroy(handle);
}

static int32_t _release_wrot_resource(CMDQ_EVENT_ENUM resourceEvent)
{
	/* need lock  */
	primary_display_manual_lock();
	_release_wrot_resource_nolock(resourceEvent);
	primary_display_manual_unlock();

	return 0;
}

/* wait for mmsys mgr ready */
#if 0
int _switch_mmsys_clk(int mmsys_clk_old, int mmsys_clk_new)
{
	int ret = 0;
	cmdqRecHandle handle;
	disp_ddp_path_config *pconfig = dpmgr_path_get_last_config(primary_get_dpmgr_handle());

	DISPMSG("[disp_lowpower]%s\n", __func__);

	/* 1.create and reset cmdq */
	cmdqRecCreate(CMDQ_SCENARIO_PRIMARY_DISP, &handle);

	cmdqRecReset(handle);

	/* 2.wait sof */
	cmdqRecWaitNoClear(handle, CMDQ_EVENT_MUTEX0_STREAM_EOF);


	/* 3.switch mmsys clk */
	if (mmsys_clk_old == MMSYS_CLK_MEDIUM && mmsys_clk_new == MMSYS_CLK_LOW) {
		cmdqRecWrite(handle, 0x10000048, 0x07000000, 0x07000000); /* clear */
		cmdqRecWrite(handle, 0x10000044, 0x04000000, 0x04000000); /* set syspll2_d2 */
		cmdqRecWrite(handle, 0x10000044, 8, 8); /* update */

		/* set rdma golden setting parameters */
		set_mmsys_clk(MMSYS_CLK_LOW);

	} else if (mmsys_clk_old == MMSYS_CLK_LOW && mmsys_clk_new == MMSYS_CLK_MEDIUM) {
		/* enable vencpll */
		ddp_clk_prepare_enable(MM_VENCPLL);
		cmdqRecWrite(handle, 0x10000048, 0x07000000, 0x07000000); /* clear */
		cmdqRecWrite(handle, 0x10000044, 0x02000000, 0x02000000); /* set vencpll */
		cmdqRecWrite(handle, 0x10000044, 8, 8); /* update */

		/* set rdma golden setting parameters */
		set_mmsys_clk(MMSYS_CLK_MEDIUM);

	} else if (mmsys_clk_old == MMSYS_CLK_MEDIUM &&  mmsys_clk_new == MMSYS_CLK_HIGH) {
		/* 286 => 364 */
		/* set rdma golden setting parameters */
		set_mmsys_clk(MMSYS_CLK_HIGH);

	} else if (mmsys_clk_old == MMSYS_CLK_HIGH &&  mmsys_clk_new == MMSYS_CLK_MEDIUM) {
		/* 364 => 286 */
		/* set rdma golden setting parameters */
		set_mmsys_clk(MMSYS_CLK_MEDIUM);
	}


	/* 4.add instr for modification rdma fifo regs */
	/* rdma: dpmgr_handle can cover both dc & dl */
	if (disp_helper_get_option(DISP_OPT_DYNAMIC_RDMA_GOLDEN_SETTING))
		dpmgr_path_ioctl(primary_get_dpmgr_handle(), handle, DDP_RDMA_GOLDEN_SETTING, pconfig);

	cmdqRecFlush(handle); /* blocking */
	cmdqRecDestroy(handle);


	/* disable vencpll */
	if (mmsys_clk_old == MMSYS_CLK_MEDIUM && mmsys_clk_new == MMSYS_CLK_LOW)
		ddp_clk_disable_unprepare(MM_VENCPLL);

	return get_mmsys_clk();
}

int primary_display_switch_mmsys_clk(int mmsys_clk_old, int mmsys_clk_new)
{
	/* need lock */
	primary_display_manual_lock();
	_switch_mmsys_clk(mmsys_clk_old, mmsys_clk_new);
	primary_display_manual_unlock();
}
#endif

void _primary_display_disable_mmsys_clk(void)
{
	/* no  need lock now */
	if (disp_helper_get_option(DISP_OPT_USE_CMDQ)) {
		DISPCHECK("[LP]1.display cmdq trigger loop stop[begin]\n");
		_cmdq_stop_trigger_loop();
		DISPCHECK("[LP]1.display cmdq trigger loop stop[end]\n");
	}

	DISPCHECK("[LP]2.primary display path stop[begin]\n");
	dpmgr_path_stop(primary_get_dpmgr_handle(), CMDQ_DISABLE);
	DISPCHECK("[LP]2.primary display path stop[end]\n");

	if (dpmgr_path_is_busy(primary_get_dpmgr_handle())) {
		DISPERR("[LP]2.stop display path failed, still busy\n");
		dpmgr_path_reset(primary_get_dpmgr_handle(), CMDQ_DISABLE);
		/* even path is busy(stop fail), we still need to continue power off other module/devices */
	}

	primary_display_diagnose();
	/* can not release fence here */
	DISPCHECK("[LP]3.dpmanager path power off[begin]\n");
	dpmgr_path_power_off(primary_get_dpmgr_handle(), CMDQ_DISABLE);

	if (primary_display_is_decouple_mode()) {
		DISPCHECK("[LP]3.1 dpmanager path power off: ovl2men [begin]\n");
		if (primary_get_ovl2mem_handle())
			dpmgr_path_power_off(primary_get_ovl2mem_handle(), CMDQ_DISABLE);
		else
			DISPERR("display is decouple mode, but ovl2mem_path_handle is null\n");

		DISPCHECK("[LP]3.1 dpmanager path power off: ovl2men [end]\n");
	}
	DISPCHECK("[LP]3.dpmanager path power off[end]\n");
	if (disp_helper_get_option(DISP_OPT_MET_LOG))
		set_enterulps(1);
}

void _primary_display_enable_mmsys_clk(void)
{
	disp_ddp_path_config *data_config;

	/* do something */
	DISPCHECK("[LP]1.dpmanager path power on[begin]\n");
	if (primary_display_is_decouple_mode()) {
		if (primary_get_ovl2mem_handle() == NULL) {
			DISPERR("display is decouple mode, but ovl2mem_path_handle is null\n");
			return;
		}

		DISPCHECK("[LP]1.1 dpmanager path power on: ovl2men [begin]\n");
		dpmgr_path_power_on(primary_get_ovl2mem_handle(), CMDQ_DISABLE);
		DISPCHECK("[LP]1.1 dpmanager path power on: ovl2men [end]\n");
	}

	dpmgr_path_power_on(primary_get_dpmgr_handle(), CMDQ_DISABLE);
	DISPCHECK("[LP]1.dpmanager path power on[end]\n");
	if (disp_helper_get_option(DISP_OPT_MET_LOG))
		set_enterulps(0);

	DISPCHECK("[LP]2.dpmanager path config[begin]\n");

	/* disconnect primary path first *
	* because MMsys config register may not power off during early suspend
	* BUT session mode may change in primary_display_switch_mode() */
	ddp_disconnect_path(DDP_SCENARIO_PRIMARY_ALL, NULL);
	ddp_disconnect_path(DDP_SCENARIO_PRIMARY_RDMA0_COLOR0_DISP, NULL);


	dpmgr_path_connect(primary_get_dpmgr_handle(), CMDQ_DISABLE);
	if (primary_display_is_decouple_mode())
		dpmgr_path_connect(primary_get_ovl2mem_handle(), CMDQ_DISABLE);

	data_config = dpmgr_path_get_last_config(primary_get_dpmgr_handle());
	data_config->dst_dirty = 1;
	dpmgr_path_config(primary_get_dpmgr_handle(), data_config, NULL);

	if (primary_display_is_decouple_mode()) {

		data_config = dpmgr_path_get_last_config(primary_get_dpmgr_handle());
		data_config->rdma_dirty = 1;
		dpmgr_path_config(primary_get_dpmgr_handle(), data_config, NULL);


		data_config = dpmgr_path_get_last_config(primary_get_ovl2mem_handle());
		data_config->dst_dirty = 1;
		dpmgr_path_config(primary_get_ovl2mem_handle(), data_config, NULL);
	}


	DISPCHECK("[LP]2.dpmanager path config[end]\n");


	DISPCHECK("[LP]3.dpmgr path start[begin]\n");
	dpmgr_path_start(primary_get_dpmgr_handle(), CMDQ_DISABLE);

	if (primary_display_is_decouple_mode())
		dpmgr_path_start(primary_get_ovl2mem_handle(), CMDQ_DISABLE);


	/*
	int dpmgr_mutex_reset(disp_path_handle dp_handle, void * handle);
	dpmgr_mutex_reset(pgc->dpmgr_handle,CMDQ_DISABLE);
	*/

	DISPCHECK("[LP]3.dpmgr path start[end]\n");
	if (dpmgr_path_is_busy(primary_get_dpmgr_handle()))
		DISPERR("[LP]3.Fatal error, we didn't trigger display path but it's already busy\n");

	primary_display_diagnose();

	if (disp_helper_get_option(DISP_OPT_USE_CMDQ)) {
		DISPCHECK("[LP]4.start cmdq[begin]\n");
		_cmdq_start_trigger_loop();
		DISPCHECK("[LP]4.start cmdq[end]\n");
	}

	/* (in suspend) when we stop trigger loop
	* if no other thread is running, cmdq may disable its clock
	* all cmdq event will be cleared after suspend */
	cmdqCoreSetEvent(CMDQ_EVENT_DISP_WDMA0_EOF);
}

/* Share wrot sram end */
void _vdo_mode_enter_idle(void)
{
	DISPMSG("[disp_lowpower]%s\n", __func__);

	/* backup for DL <-> DC */
	idlemgr_pgc->session_mode_before_enter_idle = primary_get_sess_mode();

	/* DL -> DC*/
	if (!primary_is_sec() &&
	    primary_get_sess_mode() == DISP_SESSION_DIRECT_LINK_MODE &&
	    (disp_helper_get_option(DISP_OPT_IDLEMGR_SWTCH_DECOUPLE) ||
	     disp_helper_get_option(DISP_OPT_SMART_OVL))) {

		/* smart_ovl_try_switch_mode_nolock(); */
		/* switch to decouple mode */
		do_primary_display_switch_mode(DISP_SESSION_DECOUPLE_MODE, primary_get_sess_id(), 0, NULL, 0);

		set_is_dc(1);

		/* merge setting when the last one */
		/*
		if (disp_helper_get_option(DISP_OPT_DYNAMIC_RDMA_GOLDEN_SETTING))
			_idle_set_golden_setting();
		*/
	}

	/* Disable irq & increase vfp */
	if (!primary_is_sec()) {
		if (disp_helper_get_option(DISP_OPT_IDLEMGR_DISABLE_ROUTINE_IRQ)) {
			/* disable routine irq before switch to decouple mode, otherwise we need to disable two paths */
			dpmgr_path_enable_irq(primary_get_dpmgr_handle(), NULL, DDP_IRQ_LEVEL_ERROR);
		}

		if (primary_get_lcm()->params->dsi.vertical_frontporch_for_low_power)
			primary_display_dsi_vfp_change(1);
	}

	/* DC homeidle share wrot sram */
	if (disp_helper_get_option(DISP_OPT_SHARE_SRAM)
		&& primary_get_sess_mode() == DISP_SESSION_DECOUPLE_MODE)
		enter_share_sram(CMDQ_SYNC_RESOURCE_WROT0);

	/* set golden setting  */
	set_is_display_idle(1);
	if (disp_helper_get_option(DISP_OPT_DYNAMIC_RDMA_GOLDEN_SETTING))
		_idle_set_golden_setting();

	/* Enable sodi - need wait golden setting done ??? */
#ifndef CONFIG_MTK_FPGA
#ifndef NO_SPM
	if (disp_helper_get_option(DISP_OPT_SODI_SUPPORT)) {
		/* set power down mode forbidden */
		spm_sodi_mempll_pwr_mode(1);
		spm_enable_sodi(1);
	}
#endif
#endif
}

void _vdo_mode_leave_idle(void)
{
	DISPMSG("[disp_lowpower]%s\n", __func__);

	/* Disable sodi */
#ifndef CONFIG_MTK_FPGA
#ifndef NO_SPM
	if (disp_helper_get_option(DISP_OPT_SODI_SUPPORT))
		spm_enable_sodi(0);
#endif
#endif

	/* set golden setting */
	set_is_display_idle(0);
	if (disp_helper_get_option(DISP_OPT_DYNAMIC_RDMA_GOLDEN_SETTING))
		_idle_set_golden_setting();

	/* DC homeidle share wrot sram */
	if (disp_helper_get_option(DISP_OPT_SHARE_SRAM)
		&& primary_get_sess_mode() == DISP_SESSION_DECOUPLE_MODE)
		leave_share_sram(CMDQ_SYNC_RESOURCE_WROT0);

	/* Enable irq & restore vfp */
	if (!primary_is_sec()) {

		if (primary_get_lcm()->params->dsi.vertical_frontporch_for_low_power)
			primary_display_dsi_vfp_change(0);

		if (disp_helper_get_option(DISP_OPT_IDLEMGR_DISABLE_ROUTINE_IRQ)) {
			/* enable routine irq after switch to directlink mode, otherwise we need to disable two paths */
			dpmgr_path_enable_irq(primary_get_dpmgr_handle(), NULL, DDP_IRQ_LEVEL_ALL);
		}
	}

	/* DC -> DL */
	if (disp_helper_get_option(DISP_OPT_IDLEMGR_SWTCH_DECOUPLE) &&
		!disp_helper_get_option(DISP_OPT_SMART_OVL)) {
		/* switch to the mode before idle */
		do_primary_display_switch_mode(idlemgr_pgc->session_mode_before_enter_idle,
			primary_get_sess_id(), 0, NULL, 0);

		set_is_dc(0);
		if (disp_helper_get_option(DISP_OPT_DYNAMIC_RDMA_GOLDEN_SETTING))
			_idle_set_golden_setting();
	}
}

void _cmd_mode_enter_idle(void)
{
	DISPMSG("[disp_lowpower]%s\n", __func__);

	/* need leave share sram for disable mmsys clk */
	if (disp_helper_get_option(DISP_OPT_SHARE_SRAM))
		leave_share_sram(CMDQ_SYNC_RESOURCE_WROT0);

	/* please keep last */
	if (disp_helper_get_option(DISP_OPT_IDLEMGR_ENTER_ULPS))
		_primary_display_disable_mmsys_clk();
}

void _cmd_mode_leave_idle(void)
{
	DISPMSG("[disp_lowpower]%s\n", __func__);

	if (disp_helper_get_option(DISP_OPT_IDLEMGR_ENTER_ULPS))
		_primary_display_enable_mmsys_clk();


	if (disp_helper_get_option(DISP_OPT_SHARE_SRAM))
		enter_share_sram(CMDQ_SYNC_RESOURCE_WROT0);
}

void primary_display_idlemgr_enter_idle_nolock(void)
{
	if (primary_display_is_video_mode())
		_vdo_mode_enter_idle();
	else
		_cmd_mode_enter_idle();
}

void primary_display_idlemgr_leave_idle_nolock(void)
{
	if (primary_display_is_video_mode())
		_vdo_mode_leave_idle();
	else
		_cmd_mode_leave_idle();
}

static int _primary_path_idlemgr_monitor_thread(void *data)
{
	msleep(16000);
	while (1) {
		msleep(200);

		primary_display_manual_lock();

		if (primary_get_state() == DISP_SLEPT) {
			primary_display_manual_unlock();
			primary_display_wait_resume(MAX_SCHEDULE_TIMEOUT);
			continue;
		}

		if (primary_display_is_idle()) {
			primary_display_manual_unlock();
			continue;
		}

		if (((local_clock() - idlemgr_pgc->idlemgr_last_kick_time) / 1000) < 500 * 1000) {
			/* kicked in 500ms, it's not idle */
			primary_display_manual_unlock();
			continue;
		}
		/* enter idle state */
		primary_display_idlemgr_enter_idle_nolock();
		primary_display_set_idle_stat(1);

		MMProfileLogEx(ddp_mmp_get_events()->idlemgr, MMProfileFlagStart, 0, 0);

		DISPMSG("[disp_lowpower]primary enter idle state\n");

		primary_display_manual_unlock();

		wait_event_interruptible(idlemgr_pgc->idlemgr_wait_queue, !primary_display_is_idle());

		if (kthread_should_stop())
			break;
	}

	return 0;
}

/* API */
/*********************************************************************************************************************/
golden_setting_context *get_golden_setting_pgc(void)
{
	return golden_setting_pgc;
}

/* for met - begin */
unsigned int is_enterulps(void)
{
	return idlemgr_pgc->enterulps;
}

unsigned int get_clk(void)
{
	if (disp_helper_get_option(DISP_OPT_MET_LOG)) {
		if (is_enterulps())
			return 0;
		else
			return dsi_phy_get_clk(DISP_MODULE_NUM);
	} else {
		return 0;
	}
}

/* for met - end */
void primary_display_sodi_rule_init(void)
{
	/* enable sodi when display driver is ready */
#ifndef CONFIG_MTK_FPGA
#ifndef NO_SPM
	if (primary_display_is_video_mode())
		;/* spm_enable_sodi(1); */
	else
		spm_enable_sodi(1);
#endif
#endif
}

int primary_display_lowpower_init(void)
{
	/* init idlemgr */
	if (disp_helper_get_option(DISP_OPT_IDLE_MGR))
		primary_display_idlemgr_init();


	if (disp_helper_get_option(DISP_OPT_SODI_SUPPORT))
		primary_display_sodi_rule_init();

	/* wait for mmsys mgr ready */
	#if 0
	if (disp_helper_get_option(DISP_OPT_DYNAMIC_SWITCH_MMSYSCLK))
		/* callback with lock or without lock ??? */
		register_mmclk_switch_cb(_switch_mmsys_clk);
	#endif

	/* cmd mode always enable share sram */
	if (disp_helper_get_option(DISP_OPT_SHARE_SRAM) && !primary_display_is_video_mode())
		enter_share_sram(CMDQ_SYNC_RESOURCE_WROT0);

	return 0;
}

int primary_display_is_idle(void)
{
	return idlemgr_pgc->is_primary_idle;
}

void primary_display_idlemgr_kick(const char *source, int need_lock)
{
	MMProfileLogEx(ddp_mmp_get_events()->idlemgr, MMProfileFlagPulse, 1, 0);

	/* get primary lock to protect idlemgr_last_kick_time and primary_display_is_idle() */
	if (need_lock)
		primary_display_manual_lock();

	/* update kick timestamp */
	idlemgr_pgc->idlemgr_last_kick_time = sched_clock();

	if (primary_display_is_idle()) {
		primary_display_idlemgr_leave_idle_nolock();
		primary_display_set_idle_stat(0);

		MMProfileLogEx(ddp_mmp_get_events()->idlemgr, MMProfileFlagEnd, 0, 0);
		/* wake up idlemgr process to monitor next idle stat */
		wake_up_interruptible(&(idlemgr_pgc->idlemgr_wait_queue));
	}

	if (need_lock)
		primary_display_manual_unlock();
}

void enable_sodi_by_cmdq(cmdqRecHandle handler)
{
	/* Enable SPM CG Mode(Force 30+ times to ensure write success, need find root cause and fix later) */
	cmdqRecWrite(handler, 0x100062B0, 0x2, ~0);
	/* Polling EMI Status to ensure EMI is enabled */
	cmdqRecPoll(handler, 0x10006134, 0, 0x200000);
}

void disable_sodi_by_cmdq(cmdqRecHandle handler)
{
	cmdqRecWrite(handler, 0x100062B0, 0x0, 0x2);
}


void enter_share_sram(CMDQ_EVENT_ENUM resourceEvent)
{
	/* 1. try to allocate sram at the fisrt time */
	_acquire_wrot_resource_nolock(CMDQ_SYNC_RESOURCE_WROT0);

	/* 2. register call back */
	cmdqCoreSetResourceCallback(CMDQ_SYNC_RESOURCE_WROT0,
		_acquire_wrot_resource, _release_wrot_resource);
}

void leave_share_sram(CMDQ_EVENT_ENUM resourceEvent)
{
	/* 1. unregister call back */
	cmdqCoreSetResourceCallback(CMDQ_SYNC_RESOURCE_WROT0, NULL, NULL);

	/* 2. try to release share sram */
	_release_wrot_resource_nolock(CMDQ_SYNC_RESOURCE_WROT0);
}

void set_hrtnum(unsigned int new_hrtnum)
{
	if (golden_setting_pgc->hrt_num != new_hrtnum) {
		if ((golden_setting_pgc->hrt_num > golden_setting_pgc->hrt_magicnum
			&& new_hrtnum <= golden_setting_pgc->hrt_magicnum)
			|| (golden_setting_pgc->hrt_num <= golden_setting_pgc->hrt_magicnum
			&& new_hrtnum > golden_setting_pgc->hrt_magicnum)) {
			/* should not on screenidle when set hrtnum */
			if (new_hrtnum > golden_setting_pgc->hrt_magicnum)
				golden_setting_pgc->fifo_mode = 2;
			else
				golden_setting_pgc->fifo_mode = 1;

		}
		golden_setting_pgc->hrt_num = new_hrtnum;
	}
}

/* set enterulps flag after power on & power off */
void set_enterulps(unsigned flag)
{
	idlemgr_pgc->enterulps = flag;
}

void set_is_dc(unsigned int is_dc)
{
	if (golden_setting_pgc->is_dc != is_dc)
		golden_setting_pgc->is_dc = is_dc;
}
#endif
