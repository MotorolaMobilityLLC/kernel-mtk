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


#ifndef _SMI_NAME_H_
#define _SMI_NAME_H_
#include "met_drv.h"

struct smi_desc {
	unsigned long port;
	enum SMI_DEST desttype;
	enum SMI_RW rwtype;
	enum SMI_BUS bustype;
};

struct chip_smi {
	int master;
	struct smi_desc *desc;
	unsigned int count;
};

#if 0
/* define smi clock table */
#define MET_SMI_CLK_TABLES \
{ \
	{"smi-common-FIFO0", NULL, 0}, \
	{"smi-common-FIFO1", NULL, 0}, \
	{"smi-common-UPSZ0", NULL, 0}, \
	{"smi-common-UPSZ1", NULL, 0}, \
	{"smi-common-GALS0", NULL, 0}, \
	{"smi-common-GALS1", NULL, 0}, \
	{"smi-common", NULL, 0}, \
	{"smi-common-2X", NULL, 0}, \
	{"smi-larb0", NULL, 0}, \
	{"smi-larb1", NULL, 0}, \
	{"smi-larb2", NULL, 0}, \
	{"smi-larb3", NULL, 0}, \
	{"smi-larb4-vdec", NULL, 0}, \
	{"smi-larb4-mm", NULL, 0}, \
	{"smi-larb5-img", NULL, 0}, \
	{"smi-larb5-mm", NULL, 0}, \
	{"smi-larb6-cam", NULL, 0}, \
	{"smi-larb6-mm", NULL, 0}, \
	{"smi-larb7-venc-0", NULL, 0}, \
	{"smi-larb7-venc-1", NULL, 0}, \
	{"smi-larb7-venc-2", NULL, 0}, \
	{"smi-larb7-mm", NULL, 0}, \
	{"smi-larb8-mjc-0", NULL, 0}, \
	{"smi-larb8-mjc-1", NULL, 0}, \
	{"smi-larb8-mm", NULL, 0}, \
	{"mtcmos-mm", NULL, 0}, \
	{"mtcmos-img", NULL, 0}, \
	{"mtcmos-cam", NULL, 0}, \
	{"mtcmos-vde", NULL, 0}, \
	{"mtcmos-ven", NULL, 0}, \
	{"mtcmos-mjc", NULL, 0}, \
}

#else
#define MET_SMI_CLK_TABLES
#endif



#endif	/* _SMI_NAME_H_ */
