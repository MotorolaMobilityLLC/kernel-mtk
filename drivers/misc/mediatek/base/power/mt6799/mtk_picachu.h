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

#ifndef __MTK_PICACHU_H__
#define __MTK_PICACHU_H__

#include <linux/kernel.h>

enum mt_picachu_domain_id {
	MT_PICACHU_DOMAIN1, /* LL Only */
	MT_PICACHU_DOMAIN2, /* LL + B */
	MT_PICACHU_DOMAIN3, /* L */

	NR_PICACHU_DOMAINS,
};

extern bool picachu_need_higher_volt(enum mt_picachu_domain_id domain_id);

#endif /* __MTK_PICACHU_H__ */
