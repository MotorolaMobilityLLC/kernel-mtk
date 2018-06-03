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
#ifndef __FPS_COMPOSER_H__
#define __FPS_COMPOSER_H__

struct queue_info {
	struct list_head list;
	int pid;
	int ui_pid;
	unsigned long long bufferID;
	int tgid;
	int api;
	int frame_type;
	int render_method;
	int render;
	unsigned long long t_enqueue_start;
	unsigned long long t_enqueue_end;
	unsigned long long t_dequeue_start;
	unsigned long long t_dequeue_end;
	unsigned long long enqueue_length;
	unsigned long long dequeue_length;
	unsigned long long frame_time;
	unsigned long long Q2Q_time;
};

struct connect_api_info {
	struct list_head list;
	int pid;
	int tgid;
	unsigned long long bufferID;
	int api;
};

int fpsgo_composer_init(void);
void fpsgo_composer_exit(void);

void fpsgo_ctrl2comp_vysnc_aligned_frame_done
	(int pid, int ui_pid, unsigned long long frame_time,
	int render, unsigned long long t_frame_done, int render_method);
void fpsgo_ctrl2comp_vysnc_aligned_frame_start(int pid, unsigned long long t_frame_start);
void fpsgo_ctrl2comp_dequeue_end(int pid, unsigned long long dequeue_end_time, unsigned long long bufferID);
void fpsgo_ctrl2comp_dequeue_start(int pid, unsigned long long dequeue_start_time, unsigned long long bufferID);
void fpsgo_ctrl2comp_enqueue_end(int pid, unsigned long long enqueue_end_time, unsigned long long bufferID);
void fpsgo_ctrl2comp_enqueue_start(int pid, unsigned long long enqueue_start_time, unsigned long long bufferID);
void fpsgo_fbt2comp_destroy_frame_info(int pid);
void fpsgo_ctrl2comp_connect_api(int pid, unsigned long long bufferID, int api);
void fpsgo_ctrl2comp_disconnect_api(int pid, unsigned long long bufferID, int api);
void fpsgo_ctrl2comp_resent_by_pass_info(void);

#endif

