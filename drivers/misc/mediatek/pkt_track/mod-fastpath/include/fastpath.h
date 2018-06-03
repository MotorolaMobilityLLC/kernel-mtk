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

#ifndef _FASTPATH_H
#define _FASTPATH_H

#include <linux/netdevice.h>

bool fastpath_is_support_dev(const char *dev_name);

int fastpath_in_nf(int iface, struct sk_buff *skb);
void fastpath_out_nf(int iface, struct sk_buff *skb, const struct net_device *out);

#endif
