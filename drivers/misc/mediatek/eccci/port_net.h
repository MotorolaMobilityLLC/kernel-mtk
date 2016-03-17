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
 */

#ifdef CONFIG_MTK_NET_CCMNI
#define CCMNI_U
#include "ccmni.h"
extern struct ccmni_dev_ops ccmni_ops;
#endif

#define ENABLE_GRO

extern int mbim_start_xmit(struct sk_buff *skb, int ifid);

