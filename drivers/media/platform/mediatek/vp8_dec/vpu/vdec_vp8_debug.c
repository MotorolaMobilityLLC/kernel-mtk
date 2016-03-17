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

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/genalloc.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/kfifo.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/videodev2.h>
#include <linux/of.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/unistd.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/mm.h>
#include <linux/uaccess.h>

#include "vdec_vp8_if.h"

#ifdef DBG_VDEC_VP8

#ifdef DBG_VDEC_VP8_DUMP_DISPLAY_FRAME

static void write_dump_file(char *file_name, void *va, unsigned int sz)
{
	mm_segment_t oldfs;
	struct file *dump_file;

	dump_file = filp_open(file_name, O_RDWR | O_CREAT, 0);
	if (dump_file == NULL || IS_ERR(dump_file)) {
		pr_emerg("cannot open dump file %s\n", file_name);
		return;
	}

	oldfs = get_fs();
	set_fs(KERNEL_DS);
	if (dump_file)
		dump_file->f_op->write(dump_file, va, sz, &dump_file->f_pos);
	set_fs(oldfs);
	filp_close(dump_file, 0);
}

void vdec_vp8_dump_display_frame(void *vdec_inst, void *disp_fb)
{
	char f[256];
	struct vdec_vp8_inst *inst = vdec_inst;
	struct vdec_fb *fb = disp_fb;
	unsigned int y_sz;
	unsigned int c_offset, c_sz;
	unsigned num = inst->disp_total;

#ifdef VDEC_UFO_SUPPORT

	unsigned int pic_w = ((inst->vsi->pic.pic_w + 63) >> 6) << 6;
	unsigned int pic_h = ((inst->vsi->pic.pic_h + 63) >> 6) << 6;
	unsigned int pic_sz_y = pic_w * pic_h;
	unsigned int ufo_len_sz_y = ((((pic_sz_y + 255) >> 8) + 63 + (16*8))
				     >> 6) << 6;

#ifdef Y_C_SEPARATE

	/* write Y */
	sprintf(f, "/tmp/sdcard/dump_display/vp8Output_%d_bits_Y.out", num);
	y_sz = pic_sz_y;
	write_dump_file(f, fb->base_y.va, y_sz);

	sprintf(f, "/tmp/sdcard/dump_display/vp8Output_%d_len_Y.out", num);
	y_sz = ufo_len_sz_y;
	write_dump_file(f, fb->base_y.va + pic_sz_y, y_sz);

	/*write CbCr */
	sprintf(f, "/tmp/sdcard/dump_display/vp8Output_%d_bits_CbCr.out", num);
	c_sz  = pic_sz_y >> 1;
	write_dump_file(f, fb->base_c.va, c_sz);

	c_sz = (((ufo_len_sz_y >> 1) + 15 + (16 * 8)) >> 4) << 4;
	c_offset = pic_sz_y >> 1;
	sprintf(f, "/tmp/sdcard/dump_display/vp8Output_%d_len_CbCr.out", num);
	write_dump_file(f, fb->base_c.va + c_offset, c_sz);
#else
	unsigned int ufo_pic_sz_y_bs = (((pic_sz_y + 4095) >> 12) << 12);
	unsigned int ufo_pic_sz_bs = ((ufo_pic_sz_y_bs + (pic_sz_y >> 1) + 511)
				     >> 9) << 9;

	/* write Y */
	sprintf(f, "/tmp/sdcard/dump_display/vp8Output_%d_bits_Y.out", num);
	y_sz = ufo_pic_sz_y_bs;
	write_dump_file(f, fb->base.va, y_sz);

	sprintf(f, "/tmp/sdcard/dump_display/vp8Output_%d_len_Y.out", num);
	y_sz = ufo_len_sz_y;
	write_dump_file(f, fb->base.va + ufo_pic_sz_bs, y_sz);

	/*write CbCr */
	sprintf(f, "/tmp/sdcard/dump_display/vp8Output_%d_bits_CbCr.out", num);
	c_sz = ufo_pic_sz_bs - ufo_pic_sz_y_bs;
	write_dump_file(f, fb->base.va + ufo_pic_sz_y_bs, c_sz);

	sprintf(f, "/tmp/sdcard/dump_display/vp8Output_%d_len_CbCr.out", num);
	c_sz = (((ufo_len_sz_y >> 1) + 15 + (16 * 8)) >> 4) << 4;
	c_offset = ufo_pic_sz_bs + ufo_len_sz_y;
	write_dump_file(f, fb->base.va + c_offset, c_sz);
#endif

#else

#ifdef Y_C_SEPARATE
	/* write Y */
	sprintf(f, "/tmp/sdcard/dump_display/vp8Output_%d_Y.out", num);
	y_sz = inst->vsi->pic.buf_w * inst->vsi->pic.buf_h;
	write_dump_file(f, fb->base_y.va, y_sz);

	/* write CbCr */
	sprintf(f, "/tmp/sdcard/dump_display/vp8Output_%d_CbCr.out", num);
	c_offset = 0;
	c_sz = y_sz/2;
	write_dump_file(f, fb->base_c.va + c_offset, c_sz);
#else
	/* write Y */
	sprintf(f, "/tmp/sdcard/dump_display/vp8Output_%d_Y.out", num);
	y_sz = inst->vsi->pic.buf_w * inst->vsi->pic.buf_h;
	write_dump_file(f, fb->base.va, y_sz);

	/* write CbCr */
	sprintf(f, "/tmp/sdcard/dump_display/vp8Output_%d_CbCr.out", num);
	c_offset = y_sz;
	c_sz = y_sz/2;
	write_dump_file(f, fb->base.va + c_offset, c_sz);
#endif

#endif

	inst->disp_total++;
}
#endif

#endif
