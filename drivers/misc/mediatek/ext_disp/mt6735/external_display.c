#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/module.h>
#include <linux/wait.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/vmalloc.h>

#include "mtkfb_info.h"
#include "mtkfb.h"

#include "ddp_hal.h"
#include "ddp_dump.h"
#include "ddp_path.h"
#include "ddp_drv.h"
#include "ddp_info.h"

#include <m4u.h>
#include "cmdq_def.h"
#include "cmdq_record.h"
#include "cmdq_reg.h"
#include "cmdq_core.h"

#include "ddp_manager.h"
#include "ddp_mmp.h"
#include "ddp_ovl.h"
#include "lcm_drv.h"

#include "extd_platform.h"
#include "extd_log.h"
#include "extd_utils.h"
#include "extd_hdmi_types.h"
#include "external_display.h"

#include "primary_display.h"
#include "disp_session.h"
#include "display_recorder.h"
#include "extd_info.h"

int ext_disp_use_cmdq;
int ext_disp_use_m4u;
enum EXT_DISP_PATH_MODE ext_disp_mode;

static int is_context_inited;
static int init_roi;

struct ext_disp_path_context {
	enum EXTD_POWER_STATE state;
	enum EXTD_OVL_REQ_STATUS ovl_req_state;
	int init;
	unsigned int session;
	int need_trigger_overlay;
	int suspend_config;
	enum EXT_DISP_PATH_MODE mode;
	unsigned int last_vsync_tick;
	struct mutex lock;
	struct mutex vsync_lock;
	cmdqRecHandle cmdq_handle_config;
	cmdqRecHandle cmdq_handle_trigger;
	disp_path_handle dpmgr_handle;
	disp_path_handle ovl2mem_path_handle;
};

#define pgc	_get_context()

LCM_PARAMS extd_lcm_params;

static struct ext_disp_path_context *_get_context(void)
{
	static struct ext_disp_path_context g_context;

	if (!is_context_inited) {
		memset((void *)&g_context, 0, sizeof(struct ext_disp_path_context));
		is_context_inited = 1;
		EXT_DISP_LOG("_get_context set is_context_inited\n");
	}

	return &g_context;
}

enum EXT_DISP_PATH_MODE ext_disp_path_get_mode(unsigned int session)
{
	return ext_disp_mode;
}

void ext_disp_path_set_mode(enum EXT_DISP_PATH_MODE mode, unsigned int session)
{
	ext_disp_mode = mode;
	init_roi = (mode == EXTD_DIRECT_LINK_MODE ? 1 : 0);
}

static void _ext_disp_path_lock(void)
{
	extd_sw_mutex_lock(NULL);	/* /(&(pgc->lock)); */
}

static void _ext_disp_path_unlock(void)
{
	extd_sw_mutex_unlock(NULL);	/* (&(pgc->lock)); */
}

static void _ext_disp_vsync_lock(unsigned int session)
{
	mutex_lock(&(pgc->vsync_lock));
}

static void _ext_disp_vsync_unlock(unsigned int session)
{
	mutex_unlock(&(pgc->vsync_lock));
}

static DISP_MODULE_ENUM _get_dst_module_by_lcm(disp_path_handle pHandle)
{
	return DISP_MODULE_DPI;
}

/***************************************************************
***trigger operation:    VDO+CMDQ  CMD+CMDQ VDO+CPU  CMD+CPU
*** 1.wait idle:         N         N        Y        Y
*** 2.lcm update:        N         Y        N        Y
*** 3.path start:       idle->Y    Y       idle->Y   Y
*** 4.path trigger:     idle->Y    Y       idle->Y   Y
*** 5.mutex enable:      N         N       idle->Y   Y
*** 6.set cmdq dirty:    N         Y        N        N
*** 7.flush cmdq:        Y         Y        N        N
*** 8.reset cmdq:        Y         Y        N        N
*** 9.cmdq insert token: Y         Y        N        N
****************************************************************/

/*
 * trigger operation:  VDO+CMDQ  CMD+CMDQ VDO+CPU  CMD+CPU
 *	1.wait idle:	     N         N        Y        Y
*/
static int _should_wait_path_idle(void)
{
	if (ext_disp_cmdq_enabled())
		return 0;
	else
		return dpmgr_path_is_busy(pgc->dpmgr_handle);
}

/* trigger operation:  VDO+CMDQ  CMD+CMDQ VDO+CPU  CMD+CPU
  * 2.lcm update:      N         Y        N        Y
*/
/*
static int _should_update_lcm(void)
{
	if (ext_disp_cmdq_enabled() || ext_disp_is_video_mode())
		return 0;
	else
		return 1;
}
*/

/* trigger operation:  VDO+CMDQ  CMD+CMDQ VDO+CPU  CMD+CPU
  * 3.path start:      idle->Y   Y        idle->Y  Y
*/
static int _should_start_path(void)
{
	if (ext_disp_is_video_mode())
		return dpmgr_path_is_idle(pgc->dpmgr_handle);
	else
		return 1;
}

/* trigger operation:  VDO+CMDQ  CMD+CMDQ VDO+CPU  CMD+CPU
 * 4. path trigger:    idle->Y   Y        idle->Y  Y
 * 5. mutex enable:    N         N        idle->Y  Y
*/
static int _should_trigger_path(void)
{
	/* this is not a perfect design, we can't decide path trigger(ovl/rdma/dsi..) separately with mutex enable */
	/* but it's lucky because path trigger and mutex enable is the same w/o cmdq, and it's correct w/ CMDQ(Y+N). */
	if (ext_disp_is_video_mode())
		return dpmgr_path_is_idle(pgc->dpmgr_handle);
	else if (ext_disp_cmdq_enabled())
		return 0;
	else
		return 1;
}

/* trigger operation:  VDO+CMDQ  CMD+CMDQ VDO+CPU  CMD+CPU
  * 6. set cmdq dirty: N         Y        N        N
*/
static int _should_set_cmdq_dirty(void)
{
	if (ext_disp_cmdq_enabled() && (ext_disp_is_video_mode() == 0))
		return 1;
	else
		return 0;
}

/* trigger operation:  VDO+CMDQ  CMD+CMDQ VDO+CPU  CMD+CPU
  * 7. flush cmdq:     Y         Y        N        N
*/
static int _should_flush_cmdq_config_handle(void)
{
	return ext_disp_cmdq_enabled() ? 1 : 0;
}

/* trigger operation:  VDO+CMDQ  CMD+CMDQ VDO+CPU  CMD+CPU
  * 8. reset cmdq:     Y         Y        N        N
*/
static int _should_reset_cmdq_config_handle(void)
{
	return ext_disp_cmdq_enabled() ? 1 : 0;
}

/* trigger operation:       VDO+CMDQ  CMD+CMDQ VDO+CPU  CMD+CPU
  * 9. cmdq insert token:   Y         Y        N        N
*/
static int _should_insert_wait_frame_done_token(void)
{
	return ext_disp_cmdq_enabled() ? 1 : 0;
}

static int _should_trigger_interface(void)
{
	if (pgc->mode == EXTD_DECOUPLE_MODE)
		return 0;
	else
		return 1;
}

static int _should_config_ovl_input(void)
{
	/* should extend this when display path dynamic switch is ready */
	/* if(pgc->mode == EXTD_SINGLE_LAYER_MODE ||pgc->mode == EXTD_RDMA_DPI_MODE) */
	/* return 0; */
	/* else */
	/* return 1; */

	if (ext_disp_mode == EXTD_SINGLE_LAYER_MODE || ext_disp_mode == EXTD_RDMA_DPI_MODE)
		return 0;
	else
		return 1;
}

#define OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO
static long int get_current_time_us(void)
{
	struct timeval t;

	do_gettimeofday(&t);
	return (t.tv_sec & 0xFFF) * 1000000 + t.tv_usec;
}

/*
static enum hrtimer_restart _DISP_CmdModeTimer_handler(struct hrtimer *timer)
{
	EXT_DISP_LOG("fake timer, wake up\n");
	dpmgr_signal_event(pgc->dpmgr_handle, DISP_PATH_EVENT_IF_VSYNC);
#if 0
	if ((get_current_time_us() - pgc->last_vsync_tick) > 16666) {
		dpmgr_signal_event(pgc->dpmgr_handle, DISP_PATH_EVENT_IF_VSYNC);
		pgc->last_vsync_tick = get_current_time_us();
	}
#endif
	return HRTIMER_RESTART;
}

static int _init_vsync_fake_monitor(int fps)
{
	static struct hrtimer cmd_mode_update_timer;
	static ktime_t cmd_mode_update_timer_period;

	if (fps == 0)
		fps = 60;

	cmd_mode_update_timer_period = ktime_set(0, 1000 / fps * 1000);
	EXT_DISP_LOG("[MTKFB] vsync timer_period=%d\n", 1000 / fps);
	hrtimer_init(&cmd_mode_update_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	cmd_mode_update_timer.function = _DISP_CmdModeTimer_handler;

	return 0;
}
*/

static int _build_path_direct_link(void)
{
	int ret = 0;

	DISP_MODULE_ENUM dst_module = 0;

	EXT_DISP_FUNC();
	pgc->mode = EXTD_DIRECT_LINK_MODE;

	pgc->dpmgr_handle = dpmgr_create_path(DDP_SCENARIO_SUB_DISP, pgc->cmdq_handle_config);
	if (pgc->dpmgr_handle)
		EXT_DISP_LOG("dpmgr create path SUCCESS(%p)\n", pgc->dpmgr_handle);
	else {
		EXT_DISP_LOG("dpmgr create path FAIL\n");
		return -1;
	}

	dst_module = DISP_MODULE_DPI;
	dpmgr_path_set_dst_module(pgc->dpmgr_handle, dst_module);
	/* EXT_DISP_LOG("dpmgr set dst module FINISHED(%s)\n", ddp_get_module_name(dst_module)); */
	/*
	{
		M4U_PORT_STRUCT sPort;
		sPort.ePortID = M4U_PORT_DISP_OVL1;
		sPort.Virtuality = ext_disp_use_m4u;
		sPort.Security = 0;
		sPort.Distance = 1;
		sPort.Direction = 0;
		ret = m4u_config_port(&sPort);
		if (ret == 0) {
			EXT_DISP_LOG("config M4U Port %s to %s SUCCESS\n",
				ddp_get_module_name(M4U_PORT_DISP_OVL1), ext_disp_use_m4u ? "virtual" : "physical");
		} else {
			EXT_DISP_LOG("config M4U Port %s to %s FAIL(ret=%d)\n", ddp_get_module_name(M4U_PORT_DISP_OVL1),
				ext_disp_use_m4u ? "virtual" : "physical", ret);
			return -1;
		}
	}
	*/

	dpmgr_set_lcm_utils(pgc->dpmgr_handle, NULL);
	dpmgr_enable_event(pgc->dpmgr_handle, DISP_PATH_EVENT_IF_VSYNC);
	dpmgr_enable_event(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_DONE);
	dpmgr_enable_event(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_START);

	return ret;
}


static int _build_path_decouple(void)
{
	return 0;
}

static int _build_path_single_layer(void)
{
	return 0;
}

static int _build_path_rdma_dpi(void)
{
	int ret = 0;

	DISP_MODULE_ENUM dst_module = 0;

	pgc->mode = EXTD_RDMA_DPI_MODE;

	pgc->dpmgr_handle = dpmgr_create_path(DDP_SCENARIO_SUB_RDMA1_DISP, pgc->cmdq_handle_config);
	if (pgc->dpmgr_handle)
		EXT_DISP_LOG("dpmgr create path SUCCESS(%p)\n", pgc->dpmgr_handle);
	else {
		EXT_DISP_LOG("dpmgr create path FAIL\n");
		return -1;
	}

	dst_module = _get_dst_module_by_lcm(pgc->dpmgr_handle);
	dpmgr_path_set_dst_module(pgc->dpmgr_handle, dst_module);
	EXT_DISP_LOG("dpmgr set dst module FINISHED(%s)\n", ddp_get_module_name(dst_module));

	{
#ifdef MTK_FB_RDMA1_SUPPORT
		M4U_PORT_STRUCT sPort;

		sPort.ePortID = M4U_PORT_DISP_RDMA1;
		sPort.Virtuality = ext_disp_use_m4u;
		sPort.Security = 0;
		sPort.Distance = 1;
		sPort.Direction = 0;
		ret = m4u_config_port(&sPort);
		if (ret == 0) {
			EXT_DISP_LOG("config M4U Port %s to %s SUCCESS\n", ddp_get_module_name(DISP_MODULE_RDMA1),
				ext_disp_use_m4u ? "virtual" : "physical");
		} else {
			EXT_DISP_LOG("config M4U Port %s to %s FAIL(ret=%d)\n", ddp_get_module_name(DISP_MODULE_RDMA1),
				ext_disp_use_m4u ? "virtual" : "physical", ret);
			return -1;
		}
#endif
	}

	dpmgr_set_lcm_utils(pgc->dpmgr_handle, NULL);
	dpmgr_enable_event(pgc->dpmgr_handle, DISP_PATH_EVENT_IF_VSYNC);
	dpmgr_enable_event(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_DONE);

	return ret;
}

static void _cmdq_build_trigger_loop(void)
{
	int ret = 0;

	cmdqRecCreate(CMDQ_SCENARIO_TRIGGER_LOOP, &(pgc->cmdq_handle_trigger));
	EXT_DISP_LOG("ext_disp path trigger thread cmd handle=%p\n", pgc->cmdq_handle_trigger);
	cmdqRecReset(pgc->cmdq_handle_trigger);

	if (ext_disp_is_video_mode()) {
		/* wait and clear stream_done, HW will assert mutex enable automatically in frame done reset. */
		/* todo: should let dpmanager to decide wait which mutex's eof. */
		ret = cmdqRecWait(pgc->cmdq_handle_trigger,
				dpmgr_path_get_mutex(pgc->dpmgr_handle) + CMDQ_EVENT_MUTEX0_STREAM_EOF);

		/* for some module(like COLOR) to read hw register to GPR after frame done */
		dpmgr_path_build_cmdq(pgc->dpmgr_handle, pgc->cmdq_handle_trigger, CMDQ_AFTER_STREAM_EOF);
	} else {
		/* DSI command mode doesn't have mutex_stream_eof, need use CMDQ token instead */
		ret = cmdqRecWait(pgc->cmdq_handle_trigger, CMDQ_SYNC_TOKEN_CONFIG_DIRTY);

		/* ret = cmdqRecWait(pgc->cmdq_handle_trigger, CMDQ_EVENT_MDP_DSI0_TE_SOF); */
		/* for operations before frame transfer, such as waiting for DSI TE */
		dpmgr_path_build_cmdq(pgc->dpmgr_handle, pgc->cmdq_handle_trigger, CMDQ_BEFORE_STREAM_SOF);

		/* cleat frame done token, now the config thread will not allowed to config registers. */
		/* remember that config thread's priority is higher than trigger thread*/
		/* so all the config queued before will be applied then STREAM_EOF token be cleared */
		/* this is what CMDQ did as "Merge" */
		ret = cmdqRecClearEventToken(pgc->cmdq_handle_trigger, CMDQ_SYNC_TOKEN_STREAM_EOF);

		/* enable mutex, only cmd mode need this */
		/* this is what CMDQ did as "Trigger" */
		dpmgr_path_trigger(pgc->dpmgr_handle, pgc->cmdq_handle_trigger, CMDQ_ENABLE);

		/* waiting for frame done, because we can't use mutex stream eof here*/
		/* so need to let dpmanager help to decide which event to wait */
		/* most time we wait rdmax frame done event. */
		ret = cmdqRecWait(pgc->cmdq_handle_trigger, CMDQ_EVENT_DISP_RDMA1_EOF);
		dpmgr_path_build_cmdq(pgc->dpmgr_handle, pgc->cmdq_handle_trigger, CMDQ_WAIT_STREAM_EOF_EVENT);

		/* dsi is not idle rightly after rdma frame done, */
		/* so we need to polling about 1us for dsi returns to idle */
		/* do not polling dsi idle directly which will decrease CMDQ performance */
		dpmgr_path_build_cmdq(pgc->dpmgr_handle, pgc->cmdq_handle_trigger, CMDQ_CHECK_IDLE_AFTER_STREAM_EOF);

		/* for some module(like COLOR) to read hw register to GPR after frame done */
		dpmgr_path_build_cmdq(pgc->dpmgr_handle, pgc->cmdq_handle_trigger, CMDQ_AFTER_STREAM_EOF);

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
	cmdqRecDumpCommand(pgc->cmdq_handle_trigger);
	EXT_DISP_LOG("ext display BUILD cmdq trigger loop finished\n");
}

static void _cmdq_start_trigger_loop(void)
{
	int ret = 0;

	/* this should be called only once because trigger loop will nevet stop */
	ret = cmdqRecStartLoop(pgc->cmdq_handle_trigger);
	if (!ext_disp_is_video_mode()) {
		/* need to set STREAM_EOF for the first time, otherwise we will stuck in dead loop */
		cmdqCoreSetEvent(CMDQ_SYNC_TOKEN_STREAM_EOF);
		/* /dprec_event_op(DPREC_EVENT_CMDQ_SET_EVENT_ALLOW); */
	}

	EXT_DISP_LOG("START cmdq trigger loop finished\n");
}

static void _cmdq_stop_trigger_loop(void)
{
	int ret = 0;

	/* this should be called only once because trigger loop will nevet stop */
	ret = cmdqRecStopLoop(pgc->cmdq_handle_trigger);

	EXT_DISP_LOG("ext display STOP cmdq trigger loop finished\n");
}


static void _cmdq_set_config_handle_dirty(void)
{
	if (!ext_disp_is_video_mode()) {
		/* only command mode need to set dirty */
		cmdqRecSetEventToken(pgc->cmdq_handle_config, CMDQ_SYNC_TOKEN_CONFIG_DIRTY);
		/* /dprec_event_op(DPREC_EVENT_CMDQ_SET_DIRTY); */
	}
}

static void _cmdq_reset_config_handle(void)
{
	cmdqRecReset(pgc->cmdq_handle_config);
	/* /dprec_event_op(DPREC_EVENT_CMDQ_RESET); */
}

static void _cmdq_flush_config_handle(int blocking, void *callback, unsigned int userdata)
{
	if (blocking) {
		/* it will be blocked until mutex done */
		cmdqRecFlush(pgc->cmdq_handle_config);
	} else {
		if (callback)
			cmdqRecFlushAsyncCallback(pgc->cmdq_handle_config, callback, userdata);
		else
			cmdqRecFlushAsync(pgc->cmdq_handle_config);
	}
	/* dprec_event_op(DPREC_EVENT_CMDQ_FLUSH); */
}

static void _cmdq_insert_wait_frame_done_token(int clear_event)
{
	if (ext_disp_is_video_mode()) {
		if (clear_event == 0) {
			cmdqRecWaitNoClear(pgc->cmdq_handle_config,
					dpmgr_path_get_mutex(pgc->dpmgr_handle) + CMDQ_EVENT_MUTEX0_STREAM_EOF);
		} else {
			cmdqRecWait(pgc->cmdq_handle_config,
					dpmgr_path_get_mutex(pgc->dpmgr_handle) + CMDQ_EVENT_MUTEX0_STREAM_EOF);
		}
	} else {
		if (clear_event == 0)
			cmdqRecWaitNoClear(pgc->cmdq_handle_config, CMDQ_SYNC_TOKEN_STREAM_EOF);
		else
			cmdqRecWait(pgc->cmdq_handle_config, CMDQ_SYNC_TOKEN_STREAM_EOF);
	}

	/* /dprec_event_op(DPREC_EVENT_CMDQ_WAIT_STREAM_EOF); */
}

static int _convert_disp_input_to_rdma(RDMA_CONFIG_STRUCT *dst, disp_input_config *src)
{
	int ret = 0;
	unsigned int Bpp = 0;
	unsigned int bpp = 0;

	if (!src || !dst) {
		EXT_DISP_ERR("%s src(0x%p) or dst(0x%p) is null\n",
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

static int _convert_disp_input_to_ovl(OVL_CONFIG_STRUCT *dst, disp_input_config *src)
{
	int ret;
	unsigned int Bpp = 0;
	unsigned int bpp = 0;

	if (!src || !dst) {
		EXT_DISP_ERR("%s src(0x%p) or dst(0x%p) is null\n",
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

	ret = disp_fmt_to_hw_fmt(src->src_fmt, (unsigned int *)(&(dst->fmt)),
				(unsigned int *)(&Bpp), (unsigned int *)(&bpp));

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
	if (src->buffer_source != DISP_BUFFER_ALPHA) {
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
	if (DISP_FORMAT_ARGB8888 == src->src_fmt ||
	    DISP_FORMAT_ABGR8888 == src->src_fmt ||
	    DISP_FORMAT_RGBA8888 == src->src_fmt ||
	    DISP_FORMAT_BGRA8888 == src->src_fmt || src->buffer_source == DISP_BUFFER_ALPHA) {
		/* nothing */
	} else {
		dst->aen = FALSE;
		dst->sur_aen = FALSE;
	}
#endif

	dst->identity = src->identity;
	dst->connected_type = src->connected_type;
	dst->security = src->security;
	dst->yuv_range = src->yuv_range;

	if (src->buffer_source == DISP_BUFFER_ALPHA)
		dst->source = OVL_LAYER_SOURCE_RESERVED;	/* dim layer, constant alpha */
	else if (src->buffer_source == DISP_BUFFER_ION || src->buffer_source == DISP_BUFFER_MVA)
		dst->source = OVL_LAYER_SOURCE_MEM;	/* from memory */
	else {
		EXT_DISP_ERR("unknown source = %d", src->buffer_source);
		dst->source = OVL_LAYER_SOURCE_MEM;
	}

	return ret;
}

static int _ext_disp_trigger(int blocking, void *callback, unsigned int userdata)
{
	bool reg_flush = false;
	/*disp_session_vsync_config vsync_config;*/

	EXT_DISP_FUNC();

	if (_should_wait_path_idle())
		dpmgr_wait_event_timeout(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_DONE, HZ / 2);

	if (_should_start_path()) {
		reg_flush = true;
		dpmgr_path_start(pgc->dpmgr_handle, ext_disp_cmdq_enabled());
		MMProfileLogEx(ddp_mmp_get_events()->Extd_State, MMProfileFlagPulse, Trigger, 1);
	}

	if (_should_trigger_path()) {
		/* trigger_loop_handle is used only for build trigger loop*/
		dpmgr_path_trigger(pgc->dpmgr_handle, NULL, ext_disp_cmdq_enabled());
	}

	if (_should_set_cmdq_dirty())
		_cmdq_set_config_handle_dirty();

	if (_should_flush_cmdq_config_handle()) {
		if (reg_flush)
			MMProfileLogEx(ddp_mmp_get_events()->Extd_State, MMProfileFlagPulse, Trigger, 2);
/*
		if (_should_start_path()) {
			EXT_DISP_LOG("Wait Main Display Vsync\n");
			primary_display_wait_for_vsync(&vsync_config);
		}
*/

		_cmdq_flush_config_handle(blocking, callback, userdata);
	}

	if (_should_reset_cmdq_config_handle())
		_cmdq_reset_config_handle();

	if (_should_insert_wait_frame_done_token())
		_cmdq_insert_wait_frame_done_token(0);

	return 0;
}

static int _ext_disp_trigger_EPD(int blocking, void *callback, unsigned int userdata)
{
	/* /EXT_DISP_FUNC(); */

	if (_should_wait_path_idle())
		dpmgr_wait_event_timeout(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_DONE, HZ / 2);

	if (_should_start_path()) {
		dpmgr_path_start(pgc->dpmgr_handle, ext_disp_cmdq_enabled());
		MMProfileLogEx(ddp_mmp_get_events()->Extd_State, MMProfileFlagPulse, Trigger, 1);
	}

	if (_should_trigger_path()) {
		/* trigger_loop_handle is used only for build trigger loop*/
		dpmgr_path_trigger(pgc->dpmgr_handle, NULL, ext_disp_cmdq_enabled());
	}

	if (_should_set_cmdq_dirty())
		_cmdq_set_config_handle_dirty();

	if (_should_flush_cmdq_config_handle())
		_cmdq_flush_config_handle(blocking, callback, userdata);

	if (_should_reset_cmdq_config_handle())
		_cmdq_reset_config_handle();

	if (_should_insert_wait_frame_done_token()) {
		/* 1 for clear event, cmdqRecWait */
		_cmdq_insert_wait_frame_done_token(1);
	}

	return 0;
}

/* <-- Donglei for K2 compile
//need remove
static unsigned int cmdqDdpClockOn(uint64_t engineFlag)
{
	return 0;
}

static unsigned int cmdqDdpClockOff(uint64_t engineFlag)
{
	return 0;
}

static unsigned int cmdqDdpDumpInfo(uint64_t engineFlag,
			char     *pOutBuf,
			unsigned int bufSize)
{
	ext_disp_diagnose();
	return 0;
}

static unsigned int cmdqDdpResetEng(uint64_t engineFlag)
{
	return 0;
}
*/

void ext_disp_probe(void)
{
	EXT_DISP_FUNC();

	ext_disp_use_cmdq = CMDQ_ENABLE;
	ext_disp_use_m4u = 1;
	ext_disp_mode = EXTD_DIRECT_LINK_MODE;
}

int ext_disp_init(char *lcm_name, unsigned int session)
{
	disp_ddp_path_config *data_config;
	enum EXT_DISP_STATUS ret;

	EXT_DISP_FUNC();
	ret = EXT_DISP_STATUS_OK;

	dpmgr_init();

	extd_mutex_init(&(pgc->lock));
	_ext_disp_path_lock();

#if 0
	ret = cmdqCoreRegisterCB(CMDQ_GROUP_DISP, cmdqDdpClockOn, cmdqDdpDumpInfo, cmdqDdpResetEng, cmdqDdpClockOff);
	if (ret) {
		EXT_DISP_ERR("cmdqCoreRegisterCB failed, ret=%d\n", ret);
		ret = EXT_DISP_STATUS_ERROR;
		goto done;
	}
#endif
	ret = cmdqRecCreate(CMDQ_SCENARIO_MHL_DISP, &(pgc->cmdq_handle_config));
	if (ret) {
		EXT_DISP_LOG("cmdqRecCreate FAIL, ret=%d\n", ret);
		ret = EXT_DISP_STATUS_ERROR;
		goto done;
	} else
		EXT_DISP_LOG("cmdqRecCreate SUCCESS, g_cmdq_handle=%p\n", pgc->cmdq_handle_config);

	if (ext_disp_mode == EXTD_DIRECT_LINK_MODE)
		_build_path_direct_link();
	else if (ext_disp_mode == EXTD_DECOUPLE_MODE)
		_build_path_decouple();
	else if (ext_disp_mode == EXTD_SINGLE_LAYER_MODE)
		_build_path_single_layer();
	else if (ext_disp_mode == EXTD_RDMA_DPI_MODE)
		_build_path_rdma_dpi();
	else
		EXT_DISP_LOG("ext_disp display mode is WRONG\n");

	if (ext_disp_use_cmdq == CMDQ_ENABLE) {
		if (DISP_SESSION_DEV(session) != DEV_EINK + 1) {
			_cmdq_build_trigger_loop();
			EXT_DISP_LOG("ext_disp display BUILD cmdq trigger loop finished\n");

			_cmdq_start_trigger_loop();
		}
	}
	pgc->session = session;

	EXT_DISP_LOG("ext_disp display START cmdq trigger loop finished\n");

	dpmgr_path_set_video_mode(pgc->dpmgr_handle, ext_disp_is_video_mode());
	dpmgr_path_init(pgc->dpmgr_handle, CMDQ_DISABLE);

	data_config = vmalloc(sizeof(disp_ddp_path_config));
	if (data_config) {
		memset((void *)data_config, 0, sizeof(disp_ddp_path_config));
		memcpy(&(data_config->dispif_config), &extd_lcm_params, sizeof(LCM_PARAMS));

		data_config->dst_w = extd_lcm_params.dpi.width;
		data_config->dst_h = extd_lcm_params.dpi.height;
		data_config->dst_dirty = 1;
		init_roi = 0;
		ret = dpmgr_path_config(pgc->dpmgr_handle, data_config, CMDQ_DISABLE);
		EXT_DISP_LOG("ext_disp_init roi w:%d, h:%d\n", data_config->dst_w, data_config->dst_h);
	} else
		EXT_DISP_LOG("allocate buffer failed!!!\n");

	/* this will be set to always enable cmdq later */
/*
	if (ext_disp_is_video_mode())
		dpmgr_map_event_to_irq(pgc->dpmgr_handle, DISP_PATH_EVENT_IF_VSYNC, DDP_IRQ_DPI_VSYNC);
*/

	if (ext_disp_use_cmdq == CMDQ_ENABLE)
		_cmdq_reset_config_handle();

	mutex_init(&(pgc->vsync_lock));
	pgc->state = EXTD_INIT;

 done:

	_ext_disp_path_unlock();
	dpmgr_path_stop(pgc->dpmgr_handle, CMDQ_DISABLE);

	EXT_DISP_LOG("ext_disp_init done\n");
	return ret;
}


int ext_disp_deinit(unsigned int session)
{
	EXT_DISP_FUNC();

	_ext_disp_path_lock();

	if (pgc->state == EXTD_DEINIT)
		goto deinit_exit;

	if (pgc->state == EXTD_SUSPEND)
		dpmgr_path_power_on(pgc->dpmgr_handle, CMDQ_DISABLE);

	dpmgr_path_deinit(pgc->dpmgr_handle, CMDQ_DISABLE);
	dpmgr_destroy_path(pgc->dpmgr_handle, NULL);

	cmdqRecDestroy(pgc->cmdq_handle_config);
	cmdqRecDestroy(pgc->cmdq_handle_trigger);

	pgc->state = EXTD_DEINIT;

 deinit_exit:
	_ext_disp_path_unlock();
	is_context_inited = 0;
	EXT_DISP_LOG("ext_disp_deinit done\n");
	return 0;
}

int ext_disp_wait_for_vsync(void *config, unsigned int session)
{
	int ret = 0;
	disp_session_vsync_config *c = (disp_session_vsync_config *) config;

	EXT_DISP_FUNC();

	_ext_disp_path_lock();
	if (pgc->state == EXTD_DEINIT) {
		_ext_disp_path_unlock();
		msleep(20);
		return -1;
	}
	_ext_disp_path_unlock();

	_ext_disp_vsync_lock(session);
	ret = dpmgr_wait_event(pgc->dpmgr_handle, DISP_PATH_EVENT_IF_VSYNC);

	if (ret == -2) {
		EXT_DISP_LOG("vsync for ext display path not enabled yet\n");
		_ext_disp_vsync_unlock(session);
		return -1;
	}
	EXT_DISP_LOG("ext_disp_wait_for_vsync - vsync signaled\n");
	c->vsync_ts = get_current_time_us();
	c->vsync_cnt++;

	_ext_disp_vsync_unlock(session);
	return ret;
}

int ext_disp_suspend(unsigned int session)
{
	enum EXT_DISP_STATUS ret = EXT_DISP_STATUS_OK;

	EXT_DISP_FUNC();

	_ext_disp_path_lock();

	if (pgc->state == EXTD_DEINIT || pgc->state == EXTD_SUSPEND || session != pgc->session) {
		EXT_DISP_ERR("status is not EXTD_RESUME or session is not match\n");
		goto done;
	}

	pgc->need_trigger_overlay = 0;

	if (dpmgr_path_is_busy(pgc->dpmgr_handle))
		dpmgr_wait_event_timeout(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_DONE, HZ / 30);

	if (ext_disp_use_cmdq == CMDQ_ENABLE && DISP_SESSION_DEV(session) != DEV_EINK + 1)
		_cmdq_stop_trigger_loop();

	dpmgr_path_stop(pgc->dpmgr_handle, CMDQ_DISABLE);
	dpmgr_path_power_off(pgc->dpmgr_handle, CMDQ_DISABLE);
	if (dpmgr_path_is_busy(pgc->dpmgr_handle))
		dpmgr_wait_event_timeout(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_DONE, HZ / 30);

	dpmgr_path_reset(pgc->dpmgr_handle, CMDQ_DISABLE);

	pgc->state = EXTD_SUSPEND;

 done:
	_ext_disp_path_unlock();

	EXT_DISP_LOG("ext_disp_suspend done\n");
	return ret;
}

int ext_disp_resume(unsigned int session)
{
	enum EXT_DISP_STATUS ret = EXT_DISP_STATUS_OK;

	EXT_DISP_FUNC();

	_ext_disp_path_lock();

	if (pgc->state != EXTD_SUSPEND) {
		EXT_DISP_ERR("EXTD_DEINIT/EXTD_INIT/EXTD_RESUME\n");
		goto done;
	}

	if (_should_reset_cmdq_config_handle() && DISP_SESSION_DEV(session) != DEV_EINK + 1)
		_cmdq_reset_config_handle();

	dpmgr_path_power_on(pgc->dpmgr_handle, CMDQ_DISABLE);

	if (ext_disp_use_cmdq == CMDQ_ENABLE && DISP_SESSION_DEV(session) != DEV_EINK + 1)
		_cmdq_start_trigger_loop();

	if (dpmgr_path_is_busy(pgc->dpmgr_handle)) {
		EXT_DISP_LOG("stop display path failed, still busy\n");
		ret = -1;
		goto done;
	}

	if (DISP_SESSION_DEV(session) == DEV_EINK + 1)
		pgc->suspend_config = 0;

	pgc->state = EXTD_RESUME;

 done:
	_ext_disp_path_unlock();
	EXT_DISP_LOG("ext_disp_resume done\n");
	MMProfileLogEx(ddp_mmp_get_events()->Extd_State, MMProfileFlagPulse, Resume, 1);
	return ret;
}

int ext_disp_trigger(int blocking, void *callback, unsigned int userdata, unsigned int session)
{
	int ret = 0;

	EXT_DISP_FUNC();

	if (pgc->state == EXTD_DEINIT || pgc->state == EXTD_SUSPEND || pgc->need_trigger_overlay < 1) {
		EXT_DISP_LOG("trigger ext display is already slept\n");
		MMProfileLogEx(ddp_mmp_get_events()->Extd_ErrorInfo, MMProfileFlagPulse, Trigger, 0);
		return -1;
	}

	_ext_disp_path_lock();

	if (_should_trigger_interface()) {
		if (DISP_SESSION_TYPE(session) == DISP_SESSION_EXTERNAL && DISP_SESSION_DEV(session) == DEV_MHL + 1)
			_ext_disp_trigger(blocking, callback, userdata);
		else if (DISP_SESSION_TYPE(session) == DISP_SESSION_EXTERNAL
			&& DISP_SESSION_DEV(session) == DEV_EINK + 1)
			_ext_disp_trigger_EPD(blocking, callback, userdata);
	} else
		dpmgr_path_trigger(pgc->ovl2mem_path_handle, NULL, ext_disp_use_cmdq);

	pgc->state = EXTD_RESUME;
	_ext_disp_path_unlock();
	EXT_DISP_LOG("ext_disp_trigger done\n");

	return ret;
}

int ext_disp_suspend_trigger(void *callback, unsigned int userdata, unsigned int session)
{
	enum EXT_DISP_STATUS ret = EXT_DISP_STATUS_OK;

	EXT_DISP_FUNC();

	if (pgc->state != EXTD_RESUME) {
		EXT_DISP_LOG("trigger ext display is already slept\n");
		MMProfileLogEx(ddp_mmp_get_events()->Extd_ErrorInfo, MMProfileFlagPulse, Trigger, 0);
		return -1;
	}

	MMProfileLogEx(ddp_mmp_get_events()->Extd_State, MMProfileFlagPulse, Suspend, 0);

	_ext_disp_path_lock();

	if (_should_reset_cmdq_config_handle())
		_cmdq_reset_config_handle();

	if (_should_insert_wait_frame_done_token())
		_cmdq_insert_wait_frame_done_token(0);

	pgc->need_trigger_overlay = 0;

	if (ext_disp_use_cmdq == CMDQ_ENABLE && DISP_SESSION_DEV(session) != DEV_EINK + 1)
		_cmdq_stop_trigger_loop();

	dpmgr_path_stop(pgc->dpmgr_handle, ext_disp_cmdq_enabled());

	if (_should_flush_cmdq_config_handle()) {
		_cmdq_flush_config_handle(1, 0, 0);
		_cmdq_reset_config_handle();
	}

	dpmgr_path_reset(pgc->dpmgr_handle, CMDQ_DISABLE);
	dpmgr_path_power_off(pgc->dpmgr_handle, CMDQ_DISABLE);

	if (DISP_SESSION_DEV(session) == DEV_EINK + 1)
		pgc->suspend_config = 1;

	pgc->state = EXTD_SUSPEND;

	_ext_disp_path_unlock();

	MMProfileLogEx(ddp_mmp_get_events()->Extd_State, MMProfileFlagPulse, Suspend, 1);
	return ret;
}

int ext_disp_config_input_multiple(disp_session_input_config *input, int idx, unsigned int session)
{
	int ret = 0;
	int i = 0;
	int config_layer_id = 0;
#ifdef MTK_FB_RDMA1_SUPPORT
	M4U_PORT_STRUCT sPort;
#endif

	/* /EXT_DISP_FUNC(); */

	disp_ddp_path_config *data_config;

	if (pgc->state != EXTD_INIT && pgc->state != EXTD_RESUME && pgc->suspend_config != 1) {
		EXT_DISP_LOG("config ext disp is already slept, state:%d\n", pgc->state);
		MMProfileLogEx(ddp_mmp_get_events()->Extd_ErrorInfo, MMProfileFlagPulse, Config, idx);
		return -2;
	}

	_ext_disp_path_lock();

	/* all dirty should be cleared in dpmgr_path_get_last_config() */
	data_config = dpmgr_path_get_last_config(pgc->dpmgr_handle);

	/* hope we can use only 1 input struct for input config, just set layer number */
	if (_should_config_ovl_input()) {
		for (i = 0; i < input->config_layer_num; i++) {
			config_layer_id = input->config[i].layer_id;
			ret = _convert_disp_input_to_ovl(&(data_config->ovl_config[config_layer_id]),
				&(input->config[i]));
			dprec_mmp_dump_ovl_layer(&(data_config->ovl_config[config_layer_id]), config_layer_id, 2);

			if (init_roi == 1) {
				memcpy(&(data_config->dispif_config), &extd_lcm_params, sizeof(LCM_PARAMS));

				EXT_DISP_LOG("set dest w:%d, h:%d\n",
						extd_lcm_params.dpi.width, extd_lcm_params.dpi.height);
				data_config->dst_w = extd_lcm_params.dpi.width;
				data_config->dst_h = extd_lcm_params.dpi.height;
				data_config->dst_dirty = 1;
				data_config->rdma_config.address = 0;
			}
			data_config->ovl_dirty = 1;
			pgc->need_trigger_overlay = 1;
		}
	} else {
		OVL_CONFIG_STRUCT ovl_config;

		_convert_disp_input_to_ovl(&ovl_config, &(input->config[i]));
		dprec_mmp_dump_ovl_layer(&ovl_config, input->config[i].layer_id, 2);

		ret = _convert_disp_input_to_rdma(&(data_config->rdma_config), &(input->config[i]));
		if (data_config->rdma_config.address) {
			data_config->rdma_dirty = 1;
			pgc->need_trigger_overlay = 1;
		}

		if (pgc->ovl_req_state == EXTD_OVL_REMOVE_REQ) {
			EXT_DISP_LOG("config M4U Port DISP_MODULE_RDMA1\n");
#ifdef MTK_FB_RDMA1_SUPPORT
			sPort.ePortID = M4U_PORT_DISP_RDMA1;
			sPort.Virtuality = 1;
			sPort.Security = 0;
			sPort.Distance = 1;
			sPort.Direction = 0;
			ret = m4u_config_port(&sPort);
			if (ret != 0)
				EXT_DISP_LOG("config M4U Port DISP_MODULE_RDMA1 FAIL\n");
#endif
			pgc->ovl_req_state = EXTD_OVL_REMOVING;
		}
	}

	if (_should_wait_path_idle())
		dpmgr_wait_event_timeout(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_DONE, HZ / 2);

	memcpy(&(data_config->dispif_config), &extd_lcm_params, sizeof(LCM_PARAMS));
	ret = dpmgr_path_config(pgc->dpmgr_handle, data_config,
				ext_disp_cmdq_enabled() ? pgc->cmdq_handle_config : NULL);

	/* this is used for decouple mode, to indicate whether we need to trigger ovl */
	/* pgc->need_trigger_overlay = 1; */
	init_roi = 0;

	_ext_disp_path_unlock();
	if (data_config->ovl_dirty) {
		EXT_DISP_LOG("config_input_multiple idx:%d -w:%d, h:%d, pitch:%d\n",
				idx, data_config->ovl_config[0].src_w, data_config->ovl_config[0].src_h,
				data_config->ovl_config[0].src_pitch);
	} else {
		EXT_DISP_LOG("config_input_multiple idx:%d -w:%d, h:%d, pitch:%d, mva:0x%lx\n",
				idx, data_config->rdma_config.width, data_config->rdma_config.height,
				data_config->rdma_config.pitch, data_config->rdma_config.address);
	}

	return ret;
}

int ext_disp_is_alive(void)
{
	unsigned int temp = 0;

	EXT_DISP_FUNC();
	_ext_disp_path_lock();
	temp = pgc->state;
	_ext_disp_path_unlock();

	return temp;
}

int ext_disp_is_sleepd(void)
{
	unsigned int temp = 0;
	/* EXT_DISP_FUNC(); */
	_ext_disp_path_lock();
	temp = !pgc->state;
	_ext_disp_path_unlock();

	return temp;
}

int ext_disp_get_width(void)
{
	return 0;
}

int ext_disp_get_height(void)
{
	return 0;
}

unsigned int ext_disp_get_sess_id(void)
{
	unsigned int session_id = is_context_inited > 0 ? pgc->session : 0;
	return session_id;
}

int ext_disp_is_video_mode(void)
{
	/* TODO: we should store the video/cmd mode in runtime, because ROME will support cmd/vdo dynamic switch */
	return true;
}

int ext_disp_diagnose(void)
{
	int ret = 0;

	if (is_context_inited > 0) {
		EXT_DISP_LOG("ext_disp_diagnose, is_context_inited --%d\n", is_context_inited);
		dpmgr_check_status(pgc->dpmgr_handle);
	}

	return ret;
}

CMDQ_SWITCH ext_disp_cmdq_enabled(void)
{
	return ext_disp_use_cmdq;
}

int ext_disp_switch_cmdq(CMDQ_SWITCH use_cmdq)
{
	_ext_disp_path_lock();

	ext_disp_use_cmdq = use_cmdq;
	EXT_DISP_LOG("display driver use %s to config register now\n", (use_cmdq == CMDQ_ENABLE) ? "CMDQ" : "CPU");

	_ext_disp_path_unlock();
	return ext_disp_use_cmdq;
}

void ext_disp_get_curr_addr(unsigned long *input_curr_addr, int module)
{
	if (module == 1)
		ovl_get_address(DISP_MODULE_OVL1, input_curr_addr);
	else
		dpmgr_get_input_address(pgc->dpmgr_handle, input_curr_addr);
}

int ext_disp_factory_test(int mode, void *config)
{
	dpmgr_factory_mode_test(DISP_MODULE_DPI, NULL, config);

	return 0;
}

int ext_disp_get_handle(disp_path_handle *dp_handle, cmdqRecHandle *pHandle)
{
	*dp_handle = pgc->dpmgr_handle;
	*pHandle = pgc->cmdq_handle_config;
	return pgc->mode;
}

int ext_disp_set_ovl1_status(int status)
{
	dpmgr_set_ovl1_status(status);
	return 0;
}

int ext_disp_set_lcm_param(LCM_PARAMS *pLCMParam)
{
	if (pLCMParam)
		memcpy(&extd_lcm_params, pLCMParam, sizeof(LCM_PARAMS));

	return 0;
}

enum EXTD_OVL_REQ_STATUS ext_disp_get_ovl_req_status(unsigned int session)
{
	enum EXTD_OVL_REQ_STATUS ret = EXTD_OVL_NO_REQ;

	_ext_disp_path_lock();
	ret = pgc->ovl_req_state;
	_ext_disp_path_unlock();

	return ret;
}

int ext_disp_path_change(enum EXTD_OVL_REQ_STATUS action, unsigned int session)
{
	EXT_DISP_FUNC();
/* if(pgc->state == EXTD_DEINIT) */
/* { */
/* EXT_DISP_LOG("external display do not init!\n"); */
/* return -1; */
/* } */

	if (EXTD_OVERLAY_CNT > 0) {
		_ext_disp_path_lock();
		switch (action) {
		case EXTD_OVL_NO_REQ:
			if (pgc->ovl_req_state == EXTD_OVL_REMOVED) {
				/* 0 - DDP_OVL1_STATUS_IDLE */
				dpmgr_set_ovl1_status(0);
			}
			pgc->ovl_req_state = EXTD_OVL_NO_REQ;
			break;
		case EXTD_OVL_REQUSTING_REQ:
			if (ovl_get_status() == DDP_OVL1_STATUS_PRIMARY) {
				/* 1 - DDP_OVL1_STATUS_SUB_REQUESTING */
				dpmgr_set_ovl1_status(DDP_OVL1_STATUS_SUB_REQUESTING);
			}
			break;
		case EXTD_OVL_IDLE_REQ:
			if (ovl_get_status() == DDP_OVL1_STATUS_SUB) {
				/* 1 - DDP_OVL1_STATUS_SUB_REQUESTING */
				dpmgr_set_ovl1_status(DDP_OVL1_STATUS_IDLE);
			}
			break;
		case EXTD_OVL_SUB_REQ:
			pgc->ovl_req_state = EXTD_OVL_SUB_REQ;
			break;
		case EXTD_OVL_REMOVE_REQ:
			if (DISP_SESSION_DEV(session) != DEV_EINK + 1) {
				dpmgr_remove_ovl1_sub(pgc->dpmgr_handle, pgc->cmdq_handle_config);
				ext_disp_path_set_mode(EXTD_RDMA_DPI_MODE, session);
				pgc->ovl_req_state = EXTD_OVL_REMOVE_REQ;
			}
			break;
		case EXTD_OVL_REMOVING:
			pgc->ovl_req_state = EXTD_OVL_REMOVING;
			break;
		case EXTD_OVL_REMOVED:
			pgc->ovl_req_state = EXTD_OVL_REMOVED;
			break;
		case EXTD_OVL_INSERT_REQ:
			if (ext_disp_mode != EXTD_DIRECT_LINK_MODE) {
				dpmgr_insert_ovl1_sub(pgc->dpmgr_handle, pgc->cmdq_handle_config);
				ext_disp_path_set_mode(EXTD_DIRECT_LINK_MODE, session);
				pgc->ovl_req_state = EXTD_OVL_INSERT_REQ;
			}
			break;
		case EXTD_OVL_INSERTED:
			pgc->ovl_req_state = EXTD_OVL_INSERTED;
			break;
		default:
			break;
		}

		_ext_disp_path_unlock();
	}

	return 0;
}

int ext_disp_wait_ovl_available(int ovl_num)
{
	int ret = 0;

	if (EXTD_OVERLAY_CNT > 0) {
		/*wait OVL can be used by external display */
		ret = dpmgr_wait_ovl_available(ovl_num);
	}

	return ret;
}

bool ext_disp_path_source_is_RDMA(unsigned int session)
{
	bool is_rdma = false;

	if ((ext_disp_mode == EXTD_RDMA_DPI_MODE) && (pgc->ovl_req_state != EXTD_OVL_REMOVE_REQ)
	    && (pgc->ovl_req_state != EXTD_OVL_REMOVING)) {
		/* path source module is RDMA */
		is_rdma = true;
	}

	return is_rdma;
}

int ext_disp_is_dim_layer(unsigned long mva)
{
	int ret = 0;

	ret = is_dim_layer(mva);

	return ret;
}
