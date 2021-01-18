/* SPDX-License-Identifier: GPL-2.0 */
/*  Himax Android Driver Sample Code for HX83112 chipset
 *
 *  Copyright (C) 2019 Himax Corporation.
 *
 *  This software is licensed under the terms of the GNU General Public
 *  License version 2,  as published by the Free Software Foundation,  and
 *  may be copied,  distributed,  and modified under those terms.
 *
 *  This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */

#include "himax_platform.h"
#include "himax_common.h"
#include "himax_ic_core.h"
#include <linux/slab.h>

#define hx83112a_data_adc_num 216
#define hx83112b_data_adc_num 216
#define hx83112d_data_adc_num 64
#define hx83112e_data_adc_num 64
#define hx83112f_data_adc_num 48
#define hx83112_notouch_frame            0

#define hx83112f_fw_addr_raw_out_sel     0x100072ec
#define hx83112f_notouch_frame            0
