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

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/module.h>
#include <generated/autoconf.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/param.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/workqueue.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/module.h>
#include <linux/wait.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/compat.h>

#include "m4u.h"

#include "mtk_sync.h"
#include "debug.h"
#include "disp_drv_log.h"
#include "disp_lcm.h"
#include "disp_utils.h"
#include "mtkfb_console.h"
#include "ddp_hal.h"
#include "ddp_dump.h"
#include "ddp_path.h"
#include "ddp_drv.h"
#include "ddp_info.h"
#include "primary_display.h"
#include "cmdq_def.h"
#include "cmdq_record.h"
#include "cmdq_reg.h"
#include "cmdq_core.h"
#include "ddp_manager.h"
#include "disp_drv_platform.h"
#include "display_recorder.h"
#include "mtk_disp_mgr.h"
#include "disp_session.h"
#include "mtk_ovl.h"
#include "ddp_mmp.h"
#include "mtkfb_fence.h"
#include "extd_multi_control.h"
#include "m4u.h"
#include "mtk_hrt.h"
#include "compat_mtk_disp_mgr.h"


#define DDP_OUTPUT_LAYID 4

/* #define NO_PQ_IOCTL */

static unsigned int session_config[MAX_SESSION_COUNT];
static DEFINE_MUTEX(disp_session_lock);

static dev_t mtk_disp_mgr_devno;
static struct cdev *mtk_disp_mgr_cdev;
static struct class *mtk_disp_mgr_class;

static int mtk_disp_mgr_open(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t mtk_disp_mgr_read(struct file *file, char __user *data, size_t len, loff_t *ppos)
{
	return 0;
}

static int mtk_disp_mgr_release(struct inode *inode, struct file *file)
{
	return 0;
}

static int mtk_disp_mgr_flush(struct file *a_pstFile, fl_owner_t a_id)
{
	return 0;
}

static int mtk_disp_mgr_mmap(struct file *file, struct vm_area_struct *vma)
{
	static const unsigned long addr_min = 0x14000000;
	static const unsigned long addr_max = 0x14025000;
	static const unsigned long size = addr_max - addr_min;
	const unsigned long require_size = vma->vm_end - vma->vm_start;
	unsigned long pa_start = vma->vm_pgoff << PAGE_SHIFT;
	unsigned long pa_end = pa_start + require_size;

	DISPDBG("mmap size %ld, vmpg0ff 0x%lx, pastart 0x%lx, paend 0x%lx\n",
		require_size, vma->vm_pgoff, pa_start, pa_end);

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	if (require_size > size || (pa_start < addr_min || pa_end > addr_max)) {
		DISPERR("mmap size range over flow!!\n");
		return -EAGAIN;
	}
	if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, (vma->vm_end - vma->vm_start), vma->vm_page_prot)) {
		DISPERR("display mmap failed!!\n");
		return -EAGAIN;
	}

	return 0;
}

int _session_inited(disp_session_config config)
{
#if 0
	int i, idx = -1;

	for (i = 0; i < MAX_SESSION_COUNT; i++) {
		if (session_config[i] == 0 && idx == -1)
			idx = i;


		if (session_config[i] == session) {
			ret = DCP_STATUS_ALREADY_EXIST;
			DISPMSG("session(0x%x) already exists\n", session);
			break;
		}
	}
#endif
	return 0;
}

int disp_create_session(disp_session_config *config)
{
	int ret = 0;
	int is_session_inited = 0;
	unsigned int session = MAKE_DISP_SESSION(config->type, config->device_id);
	int i, idx = -1;
	/* 1.To check if this session exists already */
	mutex_lock(&disp_session_lock);
	for (i = 0; i < MAX_SESSION_COUNT; i++) {
		if (session_config[i] == session) {
			is_session_inited = 1;
			idx = i;
			DISPMSG("create session is exited:0x%x\n", session);
			break;
		}
	}

	if (is_session_inited == 1) {
		config->session_id = session;
		goto done;
	}

	for (i = 0; i < MAX_SESSION_COUNT; i++) {
		if (session_config[i] == 0 && idx == -1) {
			idx = i;
			break;
		}
	}
	/* 1.To check if support this session (mode,type,dev) */
	/* 2. Create this session */
	if (idx != -1) {
		config->session_id = session;
		session_config[idx] = session;
		DISPDBG("New session(0x%x)\n", session);
	} else {
		DISPERR("Invalid session creation request\n");
		ret = -1;
	}
done:
	mutex_unlock(&disp_session_lock);

	DISPMSG("new session done\n");
	return ret;
}

static int release_session_buffer(unsigned int session)
{
	int i = 0;

	mutex_lock(&disp_session_lock);

	for (i = 0; i < MAX_SESSION_COUNT; i++) {
		if (session_config[i] == session)
			break;
	}

	if (i == MAX_SESSION_COUNT) {
		DISPERR("%s: no session %u found!\n", __func__, session);
		mutex_unlock(&disp_session_lock);
		return -1;
	}

	mtkfb_release_session_fence(session);

	mutex_unlock(&disp_session_lock);
	return 0;
}


int disp_destroy_session(disp_session_config *config)
{
	int ret = -1;
	unsigned int session = config->session_id;
	int i;

	DISPMSG("disp_destroy_session, 0x%x", config->session_id);

	/* 1.To check if this session exists already, and remove it */
	mutex_lock(&disp_session_lock);
	for (i = 0; i < MAX_SESSION_COUNT; i++) {
		if (session_config[i] == session) {
			session_config[i] = 0;
			ret = 0;
			break;
		}
	}

	mutex_unlock(&disp_session_lock);

	if (DISP_SESSION_TYPE(config->session_id) != DISP_SESSION_PRIMARY)
		external_display_switch_mode(config->mode, session_config, config->session_id);

	if (DISP_SESSION_TYPE(config->session_id) != DISP_SESSION_PRIMARY)
		release_session_buffer(config->session_id);

	/* 2. Destroy this session */
	if (ret == 0)
		DISPMSG("Destroy session(0x%x)\n", session);
	else
		DISPERR("session(0x%x) does not exists\n", session);


	return ret;
}


int _ioctl_create_session(unsigned long arg)
{
	int ret = 0;
	void __user *argp = (void __user *)arg;
	disp_session_config config;

	if (copy_from_user(&config, argp, sizeof(config))) {
		DISPERR("[FB]: copy_from_user failed! line:%d\n", __LINE__);
		return -EFAULT;
	}

	if (disp_create_session(&config) != 0)
		ret = -EFAULT;


	if (copy_to_user(argp, &config, sizeof(config))) {
		DISPERR("[FB]: copy_to_user failed! line:%d\n", __LINE__);
		ret = -EFAULT;
	}

	return ret;
}

int _ioctl_destroy_session(unsigned long arg)
{
	int ret = 0;
	void __user *argp = (void __user *)arg;
	disp_session_config config;

	if (copy_from_user(&config, argp, sizeof(config))) {
		DISPERR("[FB]: copy_from_user failed! line:%d\n", __LINE__);
		return -EFAULT;
	}

	if (disp_destroy_session(&config) != 0)
		ret = -EFAULT;


	return ret;
}


char *disp_session_mode_spy(unsigned int session_id)
{
	switch (DISP_SESSION_TYPE(session_id)) {
	case DISP_SESSION_PRIMARY:
		return "P";
	case DISP_SESSION_EXTERNAL:
		return "E";
	case DISP_SESSION_MEMORY:
		return "M";
	default:
		return "Unknown";
	}
}
static int __trigger_display(disp_session_config *config)
{
	int ret = 0;

	unsigned int session_id = 0;

	disp_session_sync_info *session_info;

	session_id = config->session_id;
	session_info = disp_get_session_sync_info_for_debug(session_id);
	if (session_info) {
		unsigned int proc_name = (current->comm[0] << 24) |
		    (current->comm[1] << 16) | (current->comm[2] << 8) | (current->comm[3] << 0);
		dprec_start(&session_info->event_trigger, proc_name, 0);
	}

	if (DISP_SESSION_TYPE(session_id) == DISP_SESSION_PRIMARY) {
		DISPPR_FENCE("T+/%s%d/%d\n", disp_session_mode_spy(session_id),
			     DISP_SESSION_DEV(session_id), config->present_fence_idx);
	} else {
		DISPPR_FENCE("T+/%s%d\n", disp_session_mode_spy(session_id),
			     DISP_SESSION_DEV(session_id));
	}

	if (DISP_SESSION_TYPE(session_id) == DISP_SESSION_PRIMARY) {
		pr_err("%s: legecy API are not supported!\n", __func__);
		BUG();
	} else if (DISP_SESSION_TYPE(session_id) == DISP_SESSION_EXTERNAL) {
		mutex_lock(&disp_session_lock);
		ret = external_display_trigger(config->tigger_mode, session_id);
		mutex_unlock(&disp_session_lock);
	} else if (DISP_SESSION_TYPE(session_id) == DISP_SESSION_MEMORY) {
		ovl2mem_trigger(1, NULL, 0);
	} else {
		DISPERR("session type is wrong:0x%08x\n", session_id);
		ret = -1;
	}

	if (session_info)
		dprec_done(&session_info->event_trigger, 0, 0);


	return ret;
}

int _ioctl_trigger_session(unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	disp_session_config config;

	if (copy_from_user(&config, argp, sizeof(config))) {
		DISPERR("[FB]: copy_from_user failed! line:%d\n", __LINE__);
		return -EFAULT;
	}
	return __trigger_display(&config);
}


int _ioctl_prepare_present_fence(unsigned long arg)
{
	int ret = 0;

	void __user *argp = (void __user *)arg;
	struct fence_data data;
	disp_present_fence preset_fence_struct;
	static unsigned int fence_idx;
	disp_sync_info *layer_info = NULL;
	int timeline_id = disp_sync_get_present_timeline_id();

	if (copy_from_user(&preset_fence_struct, (void __user *)arg, sizeof(disp_present_fence))) {
		pr_err("[FB Driver]: copy_from_user failed! line:%d\n", __LINE__);
		return -EFAULT;
	}

	if (DISP_SESSION_TYPE(preset_fence_struct.session_id) != DISP_SESSION_PRIMARY) {
		DISPERR("non-primary ask for present fence! session=0x%x\n",
			preset_fence_struct.session_id);
		data.fence = MTK_FB_INVALID_FENCE_FD;
		data.value = 0;
	} else {
		layer_info = _get_sync_info(preset_fence_struct.session_id, timeline_id);
		if (layer_info == NULL) {
			DISPERR("layer_info is null\n");
			ret = -EFAULT;
			return ret;
		}
		/* create fence */
		data.fence = MTK_FB_INVALID_FENCE_FD;
		data.value = ++fence_idx;
		ret = fence_create(layer_info->timeline, &data);
		if (ret != 0) {
			DISPPR_ERROR("%s%d,layer%d create Fence Object failed!\n",
				     disp_session_mode_spy(preset_fence_struct.session_id),
				     DISP_SESSION_DEV(preset_fence_struct.session_id), timeline_id);
			ret = -EFAULT;
		}
	}

	preset_fence_struct.present_fence_fd = data.fence;
	preset_fence_struct.present_fence_index = data.value;
	if (copy_to_user(argp, &preset_fence_struct, sizeof(preset_fence_struct))) {
		pr_err("[FB Driver]: copy_to_user failed! line:%d\n", __LINE__);
		ret = -EFAULT;
	}
	MMProfileLogEx(ddp_mmp_get_events()->present_fence_get, MMProfileFlagPulse,
		       preset_fence_struct.present_fence_fd,
		       preset_fence_struct.present_fence_index);
	DISPPR_FENCE("P+/%s%d/L%d/id%d/fd%d\n",
		     disp_session_mode_spy(preset_fence_struct.session_id),
		     DISP_SESSION_DEV(preset_fence_struct.session_id), timeline_id,
		     preset_fence_struct.present_fence_index, preset_fence_struct.present_fence_fd);

	return ret;
}

int _ioctl_prepare_buffer(unsigned long arg, ePREPARE_FENCE_TYPE type)
{
	int ret = 0;
	void __user *argp = (void __user *)arg;
	disp_buffer_info info;
	struct mtkfb_fence_buf_info *buf, *buf2;

	if (copy_from_user(&info, (void __user *)arg, sizeof(info))) {
		pr_err("[FB Driver]: copy_from_user failed! line:%d\n", __LINE__);
		return -EFAULT;
	}

	if (type == PREPARE_INPUT_FENCE)
		/* do nothing */;
	else if (type == PREPARE_PRESENT_FENCE)
		info.layer_id = disp_sync_get_present_timeline_id();
	else if (type == PREPARE_OUTPUT_FENCE)
		info.layer_id = disp_sync_get_output_timeline_id();
	else
		DISPPR_ERROR("type is wrong: %d\n", type);


	if (info.layer_en) {
		buf = disp_sync_prepare_buf(&info);
		if (buf != NULL) {
			info.fence_fd = buf->fence;
			info.index = buf->idx;
		} else {
			DISPPR_ERROR("P+ FAIL /%s%d/l%d/e%d/ion%d/c%d/id%d/ffd%d\n",
				     disp_session_mode_spy(info.session_id),
				     DISP_SESSION_DEV(info.session_id), info.layer_id,
				     info.layer_en, info.ion_fd, info.cache_sync, info.index,
				     info.fence_fd);
			info.fence_fd = MTK_FB_INVALID_FENCE_FD;	/* invalid fd */
			info.index = 0;
		}

		if (type == PREPARE_OUTPUT_FENCE) {
			if (primary_display_is_decouple_mode() && primary_display_is_mirror_mode()) {
				/*create second fence for wdma when decouple mirror mode */
				info.layer_id = disp_sync_get_output_interface_timeline_id();
				buf2 = disp_sync_prepare_buf(&info);
				if (buf2 != NULL) {
					info.interface_fence_fd = buf2->fence;
					info.interface_index = buf2->idx;
				} else {
					DISPPR_ERROR("P+ FAIL /%s%d/l%d/e%d/ion%d/c%d/id%d/ffd%d\n",
						     disp_session_mode_spy(info.session_id),
						     DISP_SESSION_DEV(info.session_id),
						     info.layer_id, info.layer_en, info.ion_fd,
						     info.cache_sync, info.index, info.fence_fd);
					info.interface_fence_fd = MTK_FB_INVALID_FENCE_FD;	/* invalid fd */
					info.interface_index = 0;
				}
			} else {
				info.interface_fence_fd = MTK_FB_INVALID_FENCE_FD;	/* invalid fd */
				info.interface_index = 0;
			}
		}
	} else {
		DISPPR_ERROR("P+ FAIL /%s%d/l%d/e%d/ion%d/c%d/id%d/ffd%d\n",
			     disp_session_mode_spy(info.session_id),
			     DISP_SESSION_DEV(info.session_id), info.layer_id, info.layer_en,
			     info.ion_fd, info.cache_sync, info.index, info.fence_fd);
		info.fence_fd = MTK_FB_INVALID_FENCE_FD;	/* invalid fd */
		info.index = 0;
	}
	if (copy_to_user(argp, &info, sizeof(info))) {
		pr_err("[FB Driver]: copy_to_user failed! line:%d\n", __LINE__);
		ret = -EFAULT;
	}
	return ret;
}

const char *_disp_format_spy(DISP_FORMAT format)
{
	switch (format) {
	case DISP_FORMAT_RGB565:
		return "RGB565";
	case DISP_FORMAT_RGB888:
		return "RGB888";
	case DISP_FORMAT_BGR888:
		return "BGR888";
	case DISP_FORMAT_ARGB8888:
		return "ARGB8888";
	case DISP_FORMAT_ABGR8888:
		return "ABGR8888";
	case DISP_FORMAT_RGBA8888:
		return "RGBA8888";
	case DISP_FORMAT_BGRA8888:
		return "BGRA8888";
	case DISP_FORMAT_YUV422:
		return "YUV422";
	case DISP_FORMAT_XRGB8888:
		return "XRGB8888";
	case DISP_FORMAT_XBGR8888:
		return "XBGR8888";
	case DISP_FORMAT_RGBX8888:
		return "RGBX8888";
	case DISP_FORMAT_BGRX8888:
		return "BGRX8888";
	case DISP_FORMAT_UYVY:
		return "UYVY";
	case DISP_FORMAT_YUV420_P:
		return "YUV420_P";
	case DISP_FORMAT_YV12:
		return "YV12";
	case DISP_FORMAT_PABGR8888:
		return "PABGR";
	case DISP_FORMAT_PARGB8888:
		return "PARGB";
	case DISP_FORMAT_PBGRA8888:
		return "PBGRA";
	case DISP_FORMAT_PRGBA8888:
		return "PRGBA";
	default:
		return "unknown";
	}
}

#if 0 /* defined but not used */
static int _sync_convert_fb_layer_to_disp_input(unsigned int session_id, disp_input_config *src,
						primary_disp_input_config *dst,
						unsigned int dst_mva)
{
	unsigned int layerpitch = 0;

	dst->layer = src->layer_id;

	if (!src->layer_enable) {
		dst->layer_en = 0;
		dst->isDirty = true;
		return 0;
	}

	dst->fmt = disp_fmt_to_unified_fmt(src->src_fmt);
	layerpitch = UFMT_GET_bpp(dst->fmt) / 8;

	dst->buffer_source = src->buffer_source;

	dst->vaddr = (unsigned long)src->src_base_addr;
	dst->security = src->security;
	if (src->src_phy_addr != NULL)
		dst->addr = (unsigned long)src->src_phy_addr;
	else
		dst->addr = dst_mva;

	dst->isTdshp = src->isTdshp;
	dst->buff_idx = src->next_buff_idx;
	dst->identity = src->identity;
	dst->connected_type = src->connected_type;

	/* set Alpha blending */
	dst->aen = src->alpha_enable;
	dst->alpha = src->alpha;
	dst->sur_aen = src->sur_aen;
	dst->src_alpha = src->src_alpha;
	dst->dst_alpha = src->dst_alpha;
#if 1
	if (DISP_FORMAT_ARGB8888 == src->src_fmt || DISP_FORMAT_ABGR8888 == src->src_fmt
	    || DISP_FORMAT_RGBA8888 == src->src_fmt || DISP_FORMAT_BGRA8888 == src->src_fmt) {
		dst->aen = TRUE;
	}
#endif

	/* set src width, src height */
	dst->src_x = src->src_offset_x;
	dst->src_y = src->src_offset_y;
	dst->src_w = src->src_width;
	dst->src_h = src->src_height;
	dst->dst_x = src->tgt_offset_x;
	dst->dst_y = src->tgt_offset_y;
	dst->dst_w = src->tgt_width;
	dst->dst_h = src->tgt_height;
	if (dst->dst_w > dst->src_w)
		dst->dst_w = dst->src_w;
	if (dst->dst_h > dst->src_h)
		dst->dst_h = dst->src_h;

	dst->src_pitch = src->src_pitch * layerpitch;

	/* set color key */
	dst->key = src->src_color_key;
	dst->keyEn = src->src_use_color_key;

	/* data transferring is triggerred in MTKFB_TRIG_OVERLAY_OUT */
	dst->layer_en = src->layer_enable;
	dst->isDirty = true;
	dst->yuv_range = src->yuv_range;
	dst->buffer_source = src->buffer_source;

	return 0;
}
#endif
static int set_memory_buffer(disp_session_input_config *input)
{
	int i = 0;
	int layer_id = 0;
	unsigned int dst_size = 0;
	unsigned long dst_mva = 0;
	unsigned int session_id = 0;
	disp_session_sync_info *session_info;


	session_id = input->session_id;
	session_info = disp_get_session_sync_info_for_debug(session_id);

	for (i = 0; i < input->config_layer_num; i++) {
		dst_mva = 0;
		layer_id = input->config[i].layer_id;
		if (input->config[i].layer_enable) {
			if (input->config[i].buffer_source == DISP_BUFFER_ALPHA) {
				DISPPR_FENCE("ML %d is dim layer,fence %d\n",
					     input->config[i].layer_id,
					     input->config[i].next_buff_idx);
				input->config[i].src_offset_x = 0;
				input->config[i].src_offset_y = 0;
				input->config[i].sur_aen = 0;
				input->config[i].src_fmt = DISP_FORMAT_RGB888;
				input->config[i].src_pitch = input->config[i].src_width;
				input->config[i].src_phy_addr = 0;
				input->config[i].next_buff_idx = 0;
				/* force dim layer as non-sec */
				input->config[i].security = DISP_NORMAL_BUFFER;
			}
			if (input->config[i].src_phy_addr) {
				dst_mva = (unsigned long)input->config[i].src_phy_addr;
			} else {
				disp_sync_query_buf_info(session_id, layer_id,
							 (unsigned int)(input->config[i].next_buff_idx),
							 (unsigned long *)&dst_mva, &dst_size);
				input->config[i].src_phy_addr = (void *)dst_mva;
			}

			if (dst_mva == 0)
				input->config[i].layer_enable = 0;


			DISPPR_FENCE
			    ("S+/ML%d/e%d/id%d/%dx%d(%d,%d)(%d,%d)/%s/%d/0x%p/mva0x%lx/t%d/sec%d\n",
			     input->config[i].layer_id, input->config[i].layer_enable,
			     input->config[i].next_buff_idx, input->config[i].src_width,
			     input->config[i].src_height, input->config[i].src_offset_x,
			     input->config[i].src_offset_y, input->config[i].tgt_offset_x,
			     input->config[i].tgt_offset_y,
			     _disp_format_spy(input->config[i].src_fmt), input->config[i].src_pitch,
			     input->config[i].src_phy_addr, dst_mva, get_ovl2mem_ticket(),
			     input->config[i].security);
		} else {
			DISPPR_FENCE("S+/ML%d/e%d/id%d\n", input->config[i].layer_id,
				     input->config[i].layer_enable, input->config[i].next_buff_idx);
		}

		/* /disp_sync_put_cached_layer_info(session_id, layer_id, &input->config[i], get_ovl2mem_ticket()); */
		mtkfb_update_buf_ticket(session_id, layer_id, input->config[i].next_buff_idx,
					get_ovl2mem_ticket());

		if (input->config[i].layer_enable) {
			mtkfb_update_buf_info(input->session_id, input->config[i].layer_id,
					      input->config[i].next_buff_idx, 0,
					      input->config[i].frm_sequence);
		}
		if (session_info) {
			dprec_submit(&session_info->event_setinput, input->config[i].next_buff_idx,
				     (input->config_layer_num << 28) | (input->
									config[i].layer_id << 24) |
				     (input->config[i].src_fmt << 12) | input->config[i].
				     layer_enable);
		}
	}

	ovl2mem_input_config(input);

	return 0;

}

static int set_external_buffer(disp_session_input_config *input)
{
	int i = 0;
	int ret = 0;
	int layer_id = 0;
	unsigned int dst_size = 0;
	unsigned long dst_mva = 0;
	unsigned int session_id = 0;
	unsigned int mva_offset = 0;
	disp_session_sync_info *session_info;

	session_id = input->session_id;
	session_info = disp_get_session_sync_info_for_debug(session_id);

	for (i = 0; i < input->config_layer_num; ++i) {
		dst_mva = 0;
		layer_id = input->config[i].layer_id;
		if (input->config[i].layer_enable) {
			if (input->config[i].buffer_source == DISP_BUFFER_ALPHA) {
				DISPPR_FENCE("EL %d is dim layer,fence %d\n",
					     input->config[i].layer_id,
					     input->config[i].next_buff_idx);
				input->config[i].src_offset_x = 0;
				input->config[i].src_offset_y = 0;
				input->config[i].sur_aen = 0;
				input->config[i].src_fmt = DISP_FORMAT_RGB888;
				input->config[i].src_pitch = input->config[i].src_width;
				input->config[i].src_phy_addr = 0;
				input->config[i].next_buff_idx = 0;
				/* force dim layer as non-sec */
				input->config[i].security = DISP_NORMAL_BUFFER;
			}
			if (input->config[i].src_phy_addr) {
				dst_mva = (unsigned long)input->config[i].src_phy_addr;
			} else {
				disp_sync_query_buf_info(session_id, layer_id,
							(unsigned int)input->config[i].next_buff_idx,
							(unsigned long *)&dst_mva, &dst_size);
				input->config[i].src_phy_addr = (void*)dst_mva;
			}

			if (dst_mva == 0)
				input->config[i].layer_enable = 0;


			DISPPR_FENCE
			    ("S+/EL%d/e%d/id%d/%dx%d(%d,%d)(%d,%d)/%s/%d/0x%p/mva0x%lx\n",
			     input->config[i].layer_id, input->config[i].layer_enable,
			     input->config[i].next_buff_idx, input->config[i].src_width,
			     input->config[i].src_height, input->config[i].src_offset_x,
			     input->config[i].src_offset_y, input->config[i].tgt_offset_x,
			     input->config[i].tgt_offset_y,
			     _disp_format_spy(input->config[i].src_fmt), input->config[i].src_pitch,
			     input->config[i].src_phy_addr, dst_mva);
		} else {
			DISPPR_FENCE("S+/EL%d/e%d/id%d\n", input->config[i].layer_id,
				     input->config[i].layer_enable, input->config[i].next_buff_idx);
		}

		disp_sync_put_cached_layer_info(session_id, layer_id, &input->config[i], dst_mva);

		if (input->config[i].layer_enable) {
			/*which is calculated by pitch and ROI. */
			unsigned int Bpp, x, y, pitch;

			x = input->config[i].src_offset_x;
			y = input->config[i].src_offset_y;
			pitch = input->config[i].src_pitch;
			Bpp = UFMT_GET_bpp(disp_fmt_to_unified_fmt(input->config[i].src_fmt)) / 8;
			mva_offset = (x + y * pitch) * Bpp;
			mtkfb_update_buf_info(input->session_id, input->config[i].layer_id,
					      input->config[i].next_buff_idx, mva_offset,
					      input->config[i].frm_sequence);
			mtkfb_update_buf_info_new(input->session_id, mva_offset,
						  (disp_input_config *) input->config);
		}

		if (session_info) {
			dprec_submit(&session_info->event_setinput, input->config[i].next_buff_idx,
				     (input->config_layer_num << 28) | (input->config[i].layer_id << 24)
				     | (input->config[i].src_fmt << 12) | input->config[i].layer_enable);
		}
	}

	ret = external_display_config_input(input, input->config[0].next_buff_idx, session_id);

	if (ret == -2) {
		for (i = 0; i < input->config_layer_num; i++)
			mtkfb_release_layer_fence(input->session_id, i);
	}


	return 0;
}

static int input_config_preprocess(struct disp_frame_cfg_t *cfg)
{
	int i = 0;
	int layer_id = 0;
	unsigned int dst_size = 0;
	unsigned long dst_mva = 0;
	unsigned int session_id = 0;
	unsigned int mva_offset = 0;
	disp_session_sync_info *session_info;

	session_id = cfg->session_id;
	session_info = disp_get_session_sync_info_for_debug(session_id);

	if (cfg->input_layer_num == 0
	    || cfg->input_layer_num > primary_display_get_max_layer()) {
		DISPERR("set_primary_buffer, config_layer_num invalid = %d!\n",
			cfg->input_layer_num);
		return 0;
	}

	for (i = 0; i < cfg->input_layer_num; i++) {
		dst_mva = 0;
		layer_id = cfg->input_cfg[i].layer_id;
		if (layer_id >= primary_display_get_max_layer()) {
			DISPERR("set_primary_buffer, invalid layer_id = %d!\n", layer_id);
			continue;
		}

		if (cfg->input_cfg[i].layer_enable) {
			unsigned int Bpp, x, y, pitch;

			if (cfg->input_cfg[i].buffer_source == DISP_BUFFER_ALPHA) {
				DISPPR_FENCE("PL %d is dim layer,fence %d\n",
					     cfg->input_cfg[i].layer_id,
					     cfg->input_cfg[i].next_buff_idx);

				cfg->input_cfg[i].src_offset_x = 0;
				cfg->input_cfg[i].src_offset_y = 0;
				cfg->input_cfg[i].sur_aen = 0;
				cfg->input_cfg[i].src_fmt = DISP_FORMAT_RGB888;
				cfg->input_cfg[i].src_pitch = cfg->input_cfg[i].src_width;
				cfg->input_cfg[i].src_phy_addr = (void *)get_dim_layer_mva_addr();
				cfg->input_cfg[i].next_buff_idx = 0;
				/* force dim layer as non-sec */
				cfg->input_cfg[i].security = DISP_NORMAL_BUFFER;
			}
			if (cfg->input_cfg[i].src_phy_addr) {
				dst_mva = (unsigned long)cfg->input_cfg[i].src_phy_addr;
			} else {
				disp_sync_query_buf_info(session_id, layer_id,
						(unsigned int)cfg->input_cfg[i].next_buff_idx,
						(unsigned long *)&dst_mva, &dst_size);
			}

			cfg->input_cfg[i].src_phy_addr = (void*)dst_mva;

			if (dst_mva == 0) {
				DISPPR_ERROR("disable layer %d because of no valid mva\n",
					     cfg->input_cfg[i].layer_id);
				DISPERR("S+/PL%d/e%d/id%d/%dx%d(%d,%d)(%d,%d)/%s/%d/0x%p/mva0x%lx/sec%d/s%d\n",
				     cfg->input_cfg[i].layer_id, cfg->input_cfg[i].layer_enable,
				     cfg->input_cfg[i].next_buff_idx, cfg->input_cfg[i].src_width,
				     cfg->input_cfg[i].src_height, cfg->input_cfg[i].src_offset_x,
				     cfg->input_cfg[i].src_offset_y, cfg->input_cfg[i].tgt_offset_x,
				     cfg->input_cfg[i].tgt_offset_y,
				     _disp_format_spy(cfg->input_cfg[i].src_fmt), cfg->input_cfg[i].src_pitch,
				     cfg->input_cfg[i].src_phy_addr, dst_mva, cfg->input_cfg[i].security,
				     cfg->input_cfg[i].buffer_source);
				/*disp_aee_print("no valid mva\n");*/
				cfg->input_cfg[i].layer_enable = 0;
			}
			/* OVL addr is not the start address of buffer, which is calculated by pitch and ROI. */
			x = cfg->input_cfg[i].src_offset_x;
			y = cfg->input_cfg[i].src_offset_y;
			pitch = cfg->input_cfg[i].src_pitch;
			Bpp = UFMT_GET_bpp(disp_fmt_to_unified_fmt(cfg->input_cfg[i].src_fmt)) / 8;

			mva_offset = (x + y * pitch) * Bpp;
			mtkfb_update_buf_info(cfg->session_id, cfg->input_cfg[i].layer_id,
					      cfg->input_cfg[i].next_buff_idx, mva_offset,
					      cfg->input_cfg[i].frm_sequence);

			DISPPR_FENCE("S+/PL%d/e%d/id%d/%dx%d(%d,%d)(%d,%d)/%s/%d/0x%p/mva0x%lx/sec%d\n",
			     cfg->input_cfg[i].layer_id, cfg->input_cfg[i].layer_enable,
			     cfg->input_cfg[i].next_buff_idx, cfg->input_cfg[i].src_width,
			     cfg->input_cfg[i].src_height, cfg->input_cfg[i].src_offset_x,
			     cfg->input_cfg[i].src_offset_y, cfg->input_cfg[i].tgt_offset_x,
			     cfg->input_cfg[i].tgt_offset_y,
			     _disp_format_spy(cfg->input_cfg[i].src_fmt), cfg->input_cfg[i].src_pitch,
			     cfg->input_cfg[i].src_phy_addr, dst_mva, cfg->input_cfg[i].security);
		} else {
			DISPPR_FENCE("S+/PL%d/e%d/id%d\n", cfg->input_cfg[i].layer_id,
				     cfg->input_cfg[i].layer_enable, cfg->input_cfg[i].next_buff_idx);
		}

		disp_sync_put_cached_layer_info(session_id, layer_id, &cfg->input_cfg[i], dst_mva);

		if (session_info)
			dprec_submit(&session_info->event_setinput, cfg->input_cfg[i].next_buff_idx, dst_mva);
	}
	return 0;
}

static int __set_input(disp_session_input_config *session_input, int overlap_layer_num)
{
	int ret = 0;
	unsigned int session_id = 0;
	disp_session_sync_info *session_info;

	session_input->setter = SESSION_USER_HWC;
	session_id = session_input->session_id;

	session_info = disp_get_session_sync_info_for_debug(session_id);

	if (session_info)
		dprec_start(&session_info->event_setinput, overlap_layer_num, session_input->config_layer_num);


	DISPPR_FENCE("S+/%s%d/count%d\n", disp_session_mode_spy(session_id),
		     DISP_SESSION_DEV(session_id), session_input->config_layer_num);

	if (DISP_SESSION_TYPE(session_id) == DISP_SESSION_PRIMARY) {
		pr_err("%s: legecy API are not supported!\n", __func__);
		BUG();
	} else if (DISP_SESSION_TYPE(session_id) == DISP_SESSION_EXTERNAL) {
		ret = set_external_buffer(session_input);
	} else if (DISP_SESSION_TYPE(session_id) == DISP_SESSION_MEMORY) {
		ret = set_memory_buffer(session_input);
	} else {
		DISPERR("session type is wrong:0x%08x\n", session_id);
		return -1;
	}

	if (session_info)
		dprec_done(&session_info->event_setinput, 0, session_input->config_layer_num);


	return ret;

}

int _ioctl_set_input_buffer(unsigned long arg)
{
	int ret = 0;
	void __user *argp = (void __user *)arg;
	disp_session_input_config *session_input;

	session_input = kmalloc(sizeof(*session_input), GFP_KERNEL);

	if (!session_input)
		return -ENOMEM;

	if (copy_from_user(session_input, argp, sizeof(*session_input))) {
		DISPERR("[FB]: copy_from_user failed! line:%d\n", __LINE__);
		kfree(session_input);
		return -EFAULT;
	}
	ret = __set_input(session_input, 4);
	kfree(session_input);
	return ret;
}

static int _sync_convert_fb_layer_to_disp_output(unsigned int session_id, disp_output_config *src,
						 disp_mem_output_config *dst, unsigned int dst_mva)
{
	dst->fmt = disp_fmt_to_unified_fmt(src->fmt);

	dst->vaddr = (unsigned long)(src->va);
	dst->security = src->security;
	dst->w = src->width;
	dst->h = src->height;

/* set_overlay will not use fence+ion handle */
#if defined(MTK_FB_ION_SUPPORT)
	if (src->pa != NULL)
		dst->addr = (unsigned long)(src->pa);
	else
		dst->addr = (unsigned long)dst_mva;

#else
	dst->addr = (unsigned long)src->pa;
#endif

	dst->buff_idx = src->buff_idx;
	dst->interface_idx = src->interface_idx;

	dst->x = src->x;
	dst->y = src->y;
	dst->pitch = src->pitch * UFMT_GET_Bpp(dst->fmt);
	return 0;
}

static int output_config_preprocess(struct disp_frame_cfg_t *cfg)
{
	unsigned int session_id = 0;
	unsigned long dst_mva = 0;
	disp_session_sync_info *session_info;

	session_id = cfg->session_id;
	session_info = disp_get_session_sync_info_for_debug(session_id);

	if (cfg->output_cfg.pa) {
		dst_mva = (unsigned long)cfg->output_cfg.pa;
	} else {
		dst_mva = mtkfb_query_buf_mva(session_id, disp_sync_get_output_timeline_id(),
						cfg->output_cfg.buff_idx);
	}
	cfg->output_cfg.pa = (void*)dst_mva;

	if (!dst_mva) {
		DISPERR("%s output mva=0!!, skip it\n", __func__);
		cfg->output_en = 0;
		goto out;
	}

	/* must be mirror mode */
	if (primary_display_is_decouple_mode()) {
		disp_sync_put_cached_layer_info_v2(session_id,
				disp_sync_get_output_interface_timeline_id(),
				cfg->output_cfg.interface_idx, 1, dst_mva);

		disp_sync_put_cached_layer_info_v2(session_id,
				disp_sync_get_output_timeline_id(),
				cfg->output_cfg.buff_idx, 1, dst_mva);
	}

	DISPPR_FENCE("S+O/%s%d/L%d(id%d)/L%d(id%d)/%dx%d(%d,%d)/%s/%d/0x%08x/mva0x%08lx/sec%d\n",
	     disp_session_mode_spy(session_id), DISP_SESSION_DEV(session_id),
	     disp_sync_get_output_timeline_id(), cfg->output_cfg.buff_idx,
	     disp_sync_get_output_interface_timeline_id(),
	     cfg->output_cfg.interface_idx, cfg->output_cfg.width,
	     cfg->output_cfg.height, cfg->output_cfg.x, cfg->output_cfg.y,
	     _disp_format_spy(cfg->output_cfg.fmt), cfg->output_cfg.pitch,
	     cfg->output_cfg.pitchUV, dst_mva, cfg->output_cfg.security);

	mtkfb_update_buf_info(cfg->session_id,
			      disp_sync_get_output_interface_timeline_id(),
			      cfg->output_cfg.buff_idx, 0,
			      cfg->output_cfg.frm_sequence);

	if (session_info) {
		dprec_submit(&session_info->event_setoutput, cfg->output_cfg.buff_idx,
			     dst_mva);
	}
	DISPDBG("_ioctl_set_output_buffer done idx 0x%x, mva %lx, fmt %x, w %x, h %x (%x %x), p %x\n",
	     cfg->output_cfg.buff_idx, dst_mva, cfg->output_cfg.fmt,
	     cfg->output_cfg.width, cfg->output_cfg.height,
	     cfg->output_cfg.x, cfg->output_cfg.y, cfg->output_cfg.pitch);
out:
	return 0;
}

static int __set_output(disp_session_output_config *session_output)
{
	unsigned int session_id = 0;
	unsigned long dst_mva = 0;
	disp_session_sync_info *session_info;

	session_id = session_output->session_id;
	session_info = disp_get_session_sync_info_for_debug(session_id);

	if (session_info)
		dprec_start(&session_info->event_setoutput, session_output->config.buff_idx, 0);

	if (DISP_SESSION_TYPE(session_id) == DISP_SESSION_PRIMARY) {
		pr_err("%s: legecy API are not supported!\n", __func__);
		BUG();
	} else if (DISP_SESSION_TYPE(session_id) == DISP_SESSION_MEMORY) {
		disp_mem_output_config primary_output;

		memset((void *)&primary_output, 0, sizeof(primary_output));
		if (session_output->config.pa) {
			dst_mva = (unsigned long)session_output->config.pa;
		} else {
			dst_mva = mtkfb_query_buf_mva(session_id, disp_sync_get_output_timeline_id(),
						      (unsigned int)(session_output->config.buff_idx));
		}

		mtkfb_update_buf_ticket(session_id, disp_sync_get_output_timeline_id(),
					session_output->config.buff_idx, get_ovl2mem_ticket());
		_sync_convert_fb_layer_to_disp_output(session_output->session_id,
						      &(session_output->config), &primary_output,
						      dst_mva);
		primary_output.dirty = 1;

		DISPPR_FENCE("S+/%s%d/L%d/id%d/%dx%d(%d,%d)/%s/%d/%d/0x%p/mva0x%lx/t%d/sec%d(%d)\n",
			     disp_session_mode_spy(session_id), DISP_SESSION_DEV(session_id),
			     disp_sync_get_output_timeline_id(),
			     session_output->config.buff_idx,
			     session_output->config.width,
			     session_output->config.height,
			     session_output->config.x,
			     session_output->config.y,
			     _disp_format_spy(session_output->config.fmt),
			     session_output->config.pitch,
			     session_output->config.pitchUV,
			     session_output->config.pa,
			     dst_mva, get_ovl2mem_ticket(),
			     session_output->config.security, primary_output.security);

		if (dst_mva)
			ovl2mem_output_config(&primary_output);
		else
			DISPERR("error buffer idx 0x%x\n", session_output->config.buff_idx);

		mtkfb_update_buf_info(session_output->session_id, disp_sync_get_output_timeline_id(),
				      session_output->config.buff_idx, 0,
				      session_output->config.frm_sequence);

		if (session_info)
			dprec_submit(&session_info->event_setoutput, session_output->config.buff_idx, dst_mva);

		DISPDBG("_ioctl_set_output_buffer done idx 0x%x, mva %lx, fmt %x, w %x, h %x, p %x\n",
		     session_output->config.buff_idx, dst_mva, session_output->config.fmt,
		     session_output->config.width, session_output->config.height,
		     session_output->config.pitch);
	}


	if (session_info)
		dprec_done(&session_info->event_setoutput, session_output->config.buff_idx, 0);

	return 0;
}

int _ioctl_set_output_buffer(unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	disp_session_output_config session_output;

	if (copy_from_user(&session_output, argp, sizeof(session_output))) {
		DISPERR("[FB]: copy_from_user failed! line:%d\n", __LINE__);
		return -EFAULT;
	}

	return __set_output(&session_output);
}


static int __frame_config_set_input(struct disp_frame_cfg_t *frame_cfg)
{
	int ret = 0;
	disp_session_input_config *session_input;

	session_input = kmalloc(sizeof(*session_input), GFP_KERNEL);
	if (!session_input)
		return -ENOMEM;

	session_input->setter = SESSION_USER_HWC;
	session_input->session_id = frame_cfg->session_id;
	session_input->config_layer_num = frame_cfg->input_layer_num;
	memcpy(session_input->config, frame_cfg->input_cfg, sizeof(frame_cfg->input_cfg));

	ret = __set_input(session_input, frame_cfg->overlap_layer_num);
	kfree(session_input);
	return ret;
}

static int __frame_config_set_output(struct disp_frame_cfg_t *frame_cfg)
{
	disp_session_output_config session_output;

	if (!frame_cfg->output_en)
		return 0;

	session_output.session_id = frame_cfg->session_id;
	memcpy(&session_output.config, &frame_cfg->output_cfg, sizeof(frame_cfg->output_cfg));

	return __set_output(&session_output);
}

static int __frame_config_trigger(struct disp_frame_cfg_t *frame_cfg)
{
	disp_session_config config;

	config.session_id = frame_cfg->session_id;
	config.need_merge = 0;
	config.present_fence_idx = frame_cfg->present_fence_idx;
	config.tigger_mode = frame_cfg->tigger_mode;

	return __trigger_display(&config);
}

int _ioctl_frame_config(unsigned long arg)
{
	struct disp_frame_cfg_t *frame_cfg = kzalloc(sizeof(struct disp_frame_cfg_t), GFP_KERNEL);

	if (frame_cfg == NULL) {
		pr_err("error: kzalloc %zu memory fail!\n", sizeof(*frame_cfg));
		return -EFAULT;
	}

	if (copy_from_user(frame_cfg, (void __user *)arg, sizeof(*frame_cfg))) {
		pr_err("[FB Driver]: copy_from_user failed! line:%d\n", __LINE__);
		kfree(frame_cfg);
		return -EFAULT;
	}

	frame_cfg->setter = SESSION_USER_HWC;

	if (DISP_SESSION_TYPE(frame_cfg->session_id) == DISP_SESSION_PRIMARY) {
		input_config_preprocess(frame_cfg);
		if (frame_cfg->output_en)
			output_config_preprocess(frame_cfg);
		primary_display_frame_cfg(frame_cfg);
	} else {
		/* set input */
		__frame_config_set_input(frame_cfg);
		/* set output */
		__frame_config_set_output(frame_cfg);
		/* trigger */
		__frame_config_trigger(frame_cfg);
	}

	kfree(frame_cfg);

	return 0;
}

int disp_mgr_get_session_info(disp_session_info *info)
{
	unsigned int session_id = 0;

	session_id = info->session_id;

	if (DISP_SESSION_TYPE(session_id) == DISP_SESSION_PRIMARY) {
		primary_display_get_info(info);
#ifdef CONFIG_MTK_HDMI_SUPPORT
	} else if (DISP_SESSION_TYPE(session_id) == DISP_SESSION_EXTERNAL) {
		external_display_get_info(info, session_id);
#endif
	} else if (DISP_SESSION_TYPE(session_id) == DISP_SESSION_MEMORY) {
		ovl2mem_get_info(info);
	} else {
		DISPERR("session type is wrong:0x%08x\n", session_id);
		return -1;
	}

	return 0;
}


int _ioctl_get_info(unsigned long arg)
{
	int ret = 0;
	void __user *argp = (void __user *)arg;
	disp_session_info info;

	if (copy_from_user(&info, argp, sizeof(info))) {
		DISPERR("[FB]: copy_from_user failed! line:%d\n", __LINE__);
		return -EFAULT;
	}

	ret = disp_mgr_get_session_info(&info);

	if (copy_to_user(argp, &info, sizeof(info))) {
		DISPERR("[FB]: copy_to_user failed! line:%d\n", __LINE__);
		ret = -EFAULT;
	}

	return ret;
}

int _ioctl_get_is_driver_suspend(unsigned long arg)
{
	int ret = 0;
	void __user *argp = (void __user *)arg;
	unsigned int is_suspend = 0;

	is_suspend = primary_display_is_sleepd();
	DISPDBG("ioctl_get_is_driver_suspend, is_suspend=%d\n", is_suspend);
	if (copy_to_user(argp, &is_suspend, sizeof(int))) {
		DISPERR("[FB]: copy_to_user failed! line:%d\n", __LINE__);
		ret = -EFAULT;
	}

	return ret;
}

int _ioctl_get_display_caps(unsigned long arg)
{
	int ret = 0;
	disp_caps_info caps_info;
	void __user *argp = (void __user *)arg;

	if (copy_from_user(&caps_info, argp, sizeof(caps_info))) {
		DISPERR("[FB]: copy_to_user failed! line:%d\n", __LINE__);
		ret = -EFAULT;
	}
	memset(&caps_info, 0, sizeof(caps_info));
#ifdef DISP_HW_MODE_CAP
	caps_info.output_mode = DISP_HW_MODE_CAP;
#else
	caps_info.output_mode = DISP_OUTPUT_CAP_DIRECT_LINK;
#endif

#ifdef DISP_HW_PASS_MODE
	caps_info.output_pass = DISP_HW_PASS_MODE;
#else
	caps_info.output_pass = DISP_OUTPUT_CAP_SINGLE_PASS;
#endif

#ifdef DISP_HW_MAX_LAYER
	caps_info.max_layer_num = DISP_HW_MAX_LAYER;
#else
	caps_info.max_layer_num = 4;
#endif
	caps_info.is_support_frame_cfg_ioctl = 1;

#ifdef CONFIG_MTK_LCM_PHYSICAL_ROTATION_HW
	caps_info.is_output_rotated = 1;
#endif

	if (primary_display_partial_support()) {
		caps_info.disp_feature |= DISP_FEATURE_PARTIAL;
	}

	if (disp_helper_get_option(DISP_OPT_HRT))
		caps_info.disp_feature |= DISP_FEATURE_HRT;

	DISPDBG("%s mode:%d, pass:%d, max_layer_num:%d\n",
		__func__, caps_info.output_mode, caps_info.output_pass, caps_info.max_layer_num);

	if (copy_to_user(argp, &caps_info, sizeof(caps_info))) {
		DISPERR("[FB]: copy_to_user failed! line:%d\n", __LINE__);
		ret = -EFAULT;
	}

	return ret;
}

int _ioctl_wait_vsync(unsigned long arg)
{
	int ret = 0;
	void __user *argp = (void __user *)arg;
	disp_session_vsync_config vsync_config;
	disp_session_sync_info *session_info;

	if (copy_from_user(&vsync_config, argp, sizeof(vsync_config))) {
		DISPPR_ERROR("[FB]: copy_from_user failed! line:%d\n", __LINE__);
		return -EFAULT;
	}

	session_info = disp_get_session_sync_info_for_debug(vsync_config.session_id);
	if (session_info)
		dprec_start(&session_info->event_waitvsync, 0, 0);

	if (DISP_SESSION_TYPE(vsync_config.session_id) == DISP_SESSION_EXTERNAL)
		ret = external_display_wait_for_vsync(&vsync_config, vsync_config.session_id);
	else
		ret = primary_display_wait_for_vsync(&vsync_config);

	if (session_info)
		dprec_done(&session_info->event_waitvsync, 0, 0);

	if (copy_to_user(argp, &vsync_config, sizeof(vsync_config))) {
		DISPPR_ERROR("[FB]: copy_to_user failed! line:%d\n", __LINE__);
		return -EFAULT;
	}
	return ret;
}

int _ioctl_set_vsync(unsigned long arg)
{
	int ret = 0;
	void __user *argp = (void __user *)arg;
	unsigned int fps = 0;

	if (copy_from_user(&fps, argp, sizeof(unsigned int))) {
		DISPMSG("[FB]: copy_from_user failed! line:%d\n", __LINE__);
		return -EFAULT;
	}

	ret = primary_display_force_set_vsync_fps(fps);
	return ret;
}

int _ioctl_query_valid_layer(unsigned long arg)
{
	int ret = 0;
	disp_layer_info disp_info_user;
	void __user *argp = (void __user *)arg;

	if (copy_from_user(&disp_info_user, argp, sizeof(disp_info_user))) {
		DISPERR("[FB]: copy_to_user failed! line:%d\n", __LINE__);
		ret = -EFAULT;
	}

	ret = dispsys_hrt_calc(&disp_info_user);

	if (copy_to_user(argp, &disp_info_user, sizeof(disp_info_user))) {
		DISPERR("[FB]: copy_to_user failed! line:%d\n", __LINE__);
		ret = -EFAULT;
	}

	return ret;
}

int _ioctl_set_scenario(unsigned long arg)
{
	int ret = -1;
	struct disp_scenario_config_t scenario_cfg;
	void __user *argp = (void __user *)arg;

	if (copy_from_user(&scenario_cfg, argp, sizeof(scenario_cfg))) {
		DISPERR("[FB]: copy_to_user failed! line:%d\n", __LINE__);
		return -EFAULT;
	}

	if (DISP_SESSION_TYPE(scenario_cfg.session_id) == DISP_SESSION_PRIMARY)
		ret = primary_display_set_scenario(scenario_cfg.scenario);

	if (ret) {
		DISPERR("session(0x%x) set scenario (%d) fail, ret=%d\n",
			scenario_cfg.session_id, scenario_cfg.scenario, ret);
	}

	return ret;
}

int set_session_mode(disp_session_config *config_info, int force)
{
	int ret = 0;

	if (DISP_SESSION_TYPE(config_info->session_id) == DISP_SESSION_PRIMARY)
		primary_display_switch_mode(config_info->mode, config_info->session_id, 0);
	else
		DISPERR("[FB]: session(0x%x) swith mode(%d) fail\n", config_info->session_id, config_info->mode);

	external_display_switch_mode(config_info->mode, session_config, config_info->session_id);

	return ret;
}

int _ioctl_set_session_mode(unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	disp_session_config config_info;

	if (copy_from_user(&config_info, argp, sizeof(disp_session_config))) {
		DISPERR("[FB]: copy_from_user failed! line:%d\n", __LINE__);
		return -EFAULT;
	}
	return set_session_mode(&config_info, 0);
}

const char *_session_ioctl_spy(unsigned int cmd)
{
	switch (cmd) {
	case DISP_IOCTL_CREATE_SESSION:
		{
			return "DISP_IOCTL_CREATE_SESSION";
		}
	case DISP_IOCTL_DESTROY_SESSION:
		{
			return "DISP_IOCTL_DESTROY_SESSION";
		}
	case DISP_IOCTL_TRIGGER_SESSION:
		{
			return "DISP_IOCTL_TRIGGER_SESSION";
		}
	case DISP_IOCTL_SET_INPUT_BUFFER:
		{
			return "DISP_IOCTL_SET_INPUT_BUFFER";
		}
	case DISP_IOCTL_PREPARE_INPUT_BUFFER:
		{
			return "DISP_IOCTL_PREPARE_INPUT_BUFFER";
		}
	case DISP_IOCTL_WAIT_FOR_VSYNC:
		{
			return "DISP_IOCL_WAIT_FOR_VSYNC";
		}
	case DISP_IOCTL_GET_SESSION_INFO:
		return "DISP_IOCTL_GET_SESSION_INFO";
	case DISP_IOCTL_AAL_EVENTCTL:
		return "DISP_IOCTL_AAL_EVENTCTL";
	case DISP_IOCTL_AAL_GET_HIST:
		return "DISP_IOCTL_AAL_GET_HIST";
	case DISP_IOCTL_AAL_INIT_REG:
		return "DISP_IOCTL_AAL_INIT_REG";
	case DISP_IOCTL_AAL_SET_PARAM:
		return "DISP_IOCTL_AAL_SET_PARAM";
	case DISP_IOCTL_SET_GAMMALUT:
		return "DISP_IOCTL_SET_GAMMALUT";
	case DISP_IOCTL_SET_CCORR:
		return "DISP_IOCTL_SET_CCORR";
	case DISP_IOCTL_SET_PQPARAM:
		return "DISP_IOCTL_SET_PQPARAM";
	case DISP_IOCTL_GET_PQPARAM:
		return "DISP_IOCTL_GET_PQPARAM";
	case DISP_IOCTL_GET_PQINDEX:
		return "DISP_IOCTL_GET_PQINDEX";
	case DISP_IOCTL_SET_PQINDEX:
		return "DISP_IOCTL_SET_PQINDEX";
	case DISP_IOCTL_SET_COLOR_REG:
		return "DISP_IOCTL_SET_COLOR_REG";
	case DISP_IOCTL_SET_TDSHPINDEX:
		return "DISP_IOCTL_SET_TDSHPINDEX";
	case DISP_IOCTL_GET_TDSHPINDEX:
		return "DISP_IOCTL_GET_TDSHPINDEX";
	case DISP_IOCTL_SET_PQ_CAM_PARAM:
		return "DISP_IOCTL_SET_PQ_CAM_PARAM";
	case DISP_IOCTL_GET_PQ_CAM_PARAM:
		return "DISP_IOCTL_GET_PQ_CAM_PARAM";
	case DISP_IOCTL_SET_PQ_GAL_PARAM:
		return "DISP_IOCTL_SET_PQ_GAL_PARAM";
	case DISP_IOCTL_GET_PQ_GAL_PARAM:
		return "DISP_IOCTL_GET_PQ_GAL_PARAM";
	case DISP_IOCTL_OD_CTL:
		return "DISP_IOCTL_OD_CTL";
	case DISP_IOCTL_GET_DISPLAY_CAPS:
		return "DISP_IOCTL_GET_DISPLAY_CAPS";
	case DISP_IOCTL_QUERY_VALID_LAYER:
		return "DISP_IOCTL_QUERY_VALID_LAYER";
	default:
		{
			return "unknown";
		}
	}
}

long mtk_disp_mgr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = -1;

	/* DISPMSG("mtk_disp_mgr_ioctl, cmd=%s, arg=0x%08x\n", _session_ioctl_spy(cmd), arg); */

	switch (cmd) {
	case DISP_IOCTL_CREATE_SESSION:
		{
			return _ioctl_create_session(arg);
		}
	case DISP_IOCTL_DESTROY_SESSION:
		{
			return _ioctl_destroy_session(arg);
		}
	case DISP_IOCTL_TRIGGER_SESSION:
		{
			return _ioctl_trigger_session(arg);
		}
	case DISP_IOCTL_GET_PRESENT_FENCE:
		{
			/* return _ioctl_prepare_buffer(arg, PREPARE_PRESENT_FENCE); */
			return _ioctl_prepare_present_fence(arg);
		}
	case DISP_IOCTL_PREPARE_INPUT_BUFFER:
		{
			return _ioctl_prepare_buffer(arg, PREPARE_INPUT_FENCE);
		}
	case DISP_IOCTL_SET_INPUT_BUFFER:
		{
			return _ioctl_set_input_buffer(arg);
		}
	case DISP_IOCTL_WAIT_FOR_VSYNC:
		{
			return _ioctl_wait_vsync(arg);
		}
	case DISP_IOCTL_GET_SESSION_INFO:
		{
			return _ioctl_get_info(arg);
		}
	case DISP_IOCTL_GET_DISPLAY_CAPS:
		{
			return _ioctl_get_display_caps(arg);
		}
	case DISP_IOCTL_SET_VSYNC_FPS:
		{
			return _ioctl_set_vsync(arg);
		}
	case DISP_IOCTL_SET_SESSION_MODE:
		{
			return _ioctl_set_session_mode(arg);
		}
	case DISP_IOCTL_PREPARE_OUTPUT_BUFFER:
		{
			return _ioctl_prepare_buffer(arg, PREPARE_OUTPUT_FENCE);
		}
	case DISP_IOCTL_SET_OUTPUT_BUFFER:
		{
			return _ioctl_set_output_buffer(arg);
		}
	case DISP_IOCTL_FRAME_CONFIG:
		return _ioctl_frame_config(arg);
	case DISP_IOCTL_GET_LCMINDEX:
		{
			return primary_display_get_lcm_index();
		}
	case DISP_IOCTL_QUERY_VALID_LAYER:
		{
			return _ioctl_query_valid_layer(arg);
		}
	case DISP_IOCTL_SET_SCENARIO:
	{
		return _ioctl_set_scenario(arg);
	}
	case DISP_IOCTL_AAL_EVENTCTL:
	case DISP_IOCTL_AAL_GET_HIST:
	case DISP_IOCTL_AAL_INIT_REG:
	case DISP_IOCTL_AAL_SET_PARAM:
	case DISP_IOCTL_SET_GAMMALUT:
	case DISP_IOCTL_SET_CCORR:
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
	case DISP_IOCTL_OD_CTL:
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
	{
#ifndef NO_PQ_IOCTL
		ret = primary_display_user_cmd(cmd, arg);
#endif
		break;
	}
	default:
	{
		DISPERR("[session]ioctl not supported, 0x%08x\n", cmd);
	}
	}

	return ret;
}

#ifdef CONFIG_COMPAT
const char *_session_compat_ioctl_spy(unsigned int cmd)
{
	switch (cmd) {
	case COMPAT_DISP_IOCTL_CREATE_SESSION:
		{
			return "DISP_IOCTL_CREATE_SESSION";
		}
	case COMPAT_DISP_IOCTL_DESTROY_SESSION:
		{
			return "DISP_IOCTL_DESTROY_SESSION";
		}
	case COMPAT_DISP_IOCTL_TRIGGER_SESSION:
		{
			return "DISP_IOCTL_TRIGGER_SESSION";
		}
	case COMPAT_DISP_IOCTL_SET_INPUT_BUFFER:
		{
			return "DISP_IOCTL_SET_INPUT_BUFFER";
		}
	case COMPAT_DISP_IOCTL_PREPARE_INPUT_BUFFER:
		{
			return "DISP_IOCTL_PREPARE_INPUT_BUFFER";
		}
	case COMPAT_DISP_IOCTL_WAIT_FOR_VSYNC:
		{
			return "DISP_IOCL_WAIT_FOR_VSYNC";
		}
	case COMPAT_DISP_IOCTL_GET_SESSION_INFO:
		{
			return "DISP_IOCTL_GET_SESSION_INFO";
		}
	case COMPAT_DISP_IOCTL_PREPARE_OUTPUT_BUFFER:
		{
			return "DISP_IOCTL_PREPARE_OUTPUT_BUFFER";
		}
	case COMPAT_DISP_IOCTL_SET_OUTPUT_BUFFER:
		{
			return "DISP_IOCTL_SET_OUTPUT_BUFFER";
		}
	case COMPAT_DISP_IOCTL_SET_SESSION_MODE:
		{
			return "DISP_IOCTL_SET_SESSION_MODE";
		}
	default:
		{
			return "unknown";
		}
	}
}

static long mtk_disp_mgr_compat_ioctl(struct file *file, unsigned int cmd,  unsigned long arg)
{
	long ret = -ENOIOCTLCMD;
	void __user *data32 = compat_ptr(arg);

	/*DISPMSG("mtk_disp_mgr_compat_ioctl, cmd=%s, arg=0x%08lx\n", _session_compat_ioctl_spy(cmd), arg);*/
	switch (cmd) {
	case COMPAT_DISP_IOCTL_CREATE_SESSION:
		{
			return _compat_ioctl_create_session(file, arg);
		}
	case COMPAT_DISP_IOCTL_DESTROY_SESSION:
		{
			return _compat_ioctl_destroy_session(file, arg);
		}
	case COMPAT_DISP_IOCTL_TRIGGER_SESSION:
		{
			return _compat_ioctl_trigger_session(file, arg);
		}
	case COMPAT_DISP_IOCTL_GET_PRESENT_FENCE:
		{
			return _compat_ioctl_prepare_present_fence(file, arg);
		}
	case COMPAT_DISP_IOCTL_PREPARE_INPUT_BUFFER:
		{
			return _compat_ioctl_prepare_buffer(file, arg, PREPARE_INPUT_FENCE);
		}
	case COMPAT_DISP_IOCTL_SET_INPUT_BUFFER:
		{
			return _compat_ioctl_set_input_buffer(file, arg);
		}
	case COMPAT_DISP_IOCTL_FRAME_CONFIG:
	{
		return _compat_ioctl_frame_config(file, arg);
	}
	case COMPAT_DISP_IOCTL_WAIT_FOR_VSYNC:
		{
			return _compat_ioctl_wait_vsync(file, arg);
		}
	case COMPAT_DISP_IOCTL_GET_SESSION_INFO:
		{
			return _compat_ioctl_get_info(file, arg);
		}
	case COMPAT_DISP_IOCTL_GET_DISPLAY_CAPS:
		{
			return _compat_ioctl_get_display_caps(file, arg);
		}
	case COMPAT_DISP_IOCTL_SET_VSYNC_FPS:
		{
			return _compat_ioctl_set_vsync(file, arg);
		}
	case COMPAT_DISP_IOCTL_SET_SESSION_MODE:
		{
		    return _compat_ioctl_set_session_mode(file, arg);
		}
	case COMPAT_DISP_IOCTL_PREPARE_OUTPUT_BUFFER:
		{
		    return _compat_ioctl_prepare_buffer(file, arg, PREPARE_OUTPUT_FENCE);
		}
	case COMPAT_DISP_IOCTL_SET_OUTPUT_BUFFER:
		{
		    return _compat_ioctl_set_output_buffer(file, arg);
		}
	case DISP_IOCTL_SET_SCENARIO:
	{
		/* arg of this ioctl is all unsigned int, don't need special compat ioctl */
		return file->f_op->unlocked_ioctl(file, cmd, (unsigned long)data32);
	}

	case DISP_IOCTL_AAL_GET_HIST:
	case DISP_IOCTL_AAL_EVENTCTL:
	case DISP_IOCTL_AAL_INIT_REG:
	case DISP_IOCTL_AAL_SET_PARAM:
#ifndef NO_PQ_IOCTL
		{
			void __user *data32;
			data32 = compat_ptr(arg);
			ret = file->f_op->unlocked_ioctl(file, cmd, (unsigned long)data32);
			return ret;
		}
#endif
	case DISP_IOCTL_SET_GAMMALUT:
	case DISP_IOCTL_SET_CCORR:
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
	case DISP_IOCTL_OD_CTL:
	case DISP_IOCTL_WRITE_REG:
	case DISP_IOCTL_READ_REG:
	case DISP_IOCTL_MUTEX_CONTROL:
	case DISP_IOCTL_PQ_GET_TDSHP_FLAG:
	case DISP_IOCTL_PQ_SET_TDSHP_FLAG:
	case DISP_IOCTL_PQ_GET_DC_PARAM:
	case DISP_IOCTL_PQ_GET_DS_PARAM:
	case DISP_IOCTL_PQ_SET_DC_PARAM:
	case DISP_IOCTL_PQ_GET_MDP_COLOR_CAP:
	case DISP_IOCTL_PQ_GET_MDP_TDSHP_REG:
	case DISP_IOCTL_WRITE_SW_REG:
	case DISP_IOCTL_READ_SW_REG:
		{
#ifndef NO_PQ_IOCTL
			ret = primary_display_user_cmd(cmd, arg);
#endif
			break;
		}

	default:
		{
				DISPERR("[%s]ioctl not supported, 0x%08x\n", __func__, cmd);
				return -ENOIOCTLCMD;
		}
	}

	return ret;
}
#endif

static const struct file_operations mtk_disp_mgr_fops = {
	.owner = THIS_MODULE,
	.mmap = mtk_disp_mgr_mmap,
	.unlocked_ioctl = mtk_disp_mgr_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = mtk_disp_mgr_compat_ioctl,
#endif

	.open = mtk_disp_mgr_open,
	.release = mtk_disp_mgr_release,
	.flush = mtk_disp_mgr_flush,
	.read = mtk_disp_mgr_read,
};

static int mtk_disp_mgr_probe(struct platform_device *pdev)
{
	struct class_device;
	struct class_device *class_dev = NULL;

	pr_debug("mtk_disp_mgr_probe called!\n");

	if (alloc_chrdev_region(&mtk_disp_mgr_devno, 0, 1, DISP_SESSION_DEVICE))
		return -EFAULT;


	mtk_disp_mgr_cdev = cdev_alloc();
	mtk_disp_mgr_cdev->owner = THIS_MODULE;
	mtk_disp_mgr_cdev->ops = &mtk_disp_mgr_fops;

	cdev_add(mtk_disp_mgr_cdev, mtk_disp_mgr_devno, 1);

	mtk_disp_mgr_class = class_create(THIS_MODULE, DISP_SESSION_DEVICE);
	class_dev =
	    (struct class_device *)device_create(mtk_disp_mgr_class, NULL, mtk_disp_mgr_devno, NULL,
						 DISP_SESSION_DEVICE);
	disp_sync_init();
	external_display_control_init();

	return 0;
}

static int mtk_disp_mgr_remove(struct platform_device *pdev)
{
	return 0;
}

static void mtk_disp_mgr_shutdown(struct platform_device *pdev)
{

}

static int mtk_disp_mgr_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	return 0;
}

static int mtk_disp_mgr_resume(struct platform_device *pdev)
{
	return 0;
}


static struct platform_driver mtk_disp_mgr_driver = {
	.probe = mtk_disp_mgr_probe,
	.remove = mtk_disp_mgr_remove,
	.shutdown = mtk_disp_mgr_shutdown,
	.suspend = mtk_disp_mgr_suspend,
	.resume = mtk_disp_mgr_resume,
	.driver = {
		   .name = DISP_SESSION_DEVICE,
		   },
};

static void mtk_disp_mgr_device_release(struct device *dev)
{

}

static u64 mtk_disp_mgr_dmamask = ~(u32) 0;

static struct platform_device mtk_disp_mgr_device = {
	.name = DISP_SESSION_DEVICE,
	.id = 0,
	.dev = {
		.release = mtk_disp_mgr_device_release,
		.dma_mask = &mtk_disp_mgr_dmamask,
		.coherent_dma_mask = 0xffffffff,
		},
	.num_resources = 0,
};

static int __init mtk_disp_mgr_init(void)
{
	pr_debug("mtk_disp_mgr_init\n");
	if (platform_device_register(&mtk_disp_mgr_device))
		return -ENODEV;


	if (platform_driver_register(&mtk_disp_mgr_driver)) {
		platform_device_unregister(&mtk_disp_mgr_device);
		return -ENODEV;
	}


	return 0;
}

static void __exit mtk_disp_mgr_exit(void)
{
	cdev_del(mtk_disp_mgr_cdev);
	unregister_chrdev_region(mtk_disp_mgr_devno, 1);

	platform_driver_unregister(&mtk_disp_mgr_driver);
	platform_device_unregister(&mtk_disp_mgr_device);

	device_destroy(mtk_disp_mgr_class, mtk_disp_mgr_devno);
	class_destroy(mtk_disp_mgr_class);
}
module_init(mtk_disp_mgr_init);
module_exit(mtk_disp_mgr_exit);

MODULE_DESCRIPTION("MediaTek Display Manager");
