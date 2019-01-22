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

#include <linux/module.h>
#include <linux/skbuff.h>
#include <net/dsfield.h>
#include <net/xfrm.h>
#include "fastpath.h"
#include "desc.h"
#include "protocol.h"

#ifndef FASTPATH_NETFILTER
static inline int _fp_ip46_tcp_limit(struct fp_desc *desc, struct sk_buff *skb, struct interface *iface,
				     struct tcpheader *tcp)
{
	if (unlikely(fastpath_limit_syn)) {
		if (tcp->th_flags & TH_SYN) {
			iface->syn_count++;
			if (iface->syn_count > fastpath_limit_syn) {
				desc->flag = DESC_FLAG_NOMEM;
				return 1;
			}
		}
	}
	return 0;
}

static inline int _fp_ip46_udp_limit(struct fp_desc *desc, struct sk_buff *skb, struct interface *iface)
{
	if (unlikely(fastpath_limit_udp)) {
		if (fastpath_limit_size == 0 || skb->len <= fastpath_limit_size) {
			iface->udp_count++;
			if (iface->udp_count > fastpath_limit_udp) {
				desc->flag = DESC_FLAG_NOMEM;
				return 1;
			}
		}
	}
	return 0;
}
#endif

static inline void fp_ip4_tcp(struct fp_desc *desc, struct sk_buff *skb, struct tuple *t, struct interface *iface,
			      void *l3_header, void *l4_header)
{
	struct tcpheader *tcp = (struct tcpheader *) l4_header;

#ifndef FASTPATH_NETFILTER
	/* check if the packet should be dropped due to tcp limitation */
	if (unlikely(_fp_ip46_tcp_limit(desc, skb, iface, tcp)))
		return;
#endif

	if (unlikely(tcp->th_flags & (TH_FIN | TH_SYN | TH_RST)))
		return;		/* do not handle TCP packet with FIN/SYN/RST flags */

	t->nat.s.tcp.port = tcp->th_sport;
	t->nat.d.tcp.port = tcp->th_dport;

	desc->flag |= DESC_FLAG_TRACK_NAT;
}

static inline void fp_ip4_udp(struct fp_desc *desc, struct sk_buff *skb, struct tuple *t, struct interface *iface,
			      void *l3_header, void *l4_header)
{
	struct udpheader *udp = (struct udpheader *) l4_header;

#ifndef FASTPATH_NETFILTER
	/* check if the packet should be dropped due to udp limitation */
	if (unlikely(_fp_ip46_udp_limit(desc, skb, iface)))
		return;
#endif

	t->nat.s.udp.port = udp->uh_sport;
	t->nat.d.udp.port = udp->uh_dport;

	desc->flag |= DESC_FLAG_TRACK_NAT;
}

static inline void fp_ip6_tcp_lan(struct fp_desc *desc, struct sk_buff *skb, struct router_tuple *t,
				  struct interface *iface, void *l3_header, void *l4_header)
{
	struct tcpheader *tcp = (struct tcpheader *) l4_header;

#ifndef FASTPATH_NETFILTER
	/* check if the packet should be dropped due to tcp limitation */
	if (unlikely(_fp_ip46_tcp_limit(desc, skb, iface, tcp)))
		return;
#endif

	if (unlikely(tcp->th_flags & (TH_FIN | TH_SYN | TH_RST)))
		return;		/* do not handle TCP packet with FIN/SYN/RST flags */

	t->in.tcp.port = tcp->th_sport;
	t->out.tcp.port = tcp->th_dport;

	desc->flag |= DESC_FLAG_TRACK_ROUTER;
}

static inline void fp_ip6_udp_lan(struct fp_desc *desc, struct sk_buff *skb, struct router_tuple *t,
				  struct interface *iface, void *l3_header, void *l4_header)
{
	struct udpheader *udp = (struct udpheader *) l4_header;

#ifndef FASTPATH_NETFILTER
	/* check if the packet should be dropped due to udp limitation */
	if (unlikely(_fp_ip46_udp_limit(desc, skb, iface)))
		return;
#endif

	t->in.udp.port = udp->uh_sport;
	t->out.udp.port = udp->uh_dport;

	desc->flag |= DESC_FLAG_TRACK_ROUTER;
}
