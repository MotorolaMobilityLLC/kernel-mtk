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

#ifndef _FLASHLIGHT_CORE_H
#define _FLASHLIGHT_CORE_H

#include <linux/list.h>
#include "flashlight.h"




#define PT_NOTIFY_STRICT   3



/* sysfs - capability */
#define FLASHLIGHT_CAPABILITY_TMPBUF_SIZE 64
#define FLASHLIGHT_CAPABILITY_BUF_SIZE \
	(FLASHLIGHT_TYPE_MAX * FLASHLIGHT_CT_MAX * FLASHLIGHT_PART_MAX * \
	 FLASHLIGHT_CAPABILITY_TMPBUF_SIZE + 1)

/* sysfs - current */
#define FLASHLIGHT_CURRENT_NUM    3
#define FLASHLIGHT_CURRENT_TYPE   0
#define FLASHLIGHT_CURRENT_CT     1
#define FLASHLIGHT_CURRENT_PART   2
#define FLASHLIGHT_DUTY_CURRENT_TMPBUF_SIZE 6
#define FLASHLIGHT_DUTY_CURRENT_BUF_SIZE \
	(FLASHLIGHT_MAX_DUTY_NUM * FLASHLIGHT_DUTY_CURRENT_TMPBUF_SIZE + 1)





/* flashlight devices */


extern const int flashlight_device_num;

struct flashlight_dev {
	struct list_head node;
	struct flashlight_operations *ops;
	struct flashlight_device_id dev_id;
	/* device status */
	int enable;
	int level;
	int low_pt_level;
	int charger_status;
};

/* device arguments */
struct flashlight_dev_arg {
	int arg;
};




int flashlight_dev_register_by_device_id(
		struct flashlight_device_id *dev_id,
		struct flashlight_operations *dev_ops);
int flashlight_dev_unregister_by_device_id(struct flashlight_device_id *dev_id);




#endif /* _FLASHLIGHT_CORE_H */

