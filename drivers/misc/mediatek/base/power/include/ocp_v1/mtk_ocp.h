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

#ifndef __MTK_OCP_H__
#define __MTK_OCP_H__

#include <linux/kernel.h>


/* turn on/off OCP driver */
#define OCP_FEATURE_ENABLED	(0)

enum ocp_cluster {
	OCP_LL = 0,
	OCP_L,
	OCP_B,

	NR_OCP_CLUSTER,
};

/* OCP APIs */
extern int mt_ocp_set_target(enum ocp_cluster cluster, unsigned int target);
extern unsigned int mt_ocp_get_avgpwr(enum ocp_cluster cluster);
extern bool mt_ocp_get_status(enum ocp_cluster cluster);
extern unsigned int mt_ocp_get_mcusys_pwr(void);

#endif /* __MTK_OCP_H__ */

