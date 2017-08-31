#include <linux/module.h>
#include <linux/skbuff.h>
#include <net/dsfield.h>
#include <net/xfrm.h>
#include "fastpath.h"
#include "desc.h"
#include "protocol.h"

static inline void fp_ip6_udp_lan(struct fp_desc *desc, struct sk_buff *skb, struct router_tuple *t,
				  struct interface *iface, void *l3_header, void *l4_header)
{
	struct udpheader *udp = (struct udpheader *) l4_header;

#ifndef FASTPATH_NETFILTER
	/* check if the packet should be dropped due to udp limitation */
	if (unlikely(_fp_ip46_udp_limit(desc, skb, iface)))
		return;
#endif

	/* check if the packet should be handled by ipsec6 first */
	if (unlikely(fastpath_ipsec && _fp_is_ip6_ipsec_lan(desc, skb, l3_header, l4_header)))
		return;

	t->in.udp.port = udp->uh_sport;
	t->out.udp.port = udp->uh_dport;

	desc->flag |= DESC_FLAG_TRACK_ROUTER;
}

/* static inline void fp_ip6_udp_wan( */
/*	struct fp_desc *desc, */
/*	struct sk_buff *skb, */
/*	struct router_tuple *t, */
/*	struct interface *iface, */
/*	void *l3_header, */
/*	void *l4_header) */
/* { */
/* router_tuple *found_router_tuple; */
/* struct udpheader *udp = (struct udpheader *)l4_header; */
/*  */
/* #ifndef FASTPATH_NETFILTER */
/* //check if the packet should be dropped due to udp limitation */
/* if(unlikely(_fp_ip46_udp_limit(desc, skb, iface))) return; */
/* #endif */
/*  */
/* t->in.udp.port  = udp->uh_sport; */
/* t->out.udp.port = udp->uh_dport; */
/*  */
/* found_router_tuple = get_router_tuple_tcpudp(t); */
/* if(likely(found_router_tuple)){ */
/* fp_ip6_route(desc, skb, found_router_tuple); */
/* DEC_REF_ROUTER_TUPLE(found_router_tuple); */
/* return; */
/* }else{//not found route */
/* desc->flag |= DESC_FLAG_TRACK_ROUTER; */
/* return; */
/* } */
/* } */
