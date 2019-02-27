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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 */


/*
 * Filename:
 * ---------
 *  mtk-soc-speaker-amp.h
 *
 * Project:
 * --------
 *    Speaker amp co-load function
 *
 * Description:
 * ------------
 *   Every speaker amp register in theis file
 *
 * Author:
 * -------
 * Shane Chien
 *
 */

#ifndef _SPEAKER_AMP_H
#define _SPEAKER_AMP_H

struct amp_i2c_control {
	int (*i2c_probe)(struct i2c_client *,
			 const struct i2c_device_id *);
	int (*i2c_remove)(struct i2c_client *);
	void (*i2c_shutdown)(struct i2c_client *);
};

enum speaker_amp_type {
	NOT_SMARTPA = 0,
	RICHTEK_RT5509,
	AMP_TYPE_NUM
};

int get_amp_index(void);

#endif

