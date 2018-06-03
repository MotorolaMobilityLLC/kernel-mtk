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

#ifndef _TUPLE_H
#define _TUPLE_H

/* #include <asm/atomic.h> */
#include <linux/atomic.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <net/ip.h>
#include <net/ipv6.h>
#include "fp_config.h"

#define MAX_VLAN_LEVEL	2

struct tuple {
	union {
		struct {
#ifdef SUPPORT_PPPOE
			u_int8_t pppoe_out;
			u_int16_t pppoe_out_id;
#endif
			u_int32_t src;	/*src */
			u_int32_t dst;	/*dst */
			union {
				u_int16_t all;
				struct {
					u_int16_t port;
				} tcp;
				struct {
					u_int16_t port;
				} udp;
				struct {
					u_int16_t callid;
				} gre;
			} s;
			union {
				u_int16_t all;
				struct {
					u_int16_t port;
				} tcp;
				struct {
					u_int16_t port;
				} udp;
				struct {
					u_int16_t callid;
				} gre;
			} d;
			u_int8_t proto;
		} nat;
		struct {
			union {
				struct {
					u_int8_t dst_unused[2];
					u_int8_t dst_mac[6];
				} __attribute__ ((__packed__));
				struct {
					u_int16_t dst_unused2;
					u_int16_t dst_mac1;
					u_int32_t dst_mac2;
				};
			};
			u_int8_t vlevel_src;
			u_int16_t vlan_src[MAX_VLAN_LEVEL];
		} bridge;
	};
#ifndef FASTPATH_NETFILTER
	u_int8_t iface_in;
#else
	struct net_device *dev_in;
#endif
	u_int32_t action;
};

struct router_tuple {
	struct list_head list;
	union {
		struct {
			u_int8_t dst_unused[2];
			u_int8_t smac[6];
		} __attribute__ ((__packed__));
	};
	union {
		struct {
			u_int8_t dst_unused3[2];
			u_int8_t dmac[6];
		} __attribute__ ((__packed__));
	};

	struct in6_addr saddr;
	struct in6_addr daddr;

#ifndef FASTPATH_NETFILTER
	u_int8_t iface_src;
	u_int8_t iface_dst;
#else
	struct net_device *dev_src;
	struct net_device *dev_dst;
#endif

	u_int8_t vlevel_src;
	u_int8_t vlevel_dst;
	u_int16_t vlan_src[MAX_VLAN_LEVEL];
	u_int16_t vlan_dst[MAX_VLAN_LEVEL];

	struct timer_list timeout_used;
	struct timer_list timeout_unused;

	/* for ack aggregation */
	struct timer_list timeout_ack;
	uint32_t ul_ack;
	struct sk_buff *ul_ack_skb;
	uint32_t ack_agg_enable;
	spinlock_t ack_lock;
	union {
		u_int16_t all;
		struct {
			u_int16_t port;
		} tcp;
		struct {
			u_int16_t port;
		} udp;
	} in;
	union {
		u_int16_t all;
		struct {
			u_int16_t port;
		} tcp;
		struct {
			u_int16_t port;
		} udp;
	} out;
	u_int8_t proto;

	atomic_t ref_count;
	unsigned long last;
};

#endif
