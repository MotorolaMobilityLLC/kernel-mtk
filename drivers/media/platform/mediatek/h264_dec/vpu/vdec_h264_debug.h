/*
 * Copyright (c) 2015 MediaTek Inc.
 * Author: PC Chen <pc.chen@mediatek.com>
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

#ifndef VDEC_H264_DEBUG_H_
#define VDEC_H264_DEBUG_H_

void vdec_h264_dump_file_open(void);
void vdec_h264_dump_file_close(void);
void vdec_h264_dump_input_file(unsigned char *data, unsigned int size);
void vdec_h264_dump_input_stream(unsigned long bs_va, unsigned int size);
void vdec_h264_dump_display_frame(void *vdec_inst, void **disp_fb);
void vdec_h264_dump_fb_count(unsigned long h_inst);
unsigned int checksum(unsigned int *buf, int sz);

#endif
