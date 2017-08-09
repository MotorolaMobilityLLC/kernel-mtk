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

#include "mtk_vpu_core.h"

#include "vdec_h264_if.h"
#include "vdec_h264_vpu.h"

#ifdef DBG_VDEC_H264

void vdec_h264_dump_file_open(void)
{
#ifdef DBG_VDEC_H264_DUMP_INPUT_FILE
	input_file = filp_open(INPUT_FILE_PATH, O_RDWR | O_CREAT, 0);
	if (input_file == NULL || IS_ERR(input_file)) {
		pr_err("cannot open input dump file %s\n", INPUT_FILE_PATH);
#endif
}

void vdec_h264_dump_file_close(void)
{
#ifdef DBG_VDEC_H264_DUMP_INPUT_FILE
	filp_close(input_file, 0);
#endif
}

void vdec_h264_dump_input_file(unsigned char *data, unsigned int size)
{
	mm_segment_t oldfs;

	oldfs = get_fs();
	set_fs(KERNEL_DS);

#ifdef DBG_VDEC_H264_DUMP_INPUT_FILE
	if (input_file)
		input_file->f_op->write(input_file, data, size,
					&input_file->f_pos);
#endif
	set_fs(oldfs);
}

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

#ifdef DBG_VDEC_H264_DUMP_DISPLAY_FRAME
void vdec_h264_dump_display_frame(void *vdec_inst, void **disp_fb)
{
	char f[256];
	struct vdec_h264_inst *inst = vdec_inst;
	struct vdec_fb *fb;
	unsigned int y_sz;
	unsigned num;
	unsigned int pic_w = ((inst->vsi->pic.pic_w + 15) >> 4) << 4;
	unsigned int pic_h = ((inst->vsi->pic.pic_h + 31) >> 5) << 5;
	unsigned int pic_sz_y = ((pic_w * pic_h + 511) >> 9) << 9;

#ifdef VDEC_UFO_SUPPORT
	unsigned int ufo_len_sz_y = ((((pic_sz_y + 255) >> 8) + 63 +
				    (16 * 8)) >> 6) << 6;
#ifdef Y_C_SEPARATE
	unsigned int pic_sz_c = pic_sz_y >> 1;
	unsigned int ufo_len_sz_c = (((ufo_len_sz_y >> 1) + 15 +
				    (16 * 8)) >> 4) << 4;
#else
	unsigned int pic_sz_y_bs = (((pic_sz_y + 4095) >> 12) << 12);
	unsigned int pic_sz_bs = ((pic_sz_y_bs + (pic_sz_y >> 1) + 511) >> 9)
				 << 9;
	unsigned int ufo_pic_sz_bs = pic_sz_bs;
	unsigned int ufo_pic_sz_y_bs = pic_sz_y_bs;
	unsigned int c_offset, c_sz;
#endif
#endif
	if (*disp_fb == NULL)
		return;

	fb = *disp_fb;
	num = inst->vsi->list_disp.reserved;

#ifdef DBG_DUMP_UFO_MODE

#ifdef Y_C_SEPARATE

	/* write Y */
	sprintf(f, "/tmp/sdcard/dump_display/h264Output_%d_bits_Y.out", num);
	y_sz = pic_sz_y;
	write_dump_file(f, fb->base_y.va, y_sz);

	sprintf(f, "/tmp/sdcard/dump_display/h264Output_%d_len_Y.out", num);
	y_sz = ufo_len_sz_y;
	write_dump_file(f, fb->base_y.va + pic_sz_y, y_sz);

	/*write CbCr */
	sprintf(f, "/tmp/sdcard/dump_display/h264Output_%d_bits_CbCr.out", num);
	write_dump_file(f, fb->base_c.va, pic_sz_c);

	sprintf(f, "/tmp/sdcard/dump_display/h264Output_%d_len_CbCr.out", num);
	write_dump_file(f, fb->base_c.va + pic_sz_c, ufo_len_sz_c);
#else
	/* write Y */
	sprintf(f, "/tmp/sdcard/dump_display/h264Output_%d_bits_Y.out", num);
	y_sz = ufo_pic_sz_y_bs;
	write_dump_file(f, fb->base.va, y_sz);

	sprintf(f, "/tmp/sdcard/dump_display/h264Output_%d_len_Y.out", num);
	y_sz = ufo_len_sz_y;
	write_dump_file(f, fb->base.va + ufo_pic_sz_bs, y_sz);

	/*write CbCr */
	sprintf(f, "/tmp/sdcard/dump_display/h264Output_%d_bits_CbCr.out", num);
	y_sz = ufo_pic_sz_bs - ufo_pic_sz_y_bs;
	write_dump_file(f, fb->base.va + ufo_pic_sz_y_bs, y_sz);

	sprintf(f, "/tmp/sdcard/dump_display/h264Output_%d_len_CbCr.out", num);
	c_sz = (((ufo_len_sz_y >> 1) + 15 + (16 * 8)) >> 4) << 4;
	c_offset = ufo_pic_sz_bs + ufo_len_sz_y;
	write_dump_file(f, fb->base.va + c_offset, c_sz);
#endif

#else

#ifdef Y_C_SEPARATE
	/* write Y */
	sprintf(f, "/tmp/sdcard/dump_display/h264Output_%d_Y.out", num);
	y_sz = inst->vsi->pic.buf_w * inst->vsi->pic.buf_h;
	write_dump_file(f, fb->base_y.va, y_sz);

	/* write CbCr */
	sprintf(f, "/tmp/sdcard/dump_display/h264Output_%d_CbCr.out", num);
	write_dump_file(f, fb->base_c.va, y_sz/2);
#else
	/* write Y */
	sprintf(f, "/tmp/sdcard/dump_display/h264Output_%d_Y.out", num);
	y_sz = inst->vsi->pic.buf_w * inst->vsi->pic.buf_h;
	write_dump_file(f, fb->base.va, y_sz);

	/* write CbCr */
	sprintf(f, "/tmp/sdcard/dump_display/h264Output_%d_CbCr.out", num);
	write_dump_file(f, fb->base.va + y_sz, y_sz/2);
#endif

#endif

	inst->vsi->list_disp.reserved++;
}
#endif

void vdec_h264_dump_input_stream(unsigned long bs_va, unsigned int size)
{
	int sz = size;
	int count = 1;

	while (sz >= 6 && count <= 4) {
		pr_debug("Dump bs [%02x %02x %02x %02x %02x %02x]\n",
			 *((unsigned char *)(bs_va)),
			 *((unsigned char *)(bs_va+1)),
			 *((unsigned char *)(bs_va+2)),
			 *((unsigned char *)(bs_va+3)),
			 *((unsigned char *)(bs_va+4)),
			 *((unsigned char *)(bs_va+5)));
		bs_va += 6;
		count++;
		sz -= 6;
	}
}

unsigned int checksum(unsigned int *buf, int sz)
{
	unsigned int checksum = 0;

	while (sz > 0) {
		checksum += *buf++;
		sz -= 4;
	}

	return checksum;
}

void vdec_h264_dump_fb_count(unsigned long h_inst)
{
	int free_cnt = 0;
	int disp_cnt = 0;
	struct vdec_h264_inst *inst;

	inst = (struct vdec_h264_inst *)h_inst;
	free_cnt = inst->vsi->list_free.count;
	disp_cnt = inst->vsi->list_disp.count;

	pr_debug("[FB] free_cnt=%d disp_cnt=%d\n", free_cnt, disp_cnt);
}

static int h264_fwrite(char *buf, int len, struct file *filp)
{
	int writelen;
	mm_segment_t oldfs;

	if (filp == NULL)
		return -ENOENT;
	if (filp->f_op->write == NULL)
		return -ENOSYS;
	if (((filp->f_flags & O_ACCMODE) & (O_WRONLY | O_RDWR)) == 0)
		return -EACCES;

	oldfs = get_fs();
	set_fs(KERNEL_DS);
	writelen = filp->f_op->write(filp, buf, len, &filp->f_pos);
	set_fs(oldfs);

	return 0;
}

static void h264_fputs(char *str, struct file *filp)
{
	h264_fwrite(str, strlen(str), filp);
}

void h264_fprintf(struct file *filp, const char *fmt, ...)
{
	static char s_buf[1024];

	va_list args;

	va_start(args, fmt);
	vsprintf(s_buf, fmt, args);
	va_end(args);

	h264_fputs(s_buf, filp);
}

#endif
