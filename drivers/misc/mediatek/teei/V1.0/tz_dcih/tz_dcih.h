/*
 * Copyright (c) 2015-2016 MICROTRUST Incorporated
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _TZ_DCIH_H_
#define _TZ_DCIH_H_

#include <linux/completion.h>

#define TOTAL_DRM_DRIVER_NUM (8)
enum drm_dcih_buf_mode {
	DRM_DCIH_BUF_MODE_FORWARD = 0x0,
	DRM_DCIH_BUF_MODE_BACKWARD = 0x1,

	DRM_DCIH_BUF_MODE_INVALID = 0xFF
};
struct drm_dcih_info {
	unsigned char buf_mode;		/* buf mode, 0 for forward, 1 for backward */
	unsigned char is_inited;	/* DCIH driver inited or not */
	unsigned char is_shared;	/* DCIH driver shared with secure driver or not */
	unsigned int dcih_id;		/* DCIH driver id */
	unsigned int buf_size;		/* DCIH driver allocated buf size */
	unsigned long virt_addr;	/* DCIH driver allocated virtual buf addr */
	unsigned long phy_addr;		/* DCIH driver allocated phy buf addr */
	struct completion tee_signal;	/* Wait for the signal from TEE driver (DCIH of TEE->REE) */
	struct completion ree_signal;	/* Notify to REE driver (DCIH of TEE->REE) */
};

extern unsigned long message_buff;
extern struct semaphore fdrv_sema;

void init_dcih_service(void);
unsigned long tz_create_share_buffer(unsigned int driver_id, unsigned int buff_size, enum drm_dcih_buf_mode mode);
int tz_sec_drv_notification(unsigned int driver_id);
int __send_drm_command(unsigned long share_memory_size);

#endif
