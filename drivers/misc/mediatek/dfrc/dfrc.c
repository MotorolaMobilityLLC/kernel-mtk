/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */
#include "dfrc.h"

#include <linux/module.h>
#include <linux/device.h>
#include <linux/list.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>
#include <linux/kthread.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>

#include <ged_frr.h>

#include "dfrc_drv.h"

#define DFRC_DEVNAME "mtk_dfrc"

#define DFRC_LOG_LEVEL 4

#if (DFRC_LOG_LEVEL > 0)
#define DFRC_ERR(x, ...) pr_err("[DFRC] " x, ##__VA_ARGS__)
#else
#define DFRC_ERR(...)
#endif

#if (DFRC_LOG_LEVEL > 1)
#define DFRC_WRN(x, ...) pr_warn("[DFRC] " x, ##__VA_ARGS__)
#else
#define DFRC_WRN(...)
#endif

#if (DFRC_LOG_LEVEL > 2)
#define DFRC_INFO(x, ...) pr_info("[DFRC] " x, ##__VA_ARGS__)
#else
#define DFRC_INFO(...)
#endif

#if (DFRC_LOG_LEVEL > 3)
#define DFRC_DBG(x, ...) pr_debug("[DFRC] " x, ##__VA_ARGS__)
#else
#define DFRC_DBG(...)
#endif

struct DFRC_DRV_POLICY_NODE {
	struct DFRC_DRV_POLICY policy;
	struct list_head list;
};

struct DFRC_DRV_POLICY_ARR_STATISTICS {
	int num_config;
	unsigned long long sequence;
	int fps;
	int mode;
	struct DFRC_DRV_POLICY policy;
};

struct DFRC_DRV_POLICY_FRR_STATISTICS {
	int num_config;
	struct list_head list;
};

struct DFRC_DRV_PANEL_INFO_LIST {
	int support_120;
	int support_90;
	int num;
	struct DFRC_DRV_REFRESH_RANGE *range;
};

/* these parameters are for device node and operation */
static dev_t dfrc_devno;
static struct cdev *dfrc_cdev;
static struct class *dfrc_class;
static struct mutex g_mutex_fop;
static int g_has_opened;

/* these parameters use to store policy list, focus pid, fg pid, and so on */
static struct mutex g_mutex_data;
static LIST_HEAD(g_fps_policy_list);
static int g_num_fps_policy;
static struct DFRC_DRV_HWC_INFO g_hwc_info;
static struct DFRC_DRV_INPUT_WINDOW_INFO g_input_window_info;
static int g_event_count;
static int g_processed_count;
static int g_cond_remake;
static int g_run_rrc_fps;
static int g_allow_rrc_policy = DFRC_ALLOW_VIDEO | DFRC_ALLOW_TOUCH;
static int g_init_done;
static struct DFRC_DRV_PANEL_INFO_LIST g_fps_info;
static struct DFRC_DRV_WINDOW_STATE g_window_state;
static struct DFRC_DRV_FOREGROUND_WINDOW_INFO g_fg_window_info;
static struct DFRC_DRV_VSYNC_REQUEST g_request_now;

/* these parameters use to record the request of rrc */
static struct mutex g_mutex_rrc;
static struct DFRC_DRV_POLICY g_policy_rrc_input;
static struct DFRC_DRV_POLICY g_policy_rrc_video;
static struct DFRC_DRV_POLICY g_policy_thermal;
static struct DFRC_DRV_POLICY g_policy_loading;

/* these parametersuse to notify sw vsync */
static struct mutex g_mutex_request;
static struct DFRC_DRV_VSYNC_REQUEST g_request_notified;
static struct DFRC_DRV_POLICY *g_request_policy;
static int g_cond_request;

/* use to store the current vsync mode. only use them in make_policy_kthread */
/* therefore they do not need to protect by mutex */
static int g_current_fps;
static int g_current_sw_mode;
static int g_current_hw_mode;

static wait_queue_head_t g_wq_vsync_request;
static wait_queue_head_t g_wq_make_policy;
static wait_queue_head_t g_wq_fps_change;
static struct task_struct *g_task_make_policy;

static bool g_stop_thread;

/* use to create debug node */
static int debug_init;
static struct dentry *debug_dir;
static struct dentry *debugfs_dump_info;
static struct dentry *debugfs_dump_reason;

/* use to store debug info */
#define DUMP_MAX_LENGTH (1024)
static char g_string_info[DUMP_MAX_LENGTH];
static ssize_t g_string_info_len;
static char g_string_reason[DUMP_MAX_LENGTH];
static ssize_t g_string_reason_len;

void dfrc_remake_policy_locked(void)
{
	g_event_count++;
	g_cond_remake = 1;
	wake_up_interruptible(&g_wq_make_policy);
}

long dfrc_reg_policy_locked(const struct DFRC_DRV_POLICY *policy)
{
	long res = 0L;
	struct list_head *iter;
	bool is_new = true;
	struct DFRC_DRV_POLICY_NODE *node;

	list_for_each(iter, &g_fps_policy_list) {
		node = list_entry(iter, struct DFRC_DRV_POLICY_NODE, list);
		if (node->policy.sequence == policy->sequence) {
			is_new = false;
			break;
		}
	}

	if (is_new) {
		node = vmalloc(sizeof(struct DFRC_DRV_POLICY_NODE));
		if (node == NULL) {
			DFRC_WRN("reg_fps_policy: failed to allocate memory\n");
			res =  -ENOMEM;
		} else {
			INIT_LIST_HEAD(&node->list);
			memcpy(&node->policy, policy, sizeof(struct DFRC_DRV_POLICY));
			list_add(&node->list, &g_fps_policy_list);
			g_num_fps_policy++;
			DFRC_INFO("reg_fps_policy: register policy[%llu]\n", node->policy.sequence);
		}
	} else {
		DFRC_INFO("reg_fps_policy: the policy[%llu] is existed\n", policy->sequence);
	}

	return res;
}

long dfrc_reg_policy(const struct DFRC_DRV_POLICY *policy)
{
	long res = 0L;

	mutex_lock(&g_mutex_data);
	res = dfrc_reg_policy_locked(policy);
	mutex_unlock(&g_mutex_data);
	return res;
}

void dfrc_init_kernel_policy(void)
{
	/* init rrc video policy */
	g_policy_rrc_video.sequence = DFRC_DRV_API_RRC_VIDEO;
	g_policy_rrc_video.api = DFRC_DRV_API_RRC_VIDEO;
	g_policy_rrc_video.pid = 0;
	g_policy_rrc_video.fps = -1;
	dfrc_reg_policy_locked(&g_policy_rrc_video);

	/* init rrc input policy */
	g_policy_rrc_input.sequence = DFRC_DRV_API_RRC_TOUCH;
	g_policy_rrc_input.api = DFRC_DRV_API_RRC_TOUCH;
	g_policy_rrc_input.pid = 0;
	g_policy_rrc_input.fps = -1;
	dfrc_reg_policy_locked(&g_policy_rrc_input);

	g_policy_thermal.sequence = DFRC_DRV_API_THERMAL;
	g_policy_thermal.api = DFRC_DRV_API_THERMAL;
	g_policy_thermal.pid = 0;
	g_policy_thermal.fps = -1;
	dfrc_reg_policy_locked(&g_policy_thermal);

	g_policy_loading.sequence = DFRC_DRV_API_LOADING;
	g_policy_loading.api = DFRC_DRV_API_LOADING;
	g_policy_loading.pid = 0;
	g_policy_loading.fps = -1;
	dfrc_reg_policy_locked(&g_policy_loading);
}

long dfrc_set_policy_locked(const struct DFRC_DRV_POLICY *policy)
{
	long res = 0L;
	struct list_head *iter;
	bool is_new = true;
	int change = false;
	struct DFRC_DRV_POLICY_NODE *node;

	list_for_each(iter, &g_fps_policy_list) {
		node = list_entry(iter, struct DFRC_DRV_POLICY_NODE, list);
		if (node->policy.sequence == policy->sequence) {
			is_new = false;
			if (node->policy.fps != policy->fps ||
					node->policy.mode != policy->mode ||
					node->policy.target_pid != policy->target_pid ||
					node->policy.gl_context_id != policy->gl_context_id) {
				change = true;
				if (node->policy.mode == DFRC_DRV_MODE_FRR &&
						node->policy.mode != policy->mode) {
					if (node->policy.api == DFRC_DRV_API_GIFT) {
						ged_frr_table_set_fps(node->policy.target_pid,
								(uint64_t)node->policy.gl_context_id,
								GED_FRR_TABLE_ZERO);
					} else {
						ged_frr_table_set_fps(GED_FRR_TABLE_FOR_ALL_PID,
								GED_FRR_TABLE_FOR_ALL_CID,
								GED_FRR_TABLE_ZERO);
					}
				}
				node->policy.fps = policy->fps;
				node->policy.mode = policy->mode;
				node->policy.target_pid = policy->target_pid;
				node->policy.gl_context_id = policy->gl_context_id;
				DFRC_INFO("set_policy: [%llu] fps:%d mode:%d t_pid:%d gl_id:%llu\n",
						node->policy.sequence, policy->fps, policy->mode,
						policy->target_pid, policy->gl_context_id);
				break;
			}
		}
	}

	if (is_new) {
		DFRC_INFO("set_policy: can not find policy[%llu]\n", policy->sequence);
		res = -ENODEV;
	} else if (change) {
		dfrc_remake_policy_locked();
	}

	return res;
}

long dfrc_set_policy(const struct DFRC_DRV_POLICY *policy)
{
	long res = 0L;

	mutex_lock(&g_mutex_data);
	res = dfrc_set_policy_locked(policy);
	mutex_unlock(&g_mutex_data);
	return res;
}

long dfrc_unreg_policy(const unsigned long long sequence)
{
	long res = 0L;
	struct list_head *iter;
	bool is_new = true;
	struct DFRC_DRV_POLICY_NODE *node;

	mutex_lock(&g_mutex_data);
	list_for_each(iter, &g_fps_policy_list) {
		node = list_entry(iter, struct DFRC_DRV_POLICY_NODE, list);
		if (node->policy.sequence == sequence) {
			if (node->policy.mode == DFRC_DRV_MODE_FRR) {
				if (node->policy.api == DFRC_DRV_API_GIFT) {
					ged_frr_table_set_fps(node->policy.target_pid,
							(uint64_t)node->policy.gl_context_id,
							GED_FRR_TABLE_ZERO);
				} else {
					ged_frr_table_set_fps(GED_FRR_TABLE_FOR_ALL_PID,
							GED_FRR_TABLE_FOR_ALL_CID,
							GED_FRR_TABLE_ZERO);
				}
			}
			is_new = false;
			list_del(&node->list);
			vfree(node);
			g_num_fps_policy--;
			DFRC_INFO("unreg_fps_policy: unregister policy[%llu]", sequence);
			break;
		}
	}

	if (is_new) {
		DFRC_INFO("unreg_fps_policy: can not find policy[%llu]\n", sequence);
		res = -ENODEV;
	} else {
		dfrc_remake_policy_locked();
	}

	mutex_unlock(&g_mutex_data);
	return res;
}

void dfrc_set_hwc_info(const struct DFRC_DRV_HWC_INFO *hwc_info)
{
	mutex_lock(&g_mutex_data);
	g_hwc_info = *hwc_info;
	dfrc_remake_policy_locked();
	mutex_unlock(&g_mutex_data);
}

void dfrc_set_input_window(const struct DFRC_DRV_INPUT_WINDOW_INFO *input_window_info)
{
	mutex_lock(&g_mutex_data);
	g_input_window_info = *input_window_info;
	dfrc_remake_policy_locked();
	mutex_unlock(&g_mutex_data);
}

void dfrc_reset_state(void)
{
	struct list_head *iter, *next;
	struct DFRC_DRV_POLICY_NODE *node;

	mutex_lock(&g_mutex_data);
	g_run_rrc_fps = 1;
	g_allow_rrc_policy = DFRC_ALLOW_VIDEO;
	list_for_each_safe(iter, next, &g_fps_policy_list) {
		node = list_entry(iter, struct DFRC_DRV_POLICY_NODE, list);
		if (node->policy.pid != 0) {
			list_del(&node->list);
			vfree(node);
			g_num_fps_policy--;
		}
	}
	dfrc_remake_policy_locked();
	mutex_unlock(&g_mutex_data);
}

long dfrc_get_request_set(struct DFRC_DRC_REQUEST_SET *request_set)
{
	long res = 0L;
	int size;

	mutex_lock(&g_mutex_request);
	if (g_request_policy != NULL && request_set->policy != NULL) {
		size = request_set->num > g_request_notified.num_policy ? g_request_notified.num_policy :
				request_set->num;
		size *= sizeof(struct DFRC_DRV_POLICY);
		if (copy_to_user((void *)request_set->policy, g_request_policy, size)) {
			DFRC_WRN("get_request_set: failed to copy data to user\n");
			res = -EFAULT;
		}
	}
	mutex_unlock(&g_mutex_request);
	return res;
}

void dfrc_get_vsync_request(struct DFRC_DRV_VSYNC_REQUEST *request)
{
	mutex_lock(&g_mutex_request);
	if (!memcmp(request, &g_request_notified, sizeof(g_request_notified))) {
		mutex_unlock(&g_mutex_request);
		wait_event_interruptible(g_wq_vsync_request, g_cond_request);
		mutex_lock(&g_mutex_request);
		g_cond_request = 0;
	}
	*request = g_request_notified;
	mutex_unlock(&g_mutex_request);
}

void dfrc_get_panel_info(struct DFRC_DRV_PANEL_INFO *panel_info)
{
	mutex_lock(&g_mutex_data);
	panel_info->support_120 = g_fps_info.support_120;
	panel_info->support_90 = g_fps_info.support_90;
	panel_info->num = g_fps_info.num;
	mutex_unlock(&g_mutex_data);
}

int dfrc_allow_rrc_adjust_fps(void)
{
	int allow;

	mutex_lock(&g_mutex_data);
	allow = g_allow_rrc_policy;
	mutex_unlock(&g_mutex_data);
	return allow;
}

static int rrc_fps_is_invalid_fps_locked(int fps)
{
	int res = 1;
	int i;

	if (fps == -1) {
		return 0;
	}

	for (i = 0; i < g_fps_info.num; i++) {
		if (g_fps_info.range[i].min_fps <= fps && fps <= g_fps_info.range[i].max_fps) {
			res = 0;
			break;
		}
	}
	return res;
}

void dfrc_set_window_state(const struct DFRC_DRV_WINDOW_STATE *window_state)
{
	mutex_lock(&g_mutex_data);
	g_window_state = *window_state;
	dfrc_remake_policy_locked();
	mutex_unlock(&g_mutex_data);
}

void dfrc_set_fg_window(const struct DFRC_DRV_FOREGROUND_WINDOW_INFO *fg_window_info)
{
	mutex_lock(&g_mutex_data);
	g_fg_window_info = *fg_window_info;
	dfrc_remake_policy_locked();
	mutex_unlock(&g_mutex_data);
}

long dfrc_set_kernel_policy(int api, int fps, int mode, int target_pid, unsigned long long gl_context_id)
{
	long res = 0L;
	struct DFRC_DRV_POLICY *temp;

	mutex_lock(&g_mutex_data);
	if (!g_init_done) {
		res = -ENODEV;
		DFRC_WRN("[RRC_DRV] api_%d failed to set %d fps: not ready\n", api, fps);
		goto set_kernel_policy_exit;
	}

	if (rrc_fps_is_invalid_fps_locked(fps)) {
		res = -EINVAL;
		DFRC_WRN("[RRC_DRV] api_%d failed to set %d fps: fps is invalid\n", api, fps);
		goto set_kernel_policy_exit;
	}

	switch (api) {
	case DFRC_DRV_API_RRC_TOUCH:
		temp = &g_policy_rrc_input;
		break;
	case DFRC_DRV_API_RRC_VIDEO:
		temp = &g_policy_rrc_video;
		break;
	case DFRC_DRV_API_THERMAL:
		temp = &g_policy_thermal;
		break;
	case DFRC_DRV_API_LOADING:
		temp = &g_policy_loading;
		break;
	default:
		DFRC_INFO("[RRC_DRV] api_%d failed to set %d fps: api is invalid\n", api, fps);
		temp = NULL;
		res = -EINVAL;
		break;
	}
	if (temp != NULL) {
		temp->fps = fps;
		temp->mode = mode;
		temp->target_pid = target_pid;
		temp->gl_context_id = gl_context_id;
		dfrc_set_policy_locked(temp);
	}

set_kernel_policy_exit:
	mutex_unlock(&g_mutex_data);

	return res;
}

long dfrc_get_panel_info_number(int *num)
{
	long res = 0L;

	mutex_lock(&g_mutex_data);
	if (!g_init_done) {
		res = -ENODEV;
		DFRC_WRN("failed to get info number: does not init\n");
	}

	if (g_fps_info.num == 0) {
		DFRC_WRN("failed to get info number: size is 0\n");
		*num = 1;
	} else {
		*num = g_fps_info.num;
	}
	mutex_unlock(&g_mutex_data);

	return res;
}

long dfrc_get_panel_fps(struct DFRC_DRV_REFRESH_RANGE *range)
{
	int res = 0;

	mutex_lock(&g_mutex_data);
	if (!g_init_done) {
		res = -ENODEV;
		DFRC_INFO("failed to get panel fps: does not init\n");
	}

	if (g_fps_info.num == 0) {
		range->min_fps = 60;
		range->max_fps = 60;
	} else if (range->index >= g_fps_info.num || range->index < 0) {
		range->min_fps = 60;
		range->max_fps = 60;
		res = -EINVAL;
	} else {
		range->min_fps = g_fps_info.range[range->index].min_fps;
		range->max_fps = g_fps_info.range[range->index].max_fps;
	}

	mutex_unlock(&g_mutex_data);
	return res;
}

static long dfrc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct DFRC_DRV_POLICY policy;
	unsigned long long sequence;
	struct DFRC_DRV_HWC_INFO hwc_info;
	struct DFRC_DRV_INPUT_WINDOW_INFO input_window_info;
	struct DFRC_DRV_VSYNC_REQUEST request;
	struct DFRC_DRV_PANEL_INFO panel_info;
	struct DFRC_DRV_REFRESH_RANGE range;
	struct DFRC_DRV_WINDOW_STATE window_state;
	struct DFRC_DRV_FOREGROUND_WINDOW_INFO fg_window_info;
	struct DFRC_DRC_REQUEST_SET request_set;
	long res = 0L;

	switch (cmd) {
	case DFRC_IOCTL_CMD_REG_POLICY:
		if (copy_from_user(&policy, (void *)arg, sizeof(policy))) {
			DFRC_WRN("reg_fps_policy : failed to copy data from user\n");
			return -EFAULT;
		}
		res = dfrc_reg_policy(&policy);
		if (res)
			DFRC_WRN("reg_fps_policy : failed to register fps policy\n");
		break;

	case DFRC_IOCTL_CMD_SET_POLICY:
		if (copy_from_user(&policy, (void *)arg, sizeof(policy))) {
			DFRC_WRN("set_fps_policy : failed to copy data from user\n");
			return -EFAULT;
		}
		res = dfrc_set_policy(&policy);
		if (res)
			DFRC_WRN("set_fps_policy : failed to set fps policy with %dfps\n",
					policy.fps);
		break;

	case DFRC_IOCTL_CMD_UNREG_POLICY:
		if (copy_from_user(&sequence, (void *)arg, sizeof(sequence))) {
			DFRC_WRN("set_unreg_policy : failed to copy data from user\n");
			return -EFAULT;
		}
		res = dfrc_unreg_policy(sequence);
		if (res)
			DFRC_WRN("set_unreg_policy : failed to unreg fps policy\n");
		break;

	case DFRC_IOCTL_CMD_SET_HWC_INFO:
		if (copy_from_user(&hwc_info, (void *)arg, sizeof(hwc_info))) {
			DFRC_WRN("set_hwc_info : failed to copy data from user\n");
			return -EFAULT;
		}
		dfrc_set_hwc_info(&hwc_info);
		break;

	case DFRC_IOCTL_CMD_SET_INPUT_WINDOW:
		if (copy_from_user(&input_window_info, (void *)arg, sizeof(input_window_info))) {
			DFRC_WRN("set_input_window_info : failed to copy data from user\n");
			return -EFAULT;
		}
		dfrc_set_input_window(&input_window_info);
		break;

	case DFRC_IOCTL_CMD_RESET_STATE:
		dfrc_reset_state();
		break;

	case DFRC_IOCTL_CMD_GET_REQUEST_SET:
		if (copy_from_user(&request_set, (void *)arg, sizeof(request_set))) {
			DFRC_WRN("get_request_set: failed to copy data from user\n");
			return -EFAULT;
		}
		res = dfrc_get_request_set(&request_set);
		break;

	case DFRC_IOCTL_CMD_GET_VSYNC_REQUEST:
		if (copy_from_user(&request, (void *)arg, sizeof(request))) {
			DFRC_WRN("get_vsync_request: failed to copy data from user\n");
			return -EFAULT;
		}
		dfrc_get_vsync_request(&request);
		if (copy_to_user((void *)arg, &request, sizeof(request))) {
			DFRC_WRN("get_vsync_request: failed to copy data to user\n");
			return -EFAULT;
		}
		break;

	case DFRC_IOCTL_CMD_GET_PANEL_INFO:
		dfrc_get_panel_info(&panel_info);
		if (copy_to_user((void *)arg, &panel_info, sizeof(panel_info))) {
			DFRC_WRN("get_panel_info: failed to copy data to user\n");
			return -EFAULT;
		}
		break;

	case DFRC_IOCTL_CMD_GET_REFRESH_RANGE:
		if (copy_from_user(&range, (void *)arg, sizeof(range))) {
			DFRC_WRN("get_refresh_range: failed to copy data from user\n");
			return -EFAULT;
		}
		dfrc_get_panel_fps(&range);
		if (copy_to_user((void *)arg, &range, sizeof(range))) {
			DFRC_WRN("get_refresh_range: failed to copy data to user\n");
			return -EFAULT;
		}
		break;

	case DFRC_IOCTL_CMD_SET_WINDOW_STATE:
		if (copy_from_user(&window_state, (void *)arg, sizeof(window_state))) {
			DFRC_WRN("set_window_state : failed to copy data from user\n");
			return -EFAULT;
		}
		dfrc_set_window_state(&window_state);
		break;

	case DFRC_IOCTL_CMD_SET_FOREGROUND_WINDOW:
		if (copy_from_user(&fg_window_info, (void *)arg, sizeof(fg_window_info))) {
			DFRC_WRN("set_fg_window : failed to copy data from user\n");
			return -EFAULT;
		}
		dfrc_set_fg_window(&fg_window_info);
		break;

	default:
		return -ENODATA;
	}

	return res;
}

#if IS_ENABLED(CONFIG_COMPAT)
static long compat_dfrc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case DFRC_IOCTL_CMD_REG_POLICY:
	case DFRC_IOCTL_CMD_SET_POLICY:
	case DFRC_IOCTL_CMD_UNREG_POLICY:
	case DFRC_IOCTL_CMD_SET_HWC_INFO:
	case DFRC_IOCTL_CMD_SET_INPUT_WINDOW:
	case DFRC_IOCTL_CMD_RESET_STATE:
	case DFRC_IOCTL_CMD_GET_REQUEST_SET:
	case DFRC_IOCTL_CMD_GET_VSYNC_REQUEST:
	case DFRC_IOCTL_CMD_GET_PANEL_INFO:
		break;

	default:
		return -ENODATA;
	}

	return 0L;
}
#endif

static void dfrc_idump(const char *fmt, ...)
{
	va_list vargs;
	size_t tmp;

	va_start(vargs, fmt);

	if (g_string_info_len >= DUMP_MAX_LENGTH - 32) {
		va_end(vargs);
		return;
	}

	tmp = vscnprintf(g_string_info + g_string_info_len,
			DUMP_MAX_LENGTH - g_string_info_len, fmt, vargs);
	g_string_info_len += tmp;
	if (g_string_info_len > DUMP_MAX_LENGTH)
		g_string_info_len = DUMP_MAX_LENGTH;

	va_end(vargs);
}

static void dfrc_rdump(const char *fmt, ...)
{
	va_list vargs;
	size_t tmp;

	va_start(vargs, fmt);

	if (g_string_reason_len >= DUMP_MAX_LENGTH - 32) {
		va_end(vargs);
		return;
	}

	tmp = vscnprintf(g_string_reason + g_string_reason_len,
			DUMP_MAX_LENGTH - g_string_reason_len, fmt, vargs);
	g_string_reason_len += tmp;
	if (g_string_reason_len > DUMP_MAX_LENGTH)
		g_string_reason_len = DUMP_MAX_LENGTH;

	va_end(vargs);
}

static void dfrc_dump_info(void)
{
	int i;

	dfrc_idump("Number of display[%d]  Single layer[%d]\n",
			g_hwc_info.num_display, g_hwc_info.single_layer);
	dfrc_idump("Force window pid[%d]\n", g_input_window_info.pid);
	dfrc_idump("Foreground window pid[%d]\n", g_fg_window_info.pid);
	dfrc_idump("Window state[%08x]\n", g_window_state);
	dfrc_idump("Allow RRC policy[0x%x]\n", g_allow_rrc_policy);
	dfrc_idump("Support panel refresh rate: %d\n", g_fps_info.num);
	for (i = 0; i < g_fps_info.num; i++) {
		dfrc_idump("    [%d] %d~%d\n",
				i, g_fps_info.range[i].min_fps, g_fps_info.range[i].max_fps);
	}
}

static void dfrc_dump_policy_list(void)
{
	struct list_head *iter;
	int i = 0;
	struct DFRC_DRV_POLICY_NODE *node;

	dfrc_idump("All Fps Policy\n");
	list_for_each(iter, &g_fps_policy_list) {
		node = list_entry(iter, struct DFRC_DRV_POLICY_NODE, list);
		dfrc_idump("    [%d]  sequence[%llu]  api[%d]  pid[%d]  fps[%d]  mode[%d]  ",
				i, node->policy.sequence, node->policy.api,
				node->policy.pid, node->policy.fps, node->policy.mode);
		dfrc_idump("target_pid[%d]  context_id[%p]\n",
				node->policy.target_pid, node->policy.gl_context_id);
		i++;
	}
}

static void dfrc_reset_info_buffer(void)
{
	g_string_info_len = 0;
	memset(g_string_info, 0, sizeof(g_string_info));
}

static void dfrc_reset_reason_buffer(void)
{
	g_string_reason_len = 0;
	memset(g_string_reason, 0, sizeof(g_string_reason));
}

static ssize_t dfrc_debug_dump_info_read(struct file *file, char __user *buf, size_t size,
		loff_t *ppos)
{
	mutex_lock(&g_mutex_data);
	dfrc_reset_info_buffer();
	dfrc_dump_info();
	dfrc_dump_policy_list();
	mutex_unlock(&g_mutex_data);
	return simple_read_from_buffer(buf, size, ppos, g_string_info, g_string_info_len);
}

static ssize_t dfrc_debug_dump_reason_read(struct file *file, char __user *buf, size_t size,
		loff_t *ppos)
{
	ssize_t res;

	mutex_lock(&g_mutex_data);
	res = simple_read_from_buffer(buf, size, ppos, g_string_reason, g_string_reason_len);
	mutex_unlock(&g_mutex_data);
	return res;
}

static void dfrc_rdump_vsync_request(struct DFRC_DRV_VSYNC_REQUEST *request)
{
	dfrc_rdump("fps:%d  mode:%d  sw_fps:%d  sw_mode: %d  valid_info:%d  transient:%d  num:%d\n",
		request->fps, request->mode, request->sw_fps, request->sw_mode,
		request->valid_info, request->transient_state, request->num_policy);
}

static void dfrc_rdump_arr_statistics(struct DFRC_DRV_POLICY_ARR_STATISTICS *arr_statistics)
{
	dfrc_rdump("num:%d\n", arr_statistics->num_config);
	dfrc_rdump("    + seq:%llu  fps:%d  mode:%d\n", arr_statistics->sequence,
			arr_statistics->fps, arr_statistics->mode);
	dfrc_rdump("      policy detail:\n");
	dfrc_rdump("      seq[%llu]  api[%d]  pid[%d]  fps[%d]  mode[%d]  t_pid[%d]  gl_id[%llu]\n",
			arr_statistics->policy.sequence, arr_statistics->policy.api,
			arr_statistics->policy.pid, arr_statistics->policy.mode,
			arr_statistics->policy.target_pid, arr_statistics->policy.gl_context_id);
}

static void dfrc_rdump_frr_statistics(struct DFRC_DRV_POLICY_FRR_STATISTICS *frr_statistics)
{
	struct list_head *iter;
	struct DFRC_DRV_POLICY_NODE *node;
	struct DFRC_DRV_POLICY *policy;

	dfrc_rdump("num:%d\n", frr_statistics->num_config);
	list_for_each(iter, &frr_statistics->list) {
		node = list_entry(iter, struct DFRC_DRV_POLICY_NODE, list);
		policy = &node->policy;
		dfrc_rdump("+    seq[%llu]  api[%d]  pid[%d]  mode[%d]  t_pid[%d]  gl_id[%d]\n",
				policy->sequence, policy->api, policy->pid,
				policy->mode, policy->target_pid, policy->gl_context_id);
	}
}

static void dfrc_add_policy_to_statistics(struct DFRC_DRV_POLICY *policy,
					struct DFRC_DRV_POLICY_ARR_STATISTICS *arr_statistics,
					struct DFRC_DRV_POLICY_FRR_STATISTICS *frr_statistics)
{
	struct DFRC_DRV_POLICY_ARR_STATISTICS *arr_set;
	struct DFRC_DRV_POLICY_FRR_STATISTICS *frr_set;
	struct DFRC_DRV_POLICY_NODE *node;

	if (policy->mode == DFRC_DRV_MODE_FRR) {
		frr_set = &frr_statistics[policy->api];
		frr_set->num_config++;
		node = vmalloc(sizeof(struct DFRC_DRV_POLICY_NODE));
		if (node == NULL) {
			DFRC_WRN("forage_police: faile to allocate memory\n");
		} else {
			INIT_LIST_HEAD(&node->list);
			memcpy(&node->policy, policy, sizeof(struct DFRC_DRV_POLICY));
			list_add(&node->list, &frr_statistics->list);
		}
	} else if (policy->mode == DFRC_DRV_MODE_ARR) {
		arr_set = &arr_statistics[policy->api];
		arr_set->num_config++;
		if (policy->fps > arr_set->fps) {
			arr_set->sequence = policy->sequence;
			arr_set->fps = policy->fps;
			arr_set->mode = policy->mode;
			arr_set->policy = *policy;
		}
	}
}

static int dfrc_forage_police_locked(struct DFRC_DRV_POLICY_ARR_STATISTICS *arr_statistics,
					struct DFRC_DRV_POLICY_FRR_STATISTICS *frr_statistics)
{
	struct list_head *iter;
	struct DFRC_DRV_POLICY *policy;
	int num_valid_policy = 0;
	struct DFRC_DRV_POLICY_NODE *node;

	if (g_num_fps_policy == 0) {
		dfrc_rdump("Can not find any config, use default setting\n");
	} else if (g_window_state.window_flag & DFRC_DRV_FLAG_MULTI_WINDOW) {
		dfrc_rdump("Multi-Window is enabled, use default setting\n");
	} else if (g_hwc_info.num_display > 1) {
		dfrc_rdump("In the Multi-Display scenariom use default setting\n");
	} else {
		list_for_each(iter, &g_fps_policy_list) {
			node = list_entry(iter, struct DFRC_DRV_POLICY_NODE, list);
			policy = &node->policy;
			if (policy->fps == -1)
				continue;

			/* change condition from touch window to fore window */
			if (policy->api == DFRC_DRV_API_GIFT &&
					policy->target_pid == g_fg_window_info.pid) {
				num_valid_policy++;
				dfrc_add_policy_to_statistics(policy, arr_statistics,
						frr_statistics);
			} else if (policy->api == DFRC_DRV_API_VIDEO) {
			} else if (policy->api == DFRC_DRV_API_RRC_TOUCH) {
			} else if (policy->api == DFRC_DRV_API_RRC_VIDEO) {
			} else if (policy->api == DFRC_DRV_API_THERMAL) {
				num_valid_policy++;
				dfrc_add_policy_to_statistics(policy, arr_statistics,
						frr_statistics);
			} else if (policy->api == DFRC_DRV_API_LOADING) {
				num_valid_policy++;
				dfrc_add_policy_to_statistics(policy, arr_statistics,
						frr_statistics);
			} else if (policy->api == DFRC_DRV_API_WHITELIST) {
				num_valid_policy++;
				dfrc_add_policy_to_statistics(policy, arr_statistics,
						frr_statistics);
			}
		}
	}

	return num_valid_policy;
}

static void dfrc_select_policy_locked(int *which,
		struct DFRC_DRV_POLICY_ARR_STATISTICS **expected_arr,
		struct DFRC_DRV_POLICY_FRR_STATISTICS **expected_frr,
		struct DFRC_DRV_POLICY_ARR_STATISTICS *arr_statistics,
		struct DFRC_DRV_POLICY_FRR_STATISTICS *frr_statistics)
{
	struct DFRC_DRV_POLICY_ARR_STATISTICS *arr_set;
	int arr_upper_bound = -1;

	*which = DFRC_DRV_MODE_DEFAULT;
	*expected_arr = NULL;
	*expected_frr = NULL;

	if (frr_statistics[DFRC_DRV_API_THERMAL].num_config) {
		*which = DFRC_DRV_MODE_FRR;
		*expected_frr = &frr_statistics[DFRC_DRV_API_THERMAL];
		dfrc_rdump("choose thermal config with frr\n");
		return;
	} else if (arr_statistics[DFRC_DRV_API_THERMAL].num_config) {
		*which = DFRC_DRV_MODE_ARR;
		*expected_arr = &arr_statistics[DFRC_DRV_API_THERMAL];
		arr_upper_bound = arr_statistics[DFRC_DRV_API_THERMAL].fps;
		dfrc_rdump("choose thermal config with arr\n");
	}

	if (frr_statistics[DFRC_DRV_API_LOADING].num_config && *which == DFRC_DRV_MODE_DEFAULT) {
		*which = DFRC_DRV_MODE_FRR;
		*expected_frr = &frr_statistics[DFRC_DRV_API_LOADING];
		dfrc_rdump("choose loading config with frr\n");
		return;
	} else if (arr_statistics[DFRC_DRV_API_LOADING].num_config &&
			(*which == DFRC_DRV_MODE_DEFAULT || *which == DFRC_DRV_MODE_ARR)) {
		*which = DFRC_DRV_MODE_ARR;
		arr_set = &arr_statistics[DFRC_DRV_API_LOADING];
		if (arr_upper_bound == -1 || arr_set->fps < arr_upper_bound) {
			*expected_arr = arr_set;
			if (arr_upper_bound != -1 && arr_set->fps < arr_upper_bound)
				dfrc_rdump("loading config is lower than upper bound\n");
		}
		dfrc_rdump("choose loading config with arr\n");
		return;
	}

	if (frr_statistics[DFRC_DRV_API_WHITELIST].num_config && *which == DFRC_DRV_MODE_DEFAULT) {
		*which = DFRC_DRV_MODE_FRR;
		*expected_frr = &frr_statistics[DFRC_DRV_API_WHITELIST];
		dfrc_rdump("choose whitelist config with frr\n");
		return;
	} else if (arr_statistics[DFRC_DRV_API_WHITELIST].num_config &&
			(*which == DFRC_DRV_MODE_DEFAULT || *which == DFRC_DRV_MODE_ARR)) {
		*which = DFRC_DRV_MODE_ARR;
		arr_set = &arr_statistics[DFRC_DRV_API_WHITELIST];
		if (arr_upper_bound == -1 || arr_set->fps < arr_upper_bound) {
			*expected_arr = arr_set;
			if (arr_upper_bound != -1 && arr_set->fps < arr_upper_bound)
				dfrc_rdump("whitelist config is lower than upper bound\n");
		}
		dfrc_rdump("choose whitelist config with arr\n");
		return;
	}

	if (frr_statistics[DFRC_DRV_API_GIFT].num_config && *which == DFRC_DRV_MODE_DEFAULT) {
		*which = DFRC_DRV_MODE_FRR;
		*expected_frr = &frr_statistics[DFRC_DRV_API_GIFT];
		dfrc_rdump("choose gift config with frr\n");
		return;
	} else if (arr_statistics[DFRC_DRV_API_GIFT].num_config &&
			(*which == DFRC_DRV_MODE_DEFAULT || *which == DFRC_DRV_MODE_ARR)) {
		*which = DFRC_DRV_MODE_ARR;
		arr_set = &arr_statistics[DFRC_DRV_API_GIFT];
		if (arr_upper_bound == -1 || arr_set->fps < arr_upper_bound) {
			*expected_arr = arr_set;
			if (arr_upper_bound != -1 && arr_set->fps < arr_upper_bound)
				dfrc_rdump("gift config is lower than upper bound\n");
		}
		dfrc_rdump("choose gift config with arr\n");
		return;
	}
}

static void dfrc_pack_choosed_frr_policy(int num, struct DFRC_DRV_POLICY *new_policy,
					struct DFRC_DRV_POLICY_FRR_STATISTICS *frr_statistics)
{
	int i = 0;
	struct list_head *iter;
	struct DFRC_DRV_POLICY_NODE *node;

	list_for_each(iter, &frr_statistics->list) {
		node = list_entry(iter, struct DFRC_DRV_POLICY_NODE, list);
		if (i < num)
			new_policy[i] = node->policy;
		else
			break;
		i++;
	}
}

static void dfrc_pack_choosed_arr_policy(int num, struct DFRC_DRV_POLICY *new_policy,
					struct DFRC_DRV_POLICY_ARR_STATISTICS *arr_statistics)
{
	int i = 0;

	if (num >= 1) {
		new_policy[i] = arr_statistics->policy;
	}
}

static void dfrc_adjust_vsync(int which,
				struct DFRC_DRV_POLICY_ARR_STATISTICS *expected_arr,
				struct DFRC_DRV_POLICY_FRR_STATISTICS *expected_frr,
				struct DFRC_DRV_POLICY_ARR_STATISTICS *arr_statistics,
				struct DFRC_DRV_POLICY_FRR_STATISTICS *frr_statistics)
{
	bool change = false;
	struct DFRC_DRV_VSYNC_REQUEST new_request;
	int fps = -1;
	int sw_mode = DFRC_DRV_SW_MODE_CALIBRATED_SW;
	int hw_mode = DFRC_DRV_HW_MODE_ARR;
	struct DFRC_DRV_POLICY *new_policy = NULL;
	int size;
	struct list_head *iter;
	struct DFRC_DRV_POLICY_NODE *node;

	memset(&new_request, 0, sizeof(new_request));
	if (which == DFRC_DRV_MODE_DEFAULT) {
		dfrc_rdump("use default mode\n");
		fps = -1;
		sw_mode = DFRC_DRV_SW_MODE_CALIBRATED_SW;
		hw_mode = DFRC_DRV_HW_MODE_DEFAULT;
		new_request.fps = -1;
		new_request.mode = DFRC_DRV_MODE_DEFAULT;
		new_request.sw_fps = 60;
		new_request.sw_mode = DFRC_DRV_SW_MODE_CALIBRATED_SW;
		new_request.valid_info = false;
		new_request.transient_state = false;
		new_request.num_policy = 0;
		dfrc_rdump_vsync_request(&new_request);
		ged_frr_table_set_fps(GED_FRR_TABLE_FOR_ALL_PID, GED_FRR_TABLE_FOR_ALL_CID, GED_FRR_TABLE_ZERO);
	} else if (which == DFRC_DRV_MODE_FRR) {
		dfrc_rdump("use frr mode\n");
		fps = 60;
		sw_mode = DFRC_DRV_SW_MODE_CALIBRATED_SW;
		hw_mode = DFRC_DRV_HW_MODE_DEFAULT;
		new_request.fps = -1;
		new_request.mode = DFRC_DRV_MODE_FRR;
		new_request.sw_fps = 60;
		new_request.sw_mode = DFRC_DRV_SW_MODE_CALIBRATED_SW;
		new_request.valid_info = false;
		new_request.transient_state = false;
		new_request.num_policy = expected_frr->num_config;
		dfrc_rdump_vsync_request(&new_request);
		dfrc_rdump_frr_statistics(frr_statistics);

		new_policy = vmalloc(sizeof(struct DFRC_DRV_POLICY) * expected_frr->num_config);
		dfrc_pack_choosed_frr_policy(expected_frr->num_config, new_policy, frr_statistics);

		list_for_each(iter, &frr_statistics->list) {
			node = list_entry(iter, struct DFRC_DRV_POLICY_NODE, list);
			if (node->policy.api != DFRC_DRV_API_GIFT) {
				ged_frr_table_set_fps(GED_FRR_TABLE_FOR_ALL_PID,
						GED_FRR_TABLE_FOR_ALL_CID, node->policy.fps);
			} else {
				ged_frr_table_set_fps(node->policy.target_pid,
						(uint64_t)node->policy.gl_context_id,
						node->policy.fps);
			}
		}
	} else if (which == DFRC_DRV_MODE_ARR) {
		dfrc_rdump("use arr mode\n");
		fps = expected_arr->fps;
		sw_mode = DFRC_DRV_SW_MODE_PASS_HW;
		hw_mode = DFRC_DRV_HW_MODE_ARR;
		new_request.fps = expected_arr->fps;
		new_request.mode = expected_arr->mode;
		new_request.sw_fps = expected_arr->fps;
		new_request.sw_mode = DFRC_DRV_SW_MODE_PASS_HW;
		new_request.valid_info = true;
		new_request.transient_state = false;
		new_request.num_policy = 1;
		if (g_input_window_info.pid != g_fg_window_info.pid &&
				(expected_arr->policy.api != DFRC_DRV_API_THERMAL ||
				expected_arr->policy.api != DFRC_DRV_API_LOADING)) {
			new_request.fps = 60;
			new_request.transient_state = true;
		}
		dfrc_rdump_vsync_request(&new_request);
		dfrc_rdump_arr_statistics(arr_statistics);

		new_policy = vmalloc(sizeof(struct DFRC_DRV_POLICY));
		dfrc_pack_choosed_arr_policy(1, new_policy, arr_statistics);
		ged_frr_table_set_fps(GED_FRR_TABLE_FOR_ALL_PID, GED_FRR_TABLE_FOR_ALL_CID,
				GED_FRR_TABLE_UPPER_BOUND_FPS);
	}

	if (memcmp(&new_request, &g_request_now, sizeof(g_request_now))) {
		change = true;
	} else {
		if ((new_policy != NULL && g_request_policy == NULL) ||
				(new_policy == NULL && g_request_policy != NULL)) {
			change = true;
		} else if (new_policy != NULL && g_request_policy != NULL) {
			size = sizeof(struct DFRC_DRV_POLICY) * new_request.num_policy;
			if (memcmp(new_policy, g_request_policy, size)) {
				change = true;
			}
		}
	}

	if (change) {
		mutex_lock(&g_mutex_request);
		g_request_notified = new_request;
		if (g_request_policy != NULL)
			vfree(g_request_policy);
		g_request_policy = new_policy;
		g_cond_request = 1;
		wake_up_interruptible(&g_wq_vsync_request);
		mutex_unlock(&g_mutex_request);
	}

	/* change the fps of hw vsync */
	if (fps != g_current_fps || hw_mode != g_current_hw_mode) {
		/*primary_display_force_set_vsync_fps(fps);*/
	}

	/* fps is changed, notify user space processes */
	if (change) {
		DFRC_INFO("adjust vsync: [%d|%d|%d] -> [%d|%d|%d]",
				g_current_fps, g_current_sw_mode, g_current_hw_mode,
				fps, sw_mode, hw_mode);
		g_current_fps = fps;
		g_current_sw_mode = sw_mode;
		g_current_hw_mode = hw_mode;
	}
}

static void dfrc_reset_statistics(struct DFRC_DRV_POLICY_ARR_STATISTICS *expected_arr,
					struct DFRC_DRV_POLICY_FRR_STATISTICS *expected_frr,
					struct DFRC_DRV_POLICY_ARR_STATISTICS *arr_statistics,
					struct DFRC_DRV_POLICY_FRR_STATISTICS *frr_statistics)
{
	int i;

	expected_arr = NULL;
	expected_frr = NULL;
	for (i = 0; i < DFRC_DRV_API_MAXIMUM; i++) {
		memset(&arr_statistics[i], 0, sizeof(struct DFRC_DRV_POLICY_ARR_STATISTICS));
		memset(&frr_statistics[i], 0, sizeof(struct DFRC_DRV_POLICY_FRR_STATISTICS));
		frr_statistics[i].list.next = &frr_statistics[i].list;
		frr_statistics[i].list.prev = &frr_statistics[i].list;
	}
}

static void dfrc_clear_statistics(struct DFRC_DRV_POLICY_ARR_STATISTICS *arr_statistics,
					struct DFRC_DRV_POLICY_FRR_STATISTICS *frr_statistics)
{
	struct list_head *iter, *next;
	struct DFRC_DRV_POLICY_NODE *node;
	struct DFRC_DRV_POLICY_FRR_STATISTICS *frr_set;
	int i;

	for (i = 0; i < DFRC_DRV_API_MAXIMUM; i++) {
		frr_set = &frr_statistics[i];
		list_for_each_safe(iter, next, &frr_set->list) {
			node = list_entry(iter, struct DFRC_DRV_POLICY_NODE, list);
			list_del(&node->list);
			vfree(node);
		}
	}
}

static int dfrc_make_policy_kthread_func(void *data)
{
	int which = DFRC_DRV_MODE_DEFAULT;
	struct DFRC_DRV_POLICY_ARR_STATISTICS arr_statistics[DFRC_DRV_API_MAXIMUM];
	struct DFRC_DRV_POLICY_FRR_STATISTICS frr_statistics[DFRC_DRV_API_MAXIMUM];
	struct DFRC_DRV_POLICY_ARR_STATISTICS *expected_arr = NULL;
	struct DFRC_DRV_POLICY_FRR_STATISTICS *expected_frr = NULL;
	int num;

	struct sched_param param = { .sched_priority = 94 };

	sched_setscheduler(current, SCHED_RR, &param);
	while (true) {
		mutex_lock(&g_mutex_data);
		if (g_stop_thread)
			return 0;

		/* if count is not equal to g_event_count, we has new request. */
		if (g_processed_count == g_event_count) {
			mutex_unlock(&g_mutex_data);
			wait_event_interruptible(g_wq_make_policy, g_cond_remake);
			mutex_lock(&g_mutex_data);
			g_cond_remake = 0;
		}

		dfrc_reset_statistics(expected_arr, expected_frr, arr_statistics, frr_statistics);

		/* find the new fps setting */
		dfrc_reset_reason_buffer();
		which = DFRC_DRV_MODE_DEFAULT;
		num = dfrc_forage_police_locked(arr_statistics, frr_statistics);
		if (num != 0) {
			dfrc_select_policy_locked(&which, &expected_arr, &expected_frr,
					arr_statistics, frr_statistics);
		}
		g_processed_count = g_event_count;
		mutex_unlock(&g_mutex_data);

		/* adjust vsync */
		dfrc_adjust_vsync(which, expected_arr, expected_frr,
				arr_statistics, frr_statistics);

		dfrc_clear_statistics(arr_statistics, frr_statistics);
	}

	return 0;
}

static int dfrc_init_param(void)
{
	mutex_init(&g_mutex_data);
	mutex_init(&g_mutex_request);
	mutex_init(&g_mutex_fop);
	mutex_init(&g_mutex_rrc);
	init_waitqueue_head(&g_wq_make_policy);
	init_waitqueue_head(&g_wq_vsync_request);
	init_waitqueue_head(&g_wq_fps_change);

	/* init thread to decide fps setting */
	g_task_make_policy = kthread_create(dfrc_make_policy_kthread_func, NULL,
			"dfrc_make_policy_kthread_func");
	if (IS_ERR(g_task_make_policy)) {
		DFRC_ERR("dfrc_init_param: failed to create dfrc_make_policy_kthread_func\n");
		return -ENODEV;
	}
	wake_up_process(g_task_make_policy);

	dfrc_rdump("Use initial setting\n");

	return 0;
}

static int dfrc_open(struct inode *inode, struct file *file)
{
	unsigned int *pStatus;
	int res = 0;

	mutex_lock(&g_mutex_fop);
	if (g_has_opened) {
		DFRC_WRN("device is busy\n");
		res = -EBUSY;
		goto open_exit;
	}

	file->private_data = vmalloc(sizeof(unsigned int));

	if (file->private_data == NULL) {
		DFRC_WRN("Not enough entry for RRC open operation\n");
		res = -ENOMEM;
		goto open_exit;
	}

	pStatus = (unsigned int *)file->private_data;
	*pStatus = 0;
	g_has_opened = 1;

open_exit:
	mutex_unlock(&g_mutex_fop);

	return res;
}

static int dfrc_release(struct inode *inode, struct file *file)
{
	int res = 0;

	mutex_lock(&g_mutex_fop);
	if (!g_has_opened) {
		res = -ENXIO;
		goto release_exit;
	}

	if (file->private_data != NULL) {
		vfree(file->private_data);
		file->private_data = NULL;
	}
	g_has_opened = 0;

release_exit:
	mutex_unlock(&g_mutex_fop);

	return res;
}

static const struct file_operations dfrc_fops = {
	.owner          = THIS_MODULE,
	.unlocked_ioctl = dfrc_ioctl,
#if IS_ENABLED(CONFIG_COMPAT)
	.compat_ioctl   = compat_dfrc_ioctl,
#endif
	.open           = dfrc_open,
	.release        = dfrc_release,
};

static const struct file_operations debug_fops_info = {
	.read = dfrc_debug_dump_info_read,
};

static const struct file_operations debug_fops_reason = {
	.read = dfrc_debug_dump_reason_read,
};

static void dfrc_debug_init(void)
{
	dfrc_reset_info_buffer();
	dfrc_reset_reason_buffer();

	if (!debug_init) {
		debug_init = 1;
		debug_dir = debugfs_create_dir("dfrc", NULL);
		if (debug_dir) {
			debugfs_dump_info = debugfs_create_file("info",
					S_IFREG | S_IRUGO, debug_dir, NULL,
					&debug_fops_info);

			debugfs_dump_reason = debugfs_create_file("reason",
					S_IFREG | S_IRUGO, debug_dir, NULL,
					&debug_fops_reason);
		}
	}
}

static int dfrc_probe(struct platform_device *pdev)
{
	struct class_device *class_dev = NULL;
	int ret = 0;

	ret = alloc_chrdev_region(&dfrc_devno, 0, 1, DFRC_DEVNAME);
	if (ret)
		DFRC_ERR("Can't Get Major number for FPS policy Device\n");
	else
		DFRC_DBG("Get FPS policy Device Major number (%d)\n", dfrc_devno);
	dfrc_cdev = cdev_alloc();
	dfrc_cdev->owner = THIS_MODULE;
	dfrc_cdev->ops = &dfrc_fops;
	ret = cdev_add(dfrc_cdev, dfrc_devno, 1);
	dfrc_class = class_create(THIS_MODULE, DFRC_DEVNAME);
	class_dev = (struct class_device *)device_create(dfrc_class, NULL,
			dfrc_devno, NULL, DFRC_DEVNAME);

	dfrc_debug_init();

	g_init_done = 1;
	g_fps_info.support_120 = 0;
	g_fps_info.support_90 = 0;
	g_fps_info.num = 2;
	g_fps_info.range = vmalloc(sizeof(struct DFRC_DRV_REFRESH_RANGE) * g_fps_info.num);
	g_fps_info.range[0].min_fps = 30;
	g_fps_info.range[0].max_fps = 60;
	g_fps_info.range[1].min_fps = 15;
	g_fps_info.range[1].max_fps = 30;

	dfrc_init_kernel_policy();

	return ret;
}

static int dfrc_remove(struct platform_device *pdev)
{
	DFRC_DBG("start RRC FPS driver remove\n");
	device_destroy(dfrc_class, dfrc_devno);
	class_destroy(dfrc_class);
	cdev_del(dfrc_cdev);
	unregister_chrdev_region(dfrc_devno, 1);
	DFRC_DBG("done RRC FPS driver remove\n");

	return 0;
}

static void dfrc_shutdown(struct platform_device *pdev)
{
}

static int dfrc_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	return 0;
}

static int dfrc_resume(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver dfrc_driver = {
	.probe          = dfrc_probe,
	.remove         = dfrc_remove,
	.shutdown       = dfrc_shutdown,
	.suspend        = dfrc_suspend,
	.resume         = dfrc_resume,
	.driver = {
		.name = DFRC_DEVNAME,
	},
};

static void dfrc_device_release(struct device *dev)
{
}

static u64 dfrc_dmamask = ~(u32)0;

static struct platform_device dfrc_device = {
	.name    = DFRC_DEVNAME,
	.id     = 0,
	.dev    = {
		.release = dfrc_device_release,
		.dma_mask = &dfrc_dmamask,
		.coherent_dma_mask = 0xffffffff,
	},
	.num_resources = 0,
};

static int __init dfrc_init(void)
{
	int res = 0;

	DFRC_INFO("start to initialize fps policy\n");

	DFRC_INFO("register fps policy device\n");
	if (platform_device_register(&dfrc_device)) {
		DFRC_ERR("failed to register fps policy device\n");
		res = -ENODEV;
		return res;
	}

	DFRC_INFO("register fps policy driver\n");
	if (platform_driver_register(&dfrc_driver)) {
		DFRC_ERR("failed to register fps policy driver\n");
		res = -ENODEV;
		return res;
	}

	res = dfrc_init_param();

	return res;
}

static void __exit dfrc_exit(void)
{
	platform_driver_unregister(&dfrc_driver);
	platform_device_unregister(&dfrc_device);
}

module_init(dfrc_init);
module_exit(dfrc_exit);
MODULE_AUTHOR("Jih-Cheng, Chiu <Jih-Cheng.Chiu@mediatek.com>");
MODULE_DESCRIPTION("FPS policy driver");
MODULE_LICENSE("GPL");
