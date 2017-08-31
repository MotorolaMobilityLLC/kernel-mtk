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

#define LOG_TAG "ddp_manager"

#include <linux/slab.h>
#include <linux/mutex.h>

#include "disp_helper.h"
#include "lcm_drv.h"
#include "ddp_reg.h"
#include "ddp_path.h"
#include "ddp_irq.h"
#include "ddp_drv.h"
#include "ddp_debug.h"
#include "ddp_manager.h"
#include "ddp_rdma.h"
#include "ddp_rdma_ex.h"
#include "ddp_ovl.h"

#include "ddp_log.h"
#include "ddp_clkmgr.h"
/* #pragma GCC optimize("O0") */

#include "disp_rsz.h"
#include "mtk_hrt.h"
#include "ddp_rsz.h"

static int ddp_manager_init;
#define DDP_MAX_MANAGER_HANDLE (DISP_MUTEX_DDP_COUNT+DISP_MUTEX_DDP_FIRST)

struct DPMGR_WQ_HANDLE {
	unsigned int init;
	enum DISP_PATH_EVENT event;
	wait_queue_head_t wq;
	unsigned long long data;
};

struct DDP_IRQ_EVENT_MAPPING {
	enum DDP_IRQ_BIT irq_bit;
};

struct ddp_path_handle {
	struct cmdqRecStruct *cmdqhandle;
	int hwmutexid;
	int power_state;
	enum DDP_MODE mode;
	struct mutex mutex_lock;
	struct DDP_IRQ_EVENT_MAPPING irq_event_map[DISP_PATH_EVENT_NUM];
	struct DPMGR_WQ_HANDLE wq_list[DISP_PATH_EVENT_NUM];
	enum DDP_SCENARIO_ENUM scenario;
	enum DISP_MODULE_ENUM mem_module;
	struct disp_ddp_path_config last_config;
};

struct DDP_MANAGER_CONTEXT {
	int handle_cnt;
	int mutex_idx;
	int power_state;
	struct mutex mutex_lock;
	int module_usage_table[DISP_MODULE_NUM];
	struct ddp_path_handle *module_path_table[DISP_MODULE_NUM];
	struct ddp_path_handle *handle_pool[DDP_MAX_MANAGER_HANDLE];
};

static bool ddp_valid_engine[DISP_MODULE_NUM] = {
	1, /* DISP_MODULE_OVL0 */
	1, /* DISP_MODULE_OVL1 */
	1, /* DISP_MODULE_OVL0_2L */
	1, /* DISP_MODULE_OVL1_2L */
	0, /* DISP_MODULE_OVL0_VIRTUAL */

	0, /* DISP_MODULE_OVL0_2L_VIRTUAL */
	0, /* DISP_MODULE_OVL1_2L_VIRTUAL */
	1, /* DISP_MODULE_RDMA0 */
	1, /* DISP_MODULE_RDMA1 */
	0, /* DISP_MODULE_RDMA2 */

	1, /* DISP_MODULE_WDMA0 */
	1, /* DISP_MODULE_WDMA1 */
	1, /* DISP_MODULE_COLOR0 */
	1, /* DISP_MODULE_COLOR1 */
	1, /* DISP_MODULE_CCORR0 */

	1, /* DISP_MODULE_CCORR1 */
	1, /* DISP_MODULE_AAL0 */
	1, /* DISP_MODULE_AAL1 */
	1, /* DISP_MODULE_GAMMA0 */
	1, /* DISP_MODULE_GAMMA1 */

	1, /* DISP_MODULE_OD */
	1, /* DISP_MODULE_DITHER0 */
	1, /* DISP_MODULE_DITHER1 */
	0, /* DISP_PATH0 */
	0, /* DISP_PATH1 */

	1, /* DISP_MODULE_UFOE */
	1, /* DISP_MODULE_DSC */
	1, /* DISP_MODULE_DSC_2ND */
	1, /* DISP_MODULE_SPLIT0 */
	0, /* DISP_MODULE_DPI */

	0, /* DISP_MODULE_DSI0 */
	0, /* DISP_MODULE_DSI1 */
	0, /* DISP_MODULE_DSIDUAL */
	0, /* DISP_MODULE_PWM0 */
	0, /* DISP_MODULE_PWM1 */

	0, /* DISP_MODULE_CONFIG */
	0, /* DISP_MODULE_MUTEX */
	0, /* DISP_MODULE_SMI_COMMON */
	0, /* DISP_MODULE_SMI_LARB0 */
	0, /* DISP_MODULE_SMI_LARB1 */

	0, /* DISP_MODULE_MIPI0 */
	0, /* DISP_MODULE_MIPI1 */
	1, /* DISP_MODULE_RSZ0 */
	1, /* DISP_MODULE_RSZ1 */

	0, /* DISP_MODULE_MTCMOS */
	0, /* DISP_MODULE_FAKE_ENG */
	1, /* DISP_MODULE_MDP_WROT0 */
	1, /* DISP_MODULE_MDP_WROT1 */
	0, /* DISP_MODULE_CLOCK_MUX */
	0, /* DISP_MODULE_UNKNOWN */
};

int get_ddp_valid_engine(int module_num)
{
	return ddp_valid_engine[module_num];
}

#define DEFAULT_IRQ_EVENT_SCENARIO (4)
static struct DDP_IRQ_EVENT_MAPPING ddp_irq_event_list[DEFAULT_IRQ_EVENT_SCENARIO][DISP_PATH_EVENT_NUM] = {
	{			/* ovl0 path */
	 {DDP_IRQ_RDMA0_DONE},	/*FRAME_DONE */
	 {DDP_IRQ_RDMA0_START},	/*FRAME_START */
	 {DDP_IRQ_RDMA0_REG_UPDATE},	/*FRAME_REG_UPDATE */
	 {DDP_IRQ_RDMA0_TARGET_LINE},	/*FRAME_TARGET_LINE */
	 {DDP_IRQ_WDMA0_FRAME_COMPLETE},	/*FRAME_COMPLETE */
	 {DDP_IRQ_RDMA0_TARGET_LINE},	/*FRAME_STOP */
	 {DDP_IRQ_RDMA0_REG_UPDATE},	/*IF_CMD_DONE */
	 {DDP_IRQ_DSI0_EXT_TE},	/*IF_VSYNC */
	 {DDP_IRQ_UNKNOWN}, /*TRIGER*/ {DDP_IRQ_AAL0_OUT_END_FRAME},	/*AAL_OUT_END_EVENT */
	 },
	{			/* ovl1 path */
	 {DDP_IRQ_RDMA1_DONE},	/*FRAME_DONE */
	 {DDP_IRQ_RDMA1_START},	/*FRAME_START */
	 {DDP_IRQ_RDMA1_REG_UPDATE},	/*FRAME_REG_UPDATE */
	 {DDP_IRQ_RDMA1_TARGET_LINE},	/*FRAME_TARGET_LINE */
	 {DDP_IRQ_WDMA1_FRAME_COMPLETE},	/*FRAME_COMPLETE */
	 {DDP_IRQ_RDMA1_TARGET_LINE},	/*FRAME_STOP */
	 {DDP_IRQ_RDMA1_REG_UPDATE},	/*IF_CMD_DONE */
	 {DDP_IRQ_RDMA1_TARGET_LINE},	/*IF_VSYNC */
	 {DDP_IRQ_UNKNOWN}, /*TRIGER*/ {DDP_IRQ_AAL1_OUT_END_FRAME},	/*AAL_OUT_END_EVENT */
	 },
	{			/* rdma path */
	 {DDP_IRQ_RDMA2_DONE},	/*FRAME_DONE */
	 {DDP_IRQ_RDMA2_START},	/*FRAME_START */
	 {DDP_IRQ_RDMA2_REG_UPDATE},	/*FRAME_REG_UPDATE */
	 {DDP_IRQ_RDMA2_TARGET_LINE},	/*FRAME_TARGET_LINE */
	 {DDP_IRQ_UNKNOWN},	/*FRAME_COMPLETE */
	 {DDP_IRQ_RDMA2_TARGET_LINE},	/*FRAME_STOP */
	 {DDP_IRQ_RDMA2_REG_UPDATE},	/*IF_CMD_DONE */
	 {DDP_IRQ_RDMA2_TARGET_LINE},	/*IF_VSYNC */
	 {DDP_IRQ_UNKNOWN}, /*TRIGER*/ {DDP_IRQ_UNKNOWN},	/*AAL_OUT_END_EVENT */
	 },
	{			/* ovl0 path */
	 {DDP_IRQ_RDMA0_DONE},	/*FRAME_DONE */
	 {DDP_IRQ_MUTEX1_SOF},	/*FRAME_START */
	 {DDP_IRQ_RDMA0_REG_UPDATE},	/*FRAME_REG_UPDATE */
	 {DDP_IRQ_RDMA0_TARGET_LINE},	/*FRAME_TARGET_LINE */
	 {DDP_IRQ_WDMA0_FRAME_COMPLETE},	/*FRAME_COMPLETE */
	 {DDP_IRQ_RDMA0_TARGET_LINE},	/*FRAME_STOP */
	 {DDP_IRQ_RDMA0_REG_UPDATE},	/*IF_CMD_DONE */
	 {DDP_IRQ_DSI0_EXT_TE},	/*IF_VSYNC */
	 {DDP_IRQ_UNKNOWN}, /*TRIGER*/ {DDP_IRQ_AAL0_OUT_END_FRAME},	/*AAL_OUT_END_EVENT */
	 }
};

static char *path_event_name(enum DISP_PATH_EVENT event)
{
	switch (event) {
	case DISP_PATH_EVENT_FRAME_START:
		return "FRAME_START";
	case DISP_PATH_EVENT_FRAME_DONE:
		return "FRAME_DONE";
	case DISP_PATH_EVENT_FRAME_REG_UPDATE:
		return "REG_UPDATE";
	case DISP_PATH_EVENT_FRAME_TARGET_LINE:
		return "TARGET_LINE";
	case DISP_PATH_EVENT_FRAME_COMPLETE:
		return "FRAME COMPLETE";
	case DISP_PATH_EVENT_FRAME_STOP:
		return "FRAME_STOP";
	case DISP_PATH_EVENT_IF_CMD_DONE:
		return "FRAME_STOP";
	case DISP_PATH_EVENT_IF_VSYNC:
		return "VSYNC";
	case DISP_PATH_EVENT_TRIGGER:
		return "TRIGGER";
	case DISP_PATH_EVENT_DELAYED_TRIGGER_33ms:
		return "DELAY_TRIG";
	default:
		return "unknown event";
	}
	return "unknown event";
}

static struct DDP_MANAGER_CONTEXT *_get_context(void)
{
	static int is_context_inited;
	static struct DDP_MANAGER_CONTEXT context;

	if (!is_context_inited) {
		memset((void *)&context, 0, sizeof(struct DDP_MANAGER_CONTEXT));
		context.mutex_idx = (1 << DISP_MUTEX_DDP_COUNT) - 1;
		mutex_init(&context.mutex_lock);
		is_context_inited = 1;
	}
	return &context;
}

static int path_top_clock_off(void)
{
	int i = 0;
	struct DDP_MANAGER_CONTEXT *context = _get_context();

	if (context->power_state) {
		for (i = 0; i < DDP_MAX_MANAGER_HANDLE; i++) {
			if (context->handle_pool[i] != NULL &&
			    context->handle_pool[i]->power_state != 0)
				return 0;
		}
		context->power_state = 0;
		for (i = 0 ; i < DISP_MODULE_NUM ; i++)
			if (ddp_valid_engine[i])
				ddp_clk_disable_by_module(i);
		ddp_path_top_clock_off();
	}
	return 0;
}

static int path_top_clock_on(void)
{
	struct DDP_MANAGER_CONTEXT *context = _get_context();
	int i;

	if (!context->power_state) {
		context->power_state = 1;
		ddp_path_top_clock_on();
		for (i = 0 ; i < DISP_MODULE_NUM ; i++)
			if (ddp_valid_engine[i])
				ddp_clk_enable_by_module(i);
	}
	return 0;
}

static int module_power_off(enum DISP_MODULE_ENUM module)
{
	if (module == DISP_MODULE_DSI0 ||
	    module == DISP_MODULE_DSI1 ||
	    module == DISP_MODULE_DSIDUAL)
		return 0;

	if (ddp_modules_driver[module] != 0) {
		if (ddp_modules_driver[module]->power_off != 0)
			ddp_modules_driver[module]->power_off(module, NULL);	/* now just 0; */
	}
	return 0;
}

static int module_power_on(enum DISP_MODULE_ENUM module)
{
	if (module == DISP_MODULE_DSI0 ||
	    module == DISP_MODULE_DSI1 ||
	    module == DISP_MODULE_DSIDUAL)
		return 0;

	if (ddp_modules_driver[module] != 0) {
		if (ddp_modules_driver[module]->power_on != 0)
			ddp_modules_driver[module]->power_on(module, NULL);	/* now just 0; */
	}
	return 0;
}

static struct ddp_path_handle *find_handle_by_module(enum DISP_MODULE_ENUM module)
{
	if ((module == DISP_MODULE_DSI0) && (!_get_context()->module_path_table[DISP_MODULE_DSI0]))
		return _get_context()->module_path_table[DISP_MODULE_DSIDUAL];

	return _get_context()->module_path_table[module];
}

int dpmgr_module_notify(enum DISP_MODULE_ENUM module, enum DISP_PATH_EVENT event)
{
	int ret = 0;
	struct ddp_path_handle *handle = find_handle_by_module(module);

	if (handle)
		ret = dpmgr_signal_event(handle, event);

	mmprofile_log_ex(ddp_mmp_get_events()->primary_display_aalod_trigger, MMPROFILE_FLAG_PULSE,
		       module, event);
	return ret;
}

static int assign_default_irqs_table(enum DDP_SCENARIO_ENUM scenario, struct DDP_IRQ_EVENT_MAPPING *irq_events)
{
	int idx = 0;

	switch (scenario) {
	case DDP_SCENARIO_PRIMARY_DISP:
	case DDP_SCENARIO_PRIMARY_RDMA0_COLOR0_DISP:
	case DDP_SCENARIO_PRIMARY_RDMA0_DISP:
	case DDP_SCENARIO_PRIMARY_BYPASS_RDMA:
	case DDP_SCENARIO_PRIMARY_DITHER_MEMOUT:
	case DDP_SCENARIO_PRIMARY_UFOE_MEMOUT:
	case DDP_SCENARIO_PRIMARY_ALL:
	case DDP_SCENARIO_DITHER_1TO2:
	case DDP_SCENARIO_UFOE_1TO2:

	case DDP_SCENARIO_PRIMARY_DISP_LEFT:
	case DDP_SCENARIO_PRIMARY_ALL_LEFT:
	case DDP_SCENARIO_PRIMARY_OVL_MEMOUT_LEFT:
	case DDP_SCENARIO_PRIMARY_RDMA_COLOR_DISP_LEFT:


	case DDP_SCENARIO_PRIMARY_OVL0_RSZ0_DISP:
	case DDP_SCENARIO_PRIMARY_OVL02L_RSZ0_DISP:

	case DDP_SCENARIO_PRIMARY_OVL0_RSZ0_1TO2:
	case DDP_SCENARIO_PRIMARY_OVL02L_RSZ0_1TO2:

	case DDP_SCENARIO_PRIMARY_RDMA0_CCORR0_RSZ0_DISP:

	case DDP_SCENARIO_PRIMARY_OVL0_RSZ0_MEMOUT:
	case DDP_SCENARIO_PRIMARY_OVL02L_RSZ0_MEMOUT:

	case DDP_SCENARIO_PRIMARY_OVL0_RSZ0_DISP_LEFT:
	case DDP_SCENARIO_PRIMARY_CCORR0_RSZ0_DISP_LEFT:

	case DDP_SCENARIO_PRIMARY_OVL0_RSZ0_1TO2_LEFT:
	case DDP_SCENARIO_PRIMARY_CCORR0_RSZ0_1TO2_LEFT:

	case DDP_SCENARIO_PRIMARY_RDMA0_CCORR0_RSZ0_DISP_LEFT:

	case DDP_SCENARIO_PRIMARY_OVL0_RSZ0_MEMOUT_LEFT:
		idx = 0;
		break;
	case DDP_SCENARIO_SUB_DISP:
	case DDP_SCENARIO_SUB_RDMA1_DISP:
	case DDP_SCENARIO_SUB_OVL_MEMOUT:
	case DDP_SCENARIO_SUB_ALL:
		idx = 1;
		break;
	case DDP_SCENARIO_PRIMARY_OVL_MEMOUT:
		idx = 3;
		break;
	default:
		DISP_LOG_E("%s: unknown scenario %d\n", __func__, scenario);
	}
	memcpy(irq_events, ddp_irq_event_list[idx], sizeof(ddp_irq_event_list[idx]));
	return 0;
}

#if 0  /* defined but not used */
static int acquire_free_bit(unsigned int total)
{
	int free_id = 0;

	while (total) {
		if (total & 0x1)
			return free_id;

		total >>= 1;
		++free_id;
	}
	return -1;
}
#endif

static int acquire_mutex(enum DDP_SCENARIO_ENUM scenario)
{
	/* primay use mutex 0 */
	int mutex_id = 0;
	struct DDP_MANAGER_CONTEXT *content = _get_context();
	int mutex_idx_free = content->mutex_idx;

	ASSERT(scenario >= 0 && scenario < DDP_SCENARIO_MAX);
	while (mutex_idx_free) {
		if (mutex_idx_free & 0x1) {
			content->mutex_idx &= (~(0x1 << mutex_id));
			mutex_id += DISP_MUTEX_DDP_FIRST;
			break;
		}
		mutex_idx_free >>= 1;
		++mutex_id;
	}
	ASSERT(mutex_id < (DISP_MUTEX_DDP_FIRST + DISP_MUTEX_DDP_COUNT));
	DDPDBG("scenario %s acquire mutex %d, left mutex 0x%x!\n",
	       ddp_get_scenario_name(scenario), mutex_id, content->mutex_idx);

	return mutex_id;
}

static int release_mutex(int mutex_idx)
{
	struct DDP_MANAGER_CONTEXT *content = _get_context();

	ASSERT(mutex_idx < (DISP_MUTEX_DDP_FIRST + DISP_MUTEX_DDP_COUNT));
	content->mutex_idx |= 1 << (mutex_idx - DISP_MUTEX_DDP_FIRST);
	DDPDBG("release mutex %d, left mutex 0x%x!\n",
	       mutex_idx, content->mutex_idx);
	return 0;
}

int dpmgr_path_set_video_mode(disp_path_handle dp_handle, int is_vdo_mode)
{
	struct ddp_path_handle *handle = NULL;

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;
	handle->mode = is_vdo_mode ? DDP_VIDEO_MODE : DDP_CMD_MODE;
	DDPDBG("set scenario %s mode: %s\n",
	       ddp_get_scenario_name(handle->scenario),
	       is_vdo_mode ? "Video_Mode" : "Cmd_Mode");
	return 0;
}

disp_path_handle dpmgr_create_path(enum DDP_SCENARIO_ENUM scenario, struct cmdqRecStruct *cmdq_handle)
{
	int i = 0;
	int module_name;
	struct ddp_path_handle *path_handle = NULL;
	int *modules = ddp_get_scenario_list(scenario);
	int module_num = ddp_get_module_num(scenario);
	struct DDP_MANAGER_CONTEXT *content = _get_context();

	path_handle = kzalloc(sizeof(*path_handle), GFP_KERNEL);
	if (path_handle != NULL) {
		path_handle->cmdqhandle = cmdq_handle;
		path_handle->scenario = scenario;
		path_handle->hwmutexid = acquire_mutex(scenario);
		path_handle->last_config.path_handle = path_handle;
		assign_default_irqs_table(scenario, path_handle->irq_event_map);
		DDPDBG("create handle %p on scenario %s\n", path_handle,
			   ddp_get_scenario_name(scenario));
		for (i = 0; i < module_num; i++) {
			module_name = modules[i];
			DDPDBG(" scenario %s include module %s\n",
				   ddp_get_scenario_name(scenario),
				   ddp_get_module_name(module_name));
			content->module_usage_table[module_name]++;
			content->module_path_table[module_name] = path_handle;
		}
		content->handle_cnt++;
		content->handle_pool[path_handle->hwmutexid] = path_handle;
	} else {
		DISP_LOG_E("Fail to create handle on scenario %s\n",
			   ddp_get_scenario_name(scenario));
	}
	return path_handle;
}

int dpmgr_get_scenario(disp_path_handle dp_handle)
{
	struct ddp_path_handle *handle = (struct ddp_path_handle *)dp_handle;

	return handle->scenario;
}

static int _dpmgr_path_connect(enum DDP_SCENARIO_ENUM scenario, void *handle)
{
	int i = 0, k = 0, module = 0;
	int *modules = NULL;
	int module_num = 0;

	enum DDP_SCENARIO_ENUM scn[2] = { DDP_SCENARIO_MAX, DDP_SCENARIO_MAX };
	bool has_dual_path = false;

	scn[0] = scenario;
	has_dual_path = ddp_path_is_dual(scn[0]);
	if (has_dual_path)
		scn[1] = ddp_get_dual_module(scn[0]);

	for (k = 0; k <= has_dual_path; k++) {
		ddp_connect_path(scn[k], handle);

		modules = ddp_get_scenario_list(scn[k]);
		module_num = ddp_get_module_num(scn[k]);

		for (i = 0; i < module_num; i++) {
			module = modules[i];
			if (ddp_modules_driver[module] && ddp_modules_driver[module]->connect) {
				int prev = i == 0 ? DISP_MODULE_UNKNOWN : modules[i - 1];
				int next = i == module_num - 1 ? DISP_MODULE_UNKNOWN : modules[i + 1];

				ddp_modules_driver[module]->connect(module, prev, next, 1, handle);
			}
		}
	}

	return 0;
}

static int _dpmgr_path_disconnect(enum DDP_SCENARIO_ENUM scenario, void *handle)
{
	int i = 0, k = 0, module = 0;
	int *modules = NULL;
	int module_num = 0;

	enum DDP_SCENARIO_ENUM scn[2] = { DDP_SCENARIO_MAX, DDP_SCENARIO_MAX };
	bool has_dual_path = false;

	scn[0] = scenario;
	has_dual_path = ddp_path_is_dual(scn[0]);
	if (has_dual_path)
		scn[1] = ddp_get_dual_module(scn[0]);

	for (k = 0; k <= has_dual_path; k++) {
		ddp_disconnect_path(scn[k], handle);

		modules = ddp_get_scenario_list(scn[k]);
		module_num = ddp_get_module_num(scn[k]);

		for (i = 0; i < module_num; i++) {
			module = modules[i];
			if (ddp_modules_driver[module] && ddp_modules_driver[module]->connect) {
				int prev = i == 0 ? DISP_MODULE_UNKNOWN : modules[i - 1];
				int next = i == module_num - 1 ? DISP_MODULE_UNKNOWN : modules[i + 1];

				ddp_modules_driver[module]->connect(module, prev, next, 0, handle);
			}
		}
	}

	return 0;
}

int dpmgr_modify_path_power_on_new_modules(disp_path_handle dp_handle,
					   enum DDP_SCENARIO_ENUM new_scenario, int sw_only)
{
	int i = 0, k = 0;
	int module_name = 0;
	struct DDP_MANAGER_CONTEXT *content = _get_context();
	struct ddp_path_handle *handle = (struct ddp_path_handle *)dp_handle;

	int *new_modules = NULL;
	int new_module_num = 0;

	enum DDP_SCENARIO_ENUM scn[2] = { DDP_SCENARIO_MAX, DDP_SCENARIO_MAX };
	bool has_dual_path = false;

	scn[0] = new_scenario;
	has_dual_path = ddp_path_is_dual(scn[0]);
	if (has_dual_path)
		scn[1] = ddp_get_dual_module(scn[0]);

	for (k = 0; k <= has_dual_path; k++) {
		new_modules = ddp_get_scenario_list(scn[k]);
		new_module_num = ddp_get_module_num(scn[k]);

		for (i = 0; i < new_module_num; i++) {
			module_name = new_modules[i];
			if (content->module_usage_table[module_name] == 0) { /* new module's count = 0 */
				content->module_usage_table[module_name]++;
				content->module_path_table[module_name] = handle;
				if (!sw_only)
					module_power_on(module_name);
			}
		}
	}
	return 0;
}

/**
 * NOTES: modify path should call API like this :
 *   old_scenario = dpmgr_get_scenario(handle);
 *   dpmgr_modify_path_power_on_new_modules();
 *   dpmgr_modify_path();
 *
 * after cmdq handle exec done:
 *   dpmgr_modify_path_power_off_old_modules();
 */
int dpmgr_modify_path(disp_path_handle dp_handle, enum DDP_SCENARIO_ENUM new_scenario,
			struct cmdqRecStruct *cmdq_handle, enum DDP_MODE mode, int sw_only)
{
	struct ddp_path_handle *handle;
	enum DDP_SCENARIO_ENUM old_scenario;

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;
	old_scenario = handle->scenario;
	handle->cmdqhandle = cmdq_handle;
	handle->scenario = new_scenario;
	DISP_LOG_I("modify handle %p from %s to %s\n", handle, ddp_get_scenario_name(old_scenario),
		   ddp_get_scenario_name(new_scenario));

	if (!sw_only) {
		/* mutex set will clear old settings */
		ddp_mutex_set(handle->hwmutexid, new_scenario, mode, cmdq_handle);

		ddp_mutex_Interrupt_enable(handle->hwmutexid, cmdq_handle);
		/* disconnect old path first */
		_dpmgr_path_disconnect(old_scenario, cmdq_handle);

		/* connect new path */
		_dpmgr_path_connect(new_scenario, cmdq_handle);
	}
	return 0;
}

int dpmgr_modify_path_start_new_modules(enum DDP_SCENARIO_ENUM old_scenario,
					enum DDP_SCENARIO_ENUM new_scenario,
					struct cmdqRecStruct *qhandle, int sw_only)
{
	int i = 0, k = 0;
	int m = 0; /* module_name */
	enum DDP_SCENARIO_ENUM scn[2] = { DDP_SCENARIO_MAX, DDP_SCENARIO_MAX };
	bool has_dual_path = false;

	int *new_m = NULL;
	int new_m_num = 0;

	scn[0] = new_scenario;
	has_dual_path = ddp_path_is_dual(scn[0]);
	if (has_dual_path)
		scn[1] = ddp_get_dual_module(scn[0]);

	for (k = 0; k <= has_dual_path; k++) {
		new_m = ddp_get_scenario_list(scn[k]);
		new_m_num = ddp_get_module_num(scn[k]);

		for (i = 0; i < new_m_num; i++) {
			m = new_m[i];
			if (ddp_is_module_in_scenario(old_scenario, m))
				continue;

			if (ddp_modules_driver[m] && ddp_modules_driver[m]->start) {
				if (!sw_only)
					ddp_modules_driver[m]->start(m, qhandle);

				DDPDBG("start new module %s\n", ddp_get_module_name(m));
			}
		}
	}

	return 0;
}

int dpmgr_modify_path_power_off_old_modules(enum DDP_SCENARIO_ENUM old_scenario,
					    enum DDP_SCENARIO_ENUM new_scenario, int sw_only)
{
	int i = 0, k = 0;
	int module_name = 0;

	struct DDP_MANAGER_CONTEXT *content = _get_context();

	int *old_modules = NULL;
	int old_module_num = 0;

	enum DDP_SCENARIO_ENUM scn[2] = { DDP_SCENARIO_MAX, DDP_SCENARIO_MAX };
	bool has_dual_path = false;

	scn[0] = old_scenario;
	has_dual_path = ddp_path_is_dual(scn[0]);
	if (has_dual_path)
		scn[1] = ddp_get_dual_module(scn[0]);

	for (k = 0; k <= has_dual_path; k++) {
		old_modules = ddp_get_scenario_list(scn[k]);
		old_module_num = ddp_get_module_num(scn[k]);

		for (i = 0; i < old_module_num; i++) {
			module_name = old_modules[i];
			if (!ddp_is_module_in_scenario(new_scenario, module_name)) {
				content->module_usage_table[module_name]--;
				content->module_path_table[module_name] = NULL;
				if (!sw_only)
					module_power_off(module_name);
			}
		}
	}

	return 0;
}

int dpmgr_destroy_path_handle(disp_path_handle dp_handle)
{
	int i = 0;
	int module_name;
	struct ddp_path_handle *handle;
	int *modules;
	int module_num;
	struct DDP_MANAGER_CONTEXT *content;

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;
	modules = ddp_get_scenario_list(handle->scenario);
	module_num = ddp_get_module_num(handle->scenario);
	content = _get_context();

	DDPDBG("destroy path handle %p on scenario %s\n", handle,
		   ddp_get_scenario_name(handle->scenario));
	if (handle != NULL) {
		release_mutex(handle->hwmutexid);
		for (i = 0; i < module_num; i++) {
			module_name = modules[i];
			content->module_usage_table[module_name]--;
			content->module_path_table[module_name] = NULL;
		}
		content->handle_cnt--;
		ASSERT(content->handle_cnt >= 0);
		content->handle_pool[handle->hwmutexid] = NULL;
		kfree(handle);
	}
	return 0;
}

int dpmgr_destroy_path(disp_path_handle dp_handle, struct cmdqRecStruct *cmdq_handle)
{
	struct ddp_path_handle *handle = (struct ddp_path_handle *)dp_handle;

	if (handle)
		_dpmgr_path_disconnect(handle->scenario, cmdq_handle);

	dpmgr_destroy_path_handle(dp_handle);
	return 0;
}

int dpmgr_path_memout_clock(disp_path_handle dp_handle, int clock_switch)
{
#if 0
	ASSERT(dp_handle != NULL);
	struct ddp_path_handle *handle = (struct ddp_path_handle *)dp_handle;

	handle->mem_module =
	    ddp_is_scenario_on_primary(handle->scenario) ? DISP_MODULE_WDMA0 : DISP_MODULE_WDMA1;
	if (handle->mem_module == DISP_MODULE_WDMA0 || handle->mem_module == DISP_MODULE_WDMA1) {
		if (ddp_modules_driver[handle->mem_module] != 0) {
			if (clock_switch) {
				if (ddp_modules_driver[handle->mem_module]->power_on != 0)
					ddp_modules_driver[handle->mem_module]->power_on(handle->
											 mem_module,
											 NULL);

			} else {
				if (ddp_modules_driver[handle->mem_module]->power_off != 0)
					ddp_modules_driver[handle->mem_module]->power_off(handle->
											  mem_module,
											  NULL);

				handle->mem_module = DISP_MODULE_UNKNOWN;
			}
		}
		return 0;
	}
	return -1;
#endif
	return 0;
}

int dpmgr_path_add_memout(disp_path_handle dp_handle, enum DISP_MODULE_ENUM engine, void *cmdq_handle)
{
	struct ddp_path_handle *handle;
	enum DISP_MODULE_ENUM wdma;
	struct DDP_MANAGER_CONTEXT *context;

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;
	ASSERT(handle->scenario == DDP_SCENARIO_PRIMARY_DISP ||
	       handle->scenario == DDP_SCENARIO_SUB_DISP ||
	       handle->scenario == DDP_SCENARIO_PRIMARY_OVL_MEMOUT ||
	       handle->scenario == DDP_SCENARIO_PRIMARY_DISP_LEFT ||
	       handle->scenario == DDP_SCENARIO_PRIMARY_OVL0_RSZ0_DISP ||
	       handle->scenario == DDP_SCENARIO_PRIMARY_OVL02L_RSZ0_DISP ||
	       handle->scenario == DDP_SCENARIO_PRIMARY_CCORR0_RSZ0_DISP ||
	       handle->scenario == DDP_SCENARIO_PRIMARY_OVL0_RSZ0_DISP_LEFT ||
	       handle->scenario == DDP_SCENARIO_PRIMARY_CCORR0_RSZ0_DISP_LEFT);
	wdma = ddp_is_scenario_on_primary(handle->scenario) ? DISP_MODULE_WDMA0 : DISP_MODULE_WDMA1;

	if (ddp_is_module_in_scenario(handle->scenario, wdma) == 1) {
		DDPERR("dpmgr_path_add_memout error, wdma is already in scenario=%s\n",
		       ddp_get_scenario_name(handle->scenario));
		return -1;
	}
	/* update contxt */
	context = _get_context();
	context->module_usage_table[wdma]++;
	context->module_path_table[wdma] = handle;
	if (handle->scenario == DDP_SCENARIO_PRIMARY_DISP_LEFT) {
		context->module_usage_table[DISP_MODULE_WDMA1]++;
		context->module_path_table[DISP_MODULE_WDMA1] = handle;
	}

	if (engine == DISP_MODULE_OVL0) {
		if (disp_helper_get_option(DISP_OPT_RSZ)) {
			handle->scenario = primary_get_OVL1to2_scenario(dpmgr_path_get_last_config_notclear(dp_handle));

			disp_rsz_print_hrt_info(dpmgr_path_get_last_config_notclear(dp_handle), __func__);
		} else {
			if (handle->scenario == DDP_SCENARIO_PRIMARY_DISP_LEFT)
				handle->scenario = DDP_SCENARIO_PRIMARY_ALL_LEFT;
			else
				handle->scenario = DDP_SCENARIO_PRIMARY_ALL;
		}
	} else if (engine == DISP_MODULE_OVL1) {
		handle->scenario = DDP_SCENARIO_SUB_ALL;
	} else if (engine == DISP_MODULE_DITHER0) {
		handle->scenario = DDP_SCENARIO_DITHER_1TO2;
	} else if (engine == DISP_MODULE_UFOE) {
		handle->scenario = DDP_SCENARIO_UFOE_1TO2;
	} else {
		pr_err("%s error: engine=%d\n", __func__, engine);
		ASSERT(0);
		return 0;
	}
	/* update connected */
	_dpmgr_path_connect(handle->scenario, cmdq_handle);
	ddp_mutex_set(handle->hwmutexid, handle->scenario, handle->mode, cmdq_handle);

	/* wdma just need start. */
	if (ddp_modules_driver[wdma] != 0) {
		if (ddp_modules_driver[wdma]->init != 0)
			ddp_modules_driver[wdma]->init(wdma, cmdq_handle);

		if (ddp_modules_driver[wdma]->start != 0)
			ddp_modules_driver[wdma]->start(wdma, cmdq_handle);
	}

	if (disp_helper_get_option(DISP_OPT_RSZ)) {
		if (ddp_path_is_dual(handle->scenario)) {
			if (ddp_modules_driver[DISP_MODULE_WDMA1]->init != 0)
				ddp_modules_driver[DISP_MODULE_WDMA1]->init(DISP_MODULE_WDMA1, cmdq_handle);

			if (ddp_modules_driver[DISP_MODULE_WDMA1]->start != 0)
				ddp_modules_driver[DISP_MODULE_WDMA1]->start(DISP_MODULE_WDMA1, cmdq_handle);
		}
	} else {
		if (handle->scenario == DDP_SCENARIO_PRIMARY_ALL_LEFT) {
			if (ddp_modules_driver[DISP_MODULE_WDMA1]->init != 0)
				ddp_modules_driver[DISP_MODULE_WDMA1]->init(DISP_MODULE_WDMA1, cmdq_handle);

			if (ddp_modules_driver[DISP_MODULE_WDMA1]->start != 0)
				ddp_modules_driver[DISP_MODULE_WDMA1]->start(DISP_MODULE_WDMA1, cmdq_handle);
		}
	}

	return 0;
}

int dpmgr_path_remove_memout(disp_path_handle dp_handle, void *cmdq_handle)
{
	enum DDP_SCENARIO_ENUM dis_scn;
	enum DDP_SCENARIO_ENUM new_scn;
	struct ddp_path_handle *handle;
	enum DISP_MODULE_ENUM wdma;
	struct DDP_MANAGER_CONTEXT *context;

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;
	ASSERT(handle->scenario == DDP_SCENARIO_PRIMARY_DISP ||
	       handle->scenario == DDP_SCENARIO_PRIMARY_ALL ||
	       handle->scenario == DDP_SCENARIO_DITHER_1TO2 ||
	       handle->scenario == DDP_SCENARIO_UFOE_1TO2 ||
	       handle->scenario == DDP_SCENARIO_SUB_DISP ||
	       handle->scenario == DDP_SCENARIO_SUB_ALL);
	wdma = ddp_is_scenario_on_primary(handle->scenario) ? DISP_MODULE_WDMA0 : DISP_MODULE_WDMA1;

	if (ddp_is_module_in_scenario(handle->scenario, wdma) == 0) {
		DDPERR("dpmgr_path_remove_memout error, wdma is not in scenario=%s\n",
		       ddp_get_scenario_name(handle->scenario));
		return -1;
	}
	/* update contxt */
	context = _get_context();
	context->module_usage_table[wdma]--;
	context->module_path_table[wdma] = 0;
	/* wdma just need stop */
	if (ddp_modules_driver[wdma] != 0) {
		if (ddp_modules_driver[wdma]->stop != 0)
			ddp_modules_driver[wdma]->stop(wdma, cmdq_handle);

		if (ddp_modules_driver[wdma]->deinit != 0)
			ddp_modules_driver[wdma]->deinit(wdma, cmdq_handle);
	}
	if (handle->scenario == DDP_SCENARIO_PRIMARY_ALL) {
		dis_scn = DDP_SCENARIO_PRIMARY_OVL_MEMOUT;
		new_scn = DDP_SCENARIO_PRIMARY_DISP;
	} else if (handle->scenario == DDP_SCENARIO_DITHER_1TO2) {
		dis_scn = DDP_SCENARIO_PRIMARY_DITHER_MEMOUT;
		new_scn = DDP_SCENARIO_PRIMARY_DISP;
	} else if (handle->scenario == DDP_SCENARIO_UFOE_1TO2) {
		dis_scn = DDP_SCENARIO_PRIMARY_UFOE_MEMOUT;
		new_scn = DDP_SCENARIO_PRIMARY_DISP;
	} else if (handle->scenario == DDP_SCENARIO_SUB_ALL) {
		dis_scn = DDP_SCENARIO_SUB_OVL_MEMOUT;
		new_scn = DDP_SCENARIO_SUB_DISP;
	} else {
		pr_err("%s: error scenario =%d\n", __func__, handle->scenario);
		ASSERT(0);
		return 0;
	}
	_dpmgr_path_disconnect(dis_scn, cmdq_handle);
	handle->scenario = new_scn;
	/* update connected */
	_dpmgr_path_connect(handle->scenario, cmdq_handle);
	ddp_mutex_set(handle->hwmutexid, handle->scenario, handle->mode, cmdq_handle);

	return 0;
}

int dpmgr_insert_ovl1_sub(disp_path_handle dp_handle, void *cmdq_handle)
{
	int ret = 0;
	struct ddp_path_handle *handle = (struct ddp_path_handle *)dp_handle;
	struct DDP_MANAGER_CONTEXT *context;

	ASSERT(dp_handle != NULL);

	if ((handle->scenario != DDP_SCENARIO_SUB_RDMA1_DISP) && (handle->scenario != DDP_SCENARIO_SUB_DISP)) {
		DDPERR("dpmgr_insert_ovl1_sub error, handle->scenario=%s\n",
			ddp_get_scenario_name(handle->scenario));
		return -1;
	}

	if (ddp_is_module_in_scenario(handle->scenario, DISP_MODULE_OVL1) == 1) {
		DISP_LOG_I("dpmgr_insert_ovl1_sub, OVL1 already in the path\n");
		return -1;
	}

	DDPDBG("dpmgr_insert_ovl1_sub\n");
	/* update contxt*/
	context = _get_context();
	context->module_usage_table[DISP_MODULE_OVL1]++;
	context->module_path_table[DISP_MODULE_OVL1] = handle;

	/* update connected*/
	ddp_connect_path(DDP_SCENARIO_SUB_DISP, cmdq_handle);
	ret = ddp_mutex_set(handle->hwmutexid, DDP_SCENARIO_SUB_DISP, handle->mode, cmdq_handle);

	handle->scenario = DDP_SCENARIO_SUB_DISP;

	/*ovl1 just need start.*/
	if (ddp_modules_driver[DISP_MODULE_OVL1] != 0) {
		if (ddp_modules_driver[DISP_MODULE_OVL1]->init != 0)
			ddp_modules_driver[DISP_MODULE_OVL1]->init(DISP_MODULE_OVL1, cmdq_handle);

		if (ddp_modules_driver[DISP_MODULE_OVL1]->start != 0)
			ddp_modules_driver[DISP_MODULE_OVL1]->start(DISP_MODULE_OVL1, cmdq_handle);
	}

	return ret;
}

int dpmgr_remove_ovl1_sub(disp_path_handle dp_handle, void *cmdq_handle)
{
	int ret = 0;
	struct ddp_path_handle *handle;
	struct DDP_MANAGER_CONTEXT *context;

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;
	if ((handle->scenario != DDP_SCENARIO_SUB_RDMA1_DISP) && (handle->scenario != DDP_SCENARIO_SUB_DISP)) {
		DDPERR("dpmgr_remove_ovl1_sub error, handle->scenario=%s\n", ddp_get_scenario_name(handle->scenario));
		return -1;
	}

	if (ddp_is_module_in_scenario(handle->scenario, DISP_MODULE_OVL1) == 0) {
		DISP_LOG_I("dpmgr_remove_ovl1_sub, OVL1 already not in the path\n");
		return -1;
	}

	DDPDBG("dpmgr_remove_ovl1_sub\n");
	/* update contxt */
	context = _get_context();
	context->module_usage_table[DISP_MODULE_OVL1]--;
	context->module_path_table[DISP_MODULE_OVL1] = 0;

	ddp_disconnect_path(handle->scenario, cmdq_handle);
	ddp_connect_path(DDP_SCENARIO_SUB_RDMA1_DISP, cmdq_handle);
	ret = ddp_mutex_set(handle->hwmutexid, DDP_SCENARIO_SUB_RDMA1_DISP, handle->mode, cmdq_handle);

	handle->scenario = DDP_SCENARIO_SUB_RDMA1_DISP;

	/*ovl1 just need stop */
	if (ddp_modules_driver[DISP_MODULE_OVL1] != 0) {
		if (ddp_modules_driver[DISP_MODULE_OVL1]->stop != 0)
			ddp_modules_driver[DISP_MODULE_OVL1]->stop(DISP_MODULE_OVL1, cmdq_handle);

		if (ddp_modules_driver[DISP_MODULE_OVL1]->deinit != 0)
			ddp_modules_driver[DISP_MODULE_OVL1]->deinit(DISP_MODULE_OVL1, cmdq_handle);
	}

	return ret;
}

int dpmgr_path_set_dst_module(disp_path_handle dp_handle, enum DISP_MODULE_ENUM dst_module)
{
	struct ddp_path_handle *handle;

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;
	ASSERT((handle->scenario >= 0 && handle->scenario < DDP_SCENARIO_MAX));
	DDPDBG("set dst module on scenario %s, module %s\n",
		   ddp_get_scenario_name(handle->scenario), ddp_get_module_name(dst_module));

	return ddp_set_dst_module(handle->scenario, dst_module);
}

int dpmgr_path_get_mutex(disp_path_handle dp_handle)
{
	struct ddp_path_handle *handle = NULL;

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;
	return handle->hwmutexid;
}

enum DISP_MODULE_ENUM dpmgr_path_get_dst_module(disp_path_handle dp_handle)
{
	struct ddp_path_handle *handle;
	enum DISP_MODULE_ENUM dst_module;

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;
	ASSERT((handle->scenario >= 0 && handle->scenario < DDP_SCENARIO_MAX));
	dst_module = ddp_get_dst_module(handle->scenario);

	DISP_LOG_V("get dst module on scenario %s, module %s\n",
		   ddp_get_scenario_name(handle->scenario), ddp_get_module_name(dst_module));
	return dst_module;
}

enum dst_module_type dpmgr_path_get_dst_module_type(disp_path_handle dp_handle)
{
	enum DISP_MODULE_ENUM dst_module = dpmgr_path_get_dst_module(dp_handle);

	if (dst_module == DISP_MODULE_WDMA0 || dst_module == DISP_MODULE_WDMA1)
		return DST_MOD_WDMA;
	else
		return DST_MOD_REAL_TIME;
}

int dpmgr_path_connect(disp_path_handle dp_handle, int encmdq)
{
	struct ddp_path_handle *handle;
	struct cmdqRecStruct *cmdqHandle;

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;
	cmdqHandle = encmdq ? handle->cmdqhandle : NULL;

	_dpmgr_path_connect(handle->scenario, cmdqHandle);

	ddp_mutex_set(handle->hwmutexid, handle->scenario, handle->mode, cmdqHandle);
	ddp_mutex_Interrupt_enable(handle->hwmutexid, cmdqHandle);
	return 0;
}

int dpmgr_path_disconnect(disp_path_handle dp_handle, int encmdq)
{
	struct ddp_path_handle *handle;
	struct cmdqRecStruct *cmdqHandle;

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;
	cmdqHandle = encmdq ? handle->cmdqhandle : NULL;

	DDPDBG("dpmgr_path_disconnect on scenario %s\n",
		   ddp_get_scenario_name(handle->scenario));
	ddp_mutex_clear(handle->hwmutexid, cmdqHandle);
	ddp_mutex_Interrupt_disable(handle->hwmutexid, cmdqHandle);
	_dpmgr_path_disconnect(handle->scenario, cmdqHandle);
	return 0;
}

int dpmgr_path_init(disp_path_handle dp_handle, int encmdq)
{
	int i = 0;
	int module_name;
	struct ddp_path_handle *handle;
	int *modules;
	int module_num;
	struct cmdqRecStruct *cmdqHandle;

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;
	modules = ddp_get_scenario_list(handle->scenario);
	module_num = ddp_get_module_num(handle->scenario);
	cmdqHandle = encmdq ? handle->cmdqhandle : NULL;

	DDPDBG("path init on scenario %s\n", ddp_get_scenario_name(handle->scenario));
	/* open top clock */
	path_top_clock_on();
	/* seting mutex */
	ddp_mutex_set(handle->hwmutexid, handle->scenario, handle->mode, cmdqHandle);
	ddp_mutex_Interrupt_enable(handle->hwmutexid, cmdqHandle);
	/* connect path; */
	_dpmgr_path_connect(handle->scenario, cmdqHandle);

	/* each module init */
	for (i = 0; i < module_num; i++) {
		module_name = modules[i];
		if (ddp_modules_driver[module_name] != 0) {
			if (ddp_modules_driver[module_name]->init != 0)
				ddp_modules_driver[module_name]->init(module_name, cmdqHandle);

			if (ddp_modules_driver[module_name]->set_listener != 0)
				ddp_modules_driver[module_name]->set_listener(module_name,
									      dpmgr_module_notify);
		}
	}

	/* after init this path will power on; */
	handle->power_state = 1;
	return 0;
}

int dpmgr_path_deinit(disp_path_handle dp_handle, int encmdq)
{
	int i = 0;
	int module_name;
	int *modules;
	int module_num;
	struct cmdqRecStruct *cmdqHandle;
	struct ddp_path_handle *handle;

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;
	modules = ddp_get_scenario_list(handle->scenario);
	module_num = ddp_get_module_num(handle->scenario);
	cmdqHandle = encmdq ? handle->cmdqhandle : NULL;

	DDPDBG("path deinit on scenario %s\n", ddp_get_scenario_name(handle->scenario));
	ddp_mutex_Interrupt_disable(handle->hwmutexid, cmdqHandle);
	ddp_mutex_clear(handle->hwmutexid, cmdqHandle);
	_dpmgr_path_disconnect(handle->scenario, cmdqHandle);
	for (i = 0; i < module_num; i++) {
		module_name = modules[i];
		if (ddp_modules_driver[module_name] != 0) {
			if (ddp_modules_driver[module_name]->deinit != 0)
				ddp_modules_driver[module_name]->deinit(module_name, cmdqHandle);

			if (ddp_modules_driver[module_name]->set_listener != 0)
				ddp_modules_driver[module_name]->set_listener(module_name, NULL);
		}
	}
	handle->power_state = 0;
	/* close top clock when last path init */
	path_top_clock_off();
	return 0;
}

int dpmgr_path_start(disp_path_handle dp_handle, int encmdq)
{
	int i = 0, k = 0;
	int module_name;
	struct ddp_path_handle *handle;
	int *modules;
	int module_num, path_num;
	struct cmdqRecStruct *cmdqHandle;
	enum DDP_SCENARIO_ENUM scn[2] = { DDP_SCENARIO_MAX, DDP_SCENARIO_MAX };

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;
	cmdqHandle = encmdq ? handle->cmdqhandle : NULL;

	scn[0] = handle->scenario;
	path_num = 1;
	if (ddp_path_is_dual(handle->scenario)) {
		scn[1] = ddp_get_dual_module(scn[0]);
		path_num++;
	}

	DISP_LOG_I("path start on scenario %s\n", ddp_get_scenario_name(handle->scenario));
	for (k = 0; k < path_num; k++) {
		modules = ddp_get_scenario_list(scn[k]);
		module_num = ddp_get_module_num(scn[k]);

		for (i = 0; i < module_num; i++) {
			module_name = modules[i];
			if (ddp_modules_driver[module_name] != 0) {
				if (ddp_modules_driver[module_name]->start != 0)
					ddp_modules_driver[module_name]->start(module_name, cmdqHandle);
			}
		}
	}

	return 0;
}

int dpmgr_path_start_by_scenario(disp_path_handle dp_handle, int encmdq, enum DDP_SCENARIO_ENUM scenario,
									struct cmdqRecStruct *input_cmdq_handle)
{
	int i = 0;
	int module_name;
	struct ddp_path_handle *handle;
	int *modules;
	int module_num;
	struct cmdqRecStruct *cmdqHandle;

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;
	modules = ddp_get_scenario_list(scenario);
	module_num = ddp_get_module_num(scenario);
	if (input_cmdq_handle)
		cmdqHandle = encmdq ? handle->cmdqhandle : NULL;
	else
		cmdqHandle = input_cmdq_handle;

	DISP_LOG_I("path start on scenario %s\n", ddp_get_scenario_name(scenario));
	for (i = 0; i < module_num; i++) {
		module_name = modules[i];
		if (ddp_modules_driver[module_name] != 0) {
			if (ddp_modules_driver[module_name]->start != 0)
				ddp_modules_driver[module_name]->start(module_name, cmdqHandle);
		}
	}
	return 0;
}

int dpmgr_modify_path_start_new_module(disp_path_handle dp_handle, int encmdq, enum DDP_SCENARIO_ENUM old_scenario,
			enum DDP_SCENARIO_ENUM new_scenario, struct cmdqRecStruct *input_cmdq_handle)
{
	struct ddp_path_handle *handle;
	struct cmdqRecStruct *cmdqHandle;
	int *new_modules = ddp_get_scenario_list(new_scenario);
	int new_module_num = ddp_get_module_num(new_scenario);
	int i = 0, module_name = 0;

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;
	if (input_cmdq_handle)
		cmdqHandle = encmdq ? handle->cmdqhandle : NULL;
	else
		cmdqHandle = input_cmdq_handle;

	for (i = 0; i < new_module_num; i++) {
		module_name = new_modules[i];
		if (module_name == DISP_MODULE_DSI0 ||
		module_name == DISP_MODULE_DSI1 ||
		module_name == DISP_MODULE_DSIDUAL)
			continue;
		if (!ddp_is_module_in_scenario(old_scenario, module_name)) {
			if (ddp_modules_driver[module_name] != 0)
				if (ddp_modules_driver[module_name]->start != 0)
					ddp_modules_driver[module_name]->start(module_name, cmdqHandle);
		}
	}

	if (ddp_path_is_dual(new_scenario)) {
		enum DDP_SCENARIO_ENUM dual_scenario;

		dual_scenario = ddp_get_dual_module(new_scenario);
		new_modules = ddp_get_scenario_list(dual_scenario);
		new_module_num = ddp_get_module_num(dual_scenario);
		for (i = 0; i < new_module_num; i++) {
			module_name = new_modules[i];
			if (module_name == DISP_MODULE_DSI0 ||
				module_name == DISP_MODULE_DSI1 ||
				module_name == DISP_MODULE_DSIDUAL)
				continue;
			if (!ddp_is_module_in_scenario(old_scenario, module_name)) {
				if (ddp_modules_driver[module_name] != 0)
					if (ddp_modules_driver[module_name]->start != 0)
						ddp_modules_driver[module_name]->start(module_name, cmdqHandle);
			}
		}
	}

	return 0;
}

int dpmgr_path_stop(disp_path_handle dp_handle, int encmdq)
{
	int i = 0, k = 0;
	int module_name;
	struct ddp_path_handle *handle;
	int *modules;
	int module_num, path_num;
	struct cmdqRecStruct *cmdqHandle;
	enum DDP_SCENARIO_ENUM scn[2] = { DDP_SCENARIO_MAX, DDP_SCENARIO_MAX };

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;
	cmdqHandle = encmdq ? handle->cmdqhandle : NULL;

	scn[0] = handle->scenario;
	path_num = 1;
	if (ddp_path_is_dual(handle->scenario)) {
		scn[1] = ddp_get_dual_module(scn[0]);
		path_num++;
	}

	DISP_LOG_I("path stop on scenario %s\n", ddp_get_scenario_name(handle->scenario));
	for (k = 0; k < path_num; k++) {
		modules = ddp_get_scenario_list(scn[k]);
		module_num = ddp_get_module_num(scn[k]);

		for (i = module_num - 1; i >= 0; i--) {
			module_name = modules[i];
			if (ddp_modules_driver[module_name] != 0) {
				if (ddp_modules_driver[module_name]->stop != 0)
					ddp_modules_driver[module_name]->stop(module_name, cmdqHandle);
			}
		}
	}

	return 0;
}

int dpmgr_path_ioctl(disp_path_handle dp_handle, void *cmdq_handle,
		     enum DDP_IOCTL_NAME ioctl_cmd, void *params)
{
	int i = 0, k = 0;
	int ret = 0;
	int module_name;
	int *modules = NULL;
	int module_num = 0;
	struct ddp_path_handle *handle = NULL;

	enum DDP_SCENARIO_ENUM scn[2] = { DDP_SCENARIO_MAX, DDP_SCENARIO_MAX };
	bool has_dual_path = false;

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;

	scn[0] = handle->scenario;
	has_dual_path = ddp_path_is_dual(scn[0]);
	if (has_dual_path)
		scn[1] = ddp_get_dual_module(scn[0]);

	for (k = 0; k <= has_dual_path; k++) {
		modules = ddp_get_scenario_list(scn[k]);
		module_num = ddp_get_module_num(scn[k]);
		DDPDBG("path IOCTL on scenario %s\n", ddp_get_scenario_name(scn[k]));

		for (i = module_num - 1; i >= 0; i--) {
			module_name = modules[i];
			if (ddp_modules_driver[module_name] != 0) {
				if (ddp_modules_driver[module_name]->ioctl != 0) {
					ret += ddp_modules_driver[module_name]->ioctl(module_name, cmdq_handle,
										      ioctl_cmd, params);
				}
			}
		}
	}
	return ret;
}

int dpmgr_path_enable_irq(disp_path_handle dp_handle, void *cmdq_handle, enum DDP_IRQ_LEVEL irq_level)
{
	int i = 0, k = 0;
	int ret = 0;
	int module_name;
	struct ddp_path_handle *handle;
	int *modules;
	int module_num;
	enum DDP_SCENARIO_ENUM scn[2] = { DDP_SCENARIO_MAX, DDP_SCENARIO_MAX };
	bool has_dual_path = false;

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;
	modules = ddp_get_scenario_list(handle->scenario);
	module_num = ddp_get_module_num(handle->scenario);

	DDPDBG("path enable irq on scenario %s, level %d\n",
		   ddp_get_scenario_name(handle->scenario), irq_level);

	if (irq_level != DDP_IRQ_LEVEL_ALL)
		ddp_mutex_Interrupt_disable(handle->hwmutexid, cmdq_handle);
	else
		ddp_mutex_Interrupt_enable(handle->hwmutexid, cmdq_handle);

	scn[0] = handle->scenario;
	has_dual_path = ddp_path_is_dual(scn[0]);
	if (has_dual_path)
		scn[1] = ddp_get_dual_module(scn[0]);

	for (k = 0; k <= has_dual_path; k++) {
		modules = ddp_get_scenario_list(scn[k]);
		module_num = ddp_get_module_num(scn[k]);

		for (i = module_num - 1; i >= 0; i--) {
			module_name = modules[i];
			if (ddp_modules_driver[module_name] && ddp_modules_driver[module_name]->enable_irq) {
				DDPDBG("scenario %s, module %s enable irq level %d\n",
					   ddp_get_scenario_name(handle->scenario),
					   ddp_get_module_name(module_name), irq_level);
				ret += ddp_modules_driver[module_name]->enable_irq(module_name, cmdq_handle,
										   irq_level);
			}
		}
	}


	return ret;
}

int dpmgr_path_reset(disp_path_handle dp_handle, int encmdq)
{
	int i = 0, k = 0;
	int ret = 0;
	int error = 0;
	int module_name;
	int *modules;
	int module_num;
	struct ddp_path_handle *handle;
	struct cmdqRecStruct *cmdqHandle;

	enum DDP_SCENARIO_ENUM scn[2] = { DDP_SCENARIO_MAX, DDP_SCENARIO_MAX };
	bool has_dual_path = false;

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;

	scn[0] = handle->scenario;
	if (has_dual_path)
		scn[1] = ddp_path_is_dual(scn[0]);

	cmdqHandle = encmdq ? handle->cmdqhandle : NULL;

	DISP_LOG_I("path reset on scenario %s\n", ddp_get_scenario_name(handle->scenario));
	/* first reset mutex */
	ddp_mutex_reset(handle->hwmutexid, cmdqHandle);

	for (k = 0; k <= has_dual_path; k++) {
		modules = ddp_get_scenario_list(scn[k]);
		module_num = ddp_get_module_num(scn[k]);

		for (i = 0; i < module_num; i++) {
			module_name = modules[i];
			if (ddp_modules_driver[module_name] == 0)
				continue;

			if (ddp_modules_driver[module_name]->reset != 0) {
				ret = ddp_modules_driver[module_name]->reset(module_name, cmdqHandle);
				if (ret != 0)
					error++;
			}
		}
	}
	return error > 0 ? -1 : 0;
}

static unsigned int dpmgr_is_PQ(enum DISP_MODULE_ENUM module)
{
	unsigned int isPQ = 0;

	switch (module) {
	case DISP_MODULE_COLOR0:
	case DISP_MODULE_COLOR1:
	case DISP_MODULE_CCORR0:
	case DISP_MODULE_CCORR1:
	case DISP_MODULE_AAL0:
	case DISP_MODULE_AAL1:
	case DISP_MODULE_GAMMA0:
	case DISP_MODULE_GAMMA1:
	case DISP_MODULE_DITHER0:
	case DISP_MODULE_DITHER1:
		/* case DISP_MODULE_PWM0  : */
		isPQ = 1;
		break;
	default:
		isPQ = 0;
	}

	return isPQ;
}

static unsigned int dpmgr_scenario_is_SUB(enum DDP_SCENARIO_ENUM ddp_scenario)
{
	unsigned int isSUB = 0;

	switch (ddp_scenario) {
	case DDP_SCENARIO_SUB_DISP:
	case DDP_SCENARIO_SUB_RDMA1_DISP:
	case DDP_SCENARIO_SUB_OVL_MEMOUT:
	case DDP_SCENARIO_SUB_ALL:
		isSUB = 1;
		break;
	default:
		isSUB = 0;
	}

	return isSUB;
}

int dpmgr_path_update_partial_roi(disp_path_handle dp_handle,
		struct disp_rect partial, void *cmdq_handle)
{
	struct ddp_path_handle *handle;

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;

	if (ddp_path_is_dual(handle->scenario)) {
		partial.width /= 2;
		partial.is_dual = true;
	}

	return dpmgr_path_ioctl(dp_handle, cmdq_handle, DDP_PARTIAL_UPDATE,
			&partial);
}

struct tile_roi {
	u32 src_x;
	u32 src_y;
	u32 addr;
	enum UNIFIED_COLOR_FMT fmt;
	u32 pitch;
	u32 x;
	u32 y;
	u32 w;
	u32 h;
	u32 bg_w;
	u32 bg_h;
};

int dpmgr_path_split_layer_roi_to_pipe_roi(struct tile_roi *pipe_roi, struct tile_roi *layer_roi,
					   u32 left_bg_w, u32 right_bg_w, u32 is_left)
{
	u32 layer_dst_x1 = 0, layer_dst_x2 = 0;
	u32 bg_boundary_x = 0;

	layer_dst_x1 = layer_roi->x;
	layer_dst_x2 = layer_roi->x + layer_roi->w - 1;

	/* update pipe src_x, x, w, bg_w */
	if (is_left) {
		if (left_bg_w == 0) {
			DDPDBG("%s:left pipe:no bg ROI\n", __func__);
			return -EINVAL;
		}

		bg_boundary_x = left_bg_w - 1;
		pipe_roi->bg_w = left_bg_w;

		if (bg_boundary_x < layer_dst_x1) {
			pipe_roi->w = 0;
			pipe_roi->x = 0;
			pipe_roi->src_x = 0;
		} else {
			pipe_roi->x = layer_dst_x1;

			if (bg_boundary_x < layer_dst_x2)
				pipe_roi->w = bg_boundary_x - layer_dst_x1 + 1;
			else
				pipe_roi->w = layer_roi->w;

			pipe_roi->src_x = layer_roi->src_x;
		}
	} else if (!is_left) {
		if (right_bg_w == 0) {
			DDPDBG("%s:right pipe:no bg ROI\n", __func__);
			return -EINVAL;
		}

		bg_boundary_x = layer_roi->bg_w - right_bg_w;
		pipe_roi->bg_w = right_bg_w;

		if (layer_dst_x2 < bg_boundary_x) {
			pipe_roi->x = 0;
			pipe_roi->w = 0;
			pipe_roi->src_x = 0;
		} else {
			if (bg_boundary_x < layer_dst_x1) {
				pipe_roi->x = layer_dst_x1 - bg_boundary_x;
				pipe_roi->w = layer_roi->w;
				pipe_roi->src_x = layer_roi->src_x;
			} else {
				pipe_roi->x = 0;
				pipe_roi->w = layer_dst_x2 - bg_boundary_x + 1;

				if (layer_roi->addr)
					pipe_roi->src_x = layer_roi->src_x + (bg_boundary_x - layer_dst_x1);
			}
		}
	}
	pipe_roi->src_y = layer_roi->src_y;
	pipe_roi->addr = layer_roi->addr;
	pipe_roi->fmt = layer_roi->fmt;
	pipe_roi->pitch = layer_roi->pitch;
	pipe_roi->y = layer_roi->y;
	pipe_roi->h = layer_roi->h;
	pipe_roi->bg_h = layer_roi->bg_h;

#if 0
	DDPDBG("%s:pipe%d(boundary_x:%u):addr:0x%x,fmt:%s,pitch:%d,(%u,%u)->(%u,%u,%ux%u),bg(%ux%u)\n",
		__func__, is_left ? 1 : 2,  bg_boundary_x,
		pipe_roi->addr, unified_color_fmt_name(pipe_roi->fmt), pipe_roi->pitch,
		pipe_roi->src_x, pipe_roi->src_y,
		pipe_roi->x, pipe_roi->y, pipe_roi->w, pipe_roi->h,
		pipe_roi->bg_w, pipe_roi->bg_h);
#endif

	return 0;
}

int dpmgr_path_module_position_compare(enum DISP_MODULE_ENUM module_a,
				       enum DISP_MODULE_ENUM module_b,
				       disp_path_handle dp_handle)
{
	struct ddp_path_handle *phandle = NULL;
	int module_num = 0;
	int *modules = NULL;
	int i = 0;
	int found = -2, idx_a = -1, idx_b = -1;

	if (!dp_handle)
		return 0xff;

	phandle = (struct ddp_path_handle *)dp_handle;
	module_num = ddp_get_module_num(phandle->scenario);
	modules = ddp_get_scenario_list(phandle->scenario);
	for (i = 0; i < module_num; i++) {
		if (modules[i] == module_a) {
			idx_a = i;
			found++;
		}
		if (modules[i] == module_b) {
			idx_b = i;
			found++;
		}
	}

	if (!found)
		return idx_a - idx_b;

	return 0xff;
}

bool dpmgr_path_is_module_before_rsz(enum DISP_MODULE_ENUM module, disp_path_handle dp_handle)
{
	struct ddp_path_handle *phandle = NULL;
	int module_num = 0;
	int *modules = NULL;
	int i = 0;

	if (!dp_handle)
		return -EINVAL;

	phandle = (struct ddp_path_handle *)dp_handle;
	if (!phandle->last_config.rsz_enable)
		return false;

	module_num = ddp_get_module_num(phandle->scenario);
	modules = ddp_get_scenario_list(phandle->scenario);
	for (i = 0; i < module_num; i++) {
		if (modules[i] == module)
			return true;

		if (modules[i] == DISP_MODULE_RSZ0 || modules[i] == DISP_MODULE_RSZ1)
			return false;
	}

	return false;
}

int dpmgr_path_update_offset_before_rsz(struct disp_ddp_path_config *src_config,
					struct disp_ddp_path_config *dst_config)
{
	struct OVL_CONFIG_STRUCT *src_lconfig = NULL;
	struct OVL_CONFIG_STRUCT *dst_lconfig = NULL;
	struct RDMA_CONFIG_STRUCT *src_rdma_config = NULL;
	struct RDMA_CONFIG_STRUCT *dst_rdma_config = NULL;
	int i = 0;

	if (src_config->ovl_dirty) {
		for (i = 0; i < TOTAL_OVL_LAYER_NUM; i++) {
			src_lconfig = &src_config->ovl_config[i];
			dst_lconfig = &dst_config->ovl_config[i];

			if (!src_lconfig->layer_en)
				continue;

			if (src_lconfig->src_w < src_lconfig->dst_w &&
			    src_lconfig->src_h < src_lconfig->dst_h) {
				dst_lconfig->dst_x = src_lconfig->dst_x * 10000 / src_config->rsz_config.ratio;
				dst_lconfig->dst_y = src_lconfig->dst_y * 10000 / src_config->rsz_config.ratio;
			}
		}
	}

	if (src_config->rdma_dirty) {
		src_rdma_config = &src_config->rdma_config;
		dst_rdma_config = &dst_config->rdma_config;
		if (src_rdma_config->src_w < src_rdma_config->dst_w &&
		    src_rdma_config->src_h < src_rdma_config->dst_h) {
			dst_rdma_config->dst_x = src_rdma_config->dst_x * 10000 / src_config->rsz_config.ratio;
			dst_rdma_config->dst_y = src_rdma_config->dst_y * 10000 / src_config->rsz_config.ratio;
		}
	}

	return 0;
}

int dpmgr_path_convert_wdma_roi_to_layer(struct tile_roi *layer_roi,
					 const struct WDMA_CONFIG_STRUCT wdma_config,
					 const struct RSZ_CONFIG_STRUCT rsz_config,
					 const struct disp_ddp_path_config *pconfig)
{
	layer_roi->src_x = 0;
	layer_roi->src_y = 0;
	layer_roi->addr = wdma_config.dstAddress;
	layer_roi->fmt = wdma_config.outputFormat;
	layer_roi->pitch = wdma_config.dstPitch;
	layer_roi->x = 0;
	layer_roi->y = 0;
	layer_roi->w = wdma_config.srcWidth;
	layer_roi->h = wdma_config.srcHeight;

	if (wdma_config.srcWidth < pconfig->dst_w &&
	    wdma_config.srcHeight < pconfig->dst_h) {
		layer_roi->bg_w = rsz_config.frm_in_w;
		layer_roi->bg_h = rsz_config.frm_in_h;
	} else {
		layer_roi->bg_w = pconfig->dst_w;
		layer_roi->bg_h = pconfig->dst_h;
	}
	return 0;
}

int dpmgr_path_convert_pipe_roi_to_wdma(struct WDMA_CONFIG_STRUCT *wdma_config,
					const struct tile_roi pipe_roi)
{
	wdma_config->dstAddress = pipe_roi.addr +
			pipe_roi.src_x * UFMT_GET_Bpp(pipe_roi.fmt);
	wdma_config->outputFormat = pipe_roi.fmt;
	wdma_config->dstPitch = pipe_roi.pitch;
	wdma_config->srcWidth = pipe_roi.w;
	wdma_config->srcHeight = pipe_roi.h;
	wdma_config->clipX = 0;
	wdma_config->clipY = 0;
	wdma_config->clipWidth = pipe_roi.w;
	wdma_config->clipHeight = pipe_roi.h;

	return 0;
}

int dpmgr_path_convert_rdma_roi_to_layer(struct tile_roi *layer_roi,
					const struct RDMA_CONFIG_STRUCT rdma_config,
					const struct RSZ_CONFIG_STRUCT rsz_config,
					const struct disp_ddp_path_config *pconfig)
{
	layer_roi->src_x = rdma_config.src_x;
	layer_roi->src_y = rdma_config.src_y;
	layer_roi->addr = rdma_config.address;
	layer_roi->fmt = rdma_config.inputFormat;
	layer_roi->pitch = rdma_config.pitch;
	layer_roi->x = rdma_config.dst_x;
	layer_roi->y = rdma_config.dst_y;
	layer_roi->w = rdma_config.src_w;
	layer_roi->h = rdma_config.src_h;

	if (rdma_config.src_w < rdma_config.width &&
	    rdma_config.src_h < rdma_config.height) {
		layer_roi->bg_w = rsz_config.frm_in_w;
		layer_roi->bg_h = rsz_config.frm_in_h;
	} else {
		layer_roi->bg_w = rdma_config.dst_w;
		layer_roi->bg_h = rdma_config.dst_h;
	}

	return 0;
}

int dpmgr_path_convert_pipe_roi_to_rdma(struct RDMA_CONFIG_STRUCT *rdma_config,
					const struct tile_roi pipe_roi)
{
	rdma_config->src_x = pipe_roi.src_x;
	rdma_config->src_y = pipe_roi.src_y;
	rdma_config->address = pipe_roi.addr;
	rdma_config->inputFormat = pipe_roi.fmt;
	rdma_config->pitch = pipe_roi.pitch;
	rdma_config->dst_x = pipe_roi.x;
	rdma_config->dst_y = pipe_roi.y;
	rdma_config->width = pipe_roi.w;
	rdma_config->height = pipe_roi.h;
	rdma_config->dst_w = pipe_roi.bg_w;
	rdma_config->dst_h = pipe_roi.bg_h;

	return 0;
}

int dpmgr_path_convert_ovl_roi_to_layer(struct tile_roi *layer_roi,
					const struct OVL_CONFIG_STRUCT ovl_config,
					const struct RSZ_CONFIG_STRUCT rsz_config,
					const struct disp_ddp_path_config *pconfig)
{
	layer_roi->src_x = ovl_config.src_x;
	layer_roi->src_y = ovl_config.src_y;
	layer_roi->addr = ovl_config.addr;
	layer_roi->fmt = ovl_config.fmt;
	layer_roi->pitch = ovl_config.src_pitch;
	layer_roi->x = ovl_config.dst_x;
	layer_roi->y = ovl_config.dst_y;
	layer_roi->w = ovl_config.src_w;
	layer_roi->h = ovl_config.src_h;

	if (ovl_config.src_w < ovl_config.dst_w &&
	    ovl_config.src_h < ovl_config.dst_h) {
		layer_roi->bg_w = rsz_config.frm_in_w;
		layer_roi->bg_h = rsz_config.frm_in_h;
	} else {
		layer_roi->bg_w = pconfig->dst_w;
		layer_roi->bg_h = pconfig->dst_h;
	}

	return 0;
}

int dpmgr_path_convert_pipe_roi_to_ovl(struct OVL_CONFIG_STRUCT *lconfig,
				       const struct tile_roi pipe_roi)
{
	lconfig->src_x = pipe_roi.src_x;
	lconfig->src_y = pipe_roi.src_y;
	lconfig->addr = pipe_roi.addr;
	lconfig->fmt = pipe_roi.fmt;
	lconfig->src_pitch = pipe_roi.pitch;
	lconfig->dst_x = pipe_roi.x;
	lconfig->dst_y = pipe_roi.y;
	lconfig->dst_w = pipe_roi.w;
	lconfig->dst_h = pipe_roi.h;

	return 0;
}

enum DISP_MODULE_ENUM dpmgr_path_ovl_pma_swap(enum DISP_MODULE_ENUM from_ovl,
					      struct disp_ddp_path_config *pconfig,
					      int is_left)
{
	enum DISP_MODULE_ENUM swap_to_ovl = from_ovl;
	struct RSZ_CONFIG_STRUCT *rsz_config = &pconfig->rsz_config;
	int k = is_left;

	if (!pconfig->ovl_pma_enable)
		return swap_to_ovl;

	switch (from_ovl) {
	case DISP_MODULE_OVL0:
		swap_to_ovl = DISP_MODULE_OVL0_2L;
		pconfig->dst_w = rsz_config->tw[k].out_len;
		pconfig->dst_h = rsz_config->th.out_len;
		break;
	case DISP_MODULE_OVL1:
		swap_to_ovl = DISP_MODULE_OVL1_2L;
		pconfig->dst_w = rsz_config->tw[k].out_len;
		pconfig->dst_h = rsz_config->th.out_len;
		break;
	case DISP_MODULE_OVL0_2L:
		swap_to_ovl = DISP_MODULE_OVL0;
		pconfig->dst_w = rsz_config->tw[k].in_len;
		pconfig->dst_h = rsz_config->th.in_len;
		break;
	case DISP_MODULE_OVL1_2L:
		swap_to_ovl = DISP_MODULE_OVL1;
		pconfig->dst_w = rsz_config->tw[k].in_len;
		pconfig->dst_h = rsz_config->th.in_len;
		break;
	default:
		break;
	}

#if 0
	if (from_ovl == DISP_MODULE_OVL0 || from_ovl == DISP_MODULE_OVL0_2L ||
	    from_ovl == DISP_MODULE_OVL1 || from_ovl == DISP_MODULE_OVL1_2L)
		DDPMSG("%s:PMA:swap ovl:%s->%s\n",
		       __func__, ddp_get_module_name(from_ovl),
		       ddp_get_module_name(swap_to_ovl));
#endif

	return swap_to_ovl;
}


int dpmgr_path_set_dual_config(struct disp_ddp_path_config *src_config,
			       struct disp_ddp_path_config *dst_config, int is_left)
{
	int ret = 0;
	int i = 0;
	struct RDMA_CONFIG_STRUCT *s_rdma_config = NULL, *d_rdma_config = NULL;

	memcpy(dst_config, src_config, sizeof(struct disp_ddp_path_config));

	if (src_config->wdma_dirty) {
		struct WDMA_CONFIG_STRUCT *s_wconfig = &src_config->wdma_config;
		struct WDMA_CONFIG_STRUCT *d_wconfig = &dst_config->wdma_config;
		struct tile_roi layer_roi = { 0 };
		struct tile_roi pipe_roi = { 0 };
		u32 left_bg_w = src_config->dst_w, right_bg_w = 0;

		if (s_wconfig->clipWidth < src_config->dst_w &&
		    s_wconfig->clipHeight < src_config->dst_h) {
			left_bg_w = src_config->rsz_config.tw[0].in_len;
			right_bg_w = src_config->rsz_config.tw[1].in_len;
		} else if (src_config->is_dual) {
			left_bg_w = src_config->dst_w / 2;
			right_bg_w = src_config->dst_w / 2;
		}

		dpmgr_path_convert_wdma_roi_to_layer(&layer_roi, *d_wconfig,
						     src_config->rsz_config,
						     src_config);
		ret = dpmgr_path_split_layer_roi_to_pipe_roi(&pipe_roi, &layer_roi,
						       left_bg_w, right_bg_w, is_left);
		dpmgr_path_convert_pipe_roi_to_wdma(d_wconfig, pipe_roi);
	}

#ifdef CONFIG_MTK_LCM_PHYSICAL_ROTATION_HW
	is_left = !is_left;
#endif

	dpmgr_path_update_offset_before_rsz(src_config, dst_config);

	if (src_config->rdma_dirty) {
		struct tile_roi layer_roi = { 0 };
		struct tile_roi pipe_roi = { 0 };
		u32 left_w = src_config->dst_w, right_w = 0;

		s_rdma_config = &src_config->rdma_config;
		d_rdma_config = &dst_config->rdma_config;

		if ((s_rdma_config->src_w < s_rdma_config->width) &&
		    (s_rdma_config->src_h < s_rdma_config->height)) {
			left_w = src_config->rsz_config.tw[0].in_len;
			right_w = src_config->rsz_config.tw[1].in_len;
		} else if (src_config->is_dual) {
			left_w = s_rdma_config->dst_w / 2;
			right_w = s_rdma_config->dst_w / 2;
		}

		dpmgr_path_convert_rdma_roi_to_layer(&layer_roi, *d_rdma_config,
						     src_config->rsz_config,
						     src_config);
		dpmgr_path_split_layer_roi_to_pipe_roi(&pipe_roi, &layer_roi,
						       left_w, right_w, is_left);
		dpmgr_path_convert_pipe_roi_to_rdma(d_rdma_config, pipe_roi);

		if (!is_left)
			dst_config->ovl_layer_scanned = 0;
	}

	if (src_config->ovl_partial_dirty) {
		if (!is_left)
			dst_config->ovl_partial_roi.x = 0;

		dst_config->ovl_partial_roi.width /= 2;
	}

	if (src_config->ovl_dirty) {
		for (i = 0; i < TOTAL_OVL_LAYER_NUM; i++) {
			struct tile_roi layer_roi = { 0 };
			struct tile_roi pipe_roi = { 0 };
			u32 left_bg_w = src_config->dst_w, right_bg_w = 0;
			struct OVL_CONFIG_STRUCT *s_lconfig = NULL;
			struct OVL_CONFIG_STRUCT *d_lconfig = NULL;

			if (!dst_config->ovl_config[i].layer_en) {
				dst_config->ovl_config[i].ext_sel_layer = -1;
				continue;
			}

			s_lconfig = &src_config->ovl_config[i];
			d_lconfig = &dst_config->ovl_config[i];

			if ((s_lconfig->src_w < s_lconfig->dst_w) &&
			    (s_lconfig->src_h < s_lconfig->dst_h)) {
				left_bg_w = src_config->rsz_config.tw[0].in_len;
				right_bg_w = src_config->rsz_config.tw[1].in_len;
			} else if (src_config->is_dual) {
				left_bg_w = src_config->dst_w / 2;
				right_bg_w = src_config->dst_w / 2;
			}

			dpmgr_path_convert_ovl_roi_to_layer(&layer_roi, *d_lconfig,
							    src_config->rsz_config,
							    src_config);
			dpmgr_path_split_layer_roi_to_pipe_roi(&pipe_roi, &layer_roi,
							       left_bg_w, right_bg_w, is_left);
			dpmgr_path_convert_pipe_roi_to_ovl(d_lconfig, pipe_roi);

			if (!d_lconfig->dst_w)
				d_lconfig->layer_en = 0;
		}

		if (!is_left)
			dst_config->ovl_layer_scanned = 0;
	}
	return 0;
}

int dpmgr_path_config(disp_path_handle dp_handle, struct disp_ddp_path_config *config, void *cmdq_handle)
{
	int i = 0, k = 0;
	int module_name = 0;
	struct ddp_path_handle *handle = NULL;
	int *modules = NULL;
	int module_num = 0;
	struct disp_ddp_path_config *path_config = NULL;
	struct RSZ_CONFIG_STRUCT *rsz_config = NULL;
	int rsz_idx = 0;

	bool has_dual_path = false;
	enum DDP_SCENARIO_ENUM scn[2] = { DDP_SCENARIO_MAX, DDP_SCENARIO_MAX };

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;

	scn[0] = handle->scenario;
	has_dual_path = ddp_path_is_dual(scn[0]);
	if (has_dual_path)
		scn[1] = ddp_get_dual_module(scn[0]);

	config->is_dual = ddp_path_is_dual(handle->scenario);

	rsz_config = &config->rsz_config;

	DISP_LOG_V("path config ovl %d, rdma %d, wdma %d, dst %d on handle %p scenario %s\n",
		   config->ovl_dirty, config->rdma_dirty, config->wdma_dirty, config->dst_dirty,
		   handle, ddp_get_scenario_name(handle->scenario));

	if (has_dual_path || config->rsz_enable) {
		path_config = kzalloc(sizeof(struct disp_ddp_path_config), GFP_KERNEL);
		if (!path_config) {
			DISP_LOG_E("path_config NULL!\n");
			return -EINVAL;
		}
	}

	if (&handle->last_config != config)
		memcpy(&handle->last_config, config, sizeof(*config));

	for (k = 0; k <= has_dual_path; k++) {
		modules = ddp_get_scenario_list(scn[k]);
		module_num = ddp_get_module_num(scn[k]);

		if (config->rsz_enable)
			rsz_idx = module_num;

		if (has_dual_path || config->rsz_enable) {
			dpmgr_path_set_dual_config(&handle->last_config, path_config, !k);
			config = path_config;
		}

		if (has_dual_path)
			path_config->dst_w /= 2;

		for (i = 0; i < module_num; i++) {
			module_name = modules[i];

			if (ddp_modules_driver[module_name] == NULL)
				continue;

			if (config->rsz_enable && config->dst_dirty) {
				if (module_name == DISP_MODULE_RSZ0 || module_name == DISP_MODULE_RSZ1)
					rsz_idx = i;

				if (i < rsz_idx) {
					config->dst_w = rsz_config->tw[k].in_len;
					config->dst_h = rsz_config->th.in_len;
				} else {
					config->dst_w = rsz_config->tw[k].out_len;
					config->dst_h = rsz_config->th.out_len;
				}
			}

			if (disp_helper_get_option(DISP_OPT_OVL_PMA)) {
				if (module_name == DISP_MODULE_OVL0 || module_name == DISP_MODULE_OVL0_2L ||
				    module_name == DISP_MODULE_OVL1 || module_name == DISP_MODULE_OVL1_2L)
					module_name = dpmgr_path_ovl_pma_swap(module_name, config, k);
			}

			if (ddp_modules_driver[module_name]->config != 0)
				ddp_modules_driver[module_name]->config(module_name, config, cmdq_handle);

			/* External display scenario PQ need bypass */
			if ((disp_helper_get_option(DISP_OPT_BYPASS_PQ) ||
			    (dpmgr_scenario_is_SUB(handle->scenario) == 1)) &&
			    dpmgr_is_PQ(module_name) == 1) {
				if (ddp_modules_driver[module_name]->bypass != NULL)
					ddp_modules_driver[module_name]->bypass(module_name, 1);
			}
		}
	}

	if (has_dual_path || config->rsz_enable)
		kfree(path_config);

	return 0;
}

struct disp_ddp_path_config *dpmgr_path_get_last_config_notclear(disp_path_handle dp_handle)
{
	struct ddp_path_handle *handle = (struct ddp_path_handle *)dp_handle;

	ASSERT(dp_handle != NULL);
	return &handle->last_config;
}

struct disp_ddp_path_config *dpmgr_path_get_last_config(disp_path_handle dp_handle)
{
	struct ddp_path_handle *handle = (struct ddp_path_handle *)dp_handle;

	ASSERT(dp_handle != NULL);
	handle->last_config.ovl_dirty = 0;
	handle->last_config.rdma_dirty = 0;
	handle->last_config.wdma_dirty = 0;
	handle->last_config.dst_dirty = 0;
	handle->last_config.ovl_layer_dirty = 0;
	handle->last_config.ovl_layer_scanned = 0;
	handle->last_config.ovl_partial_dirty = 0;
	return &handle->last_config;
}

void dpmgr_get_input_address(disp_path_handle dp_handle, unsigned long *addr)
{
	struct ddp_path_handle *handle = (struct ddp_path_handle *)dp_handle;
	int *modules = ddp_get_scenario_list(handle->scenario);

	if (modules[0] == DISP_MODULE_OVL0 || modules[0] == DISP_MODULE_OVL1)
		ovl_get_address(modules[0], addr);
	else if (modules[0] == DISP_MODULE_RDMA0 || modules[0] == DISP_MODULE_RDMA1 ||
		 modules[0] == DISP_MODULE_RDMA2)
		rdma_get_address(modules[0], addr);
}

int dpmgr_path_build_cmdq(disp_path_handle dp_handle, void *trigger_loop_handle,
			  enum CMDQ_STATE state, int reverse)
{
	int ret = 0;
	int i = 0;
	int module_name;
	struct ddp_path_handle *handle;
	int *modules;
	int module_num;

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;
	modules = ddp_get_scenario_list(handle->scenario);
	module_num = ddp_get_module_num(handle->scenario);

	if (reverse) {
		for (i = module_num - 1; i >= 0; i--) {
			module_name = modules[i];
			if (ddp_modules_driver[module_name]
			    && ddp_modules_driver[module_name]->build_cmdq) {
				ret = ddp_modules_driver[module_name]->build_cmdq(module_name,
										  trigger_loop_handle,
										  state);
			}
		}
	} else {
		for (i = 0; i < module_num; i++) {
			module_name = modules[i];
			if (ddp_modules_driver[module_name]
			    && ddp_modules_driver[module_name]->build_cmdq) {
				ret = ddp_modules_driver[module_name]->build_cmdq(module_name,
										  trigger_loop_handle,
										  state);
			}
		}
	}
	return ret;
}

int dpmgr_path_trigger(disp_path_handle dp_handle, void *trigger_loop_handle, int encmdq)
{
	struct ddp_path_handle *handle;
	int *modules;
	int module_num;
	int i;
	int module_name;

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;
	DISP_LOG_V("dpmgr_path_trigger on scenario %s\n", ddp_get_scenario_name(handle->scenario));
	modules = ddp_get_scenario_list(handle->scenario);
	module_num = ddp_get_module_num(handle->scenario);


	if (disp_helper_get_option(DISP_OPT_SHADOW_REGISTER)) {
		if (disp_helper_get_option(DISP_OPT_SHADOW_MODE) == 0) {
			dpmgr_path_mutex_release(dp_handle, trigger_loop_handle);
			dpmgr_path_mutex_enable(dp_handle, trigger_loop_handle);
		} else {
			dpmgr_path_mutex_enable(dp_handle, trigger_loop_handle);
			dpmgr_path_mutex_get(dp_handle, trigger_loop_handle);
			dpmgr_path_mutex_release(dp_handle, trigger_loop_handle);
		}
	}

	for (i = 0; i < module_num; i++) {
		module_name = modules[i];
		if (ddp_modules_driver[module_name] != 0) {
			if (ddp_modules_driver[module_name]->trigger != 0) {
				ddp_modules_driver[module_name]->trigger(module_name,
									 trigger_loop_handle);
			}
		}
	}

	return 0;
}

int dpmgr_path_flush(disp_path_handle dp_handle, int encmdq)
{
	struct ddp_path_handle *handle;
	struct cmdqRecStruct *cmdqHandle;

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;
	cmdqHandle = encmdq ? handle->cmdqhandle : NULL;
	DDPDBG("path flush on scenario %s\n", ddp_get_scenario_name(handle->scenario));
	return ddp_mutex_enable(handle->hwmutexid, handle->scenario, cmdqHandle);
}

int dpmgr_path_power_off(disp_path_handle dp_handle, enum CMDQ_SWITCH encmdq)
{
	int i = 0;
	int module_name;
	struct ddp_path_handle *handle;
	int *modules;
	int module_num;

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;
	modules = ddp_get_scenario_list(handle->scenario);
	module_num = ddp_get_module_num(handle->scenario);

	DISP_LOG_I("path power off on scenario %s\n", ddp_get_scenario_name(handle->scenario));
	for (i = 0; i < module_num; i++) {
		module_name = modules[i];
		if (ddp_modules_driver[module_name] && ddp_modules_driver[module_name]->power_off) {
			ddp_modules_driver[module_name]->power_off(module_name,
								   encmdq ? handle->cmdqhandle : NULL);
		}
	}
	handle->power_state = 0;
	path_top_clock_off();
	return 0;
}

int dpmgr_path_power_on(disp_path_handle dp_handle, enum CMDQ_SWITCH encmdq)
{
	int i = 0;
	int module_name;
	int *modules;
	int module_num;
	struct ddp_path_handle *handle;

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;
	modules = ddp_get_scenario_list(handle->scenario);
	module_num = ddp_get_module_num(handle->scenario);

	DISP_LOG_I("path power on scenario %s\n", ddp_get_scenario_name(handle->scenario));
	path_top_clock_on();
	for (i = 0; i < module_num; i++) {
		module_name = modules[i];
		if (ddp_modules_driver[module_name] && ddp_modules_driver[module_name]->power_on) {
			ddp_modules_driver[module_name]->power_on(module_name,
								  encmdq ? handle->cmdqhandle : NULL);
		}
	}
	/* modules on this path will resume power on; */
	handle->power_state = 1;
	return 0;
}

int dpmgr_path_power_off_bypass_pwm(disp_path_handle dp_handle, enum CMDQ_SWITCH encmdq)
{
	int i = 0, k = 0;
	int module_name;
	struct ddp_path_handle *handle;
	int *modules;
	int module_num, path_num;
	enum DDP_SCENARIO_ENUM scn[2] = { DDP_SCENARIO_MAX, DDP_SCENARIO_MAX };

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;

	scn[0] = handle->scenario;
	path_num = 1;
	if (ddp_path_is_dual(handle->scenario)) {
		scn[1] = ddp_get_dual_module(scn[0]);
		path_num++;
	}

	DISP_LOG_I("path power off on scenario %s\n", ddp_get_scenario_name(handle->scenario));
	for (k = 0; k < path_num; k++) {
		modules = ddp_get_scenario_list(scn[k]);
		module_num = ddp_get_module_num(scn[k]);

		for (i = 0; i < module_num; i++) {
			module_name = modules[i];
			if (ddp_modules_driver[module_name] && ddp_modules_driver[module_name]->power_off) {
				if (module_name == DISP_MODULE_PWM0) {
					DDPMSG(" %s power off -- bypass\n", ddp_get_module_name(module_name));
				} else {
					ddp_modules_driver[module_name]->power_off(module_name,
										   encmdq ? handle->cmdqhandle : NULL);
				}
			}
		}
	}

	handle->power_state = 0;
	path_top_clock_off();
	return 0;
}

int dpmgr_path_power_on_bypass_pwm(disp_path_handle dp_handle, enum CMDQ_SWITCH encmdq)
{
	int i = 0, k = 0;
	int module_name;
	int *modules;
	int module_num, path_num;
	struct ddp_path_handle *handle;
	enum DDP_SCENARIO_ENUM scn[2] = { DDP_SCENARIO_MAX, DDP_SCENARIO_MAX };

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;

	DISP_LOG_I("path power on scenario %s\n", ddp_get_scenario_name(handle->scenario));
	path_top_clock_on();

	scn[0] = handle->scenario;
	path_num = 1;
	if (ddp_path_is_dual(handle->scenario)) {
		scn[1] = ddp_get_dual_module(scn[0]);
		path_num++;
	}

	for (k = 0; k < path_num; k++) {
		modules = ddp_get_scenario_list(scn[k]);
		module_num = ddp_get_module_num(scn[k]);

		for (i = 0; i < module_num; i++) {
			module_name = modules[i];
			if (ddp_modules_driver[module_name] && ddp_modules_driver[module_name]->power_on) {
				if (module_name == DISP_MODULE_PWM0) {
					DDPMSG(" %s power on -- bypass\n", ddp_get_module_name(module_name));
				} else {
					ddp_modules_driver[module_name]->power_on(module_name,
										  encmdq ? handle->cmdqhandle : NULL);
				}
			}
		}
	}

	/* modules on this path will resume power on; */
	handle->power_state = 1;
	return 0;
}

int dpmgr_path_power_off_default(disp_path_handle dp_handle, enum CMDQ_SWITCH encmdq)
{
	struct ddp_path_handle *handle;

	if (dp_handle == NULL) {
		DDPDUMP("--> power off default all\n");
		ddp_clk_enable_all(1);
		ddp_clk_enable_all(0);
		return 0;
	}

	handle = (struct ddp_path_handle *)dp_handle;

	DDPDUMP("--> power off default on scenario %s\n", ddp_get_scenario_name(handle->scenario));
	ddp_clk_enable_path(handle->scenario, 1);
	ddp_clk_enable_path(handle->scenario, 0);

	return 0;
}

static int is_module_in_path(enum DISP_MODULE_ENUM module, struct ddp_path_handle *handle)
{
	struct DDP_MANAGER_CONTEXT *context = _get_context();

	ASSERT(module < DISP_MODULE_UNKNOWN);
	if (context->module_path_table[module] == handle)
		return 1;

	return 0;
}

static bool can_dual_PQ(unsigned int cmd)
{
	switch (cmd) {
	case DISP_IOCTL_AAL_EVENTCTL:
	case DISP_IOCTL_AAL_INIT_REG:
	case DISP_IOCTL_AAL_SET_PARAM:
	case DISP_IOCTL_SET_GAMMALUT:
	case DISP_IOCTL_SET_CCORR:
	case DISP_IOCTL_CCORR_EVENTCTL:
	case DISP_IOCTL_SET_PQPARAM:
	case DISP_IOCTL_SET_COLOR_REG:
	case DISP_IOCTL_PQ_SET_BYPASS_COLOR:
	case DISP_IOCTL_PQ_SET_WINDOW:
	case DISP_IOCTL_WRITE_REG:
		return true;
	default:
		return false;
	}
}

static int get_pq_dual_module(enum DISP_MODULE_ENUM module, int msg)
{
	if (can_dual_PQ(msg) == false)
		return DISP_MODULE_UNKNOWN;

	switch (module) {
	case DISP_MODULE_COLOR0:
		return DISP_MODULE_COLOR1;
	case DISP_MODULE_CCORR0:
		return DISP_MODULE_CCORR1;
	case DISP_MODULE_AAL0:
		return DISP_MODULE_AAL1;
	case DISP_MODULE_GAMMA0:
		return DISP_MODULE_GAMMA1;
	default:
		return DISP_MODULE_UNKNOWN;
	}
}

int dpmgr_path_user_cmd(disp_path_handle dp_handle, int msg, unsigned long arg, void *cmdqhandle)
{
	int ret = -1;
	enum DISP_MODULE_ENUM dst = DISP_MODULE_UNKNOWN;
	struct ddp_path_handle *handle = NULL;

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;
	/* DISP_LOG_W("dpmgr_path_user_cmd msg 0x%08x\n",msg); */
	switch (msg) {
	case DISP_IOCTL_AAL_EVENTCTL:
	case DISP_IOCTL_AAL_GET_HIST:
	case DISP_IOCTL_AAL_INIT_REG:
	case DISP_IOCTL_AAL_SET_PARAM:
		/* TODO: just for verify rootcause, will be removed soon */
#ifndef CONFIG_FOR_SOURCE_PQ
		dst = DISP_MODULE_AAL0;
		if (is_module_in_path(dst, handle)) {
			if (ddp_modules_driver[dst]->cmd != NULL)
				ret = ddp_modules_driver[dst]->cmd(dst, msg, arg, cmdqhandle);

			dst = get_pq_dual_module(dst, msg);
			if (dst != DISP_MODULE_UNKNOWN && ddp_path_is_dual(handle->scenario) &&
				is_module_in_path(dst, handle)) {
				if (ddp_modules_driver[dst]->cmd != NULL)
					ret = ddp_modules_driver[dst]->cmd(dst, msg, arg, cmdqhandle);
			}
		}
#endif
		break;
	case DISP_IOCTL_SET_GAMMALUT:
		dst = DISP_MODULE_GAMMA0;
		if (ddp_modules_driver[dst]->cmd != NULL)
			ret = ddp_modules_driver[dst]->cmd(dst, msg, arg, cmdqhandle);

		if (ddp_path_is_dual(handle->scenario)) {
			dst = get_pq_dual_module(dst, msg);
			if (dst != DISP_MODULE_UNKNOWN && ddp_modules_driver[dst]->cmd != NULL)
				ret = ddp_modules_driver[dst]->cmd(dst, msg, arg, cmdqhandle);
		}
		break;
	case DISP_IOCTL_SET_CCORR:
	case DISP_IOCTL_CCORR_EVENTCTL:
	case DISP_IOCTL_CCORR_GET_IRQ:
		dst = DISP_MODULE_CCORR0;
		if (ddp_modules_driver[dst]->cmd != NULL)
			ret = ddp_modules_driver[dst]->cmd(dst, msg, arg, cmdqhandle);

		if (ddp_path_is_dual(handle->scenario)) {
			dst = get_pq_dual_module(dst, msg);
			if (dst != DISP_MODULE_UNKNOWN && ddp_modules_driver[dst]->cmd != NULL)
				ret = ddp_modules_driver[dst]->cmd(dst, msg, arg, cmdqhandle);
		}
		break;

	case DISP_IOCTL_SET_PQPARAM:
	case DISP_IOCTL_GET_PQPARAM:
	case DISP_IOCTL_SET_PQINDEX:
	case DISP_IOCTL_GET_PQINDEX:
	case DISP_IOCTL_SET_COLOR_REG:
	case DISP_IOCTL_SET_TDSHPINDEX:
	case DISP_IOCTL_GET_TDSHPINDEX:
	case DISP_IOCTL_SET_PQ_CAM_PARAM:
	case DISP_IOCTL_GET_PQ_CAM_PARAM:
	case DISP_IOCTL_SET_PQ_GAL_PARAM:
	case DISP_IOCTL_GET_PQ_GAL_PARAM:
	case DISP_IOCTL_PQ_SET_BYPASS_COLOR:
	case DISP_IOCTL_PQ_SET_WINDOW:
	case DISP_IOCTL_WRITE_REG:
	case DISP_IOCTL_READ_REG:
	case DISP_IOCTL_MUTEX_CONTROL:
	case DISP_IOCTL_PQ_GET_TDSHP_FLAG:
	case DISP_IOCTL_PQ_SET_TDSHP_FLAG:
	case DISP_IOCTL_PQ_GET_DC_PARAM:
	case DISP_IOCTL_PQ_SET_DC_PARAM:
	case DISP_IOCTL_PQ_GET_DS_PARAM:
	case DISP_IOCTL_PQ_GET_MDP_COLOR_CAP:
	case DISP_IOCTL_PQ_GET_MDP_TDSHP_REG:
	case DISP_IOCTL_WRITE_SW_REG:
	case DISP_IOCTL_READ_SW_REG:
		if (is_module_in_path(DISP_MODULE_COLOR0, handle))
			dst = DISP_MODULE_COLOR0;
		else if (is_module_in_path(DISP_MODULE_COLOR1, handle))
			dst = DISP_MODULE_COLOR1;
		else
			DISP_LOG_W("dpmgr_path_user_cmd color is not on this path\n");

		if (dst != DISP_MODULE_UNKNOWN) {
			if (ddp_modules_driver[dst]->cmd != NULL)
				ret = ddp_modules_driver[dst]->cmd(dst, msg, arg, cmdqhandle);
			if (ddp_path_is_dual(handle->scenario)) {
				dst = get_pq_dual_module(dst, msg);
				if (dst != DISP_MODULE_UNKNOWN && ddp_modules_driver[dst]->cmd != NULL)
					ret = ddp_modules_driver[dst]->cmd(dst, msg, arg, cmdqhandle);
			}
		}
		break;
	default:
		DISP_LOG_W("dpmgr_path_user_cmd io not supported\n");
		break;
	}
	return ret;
}

int dpmgr_path_set_parameter(disp_path_handle dp_handle, int io_evnet, void *data)
{
	return 0;
}

int dpmgr_path_get_parameter(disp_path_handle dp_handle, int io_evnet, void *data)
{
	return 0;
}

int dpmgr_path_is_idle(disp_path_handle dp_handle)
{
	ASSERT(dp_handle != NULL);
	return !dpmgr_path_is_busy(dp_handle);
}

int dpmgr_path_is_busy(disp_path_handle dp_handle)
{
	int i = 0;
	int module_name;
	struct ddp_path_handle *handle;
	int *modules;
	int module_num;

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;
	modules = ddp_get_scenario_list(handle->scenario);
	module_num = ddp_get_module_num(handle->scenario);

	DISP_LOG_V("path check busy on scenario %s\n", ddp_get_scenario_name(handle->scenario));
	for (i = module_num - 1; i >= 0; i--) {
		module_name = modules[i];
		if (ddp_modules_driver[module_name] != 0) {
			if (ddp_modules_driver[module_name]->is_busy != 0) {
				if (ddp_modules_driver[module_name]->is_busy(module_name)) {
					DISP_LOG_V("%s is busy\n", ddp_get_module_name(module_name));
					return 1;
				}
			}
		}
	}
	return 0;
}

int dpmgr_set_lcm_utils(disp_path_handle dp_handle, void *lcm_drv)
{
	int i = 0;
	int module_name;
	int *modules;
	int module_num;
	struct ddp_path_handle *handle;

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;
	modules = ddp_get_scenario_list(handle->scenario);
	module_num = ddp_get_module_num(handle->scenario);

	DISP_LOG_V("path set lcm drv handle %p\n", handle);
	for (i = 0; i < module_num; i++) {
		module_name = modules[i];
		if (ddp_modules_driver[module_name] != 0) {
			if ((ddp_modules_driver[module_name]->set_lcm_utils != 0) && lcm_drv) {
				DDPDBG("%s set lcm utils\n", ddp_get_module_name(module_name));
				ddp_modules_driver[module_name]->set_lcm_utils(module_name, lcm_drv);
			}
		}
	}
	return 0;
}

int dpmgr_enable_event(disp_path_handle dp_handle, enum DISP_PATH_EVENT event)
{
	struct ddp_path_handle *handle;
	struct DPMGR_WQ_HANDLE *wq_handle;

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;
	wq_handle = &handle->wq_list[event];

	DDPDBG("enable event %s on scenario %s, irtbit 0x%x\n",
		   path_event_name(event),
		   ddp_get_scenario_name(handle->scenario), handle->irq_event_map[event].irq_bit);
	if (!wq_handle->init) {
		init_waitqueue_head(&(wq_handle->wq));
		wq_handle->init = 1;
		wq_handle->data = 0;
		wq_handle->event = event;
	}
	return 0;
}

int dpmgr_map_event_to_irq(disp_path_handle dp_handle, enum DISP_PATH_EVENT event, enum DDP_IRQ_BIT irq_bit)
{
	struct ddp_path_handle *handle;
	struct DDP_IRQ_EVENT_MAPPING *irq_table;

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;
	irq_table = handle->irq_event_map;

	if (event < DISP_PATH_EVENT_NUM) {
		DDPDBG("map event %s to irq 0x%x on scenario %s\n", path_event_name(event),
			   irq_bit, ddp_get_scenario_name(handle->scenario));
		irq_table[event].irq_bit = irq_bit;
		return 0;
	}
	DISP_LOG_E("fail to map event %s to irq 0x%x on scenario %s\n", path_event_name(event),
		   irq_bit, ddp_get_scenario_name(handle->scenario));
	return -1;
}

int dpmgr_disable_event(disp_path_handle dp_handle, enum DISP_PATH_EVENT event)
{
	struct ddp_path_handle *handle;
	struct DPMGR_WQ_HANDLE *wq_handle;

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;

	DDPDBG("disable event %s on scenario %s\n", path_event_name(event),
		   ddp_get_scenario_name(handle->scenario));
	wq_handle = &handle->wq_list[event];
	wq_handle->init = 0;
	wq_handle->data = 0;
	return 0;
}

int dpmgr_check_status_by_scenario(enum DDP_SCENARIO_ENUM scenario)
{
	int i = 0;
	int *modules = ddp_get_scenario_list(scenario);
	int module_num = ddp_get_module_num(scenario);
	struct DDP_MANAGER_CONTEXT *context = _get_context();

	DDPDUMP("--> check status on scenario %s\n", ddp_get_scenario_name(scenario));

	if (!(context->power_state)) {
		DDPDUMP("cannot check ddp status due to already power off\n");
		return 0;
	}

	ddp_check_path(scenario);

	{
		DDPDUMP("path:\n");
		for (i = 0; i < module_num; i++)
			DDPDUMP("%s-\n", ddp_get_module_name(modules[i]));

		DDPDUMP("\n");
	}

	for (i = 0; i < module_num; i++)
		ddp_dump_analysis(modules[i]);

	for (i = 0; i < module_num; i++)
		ddp_dump_reg(modules[i]);

	return 0;
}

int dpmgr_check_status(disp_path_handle dp_handle)
{
	int i = 0;
	int *modules;
	int module_num, k, path_num;
	struct ddp_path_handle *handle;
	struct DDP_MANAGER_CONTEXT *context = _get_context();
	enum DDP_SCENARIO_ENUM scn[2] = { DDP_SCENARIO_MAX, DDP_SCENARIO_MAX };

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;

	scn[0] = handle->scenario;
	path_num = 1;
	if (ddp_path_is_dual(handle->scenario)) {
		scn[1] = ddp_get_dual_module(scn[0]);
		path_num++;
	}

	DDPDUMP("--> check status on scenario %s\n", ddp_get_scenario_name(handle->scenario));
	if (!(context->power_state)) {
		DDPDUMP("cannot check ddp status due to already power off\n");
		return 0;
	}

	ddp_dump_analysis(DISP_MODULE_CONFIG);
	ddp_check_hw_path(handle->scenario);
	ddp_check_clk_path(handle->scenario, 1);
	ddp_check_path(handle->scenario);
	ddp_check_mutex(handle->hwmutexid, handle->scenario, handle->mode);

	/* dump path */
	for (k = 0; k < path_num; k++) {
		modules = ddp_get_scenario_list(scn[k]);
		module_num = ddp_get_module_num(scn[k]);
		DDPDUMP("path:\n");
		for (i = 0; i < module_num; i++)
			DDPDUMP("%s-\n", ddp_get_module_name(modules[i]));

		DDPDUMP("\n");
	}
	ddp_dump_analysis(DISP_MODULE_MUTEX);

	for (k = 0; k < path_num; k++) {
		modules = ddp_get_scenario_list(scn[k]);
		module_num = ddp_get_module_num(scn[k]);
		for (i = 0; i < module_num; i++)
			ddp_dump_analysis(modules[i]);
	}

	for (k = 0; k < path_num; k++) {
		modules = ddp_get_scenario_list(scn[k]);
		module_num = ddp_get_module_num(scn[k]);
		for (i = 0; i < module_num; i++)
			ddp_dump_reg(modules[i]);
	}

	ddp_dump_reg(DISP_MODULE_CONFIG);
	ddp_dump_reg(DISP_MODULE_MUTEX);

	return 0;
}

int dpmgr_check_clk(disp_path_handle dp_handle, int clk_on)
{
	struct ddp_path_handle *handle;

	if (dp_handle == NULL) {
		DDPDUMP("--> check clk all: %d\n", clk_on);
		ddp_check_clk_all(clk_on);
		return 0;
	}

	handle = (struct ddp_path_handle *)dp_handle;

	DDPDUMP("--> check clk on scenario %s: %d\n", ddp_get_scenario_name(handle->scenario),
		clk_on);
	ddp_check_clk_path(handle->scenario, clk_on);

	return 0;
}

void dpmgr_debug_path_status(int mutex_id)
{
	int i = 0;
	struct DDP_MANAGER_CONTEXT *content = _get_context();
	disp_path_handle handle = NULL;

	if (mutex_id >= DISP_MUTEX_DDP_FIRST &&
	    mutex_id < (DISP_MUTEX_DDP_FIRST + DISP_MUTEX_DDP_COUNT)) {
		handle = (disp_path_handle) content->handle_pool[mutex_id];
		if (handle)
			dpmgr_check_status(handle);
	} else {
		for (i = DISP_MUTEX_DDP_FIRST; i < (DISP_MUTEX_DDP_FIRST + DISP_MUTEX_DDP_COUNT); i++) {
			handle = (disp_path_handle) content->handle_pool[i];
			if (handle)
				dpmgr_check_status(handle);
		}
	}
}

int dpmgr_wait_event_timeout(disp_path_handle dp_handle, enum DISP_PATH_EVENT event, int timeout)
{
	int ret = -1;
	struct ddp_path_handle *handle;
	struct DPMGR_WQ_HANDLE *wq_handle;
	unsigned long long cur_time;

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;
	wq_handle = &handle->wq_list[event];

	if (wq_handle->init) {
		cur_time = ktime_to_ns(ktime_get());

		ret = wait_event_interruptible_timeout(wq_handle->wq, cur_time < wq_handle->data,
						       timeout);
		if (ret == 0) {
			DISP_LOG_E("wait %s timeout on scenario %s\n", path_event_name(event),
				   ddp_get_scenario_name(handle->scenario));
			/* dpmgr_check_status(dp_handle); */
		} else if (ret < 0) {
			DISP_LOG_E("wait %s interrupt by other timeleft %d on scenario %s\n",
				   path_event_name(event), ret,
				   ddp_get_scenario_name(handle->scenario));
		} else {
			DISP_LOG_V("received event %s timeleft %d on scenario %s\n",
				   path_event_name(event), ret,
				   ddp_get_scenario_name(handle->scenario));
		}
		return ret;
	}
	DISP_LOG_E("wait event %s not initialized on scenario %s\n", path_event_name(event),
		   ddp_get_scenario_name(handle->scenario));
	return ret;
}

int _dpmgr_wait_event(disp_path_handle dp_handle, enum DISP_PATH_EVENT event, unsigned long long *event_ts)
{
	int ret = -1;
	struct ddp_path_handle *handle;
	struct DPMGR_WQ_HANDLE *wq_handle;
	unsigned long long cur_time;

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;
	wq_handle = &handle->wq_list[event];

	if (!wq_handle->init) {
		DISP_LOG_E("wait event %s not initialized on scenario %s\n", path_event_name(event),
			ddp_get_scenario_name(handle->scenario));
		return -2;
	}

	cur_time = ktime_to_ns(ktime_get());

	ret = wait_event_interruptible(wq_handle->wq, cur_time < wq_handle->data);
	if (ret < 0) {
		DISP_LOG_E("wait %s interrupt by other ret %d on scenario %s\n",
			   path_event_name(event), ret,
			   ddp_get_scenario_name(handle->scenario));
	}
	if (event_ts)
		*event_ts = wq_handle->data;

	return ret;
}

int dpmgr_wait_event(disp_path_handle dp_handle, enum DISP_PATH_EVENT event)
{
	return _dpmgr_wait_event(dp_handle, event, NULL);
}

int dpmgr_wait_event_ts(disp_path_handle dp_handle, enum DISP_PATH_EVENT event, unsigned long long *event_ts)
{
	return _dpmgr_wait_event(dp_handle, event, event_ts);
}

int dpmgr_signal_event(disp_path_handle dp_handle, enum DISP_PATH_EVENT event)
{
	struct ddp_path_handle *handle;
	struct DPMGR_WQ_HANDLE *wq_handle;

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;
	wq_handle = &handle->wq_list[event];

	if (handle->wq_list[event].init) {
		wq_handle->data = ktime_to_ns(ktime_get());
		wake_up_interruptible(&(handle->wq_list[event].wq));
	}
	return 0;
}

static void dpmgr_irq_handler(enum DISP_MODULE_ENUM module, unsigned int regvalue)
{
	int i = 0;
	int j = 0;
	int irq_bits_num = 0;
	int irq_bit = 0;
	struct ddp_path_handle *handle = NULL;

	handle = find_handle_by_module(module);
	if (handle == NULL)
		return;

	irq_bits_num = ddp_get_module_max_irq_bit(module);
	for (i = 0; i <= irq_bits_num; i++) {
		if (regvalue & (0x1 << i)) {
			irq_bit = MAKE_DDP_IRQ_BIT(module, i);
			dprec_stub_irq(irq_bit);
			for (j = 0; j < DISP_PATH_EVENT_NUM; j++) {
				if (handle->wq_list[j].init && irq_bit == handle->irq_event_map[j].irq_bit) {
					dprec_stub_event(j);
					handle->wq_list[j].data = ktime_to_ns(ktime_get());

					DDPIRQ("irq signal event %s on cycle %llu on scenario %s\n",
					       path_event_name(j), handle->wq_list[j].data,
					       ddp_get_scenario_name(handle->scenario));

					wake_up_interruptible(&(handle->wq_list[j].wq));
				}
			}
		}
	}
}

int dpmgr_init(void)
{
	DISP_LOG_I("ddp manager init\n");
	if (ddp_manager_init)
		return 0;

	ddp_manager_init = 1;
	ddp_debug_init();
	disp_init_irq();
	disp_register_irq_callback(dpmgr_irq_handler);
	return 0;
}

int dpmgr_factory_mode_test(int module_name, void *cmdqhandle, void *config)
{
	if (ddp_modules_driver[module_name] != 0) {
		if (ddp_modules_driver[module_name]->ioctl != 0) {
			DISP_LOG_I(" %s factory_mode_test\n", ddp_get_module_name(DISP_MODULE_DPI));
			ddp_modules_driver[DISP_MODULE_DPI]->ioctl(module_name, cmdqhandle,
								   DDP_DPI_FACTORY_TEST, config);
		}
	}

	return 0;
}

int dpmgr_factory_mode_reset(int module_name, void *cmdqhandle, void *config)
{
	if (ddp_modules_driver[module_name] != 0) {
		if (ddp_modules_driver[module_name]->ioctl != 0) {
			DISP_LOG_I(" %s factory_mode_test\n", ddp_get_module_name(DISP_MODULE_DPI));
			ddp_modules_driver[DISP_MODULE_DPI]->ioctl(module_name, cmdqhandle,
								   DDP_DPI_FACTORY_RESET, config);
		}
	}

	return 0;
}

int dpmgr_wait_ovl_available(int ovl_num)
{
	int ret = 1;

	return ret;
}

int dpmgr_path_mutex_get(disp_path_handle dp_handle, void *cmdqhandle)
{
	struct ddp_path_handle *handle = NULL;

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;

	return ddp_mutex_get(handle->hwmutexid, cmdqhandle);
}

int dpmgr_path_mutex_release(disp_path_handle dp_handle, void *cmdqhandle)
{
	struct ddp_path_handle *handle = NULL;

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;

	return ddp_mutex_release(handle->hwmutexid, cmdqhandle);
}

int dpmgr_path_mutex_enable(disp_path_handle dp_handle, void *cmdqhandle)
{
	struct ddp_path_handle *handle = NULL;

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;

	return ddp_mutex_enable(handle->hwmutexid, 0, cmdqhandle);
}

int switch_module_to_nonsec(disp_path_handle dp_handle, void *cmdqhandle, const char *caller)
{

	int i = 0;
	int module_name;
	struct ddp_path_handle *handle;
	int *modules;
	int module_num;

	ASSERT(dp_handle != NULL);
	handle = (struct ddp_path_handle *)dp_handle;
	modules = ddp_get_scenario_list(handle->scenario);
	module_num = ddp_get_module_num(handle->scenario);
	DDPMSG("[SVP] switch module to nonsec on scenario %s, caller=%s\n",
		ddp_get_scenario_name(handle->scenario), caller);

	for (i = module_num - 1; i >= 0; i--) {
		module_name = modules[i];
		if (ddp_modules_driver[module_name] != 0) {
			if (ddp_modules_driver[module_name]->switch_to_nonsec != 0)
				ddp_modules_driver[module_name]->switch_to_nonsec(module_name, cmdqhandle);
		}
	}
	return 0;
}

int dpmgr_path_update_rsz(struct disp_ddp_path_config *pconfig)
{
	struct RSZ_CONFIG_STRUCT *rsz_config = &pconfig->rsz_config;
	int is_dual = pconfig->is_dual;
	enum RSZ_COLOR_FORMAT rsz_cfmt = UNKNOWN_RSZ_CFMT;
	enum HRT_SCALE_SCENARIO hrt_scale = pconfig->hrt_scale;
	enum HRT_PATH_SCENARIO hrt_path = pconfig->hrt_path;
	struct rsz_lookuptable *list = primary_get_rsz_input_res_list();

	/* update frame in/out and ratio */
	rsz_config->frm_in_w = list[hrt_scale].in_w;
	rsz_config->frm_in_h = list[hrt_scale].in_h;
	rsz_config->frm_out_w = pconfig->dst_w;
	rsz_config->frm_out_h = pconfig->dst_h;
	rsz_config->ratio = list[hrt_scale].scaling_ratio;

	/* update tile */
	rsz_calc_tile_params(rsz_config->frm_in_w, rsz_config->frm_out_w, is_dual, rsz_config->tw);
	rsz_calc_tile_params(rsz_config->frm_in_h, rsz_config->frm_out_h, false, &rsz_config->th);

	/* update color format */
	switch (hrt_path) {
	case HRT_PATH_DUAL_PIPE_RESIZE_PARTIAL:
	case HRT_PATH_RESIZE_PARTIAL:
	case HRT_PATH_DUAL_DISP_MIRROR_RESIZE_PARTIAL:
	case HRT_PATH_DUAL_DISP_EXT_RESIZE_PARTIAL:
		rsz_cfmt = RGB888;
		break;
	case HRT_PATH_DUAL_PIPE_RESIZE_PARTIAL_PMA:
	case HRT_PATH_RESIZE_PARTIAL_PMA:
	case HRT_PATH_DUAL_DISP_MIRROR_RESIZE_PARTIAL_PMA:
	case HRT_PATH_DUAL_DISP_EXT_RESIZE_PARTIAL_PMA:
		rsz_cfmt = ARGB8101010;
		break;
	case HRT_PATH_DUAL_DISP_MIRROR_RESIZE_GENERAL:
	case HRT_PATH_DUAL_DISP_EXT_RESIZE_GENERAL:
		rsz_cfmt = RGB888;
		break;
	case HRT_PATH_RESIZE_GENERAL:
	case HRT_PATH_DUAL_PIPE_RESIZE_GENERAL:
		rsz_cfmt = RGB999;
		break;
	case HRT_PATH_GENERAL:
	case HRT_PATH_DUAL_DISP_MIRROR:
	case HRT_PATH_DUAL_DISP_EXT:
	case HRT_PATH_DUAL_PIPE:
	default:
		rsz_cfmt = UNKNOWN_RSZ_CFMT;
		break;
	}
	rsz_config->fmt = rsz_cfmt;

#if 0
	{
	int i = 0;

	DDPDBG("%s:rsz_config:h:step:%u,offset(%u.%u),len:%u->%u|frm(%ux%u)->(%u,%u),r:%d,fmt:%s\n",
	       __func__, rsz_config->th.step,
	       rsz_config->th.int_offset, rsz_config->th.sub_offset,
	       rsz_config->th.in_len, rsz_config->th.out_len,
	       rsz_config->frm_in_w, rsz_config->frm_in_h,
	       rsz_config->frm_out_w, rsz_config->frm_out_h,
	       rsz_config->ratio, rsz_cfmt_name(rsz_config->fmt));
	for (i = 0; i <= is_dual; i++)
		DDPDBG("%s:w:step:%u,offset(%u.%u),len:%u->%u\n",
		       __func__, rsz_config->tw[i].step,
		       rsz_config->tw[i].int_offset, rsz_config->tw[i].sub_offset,
		       rsz_config->tw[i].in_len, rsz_config->tw[i].out_len);
	}
#endif

	return 0;
}

bool dpmgr_path_is_module_in_scenario(disp_path_handle *dp_handle, enum DISP_MODULE_ENUM module)
{
	struct ddp_path_handle *phandle = (struct ddp_path_handle *)dp_handle;

	return ddp_is_module_in_scenario(phandle->scenario, module);
}
