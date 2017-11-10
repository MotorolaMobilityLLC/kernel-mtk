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

#include "disp_log.h"

#include <mt-plat/met_drv.h>
#include "ddp_irq.h"
#include "ddp_reg.h"
#include "ddp_met.h"
#include "ddp_ovl.h"
#include "ddp_rdma.h"
#include "ddp_rdma_ex.h"

#define DDP_IRQ_EER_ID			(0xFFFF0000)
#define DDP_IRQ_FPS_ID			(DDP_IRQ_EER_ID + 1)
#define DDP_IRQ_LAYER_FPS_ID		(DDP_IRQ_EER_ID + 2)
#define DDP_IRQ_LAYER_SIZE_ID		(DDP_IRQ_EER_ID + 3)
#define DDP_IRQ_LAYER_FORMAT_ID		(DDP_IRQ_EER_ID + 4)

#define MAX_PATH_NUM (3)
#define OVL_NUM (2)
#define RDMA_NUM (3)
#define MAX_OVL_LAYERS (4)

unsigned int met_tag_on = 0;

static const char *const parse_color_format(DpColorFormat fmt)
{
	switch (fmt) {
	case eBGR565:
		return "eBGR565";
	case eRGB565:
		return "eRGB565";
	case eRGB888:
		return "eRGB888";
	case eBGR888:
		return "eBGR888";
	case eRGBA8888:
		return "eRGBA8888";
	case eBGRA8888:
		return "eBGRA8888";
	case eARGB8888:
		return "eARGB8888";
	case eABGR8888:
		return "eABGR8888";
	case eVYUY:
		return "eVYUY";
	case eUYVY:
		return "eUYVY";
	case eYVYU:
		return "eYVYU";
	case eYUY2:
		return "eYUY2";
	default:
		return "";
	}
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
	static unsigned long sBufAddr[RDMA_NUM];

	static RDMA_BASIC_STRUCT rdmaInfo;
	char tag_name[30] = { '\0' };

	rdma_get_info(index, &rdmaInfo);
	if (rdmaInfo.addr == 0 || (rdmaInfo.addr != 0 && sBufAddr[index] != rdmaInfo.addr)) {
		sBufAddr[index] = rdmaInfo.addr;
		sprintf(tag_name, index ? "ExtDispRefresh" : "PrimDispRefresh");
		met_tag_oneshot(DDP_IRQ_FPS_ID, tag_name, 1);
	}
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
	static unsigned long sLayerBufAddr[OVL_NUM][OVL_LAYER_NUM_PER_OVL];
	static unsigned int sLayerBufFmt[OVL_NUM][OVL_LAYER_NUM_PER_OVL];
	static unsigned int sLayerBufWidth[OVL_NUM][OVL_LAYER_NUM_PER_OVL];
	static unsigned int sLayerBufHeight[OVL_NUM][OVL_LAYER_NUM_PER_OVL];

	OVL_BASIC_STRUCT ovlInfo[OVL_LAYER_NUM];
	unsigned int i, idx, enLayerCnt, layerCnt;
	char tag_name[30] = { '\0' };

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
			if (sLayerBufAddr[index][idx] != ovlInfo[i].addr) {
				sLayerBufAddr[index][idx] = ovlInfo[i].addr;
				sprintf(tag_name, "OVL%dL%d_InFps", index, idx);
				met_tag_oneshot(DDP_IRQ_LAYER_FPS_ID, tag_name, i + 1);
			}
			if (sLayerBufFmt[index][idx] != ovlInfo[i].fmt) {
				sLayerBufFmt[index][idx] = ovlInfo[i].fmt;
				sprintf(tag_name, "OVL%dL%d_Fmt_%s", index, idx, parse_color_format(ovlInfo[i].fmt));
				met_tag_oneshot(DDP_IRQ_LAYER_FORMAT_ID, tag_name, i + 1);
			}
			if (sLayerBufWidth[index][idx] != ovlInfo[i].src_w) {
				sLayerBufWidth[index][idx] = ovlInfo[i].src_w;
				sprintf(tag_name, "OVL%dL%d_Width", index, idx);
				met_tag_oneshot(DDP_IRQ_LAYER_SIZE_ID, tag_name, ovlInfo[i].src_w);
			}
			if (sLayerBufHeight[index][idx] != ovlInfo[i].src_h) {
				sLayerBufHeight[index][idx] = ovlInfo[i].src_h;
				sprintf(tag_name, "OVL%dL%d_Height", index, idx);
				met_tag_oneshot(DDP_IRQ_LAYER_SIZE_ID, tag_name, ovlInfo[i].src_h);
			}

		} else {
			sLayerBufAddr[index][idx] = 0;
			sLayerBufFmt[index][idx] = 0;
			sLayerBufWidth[index][idx] = 0;
			sLayerBufHeight[index][idx] = 0;
		}

		if ((i == (OVL_LAYER_NUM_PER_OVL - 1)) || (i == (OVL_LAYER_NUM - 1))) {
			if (enLayerCnt) {
				enLayerCnt = 0;
				sprintf(tag_name, "OVL%d_OutFps", index);
				met_tag_oneshot(DDP_IRQ_LAYER_FPS_ID, tag_name, index);
			}
		}
	}

}

static void ddp_err_irq_met_tag(const char *name)
{
	met_tag_oneshot(DDP_IRQ_EER_ID, name, 0);
}

static void met_irq_handler(DISP_MODULE_ENUM module, unsigned int reg_val)
{
	int index = 0;
	char tag_name[30] = { '\0' };

	/* DDPERR("met_irq_handler() module=%d, val=0x%x\n", module, reg_val); */
	switch (module) {
	case DISP_MODULE_RDMA0:
	case DISP_MODULE_RDMA1:
		index = module - DISP_MODULE_RDMA0;
		if (reg_val & (1 << 2))
			ddp_disp_refresh_tag_end(index);

		if (reg_val & (1 << 1))
			ddp_disp_refresh_tag_start(index);

		if (reg_val & (1 << 4)) {
			sprintf(tag_name, "rdma%d_underflow", index);
			ddp_err_irq_met_tag(tag_name);
		}
		if (reg_val & (1 << 3)) {
			sprintf(tag_name, "rdma%d_abnormal", index);
			ddp_err_irq_met_tag(tag_name);
		}
		break;

	case DISP_MODULE_OVL0:
	case DISP_MODULE_OVL1:
		index = module - DISP_MODULE_OVL0;
		if (reg_val & (1 << 1))
			ddp_inout_info_tag(index);

		break;
	default:
		break;
	}
}

void ddp_init_met_tag(int state, int rdma0_mode, int rdma1_mode)
{
	if ((!met_tag_on) && state) {
		met_tag_on = state;
		disp_register_irq_callback(met_irq_handler);
	}
	if (met_tag_on && (!state)) {
		met_tag_on = state;
		disp_unregister_irq_callback(met_irq_handler);
	}
}
