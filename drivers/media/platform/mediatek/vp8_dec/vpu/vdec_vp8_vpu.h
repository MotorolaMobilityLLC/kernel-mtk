/*
 * Copyright (c) 2015 MediaTek Inc.
 * Author: Jungchang Tsao <jungchang.tsao@mediatek.com>
 *	   PC Chen <pc.chen@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _VDEC_VP8_VPU_H_
#define _VDEC_VP8_VPU_H_

int vdec_vp8_vpu_init(void *vdec_inst, unsigned int data);
int vdec_vp8_vpu_dec_start(void *vdec_inst);
int vdec_vp8_vpu_dec_end(void *vdec_inst);
int vdec_vp8_vpu_reset(void *vdec_inst);
int vdec_vp8_vpu_deinit(void *vdec_inst);

#endif
