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

#include <linux/slab.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/netfilter/nf_conntrack_common.h>
#include <linux/netfilter/nf_conntrack_tcp.h>
#include <linux/netfilter/nf_conntrack_proto_gre.h>
#include <net/netfilter/nf_conntrack_extend.h>
#include <linux/netfilter.h>
#include <net/netfilter/nf_conntrack_core.h>
#include <linux/if_vlan.h>
#include "fp_config.h"
#include "tuple.h"
#include "pkt_track_struct.h"

#ifdef PW_RESET
void pm_reset_traffic(void);
#else
#define pm_reset_traffic()
#endif

#ifdef SUPPORT_IPSEC
static inline int ipv6_find_ah_hdr(const struct sk_buff *skb)
{
	unsigned int start = skb_network_offset(skb) + sizeof(struct ipv6hdr);
	u8 nexthdr = ipv6_hdr(skb)->nexthdr;

	while (nexthdr != NEXTHDR_AUTH) {
		struct ipv6_opt_hdr _hdr, *hp;
		unsigned int hdrlen;

		if ((!ipv6_ext_hdr(nexthdr)) || nexthdr == NEXTHDR_NONE)
			return 0;

		hp = skb_header_pointer(skb, start, sizeof(_hdr), &_hdr);
		if (hp == NULL)
			return 0;

		if (nexthdr == NEXTHDR_FRAGMENT) {
			__be16 _frag_off, *fp;

			fp = skb_header_pointer(skb, start + offsetof(struct frag_hdr, frag_off),
						sizeof(_frag_off), &_frag_off);
			if (fp == NULL)
				return 0;

			hdrlen = 8;
		} else if (nexthdr == NEXTHDR_AUTH) {
			/* impossible to reach here */
			WARN_ON(1);
		} else
			hdrlen = ipv6_optlen(hp);

		nexthdr = hp->nexthdr;
		start += hdrlen;
	}

	return start;
}
#endif

static inline void ipv6_addr_copy(struct in6_addr *a1, const struct in6_addr *a2)
{
	memcpy(a1, a2, sizeof(struct in6_addr));
}

#ifdef FASTPATH_NO_KERNEL_SUPPORT
void dummy_destructor_track_table(struct sk_buff *skb);
#endif
static inline void fastpath_dev_queue_xmit(struct sk_buff *skb_queue, unsigned long flag)
{
#ifdef CONFIG_PREEMPT_RT_FULL
	unsigned long flags;
#endif
#ifdef FASTPATH_NO_KERNEL_SUPPORT
	if (skb_queue->destructor) {
		skb_queue->destructor(skb_queue);
		WARN_ON(1);
	}
	skb_queue->destructor = dummy_destructor_track_table;
#endif
#ifdef CONFIG_PREEMPT_RT_FULL
	local_irq_save(flags);
	local_bh_disable();
	dev_queue_xmit(skb_queue);
	local_irq_restore(flags);
	_local_bh_enable();
#else
	dev_queue_xmit(skb_queue);
#endif
	(void)flag;
}

extern spinlock_t fp_lock;
#define FP_INIT_LOCK(LOCK)		    spin_lock_init((LOCK))
#define FP_LOCK(LOCK, FLAG)		    spin_lock_irqsave((LOCK), (FLAG))
#define FP_UNLOCK(LOCK, FLAG)		spin_unlock_irqrestore((LOCK), (FLAG))

#ifdef FASTPATH_NO_KERNEL_SUPPORT
#ifdef CONFIG_PREEMPT_RT_FULL
#define TRACK_TABLE_INIT_LOCK(TABLE)	        raw_spin_lock_init((&(TABLE).lock))
#define TRACK_TABLE_LOCK(TABLE, flags)	        raw_spin_lock_irqsave((&(TABLE).lock), (flags))
#define TRACK_TABLE_UNLOCK(TABLE, flags)	    raw_spin_unlock_irqrestore((&(TABLE).lock), (flags))
#else
#define TRACK_TABLE_INIT_LOCK(TABLE)	        spin_lock_init((&(TABLE).lock))
#define TRACK_TABLE_LOCK(TABLE, flags)	        spin_lock_irqsave((&(TABLE).lock), (flags))
#define TRACK_TABLE_UNLOCK(TABLE, flags)	    spin_unlock_irqrestore((&(TABLE).lock), (flags))
#endif
#endif

#define INTERFACE_TYPE_LAN	0
#define INTERFACE_TYPE_WAN	1
#define INTERFACE_TYPE_IOC	2

#define MDT_TAG_PATTERN     0x46464646

/* Timing define */
#define TRACK_TABLE_TIMEOUT_JIFFIES 100

struct interface {
	int type;
	int ready;
	struct net_device *dev;
	unsigned int global_count;
	unsigned int syn_count;
	unsigned int udp_count;
	unsigned int icmp_count;
	unsigned int icmp_unreach_count;
	unsigned int is_bridge;
#ifdef ENABLE_PORTBASE_QOS
	char dscp_mark;
#endif
};

extern struct interface ifaces[];

struct fp_cb {
	u_int32_t flag;
	u_int32_t v6src[3];
	u_int32_t src;		/* v6src[4] */
	u_int32_t v6dst[3];
	u_int32_t dst;		/* v6dst[4] */
	u_int16_t sport;
	u_int16_t dport;
	u_int8_t proto;
#ifndef FASTPATH_NETFILTER
	u_int8_t iface;
#else
	struct net_device *dev;
#endif
	u_int8_t vlevel;
#ifndef FASTPATH_NETFILTER
	u_int8_t rfu1[1];
#endif
	u_int16_t vlan[MAX_VLAN_LEVEL];
	u_int8_t mac[6];
#ifndef FASTPATH_NETFILTER
	u_int8_t rfu2[2];
#endif
	u_int32_t xfrm_dst_ptr;
	u_int32_t ipmac_ptr;
	u_int8_t index;
};

#ifdef FASTPATH_NO_KERNEL_SUPPORT
#define MAX_TRACK_NUM 32
struct fp_track_table_t {
	struct fp_cb cb;
	unsigned int valid;
	unsigned int ref_count;
	void *tracked_address;
	unsigned int timestamp;
	unsigned long jiffies;
};
struct fp_track_t {
	struct fp_track_table_t *table;
	unsigned int g_timestamp;
#ifdef CONFIG_PREEMPT_RT_FULL
	raw_spinlock_t lock;
#else
	spinlock_t lock;
#endif
	unsigned int tracked_number;
};
void del_all_track_table(void);
#endif

struct fp_wait_queue_t {
	struct list_head list_v4;
	struct list_head list_v6;
	unsigned int count_v4;
	unsigned int count_v6;
#ifdef CONFIG_PREEMPT_RT_FULL
	raw_spinlock_t lock;
#else
	spinlock_t lock;
#endif
};

struct fp_tag_info_t {
	unsigned char	in_netif_id;
	unsigned char	out_netif_id;
	unsigned short	port;
};

struct fp_tag_packet_t {
	unsigned int			guard_pattern;
	struct fp_tag_info_t	info;
};

#ifdef FASTPATH_NO_KERNEL_SUPPORT
extern struct fp_track_t fp_track;
#endif

int fastpath_brctl(int bridge, char *dev_name);

#endif
