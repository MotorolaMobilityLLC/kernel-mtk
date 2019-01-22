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

#ifndef __ENV_H__
#define __ENV_H__
#include <linux/ioctl.h>

#define CFG_ENV_SIZE		0x4000     /*16KB*/
#define CFG_ENV_OFFSET		0x20000    /*128KB*/
#define ENV_PART		"PARA"

struct env_struct {
	char sig_head[8];
	char *env_data;
	char sig_tail[8];
	int checksum;
};

#define ENV_MAGIC		'e'
#define ENV_READ		_IOW(ENV_MAGIC, 1, int)
#define ENV_WRITE		_IOW(ENV_MAGIC, 2, int)
#define ENV_SET_PID		_IOW(ENV_MAGIC, 3, int)
#define ENV_USER_INIT	_IOW(ENV_MAGIC, 4, int)

struct env_ioctl {
	char *name;
	int name_len;
	char *value;
	int value_len;
};
extern int set_env(char *name, char *value);
extern char *get_env(const char *name);

#endif
