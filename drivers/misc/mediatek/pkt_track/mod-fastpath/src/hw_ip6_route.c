#include <linux/module.h>
#include <linux/skbuff.h>
#include <net/dsfield.h>
#include <net/xfrm.h>
#include "fastpath.h"
#include "desc.h"
#include "protocol.h"

static inline void fp_ip6_route_lan(struct fp_desc *desc, struct sk_buff *skb,
				    struct router_tuple *found_router_tuple, struct ip6header *ip,
				    void *l4_header)
{
#if 0
	skb->dev = ifaces[found_router_tuple->iface_dst].dev;

	if (skb->dev->header_ops) {
		ether_addr_copy(skb->data, found_router_tuple->dmac);
		memcpy(skb->data + ETH_ALEN, found_router_tuple->smac, ETH_ALEN);
	} else {
		skb_pull_inline(skb, ETH_HLEN);
	}
#else
#ifndef FASTPATH_NETFILTER
	if (unlikely
	    (_fp_ip6_pre_send_skb
	     (desc, skb, found_router_tuple, found_router_tuple->vlevel_dst,
	      found_router_tuple->vlan_dst, found_router_tuple->dmac, found_router_tuple->smac,
	      ifaces[found_router_tuple->iface_dst].dev))) {
#else
	if (unlikely
	    (_fp_ip6_pre_send_skb
	     (desc, skb, found_router_tuple, found_router_tuple->vlevel_dst,
	      found_router_tuple->vlan_dst, found_router_tuple->dmac, found_router_tuple->smac,
	      found_router_tuple->dev_dst))) {
#endif
		return;
	}
#endif

	if (found_router_tuple->proto == IPPROTO_TCP
	    && _fp_ip6_tcp_ackagg(desc, skb, found_router_tuple, ip, (struct tcpheader *) l4_header)) {
		return;
	}
#ifndef FASTPATH_NETFILTER
	_fp_xmit(desc, found_router_tuple->iface_src, skb);
#else
	_fp_xmit(desc, found_router_tuple->dev_src, skb);
#endif
}

static inline void fp_ip6_route(struct fp_desc *desc, struct sk_buff *skb,
				struct router_tuple *found_router_tuple)
{
#if 0
	skb->dev = ifaces[found_router_tuple->iface_dst].dev;

	if (skb->dev->header_ops) {
		ether_addr_copy(skb->data, found_router_tuple->dmac);
		memcpy(skb->data + ETH_ALEN, found_router_tuple->smac, ETH_ALEN);
	} else {
		skb_pull_inline(skb, ETH_HLEN);
	}
#else
#ifndef FASTPATH_NETFILTER
	if (unlikely
	    (_fp_ip6_pre_send_skb
	     (desc, skb, found_router_tuple, found_router_tuple->vlevel_dst,
	      found_router_tuple->vlan_dst, found_router_tuple->dmac, found_router_tuple->smac,
	      ifaces[found_router_tuple->iface_dst].dev))) {
#else
	if (unlikely
	    (_fp_ip6_pre_send_skb
	     (desc, skb, found_router_tuple, found_router_tuple->vlevel_dst,
	      found_router_tuple->vlan_dst, found_router_tuple->dmac, found_router_tuple->smac,
	      found_router_tuple->dev_dst))) {
#endif
		return;
	}
#endif

#ifndef FASTPATH_NETFILTER
	_fp_xmit(desc, found_router_tuple->iface_src, skb);
#else
	_fp_xmit(desc, found_router_tuple->dev_src, skb);
#endif
}
