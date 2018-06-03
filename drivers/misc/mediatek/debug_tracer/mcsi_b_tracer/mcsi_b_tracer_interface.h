/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#ifndef __MCSI_B_TRACER_H__
#define __MCSI_B_TRACER_H__

#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/compiler.h>

#define MCSI_TRACE_BP_NUM	4

struct mcsi_b_tracer_plt;

enum MCSI_TRACE_MODE {
	MCSI_NORMAL_TRACE_MODE,
	MCSI_SNAPSHOT_MODE,
};

struct mcsi_trace_filter {
	unsigned int low_content;
	unsigned int high_content;
	unsigned int low_content_enable;
	unsigned int high_content_enable;
};

struct mcsi_trace_trigger_point {
	unsigned int cfg_trigger_ctrl0;
	unsigned int cfg_trigger_ctrl1;

	unsigned int tri_lo_addr;
	unsigned int tri_hi_addr;
	unsigned int tri_lo_addr_mask;
	unsigned int tri_hi_addr_mask;
	unsigned int tri_other_match;
	unsigned int tri_other_match_mask;
};

struct mcsi_trace_port {
	unsigned char tp_sel;
	unsigned char tp_num;
};

struct mcsi_b_tracer_plt_operations {
	/* if platform needs special settings before */
	int (*start)(struct mcsi_b_tracer_plt *plt);
	/* dump for tracer */
	int (*dump)(struct mcsi_b_tracer_plt *plt, char *buf, int len);
	/* setup the mcsi_b_tracer functionality */
	int (*setup)(struct mcsi_b_tracer_plt *plt, unsigned char force, unsigned char enable);
	/* disable the mcsi_b_tracer functionality */
	int (*disable)(struct mcsi_b_tracer_plt *plt);
	/* pause/resume recording */
	int (*set_recording)(struct mcsi_b_tracer_plt *plt, unsigned char pause);
	/* setup tracer filter */
	int (*set_filter)(struct mcsi_b_tracer_plt *plt, struct mcsi_trace_filter f);
	/* setup trigger point */
	int (*set_trigger)(struct mcsi_b_tracer_plt *plt, struct mcsi_trace_trigger_point trigger);
	/* setup trace point */
	int (*set_port)(struct mcsi_b_tracer_plt *plt, struct mcsi_trace_port port);
	/* dump current setting of tracers */
	int (*dump_setting)(struct mcsi_b_tracer_plt *plt, char *buf, int len);
	/* if you want to do anything more than mcsi_b_tracer.c:mcsi_b_tracer_probe() */
	int (*probe)(struct mcsi_b_tracer_plt *plt, struct platform_device *pdev);
	/* if you want to do anything more than mcsi_b_tracer.c:mcsi_b_tracer_remove() */
	int (*remove)(struct mcsi_b_tracer_plt *plt, struct platform_device *pdev);
	/* if you want to do anything more than mcsi_b_tracer.c:mcsi_b_tracer_suspend() */
	int (*suspend)(struct mcsi_b_tracer_plt *plt, struct platform_device *pdev, pm_message_t state);
	/* if you want to do anything more than mcsi_b_tracer.c:mcsi_b_tracer_resume() */
	int (*resume)(struct mcsi_b_tracer_plt *plt, struct platform_device *pdev);
};


struct mcsi_b_tracer_plt {
	unsigned int min_buf_len;
	unsigned int trigger_counter;
	unsigned char enabled;
	unsigned char recording;
	struct mcsi_b_tracer_plt_operations *ops;
	struct mcsi_b_tracer *common;
	void __iomem *base;
	void __iomem *etb_base;
	void __iomem *dem_base;
	enum MCSI_TRACE_MODE mode;
	struct mcsi_trace_port trace_port;
	struct mcsi_trace_trigger_point trigger_point;
	struct mcsi_trace_filter filter;
};

struct mcsi_b_tracer {
	struct platform_driver plt_drv;
	struct mcsi_b_tracer_plt *cur_plt;
};

/* for platform register their specific mcsi_b_tracer behaviors
 * (chip or various versions of mcsi_b_tracer)
 */
int mcsi_b_tracer_register(struct mcsi_b_tracer_plt *plt);

#endif /* end of __MCSI_B_TRACER_H__ */
