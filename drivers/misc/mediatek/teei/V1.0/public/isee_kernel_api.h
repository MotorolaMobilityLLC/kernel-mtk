/*
 * Copyright (c) 2015-2016 MICROTRUST Incorporated
 * All rights reserved
 *
 * This file and software is confidential and proprietary to MICROTRUST Inc.
 * Unauthorized copying of this file and software is strictly prohibited.
 * You MUST NOT disclose this file and software unless you get a license
 * agreement from MICROTRUST Incorporated.
 */

#ifndef _ISEE_KERNEL_API_H_

#define DRM_M4U_DRV_DRIVER_ID   (0x77aa)
#define DRM_WVL1_MODULAR_DRV_DRIVER_ID  (0x210)

int is_teei_ready(void);
unsigned long tz_get_share_buffer(unsigned int driver_id);
int tz_wait_for_notification(unsigned int driver_id);
int tz_notify_driver(unsigned int driver_id);

#endif //_ISEE_KERNEL_API_H_
