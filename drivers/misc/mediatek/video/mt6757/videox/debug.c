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

#include <linux/string.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/fb.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <linux/debugfs.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/delay.h>
#include "m4u.h"
#include "disp_drv_log.h"
#include "mtkfb.h"
#include "debug.h"
#include "lcm_drv.h"
#include "ddp_ovl.h"
#include "ddp_path.h"
#include "ddp_reg.h"
#include "primary_display.h"
#include "display_recorder.h"
#ifdef CONFIG_MTK_LEGACY
#include <mach/mt_gpio.h>
#include <cust_gpio_usage.h>
#else
#include "disp_dts_gpio.h"
#endif
#ifdef CONFIG_MTK_CLKMGR
#include <mach/mt_clkmgr.h>
#endif
#include "mtkfb_fence.h"
#include "disp_helper.h"
#include "ddp_manager.h"
#include "ddp_log.h"
#include "ddp_dsi.h"

#include "cmdq_def.h"
#include "cmdq_record.h"
#include "cmdq_reg.h"
#include "cmdq_core.h"
#include "disp_lowpower.h"
#include "disp_recovery.h"
#include "disp_partial.h"

static struct dentry *mtkfb_dbgfs;
unsigned int g_mobilelog = 1;

static int draw_buffer(char *va, int w, int h,
		       enum UNIFIED_COLOR_FMT ufmt, char r, char g, char b, char a)
{
	int i, j;
	int Bpp = UFMT_GET_Bpp(ufmt);

	for (i = 0; i < h; i++)
		for (j = 0; j < w; j++) {
			int x = j * Bpp + i * w * Bpp;

			if (ufmt == UFMT_RGB888 || ufmt == UFMT_RGBA8888) {
				va[x++] = r;
				va[x++] = g;
				va[x++] = b;
				if (Bpp == 4)
					va[x++] = a;
			}

			if (ufmt == UFMT_RGB565) {
				va[x++] = (b & 0x1f) | ((g & 0x7) << 5);
				va[x++] = (g & 0x7) | (r & 0x1f);
			}
		}
	return 0;
}

static int primary_display_basic_test(int layer_num, int w, int h, DISP_FORMAT fmt, int frame_num,
				      int vsync)
{
	disp_session_input_config *input_config;
	int session_id = MAKE_DISP_SESSION(DISP_SESSION_PRIMARY, 0);
	unsigned int Bpp;
	int frame, i, ret;
	enum UNIFIED_COLOR_FMT ufmt;

	/* allocate buffer */
	unsigned long size;
	unsigned char *buf_va;
	dma_addr_t buf_pa;
	unsigned int buf_mva;
	unsigned long size_align;
	m4u_client_t *client = NULL;

	ufmt = disp_fmt_to_unified_fmt(fmt);
	Bpp = UFMT_GET_bpp(ufmt) / 8;
	size = w * h * Bpp;
	size_align = round_up(size, PAGE_SIZE);

	DISPMSG("%s: layer_num=%u,w=%d,h=%d,fmt=%s,frame_num=%d,vsync=%d, size=%lu\n",
		__func__, layer_num, w, h, unified_color_fmt_name(ufmt), frame_num, vsync, size);

	input_config = kmalloc(sizeof(*input_config), GFP_KERNEL);
	if (!input_config)
		return -ENOMEM;

	buf_va = dma_alloc_coherent(disp_get_device(), size, &buf_pa, GFP_KERNEL);
	if (!(buf_va)) {
		DISPMSG("dma_alloc_coherent error!  dma memory not available. size=%lu\n", size);
		kfree(input_config);
		return -1;
	}

	if (disp_helper_get_option(DISP_OPT_USE_M4U)) {
		static struct sg_table table;
		struct sg_table *sg_table = &table;

		sg_alloc_table(sg_table, 1, GFP_KERNEL);

		sg_dma_address(sg_table->sgl) = buf_pa;
		sg_dma_len(sg_table->sgl) = size_align;
		client = m4u_create_client();
		if (IS_ERR_OR_NULL(client))
			DISPMSG("create client fail!\n");

		ret = m4u_alloc_mva(client, M4U_PORT_DISP_OVL0, 0, sg_table, size_align,
				    M4U_PROT_READ | M4U_PROT_WRITE, 0, &buf_mva);
		if (ret)
			DISPMSG("m4u_alloc_mva returns fail: %d\n", ret);
		DDPMSG("%s MVA is 0x%x PA is 0x%pa\n", __func__, buf_mva, &buf_pa);
	}


	draw_buffer(buf_va, w, h, ufmt, 255, 0, 0, 255);

	for (frame = 0; frame < frame_num; frame++) {

		memset(input_config, 0, sizeof(*input_config));
		input_config->config_layer_num = layer_num;
		input_config->session_id = session_id;

		for (i = 0; i < layer_num; i++) {
			int enable;

			if (i == frame % (layer_num + 1) - 1)
				enable = 0;
			else
				enable = 1;

			input_config->config[i].layer_id = i;
			input_config->config[i].layer_enable = enable;
			input_config->config[i].src_base_addr = 0;
			if (disp_helper_get_option(DISP_OPT_USE_M4U))
				input_config->config[i].src_phy_addr = (void *)((unsigned long)buf_mva);
			else
				input_config->config[i].src_phy_addr = (void *)((unsigned long)buf_pa);
			input_config->config[i].next_buff_idx = -1;
			input_config->config[i].src_fmt = fmt;
			input_config->config[i].src_pitch = w;
			input_config->config[i].src_offset_x = 0;
			input_config->config[i].src_offset_y = 0;
			input_config->config[i].src_width = w;
			input_config->config[i].src_height = h;

			input_config->config[i].tgt_offset_x = w * i;
			input_config->config[i].tgt_offset_y = h * i;
			input_config->config[i].tgt_width = w;
			input_config->config[i].tgt_height = h;
			input_config->config[i].alpha_enable = 1;
			input_config->config[i].alpha = 0xff;
			input_config->config[i].security = DISP_NORMAL_BUFFER;
		}
		primary_display_config_input_multiple(input_config);
		primary_display_trigger(0, NULL, 0);

		if (vsync) {
			disp_session_vsync_config vsync_config;

			vsync_config.session_id = session_id;
			primary_display_wait_for_vsync(&vsync_config);
		}
	}

	/* disable all layers */
	memset(input_config, 0, sizeof(*input_config));
	input_config->config_layer_num = layer_num;
	for (i = 0; i < layer_num; i++)
		input_config->config[i].layer_id = i;

	primary_display_config_input_multiple(input_config);
	primary_display_trigger(1, NULL, 0);

	if (disp_helper_get_option(DISP_OPT_USE_M4U)) {
		/* dealloc mva */
		m4u_destroy_client(client);
	}

	dma_free_coherent(disp_get_device(), size, buf_va, buf_pa);
	kfree(input_config);
	return 0;
}

#if 0 /* defined but not used */
static char STR_HELP[] =
	"\n"
	"USAGE\n"
	"        echo [ACTION]... > /d/mtkfb\n"
	"\n"
	"        suspend\n"
	"             enter suspend mode\n"
	"\n"
	"        resume\n"
	"             leave suspend mode\n"
	"\n"
	"        lcm:[on|off|init]\n"
	"             power on/off lcm\n"
	"\n"
	"        cabc:[ui|mov|still]\n"
	"             cabc mode, UI/Moving picture/Still picture\n"
	"\n"
	"       esd:[on|off]\n"
	"             esd kthread on/off\n"
	"       HQA:[NormalToFactory|FactoryToNormal]\n"
	"             for HQA requirement\n"
	"\n"
	"       dump_layer:[on|off[,down_sample_x[,down_sample_y]][,layer(0:L0,1:L1,2:L2,3:L3,4:L0-3)]\n"
	"             Start/end to capture current enabled OVL layer every frame\n";
#endif

static void process_dbg_opt(const char *opt)
{
	int ret;

	if (0 == strncmp(opt, "helper", 6)) {
		/*ex: echo helper:DISP_OPT_BYPASS_OVL,0 > /d/mtkfb */
		char option[100] = "";
		char *tmp;
		int value, i;

		tmp = (char *)(opt + 7);
		for (i = 0; i < 100; i++) {
			if (tmp[i] != ',' && tmp[i] != ' ')
				option[i] = tmp[i];
			else
				break;
		}
		tmp += i + 1;
		ret = sscanf(tmp, "%d\n", &value);
		if (ret != 1) {
			pr_err("error to parse cmd %s: %s %s ret=%d\n", opt, option, tmp, ret);
			return;
		}

		DISPMSG("will set option %s to %d\n", option, value);
		disp_helper_set_option_by_name(option, value);
	} else if (0 == strncmp(opt, "switch_mode:", 12)) {
		int session_id = MAKE_DISP_SESSION(DISP_SESSION_PRIMARY, 0);
		int sess_mode;

		ret = sscanf(opt, "switch_mode:%d\n", &sess_mode);
		if (ret != 1) {
			pr_err("error to parse cmd %s\n", opt);
			return;
		}

		primary_display_switch_mode(sess_mode, session_id, 1);
	} else if (0 == strncmp(opt, "clk_change:", 11)) {
		char *p = (char *)opt + 11;
		unsigned int clk = 0;

		ret = kstrtouint(p, 0, &clk);
		if (ret) {
			pr_err("error to parse cmd %s\n", opt);
			return;
		}
		DISPCHECK("clk_change:%d\n", clk);
		primary_display_mipi_clk_change(clk);
	} else if (0 == strncmp(opt, "dsipattern", 10)) {
		char *p = (char *)opt + 11;
		unsigned int pattern;

		ret = kstrtouint(p, 0, &pattern);
		if (ret) {
			pr_err("error to parse cmd %s\n", opt);
			return;
		}

		if (pattern) {
			DSI_BIST_Pattern_Test(DISP_MODULE_DSI0, NULL, true, pattern);
			DISPMSG("enable dsi pattern: 0x%08x\n", pattern);
		} else {
			primary_display_manual_lock();
			DSI_BIST_Pattern_Test(DISP_MODULE_DSI0, NULL, false, 0);
			primary_display_manual_unlock();
			return;
		}
	} else if (0 == strncmp(opt, "mobile:", 7)) {
		if (0 == strncmp(opt + 7, "on", 2))
			g_mobilelog = 1;
		else if (0 == strncmp(opt + 7, "off", 3))
			g_mobilelog = 0;

	} else if (0 == strncmp(opt, "force_fps:", 9)) {
		unsigned int keep;
		unsigned int skip;

		ret = sscanf(opt, "force_fps:%d,%d\n", &keep, &skip);
		if (ret != 2) {
			pr_err("error to parse cmd %s\n", opt);
			return;
		}

		DISPMSG("force set fps, keep %d, skip %d\n", keep, skip);
		primary_display_force_set_fps(keep, skip);
	} else if (0 == strncmp(opt, "AAL_trigger", 11)) {
		int i = 0;
		disp_session_vsync_config vsync_config;

		for (i = 0; i < 1200; i++) {
			primary_display_wait_for_vsync(&vsync_config);
			dpmgr_module_notify(DISP_MODULE_AAL0, DISP_PATH_EVENT_TRIGGER);
		}
#ifdef CONFIG_MTK_DISPLAY_120HZ_SUPPORT
	} else if (0 == strncmp(opt, "odbypass:", 9)) {
		char *p = (char *)opt + 9;
		int bypass = kstrtoul(p, 16, (unsigned long int *)&p);

		primary_display_od_bypass(bypass);
		DISPMSG("OD bypass: %d\n", bypass);
		return;
#endif
	} else if (0 == strncmp(opt, "diagnose", 8)) {
		primary_display_diagnose();
		return;
	} else if (0 == strncmp(opt, "_efuse_test", 11)) {
		primary_display_check_test();
	} else if (0 == strncmp(opt, "dprec_reset", 11)) {
		dprec_logger_reset_all();
		return;
	} else if (0 == strncmp(opt, "suspend", 7)) {
		primary_display_suspend();
		return;
	} else if (0 == strncmp(opt, "resume", 6)) {
		primary_display_resume();
	} else if (0 == strncmp(opt, "ata", 3)) {
		mtkfb_fm_auto_test();
		return;
	} else if (0 == strncmp(opt, "dalprintf", 9)) {
		DAL_Printf("display aee layer test\n");
	} else if (0 == strncmp(opt, "dalclean", 8)) {
		DAL_Clean();
	} else if (0 == strncmp(opt, "daltest", 7)) {
		int i = 1000;

		while (i--) {
			DAL_Printf("display aee layer test\n");
			msleep(20);
			DAL_Clean();
			msleep(20);
		}
	} else if (0 == strncmp(opt, "lfr_setting:", 12)) {
		unsigned int enable;
		unsigned int mode;
		/* unsigned int  mode=3; */
		unsigned int type = 0;
		unsigned int skip_num = 1;

		ret = sscanf(opt, "lfr_setting:%d,%d\n", &enable, &mode);
		if (ret != 2) {
			pr_err("error to parse cmd %s\n", opt);
			return;
		}
		DDPMSG("--------------enable/disable lfr--------------\n");
		if (enable) {
			DDPMSG("lfr enable %d mode =%d\n", enable, mode);
			enable = 1;
			DSI_Set_LFR(DISP_MODULE_DSI0, NULL, mode, type, enable, skip_num);
		} else {
			DDPMSG("lfr disable %d mode=%d\n", enable, mode);
			enable = 0;
			DSI_Set_LFR(DISP_MODULE_DSI0, NULL, mode, type, enable, skip_num);
		}
	} else if (0 == strncmp(opt, "vsync_switch:", 13)) {
		char *p = (char *)opt + 13;
		unsigned int method = 0;

		ret = kstrtouint(p, 0, &method);
		if (ret) {
			pr_err("error to parse cmd %s\n", opt);
			return;
		}
		primary_display_vsync_switch(method);

	} else if (0 == strncmp(opt, "dsi0_clk:", 9)) {
		char *p = (char *)opt + 9;
		UINT32 clk;

		ret = kstrtouint(p, 0, &clk);
		if (ret) {
			pr_err("error to parse cmd %s\n", opt);
			return;
		}
	} else if (0 == strncmp(opt, "dst_switch:", 11)) {
		char *p = (char *)opt + 11;
		UINT32 mode;

		ret = kstrtouint(p, 0, &mode);
		if (ret) {
			pr_err("error to parse cmd %s\n", opt);
			return;
		}
		primary_display_switch_dst_mode(mode % 2);
		return;
	} else if (0 == strncmp(opt, "cv_switch:", 10)) {
		char *p = (char *)opt + 10;
		UINT32 mode;

		ret = kstrtouint(p, 0, &mode);
		if (ret) {
			pr_err("error to parse cmd %s\n", opt);
			return;
		}
		disp_helper_set_option(DISP_OPT_CV_BYSUSPEND, mode % 2);
		return;
	} else if (0 == strncmp(opt, "cmmva_dprec", 11)) {
		dprec_handle_option(0x7);
	} else if (0 == strncmp(opt, "cmmpa_dprec", 11)) {
		dprec_handle_option(0x3);
	} else if (0 == strncmp(opt, "dprec", 5)) {
		char *p = (char *)opt + 6;
		unsigned int option;

		ret = kstrtouint(p, 0, &option);
		if (ret) {
			pr_err("error to parse cmd %s\n", opt);
			return;
		}
		dprec_handle_option(option);
	} else if (0 == strncmp(opt, "maxlayer", 8)) {
		char *p = (char *)opt + 9;
		unsigned int maxlayer;

		ret = kstrtouint(p, 0, &maxlayer);
		if (ret) {
			pr_err("error to parse cmd %s\n", opt);
			return;
		}

		if (maxlayer)
			primary_display_set_max_layer(maxlayer);
		else
			DISPERR("can't set max layer to 0\n");
	} else if (0 == strncmp(opt, "primary_reset", 13)) {
		primary_display_reset();
	} else if (0 == strncmp(opt, "esd_check", 9)) {
		char *p = (char *)opt + 10;
		unsigned int enable;

		ret = kstrtouint(p, 0, &enable);
		if (ret) {
			pr_err("error to parse cmd %s\n", opt);
			return;
		}
		primary_display_esd_check_enable(enable);
	} else if (0 == strncmp(opt, "esd_recovery", 12)) {
		primary_display_esd_recovery();
	} else if (0 == strncmp(opt, "lcm0_reset", 10)) {
		DISPCHECK("lcm0_reset\n");
#if 1
		DISP_CPU_REG_SET(DISPSYS_CONFIG_BASE + 0x150, 1);
		msleep(20);
		DISP_CPU_REG_SET(DISPSYS_CONFIG_BASE + 0x150, 0);
		msleep(20);
		DISP_CPU_REG_SET(DISPSYS_CONFIG_BASE + 0x150, 1);
#else
#ifdef CONFIG_MTK_LEGACY
		mt_set_gpio_mode(GPIO106 | 0x80000000, GPIO_MODE_00);
		mt_set_gpio_dir(GPIO106 | 0x80000000, GPIO_DIR_OUT);
		mt_set_gpio_out(GPIO106 | 0x80000000, GPIO_OUT_ONE);
		msleep(20);
		mt_set_gpio_out(GPIO106 | 0x80000000, GPIO_OUT_ZERO);
		msleep(20);
		mt_set_gpio_out(GPIO106 | 0x80000000, GPIO_OUT_ONE);
#else
		ret = disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_RST_OUT1);
		msleep(20);
		ret |= disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_RST_OUT0);
		msleep(20);
		ret |= disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_RST_OUT1);
#endif
#endif
	} else if (0 == strncmp(opt, "lcm0_reset0", 11)) {
		DISP_CPU_REG_SET(DDP_REG_BASE_MMSYS_CONFIG + 0x150, 0);
	} else if (0 == strncmp(opt, "lcm0_reset1", 11)) {
		DISP_CPU_REG_SET(DDP_REG_BASE_MMSYS_CONFIG + 0x150, 1);
	} else if (0 == strncmp(opt, "dump_layer:", 11)) {
		if (0 == strncmp(opt + 11, "on", 2)) {
			ret = sscanf(opt, "dump_layer:on,%d,%d,%d\n",
				     &gCapturePriLayerDownX, &gCapturePriLayerDownY, &gCapturePriLayerNum);
			if (ret != 3) {
				pr_err("error to parse cmd %s\n", opt);
				return;
			}

			gCapturePriLayerEnable = 1;
			gCaptureWdmaLayerEnable = 1;
			if (gCapturePriLayerDownX == 0)
				gCapturePriLayerDownX = 20;
			if (gCapturePriLayerDownY == 0)
				gCapturePriLayerDownY = 20;
			DDPMSG("dump_layer En %d DownX %d DownY %d,Num %d", gCapturePriLayerEnable,
			       gCapturePriLayerDownX, gCapturePriLayerDownY, gCapturePriLayerNum);

		} else if (0 == strncmp(opt + 11, "off", 3)) {
			gCapturePriLayerEnable = 0;
			gCaptureWdmaLayerEnable = 0;
			gCapturePriLayerNum = TOTAL_OVL_LAYER_NUM;
			DDPMSG("dump_layer En %d\n", gCapturePriLayerEnable);
		}

	} else if (0 == strncmp(opt, "dump_wdma_layer:", 16)) {
		if (0 == strncmp(opt + 16, "on", 2)) {
			ret = sscanf(opt, "dump_wdma_layer:on,%d,%d\n",
				     &gCapturePriLayerDownX, &gCapturePriLayerDownY);
			if (ret != 2) {
				pr_err("error to parse cmd %s\n", opt);
				return;
			}

			gCaptureWdmaLayerEnable = 1;
			if (gCapturePriLayerDownX == 0)
				gCapturePriLayerDownX = 20;
			if (gCapturePriLayerDownY == 0)
				gCapturePriLayerDownY = 20;
			DDPMSG("dump_wdma_layer En %d DownX %d DownY %d", gCaptureWdmaLayerEnable,
			       gCapturePriLayerDownX, gCapturePriLayerDownY);

		} else if (0 == strncmp(opt + 16, "off", 3)) {
			gCaptureWdmaLayerEnable = 0;
			DDPMSG("dump_layer En %d\n", gCaptureWdmaLayerEnable);
		}
	} else if (0 == strncmp(opt, "dump_rdma_layer:", 16)) {
#if defined(CONFIG_MT_ENG_BUILD) || !defined(CONFIG_MTK_GMO_RAM_OPTIMIZE)
		if (0 == strncmp(opt + 16, "on", 2)) {
			ret = sscanf(opt, "dump_rdma_layer:on,%d,%d\n",
				     &gCapturePriLayerDownX, &gCapturePriLayerDownY);
			if (ret != 2) {
				pr_err("error to parse cmd %s\n", opt);
				return;
			}

			gCaptureRdmaLayerEnable = 1;
			if (gCapturePriLayerDownX == 0)
				gCapturePriLayerDownX = 20;
			if (gCapturePriLayerDownY == 0)
				gCapturePriLayerDownY = 20;
			DDPMSG("dump_wdma_layer En %d DownX %d DownY %d", gCaptureRdmaLayerEnable,
			       gCapturePriLayerDownX, gCapturePriLayerDownY);

		} else if (0 == strncmp(opt + 16, "off", 3)) {
			gCaptureRdmaLayerEnable = 0;
			DDPMSG("dump_layer En %d\n", gCaptureRdmaLayerEnable);
		}
#endif
	} else if (0 == strncmp(opt, "enable_idlemgr:", 15)) {
		char *p = (char *)opt + 15;
		UINT32 flg;

		ret = kstrtouint(p, 0, &flg);
		if (ret) {
			pr_err("error to parse cmd %s\n", opt);
			return;
		}
		enable_idlemgr(flg);
	} else if (0 == strncmp(opt, "fps:", 4)) {
		char *p = (char *)opt+4;
		int fps = kstrtoul(p, 10, (unsigned long int *)&p);

		DDPMSG("change fps\n");
		primary_display_set_lcm_refresh_rate(fps);
		return;
	} else if (0 == strncmp(opt, "disp_mode:", 10)) {
		char *p = (char *)opt + 10;
		unsigned long int disp_mode = 0;

		ret = kstrtoul(p, 10, &disp_mode);
		gTriggerDispMode = (int)disp_mode;
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		DISPMSG("DDP: gTriggerDispMode=%d\n", gTriggerDispMode);
	}

	if (0 == strncmp(opt, "primary_basic_test:", 19)) {
		int layer_num, w, h, fmt, frame_num, vsync;

		ret = sscanf(opt, "primary_basic_test:%d,%d,%d,%d,%d,%d\n",
			     &layer_num, &w, &h, &fmt, &frame_num, &vsync);
		if (ret != 6) {
			pr_err("error to parse cmd %s\n", opt);
			return;
		}

		if (fmt == 0)
			fmt = DISP_FORMAT_RGBA8888;
		else if (fmt == 1)
			fmt = DISP_FORMAT_RGB888;
		else if (fmt == 2)
			fmt = DISP_FORMAT_RGB565;

		primary_display_basic_test(layer_num, w, h, fmt, frame_num, vsync);
	}

	if (0 == strncmp(opt, "pan_disp_test:", 13)) {
		int frame_num;
		int bpp;

		ret = sscanf(opt, "pan_disp_test:%d,%d\n", &frame_num, &bpp);
		if (ret != 2) {
			pr_err("error to parse cmd %s\n", opt);
			return;
		}

		pan_display_test(frame_num, bpp);
	}

	if (0 == strncmp(opt, "scenario:", 8)) {
		int scen;

		ret = sscanf(opt, "scenario:%d\n", &scen);
		if (ret != 1) {
			pr_err("error to parse cmd %s\n", opt);
			return;
		}
		primary_display_set_scenario(scen);
	}

}


static void process_dbg_cmd(char *cmd)
{
	char *tok;

	DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG", "[mtkfb_dbg] %s\n", cmd);

	while ((tok = strsep(&cmd, " ")) != NULL)
		process_dbg_opt(tok);
}

static int debug_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

int debug_get_info(unsigned char *stringbuf, int buf_len)
{
	int n = 0;

	DISPFUNC();

	n += mtkfb_get_debug_state(stringbuf + n, buf_len - n);

	n += primary_display_get_debug_state(stringbuf + n, buf_len - n);

	n += disp_sync_get_debug_info(stringbuf + n, buf_len - n);

	n += dprec_logger_get_result_string_all(stringbuf + n, buf_len - n);

	n += disp_helper_get_option_list(stringbuf + n, buf_len - n);

	n += dprec_logger_get_buf(DPREC_LOGGER_ERROR, stringbuf + n, buf_len - n);

	n += dprec_logger_get_buf(DPREC_LOGGER_FENCE, stringbuf + n, buf_len - n);

	n += dprec_logger_get_buf(DPREC_LOGGER_DUMP, stringbuf + n, buf_len - n);

	n += dprec_logger_get_buf(DPREC_LOGGER_DEBUG, stringbuf + n, buf_len - n);

	n += dprec_logger_get_buf(DPREC_LOGGER_STATUS, stringbuf + n, buf_len - n);

	stringbuf[n++] = 0;
	return n;
}

void debug_info_dump_to_printk(char *buf, int buf_len)
{
	int i = 0;
	int n = buf_len;

	for (i = 0; i < n; i += 256)
		DISPMSG("%s", buf + i);
}

static ssize_t debug_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
	int debug_bufmax;
	char *str = "idlemgr disable mtcmos now, all the regs may 0x00000000\n";
	static int n;

	/* Debugfs read only fetch 4096 byte each time, thus whole ringbuffer need massive
	 * iteration. We only copy ringbuffer content to debugfs buffer at first time (*ppos = 0)
	 */
	if (*ppos != 0 || !is_buffer_init)
		goto out;

	DISPFUNC();

	debug_bufmax = DEBUG_BUFFER_SIZE - 1;
	n = debug_get_info(debug_buffer, debug_bufmax);
	/* debug_info_dump_to_printk(); */
out:
	if (is_mipi_enterulps())
		return simple_read_from_buffer(ubuf, count, ppos, str, strlen(str));
	else
		return simple_read_from_buffer(ubuf, count, ppos, debug_buffer, n);
}

static ssize_t debug_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos)
{
	const int debug_bufmax = 1024 - 1;
	size_t ret;
	char cmd_buffer[1024];

	ret = count;

	if (count > debug_bufmax)
		count = debug_bufmax;

	if (copy_from_user(&cmd_buffer, ubuf, count))
		return -EFAULT;

	cmd_buffer[count] = 0;

	process_dbg_cmd(cmd_buffer);

	return ret;
}


static const struct file_operations debug_fops = {
	.read = debug_read,
	.write = debug_write,
	.open = debug_open,
};

static ssize_t kick_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
	return simple_read_from_buffer(ubuf, count, ppos, get_kick_dump(), get_kick_dump_size());
}

static const struct file_operations kickidle_fops = {
	.read = kick_read,
};

static ssize_t partial_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
	char p[10];
	int support = 0;
	struct disp_rect roi = {0, 0, 0, 0};

	if (primary_display_partial_support()) {
		if (!ddp_debug_force_roi()) {
			support = 1;
		} else {
			roi.x = ddp_debug_force_roi_x();
			roi.y = ddp_debug_force_roi_y();
			roi.width = ddp_debug_force_roi_w();
			roi.height = ddp_debug_force_roi_h();
			if (!is_equal_full_lcm(&roi))
				support = 1;
		}
	}
	snprintf(p, 10, "%d\n", support);
	return simple_read_from_buffer(ubuf, count, ppos, p, strlen(p));
}

static const struct file_operations partial_fops = {
	.read = partial_read,
};

void DBG_Init(void)
{
	struct dentry *d_folder;
	struct dentry *d_file;

	mtkfb_dbgfs = debugfs_create_file("mtkfb", S_IFREG | S_IRUGO, NULL, (void *)0, &debug_fops);

	d_folder = debugfs_create_dir("displowpower", NULL);
	if (d_folder) {
		d_file = debugfs_create_file("kickdump", S_IFREG | S_IRUGO, d_folder, NULL, &kickidle_fops);
		d_file = debugfs_create_file("partial", S_IFREG | S_IRUGO, d_folder, NULL, &partial_fops);
	}
}

void DBG_Deinit(void)
{
	debugfs_remove(mtkfb_dbgfs);
}
