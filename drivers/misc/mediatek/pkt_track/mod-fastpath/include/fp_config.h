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

#ifndef _FP_CONFIG_H
#define _FP_CONFIG_H

#define FASTPATH_RX_RESERVE (32)
#define FASTPATH_MAJOR  (230)
#define FASTPATH_MINOR  (0)
#define FASTPATH_SKBLIST
#define FASTPATH_NO_KERNEL_SUPPORT
#define FASTPATH_DISABLE_TCP_WINDOW_CHECK
#define FASTPATH_NETFILTER

#define XFRM_SKB_CB_EXT_OFFSET 108
#define XFRM_SKB_CB_DST_OFFSET 104

#define MD_DIRECT_TETHERING_RULE_NUM 128	/* Must be power of 2 */

#endif
