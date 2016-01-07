/*****************************************************************************/
/*****************************************************************************/
#include "extd_info.h"

#if defined(CONFIG_MTK_HDMI_SUPPORT)
#define _tx_c_
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
/*#include <linux/earlysuspend.h>*/
#include <linux/kthread.h>
/*#include <linux/rtpm_prio.h>*/
#include <linux/vmalloc.h>
/*#include <linux/disp_assert_layer.h>*/
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/switch.h>
/*#include <linux/mmprofile.h>*/

#include <linux/uaccess.h>
#include <linux/atomic.h>
#include <asm/cacheflush.h>
#include <linux/io.h>
/*#include <mach/dma.h>*/
#include <mach/irqs.h>
#include <asm/tlbflush.h>
#include <asm/page.h>

#include "m4u.h"
/*#include <mach/mt_typedefs.h>*/
#include <linux/types.h>

/*#include <mach/mt_reg_base.h>*/
#include <mach/mt_clkmgr.h>
/*#include <mach/mt_boot.h>*/
/*#include "mach/eint.h"*/
#include "mach/irqs.h"

#include "mtkfb_info.h"
#include "mtkfb.h"

#include "mtkfb_fence.h"
#include "display_recorder.h"

#include "ddp_info.h"
#include "ddp_irq.h"
#include "ddp_mmp.h"

#include "disp_session.h"

#include "extd_platform.h"
#include "extd_hdmi.h"
#include "extd_factory.h"
#include "extd_log.h"
#include "extd_utils.h"
#include "extd_hdmi_types.h"
#include "external_display.h"

#ifdef CONFIG_MTK_SMARTBOOK_SUPPORT
#include <linux/sbsuspend.h>
#include "smartbook.h"
#endif

#ifdef I2C_DBG
#include "tmbslHdmiTx_types.h"
#include "tmbslTDA9989_local.h"
#endif

/* ~~~~~~~~~~~~~~~~~~~~~~~the static variable~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*the following declare is for debug*/
static int hdmi_force_to_on;
static atomic_t hdmi_fake_in = ATOMIC_INIT(false);

static int first_frame_done;
static int wait_vsync_enable;
static bool otg_enable_status;
static bool hdmi_vsync_flag;
static wait_queue_head_t hdmi_vsync_wq;
static unsigned long hdmi_reschange = HDMI_VIDEO_RESOLUTION_NUM;

static struct switch_dev hdmi_switch_data;
static struct switch_dev hdmires_switch_data;

static struct HDMI_DRIVER *hdmi_drv;
static struct _t_hdmi_context hdmi_context;
static struct _t_hdmi_context *p = &hdmi_context;

static unsigned int hdmi_layer_num;
static unsigned long ovl_config_address[EXTD_OVERLAY_CNT];
static unsigned int hdmi_resolution_param_table[][3] = {
	{720, 480, 60},
	{1280, 720, 60},
	{1920, 1080, 30},
	{1920, 1080, 60},
};

static DEFINE_SEMAPHORE(hdmi_update_mutex);
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~the gloable variable~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#ifdef EXTD_DBG_USE_INNER_BUF
unsigned long hdmi_va, hdmi_mva_r;
#endif

struct HDMI_PARAMS _s_hdmi_params = { 0 };

struct HDMI_PARAMS *hdmi_params = &_s_hdmi_params;
disp_ddp_path_config extd_dpi_params;

struct task_struct *hdmi_fence_release_task = NULL;
wait_queue_head_t hdmi_fence_release_wq;
atomic_t hdmi_fence_release_event = ATOMIC_INIT(0);

struct task_struct *hdmi_wait_vsync_task = NULL;
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~the definition~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#define IS_HDMI_ON()            (HDMI_POWER_STATE_ON == atomic_read(&p->state))
#define IS_HDMI_OFF()           (HDMI_POWER_STATE_OFF == atomic_read(&p->state))
#define IS_HDMI_STANDBY()       (HDMI_POWER_STATE_STANDBY == atomic_read(&p->state))

#define IS_HDMI_NOT_ON()        (HDMI_POWER_STATE_ON != atomic_read(&p->state))
#define IS_HDMI_NOT_OFF()       (HDMI_POWER_STATE_OFF != atomic_read(&p->state))
#define IS_HDMI_NOT_STANDBY()   (HDMI_POWER_STATE_STANDBY != atomic_read(&p->state))

#define SET_HDMI_ON()           atomic_set(&p->state, HDMI_POWER_STATE_ON)
#define SET_HDMI_OFF()          atomic_set(&p->state, HDMI_POWER_STATE_OFF)
#define SET_HDMI_STANDBY()      atomic_set(&p->state, HDMI_POWER_STATE_STANDBY)


#define IS_HDMI_FAKE_PLUG_IN()  (true == atomic_read(&hdmi_fake_in))
#define SET_HDMI_FAKE_PLUG_IN() (atomic_set(&hdmi_fake_in, true))
#define SET_HDMI_NOT_FAKE()     (atomic_set(&hdmi_fake_in, false))

#define MHL_SESSION_ID          (0x20001)
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~extern declare~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* extern unsigned char kara_1280x720[2764800]; */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* --------------------------------------------------------------------------- */
/* Information Dump Routines */
/* --------------------------------------------------------------------------- */
static int mhl_forcr_on_cnt;
void hdmi_force_on(int from_uart_drv)
{
	if ((hdmi_force_to_on > 0) && (from_uart_drv > 0))
		return;

	HDMI_LOG("hdmi_force_on %d force on %d, cnt %d\n", from_uart_drv, hdmi_force_to_on, mhl_forcr_on_cnt);
	mhl_forcr_on_cnt++;
	hdmi_force_to_on = 1;
	if (hdmi_drv && hdmi_drv->force_on)
		hdmi_drv->force_on(from_uart_drv);
}

/* <--for debug */
void hdmi_cable_fake_plug_in(void)
{
	SET_HDMI_FAKE_PLUG_IN();
	HDMI_LOG("[HDMIFake]Cable Plug In\n");

	if (p->is_force_disable == false) {
		if (IS_HDMI_STANDBY()) {
			hdmi_resume();
			/* /msleep(1000); */
			hdmi_reschange = HDMI_VIDEO_RESOLUTION_NUM;
			switch_set_state(&hdmi_switch_data, HDMI_STATE_ACTIVE);
		}
	}
}

void hdmi_cable_fake_plug_out(void)
{
	SET_HDMI_NOT_FAKE();
	HDMI_LOG("[HDMIFake]Disable\n");

	if (p->is_force_disable == false) {
		if (IS_HDMI_ON()) {
			if (hdmi_drv->get_state() != HDMI_STATE_ACTIVE) {
				hdmi_suspend();
				switch_set_state(&hdmi_switch_data, HDMI_STATE_NO_DEVICE);
				switch_set_state(&hdmires_switch_data, 0);
			}
		}
	}
}

int hdmi_cable_fake_connect(int connect)
{
	if (connect == 0)
		hdmi_cable_fake_plug_out();
	else
		hdmi_cable_fake_plug_in();

	return 0;
}

int hdmi_allocate_hdmi_buffer(void)
{
/*
	M4U_PORT_STRUCT m4uport;
	int ret = 0;
	int hdmiPixelSize = p->hdmi_width * p->hdmi_height;
	int hdmiDataSize = hdmiPixelSize * 4;
	int hdmiBufferSize = hdmiDataSize * 1;

	HDMI_FUNC();
#ifdef EXTD_DBG_USE_INNER_BUF
	if ((hdmi_va)) {
		HDMI_LOG("return in %d\n", __LINE__);
		return 0;
	}

	hdmi_va = (unsigned long)vmalloc(hdmiBufferSize);
	if (((void *)hdmi_va) == NULL) {
		HDMI_LOG("vmalloc %d bytes fail!!!\n", hdmiBufferSize);
		return -1;
	}

	m4u_client_t *client = NULL;

	client = m4u_create_client();
	if (IS_ERR_OR_NULL(client))
		HDMI_LOG("create client fail!\n");

	ret = m4u_alloc_mva(client, M4U_PORT_DISP_OVL1, hdmi_va, 0, hdmiBufferSize,
				M4U_PROT_READ | M4U_PROT_WRITE, 0, &hdmi_mva_r);

	memcpy(hdmi_va, kara_1280x720, 2764800);
	HDMI_LOG("hdmi_va=%p, hdmi_mva_r=%p, size %d\n", hdmi_va, hdmi_mva_r, hdmiBufferSize);
#endif
*/
	return 0;
}

int hdmi_free_hdmi_buffer(void)
{
	return 0;
}

static int hdmi_wait_vsync_kthread(void *data)
{
	disp_session_vsync_config vsync_config;
/*
	struct sched_param param = {.sched_priority = RTPM_PRIO_SCRN_UPDATE };
	sched_setscheduler(current, SCHED_RR, &param);
*/

	for (;;) {
		ext_disp_wait_for_vsync((void *)&vsync_config, MHL_SESSION_ID);

		if (wait_vsync_enable == 0)
			break;

		if (kthread_should_stop())
			break;
	}

	hdmi_wait_vsync_task = NULL;
	return 0;
}

int hdmi_wait_vsync_debug(int enable)
{
	wait_vsync_enable = enable;

	if (enable != 0 && hdmi_wait_vsync_task == NULL) {
		hdmi_wait_vsync_task = kthread_create(hdmi_wait_vsync_kthread, NULL, "hdmi_wait_vsync_kthread");
		wake_up_process(hdmi_wait_vsync_task);
	}

	return 0;
}
#ifdef CONFIG_SINGLE_PANEL_OUTPUT
bool is_hdmi_plug_out_valid(void)
{
	bool ret = false;

	if (is_hdmi_plug_out_flag == 1)
		ret = true;
	return ret;
}
void clr_hdmi_plug_out_valid(void)
{
	is_hdmi_plug_out_flag = 0;
}

bool is_hdmi_video_on(void)
{
	return p->is_mhl_video_on;
}
#endif

/* -->for debug */

bool is_hdmi_active(void)
{
	bool active = IS_HDMI_ON() && p->is_clock_on;
	return active;
}

unsigned int hdmi_get_width(void)
{
	return p->hdmi_width;
}

unsigned int hdmi_get_height(void)
{
	return p->hdmi_height;
}

int hdmi_waitVsync(void)
{

	unsigned int session_id = ext_disp_get_sess_id();
	disp_session_sync_info *session_info = disp_get_session_sync_info_for_debug(session_id);

	if (session_info)
		dprec_start(&session_info->event_waitvsync, 0, 0);

	if (p->is_clock_on == false) {
		HDMI_ERR("[hdmi]:hdmi has suspend, return directly\n");
		msleep(20);
		return 0;
	}

	hdmi_vsync_flag = 0;

	if (wait_event_interruptible_timeout(hdmi_vsync_wq, hdmi_vsync_flag, HZ / 10) == 0)
		HDMI_ERR("[hdmi] Wait VSync timeout. early_suspend=%d\n", p->is_clock_on);

	if (session_info)
		dprec_done(&session_info->event_waitvsync, 1, 0);

	return 0;
}

int hdmi_get_support_info(void)
{
	int value = 0, temp = 0;

#ifdef USING_SCALE_ADJUSTMENT
	value |= HDMI_SCALE_ADJUSTMENT_SUPPORT;
#endif

#ifdef MHL_PHONE_GPIO_REUSAGE
	value |= HDMI_PHONE_GPIO_REUSAGE;
#endif

#ifdef MTK_AUDIO_MULTI_CHANNEL_SUPPORT
	temp = hdmi_drv->get_external_device_capablity();
#else
	temp = 0x2 << 3;
#endif

	value |= temp;
	value |= HDMI_FACTORY_MODE_NEW;

	HDMI_LOG("value is 0x%x\n", value);
	return value;
}


/* Configure video attribute */
int hdmi_video_config(enum HDMI_VIDEO_RESOLUTION vformat, enum HDMI_VIDEO_INPUT_FORMAT vin,
enum HDMI_VIDEO_OUTPUT_FORMAT vout)
{
	if (p->is_mhl_video_on == true || first_frame_done == 0)
		return 0;

	pr_warn("hdmi_video_config video_on=%d\n", p->is_mhl_video_on);
	/*RETIF(IS_HDMI_NOT_ON(), 0);*/
	if (IS_HDMI_NOT_ON()) {
		HDMI_LOG("return in %d\n", __LINE__);
		return 0;
	}

	p->is_mhl_video_on = true;

	if (IS_HDMI_FAKE_PLUG_IN())
		return 0;

	return hdmi_drv->video_config(vformat, vin, vout);
}

/* Configure audio attribute, will be called by audio driver */
int hdmi_audio_config(unsigned int format)
{
	enum HDMI_AUDIO_FORMAT audio_format = HDMI_AUDIO_44K_2CH;
	unsigned int channel_count = format & 0x0F;
	unsigned int sampleRate = (format & 0x70) >> 4;
	unsigned int bitWidth = (format & 0x180) >> 7, sampleBit = 0;

	HDMI_LOG("channel_count: %d, sampleRate: %d, bitWidth: %d\n", channel_count, sampleRate, bitWidth);

	if (bitWidth == HDMI_MAX_BITWIDTH_16) {
		sampleBit = 16;
		HDMI_LOG("HDMI_MAX_BITWIDTH_16\n");
	} else if (bitWidth == HDMI_MAX_BITWIDTH_24) {
		sampleBit = 24;
		HDMI_LOG("HDMI_MAX_BITWIDTH_24\n");
	}

	if (channel_count == HDMI_MAX_CHANNEL_2 && sampleRate == HDMI_MAX_SAMPLERATE_44) {
		audio_format = HDMI_AUDIO_44K_2CH;
		HDMI_LOG("AUDIO_44K_2CH\n");
	} else if (channel_count == HDMI_MAX_CHANNEL_2 && sampleRate == HDMI_MAX_SAMPLERATE_48) {
		audio_format = HDMI_AUDIO_48K_2CH;
		HDMI_LOG("AUDIO_48K_2CH\n");
	} else if (channel_count == HDMI_MAX_CHANNEL_8 && sampleRate == HDMI_MAX_SAMPLERATE_32) {
		audio_format = HDMI_AUDIO_32K_8CH;
		HDMI_LOG("AUDIO_32K_8CH\n");
	} else if (channel_count == HDMI_MAX_CHANNEL_8 && sampleRate == HDMI_MAX_SAMPLERATE_44) {
		audio_format = HDMI_AUDIO_44K_8CH;
		HDMI_LOG("AUDIO_44K_8CH\n");
	} else if (channel_count == HDMI_MAX_CHANNEL_8 && sampleRate == HDMI_MAX_SAMPLERATE_48) {
		audio_format = HDMI_AUDIO_48K_8CH;
		HDMI_LOG("AUDIO_48K_8CH\n");
	} else if (channel_count == HDMI_MAX_CHANNEL_8 && sampleRate == HDMI_MAX_SAMPLERATE_96) {
		audio_format = HDMI_AUDIO_96K_8CH;
		HDMI_LOG("AUDIO_96K_8CH\n");
	}

	else if (channel_count == HDMI_MAX_CHANNEL_8 && sampleRate == HDMI_MAX_SAMPLERATE_192) {
		audio_format = HDMI_AUDIO_192K_8CH;
		HDMI_LOG("AUDIO_192K_8CH\n");
	} else
		HDMI_LOG("audio format is not supported\n");

	/*RETIF(!p->is_enabled, 0);*/
	if (!p->is_enabled) {
		HDMI_LOG("return in %d\n", __LINE__);
		return 0;
	}

	hdmi_drv->audio_config(audio_format, sampleBit);

	return 0;
}

static void _hdmi_rdma_irq_handler(DISP_MODULE_ENUM module, unsigned int param)
{
	/*RET_VOID_IF_NOLOG(!is_hdmi_active());*/
	if (!is_hdmi_active())
		return;

	if (param & 0x2) {	/* start */
		atomic_set(&hdmi_fence_release_event, 1);
		wake_up_interruptible(&hdmi_fence_release_wq);

		if (hdmi_params->cabletype == MHL_SMB_CABLE) {	/* rdma1 register updated */
			hdmi_vsync_flag = 1;
			wake_up_interruptible(&hdmi_vsync_wq);
		}
	}

	/*frame done*/
	if (param & 0x4 && first_frame_done == 0)
		first_frame_done = 1;
}

static int hdmi_fence_release_kthread(void *data)
{

	int layid = 0;
	unsigned int session_id = 0;
	int fence_idx = 0;
	bool ovl_reg_updated = false;
	unsigned long input_curr_addr[EXTD_OVERLAY_CNT];
	struct sched_param param = {.sched_priority = 94 };/*RTPM_PRIO_SCRN_UPDATE*/

	sched_setscheduler(current, SCHED_RR, &param);


	for (;;) {
		wait_event_interruptible(hdmi_fence_release_wq, atomic_read(&hdmi_fence_release_event));
		atomic_set(&hdmi_fence_release_event, 0);

		ovl_reg_updated = false;
		session_id = ext_disp_get_sess_id();
		fence_idx = -1;

		if (session_id == 0)
			continue;

		if (ext_disp_path_source_is_RDMA(MHL_SESSION_ID)) {
			if (ext_disp_get_ovl_req_status(MHL_SESSION_ID) == EXTD_OVL_REMOVED)
				ext_disp_path_change(EXTD_OVL_NO_REQ, MHL_SESSION_ID);
			ext_disp_get_curr_addr(input_curr_addr, 0);
			fence_idx = disp_sync_find_fence_idx_by_addr(session_id, 0, input_curr_addr[0]);
			mtkfb_release_fence(session_id, 0, fence_idx);
		} else {
			if (ext_disp_get_ovl_req_status(MHL_SESSION_ID) == EXTD_OVL_INSERTED)
				ext_disp_path_change(EXTD_OVL_NO_REQ, MHL_SESSION_ID);

			ext_disp_get_curr_addr(input_curr_addr, 1);
			for (layid = 0; layid < EXTD_OVERLAY_CNT; layid++) {
				if (ovl_config_address[layid] != input_curr_addr[layid]) {
					ovl_config_address[layid] = input_curr_addr[layid];
					ovl_reg_updated = true;
				}

				if (ext_disp_is_dim_layer(input_curr_addr[layid]) == 1)
					fence_idx = disp_sync_find_fence_idx_by_addr(session_id, layid, 0);
				else
					fence_idx = disp_sync_find_fence_idx_by_addr(session_id,
										layid, input_curr_addr[layid]);

				mtkfb_release_fence(session_id, layid, fence_idx);
			}

			if (ovl_reg_updated == false) {
				MMProfileLogEx(ddp_mmp_get_events()->Extd_trigger,
						MMProfileFlagPulse, input_curr_addr[0], input_curr_addr[1]);
			}

			MMProfileLogEx(ddp_mmp_get_events()->Extd_UsedBuff,
					MMProfileFlagPulse, input_curr_addr[0], input_curr_addr[1]);
		}
		hdmi_video_config(p->output_video_resolution, HDMI_VIN_FORMAT_RGB888, HDMI_VOUT_FORMAT_RGB888);

		if (kthread_should_stop())
			break;
	}

	return 0;
}

/* Allocate memory, set M4U, LCD, MDP, DPI */
/* LCD overlay to memory -> MDP resize and rotate to memory -> DPI read to HDMI */
/* Will only be used in ioctl(MTK_HDMI_AUDIO_VIDEO_ENABLE) */
static enum HDMI_STATUS hdmi_drv_init(void)
{
	HDMI_FUNC();

	p->hdmi_width = hdmi_resolution_param_table[hdmi_params->init_config.vformat][0];
	p->hdmi_height = hdmi_resolution_param_table[hdmi_params->init_config.vformat][1];
	p->bg_width = 0;
	p->bg_height = 0;

	p->output_video_resolution = hdmi_params->init_config.vformat;
	p->output_audio_format = hdmi_params->init_config.aformat;
	p->scaling_factor = hdmi_params->scaling_factor < 10 ? hdmi_params->scaling_factor : 10;

	p->is_clock_on = false;	/* <--Donglei */

	if (!hdmi_fence_release_task) {
		/* disp_register_module_irq_callback(DISP_MODULE_RDMA2, _hdmi_rdma_irq_handler); K2 no RDMA2 */
#ifdef CONFIG_SINGLE_PANEL_OUTPUT
		disp_register_module_irq_callback(DISP_MODULE_RDMA0, _hdmi_rdma_irq_handler);
#else
		disp_register_module_irq_callback(DISP_MODULE_RDMA, _hdmi_rdma_irq_handler);
#endif
		hdmi_fence_release_task = kthread_create(hdmi_fence_release_kthread,
							NULL, "hdmi_fence_release_kthread");
		wake_up_process(hdmi_fence_release_task);
	}

	return HDMI_STATUS_OK;
}

/* Release memory */
/* Will only be used in ioctl(MTK_HDMI_AUDIO_VIDEO_ENABLE) */
static enum HDMI_STATUS hdmi_drv_deinit(void)
{
	HDMI_FUNC();
	hdmi_free_hdmi_buffer();

	return HDMI_STATUS_OK;
}

/* Reset HDMI Driver state */
static void hdmi_state_reset(void)
{
	HDMI_FUNC();
#ifndef CONFIG_SINGLE_PANEL_OUTPUT
	if (hdmi_drv->get_state() == HDMI_STATE_ACTIVE) {
		switch_set_state(&hdmi_switch_data, HDMI_STATE_ACTIVE);
		hdmi_reschange = HDMI_VIDEO_RESOLUTION_NUM;
	} else
#endif
	{
		switch_set_state(&hdmi_switch_data, HDMI_STATE_NO_DEVICE);
		switch_set_state(&hdmires_switch_data, 0);
	}
}

/*static*/ void hdmi_suspend(void)
{
	int i = 0;
	int session_id = 0;

	HDMI_FUNC();
	/*RET_VOID_IF(IS_HDMI_NOT_ON());*/
	if (IS_HDMI_NOT_ON())
		return;

	MMProfileLogEx(ddp_mmp_get_events()->Extd_State, MMProfileFlagStart, Plugout, 0);

	if (down_interruptible(&hdmi_update_mutex)) {
		HDMI_ERR("[hdmi][HDMI] can't get semaphore in %s()\n", __func__);
		return;
	}

	SET_HDMI_STANDBY();
	hdmi_drv->suspend();
	p->is_mhl_video_on = false;
	p->is_clock_on = false;

	ext_disp_suspend(MHL_SESSION_ID);
	session_id = ext_disp_get_sess_id();

	for (i = 0; i < EXTD_OVERLAY_CNT; i++)
		mtkfb_release_layer_fence(session_id, i);

	first_frame_done = 0;
	up(&hdmi_update_mutex);

	MMProfileLogEx(ddp_mmp_get_events()->Extd_State, MMProfileFlagEnd, Plugout, 0);
}

/*static*/ void hdmi_resume(void)
{
	HDMI_LOG("p->state is %d,(0:off, 1:on, 2:standby)\n", atomic_read(&p->state));
	/*RET_VOID_IF(IS_HDMI_NOT_STANDBY());*/
	if (IS_HDMI_NOT_STANDBY())
		return;

	MMProfileLogEx(ddp_mmp_get_events()->Extd_State, MMProfileFlagStart, Plugin, 0);

	if (down_interruptible(&hdmi_update_mutex)) {
		HDMI_ERR("[hdmi][HDMI] can't get semaphore in %s()\n", __func__);
		return;
	}

	SET_HDMI_ON();
	p->is_clock_on = true;
	hdmi_drv->resume();
	up(&hdmi_update_mutex);

	MMProfileLogEx(ddp_mmp_get_events()->Extd_State, MMProfileFlagEnd, Plugin, 0);
}

/*static*/ void hdmi_power_on(void)
{
	HDMI_FUNC();

	/*RET_VOID_IF(IS_HDMI_NOT_OFF());*/
	if (IS_HDMI_NOT_OFF())
		return;

	if (down_interruptible(&hdmi_update_mutex)) {
		HDMI_ERR("[hdmi][HDMI] can't get semaphore in %s()\n", __func__);
		return;
	}

	SET_HDMI_STANDBY();
	hdmi_drv->power_on();
	ext_disp_resume(MHL_SESSION_ID);
	up(&hdmi_update_mutex);

	if (p->is_force_disable == false) {
		if (IS_HDMI_FAKE_PLUG_IN()) {
			hdmi_resume();
			msleep(1000);
			switch_set_state(&hdmi_switch_data, HDMI_STATE_ACTIVE);
			hdmi_reschange = HDMI_VIDEO_RESOLUTION_NUM;
		} else {
#ifndef CONFIG_SINGLE_PANEL_OUTPUT
			/* this is just a ugly workaround for some tv sets... */
			if (hdmi_drv->get_state() == HDMI_STATE_ACTIVE) {
				hdmi_drv->get_params(hdmi_params);
				hdmi_resume();
			}
#endif
			hdmi_state_reset();
		}
	}
}

/*static*/ void hdmi_power_off(void)
{
	HDMI_FUNC();
	/*RET_VOID_IF(IS_HDMI_OFF());*/
	if (IS_HDMI_OFF())
		return;

	if (down_interruptible(&hdmi_update_mutex)) {
		HDMI_ERR("[hdmi][HDMI] can't get semaphore in %s()\n", __func__);
		return;
	}

	hdmi_drv->power_off();
	ext_disp_suspend(MHL_SESSION_ID);
	p->is_clock_on = false;
	SET_HDMI_OFF();
	up(&hdmi_update_mutex);

	switch_set_state(&hdmires_switch_data, 0);
}

static void hdmi_resolution_setting(int arg)
{
	HDMI_FUNC();

	if (hdmi_drv && hdmi_drv->get_params) {
		hdmi_params->init_config.vformat = arg;
		hdmi_drv->get_params(hdmi_params);

		extd_dpi_params.dispif_config.dpi.clk_pol = hdmi_params->clk_pol;
		extd_dpi_params.dispif_config.dpi.de_pol = hdmi_params->de_pol;
		extd_dpi_params.dispif_config.dpi.hsync_pol = hdmi_params->hsync_pol;
		extd_dpi_params.dispif_config.dpi.vsync_pol = hdmi_params->vsync_pol;

		extd_dpi_params.dispif_config.dpi.hsync_pulse_width = hdmi_params->hsync_pulse_width;
		extd_dpi_params.dispif_config.dpi.hsync_back_porch = hdmi_params->hsync_back_porch;
		extd_dpi_params.dispif_config.dpi.hsync_front_porch = hdmi_params->hsync_front_porch;

		extd_dpi_params.dispif_config.dpi.vsync_pulse_width = hdmi_params->vsync_pulse_width;
		extd_dpi_params.dispif_config.dpi.vsync_back_porch = hdmi_params->vsync_back_porch;
		extd_dpi_params.dispif_config.dpi.vsync_front_porch = hdmi_params->vsync_front_porch;

		extd_dpi_params.dispif_config.dpi.dpi_clock = hdmi_params->input_clock;

		p->bg_height = ((hdmi_params->height * p->scaling_factor) / 100 >> 2) << 2;
		p->bg_width = ((hdmi_params->width * p->scaling_factor) / 100 >> 2) << 2;
		p->hdmi_width = hdmi_params->width - p->bg_width;
		p->hdmi_height = hdmi_params->height - p->bg_height;

		p->output_video_resolution = hdmi_params->init_config.vformat;

		extd_dpi_params.dispif_config.dpi.width = p->hdmi_width;
		extd_dpi_params.dispif_config.dpi.height = p->hdmi_height;
		extd_dpi_params.dispif_config.dpi.bg_width = p->bg_width;
		extd_dpi_params.dispif_config.dpi.bg_height = p->bg_height;

		extd_dpi_params.dispif_config.dpi.format = LCM_DPI_FORMAT_RGB888;
		extd_dpi_params.dispif_config.dpi.rgb_order = LCM_COLOR_ORDER_RGB;
		extd_dpi_params.dispif_config.dpi.i2x_en = true;
		extd_dpi_params.dispif_config.dpi.i2x_edge = 2;
		extd_dpi_params.dispif_config.dpi.embsync = false;
	}

	ext_disp_set_lcm_param(&(extd_dpi_params.dispif_config));
	HDMI_LOG("hdmi_resolution_setting_res (%d)\n", arg);
}

int hdmi_check_resolution(int src_w, int src_h, int physical_w, int physical_h)
{
	int ret = 0;

	if (physical_w <= 0 || physical_h <= 0 || src_w > physical_w || src_h > physical_h) {
		HDMI_LOG("hdmi_check_resolution fail\n");
		ret = -1;
	}

	return ret;
}

int hdmi_recompute_bg(int src_w, int src_h)
{
	int ret = 0;
	/* fail -1; 0 ok but not changed; 1 ok and changed */
/*
	HDMI_LOG("hdmi_recompute_bg(%d*%d), resolution: %d\n", src_w, src_h, p->output_video_resolution);
	int physical_w = 0, physical_h = 0;
	int temp = 0;
	int ret = 0;

	int extd_path_state = ext_disp_is_alive();

	if ((extd_path_state == EXTD_DEINIT || extd_path_state == EXTD_SUSPEND) || is_hdmi_active() == false) {
		HDMI_LOG("hdmi is not alive\n");
		return -1;
	}

	switch (p->output_video_resolution) {
	case HDMI_VIDEO_720x480p_60Hz:
		{
			physical_w = 720;
			physical_h = 480;
			break;
		}
	case HDMI_VIDEO_1280x720p_60Hz:
		{
			physical_w = 1280;
			physical_h = 720;
			break;
		}
	case HDMI_VIDEO_1920x1080p_30Hz:
	case HDMI_VIDEO_1920x1080p_60Hz:
		{
			physical_w = 1920;
			physical_h = 1080;
			break;
		}
	default:
		{
			HDMI_LOG("hdmi_recompute_bg, resolution fail\n");
			break;
		}
	}

	ret = hdmi_check_resolution(src_w, src_h, physical_w, physical_h);
	if (ret >= 0) {
		extd_dpi_params.dispif_config.dpi.bg_width = physical_w - src_w;
		extd_dpi_params.dispif_config.dpi.bg_height = physical_h - src_h;

		extd_dpi_params.dispif_config.dpi.width = src_w;
		extd_dpi_params.dispif_config.dpi.height = src_h;
		HDMI_LOG("bg_width*bg_height(%d*%d)\n", extd_dpi_params.dispif_config.dpi.bg_width,
			extd_dpi_params.dispif_config.dpi.bg_height);
	}
*/
	return ret;
}

/* HDMI Driver state callback function */
void hdmi_state_callback(enum HDMI_STATE state)
{
	HDMI_LOG("[hdmi]%s, state = %d\n", __func__, state);
	/*RET_VOID_IF((p->is_force_disable == true));*/
	/*RET_VOID_IF(IS_HDMI_FAKE_PLUG_IN());*/
	if ((p->is_force_disable == true) || (IS_HDMI_FAKE_PLUG_IN()))
		return;

	switch (state) {
	case HDMI_STATE_NO_DEVICE:
		{
			hdmi_suspend();
			switch_set_state(&hdmi_switch_data, HDMI_STATE_NO_DEVICE);
			switch_set_state(&hdmires_switch_data, 0);

#if defined(CONFIG_MTK_SMARTBOOK_SUPPORT) && defined(CONFIG_HAS_SBSUSPEND)
			if (hdmi_params->cabletype == MHL_SMB_CABLE)
				sb_plug_out();
#endif
			HDMI_LOG("[hdmi]%s, state = %d out!\n", __func__, state);
			break;
		}
	case HDMI_STATE_ACTIVE:
		{
			if (IS_HDMI_ON()) {
				HDMI_LOG("[hdmi]%s, already on(%d) !\n", __func__, atomic_read(&p->state));
				break;
			}

			hdmi_drv->get_params(hdmi_params);
			hdmi_resume();

			if (atomic_read(&p->state) > HDMI_POWER_STATE_OFF) {
				hdmi_reschange = HDMI_VIDEO_RESOLUTION_NUM;
				switch_set_state(&hdmi_switch_data, HDMI_STATE_ACTIVE);
			}
#if defined(CONFIG_MTK_SMARTBOOK_SUPPORT) && defined(CONFIG_HAS_SBSUSPEND)
			if (hdmi_params->cabletype == MHL_SMB_CABLE)
				sb_plug_in();
#endif
			HDMI_LOG("[hdmi]%s, state = %d out!\n", __func__, state);
			break;
		}
	default:
		{
			HDMI_LOG("[hdmi]%s, state not support\n", __func__);
			break;
		}
	}
}

void hdmi_set_layer_num(int layer_num)
{
	if (layer_num >= 0)
		hdmi_layer_num = layer_num;
}

int hdmi_enable(int enable)
{
	HDMI_FUNC();
	if (enable) {
		if (p->is_enabled) {
			HDMI_LOG("[hdmi] hdmi already enable, %s()\n", __func__);
			return 0;
		}

		if (hdmi_drv->enter)
			hdmi_drv->enter();

		hdmi_drv_init();
		hdmi_power_on();
		p->is_enabled = true;
	} else {
		if (!p->is_enabled)
			return 0;

		hdmi_power_off();
		hdmi_drv_deinit();
		/* when disable hdmi, HPD is disabled */
		switch_set_state(&hdmi_switch_data, HDMI_STATE_NO_DEVICE);

		p->is_enabled = false;

		if (hdmi_drv->exit)
			hdmi_drv->exit();
	}

	return 0;
}

int hdmi_power_enable(int enable)
{
	HDMI_FUNC();
	/*RETIF(!p->is_enabled, 0);*/
	if (!p->is_enabled) {
		HDMI_LOG("return in %d\n", __LINE__);
		return 0;
	}

	if (enable) {
		/*RETIF(otg_enable_status, 0);*/
		if (otg_enable_status) {
			HDMI_LOG("return in %d\n", __LINE__);
			return 0;
		}
		hdmi_power_on();
	} else {
		hdmi_power_off();
		switch_set_state(&hdmi_switch_data, HDMI_STATE_NO_DEVICE);
	}

	return 0;
}

void hdmi_force_disable(int enable)
{
	HDMI_FUNC();
/*
	RETIF(!p->is_enabled, 0);
	RETIF(IS_HDMI_OFF(), 0);
*/
	if (!p->is_enabled) {
		HDMI_LOG("return in %d\n", __LINE__);
		return;
	}
	if (IS_HDMI_OFF()) {
		HDMI_LOG("return in %d\n", __LINE__);
		return;
	}

	if (enable) {
		if (p->is_force_disable == true)
			return;

		if (IS_HDMI_FAKE_PLUG_IN() || (hdmi_drv->get_state() == HDMI_STATE_ACTIVE)) {
			hdmi_suspend();
			switch_set_state(&hdmi_switch_data, HDMI_STATE_NO_DEVICE);
			switch_set_state(&hdmires_switch_data, 0);
		}

		p->is_force_disable = true;
	} else {
		if (p->is_force_disable == false)
			return;

		if (IS_HDMI_FAKE_PLUG_IN() || (hdmi_drv->get_state() == HDMI_STATE_ACTIVE)) {
			hdmi_resume();
			msleep(1000);
			switch_set_state(&hdmi_switch_data, HDMI_STATE_ACTIVE);
			hdmi_reschange = HDMI_VIDEO_RESOLUTION_NUM;
		}

		p->is_force_disable = false;
	}
}

void hdmi_set_USBOTG_status(int status)
{
	HDMI_LOG("MTK_HDMI_USBOTG_STATUS, arg=%d, enable %d\n", status, p->is_enabled);
/*
	RETIF(!p->is_enabled, 0);
	RETIF((hdmi_params->cabletype != MHL_CABLE), 0);
*/
	if (!p->is_enabled) {
		HDMI_LOG("return in %d\n", __LINE__);
		return;
	}
	if (hdmi_params->cabletype != MHL_CABLE) {
		HDMI_LOG("return in %d\n", __LINE__);
		return;
	}

	if (status)
		otg_enable_status = true;
	else {
		otg_enable_status = false;
		/*RETIF(p->is_force_disable, 0);*/
		if (p->is_force_disable) {
			HDMI_LOG("return in %d\n", __LINE__);
			return;
		}
		hdmi_power_on();
	}
}

int hdmi_set_audio_enable(int enable)
{
	/*RETIF(!p->is_enabled, 0);*/
	if (!p->is_enabled) {
		HDMI_LOG("return in %d\n", __LINE__);
		return 0;
	}

	hdmi_drv->audio_enable(enable);
	return 0;
}

void hdmi_set_video_enable(int enable)
{
	/*RETIF(!p->is_enabled, 0);*/
	if (!p->is_enabled) {
		HDMI_LOG("return in %d\n", __LINE__);
		return;
	}

	hdmi_drv->video_enable(enable);
}

int hdmi_set_resolution(int res)
{
	int extd_path_state = 0;
	int i = 0;
	int session_id = 0;

	HDMI_LOG("video resolution configuration, res:%d, old res:%ld, video_on:%d\n",
		res, hdmi_reschange, p->is_mhl_video_on);
	/*RETIF(!p->is_enabled, 0);*/
	if (!p->is_enabled) {
		HDMI_LOG("return in %d\n", __LINE__);
		return 0;
	}


	if (hdmi_reschange == res) {
		HDMI_LOG("hdmi_reschange=%ld\n", hdmi_reschange);
		return 0;
	}

	p->is_clock_on = false;
	hdmi_reschange = res;

	MMProfileLogEx(ddp_mmp_get_events()->Extd_State, MMProfileFlagStart, ResChange, res);

	if (down_interruptible(&hdmi_update_mutex)) {
		HDMI_LOG("[HDMI] can't get semaphore in\n");
		return 0;
	}

	/*if ddp path already start, stop ddp path, and release all fence of external session */
	extd_path_state = ext_disp_is_alive();

	if (extd_path_state == EXTD_RESUME) {
		ext_disp_suspend(MHL_SESSION_ID);

		session_id = ext_disp_get_sess_id();

		for (i = 0; i < EXTD_OVERLAY_CNT; i++)
			mtkfb_release_layer_fence(session_id, i);
	}

	hdmi_resolution_setting(res);
	p->is_mhl_video_on = false;
	first_frame_done = 0;

	up(&hdmi_update_mutex);

	switch_set_state(&hdmires_switch_data, hdmi_reschange + 1);
	p->is_clock_on = true;
	MMProfileLogEx(ddp_mmp_get_events()->Extd_State, MMProfileFlagEnd, ResChange, hdmi_reschange + 1);

	return 0;
}

int hdmi_get_dev_info(int is_sf, void *info)
{
	int ret = 0;

	if (is_sf == AP_GET_INFO) {
		int displayid = 0;
		mtk_dispif_info_t hdmi_info;

		if (!info) {
			HDMI_LOG("ioctl pointer is NULL\n");
			return -EFAULT;
		}

		MMProfileLogEx(ddp_mmp_get_events()->Extd_DevInfo, MMProfileFlagStart, p->is_enabled, p->is_clock_on);

		if (copy_from_user(&displayid, info, sizeof(displayid))) {
			MMProfileLogEx(ddp_mmp_get_events()->Extd_ErrorInfo, MMProfileFlagPulse, Devinfo, 0);
			HDMI_ERR(": copy_from_user failed! line:%d\n", __LINE__);
			return -EAGAIN;
		}

		if (displayid != MTKFB_DISPIF_HDMI)
			HDMI_LOG(": invalid display id:%d\n", displayid);

		memset(&hdmi_info, 0, sizeof(hdmi_info));
		hdmi_info.displayFormat = DISPIF_FORMAT_RGB888;
		hdmi_info.displayHeight = p->hdmi_height;
		hdmi_info.displayWidth = p->hdmi_width;
		hdmi_info.display_id = displayid;
		hdmi_info.isConnected = 1;
		hdmi_info.displayMode = DISPIF_MODE_COMMAND;

		if (hdmi_params->cabletype == MHL_SMB_CABLE)
			hdmi_info.displayType = HDMI_SMARTBOOK;
		else if (hdmi_params->cabletype == MHL_CABLE)
			hdmi_info.displayType = MHL;
		else
			hdmi_info.displayType = HDMI;

		hdmi_info.isHwVsyncAvailable = HW_DPI_VSYNC_SUPPORT;
		hdmi_info.vsyncFPS = 60;

		if (copy_to_user(info, &hdmi_info, sizeof(hdmi_info))) {
			MMProfileLogEx(ddp_mmp_get_events()->Extd_ErrorInfo, MMProfileFlagPulse, Devinfo, 1);
			HDMI_ERR("copy_to_user failed! line:%d\n", __LINE__);
			ret = -EFAULT;
		}

		MMProfileLogEx(ddp_mmp_get_events()->Extd_DevInfo, MMProfileFlagEnd,
				p->is_enabled, hdmi_info.displayType);
		HDMI_LOG("DEV_INFO configuration get displayType-%d\n", hdmi_info.displayType);
	} else if (is_sf == SF_GET_INFO) {
		disp_session_info *dispif_info = (disp_session_info *) info;

		memset((void *)dispif_info, 0, sizeof(disp_session_info));

		dispif_info->isOVLDisabled = (hdmi_layer_num == 1) ? 1 : 0;
/* if(ext_disp_path_get_mode() == EXTD_RDMA_DPI_MODE) */
/* { */
/* dispif_info->isOVLDisabled = 0; */
/* } */

		dispif_info->maxLayerNum = hdmi_layer_num;
		dispif_info->displayFormat = DISPIF_FORMAT_RGB888;
		dispif_info->displayHeight = p->hdmi_height;
		dispif_info->displayWidth = p->hdmi_width;
		dispif_info->displayMode = DISP_IF_MODE_VIDEO;

		if (hdmi_params->cabletype == MHL_SMB_CABLE) {
			dispif_info->displayType = DISP_IF_HDMI_SMARTBOOK;
			if (IS_HDMI_OFF())
				dispif_info->displayType = DISP_IF_MHL;
		} else if (hdmi_params->cabletype == MHL_CABLE)
			dispif_info->displayType = DISP_IF_MHL;
		else
			dispif_info->displayType = DISP_IF_HDMI;

		dispif_info->isHwVsyncAvailable = HW_DPI_VSYNC_SUPPORT;
		dispif_info->vsyncFPS = 60;

		if (dispif_info->displayWidth * dispif_info->displayHeight <= 240 * 432)
			dispif_info->physicalHeight = dispif_info->physicalWidth = 0;
		else if (dispif_info->displayWidth * dispif_info->displayHeight <= 320 * 480)
			dispif_info->physicalHeight = dispif_info->physicalWidth = 0;
		else if (dispif_info->displayWidth * dispif_info->displayHeight <= 480 * 854)
			dispif_info->physicalHeight = dispif_info->physicalWidth = 0;
		else
			dispif_info->physicalHeight = dispif_info->physicalWidth = 0;

		dispif_info->isConnected = 1;
		dispif_info->isHDCPSupported = hdmi_params->HDCPSupported;

		/* /HDMI_LOG("extd_hdmi_get_dev_info lays %d, type %d, H %d\n", dispif_info->maxLayerNum,
				dispif_info->displayType, dispif_info->displayHeight); */
	}

	return ret;
}

int hdmi_get_capability(void *info)
{
	int ret = 0;
	int query_type = 0;

	query_type = hdmi_get_support_info();

	if (copy_to_user(info, &query_type, sizeof(query_type))) {
		HDMI_LOG(": copy_to_user error! line:%d\n", __LINE__);
		ret = -EFAULT;
	}
	HDMI_LOG("[hdmi][HDMI] query_type  done %x\n", query_type);

	return ret;
}

int hdmi_get_edid(void *edid_info)
{
	int ret = 0;
	HDMI_EDID_T pv_get_info;

	memset(&pv_get_info, 0, sizeof(pv_get_info));

	if (!edid_info) {
		HDMI_LOG("ioctl pointer is NULL\n");
		return -EFAULT;
	}

	if (hdmi_drv->getedid)
		hdmi_drv->getedid(&pv_get_info);

	if (copy_to_user(edid_info, &pv_get_info, sizeof(pv_get_info))) {
		HDMI_LOG("copy_to_user failed! line:%d\n", __LINE__);
		ret = -EFAULT;
	}

	return ret;
}

int hdmi_get_device_type(void)
{
	int device_type = -1;

	if (IS_HDMI_ON()) {
		if (hdmi_params->cabletype == MHL_SMB_CABLE)
			device_type = DISP_IF_HDMI_SMARTBOOK;
		else if (hdmi_params->cabletype == MHL_CABLE)
			device_type = DISP_IF_MHL;
	}

	return device_type;
}

int hdmi_ioctl(unsigned int ioctl_cmd, int param1, int param2, unsigned long *params)
{
	/* HDMI_LOG("hdmi_ioctl ioctl_cmd:%d\n", ioctl_cmd); */
	int ret = 0;

	switch (ioctl_cmd) {
	case RECOMPUTE_BG_CMD:
		ret = hdmi_recompute_bg(param1, param2);
		break;
	case GET_DEV_TYPE_CMD:
		ret = hdmi_get_device_type();
		break;
	case SET_LAYER_NUM_CMD:
		hdmi_set_layer_num(param1);
		break;
	default:
		HDMI_LOG("hdmi_ioctl unknown command\n");
		break;
	}

	return ret;
}

void hdmi_udelay(unsigned int us)
{
	udelay(us);
}

void hdmi_mdelay(unsigned int ms)
{
	msleep(ms);
}

int hdmi_init(void)
{
	int ret = 0;

	HDMI_ERR(" start\n");
	/* for support hdmi hotplug, inform AP the event */
	hdmi_switch_data.name = "hdmi";
	hdmi_switch_data.index = 0;
	hdmi_switch_data.state = HDMI_STATE_NO_DEVICE;
	ret = switch_dev_register(&hdmi_switch_data);

	if (ret)
		HDMI_ERR("[hdmi][HDMI]switch_dev_register failed, returned:%d!\n", ret);

	hdmires_switch_data.name = "res_hdmi";
	hdmires_switch_data.index = 0;
	hdmires_switch_data.state = 0;
	ret = switch_dev_register(&hdmires_switch_data);

	if (ret)
		HDMI_ERR("[hdmi][HDMI]switch_dev_register failed, returned:%d!\n", ret);
	HDMI_ERR(" done\n");
	return 0;
}

int hdmi_post_init(void)
{
	/*int boot_mode = 0;*/
	/*struct EXTD_DRIVER *extd_factory_driver = NULL;*/
	static const struct HDMI_UTIL_FUNCS hdmi_utils = {
		.udelay = hdmi_udelay,
		.mdelay = hdmi_mdelay,
		.state_callback = hdmi_state_callback,
	};
	HDMI_ERR(" start\n");

	hdmi_drv = (struct HDMI_DRIVER *) HDMI_GetDriver();

	if (NULL == hdmi_drv) {
		HDMI_ERR("[hdmi]%s, hdmi_init fail, can not get hdmi driver handle\n", __func__);
		return -1;
	}

	hdmi_drv->set_util_funcs(&hdmi_utils);

	hdmi_params->init_config.vformat = HDMI_VIDEO_1280x720p_60Hz;
	hdmi_drv->get_params(hdmi_params);

	hdmi_drv->init();	/* need to remove to power on function Donglei */
	if (hdmi_drv->register_callback != NULL) {
		/*
		boot_mode = get_boot_mode();
		if (boot_mode == FACTORY_BOOT || boot_mode == ATE_FACTORY_BOOT) {
			extd_factory_driver = EXTD_Factory_HDMI_Driver();
			if (extd_factory_driver)
				extd_factory_driver->init();
		} else
		*/
		HDMI_ERR("[hdmi} register_callback\n");
		hdmi_drv->register_callback(hdmi_state_callback);
	}

	memset((void *)&hdmi_context, 0, sizeof(struct _t_hdmi_context));
	memset((void *)&extd_dpi_params, 0, sizeof(extd_dpi_params));

	p->output_mode = hdmi_params->output_mode;

	SET_HDMI_OFF();

	init_waitqueue_head(&hdmi_fence_release_wq);
	init_waitqueue_head(&hdmi_vsync_wq);

	/*Extd_DBG_Init();*/
	HDMI_ERR(" done\n");
	return 0;
}
#endif

const struct EXTD_DRIVER *EXTD_HDMI_Driver(void)
{
	static const struct EXTD_DRIVER extd_driver_hdmi = {
#if defined(CONFIG_MTK_HDMI_SUPPORT)
		.init =              hdmi_init,
		.post_init =         hdmi_post_init,
		.deinit =            NULL,
		.enable =            hdmi_enable,
		.power_enable =      hdmi_power_enable,
		.set_audio_enable =  hdmi_set_audio_enable,
		.set_resolution =    hdmi_set_resolution,
		.get_dev_info =      hdmi_get_dev_info,
		.get_capability =    hdmi_get_capability,
		.get_edid =          hdmi_get_edid,
		.wait_vsync =        hdmi_waitVsync,
		.fake_connect =      hdmi_cable_fake_connect,
		.factory_mode_test = NULL,
		.ioctl =             hdmi_ioctl,
#else
		.init = 0,
#endif
	};

	return &extd_driver_hdmi;
}
