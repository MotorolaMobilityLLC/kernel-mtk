#include <linux/module.h>
#include <linux/skbuff.h>
#include <net/dsfield.h>
#include <net/xfrm.h>
#include "fastpath.h"
#include "desc.h"
#include "protocol.h"

static inline void fp_ip6_icmp_lan(struct fp_desc *desc, struct sk_buff *skb, struct router_tuple *t,
				   struct interface *iface, void *l3_header, void *l4_header)
{
#ifndef FASTPATH_NETFILTER
	icmpheader *icmp = (icmpheader *) l4_header;

	/* check if the packet should be dropped due to icmp limitation */
	if (unlikely(_fp_ip46_icmp_limit(desc, skb, iface, icmp)))
		return;
#endif

	/* check if the packet should be handled by ipsec6 first */
	if (unlikely(fastpath_ipsec && _fp_is_ip6_ipsec_lan(desc, skb, l3_header, l4_header)))
		return;

	desc->flag |= DESC_FLAG_UNKNOWN_PROTOCOL;
}

static inline void fp_ip6_icmp_wan(struct fp_desc *desc, struct sk_buff *skb, struct router_tuple *t,
				   struct interface *iface, void *l3_header, void *l4_header)
{
#ifndef FASTPATH_NETFILTER
	icmpheader *icmp = (icmpheader *) l4_header;

	/* check if the packet should be dropped due to icmp limitation */
	if (unlikely(_fp_ip46_icmp_limit(desc, skb, iface, icmp)))
		return;
#endif

	desc->flag |= DESC_FLAG_UNKNOWN_PROTOCOL;
}
