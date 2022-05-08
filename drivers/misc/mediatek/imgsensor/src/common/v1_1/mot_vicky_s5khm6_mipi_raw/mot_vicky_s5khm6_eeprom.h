/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MOT_VICKY_S5KHM6_EEPROM_H__
#define __MOT_VICKY_S5KHM6_EEPROM_H__

#include "kd_camera_typedef.h"

/*
 * LRC
 *
 * @param data Buffer
 * @return size of data
 */
unsigned int read_mot_vicky_s5khm6_LRC(BYTE *data);

/*
 * DCC
 *
 * @param data Buffer
 * @return size of data
 */
unsigned int read_mot_vicky_s5khm6_DCC(BYTE *data);

#endif

