/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MOT_DUBAI_HI1336_EEPROM_H__
#define __MOT_DUBAI_HI1336_EEPROM_H__

#include "kd_camera_typedef.h"

#include "adaptor-subdrv.h"

/*
 * LRC
 *
 * @param data Buffer
 * @return size of data
 */
unsigned int read_mot_dubai_hi1336_LRC(struct subdrv_ctx *ctx, u8 *data);

/*
 * DCC
 *
 * @param data Buffer
 * @return size of data
 */
unsigned int read_mot_dubai_hi1336_DCC(struct subdrv_ctx *ctx, u8 *data);

#endif

