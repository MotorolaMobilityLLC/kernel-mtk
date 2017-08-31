/*
 * Copyright (C) 2017 MediaTek Inc.
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

#ifndef __MTK_MCDI_MBOX_H__
#define __MTK_MCDI_MBOX_H__

#define MCDI_MBOX                               3
#define MCDI_MBOX_SLOT_OFFSET_START             0
#define MCDI_MBOX_SLOT_OFFSET_END               15

/* MBOX: AP write, SSPM read */
#define MCDI_MBOX_CLUSTER_0_CAN_POWER_OFF       0
#define MCDI_MBOX_CLUSTER_1_CAN_POWER_OFF       1
#define MCDI_MBOX_CLUSTER_2_CAN_POWER_OFF       2
#define MCDI_MBOX_CLUSTER_0_ATF_ACTION_DONE     3
#define MCDI_MBOX_CLUSTER_1_ATF_ACTION_DONE     4
#define MCDI_MBOX_CLUSTER_2_ATF_ACTION_DONE     5
#define MCDI_MBOX_PAUSE_ACTION                  6
/* MBOX: AP read, SSPM write */
#define MCDI_MBOX_CPU_CLUSTER_PWR_STAT          7
#define MCDI_MBOX_ACTION_STAT                   8

/* MCDI_MBOX_ACTION_STAT */
#define MCDI_ACTION_INIT                        0
#define MCDI_ACTION_WAIT_EVENT                  1
#define MCDI_ACTION_PAUSED                      2
#define MCDI_ACTION_WORKING                     3

#endif /* __MTK_MCDI_MBOX_H__ */
