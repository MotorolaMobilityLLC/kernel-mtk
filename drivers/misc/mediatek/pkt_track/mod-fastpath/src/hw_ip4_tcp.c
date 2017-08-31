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
#endif

static inline void _fp_ip4_tcp(struct fp_desc *desc, struct sk_buff *skb, struct tuple *t,
			       struct nat_tuple *found_nat_tuple, struct ip4header *ip, struct tcpheader *tcp)
{
	unsigned csum_l4;

	if (ip->ip_src != found_nat_tuple->nat_src_ip) {
		/* modify IP checksum */
		csum_l4 = _fp_ip4_snat_top(found_nat_tuple, ip);
	} else {
		csum_l4 = 0xffffffff;
	}
	/* modify TCP checksum */
	tcp->th_sum =
	    _fp_l4_csum(&csum_l4, &tcp->th_sport, &found_nat_tuple->nat_src.tcp.port, &tcp->th_sum);
	tcp->th_sport = found_nat_tuple->nat_src.tcp.port;

	if (ip->ip_dst != found_nat_tuple->nat_dst_ip) {
		/* modify IP checksum */
		csum_l4 = _fp_ip4_dnat_top(found_nat_tuple, ip);
	} else {
		csum_l4 = 0xffffffff;
	}
	/* modify TCP checksum */
	tcp->th_sum =
	    _fp_l4_csum(&csum_l4, &tcp->th_dport, &found_nat_tuple->nat_dst.tcp.port, &tcp->th_sum);
	tcp->th_dport = found_nat_tuple->nat_dst.tcp.port;

	if (unlikely(_fp_ip4_bottom(desc, skb, t, found_nat_tuple, ip)))
		return;

	if (_fp_ip4_tcp_ackagg(desc, skb, found_nat_tuple, ip, tcp))
		return;

#ifndef FASTPATH_NETFILTER
	_fp_xmit(desc, found_nat_tuple->iface_src, skb);
#else
	_fp_xmit(desc, found_nat_tuple->dev_src, skb);
#endif
}

#ifdef SUPPORT_IOC
static inline void fp_ip4_tcp_ioc(void)
{
}
#endif

static inline void fp_ip4_tcp(struct fp_desc *desc, struct sk_buff *skb, struct tuple *t, struct interface *iface,
			      void *l3_header, void *l4_header)
{
/* struct ip4header *ip = (struct ip4header *)l3_header; */
	struct tcpheader *tcp = (struct tcpheader *) l4_header;

#ifndef FASTPATH_NETFILTER
	/* check if the packet should be dropped due to tcp limitation */
	if (unlikely(_fp_ip46_tcp_limit(desc, skb, iface, tcp)))
		return;

#endif

	/* check if the packet should be handled by ipsec4 first */
/* if(unlikely(fastpath_ipsec && _fp_is_ip4_ipsec_lan(desc, skb, l3_header, l4_header))) return; */

	if (unlikely(tcp->th_flags & (TH_FIN | TH_SYN | TH_RST)))
		return;		/* do not handle TCP packet with FIN/SYN/RST flags */

	t->nat.s.tcp.port = tcp->th_sport;
	t->nat.d.tcp.port = tcp->th_dport;

	desc->flag |= DESC_FLAG_TRACK_NAT;
}
