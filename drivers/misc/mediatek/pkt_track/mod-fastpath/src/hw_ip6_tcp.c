#include <linux/module.h>
#include <linux/skbuff.h>
#include <net/dsfield.h>
#include <net/xfrm.h>
#include "fastpath.h"
#include "desc.h"
#include "protocol.h"

static inline void fp_ip6_tcp_lan(struct fp_desc *desc, struct sk_buff *skb, struct router_tuple *t,
				  struct interface *iface, void *l3_header, void *l4_header)
{
	struct tcpheader *tcp = (struct tcpheader *) l4_header;

#ifndef FASTPATH_NETFILTER
	/* check if the packet should be dropped due to tcp limitation */
	if (unlikely(_fp_ip46_tcp_limit(desc, skb, iface, tcp)))
		return;
#endif

	/* check if the packet should be handled by ipsec6 first */
	if (unlikely(fastpath_ipsec && _fp_is_ip6_ipsec_lan(desc, skb, l3_header, l4_header)))
		return;

	t->in.tcp.port = tcp->th_sport;
	t->out.tcp.port = tcp->th_dport;

	desc->flag |= DESC_FLAG_TRACK_ROUTER;
}

/* static inline void fp_ip6_tcp_wan( */
/*	struct fp_desc *desc, */
/*	struct sk_buff *skb, */
/*	struct router_tuple *t, */
/*	struct interface *iface, */
/*	void *l3_header, */
/*	void *l4_header) */
/* { */
/* struct router_tuple *found_router_tuple; */
/* struct tcpheader *tcp = (struct tcpheader *)l4_header; */
/*  */
/* #ifndef FASTPATH_NETFILTER */
/* //check if the packet should be dropped due to tcp limitation */
/* if(unlikely(_fp_ip46_tcp_limit(desc, skb, iface, tcp))) return; */
/* #endif */
/*  */
/* t->in.tcp.port  = tcp->th_sport; */
/* t->out.tcp.port = tcp->th_dport; */
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
