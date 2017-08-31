#include <linux/module.h>
#include <linux/skbuff.h>
#include <net/dsfield.h>
#include <net/xfrm.h>
#include "fastpath.h"
#include "desc.h"
#include "protocol.h"

#ifndef FASTPATH_NETFILTER
static inline int _fp_ip46_icmp_limit(struct fp_desc *desc, struct sk_buff *skb, struct interface *iface,
				      icmpheader *icmp)
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
	if (unlikely(fastpath_limit_icmp)) {
		iface->icmp_count++;
		if (iface->icmp_count > fastpath_limit_icmp) {
			desc->flag = DESC_FLAG_NOMEM;
			return 1;
		}
	}
	if (unlikely(fastpath_limit_icmp_unreach)) {
		if (icmp->icmp_type == 3 && icmp->icmp_code == 3) {	/* ICMP Unreachable */
			iface->icmp_unreach_count++;
			if (iface->icmp_unreach_count > fastpath_limit_icmp_unreach) {
				desc->flag = DESC_FLAG_NOMEM;
				return 1;
			}
		}
	}

	return 0;
}
#endif

static inline void fp_ip4_icmp_lan(struct fp_desc *desc, struct sk_buff *skb, struct tuple *t,
				   struct interface *iface, void *l3_header, void *l4_header)
{
#ifndef FASTPATH_NETFILTER
	icmpheader *icmp = (icmpheader *) l4_header;

	/* check if the packet should be dropped due to icmp limitation */
	if (unlikely(_fp_ip46_icmp_limit(desc, skb, iface, icmp)))
		return;
#endif

	/* check if the packet should be handled by ipsec4 first */
/* if(unlikely(fastpath_ipsec && _fp_is_ip4_ipsec_lan(desc, skb, l3_header, l4_header))) return; */

	desc->flag |= DESC_FLAG_UNKNOWN_PROTOCOL;
}

static inline void fp_ip4_icmp_wan(struct fp_desc *desc, struct sk_buff *skb, struct tuple *t,
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
