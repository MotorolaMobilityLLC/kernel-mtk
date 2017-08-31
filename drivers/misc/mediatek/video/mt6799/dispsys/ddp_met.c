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

#define LOG_TAG "MET"

#include "ddp_log.h"

#include <mt-plat/met_drv.h>
#include "ddp_irq.h"
#include "ddp_reg.h"
#include "ddp_met.h"
#include "ddp_path.h"
#include "ddp_ovl.h"
#include "ddp_rdma.h"

#define DDP_IRQ_ERR_ID				(0xFFFF0000)
#define DDP_IRQ_FPS_ID				(DDP_IRQ_ERR_ID + 1)
#define DDP_IRQ_LAYER_FPS_ID		(DDP_IRQ_ERR_ID + 2)
#define DDP_IRQ_WND_MIN_FPS_ID		(DDP_IRQ_ERR_ID + 3)
#define DDP_IRQ_WND_MAX_FPS_ID		(DDP_IRQ_ERR_ID + 4)
#define DDP_IRQ_WND_FPS_SLP_ID		(DDP_IRQ_ERR_ID + 5)
#define DDP_IRQ_CONTINUOUS_DROP_ID	(DDP_IRQ_ERR_ID + 6)

#define DDP_IRQ_WTPF_ID				(DDP_IRQ_ERR_ID + 10)

#define MAX_PATH_NUM (3)
#define RDMA_NUM (2)
#define MAX_OVL_LAYERS (4)
#define OVL_LAYER_NUM_PER_OVL (4)


struct SWindowFps {
	unsigned long long last_time;
	unsigned int frame_num;
};
static unsigned int met_tag_on;

/* Calc fps per WND_xx_SIZE frames, this can be changed through adb shell cmd */
unsigned int DISP_FPS_WND_MIN_SIZE = 1;
unsigned int DISP_FPS_WND_MAX_SIZE = 5;

static struct SWindowFps window_min_fps;
static struct SWindowFps window_max_fps;
static int last_wnd_min_fps;
static int last_wnd_max_fps;
static int last_wnd_min_slope;
static unsigned long long last_config;
static unsigned int continuous_drop_frame;

/**
 * check if it's decouple mode
 *
 * mutex_id  |  decouple  |  direct-link
 * -------------------------------------
 * OVL_Path  |      1     |       0
 * RDMA_Path |      0     |       X
 *
 */
int dpp_disp_is_decouple(void)
{
	if (ddp_is_moudule_in_mutex(0, DISP_MODULE_OVL0) ||
	    ddp_is_moudule_in_mutex(0, DISP_MODULE_OVL0_2L))
		return 0;
	else
		return 1;
}

/**
 * Represent to LCM display refresh rate
 * Primary Display:  map to RDMA0 sof/eof ISR, for all display mode
 * External Display: map to RDMA1 sof/eof ISR, for all display mode
 * NOTICE:
 *		for WFD, nothing we can do here
 */
static void ddp_disp_refresh_tag_start(unsigned int index)
{
	static unsigned long rdma_buf_addr[RDMA_NUM];
	static unsigned long ovl_buf_addr[OVL_NUM*OVL_LAYER_NUM_PER_OVL];

	struct RDMA_BASIC_STRUCT rdma_info;
	struct OVL_BASIC_STRUCT ovl_info[OVL_NUM*OVL_LAYER_NUM_PER_OVL];
	int has_layer_changed;
	int i, j, idx;

	char tag_name[30] = { '\0' };
	int wnd_min_fps, wnd_max_fps;
	int wnd_min_slope;

	if (dpp_disp_is_decouple() == 1) {

		rdma_get_info(index, &rdma_info);
		if (rdma_info.addr != 0 && rdma_buf_addr[index] != rdma_info.addr) {
			rdma_buf_addr[index] = rdma_info.addr;
			sprintf(tag_name, index ? "ExtDispRefresh" : "PrimDispRefresh");
			met_tag_oneshot(DDP_IRQ_FPS_ID, tag_name, 1);
			continuous_drop_frame = 0;
		} else {
			continuous_drop_frame++;
		}

		if (window_min_fps.last_time == 0) {
			window_min_fps.last_time = sched_clock();
			window_max_fps.last_time = sched_clock();
		}
		window_min_fps.frame_num++;
		window_max_fps.frame_num++;
	} else {

		has_layer_changed = 0;

		/*Traversal layers and get layer info*/
		memset(ovl_info, 0, sizeof(ovl_info));/*essential for structure comparision*/

		for (i = 0; i < OVL_NUM; i++) {

			ovl_get_info(i, &(ovl_info[i*OVL_LAYER_NUM_PER_OVL]));

			for (j = 0; j < OVL_LAYER_NUM_PER_OVL; j++) {
				idx = (i * OVL_LAYER_NUM_PER_OVL) + j;

				if ((ovl_info[idx].layer_en && (ovl_info[idx].addr != ovl_buf_addr[idx]))
					|| (!ovl_info[idx].layer_en && ovl_buf_addr[idx] != 0)) {
					has_layer_changed = 1;
					if (ovl_info[idx].layer_en)
						ovl_buf_addr[idx] = ovl_info[idx].addr;
					else
						ovl_buf_addr[idx] = 0x0;
				}
			}
		}

		if (has_layer_changed) {
			sprintf(tag_name, index ? "ExtDispRefresh" : "PrimDispRefresh");
			met_tag_oneshot(DDP_IRQ_FPS_ID, tag_name, 1);
			if (window_min_fps.last_time == 0) {
				window_min_fps.last_time = sched_clock();
				window_max_fps.last_time = sched_clock();
			}
			window_min_fps.frame_num++;
			window_max_fps.frame_num++;
			continuous_drop_frame = 0;
		} else {
			continuous_drop_frame++;
		}
	}
	/* output little window size fps and it's slope */
	if (window_min_fps.frame_num >= DISP_FPS_WND_MIN_SIZE) {
		wnd_min_fps = 1000*1000*1000/((sched_clock()-window_min_fps.last_time)/window_min_fps.frame_num);
		sprintf(tag_name, index ? "ExtWndFps(%df)" : "PrimWndFps(%df)", DISP_FPS_WND_MIN_SIZE);
		met_tag_oneshot(DDP_IRQ_WND_MIN_FPS_ID, tag_name, wnd_min_fps);
		wnd_min_slope = wnd_min_fps - last_wnd_min_fps;

		sprintf(tag_name, index ? "ExtWndFpsSoS" : "PrimWndFpsSoS");
		if (last_wnd_min_slope != 0)
			met_tag_oneshot(DDP_IRQ_WND_FPS_SLP_ID, tag_name,
							(wnd_min_slope-last_wnd_min_slope)/wnd_min_slope);
		last_wnd_min_fps = wnd_min_fps;
		last_wnd_min_slope = wnd_min_slope;
		/* clean up for next window stat */
		window_min_fps.frame_num = 0;
		window_min_fps.last_time = sched_clock();
	}
	/* output big window size fps */
	if (window_max_fps.frame_num >= DISP_FPS_WND_MAX_SIZE) {
		wnd_max_fps = 1000*1000*1000/((sched_clock()-window_max_fps.last_time)/window_max_fps.frame_num);
		sprintf(tag_name, index ? "ExtWndFps(%df)" : "PrimWndFps(%df)", DISP_FPS_WND_MAX_SIZE);
		met_tag_oneshot(DDP_IRQ_WND_MAX_FPS_ID, tag_name, wnd_max_fps);

		last_wnd_max_fps = wnd_max_fps;
		/* clean up for next window stat */
		window_max_fps.frame_num = 0;
		window_max_fps.last_time = sched_clock();
	}

	/* output waste time per frame -- WTPF(ms) */
	if (last_config != 0) {
		sprintf(tag_name, "WTPF(ms)");
		met_tag_oneshot(DDP_IRQ_WTPF_ID, tag_name, (sched_clock()-last_config)/1000/1000);
	}

	/* output continuous drop frame */
	sprintf(tag_name, index ? "ExtContinuousDrop" : "PrimContinuousDrop");
	met_tag_oneshot(DDP_IRQ_CONTINUOUS_DROP_ID, tag_name, continuous_drop_frame);
}

static void ddp_disp_refresh_tag_end(unsigned int index)
{
	char tag_name[30] = { '\0' };

	sprintf(tag_name, index ? "ExtDispRefresh" : "PrimDispRefresh");
	met_tag_oneshot(DDP_IRQ_FPS_ID, tag_name, 0);
}

/**
 * Represent to OVL0/0VL1 each layer's refresh rate
 */
static void ddp_inout_info_tag(unsigned int index)
{
#if 0
	static unsigned long sLayerBufAddr[OVL_NUM][OVL_LAYER_NUM_PER_OVL];
	static unsigned int sLayerBufFmt[OVL_NUM][OVL_LAYER_NUM_PER_OVL];
	static unsigned int sLayerBufWidth[OVL_NUM][OVL_LAYER_NUM_PER_OVL];
	static unsigned int sLayerBufHeight[OVL_NUM][OVL_LAYER_NUM_PER_OVL];

	struct OVL_BASIC_STRUCT ovlInfo[OVL_NUM*OVL_LAYER_NUM_PER_OVL];
	unsigned int flag, i, idx, enLayerCnt, layerCnt;

	char tag_name[30] = { '\0' };
	uint32_t layer_change_bits = 0;
	uint32_t layer_enable_bits = 0;

	memset((void *)ovlInfo, 0, sizeof(ovlInfo));
	ovl_get_info(index, ovlInfo);

	/* Any layer enable bit changes , new frame refreshes */
	enLayerCnt = 0;
	if (ovl_get_status() == DDP_OVL1_STATUS_PRIMARY)
		layerCnt = OVL_LAYER_NUM;
	else
		layerCnt = OVL_LAYER_NUM_PER_OVL;

	for (i = 0; i < layerCnt; i++) {
		if (ovl_get_status() == DDP_OVL1_STATUS_PRIMARY)
			index = 1 - i / OVL_LAYER_NUM_PER_OVL;

		idx = i % OVL_LAYER_NUM_PER_OVL;

		if (ovlInfo[i].layer_en) {
			enLayerCnt++;
			layer_enable_bits |= (1 << i);
			if (sLayerBufAddr[index][idx] != ovlInfo[i].addr) {
				sLayerBufAddr[index][idx] = ovlInfo[i].addr;
				sprintf(tag_name, "OVL%dL%d_InFps", index, idx);
				met_tag_oneshot(DDP_IRQ_LAYER_FPS_ID, tag_name, i+1);
				layer_change_bits |= 1 << i;
			}
		} else {
			sLayerBufAddr[index][idx] = 0;
		}

		if ((i == (OVL_LAYER_NUM_PER_OVL - 1)) || (i == (OVL_LAYER_NUM - 1))) {
			if (enLayerCnt) {
				enLayerCnt = 0;
				sprintf(tag_name, "OVL%d_OutFps", index);
				met_tag_oneshot(DDP_IRQ_LAYER_FPS_ID, tag_name, index);
			}
		}
	}


	/*CLS:met mmsys profile*/
	{
		int i;

		for (i = 0; i < OVL_LAYER_NUM; i++) {
			if (layer_change_bits & (1 << i)) {
				MET_UDTL_GET_PROP(OVL_LAYER_Props).layer	= i;
				MET_UDTL_GET_PROP(OVL_LAYER_Props).layer_en	= layer_enable_bits;
				MET_UDTL_GET_PROP(OVL_LAYER_Props).fmt	= ovlInfo[i].fmt;
				MET_UDTL_GET_PROP(OVL_LAYER_Props).addr	= ovlInfo[i].addr;
				MET_UDTL_GET_PROP(OVL_LAYER_Props).src_w	= ovlInfo[i].src_w;
				MET_UDTL_GET_PROP(OVL_LAYER_Props).src_h	= ovlInfo[i].src_h;
				MET_UDTL_GET_PROP(OVL_LAYER_Props).src_pitch	= ovlInfo[i].src_pitch;
				MET_UDTL_GET_PROP(OVL_LAYER_Props).bpp	= ovlInfo[i].bpp;

				MET_UDTL_TRACELINE_PROP(MMSYS, OVL_LAYERS__LAYER, OVL_LAYER_Props);
			}
		}
	}


#endif

}

static void ddp_err_irq_met_tag(const char *name)
{
	met_tag_oneshot(DDP_IRQ_ERR_ID, name, 1);
	met_tag_oneshot(DDP_IRQ_ERR_ID, name, 0);
}

static void met_irq_handler(enum DISP_MODULE_ENUM module, unsigned int reg_val)
{
	int index = 0;
	char tag_name[30] = { '\0' };
	int mutexID;
	/* DDPERR("met_irq_handler() module=%d, val=0x%x\n", module, reg_val); */
	switch (module) {
	case DISP_MODULE_DSI0:
	case DISP_MODULE_DSI1:
		index = module - DISP_MODULE_DSI0;
		if (reg_val & (1 << 2)) /* TE signal */
			ddp_disp_refresh_tag_start(index);

		if (reg_val & (1 << 4)) /* frame done signal */
			ddp_disp_refresh_tag_end(index);

		break;

	case DISP_MODULE_RDMA0:
	case DISP_MODULE_RDMA1:
		index = module - DISP_MODULE_RDMA0;
		if (reg_val & (1 << 1))
			ddp_disp_refresh_tag_start(index);

		if (reg_val & (1 << 2))
			ddp_disp_refresh_tag_end(index);

		if (reg_val & (1 << 3)) {
			sprintf(tag_name, "rdma%d_abnormal", index);
			ddp_err_irq_met_tag(tag_name);
		}

		if (reg_val & (1 << 4)) {
			sprintf(tag_name, "rdma%d_underflow", index);
			ddp_err_irq_met_tag(tag_name);
		}

		if (reg_val & (1 << 6)) {
			sprintf(tag_name, "rdma%d_fifoempty", index);
			ddp_err_irq_met_tag(tag_name);
		}
		break;

	case DISP_MODULE_OVL0:
	case DISP_MODULE_OVL1:
		index = module - DISP_MODULE_OVL0;
		if (reg_val & (1 << 1)) {/*EOF*/
			ddp_inout_info_tag(index);
			if (met_mmsys_event_disp_ovl_eof)
				met_mmsys_event_disp_ovl_eof(index);
		}

		break;

	case DISP_MODULE_MUTEX:
		/*reg_val is  DISP_REG_GET(DISP_REG_CONFIG_MUTEX_INTSTA) & 0x7C1F; */
		for (mutexID = DISP_MUTEX_DDP_FIRST; mutexID <= DISP_MUTEX_DDP_LAST; mutexID++) {
			if (reg_val & (0x1<<mutexID))
				if (met_mmsys_event_disp_sof)
					met_mmsys_event_disp_sof(mutexID);

			if (reg_val & (0x1<<(mutexID+DISP_MUTEX_TOTAL)))
				if (met_mmsys_event_disp_mutex_eof)
					met_mmsys_event_disp_mutex_eof(mutexID);
		}
		break;

	default:
		break;
	}
}

void ddp_init_met_tag(int state, int rdma0_mode, int rdma1_mode)
{
	if ((!met_tag_on) && state) {
		met_tag_on = state;
		/* this will failed, maybe callback num exceed 10 */
		/*disp_register_irq_callback(met_irq_handler);*/

		disp_register_module_irq_callback(DISP_MODULE_RDMA0, met_irq_handler);
	}
	if (met_tag_on && (!state)) {
		met_tag_on = state;
		/*disp_unregister_irq_callback(met_irq_handler);*/

		disp_unregister_module_irq_callback(DISP_MODULE_RDMA0, met_irq_handler);
	}
}
EXPORT_SYMBOL(ddp_init_met_tag);
