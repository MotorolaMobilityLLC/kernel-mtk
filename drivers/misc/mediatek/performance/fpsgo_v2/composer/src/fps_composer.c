/*
 * Copyright (C) 2017 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/slab.h>
#include <linux/device.h>
#include <linux/seq_file.h>
#include <linux/security.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/mutex.h>

#include <fpsgo_common.h>

#include "fpsgo_base.h"
#include "fpsgo_common.h"
#include "fpsgo_usedext.h"
#include "fps_composer.h"
#include "fbt_cpu.h"
#include "fstb.h"
#include "xgf.h"

/*#define FPSGO_COM_DEBUG*/

#ifdef FPSGO_COM_DEBUG
#define FPSGO_COM_TRACE(...)	xgf_trace("fpsgo_com:" __VA_ARGS__)
#else
#define FPSGO_COM_TRACE(...)
#endif

static struct queue_info queue_info_head;
static struct connect_api_info connect_api_info_head;

static DEFINE_MUTEX(fpsgo_com_queue_list_lock);
static DEFINE_MUTEX(fpsgo_com_connect_api_list_lock);

static inline void fpsgo_com_lock(const char *tag)
{
	mutex_lock(&fpsgo_com_queue_list_lock);
}

static inline void fpsgo_com_unlock(const char *tag)
{
	mutex_unlock(&fpsgo_com_queue_list_lock);
}

static inline void fpsgo_com_connect_api_lock(const char *tag)
{
	mutex_lock(&fpsgo_com_connect_api_list_lock);
}

static inline void fpsgo_com_connect_api_unlock(const char *tag)
{
	mutex_unlock(&fpsgo_com_connect_api_list_lock);
}

static inline int fpsgo_com_get_tgid(int pid)
{
	struct task_struct *tsk;
	int tgid = 0;

	rcu_read_lock();
	tsk = find_task_by_vpid(pid);
	if (tsk)
		get_task_struct(tsk);
	rcu_read_unlock();

	if (!tsk)
		return 0;

	tgid = tsk->tgid;
	put_task_struct(tsk);

	return tgid;
}

static inline int fpsgo_com_check_is_surfaceflinger(int pid)
{
	struct task_struct *tsk;
	int is_surfaceflinger = 0;

	FPSGO_COM_TRACE("%s pid[%d]", __func__, pid);

	rcu_read_lock();
	tsk = find_task_by_vpid(pid);

	if (tsk)
		get_task_struct(tsk);
	rcu_read_unlock();

	if (!tsk)
		return 0;

	if (strstr(tsk->comm, "surfaceflinger"))
		is_surfaceflinger = 1;
	put_task_struct(tsk);

	return is_surfaceflinger;
}

void fpsgo_com_check_frame_info(void)
{
	struct queue_info *tmp, *tmp2;
	struct task_struct *tsk;

	fpsgo_com_lock(__func__);

	rcu_read_lock();
	list_for_each_entry_safe(tmp, tmp2, &queue_info_head.list, list) {
		tsk = find_task_by_vpid(tmp->pid);
		if (!tsk) {
			list_del(&(tmp->list));
			kfree(tmp);
		}
	}
	rcu_read_unlock();

	fpsgo_com_unlock(__func__);
}

int fpsgo_com_get_connect_api(int tgid, unsigned long long bufferID)
{
	struct connect_api_info *tmp, *tmp2;
	int find = 0;
	int api = 0;

	fpsgo_com_connect_api_lock(__func__);

	list_for_each_entry_safe(tmp, tmp2, &connect_api_info_head.list, list) {
		if (!find && (tgid == tmp->tgid) && (bufferID == tmp->bufferID)) {
			api = tmp->api;
			find = 1;
			break;
		}
	}
	fpsgo_com_connect_api_unlock(__func__);

	if (!find) {
		FPSGO_COM_TRACE("%s: NON pair connect api info tgid %d bufferID %llu!!!\n",
			__func__, tgid, bufferID);
	}

	return api;
}

int fpsgo_com_check_frame_type_is_by_pass(int api, int type)
{
	int new_type = 0;

	switch (api) {
	case NATIVE_WINDOW_API_MEDIA:
	case NATIVE_WINDOW_API_CAMERA:
		new_type = BY_PASS_TYPE;
		return new_type;
	default:
		break;
	}
	return type;
}

void fpsgo_fbt2comp_destroy_frame_info(int pid)
{
	struct queue_info *tmp, *tmp2;
	int find = 0;

	fpsgo_com_lock(__func__);

	list_for_each_entry_safe(tmp, tmp2, &queue_info_head.list, list) {
		if (pid == tmp->pid) {
			FPSGO_COM_TRACE("distory pid[%d] type[%d]",
				pid, tmp->frame_type);
			list_del(&(tmp->list));
			kfree(tmp);
			find = 1;
			break;
		}
	}

	fpsgo_com_unlock(__func__);

	if (!find) {
		FPSGO_COM_TRACE("%s:FPSGo composer distory frame info fail : %d !!!!\n",
			__func__, pid);
		return;
	}
}

int fpsgo_com_store_frame_info(int pid,
	unsigned long long enqueue_start_time, unsigned long long bufferID)
{
	struct queue_info *frame_info;
	int tgid, api;
	int type = NON_VSYNC_ALIGNED_TYPE;

	FPSGO_COM_TRACE("%s pid[%d]", __func__, pid);

	tgid = fpsgo_com_get_tgid(pid);
	api = fpsgo_com_get_connect_api(tgid, bufferID);
	if (api)
		type = fpsgo_com_check_frame_type_is_by_pass(api, type);


	frame_info = kzalloc(sizeof(struct queue_info), GFP_KERNEL);
	if (!frame_info)
		return -ENOMEM;


	fpsgo_com_lock(__func__);

	memset(frame_info, 0, sizeof(struct queue_info));
	INIT_LIST_HEAD(&(frame_info->list));

	frame_info->pid = pid;
	frame_info->tgid = tgid;
	frame_info->frame_type = type;
	frame_info->api = api;
	frame_info->bufferID = bufferID;
	if (type == NON_VSYNC_ALIGNED_TYPE)
		frame_info->t_enqueue_start = enqueue_start_time;
	list_add(&(frame_info->list), &(queue_info_head.list));

	fpsgo_com_unlock(__func__);

	FPSGO_COM_TRACE("store pid[%d] type[%d] enqueue_s:%llu api:%d bufferID:%llu",
			pid, type, enqueue_start_time, api, bufferID);

	return 0;
}

int fpsgo_com_store_vysnc_aligned_frame_info(int pid,
	int ui_pid, unsigned long long frame_time, int render, int render_method)
{
	struct queue_info *frame_info;
	int tgid;

	tgid = fpsgo_com_get_tgid(pid);

	frame_info = kzalloc(sizeof(struct queue_info), GFP_KERNEL);
	if (!frame_info)
		return -ENOMEM;

	fpsgo_com_lock(__func__);

	memset(frame_info, 0, sizeof(struct queue_info));
	INIT_LIST_HEAD(&(frame_info->list));

	frame_info->pid = pid;
	frame_info->ui_pid = ui_pid;
	frame_info->tgid = tgid;
	frame_info->frame_type = VSYNC_ALIGNED_TYPE;
	frame_info->frame_time = frame_time;
	frame_info->render = render;
	frame_info->render_method = render_method;
	list_add(&(frame_info->list), &(queue_info_head.list));
	fpsgo_com_unlock(__func__);
	FPSGO_COM_TRACE("store pid[%d] ui_pid[%d] type[%d] Q2Q_time:%llu",
			pid, ui_pid, frame_info->frame_type, frame_time);

	return 0;
}

void fpsgo_ctrl2comp_enqueue_start(int pid,
	unsigned long long enqueue_start_time, unsigned long long bufferID)
{
	struct queue_info *tmp, *tmp2;
	int find = 0;

	FPSGO_COM_TRACE("%s pid[%d] bufferID %llu", __func__, pid, bufferID);

	if (fpsgo_com_check_is_surfaceflinger(pid))
		return;

	fpsgo_com_lock(__func__);

	list_for_each_entry_safe(tmp, tmp2, &queue_info_head.list, list) {
		if (!find && (pid == tmp->pid)) {
			switch (tmp->frame_type) {
			case VSYNC_ALIGNED_TYPE:
				tmp->t_enqueue_start = enqueue_start_time;
				find = 1;
				if (!tmp->bufferID)
					tmp->bufferID = bufferID;
				FPSGO_COM_TRACE("pid[%d] type[%d] enqueue_s:%llu frame_time:%llu bufferID:%llu",
					pid, tmp->frame_type, enqueue_start_time, tmp->frame_time, bufferID);
				fpsgo_comp2fbt_enq_start(pid, enqueue_start_time);
				break;
			case NON_VSYNC_ALIGNED_TYPE:
				if (tmp->t_enqueue_start) {
					tmp->frame_time =
						enqueue_start_time - tmp->t_enqueue_end - tmp->dequeue_length;
					tmp->Q2Q_time = enqueue_start_time - tmp->t_enqueue_start;
				}
				tmp->t_enqueue_start = enqueue_start_time;
				find = 1;
				FPSGO_COM_TRACE("pid[%d] type[%d] enqueue_s:%llu frame_time:%llu",
					pid, tmp->frame_type, enqueue_start_time, tmp->frame_time);
				FPSGO_COM_TRACE("update pid[%d] tgid[%d] bufferID:%llu api:%d",
					tmp->pid, tmp->tgid, tmp->bufferID, tmp->api);
				fpsgo_systrace_c_fbt_gm(-300, tmp->frame_time,
					"%d_%d-frame_time", pid, tmp->frame_type);
				fpsgo_systrace_c_fbt_gm(-300, tmp->Q2Q_time,
					"%d_%d-Q2Q_time", pid, tmp->frame_type);
				fpsgo_comp2fbt_enq_start(pid, enqueue_start_time);
				fpsgo_comp2fbt_frame_start(pid, tmp->Q2Q_time,
					tmp->frame_time, NON_VSYNC_ALIGNED_TYPE, enqueue_start_time);
				fpsgo_comp2fstb_queue_time_update(pid, tmp->frame_type,
					tmp->render_method, enqueue_start_time);
				fpsgo_comp2xgf_qudeq_notify(pid, XGF_QUEUE_START);
				break;
			case BY_PASS_TYPE:
				find = 1;
				fpsgo_systrace_c_fbt_gm(-100, 0, "%d-frame_time", pid);
				break;
			default:
				FPSGO_COM_TRACE("type not found pid[%d] type[%d]",
				pid, tmp->frame_type);
				break;
			}
			break;
		}
	}
	fpsgo_com_unlock(__func__);

	if (!find) {
		int ret;

		ret = fpsgo_com_store_frame_info(pid, enqueue_start_time, bufferID);
		return;
	}

	return;

}

void fpsgo_ctrl2comp_enqueue_end(int pid,
	unsigned long long enqueue_end_time, unsigned long long bufferID)
{
	struct queue_info *tmp, *tmp2;
	int find = 0;

	FPSGO_COM_TRACE("%s pid[%d]", __func__, pid);

	if (fpsgo_com_check_is_surfaceflinger(pid))
		return;

	fpsgo_com_lock(__func__);

	list_for_each_entry_safe(tmp, tmp2, &queue_info_head.list, list) {
		if (!find && (pid == tmp->pid)) {
			switch (tmp->frame_type) {
			case VSYNC_ALIGNED_TYPE:
			case NON_VSYNC_ALIGNED_TYPE:
				tmp->t_enqueue_end = enqueue_end_time;
				tmp->enqueue_length =
					enqueue_end_time - tmp->t_enqueue_start;
				find = 1;
				FPSGO_COM_TRACE("pid[%d] type[%d] enqueue_e:%llu enqueue_l:%llu",
					pid, tmp->frame_type, enqueue_end_time, tmp->enqueue_length);
				fpsgo_comp2fbt_enq_end(pid, enqueue_end_time);
				if (tmp->frame_type == NON_VSYNC_ALIGNED_TYPE) {
					fpsgo_comp2xgf_qudeq_notify(pid, XGF_QUEUE_END);
					fpsgo_systrace_c_fbt_gm(-300, tmp->enqueue_length,
					"%d_%d-enqueue_length", pid, tmp->frame_type);
				}
				break;
			case BY_PASS_TYPE:
				find = 1;
				break;
			default:
				FPSGO_COM_TRACE("type not found pid[%d] type[%d]",
				pid, tmp->frame_type);
				break;
			}
			break;
		}
	}
	fpsgo_com_unlock(__func__);

	if (!find) {
		FPSGO_COM_TRACE("%s: NON pair frame info : %d !!!!\n",
			__func__, pid);
		return;
	}

	return;

}

void fpsgo_ctrl2comp_dequeue_start(int pid,
	unsigned long long dequeue_start_time, unsigned long long bufferID)
{
	struct queue_info *tmp, *tmp2;
	int find = 0;

	if (fpsgo_com_check_is_surfaceflinger(pid))
		return;

	FPSGO_COM_TRACE("%s pid[%d]", __func__, pid);

	fpsgo_com_lock(__func__);
	list_for_each_entry_safe(tmp, tmp2, &queue_info_head.list, list) {
		if (!find && (pid == tmp->pid)) {
			switch (tmp->frame_type) {
			case VSYNC_ALIGNED_TYPE:
			case NON_VSYNC_ALIGNED_TYPE:
				tmp->t_dequeue_start = dequeue_start_time;
				find = 1;
				FPSGO_COM_TRACE("pid[%d] type[%d] dequeue_s:%llu",
					pid, tmp->frame_type, dequeue_start_time);
				fpsgo_comp2fbt_deq_start(pid, dequeue_start_time);
				if (tmp->frame_type == NON_VSYNC_ALIGNED_TYPE)
					fpsgo_comp2xgf_qudeq_notify(pid, XGF_DEQUEUE_START);
				break;
			case BY_PASS_TYPE:
				find = 1;
				break;
			default:
				FPSGO_COM_TRACE("type not found pid[%d] type[%d]",
				pid, tmp->frame_type);
				break;
			}
			break;
		}
	}
	fpsgo_com_unlock(__func__);

	if (!find) {
		FPSGO_COM_TRACE("%s: NON pair frame info : %d !!!!\n",
			__func__, pid);
		return;
	}
}

void fpsgo_ctrl2comp_dequeue_end(int pid,
	unsigned long long dequeue_end_time, unsigned long long bufferID)
{
	struct queue_info *tmp, *tmp2;
	int find = 0;

	FPSGO_COM_TRACE("%s pid[%d]", __func__, pid);

	if (fpsgo_com_check_is_surfaceflinger(pid))
		return;

	fpsgo_com_lock(__func__);

	list_for_each_entry_safe(tmp, tmp2, &queue_info_head.list, list) {
		if (!find && (pid == tmp->pid)) {
			switch (tmp->frame_type) {
			case VSYNC_ALIGNED_TYPE:
			case NON_VSYNC_ALIGNED_TYPE:
				tmp->t_dequeue_end = dequeue_end_time;
				tmp->dequeue_length =
					dequeue_end_time - tmp->t_dequeue_start;
				find = 1;
				FPSGO_COM_TRACE("pid[%d] type[%d] dequeue_e:%llu dequeue_l:%llu",
					pid, tmp->frame_type, dequeue_end_time, tmp->dequeue_length);
				fpsgo_comp2fbt_deq_end(pid, dequeue_end_time, tmp->dequeue_length);
				if (tmp->frame_type == NON_VSYNC_ALIGNED_TYPE) {
					fpsgo_comp2xgf_qudeq_notify(pid, XGF_QUEUE_END);
					fpsgo_systrace_c_fbt_gm(-300, tmp->dequeue_length,
					"%d_%d-dequeue_length", pid, tmp->frame_type);
				}
				break;
			case BY_PASS_TYPE:
				find = 1;
				break;
			default:
				FPSGO_COM_TRACE("type not found pid[%d] type[%d]",
				pid, tmp->frame_type);
				break;
			}
			break;
		}
	}

	fpsgo_com_unlock(__func__);

	if (!find) {
		FPSGO_COM_TRACE("%s: NON pair frame info : %d !!!!\n",
			__func__, pid);
		return;
	}
}

void fpsgo_ctrl2comp_vysnc_aligned_frame_start(int pid,
	unsigned long long t_frame_start)
{
	struct queue_info *tmp, *tmp2;
	int find = 0;
	int api = 0;
	int new_type = VSYNC_ALIGNED_TYPE;

	FPSGO_COM_TRACE("%s pid[%d]", __func__, pid);

	fpsgo_com_lock(__func__);

	list_for_each_entry_safe(tmp, tmp2, &queue_info_head.list, list) {
		if (!find && (pid == tmp->ui_pid) && (tmp->frame_type != BY_PASS_TYPE)) {
			if (!tmp->api && tmp->bufferID) {
				api = fpsgo_com_get_connect_api(tmp->tgid, tmp->bufferID);
				if (api)
					new_type = fpsgo_com_check_frame_type_is_by_pass(api, new_type);
			}
			tmp->frame_type = new_type;
			find = 1;
			FPSGO_COM_TRACE("vysnc_aligned_frame_start pid[%d] ui_pid[%d] type[%d]",
					tmp->pid, pid, tmp->frame_type);
			if (tmp->render) {
				fpsgo_systrace_c_fbt_gm(-300, tmp->frame_time,
				"%d_%d-frame_time", tmp->pid, tmp->frame_type);
				fpsgo_comp2fbt_frame_start(tmp->pid, tmp->frame_time,
				tmp->frame_time, VSYNC_ALIGNED_TYPE, t_frame_start);
			}
			break;
		}
	}

	fpsgo_com_unlock(__func__);

	if (!find) {
		FPSGO_COM_TRACE("%s: NON pair frame info : %d !!!!\n",
			__func__, pid);
		return;
	}

	return;

}

void fpsgo_ctrl2comp_vysnc_aligned_frame_done(int pid,
	int ui_pid, unsigned long long frame_time, int render,
	unsigned long long t_frame_done, int render_method)
{
	struct queue_info *tmp, *tmp2;
	int find = 0;
	int api = 0;
	int new_type = VSYNC_ALIGNED_TYPE;

	FPSGO_COM_TRACE("%s pid[%d] ui_pid[%d]", __func__, pid, ui_pid);

	fpsgo_com_lock(__func__);

	list_for_each_entry_safe(tmp, tmp2, &queue_info_head.list, list) {
		if (!find && (pid == tmp->pid) && (tmp->frame_type != BY_PASS_TYPE)) {
			tmp->ui_pid = ui_pid;
			tmp->render = render;
			if (!tmp->api && tmp->bufferID) {
				api = fpsgo_com_get_connect_api(tmp->tgid, tmp->bufferID);
				if (api)
					new_type = fpsgo_com_check_frame_type_is_by_pass(api, new_type);
			}
			if (render) {
				tmp->frame_time = frame_time;
				tmp->render_method = render_method;
				fpsgo_comp2fstb_queue_time_update(pid, tmp->frame_type,
					tmp->render_method, t_frame_done);
			}
			tmp->frame_type = new_type;
			fpsgo_systrace_c_fbt_gm(-300, render, "%d_%d-render", pid, tmp->frame_type);
			find = 1;
			fpsgo_comp2fbt_frame_complete(pid, VSYNC_ALIGNED_TYPE, render, t_frame_done);
			FPSGO_COM_TRACE("frame_done pid[%d] ui[%d] type[%d] frame_t:%llu render:%d method:%d",
				tmp->pid, pid, tmp->frame_type, frame_time, render, render_method);
			break;
		}
	}
	fpsgo_com_unlock(__func__);

	if (!find && render) {
		int ret;

		ret =
			fpsgo_com_store_vysnc_aligned_frame_info(pid, ui_pid,
			frame_time, render, render_method);
		return;

	}

	return;

}

int fpsgo_com_store_connect_api_info(int pid, int tgid, u64 bufferID, int api)
{
	struct connect_api_info *connect_api;

	connect_api = kzalloc(sizeof(struct connect_api_info), GFP_KERNEL);

	if (!connect_api)
		return -ENOMEM;

	fpsgo_com_connect_api_lock(__func__);

	memset(connect_api, 0, sizeof(struct connect_api_info));
	INIT_LIST_HEAD(&(connect_api->list));

	connect_api->pid = pid;
	connect_api->tgid = tgid;
	connect_api->bufferID = bufferID;
	connect_api->api = api;
	list_add(&(connect_api->list), &(connect_api_info_head.list));

	fpsgo_com_connect_api_unlock(__func__);

	FPSGO_COM_TRACE("store connect_api_info pid[%d] bufferID[%llu]",
		pid, bufferID);

	return 0;
}

void fpsgo_ctrl2comp_connect_api(int pid, unsigned long long bufferID, int api)
{
	struct connect_api_info *tmp, *tmp2;
	int find = 0;
	int tgid, type;

	if (fpsgo_com_check_is_surfaceflinger(pid))
		return;

	tgid = fpsgo_com_get_tgid(pid);
	type = fpsgo_com_check_frame_type_is_by_pass(api, NON_VSYNC_ALIGNED_TYPE);

	FPSGO_COM_TRACE("%s pid[%d]", __func__, pid);

	fpsgo_com_connect_api_lock(__func__);

	list_for_each_entry_safe(tmp, tmp2, &connect_api_info_head.list, list) {
		if (!find && (pid == tmp->pid)) {
			find = 1;
			break;
		}
	}

	fpsgo_com_connect_api_unlock(__func__);

	if (!find) {
		int ret;

		ret = fpsgo_com_store_connect_api_info(pid, tgid, bufferID, api);
		if (ret) {
			FPSGO_COM_TRACE("%s: store frame info fail : %d !!!!\n",
			__func__, pid);
			return;
		}
		if (type == BY_PASS_TYPE)
			fpsgo_comp2fbt_bypass_connect(pid);
	}
}

void fpsgo_ctrl2comp_disconnect_api(int pid, unsigned long long bufferID, int api)
{
	struct connect_api_info *tmp, *tmp2;
	struct queue_info *pos, *next;
	int find_api = 0;
	int find_frame = 0;
	int tgid, type;

	if (fpsgo_com_check_is_surfaceflinger(pid))
		return;

	FPSGO_COM_TRACE("%s pid[%d]", __func__, pid);

	tgid = fpsgo_com_get_tgid(pid);

	fpsgo_com_connect_api_lock(__func__);

	list_for_each_entry_safe(tmp, tmp2, &connect_api_info_head.list, list) {
		if (pid == tmp->pid) {
			type = fpsgo_com_check_frame_type_is_by_pass(tmp->api, NON_VSYNC_ALIGNED_TYPE);
			FPSGO_COM_TRACE("delete connect api pid[%d] api[%d] type[%d]", pid, tmp->api, type);
			list_del(&(tmp->list));
			kfree(tmp);
			find_api = 1;
			break;
		}
	}

	fpsgo_com_connect_api_unlock(__func__);

	if (!find_api) {
		FPSGO_COM_TRACE("%s: FPSGo composer distory connect api fail : %d !!!!\n",
			__func__, pid);
		return;
	}

	if (find_api && type == BY_PASS_TYPE)
		fpsgo_comp2fbt_bypass_disconnect(pid);

	fpsgo_com_lock(__func__);
	list_for_each_entry_safe(pos, next, &queue_info_head.list, list) {
		if (tgid == pos->tgid && bufferID == pos->bufferID) {
			FPSGO_COM_TRACE("distory frame info pid[%d] type[%d]",
				pid, pos->frame_type);
			list_del(&(pos->list));
			kfree(pos);
			find_frame = 1;
		}
	}
	fpsgo_com_unlock(__func__);

	if (!find_frame) {
		FPSGO_COM_TRACE("%s: FPSGo composer no mapping frame info : %d !!!!\n",
			__func__, pid);
		return;
	}
}

void fpsgo_ctrl2comp_resent_by_pass_info(void)
{
	struct connect_api_info *tmp, *tmp2;

	fpsgo_com_connect_api_lock(__func__);

	list_for_each_entry_safe(tmp, tmp2, &connect_api_info_head.list, list) {
		switch (tmp->api) {
		case NATIVE_WINDOW_API_MEDIA:
		case NATIVE_WINDOW_API_CAMERA:
			FPSGO_COM_TRACE("resent bypass info pid[%d] api[%d]",
				tmp->pid, tmp->api);
			fpsgo_comp2fbt_bypass_connect(tmp->pid);
			break;
		default:
			FPSGO_COM_TRACE("non bypass info pid[%d] api[%d]",
				tmp->pid, tmp->api);
			break;
		}
	}

	fpsgo_com_connect_api_unlock(__func__);
}

#define FPSGO_COM_DEBUGFS_ENTRY(name) \
static int fspgo_com_##name##_open(struct inode *i, struct file *file) \
{ \
	return single_open(file, fspgo_com_##name##_show, i->i_private); \
} \
\
static const struct file_operations fspgo_com_##name##_fops = { \
	.owner = THIS_MODULE, \
	.open = fspgo_com_##name##_open, \
	.read = seq_read, \
	.write = fspgo_com_##name##_write, \
	.llseek = seq_lseek, \
	.release = single_release, \
}

static int fspgo_com_frame_info_show(struct seq_file *m, void *unused)
{
	struct queue_info *tmp;
	struct task_struct *tsk;

	fpsgo_com_check_frame_info();
	seq_puts(m, "\n  PID  NAME  TGID  TYPE  API  BufferID");
	seq_puts(m, "    FRAME_L    ENQ_L    ENQ_S    ENQ_E");
	seq_puts(m, "    DEQ_L     DEQ_S    DEQ_E\n");
	fpsgo_com_lock(__func__);
	rcu_read_lock();
	list_for_each_entry(tmp, &queue_info_head.list, list) {
		tsk = find_task_by_vpid(tmp->tgid);
		if (tsk) {
			seq_printf(m, "%5d %4s %4d %4d %4d %4llu",
				tmp->pid, tsk->comm, tmp->tgid, tmp->frame_type,
				tmp->api, tmp->bufferID);
			seq_printf(m, "  %4llu %4llu %4llu %4llu %4llu %4llu %4llu\n",
				tmp->frame_time, tmp->enqueue_length,
				tmp->t_enqueue_start, tmp->t_enqueue_end,
				tmp->dequeue_length, tmp->t_dequeue_start,
				tmp->t_dequeue_end);
		}
	}

	rcu_read_unlock();
	fpsgo_com_unlock(__func__);

	return 0;
}

static ssize_t fspgo_com_frame_info_write(struct file *flip,
			const char *ubuf, size_t cnt, loff_t *data)
{
	int val;
	int ret;

	ret = kstrtoint_from_user(ubuf, cnt, 0, &val);
	if (ret)
		return ret;

	return cnt;
}

FPSGO_COM_DEBUGFS_ENTRY(frame_info);

static int fspgo_com_connect_api_info_show
	(struct seq_file *m, void *unused)
{
	struct connect_api_info *tmp, *tmp2;
	struct task_struct *tsk;

	fpsgo_com_connect_api_lock(__func__);
	seq_puts(m, "\n  PID  TGID   NAME    BufferID    API\n");

	rcu_read_lock();
	list_for_each_entry_safe(tmp, tmp2, &connect_api_info_head.list, list) {
		tsk = find_task_by_vpid(tmp->tgid);
		if (!tsk) {
			list_del(&(tmp->list));
			kfree(tmp);
		} else
			seq_printf(m, "%5d %5d %5s %4llu %5d\n",
			tmp->pid, tmp->tgid, tsk->comm, tmp->bufferID, tmp->api);
	}
	rcu_read_unlock();

	fpsgo_com_connect_api_unlock(__func__);

	return 0;

}

static ssize_t fspgo_com_connect_api_info_write(struct file *flip,
			const char *ubuf, size_t cnt, loff_t *data)
{
	int val;
	int ret;

	ret = kstrtoint_from_user(ubuf, cnt, 0, &val);
	if (ret)
		return ret;

	return cnt;
}

FPSGO_COM_DEBUGFS_ENTRY(connect_api_info);

void fpsgo_composer_exit(void)
{

}

int __init fpsgo_composer_init(void)
{
	struct proc_dir_entry *pe;

	INIT_LIST_HEAD(&(queue_info_head.list));
	INIT_LIST_HEAD(&(connect_api_info_head.list));

	if (!proc_mkdir("fpsgo_com", NULL))
		return -1;

	pe = proc_create("fpsgo_com/frame_info",
			0660, NULL, &fspgo_com_frame_info_fops);
	if (!pe)
		return -ENOMEM;

	pe = proc_create("fpsgo_com/connect_api_info",
			0660, NULL, &fspgo_com_connect_api_info_fops);
	if (!pe)
		return -ENOMEM;

	return 0;
}

